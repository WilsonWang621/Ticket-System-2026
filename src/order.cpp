//
// Created by lenovo on 2026/5/29.
//
#include <../include/service/order.h>
#include <climits>

namespace sjtu {

    OrderService::OrderService():initialized_(false), user_service_(nullptr), train_service_(nullptr), user_order_index_(nullptr), pending_order_index_(nullptr){ }

    OrderService::~OrderService() {
         delete user_order_index_;
         delete pending_order_index_;
         user_order_index_ = nullptr;
         pending_order_index_ = nullptr;
    }

    bool OrderService::init(const std::string& data_dir, UserService* user_service, TrainService* train_service) {
         data_dir_ = data_dir;
         initialized_ = order_file_.open("ts_order_dat");
         user_service_ = user_service;
         train_service_ = train_service;
         user_order_index_ = new BPT<Data>("ts_order_index");
         pending_order_index_ = new BPT<Data>("ts_pending_index");
         return initialized_;
     }

    void OrderService::clear() {
         if (!initialized_) return;
         order_file_.close();
         delete user_order_index_;
         delete pending_order_index_;
         user_order_index_ = nullptr;
         pending_order_index_ = nullptr;
    }

    void OrderService::reset_index_files() {
        delete user_order_index_;
        delete pending_order_index_;
        user_order_index_ = nullptr;
        pending_order_index_ = nullptr;
        std::remove("init_ts_user_order_index");
        std::remove("data_ts_user_order_index");
        std::remove("init_ts_pending_order_index");
        std::remove("data_ts_pending_order_index");
        user_order_index_ = new BPT<Data>("ts_user_order_index");
        pending_order_index_ = new BPT<Data>("ts_pending_order_index");
    }

    bool OrderService::append_order_index(const std::string& username, int order_offset) {
        if (user_order_index_ == nullptr) {
            return false;
        }
        user_order_index_->add(Data(username, order_offset));
        return true;
    }

    std::string OrderService::make_pending_key(std::string trainID, int running_date) {
        return trainID + "#" + std::to_string(running_date);
    }

    void OrderService::build_order_view(const OrderRecord& order, const TrainRecord& train, OrderView& view) {
        const long long base_time = to_absolute_minutes(order.running_date, train.start_time_minutes);
        view.status = order.status;
        copy_to_buffer(order.train_id, view.train_id, sizeof(view.train_id));
        copy_to_buffer(train.stations[order.from_index], view.from, sizeof(view.from));
        copy_to_buffer(train.stations[order.to_index], view.to, sizeof(view.to));
        view.leaving = absolute_minutes_to_datetime(base_time + train.departure_offsets[order.from_index]);
        view.arriving = absolute_minutes_to_datetime(base_time + train.arrival_offsets[order.to_index]);
        view.price = train.prefix_prices[order.to_index] - train.prefix_prices[order.from_index];
        view.ticket_num = order.ticket_num;
    }

    bool OrderService::get_order(int order_offset, sjtu::OrderRecord &order) const {
        return order_file_.read(order_offset, order);
    }

    BuyTicketResult OrderService::buy_ticket(const BuyTicketQuery& request) {
        BuyTicketResult result;
        result.state = BuyTicketState::Failed;
        if (!user_service_->is_logged_in(from_buffer(request.username))) {
            return result;
        }
        TrainRecord train;
        int train_offset = -1;
        if (train_service_->get_train(request.trainID, train, train_offset)) {
            return result;
        }
        if (!train.released) {
            return result;
        }
        int from_index = -1, to_index = -1;
        if (TrainService::locate_station(train, from_buffer(request.from), from_index) || TrainService::locate_station(train, from_buffer(request.to), to_index) || from_index >= to_index) {
            return result;
        }

        if (request.ticketNum > train.seatNum) return result;

        const int running_date = sjtu::TrainService::resolve_running_date(train, from_index, request.departure_date);
        if (running_date == -1) return result;

        SeatRecord seat_record;
        int seat_offset = -1;
        if (!train_service_->load_or_create_seat_record(train_offset, running_date, train, seat_record, seat_offset)) {
            return result;
        }

        const int min_seat = train_service_->query_min_remaining_seat(seat_record, from_index, to_index);
        const int unit_price = train.prefix_prices[to_index] - train.prefix_prices[from_index];

        OrderRecord order{};
        order.create_timestamp = order_file_.record_count();
        order.user_offset = -1;
        order.train_offset = train_offset;
        order.seat_offset = seat_offset;
        order.running_date = running_date;
        order.from_index = from_index;
        order.to_index = to_index;
        order.ticket_num = request.ticketNum;
        order.total_price = unit_price * request.ticketNum;
        copy_to_buffer(request.username, order.username, sizeof(order.username));
        copy_to_buffer(request.trainID, order.train_id, sizeof(order.train_id));

        if (min_seat >= request.ticketNum) {
            train_service_->apply_seat_delta(seat_record, from_index, to_index, -request.ticketNum);
            if (!train_service_->write_seat_record(seat_offset, seat_record)) {
                return result;
            }
            order.status = sjtu::OrderState::kSuccess;
            const int order_offset = order_file_.append(order);
            if (order_offset == -1) {
                return result;
            }
            order.order_id = order_offset;
            order_file_.write(order_offset, order);
            append_order_index(from_buffer(order.username), order_offset);
            result.state = BuyTicketState::Success;
            result.total_price = order.total_price;
            result.order_offset = order_offset;
            return result;
        }

        if (!request.allow_queue) return result;

        order.status = sjtu::OrderState::kPending;
        const int order_offset = order_file_.append(order);
        if (order_offset == -1) {
            return result;
        }
        order.order_id = order_offset;
        order_file_.write(order_offset, order);
        append_order_index(from_buffer(order.username), order_offset);
        pending_order_index_->add(Data(make_pending_key(from_buffer(order.train_id), running_date), order_offset));
        result.state = sjtu::BuyTicketState::Pending;
        result.order_offset = order_offset;
        return result;
    }

    bool OrderService::query_order(const std::string& username, std::vector<sjtu::OrderView>& orders) {
        orders.clear();
        if (user_service_ == nullptr || train_service_ == nullptr || !user_service_->is_logged_in(username)) {
            return false;
        }

        std::vector<Data> order_entries;
        user_order_index_->range_query(Data(username, INT_MIN), Data(username, INT_MAX), order_entries);
        for (int index = static_cast<int>(order_entries.size()) - 1; index >= 0; --index) {
            OrderRecord order{};
            if (!order_file_.read(order_entries[index].value, order)) {
                continue;
            }
            TrainRecord train{};
            if (!train_service_->get_train_by_offset(order.train_offset, train)) {
                continue;
            }
            OrderView view{};
            build_order_view(order, train, view);
            orders.push_back(view);
        }
        return true;
    }

    bool OrderService::refund_ticket(std::string& username, int n) {
        if (user_service_ == nullptr || train_service_ == nullptr || !user_service_->is_logged_in(username)) {
            return false;
        }

        std::vector<Data> order_entries;
        user_order_index_->range_query(Data(username, INT_MIN), Data(username, INT_MAX), order_entries);
        if (n <= 0 || n > static_cast<int>(order_entries.size())) {
            return false;
        }

        const int target_offset = order_entries[order_entries.size() - n].value;
        OrderRecord order{};
        if (!order_file_.read(target_offset, order)) {
            return false;
        }
        if (order.status == sjtu::OrderState::kRefunded) {
            return false;
        }

        if (order.status == sjtu::OrderState::kPending) {
            order.status = sjtu::OrderState::kRefunded;
            if (!order_file_.write(target_offset, order)) {
                return false;
            }
            pending_order_index_->remove(Data(make_pending_key(from_buffer(order.train_id), order.running_date), target_offset));
            return true;
        }

        sjtu::SeatRecord seat_record{};
        if (!train_service_->read_seat_record(order.seat_offset, seat_record)) {
            return false;
        }
        train_service_->apply_seat_delta(seat_record, order.from_index, order.to_index, order.ticket_num);
        if (!train_service_->write_seat_record(order.seat_offset, seat_record)) {
            return false;
        }

        order.status = sjtu::OrderState::kRefunded;
        if (!order_file_.write(target_offset, order)) {
            return false;
        }

        try_promote_pending_orders(from_buffer(order.train_id), order.running_date);
        return true;
    }

    void OrderService::try_promote_pending_orders(const std::string& train_id, int running_date) {
        if (pending_order_index_ == nullptr || train_service_ == nullptr) {
            return;
        }

        std::vector<Data> pending_orders;
        pending_order_index_->range_query(Data(make_pending_key(train_id, running_date), INT_MIN),
                                          Data(make_pending_key(train_id, running_date), INT_MAX),
                                          pending_orders);
        if (pending_orders.empty()) {
            return;
        }

        TrainRecord train{};
        int train_offset = -1;
        if (!train_service_->get_train(train_id, train, train_offset)) {
            return;
        }

        sjtu::SeatRecord seat_record{};
        int seat_offset = -1;
        if (!train_service_->load_or_create_seat_record(train_offset, running_date, train, seat_record, seat_offset)) {
            return;
        }

        for (int index = 0; index < static_cast<int>(pending_orders.size()); ++index) {
            const int order_offset = pending_orders[index].value;
            sjtu::OrderRecord order{};
            if (!order_file_.read(order_offset, order)) {
                continue;
            }
            if (order.status != sjtu::OrderState::kPending) {
                continue;
            }
            const int remain = train_service_->query_min_remaining_seat(seat_record, order.from_index, order.to_index);
            if (remain < order.ticket_num) {
                continue;
            }

            train_service_->apply_seat_delta(seat_record, order.from_index, order.to_index, -order.ticket_num);
            order.status = OrderState::kSuccess;
            order_file_.write(order_offset, order);
            pending_order_index_->remove(Data(make_pending_key(train_id, running_date), order_offset));
        }

        train_service_->write_seat_record(seat_offset, seat_record);
    }


}
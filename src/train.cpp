//
// Created by lenovo on 2026/5/29.
//
#include <../include/service/train.h>
#include <../include/util/internal_utils.h>
#include <map>
#include<climits>
#include<algorithm>

namespace sjtu {
    unsigned long long hash_key(const std::string &text) {
        Data data(text, 0);
        return data.key;
    }

    void fill_ticket_result(const TrainRecord &train, int from_index, int to_index, int running_date, int seat, TicketQueryResult &result) {
        const long long base_time =to_absolute_minutes(running_date, train.start_time_minutes);
        copy_to_buffer(train.trainID,  result.train_id, sizeof(result.train_id));
        copy_to_buffer(train.stations[from_index], result.from, sizeof(result.from));
        copy_to_buffer(train.stations[to_index], result.to, sizeof(result.to));
        result.leaving = absolute_minutes_to_datetime(base_time + train.departure_offsets[from_index]);
        result.arriving = absolute_minutes_to_datetime(base_time + train.arrival_offsets[to_index]);
        result.price = train.prefix_prices[to_index] - train.prefix_prices[from_index];
        result.seat = seat;
        result.total_time = static_cast<int>(to_absolute_minutes(result.arriving) - to_absolute_minutes(result.leaving));
    }

    bool my_strcmp(const char* str1, const char* str2) {
        if (str1 == nullptr || str2 == nullptr) {
            if (str1 == str2) return 0;
            return (str1 == nullptr) ? -1 : 1;
        }

        while (*str1 != '\0' && *str2 != '\0' && *str1 == *str2) {
            str1++;
            str2++;
        }

        return (*str1 - *str2);
    }

    auto compare_by_cost =[](const TicketQueryResult &lhs, const TicketQueryResult &rhs){
        if (lhs.price != rhs.price) {
            return lhs.price < rhs.price;
        }
        return my_strcmp(lhs.train_id, rhs.train_id);
    };

    auto compare_by_time =[](const TicketQueryResult &lhs, const TicketQueryResult &rhs) {
        if (lhs.total_time != rhs.total_time) {
            return lhs.total_time < rhs.total_time;
        }
        return my_strcmp(lhs.train_id, rhs.train_id);
    };

    bool better(TransferQueryResult &lhs, TransferQueryResult &rhs, const TicketQueryRequest& request) {
        if (!rhs.exists) {
            return true;
        }
        if (request.sort_policy == TicketSortPolicy::byCost) {
            if (lhs.total_price != rhs.total_price) {
                return lhs.total_price < rhs.total_price;
            }
            return lhs.total_time < rhs.total_time;
        }
        if (lhs.total_time != rhs.total_time) {
            return lhs.total_time < rhs.total_time;
        }
        return lhs.total_price < rhs.total_price;
    }

}

namespace sjtu {
    TrainService::TrainService() : initialized_(false), train_index_(nullptr),station_index_ (nullptr), seat_index_(nullptr){  }

     TrainService::~TrainService() {
        delete train_index_;
        delete station_index_;
        delete seat_index_;
        train_index_ = nullptr;
        station_index_ = nullptr;
        seat_index_ = nullptr;
    }

    bool TrainService::init(const std::string &data_dir) {
        data_dir_ = data_dir;
        initialized_ = train_file_.open("ts_trains.dat") && seat_file_.open("ts_seats.dat");
        delete train_index_;
        delete station_index_;
        delete seat_index_;
        train_index_ = new BPT<Data>("ts_train_index");
        station_index_ = new BPT<Data>("ts_station_index");
        seat_index_ = new BPT<Data>("ts_seat_index");
        return initialized_;
    }

    void TrainService::clear() {
        if (!initialized_) return;
        train_file_.clear();
        seat_file_.clear();
        reset_index_files();
    }

    void TrainService::reset_index_files() {
        delete train_index_;
        delete station_index_;
        delete seat_index_;
        train_index_ = nullptr;
        station_index_ = nullptr;
        seat_index_ = nullptr;
        std::remove("init_ts_train_index");
        std::remove("data_ts_train_index");
        std::remove("init_ts_station_index");
        std::remove("data_ts_station_index");
        std::remove("init_ts_seat_index");
        std::remove("data_ts_seat_index");
        train_index_ = new BPT<Data>("ts_train_index");
        station_index_ = new BPT<Data>("ts_station_index");
        seat_index_ = new BPT<Data>("ts_seat_index");
    }

    bool TrainService::find_train_offset(const std::string& train_id, int& train_offset) const {
        Data probe(train_id, INT_MIN);
        Data result;
        if (!train_index_->lower_bound(probe, result)) {
            return false;
        }
        if (result.key != hash_key(train_id)) {
            return false;
        }
        train_offset = result.value;
        return true;

    }

    int TrainService::pack_station_entry(int train_offset, int station_index) { //station index <= 100 < (1 << 7)
        return (train_offset << 7) | station_index;
    }

    int TrainService::unpack_train_offset(int packed_value) {
        return packed_value >> 7;
    }

    int TrainService::unpack_station_index(int packed_value) {
        return packed_value & 127;
    }


    bool TrainService::get_train(const std::string& train_id, sjtu::TrainRecord& train, int& train_offset) const {
        if (!find_train_offset(train_id, train_offset)) {
            return false;
        }
        return train_file_.read(train_offset, train);
    }

    bool TrainService::get_train_by_offset(int train_offset, sjtu::TrainRecord& train) const {
        return train_file_.read(train_offset, train);
    }

    std::string TrainService::make_seat_key(int train_offset, int running_date) {
        return std::to_string(train_offset) + "#" + std::to_string(running_date);
    }

    bool TrainService::get_existing_seat_record(int train_offset, int running_date, SeatRecord& seat_record,
                                                int& seat_offset) const {
        const std::string key = make_seat_key(train_offset, running_date);
        Data probe(key, INT_MIN);
        Data result;
        if (!seat_index_->lower_bound(probe, result)) {
            return false;
        }
        if (result.key != hash_key(key)) {
            return false;
        }
        seat_offset = result.value;
        return seat_file_.read(seat_offset, seat_record);
    }

    bool TrainService::locate_station(const TrainRecord& train, const std::string& station_name,
                                      int& station_index) {
        for (int i = 0; i < train.stationNum; i++) {
            if (from_buffer(train.stations[i]) == station_name) {
                station_index = i;
                return true;
            }
        }
        return false;
    }

    int TrainService::resolve_running_date(const TrainRecord& train, int station_index,
                                           const Date& station_departure_date) {
        const int station_day = date_to_ordinal(station_departure_date);
        const int total_offset = train.start_time_minutes + train.departure_offsets[station_index];
        const int running_date = station_day - total_offset / kMinutesPerDay;
        if (running_date < train.sale_begin || running_date > train.sale_end) {
            return -1;
        }
        return running_date;
    }

    int TrainService::query_min_remaining_seat(const SeatRecord& seat_record, int from_index, int to_index) {
        int result = INT_MAX;
        for (int i = from_index; i < to_index; i++) {
            if (result > seat_record.remain[i]) {
                result = seat_record.remain[i];
            }
        }
        return result == INT_MAX ? 0 : result;
    }

    bool TrainService::load_or_create_seat_record(int train_offset, int running_date, const sjtu::TrainRecord& train,
                                                  sjtu::SeatRecord& seat_record, int& seat_offset) {
        if (get_existing_seat_record(train_offset, running_date, seat_record, seat_offset)) {
            return true;
        }

        SeatRecord fresh;
        fresh.train_offset = train_offset;
        fresh.segment_num = train.stationNum - 1;
        fresh.running_date = running_date;
        for (int i = 0; i < fresh.segment_num; i++) {
            fresh.remain[i] = train.seatNum;
        }

        seat_offset = seat_file_.append(fresh);
        if (seat_offset == -1) {
            return false;
        }
        seat_index_->add(Data(make_seat_key(train_offset, running_date), seat_offset));
        seat_record = fresh;
        return true;
    }


    bool TrainService::add_train(const TrainRecord& train) {
        int discarded_offset = -1;
        if (find_train_offset(train.trainID, discarded_offset)) {
            return false;
        }
        const int offset = train_file_.append(train);
        if (offset == -1) {
            return false;
        }
        train_index_->add(Data(from_buffer(train.trainID), offset));
        return true;
    }

    bool TrainService::read_seat_record(int seat_offset, sjtu::SeatRecord &seat_record) const {
        return seat_file_.read(seat_offset, seat_record);
    }

    bool TrainService::write_seat_record(int seat_offset, sjtu::SeatRecord &seat_record) {
        return seat_file_.write(seat_offset, seat_record);
    }

    void TrainService::apply_seat_delta(sjtu::SeatRecord &seat_record, int from_index, int to_index, int delta) {
        for (int index = from_index; index < to_index; ++index) {
            seat_record.remain[index] += delta;
        }
    }

    bool TrainService::delete_train(const std::string& trainID) {
        int offset = -1;
        if (!find_train_offset(trainID, offset)) {
            return false;
        }
        TrainRecord train;
        if (!train_file_.read(offset, train)) {
            return false;
        }
        if (train.released) {
            return false;
        }
        train_index_->remove(Data(trainID, offset));
        return train_file_.recycle(offset);
    }

    bool TrainService::release_train(const std::string& trainID) {  //发布后可以在station_index_里面加入对应信息
        int offset = -1;
        if (!find_train_offset(trainID, offset)) {
            return false;
        }
        TrainRecord train;
        if (!train_file_.read(offset, train)) {
            return false;
        }
        if (train.released) {
            return false;
        }
        train.released = true;
        if (!train_file_.write(offset, train)) {
            return false;
        }
        for (int idx = 0; idx < train.stationNum; idx++) {
            station_index_->add(Data(from_buffer(train.stations[idx]), pack_station_entry(offset, idx)));
        }
        return true;
    }

    bool TrainService::query_train(const std::string& train_id, const Date& date, TrainQueryView& result) const {
        int offset = -1;
        TrainRecord train;
        if (!get_train(train_id, train, offset)) {
            return false;
        }
        if (!train_file_.read(offset, train)) {
            return false;
        }
        const int running_date = date_to_ordinal(date);    //int 天
        if (running_date < train.sale_begin || running_date > train.sale_end) {
            return false;
        }
        copy_to_buffer(train.trainID, result.train_id, sizeof(result.train_id));
        result.type = train.type;
        result.station_num = train.stationNum;

        SeatRecord seat_record;
        int seat_offset = -1;
        bool has_seat = train.released && get_existing_seat_record(offset, running_date, seat_record, seat_offset);

        for (int idx = 0; idx < train.stationNum; idx++) {
            TrainStationView& view = result.stations[idx];
            copy_to_buffer(train.stations[idx], view.station_name, sizeof(view.station_name));
            view.price_from_start = train.prefix_prices[idx];
            view.has_arriving_time = idx != 0;
            view.has_leaving_time = idx != train.stationNum - 1;
            view.has_seat_to_next = idx != train.stationNum - 1;
            if (view.has_arriving_time) {
                view.arriving = absolute_minutes_to_datetime(to_absolute_minutes(running_date, train.start_time_minutes) + train.arrival_offsets[idx]);
            }
            if (view.has_leaving_time) {
                view.leaving = absolute_minutes_to_datetime(to_absolute_minutes(running_date, train.start_time_minutes) + train.departure_offsets[idx]);
            }
            if (view.has_seat_to_next) {
                if (has_seat) {
                    view.seat_to_next = seat_record.remain[idx];
                }
                else{
                    view.seat_to_next = train.seatNum;
                }
            }
        }
        return true;
    }

    bool TrainService::query_ticket(const TicketQueryRequest& request, std::vector<TicketQueryResult>& results) const {
        results.clear();
        std::vector<Data> station_matches;
        station_index_->range_query(Data(request.from, INT_MIN), Data(request.from, INT_MAX), station_matches);

        for (int i = 0; i < station_matches.size(); i++) {
            const int packed = station_matches[i].value;
            const int train_offset = unpack_train_offset(packed);
            const int from_idx = unpack_station_index(packed);

            TrainRecord train;
            if (!train_file_.read(train_offset, train)) {
                continue;
            }
            int to_idx = -1;
            if (!locate_station(train, request.to, to_idx) || from_idx >= to_idx) {
                continue;
            }

            const int running_date = resolve_running_date(train, from_idx, request.departure_date);
            if (running_date == -1) {
                continue;
            }

            int seat = train.seatNum;
            SeatRecord seat_record;
            int seat_offset = -1;
            if (get_existing_seat_record(train_offset, running_date, seat_record, seat_offset)) {
                seat = query_min_remaining_seat(seat_record, from_idx, to_idx);
            }

            TicketQueryResult item;
            fill_ticket_result(train, from_idx, to_idx, running_date, seat, item);
            results.push_back(item);
        }

        if (!results.empty()) {
            if (request.sort_policy == TicketSortPolicy::byCost) {
                std::sort(&results[0], &results[0] + results.size(), compare_by_cost);
            }
            if (request.sort_policy == TicketSortPolicy::byTime) {
                std::sort(&results[0], &results[0] + results.size(), compare_by_time);
            }
        }
        return true;
    }

    bool TrainService::query_transfer(const TicketQueryRequest& request, TransferQueryResult& result) const {
        std::vector<Data> from_matches;
        station_index_->range_query(Data(request.from, INT_MIN), Data(request.from, INT_MAX), from_matches);
        if (from_matches.empty()) {
            return false;
        }

        std::map<std::string, std::vector<int>>station_cache;
        std::map<int, TrainRecord> train_cache;

        auto load_station_candidates = [&](const std::string &station_name) -> const std::vector<int> & {
            auto found = station_cache.find(station_name);
            if (found != station_cache.end()) {
                return found->second;
            }
            std::vector<Data> raw;
            station_index_->range_query(Data(station_name, INT_MIN), Data(station_name, INT_MAX), raw);
            std::vector<int> values;
            for (int idx = 0; idx < static_cast<int>(raw.size()); ++idx) {
                values.push_back(raw[idx].value);
            }
            station_cache.insert({station_name, values});
            return station_cache.find(station_name)->second;
        };

        auto load_train = [&](int train_offset, TrainRecord &train) -> bool {
            auto found = train_cache.find(train_offset);
            if (found != train_cache.end()) {
                train = found->second;
                return true;
            }
            if (!train_file_.read(train_offset, train)) {
                return false;
            }
            train_cache.insert({train_offset, train});
            return true;
        };

        for (int index = 0; index < static_cast<int>(from_matches.size()); ++index) {  //三层循环 枚举 first train / mid station /second train
            const int packed_first = from_matches[index].value;
            const int train1_offset = unpack_train_offset(packed_first);
            const int from_index = unpack_station_index(packed_first);

            TrainRecord train1{};
            if (!load_train(train1_offset, train1)) {
                continue;
            }

            const int running_date1 = resolve_running_date(train1, from_index, request.departure_date);
            if (running_date1 == -1) {
                continue;
            }

            const long long first_departure_abs = to_absolute_minutes(running_date1, train1.start_time_minutes) + train1.departure_offsets[from_index];

            SeatRecord seat_record1{};
            int seat_offset1 = -1;
            const bool has_seat_record1 = get_existing_seat_record(train1_offset, running_date1, seat_record1, seat_offset1);

            int first_leg_min_seat = train1.seatNum;
            for (int mid_index = from_index + 1; mid_index < train1.stationNum; ++mid_index) {
                if (has_seat_record1 && seat_record1.remain[mid_index - 1] < first_leg_min_seat) {
                    first_leg_min_seat = seat_record1.remain[mid_index - 1];
                }
                const std::string transfer_station = from_buffer(train1.stations[mid_index]);
                const long long arrival_mid_abs = to_absolute_minutes(running_date1, train1.start_time_minutes) + train1.arrival_offsets[mid_index];
                const int first_price = train1.prefix_prices[mid_index] - train1.prefix_prices[from_index];

                const std::vector<int> &second_candidates = load_station_candidates(transfer_station);
                for (int second_index = 0; second_index < static_cast<int>(second_candidates.size()); ++second_index) {
                    const int packed_second = second_candidates[second_index];
                    const int train2_offset = unpack_train_offset(packed_second);
                    const int transfer_index = unpack_station_index(packed_second);
                    if (train2_offset == train1_offset) {
                        continue;
                    }

                    TrainRecord train2{};
                    if (!load_train(train2_offset, train2)) {
                        continue;
                    }

                    int destination_index = -1;
                    if (!locate_station(train2, request.to, destination_index) || destination_index <= transfer_index) {
                        continue;
                    }

                    const int train2_total_departure_offset = train2.start_time_minutes + train2.departure_offsets[transfer_index];
                    long long needed_running_date = (arrival_mid_abs - train2_total_departure_offset + kMinutesPerDay - 1) / kMinutesPerDay;
                    if (needed_running_date < train2.sale_begin) {
                        needed_running_date = train2.sale_begin;
                    }
                    if (needed_running_date > train2.sale_end) {
                        continue;
                    }

                    const int running_date2 = static_cast<int>(needed_running_date);
                    const long long second_departure_abs = to_absolute_minutes(running_date2, train2.start_time_minutes) + train2.departure_offsets[transfer_index];
                    if (second_departure_abs < arrival_mid_abs) {
                        continue;
                    }

                    int second_leg_seat = train2.seatNum;
                    SeatRecord seat_record2{};
                    int seat_offset2 = -1;
                    if (get_existing_seat_record(train2_offset, running_date2, seat_record2, seat_offset2)) {
                        second_leg_seat = query_min_remaining_seat(seat_record2, transfer_index, destination_index);
                    }

                    TransferQueryResult candidate{};
                    candidate.exists = true;
                    fill_ticket_result(train1, from_index, mid_index, running_date1, first_leg_min_seat, candidate.first);
                    fill_ticket_result(train2, transfer_index, destination_index, running_date2, second_leg_seat, candidate.second);
                    candidate.total_price = first_price + candidate.second.price;
                    candidate.total_time = static_cast<int>(to_absolute_minutes(candidate.second.arriving) - first_departure_abs);

                    if (better(candidate, result, request)) {
                        result = candidate;
                    }
                }
            }
        }
        return result.exists;
    }

}

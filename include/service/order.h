//
// Created by lenovo on 2026/5/29.
//

#ifndef TICKET_SYSTEM_2026_1_ORDER_H
#define TICKET_SYSTEM_2026_1_ORDER_H

#include <string>
#include "../model/data_types.h"
#include <map>
#include "../storage/bpt.h"
#include "../storage/file_manager.h"
#include<util/internal_utils.h>
#include "service/user.h"
#include "service/train.h"

namespace sjtu {
    class UserService;
    class TrainService;

    class OrderService {
    public:
        OrderService();
        ~OrderService();

        bool init(const std::string &data_dir, UserService *user_service, TrainService *train_service);
        void clear();
        bool get_order(int order_offset, OrderRecord &record)const;
        void try_promote_pending_orders(const std::string &train_id, int running_date);

        BuyTicketResult buy_ticket(const BuyTicketQuery& request);
        bool query_order(const std::string &username, std::vector<sjtu::OrderView> &orders);
        bool refund_ticket(std::string &username, int n);

    private:
        std::string data_dir_;
        bool initialized_;
        UserService* user_service_;
        TrainService* train_service_;
        RecordFile<OrderRecord> order_file_;
        BPT<Data>* user_order_index_;
        BPT<Data>* pending_order_index_;

        void reset_index_files();
        bool append_order_index(const std::string &username, int order_offset);
        std::string make_pending_key(std::string trainID, int running_date);
        void build_order_view(const OrderRecord &order, const TrainRecord &train, OrderView &view);
    };
}

#endif // TICKET_SYSTEM_2026_1_ORDER_H

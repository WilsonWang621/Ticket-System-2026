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

namespace sjtu {
    class UserService;
    class TrainService;

    class OrderService {
    public:
        OrderService();
        ~OrderService();



    private:
        std::string data_dir_;
        bool initialized_;
        UserService* user_service_;
        TrainService* train_service_;
        RecordFile<OrderRecord> order_file_;
        BPT<Data>* user_order_index_;
        BPT<Data>* pending_order_index_;

    };
}

#endif // TICKET_SYSTEM_2026_1_ORDER_H

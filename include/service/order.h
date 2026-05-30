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
        UserService* user_service_;
        TrainService* train_service_;

    };
}

#endif // TICKET_SYSTEM_2026_1_ORDER_H

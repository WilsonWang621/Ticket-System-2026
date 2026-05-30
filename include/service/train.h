//
// Created by lenovo on 2026/5/29.
//

#ifndef TICKET_SYSTEM_2026_1_TRAIN_H
#define TICKET_SYSTEM_2026_1_TRAIN_H
#include <string>
#include "../model/data_types.h"
#include <vector>
#include "../storage/bpt.h"
#include "../storage/file_manager.h"

namespace sjtu {
    class TrainService {
    public:
        TrainService();
        ~TrainService();

        bool add_train(const TrainRecord &train);
        bool delete_train(const std::string &trainID);
        bool release_train(const std::string& trainID);
        bool query_train(const std::string &train_id, const sjtu::Date &date, TrainQueryView &result) const;
        bool query_ticket(const std::string &train_id, const Date &date, std::vector<TicketQueryResult>& results) const;
        bool query_transfer(TicketQueryRequest &request, std::vector<TransferQueryResult>& results) const;
    private:

    };
}
#endif // TICKET_SYSTEM_2026_1_TRAIN_H

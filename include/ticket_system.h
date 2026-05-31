#ifndef TICKET_SYSTEM_2026_TICKET_SYSTEM_H
#define TICKET_SYSTEM_2026_TICKET_SYSTEM_H
#include <string>
#include <util/parser.h>
#include <vector>
#include "service/train.h"
#include "service/user.h"
namespace sjtu{

class TicketSystem {
public:
    TicketSystem();
    ~TicketSystem();
    void run();


private:
    UserService user_service_;
    TrainService train_service_;

    bool handle_add_user(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_login(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_logout(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_query_profile(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_modify_profile(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_add_train(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_delete_train(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_release_train(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_query_train(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_query_ticket(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_query_transfer(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_buy_ticket(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_query_order(const parsedCommand &command, std::vector<std::string> &output_lines);
    bool handle_refund_ticket(const parsedCommand &command, std::vector<std::string> &output_lines);

    bool excute(parsedCommand &command, std::vector<std::string> &output);

};

}

#endif // TICKET_SYSTEM_2026_TICKET_SYSTEM_H

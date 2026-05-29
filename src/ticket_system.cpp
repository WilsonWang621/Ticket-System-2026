#include<ticket_system.h>
#include <iostream>
namespace sjtu{
    bool TicketSystem::excute(parsedCommand& command, std::vector<std::string> output) {
        if (command.command_name == "add_user") {
            return handle_add_train(command, output);
        }
        if (command.command_name == "login") {
            return handle_login(command, output);
        }
        if (command.command_name == "logout") {
            return handle_logout(command, output);
        }
        if (command.command_name == "query_profile") {
            return handle_query_profile(command, output);
        }
        if (command.command_name == "modify_profile") {
            return handle_modify_profile(command, output);
        }
        if (command.command_name == "add_train") {
            return handle_add_train(command, output);
        }
        if (command.command_name == "delete_train") {
            return handle_delete_train(command, output);
        }
        if (command.command_name == "release_train") {
            return handle_release_train(command, output);
        }
        if (command.command_name == "query_train") {
            return handle_query_train(command, output);
        }
        if (command.command_name == "query_ticket") {
            return handle_query_ticket(command, output);
        }
        if (command.command_name == "query_transfer") {
            return handle_query_transfer(command, output);
        }
        if (command.command_name == "buy_ticket") {
            return handle_buy_ticket(command, output);
        }
        if (command.command_name == "query_order") {
            return handle_query_order(command, output);
        }
        if (command.command_name == "refund") {
            return handle_refund_ticket(command, output);
        }

        if (command.command_name == "clean") {

        }
        if (command.command_name == "exit") {
            return true;
        }
    }

    void TicketSystem::run() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            parsedCommand command;
            CommandParser::parsed(line, command);

            std::vector<std::string> output;
            if (excute(command, output) || output.empty()) {
                continue;
            }

            std::cout << "[" << output[0] << "]" << " ";
            for (int i = 1; i < output.size(); i++) {
                std::cout << output[i] << " ";
            }
            std::cout << '\n';
            if (command.command_name == "exit") {
                break;
            }
        }
    }

}

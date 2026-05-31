//
// Created by lenovo on 2026/5/29.
//

#ifndef TICKET_SYSTEM_2026_1_PARSER_H
#define TICKET_SYSTEM_2026_1_PARSER_H

#include <string>
namespace sjtu {
constexpr int kMaxCommandArguments = 16;

struct commandArgument {
    char arg = '\0';
    std::string value;
};

struct parsedCommand {
    int timestamp = 0;
    std::string command_name;
    commandArgument arguments[kMaxCommandArguments];
    int argument_count = 0;
};

class CommandParser {
public:
    static void parsed(std::string &line, parsedCommand &command);
    void splitPipe(std::string &line, std::string part[]);
};

}
#endif // TICKET_SYSTEM_2026_1_PARSER_H

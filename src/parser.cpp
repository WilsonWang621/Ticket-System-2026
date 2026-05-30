//
// Created by lenovo on 2026/5/29.
//
#include <../include/util/parser.h>

namespace sjtu {
    void CommandParser::parsed(std::string& line, parsedCommand& command) {
        int cnt = -2;
        bool empty = true;
        std::string tmp;
        for (int i = 0; i < line.size(); i++) {
            if (line[i] == ' ' && !empty) {
                if (cnt == -2) {
                    command.timestamp = std::stoi(tmp);
                }
                if (cnt == -1) {
                    command.command_name = tmp;
                }
                else {
                    command.arguments[cnt].value = tmp;
                }
                cnt++;
                tmp = "";
                empty = true;
            }
            else {
                if (line[i] == '-') {
                    command.arguments[cnt].arg = line[++i];
                    empty = true;
                }
                else {
                    tmp += line[i];
                    empty = false;
                }
            }
        }
    }

}
//
// Created by lenovo on 2026/5/29.
//
#include <../include/util/parser.h>

namespace sjtu {
    namespace {
        void skip_spaces(const std::string &line, int &pos) {
            while (pos < static_cast<int>(line.size()) && line[pos] == ' ') {
                ++pos;
            }
        }

        std::string next_token(const std::string &line, int &pos) {
            skip_spaces(line, pos);
            int begin = pos;
            while (pos < static_cast<int>(line.size()) && line[pos] != ' ') {
                ++pos;
            }
            return line.substr(begin, pos - begin);
        }
    }

    void CommandParser::parsed(std::string& line, parsedCommand& command) {
        command = parsedCommand{};
        int pos = 0;
        skip_spaces(line, pos);
        if (pos < static_cast<int>(line.size()) && line[pos] == '[') {
            ++pos;
            while (pos < static_cast<int>(line.size()) && line[pos] >= '0' && line[pos] <= '9') {
                command.timestamp = command.timestamp * 10 + line[pos] - '0';
                ++pos;
            }
            if (pos < static_cast<int>(line.size()) && line[pos] == ']') {
                ++pos;
            }
        }

        command.command_name = next_token(line, pos);

        std::string flag;
        std::string value;
        while (pos < static_cast<int>(line.size())) {
            flag = next_token(line, pos);
            value = next_token(line, pos);
            if (flag.empty() || value.empty()) {
                break;
            }
            if (flag.size() != 2 || flag[0] != '-') {
                continue;
            }
            if (command.argument_count >= kMaxCommandArguments) {
                break;
            }
            command.arguments[command.argument_count].arg = flag[1];
            command.arguments[command.argument_count].value = value;
            ++command.argument_count;
        }
    }

}

#include <ticket_system.h>

#include <iostream>
#include <string>

#include <util/internal_utils.h>

namespace {
    bool get_arg(const sjtu::parsedCommand &command, char name, std::string &value) {  //匹配参数并获取对应value
        for (int i = 0; i < command.argument_count; ++i) {
            if (command.arguments[i].arg == name) {
                value = command.arguments[i].value;
                return true;
            }
        }
        return false;
    }

    std::string require_arg(const sjtu::parsedCommand &command, char name) {     //获取对应参数的value
        std::string value;
        get_arg(command, name, value);
        return value;
    }

    bool has_arg(const sjtu::parsedCommand &command, char name) {     //检查是否有目标参数类型
        std::string ignored;
        return get_arg(command, name, ignored);
    }

    sjtu::vector<std::string> split_pipe(const std::string &text) {    //分割 |
        sjtu::vector<std::string> result;
        std::string current;
        for (char ch : text) {
            if (ch == '|') {
                result.push_back(current);
                current.clear();
            }
            else {
                current.push_back(ch);
            }
        }
        result.push_back(current);
        return result;
    }

    sjtu::Date parse_date(const std::string &text) {     // 把string 类的日期转化成 Date类型
        sjtu::Date date{};
        date.month = std::stoi(text.substr(0, 2));
        date.day = std::stoi(text.substr(3, 2));
        return date;
    }

    sjtu::ClockTime parse_time(const std::string &text) { // 把string 类的日期转化成 time类型
        sjtu::ClockTime time{};
        time.hour = std::stoi(text.substr(0, 2));
        time.minute = std::stoi(text.substr(3, 2));
        return time;
    }

    int parse_minutes(const std::string &text) {
        return std::stoi(text);
    }

    std::string two_digits(int value) {   //调整位数
        if (value < 10) {
            return "0" + std::to_string(value);
        }
        return std::to_string(value);
    }
    //按要求调整输出格式
    std::string format_date(const sjtu::Date &date) {
        return two_digits(date.month) + "-" + two_digits(date.day);
    }

    std::string format_time(const sjtu::ClockTime &time) {
        return two_digits(time.hour) + ":" + two_digits(time.minute);
    }

    std::string format_datetime(const sjtu::DateTime &time) {
        return format_date(time.date) + " " + format_time(time.time);
    }

    std::string format_ticket(const sjtu::TicketQueryResult &ticket) {
        return std::string(ticket.train_id) + " " +
               ticket.from + " " + format_datetime(ticket.leaving) + " -> " +
               ticket.to + " " + format_datetime(ticket.arriving) + " " +
               std::to_string(ticket.price) + " " + std::to_string(ticket.seat);
    }

    std::string format_profile(const sjtu::UserProfile &user) {
        return std::string(user.username) + " " +
               user.name + " " +
               user.mailAddr + " " +
               std::to_string(user.privilege);
    }

    std::string format_order(const sjtu::OrderView &orders) {
        std::string s = "";
        if (orders.status == sjtu::OrderState::kPending) {
            s += "[pending]";
        }
        else if (orders.status == sjtu::OrderState::kRefunded) {
            s += "[refunded]";
        }
        else {
            s += "[success]";
        }
        return s + " " + orders.train_id + " " + orders.from + " " + format_datetime(orders.leaving) + " -> " +
            orders.to + " " + format_datetime(orders.arriving) + " " + std::to_string(orders.price) + " " + std::to_string(orders.ticket_num);
    }

    void push_success(sjtu::vector<std::string> &output_lines) {
        output_lines.push_back("0");
    }

    void push_failure(sjtu::vector<std::string> &output_lines) {
        output_lines.push_back("-1");
    }
}

namespace sjtu {
    TicketSystem::TicketSystem() {
        user_service_.init(".");
        train_service_.init(".");
        order_service_.init(".", &user_service_, &train_service_);
    }

    TicketSystem::~TicketSystem() = default;

    // 统一的思路就是先开一个对应指令的结构体，然后把解析好的信息喂给对应结构体变量然后 作为结构体参数和输出信息outline一起作为参数传入相应的处理函数
    bool TicketSystem::handle_add_user(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        UserProfile user{};
        copy_to_buffer(require_arg(command, 'u'), user.username, sizeof(user.username));
        copy_to_buffer(require_arg(command, 'p'), user.password, sizeof(user.password));
        copy_to_buffer(require_arg(command, 'n'), user.name, sizeof(user.name));
        copy_to_buffer(require_arg(command, 'm'), user.mailAddr, sizeof(user.mailAddr));
        user.privilege = std::stoi(require_arg(command, 'g'));

        if (user_service_.add_user(require_arg(command, 'c'), user)) {
            push_success(output_lines);
        }
        else {
            push_failure(output_lines);
        }
        return false;
    }

    bool TicketSystem::handle_login(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        if (user_service_.login(require_arg(command, 'u'), require_arg(command, 'p'))) {
            push_success(output_lines);
        }
        else {
            push_failure(output_lines);
        }
        return false;
    }

    bool TicketSystem::handle_logout(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        if (user_service_.logout(require_arg(command, 'u'))) {
            push_success(output_lines);
        }
        else {
            push_failure(output_lines);
        }
        return false;
    }

    bool TicketSystem::handle_query_profile(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        UserProfile user{};
        if (!user_service_.query_profile(require_arg(command, 'c'), require_arg(command, 'u'), user)) {
            push_failure(output_lines);
            return false;
        }

        output_lines.push_back(format_profile(user));
        return false;
    }

    bool TicketSystem::handle_modify_profile(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        ProfileUpdateRequest request{};
        std::string value;
        if (get_arg(command, 'p', value)) {
            request.change_password = true;
            copy_to_buffer(value, request.password, sizeof(request.password));
        }
        if (get_arg(command, 'n', value)) {
            request.change_name = true;
            copy_to_buffer(value, request.name, sizeof(request.name));
        }
        if (get_arg(command, 'm', value)) {
            request.change_mail = true;
            copy_to_buffer(value, request.mail, sizeof(request.mail));
        }
        if (get_arg(command, 'g', value)) {
            request.change_privilege = true;
            request.privilege = std::stoi(value);
        }

        UserProfile user{};
        if (!user_service_.modify_profile(require_arg(command, 'c'), require_arg(command, 'u'), request, user)) {
            push_failure(output_lines);
            return false;
        }

        output_lines.push_back(format_profile(user));
        return false;
    }

    bool TicketSystem::handle_add_train(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        TrainRecord train{};
        copy_to_buffer(require_arg(command, 'i'), train.trainID, sizeof(train.trainID));
        train.stationNum = std::stoi(require_arg(command, 'n'));
        train.seatNum = std::stoi(require_arg(command, 'm'));
        train.type = require_arg(command, 'y').empty() ? '\0' : require_arg(command, 'y')[0];

        const sjtu::vector<std::string> stations = split_pipe(require_arg(command, 's'));
        for (int i = 0; i < train.stationNum && i < static_cast<int>(stations.size()); ++i) {
            copy_to_buffer(stations[i], train.stations[i], sizeof(train.stations[i]));
        }

        const sjtu::vector<std::string> prices = split_pipe(require_arg(command, 'p'));
        train.prefix_prices[0] = 0;
        for (int i = 0; i + 1 < train.stationNum && i < static_cast<int>(prices.size()); ++i) {
            train.prices[i] = std::stoi(prices[i]);
            train.prefix_prices[i + 1] = train.prefix_prices[i] + train.prices[i];
        }

        train.startTime = parse_time(require_arg(command, 'x'));
        train.start_time_minutes = time_to_minutes(train.startTime);

        const sjtu::vector<std::string> travel_times = split_pipe(require_arg(command, 't'));
        for (int i = 0; i + 1 < train.stationNum && i < static_cast<int>(travel_times.size()); ++i) {
            train.travelTimes[i] = parse_minutes(travel_times[i]);
        }

        if (has_arg(command, 'o')) {
            const sjtu::vector<std::string> stopover_times = split_pipe(require_arg(command, 'o'));
            for (int i = 0; i + 2 < train.stationNum && i < static_cast<int>(stopover_times.size()); ++i) {
                train.stopoverTimes[i] = parse_minutes(stopover_times[i]);
            }
        }

        train.arrival_offsets[0] = 0;
        train.departure_offsets[0] = 0;
        for (int i = 0; i + 1 < train.stationNum; ++i) {
            train.arrival_offsets[i + 1] = train.departure_offsets[i] + train.travelTimes[i];
            if (i + 2 < train.stationNum) {
                train.departure_offsets[i + 1] = train.arrival_offsets[i + 1] + train.stopoverTimes[i];
            }
        }

        const sjtu::vector<std::string> sale_dates = split_pipe(require_arg(command, 'd'));
        train.sale_begin = date_to_ordinal(parse_date(sale_dates[0]));
        train.sale_end = date_to_ordinal(parse_date(sale_dates[1]));

        if (train_service_.add_train(train)) {
            push_success(output_lines);
        }
        else {
            push_failure(output_lines);
        }
        return false;
    }

    bool TicketSystem::handle_delete_train(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        if (train_service_.delete_train(require_arg(command, 'i'))) {
            push_success(output_lines);
        }
        else {
            push_failure(output_lines);
        }
        return false;
    }

    bool TicketSystem::handle_release_train(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        if (train_service_.release_train(require_arg(command, 'i'))) {
            push_success(output_lines);
        }
        else {
            push_failure(output_lines);
        }
        return false;
    }

    bool TicketSystem::handle_query_train(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        TrainQueryView view{};
        if (!train_service_.query_train(require_arg(command, 'i'), parse_date(require_arg(command, 'd')), view)) {
            push_failure(output_lines);
            return false;
        }

        output_lines.push_back(std::string(view.train_id) + " " + view.type);

        for (int i = 0; i < view.station_num; ++i) {
            const TrainStationView &station = view.stations[i];
            std::string line = std::string(station.station_name) + " ";
            if (station.has_arriving_time) {
                line += format_datetime(station.arriving);
            }
            else {
                line += "xx-xx xx:xx";
            }
            line += " -> ";
            if (station.has_leaving_time) {
                line += format_datetime(station.leaving);
            }
            else {
                line += "xx-xx xx:xx";
            }
            line += " " + std::to_string(station.price_from_start) + " ";
            if (station.has_seat_to_next) {
                line += std::to_string(station.seat_to_next);
            }
            else {
                line += "x";
            }
            output_lines.push_back(line);
        }
        return false;
    }

    bool TicketSystem::handle_query_ticket(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        TicketQueryRequest request{};
        copy_to_buffer(require_arg(command, 's'), request.from, sizeof(request.from));
        copy_to_buffer(require_arg(command, 't'), request.to, sizeof(request.to));
        request.departure_date = parse_date(require_arg(command, 'd'));
        request.sort_policy = require_arg(command, 'p') == "cost" ? TicketSortPolicy::byCost : TicketSortPolicy::byTime;

        sjtu::vector<TicketQueryResult> results;
        train_service_.query_ticket(request, results);

        output_lines.push_back(std::to_string(results.size()));
        for (const TicketQueryResult &ticket : results) {
            output_lines.push_back(format_ticket(ticket));
        }
        return false;
    }

    bool TicketSystem::handle_query_transfer(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        TicketQueryRequest request{};
        copy_to_buffer(require_arg(command, 's'), request.from, sizeof(request.from));
        copy_to_buffer(require_arg(command, 't'), request.to, sizeof(request.to));
        request.departure_date = parse_date(require_arg(command, 'd'));
        request.sort_policy = require_arg(command, 'p') == "cost" ? TicketSortPolicy::byCost : TicketSortPolicy::byTime;

        TransferQueryResult result{};
        if (!train_service_.query_transfer(request, result)) {
            output_lines.push_back("0");
            return false;
        }

        output_lines.push_back(format_ticket(result.first));
        output_lines.push_back(format_ticket(result.second));
        return false;
    }

    bool TicketSystem::handle_buy_ticket(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        BuyTicketQuery request;
        copy_to_buffer(require_arg(command, 'u'), request.username, sizeof(request.username));
        copy_to_buffer(require_arg(command, 'i'), request.trainID, sizeof(request.trainID));
        copy_to_buffer(require_arg(command, 'f'), request.from, sizeof(request.from));
        copy_to_buffer(require_arg(command, 't'), request.to, sizeof(request.to));
        request.ticketNum = std::stoi(require_arg(command, 'n'));
        request.departure_date = parse_date(require_arg(command, 'd'));
        if (has_arg(command, 'q')) {
            if (require_arg(command, 'q') == "true") {
                request.allow_queue = true;
            }
            else {
                request.allow_queue = false;
            }
        }
        BuyTicketResult result = order_service_.buy_ticket(request);
        if (result.state == BuyTicketState::Failed) {
            output_lines.push_back(std::to_string(-1));
        }
        else if (result.state == BuyTicketState::Pending) {
            output_lines.push_back("queue");
        }
        else {
            output_lines.push_back(std::to_string(result.total_price));
        }
        return false;
    }

    bool TicketSystem::handle_query_order(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        sjtu::vector<sjtu::OrderView> orders;
        if (!order_service_.query_order(require_arg(command, 'u'), orders)){
            output_lines.push_back(std::to_string(-1));
            return false;
        }
        output_lines.push_back(std::to_string(orders.size()));
        for (size_t i = 0; i < orders.size(); i++) {
            output_lines.push_back(format_order(orders[i]));
        }
        return false;
    }

    bool TicketSystem::handle_refund_ticket(const parsedCommand &command, sjtu::vector<std::string> &output_lines) {
        bool flag;
        std::string name = require_arg(command, 'u');
        if (has_arg(command, 'n')) {
            flag = order_service_.refund_ticket(name, std::stoi(require_arg(command, 'n')));
        }
        else {
            flag = order_service_.refund_ticket(name,  1);
        }
        if (!flag) {
            push_failure(output_lines);
        }
        else {
            push_success(output_lines);
        }
        return false;
    }

    bool TicketSystem::execute(parsedCommand& command, sjtu::vector<std::string> &output) {
        if (command.command_name == "add_user") return handle_add_user(command, output);
        if (command.command_name == "login") return handle_login(command, output);
        if (command.command_name == "logout") return handle_logout(command, output);
        if (command.command_name == "query_profile") return handle_query_profile(command, output);
        if (command.command_name == "modify_profile") return handle_modify_profile(command, output);
        if (command.command_name == "add_train") return handle_add_train(command, output);
        if (command.command_name == "delete_train") return handle_delete_train(command, output);
        if (command.command_name == "release_train") return handle_release_train(command, output);
        if (command.command_name == "query_train") return handle_query_train(command, output);
        if (command.command_name == "query_ticket") return handle_query_ticket(command, output);
        if (command.command_name == "query_transfer") return handle_query_transfer(command, output);
        if (command.command_name == "buy_ticket") return handle_buy_ticket(command, output);
        if (command.command_name == "query_order") return handle_query_order(command, output);
        if (command.command_name == "refund_ticket") return handle_refund_ticket(command, output);

        if (command.command_name == "clean") {
            user_service_.clear();
            train_service_.clear();
            order_service_.clear();
            user_service_.init(".");
            train_service_.init(".");
            order_service_.init(".", &user_service_, &train_service_);
            push_success(output);
            return false;
        }
        if (command.command_name == "exit") {
            output.push_back("bye");
            return true;
        }
        push_failure(output);
        return false;
    }

    void TicketSystem::run() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                continue;
            }

            parsedCommand command;
            CommandParser::parsed(line, command);

            sjtu::vector<std::string> output;
            const bool should_exit = execute(command, output);
            if (!output.empty()) {
                std::cout << "[" << command.timestamp << "] " << output[0] << '\n';
                for (int i = 1; i < static_cast<int>(output.size()); ++i) {
                    std::cout << output[i] << '\n';
                }
                std::cout.flush();
            }
            if (should_exit) {
                break;
            }
        }
    }
}

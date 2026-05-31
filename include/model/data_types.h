//
// Created by lenovo on 2026/5/30.
//

#ifndef TICKET_SYSTEM_2026_1_DATA_TYPES_H
#define TICKET_SYSTEM_2026_1_DATA_TYPES_H

namespace sjtu {
    constexpr int kMaxUsernameLength = 20;
    constexpr int kMaxPasswordLength = 30;
    constexpr int kMaxNameBytes = 32;
    constexpr int kMaxMailLength = 30;
    constexpr int kMaxTrainIdLength = 20;
    constexpr int kMaxStationNum = 100;
    constexpr int kMaxSegmentNum = kMaxStationNum - 1;
    constexpr int kMaxStationNameBytes = 40;

    enum class TicketSortPolicy{  // -p cost time
        byTime,
        byCost
    };

    enum class OrderState {
        kSuccess,
        kPending,
        kRefunded
    };

    enum class BuyTicketState {
        Success,
        Pending,
        Failed
    };

    struct Date {
        int month = 0;
        int day = 0;
    };

    struct ClockTime {
        int hour = 0;
        int minute = 0;
    };

    struct DateTime {
        Date date;
        ClockTime time;
    };

    struct UserProfile {
        char username[kMaxNameBytes + 1]{};
        char password[kMaxPasswordLength + 1]{};
        char name[kMaxNameBytes + 1]{};
        char mailAddr[kMaxMailLength + 1]{};
        int privilege = 0;
        bool deleted = false;
    };

    struct TrainRecord {
        char trainID[kMaxTrainIdLength + 1]{};
        int stationNum = 0;
        char stations[kMaxStationNum][kMaxStationNameBytes + 1]{};
        int seatNum = 0;
        int prices[kMaxSegmentNum]{};
        int prefix_prices[kMaxSegmentNum]{}; //维护前缀和，用于查询两站之间车票价格
        ClockTime startTime;
        int travelTimes[kMaxSegmentNum]{};
        int stopoverTimes[kMaxSegmentNum - 1]{};
        int arrival_offsets[kMaxStationNum]{};  //到每站的时间
        int departure_offsets[kMaxStationNum]{};//从每站出发的时间
        int sale_begin = 0;
        int sale_end = 0;
        int start_time_minutes = 0;
        char type = 0;
        bool deleted = false;
        bool released = false;
    };


    struct SeatRecord {
        int train_offset = -1;
        int running_date = -1;
        int segment_num = 0;
        int remain[kMaxSegmentNum]{};
        bool deleted = false;
    };

    struct OrderRecord {
        char username[kMaxUsernameLength + 1]{};
        char trainID[kMaxTrainIdLength + 1]{};

        int timestamp = -1;
        int num = 0;
        int price = 0;
        DateTime time;
        int fromIndex = -1;
        int toIndex = -1;
        OrderState status = OrderState::kSuccess;
    };

    struct StationTrainRecord {  //给 query_ticket 和 query_transfer 用的辅助索引
        char station[kMaxStationNum + 1]{};
        char trainID[kMaxTrainIdLength + 1]{};
        int trainRecordId = -1;
        int stationIndex = -1;
    };

    struct ProfileUpdateRequest { //modify_profile
        bool change_password = false;
        bool change_name = false;
        bool change_mail = false;
        bool change_privilege = false;
        char password[kMaxPasswordLength + 1]{};
        char name[kMaxNameBytes + 1]{};
        char mail[kMaxMailLength + 1]{};
        int privilege = 0;
    };

    struct TrainStationView {
        char station_name[kMaxStationNameBytes + 1]{};
        DateTime arriving;
        DateTime leaving;
        int price_from_start = 0;
        int seat_to_next = 0;
        bool has_arriving_time = false;
        bool has_leaving_time = false;
        bool has_seat_to_next = false;
    };

    struct TrainQueryView { //query_train
        char train_id[kMaxTrainIdLength + 1]{};
        char type{};
        int station_num = 0;
        TrainStationView stations[kMaxStationNum];
    };

    struct TicketQueryRequest {
        char from[kMaxStationNameBytes + 1]{};
        char to[kMaxStationNameBytes + 1]{};
        Date departure_date;
        TicketSortPolicy sort_policy = TicketSortPolicy::byTime;
    };

    struct TicketQueryResult {
        char train_id[kMaxTrainIdLength + 1]{};
        char from[kMaxStationNameBytes + 1]{};
        char to[kMaxStationNameBytes + 1]{};
        DateTime leaving;
        DateTime arriving;
        int price = 0;
        int seat = 0;
        int total_time = 0;
    };

    struct TransferQueryResult {
        bool exists = false;
        TicketQueryResult first;
        TicketQueryResult second;
        int total_price = 0;
        int total_time = 0;
    };

    struct BuyTicketQuery {
        char username[kMaxUsernameLength + 1]{};
        char trainID[kMaxTrainIdLength + 1]{};
        Date departure_date;
        int ticketNum{};
        char from[kMaxStationNameBytes + 1]{};
        char to[kMaxStationNameBytes + 1]{};
        bool allow_queue{};
    };

    struct BuyTicketResult {
        BuyTicketState state = BuyTicketState::Failed;
        int total_price = 0;
        int order_offset = -1;
    };
}
#endif // TICKET_SYSTEM_2026_1_DATA_TYPES_H

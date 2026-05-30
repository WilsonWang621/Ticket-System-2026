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

    enum class TicketSOrtPolicy{  // -p cost time
        byTime,
        byCost
    };

    enum class State {
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
        Date sale_begin;
        Date sale_end;
        char type = 0;
        bool deleted = false;
        bool released = false;
    };


    struct SeatRecord {
        int trainId = -1;
        Date date;
        int remain[kMaxSegmentNum]{};
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

        State status = State::Success;
    };

    struct StationTrainRecord {  //给 query_ticket 和 query_transfer 用的辅助索引
        char station[kMaxStationNum + 1]{};
        char trainID[kMaxTrainIdLength + 1]{};
        int trainRecordId = -1;
        int stationIndex = -1;
    };

}
#endif // TICKET_SYSTEM_2026_1_DATA_TYPES_H

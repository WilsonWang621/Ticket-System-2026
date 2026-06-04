//
// Created by lenovo on 2026/5/30.
//

#ifndef TICKET_SYSTEM_2026_1_INTERNAL_UTILS_H
#define TICKET_SYSTEM_2026_1_INTERNAL_UTILS_H

#include <cstdio>
#include <string>
#include "../model/data_types.h"

namespace sjtu {

    constexpr int kMinutesPerDay = 24 * 60;
    constexpr int kIntMin = -2147483647 - 1;
    constexpr int kIntMax = 2147483647;

    inline std::string from_buffer(const char *buffer) {
        return {buffer};
    }

    inline void copy_to_buffer(const std::string &src, char* target, size_t capcity) {
        if (capcity == 0) return;
        size_t index = 0;
        while (index + 1 < capcity && index < src.size()) {
            target[index] = src[index];
            index++;
        }
        target[index] = '\0';
    }

    inline int month_days(int month) {
        static const int days[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        return days[month];
    }

    inline int date_to_ordinal(const Date& date) {
        int result = 0;
        for (int month = 1; month < date.month; month++) {
            result += month_days(month);
        }
        result += date.day - 1;
        return result;
    }

    inline Date ordinal_to_date(int ordinal) {
        Date result{};
        int month = 1;
        while (month <= 12) {
            const int days = month_days(month);
            if (ordinal < days) {
                result.month = month;
                result.day = ordinal + 1;
                return result;
            }
            ordinal -= days;
            ++month;
        }
        result.month = 12;
        result.day = 31;
        return result;
    }

    inline int time_to_minutes(const sjtu::ClockTime &time) {
        return time.hour * 60 + time.minute;
    }

    inline sjtu::ClockTime minutes_to_time(int total_minutes) {
        while (total_minutes < 0) {
            total_minutes += kMinutesPerDay;
        }
        total_minutes %= kMinutesPerDay;
        sjtu::ClockTime result{};
        result.hour = total_minutes / 60;
        result.minute = total_minutes % 60;
        return result;
    }

    inline long long to_absolute_minutes(int ordinal, int minute_of_day) {
        return static_cast<long long>(ordinal) * kMinutesPerDay + minute_of_day;
    }

    inline long long to_absolute_minutes(const sjtu::DateTime &time) {
        return to_absolute_minutes(date_to_ordinal(time.date), time_to_minutes(time.time));
    }

    inline DateTime absolute_minutes_to_datetime(long long absolute_minutes) {
        if (absolute_minutes < 0) {
            absolute_minutes = 0;
        }
        DateTime result{};
        const int ordinal = static_cast<int>(absolute_minutes / kMinutesPerDay);
        const int minute_of_day = static_cast<int>(absolute_minutes % kMinutesPerDay);
        result.date = ordinal_to_date(ordinal);
        result.time = minutes_to_time(minute_of_day);
        return result;
    }
}
#endif // TICKET_SYSTEM_2026_1_INTERNAL_UTILS_H

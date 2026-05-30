//
// Created by lenovo on 2026/5/30.
//

#ifndef TICKET_SYSTEM_2026_1_INTERNAL_UTILS_H
#define TICKET_SYSTEM_2026_1_INTERNAL_UTILS_H

#include <string>
#include "../model/data_types.h"

namespace sjtu {
    inline std::string from_buffer(const char *buffer) {
        return {buffer};
    }

    inline void copy_to_buffer(const std::string &src, char* target, std::size_t capcity) {
        if (capcity == 0) return;
        std::size_t index = 0;
        while (index + 1 < capcity && index < src.size()) {
            target[index] = src[index];
            index++;
        }
        target[index] = '\0';
    }


}
#endif // TICKET_SYSTEM_2026_1_INTERNAL_UTILS_H

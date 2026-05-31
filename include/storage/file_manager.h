//
// Created by lenovo on 2026/5/30.
//

#ifndef TICKET_SYSTEM_2026_1_FILE_MANEGER_H
#define TICKET_SYSTEM_2026_1_FILE_MANEGER_H

#include <fstream>
#include <string>

namespace sjtu {

    template <typename Record>
    class RecordFile {
    private:
        struct Header {
            int total_slots = 0;
            int active_count = 0;
            int free_head = -1;
        };

        struct Slot {
            int next_free;
            bool active;
            Record record;
        };

        mutable std::fstream file_;
        std::string file_name_;
        bool opened_ = false;
        Header header_;

        static Header default_header() {
            Header header{};
            header.total_slots = 0;
            header.active_count = 0;
            header.free_head = -1;
            return header;
        }

        std::streamoff slot_offset(int index) const {
            return static_cast<std::streamoff>(sizeof(Header)) + static_cast<std::streamoff>(sizeof(Slot)) * static_cast<std::streamoff>(index);
        }

        bool write_header() {
            if (!opened_ || !file_.is_open()) {
                return false;
            }
            file_.seekp(0, std::ios::beg);
            file_.write(reinterpret_cast<const char *>(&header_), sizeof(Header));
            file_.flush();
            return !file_.fail();
        }

        bool read_header() {  //最后要写入内存，不能是const
            if (!opened_ || !file_.is_open()) {
                return false;
            }
            file_.seekg(0, std::ios::beg);
            file_.read(reinterpret_cast<char*>(&header_), sizeof(Header));
            return !file_.fail();
        }

        bool write_slot(int index, Slot& slot) {
            if (!opened_ || !file_.is_open() || index < 0 || index >= header_.total_slots) {
                return false;
            }
            file_.seekp(slot_offset(index), std::ios::beg);
            file_.write(reinterpret_cast<const char*>(&slot), sizeof(Slot));
            return !file_.fail();
        }

        bool read_slot(int index, Slot& slot) const{
            if (!opened_ || !file_.is_open() || index < 0 || index >= header_.total_slots) {
                return false;
            }
            file_.seekg(slot_offset(index), std::ios::beg);
            file_.read(reinterpret_cast<char*>(&slot), sizeof(Slot));
            return !file_.fail();
        }

        bool create_empty_file() {
            std::fstream new_file(file_name_.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
            if (!new_file.is_open()) {
                return false;
            }
            header_ = default_header();
            new_file.write(reinterpret_cast<const char*>(&header_), sizeof(Header));
            new_file.close();
            return true;
        }

    public:
        RecordFile() : header_(default_header()) {}

        explicit RecordFile(const std::string &file_name):RecordFile() {
            open(file_name);
        }

        ~RecordFile() {
            close();
        }

        bool open(const std::string &file_name) {
            close();
            file_name_ = file_name;
            opened_ = true;

            file_.open(file_name_.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            if (!file_.is_open()) {
                if (!create_empty_file()) {
                    opened_ = false;
                    return false;
                }
                file_.open(file_name_.c_str(), std::ios::in | std::ios::out | std::ios::binary);
                if (!file_.is_open()) {
                    opened_ = false;
                    return false;
                }
            }

            file_.seekg(0, std::ios::end);
            if (file_.tellg() < static_cast<std::streamoff>(sizeof(Header))) {
                file_.close();
                if (!create_empty_file()) {
                    opened_ = false;
                    return false;
                }
                file_.open(file_name_.c_str(), std::ios::in | std::ios::out | std::ios::binary);
                if (!file_.is_open()) {
                    opened_ = false;
                    return false;
                }
            }
            return read_header();
        }

        void close() {
            if (opened_ && file_.is_open()) {
                write_header();
                file_.close();
            }
            opened_ = false;
            header_ = default_header();
        }

        void clear() {
            if (!opened_) return;
            if (file_.is_open()) {
                file_.close();
            }
            create_empty_file();
            file_.open(file_name_.c_str(), std::ios::in | std::ios::out | std::ios::binary);
            header_ = default_header();
        }

        int allocate() {
            if (!opened_ || !file_.is_open()) {
                return -1;
            }

            Slot slot{};
            slot.next_free = -1;
            slot.active = true;

            int index;
            if (header_.free_head != -1) { //有回收的空值slot
                index = header_.free_head;
                Slot recycled{};
                if (!read_slot(index, recycled)) {
                    return -1;
                }
                header_.free_head = recycled.next_free;
            }
            else {
                index = header_.total_slots++;
            }

            ++header_.active_count;
            if (!write_slot(index, slot)) {
                return -1;
            }
            if (!write_header()) {
                return -1;
            }
            return index;
        }

        bool recycle(int index) {
            Slot slot{};
            if (!read_slot(index, slot) || !slot.active) {
                return false;
            }
            slot.active = false;
            slot.next_free = header_.free_head;
            header_.free_head = index;
            --header_.active_count;
            if (!write_slot(index, slot)) {
                return false;
            }
            return write_header();
        }

        int append(const Record &record) {  // 添加
            int index = allocate();
            if (index == -1) {
                return -1;
            }
            Slot slot{};
            if (!read_slot(index, slot) || !slot.active) {
                return -1;
            }
            slot.record = record;

            if (!write_slot(index, slot)) {
                return -1;
            }
            return index;
        }

        bool read(int index, Record &record) const { //读到参数的引用里
            Slot slot{};
            if (!read_slot(index, slot) || slot.active == 0) {
                return false;
            }
            record = slot.record;
            return true;
        }

        bool write(int index, Record& record) {  //modify修改
            Slot slot;
            if (!read_slot(index, slot) || !slot.active) {
                return false;
            }
            slot.record = record;
            return write_slot(index, slot);
        }

        int record_count() const {
            return header_.active_count;
        }

        bool empty() const {
            return header_.active_count == 0;
        }
    };

}
#endif // TICKET_SYSTEM_2026_1_FILE_MANEGER_H

//
// Created by lenovo on 2026/5/29.
//

#ifndef TICKET_SYSTEM_2026_1_TRAIN_H
#define TICKET_SYSTEM_2026_1_TRAIN_H
#include <string>
#include "../model/data_types.h"
#include <vector>
#include <map>
#include "../storage/bpt.h"
#include "../storage/file_manager.h"

namespace sjtu {
    class TrainService {
    public:
        TrainService();
        ~TrainService();

        bool init(const std::string &data_dir);
        void clear();

        bool get_train(const std::string &train_id, sjtu::TrainRecord &train, int &train_offset) const;
        bool get_train_by_offset(int train_offset, sjtu::TrainRecord &train) const;
        bool get_existing_seat_record(int train_offset, int running_date, SeatRecord &seat_record, int &seat_offset) const;
        bool read_seat_record(int seat_offset, sjtu::SeatRecord &seat_record) const;
        bool write_seat_record(int seat_offset, SeatRecord &seat_record);
        static bool locate_station(const TrainRecord &train, const std::string &station_name, int &station_index) ;
        static int resolve_running_date(const TrainRecord &train, int station_index, const Date &station_departure_date) ;
        static int query_min_remaining_seat(const SeatRecord &seat_record, int from_index, int to_index) ;
        bool load_or_create_seat_record(int train_offset, int running_date, const sjtu::TrainRecord &train, sjtu::SeatRecord &seat_record, int &seat_offset);
        void apply_seat_delta(sjtu::SeatRecord &seat_record, int from_index, int to_index, int delta);

        bool add_train(const TrainRecord &train);
        bool delete_train(const std::string &trainID);
        bool release_train(const std::string& trainID);
        bool query_train(const std::string &train_id, const Date &date, TrainQueryView &result) const;
        bool query_ticket(const TicketQueryRequest &request,std::vector<TicketQueryResult> &results) const;
        bool query_transfer(const TicketQueryRequest &request, TransferQueryResult &result) const;

    private:
        std::string data_dir_;
        bool initialized_;
        RecordFile<TrainRecord> train_file_;
        RecordFile<SeatRecord> seat_file_;
        BPT<Data> *train_index_;
        BPT<Data> *station_index_;   //发布后可以在station_index_里面加入对应信息
        BPT<Data> *seat_index_;

        bool find_train_offset(const std::string &train_id, int &train_offset) const;
        static int pack_station_entry(int train_offset, int station_index);  //加密
        static int unpack_train_offset(int packed_value);   //解密offset
        static int unpack_station_index(int packed_value);  //解码index
        static std::string make_seat_key(int train_offset, int running_date);//
        void reset_index_files(); //完全清空并重建 B+ 树索引系统

    };
}
#endif // TICKET_SYSTEM_2026_1_TRAIN_H

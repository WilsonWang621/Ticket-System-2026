1.
struct UserRecord {
char username[21];
char password[31];
char name[16];      // 2~5 个汉字，UTF-8 最多 15 字节左右，留 16
char mail[31];
int privilege;
};

RecordFile<UserRecord> user_file;
BPT<Data> user_index;

username -> user_record_id

add_user
login
query_profile
modify_profile

2.
struct TrainRecord{};

RecordFile<TrainRecord> train_file;
BPT<Data> train_index;

trainID -> train_record_id

add_train
delete_train
release_train
query_train
query_ticket
query_transfer
buy_ticket
refund_ticket
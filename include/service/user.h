#ifndef TICKET_SYSTEM_2026_ACCOUNT_H
#define TICKET_SYSTEM_2026_ACCOUNT_H

#include <string>
#include "../model/data_types.h"
#include <map>
#include "../storage/bpt.h"
#include "../storage/file_manager.h"

namespace sjtu {
    class UserService {
    public:
        UserService();
        ~UserService();

        bool init(const std::string &data_dir);
        void clear();
        void clear_runtime_state();

        bool is_logged_in(const std::string &username) const;
        bool get_user(const std::string &username, UserProfile &user) const;
        int get_privilege(const std::string &username) const;
        bool user_exists(const std::string &username) const;

        bool add_user(const std::string &current_username, UserProfile &new_user);
        bool login(const std::string &username, const std::string &password);
        bool logout(const std::string &username);
        bool query_profile(const std::string& current_username, const std::string& username, UserProfile &result) const;
        bool modify_profile(const std::string &current_username,const std::string &target_username,
                        const ProfileUpdateRequest &request, UserProfile &result);
    private:
        std::string data_dir_;
        bool initialized_;
        RecordFile<UserProfile> user_file_; //offset -> UserRecord
        BPT<Data> *user_index_; // username_hash -> offset
        std::map<std::string, bool> logged_in_; //登陆过哪些

        bool find_user_offset(const std::string &username, int &offset) const;
        void reset_index_file();
    };
}

#endif // TICKET_SYSTEM_2026_ACCOUNT_H

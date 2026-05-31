//
// Created by lenovo on 2026/5/29.
//
#include <../include/service/user.h>
#include <../include/util/internal_utils.h>
#include <climits>

namespace {
    unsigned long long hash_key(const std::string &text) {
        Data data(text, 0);
        return data.key;
    }
}

namespace sjtu {
    UserService::UserService() : initialized_(false), user_index_(nullptr) {}

     UserService::~UserService() {
        delete user_index_;
        user_index_ = nullptr;
    }

    bool UserService::init(const std::string &data_dir) {
        data_dir_ = data_dir;
        initialized_ = user_file_.open("ts_users.dat");
        delete user_index_;
        user_index_ = new BPT<Data>("ts_user_index");
        logged_in_.clear();
        return initialized_;
    }

    void UserService::reset_index_file() {
        delete user_index_;
        user_index_ = nullptr;
        std::remove("init_ts_user_index");
        std::remove("data_ts_user_index");
        user_index_ = new BPT<Data>("ts_user_index");
    }

    void UserService::clear() {
        if (!initialized_) {
            return;
        }
        user_file_.clear();
        reset_index_file();
        logged_in_.clear();
    }

    void UserService::clear_runtime_state() {
        logged_in_.clear();
    }

    bool UserService::find_user_offset(const std::string& username, int& offset) const {
        if (user_index_ == nullptr) {
            return false;
        }
        Data probe(username, INT_MIN);
        Data result;
        if (!user_index_->lower_bound(probe, result)) {
            return false;
        }
        if (result.key != hash_key(username)) {
            return false;
        }
        offset = result.value;
        return true;
    }

    bool UserService::get_user(const std::string& username, UserProfile& user) const {
        int offset = -1;
        if (!find_user_offset(username, offset)) {
            return false;
        }
        return user_file_.read(offset, user);
    }

    bool UserService::is_logged_in(const std::string& username) const {
        return logged_in_.find(username) != logged_in_.end();
    }

    int UserService::get_privilege(const std::string& username) const {
        UserProfile user;
        if (!get_user(username, user)) {
            return -1;
        }
        return user.privilege;
    }

    bool UserService::user_exists(const std::string& username) const {
        UserProfile user;
        return get_user(username, user);
    }

    bool UserService::add_user(const std::string& current_username, UserProfile& new_user) {
        int discarded_offset = -1;
        if (find_user_offset(from_buffer(new_user.username), discarded_offset)) {
            return false;
        }

        UserProfile record = new_user;
        if (user_file_.empty()) {
            record.privilege = 10;
        }
        else {
            if (!is_logged_in(current_username)) {
                return false;
            }
            const int cur_privilege = get_privilege(current_username);
            if (cur_privilege <= record.privilege) {
                return false;
            }
        }
        const int offset = user_file_.append(record);
        if (offset == -1) {
            return false;
        }
        user_index_->add(Data(from_buffer(record.username), offset));
        return true;
    }

    bool UserService::login(const std::string& username, const std::string& password) {
        if (is_logged_in(username)) {
            return false;
        }
        UserProfile user;
        if (!get_user(username, user)) {
            return false;
        }
        if (from_buffer(user.password) != password) {
            return false;
        }
        logged_in_.insert({username, true});
        return true;
    }

    bool UserService::logout(const std::string& username) {
        if (logged_in_.find(username) == logged_in_.end()) {
            return false;
        }
        logged_in_.erase(logged_in_.find(username));
        return true;
    }

    bool UserService::query_profile(const std::string& current_username, const std::string& username, UserProfile &result) const {
        if (!is_logged_in(current_username)) {
            return false;
        }
        UserProfile current;
        if (!get_user(current_username, current)) {
            return false;
        }
        UserProfile target;
        if (!get_user(username, target)) {
            return false;
        }
        if (current_username != username) {
            const int cur_privilege = get_privilege(current_username);
            if (target.privilege >= cur_privilege) {
                return false;
            }
        }
        result = target;
        return true;
    }


    bool UserService::modify_profile(const std::string& current_username, const std::string& target_username,
                                     const ProfileUpdateRequest& request, UserProfile& result) {
        if (!is_logged_in(current_username)) {
            return false;
        }

        int offset = -1;
        if (!find_user_offset(target_username, offset)) {
            return false;
        }

        UserProfile target;
        if (!user_file_.read(offset, target)) {
            return false;
        }
        const int current_privilege = get_privilege(current_username);
        if (current_username != target_username && current_privilege <= target.privilege) {
            return false;
        }
        if (request.change_privilege && current_privilege <= request.privilege) {
            return false;
        }

        if (request.change_password) {
            copy_to_buffer(from_buffer(request.password), target.password, sizeof(target.password));
        }
        if (request.change_name) {
            copy_to_buffer(from_buffer(request.name), target.name, sizeof(target.name));
        }
        if (request.change_mail) {
            copy_to_buffer(from_buffer(request.mail), target.mailAddr, sizeof(target.mailAddr));
        }
        if (request.change_privilege) {
            target.privilege = request.privilege;
        }

        if (!user_file_.write(offset, target)) {
            return false;
        }
        result = target;
        return true;
    }

}


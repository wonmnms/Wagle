#pragma once
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "chat/user.h"
#include "protocol/message.h"

namespace wagle {

class ChatRoom {
   public:
    ChatRoom(const std::string& id, const std::string& name,
             bool is_private = false);

    // 사용자 입장
    void join(std::shared_ptr<User> user);

    // 사용자 퇴장
    void leave(std::shared_ptr<User> user);

    // 모든 사용자에게 메시지 전송
    void broadcast(const Message& msg);

    // Getters
    std::string getId() const { return id_; }
    std::string getName() const { return name_; }
    bool isPrivate() const { return is_private_; }
    size_t getMaxUsers() const { return max_users_; }
    time_t getCreatedTime() const { return created_time_; }
    const std::deque<Message>& getRecentMessages() const {
        return recent_messages_;
    }
    size_t getUserCount() const { return users_.size(); }

    // Setters
    void setMaxUsers(size_t max) { max_users_ = max; }
    void setPrivate(bool is_private) { is_private_ = is_private; }

    // 사용자 수 업데이트 메시지 전송
    void broadcastUserCount();

   private:
    std::string id_;       // 채팅방 고유 ID
    std::string name_;     // 채팅방 이름
    bool is_private_;      // 비공개 채팅방 여부
    size_t max_users_;     // 최대 사용자 수
    time_t created_time_;  // 생성 시간
    std::set<std::shared_ptr<User>> users_;
    std::deque<Message> recent_messages_;
    mutable std::mutex mutex_;  // 스레드 안전성을 위한 뮤텍스
    static const size_t MAX_RECENT_MESSAGES = 100;
};

}  // namespace wagle
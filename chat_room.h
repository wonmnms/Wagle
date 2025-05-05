#pragma once
#include <string>
#include <set>
#include <deque>
#include <memory>
#include "protocol/message.h"
#include "chat/user.h"

namespace wagle {

class ChatRoom {
public:
    // 사용자 입장
    bool join(std::shared_ptr<User> user);
    
    // 사용자 퇴장
    void leave(std::shared_ptr<User> user);
    
    // 모든 사용자에게 메시지 전송
    void broadcast(const Message& msg);
    
    // 최근 메시지 가져오기
    const std::deque<Message>& getRecentMessages() const { return recent_messages_; }
    
    bool isNameTaken(const std::string& name);  // 중복 닉네임 검사 함수 선언

private:
    std::set<std::shared_ptr<User>> users_;
    std::deque<Message> recent_messages_;
    static const size_t MAX_RECENT_MESSAGES = 100;
};

} // namespace wagle
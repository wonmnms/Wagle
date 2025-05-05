#include "chat/chat_room.h"

#include <boost/asio.hpp>
#include <iostream>

namespace wagle {

    bool ChatRoom::join(std::shared_ptr<User> user) {
        // 1. 중복 닉네임 검사
        if (isNameTaken(user->getName())) {
            Message reject_msg(MessageType::REJECT_DUPLICATE_NAME, "SERVER",
                "Nickname already in use.");
            boost::asio::write(user->getSocket(),
                boost::asio::buffer(reject_msg.serialize()));
            return false;
        }

    // 사용자 추가
    users_.insert(user);

    // 사용자 입장 메시지
    Message join_msg(MessageType::CONNECT, "SERVER",
                     user->getName() + " has joined the chat.");
    broadcast(join_msg);

    // 최근 메시지 전송
    for (const auto& msg : recent_messages_) {
        boost::asio::write(user->getSocket(),
                           boost::asio::buffer(msg.serialize()));
    }
}

void ChatRoom::leave(std::shared_ptr<User> user) {
    // 사용자 제거
    users_.erase(user);

    // 사용자 퇴장 메시지
    Message leave_msg(MessageType::DISCONNECT, "SERVER",
                      user->getName() + " has left the chat.");
    broadcast(leave_msg);
}

void ChatRoom::broadcast(const Message& msg) {
    // 메시지 저장
    recent_messages_.push_back(msg);

    // 최대 메시지 수 유지
    while (recent_messages_.size() > MAX_RECENT_MESSAGES) {
        recent_messages_.pop_front();
    }

    // 모든 사용자에게 전송
    std::string serialized_msg = msg.serialize();
    for (auto& user : users_) {
        try {
            boost::asio::write(user->getSocket(),
                               boost::asio::buffer(serialized_msg));
        } catch (std::exception& e) {
            std::cerr << "Error broadcasting message: " << e.what()
                      << std::endl;
        }
    }
}

}  // namespace wagle
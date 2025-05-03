#include "chat/chat_room.h"

#include <boost/asio.hpp>
#include <iostream>

namespace wagle {

void ChatRoom::join(std::shared_ptr<User> user) {
    // 사용자 추가
    users_.insert(user);

    // 사용자 입장 메시지 생성
    Message join_msg(MessageType::CONNECT, "SERVER",
                     user->getName() + " has joined the chat.");

    // 다른 사용자들에게만 입장 메시지 전송 (본인 제외)
    for (auto& other_user : users_) {
        if (other_user !=
            user) {  // 본인을 제외한 다른 사용자들에게만 메시지 전송
            try {
                boost::asio::write(other_user->getSocket(),
                                   boost::asio::buffer(join_msg.serialize()));
            } catch (std::exception& e) {
                std::cerr << "Error sending join message: " << e.what()
                          << std::endl;
            }
        }
    }

    // 메시지 저장 (이후 입장하는 사용자들이 볼 수 있게)
    recent_messages_.push_back(join_msg);
    while (recent_messages_.size() > MAX_RECENT_MESSAGES) {
        recent_messages_.pop_front();
    }

    // 최근 메시지 전송 (새로 입장한 사용자에게)
    for (const auto& msg : recent_messages_) {
        // 본인의 입장 메시지는 제외하고 전송
        if (!(msg.getType() == MessageType::CONNECT &&
              msg.getContent() == user->getName() + " has joined the chat.")) {
            boost::asio::write(user->getSocket(),
                               boost::asio::buffer(msg.serialize()));
        }
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
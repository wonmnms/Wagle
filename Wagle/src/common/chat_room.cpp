#include "chat/chat_room.h"

#include <boost/asio.hpp>
#include <ctime>
#include <iostream>
#include <mutex>

namespace wagle {

// 뮤텍스 추가 - 여러 스레드에서 동시에 채팅방에 접근할 때 데이터 보호
std::mutex chat_room_mutex;

ChatRoom::ChatRoom(const std::string& id, const std::string& name,
                   bool is_private)
    : id_(id),
      name_(name),
      is_private_(is_private),
      max_users_(0),
      created_time_(std::time(nullptr)) {}

void ChatRoom::join(std::shared_ptr<User> user) {
    std::unique_lock<std::mutex> lock(mutex_);

    // 최대 사용자 수 체크
    if (max_users_ > 0 && users_.size() >= max_users_) {
        throw std::runtime_error("Chat room is full");
    }

    // 사용자 추가
    users_.insert(user);

    // 사용자 입장 메시지 생성
    Message join_msg(MessageType::CONNECT, "SERVER",
                     user->getName() + " has joined the chat.");
    join_msg.setRoomId(id_);

    // 다른 사용자들에게만 입장 메시지 전송 (본인 제외)
    for (auto& other_user : users_) {
        if (other_user != user) {
            try {
                boost::asio::write(other_user->getSocket(),
                                   boost::asio::buffer(join_msg.serialize()));
            } catch (std::exception& e) {
                std::cerr << "Error sending join message: " << e.what()
                          << std::endl;
            }
        }
    }

    // 메시지 저장
    recent_messages_.push_back(join_msg);
    while (recent_messages_.size() > MAX_RECENT_MESSAGES) {
        recent_messages_.pop_front();
    }

    // 락 해제
    lock.unlock();

    // 최근 메시지 전송
    {
        std::unique_lock<std::mutex> read_lock(mutex_);
        for (const auto& msg : recent_messages_) {
            if (!(msg.getType() == MessageType::CONNECT &&
                  msg.getContent() ==
                      user->getName() + " has joined the chat.")) {
                try {
                    boost::asio::write(user->getSocket(),
                                       boost::asio::buffer(msg.serialize()));
                } catch (std::exception& e) {
                    std::cerr << "Error sending recent message: " << e.what()
                              << std::endl;
                }
            }
        }
    }

    // 사용자 수 업데이트 브로드캐스트
    broadcastUserCount();
}

void ChatRoom::leave(std::shared_ptr<User> user) {
    std::unique_lock<std::mutex> lock(mutex_);

    // 이미 채팅방에 없는 경우 무시
    if (users_.find(user) == users_.end()) {
        return;
    }

    // 사용자 제거
    users_.erase(user);
    lock.unlock();

    // 사용자 퇴장 메시지
    Message leave_msg(MessageType::DISCONNECT, "SERVER",
                      user->getName() + " has left the chat.");
    leave_msg.setRoomId(id_);
    broadcast(leave_msg);

    // 사용자 수 업데이트 브로드캐스트
    broadcastUserCount();
}

void ChatRoom::broadcast(const Message& msg) {
    std::unique_lock<std::mutex> lock(mutex_);

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

void ChatRoom::broadcastUserCount() {
    std::unique_lock<std::mutex> lock(mutex_);

    // 사용자 수 메시지 생성
    Message count_msg(MessageType::USER_COUNT, "SERVER",
                      std::to_string(users_.size()));
    count_msg.setRoomId(id_);

    // 모든 사용자에게 전송
    std::string serialized_msg = count_msg.serialize();
    for (auto& user : users_) {
        try {
            boost::asio::write(user->getSocket(),
                               boost::asio::buffer(serialized_msg));
        } catch (std::exception& e) {
            std::cerr << "Error broadcasting user count: " << e.what()
                      << std::endl;
        }
    }
}

}  // namespace wagle
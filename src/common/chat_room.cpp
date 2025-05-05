#include "chat/chat_room.h"
#include <iostream>
#include <boost/asio.hpp>
#include <mutex>

namespace wagle {

// 뮤텍스 추가 - 여러 스레드에서 동시에 채팅방에 접근할 때 데이터 보호
std::mutex chat_room_mutex;

void ChatRoom::join(std::shared_ptr<User> user) {
    std::unique_lock<std::mutex> lock(chat_room_mutex);
    
    // 사용자 추가
    users_.insert(user);
    
    // 사용자 입장 메시지 생성
    Message join_msg(MessageType::CONNECT, "SERVER", user->getName() + " has joined the chat.");
    
    // 다른 사용자들에게만 입장 메시지 전송 (본인 제외)
    for (auto& other_user : users_) {
        if (other_user != user) {  // 본인을 제외한 다른 사용자들에게만 메시지 전송
            try {
                boost::asio::write(other_user->getSocket(), boost::asio::buffer(join_msg.serialize()));
            } catch (std::exception& e) {
                std::cerr << "Error sending join message: " << e.what() << std::endl;
            }
        }
    }
    
    // 메시지 저장 (이후 입장하는 사용자들이 볼 수 있게)
    recent_messages_.push_back(join_msg);
    while (recent_messages_.size() > MAX_RECENT_MESSAGES) {
        recent_messages_.pop_front();
    }
    
    // 락 잠시 해제 - 최근 메시지 전송 중 데드락 방지
    lock.unlock();
    
    // 최근 메시지 전송 (새로 입장한 사용자에게)
    // 새로운 락 범위 시작
    {
        std::unique_lock<std::mutex> read_lock(chat_room_mutex);
        for (const auto& msg : recent_messages_) {
            // 본인의 입장 메시지는 제외하고 전송
            if (!(msg.getType() == MessageType::CONNECT && 
                msg.getContent() == user->getName() + " has joined the chat.")) {
                try {
                    boost::asio::write(user->getSocket(), boost::asio::buffer(msg.serialize()));
                } catch (std::exception& e) {
                    std::cerr << "Error sending recent message: " << e.what() << std::endl;
                }
            }
        }
    }
    
    // 사용자 수 업데이트 브로드캐스트
    broadcastUserCount();
}

void ChatRoom::leave(std::shared_ptr<User> user) {
    std::unique_lock<std::mutex> lock(chat_room_mutex);
    
    // 이미 채팅방에 없는 경우 무시
    if (users_.find(user) == users_.end()) {
        return;
    }
    
    // 사용자 제거
    users_.erase(user);
    lock.unlock();  // 락 해제 - broadcast가 내부적으로 락을 획득하므로
    
    // 사용자 퇴장 메시지
    Message leave_msg(MessageType::DISCONNECT, "SERVER", user->getName() + " has left the chat.");
    broadcast(leave_msg);
    
    // 사용자 수 업데이트 브로드캐스트
    broadcastUserCount();
}

void ChatRoom::broadcast(const Message& msg) {
    std::unique_lock<std::mutex> lock(chat_room_mutex);
    
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
            boost::asio::write(user->getSocket(), boost::asio::buffer(serialized_msg));
        } catch (std::exception& e) {
            std::cerr << "Error broadcasting message: " << e.what() << std::endl;
            // 오류 발생 시 로그만 기록하고 계속 진행 - 연결 끊어진 사용자는 다른 부분에서 처리됨
        }
    }
}

void ChatRoom::broadcastUserCount() {
    std::unique_lock<std::mutex> lock(chat_room_mutex);
    
    // 사용자 수 메시지 생성
    Message count_msg(MessageType::USER_COUNT, "SERVER", std::to_string(users_.size()));
    
    // 모든 사용자에게 전송
    std::string serialized_msg = count_msg.serialize();
    for (auto& user : users_) {
        try {
            boost::asio::write(user->getSocket(), boost::asio::buffer(serialized_msg));
        } catch (std::exception& e) {
            std::cerr << "Error broadcasting user count: " << e.what() << std::endl;
        }
    }
}

} // namespace wagle
#pragma once
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include "chat/chat_room.h"
#include "chat/user.h"

namespace wagle {

class ChatRoomManager {
   public:
    // 채팅방 관리
    std::shared_ptr<ChatRoom> createRoom(const std::string& name,
                                         bool is_private = false);
    bool deleteRoom(const std::string& room_id);
    std::shared_ptr<ChatRoom> getRoom(const std::string& room_id);

    // 채팅방 목록 관리
    std::vector<std::shared_ptr<ChatRoom>> getPublicRooms() const;
    std::vector<std::shared_ptr<ChatRoom>> getUserRooms(
        const std::string& username) const;

    // 사용자 관리
    bool addUserToRoom(const std::string& room_id, std::shared_ptr<User> user);
    bool removeUserFromRoom(const std::string& room_id,
                            std::shared_ptr<User> user);

   private:
    std::unordered_map<std::string, std::shared_ptr<ChatRoom>> rooms_;
    std::unordered_map<std::string, std::set<std::string>>
        user_rooms_;  // username -> room_ids
    mutable std::mutex rooms_mutex_;
    mutable std::mutex user_rooms_mutex_;

    // 내부 유틸리티 함수
    std::string generateRoomId() const;
};

}  // namespace wagle
#include "chat/chat_room_manager.h"

namespace wagle {

const std::string ChatRoomManager::DEFAULT_ROOM_NAME = "General";

ChatRoomManager::ChatRoomManager() {
    // 기본 채팅방 생성
    rooms_[DEFAULT_ROOM_NAME] = std::make_shared<ChatRoom>();
}

bool ChatRoomManager::createRoom(const std::string& room_name) {
    std::unique_lock<std::mutex> lock(rooms_mutex_);
    
    // 빈 이름이거나 이미 존재하는 방이면 실패
    if (room_name.empty() || rooms_.find(room_name) != rooms_.end()) {
        return false;
    }
    
    // 새 채팅방 생성
    rooms_[room_name] = std::make_shared<ChatRoom>();
    return true;
}

std::shared_ptr<ChatRoom> ChatRoomManager::getRoom(const std::string& room_name) {
    std::unique_lock<std::mutex> lock(rooms_mutex_);
    
    auto it = rooms_.find(room_name);
    if (it != rooms_.end()) {
        return it->second;
    }
    
    return nullptr;
}

std::vector<ChatRoomInfo> ChatRoomManager::getRoomList() const {
    std::unique_lock<std::mutex> lock(rooms_mutex_);
    
    std::vector<ChatRoomInfo> room_list;
    for (const auto& room_pair : rooms_) {
        bool is_default = (room_pair.first == DEFAULT_ROOM_NAME);
        room_list.emplace_back(room_pair.first, room_pair.second->getUserCount(), is_default);
    }
    
    return room_list;
}

bool ChatRoomManager::deleteRoom(const std::string& room_name) {
    std::unique_lock<std::mutex> lock(rooms_mutex_);
    
    // 기본 방은 삭제 불가
    if (room_name == DEFAULT_ROOM_NAME) {
        return false;
    }
    
    auto it = rooms_.find(room_name);
    if (it != rooms_.end()) {
        // 방에 사용자가 있으면 삭제 불가
        if (it->second->getUserCount() > 0) {
            return false;
        }
        
        rooms_.erase(it);
        return true;
    }
    
    return false;
}

bool ChatRoomManager::roomExists(const std::string& room_name) const {
    std::unique_lock<std::mutex> lock(rooms_mutex_);
    return rooms_.find(room_name) != rooms_.end();
}

} // namespace wagle
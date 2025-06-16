#include "chat/chat_room_manager.h"

#include <algorithm>
#include <chrono>
#include <random>

namespace wagle {

std::string ChatRoomManager::generateRoomId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex = "0123456789abcdef";

    std::string id;
    id.reserve(8);
    for (int i = 0; i < 8; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

std::shared_ptr<ChatRoom> ChatRoomManager::createRoom(const std::string& name,
                                                      bool is_private) {
    std::string room_id = generateRoomId();
    auto room = std::make_shared<ChatRoom>(room_id, name, is_private);

    std::lock_guard<std::mutex> lock(rooms_mutex_);
    rooms_[room_id] = room;
    return room;
}

bool ChatRoomManager::deleteRoom(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    return rooms_.erase(room_id) > 0;
}

std::shared_ptr<ChatRoom> ChatRoomManager::getRoom(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    auto it = rooms_.find(room_id);
    return (it != rooms_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<ChatRoom>> ChatRoomManager::getPublicRooms() const {
    std::lock_guard<std::mutex> lock(rooms_mutex_);
    std::vector<std::shared_ptr<ChatRoom>> public_rooms;

    for (const auto& pair : rooms_) {
        if (!pair.second->isPrivate()) {
            public_rooms.push_back(pair.second);
        }
    }

    return public_rooms;
}

std::vector<std::shared_ptr<ChatRoom>> ChatRoomManager::getUserRooms(
    const std::string& username) const {
    std::vector<std::shared_ptr<ChatRoom>> user_rooms;

    {
        std::lock_guard<std::mutex> lock(user_rooms_mutex_);
        auto it = user_rooms_.find(username);
        if (it == user_rooms_.end()) {
            return user_rooms;
        }

        std::lock_guard<std::mutex> rooms_lock(rooms_mutex_);
        for (const auto& room_id : it->second) {
            auto room_it = rooms_.find(room_id);
            if (room_it != rooms_.end()) {
                user_rooms.push_back(room_it->second);
            }
        }
    }

    return user_rooms;
}

bool ChatRoomManager::addUserToRoom(const std::string& room_id,
                                    std::shared_ptr<User> user) {
    auto room = getRoom(room_id);
    if (!room) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(user_rooms_mutex_);
        user_rooms_[user->getName()].insert(room_id);
    }

    room->join(user);
    return true;
}

bool ChatRoomManager::removeUserFromRoom(const std::string& room_id,
                                         std::shared_ptr<User> user) {
    auto room = getRoom(room_id);
    if (!room) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(user_rooms_mutex_);
        auto it = user_rooms_.find(user->getName());
        if (it != user_rooms_.end()) {
            it->second.erase(room_id);
            if (it->second.empty()) {
                user_rooms_.erase(it);
            }
        }
    }

    room->leave(user);
    return true;
}

}  // namespace wagle
#pragma once
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "chat/chat_room.h"

namespace wagle {

struct ChatRoomInfo {
    std::string name;
    size_t user_count;
    bool is_default;  // 기본 방인지 (삭제 불가)
    
    ChatRoomInfo(const std::string& n, size_t count, bool def = false)
        : name(n), user_count(count), is_default(def) {}
};

class ChatRoomManager {
public:
    ChatRoomManager();
    
    // 채팅방 생성
    bool createRoom(const std::string& room_name);
    
    // 채팅방 가져오기
    std::shared_ptr<ChatRoom> getRoom(const std::string& room_name);
    
    // 채팅방 목록 가져오기
    std::vector<ChatRoomInfo> getRoomList() const;
    
    // 채팅방 삭제 (기본 방은 삭제 불가)
    bool deleteRoom(const std::string& room_name);
    
    // 채팅방 존재 여부 확인
    bool roomExists(const std::string& room_name) const;
    
private:
    mutable std::mutex rooms_mutex_;
    std::map<std::string, std::shared_ptr<ChatRoom>> rooms_;
    static const std::string DEFAULT_ROOM_NAME;
};

} // namespace wagle
#pragma once
#include <string>
#include <vector>

namespace wagle {

// 메시지 타입 정의
enum class MessageType {
    CONNECT,         // 사용자 연결
    DISCONNECT,      // 사용자 연결 해제
    CHAT_MSG,        // 채팅 메시지
    USER_LIST,       // 사용자 목록
    USER_COUNT,      // 사용자 수
    USERNAME_ERROR,  // 사용자 이름 오류
    ROOM_CREATE,     // 채팅방 생성
    ROOM_DELETE,     // 채팅방 삭제
    ROOM_JOIN,       // 채팅방 참여
    ROOM_LEAVE,      // 채팅방 퇴장
    ROOM_LIST,       // 채팅방 목록 요청
    ROOM_INFO,       // 채팅방 정보
    ROOM_MESSAGE,    // 채팅방 메시지
    ROOM_ERROR       // 채팅방 관련 에러
};

// 메시지 클래스
class Message {
   public:
    Message() = default;
    Message(MessageType type, const std::string& content);
    Message(MessageType type, const std::string& sender,
            const std::string& content);
    Message(MessageType type, const std::string& sender,
            const std::string& content, const std::string& room_id);

    // 직렬화: 메시지를 문자열로 변환
    std::string serialize() const;

    // 역직렬화: 문자열을 메시지로 변환
    static Message deserialize(const std::string& data);

    // UTF-8 문자열의 바이트 길이 계산
    static std::size_t utf8Length(const std::string& str);

    // 특수 문자 이스케이프/언이스케이프 처리
    std::string escapeSpecialChars(const std::string& str) const;
    std::string unescapeSpecialChars(const std::string& str) const;

    // Getters
    MessageType getType() const { return type_; }
    std::string getSender() const { return sender_; }
    std::string getContent() const { return content_; }
    std::string getRoomId() const { return room_id_; }

    // Setters
    void setType(MessageType type) { type_ = type; }
    void setSender(const std::string& sender) { sender_ = sender; }
    void setContent(const std::string& content) { content_ = content; }
    void setRoomId(const std::string& room_id) { room_id_ = room_id; }

   private:
    MessageType type_ = MessageType::CHAT_MSG;
    std::string sender_;
    std::string content_;
    std::string room_id_;  // 채팅방 ID
};

}  // namespace wagle
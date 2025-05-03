#pragma once
#include <string>
#include <vector>

namespace wagle {

// MessageType에 USER_COUNT 추가
enum class MessageType {
    CONNECT,    // 사용자 연결
    DISCONNECT, // 사용자 연결 해제
    CHAT_MSG,   // 채팅 메시지
    USER_LIST,  // 사용자 목록
    USER_COUNT  // 사용자 수 업데이트
};

// 메시지 클래스
class Message {
   public:
    Message() = default;
    Message(MessageType type, const std::string& content);
    Message(MessageType type, const std::string& sender,
            const std::string& content);

    // 직렬화: 메시지를 문자열로 변환
    std::string serialize() const;

    // 역직렬화: 문자열을 메시지로 변환
    static Message deserialize(const std::string& data);

    MessageType getType() const { return type_; }
    std::string getSender() const { return sender_; }
    std::string getContent() const { return content_; }

   private:
    MessageType type_ = MessageType::CHAT_MSG;
    std::string sender_;
    std::string content_;
};

}  // namespace wagle
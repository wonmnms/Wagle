#include "protocol/message.h"

#include <cstring>
#include <iostream>
#include <sstream>

namespace wagle {

Message::Message(MessageType type, const std::string& content)
    : type_(type), content_(content) {}

Message::Message(MessageType type, const std::string& sender,
                 const std::string& content)
    : type_(type), sender_(sender), content_(content) {}

Message::Message(MessageType type, const std::string& sender,
                 const std::string& content, const std::string& room_id)
    : type_(type), sender_(sender), content_(content), room_id_(room_id) {}

std::string Message::serialize() const {
    std::stringstream ss;

    // 메시지 형식: <타입>:<발신자>:<내용>:<채팅방ID>
    std::string safe_sender = escapeSpecialChars(sender_);
    std::string safe_content = escapeSpecialChars(content_);
    std::string safe_room_id = escapeSpecialChars(room_id_);

    int type_int = static_cast<int>(type_);
    ss << type_int << ":" << safe_sender << ":" << safe_content << ":"
       << safe_room_id << "\n";

    return ss.str();
}

// 직렬화 위해 콜론 등의 특수문자 이스케이프 처리
std::string Message::escapeSpecialChars(const std::string& str) const {
    std::string result = str;
    size_t pos = 0;

    // 콜론을 유니코드 대체 문자로 변경 (예: ˸)
    while ((pos = result.find(':', pos)) != std::string::npos) {
        result.replace(pos, 1, "\xCB\xB8");
        pos += 2;  // 2바이트 유니코드 문자
    }

    return result;
}

// 직렬화된 문자열에서 특수문자 원복
std::string Message::unescapeSpecialChars(const std::string& str) const {
    std::string result = str;
    size_t pos = 0;

    // 유니코드 대체 문자를 콜론으로 복원
    while ((pos = result.find("\xCB\xB8", pos)) != std::string::npos) {
        result.replace(pos, 2, ":");
        pos += 1;  // 1바이트 문자로 변경됨
    }

    return result;
}

Message Message::deserialize(const std::string& data) {
    Message msg;
    std::stringstream ss(data);
    std::string type_str, sender, content, room_id;

    // 타입 파싱
    std::getline(ss, type_str, ':');
    msg.type_ = static_cast<MessageType>(std::stoi(type_str));

    // 발신자 파싱
    std::getline(ss, sender, ':');
    msg.sender_ = msg.unescapeSpecialChars(sender);

    // 내용 파싱
    std::getline(ss, content, ':');
    msg.content_ = msg.unescapeSpecialChars(content);

    // 채팅방 ID 파싱 (있는 경우)
    if (std::getline(ss, room_id, ':')) {
        msg.room_id_ = msg.unescapeSpecialChars(room_id);
    }

    return msg;
}

std::size_t Message::utf8Length(const std::string& str) {
    std::size_t length = 0;
    for (unsigned char c : str) {
        if ((c & 0xC0) != 0x80) {
            length++;
        }
    }
    return length;
}

}  // namespace wagle
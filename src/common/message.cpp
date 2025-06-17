#include "protocol/message.h"

#include <iostream>
#include <sstream>
#include <cstring>

namespace wagle {

Message::Message(MessageType type, const std::string& content)
    : type_(type), content_(content) {}

Message::Message(MessageType type, const std::string& sender,
                 const std::string& content)
    : type_(type), sender_(sender), content_(content) {}

Message::Message(MessageType type, const std::string& sender,
                 const std::string& content, const std::string& room_name)
    : type_(type), sender_(sender), content_(content), room_name_(room_name) {}

std::string Message::serialize() const {
    std::stringstream ss;

    // 메시지 형식: <타입>:<발신자>:<내용>:<방이름>
    // 콜론은 직렬화에 사용되므로 특수 문자 이스케이프 처리
    std::string safe_sender = escapeSpecialChars(sender_);
    std::string safe_content = escapeSpecialChars(content_);
    std::string safe_room_name = escapeSpecialChars(room_name_);

    int type_int = static_cast<int>(type_);
    ss << type_int << ":" << safe_sender << ":" << safe_content << ":" << safe_room_name << "\n";

    return ss.str();
}

// 직렬화 위해 콜론 등의 특수문자 이스케이프 처리
std::string Message::escapeSpecialChars(const std::string& str) const {
    std::string result = str;
    size_t pos = 0;
    
    // 콜론을 유니코드 대체 문자로 변경 (예: ˸)
    while ((pos = result.find(':', pos)) != std::string::npos) {
        result.replace(pos, 1, "\xCB\xB8");
        pos += 2; // 2바이트 유니코드 문자
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
        pos += 1; // 1바이트 문자로 변경됨
    }
    
    return result;
}

Message Message::deserialize(const std::string& data) {
    std::stringstream ss(data);
    std::string line;

    if (!std::getline(ss, line)) {
        return Message();
    }

    size_t first_colon = line.find(':');
    if (first_colon == std::string::npos) {
        return Message();
    }

    std::string type_str = line.substr(0, first_colon);
    int type_int = std::stoi(type_str);
    MessageType type = static_cast<MessageType>(type_int);

    size_t second_colon = line.find(':', first_colon + 1);
    if (second_colon == std::string::npos) {
        std::string content = line.substr(first_colon + 1);
        Message msg;
        content = msg.unescapeSpecialChars(content);
        return Message(type, content);
    }

    size_t third_colon = line.find(':', second_colon + 1);
    if (third_colon == std::string::npos) {
        std::string sender = line.substr(first_colon + 1, second_colon - first_colon - 1);
        std::string content = line.substr(second_colon + 1);
        
        Message msg;
        sender = msg.unescapeSpecialChars(sender);
        content = msg.unescapeSpecialChars(content);
        return Message(type, sender, content);
    }

    std::string sender = line.substr(first_colon + 1, second_colon - first_colon - 1);
    std::string content = line.substr(second_colon + 1, third_colon - second_colon - 1);
    std::string room_name = line.substr(third_colon + 1);
    
    Message msg;
    sender = msg.unescapeSpecialChars(sender);
    content = msg.unescapeSpecialChars(content);
    room_name = msg.unescapeSpecialChars(room_name);

    return Message(type, sender, content, room_name);
}

std::size_t Message::utf8Length(const std::string& str) {
    std::size_t length = 0;
    for (std::size_t i = 0; i < str.length();) {
        unsigned char c = str[i];
        if (c < 0x80) {
            // ASCII 문자 (1바이트)
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2바이트 문자
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            // 3바이트 문자 (한글, 대부분의 기본 이모티콘 포함)
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4바이트 문자 (확장 이모티콘, 특수 유니코드 문자 등)
            i += 4;
        } else {
            // 잘못된 UTF-8 시퀀스, 1바이트 건너뜀
            i += 1;
        }
        length += 1;
    }
    return length;
}

}  // namespace wagle
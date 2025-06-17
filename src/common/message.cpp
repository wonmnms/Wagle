#include "protocol/message.h"

#include <iostream>
#include <sstream>

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
    std::string safe_sender = sender_;
    std::string safe_content = content_;
    // 콜론을 유니코드 대체 문자(˸, UTF-8: \xCB\xB8)로 변경
    size_t pos = 0;
    while ((pos = safe_sender.find(':', pos)) != std::string::npos) {
        safe_sender.replace(pos, 1, "\xCB\xB8");
        pos += 2;
    }
    pos = 0;
    while ((pos = safe_content.find(':', pos)) != std::string::npos) {
        safe_content.replace(pos, 1, "\xCB\xB8");
        pos += 2;
    }
    int type_int = static_cast<int>(type_);
    ss << type_int << ":" << safe_sender << ":" << safe_content << "\n";
    return ss.str();
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
        // 대체된 콜론 복원
        for (size_t i = 0; i < content.length(); ++i) {
            if (content[i] == '\xCB' && i+1 < content.length() && content[i+1] == '\xB8') {
                content.replace(i, 2, ":");
            }
        }
        return Message(type, content);
    }

    std::string sender = line.substr(first_colon + 1, second_colon - first_colon - 1);
    std::string content = line.substr(second_colon + 1);
    
    // 대체된 콜론 복원
    for (size_t i = 0; i < sender.length(); ++i) {
        if (sender[i] == '\xCB' && i+1 < sender.length() && sender[i+1] == '\xB8') {
            sender.replace(i, 2, ":");
        }
    }
    
    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\xCB' && i+1 < content.length() && content[i+1] == '\xB8') {
            content.replace(i, 2, ":");
        }
    }

    return Message(type, sender, content);
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
            // 3바이트 문자 (한글 포함)
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4바이트 문자 (이모지 등)
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
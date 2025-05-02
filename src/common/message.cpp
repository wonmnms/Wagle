#include "protocol/message.h"

#include <iostream>
#include <sstream>

namespace wagle {

Message::Message(MessageType type, const std::string& content)
    : type_(type), content_(content) {}

Message::Message(MessageType type, const std::string& sender,
                 const std::string& content)
    : type_(type), sender_(sender), content_(content) {}

std::string Message::serialize() const {
    std::stringstream ss;

    // 메시지 형식: <타입>:<발신자>:<내용>
    int type_int = static_cast<int>(type_);
    ss << type_int << ":" << sender_ << ":" << content_ << "\n";

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
        return Message(type, line.substr(first_colon + 1));
    }

    std::string sender =
        line.substr(first_colon + 1, second_colon - first_colon - 1);
    std::string content = line.substr(second_colon + 1);

    return Message(type, sender, content);
}

}  // namespace wagle
#include "chat/user.h"

namespace wagle {

User::User(tcp::socket& socket, const std::string& name)
    : socket_(socket), name_(name) {}

std::string User::getName() const {
    return name_;
}

User::tcp::socket& User::getSocket() {
    return socket_;
}

bool User::operator==(const User& other) const {
    return name_ == other.name_;
}

bool User::operator<(const User& other) const {
    return name_ < other.name_;
}

} // namespace wagle

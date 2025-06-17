#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace wagle {

class User : public std::enable_shared_from_this<User> {
   public:
    using tcp = boost::asio::ip::tcp;

    User(tcp::socket& socket, const std::string& name)
        : socket_(socket), name_(name) {}

    virtual ~User() = default;

    std::string getName() const { return name_; }
    virtual tcp::socket& getSocket() { return socket_; }
    
    // 사용자 비교를 위한 연산자 (이름 기반)
    bool operator==(const User& other) const {
        return name_ == other.name_;
    }
    
    bool operator<(const User& other) const {
        return name_ < other.name_;
    }

   private:
    tcp::socket& socket_;
    std::string name_;
};

}  // namespace wagle
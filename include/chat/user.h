#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace wagle {

class User : public std::enable_shared_from_this<User> {
   public:
    using tcp = boost::asio::ip::tcp;

    User(tcp::socket socket, const std::string& name)
        : socket_(std::move(socket)), name_(name) {}

    std::string getName() const { return name_; }
    tcp::socket& getSocket() { return socket_; }

   private:
    tcp::socket socket_;
    std::string name_;
};

}  // namespace wagle
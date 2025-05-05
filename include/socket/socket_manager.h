#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "chat/chat_room.h"
#include "chat/user.h"

namespace wagle {

class Session : public std::enable_shared_from_this<Session> {
   public:
    using tcp = boost::asio::ip::tcp;

    Session(tcp::socket socket, ChatRoom& room)
        : socket_(std::move(socket)), room_(room) {}

    void start() {
        // 우선 사용자 이름을 받음
        readUsername();
    }

   private:
    void readUsername() {
        auto self(shared_from_this());
        boost::asio::async_read_until(
            socket_, buffer_, '\n',
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 이름 추출
                    std::string data;
                    std::istream is(&buffer_);
                    std::getline(is, data);

                    // CONNECT 메시지인지 확인
                    Message msg = Message::deserialize(data);
                    if (msg.getType() == MessageType::CONNECT) {
                        username_ = msg.getContent();

                        // 사용자 생성 및 채팅방 참여
                        auto user = std::make_shared<User>(std::move(socket_),
                                                           username_);
                        user_ = user;
                        if (!room_.join(user)) {
                            socket_.close(); //닉네임이 중복될 시 소켓을 닫고
                            return;               // 세션 종료
                        }

                        // 메시지 읽기 시작
                        readMessage();
                    }
                }
            });
    }

    void readMessage() {
        auto self(shared_from_this());
        boost::asio::async_read_until(
            user_->getSocket(), buffer_, '\n',
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 메시지 추출
                    std::string data;
                    std::istream is(&buffer_);
                    std::getline(is, data);

                    // 메시지 파싱 및 처리
                    Message msg = Message::deserialize(data);
                    if (msg.getType() == MessageType::CHAT_MSG) {
                        // 발신자 정보 추가하여 브로드캐스트
                        Message broadcast_msg(MessageType::CHAT_MSG, username_,
                                              msg.getContent());
                        room_.broadcast(broadcast_msg);
                    }

                    // 다음 메시지 읽기
                    readMessage();
                } else {
                    // 연결 종료
                    room_.leave(user_);
                }
            });
    }

    tcp::socket socket_;
    ChatRoom& room_;
    boost::asio::streambuf buffer_;
    std::string username_;
    std::shared_ptr<User> user_;
};

class SocketManager {
   public:
    using tcp = boost::asio::ip::tcp;

    SocketManager(boost::asio::io_context& io_context,
                  const tcp::endpoint& endpoint)
        : acceptor_(io_context, endpoint) {
        startAccept();
    }

   private:
    void startAccept() {
        acceptor_.async_accept([this](boost::system::error_code ec,
                                      tcp::socket socket) {
            if (!ec) {
                std::cout << "New client connected" << std::endl;
                std::make_shared<Session>(std::move(socket), room_)->start();
            }

            // 다음 연결 대기
            startAccept();
        });
    }

    tcp::acceptor acceptor_;
    ChatRoom room_;
};

}  // namespace wagle
#include <boost/asio.hpp>
#include <deque>
#include <iostream>
#include <string>
#include <thread>

#include "protocol/message.h"

using boost::asio::ip::tcp;

class ChatClient {
   public:
    ChatClient(boost::asio::io_context& io_context,
               const tcp::resolver::results_type& endpoints)
        : io_context_(io_context), socket_(io_context) {
        connect(endpoints);
    }

    void close() {
        boost::asio::post(io_context_, [this]() { socket_.close(); });
    }

    void write(const wagle::Message& msg) {
        boost::asio::post(io_context_, [this, msg]() {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!write_in_progress) {
                writeImpl();
            }
        });
    }

   private:
    void connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(
            socket_, endpoints,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    readMessages();
                } else {
                    std::cerr << "Connect failed: " << ec.message()
                              << std::endl;
                }
            });
    }

    void readMessages() {
        boost::asio::async_read_until(
            socket_, buffer_, '\n',
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 메시지 추출 및 표시
                    std::string data;
                    std::istream is(&buffer_);
                    std::getline(is, data);

                    wagle::Message msg = wagle::Message::deserialize(data);
                    if (msg.getType() == wagle::MessageType::CHAT_MSG) {
                        std::cout << msg.getSender() << ": " << msg.getContent()
                                  << std::endl;
                    } else if (msg.getType() == wagle::MessageType::CONNECT ||
                               msg.getType() ==
                                   wagle::MessageType::DISCONNECT) {
                        std::cout << "** " << msg.getContent() << " **"
                                  << std::endl;
                    }

                    // 다음 메시지 읽기
                    readMessages();
                } else {
                    std::cerr << "Read failed: " << ec.message() << std::endl;
                    socket_.close();
                }
            });
    }

    void writeImpl() {
        std::string serialized_msg = write_msgs_.front().serialize();
        boost::asio::async_write(
            socket_, boost::asio::buffer(serialized_msg),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        writeImpl();
                    }
                } else {
                    std::cerr << "Write failed: " << ec.message() << std::endl;
                    socket_.close();
                }
            });
    }

    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    std::deque<wagle::Message> write_msgs_;
};

int main(int argc, char* argv[]) {
    try {
        // 서버 주소 및 포트 설정 (기본값 localhost:8080)
        std::string host = "localhost";
        std::string port = "8080";

        if (argc > 1) host = argv[1];
        if (argc > 2) port = argv[2];

        // IO 컨텍스트 및 리졸버 생성
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, port);

        // 클라이언트 생성
        ChatClient client(io_context, endpoints);

        // IO 스레드 시작
        std::thread io_thread([&io_context]() { io_context.run(); });

        // 사용자 이름 입력
        std::string username;
        std::cout << "Enter your username: ";
        std::getline(std::cin, username);

        // 연결 메시지 전송
        client.write(wagle::Message(wagle::MessageType::CONNECT, username));

        // 채팅 메시지 입력 루프
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "/quit") {
                break;
            }

            // 채팅 메시지 전송
            client.write(wagle::Message(wagle::MessageType::CHAT_MSG, line));
        }

        // 종료
        client.close();
        io_thread.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
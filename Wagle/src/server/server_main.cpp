#include <boost/asio.hpp>
#include <csignal>
#include <iostream>
#include <thread>

#include "socket/socket_manager.h"

using boost::asio::ip::tcp;

// 전역 변수
wagle::SocketManager* server = nullptr;

// 시그널 핸들러
void signal_handler(int /*signal*/) {
    if (server) {
        delete server;
        server = nullptr;
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    try {
        // 시그널 핸들러 설정
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 서버 주소 및 포트 설정
        std::string host = "0.0.0.0";
        std::string port = "8080";

        if (argc > 1) host = argv[1];
        if (argc > 2) port = argv[2];

        // IO 컨텍스트 생성
        boost::asio::io_context io_context;

        // 엔드포인트 생성
        tcp::endpoint endpoint(boost::asio::ip::make_address(host),
                               std::atoi(port.c_str()));

        // 서버 생성
        server = new wagle::SocketManager(io_context, endpoint);

        // IO 스레드 시작
        std::thread io_thread([&io_context]() { io_context.run(); });

        // 메인 스레드 대기
        io_thread.join();

        // 서버 정리
        delete server;
        server = nullptr;
    } catch (std::exception& e) {
        // ncurses 종료
        wagle::cleanup_server_ui();

        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
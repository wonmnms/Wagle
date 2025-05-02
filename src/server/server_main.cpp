#include <boost/asio.hpp>
#include <iostream>

#include "socket/socket_manager.h"

using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        // 포트 설정 (기본값 8080)
        unsigned short port = 8080;
        if (argc > 1) {
            port = std::stoi(argv[1]);
        }

        // IO 컨텍스트 및 소켓 매니저 생성
        boost::asio::io_context io_context;
        wagle::SocketManager manager(io_context,
                                     tcp::endpoint(tcp::v4(), port));

        std::cout << "Wagle chat server started on port " << port << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;

        // 서버 실행
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
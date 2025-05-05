#include <iostream>
#include <boost/asio.hpp>
#include <signal.h>
#include "socket/socket_manager.h"

using boost::asio::ip::tcp;

// Ctrl+C 핸들러
boost::asio::io_context* global_io_context = nullptr;

void signal_handler(int signal) {
    if (global_io_context) {
        global_io_context->stop();
    }
}

int main(int argc, char* argv[]) {
    try {
        // 포트 설정 (기본값 8080)
        unsigned short port = 8080;
        if (argc > 1) {
            port = std::stoi(argv[1]);
        }
        
        // IO 컨텍스트 및 소켓 매니저 생성
        boost::asio::io_context io_context;
        global_io_context = &io_context;
        
        // Ctrl+C 시그널 핸들러 등록
        signal(SIGINT, signal_handler);
        
        // 소켓 매니저 생성
        wagle::SocketManager manager(io_context, tcp::endpoint(tcp::v4(), port));
        
        // 서버 실행
        io_context.run();
    }
    catch (std::exception& e) {
        // ncurses 종료
        wagle::cleanup_server_ui();
        
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
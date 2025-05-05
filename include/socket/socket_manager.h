#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <ncurses.h>
#include <ctime>
#include <cstdarg>
#include <set>  // 추가: 사용자 이름 관리를 위한 set
#include "chat/chat_room.h"
#include "chat/user.h"

namespace wagle {

// 서버 UI 관련 전역 변수
extern WINDOW* main_win;
extern WINDOW* status_win;
extern WINDOW* log_win;

// 총 접속 시도 수
extern int total_connections;

// 사용 중인 사용자 이름 목록
extern std::set<std::string> active_usernames;  // 추가: 활성 사용자 이름 집합

// 함수 선언
void init_server_ui();
void cleanup_server_ui();
void update_status_window(size_t user_count);
void add_log_message(const char* format, ...);

// 세션 클래스
class Session : public std::enable_shared_from_this<Session> {
public:
    using tcp = boost::asio::ip::tcp;
    
    Session(tcp::socket socket, ChatRoom& room);
    void start();
    
private:
    void readUsername();
    void readMessage();
    
    tcp::socket socket_;
    ChatRoom& room_;
    boost::asio::streambuf buffer_;
    std::string username_;
    std::string client_address_;
    std::shared_ptr<User> user_;
};

// 소켓 매니저 클래스
class SocketManager {
public:
    using tcp = boost::asio::ip::tcp;
    
    SocketManager(boost::asio::io_context& io_context, const tcp::endpoint& endpoint);
    ~SocketManager();
    
private:
    void startAccept();
    
    tcp::acceptor acceptor_;
    ChatRoom room_;
};

} // namespace wagle
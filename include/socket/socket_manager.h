#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <ncurses.h>
#include <ctime>
#include <cstdarg>
#include <set>
#include "chat/chat_room.h"
#include "chat/chat_room_manager.h"
#include "chat/user.h"

namespace wagle {

// 서버 UI 관련 전역 변수
extern WINDOW* main_win;
extern WINDOW* status_win;
extern WINDOW* log_win;

// 총 접속 시도 수
extern int total_connections;

// 사용 중인 사용자 이름 목록
extern std::set<std::string> active_usernames;

// 함수 선언
void init_server_ui();
void cleanup_server_ui();
void update_status_window(size_t user_count);
void add_log_message(const char* format, ...);

// 세션 클래스
class Session : public std::enable_shared_from_this<Session> {
public:
    using tcp = boost::asio::ip::tcp;
    
    Session(tcp::socket socket, ChatRoomManager& room_manager);
    void start();
    
private:
    void readUsername();
    void readMessage();
    void handleRoomListRequest();
    void handleRoomCreateRequest(const std::string& room_name);
    void handleRoomJoinRequest(const std::string& room_name);
    void handleRoomLeaveRequest();
    
    tcp::socket socket_;
    ChatRoomManager& room_manager_;
    boost::asio::streambuf buffer_;
    std::string username_;
    std::string client_address_;
    std::string current_room_;  // 현재 입장한 채팅방
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
    ChatRoomManager room_manager_;  // ChatRoom 대신 ChatRoomManager 사용
};

} // namespace wagle
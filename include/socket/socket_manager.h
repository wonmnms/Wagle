#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <ncurses.h>
#include <ctime>
#include <cstdarg>
#include <set>
#include <string>
#include "chat/chat_room_manager.h" // ChatRoomInfo 정의 포함

// Forward declarations
namespace wagle {
    class ChatRoom;
    class User;
    class Message;
    enum class MessageType;
}

namespace wagle {

// 서버 UI 관련 전역 변수
extern WINDOW* main_win;
extern WINDOW* status_win;
extern WINDOW* log_win;
extern int total_connections;
extern std::set<std::string> active_usernames;

// 함수 선언
void init_server_ui();
void cleanup_server_ui();
void update_status_window(size_t user_count, const std::vector<wagle::ChatRoomInfo>& room_list);
void add_log_message(const char* format, ...);

// Session용 User 클래스
class SessionUser : public std::enable_shared_from_this<SessionUser> {
public:
    using tcp = boost::asio::ip::tcp;

    SessionUser(tcp::socket& socket, const std::string& name);
    std::string getName() const;
    tcp::socket& getSocket();

private:
    tcp::socket& socket_;
    std::string name_;
};

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
    std::string current_room_;
    std::shared_ptr<SessionUser> user_;
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
    ChatRoomManager room_manager_;  // 멤버 변수로 사용하려면 실제 타입이 필요
};

} // namespace wagle
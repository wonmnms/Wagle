#include <locale.h>
#include <mutex>
#include "socket/socket_manager.h"
#include "protocol/message.h"
#include "chat/chat_room_manager.h"
#include "chat/chat_room.h"
#include "chat/user.h"

namespace wagle {

// 전역 변수 정의
WINDOW* main_win = nullptr;
WINDOW* status_win = nullptr;
WINDOW* log_win = nullptr;
int total_connections = 0;
std::set<std::string> active_usernames;
std::mutex username_mutex;

// SessionUser 메서드 구현
SessionUser::SessionUser(tcp::socket& socket, const std::string& name)
    : socket_(socket), name_(name) {}

std::string SessionUser::getName() const {
    return name_;
}

SessionUser::tcp::socket& SessionUser::getSocket() {
    return socket_;
}

// 서버 UI 함수들
void init_server_ui() {
    setlocale(LC_ALL, "");
    if (stdscr) {
        if (status_win) delwin(status_win);
        if (log_win) delwin(log_win);
        if (main_win) delwin(main_win);
        endwin();
        refresh();
    }
    initscr();
    cbreak();
    noecho();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    main_win = newwin(max_y, max_x, 0, 0);
    box(main_win, 0, 0);
    mvwprintw(main_win, 0, 2, " Wagle Chat Server ");
    // status_win을 우측에 길게 배치 (높이: max_y-4, 너비: max_x/2)
    int status_height = max_y - 4;
    int status_width = max_x / 2;
    int status_starty = 2;
    int status_startx = max_x - status_width - 2;
    status_win = newwin(status_height, status_width, status_starty, status_startx);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " Server Status ");
    // log_win은 좌측에 남은 공간에 배치
    log_win = newwin(max_y - 4, max_x - status_width - 4, 2, 2);
    scrollok(log_win, TRUE);
    refresh();
    wrefresh(main_win);
    wrefresh(status_win);
    wrefresh(log_win);
}

void cleanup_server_ui() {
    if (status_win) delwin(status_win);
    if (log_win) delwin(log_win);
    if (main_win) delwin(main_win);
    endwin();
    
    status_win = nullptr;
    log_win = nullptr;
    main_win = nullptr;
}

void update_status_window(size_t user_count, const std::vector<wagle::ChatRoomInfo>& room_list) {
    if (!status_win) return;
    werase(status_win);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " Server Status ");
    mvwprintw(status_win, 1, 2, "Online Users: %zu", user_count);
    int line = 2;
    mvwprintw(status_win, line++, 2, "Rooms:");
    if (room_list.empty()) {
        mvwprintw(status_win, line++, 4, "(No rooms)");
    } else {
        for (const auto& room_info : room_list) {
            mvwprintw(status_win, line++, 4, "- %s (%zu)%s", room_info.name.c_str(), room_info.user_count, room_info.is_default ? " [default]" : "");
            // status_win의 크기만큼 모두 출력 (line 제한 없음)
        }
    }
    wrefresh(status_win);
}

void add_log_message(const char* format, ...) {
    if (!log_win) return;
    
    va_list args;
    va_start(args, format);
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char time_str[10];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
    
    wprintw(log_win, "[%s] ", time_str);
    vw_printw(log_win, format, args);
    wprintw(log_win, "\n");
    wrefresh(log_win);
    
    va_end(args);
}

// Session 클래스 구현
Session::Session(tcp::socket socket, ChatRoomManager& room_manager)
    : socket_(std::move(socket)), room_manager_(room_manager) {
    total_connections++;
}

void Session::start() {
    try {
        client_address_ = socket_.remote_endpoint().address().to_string();
        add_log_message("New connection from %s", client_address_.c_str());
    } catch (std::exception& e) {
        client_address_ = "unknown";
        add_log_message("New connection from unknown address");
    }
    
    readUsername();
}

void Session::readUsername() {
    auto self(shared_from_this());
    boost::asio::async_read_until(
        socket_, buffer_, '\n',
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                std::string data;
                std::istream is(&buffer_);
                std::getline(is, data);
                
                Message msg = Message::deserialize(data);
                if (msg.getType() == MessageType::CONNECT) {
                    username_ = msg.getSender();
                    
                    bool isValid = true;
                    std::string errorMsg;
                    
                    if (username_.empty()) {
                        isValid = false;
                        errorMsg = "Username cannot be empty";
                    } else {
                        std::unique_lock<std::mutex> lock(username_mutex);
                        if (active_usernames.find(username_) != active_usernames.end()) {
                            isValid = false;
                            errorMsg = "Username already in use";
                        } else {
                            active_usernames.insert(username_);
                        }
                    }
                    
                    if (!isValid) {
                        Message error_msg(MessageType::DISCONNECT, "SERVER", errorMsg);
                        boost::asio::write(socket_, boost::asio::buffer(error_msg.serialize()));
                        add_log_message("Username validation failed: %s (%s)", 
                                      username_.c_str(), errorMsg.c_str());
                        readUsername();
                        return;
                    }
                    
                    add_log_message("User connected: %s (%s)", username_.c_str(), client_address_.c_str());
                    
                    Message confirm_msg(MessageType::CONNECT, "SERVER", "Connection successful");
                    boost::asio::write(socket_, boost::asio::buffer(confirm_msg.serialize()));
                    
                    readMessage();
                }
            }
        });
}

void Session::readMessage() {
    auto self(shared_from_this());
    boost::asio::async_read_until(
        socket_, buffer_, '\n',
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                std::string data;
                std::istream is(&buffer_);
                std::getline(is, data);
                
                Message msg = Message::deserialize(data);
                
                switch (msg.getType()) {
                    case MessageType::ROOM_LIST:
                        handleRoomListRequest();
                        break;
                        
                    case MessageType::ROOM_CREATE:
                        handleRoomCreateRequest(msg.getContent());
                        break;
                        
                    case MessageType::ROOM_JOIN:
                        handleRoomJoinRequest(msg.getContent());
                        break;
                        
                    case MessageType::ROOM_LEAVE:
                        handleRoomLeaveRequest();
                        break;
                        
                    case MessageType::CHAT_MSG:
                        if (!current_room_.empty()) {
                            auto room = room_manager_.getRoom(current_room_);
                            if (room) {
                                Message broadcast_msg(MessageType::CHAT_MSG, username_, msg.getContent(), current_room_);
                                room->broadcast(broadcast_msg);
                            }
                        }
                        break;
                        
                    default:
                        break;
                }
                
                readMessage();
            } else {
                add_log_message("User disconnected: %s (%s)", username_.c_str(), client_address_.c_str());
                {
                    std::unique_lock<std::mutex> lock(username_mutex);
                    active_usernames.erase(username_);
                }
                if (!current_room_.empty()) {
                    auto room = room_manager_.getRoom(current_room_);
                    if (room && user_) {
                        auto user_ptr = std::make_shared<User>(user_->getSocket(), user_->getName());
                        room->leave(user_ptr);
                    }
                }
                
                auto room_list = room_manager_.getRoomList();
                size_t total_users = 0;
                for (const auto& room_info : room_list) {
                    total_users += room_info.user_count;
                }
                update_status_window(total_users, room_list);
            }
        });
}

void Session::handleRoomListRequest() {
    auto room_list = room_manager_.getRoomList();
    
    std::string room_list_data;
    for (const auto& room_info : room_list) {
        if (!room_list_data.empty()) {
            room_list_data += ";";
        }
        room_list_data += room_info.name + "," + std::to_string(room_info.user_count) + "," + 
                         (room_info.is_default ? "1" : "0");
    }
    
    Message response(MessageType::ROOM_LIST, "SERVER", room_list_data);
    boost::asio::write(socket_, boost::asio::buffer(response.serialize()));
}

void Session::handleRoomCreateRequest(const std::string& room_name) {
    if (room_manager_.createRoom(room_name)) {
        Message response(MessageType::ROOM_CREATE, "SERVER", "Room created successfully");
        boost::asio::write(socket_, boost::asio::buffer(response.serialize()));
        add_log_message("Room created: %s by %s", room_name.c_str(), username_.c_str());
    } else {
        Message response(MessageType::ROOM_ERROR, "SERVER", "Failed to create room (name already exists or invalid)");
        boost::asio::write(socket_, boost::asio::buffer(response.serialize()));
    }
}

void Session::handleRoomJoinRequest(const std::string& room_name) {
    auto room = room_manager_.getRoom(room_name);
    if (!room) {
        Message response(MessageType::ROOM_ERROR, "SERVER", "Room does not exist");
        boost::asio::write(socket_, boost::asio::buffer(response.serialize()));
        return;
    }
    
    // 이전 방에서 나가기
    if (!current_room_.empty()) {
        auto old_room = room_manager_.getRoom(current_room_);
        if (old_room && user_) {
            auto user_ptr = std::make_shared<User>(user_->getSocket(), user_->getName());
            old_room->leave(user_ptr);
        }
    }
    
    // User 객체 생성 또는 업데이트
    if (!user_) {
        user_ = std::make_shared<SessionUser>(socket_, username_);
    }
    
    // 새 방에 입장
    current_room_ = room_name;
    auto user_ptr = std::make_shared<User>(user_->getSocket(), user_->getName());
    room->join(user_ptr);
    
    Message response(MessageType::ROOM_JOIN, "SERVER", "Joined room: " + room_name, room_name);
    boost::asio::write(socket_, boost::asio::buffer(response.serialize()));
    
    add_log_message("User %s joined room: %s", username_.c_str(), room_name.c_str());
    
    auto room_list = room_manager_.getRoomList();
    size_t total_users = 0;
    for (const auto& room_info : room_list) {
        total_users += room_info.user_count;
    }
    update_status_window(total_users, room_list);
}

void Session::handleRoomLeaveRequest() {
    if (!current_room_.empty()) {
        auto room = room_manager_.getRoom(current_room_);
        if (room && user_) {
            auto user_ptr = std::make_shared<User>(user_->getSocket(), user_->getName());
            room->leave(user_ptr);
        }
        current_room_.clear();
        
        Message response(MessageType::ROOM_LEAVE, "SERVER", "Left room");
        boost::asio::write(socket_, boost::asio::buffer(response.serialize()));
        
        add_log_message("User %s left room", username_.c_str());
        
        auto room_list = room_manager_.getRoomList();
        size_t total_users = 0;
        for (const auto& room_info : room_list) {
            total_users += room_info.user_count;
        }
        update_status_window(total_users, room_list);
    }
}

// SocketManager 클래스 구현
SocketManager::SocketManager(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
    : acceptor_(io_context, endpoint) {
    
    setlocale(LC_ALL, "");
    
    init_server_ui();
    add_log_message("Server started on port %d", endpoint.port());
    update_status_window(0, {});
    
    startAccept();
}

SocketManager::~SocketManager() {
    cleanup_server_ui();
}

void SocketManager::startAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), room_manager_)->start();
            }
            
            startAccept();
        });
}

} // namespace wagle
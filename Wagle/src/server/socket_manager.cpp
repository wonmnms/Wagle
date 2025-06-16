#include "socket/socket_manager.h"

#include <locale.h>

#include <boost/asio.hpp>
#include <cstdarg>
#include <ctime>
#include <iostream>
#include <mutex>
#include <set>

namespace wagle {

// 전역 변수 정의
WINDOW* main_win = nullptr;
WINDOW* status_win = nullptr;
WINDOW* log_win = nullptr;
int total_connections = 0;
std::set<std::string> active_usernames;  // 활성 사용자 이름 집합 정의
std::mutex username_mutex;  // 사용자 이름 동시 접근 제어를 위한 뮤텍스

// 서버 UI 초기화
void init_server_ui() {
    // UTF-8 한글 지원 설정
    setlocale(LC_ALL, "");

    // ncurses 초기화
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // 색상 설정
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);

    // 메인 윈도우 생성
    main_win = newwin(LINES - 2, COLS, 0, 0);
    box(main_win, 0, 0);
    mvwprintw(main_win, 0, 2, " Wagle Chat Server ");

    // 상태 윈도우 생성
    status_win = newwin(2, COLS, LINES - 2, 0);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " Status ");

    // 로그 윈도우 생성
    log_win = newwin(LINES - 4, COLS - 2, 1, 1);
    scrollok(log_win, TRUE);

    // 화면 갱신
    refresh();
    wrefresh(main_win);
    wrefresh(status_win);
    wrefresh(log_win);
}

// 서버 UI 정리
void cleanup_server_ui() {
    if (log_win) delwin(log_win);
    if (status_win) delwin(status_win);
    if (main_win) delwin(main_win);
    endwin();
}

// 상태 윈도우 업데이트
void update_status_window(size_t user_count) {
    if (status_win) {
        mvwprintw(status_win, 1, 2, "Connected users: %zu", user_count);
        wrefresh(status_win);
    }
}

// 로그 메시지 추가
void add_log_message(const char* format, ...) {
    if (log_win) {
        va_list args;
        va_start(args, format);

        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);

        wprintw(log_win, "%s\n", buffer);
        wrefresh(log_win);

        va_end(args);
    }
}

// 세션 생성자
Session::Session(tcp::socket socket, ChatRoomManager& room_manager)
    : socket_(std::move(socket)),
      room_manager_(room_manager),
      client_address_(socket_.remote_endpoint().address().to_string()) {
    total_connections++;
}

// 세션 시작
void Session::start() { readUsername(); }

// 사용자 이름 읽기
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
                        if (active_usernames.find(username_) !=
                            active_usernames.end()) {
                            isValid = false;
                            errorMsg = "Username already in use";
                        } else {
                            active_usernames.insert(username_);
                        }
                    }

                    if (!isValid) {
                        Message error_msg(MessageType::USERNAME_ERROR, "SERVER",
                                          errorMsg);
                        boost::asio::async_write(
                            socket_, boost::asio::buffer(error_msg.serialize()),
                            [this, self](boost::system::error_code ec,
                                         std::size_t /*length*/) {
                                if (!ec) {
                                    readUsername();
                                }
                            });
                        return;
                    }

                    add_log_message("User connected: %s (%s)",
                                    username_.c_str(), client_address_.c_str());

                    Message confirm_msg(MessageType::CONNECT, "SERVER",
                                        "Connection successful");
                    boost::asio::async_write(
                        socket_, boost::asio::buffer(confirm_msg.serialize()),
                        [this, self](boost::system::error_code ec,
                                     std::size_t /*length*/) {
                            if (!ec) {
                                user_ = std::make_shared<User>(
                                    std::move(socket_), username_);

                                // 기본 채팅방 생성 및 참여
                                auto default_room =
                                    room_manager_.createRoom("General", false);
                                current_room_id_ = default_room->getId();
                                room_manager_.addUserToRoom(current_room_id_,
                                                            user_);

                                update_status_window(
                                    room_manager_.getRoom(current_room_id_)
                                        ->getUserCount());

                                readMessage();
                            }
                        });
                }
            } else {
                // 연결 오류 처리
                add_log_message("Connection error: %s", ec.message().c_str());
            }
        });
}

// 메시지 읽기
void Session::readMessage() {
    auto self(shared_from_this());
    boost::asio::async_read_until(
        user_->getSocket(), buffer_, '\n',
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                std::string data;
                std::istream is(&buffer_);
                std::getline(is, data);

                Message msg = Message::deserialize(data);

                switch (msg.getType()) {
                    case MessageType::CHAT_MSG:
                        handleChatMessage(msg);
                        break;

                    case MessageType::ROOM_CREATE:
                    case MessageType::ROOM_DELETE:
                    case MessageType::ROOM_JOIN:
                    case MessageType::ROOM_LEAVE:
                    case MessageType::ROOM_LIST:
                    case MessageType::ROOM_INFO:
                        handleRoomCommand(msg);
                        break;

                    default:
                        break;
                }

                readMessage();
            } else {
                add_log_message("User disconnected: %s (%s)", username_.c_str(),
                                client_address_.c_str());

                {
                    std::unique_lock<std::mutex> lock(username_mutex);
                    active_usernames.erase(username_);
                }

                if (!current_room_id_.empty()) {
                    room_manager_.removeUserFromRoom(current_room_id_, user_);
                }

                update_status_window(
                    room_manager_.getRoom(current_room_id_)->getUserCount());
            }
        });
}

// 채팅방 명령 처리
void Session::handleRoomCommand(const Message& msg) {
    switch (msg.getType()) {
        case MessageType::ROOM_CREATE: {
            auto room = room_manager_.createRoom(msg.getContent(), false);
            room_manager_.addUserToRoom(room->getId(), user_);
            current_room_id_ = room->getId();
            // 방 생성 메시지 + 자동 입장 메시지 전송
            Message response(MessageType::ROOM_CREATE, "SERVER",
                             "Room created: " + room->getName(), room->getId());
            boost::asio::write(user_->getSocket(),
                               boost::asio::buffer(response.serialize()));
            Message join_response(MessageType::ROOM_JOIN, "SERVER",
                                  "Joined room: " + room->getName(),
                                  room->getId());
            boost::asio::write(user_->getSocket(),
                               boost::asio::buffer(join_response.serialize()));
            // 방 목록 전송
            auto rooms = room_manager_.getPublicRooms();
            std::string room_list;
            for (const auto& r : rooms) {
                room_list += r->getName() + " (" +
                             std::to_string(r->getUserCount()) + " users)\n";
            }
            Message list_response(MessageType::ROOM_LIST, "SERVER", room_list);
            boost::asio::write(user_->getSocket(),
                               boost::asio::buffer(list_response.serialize()));
            break;
        }

        case MessageType::ROOM_JOIN: {
            if (room_manager_.addUserToRoom(msg.getContent(), user_)) {
                current_room_id_ = msg.getContent();
                Message response(
                    MessageType::ROOM_JOIN, "SERVER",
                    "Joined room: " +
                        room_manager_.getRoom(current_room_id_)->getName(),
                    current_room_id_);
                boost::asio::write(user_->getSocket(),
                                   boost::asio::buffer(response.serialize()));
            } else {
                Message response(MessageType::ROOM_ERROR, "SERVER",
                                 "Failed to join room");
                boost::asio::write(user_->getSocket(),
                                   boost::asio::buffer(response.serialize()));
            }
            break;
        }

        case MessageType::ROOM_LEAVE: {
            if (room_manager_.removeUserFromRoom(current_room_id_, user_)) {
                Message response(
                    MessageType::ROOM_LEAVE, "SERVER",
                    "Left room: " +
                        room_manager_.getRoom(current_room_id_)->getName(),
                    current_room_id_);
                boost::asio::write(user_->getSocket(),
                                   boost::asio::buffer(response.serialize()));
                current_room_id_.clear();
            }
            break;
        }

        case MessageType::ROOM_LIST: {
            auto rooms = room_manager_.getPublicRooms();
            std::string room_list;
            for (const auto& room : rooms) {
                room_list += room->getName() + " (" +
                             std::to_string(room->getUserCount()) + " users)\n";
            }
            Message response(MessageType::ROOM_LIST, "SERVER", room_list);
            boost::asio::write(user_->getSocket(),
                               boost::asio::buffer(response.serialize()));
            break;
        }

        default:
            break;
    }
}

// 채팅 메시지 처리
void Session::handleChatMessage(const Message& msg) {
    if (!current_room_id_.empty()) {
        Message room_msg(MessageType::ROOM_MESSAGE, username_, msg.getContent(),
                         current_room_id_);
        auto room = room_manager_.getRoom(current_room_id_);
        if (room) {
            room->broadcast(room_msg);
        }
    }
}

// 소켓 매니저 생성자
SocketManager::SocketManager(boost::asio::io_context& io_context,
                             const tcp::endpoint& endpoint)
    : acceptor_(io_context, endpoint) {
    // UTF-8 한글 지원 설정
    setlocale(LC_ALL, "");

    // ncurses UI 초기화
    init_server_ui();
    add_log_message("Server started on port %d", endpoint.port());
    update_status_window(0);

    startAccept();
}

// 소켓 매니저 소멸자
SocketManager::~SocketManager() {
    // ncurses 종료
    cleanup_server_ui();
}

// 연결 수락 시작
void SocketManager::startAccept() {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket), room_manager_)
                    ->start();
            }

            // 다음 연결 대기
            startAccept();
        });
}

}  // namespace wagle
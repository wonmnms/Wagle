#include <locale.h>
#include <mutex>
#include "socket/socket_manager.h"

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
    
    // 이미 초기화되어 있으면 정리
    if (stdscr) {
        if (status_win) delwin(status_win);
        if (log_win) delwin(log_win);
        if (main_win) delwin(main_win);
        endwin();
        refresh();
    }
    
    // ncurses 초기화
    initscr();
    cbreak();
    noecho();
    
    // 색상 설정
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    
    // 화면 크기 가져오기
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // 메인 윈도우 설정
    main_win = newwin(max_y, max_x, 0, 0);
    box(main_win, 0, 0);
    mvwprintw(main_win, 0, 2, " Wagle Chat Server ");
    
    // 상태 윈도우 설정 (우측 상단)
    status_win = newwin(4, 30, 2, max_x - 32);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " Server Status ");
    
    // 로그 메시지만 표시하도록 수정 (메시지 내역 제거)
    log_win = newwin(max_y - 4, max_x - 35, 2, 2);
    scrollok(log_win, TRUE);
    
    // 화면 업데이트
    refresh();
    wrefresh(main_win);
    wrefresh(status_win);
    wrefresh(log_win);
}

// 서버 UI 종료
void cleanup_server_ui() {
    if (status_win) delwin(status_win);
    if (log_win) delwin(log_win);
    if (main_win) delwin(main_win);
    endwin();
    
    // 포인터 초기화
    status_win = nullptr;
    log_win = nullptr;
    main_win = nullptr;
}

// 상태 창 업데이트
void update_status_window(size_t user_count) {
    if (!status_win) return;
    
    // 상태창 지우기
    werase(status_win);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " Server Status ");
    
    // 사용자 수 표시
    mvwprintw(status_win, 1, 2, "Online Users: %zu", user_count);
    
    // 총 연결 수 표시
    mvwprintw(status_win, 2, 2, "Total Connections: %d", total_connections);
    
    // 화면 업데이트
    wrefresh(status_win);
}

// 로그 메시지 추가
void add_log_message(const char* format, ...) {
    if (!log_win) return;
    
    va_list args;
    va_start(args, format);
    
    // 시간 정보 추가
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

// 세션 생성자
Session::Session(tcp::socket socket, ChatRoom& room)
    : socket_(std::move(socket)), room_(room) {
    // 연결 수 증가
    total_connections++;
}

// 세션 시작
void Session::start() {
    // 클라이언트 주소 가져오기
    try {
        client_address_ = socket_.remote_endpoint().address().to_string();
        add_log_message("New connection from %s", client_address_.c_str());
    } catch (std::exception& e) {
        client_address_ = "unknown";
        add_log_message("New connection from unknown address");
    }
    
    // 사용자 이름을 먼저 받음
    readUsername();
}

// 사용자 이름 읽기
void Session::readUsername() {
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
                    username_ = msg.getSender();  // getSender()로 사용자 이름 가져오기
                    
                    // 이름이 비어있거나 중복되었는지 확인
                    bool isValid = true;
                    std::string errorMsg;
                    
                    // 빈 이름 체크
                    if (username_.empty()) {
                        isValid = false;
                        errorMsg = "Username cannot be empty";
                    } 
                    // 중복 이름 체크 (스레드 안전하게)
                    else {
                        std::unique_lock<std::mutex> lock(username_mutex);
                        if (active_usernames.find(username_) != active_usernames.end()) {
                            isValid = false;
                            errorMsg = "Username already in use";
                        } else {
                            // 이름이 유효하면 활성 사용자 목록에 추가
                            active_usernames.insert(username_);
                        }
                        // 락 자동 해제
                    }
                    
                    if (!isValid) {
                        // 에러 메시지 전송
                        Message error_msg(MessageType::DISCONNECT, "SERVER", errorMsg);
                        boost::asio::write(socket_, boost::asio::buffer(error_msg.serialize()));
                        
                        // 로그 출력
                        add_log_message("Username validation failed: %s (%s)", 
                                      username_.c_str(), errorMsg.c_str());
                        
                        // 다시 이름 읽기
                        readUsername();
                        return;
                    }
                    
                    // 로그 출력
                    add_log_message("User connected: %s (%s)", username_.c_str(), client_address_.c_str());
                    
                    // 연결 확인 메시지 전송
                    Message confirm_msg(MessageType::CONNECT, "SERVER", "Connection successful");
                    boost::asio::write(socket_, boost::asio::buffer(confirm_msg.serialize()));
                    
                    // 사용자 생성 및 채팅방 참여
                    auto user = std::make_shared<User>(std::move(socket_), username_);
                    user_ = user;
                    room_.join(user);
                    
                    // 상태창 업데이트
                    update_status_window(room_.getUserCount());
                    
                    // 메시지 읽기 시작
                    readMessage();
                }
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
                // 메시지 추출
                std::string data;
                std::istream is(&buffer_);
                std::getline(is, data);
                
                // 메시지 파싱 및 처리
                Message msg = Message::deserialize(data);
                if (msg.getType() == MessageType::CHAT_MSG) {
                    // 발신자 정보 추가하여 브로드캐스트
                    Message broadcast_msg(MessageType::CHAT_MSG, username_, msg.getContent());
                    room_.broadcast(broadcast_msg);
                }
                
                // 다음 메시지 읽기
                readMessage();
            } else {
                // 로그 출력
                add_log_message("User disconnected: %s (%s)", 
                              username_.c_str(), client_address_.c_str());
                
                // 사용자 이름 해제 (스레드 안전하게)
                {
                    std::unique_lock<std::mutex> lock(username_mutex);
                    active_usernames.erase(username_);
                }
                
                // 연결 종료
                room_.leave(user_);
                
                // 상태창 업데이트
                update_status_window(room_.getUserCount());
            }
        });
}

// 소켓 매니저 생성자
SocketManager::SocketManager(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
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
                std::make_shared<Session>(std::move(socket), room_)->start();
            }
            
            // 다음 연결 대기
            startAccept();
        });
}

} // namespace wagle
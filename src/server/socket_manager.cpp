#include <locale.h>
#include "socket/socket_manager.h"

namespace wagle {

// 전역 변수 정의
WINDOW* main_win = nullptr;
WINDOW* status_win = nullptr;
WINDOW* log_win = nullptr;
int total_connections = 0;

// 서버 UI 초기화
void init_server_ui() {
    // UTF-8 한글 지원 설정
    setlocale(LC_ALL, "");
    
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
    status_win = newwin(5, 30, 2, max_x - 32);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " Server Status ");
    
    // 로그 윈도우 설정
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
}

// 상태 창 업데이트
void update_status_window(size_t user_count) {
    // 상태창 지우기
    werase(status_win);
    box(status_win, 0, 0);
    mvwprintw(status_win, 0, 2, " Server Status ");
    
    // 사용자 수 표시
    mvwprintw(status_win, 2, 2, "Online Users: %zu", user_count);
    
    // 총 연결 수 표시
    mvwprintw(status_win, 3, 2, "Total Connections: %d", total_connections);
    
    // 화면 업데이트
    wrefresh(status_win);
}

// 로그 메시지 추가
void add_log_message(const char* format, ...) {
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
                    username_ = msg.getContent();
                    
                    // 로그 출력
                    add_log_message("User connected: %s (%s)", username_.c_str(), client_address_.c_str());
                    
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
                    // 로그 출력
                    add_log_message("Message from %s: %s", 
                                  username_.c_str(), msg.getContent().c_str());
                    
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
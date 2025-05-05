#include <iostream>
#include <string>
#include <thread>
#include <deque>
#include <boost/asio.hpp>
#include <ncurses.h>
#include <locale.h>
#include "protocol/message.h"

using boost::asio::ip::tcp;

// 윈도우 크기 및 위치 상수
const int USER_COUNT_WIDTH = 20;
const int USER_COUNT_HEIGHT = 3;

// 윈도우 포인터
WINDOW *chat_win = nullptr;
WINDOW *input_win = nullptr;
WINDOW *user_count_win = nullptr;
WINDOW *username_win = nullptr;   // 추가: 사용자 이름 입력 윈도우
WINDOW *error_win = nullptr;      // 추가: 에러 메시지 윈도우
std::string current_username;

// 색상 쌍 정의
enum ColorPairs {
    COLOR_PAIR_INPUT = 1,
    COLOR_PAIR_SYSTEM = 2,
    COLOR_PAIR_MY_MESSAGE = 3,
    COLOR_PAIR_ERROR = 4  // 추가: 에러 메시지 색상 쌍
};

// 입력 포커스를 입력창으로 이동시키는 함수
void set_focus_to_input() {
    if (input_win) {
        // 입력창 활성화
        keypad(input_win, TRUE);
        // 커서 위치 설정
        wmove(input_win, 1, 2);
        // 커서 표시
        curs_set(1);
        // 화면 갱신
        wrefresh(input_win);
    }
}

// 현재 사용자 수
int current_user_count = 0;

// 타임스탬프 함수
std::string get_timestamp() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char time_str[10];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
    return std::string(time_str);
}

// 화면 완전히 지우기 함수 (개선)
void clear_all_windows() {
    // 모든 윈도우 삭제
    if (chat_win) delwin(chat_win);
    if (input_win) delwin(input_win);
    if (user_count_win) delwin(user_count_win);
    if (username_win) delwin(username_win);
    if (error_win) delwin(error_win);
    
    // 포인터 초기화
    chat_win = nullptr;
    input_win = nullptr;
    user_count_win = nullptr;
    username_win = nullptr;
    error_win = nullptr;
    
    // 화면 완전 초기화
    endwin();
    refresh();
    clear();
    refresh();
}

// ncurses 초기화 함수 개선
void init_ncurses() {
    // 화면이 이미 초기화되어 있으면 모두 정리
    if (stdscr) {
        clear_all_windows();
    }
    
    // UTF-8 한글 지원 설정
    setlocale(LC_ALL, "");
    
    // ncurses 초기화
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // 색상 설정
    start_color();
    init_pair(COLOR_PAIR_INPUT, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_PAIR_SYSTEM, COLOR_BLUE, COLOR_BLACK);  // 시스템 메시지용 파란색
    init_pair(COLOR_PAIR_MY_MESSAGE, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);    // 에러 메시지용 빨간색
    
    // 화면 크기 가져오기
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // 채팅 윈도우 생성
    chat_win = newwin(max_y - 3, max_x - USER_COUNT_WIDTH - 2, 0, 0);
    scrollok(chat_win, TRUE);
    
    // 입력 윈도우 생성
    input_win = newwin(3, max_x, max_y - 3, 0);
    box(input_win, 0, 0);
    mvwprintw(input_win, 0, 2, " Input ");
    
    // 사용자 수 윈도우 생성 (우측) - 기본 터미널 색상으로 변경
    user_count_win = newwin(USER_COUNT_HEIGHT, USER_COUNT_WIDTH, 0, max_x - USER_COUNT_WIDTH);
    box(user_count_win, 0, 0);
    mvwprintw(user_count_win, 0, 2, " Online Users ");
    mvwprintw(user_count_win, 1, 2, "Count: 0");
    
    // 화면 업데이트
    refresh();
    wrefresh(chat_win);
    wrefresh(input_win);
    wrefresh(user_count_win);
    
    // 입력창에 포커스 설정
    set_focus_to_input();
}

// 사용자 이름 입력 화면 설정 (개선)
void setup_username_screen() {
    // 화면이 이미 초기화되어 있으면 모두 정리
    if (stdscr) {
        clear_all_windows();
    }
    
    // UTF-8 한글 지원 설정
    setlocale(LC_ALL, "");
    
    // ncurses 기본 초기화
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // 색상 설정
    start_color();
    init_pair(COLOR_PAIR_INPUT, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);
    
    // 화면 클리어
    clear();
    refresh();
    
    // 화면 크기 가져오기
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // 닉네임 입력 창 중앙에 배치
    int height = 5;
    int width = 40;
    int starty = (max_y - height) / 2;
    int startx = (max_x - width) / 2;
    
    // 입력 윈도우를 중앙에 위치시킴
    username_win = newwin(height, width, starty, startx);
    box(username_win, 0, 0);
    mvwprintw(username_win, 0, 2, " Enter your username ");
    mvwprintw(username_win, 2, (width - 21) / 2, "Enter your username: ");
    
    wrefresh(username_win);
}

// 에러 메시지 표시 함수 (개선)
void show_error_message(const std::string& message) {
    // 에러 윈도우가 없으면 생성
    if (error_win == nullptr && username_win != nullptr) {
        int win_height, win_width;
        getmaxyx(username_win, win_height, win_width);
        
        // 사용자 이름 입력 창 위에 에러 메시지 표시
        error_win = newwin(1, win_width - 4, getbegy(username_win) - 2, getbegx(username_win) + 2);
    }
    
    if (error_win) {
        // 에러 메시지 출력
        werase(error_win);
        wattron(error_win, COLOR_PAIR(COLOR_PAIR_ERROR));
        wprintw(error_win, "%s", message.c_str());
        wattroff(error_win, COLOR_PAIR(COLOR_PAIR_ERROR));
        wrefresh(error_win);
    }
}

// 사용자 수 업데이트 함수 수정
void update_user_count(int count) {
    current_user_count = count;
    
    if (user_count_win) {
        werase(user_count_win);
        box(user_count_win, 0, 0);
        mvwprintw(user_count_win, 0, 2, " Online Users ");
        mvwprintw(user_count_win, 1, 2, "Count: %d", count);
        wrefresh(user_count_win);
        
        // 사용자 수 업데이트 후 입력창으로 포커스 복귀
        set_focus_to_input();
    }
}

// 채팅 메시지 표시 함수 수정
void print_chat_message(const std::string& sender, const std::string& content) {
    if (chat_win) {
        std::string timestamp = get_timestamp();
        if (sender == current_username) {
            wattron(chat_win, COLOR_PAIR(COLOR_PAIR_MY_MESSAGE));
        }
        wprintw(chat_win, "[%s] %s: %s\n", timestamp.c_str(), sender.c_str(), content.c_str());
        if (sender == current_username) {
            wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_MY_MESSAGE));
        }
        wrefresh(chat_win);
        set_focus_to_input();
    }
}

// 시스템 메시지 표시 함수 수정
void print_system_message(const std::string& message) {
    if (chat_win) {
        std::string timestamp = get_timestamp();
        
        // 파란색으로 시스템 메시지 출력
        wattron(chat_win, COLOR_PAIR(COLOR_PAIR_SYSTEM));
        wprintw(chat_win, "[%s] ** %s **\n", timestamp.c_str(), message.c_str());
        wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_SYSTEM));
        
        wrefresh(chat_win);
        
        // 메시지 출력 후 입력창으로 포커스 복귀
        set_focus_to_input();
    }
}

class ChatClient {
public:
    ChatClient(boost::asio::io_context& io_context, const tcp::resolver::results_type& endpoints)
        : io_context_(io_context), socket_(io_context), connected_(false) {
        connect(endpoints);
    }
    
    bool is_connected() const {
        return connected_;
    }
    
    void close() {
        boost::asio::post(io_context_, [this]() { 
            connected_ = false;
            socket_.close(); 
        });
    }
    
    void write(const wagle::Message& msg) {
        boost::asio::post(io_context_,
            [this, msg]() {
                bool write_in_progress = !write_msgs_.empty();
                write_msgs_.push_back(msg);
                if (!write_in_progress) {
                    writeImpl();
                }
            });
    }
    
    // 사용자 이름 검증 요청 함수 개선
    bool validate_username(const std::string& username, std::string& error_message) {
        if (!connected_) {
            error_message = "서버에 연결되어 있지 않습니다.";
            return false;
        }
        
        if (username.empty()) {
            error_message = "이름은 한 글자 이상 입력해야 합니다.";
            return false;
        }
        
        // 서버에 사용자 이름 검증 요청
        wagle::Message connect_msg(wagle::MessageType::CONNECT, username, "");
        
        std::string serialized_msg = connect_msg.serialize();
        boost::system::error_code ec;
        boost::asio::write(socket_, boost::asio::buffer(serialized_msg), ec);
        
        if (ec) {
            error_message = "서버 연결 오류: " + ec.message();
            return false;
        }
        
        // 서버 응답 대기
        boost::asio::streambuf response;
        boost::asio::read_until(socket_, response, '\n', ec);
        
        if (ec) {
            error_message = "서버 응답 오류: " + ec.message();
            return false;
        }
        
        // 응답 파싱
        std::istream is(&response);
        std::string data;
        std::getline(is, data);
        
        wagle::Message response_msg = wagle::Message::deserialize(data);
        
        // 응답이 오류 메시지인지 확인
        if (response_msg.getType() == wagle::MessageType::DISCONNECT) {
            error_message = "이미 사용 중인 이름입니다.";
            return false;
        }
        
        // 정상 처리
        return true;
    }
    
private:
    void connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, tcp::endpoint) {
                if (!ec) {
                    connected_ = true;
                    readMessages();
                } else {
                    connected_ = false;
                    print_system_message("Connect failed: " + ec.message());
                }
            });
    }
    
    void readMessages() {
        boost::asio::async_read_until(socket_, buffer_, '\n',
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 메시지 추출 및 표시
                    std::string data;
                    std::istream is(&buffer_);
                    std::getline(is, data);
                    
                    wagle::Message msg = wagle::Message::deserialize(data);
                    
                    switch (msg.getType()) {
                        case wagle::MessageType::CHAT_MSG:
                            print_chat_message(msg.getSender(), msg.getContent());
                            break;
                            
                        case wagle::MessageType::CONNECT:
                        case wagle::MessageType::DISCONNECT:
                            print_system_message(msg.getContent());
                            break;
                            
                        case wagle::MessageType::USER_COUNT:
                            update_user_count(std::stoi(msg.getContent()));
                            break;
                            
                        default:
                            break;
                    }
                    
                    // 다음 메시지 읽기
                    readMessages();
                } else {
                    connected_ = false;
                    print_system_message("Read failed: " + ec.message());
                    socket_.close();
                }
            });
    }
    
    void writeImpl() {
        std::string serialized_msg = write_msgs_.front().serialize();
        boost::asio::async_write(socket_,
            boost::asio::buffer(serialized_msg),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        writeImpl();
                    }
                } else {
                    connected_ = false;
                    print_system_message("Write failed: " + ec.message());
                    socket_.close();
                }
            });
    }
    
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    std::deque<wagle::Message> write_msgs_;
    bool connected_;  // 연결 상태 추적
};

int main(int argc, char* argv[]) {
    try {
        // UTF-8 한글 지원 설정 - 프로그램 시작 시 한 번만 설정
        setlocale(LC_ALL, "");
        
        // 서버 주소 및 포트 설정 (기본값 localhost:8080)
        std::string host = "localhost";
        std::string port = "8080";
        
        if (argc > 1) host = argv[1];
        if (argc > 2) port = argv[2];
        
        // IO 컨텍스트 및 리졸버 생성
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, port);
        
        // 클라이언트 생성
        ChatClient client(io_context, endpoints);
        
        // IO 스레드 시작
        std::thread io_thread([&io_context]() { io_context.run(); });
        
        // 서버 연결 대기
        int retry_count = 0;
        while (!client.is_connected() && retry_count < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retry_count++;
        }
        
        if (!client.is_connected()) {
            // ncurses 초기화
            initscr();
            printw("서버에 연결할 수 없습니다. 프로그램을 종료합니다.");
            refresh();
            getch();
            endwin();
            
            // 스레드 종료 및 종료
            io_context.stop();
            io_thread.join();
            return 1;
        }
        
        // 사용자 이름 입력 루프 (검증 로직 추가)
        bool username_valid = false;
        std::string username;
        std::string error_message;
        
        while (!username_valid) {
            // 사용자 이름 입력 화면 설정
            setup_username_screen();
            
            // 이전 에러 메시지가 있으면 표시
            if (!error_message.empty()) {
                show_error_message(error_message);
            }
            
            // 사용자 이름 입력
            wmove(username_win, 2, (40 - 21) / 2 + 21);  // 입력 위치로 이동
            echo();  // 입력 내용 표시
            char username_buf[32] = {0};
            wgetnstr(username_win, username_buf, sizeof(username_buf) - 1);
            noecho();  // 입력 내용 표시 중지
            
            username = username_buf;
            
            // 서버에 사용자 이름 검증 요청
            if (!client.validate_username(username, error_message)) {
                continue;
            }
            
            // 검증 성공
            username_valid = true;
            current_username = username;
        }

        // 채팅 화면으로 전환
        init_ncurses();
        
        // 채팅 메시지 입력 루프
        char input[256];
        while (true) {
            // 입력 받기
            wmove(input_win, 1, 2);
            wclrtoeol(input_win);
            box(input_win, 0, 0);
            mvwprintw(input_win, 0, 2, " Input ");
            
            // 포커스 설정
            set_focus_to_input();
            
            // 기존 방식으로 입력 받기
            echo();
            wgetnstr(input_win, input, sizeof(input) - 1);
            noecho();
            
            std::string line(input);
            
            // 사용자 수 윈도우 다시 그리기 (입력 중 가려질 수 있으므로)
            update_user_count(current_user_count);
            
            if (line == "/quit")
                break;
            
            // 채팅 메시지 전송
            client.write(wagle::Message(wagle::MessageType::CHAT_MSG, current_username, line));
        }

        // 종료
        client.close();
        io_thread.join();
        
        // ncurses 종료
        clear_all_windows();
        endwin();
    }
    catch (std::exception& e) {
        // ncurses 종료
        clear_all_windows();
        endwin();
        
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
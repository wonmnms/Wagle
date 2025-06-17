#include <iostream>
#include <string>
#include <thread>
#include <deque>
#include <vector>
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
WINDOW *username_win = nullptr;
WINDOW *error_win = nullptr;
WINDOW *room_list_win = nullptr;    // 채팅방 목록 윈도우
WINDOW *room_create_win = nullptr;  // 채팅방 생성 윈도우

std::string current_username;
std::string current_room;

// 색상 쌍 정의
enum ColorPairs {
    COLOR_PAIR_INPUT = 1,
    COLOR_PAIR_SYSTEM = 2,
    COLOR_PAIR_MY_MESSAGE = 3,
    COLOR_PAIR_ERROR = 4,
    COLOR_PAIR_SELECTED = 5  // 선택된 항목용
};

// 채팅방 정보 구조체
struct RoomInfo {
    std::string name;
    int user_count;
    bool is_default;
    
    RoomInfo(const std::string& n, int count, bool def) 
        : name(n), user_count(count), is_default(def) {}
};

std::vector<RoomInfo> room_list;
int selected_room_index = 0;

// 입력 포커스를 입력창으로 이동시키는 함수
void set_focus_to_input() {
    if (input_win) {
        keypad(input_win, TRUE);
        wmove(input_win, 1, 2);
        curs_set(1);
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

// 화면 완전히 지우기 함수
void clear_all_windows() {
    if (chat_win) delwin(chat_win);
    if (input_win) delwin(input_win);
    if (user_count_win) delwin(user_count_win);
    if (username_win) delwin(username_win);
    if (error_win) delwin(error_win);
    if (room_list_win) delwin(room_list_win);
    if (room_create_win) delwin(room_create_win);
    
    chat_win = nullptr;
    input_win = nullptr;
    user_count_win = nullptr;
    username_win = nullptr;
    error_win = nullptr;
    room_list_win = nullptr;
    room_create_win = nullptr;
    
    endwin();
    refresh();
    clear();
    refresh();
}

// ncurses 초기화 함수 (이모티콘 지원 개선)
void init_ncurses() {
    if (stdscr) {
        clear_all_windows();
    }
    
    // UTF-8 및 이모티콘 지원을 위한 로케일 설정
    setlocale(LC_ALL, "");
    setlocale(LC_CTYPE, "");
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // UTF-8 모드 활성화
    set_escdelay(25);  // ESC 키 지연 최소화
    
    start_color();
    init_pair(COLOR_PAIR_INPUT, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_PAIR_SYSTEM, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_PAIR_MY_MESSAGE, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    chat_win = newwin(max_y - 3, max_x - USER_COUNT_WIDTH - 2, 0, 0);
    scrollok(chat_win, TRUE);
    
    input_win = newwin(3, max_x, max_y - 3, 0);
    box(input_win, 0, 0);
    mvwprintw(input_win, 0, 2, " Input ");
    
    user_count_win = newwin(USER_COUNT_HEIGHT, USER_COUNT_WIDTH, 0, max_x - USER_COUNT_WIDTH);
    box(user_count_win, 0, 0);
    mvwprintw(user_count_win, 0, 2, " Online Users ");
    mvwprintw(user_count_win, 1, 2, "Count: 0");
    
    refresh();
    wrefresh(chat_win);
    wrefresh(input_win);
    wrefresh(user_count_win);
    
    set_focus_to_input();
}

// 채팅방 목록 화면 설정 (이모티콘 지원 개선)
void setup_room_list_screen() {
    if (stdscr) {
        clear_all_windows();
    }
    
    // UTF-8 및 이모티콘 지원을 위한 로케일 설정
    setlocale(LC_ALL, "");
    setlocale(LC_CTYPE, "");
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // UTF-8 모드 활성화
    set_escdelay(25);
    
    start_color();
    init_pair(COLOR_PAIR_INPUT, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_SELECTED, COLOR_BLACK, COLOR_WHITE);
    
    clear();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // 채팅방 목록 윈도우 중앙에 배치
    int height = max_y - 6;
    int width = max_x - 10;
    int starty = 3;
    int startx = 5;
    
    room_list_win = newwin(height, width, starty, startx);
    box(room_list_win, 0, 0);
    mvwprintw(room_list_win, 0, 2, " 💬 Chat Rooms ");  // 이모티콘 추가
    mvwprintw(room_list_win, height - 2, 2, " ⬆️⬇️: Navigate | ⏎: Join | C: Create Room | Q: Quit ");  // 이모티콘 추가
    
    wrefresh(room_list_win);
}

// 채팅방 목록 표시 (이모티콘 지원)
void display_room_list() {
    if (!room_list_win) return;
    
    int height, width;
    getmaxyx(room_list_win, height, width);
    
    // 기존 내용 지우기 (테두리와 제목 제외)
    for (int i = 1; i < height - 3; i++) {
        wmove(room_list_win, i, 1);
        for (int j = 1; j < width - 1; j++) {
            waddch(room_list_win, ' ');
        }
    }
    
    // 채팅방 목록 표시
    for (size_t i = 0; i < room_list.size() && i < (size_t)(height - 4); i++) {
        int y = 2 + i;
        
        if ((int)i == selected_room_index) {
            wattron(room_list_win, COLOR_PAIR(COLOR_PAIR_SELECTED));
        }
        
        // 이모티콘을 사용한 채팅방 표시
        std::string room_indicator = room_list[i].is_default ? "🏠" : "💬";
        mvwprintw(room_list_win, y, 3, "%s %-18s 👥 %d %s", 
                 room_indicator.c_str(),
                 room_list[i].name.c_str(), 
                 room_list[i].user_count,
                 room_list[i].is_default ? "[Default]" : "");
        
        if ((int)i == selected_room_index) {
            wattroff(room_list_win, COLOR_PAIR(COLOR_PAIR_SELECTED));
        }
    }
    
    wrefresh(room_list_win);
}

// 채팅방 생성 화면 설정 (이모티콘 지원)
void setup_room_create_screen() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int height = 5;
    int width = 50;
    int starty = (max_y - height) / 2;
    int startx = (max_x - width) / 2;
    
    room_create_win = newwin(height, width, starty, startx);
    box(room_create_win, 0, 0);
    mvwprintw(room_create_win, 0, 2, " ➕ Create New Room ");  // 이모티콘 추가
    mvwprintw(room_create_win, 2, 2, "Room Name: ");
    
    wrefresh(room_create_win);
}

// 사용자 이름 입력 화면 설정
void setup_username_screen() {
    if (stdscr) {
        clear_all_windows();
    }
    
    setlocale(LC_ALL, "");
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    start_color();
    init_pair(COLOR_PAIR_INPUT, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);
    
    clear();
    refresh();
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    int height = 5;
    int width = 40;
    int starty = (max_y - height) / 2;
    int startx = (max_x - width) / 2;
    
    username_win = newwin(height, width, starty, startx);
    box(username_win, 0, 0);
    mvwprintw(username_win, 0, 2, " Enter your username ");
    mvwprintw(username_win, 2, (width - 21) / 2, "Enter your username: ");
    
    wrefresh(username_win);
}

// 에러 메시지 표시 함수
void show_error_message(const std::string& message) {
    if (error_win == nullptr && username_win != nullptr) {
        int win_height, win_width;
        getmaxyx(username_win, win_height, win_width);
        
        error_win = newwin(1, win_width - 4, getbegy(username_win) - 2, getbegx(username_win) + 2);
    }
    
    if (error_win) {
        werase(error_win);
        wattron(error_win, COLOR_PAIR(COLOR_PAIR_ERROR));
        wprintw(error_win, "%s", message.c_str());
        wattroff(error_win, COLOR_PAIR(COLOR_PAIR_ERROR));
        wrefresh(error_win);
    }
}

// 사용자 수 업데이트 함수
void update_user_count(int count) {
    current_user_count = count;
    
    if (user_count_win) {
        werase(user_count_win);
        box(user_count_win, 0, 0);
        mvwprintw(user_count_win, 0, 2, " Online Users ");
        mvwprintw(user_count_win, 1, 2, "Count: %d", count);
        wrefresh(user_count_win);
        
        set_focus_to_input();
    }
}

// 채팅 메시지 표시 함수
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

// 시스템 메시지 표시 함수
void print_system_message(const std::string& message) {
    if (chat_win) {
        std::string timestamp = get_timestamp();
        
        wattron(chat_win, COLOR_PAIR(COLOR_PAIR_SYSTEM));
        wprintw(chat_win, "[%s] ** %s **\n", timestamp.c_str(), message.c_str());
        wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_SYSTEM));
        
        wrefresh(chat_win);
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
    
    bool validate_username(const std::string& username, std::string& error_message) {
        if (!connected_) {
            error_message = "서버에 연결되어 있지 않습니다.";
            return false;
        }
        
        if (username.empty()) {
            error_message = "이름은 한 글자 이상 입력해야 합니다.";
            return false;
        }
        
        wagle::Message connect_msg(wagle::MessageType::CONNECT, username, "");
        
        std::string serialized_msg = connect_msg.serialize();
        boost::system::error_code ec;
        boost::asio::write(socket_, boost::asio::buffer(serialized_msg), ec);
        
        if (ec) {
            error_message = "서버 연결 오류: " + ec.message();
            return false;
        }
        
        boost::asio::streambuf response;
        boost::asio::read_until(socket_, response, '\n', ec);
        
        if (ec) {
            error_message = "서버 응답 오류: " + ec.message();
            return false;
        }
        
        std::istream is(&response);
        std::string data;
        std::getline(is, data);
        
        wagle::Message response_msg = wagle::Message::deserialize(data);
        
        if (response_msg.getType() == wagle::MessageType::DISCONNECT) {
            error_message = "이미 사용 중인 이름입니다.";
            return false;
        }
        
        return true;
    }
    
    // 채팅방 목록 요청
    void request_room_list() {
        wagle::Message msg(wagle::MessageType::ROOM_LIST, current_username, "");
        write(msg);
    }
    
    // 채팅방 생성 요청
    void create_room(const std::string& room_name) {
        wagle::Message msg(wagle::MessageType::ROOM_CREATE, current_username, room_name);
        write(msg);
    }
    
    // 채팅방 입장 요청
    void join_room(const std::string& room_name) {
        wagle::Message msg(wagle::MessageType::ROOM_JOIN, current_username, room_name);
        write(msg);
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
                            
                        case wagle::MessageType::ROOM_LIST:
                            handle_room_list_response(msg.getContent());
                            break;
                            
                        case wagle::MessageType::ROOM_JOIN:
                            current_room = msg.getRoomName();
                            print_system_message(msg.getContent());
                            break;
                            
                        case wagle::MessageType::ROOM_ERROR:
                            print_system_message("Error: " + msg.getContent());
                            break;
                            
                        default:
                            break;
                    }
                    
                    readMessages();
                } else {
                    connected_ = false;
                    print_system_message("Read failed: " + ec.message());
                    socket_.close();
                }
            });
    }
    
    void handle_room_list_response(const std::string& data) {
        room_list.clear();
        
        if (data.empty()) return;
        
        std::string data_copy = data;  // const 매개변수의 복사본 생성
        size_t pos = 0;
        std::string token;
        
        while ((pos = data_copy.find(';')) != std::string::npos || !data_copy.substr(0).empty()) {
            if (pos != std::string::npos) {
                token = data_copy.substr(0, pos);
                data_copy.erase(0, pos + 1);
            } else {
                token = data_copy;
                data_copy.clear();
            }
            
            // room_name,user_count,is_default 형식 파싱
            size_t comma1 = token.find(',');
            size_t comma2 = token.find(',', comma1 + 1);
            
            if (comma1 != std::string::npos && comma2 != std::string::npos) {
                std::string name = token.substr(0, comma1);
                int count = std::stoi(token.substr(comma1 + 1, comma2 - comma1 - 1));
                bool is_default = (token.substr(comma2 + 1) == "1");
                
                room_list.emplace_back(name, count, is_default);
            }
            
            if (data_copy.empty()) break;
        }
        
        // 채팅방 목록 화면 갱신
        display_room_list();
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
    bool connected_;
};

int main(int argc, char* argv[]) {
    try {
        // UTF-8 및 이모티콘 지원을 위한 로케일 설정
        setlocale(LC_ALL, "");
        setlocale(LC_CTYPE, "");
        
        // 환경 변수 설정으로 터미널의 UTF-8 지원 강화
        setenv("LANG", "en_US.UTF-8", 1);
        setenv("LC_ALL", "en_US.UTF-8", 1);
        
        std::string host = "localhost";
        std::string port = "8080";
        
        if (argc > 1) host = argv[1];
        if (argc > 2) port = argv[2];
        
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(host, port);
        
        ChatClient client(io_context, endpoints);
        
        std::thread io_thread([&io_context]() { io_context.run(); });
        
        int retry_count = 0;
        while (!client.is_connected() && retry_count < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            retry_count++;
        }
        
        if (!client.is_connected()) {
            initscr();
            printw("서버에 연결할 수 없습니다. 프로그램을 종료합니다.");
            refresh();
            getch();
            endwin();
            
            io_context.stop();
            io_thread.join();
            return 1;
        }
        
        // 사용자 이름 입력
        bool username_valid = false;
        std::string username;
        std::string error_message;
        
        while (!username_valid) {
            setup_username_screen();
            
            if (!error_message.empty()) {
                show_error_message(error_message);
            }
            
            wmove(username_win, 2, (40 - 21) / 2 + 21);
            echo();
            char username_buf[32] = {0};
            wgetnstr(username_win, username_buf, sizeof(username_buf) - 1);
            noecho();
            
            username = username_buf;
            
            if (!client.validate_username(username, error_message)) {
                continue;
            }
            
            username_valid = true;
            current_username = username;
        }

        // 채팅방 목록 화면으로 전환
        setup_room_list_screen();
        client.request_room_list();
        
        // 채팅방 선택 루프
        bool room_selected = false;
        while (!room_selected) {
            int ch = getch();
            
            switch (ch) {
                case KEY_UP:
                    if (selected_room_index > 0) {
                        selected_room_index--;
                        display_room_list();
                    }
                    break;
                    
                case KEY_DOWN:
                    if (selected_room_index < (int)room_list.size() - 1) {
                        selected_room_index++;
                        display_room_list();
                    }
                    break;
                    
                case '\n':  // Enter key
                case '\r':
                    if (!room_list.empty() && selected_room_index < (int)room_list.size()) {
                        client.join_room(room_list[selected_room_index].name);
                        room_selected = true;
                    }
                    break;
                    
                case 'c':
                case 'C':
                    // 채팅방 생성 - 중괄호로 스코프 분리
                    {
                        setup_room_create_screen();
                        wmove(room_create_win, 2, 13);
                        echo();
                        char room_name_buf[32] = {0};
                        wgetnstr(room_create_win, room_name_buf, sizeof(room_name_buf) - 1);
                        noecho();
                        
                        if (strlen(room_name_buf) > 0) {
                            client.create_room(std::string(room_name_buf));
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                            client.request_room_list();
                        }
                        
                        delwin(room_create_win);
                        room_create_win = nullptr;
                        setup_room_list_screen();
                        display_room_list();
                    }
                    break;
                    
                case 'q':
                case 'Q':
                    clear_all_windows();
                    endwin();
                    client.close();
                    io_thread.join();
                    return 0;
                    
                default:
                    break;
            }
        }

        // 채팅 화면으로 전환
        init_ncurses();
        
        // 채팅 메시지 입력 루프
        char input[512];  // 이모티콘을 위해 버퍼 크기 증가
        bool return_to_rooms = false;
        
        while (true) {
            wmove(input_win, 1, 2);
            wclrtoeol(input_win);
            box(input_win, 0, 0);
            mvwprintw(input_win, 0, 2, " 💬 Input (/quit to exit, /rooms to return to room list) ");  // 이모티콘 추가
            
            set_focus_to_input();
            
            echo();
            wgetnstr(input_win, input, sizeof(input) - 1);
            noecho();
            
            std::string line(input);
            
            // 사용자 수 윈도우 다시 그리기
            update_user_count(current_user_count);
            
            if (line == "/quit") {
                break;
            } else if (line == "/rooms") {
                // 현재 채팅방에서 나가기
                client.write(wagle::Message(wagle::MessageType::ROOM_LEAVE, current_username, ""));
                return_to_rooms = true;
                break;
            } else {
                // 채팅 메시지 전송 (이모티콘 포함)
                client.write(wagle::Message(wagle::MessageType::CHAT_MSG, current_username, line));
            }
        }

        // 채팅방 목록으로 돌아가기
        if (return_to_rooms) {
            // 약간의 지연을 두어 서버에서 퇴장 처리 완료 대기
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // 채팅방 목록 화면으로 전환
            setup_room_list_screen();
            client.request_room_list();
            selected_room_index = 0;  // 선택 인덱스 초기화
            // 채팅방 선택 루프로 다시 돌아가기
            room_selected = false;
            while (!room_selected) {
                int ch = getch();
                switch (ch) {
                    case KEY_UP:
                        if (selected_room_index > 0) {
                            selected_room_index--;
                            display_room_list();
                        }
                        break;
                    case KEY_DOWN:
                        if (selected_room_index < (int)room_list.size() - 1) {
                            selected_room_index++;
                            display_room_list();
                        }
                        break;
                    case '\n':  // Enter key
                    case '\r':
                        if (!room_list.empty() && selected_room_index < (int)room_list.size()) {
                            client.join_room(room_list[selected_room_index].name);
                            room_selected = true;
                        }
                        break;
                    case 'c':
                    case 'C':
                        // 채팅방 생성 - 중괄호로 스코프 분리
                        {
                            setup_room_create_screen();
                            wmove(room_create_win, 2, 13);
                            echo();
                            char room_name_buf[32] = {0};
                            wgetnstr(room_create_win, room_name_buf, sizeof(room_name_buf) - 1);
                            noecho();
                            if (strlen(room_name_buf) > 0) {
                                client.create_room(std::string(room_name_buf));
                                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                client.request_room_list();
                            }
                            delwin(room_create_win);
                            room_create_win = nullptr;
                            setup_room_list_screen();
                            display_room_list();
                        }
                        break;
                    case 'q':
                    case 'Q':
                        clear_all_windows();
                        endwin();
                        client.close();
                        io_thread.join();
                        return 0;
                    default:
                        break;
                }
            }
            // 새로운 방 선택 후 다시 채팅 화면으로
            init_ncurses();
            return_to_rooms = false;  // 플래그 리셋
        }

        // 종료
        client.close();
        io_thread.join();
        
        clear_all_windows();
        endwin();
    }
    catch (std::exception& e) {
        clear_all_windows();
        endwin();
        
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
#include <locale.h>
#include <boost/asio.hpp>
#include <deque>
#include <iostream>
#include <string>
#include <thread>
#include <ncurses.h>

#include "protocol/message.h"

using boost::asio::ip::tcp;

// 윈도우 크기 및 위치 상수
const int ROOM_LIST_WIDTH = 25;   // 채팅방 목록 윈도우 너비
const int USER_LIST_WIDTH = 20;   // 사용자 목록 윈도우 너비
const int USER_COUNT_HEIGHT = 3;  // 사용자 수 표시 윈도우 높이

// 윈도우 포인터
WINDOW* chat_win = nullptr;        // 채팅 메시지 윈도우
WINDOW* input_win = nullptr;       // 입력 윈도우
WINDOW* room_list_win = nullptr;   // 채팅방 목록 윈도우
WINDOW* user_list_win = nullptr;   // 사용자 목록 윈도우
WINDOW* user_count_win = nullptr;  // 사용자 수 표시 윈도우
WINDOW* username_win = nullptr;    // 사용자 이름 입력 윈도우
WINDOW* error_win = nullptr;       // 에러 메시지 윈도우

// 현재 상태
std::string current_username;
std::string current_room_id;
std::string current_room_name = "General";
std::vector<std::string> room_list;
std::vector<std::string> user_list;
int current_user_count = 0;

// 색상 쌍 정의
enum ColorPairs {
    COLOR_PAIR_INPUT = 1,
    COLOR_PAIR_SYSTEM = 2,
    COLOR_PAIR_MY_MESSAGE = 3,
    COLOR_PAIR_ERROR = 4,
    COLOR_PAIR_ROOM_LIST = 5,
    COLOR_PAIR_USER_LIST = 6,
    COLOR_PAIR_SELECTED = 7
};

// 입력 포커스를 입력창으로 이동시키는 함수
void set_focus_to_input() {
    if (input_win) {
        keypad(input_win, TRUE);
        wmove(input_win, 1, 2);
        curs_set(1);
        wrefresh(input_win);
    }
}

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
    if (room_list_win) delwin(room_list_win);
    if (user_list_win) delwin(user_list_win);
    if (user_count_win) delwin(user_count_win);
    if (username_win) delwin(username_win);
    if (error_win) delwin(error_win);

    chat_win = nullptr;
    input_win = nullptr;
    room_list_win = nullptr;
    user_list_win = nullptr;
    user_count_win = nullptr;
    username_win = nullptr;
    error_win = nullptr;

    endwin();
    refresh();
    clear();
    refresh();
}

// ncurses 초기화 함수
void init_ncurses() {
    if (stdscr) {
        clear_all_windows();
    }

    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    // 색상 설정
    start_color();
    init_pair(COLOR_PAIR_INPUT, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOR_PAIR_SYSTEM, COLOR_BLUE, COLOR_BLACK);
    init_pair(COLOR_PAIR_MY_MESSAGE, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_PAIR_ERROR, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_ROOM_LIST, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_PAIR_USER_LIST, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_PAIR_SELECTED, COLOR_WHITE, COLOR_BLUE);

    // 화면 크기 가져오기
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // 채팅 윈도우 생성 (중앙)
    chat_win = newwin(max_y - 3, max_x - ROOM_LIST_WIDTH - USER_LIST_WIDTH - 2,
                      0, ROOM_LIST_WIDTH);
    scrollok(chat_win, TRUE);

    // 입력 윈도우 생성 (하단)
    input_win = newwin(3, max_x - ROOM_LIST_WIDTH - USER_LIST_WIDTH - 2,
                       max_y - 3, ROOM_LIST_WIDTH);
    box(input_win, 0, 0);
    mvwprintw(input_win, 0, 2, " Input ");

    // 채팅방 목록 윈도우 생성 (왼쪽)
    room_list_win = newwin(max_y, ROOM_LIST_WIDTH, 0, 0);
    box(room_list_win, 0, 0);
    mvwprintw(room_list_win, 0, 2, " Chat Rooms ");

    // 사용자 목록 윈도우 생성 (오른쪽)
    user_list_win = newwin(max_y - USER_COUNT_HEIGHT, USER_LIST_WIDTH, 0,
                           max_x - USER_LIST_WIDTH);
    box(user_list_win, 0, 0);
    mvwprintw(user_list_win, 0, 2, " Users ");

    // 사용자 수 윈도우 생성 (우측 상단)
    user_count_win =
        newwin(USER_COUNT_HEIGHT, USER_LIST_WIDTH, 0, max_x - USER_LIST_WIDTH);
    box(user_count_win, 0, 0);
    mvwprintw(user_count_win, 0, 2, " Online Users ");
    mvwprintw(user_count_win, 1, 2, "Count: 0");

    // 화면 업데이트
    refresh();
    wrefresh(chat_win);
    wrefresh(input_win);
    wrefresh(room_list_win);
    wrefresh(user_list_win);
    wrefresh(user_count_win);
}

// 채팅방 목록 업데이트
void update_room_list() {
    if (!room_list_win) return;

    werase(room_list_win);
    box(room_list_win, 0, 0);
    mvwprintw(room_list_win, 0, 2, " Chat Rooms ");

    int y = 1;
    for (const auto& room : room_list) {
        if (room == current_room_name) {
            wattron(room_list_win, COLOR_PAIR(COLOR_PAIR_SELECTED));
        }
        mvwprintw(room_list_win, y++, 2, "%s", room.c_str());
        if (room == current_room_name) {
            wattroff(room_list_win, COLOR_PAIR(COLOR_PAIR_SELECTED));
        }
    }

    wrefresh(room_list_win);
}

// 사용자 목록 업데이트
void update_user_list() {
    if (!user_list_win) return;

    werase(user_list_win);
    box(user_list_win, 0, 0);
    mvwprintw(user_list_win, 0, 2, " Users ");

    int y = 1;
    for (const auto& user : user_list) {
        mvwprintw(user_list_win, y++, 2, "%s", user.c_str());
    }

    wrefresh(user_list_win);
}

// 사용자 수 업데이트
void update_user_count(int count) {
    current_user_count = count;

    if (user_count_win) {
        werase(user_count_win);
        box(user_count_win, 0, 0);
        mvwprintw(user_count_win, 0, 2, " Online Users ");
        mvwprintw(user_count_win, 1, 2, "Count: %d", count);
        wrefresh(user_count_win);
    }

    set_focus_to_input();
}

// 채팅 메시지 표시
void print_chat_message(const std::string& sender, const std::string& content) {
    if (chat_win) {
        std::string timestamp = get_timestamp();
        if (sender == current_username) {
            wattron(chat_win, COLOR_PAIR(COLOR_PAIR_MY_MESSAGE));
        }
        wprintw(chat_win, "[%s] %s: %s\n", timestamp.c_str(), sender.c_str(),
                content.c_str());
        if (sender == current_username) {
            wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_MY_MESSAGE));
        }
        wrefresh(chat_win);
        set_focus_to_input();
    }
}

// 시스템 메시지 표시
void print_system_message(const std::string& message) {
    if (chat_win) {
        std::string timestamp = get_timestamp();
        wattron(chat_win, COLOR_PAIR(COLOR_PAIR_SYSTEM));
        wprintw(chat_win, "[%s] ** %s **\n", timestamp.c_str(),
                message.c_str());
        wattroff(chat_win, COLOR_PAIR(COLOR_PAIR_SYSTEM));
        wrefresh(chat_win);
        set_focus_to_input();
    }
}

// 에러 메시지 표시
void show_error_message(const std::string& message) {
    if (error_win == nullptr && username_win != nullptr) {
        int win_width;
        getmaxyx(username_win, win_width, win_width);
        error_win = newwin(1, win_width - 4, getbegy(username_win) - 2,
                           getbegx(username_win) + 2);
    }

    if (error_win) {
        werase(error_win);
        wattron(error_win, COLOR_PAIR(COLOR_PAIR_ERROR));
        wprintw(error_win, "%s", message.c_str());
        wattroff(error_win, COLOR_PAIR(COLOR_PAIR_ERROR));
        wrefresh(error_win);
    }
}

// 채팅 클라이언트 클래스
class ChatClient {
   public:
    ChatClient(boost::asio::io_context& io_context,
               const tcp::resolver::results_type& endpoints)
        : io_context_(io_context), socket_(io_context), connected_(false) {
        connect(endpoints);
    }

    bool is_connected() const { return connected_; }

    void close() {
        boost::asio::post(io_context_, [this]() {
            connected_ = false;
            socket_.close();
        });
    }

    void write(const wagle::Message& msg) {
        boost::asio::post(io_context_, [this, msg]() {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!write_in_progress) {
                writeImpl();
            }
        });
    }

    bool validate_username(const std::string& username,
                           std::string& error_message) {
        if (username.empty()) {
            error_message = "Username cannot be empty";
            return false;
        }

        if (username.length() > 20) {
            error_message = "Username too long (max 20 characters)";
            return false;
        }

        if (!connected_) {
            error_message = "Not connected to server";
            return false;
        }

        // 서버에 사용자 이름 검증 요청
        wagle::Message msg(wagle::MessageType::CONNECT, username, "");
        write(msg);

        // 응답 대기
        std::promise<bool> promise;
        std::future<bool> future = promise.get_future();

        // 응답 핸들러 설정
        response_handler_ = [&promise,
                             &error_message](const wagle::Message& response) {
            if (response.getType() == wagle::MessageType::USERNAME_ERROR) {
                error_message = response.getContent();
                promise.set_value(false);
            } else if (response.getType() == wagle::MessageType::CONNECT) {
                promise.set_value(true);
            }
        };

        try {
            // 응답 대기 (최대 3초)
            if (future.wait_for(std::chrono::seconds(3)) ==
                std::future_status::timeout) {
                error_message = "Server response timeout";
                std::cerr << "닉네임 검증: 서버 응답 타임아웃" << std::endl;
                return false;
            }
            std::cerr << "닉네임 검증: 서버 응답 수신" << std::endl;

            return future.get();
        } catch (const std::exception& e) {
            error_message =
                "Error validating username: " + std::string(e.what());
            return false;
        }
    }

   private:
    void connect(const tcp::resolver::results_type& endpoints) {
        boost::asio::async_connect(
            socket_, endpoints,
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
        boost::asio::async_read_until(
            socket_, buffer_, '\n',
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    std::string data;
                    std::istream is(&buffer_);
                    std::getline(is, data);

                    wagle::Message msg = wagle::Message::deserialize(data);

                    // 응답 핸들러가 있으면 호출
                    if (response_handler_) {
                        response_handler_(msg);
                        response_handler_ = nullptr;
                    }

                    switch (msg.getType()) {
                        case wagle::MessageType::CHAT_MSG:
                            print_chat_message(msg.getSender(),
                                               msg.getContent());
                            break;
                        case wagle::MessageType::CONNECT:
                        case wagle::MessageType::DISCONNECT:
                            print_system_message(msg.getContent());
                            break;
                        case wagle::MessageType::USER_COUNT:
                            update_user_count(std::stoi(msg.getContent()));
                            break;
                        case wagle::MessageType::ROOM_LIST: {
                            room_list.clear();
                            std::istringstream iss(msg.getContent());
                            std::string room;
                            while (std::getline(iss, room, '\n')) {
                                if (!room.empty()) {
                                    room_list.push_back(room);
                                }
                            }
                            update_room_list();
                            break;
                        }
                        case wagle::MessageType::ROOM_JOIN: {
                            current_room_id = msg.getRoomId();
                            current_room_name = msg.getContent();
                            print_system_message("Joined room: " +
                                                 current_room_name);
                            update_room_list();
                            break;
                        }
                        case wagle::MessageType::ROOM_LEAVE: {
                            print_system_message("Left room: " +
                                                 msg.getContent());
                            current_room_id.clear();
                            current_room_name.clear();
                            update_room_list();
                            break;
                        }
                        case wagle::MessageType::ROOM_ERROR: {
                            print_system_message("Room error: " +
                                                 msg.getContent());
                            break;
                        }
                        case wagle::MessageType::ROOM_CREATE:
                            print_system_message(msg.getContent());
                            if (!msg.getRoomId().empty()) {
                                current_room_id = msg.getRoomId();
                                current_room_name = msg.getContent();
                                update_room_list();
                            }
                            break;
                        case wagle::MessageType::USER_LIST:
                        case wagle::MessageType::USERNAME_ERROR:
                        case wagle::MessageType::ROOM_DELETE:
                        case wagle::MessageType::ROOM_INFO:
                        case wagle::MessageType::ROOM_MESSAGE:
                            // 필요시 구현
                            break;
                        default:
                            break;
                    }

                    readMessages();
                }
            });
    }

    void writeImpl() {
        boost::asio::async_write(
            socket_, boost::asio::buffer(write_msgs_.front().serialize()),
            [this](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) {
                        writeImpl();
                    }
                }
            });
    }

    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    std::deque<wagle::Message> write_msgs_;
    bool connected_;

    // 응답 핸들러
    std::function<void(const wagle::Message&)> response_handler_;
};

void setup_username_screen() {
    clear_all_windows();
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(COLOR_PAIR_INPUT, COLOR_WHITE, COLOR_BLUE);
    int win_height = 5, win_width = 40;
    int starty = (LINES - win_height) / 2;
    int startx = (COLS - win_width) / 2;
    username_win = newwin(win_height, win_width, starty, startx);
    box(username_win, 0, 0);
    mvwprintw(username_win, 1, (win_width - 21) / 2, "Enter your username:");
    wrefresh(username_win);
}

// 메인 함수
int main(int argc, char* argv[]) {
    try {
        boost::asio::io_context io_context;

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(argc > 1 ? argv[1] : "localhost",
                                          argc > 2 ? argv[2] : "8080");

        ChatClient client(io_context, endpoints);
        std::thread io_thread([&io_context]() { io_context.run(); });

        // 사용자 이름 입력
        std::string username;
        bool username_valid = false;
        std::string error_message;

        while (!username_valid) {
            setup_username_screen();

            if (!error_message.empty()) {
                show_error_message(error_message);
                // 에러 메시지 확인을 위해 엔터 입력 대기
                wgetch(username_win);
                error_message.clear();
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

        // 채팅 화면으로 전환
        init_ncurses();

        // 채팅 메시지 입력 루프
        char input[256];
        while (true) {
            wmove(input_win, 1, 2);
            wclrtoeol(input_win);
            box(input_win, 0, 0);
            mvwprintw(input_win, 0, 2, " Input ");

            set_focus_to_input();

            echo();
            wgetnstr(input_win, input, sizeof(input) - 1);
            noecho();

            std::string line(input);

            update_user_count(current_user_count);

            if (line == "/quit")
                break;
            else if (line == "/rooms") {
                client.write(wagle::Message(wagle::MessageType::ROOM_LIST,
                                            current_username, ""));
            } else if (line.substr(0, 6) == "/join ") {
                std::string room_name = line.substr(6);
                client.write(wagle::Message(wagle::MessageType::ROOM_JOIN,
                                            current_username, room_name));
            } else if (line == "/leave") {
                client.write(wagle::Message(wagle::MessageType::ROOM_LEAVE,
                                            current_username, ""));
            } else if (line.substr(0, 7) == "/create ") {
                std::string room_name = line.substr(7);
                client.write(wagle::Message(wagle::MessageType::ROOM_CREATE,
                                            current_username, room_name));
            } else {
                client.write(wagle::Message(wagle::MessageType::CHAT_MSG,
                                            current_username, line));
            }
        }

        // 종료
        client.close();
        io_thread.join();

        clear_all_windows();
        endwin();
    } catch (std::exception& e) {
        clear_all_windows();
        endwin();

        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
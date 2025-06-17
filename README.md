# Wagle 💬

Wagle은 C++로 개발된 터미널 기반 다중 채팅방 애플리케이션입니다. ncurses 라이브러리와 Boost.Asio를 사용하여 간단하고 효율적인 실시간 채팅 환경을 제공합니다.

## 주요 기능 ✨

- **다중 채팅방 지원**: 여러 채팅방 생성 및 자유로운 이동
- **실시간 채팅**: 메시지 송수신 및 사용자 상태 업데이트
- **사용자 관리**: 닉네임 중복 확인 및 사용자 수 표시
- **직관적인 UI**: 방향키 네비게이션과 명령어 기반 조작
- **유니코드 이모티콘 지원**: 😀🎉💬👥 등 다양한 이모티콘 사용 가능
- **UTF-8 완벽 지원**: 한글 및 다국어 지원
- **반응형 터미널 UI**: 창 크기 변경 대응

## 요구사항 📋

- C++17 이상
- CMake 3.10 이상
- Boost 라이브러리
- ncurses 라이브러리 (한글 지원을 위한 ncursesw)
- POSIX 호환 운영체제 (Linux, macOS)
- UTF-8 지원 터미널

### 의존성 설치 예시 (Ubuntu)
```bash
sudo apt update
sudo apt install build-essential cmake libboost-all-dev libncursesw5-dev
```

## 빌드 방법 🔨

1. 저장소 복제:
   ```bash
   git clone https://github.com/wonmnms/Wagle.git
   cd Wagle
   ```
2. 빌드 디렉토리 생성 및 이동:
   ```bash
   mkdir build
   cd build
   ```
3. CMake를 사용하여 빌드 파일 생성:
   ```bash
   cmake ..
   ```
4. 프로젝트 빌드:
   ```bash
   make
   ```
5. 빌드 정리 (필요시):
   ```bash
   make clean          # 오브젝트 파일만 삭제
   make clean-all      # 모든 빌드 파일 삭제
   ```

## 실행 방법 🚀

빌드 후 실행 파일은 `build/` 디렉토리에 생성됩니다.

### 서버 실행
```bash
./wagle_server [포트번호]
```
포트번호는 선택사항이며, 기본값은 8080입니다.

### 클라이언트 실행
```bash
./wagle_client [서버주소] [포트번호]
```
서버주소와 포트번호는 선택사항이며, 기본값은 각각 localhost와 8080입니다.

## 사용 방법 📖

### 1. 사용자 이름 입력 👤
- 클라이언트 실행 후 고유한 닉네임을 입력합니다.
- 중복된 닉네임은 사용할 수 없습니다.

### 2. 채팅방 목록 화면 💬
- **⬆️⬇️ (방향키)**: 채팅방 선택
- **⏎ (Enter)**: 선택한 채팅방 입장
- **C**: 새 채팅방 생성
- **Q**: 프로그램 종료

### 3. 채팅 화면 💭
- **일반 텍스트**: 채팅 메시지 전송 (이모티콘 포함 🎉😊💖)
- **`/rooms`**: 채팅방 목록으로 돌아가기
- **`/quit`**: 프로그램 종료

### 4. 이모티콘 사용 😊
다양한 유니코드 이모티콘을 채팅에서 사용할 수 있습니다:
- **Windows**: Win + . 또는 Win + ;
- **Ubuntu**: Ctrl + . ➡️ spcae 입력
- 감정: 😀😊😂😢😡😍🤔
- 액션: 👍👎👏🙌✋🤝
- 객체: 💖❤️💯🔥⭐🎉
- 기타: 🚀🌟💫🎯🎪🎭

## 프로젝트 구조 📁

```
Wagle/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── chat/
│   │   ├── chat_room.h
│   │   ├── chat_room_manager.h
│   │   └── user.h
│   ├── protocol/
│   │   └── message.h
│   └── socket/
│       └── socket_manager.h
├── src/
│   ├── client/
│   │   └── client_main.cpp
│   ├── common/
│   │   ├── chat_room.cpp
│   │   ├── chat_room_manager.cpp
│   │   ├── message.cpp
│   │   └── user.cpp
│   └── server/
│       ├── server_main.cpp
│       └── socket_manager.cpp
```

최신: 2025-06-17
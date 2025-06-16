# Wagle

Wagle 은 C++로 개발된 터미널 기반 채팅 애플리케이션입니다. ncurses 라이브러리와 Boost.Asio를 사용하여 간단하고 효율적인 실시간 채팅 환경을 제공합니다.

## 주요 기능

- 실시간 채팅 메시지 송수신
- 사용자 닉네임 중복 확인
- 채팅방 참여 및 퇴장 알림
- 현재 접속중인 사용자 수 표시
- 터미널 창 크기 변경에 반응하는 반응형 UI
- 한글 지원 (UTF-8)

## 요구사항

- C++17 이상
- CMake 3.10 이상
- Boost 라이브러리
- ncurses 라이브러리 (한글 지원을 위한 ncursesw)
- POSIX 호환 운영체제 (Linux, macOS)

## 빌드 방법

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

## 실행 방법

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

## 사용 방법

1. 클라이언트를 실행하면 닉네임을 입력하는 화면이 표시됩니다.
2. 닉네임을 입력하고 엔터를 누르면 채팅방에 입장됩니다.
   - 이미 사용 중인 닉네임이면 에러 메시지가 표시됩니다.
   - 닉네임이 비어있으면 에러 메시지가 표시됩니다.
3. 채팅 화면에서 메시지를 입력하고 엔터를 누르면 채팅방의 모든 사용자에게 메시지가 전송됩니다.
4. 채팅방을 나가려면 `/quit`을 입력하고 엔터를 누릅니다.

## 프로젝트 구조

```
Wagle/
├── CMakeLists.txt        - CMake 빌드 설정
├── include/              - 헤더 파일
│   ├── chat/            - 채팅 관련 클래스
│   ├── protocol/        - 메시지 프로토콜
│   └── socket/          - 소켓 통신 관련 클래스
├── src/                  - 소스 파일
│   ├── client/          - 클라이언트 관련 코드
│   ├── common/          - 공통 코드
│   └── server/          - 서버 관련 코드
└── .gitignore           - Git 제외 설정
```

## 문제 해결

- **빌드 오류**: 필요한 라이브러리가 모두 설치되어 있는지 확인하세요.
  ```bash
  # Ubuntu/Debian
  sudo apt-get install libboost-all-dev libncursesw5-dev
  
  # macOS (Homebrew)
  brew install boost ncurses
  ```

- **서버 연결 실패**: 방화벽 설정을 확인하고, 서버가 실행 중인지 확인하세요.

- **한글 깨짐**: 터미널이 UTF-8을 지원하는지 확인하세요.
  ```bash
  # 터미널 설정 확인
  echo $LANG
  # UTF-8 설정 (예: ko_KR.UTF-8)
  export LANG=ko_KR.UTF-8
  ```

## 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 LICENSE 파일을 참조하세요.

## 기여 방법

1. 이 저장소를 포크합니다.
2. 새 기능 브랜치를 생성합니다 (`git checkout -b feature/amazing-feature`).
3. 변경사항을 커밋합니다 (`git commit -m 'Add some amazing feature'`).
4. 브랜치를 푸시합니다 (`git push origin feature/amazing-feature`).
5. Pull Request를 생성합니다.

![](https://img.shields.io/badge/build-passing-brightgreen?style=for-the-badge)
# TOS Mini Shell

> **💡 본 프로젝트의 핵심 목표**
> 본 프로젝트는 완전히 고도화된 커널 자체를 구현하는 것이 아닌, 향후 **운영체제 코어 시뮬레이터를 유연하게 결합하기 위한 '앞단 인터페이스 및 시각화 플랫폼' 구축**을 목표로 합니다. 사용자가 선택한 스케줄링 알고리즘(FCFS, RR 등)에 따라 프로세스가 동적으로 처리되는 전이 과정을 명확하게 시각화하는 것에 초점을 맞추었습니다.

---

### 1 [Interface/Architecture] 스케줄링 시각화 및 확장을 위한 '독립적 멀티 큐(Multi-Queue)' 설계

* **현상 및 원인 (Problem)**
  * 초기 구조에서는 단일 큐 내부에서 `Ready` 상태와 `Waiting` 상태의 프로세스들이 명확히 격리되지 못하고 혼재되어 있었습니다.
  * 사용자가 시각화 화면에서 알고리즘(RR, FCFS 등)을 동적으로 변경할 때마다, 단일 큐 내부에서 특정 상태의 프로세스를 찾기 위해 매번 **$O(N)$의 전체 순회(Scan)** 가 강제되었습니다.
  * 결과적으로, 알고리즘 전환 시 프로세스 간 자원 간섭과 데이터 오염(Race Condition 및 포인터 꼬임)이 발생하여, 상태 전이 과정을 사용자에게 정확하게 보여주지 못하는 인터페이스 기능 마비 및 구조적 한계에 직면했습니다.

* **해결 방안 및 공학적 근거 (Why & How)**
  * 향후 어떤 코어 스케줄러가 붙더라도 프로세스의 상태 변화를 실시간으로 명확히 시각화할 수 있도록, **준비 큐(Ready Queue), 대기 큐(Waiting Queue)를 포함한 3개의 독립적인 멀티 큐 구조로 아키텍처를 전면 리팩토링**했습니다.
  * 상태별로 독립된 메모리 공간(자료구조)을 완벽히 분리함으로써, 스케줄러가 타임 슬라이스(Tick)마다 복잡한 탐색 루프 없이 **$O(1)$의 기계적인 Push/Pop 연산만으로 인터페이스 화면에 상태를 렌더링**하도록 격리시켰습니다.

* **결과 및 성과 (Value)**
  * 스케줄러가 타임 슬라이스마다 필요한 큐에 자원 간섭 없이 안전하게 접근할 수 있는 확장성을 확보했습니다. 이를 통해 사용자가 인터페이스 상에서 다양한 스케줄링 알고리즘에 따른 프로세스 처리 흐름을 결함 없이 실시간으로 모니터링할 수 있는 기반을 완성했습니다.

---

### 2. [DX/Resource] 시뮬레이션 연속성 확보를 위한 소프트 리셋 커맨드(`rs`) 구현

* **현상 및 원인 (Problem)**
  * 사용자가 다양한 프로세스 배치와 스케줄링 조건을 바꾸어가며 시뮬레이션을 반복 검증해야 하는데, 이전 실행에서 커널과 메모리에 적재된 잔여 프로세스 데이터(PCB)가 깔끔하게 해제되지 않는 문제가 있었습니다.
  * 이를 해결하기 위해 매번 프로세스(프로그램) 자체를 종료하고 다시 켜야 하는 하드 리셋 방식을 사용했습니다. 이는 인터페이스의 **연속적인 디버깅 세션과 사용자 테스트 흐름을 끊어버려 개발 생산성 및 사용성을 심각하게 저하**시켰으며, 프로그램 종료 미숙 시 메모리 누수(Memory Leak) 위험을 내포했습니다.

* **해결 방안 및 공학적 근거 (Why & How)**
  * 인터페이스의 화면 이탈(종료) 없이 런타임 환경을 유연하게 유지하기 위해, 내부 데이터만 안전하게 초기화하는 **소프트 리셋 커맨드(`rs`) 인터페이스를 커널 내부에 직접 구현**했습니다.
  * `rs` 명령어 호출 시, 화면과 데이터가 꼬이는 예외 상황을 막기 위해 **[커널 내부 상태 초기화 ➡️ 큐의 적재 데이터 클리어 ➡️ 할당되었던 PCB 메모리 자원 역순 일괄 해제(Deallocation)]** 메커니즘을 설계하여 데이터 무결성을 보장했습니다.

* **결과 및 성과 (Value)**
  * 프로그램을 재시작하는 번거로움 없이 터미널 내부에서 `rs` 명령어 입력만으로 즉시 새로운 스케줄링 시나리오를 테스트할 수 있게 되어 **테스트 연속성과 개발자 경험(DX)을 극대화**했으며, 시스템 자원이 고갈되는 메모리릭 문제를 완벽히 차단했습니다.
  
  ### C (C11 표준)

메모리 주소(Pointer)에 직접 접근하여 커널 수준의 자료구조(PCB, Queue)를 정밀하게 제어하기 위해 채택.

가비지 컬렉터(GC) 없이 메모리를 직접 관리(alloc/free)함으로써 시스템 자원 제어 메커니즘 이해 및 구현.

Data Structures & Architecture
Custom Multi-Queue Architecture

프로세스의 상태(Ready, Waiting) 전이를 독립적으로 관리하기 위한 자체 큐 자료구조 설계.

알고리즘 스위칭 시 자원 간섭을 최소화하는 하드웨어 친화적(Cache-friendly) 구조 지향.

### PCB (Process Control Block) Table
s
프로세스의 고유 속성(PID, 상태, Remain Time, Memory Address 등)을 관리하는 커널 핵심 테이블 설계.

### Interface & Environments


CLI (Command Line Interface)

rs (Soft Reset) 등 커널 제어 명령어를 직접 파싱하고 처리하는 유저 인터페이스 파이프라인 구축.

시뮬레이션 상태를 실시간 스냅샷(Snapshot) 테이블 형태로 터미널에 렌더링하는 가시화 로직 구현.

Development Environment


OS: Windows / Linux (Cross-platform 구조 설계)

IDE/Editor: VS Code (Git Bash 통합 환경 활용)

Version Control: Git / GitHub (Conventional Commits 표준 준수)


## 현재 폴더 구조

```text
OS/
├── main.c
├── Makefile
├── README.md
├── common/
│   ├── queue.c
│   ├── queue.h
│   ├── util.c
│   └── util.h
├── user/
│   └── shell/
│       ├── builtins.c
│       ├── builtins.h
│       ├── executor.c
│       ├── executor.h
│       ├── parser.c
│       ├── parser.h
│       ├── shell.c
│       └── shell.h
├── core/
│   ├── kernel/
│   │   ├── kernel.c
│   │   └── kernel.h
│   ├── process/
│   │   ├── process.c
│   │   └── process.h
│   ├── scheduler/
│   │   ├── scheduler.c
│   │   └── scheduler.h
│   ├── memory/
│   ├── filesystem/
│   └── io/
├── runtime/
├── tests/
└── docs/
    └── ARCHITECTURE.md
```

## 빌드

```bash
cd <project-root>
make
```

빌드 결과 실행 파일 이름은 `TOS`입니다.

```text
TOS
```

정리하려면 다음을 실행합니다.

```bash
make clean
```

## 실행

```bash
./TOS
```

프롬프트는 현재 다음처럼 표시됩니다.

```text
mini-shell>
```

## 현재 구현된 명령

- `cd <dir>`
  - 쉘 프로세스의 현재 작업 디렉터리를 변경합니다.
- `pwd`
  - 현재 작업 디렉터리를 출력합니다.
- `help`
  - 사용 가능한 내장 명령어를 출력합니다.
- `exit`
  - 쉘을 종료합니다.
- `create <name> <total_time>`
  - I/O가 없는 시뮬레이션용 프로세스 PCB를 생성합니다.
  - 생성된 PCB는 `READY` 상태가 되고 Ready Queue에 들어갑니다.
- `create <name> <total_time> io <io_at> <io_wait>`
  - 한 번의 I/O 이벤트를 가진 시뮬레이션용 프로세스 PCB를 생성합니다.
  - `io_at`은 `total_time`보다 작아야 합니다.
- `ps`
  - 현재 Running 프로세스, Ready Queue, Waiting Queue, Terminated 프로세스 목록을 먼저 출력합니다.
  - 그 다음 PCB 목록을 표 형태로 출력합니다.

내장 명령어가 아닌 입력은 외부 명령어로 간주하고 `fork()` + `execvp()` + `waitpid()`로 실행합니다.

## 실행 예시

시작하기 전 Tos라는 파일로 터미널을 통해 이동해야 하며 개발모드로만 운영되어 사용되고 stdlib.h stdio.h unistd.h 등의 헤더파일 라이브러리가 필요하며  
```text
./Tos
mini-shell> create P1 10
Created P1 (logical PID=1, state=READY, total=10)
  I/O: disabled
mini-shell> create P4 10 io 2 4
Created P4 (logical PID=2, state=READY, total=10)
  I/O: enabled at time 2, wait 4
mini-shell> ps
Running: (none)
Ready Queue: P1 P4
Waiting Queue: (empty)
Terminated: (empty)

PID  Name     State       Arrival Total   Remain  Wait    IO  IO@   IOW
1    P1       READY       0       10      10      0       N   -1    -1
2    P4       READY       0       10      10      0       Y   2     4
mini-shell> pwd
/home/user/OS
mini-shell> exit
```

## 현재 구현 상태

- 쉘 입력 루프는 `user/shell/shell.c`에 있습니다.
- 입력 파싱은 `user/shell/parser.c`에서 공백 기준으로 처리합니다.
- 내장 명령어 dispatch는 `user/shell/builtins.c`에서 처리합니다.
- 외부 명령어 실행은 `user/shell/executor.c`에서 처리합니다.
- PCB와 프로세스 테이블은 `core/process/`에 있습니다.
- Ready Queue와 Waiting Queue를 포함한 스케줄러 기본 구조는 `core/scheduler/`에 있습니다.
- 공용 정수 큐와 유틸리티 함수는 `common/`에 있습니다.


현재 `core/scheduler`에는 기본 알고리즘 값과 큐 구조가 있지만, 아직 실제 FCFS나 RR 실행 로직이 구현된 상태는 아닙니다. `ps`에서도 스케줄링 알고리즘은 사용자가 나중에 `set_sched`로 설정했을 때만 표시되도록 설계되어 있습니다.



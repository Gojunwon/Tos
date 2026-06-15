# TOS Mini Shell

현재 단계의 목표는 완성된 커널 시뮬레이터가 아니라, 이후 운영체제 코어 시뮬레이터를 붙이기 위한 앞단 인터페이스를 만드는 것이며 프로세스 스케줄링의 종류에 따라 프로세스가 어떻게 처리가 되는지 사용자가 볼 수 있게 만들어주는 프로젝트입니다.
현상 및 원인 (Problem)기존 구조에서는 준비(Ready) 상태와 대기(Waiting) 상태의 프로세스들이 단일 큐 내부에서 명확히 격리되지 못했습니다. 이로 인해 RR(Round-Robin)이나 FCFS 등 스케줄링 알고리즘을 동적으로 변경할 때, 상태 전환 프로세스 간의 자원 간섭과 데이터 오염(Race Condition 및 포인터 꼬임)이 발생하여 알고리즘 확장이 불가능한 구조적 한계가 있었습니다.

해결 방안 및 공학적 근거 (Why & How)프로세스의 상태 전이 구조를 엄격히 분리하기 위해, 준비 큐(Ready Queue), 대기 큐(Waiting Queue)를 포함한 3개의 독립적인 멀티 큐 구조로 아키텍처를 전면 리팩토링했습니다.상태별로 독립된 메모리 공간(자료구조)을 확보함으로써 알고리즘 전환 시 복잡도를 $O(N)$에서 $O(1)$ 수준으로 격리시켰습니다.
결과 및 성과 (Value)스케줄러가 타임 슬라이스(Tick)마다 필요한 큐에 자원 간섭 없이 안전하게 접근할 수 있도록 확장성을 확보했으며, 다양한 스케줄링 알고리즘을 결함 없이 완벽히 구동하는 데 성공했습니다.

[DX/Resource] 소프트 리셋 커맨드(rs) 구현을 통한 자원 누수 방지 및 테스트 효율화현상 및 원인 (Problem)시뮬레이션을 반복 실행할 때, 이전 실행에서 커널과 메모리에 적재된 잔여 프로세스 데이터가 깔끔하게 해제되지 않는 문제가 있었습니다.

이를 해결하기 위해 매번 프로세스(프로그램) 자체를 종료하고 재실행해야 했는데, 이는 메모리 누수(Memory Leak) 위험을 내포할 뿐만 아니라 연속적인 디버깅 세션을 방해하여 개발 생산성을 크게 떨어뜨렸습니다.해결 방안 및 공학적 근거 (Why & How)프로세스를 아예 종료하는 하드 리셋 대신, 런타임 환경을 유지한 채 내부 데이터만 안전하게 초기화하는 소프트 리셋 커맨드(rs) 인터페이스를 커널 내부에 직접 구현했습니다.rs 명령어 호출 시 커널 내부 상태, 큐의 적재 데이터, 그리고 할당되었던 PCB 메모리 자원을 역순으로 안전하게 일괄 해제(Deallocation) 하도록 메커니즘을 설계했습니다.



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

```text
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



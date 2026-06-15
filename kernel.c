#include <stdio.h>

#include "core/kernel/kernel.h"
#include "core/memory/memory.h"
#include "common/queue.h"

static void kernel_print_named_queue(const Kernel *kernel,
                                     FILE *stream,
                                     const char *label,
                                     const IntQueue *queue) {
    size_t queue_index;

    fprintf(stream, "%s: ", label);
    if (queue_is_empty(queue)) {
        fprintf(stream, "(empty)\n");
        return;
    }

    for (queue_index = 0; queue_index < queue_size(queue); ++queue_index) {
        int process_index;
        const PCB *pcb;

        if (queue_get(queue, queue_index, &process_index) != 0) {
            continue;
        }

        pcb = process_table_get_const(&kernel->process_table, process_index);
        if (pcb == NULL) {
            continue;
        }

        fprintf(stream, "%s", pcb->name);
        if (queue_index + 1U < queue_size(queue)) {
            fprintf(stream, " ");
        }
    }

    fprintf(stream, "\n");
}

static void kernel_print_running_process(const Kernel *kernel, FILE *stream) {
    int i;

    fprintf(stream, "Running: ");
    for (i = 0; i < kernel->process_table.count; ++i) {
        const PCB *pcb = &kernel->process_table.entries[i];

        if (pcb->state == PROCESS_RUNNING) {
            fprintf(stream, "%s\n", pcb->name);
            return;
        }
    }

    fprintf(stream, "(none)\n");
}

static void kernel_print_processes_in_state(const Kernel *kernel,
                                            FILE *stream,
                                            const char *label,
                                            ProcessState state) {
    int i;
    int found = 0;

    fprintf(stream, "%s: ", label);
    for (i = 0; i < kernel->process_table.count; ++i) {
        const PCB *pcb = &kernel->process_table.entries[i];

        if (pcb->state != state) {
            continue;
        }

        if (found) {
            fprintf(stream, " ");
        }
        fprintf(stream, "%s", pcb->name);
        found = 1;
    }

    if (!found) {
        fprintf(stream, "(empty)");
    }

    fprintf(stream, "\n");
}

void kernel_init(Kernel *kernel) {
    process_table_init(&kernel->process_table);
    scheduler_init(&kernel->scheduler);
    kernel->current_time = 0;
    memory_init(); 
}

int kernel_create_process(Kernel *kernel,
                          const char *name,
                          int total_work_time,
                          int io_enabled,
                          int io_at,
                          int io_wait,
                          PCB **pcb_out) {
    int process_index;
    PCB *pcb;

    if (process_table_create(&kernel->process_table,
                             name,
                             kernel->current_time,
                             total_work_time,
                             io_enabled,
                             io_at,
                             io_wait,
                             &pcb,
                             &process_index) != 0) {
        return -1;
    }
    int mem_size = 32;

    if (memory_alloc(mem_size, &pcb->base_addr) != 0) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }   
    pcb->mem_size = mem_size;
    pcb->state = PROCESS_READY;

    if (scheduler_enqueue_ready(&kernel->scheduler, process_index) != 0) {
        // 메모리 할당 실패 시 할당받았던 메모리 해제 로직 추가 필요
        // memory_free(pcb->base_addr); 
        pcb->state = PROCESS_NEW;
        return -1;
    }

    if (pcb_out != NULL) {
        *pcb_out = pcb;
    }

    return 0;
}

/* ⭐ 추가 기능 1: 시뮬레이션 시간 진행 및 상태 업데이트 (Tick) */
void kernel_step_tick(Kernel *kernel) {
    int i;
    kernel->current_time++;

    for (i = 0; i < kernel->process_table.count; ++i) {
        PCB *pcb = &kernel->process_table.entries[i];

        // 1. 실행 중인 프로세스의 남은 시간 감소
        if (pcb->state == PROCESS_RUNNING) {
            pcb->remaining_time--;
            
            // 만약 프로세스가 종료되었다면 메모리 해제 및 상태 변경
            if (pcb->remaining_time <= 0) {
                pcb->state = PROCESS_TERMINATED;
                // memory_free(pcb->base_addr); // 메모리 관리 모듈 함수 호출
            }
            // 중간에 I/O를 요청할 타이밍인지 체크하는 로직도 여기에 추가 가능
        }
        
        // 2. 레디 큐에 있는 프로세스들의 대기 시간 증가
        else if (pcb->state == PROCESS_READY) {
            pcb->waiting_time++;
        }
        
        // 3. I/O 대기 중인 프로세스의 대기 시간 감소 및 Ready 복귀 처리
        else if (pcb->state == PROCESS_WAITING) {
            // 이 시뮬레이터 구조에 맞는 I/O 카운트다운 필드가 있다면 감산 후 0일 때 Ready로 전환
        }
    }
    
    // 스케줄러 스케줄링 함수 호출 (예: scheduler_schedule(&kernel->scheduler);)
}

void kernel_print_process_snapshot(const Kernel *kernel, FILE *stream) {
    int i;

    if (scheduler_is_configured(&kernel->scheduler)) {
        fprintf(stream, "Scheduler: %s",
                scheduler_algorithm_name(kernel->scheduler.algorithm));
        if (kernel->scheduler.algorithm == SCHEDULER_RR) {
            fprintf(stream, " (quantum=%d)", kernel->scheduler.rr_quantum);
        }
        fprintf(stream, "\n");
    }

    kernel_print_running_process(kernel, stream);
    kernel_print_named_queue(kernel,
                             stream,
                             "Ready Queue",
                             scheduler_ready_queue(&kernel->scheduler));
    kernel_print_named_queue(kernel,
                             stream,
                             "Waiting Queue",
                             &kernel->scheduler.waiting_queue);
    kernel_print_processes_in_state(kernel,
                                    stream,
                                    "Terminated",
                                    PROCESS_TERMINATED);
    fprintf(stream, "\n");

    if (kernel->process_table.count == 0) {
        fprintf(stream, "No simulated processes have been created.\n");
        return;
    }

    /* ⭐ 변경 사항: 테이블 헤더에 Base(메모리 주소), Size(크기) 컬럼 추가 */
    fprintf(stream,
            "%-4s %-8s %-11s %-7s %-7s %-7s %-7s %-3s %-5s %-5s %-6s %-5s\n",
            "PID", "Name", "State", "Arrival", "Total", "Remain", "Wait", "IO", "IO@", "IOW", "Base", "Size");

    for (i = 0; i < kernel->process_table.count; ++i) {
        const PCB *pcb = &kernel->process_table.entries[i];
        fprintf(stream,
                "%-4d %-8s %-11s %-7d %-7d %-7d %-7d %-3s %-5d %-5d 0x%04X %-5d\n",
                pcb->logical_pid,
                pcb->name,
                process_state_name(pcb->state),
                pcb->arrival_time,
                pcb->total_work_time,
                pcb->remaining_time,
                pcb->waiting_time,
                pcb->io_enabled ? "Y" : "N",
                pcb->io_enabled ? pcb->io_at : -1,
                pcb->io_enabled ? pcb->io_wait : -1,
                pcb->base_addr,  /* ⭐ 추가: 메모리 시작 주소 (16진수 포맷) */
                pcb->mem_size);  /* ⭐ 추가: 할당된 메모리 크기 */
    }
}

void kernel_reset(Kernel *kernel) {
    /* ⭐ 추가 기능 2: 리셋할 때 기존 할당된 모든 메모리 해제 */
    // int i;
    // for (i = 0; i < kernel->process_table.count; ++i) {
    //     memory_free(kernel->process_table.entries[i].base_addr);
    // }

    process_table_init(&kernel->process_table);

    queue_clear(&kernel->scheduler.ready_queue);
    queue_clear(&kernel->scheduler.waiting_queue);

    kernel->current_time = 0;

    kernel->scheduler.is_configured = 0;
}

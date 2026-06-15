#include "core/scheduler/scheduler.h"
#include <stdio.h>
#include "core/kernel/kernel.h"
#include "core/process/process.h"


static void update_waiting_time(Scheduler *sched, Kernel *kernel) {
    size_t i;

    // Ready Queue
    for (i = 0; i < queue_size(&sched->ready_queue); ++i) {
        int idx;
        queue_get(&sched->ready_queue, i, &idx);

        PCB *pcb = &kernel->process_table.entries[idx];
        pcb->waiting_time++;
    }

    // Waiting Queue
    for (i = 0; i < queue_size(&sched->waiting_queue); ++i) {
        int idx;
        queue_get(&sched->waiting_queue, i, &idx);

        PCB *pcb = &kernel->process_table.entries[idx];
        pcb->waiting_time++;
    }
}


static int has_work(const Kernel *kernel) {
    int i;

    for (i = 0; i < kernel->process_table.count; ++i) {
        const PCB *pcb = &kernel->process_table.entries[i];
        if (pcb->state != PROCESS_TERMINATED) {
            return 1;
        }
    }
    return 0;
}

static void update_waiting(Scheduler *sched, Kernel *kernel) {
    size_t count = queue_size(&sched->waiting_queue);

for (size_t i = 0; i < count; i++) {
    int idx;
    queue_pop(&sched->waiting_queue, &idx);

    PCB *pcb = &kernel->process_table.entries[idx];
    pcb->io_remaining--;

    if (pcb->io_remaining <= 0) {
        pcb->state = PROCESS_READY;
        scheduler_enqueue_ready(sched, idx);
    } else {
        queue_push(&sched->waiting_queue, idx);
    }
}

}
void scheduler_step(Scheduler *sched, Kernel *kernel) {
    if (!sched->is_configured) {
        printf("Scheduler not set\n");
        return;
    }

    kernel->current_time++;

    printf("\n[TIME %d]\n", kernel->current_time);

    // waiting 처리
    update_waiting_time(sched, kernel);
    update_waiting(sched, kernel);

    static int current_idx = -1;
    static int quantum_counter = 0;

    // CPU 비어있으면 가져오기
    if (current_idx == -1) {
        if (queue_pop(&sched->ready_queue, &current_idx) != 0) {
            printf("CPU idle\n");
            kernel_print_process_snapshot(kernel, stdout);
            return;
        }
        quantum_counter = 0;
    }

    PCB *pcb = &kernel->process_table.entries[current_idx];
    pcb->state = PROCESS_RUNNING;

    if (pcb->start_time == -1) {
        pcb->start_time = kernel->current_time;
    }

    printf("Running: %s\n", pcb->name);
    kernel_print_process_snapshot(kernel, stdout);
    // 실행
    pcb->remaining_time--;
    pcb->executed_time++;
    quantum_counter++;

    // I/O
    if (pcb->io_enabled && pcb->executed_time == pcb->io_at) {
        pcb->state = PROCESS_WAITING;
        pcb->io_remaining = pcb->io_wait;

        queue_push(&sched->waiting_queue, current_idx);

        current_idx = -1;
        return;
    }

    // 종료
    if (pcb->remaining_time <= 0) {
        pcb->state = PROCESS_TERMINATED;
        pcb->end_time = kernel->current_time;

        current_idx = -1;
        return;
    }

    // RR
    if (sched->algorithm == SCHEDULER_RR &&
        quantum_counter >= sched->rr_quantum) {

        pcb->state = PROCESS_READY;
        queue_push(&sched->ready_queue, current_idx);

        current_idx = -1;
    }
}

void scheduler_run(Scheduler *sched, Kernel *kernel) {
    if (!sched->is_configured) {
        printf("Scheduler not set\n");
        return;
    }

    printf("[RUN START] %s\n",
           scheduler_algorithm_name(sched->algorithm));

    while (has_work(kernel)) {
        scheduler_step(sched, kernel);
    }

    printf("\n[ALL PROCESSES FINISHED]\n");
}



void scheduler_init(Scheduler *scheduler) {
    scheduler->algorithm = SCHEDULER_FCFS;
    scheduler->is_configured = 0;
    scheduler->rr_quantum = DEFAULT_RR_QUANTUM;
    queue_init(&scheduler->ready_queue);
    queue_init(&scheduler->waiting_queue);
}

int scheduler_is_configured(const Scheduler *scheduler) {
    return scheduler->is_configured;
}

const char *scheduler_algorithm_name(SchedulerAlgorithm algorithm) {
    switch (algorithm) {
    case SCHEDULER_FCFS:
        return "FCFS";
    case SCHEDULER_RR:
        return "RR";
    default:
        return "UNKNOWN";
    }
}

int scheduler_enqueue_ready(Scheduler *scheduler, int process_index) {
    return queue_push(&scheduler->ready_queue, process_index);
}

size_t scheduler_ready_count(const Scheduler *scheduler) {
    return queue_size(&scheduler->ready_queue);
}

const IntQueue *scheduler_ready_queue(const Scheduler *scheduler) {
    return &scheduler->ready_queue;
}


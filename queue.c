#include "common/queue.h"

void queue_init(IntQueue *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
}
void queue_clear(IntQueue *q) {
    q->head = 0;
    q->tail = 0;
    q->size = 0;
}


int queue_is_empty(const IntQueue *queue) {
    return queue->size == 0;
}

int queue_is_full(const IntQueue *queue) {
    return queue->size >= QUEUE_CAPACITY;
}

size_t queue_size(const IntQueue *queue) {
    return queue->size;
}

int queue_push(IntQueue *queue, int value) {
    if (queue_is_full(queue)) {
        return -1;
    }

    queue->items[queue->tail] = value;
    queue->tail = (queue->tail + 1U) % QUEUE_CAPACITY;
    queue->size++;
    return 0;
}

int queue_pop(IntQueue *queue, int *value_out) {
    if (queue_is_empty(queue)) {
        return -1;
    }

    *value_out = queue->items[queue->head];
    queue->head = (queue->head + 1U) % QUEUE_CAPACITY;
    queue->size--;
    return 0;
}

int queue_peek(const IntQueue *queue, int *value_out) {
    if (queue_is_empty(queue)) {
        return -1;
    }

    *value_out = queue->items[queue->head];
    return 0;
}

int queue_get(const IntQueue *queue, size_t index, int *value_out) {
    size_t actual_index;

    if (index >= queue->size) {
        return -1;
    }

    actual_index = (queue->head + index) % QUEUE_CAPACITY;
    *value_out = queue->items[actual_index];
    return 0;
}
int queue_remove(IntQueue *q, size_t index) {
    if (index >= q->size) return -1;

    size_t i;
    size_t real_idx;

    for (i = index; i + 1 < q->size; i++) {
        size_t from = (q->head + i + 1) % QUEUE_CAPACITY;
        size_t to   = (q->head + i) % QUEUE_CAPACITY;
        q->items[to] = q->items[from];
    }

    // tail 한 칸 뒤로
    q->tail = (q->tail + QUEUE_CAPACITY - 1) % QUEUE_CAPACITY;
    q->size--;

    return 0;
}


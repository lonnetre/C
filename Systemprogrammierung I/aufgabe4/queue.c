#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "queue.h"
#include "sem.h"

typedef struct QueueNode {
    char *cmd;
    char *out;
    int flags;
    struct QueueNode *next;
} QueueNode;

static QueueNode *head = NULL;
static QueueNode *tail = NULL;

static SEM *mutex;
static SEM *items;

int queue_init(void) {
    items = semCreate(0);
    mutex = semCreate(1);
    if (mutex == NULL || items == NULL) {
        if (mutex != NULL) semDestroy(mutex);
        if (items != NULL) semDestroy(items);
        mutex = NULL;
        items = NULL;
        return -1;
    }
    head = NULL;
    tail = NULL;
    return 0;
}

void queue_deinit(void) {
    if (mutex) { semDestroy(mutex); mutex = NULL; }
    if (items) { semDestroy(items); items = NULL; }
    head = NULL;
    tail = NULL;
}

int queue_put(char *cmd, char *out, int flags) {
    QueueNode *new_node = (QueueNode*)malloc(sizeof(QueueNode));
    if (new_node == NULL) return -1;

    new_node->cmd = cmd;
    new_node->out = out;
    new_node->flags = flags;
    new_node->next = NULL;

    P(mutex);
    if (tail != NULL) {
        tail->next = new_node;
    } else {
        head = new_node;
    }
    tail = new_node;
    V(mutex);
    V(items);

    return 0;
}

int queue_get(char **cmd, char **out, int *flags) {
    P(items);
    P(mutex);

    if (head == NULL) {
        V(mutex);
        return -1;
    }

    QueueNode *node = head;
    head = node->next;
    if (head == NULL) tail = NULL;

    *cmd = node->cmd;
    *out = node->out;
    *flags = node->flags;

    free(node);
    V(mutex);
    return 0;
}

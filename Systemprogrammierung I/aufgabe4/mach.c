#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "run.h"
#include "queue.h"
#include <pthread.h>

#define QEVT_RUNNING  (INT_MIN)
#define QEVT_SHUTDOWN (INT_MIN + 1)

static void die(const char *s) {
    perror(s);
    exit(EXIT_FAILURE);
}

static int parse_positive_int_or_die(char *str) {
    errno = 0;
    char *endptr;
    long x = strtol(str, &endptr, 10);
    if (errno != 0) die("invalid number");
    if (str == endptr || *endptr != '\0') {
        fprintf(stderr, "invalid number\n");
        exit(EXIT_FAILURE);
    }
    if (x <= 0) {
        fprintf(stderr, "number not positive\n");
        exit(EXIT_FAILURE);
    }
    if (x > INT_MAX) {
        fprintf(stderr, "number too large\n");
        exit(EXIT_FAILURE);
    }
    return (int)x;
}

typedef struct {
    char *cmd;
    char *out;
} Command;

static void *output_thread_func(void *arg) {
    (void)arg;
    for (;;) {
        char *cmd = NULL;
        char *out = NULL;
        int flags = 0;
        int rc = queue_get(&cmd, &out, &flags);
        if (rc == -1) break;

        if (flags == QEVT_RUNNING) {
            if (cmd) printf("Running `%s` ...\n", cmd);
            free(cmd);
            free(out);
            continue;
        }
        if (flags == QEVT_SHUTDOWN) {
            free(cmd);
            free(out);
            break;
        }

        printf("Completed `%s`: \"%s\".\n", cmd ? cmd : "", out ? out : "");
        free(cmd);
        free(out);
    }
    return NULL;
}

static void *executeCommands(void *arg) {
    Command *command = (Command*)arg;

    char *start_msg = strdup(command->cmd);
    if (!start_msg) die("strdup");
    if (queue_put(start_msg, NULL, QEVT_RUNNING) != 0) {
        fprintf(stderr, "Failed queue_put (running)\n");
        exit(EXIT_FAILURE);
    }

    int returnValue = run_cmd(command->cmd, &(command->out));

    if (queue_put(command->cmd, command->out, returnValue) != 0) {
        fprintf(stderr, "Failed queue_put (completed)\n");
        exit(EXIT_FAILURE);
    }

    free(command);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_threads> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int numberOfThreads = parse_positive_int_or_die(argv[1]);

    if (queue_init() != 0) die("queue_init failed");

    FILE *file = fopen(argv[2], "r");
    if (file == NULL) die("Failed to open file");

    pthread_t output_thread;
    int ret = pthread_create(&output_thread, NULL, output_thread_func, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_create(output): %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }

    pthread_t threads[numberOfThreads];
    int used[numberOfThreads];
    for (int i = 0; i < numberOfThreads; i++) used[i] = 0;

    char line[4097];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] == '\n' || line[0] == '\0') {
            for (int i = 0; i < numberOfThreads; i++) {
                if (used[i]) {
                    pthread_join(threads[i], NULL);
                    used[i] = 0;
                }
            }
            continue;
        }

        int slot = -1;
        while (slot == -1) {
            for (int i = 0; i < numberOfThreads; i++) {
                if (!used[i]) { slot = i; break; }
            }
            if (slot == -1) {
                for (int i = 0; i < numberOfThreads; i++) {
                    if (used[i]) {
                        pthread_join(threads[i], NULL);
                        used[i] = 0;
                        break;
                    }
                }
            }
        }

        Command *command = malloc(sizeof(Command));
        if (!command) die("malloc");
        command->cmd = strdup(line);
        if (!command->cmd) die("strdup");
        command->out = NULL;

        ret = pthread_create(&threads[slot], NULL, executeCommands, command);
        if (ret != 0) {
            fprintf(stderr, "pthread_create(worker): %s\n", strerror(ret));
            exit(EXIT_FAILURE);
        }
        used[slot] = 1;
    }

    if (ferror(file)) die("fgets");

    for (int i = 0; i < numberOfThreads; i++) {
        if (used[i]) {
            pthread_join(threads[i], NULL);
            used[i] = 0;
        }
    }

    if (queue_put(NULL, NULL, QEVT_SHUTDOWN) != 0) {
        fprintf(stderr, "Failed queue_put (shutdown)\n");
        exit(EXIT_FAILURE);
    }

    ret = pthread_join(output_thread, NULL);
    if (ret != 0) {
        fprintf(stderr, "pthread_join(output): %s\n", strerror(ret));
    }

    if (fclose(file) != 0) perror("fclose");

    queue_deinit();
    exit(EXIT_SUCCESS);
}

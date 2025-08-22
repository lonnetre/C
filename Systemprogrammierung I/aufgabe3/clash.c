#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "plist.h"

#define PROMPT_SIZE 1338

static void memoryFree(char **array, int amount) {
    for(int i=0; i<amount; i++) {
        free(array[i]);
    }
    free(array);
}

static char* getCwd(void) {
    /*
        The  getcwd()  function shall place an absolute pathname of the current working directory in the array pointed
        to by buf, and return buf.  The pathname shall contain no components that are dot or dot-dot, or are  symbolic
        links.
        - source: getcwd(3p)
    */
    long path_max;
    size_t size;
    char *buf;
    char *ptr;

    path_max = pathconf(".", _PC_PATH_MAX);
    if (path_max == -1) {
        size = 1024;
    } else if (path_max > 10240) {
        size = 10240;
    } else {
        size = path_max;
    }

    for (buf = ptr = NULL; ptr == NULL; size *= 2)
    {
        if ((buf = realloc(buf, size)) == NULL)
        {
            perror("realloc");
            exit(EXIT_FAILURE);
        }

        ptr = getcwd(buf, size);
        if (ptr == NULL && errno != ERANGE)
        {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
    }
    return buf;
}

static void printPrompt() {
    char *cwd = getCwd();
    printf("%s: ", cwd);

    if(fflush(stdout) == EOF) {
        perror("fflsuh");
    }
    free(cwd);
}

static void removeNewLine(char *array){
    size_t arrayLength = strlen(array);
    if(array[arrayLength-1] == '\n') {
        array[arrayLength-1] = '\0';
    }
}


static bool checkBuffer(char *array) {
    size_t arrayLength = strlen(array);

    if(array[arrayLength-1] == '\n') {
        return true;
    }

    int c;
    while(true) {
        c = fgetc(stdin);
        if(ferror(stdin)) {
            perror("fgetc");
            exit(EXIT_FAILURE);
        }

        if((c != EOF) && (c != '\n')) {
            continue;
        }
        break;
    }
    return false;
}

static char** getTokens(char *array, int *tokens) {
    char **extractTokens = NULL;
    int tokensCounter = 0;

    const char delimeter[] = " \t\n";
    char *token = strtok(array, delimeter);
    while (token != NULL) {
        extractTokens = realloc(extractTokens, (tokensCounter+1) * sizeof(char*));
        //ERROR-Handling for realloc
        if (extractTokens == NULL){
            perror("realloc");
            exit(EXIT_FAILURE);
        }

        extractTokens[tokensCounter] = strdup(token);
        if (extractTokens[tokensCounter] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }

        tokensCounter++;
        token = strtok(NULL, delimeter);
    }

    if (extractTokens != NULL) {
        extractTokens = realloc(extractTokens, (tokensCounter + 1) * sizeof(char*));
        if (extractTokens == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        extractTokens[tokensCounter++] = (char*)NULL;
    }

    *tokens = tokensCounter;
    return extractTokens;
}

static void removeAndPrintZombies() {
    char tempPrompt[1338];
    int status;
    pid_t pid;

    while (true) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) {
            return;
        }
        if (pid < 0) {
            if (errno == ECHILD) {
                return;
            }
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if (removeElement(pid,tempPrompt,1338) > -1) {
            if (WIFEXITED(status)) {
                printf("Exitstatus [%s] = %d\n", tempPrompt, WEXITSTATUS(status));
            }
            else {
                printf("No exitstatus [%s]\n",tempPrompt);
            }
        }
    }
}

static void runProcess(char** tokens, char *buffer, bool checkBackground) {

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }else if (pid == 0) {
        //Childprocess
        execvp(tokens[0], tokens);
        perror("execvp");
        exit(EXIT_FAILURE);
    }else {
        //Parentprocess
        if (checkBackground) {
            int retVal = insertElement(pid, buffer);
            if (retVal == -2) { //-2 is returned if malloc fails -> exit after error message
                fprintf(stderr, "insertElement: insufficient memory to complete the operation\n");
                exit(EXIT_FAILURE);
            }
            else if (retVal == -1) { //shell should still function fine, exiting not necessary
                fprintf(stderr, "insertElement: a pair with the given pid already exists\n");
            }
        }else {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(status)) {
                printf("Exitstatus [%s] = %d\n", buffer, WEXITSTATUS(status));
            }else {
                printf("No Exitstatus");
            }
        }
    }
}

static bool checkBackgroundProcess (char *array) {
    size_t length = strlen(array);
    if (array[length-1] == '&') {
        array[length-1] = '\0';
        return true;
    }else {
        return false;
    }
}

static void changeDir(char** tokens, int numberOfTokens) {
    char* directory = tokens[1];

    //numberOfTokens includes NULL pointer
    if (numberOfTokens != 3) {
        fprintf(stderr, "usage: cd <directory>\n");
    } else if (chdir(directory) == -1) {
        perror("chdir");
    }
}


static int printProcessInfo(pid_t pid, const char* prompt) {
    printf("%d %s\n", pid, prompt);
    return 0;
}

static void showJobs(int numberOfTokens) {
    //numberOfTokens includes NULL pointer
    if (numberOfTokens != 2) {
        fprintf(stderr, "usage: jobs\n");
        return;
    }

    walkList(printProcessInfo);
}

int main(void) {
    char buffer[PROMPT_SIZE];
    char **strings = NULL;

    int tokensCounter = 0;
    while(true) {
        printPrompt();
        if(!fgets(buffer, PROMPT_SIZE, stdin)) {
            //ERROR-Handling "fgets"
            if(ferror(stdin)) {
                perror("fgets");
                exit(EXIT_FAILURE);
            }else {
                break;
            }
        }

        size_t length = strlen(buffer);
        if (length == 1 || (length > 1 && strspn(buffer, " \t\n") == length)) {
            removeAndPrintZombies();
            continue;
        }
        if(checkBuffer(buffer)) {
            if(length == 1) {
                removeAndPrintZombies();
                continue;
            }

            char *cpyBuffer = strdup(buffer);
            if (cpyBuffer == NULL) {
                perror("strdup");
                exit(EXIT_FAILURE);
            }
            removeNewLine(cpyBuffer);
            bool isBackground = checkBackgroundProcess(cpyBuffer);

            if (isBackground) {
                buffer[length-2] = '\0';
            }
            strings = getTokens(buffer, &tokensCounter);

            if (strcmp(strings[0], "cd") == 0) {
                changeDir(strings, tokensCounter);
                continue;
            }else if (strcmp(strings[0], "jobs") == 0) {
                showJobs(tokensCounter);
                free(cpyBuffer);
                removeAndPrintZombies();
                memoryFree(strings, tokensCounter);
                continue;
            }
            
            runProcess(strings, cpyBuffer, isBackground);        
            free(cpyBuffer);
            removeAndPrintZombies();
            memoryFree(strings, tokensCounter);
        }else {
            fprintf(stderr, "Input too long! only 1337 bytes are allowed\n");
        }
    }

    exit(EXIT_SUCCESS);
}
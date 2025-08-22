
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "halde.h"

int main(int argc, char *argv[]) {
    printList();

    // First test
    char *m1 = malloc(200*1024);
    if(m1 == NULL){
        perror("malloc");
        abort();
    }
    printList();

    // Second test with edge cases: allocating the maximum amount of memory, allocating memory of size 0, or requesting more memory than the system can provide.
    char *m2 = malloc(0);
    if(m2 == NULL){
        perror("malloc");
        abort(); 
    }
    printList();

    // Third test
    char *m3 = malloc(200*1024);
    if(m3 == NULL){
        perror("malloc");
        abort();
    }
    printList();

    // Fourth test
    char *m4 = malloc(200*1024);
    if(m4 == NULL){
        perror("malloc");
        abort();
    }
    printList();

    free(m1);
    free(m2);
    free(m3);
    free(m4);
    printList();

    abort();
}

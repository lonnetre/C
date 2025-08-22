#include "halde.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

/// Magic value for occupied memory chunks.
#define MAGIC ((void*)0xbaadf00d)

/// Size of the heap (in bytes).
#define SIZE (1024*1024*1)

/// Memory-chunk structure.
struct mblock {
    struct mblock* next;
    size_t size;
    char memory[];
};

/// Heap-memory area.
static char* memory;

/// Pointer to the first element of the free-memory list.
static struct mblock* head;

/// Helper function to visualise the current state of the free-memory list.
void printList(void) {
    struct mblock* lauf = head;

    // Empty list
    if (head == NULL) {
        char empty[] = "(empty)\n";
        write(STDERR_FILENO, empty, sizeof(empty));
        // return;
        abort();
    }

    // Print each element in the list
    const char fmt_init[] = "(off: %7zu, size:: %7zu)";
    const char fmt_next[] = " --> (off: %7zu, size:: %7zu)";
    const char* fmt = fmt_init;
    char buffer[sizeof(fmt_next) + 2 * 7];

    while (lauf) {
        size_t n = snprintf(buffer, sizeof(buffer), fmt, (uintptr_t)lauf - (uintptr_t)memory, lauf->size);
        if (n) {
            write(STDERR_FILENO, buffer, n);
        }

        lauf = lauf->next;
        fmt = fmt_next;
    }
    write(STDERR_FILENO, "\n", 1);
}

void* malloc(size_t size) {
    // Error handling for size
    if (size == 0) {
        return NULL;
    }
    if (size > SIZE) {
        errno = ENOMEM;
        return NULL;
    }

    // Initialize memory on first malloc call
    if (memory == NULL) {
        memory = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (memory == MAP_FAILED) {
            errno = ENOMEM;
            return NULL;
        }
        head = (struct mblock*)memory;
        head->next = NULL;
        head->size = SIZE - sizeof(struct mblock);
    }

    struct mblock* current = head;
    struct mblock* previous = NULL; // previous element of in the list

    // Loop goes through memory list
    while (current != NULL) {
        // if current->size is smaller than the requested size, it cannot fulfill the allocation
        if (current->size >= size) {
            // the current memory block is larger than the requested size
            if (current->size > size + sizeof(struct mblock)) {
                struct mblock* new_block = (struct mblock*)(current->memory + size);
                // adjusting the sizes and linking the new block to the list
                new_block->next = current->next;
                new_block->size = current->size - size - sizeof(struct mblock); // see if statement
                current->next = new_block;
                current->size = size;
            }
            // if current block is the first element
            if (previous == NULL) {
                head = current->next;
            }
            else {
                // skipping current block
                // Removes the current block and updates the free-memory list correctly
                previous->next = current->next;
            }
            // marking as "occupied" ( = MAGIC)
            current->next = MAGIC;
            return current->memory;
        }
        previous = current;
        current = current->next;
    }

    // Error handling if current == NULL
    errno = ENOMEM;
    return NULL;
}

void free(void* ptr) {
    // no memory block was allocated => there is nothing to free
    if (ptr == NULL) {
        abort();
    }

    // starting address of the memory
    struct mblock* block = (struct mblock*)((char*)ptr - sizeof(struct mblock));

	if(block == NULL){
		abort();
	}

    // checks, if next block is occupied
    if (block->next != MAGIC) {
        abort();
    }

    // connecting the freed block to the previous first block in the list
    block->next = head;
    head = block;
}

void* realloc(void* ptr, size_t size) {
    // checks, if memory block was allocated
    if (ptr == NULL) {
        return malloc(size);
    }
    // user wants to deallocate memory
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // address of the memory block head
    struct mblock* block = (struct mblock*)((char*)ptr - sizeof(struct mblock));
    // new memory block
    void* new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    // if memory reallocation is successful
    if (new_ptr) {
        // copy the data from the old memory block to the new
        memcpy(new_ptr, ptr, (size < block->size) ? size : block->size);
        free(ptr);
    }

    return new_ptr;
}

// allocates memory for an array of elements and initializes all bytes in the allocated memory to zero
void* calloc(size_t nmemb, size_t size) {

    if (nmemb == 0 || size == 0) {
        return NULL;
    }
    // total_size of memory block to be allocated
    size_t total_size = nmemb * size;
    void* ptr = malloc(total_size);

    // if memory allocation is successful
    if (ptr) {
        memset(ptr, 0, total_size);
    }

    return ptr;
}

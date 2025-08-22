#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORD_LENGTH 100

// Function to compare strings for qsort
static int compare_strings(const void *a, const void *b) {
    char* const* ca = (char* const*) a;
    char* const* cb = (char* const*) b;

    return strcmp(*ca, *cb);
}

// Function to free memory allocated for an array of strings
static void freeMemory(char **array, int amount){
    for (int i = 0; i < amount; i++) {
        free(array[i]);
    }
    free(array);
}


int main() {

    char **array_von_strings = NULL;
    int size_of_array = 0;
    char string[MAX_WORD_LENGTH + 2]; // Temporary storage for input strings (+ \0 \n)

    // Read strings from stdin until EOF is reached
    while(fgets(string, MAX_WORD_LENGTH + 2, stdin) != NULL){

        // Skip empty lines
        if (strcmp(string, "\n") == 0) {
            continue;
        }

        // Check for words longer than MAX_WORD_LENGTH - 1 characters and if the last character is not a newline character  
        if (string[strlen(string) - 1] != '\n' && strlen(string) >= MAX_WORD_LENGTH - 1) {
            int c;

            // loop reads characters from stdin and discards them until either a newline character or EOF is encountered
            while ((c = fgetc(stdin)) != '\n' && c != EOF); 

            // Error handling for stdin
            if(ferror(stdin)) {
                perror("fgetc");
                exit(EXIT_FAILURE);
            }

            // if the word is wrong => buffer is cleared successfully
            fprintf(stderr, "zu langes Wort\n");
            continue;
        }

        // Check if last char = "\n" => substitute with "\0"
        if (string[strlen(string) - 1] == '\n') {
            string[strlen(string) - 1] = '\0';
        }

        // Allocate memory for the new string
        array_von_strings = realloc(array_von_strings, (size_of_array + 1) * sizeof(char*));

        // Error handling for realloc
        if (array_von_strings == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }

        // Allocate memory for the string itself
        array_von_strings[size_of_array] = malloc((strlen(string) + 1) * sizeof(char));

        // Error handling for malloc
        if (array_von_strings[size_of_array] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        // Copy the string into the array
        strcpy(array_von_strings[size_of_array], string);
        size_of_array++;
    }

    // Error handling for fgets
    if(ferror(stdin)){
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    // Sort the array of strings
    qsort(array_von_strings, size_of_array, sizeof(char*), compare_strings);

    // Print the sorted strings to stdout
    for (int i = 0; i < size_of_array; i++) {
        fputs(array_von_strings[i], stdout);
        fputs("\n", stdout);
    }

    fflush(stdout);
    // Error handling for fflush
    if (ferror(stdout)){
        perror("fflush");
        exit(EXIT_FAILURE);
    }

    // Error handling for stdout and stderr
    if(ferror(stdout) || ferror(stderr)){
        perror("stdout or stderr");
        exit(EXIT_FAILURE);
    }

    // Free allocated memory
    freeMemory(array_von_strings, size_of_array);

    // stdout, stdin, stderr werden üblicherweise nicht geschlossen
    // fclose(stdin); 
    // fclose(stdout);
    return 0;
}


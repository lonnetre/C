#include <stdio.h>
#include <stdlib.h>

struct node {
    int value;
    struct node* next;
};

static struct node* head = NULL;

int insertElement(int value) {
    if (value < 0) {
        return -1;
    }

    // Check if the value already exists in the list
    struct node* temp = head;
    while (temp != NULL) {
        if (temp->value == value) {
            return -1;
        }
        temp = temp->next;
    }

    // Allocate memory for the new node
    temp = (struct node*) malloc(sizeof(struct node));
    if (temp == NULL) {
        return -1;
    }
    temp->value = value;
    temp->next = NULL;

    // Insert the new node at the end of the list
    if (head == NULL) {
        head = temp;
    } else {
        struct node* lastNode = head;
        while (lastNode->next != NULL) {
            lastNode = lastNode->next;
        }
        lastNode->next = temp;
    }
    return value;
}

int removeElement(void) {
    if (head == NULL) {
        return -1;
    }

    int returnValue = head->value;
    struct node* temp = head;
    head = head->next;
    free(temp);

    return returnValue;
}

int main (int argc, char* argv[]) {
    printf("insert 47: %d\n", insertElement(47));
    printf("insert 11: %d\n", insertElement(11));
    printf("insert 23: %d\n", insertElement(23));
    printf("insert 11: %d\n", insertElement(11));
    printf("insert -1: %d\n", insertElement(-1)); // Test negative value insertion

    printf("\n");

    printf("remove: %d\n", removeElement());
    printf("remove: %d\n", removeElement());

    // Free remaining nodes in the list before exiting
    struct node* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }

    return EXIT_SUCCESS;
}

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* concat(char* a, char* b) {
    size_t len = strlen(a) + strlen(b) + 1;
    char* result = malloc(len);
    strcpy(result, a);
    strcat(result, b);
    return result;
}

char* int_to_string(int x) {
    char buffer[32];
    sprintf(buffer, "%d", x);
    return strdup(buffer);
}
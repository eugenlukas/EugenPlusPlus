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

char* input_str()
{
    char buffer[4096];

    if (!fgets(buffer, sizeof(buffer), stdin))
        return NULL;

    // Remove trailing newline
    buffer[strcspn(buffer, "\n")] = '\0';

    char* result = (char*)malloc(strlen(buffer) + 1);
    strcpy(result, buffer);

    return result;
}

int input_num()
{
    char buffer[256];

    while(1)
    {
        if (!fgets(buffer, sizeof(buffer), stdin))
            return 0;

        char* end;
        long value = strtol(buffer, &end, 10);

        if (end != buffer && (*end == '\n' || *end == '\0'))
            return (int)value;

        printf("'%s' must be a number. Try again!\n", buffer);
    }
}
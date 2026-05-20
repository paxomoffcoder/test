#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static int is_english_vowel(char c)
{
    char lower = (char)tolower((unsigned char)c);

    return lower == 'a' || lower == 'e' || lower == 'i' || lower == 'o' || lower == 'u';
}

int main(void)
{
    enum { CHUNK_SIZE = 10 };
    size_t capacity = CHUNK_SIZE;
    size_t length = 0;
    char *input = malloc(capacity);
    int vowels = 0;
    int ch;

    if (input == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    while ((ch = getchar()) != EOF && ch != '\n') {
        if (length + 1 >= capacity) {
            size_t new_capacity = capacity + CHUNK_SIZE;
            char *resized = realloc(input, new_capacity);

            if (resized == NULL) {
                fprintf(stderr, "Memory reallocation failed.\n");
                free(input);
                return 1;
            }

            input = resized;
            capacity = new_capacity;
        }

        input[length++] = (char)ch;
    }

    if (length == 0 && ch == EOF) {
        printf("0\n");
        free(input);
        return 0;
    }

    input[length] = '\0';

    for (size_t i = 0; i < length; ++i) {
        if (is_english_vowel(input[i])) {
            ++vowels;
        }
    }

    printf("%d\n", vowels);
    free(input);
    return 0;
}

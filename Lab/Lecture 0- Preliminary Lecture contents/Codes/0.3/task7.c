#include <stdio.h>

int main() {
    FILE *file;
    char buffer[200];
    char str[2];   // for single character string

    /* Open file for writing */
    file = fopen("input.txt", "w+");
    if (file == NULL) {
        printf("Error opening file for writing.\n");
        return 1;
    }

    /* Write a-z using loop */
    for (char i = 'a'; i <= 'z'; i++) {
        str[0] = i;
        str[1] = '\0';
        fputs(str, file);
    }

    /* Write A-Z using loop */
    for (char i = 'A'; i <= 'Z'; i++) {
        str[0] = i;
        str[1] = '\0';
        fputs(str, file);
    }

    fclose(file);

    /* Reopen file for reading */
    file = fopen("input.txt", "r");
    if (file == NULL) {
        printf("Error opening file for reading.\n");
        return 1;
    }

    /* Read and print */
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    fclose(file);
    return 0;
}

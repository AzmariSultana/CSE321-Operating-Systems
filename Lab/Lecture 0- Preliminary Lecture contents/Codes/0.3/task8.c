#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    int n = argc - 1;
    int numbers[n];

    // Convert command line arguments to integers
    for (int i = 0; i < n; i++) {
        numbers[i] = atoi(argv[i + 1]);
    }

    // Bubble sort
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (numbers[j] > numbers[j + 1]) {
                int temp = numbers[j];
                numbers[j] = numbers[j + 1];
                numbers[j + 1] = temp;
            }
        }
    }

    // Print sorted numbers
    printf("Sorted numbers:\n");
    for (int i = 0; i < n; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");

    return 0;
}

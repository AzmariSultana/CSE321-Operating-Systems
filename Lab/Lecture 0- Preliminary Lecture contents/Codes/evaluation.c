#include <stdio.h>

int main() {
    int n;
    
    printf("Enter the number of elements: ");
    scanf("%d", &n);
    
    int arr[n];
    
    for (int i = 0; i < n; i++) {
        printf("Enter element %d: ", i + 1);
        scanf("%d", &arr[i]);
    }
    
    int even_count = 0, odd_count = 0;
    for (int i = 0; i < n; i++) {
        if (arr[i] % 2 == 0) {
            even_count++;
        } else {
            odd_count++;
        }
    }
    
    if (even_count == odd_count) {
        char message[100];
        printf("Enter a message: ");
        scanf("%s", message); 
        printf("%s %d\n", message, even_count);
    } else {
        
        printf("Even numbers: %d\n", even_count);
        printf("Odd numbers: %d\n", odd_count);
    }
    
    return 0;
}

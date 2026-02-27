#include <stdio.h>

void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

int main() {
  int n;
  
  printf("Enter the number of elements:\n");
  scanf("%d", &n);
  
  int arr[n];
  printf("Enter the elements:\n");
  for (int i=0; i<n; i++){
    scanf("%d", &arr[i]);
  }
  
  int index1, index2;
  printf("Enter the indices of 2 elements \n");
  scanf("%d %d", &index1, &index2);
  
  if (index1 >=0 && index1<n  && index2>=0 && index2<n){
    swap(&arr[index1], &arr[index2]);
    
    printf("Array after swapping: ");
    for (int i=0; i<n; i++){
      printf("%d" , arr[i]);
    }
    printf("\n");
  }else {
    printf("Invalid\n");
  }
  return 0;
}
  

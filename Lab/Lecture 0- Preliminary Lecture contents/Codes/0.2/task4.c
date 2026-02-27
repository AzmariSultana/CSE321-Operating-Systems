#include <stdio.h>

int main(){
  int arr[6];
  int i;
  
  printf("enter elements:\n");
  for(i=0; i<6; i++){
    scanf("%d", &arr[i]);
  }
  
  printf("array elements are:\n");
  for (i=0;i<6;i++){
    printf("%d", arr[i]);
  }
  
  return 0;
}
  

#include <stdio.h>

int main(){
  int i,n;
  
  printf("Enter elements:");
  scanf("%d", &n);
  int arr[n];
  
  printf("Enter %d elements:\n",n);
  for(i=0;i<n;i++){
  scanf("%d", &arr[i]);
  }
  
  printf("in reverse order:\n");
  for(i=n-1;i>=0;i--){
    printf("%d",arr[i]);
  }
  printf("\n");
  
  return 0;
}
  
  
  

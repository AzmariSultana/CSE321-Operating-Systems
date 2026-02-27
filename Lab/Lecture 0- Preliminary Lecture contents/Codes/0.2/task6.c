#include <stdio.h>

int main(){
  int total_seconds, hours, minutes, seconds;
  
  printf("Enter time in seconds: ");
  scanf("%d", &total_seconds);
  
  hours= total_seconds / 3600;
  minutes= (total_seconds % 3600)/60;
  seconds= total_seconds % 60;
  
  printf("%d seconds is: %d hours, %d minutes, %d seconds\n", total_seconds, hours, minutes, seconds);
  
  return 0;
}

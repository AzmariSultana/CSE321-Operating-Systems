#include <stdio.h>

int main(){
  int angle1, angle2, angle3;
  
  printf("Enter the angles: ");
  scanf("%d %d %d", &angle1, &angle2, &angle3);
  
  if (angle1 > 0 && angle2 >0 && angle3 > 0 && angle1+angle2+angle3==180){
    printf("Yes, a triangle can be formed\n");
  } else{
    printf("No, a triangle can't be formed\n");
  }
  
  return 0;
}
      

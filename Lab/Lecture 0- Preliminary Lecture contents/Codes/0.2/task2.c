#include <stdio.h>

int main(){
  int height=7;
  int width=5;
  int perimeter, area;
  
  perimeter= 2* (height + width);
  area= height * width;
  
  printf("Rectangle area: %d square inches\n",area);
  printf("Perimeter: %d inches\n", perimeter);
  
  return 0;
}

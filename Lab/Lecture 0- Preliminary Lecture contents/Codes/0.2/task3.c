#include <stdio.h>

int main(){
  float radius, perimeter, area;
  
  printf("Enter the radius: ");
  scanf("%f", &radius);
  
  perimeter= 2* 3.1416*radius;
  area = 3.1416*radius*radius;
  
  printf("Radius:  %f\n"  , radius);
  printf("Perimeter: %f\n", perimeter);
  printf("Area: %f\n", area);
  
  return 0;
}
  


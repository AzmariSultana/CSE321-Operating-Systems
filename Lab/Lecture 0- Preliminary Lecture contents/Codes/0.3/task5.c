#include <stdio.h>

float calculateDiameter(float radius) {
  return 2 * radius;
}

float calculateCircumference(float radius){
  return 2 * 3.1416 * radius;
}

float calculateArea(float radius) {
  return 3.1416 * radius * radius;
}

int main(){
  float radius;
  
  printf("Enter the radius of the circle:\n");
  scanf("%f", &radius);
  
  float diameter= calculateDiameter(radius);
  float circumference= calculateCircumference(radius);
  float area= calculateArea(radius);
  
  printf("Radius %.2f:\n", radius);
  printf("Diameter= %.2f\n", diameter);
  printf("Circumference = %.2f\n", circumference);
  printf("Area = %.2f\n", area);
  
  return 0;
}

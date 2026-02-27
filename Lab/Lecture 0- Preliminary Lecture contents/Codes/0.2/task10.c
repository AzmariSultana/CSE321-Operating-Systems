#include <stdio.h>

struct Student {
  char name[50];
  int id;
  float cgpa;
};

int main(){
  struct Student s;
  
  printf("Enter student name: ");
  scanf("%s", &s.name);
  
  printf("Enter Student ID: ");
  scanf("%d", &s.id);
  
  printf("Enter Student CGPA: ");
  scanf("%f", &s.cgpa);
  
  printf("Name: %s\n", s.name);
  printf("ID: %d\n", s.id);
  printf("CGPA: %.2f\n", s.cgpa);
  
  return 0;
}
  


#include <stdio.h>
#include <string.h>

int main(){
  char str1[10], str2[6];
  
  printf("Enter first string: ");
  scanf("%s", &str1);
  
  printf("Enter second string: ");
  scanf("%s", &str2);
  
  strcat(str1,str2);
  
  printf("Concatenate string: %s\n", str1);
  
  return 0;
}
  
  

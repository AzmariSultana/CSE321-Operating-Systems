#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]){
    FILE *fp;
    char input[256];
    
    if (argc!=2){
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    fp=fopen(argv[1], "a");

    if (fp==NULL){
        printf("Error opening file.\n");
        return 1;
    }
    while (1) {
        printf("Enter a string (-1 to stop): ");
        fgets(input, sizeof(input), stdin);

        input[strcspn(input,"\n")]='\0';

        if (strcmp(input, "-1")==0){
            break;
        }

        fprintf(fp, "%s\n", input);
    }
    fclose(fp);
    printf("Data written successfully.\n");

    return 0;
}
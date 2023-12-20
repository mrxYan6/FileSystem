#include <stdio.h>
#include <string.h>

int main(){
    char str[] = "root/homework/05/FileSystem";
    char* token = strtok(str, "/");
    // printf("%s\n", token);
    // printf("%s\n", token);
    while(token!=NULL){
        printf("%d\n", 1);
        printf("%s\n", token);
        token = strtok(NULL, "/");
    }
    return 0;
}
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

int redirect_out_trunc(char* filename[], int count, bool redirect){
    int file;
    for (int i = 0 ; i < count ; i++){
        file = open(filename[i], O_CREAT | O_TRUNC |O_WRONLY, 0777);
        if (file == -1){
            perror("Open did not work\n");
            return -1;
        }
        if (i != count - 1) close(file);
    }
    if (redirect){
        if (dup2(file, STDOUT_FILENO) == -1){
            perror("Dup2 did not work\n");
            return -1;
        }
    }
    
    if (count > 0) close(file);

    return 0;
}

int redirect_out_append(char* filename[], int count, bool redirect){   
    int file;
    for (int i = 0 ; i < count ; i++){
        file = open(filename[i], O_CREAT | O_APPEND |O_WRONLY, 0777);
        if (file == -1){
            perror("Open did not work\n");
            return -1;
        }
        if (i != count - 1) close(file);
    }
    if (redirect){
        if (dup2(file, STDOUT_FILENO) == -1){
            perror("Dup2 did not work\n");
            return -1;
        }
    }
    
    close(file);

    return 0;
}

int redirect_in(char* filename[], int count, bool redirect){
    int file;
    for (int i = 0 ; i < count ; i++){
        file = open(filename[i], O_RDONLY, 0777);
        if (file == -1){
            perror("Open did not work\n");
            return -1;
        }
        if (i != count - 1) close(file);
    }
    
    if (redirect){
        if (dup2(file, STDIN_FILENO) == -1){
            perror("Dup2 did not work\n");
            return -1;
        }
    }
    close(file);

    return 0;
}
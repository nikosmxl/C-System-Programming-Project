#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "general.h"

int search_history(char history[HISTORY_MAX][MAX_STRING], int counter){
    if (counter == 0){
        fprintf(stderr, "You have not typed any commands yet\n");
        return -1;
    } 
    printf("This is your last %d commands:\n", counter);
    for (int i = 0 ; i < counter ; i++){
        printf("%d: %s\n", i + 1, history[i]);
    }
    printf("Type a number to execute the corresponding command or \"exit\" to exit history:\n");
    
    char number[MAX_STRING];
    bool isnumber = true;
    fgets(number, MAX_STRING, stdin);
    if (strcmp(number, "exit\n") == 0) return -1;
    else{
        for (int i = 0 ; i < strlen(number) - 1 ; i++){
            if(!isdigit(number[i])){
                isnumber = false;
                break;
            }
        }
        
        if (isnumber){
            int j = atoi(number);
            if (j > counter || j <= 0){
                fprintf(stderr, "The number should be between 1 and %d:\n", counter);
                return -1;
            }
            return j - 1;
        }
        else{
            fprintf(stderr, "Not a number neither \"exit\"\n");
            return -1;
        }
    }
}
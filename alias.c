#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "general.h"

int create_alias(alias** myaliases, int counter){
    char* name = strtok(NULL, " ");
    if (name == NULL){
        fprintf(stderr, "Unexpected 'newline' after \"createalias\"\n");
        return -1;
    }

    char* command = strtok(NULL, "\"");
    if (command == NULL){
        fprintf(stderr, "Unexpected 'newline' after \"createalias %s\"\n", name);
        return -1;
    }

    bool found = false;
    for (int i = 0 ; i < counter ; i++){
        if (strcmp(name, myaliases[i]->aliasname) == 0){
            fprintf(stderr, "Alias already exists\n");
            found = true;
            return -1;
        }
    }
    
    alias* new_alias = malloc(sizeof(alias));
    strcpy(new_alias->aliasname, name);
    strcpy(new_alias->command, command);

    myaliases[counter] = new_alias;
    return 0;
}

int destroy_alias(alias** myaliases, int counter){
    char* name = strtok(NULL, " ");
    if (name == NULL){
        fprintf(stderr, "Unexpected 'newline' after \"destroyalias\"\n");
        return -1;
    }
    int found = -1;
    for (int i = 0 ; i < counter ; i++){
        if (strcmp(name, myaliases[i]->aliasname) == 0){
            free(myaliases[i]);
            counter--;
            found = i;
        }
        if (found != -1 && found != counter){ // found != myalias_counter because of the myalias_counter-- in the previous if
            myaliases[i] = myaliases[i + 1];
        }
    }

    if (found == -1){
        fprintf(stderr, "No alias found with that name\n");
        return -1;
    }
    return 0;
}
#define _XOPEN_SOURCE 700   // otherwise struct sigaction gives error
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <ctype.h>
#include <signal.h>
#include "redirection.h"
#include "myglob.h"
#include "general.h"
#include "history.h"
#include "alias.h"

void CtrlZ_handler(int sigtype){
    printf("Received SIGSTP (Ctrl-Z) signal\n");
}

void CtrlC_handler(int sigtype){
    printf("Received SIGINT (Ctrl-C) signal\n");
}

void mc_init(minicommand* mc){
    mc->pipe_input = false;
    mc->pipe_output = false;
    mc->red_in = false; // Redirection input
    mc->red_out_trunc = false;  // Redirection output TRUNC
    mc->red_out_append = false; // Redirection output APPEND
    mc->bg = false; // Background
    mc->arg_count = 0;  // Argument counter
    mc->red_in_args_count = 0;
    mc->red_out_trunc_args_count = 0;
    mc->red_out_append_args_count = 0;
    mc->wc_counter = 0; // Wildcards error counter
}

int exec(char* command, char* arguments[], int arg_count){
    arguments[arg_count] = NULL;
    execvp(command, arguments);
    perror("Command not found\n");
    return -1;
}

int main(){
    // Shell signal handling
    struct sigaction ctrl_c;
    ctrl_c.sa_handler = CtrlC_handler;
    sigemptyset(&ctrl_c.sa_mask);
    ctrl_c.sa_flags = SA_RESTART;

    sigaction(SIGINT, &ctrl_c, NULL);

    struct sigaction ctrl_z;
    ctrl_z.sa_handler = CtrlZ_handler;
    sigemptyset(&ctrl_z.sa_mask);
    ctrl_z.sa_flags = SA_RESTART;

    sigaction(SIGTSTP, &ctrl_z, NULL);

    //Initializations
    int pipefds[FDS_NUMBER][2];
    char input[MAX_STRING];

    minicommand* mc[MC_MAX];
    int mc_counter = 0;

    char myhistory[HISTORY_MAX][MAX_STRING];
    int history_counter = 0;

    alias* myaliases[ALIASES_MAX];
    int myalias_counter = 0;

    do{
        // Piping
        for (int i = 0 ; i < FDS_NUMBER ; i++){
            if (pipe(pipefds[i]) == -1){
                perror("Piping went wrong\n");
                return -1;
            }
        }
        int fd_counter = 0;

        // Command line
        printf("in-mysh-now:>");
        fgets(input, MAX_STRING, stdin);

        if (strcmp(input, "\n") == 0) continue;
        
        int l = strlen(input);
        if (l > 0 && input[l-1] == '\n') input[l-1] = '\0';
        if (strcmp(input, "quit") == 0) break;

        // Check if input is an alias
        for(int i = 0 ; i < myalias_counter ; i++){
            if (strcmp(input, myaliases[i]->aliasname) == 0){
                strcpy(input, myaliases[i]->command);
            }
        }

        // Check if the input is myHistory
        if (strcmp(input, "myHistory") == 0){
            int i = search_history(myhistory, history_counter);
            if (i == -1){
                continue;
            }
            else{
                strcpy(input, myhistory[i]);
            }
        }
        
        // Add command to history
        if (history_counter == HISTORY_MAX){
            for (int i = 0 ; i < history_counter - 1 ; i++){
                strcpy(myhistory[i], myhistory[i + 1]);
            }
            strcpy(myhistory[history_counter - 1], input);
        }
        else{
            strcpy(myhistory[history_counter++], input);
        }

        bool error = false;
        char* parsed = strtok(input, " ");
        while(parsed != NULL){
            // Check for alias commands
            if (strcmp(parsed, "createalias") == 0){
                if (create_alias(myaliases, myalias_counter) == 0){
                    myalias_counter++;
                }
                break;
            }
            else if(strcmp(parsed, "destroyalias") == 0){
                if (destroy_alias(myaliases, myalias_counter) == 0){
                    myalias_counter--;
                }
                break;
            }

            // New minicommand
            minicommand* curr_mc = malloc(sizeof(minicommand));
            mc_init(curr_mc);
            
            curr_mc->command = parsed;

            if (mc_counter != 0){
                if (mc[mc_counter - 1]->pipe_output == true) curr_mc->pipe_input = true;
            }

            while(parsed != NULL){
                
                // Redirection IN
                if (strcmp(parsed, "<") == 0){
                    char* filename = strtok(NULL, " ");
                    // Making sure there is a file
                    if (filename == NULL){
                        fprintf(stderr, "Unexpected 'newline' character\n");
                        error = true;
                        break;
                    }

                    if (strcmp(filename, ";") == 0){
                        fprintf(stderr, "Unexpected ';' character\n");
                        error = true;
                        break;
                    }
                    
                    bool red_end = false;
                    while (strcmp(filename, "<") != 0 && strcmp(filename, ">") != 0 &&
                    strcmp(filename, ">>") != 0 && strcmp(filename, "|") != 0 &&
                    strcmp(filename, ";") != 0){
                        // Check for ';'
                        unsigned int len = strlen(filename);
                        if (filename[len - 1] == ';'){
                            red_end = true;
                            filename[len - 1] = '\0';
                        }
                        //Check if wildcard
                        bool wildcard = false;
                        for (int i = 0 ; i < len ; i++){
                            if (filename[i] == '*' || filename[i] == '?'){
                                wildcard = true;
                                break;
                            }
                        }

                        if (wildcard){
                            int result = myglob(filename, curr_mc->red_in_args, curr_mc->red_in_args_count, true);
                            
                            if (result < 0){
                                wc_info* info = malloc(sizeof(wc_info));
                                info->result = result;
                                info->wildcard = filename;
                                curr_mc->wc[curr_mc->wc_counter++] = info;
                            }
                            else
                                curr_mc->red_in_args_count += result;
                        }
                        else
                            curr_mc->red_in_args[curr_mc->red_in_args_count++] = filename;
                        
                        if (red_end) break;

                        filename = strtok(NULL, " ");
                        if (filename == NULL) break;
                    }
                    
                    curr_mc->red_in = true;
                    parsed = filename;

                    if (red_end) break;
                    if (filename != NULL)
                        if (strcmp(filename, ";") == 0) break;
                    
                    continue;
                }
                else if (strcmp(parsed, ">") == 0){ //Redirection OUT TRUNC
                    char* filename = strtok(NULL, " ");
                    //Making sure there is a file
                    if (filename == NULL){
                        fprintf(stderr, "Unexpected 'newline' character\n");
                        error = true;
                        break;
                    }

                    if (strcmp(filename, ";") == 0){
                        fprintf(stderr, "Unexpected ';' character\n");
                        error = true;
                        break;
                    }
                    
                    bool red_end = false;
                    while (strcmp(filename, "<") != 0 && strcmp(filename, ">") != 0 &&
                    strcmp(filename, ">>") != 0 && strcmp(filename, "|") != 0 &&
                    strcmp(filename, ";") != 0){
                        // Check for ';'
                        unsigned int len = strlen(filename);
                        if (filename[len - 1] == ';'){
                            red_end = true;
                            filename[len - 1] = '\0';
                        }
                        // Check if wildcard
                        bool wildcard = false;
                        for (int i = 0 ; i < len ; i++){
                            if (filename[i] == '*' || filename[i] == '?'){
                                wildcard = true;
                                break;
                            }
                        }

                        if (wildcard){
                            int result = myglob(filename, curr_mc->red_out_trunc_args, curr_mc->red_out_trunc_args_count, true);
                            
                            if (result < 0){
                                wc_info* info = malloc(sizeof(wc_info));
                                info->result = result;
                                info->wildcard = filename;
                                curr_mc->wc[curr_mc->wc_counter++] = info;
                            }
                            else
                                curr_mc->red_out_trunc_args_count += result;
                        }
                        else
                            curr_mc->red_out_trunc_args[curr_mc->red_out_trunc_args_count++] = filename;
                        
                        if (red_end) break;

                        filename = strtok(NULL, " ");
                        if (filename == NULL) break;
                    }
                    
                    parsed = filename;
                    curr_mc->red_out_trunc = true;

                    if (red_end) break;

                    if (filename == NULL) break;
                    if (strcmp(filename, ";") == 0) break;

                    if (strcmp(filename, ">>") == 0){
                        curr_mc->red_out_trunc = false;
                        continue;
                    }

                    if (strcmp(filename, "|") == 0) continue;
                    break;
                }
                else if (strcmp(parsed, ">>") == 0){ // Redirection OUT APPEND
                    char* filename = strtok(NULL, " ");
                    // Making sure there is a file
                    if (filename == NULL){
                        fprintf(stderr, "Unexpected 'newline' character\n");
                        error = true;
                        break;
                    }

                    if (strcmp(filename, ";") == 0){
                        fprintf(stderr, "Unexpected ';' character\n");
                        error = true;
                        break;
                    }

                    bool red_end = false;
                    while (strcmp(filename, "<") != 0 && strcmp(filename, ">") != 0 &&
                    strcmp(filename, ">>") != 0 && strcmp(filename, "|") != 0 &&
                    strcmp(filename, ";") != 0){
                        // Check for ';'
                        unsigned int len = strlen(filename);
                        if (filename[len - 1] == ';'){
                            red_end = true;
                            filename[len - 1] = '\0';
                        }
                        // Check if wildcard
                        bool wildcard = false;
                        for (int i = 0 ; i < len ; i++){
                            if (filename[i] == '*' || filename[i] == '?'){
                                wildcard = true;
                                break;
                            }
                        }

                        if (wildcard){
                            int result = myglob(filename, curr_mc->red_out_append_args, curr_mc->red_out_append_args_count, true);
                            
                            if (result < 0){
                                wc_info* info = malloc(sizeof(wc_info));
                                info->result = result;
                                info->wildcard = filename;
                                curr_mc->wc[curr_mc->wc_counter++] = info;
                            }
                            else
                                curr_mc->red_out_append_args_count += result;
                        }
                        else
                            curr_mc->red_out_append_args[curr_mc->red_out_append_args_count++] = filename;
                        
                        if (red_end) break;
                        filename = strtok(NULL, " ");
                        if (filename == NULL) break;
                    }

                    parsed = filename;
                    curr_mc->red_out_append = true;

                    if (red_end) break;

                    if (filename == NULL) break;
                    if (strcmp(filename, ";") == 0) break;

                    if (strcmp(filename, ">") == 0){
                        curr_mc->red_out_append = false;
                        continue;
                    }

                    if (strcmp(filename, "|") == 0) continue;
                    break;
                }
                else if (strcmp(parsed, "|") == 0){ // Pipes
                    curr_mc->pipe_output = true;

                    break;
                }
                // Command end
                else if (strcmp(parsed, ";") == 0) break;
                // Background process
                else if (strcmp(parsed, "&") == 0 || strcmp(parsed, "&;") == 0){
                    curr_mc->bg = true;
                    break;
                }
                // Arguments and Commands
                else{
                    unsigned int len = strlen(parsed);
                    bool command_end = false;
                    // Check for command end or background process
                    if (parsed[len - 1] == ';'){
                        command_end = true;
                        parsed[len - 1] = '\0';
                        if (parsed[len - 2] == '&'){
                            curr_mc->bg = true;
                            parsed[len - 2] = '\0';
                        }
                    }

                    if (parsed[len - 1] == '&'){
                        curr_mc->bg = true;
                        parsed[len - 1] = '\0';
                        command_end = true;
                    }
                    // Check if wildcard
                    bool wildcard = false;
                    if (curr_mc->command != parsed){
                        
                        for (int i = 0 ; i < len ; i++){
                            if (parsed[i] == '*' || parsed[i] == '?'){
                                wildcard = true;
                                break;
                            }
                        }

                        if (wildcard){
                            int result = myglob(parsed, curr_mc->arguments, curr_mc->arg_count, false);
                            
                            if (result < 0){
                                wc_info* info = malloc(sizeof(wc_info));
                                info->result = result;
                                info->wildcard = parsed;
                                curr_mc->wc[curr_mc->wc_counter++] = info;
                            }
                            else
                                curr_mc->arg_count += result;
                        }
                    }
                    
                    if (!wildcard)
                        curr_mc->arguments[curr_mc->arg_count++] = parsed;

                    if (command_end) break;
                }
                parsed = strtok(NULL, " ");
            }
            if (error) break;
            mc[mc_counter++] = curr_mc;
            parsed = strtok(NULL, " ");
        }
        // Error handling and command execution
        if (!error){
            int pid[mc_counter];
            for (int i = 0 ; i < mc_counter ; i++){
                pid[i] = fork();
                // Children
                if (pid[i] == 0){
                    if (mc[i]->bg == false){ // Foreground signal handling
                        signal(SIGINT, SIG_DFL);
                        signal(SIGTSTP, SIG_DFL); 
                    }
                    else{   // Background signal handling
                        signal(SIGINT, SIG_IGN);
                        signal(SIGTSTP, SIG_IGN); 
                    }
                    //Check if there are arguments
                    bool has_args = false;
                    if (mc[i]->arg_count > 1){
                        for (int j = 1 ; j < mc[i]->arg_count ; j++){   // From 1 bacause we are not checking the command as an argument
                            if (mc[i]->arguments[j][0] != '-'){
                                has_args = true;
                                break;
                            } 

                        }
                    }
                    // Check if there was a wildcard error
                    if (mc[i]->wc_counter > 0){
                        bool execute = true;
                        for (int j = 0 ; j < mc[i]->wc_counter ; j++){
                            if (mc[i]->wc[j]->result == -1)
                                fprintf(stderr, "%s: %s: No such file or directory\n", mc[i]->command, mc[i]->wc[j]->wildcard);
                            else if (mc[i]->wc[j]->result == -2){
                                fprintf(stderr, "%s: %s: Something went wrong with glob\n", mc[i]->command, mc[i]->wc[j]->wildcard);
                                execute = false;
                            }
                            else{
                                fprintf(stderr, "My simple shell: %s: ambiguous redirection\n", mc[i]->wc[j]->wildcard);
                                execute = false;
                            }
                        }
                        // No execution if redirections, no arguments, or if error was not "No such file or directory"
                        if (!execute || (execute && !has_args) || mc[i]->red_in || mc[i]->red_out_trunc || mc[i]->red_out_append){
                            for (int i = 0 ; i < FDS_NUMBER ; i++){
                                close(pipefds[i][READ]);
                                close(pipefds[i][WRITE]);
                            }
                            return 0;
                        }              
                    }
                    
                    // Redirecting if asked
                    if (mc[i]->red_in) redirect_in(mc[i]->red_in_args, mc[i]->red_in_args_count, !has_args);

                    redirect_out_trunc(mc[i]->red_out_trunc_args, mc[i]->red_out_trunc_args_count, mc[i]->red_out_trunc);
                    
                    redirect_out_append(mc[i]->red_out_append_args, mc[i]->red_out_append_args_count, mc[i]->red_out_append);

                    // Piping if asked
                    if (mc[i]->pipe_input && !(mc[i]->red_in) && !has_args){
                        int reading_from;
                        if (fd_counter == 0) reading_from = FDS_NUMBER - 1;
                        else reading_from = fd_counter - 1;

                        if (dup2(pipefds[reading_from][READ], STDIN_FILENO) == -1){
                            perror("Dup2 did not work1\n");
                            return -1;
                        }
                    }

                    if (mc[i]->pipe_output && !(mc[i]->red_out_append) && !(mc[i]->red_out_trunc)){

                        if (dup2(pipefds[fd_counter][WRITE], STDOUT_FILENO) == -1){
                            perror("Dup2 did not work2\n");
                            return -1;
                        }
                    }

                    // Closing all file discriptors
                    for (int i = 0 ; i < FDS_NUMBER ; i++){
                        close(pipefds[i][READ]);
                        close(pipefds[i][WRITE]);
                    }
                    
                    exec(mc[i]->command, mc[i]->arguments, mc[i]->arg_count);
                }

                if (mc[i]->pipe_output){
                    fd_counter = (fd_counter + 1) % FDS_NUMBER;
                }
            }
            // Closing all file discriptors
            for (int i = 0 ; i < FDS_NUMBER ; i++){
                close(pipefds[i][READ]);
                close(pipefds[i][WRITE]);
            }
            // Waits
            int bg_process_index = 0;
            for (int i = 0 ; i < mc_counter ; i++){
                int status;
                if (mc[i]->bg == false){
                    waitpid(pid[i], &status, 0);
                }
                else{
                    printf("[%d] %d\n", bg_process_index++, pid[i]);
                    waitpid(pid[i], &status, WNOHANG);
                }
            }
        }
        // Frees
        for (int i = 0 ; i < mc_counter ; i++){
            for (int j = 0 ; j < mc[i]->wc_counter ; j++){
                free(mc[i]->wc[j]);
            }
            mc[i]->wc_counter = 0;
        }

        for (int i = 0 ; i < mc_counter ; i++){
            free(mc[i]);
        }
        mc_counter = 0;
    }while(1);
    
    return 0;
}
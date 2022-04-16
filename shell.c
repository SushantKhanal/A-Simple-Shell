#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024

void get_user_input() {
    static int entry_flag = 0;
    if(!entry_flag) {
        const char* ANSI_CODE = "\e[H\e[2J\e[3J";
        write(STDOUT_FILENO, ANSI_CODE, 12);
        entry_flag = 1;
    }
    printf("hello@hacker >");
}

void read_input (char command[], char *parameters[]) {
    get_user_input();
    char line[BUFSIZE];
    int count = 0;
    int i = 0;
    int j = 0;
    char *token, *array[100];
    while(1) {
        int c = fgetc( stdin );
        line[count++] = (char) c;
        if(c == '\n') break;
    }
    if(count == 1) return;
    token = strtok(line, " \n"); //to break the line into tokens
    while(token != NULL) {
        array[i++] = strdup (token);
        token = strtok(NULL, "\n");
    }
    strcpy(command, array[0]); //first word is command
    for(j = 0; j < i; j++) {
        parameters[j] = array[j];
    }
    parameters[i] = NULL;
}

void repl_loop() {
    char cmd[100];
    char command[100];
    char *parameters[20]; 
    char *environment_var[] = { (char *) "PATH=/bin", 0 }; 
    int status;
    while(1) {
        read_input(command, parameters);
        if( strcmp (command, "exit") == 0) {
            return;
        }
        if(fork() == 0) {
            strcpy(cmd, "/bin/");
            strcat(cmd, command);
            status = execve(cmd, parameters, environment_var);
            if(status == -1) {
                printf("Invalid Command: %s\n", command);
            }
        } else {
            wait(NULL);
        }
    }
}

int main() {
    repl_loop();
    return EXIT_SUCCESS;
}

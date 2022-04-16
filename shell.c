#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

void type_prompt() {
    static int first_time = 1;
    if(first_time) {
        // const char* xxx = " \e[1;1H\e[2J";
        const char* xxx = "\e[H\e[2J\e[3J";
        write(STDOUT_FILENO, xxx, 12);
        first_time = 0;
    }
    printf(">");
}

void read_command (char command[], char *parameters[]) {
    char line[1024];
    int count = 0, i = 0, j = 0;
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
    for(int j = 0; j < i; j++) {
        parameters[j] = array[j];
    }
    parameters[i] = NULL;
}

int main() {
    char cmd[100], command[100];
    char *parameters[20]; //array of pointers to hold the paramters
    //we assume that all commands are in the directory /bin
    char *envp[] = { (char *) "PATH=/bin", 0 }; //environment varriable
    for( ; ; ) {
        type_prompt();
        read_command(command, parameters);
        if(fork() == 0) {
            strcpy(cmd, "/bin/");
            strcat(cmd, command);
            // printf("cmd %s", cmd);
            execve(cmd, parameters, envp);
        } else {
            wait(NULL);
        }
        if( strcmp (command, "exit") == 0) {
            break;
        }
    }
    return 0;
}

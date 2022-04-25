#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE1 200 

char *builtin_strings[] = {"help", "cd", "exit" }; //user input commands for builtin functions

int builtin_cd(char **parameters) {
  if (parameters[1] == NULL) {
    printf("\n argument is expected for cd \n");
  } else {
    if (chdir(parameters[1]) != 0) {
       printf("\n Error navigating to %s \n", parameters[1]);
    }
  }
  return 1;
}

int builtin_help(char **parameters) {
  //takes argument because it is called dynamically with cd and exit
  int i;
  printf("\nType commands and press enter.\n");
  printf("\nList of built in commands:\n");
  int lengthOfBuiltIns = sizeof(builtin_strings) / sizeof(char *);
  for (i = 0; i < lengthOfBuiltIns; i++) {
    printf("  %s\n", builtin_strings[i]);
  }
  return 1;
}

int builtin_exit(char **parameters) {
  //takes argument because it is called dynamically with cd and help
  return 0;
}

int (*builtin_functions[]) (char **) = { &builtin_help, &builtin_cd, &builtin_exit }; //array of builtin function addresses

char **process_input(char* input_line) {
    int bufsize2 = 64, index = 0;
    char *token;
    char **tokens = malloc(bufsize2 * sizeof(char*));

    if (!tokens) {
        printf("Allocation error\n");
        exit(EXIT_FAILURE);
    }
    //using strtok to split user input
    token = strtok(input_line, " \t\r\n\a");
    while (token != NULL) {
        tokens[index] = token;
        index++;
        if (index >= bufsize2) {
            //adding buffer size if user input is longer than anticipated
            bufsize2 += 64;
            tokens = realloc(tokens, bufsize2 * sizeof(char*)); //reallocating memory to tokens
            if (!tokens) {
                printf("Allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, " \t\r\n\a");
    }
    tokens[index] = NULL;
    return tokens;
}

void get_user_input (char*** inputArguments) {
    char currentDir[BUFSIZE1];
    getcwd(currentDir, BUFSIZE1);
    printf("%s@helloHacker > ", currentDir); //prints current path
    char *input_line = NULL;
    ssize_t bufsize = 0; 
    if (getline(&input_line, &bufsize, stdin) == -1){
        if (feof(stdin)) { //tests the end-of-file indicator for the input stream
            exit(EXIT_SUCCESS);  
        } else  {
            printf("Error reading input command.");
            exit(EXIT_FAILURE);
        }
    }
    char* piped_input[2];
    int i;
	for (i = 0; i < 2; i++) {
        //checking for pipes
		piped_input[i] = strsep(&input_line, "|");
		if (piped_input[i] == NULL)
			break;
	}
    inputArguments[0] = process_input(piped_input[0]); 
    if (piped_input[1] == NULL) {
        //pipe is absent
        inputArguments[1] = NULL;
    } else {
        //pipe is present
        inputArguments[1] = process_input(piped_input[1]);
    }
}

int executeArguments(char **arguments) {
    // char *environment_var[] = { (char *) "PATH=/bin", 0 }; //would be necessary if execve is used
    if (arguments[0] == NULL) {
        return 1;
    }
    int lengthOfBuiltIns = sizeof(builtin_strings) / sizeof(char *);
    int i;
    for (i = 0; i < lengthOfBuiltIns; i++) {
        //help, cd & exit
        if (strcmp(arguments[0], builtin_strings[i]) == 0) {
            return (*builtin_functions[i])(arguments); //calls builtin functions dynamically
        }
    }
    if (fork() == 0) {        
        //child is created to execute the command
        if (execvp(arguments[0], arguments) == -1) {
            //execve(arguments[0], arguments, environment_var);
          printf("Invalid Command: %s\n", arguments[0]);
          exit(EXIT_FAILURE);
        }
    } else {
        wait(NULL);
    }
    return 1;
}

int executePipedArguments(char** arguments, char** pipedArguments) {
	// 0 is read end, 1 is write end
    const int PIPE_READ = 0;
    const int PIPE_WRITE = 1;   
	int fd[2]; //pipe
	pid_t p1, p2; //child processes 1 & 2

	if (pipe(fd) < 0) {
		printf("\nError initializing pipe.\n");
		exit(EXIT_FAILURE);
	}

	p1 = fork();
	if (p1 < 0) {
		printf("\nError forking.\n");
		exit(EXIT_FAILURE);
	}

	if (p1 == 0) { 
        //inside first child process
		dup2(fd[PIPE_WRITE], STDOUT_FILENO); //Duplicate "write" end of pipe
		close(fd[PIPE_READ]); //close "read" end of pipe
        close(fd[PIPE_WRITE]); //close "write" end of pipe
		if (execvp(arguments[0], arguments) < 0) {
			printf("\nInvalid Command: %s\n", arguments[0]);
			exit(EXIT_FAILURE);
		}
	} else { 
        //parent
		p2 = fork();
		if (p2 < 0) {
            //inside second child process
			printf("\nError forking.\n");
			exit(EXIT_FAILURE);
		}
		if (p2 == 0) { //child 2
        	dup2(fd[PIPE_READ], STDIN_FILENO); //Duplicate "read" end of pipe
			close(fd[PIPE_WRITE]); //close "write" end of pipe
			close(fd[PIPE_READ]); //close "read" end of pipe
			if (execvp(pipedArguments[0], pipedArguments) < 0) {
				printf("\nInvalid Command: %s\n", pipedArguments[0]);
				exit(EXIT_FAILURE);
			}
		} else {
			// parent executing, waiting for two children
            close(fd[PIPE_WRITE]); //close "write" end of pipe
			close(fd[PIPE_READ]); //close "read" end of pipe
			while(wait(NULL) > 0); //wait for all child processes
		}
	}
    return 1;
}

int main() {
    int isValid;
    do {
        char** inputArguments[2];
        get_user_input(inputArguments);
        if(inputArguments[1] == NULL) {
            //no pipes
            isValid = executeArguments(inputArguments[0]);
        } else {
            //pipes present
            isValid = executePipedArguments(inputArguments[0], inputArguments[1]);
        }
        
    } while (isValid);
    return EXIT_SUCCESS;
}

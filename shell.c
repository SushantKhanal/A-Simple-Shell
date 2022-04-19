#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024
#define MAX_BUF 200

char *builtin_strings[] = {"help", "cd", "exit" };

int builtin_cd(char **args) {
  if (args[1] == NULL) {
    printf("\n expected argument to cd \n");
  } else {
    if (chdir(args[1]) != 0) {
       printf("Error navigating to %s", args[1]);
    }
  }
  return 1;
}

int builtin_help(char **args) {
  int i;
  printf("Type commands and enter.\n");
  printf("\nBuilt in commands:\n");
  int lengthOfBuiltIns = sizeof(builtin_strings) / sizeof(char *);
  for (i = 0; i < lengthOfBuiltIns; i++) {
    printf("  %s\n", builtin_strings[i]);
  }
  return 1;
}

int builtin_exit(char **args) {
  return 0;
}

int (*builtin_functions[]) (char **) = { &builtin_help, &builtin_cd, &builtin_exit };

char **process_input(char* input_line) {
    int bufsize2 = 64, index = 0;
    char **tokens = malloc(bufsize2 * sizeof(char*));
    char *token;

    if (!tokens) {
        printf("Allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(input_line, " \t\r\n\a");
    while (token != NULL) {
        tokens[index] = token;
        index++;
        if (index >= bufsize2) {
            bufsize2 += 64;
            tokens = realloc(tokens, bufsize2 * sizeof(char*));
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
    char currentDir[MAX_BUF];
    getcwd(currentDir, MAX_BUF);
    printf("%s@helloHacker > ", currentDir);
    char *input_line = NULL;
    ssize_t bufsize = 0; 
    if (getline(&input_line, &bufsize, stdin) == -1){
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);  
        } else  {
            perror("readline");
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
        //no pipe
        inputArguments[1] = NULL;
    } else {
        //yes pipe
        inputArguments[1] = process_input(piped_input[1]);
    }
}

int executeArguments(char **arguments) {
    // char *environment_var[] = { (char *) "PATH=/bin", 0 }; 
    if (arguments[0] == NULL) {
        return 1;
    }
    int lengthOfBuiltIns = sizeof(builtin_strings) / sizeof(char *);
    int i;
    for (i = 0; i < lengthOfBuiltIns; i++) {
        if (strcmp(arguments[0], builtin_strings[i]) == 0) {
            return (*builtin_functions[i])(arguments);
        }
    }
    if (fork() == 0) {        
        if (execvp(arguments[0], arguments) == -1) {
            //execve(arguments[0], arguments, environment_var);
          printf("Invalid Command: %s\n", arguments[0]);
        }
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
    return 1;
}

int executePipedArguments(char** arguments, char** pipedArguments) {
	// 0 is read end, 1 is write end
    const int PIPE_READ = 0;
    const int PIPE_WRITE = 1;   
	int pipefd[2];
	pid_t p1, p2;

	if (pipe(pipefd) < 0) {
		printf("\nError initializing pipe.\n");
		exit(EXIT_FAILURE);;
	}

	p1 = fork();
	if (p1 < 0) {
		printf("\nError forking.\n");
		exit(EXIT_FAILURE);;
	}

	if (p1 == 0) { //child 1
        //close both end of pipes
		dup2(pipefd[PIPE_WRITE], STDOUT_FILENO); //Duplicate "write" end of pipe
		close(pipefd[PIPE_READ]); 
        close(pipefd[PIPE_WRITE]); //close "write" end of pipe
		if (execvp(arguments[0], arguments) < 0) {
			printf("Invalid Command: %s\n", arguments[0]);
			exit(EXIT_FAILURE);
		}
	} else { //parent
		p2 = fork();
		if (p2 < 0) {
			printf("\nError forking.\n");
			exit(EXIT_FAILURE);
		}
		if (p2 == 0) { //child 2
        	dup2(pipefd[PIPE_READ], STDIN_FILENO); //Duplicate "read" end of pipe
			close(pipefd[PIPE_WRITE]); //close "write" end of pipe
			close(pipefd[PIPE_READ]); //close "read" end of pipe
			if (execvp(pipedArguments[0], pipedArguments) < 0) {
				printf("Invalid Command: %s\n", pipedArguments[0]);
				exit(EXIT_FAILURE);
			}
		} else {
			// parent executing, waiting for two children
            close(pipefd[PIPE_WRITE]); //close "write" end of pipe
			close(pipefd[PIPE_READ]); //close "read" end of pipe
			wait(NULL);
			wait(NULL);
		}
	}
    return 1;
}

int main() {
    int status;
    do {
        char** inputArguments[2];
        get_user_input(inputArguments);
        if(inputArguments[1] == NULL) {
            //no pipes
            status = executeArguments(inputArguments[0]);
        } else {
            //pipes present
            status = executePipedArguments(inputArguments[0], inputArguments[1]);
        }
        
    } while (status);
    return EXIT_SUCCESS;
}

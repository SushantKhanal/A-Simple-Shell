#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024

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
  printf("Built in commands:\n");
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

char **get_user_input () {
    printf("hello@hacker >");
    char *line = NULL;
    ssize_t bufsize = 0; // have getline allocate a buffer for us

    if (getline(&line, &bufsize, stdin) == -1){
        if (feof(stdin)) {
        exit(EXIT_SUCCESS);  // We recieved an EOF
        } else  {
        perror("readline");
        exit(EXIT_FAILURE);
        }
    }

    //splitline
    int bufsize2 = 64, index = 0;
    char **tokens = malloc(bufsize2 * sizeof(char*));
    char *token;

    if (!tokens) {
        printf("Allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " \t\r\n\a");
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

int executeArguments(char **arguments) {
    if (arguments[0] == NULL) {
        // An empty command was entered.
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
        perror("Error");
        }
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
    return 1;
}

void repl_loop() {
    int status;
    do {
        char **arguments;
        arguments = get_user_input();
        // char *environment_var[] = { (char *) "PATH=/bin", 0 }; 
        status = executeArguments(arguments);
        free(arguments);
    } while (status);
}

int main() {
    repl_loop();
    return EXIT_SUCCESS;
}

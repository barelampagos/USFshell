#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_STR_LEN 128
#define MAX_ARGS 64

/*-----------------------------------------------------------------
 * Function:  print_prompt
 * Purpose:  Prints the '$' symbol to stdout and reads stdin into the input array.
 */
void print_prompt(char input[]) {
    write(1, "$ ", 3);
    read(0, input, MAX_STR_LEN);
}

/*-----------------------------------------------------------------
 * Function:  parse_args
 * Purpose:  Parses the input line, splits the input by spaces and new line
 *            characters, and places the arguments into the argv array.
 */
int parse_args(char input[], char* argv[]) {
    char* arg;
    int argc = 0;

    // Split command into arguments
    arg = strtok(input, " \n");
    while (arg != NULL) {
        argv[argc] = arg;
        arg = strtok(NULL, " \n");
        argc++;
    }
    argv[argc] = NULL;

    return argc;
}

/*-----------------------------------------------------------------
 * Function:  execute_pipe
 * Purpose:  Takes in argv (all the parameters before '|') and argv1 (all the parameters
 *            after '|'). The parent process creates a pipe and two children processes,
 *            changes the respective file decriptors, and executes both programs.
 */
void execute_pipe(char* argv[], char* argv1[]) {
    pid_t id;
    int filedes[2];

    pipe(filedes);

    id = fork();

    if (id == 0) {
        // Close stdout, copy pipe write, close pipe read
        close(1);
        dup(filedes[1]);
        close(filedes[0]);

        // Execute program 1
        if (execvp(argv[0], argv) < 0) {
            printf("Invalid command '%s'.\n", argv[0]);
            exit(1);
        }
    } else {
        wait(NULL);
        id = fork();

        if (id == 0) {

            // Close stdin, copy pipe read, close pipe write
            close(0);
            dup(filedes[0]);
            close(filedes[1]);

            // Execute program 2
            if (execvp(argv1[0], argv1) < 0) {
                printf("Invalid command '%s'.\n", argv1[0]);
                exit(1);
            }
        } else {
            // Close pipe
            close(filedes[0]);
            close(filedes[1]);
            wait(NULL);
        }
    }
}

/*-----------------------------------------------------------------
 * Function:  execute_redirect
 * Purpose:  Takes in an array of arguments and a specified file descriptor fd. The
 *            stdout file descriptor is updated to fd, and executed.
 */
void execute_redirect(char* argv[], int fd) {
    pid_t id;

    id = fork();

    // Child Process
    if (id == 0) {
          // Changing file descriptors for redirection
          close(1);
          dup(fd);
          close(fd);

          if (execvp(argv[0], argv) < 0) {
              printf("Invalid command '%s'.\n", argv[0]);
              exit(1);
          }
    }
    // Parent Process
    else {
          close(fd);
          id = wait(NULL);
    }
}

/*-----------------------------------------------------------------
 * Function:  execute_program
 * Purpose:  Takes in an array of arguments, forks a child process, and executes.
 */
void execute_program(char* argv[]) {
    pid_t id;

    id = fork();

    // Child Process
    if (id == 0) {
          if (execvp(argv[0], argv) < 0) {
              printf("Invalid command '%s'.\n", argv[0]);
              exit(1);
          }
    }
    // Parent Process
    else {
          id = wait(NULL);
    }
}

/*-----------------------------------------------------------------
 * Function:  execute_command
 * Purpose:  Takes in an array of arguments, argv, and the count of arguments, argc.
 *            Based on the first argument in argv, determines which command to execute.
 */
void execute_command(char* argv[], int argc) {
    int fd_out;
    // i and j used for iterating through arguments
    int i = 0;
    int j = 0;
    // argv1 holds any arguments after the "|" flag
    char* argv1[MAX_ARGS];

    if (strcmp(argv[0], "exit") == 0) {
          printf("Exiting USFshell.\n");
          exit(0);
    } else if (strcmp(argv[0], "cd") == 0) {
          chdir(argv[1]);
    } else {
          // Iterate through each argument
          for (i = 0; i < argc; i++) {
              // Checks for '>' symbol = redirection
              if (strcmp(argv[i], ">") == 0) {
                    if ((fd_out = open(argv[i + 1], O_CREAT | O_WRONLY, 0644)) < 0) {
                        printf("No output file specified.\n");
                        return;
                    }
                    argv[i] = NULL;
                    execute_redirect(argv, fd_out);
                    return;

              }
              // Checks for '|' symbol = pipelining
              else if (strcmp(argv[i], "|") == 0) {
                    // Parse args after | into second array
                    argv[i] = NULL;
                    i++;

                    while (i < argc) {
                        argv1[j] = argv[i];
                        j++;
                        i++;
                    }
                    argv1[j] = NULL;

                    execute_pipe(argv, argv1);
                    return;
              }
          }

          execute_program(argv);
    }
}

/*-----------------------------------------------------------------
 * Function:  clear_arrays
 * Purpose:  Resets the input and argv arrays so they can be reused.
 */
void clear_arrays(char input[], char* argv[]) {
    memset(input, 0, MAX_STR_LEN);
    memset(argv, 0, MAX_ARGS);
}

/* Main -----------------------------------------*/
int main(void) {
    char input[MAX_STR_LEN];
    char* argv[MAX_ARGS];
    int argc;

    // Continue to get user input until exit is called
    while (true) {
          // Clear input and argument arrays
          clear_arrays(input, argv);

          // Prompt user with $ and get input line
          print_prompt(input);

          // Process input line into arguments
          argc = parse_args(input, argv);

          // Execute the first argument's command
          execute_command(argv, argc);
    }

    return 0;
}

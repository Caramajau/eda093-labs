/*
 * Main source file for the lsh shell program.
 *
 * You are free to add functions to this file.
 * If you want to add functions in separate files,
 * you will need to modify CMakeLists.txt to compile
 * your additional files.
 *
 * Add appropriate comments to make your code
 * easier for us to grade.
 *
 * Using assert statements is a good way to catch errors early and make debugging easier.
 * Think of them as mini self-checks that ensure your program behaves as expected.
 * By setting up these guardrails, you're creating a more robust and maintainable solution.
 * So go ahead, sprinkle some asserts in your code; they're your friends in disguise!
 *
 * All the best!
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// The <unistd.h> header is your gateway to the OS's process management facilities.
#include <unistd.h>

#include "parse.h"

#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define READ_END 0
#define WRITE_END 1

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *);
static void handle_cmd(Command *cmd);
static void sigchld_handler(int sig);

int main(void)
{
  struct sigaction sa = {0};

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  
  sigaction(SIGCHLD, &sa, NULL);

  for (;;)
  {
    char *line;
    line = readline("> ");

    // Remove leading and trailing whitespace from the line
    stripwhite(line);

    // If the stripped line is not blank
    if (*line)
    {
      add_history(line);

      Command cmd;
      if (parse(line, &cmd) == 1)
      {
        // Print the parsed command
        print_cmd(&cmd);
        handle_cmd(&cmd);
      }
      else
      {
        printf("Parse ERROR\n");
      }
    }

    // Free the input buffer
    free(line);
  }

  return 0;
}

static void sigchld_handler(int sig) {
  // Can cast to void to avoid warning about sig being unused
  // https://stackoverflow.com/questions/10391031/defining-unused-parameters-in-c
  (void)sig;

  // In case handler comes in between some other thing
  // Shouldn't override the errno of that
  int saved_errno = errno;
  // Reap zombies right now and then stop
  while (waitpid(-1, NULL, WNOHANG) > 0) {}

  errno = saved_errno;
}

static void handle_cmd(Command *cmd_list) {
  int number_of_programs = 0;

  for (Pgm *program = cmd_list->pgm; program != NULL; program = program->next) {
    number_of_programs++;
  }

  pid_t pids[number_of_programs];

  int out_fd = STDOUT_FILENO;

  int number_of_children = 0;

  for (Pgm *program = cmd_list->pgm; program != NULL; program = program->next) {
    // Initial setup with pipe and fork
    int pipe_fds[2];
    int in_fd = STDIN_FILENO;

    // Configure pipe for all, but the "first" program
    if (program->next != NULL) {
      if (pipe(pipe_fds) < 0) {
        perror("pipe");
        break;
      }
      in_fd = pipe_fds[READ_END];
    }

    pid_t pid = fork();

    // Error
    if (pid < 0) {
      perror("fork");
      
      // If a pipe was created close it if fork errors
      if (program->next != NULL) {
        close(pipe_fds[READ_END]);
        close(pipe_fds[WRITE_END]);
      }

      break;

    // Child
    } else if (pid == 0) {
      if (in_fd != STDIN_FILENO) {
        dup2(in_fd, STDIN_FILENO);
      }
      if (out_fd != STDOUT_FILENO) {
        dup2(out_fd, STDOUT_FILENO);
      }

      // Close pipe fds so readers won't get stuck
      if (program->next != NULL) {
        // pipe_fd for read end redundant now as in_fd will have it except for when next is NULL (there you want the stdin)
        close(pipe_fds[READ_END]);
        // Very important as child reads from this one
        close(pipe_fds[WRITE_END]);
      }

      if (out_fd != STDOUT_FILENO) {
        // No need to keep as the write end has been redirected
        close(out_fd);
      }

      execvp(program->pgmlist[0], program->pgmlist);
      // Code after exec only runs if it fails
      perror(program->pgmlist[0]);
      // Don't fall back into the shell loop?
      _exit(1);
  
    // Parent
    } else {
      // Fork succeeded, save pid...
      pids[number_of_children] = pid;
      
      // ...and increment
      number_of_children++;

      // Child has its own copy so this one is unnecessary and should be closed so reader won't get stuck
      if (out_fd != STDOUT_FILENO) {
        close(out_fd);
      }

      if (program->next != NULL) {
        // Parent never reads
        close(pipe_fds[READ_END]);

        // Next (earlier) program will write here
        out_fd = pipe_fds[WRITE_END];
      }
    }
  }

  // Only wait for all children if it isn't background
  if (!cmd_list->background) {
    for (int i = 0; i < number_of_children; i++) {
      waitpid(pids[i], NULL, 0);
    }
  }
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inspiration.
 */
static void print_cmd(Command *cmd_list)
{
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd_list->rstdin ? cmd_list->rstdin : "<none>");
  printf("stdout:     %s\n", cmd_list->rstdout ? cmd_list->rstdout : "<none>");
  printf("background: %s\n", cmd_list->background ? "true" : "false");
  printf("Pgms:\n");
  print_pgm(cmd_list->pgm);
  printf("------------------------------\n");
}

/* Print a linked list of Pgm structures.
 *
 * Helper function, no need to change. It may be useful to study for inspiration.
 */
static void print_pgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;

    /* The list is stored in reverse order, so print
     * it in reverse to restore the original order.
     */
    print_pgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}


/* Strip whitespace from the start and end of a string.
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  size_t i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}

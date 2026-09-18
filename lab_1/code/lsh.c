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

#define READ_END 0
#define WRITE_END 1

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *);
static void handle_cmd(Command *cmd);

int main(void)
{
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

static void handle_cmd(Command *cmd_list) {
  Pgm *current_program = cmd_list->pgm;

  int fd[2];

  // Important to create pipe before fork so the processes actually share the same pipe
  if (pipe(fd[2]) == -1) {
    printf("Pipe failed");
    return;
  }

  pid_t pid = fork();
  if (current_program->next != NULL) {
    pid_t pid2 = fork();
    if (pid2 < 0) {
      printf("Error\n");

    } else if (pid2 == 0) {
      // Command and stuff is found in pgmlist
      if (execvp(*current_program->next->pgmlist, current_program->next->pgmlist) == -1) {
        printf("Error with: \n");
        printf(*current_program->next->pgmlist);
        printf("\n");
      } else {
        close(fd[READ_END]);
        // Make pipe also get stuff that stdout would get I think
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[WRITE_END]);
      }
      
    } else {
      waitpid(pid2, NULL, 0);
      printf("Complete\n");
    }
  }

  if (pid < 0) {
    printf("Error\n");

  } else if (pid == 0) {
    // Command and stuff is found in pgmlist
    if (execvp(*current_program->pgmlist, current_program->pgmlist) == -1) {
      printf("Error with: \n");
      printf(*current_program->pgmlist);
      printf("\n");
    }
    
  } else {
    waitpid(pid, NULL, 0);
    printf("Complete\n");
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

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"
#include "path_resolution.h"
#include "io_redirection.h"
#include "process.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

/* line number */
int line_num = 0;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
int cmd_wait();
/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "current working directory"},
  {cmd_cd, "cd", "change current working directory"},
  {cmd_wait, "wait", "show terminal prompt"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
  exit(0);
}

/* Prints current working directory */
int cmd_pwd(unused struct tokens *tokens) {
	char *pwd = (char *)malloc(1024);
	getcwd(pwd, 1024);
	printf("%s \n", pwd);
	free(pwd);
	return 1;
}

/* Change current working directory */
int cmd_cd(unused struct tokens *tokens) {
	if(chdir(tokens_get_token(tokens, 1)) == -1)
		fprintf(stderr, "%s\n", strerror(errno));
	return 1;
}

/* Wait for all background processes */
int cmd_wait() {
  int status;
  wait(&status);
  return 1;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Ignore signal */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGKILL, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    /* Saves the shell's process id */
    shell_pgid = getpid();
    
    /* set process group id */
    if (setpgid(shell_pgid, shell_pgid) == -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);

  }
}

int main(unused int argc, unused char *argv[]) {
  init_shell();

  static char line[4096];

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      /* REPLACE this to run commands as programs. */
      // fprintf(stdout, "This shell doesn't know how to run programs.\n");
      
      /* Check whether background process */
      int bc_grnd = put_background(tokens);

      /* Create child process */
      pid_t pid = fork();

      if(pid > 0) {
        if(!bc_grnd) {
          /* If not background process, Wait for the child process */
          int status;
          int rval_wait = waitpid(pid, &status, 0);
          if(rval_wait == -1)
            fprintf(stderr, "%s\n", strerror(errno));
          else if(rval_wait == 0)
            printf("%s\n", "no child has exited");
          }
      }
      else {
        
        /* Enable accepting signal */
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGKILL, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCONT, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        /* Redirect */
        int io_redirect_ind = redirect(tokens, tokens_get_length(tokens));
        
        /* Get the full path */
        char *path = (char *)malloc(sizeof(1024 * sizeof(char)));
        path = find_full_path(tokens_get_token(tokens, 0));
        
        /* Create process arguments */
        int argv_len;
        if(bc_grnd) /* Background process */
          argv_len = tokens_get_length(tokens) - 1;
        else if(redirect_opt == 0) /* No redirection */
          argv_len = tokens_get_length(tokens);
        else if(redirect_opt == 1) /* OP rediretion */
          argv_len = io_redirect_ind;
        else /* IP redirection */
          argv_len = tokens_get_length(tokens) - 1;

        char *proc_argv[argv_len+1];
        int i, /* index to proc_argv */
            j; /* index to tokens */

        proc_argv[0] = strdup(path);
        for(i=1, j=1; i<argv_len; i++, j++) { 
          if( redirect_opt == -1 && ( strcmp(tokens_get_token(tokens, j), "<") ) == 0){ /* IP redirection */
            i--; continue;
          } 
          proc_argv[i] = (char *)malloc(sizeof(char) * strlen(tokens_get_token(tokens, j)));
          strcpy(proc_argv[i], tokens_get_token(tokens, j));
        }
        proc_argv[argv_len] = '\0';
        
        /* Execute process */
        if(execv(proc_argv[0], proc_argv) == -1) {
          fprintf(stderr, "%s\n", strerror(errno));
          exit(EXIT_FAILURE);
        }
        
        /* Clean up */
        free(path);
        for(i=0; i<argv_len+1; i++) {
          free(proc_argv[i]);
        }
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}

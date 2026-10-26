/* minishell.c - one full lesson: a tiny shell built from the ideas of the 1974
   UNIX paper (sec 5 "Processes", sec 6.5 "Implementation of the Shell").

   Supports: simple commands, trailing '&' (background), builtins: cd, exit, status.
   Does NOT support: pipes, redirection (good next exercises).

   Read main() first: it is the whole shell in 15 lines. Then read the helper
   functions in the order they are called.

   Build with:  make minishell2     (flags: -std=gnu90 -Wall -Wpedantic)  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 512
#define MAXARGS 32
#define EXIT_CMD_NOT_RUN 127      /* conventional "could not exec" exit status */

/* try_builtin() results */
#define BUILTIN_NO   0            /* not a builtin: run it as an external program */
#define BUILTIN_DONE 1            /* builtin handled it already */
#define BUILTIN_QUIT 2            /* builtin asked the shell to stop */

static int last_status = 0;       /* exit status of the last foreground command */

/* ---- prototypes, in the order main() uses them ---- */
static void reap_background_jobs(void);
static void print_prompt(void);
static int  read_line(char *buf, int size);
static int  parse_line(char *line, char *argv[], int *background);
static int  try_builtin(char *argv[]);
static void launch_command(char *argv[], int background);
static void run_child(char *argv[]);
static void wait_for_foreground(pid_t id);
static void report_status(int status);

/* ------------------------------------------------------------------------ */

int main(void)
{
    char line[MAXLINE];
    char *argv[MAXARGS];
    int argc, background, action;

    for (;;) {
        reap_background_jobs();
        print_prompt();
        if (!read_line(line, sizeof line))
            break;                              /* end-of-file ends the shell (paper 6.5) */

        argc = parse_line(line, argv, &background);
        if (argc == 0)
            continue;                           /* empty line: just prompt again */

        action = try_builtin(argv);
        if (action == BUILTIN_QUIT)
            break;
        if (action == BUILTIN_DONE)
            continue;

        launch_command(argv, background);       /* fork + exec + (maybe) wait */
    }
    printf("\n");
    return 0;
}

/* ---- Step 1: housekeeping before each prompt -------------------------------
   A finished background child stays a "zombie" (dead, but its exit status is
   still stored) until the parent collects it with wait. WNOHANG means "don't
   block: if nobody has finished, return 0 immediately". */
static void reap_background_jobs(void)
{
    pid_t pid;
    int status;

    for (;;) {
        pid = waitpid(WAIT_ANY, &status, WNOHANG);    /* -1 = any child */
        if (pid <= 0)
            break;                              /* 0: none finished, -1: no children */
        printf("[done pid %d]\n", (int)pid);
    }
}

/* ---- Step 2: prompt (stdio buffering) ---------------------------
   The prompt has no newline, so a line-buffered terminal would NOT show it
   without fflush. */
static void print_prompt(void)
{
    printf("mini$ ");
    fflush(stdout);
}

/* ---- Step 3: read one line. Returns 0 on end-of-file (Ctrl+D). ------------ */
static int read_line(char *buf, int size)
{
    return fgets(buf, size, stdin) != NULL;
}

/* ---- Step 4: split the line into words --------------------------------------
   Fills argv (NULL-terminated, as exec requires) and returns argc.
   A trailing "&" is removed from argv and reported through *background. */
static int parse_line(char *line, char *argv[], int *background)
{
    int argc = 0;
    char *tok = strtok(line, " \t\n");

    while (tok != NULL && argc < MAXARGS - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;                          /* exec needs this terminator */

    *background = 0;
    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
        *background = 1;
        argv[--argc] = NULL;
    }
    return argc;
}

/* ---- Step 5: builtins (memory is NOT shared) -----------------------
   Why is cd a builtin? The current directory belongs to a process. If a CHILD
   ran chdir, only the child's directory would change, and it would die right
   after. The shell itself must do it. */
static int try_builtin(char *argv[])
{
    const char *dir;

    if (strcmp(argv[0], "exit") == 0)
        return BUILTIN_QUIT;

    if (strcmp(argv[0], "cd") == 0) {
        dir = (argv[1] != NULL) ? argv[1] : getenv("HOME");
        if (dir == NULL || chdir(dir) != 0)
            perror("cd");
        return BUILTIN_DONE;
    }

    if (strcmp(argv[0], "status") == 0) {
        printf("last status: %d\n", last_status);
        return BUILTIN_DONE;
    }
    return BUILTIN_NO;
}

/* ---- Step 6: fork, then each process takes ONE branch ------------ */
static void launch_command(char *argv[], int background)
{
    pid_t id;

    fflush(stdout);              /* empty the buffer BEFORE it gets copied */
    id = fork();

    if (id < 0) {                                /* -1: no child exists */
        perror("fork");
    } else if (id == 0) {                        /* 0: this is the CHILD */
        run_child(argv);                         /* never returns */
    } else if (background) {                     /* PARENT, command ended with & */
        printf("[started pid %d]\n", (int)id);   /* just don't wait */
    } else {                                     /* PARENT, normal command */
        wait_for_foreground(id);
    }
}

/* ---- Step 7: the child becomes the program -------------------
   Success: execvp never returns, this process's image is replaced.
   Failure: execvp returns -1 and the shell image is still intact. The child
   must DIE here, or it would carry on as a second copy of the shell.
   _exit (not exit) so we don't flush stdio buffers copied from the parent. */
static void run_child(char *argv[])
{
    execvp(argv[0], argv);
    perror(argv[0]);             /* stderr is unbuffered: appears immediately */
    _exit(EXIT_CMD_NOT_RUN);
}

/* ---- Step 8: parent waits for the child to die (paper 5.4) ------------------ */
static void wait_for_foreground(pid_t id)
{
    int status;

    if (waitpid(id, &status, 0) < 0)
        perror("waitpid");
    else
        report_status(status);
}

/* ---- Step 9: decode what happened to the child -------------------------------
   The exit status the child passed to exit()/_exit() is packed into the int
   that wait returns; the WIFEXITED, WEXITSTATUS and WTERMSIG macros unpack it. */
static void report_status(int status)
{
    if (WIFEXITED(status)) {
        last_status = WEXITSTATUS(status);
        if (last_status != 0)
            printf("[exit status %d]\n", last_status);
    } else if (WIFSIGNALED(status)) {
        last_status = 128 + WTERMSIG(status);
        printf("[killed by signal %d]\n", WTERMSIG(status));
    }
}

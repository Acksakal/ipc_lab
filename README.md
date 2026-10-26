# Unix processes & signals: learning experiments

Small C programs for learning how Unix processes and signals work, plus an interactive visualizer. Written for fun and practice, not production use.

## Files

| File | What it is |
|------|------------|
| `fork.c` | Parent creating child processes with `fork()` |
| `minishell.c` | A tiny shell: `fork` + `exec` + `wait` |
| `signal.c` | A timed multiplication quiz using `SIGALRM` and `SIGINT` |
| `process.html` | Step-by-step animation of how `minishell.c` works |

## fork.c

Creates 3 children from a single parent. Each child prints its own pid and its parent's pid, then calls `_exit(0)` so it doesn't continue the loop and fork more children. The parent prints a final message.

The commented-out block at the top is an earlier experiment with two sequential `fork()` calls. The notes below explain why it creates fewer processes than you might expect.

Things to try:
- Remove the `_exit(0)` and see how many processes appear (2³ = 8).
- Add `wait(NULL)` calls in the parent so `getppid()` in the children isn't affected by the parent exiting early.
- Redirect output (`./fork > out.txt`) to see stdio buffering duplicate lines.

## minishell.c

A minimal shell built from the ideas in the 1974 UNIX paper (sections on Processes and the Shell).

Supports:
- Simple commands (`ls -l`, `echo hi`), run with `fork` + `execvp` + `waitpid`
- Trailing `&` to run a command in the background
- Builtins: `cd`, `exit`, `status` (exit status of the last foreground command)
- `[done pid N]` messages for finished background jobs (collected with `waitpid(WNOHANG)` before each prompt)

Does not support: pipes, redirection, quoting, job control.

The code contains a few debug `puts` lines (`child here`, `parent here with &`, `last else, also parent`) that show which process is running which branch after `fork`.

## signal.c

A multiplication quiz. You get a random product (factors 0–14) and have **5 seconds** to answer.

- `alarm(5)` sends `SIGALRM` when time runs out.
- `Ctrl+C` (`SIGINT`) ends the game cleanly.
- The handlers are installed with `sigaction` and without `SA_RESTART`, so the signal interrupts the blocking `scanf`.
- `Ctrl+D` (EOF) also ends the game.

Prints your final score on exit.

## process.html

A self-contained step debugger (no dependencies): open it in any browser.

- Step through `minishell.c` one line at a time, in the parent or the child process.
- Scenarios: `echo hi`, `sleep 1 &`, a nonexistent command, and a nonexistent command *without* `_exit` (shows the child turning into a second shell).
- Choose whether the parent or the child runs first after `fork`, to see how the output order changes.
- Shows process cards (pid, image, state, zombies) and the shared terminal.
- Keys: ← → to step, Space to play/pause.
- A glossary paraphrases the relevant POSIX and C90 definitions.

## Build

```sh
gcc -std=gnu90 -Wall -Wpedantic fork.c -o fork
gcc -std=gnu90 -Wall -Wpedantic minishell.c -o minishell
gcc -std=gnu90 -Wall -Wpedantic signal.c -o signal
```

Requires a POSIX environment (Linux, macOS, WSL, Cygwin/MSYS2).


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Process structure
typedef struct {
    pid_t        pid;   // Process ID
    const char** args;  // Arguments array
} process_t;

// Create a process
process_t create_process(const char* args[]) {
    process_t process = {.pid = -1, .args = args};
    return process;
}

// Run the process synchronously
int process_run_sync(process_t* process) {
    if (!process->args) {
        fprintf(stderr, "Error: Arguments are NULL.\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // Child process
        execvp(process->args[0], (char* const*)process->args);
        perror("execvp");  // Only reached if execvp fails
        exit(EXIT_FAILURE);
    } else {
        // Parent process waits for the child
        process->pid = pid;
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return -1;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);  // Return child's exit code
        } else {
            fprintf(stderr, "Process did not exit normally.\n");
            return -1;
        }
    }
}

// Run the process asynchronously
int process_run_async(process_t* process) {
    if (!process->args) {
        fprintf(stderr, "Error: Arguments are NULL.\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // Child process
        char* const* args = (char* const*)process->args;

        sleep(2);
        fprintf(stdout, "Executing : %s\n", (char*)args);
        execvp(process->args[0], args);
        perror("execvp");  // Only reached if execvp fails
        exit(EXIT_FAILURE);
    } else {
        // Parent process does not wait
        process->pid = pid;
        return 0;  // Successfully launched the process
    }
}

// Terminate the process
int process_terminate(process_t* process) {
    if (process->pid <= 0) {
        fprintf(stderr, "Error: Invalid process ID.\n");
        return -1;
    }

    if (kill(process->pid, SIGKILL) == -1) {
        perror("kill");
        return -1;
    }

    printf("Process %d terminated.\n", process->pid);
    process->pid = -1;
    return 0;
}

int process_wait(process_t* p) {
    if (p == NULL) {
        fprintf(stderr, "Error: process_t pointer is NULL.\n");
        return -1;
    }

    int status;
    // Wait for the specific child process using its PID
    if (waitpid(p->pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    // Check if the child process terminated normally
    if (WIFEXITED(status)) {
        return WEXITSTATUS(
            status);  // Return the exit status of the child process
    } else {
        fprintf(stderr, "Child process did not exit normally.\n");
        return -1;
    }
}

// Example usage
int main() {
    // Define the arguments for the process (clang compilation)
    const char* args[] = {"clang", "-o", "output", "test.c", NULL};

    // Create the process
    process_t p = create_process(args);

    // Run synchronously
    printf("Executing ");
    int i = 0;
    while (args[i] != NULL) {
        printf(" %s ", args[i]);
        i++;
    }
    int result = process_run_sync(&p);
    if (result != 0) {
        fprintf(stderr, "Synchronous execution failed with code %d.\n", result);
    } else {
        printf("Synchronous execution succeeded.\n");
    }

    return 0;
}

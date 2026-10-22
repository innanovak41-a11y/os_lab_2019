#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // РЕБЕНОК
        printf("Child process (PID: %d) finished and becomes a ZOMBIE!\n", getpid());
        exit(0); // Ребенок сразу завершается, но родитель его не "хоронит"
    } else if (pid > 0) {
        // РОДИТЕЛЬ
        printf("Parent process (PID: %d) is sleeping for 10 seconds...\n", getpid());
        printf("Check 'ps aux | grep Z' in another terminal right now!\n");
        sleep(30); // Родитель спит и НЕ вызывает wait()
        printf("Parent woke up and exits. The zombie is gone.\n");
    } else {
        printf("Fork failed\n");
        return 1;
    }

    return 0;
}
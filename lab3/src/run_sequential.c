#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // РЕБЁНОК: заменяем себя на sequential_min_max
        execv("./sequential_min_max", argv);
        // Если execv вернулся — значит, ошибка
        perror("execv failed");
        return 1;
    } else {
        // РОДИТЕЛЬ: ждём завершения
        wait(NULL);
        printf("sequential_min_max finished\n");
    }

    return 0;
}
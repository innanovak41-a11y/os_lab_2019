#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для доступа из функции-обработчика сигнала
int global_pnum = 0;
pid_t *global_child_pids = NULL;

// Функция, которая вызовется, когда сработает будильник
void kill_children_handler(int sig) {
    printf("\n[TIMEOUT] Time is up! Killing all children with SIGKILL...\n");
    for (int i = 0; i < global_pnum; i++) {
        kill(global_child_pids[i], SIGKILL); 
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) { printf("seed must be positive\n"); return 1; }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) { printf("array_size must be positive\n"); return 1; }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) { printf("pnum must be positive\n"); return 1; }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\"\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;
  
  // Подготовка к таймауту
  global_pnum = pnum;
  global_child_pids = malloc(sizeof(pid_t) * pnum);

  if (timeout > 0) {
      signal(SIGALRM, kill_children_handler); 
      alarm(timeout);                         
  }

  // Создаём pipes ДО fork
  int pipes[pnum][2];
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        // РЕБЕНОК
        sleep(2); // Искусственная задержка для проверки таймаута!

        unsigned int portion = array_size / pnum;
        unsigned int begin = i * portion;
        unsigned int end = (i == pnum - 1) ? (unsigned int)array_size : begin + portion;

        struct MinMax local = GetMinMax(array, begin, end);

        if (with_files) {
          char filename[64];
          sprintf(filename, "child_%d.txt", i);
          FILE *f = fopen(filename, "w");
          if (f) {
            fprintf(f, "%d %d\n", local.min, local.max);
            fclose(f);
          }
        } else {
          // Закрываем все чужие pipes
          for (int k = 0; k < pnum; k++) {
            if (k != i) {
              close(pipes[k][0]);
              close(pipes[k][1]);
            }
          }
          // Пишем в свой pipe
          close(pipes[i][0]);
          write(pipes[i][1], &local, sizeof(local));
          close(pipes[i][1]);
        }
        return 0; 
      } else {
        // РОДИТЕЛЬ: сохраняем PID ребенка
        global_child_pids[i] = child_pid;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Родитель закрывает ВСЕ write-ends pipes
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  // Ожидание детей
  while (active_child_processes > 0) {
    pid_t pid = wait(NULL);
    if (pid > 0) {
      active_child_processes -= 1;
    } else if (pid == -1 && errno != EINTR) {
      break; 
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[64];
      sprintf(filename, "child_%d.txt", i);
      FILE *f = fopen(filename, "r");
      if (f) {
        fscanf(f, "%d %d", &min, &max);
        fclose(f);
      }
    } else {
      struct MinMax child_res;
      read(pipes[i][0], &child_res, sizeof(child_res));
      close(pipes[i][0]);
      min = child_res.min;
      max = child_res.max;
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(global_child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
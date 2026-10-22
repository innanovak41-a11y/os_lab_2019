#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <getopt.h>

// Подключаем GenerateArray из 3-й лабы (файл utils.c)
#include "utils.h" 

struct SumArgs {
  int *array;
  int begin;
  int end;
};

// Функция, которая будет выполняться в отдельном потоке
void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  int sum = 0;
  for (int i = sum_args->begin; i < sum_args->end; i++) {
    sum += sum_args->array[i];
  }
  // Возвращаем результат (приводим int к указателю)
  return (void *)(size_t)sum;
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  // 1. Парсинг аргументов командной строки (--threads_num, --seed, --array_size)
  while (1) {
    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    
    if (c == -1) break;
    
    switch (c) {
      case 0:
        switch (option_index) {
          case 0: threads_num = atoi(optarg); break;
          case 1: seed = atoi(optarg); break;
          case 2: array_size = atoi(optarg); break;
        }
        break;
    }
  }

  if (threads_num <= 0 || array_size <= 0 || seed <= 0) {
      printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
      return 1;
  }

  // 2. Генерация массива (ДО замера времени)
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];
  uint32_t portion = array_size / threads_num;

  struct timeval start_time, finish_time;
  gettimeofday(&start_time, NULL); // 3. СТАРТ ЗАМЕРА ВРЕМЕНИ

  // 4. Создание потоков
  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * portion;
    args[i].end = (i == threads_num - 1) ? array_size : (i * portion + portion);

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  // 5. Ожидание завершения потоков и сбор результатов
  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum); // Ждем поток и забираем результат
    total_sum += sum;
  }

  gettimeofday(&finish_time, NULL); // 6. КОНЕЦ ЗАМЕРА ВРЕМЕНИ

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %fms\n", elapsed_time);
  return 0;
}
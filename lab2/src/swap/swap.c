#include "swap.h"

void Swap(char *left, char *right) {
    char temp = *left;   // сохраняем значение, на которое указывает left
    *left = *right;      // в left записываем значение из right
    *right = temp;       // в right записываем сохранённое значение
}

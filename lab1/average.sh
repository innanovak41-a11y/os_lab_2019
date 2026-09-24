#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Нет аргументов"
    exit 1
fi
sum=0
count=$#
for arg in "$@"; do
    sum=$((sum + arg))
done
avg=$(echo "$sum $count" | awk '{printf "%.2f", $1/$2}')
echo "Количество: $count"
echo "Среднее: $avg"

#!/bin/sh
set -e

printf "Билдим скрипты\n"
make worker

TARGET="out/test"
STATS="out/stats.txt"
WORKERS=10
SECONDS_TO_WAIT=300


printf "Подготовка: файл для блокировок \"%s\", файл для статистики \"%s\"\n" $TARGET $STATS
mkdir -p out
touch $TARGET
: > $STATS

printf "Запускаем %d задач\n" $WORKERS
PIDS=""
i=1
while [ $i -le $WORKERS ]; do
    ./bin/worker "$TARGET" "$STATS" &
    PIDS="$PIDS $!"
    i=$((i + 1))
done

printf "Ожидание %d секунд\n" $SECONDS_TO_WAIT
sleep $SECONDS_TO_WAIT

printf "Остановка задач\n"
kill -INT $PIDS

wait

printf "Статистика из файла \"%s:\"\n" $STATS
cat $STATS
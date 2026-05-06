#!/bin/bash
set -e

NUMBERS_FILE="numbers.txt"
CONFIG_FILE="config"
SERVER_LOG_FILE="server.log"
SOCKET_FILE="/tmp/t4-server-socket"
CLIENTS_COUNT_TASK_1_2=100

printf " --- 0. Подготовка ---\n"

printf "0.0 Билдим скрипты\n"
make

printf "0.1 Проверка суммы в файле %s...\n" "$NUMBERS_FILE"
FILE_SUM=$(awk '{for(i=1;i<=NF;i++) sum+=$i} END {print sum}' "$NUMBERS_FILE")
echo "Сумма чисел в файле: $FILE_SUM"

if [ "$FILE_SUM" -ne 0 ]; then
    echo "Ошибка: сумма не ноль"
    exit 1
fi

echo "0.2 чистим $SERVER_LOG_FILE"
true > "$SERVER_LOG_FILE"

echo "0.3 удаляем файл сокета $SOCKET_FILE"
rm "$SOCKET_FILE" || true

echo
printf " --- 1. запуск сервера ---\n"
echo "1.0 bin/server $CONFIG_FILE $SERVER_LOG_FILE"
bin/server $CONFIG_FILE $SERVER_LOG_FILE &

echo
printf " --- 2. запуск клиентов ---\n"

echo "2.0 Запуск $CLIENTS_COUNT_TASK_1_2 клиентов (у каждого случайная задержка delay)"

pids=()
for _ in $(seq 1 "$CLIENTS_COUNT_TASK_1_2")
do
  bin/client --config="$CONFIG_FILE" --data="$NUMBERS_FILE" > /dev/null &
  pids+=($!)
done

echo "2.1 Ждем завершения всех клиентов..."
wait "${pids[@]}"

echo "2.2 Проверяем текущее состояние сервера"
RESPONSE=$(bin/client --config=config <<< 0)
echo "$RESPONSE"

if ! echo "$RESPONSE" | grep -q "Server response: 0"; then
  echo "Ошибка: сервер вернул не 0"
  exit 1
fi

echo "2.4 Проверка, что первый коннект и последний имеют одинаковый размер кучи"
grep "New client connected" server.log | head -n 1
grep "New client connected" server.log | tail -n 1

echo
printf " --- 3. Замеры эффективности ---\n"

DELAYS=(0 200 400 600 800 1000)
CLIENTS=(1 20 40 60 80 100)

CLIENT_STATS_FILE="client-stats.txt"
RESULT_FILE="result.txt"
true > "$RESULT_FILE"

echo "clients delay_ms server_time_sec max_client_delay_sec overhead_sec" >> "$RESULT_FILE"

calculate_server_time_from_logs() {
    local log_file="$1"

    local first_ts=$(grep -m 1 "Received" "$log_file" | grep -oP '\[\K[0-9:.]+(?=\])')
    local last_ts=$(grep "Received" "$log_file" | tail -n 1 | grep -oP '\[\K[0-9:.]+(?=\])')

    if [[ -z "$first_ts" || -z "$last_ts" ]]; then
        echo "Ошибка: не найден timestamp в логах для первой и последней отправки сообщения клиентом"
        exit 1
    fi

    awk -v first="$first_ts" -v last="$last_ts" 'BEGIN {
        split(first, a, ":"); split(last, b, ":");
        first_sec = a[1]*3600 + a[2]*60 + a[3];
        last_sec  = b[1]*3600 + b[2]*60 + b[3];
        printf "%.3f\n", last_sec - first_sec;
    }'
}

for delay in "${DELAYS[@]}"
do
  for clients in "${CLIENTS[@]}"
  do
    sleep 0.2

    echo
    echo "=== clients=$clients delay=${delay}ms ==="

    true > "$SERVER_LOG_FILE"
    true > "$CLIENT_STATS_FILE"

    START=$(date +%s.%3N)

    pids=()

    for _ in $(seq 1 "$clients")
    do
      bin/client \
        --config="$CONFIG_FILE" \
        --data="$NUMBERS_FILE" \
        --stats="$CLIENT_STATS_FILE" \
        --delay="$delay" \
        > /dev/null &

      pids+=($!)
    done

    wait "${pids[@]}"

    END=$(date +%s.%3N)

    SERVER_TIME=$(awk "BEGIN {print $END - $START}")

    MAX_DELAY=$(sort -nr "$CLIENT_STATS_FILE" | head -n 1)

    MAX_DELAY_SEC=$(awk "BEGIN {print $MAX_DELAY / 1000.0}")

    OVERHEAD=$(awk "BEGIN {print $SERVER_TIME - $MAX_DELAY_SEC}")

    echo "$clients $delay $SERVER_TIME $MAX_DELAY_SEC $OVERHEAD" >> "$RESULT_FILE"

    echo "server_time=$SERVER_TIME"
    echo "max_delay=$MAX_DELAY_SEC"
    echo "overhead=$OVERHEAD"

    RESPONSE=$(bin/client --config=config <<< 0)
    echo "$RESPONSE"

    if ! echo "$RESPONSE" | grep -q "Server response: 0"; then
      echo "Ошибка: сервер вернул не 0"
      exit 1
    fi
  done
done

echo
echo "Останавливаем сервер"
pkill server
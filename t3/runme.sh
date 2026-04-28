#!/bin/sh
set -e

MYINIT="$PWD/bin/myinit"
CONF="$PWD/config.txt"
LOGFILE="$PWD/myinit.log"
DUMMY_PROCESS_BIN="$PWD/bin/dummy"
DUMMY_PROCESS_OUTPUT="$PWD/out_myinit_dummy.txt"
TAIL_COPY="$PWD/tail_copy.txt"

rm -f "$TAIL_COPY" "$LOGFILE"
cat /dev/null > "$DUMMY_PROCESS_OUTPUT"

printf " --- 0. Подготовка ---\n"

printf "0.0 Билдим скрипты\n"
make

printf "0.1 Создаем файл конфига\n"
cat <<EOF > "$CONF"
/bin/sleep 100 /dev/null /dev/null
$DUMMY_PROCESS_BIN vkus_persika_marakua /dev/null $DUMMY_PROCESS_OUTPUT
/bin/tail -f $DUMMY_PROCESS_OUTPUT $TAIL_COPY
EOF

echo
printf " --- 1. Проверка запуска myinit ---\n"

printf "1.0 Запуск myinit с 3 процессами\n"
MYINIT_PID=$( $MYINIT "$CONF" "$LOGFILE" | grep -oP 'PID: \K\d+' )

if [ -z "$MYINIT_PID" ]; then
    echo "Ошибка: не удалось получить PID демона"
    exit 1
fi

echo "Подхвачен PID демона: $MYINIT_PID"
sleep 1

printf "1.1 Проверка ps: ожидаем 3 запущенных процесса из конфига\n"
pgrep -P "$MYINIT_PID" -a

echo
printf " --- 2. Проверка убийства дочернего процесса ---\n"

TARGET_PID=$(pgrep -f "vkus_persika_marakua")
printf "2.0 Убиваем процесс 2 (PID: %s)\n" "$TARGET_PID"
kill -9 "$TARGET_PID"

sleep 2
printf "2.1 Проверка ps после убийства (снова должно быть запущено 3 процесса из конфига):\n"
pgrep -P "$MYINIT_PID" -a

echo
printf " --- 3. Проверка перезагрузки конфига ---\n"

printf "3.0 Подготовка конфига с одним процессом dummy\n"
cat <<EOF > "$CONF"
$PWD/bin/dummy posle_sighup /dev/null $DUMMY_PROCESS_OUTPUT
EOF

printf "3.1 Отправляем SIGHUP\n"
kill -HUP "$MYINIT_PID"
sleep 1

printf "3.2 Проверка ps (должен остаться только один процесс dummy):\n"
pgrep -f "posle_sighup" -a

echo
printf " --- 4. Содержимое лога ---\n"
cat "$LOGFILE"

echo
printf " --- 5. Завершение myinit\n"
pkill myinit

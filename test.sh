#!/bin/sh

get_stat() {
    stat -c "%s %b" "$1"
}

set -- $(get_stat out/A)
A_size=$1
A_blocks=$2

set -- $(get_stat out/B)
B_size=$1
B_blocks=$2

set -- $(get_stat out/C)
C_size=$1
C_blocks=$2

set -- $(get_stat out/D)
D_size=$1
D_blocks=$2

set -- $(get_stat out/A.gz)
Agz_size=$1

set -- $(get_stat out/B.gz)
Bgz_size=$1

echo "Проверка логических размеров файлов..."

if [ "$A_size" -eq "$B_size" ] &&
   [ "$A_size" -eq "$C_size" ] &&
   [ "$A_size" -eq "$D_size" ]; then
    echo "OK: A B C D имеют одинаковый логический размер ($A_size)"
else
    echo "FAIL: логические размеры файлов отличаются"
    exit 1
fi


echo ""
echo "Проверка sparse файла B..."

if [ "$B_blocks" -lt "$A_blocks" ]; then
    echo "OK: B занимает меньше блоков чем A ($B_blocks < $A_blocks)"
else
    echo "FAIL: B не является sparse файлом"
    exit 1
fi


echo ""
echo "Проверка gzip..."

if [ "$Agz_size" -eq "$Bgz_size" ]; then
    echo "OK: A.gz и B.gz имеют одинаковый размер ($Agz_size)"
else
    echo "FAIL: gzip сжал файлы по-разному"
    exit 1
fi


echo ""
echo "Проверка восстановления sparse (файл C)..."

if [ "$C_blocks" -eq "$B_blocks" ]; then
    echo "OK: C имеет столько же блоков сколько B ($C_blocks)"
else
    echo "FAIL: C отличается от B"
    exit 1
fi


echo ""
echo "Проверка нестандартного размера блока (файл D)..."

if [ "$D_blocks" -ge "$B_blocks" ]; then
    echo "OK: D занимает столько же или больше блоков чем B ($D_blocks >= $B_blocks)"
else
    echo "FAIL: маленький буфер уменьшил число блоков"
    exit 1
fi

if [ "$D_blocks" -lt "$A_blocks" ]; then
    echo "OK: D всё ещё меньше чем A ($D_blocks < $A_blocks)"
else
    echo "FAIL: D занял столько же блоков как обычный файл"
    exit 1
fi


echo ""
echo "Все тесты успешно пройдены!"

#!/bin/sh
set -e

printf "0. Подготовка"
make
mkdir -p out
echo ""

echo "1. Создать файл A."
bin/fake-sparse-file-generator out/A
echo ""

echo "2. Скопировать созданный файл A через нашу программу в файл B, сделав его разреженным"
bin/sparser out/B out/A
echo ""

echo "3. Сжать A и B с помощью gzip"
gzip -c out/A > out/A.gz && gzip -c out/B > out/B.gz
echo ""

echo "4. Распаковать сжатый файл B в stdout и сохранить через программу в файл C"
gzip -cd out/B.gz | bin/sparser out/C
echo ""

echo "5. Скопировать A через программу в файл D, указав нестандартный размер блока - 100 байт."
bin/sparser out/D out/A 100
echo ""

echo "6. Программой stat вывести реальный размер файлов A, A.gz, B, B.gz, C, D"
for f in A A.gz B B.gz C D; do
    stat "out/$f" | head -n 2
    echo ""
done
echo ""

echo "7. Проверка корректности"
./test.sh
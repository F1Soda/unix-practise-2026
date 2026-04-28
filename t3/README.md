# Task 3

Запуск

```shell
./runme.sh
```

в runme запускается 3 процесса: `sleep`, `dummy`, `tail`

- `sleep` — ничего не делает
- `dummy` — простая программа, которая в промежуток времени пишет в файл `out_myinit_dummy.txt`
- `tail` — копирует вывод `dummy` из `out_myinit_dummy.txt` в `tail_copy.txt`
 
затем конфиг заменяется на запуск одного `dummy`

## Ожидаемое поведение:

После запуска `runme.sh`

в `myinit.log`:

```log
[19:09:04] [INFO] myinit started, PID: 11337
[19:09:04] [INFO] Output file /home/golik/unix-homework/t1/t3/tail_copy.txt will be created
[19:09:04] [INFO] Process started: /bin/sleep (pid 11339)
[19:09:04] [INFO] Process started: /home/golik/unix-homework/t1/t3/bin/dummy (pid 11340)
[19:09:04] [INFO] Process started: /bin/tail (pid 11341)
[19:09:05] [WARN] Child 11340 (/home/golik/unix-homework/t1/t3/bin/dummy) terminated by signal: 9
[19:09:05] [INFO] Successfully restarted child 11345 process /home/golik/unix-homework/t1/t3/bin/dummy
[19:09:06] [INFO] SIGHUP received. Reloading configuration...
[19:09:06] [INFO] Process stopped: /bin/sleep (pid 11339)
[19:09:06] [INFO] Process stopped: /home/golik/unix-homework/t1/t3/bin/dummy (pid 11345)
[19:09:06] [INFO] Process stopped: /bin/tail (pid 11341)
[19:09:06] [INFO] Process started: /home/golik/unix-homework/t1/t3/bin/dummy (pid 11353)
[19:09:08] [INFO] Shutdown signal received. Stopping all children...
[19:09:08] [INFO] Process stopped: /home/golik/unix-homework/t1/t3/bin/dummy (pid 11353)
[19:09:08] [INFO] Successfully stopping myinit: nothing to run
```

в `out_myinit_dummy.txt`:

```
[posle_sighup] PID 12719: 13
[posle_sighup] PID 12719: 97
```

в `tail_copy.txt`:

```
[vkus_persika_marakua] PID 12678: 68
[vkus_persika_marakua] PID 12683: 49
```

## Очистка созданных файлов

```shell
make clean
```
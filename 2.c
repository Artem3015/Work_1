#include <errno.h>

#include <fcntl.h>

#include <stdio.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#define MAXLINE 4096

#define SERV_FIFO "/tmp/fifo.serv"

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

char fifoname[MAXLINE];

void delete_fifo(void)

{

  unlink(fifoname);

}

int main(int argc, char **argv)

{

  int readfifo, writefifo;

  size_t len;

  ssize_t n;

  char *ptr, buff[MAXLINE];

  pid_t pid;

  /* создание FIFO с именем, включающим PID; если уже существует - OK */

  pid = getpid();

  snprintf(fifoname, sizeof(fifoname), "/tmp/fifo.%ld", (long) pid);

  if ((mkfifo(fifoname, FILE_MODE) < 0) && (errno != EEXIST))

  {

    fprintf(stderr, "Клиент: невозможно создать %s: %s\n", fifoname,

    strerror(errno));

    exit(1);

  }

  if (atexit(delete_fifo))

  {

    unlink(fifoname);

    fprintf(stderr, "Клиент: невозможно зарегистрировать delete_fifo: %s\n",

    strerror(errno));

    exit(1);

  }

  /* записываем в буфер PID и пробел */

  snprintf(buff, sizeof(buff), "%ld ", (long) pid);

  len = strlen(buff);

  ptr = buff + len;

  /* чтение полного имени файла из стандартного потока ввода */

  if (fgets(ptr, MAXLINE - len, stdin) == NULL)

  { /* fgets() гарантирует завершающий нулевой байт */

    fprintf(stderr, "Клиент: ошибка чтения имени файла из потока ввода\n");

    exit(1);

  }

  if (buff[len] == '\n')

  { /* при вводе имени файла был введен только перевод строки */

    fprintf(stderr, "Клиент: введено пустое имя файла, завершение работы\n");

    exit(1);

  }

  /* открытие FIFO сервера и запись в него PID и полного имени файла */

  if ((writefifo = open(SERV_FIFO, O_WRONLY, 0)) < 0)

  {

    fprintf(stderr, "Клиент: невозможно открыть %s: %s\n", SERV_FIFO,

    strerror(errno));

    exit(1);

  }

  if ((n=write(writefifo, buff, strlen(buff))) < 0)

  { 

    fprintf(stderr, "Клиент: ошибка записи полного имени файла в канал %s:

    %s\n", SERV_FIFO, strerror(errno));

    exit(1);

  }

  /* открытие своего FIFO; блокирование до его открытия сервером */

  if ((readfifo = open(fifoname, O_RDONLY, 0)) < 0)

  {

    fprintf(stderr, "Клиент: невозможно открыть %s: %s\n", fifoname,

    strerror(errno));

    exit(1);

  }

  /* считывание из канала FIFO, запись в стандартный поток вывода */

  while ((n = read(readfifo, buff, MAXLINE)) > 0)

  if (write(STDOUT_FILENO, buff, n) != n)

  {

    fprintf(stderr, "Клиент: ошибка записи  файла в поток вывода: %s\n", strerror(errno));

    exit(1);

  }

  if (n < 0)

  {

    fprintf(stderr, "Клиент: ошибка чтения файла из канала FIFO: %s\n", strerror(errno));

  exit(1);

  }

  close(readfifo);

  exit(0);

}

#include <errno.h>

#include <fcntl.h>

#include <stdio.h>

#include <unistd.h>

#include <sys/types.h>

#include <sys/stat.h>

#define MAXLINE 4096

#define SERV_FIFO "/tmp/fifo.serv"

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void delete_fifo(void)

{

  unlink(SERV_FIFO);

}

int main(int argc, char **argv)

{

  int readfifo, writefifo, dummyfd, fd;

  char *ptr, buff[MAXLINE], fifoname[MAXLINE];

  ssize_t n;

  FILE *readfp;

  /* создание FIFO сервера с известным именем; если уже существует - OK */

  if ((mkfifo(SERV_FIFO, FILE_MODE) < 0) && (errno != EEXIST))

  {

    fprintf(stderr, "Сервер: невозможно создать %s: %s\n", SERV_FIFO,

    strerror(errno));

    exit(1);

  }

  if (atexit(delete_fifo))

  {

    unlink(SERV_FIFO);

    fprintf(stderr, "Сервер: невозможно зарегистрировать delete_fifo: %s\n",

    strerror(errno));

    exit(1);

  }

  /* открытие FIFO сервера для чтения */

  if ((readfifo = open(SERV_FIFO, O_RDONLY, 0)) < 0)

  {

    fprintf(stderr, "Сервер: невозможно открыть %s для чтения: %s\n", SERV_FIFO,

    strerror(errno));

    exit(1);

  } /* открытие FIFO сервера для записи. Этот дескриптор не используется */

  if ((dummyfd = open(SERV_FIFO, O_WRONLY, 0)) < 0)

  {

    fprintf(stderr, "Сервер: невозможно открыть %s для записи: %s\n", SERV_FIFO,

    strerror(errno));

    exit(1);

  }

  if ((readfp = fdopen(readfifo, "r")) == NULL) /* открываем поток для fgets */

  {

    fprintf(stderr, "Сервер: невозможно переоткрыть %s для чтения: %s\n",

    SERV_FIFO, strerror(errno));

    exit(1);

  }

  setbuf(readfp, NULL); 

  /* цикл приема запросов от клиентов */

  while (fgets(buff, MAXLINE, readfp) != NULL)

    { 
      if (buff[strlen(buff)-1] == '\n') /* удаление символа перевода строки */

      buff[strlen(buff)-1] = '\0'; /* (если есть) */

      if ((ptr = strchr(buff, ' ')) == NULL)

      {

        fprintf(stderr, "Сервер: неправильный запрос: %s\n", buff);

        continue;

      }

      *ptr++ = '\0'; /* нулевой байт вставлен в buff на место пробела */

      snprintf(fifoname, sizeof(fifoname), "/tmp/fifo.%s", buff);

      if ((writefifo = open(fifoname, O_WRONLY, 0)) < 0)

      {

        fprintf(stderr, "Сервер: невозможно открыть %s для записи: %s\n",

        fifoname, strerror(errno));

        continue;

      }

      if ((fd = open(ptr, O_RDONLY)) < 0)

      { /* сообщаем клиенту об ошибке открытия файла */

        snprintf(fifoname, sizeof(fifoname), "Сервер: ошибка открытия файла %s:%s\n", ptr, strerror(errno));

        write(writefifo, fifoname, strlen(fifoname));

        close(writefifo);

      }

      else /* файл успешно открыт; копируем его в FIFO */

      {

        while ((n = read(fd, fifoname, MAXLINE)) > 0)

        if (write(writefifo, fifoname, n) != n)

        {

            snprintf(fifoname, sizeof(fifoname), "Сервер: ошибка записи файла %s в FIFO: %s\n", ptr, strerror(errno));

            write(writefifo, fifoname, strlen(fifoname));

            exit(1);

          }

          if (n < 0)

          {

              snprintf(fifoname, sizeof(fifoname), "Сервер: ошибка чтения файла %s: %s\n", ptr, strerror(errno));

              write(writefifo, fifoname, strlen(fifoname));

              exit(1);

          }

          close(fd);

          close(writefifo);
    
    }

  }

}

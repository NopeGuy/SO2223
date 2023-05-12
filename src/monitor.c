#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "tracer.h"

pedido global[100]; // array para guardar os pedidos
int pos = 0;        // ultima posicao no array global

void status(pedido global[], int N)
{
    int bytes_written;
    struct timeval now;
    gettimeofday(&now, NULL);

    int fd = open("../fifos/main", O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "Erro ao abrir o main fifo no monitor!!!\n");
        fflush(stderr);
        _exit(1);
    }

    for (int i = 0; i < N; i++)
    {
        suseconds_t tempo = now.tv_usec - global[i].inicial;
        char resposta[100];
        snprintf(resposta, sizeof(resposta), "Pid:%d encontra-se em execução há %ld ms\n", global[i].pid, tempo / 1000);
        if ((bytes_written = write(fd, resposta, strlen(resposta)) == 0))
        {
            fprintf(stderr, "Erro a enviar o status!!!\n");
            fflush(stderr);
            _exit(1);
        }
    }
    close(fd);
}

int main(int argc, char **argv)
{
    int fd_read, fd_write, bytes_read;
    pedido pedido;

    mkfifo("../fifos/main", 0666);

    if((fd_read = open("../fifos/main", O_RDONLY))==-1)
    {
        fprintf(stderr, "Erro ao abrir o main fifo (read) (poderá não estar criado) !!!\n");
        fflush(stderr);
        _exit(1);
    }
    if((fd_write = open("../fifos/main", O_WRONLY))==-1) // Para manter o servidor aberto
    {
        fprintf(stderr, "Erro ao abrir o main fifo (write)!!!\n");
        fflush(stderr);
        _exit(1);
    }

    while ((bytes_read = read(fd_read, &pedido, sizeof(pedido))) > 0)
    {
        if (!strcmp(pedido.commando, "status"))
        {
            status(global, pos);
        }
        else
        {
            int flag = 0;
            for (int j = 0; j <= pos; j++)
            {
                if (global[j].pid == pedido.pid)
                {
                    global[j] = global[pos - 1];
                    pos--;
                    flag = 1;
                    break;
                }
            }
            if (!flag)
            {
                global[pos] = pedido;
                pos++;
            }
        }
    }
    if (bytes_read == 0)
    {
        fprintf(stderr, "Erro ao ler o main fifo!!!\n");
        fflush(stderr);
        _exit(1);
    }

    close(fd_read);
    close(fd_write);

    unlink("../fifos/main");

    return 0;
}
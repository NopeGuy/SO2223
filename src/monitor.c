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

pedido global[100]; //array para guardar os pedidos
int pos=0; //ultima posicao no array global

void status(pedido global[], int N)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    int fd = open("../fifos/write", O_WRONLY);

    for (int i = 0; i < N; i++)
    {
        suseconds_t elapsed = (now.tv_sec - global[i].inicial / 1000) * 1000 + (now.tv_usec - global[i].inicial % 1000) / 1000;
        char resposta[100];
        snprintf(resposta, sizeof(resposta), "Pid:%d encontra-se em execução há %ld ms\n", global[i].pid, elapsed);
        write(fd, resposta, strlen(resposta));
    }
    close(fd);
    gettimeofday(&now, NULL);
}


int main(int argc, char **argv)
{
    memset(global, 0, sizeof(global));
    int res_fifo, res_fifo2, fd_read, fd_write, bytes_read;
    pedido pedido;

    res_fifo = mkfifo("../fifos/read", 0666);
    res_fifo2 = mkfifo("../fifos/write", 0666);

    if ((fd_read = open("../fifos/read", O_RDONLY)) == -1)
    {
        fprintf(stderr, "Erro ao abrir o main fifo (read) (poderá não estar criado) !!!\n");
        fflush(stderr);
        _exit(1);
    }
    if ((fd_write = open("../fifos/read", O_WRONLY)) == -1) // Para manter o servidor aberto
    {
        fprintf(stderr, "Erro ao abrir o main fifo (write)!!!\n");
        fflush(stderr);
        _exit(1);
    }

    while ((bytes_read = read(fd_read, &pedido, (sizeof(pedido))*2)) > 0)
    {
        if (!strcmp(pedido.commando, "status"))
        {
            status(global, pos);
        }
        else if (!strcmp(pedido.commando, "break"))
        {
            exit(1);
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

    close(fd_read);
    close(fd_write);

    unlink("../fifos/read");
    unlink("../fifos/write");

    return 0;
}
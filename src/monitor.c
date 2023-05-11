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

    int fd = open("../fifos/main", O_WRONLY);

    for (int i = 0; i < N; i++)
    {
        suseconds_t tempo = now.tv_usec - global[i].inicial;
        char resposta[60];
        snprintf(resposta, sizeof(resposta), "Pid:%d encontra-se em execução há %ld ms\n", global[i].pid, tempo / 1000);
        write(fd, resposta, strlen(resposta));
    }
    close(fd);
}

int main(int argc, char **argv)
{
    int res_fifo, fd_read, fd_write, bytes_read;
    pedido pedido;

    res_fifo = mkfifo("../fifos/main", 0666);
    
    fd_read = open("../fifos/main", O_RDONLY);
    fd_write = open("../fifos/main", O_WRONLY); // Para manter o servidor aberto

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

    close(fd_read);
    close(fd_write);

    unlink("../fifos/main");

    return 0;
}
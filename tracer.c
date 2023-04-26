#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "tracer.h"

// funcoes auxiliares

void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;         /* make n positive */
    i = 0;
    do
    {                          /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);   /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

// funções primarias

// calcula o tempo de execucao do pedido (fazer isto com os valores diretamente ou atraves da struct?)
time_t calcExec(pedido pedido)
{
    return pedido.final - pedido.inicial;
}

// cria um novo fifo para o cliente (através do pid)
int criaLigacao(pid_t pid)
{
    char *temp = "../fifos/";
    char *cPid = malloc(sizeof(pid_t));
    itoa(pid, cPid);
    strcat(temp, cPid);
    if (mkfifo(temp, 0666) == 0)
        return 0;
    return -1;
}

// copia apenas a parte do comando relevante para posteriormente acopular à struct pedido
char *extraiComandoString(int argc, char **argv)
{
    char *cmd = malloc(sizeof(argv));
    cmd = "";
    for (int i = 3; i < argc; i++)
    {
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }
    return cmd;
}
void extraiComandoArray(char** str, int argc, char* argv[]) {
    if (argc >= 3) {  // Check that there are at least 3 arguments
        // Allocate buffer for concatenated arguments
        int len = 0, i;
        for (i = 2; i < argc; i++) {
            len += strlen(argv[i]) + 1;  // Add length of argument and space
        }
        *str = malloc(len);  // Allocate buffer
        if (*str != NULL) {  // Check for allocation success
            (*str)[0] = '\0';  // Initialize buffer as empty string
            // Concatenate all arguments after the first two to buffer
            for (i = 2; i < argc; i++) {
                strncat(*str, argv[i], len - strlen(*str) - 1);
                strncat(*str, " ", len - strlen(*str) - 1);
                if (strlen(*str) >= len - 1) {
                    break;  // Buffer overflow, stop copying
                }
            }
            (*str)[strlen(*str) - 1] = '\0';  // Remove last space
        }
    }
}

// cria (struct)pedido com a informação
/* Já não é necessario
pedido criaPedido(pid_t pid, char *commando, time_t inicial,time_t final)
{
    pedido temp;
    temp.pid=pid;
    temp.commando=commando;
    temp.inicial=inicial;
    temp.final=final;
    return temp;
}
*/

int main(int argc, char *argv[])
{
    /* Para testar sem recorrer ao servidor
    int main_fd = open("../fifos/main", O_WRONLY); // Comunicação principal com o servidor (o servidor é responsável pela criação)
    if (main_fd < 0)
    {
        printf("Erro ao abrir o main fifo!!!\n");
        fflush(stdout);
        return -1;
    }
    */

    // caso do execute
    if (argc > 2 && strcmp("execute", argv[1]) == 0 && strcmp(argv[2], "-u") == 0)
    {
        // Criação do pedido, já com o comando, status a 0

        pedido pedido;
        pedido.status = 0; // estado do pedido
        int status = 0;
        pid_t pid;
        struct timeval inicial, final; // struct auxiliar de gettimeofday
        time_t start, finish;          // valores finais da timeval (o que se quer)
        char *cmd;
        cmd = extraiComandoString(argc, argv); // retira o "execute -u" do comando inicial
        pedido.commando = cmd;

        char *cmds[100];
        extraiComandoArray(cmds,argc,argv);

        // Criação de um pipe para comunicar o output para o pai

        int fildes[2];
        if (pipe(fildes) == -1)
        {
            perror("pipe do stdout do comando a executar");
            _exit(1);
        }

        if ((pid = fork()) == 0)
        {
            close(fildes[0]); // fechar o apontador de leitura (desnecessário)

            // Escreve para o stdout o pid que está a correr
            char res[20]= "Running PID ";
            char *resIn = res;
            char *sPID="";
            itoa(getpid(), sPID);
            strcat(resIn, sPID);
            write(STDOUT_FILENO, &resIn, sizeof(resIn));

            // adiciona o tempo inicial e o pid à struct
            gettimeofday(&inicial, NULL);
            start = inicial.tv_usec;
            pedido.inicial = start;
            pedido.pid = getpid();

            /* Para testar sem recorrer ao servidor             // escreve para o servidor a struct pedido com todos os valores menos o tempo final
            write(main_fd, &pedido, sizeof(pedido));
            close(main_fd);
            */

            // redirecionar o apontador do pipe fildes para o stdout
            dup2(fildes[1], STDOUT_FILENO);

            execvp(cmds[0], cmds);
            
            printf("Programa com o pid %d não concluído!\n", getpid());
            fflush(stdout);
            _exit(1);
        }
        else if (pid == -1)
            perror("fork"); // caso dê erro no fork()
        else
        {
            // escrever o output do programa do filho para o stdout

            close(fildes[1]); // fechar o extremo de escrita do output do comando
            char buff[1024];
            int bytes_read;
            while ((bytes_read = read(fildes[0], &buff, sizeof(buff))) > 0)
            {
                write(STDOUT_FILENO, &bytes_read, sizeof(bytes_read));
            }
            close(fildes[0]); // fechar o extremo de leitura do output do comando do filho

            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                // adiciona o tempo final ao pedido
                pedido.status = 1;
                gettimeofday(&final, NULL);
                finish = final.tv_usec;
                pedido.final = finish;

                // envia para o stdout o tempo de execução do pedido
                char resposta[20] = "Ended in ";
                char *tempExec="";
                itoa(calcExec(pedido), tempExec);
                strcat(tempExec, resposta);
                strcat(" ms\n", resposta);

                write(STDOUT_FILENO, &resposta, sizeof(resposta));
                _exit(0);
            }
            else
            {
                printf("Erro de espera pelo processo filho %d", pid);
                fflush(stdout);
                _exit(1);
            }
        }
    }

    // caso do status
    else if (argc == 2 && strcmp(argv[1], "status") == 0)
    {






    }
    // caso de comando inválido
    else
    {
        printf("\nComando inválido, por favor execute o cliente novamente com um dos seguintes comandos:\n 1) execute -u <comando>\n2) status\n");
        fflush(stdout);
    }

    return 0;
}
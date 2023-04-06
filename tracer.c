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

//funcoes auxiliares

 void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }
void itoa(int n, char s[])
 {
     int i, sign;
 
     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }

//funções primarias

//calcula o tempo de execucao do pedido (fazer isto com os valores diretamente ou atraves da struct?)
time_t calcExec(pedido pedido)
{
    return pedido.final-pedido.inicial;
}

//cria um novo fifo para o cliente (através do pid)
int criaLigacao(pid_t pid)
{
    char*temp="/fifo/";
    char*cPid=malloc(sizeof(pid_t));
    itoa(pid,cPid);
    strcat(temp,cPid);
    if(mkfifo(temp,0)==0) return 0;
    return -1;
}

//copia apenas a parte do comando relevante para posteriormente acopular à struct pedido
char* extraiComandoString(int argc,char** argv)
{
    char*cmd=malloc(sizeof(char)*(argc-2));
    cmd="";
    if(argc)
    for(int i=3;i<argc;i++)
    {
        strcat(cmd,argv[i]);
        strcat(cmd," ");
    }
    return cmd;
}
char** extraiComandoArray(int argc, char*comando)
{
    char*cmds[20];
    for(int i=0;i<argc-3;i++)
    {
        cmds[i]=strsep(&comando," ");
    }
    return cmds;
}

//cria (struct)pedido com a informação
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

int main(int argc,char* argv[])
{
    //caso do execute
    if(argc>2 && strcmp("execute",argv[1])==0 && strcmp(argv[2],"-u")==0)
    {
        pedido pedido;
        int status=0;
        pid_t pid;
        struct timeval inicial,final;//struct auxiliar de gettimeofday
        time_t start,finish;//valores finais da timeval (o que se quer)
        char* cmd;
        cmd=extraiComandoString(argc,argv);
        pedido.commando=cmd;

        char** cmds=malloc(sizeof(cmd)*argc);                               //ver isto
        cmds=extraiComandoArray(argc,cmd);
        

        if((pid=fork())==0)
        {
            //Escreve para o stdout o pid que está a correr
            char resIn[20]="Running PID ";
            char *sPID;
            itoa(getpid(),sPID);
            strcat(resIn,sPID);
            write(STDOUT_FILENO,&resIn,sizeof(resIn));

            //adiciona o tempo inicial da execução à struct pedido
            gettimeofday(&inicial,NULL);
            start=inicial.tv_usec;
            pedido.inicial=start;

            execvp(cmds[0],cmds);
                                        //falta escrever o resultado da execucao para o stdout
        }
        else
        {
            //adiciona o pid ao pedido
            pedido.pid=pid;

            waitpid(pid,&status,0);
            if(WIFEXITED(status))
            {
                //adiciona o tempo final ao pedido
                gettimeofday(&final,NULL);
                finish=final.tv_usec;
                pedido.final=finish;

                //envia para o stdout o tempo de execução do pedido
                char resposta[20]="Ended in ";
                char tempExec;
                itoa(calcExec(pedido),tempExec);
                strcat(tempExec,resposta);
                strcat(" ms\n",resposta);

                write(STDOUT_FILENO,&resposta,sizeof(resposta));
            }
        }

}


    //caso do status
if(argc==2 && strcmp(argv[1],"status")==0)
{





}

return 0;
}
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
    }
    return cmd;
}
char** extraiComandoArray(int argc, char*comando)
{
    char*cmds[20];
    strsep(&cmds," "); //eliminar a
    for(int i=0;i<argc-3;i++)
    {
        strsep(&cmds," ");
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
        struct timeval inicial,final;
        time_t start,finish;
        char* cmd;
        char** cmds=malloc(sizeof(char)*argc);
        
        cmd=extraiComandoString(argc,argv);
        cmds=extraiComandoArray(argc,cmd);
        pedido.commando=cmd;
        gettimeofday(&inicial,NULL);
        start=inicial.tv_usec;
        pedido.inicial=start;

        if((pid=fork())==0)
        {
            //falta escrever para o stdout que o programa iniciou com o PID x
            execvp(argv[3],cmds);
            //falta escrever o resultado da execucao para o stdout
        }
        else
        {
            pedido.pid=pid;
            waitpid(pid,&status,0);
            if(WIFEXITED(status))
            {
                gettimeofday(&final,NULL);
                finish=final.tv_usec;
                pedido.final=finish;

                //resposta a enviar 
                char resposta[20]="Ended in ";
                char tempExec;
                itoa(calcExec(pedido),tempExec);
                strcat(tempExec,resposta);
                strcat(" ms\n",resposta);

                write(STDOUT_FILENO,resposta,sizeof(resposta));
            }
        }

}


    //caso do status
if(argc==2 && strcmp(argv[1],"status")==0)
{





}

return 0;
}
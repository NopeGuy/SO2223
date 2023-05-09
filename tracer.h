#ifndef TRACER_H
#define TRACER_H


typedef struct pedido{
    pid_t pid;
    char *commando;
    suseconds_t inicial;
    suseconds_t final;
    int status; // 0 - nao acabou | 1 - acabou
}pedido;

int criaLigacao(pid_t pid);
char* extraiComandoString(int argc,char** argv);
void extraiComandoArray(char** str, int argc, char* argv[]);
pedido criaPedido(pid_t pid, char *commando, time_t inicial,time_t final);
time_t calcExec(pedido pedido);



#endif
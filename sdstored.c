#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <wait.h>
#define MAX_BUFFER 1024
//server

//struct correspondente ao limite das transformações
typedef struct comandos {
    int transform;// id da transformacao
    int max; //numero max de concurrentes
    struct comandos *prox;
} *Cmd;

typedef struct tarefas {
    int transform_atual; // 0 - nop; 1 - bcompress; 2 - bdecompress; 3 - gcompress; 4 - gdecompress; 5 - encrypt; 6 - decrypt
    int *transformacoes; //lista total de transformaçoes a efetuar em inteiro com o mesmo indice acima
    int numero; //[índice] nº processo
    int processamento; // 0 - a espera; 1 - a processar
    pid_t pid;
    int concur; // nº atual em processamento
    char *input_file; 
    char *output_file;
    struct tarefas *prox;
} *Task;

char*line[MAX_BUFFER]; // bufer que contem cada linha do ficheiro de comandos (cada comando e nº de concur maximas)
Task tarefas; // Struct principal com todos os dados (status)
Cmd lim_cmd; // Struct com limites de cada transformação

int indice_trans (char* token)
{
    int res;
    char*boda="bcompress";
        if(strcmp(token,boda)==0) res=0;
    boda="bcompress";
        if(strcmp(token,boda)==0) res=1;
    boda="bdecompress";
        if(strcmp(token,boda)==0) res=2;
    boda="gcompress";
        if(strcmp(token,boda)==0) res=3;
    boda="gdecompress";
        if(strcmp(token,boda)==0) res=4;
    boda="encrypt";
        if(strcmp(token,boda)==0) res=5;
    boda="decrypt";
        if(strcmp(token,boda)==0) res=6;
        else return 0;
        return res;
}
ssize_t readln(int fd,char* line, ssize_t size) { //lê as linhas do ficheiro
	ssize_t res = 0;
	ssize_t i = 0;
	while ((res = read(fd, &line[i], size)) > 0 && ((char) line[i] != '\n')) {
		i+=res;
	}
	return i;
}

//funcao que remove uma tarefa terminada com processamento 1 (para atualizar a struct principal); num é o numero do processo
Task removeTask(int num) {
    Task atual = tarefas;
    Task ant = NULL;

    while (atual != NULL && atual->numero != num) {
        ant = atual;
        atual = atual->prox;
    }
    if (atual != NULL && atual->processamento==1) {
        if (ant == NULL) 
        {
            tarefas = atual->prox;
        } else {
            ant->prox = atual->prox;
        }
        free(atual);
    }
    return tarefas;
}

//funcao que atualiza o estado de concurrentes de uma transformação, recebendo o indice da transformação 
int atualiza_concur(int indice_transform) {
    Task atual = tarefas;
    Task atual2 = tarefas;
    Cmd lim = lim_cmd;
    int concur=0;
    while(lim!=NULL && lim->transform!=indice_transform)lim=lim->prox;
    while(atual!=NULL && atual->transform_atual!=indice_transform)
    {
        if(atual->processamento==1) concur++;
        atual = atual->prox;
    }
        for (;atual2!=NULL && atual2->transform_atual!=indice_transform;atual2 = atual2->prox) { //do i need this second one???
        atual2->concur=concur;
                if(atual2->concur>lim->max) 
                {
                printf("Processo nº %d ultrapassa máximo concurrentes permitidas",indice_transform);
                free(atual);
                return 0;
                }
    }
    tarefas = atual2; // nao sei se posso fazer assim ?
    free(atual);
    free(atual2);
    free(lim);
    return 1;
}

//Verifica se outra transformação pode ser processada (verificando a concur e max_concur)
int disponivel(int indice_transform) {
    Task atual = tarefas;
    Cmd lim = lim_cmd;
    while(atual->transform_atual != indice_transform) atual=atual->prox;
    while(lim->transform!=indice_transform) lim=lim->prox;
    if (atual->concur < lim->max)
    {
        free(atual);
        free(lim);
        return 1;
    }
    else 
    {
        free(atual);
        free(lim);
        return 0;
    }
}

//funcao que lê a lista de comandos e max de concurrencias e põe na struct secundária 'comandos'
Cmd limite_comandos
(char* path)
{
    int fd;
    if ((fd = open(path, O_RDONLY)) < 0) {
        perror("Erro ao abrir ficheiro");
        _exit(-1);
    }
    lim_cmd = NULL;
    Cmd temp = NULL;
    Cmd ant = NULL;

    char *token;
    while (readln(fd, *line, 1024) > 0) {
        temp = malloc(sizeof(struct comandos));
        if (lim_cmd == NULL) lim_cmd = temp;
        else ant->prox = temp;
        
        token = strtok(*line, " ");
        temp->transform=indice_trans(token);
        token = strtok(NULL, " ");
        temp->max = atoi(token);
        temp->prox = NULL;
        ant = temp;
    }
    free(temp);
    free(ant);
    return lim_cmd;
}
int tamanho_array (int* a)
{   int arr=sizeof(a);
    int res=arr/sizeof(int);
    return res;
}
//traduz uma lista de comandos para uma lista de int correspondentes ao indice das transformacoes
int* lista_trans
(char* comando) 
{
    char* token;
    int* res=malloc(sizeof(int)*30);
    int i=0,j;
    token=strtok(comando," ");
    while(token!=NULL)
    {
        if((j=indice_trans(token)) != 0)
        {
            res[i]=j;
            i++;
        }
        token=strtok(NULL," ");
    }
    if(realloc(res,sizeof(int)*i)==NULL)
    perror("realloc->lista_trans");
    return res;
}
//Para reverter a lista de transformacoes de modo a recuperar um ficheiro
int* reverse(int s[]) {
    int i, j, c;
    for (i = 0, j = tamanho_array(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
    return s;
}

int main(int argc,char* argv[])
{

}
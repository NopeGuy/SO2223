#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <wait.h>
#define MAX_BUFFER 1024*1024
//server

//struct ligada das tarefas ()
typedef struct porfazer {
    int transform;// id da transformacao
    int numero; //[indice] ordem das transformacoes no ficheiro output
    int max; //numero max de concurrentes delimitada pelo ficheiro input
    int done; // se já foi inserida na struct principal -> tarefas; 0 - nao, 1 - sim
    struct porfazer *prox;
} *Undone;

typedef struct tarefas {
    int transform; // 0 - nop; 1 - bcompress; 2 - bdecompress; 3 - gcompress; 4 - gdecompress; 5 - encrypt; 6 - decrypt
    int numero; //[índice] nº processo
    int processamento; // 0 - a espera; 1 - a processar
    pid_t pid;
    int concur; // nº atual em processamento
    int max_concur; // equivalente ao max da struct input
    char *input_file; 
    char *output_file;
    struct tarefas *prox;
} *Task;

char*line[MAX_BUFFER]; // bufer que contem cada linha do ficheiro de comandos (cada comando e nº de concur maximas)
Task tarefas; // Struct principal com todos os dados (status)
Undone input; // Struct com as transformacoes do ficheiro input


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
//e o possivel novo numero maximo de concurrencias
int atualiza_concur(int indice_transform,int max_concur) {
    Task atual = tarefas;
    int concur=0;
    for (;atual!=NULL && atual->transform!=indice_transform;atual = atual->prox) {
        if(atual->processamento==1) concur++;
    }
        for (;atual!=NULL && atual->transform!=indice_transform;atual = atual->prox) {
        atual->concur=concur;
        atual->max_concur=max_concur;
        if(concur>max_concur) 
        {
        printf("Processo nº %d ultrapassa máximo concurrentes permitidas",indice_transform);
        return 0;
        }
    }
    return 1;
}
//Verifica se outra transformação pode ser processada (verificando a concur e max_concur)
int disponivel(int indice_transform) {
    Task atual = tarefas;
    while(atual->transform != indice_transform) atual=atual->prox;
    if (atual->concur<atual->max_concur) return 1;
    else return 0;
}

//POR VER!!!!
//funcao que lê a lista de comandos do ficheiro_input e põe na struct secundária *input*
Task ler_input (char* path)
{
    int fd;
    if ((fd = open(path, O_RDONLY)) < 0) {
        perror("Erro ao abrir ficheiro");
        _exit(-1);
    }
    Filtro filtros = NULL;
    Filtro temp = NULL;
    Filtro ant = NULL;

    char *token;
    while (readln(fd, line, 1) > 0) {
        temp = malloc(sizeof(struct tarefas));
        if (filtros == NULL) filtros = temp;
        else ant->prox = temp;
        
        token = strtok(line, " ");
        temp->nome_filtro = malloc(sizeof(char) * (strlen(token)+1));
        strcpy(temp->nome_filtro, token);

        token = strtok(NULL, " ");
        temp->nome_executavel = malloc(sizeof(char) * (strlen(token)+1));
        strcpy(temp->nome_executavel, token);

        token = strtok(NULL, " ");
        temp->maximo = atoi(token);

        temp->atual = 0;
        temp->prox = NULL;

        ant = temp;
    }
    return filtros;
}

int main(int argc,char* argv[])
{

}
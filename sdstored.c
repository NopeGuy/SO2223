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
// server

// struct correspondente ao limite das transformações
typedef struct comandos
{
    int transform; // id da transformacao
    int max;       // numero max de concurrentes
    struct comandos *prox;
} * Cmd;

typedef struct tarefas
{
    int transform_atual; // 0 - nop; 1 - bcompress; 2 - bdecompress; 3 - gcompress; 4 - gdecompress; 5 - encrypt; 6 - decrypt
    int *transformacoes; // lista total de transformaçoes a efetuar em inteiro com o mesmo indice acima
    int numero;          //[índice] nº processo
    int processamento;   // 0 - a espera; 1 - a processar
    pid_t pid;
    int concur; // nº atual em processamento
    char *input_file;
    char *output_file;
    struct tarefas *prox;
} * Task;

char *line[MAX_BUFFER]; // Bufer que contem cada linha do ficheiro de comandos (cada comando e nº de concur maximas)
char *path_trans;       // Path da pasta onde se encontram as transformacoes
Task tarefas;           // Struct principal com todos os dados (status)
Cmd lim_cmd;            // Struct com limites de cada transformação

int indice_trans(char *token) // Traduz as transformaçoes em inteiros
{
    int res = -1; // se indice_trans =-1 -> erro!!!
    char *boda = "nop";
    if (strcmp(token, boda) == 0)
        res = 0;
    boda = "bcompress";
    if (strcmp(token, boda) == 0)
        res = 1;
    boda = "bdecompress";
    if (strcmp(token, boda) == 0)
        res = 2;
    boda = "gcompress";
    if (strcmp(token, boda) == 0)
        res = 3;
    boda = "gdecompress";
    if (strcmp(token, boda) == 0)
        res = 4;
    boda = "encrypt";
    if (strcmp(token, boda) == 0)
        res = 5;
    boda = "decrypt";
    if (strcmp(token, boda) == 0)
        res = 6;
    return res;
}
ssize_t readln(int fd, char *line, ssize_t size)
{ // lê as linhas do ficheiro
    ssize_t res = 0;
    ssize_t i = 0;
    while ((res = read(fd, &line[i], size)) > 0 && ((char)line[i] != '\n'))
    {
        i += res;
    }
    return i;
}

// funcao que remove uma tarefa terminada com processamento 1 (para atualizar a struct principal); num é o numero do processo
Task removeTask(int num)
{
    Task atual = tarefas;
    Task ant = NULL;

    while (atual != NULL && atual->numero != num)
    {
        ant = atual;
        atual = atual->prox;
    }
    if (atual != NULL && atual->processamento == 1)
    {
        if (ant == NULL)
        {
            tarefas = atual->prox;
        }
        else
        {
            ant->prox = atual->prox;
        }
        free(atual);
    }
    return tarefas;
}

// funcao que atualiza o estado de concurrentes de uma transformação, recebendo o indice da transformação
int atualiza_concur(int indice_transform)
{
    Task atual = tarefas;
    Task atual2 = tarefas;
    Cmd lim = lim_cmd;
    int concur = 0;
    while (lim != NULL && lim->transform != indice_transform)
        lim = lim->prox;
    while (atual != NULL && atual->transform_atual != indice_transform)
    {
        if (atual->processamento == 1)
            concur++;
        atual = atual->prox;
    }
    for (; atual2 != NULL && atual2->transform_atual != indice_transform; atual2 = atual2->prox)
    { // do i need this second one???
        atual2->concur = concur;
        if (atual2->concur > lim->max)
        {
            printf("Processo nº %d ultrapassa máximo concurrentes permitidas", indice_transform);
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

// Verifica se outra transformação pode ser processada (verificando a concur e max_concur)
int disponivel(int indice_transform)
{
    Task atual = tarefas;
    Cmd lim = lim_cmd;
    while (atual->transform_atual != indice_transform)
        atual = atual->prox;
    while (lim->transform != indice_transform)
        lim = lim->prox;
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

// funcao que lê a lista de comandos e max de concurrencias e põe na struct secundária 'comandos'
Cmd limite_comandos(char *path)
{
    int fd;
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        perror("Erro ao abrir ficheiro");
        _exit(-1);
    }
    lim_cmd = NULL;
    Cmd temp = NULL;
    Cmd ant = NULL;

    char *token;
    while (readln(fd, *line, 1024) > 0)
    {
        temp = malloc(sizeof(struct comandos));
        if (lim_cmd == NULL)
            lim_cmd = temp;
        else
            ant->prox = temp;

        token = strtok(*line, " ");
        if (temp->transform = indice_trans(token) == -1)
            perror("erro indice_trans");
        token = strtok(NULL, " ");
        temp->max = atoi(token);
        temp->prox = NULL;
        ant = temp;
    }
    free(temp);
    free(ant);
    return lim_cmd;
}
int tamanho_array(int *a)
{
    int arr = sizeof(a);
    int res = arr / sizeof(int);
    return res;
}
// traduz uma lista de comandos para uma lista de int correspondentes ao indice das transformacoes
int *lista_trans(char *comando)
{
    char *token;
    int *res = malloc(sizeof(int) * 30);
    int i = 0, j;
    token = strtok(comando, " ");
    while (token != NULL)
    {
        if ((j = indice_trans(token)) != 0)
        {
            res[i] = j;
            i++;
        }
        token = strtok(NULL, " ");
    }
    if (realloc(res, sizeof(int) * i) == NULL)
        perror("realloc_lista_trans");
    return res;
}
// Para reverter a lista de transformacoes de modo a recuperar um ficheiro
int *inverter_array(int s[])
{
    int i, j, c;
    for (i = 0, j = tamanho_array(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
    return s;
}
int transforma(char *comando_cliente,char* path_trans)
{
    int pid_pai=getppid();
    int pid=getpid();
    char *comando_tmp = malloc(sizeof(comando_cliente)); // comando_cliente já deve estar preparado pelo client
    char *token;
    comando_tmp = comando_cliente;
    strtok(comando_cliente, " ");
    while (token != NULL)
    {

        switch (indice_trans(token))
        {
        case 0: //nop (por implementar)
        case 1: //bcompress
        case 2: //bdecompress
        case 3: //gcompress
        case 4: //gdecompress
        case 5: //encrypt
        case 6: //decrypt
        default:
            perror("Erro transforma -> indice_trans");
            return -1;
        }
        token=strtok(NULL," ");
    }

    free(comando_tmp);
    free(token);
    return 1;
}
int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        write(1, "erro exec server\n", 18);
        return -1;
    }
    lim_cmd = limite_comandos(argv[1]);
    path_trans = argv[2];

    if (mkfifo("main", 0666) != 0)
    {
        perror("mkfifo main");
        return -1;
    }
    while (1)
    {
        int pipe = open("main", O_RDONLY), tamanho_comando = 0;
        char comando_cliente[MAX_BUFFER]; // buffer do comando do cliente
        while (read(pipe, comando_cliente, 1) > 0)
        {
            tamanho_comando++;
        }
        comando_cliente[tamanho_comando++] = '\0'; // terminar o comando com '\0' para o strtok posteriormente
    }
    return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <wait.h>


#define MAX_BUFFER 1024
#define DEBUG 1
#define TESTING_TIME 30

//Structs Definition
typedef struct Transform {
    char name[MAX_BUFFER];
    int nExecutes;
    int maxExecutes;
    int currentExecutes;
    struct Transform *prox;
} *TRANSFORM;

typedef struct Utilizacoes {
    char name[MAX_BUFFER];
    int utilizacoes;
    struct Utilizacoes *prox;
} *UTILIZACOES;

typedef struct Task {
    char *command;
    int id;
    int numcommands; // numero de commands da tarefa
    int status; // 0 - a espera; 1 - a processar;
    char* pipeClient; //pid do cliente
    char* fifoClient;
    char *input_file; // ficheiro input
    char *output_file; //output
    int priority;
    struct Task *prox;
} *TAREFA;

//uteis 
void addTask(TAREFA t);
void executeTask(int input, int output, TAREFA t);
void closeSonTask(TAREFA t);

char process_pid[MAX_BUFFER], *path_Transforms;
TRANSFORM transforms;
TAREFA tasks;

//Auxes 
void reverse(char s[]) {
    int i, j; char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

//Rooobadooo
void itoa(int n, char s[]){
    int i, sign;
    if ((sign = n) < 0) n = -n;          
    i = 0;
    do { s[i++] = n % 10 + '0';} 
    while ((n /= 10) > 0); 

    if (sign < 0) s[i++] = '-';

    s[i] = '\0';
    reverse(s);
}

int writeToClient(char* namePipe, char* message){
    int pipeClient = open(namePipe, O_WRONLY);

    if(pipeClient < 0) {
        perror("Error trying to open client pipe");
        write(1, namePipe, strlen(namePipe));
        _exit(0);
    }

    int written = write(pipeClient, message, strlen(message)*sizeof(char));

    close(pipeClient);
    return written;
}

char* readFromClient(char* namePipe){
    char response[MAX_BUFFER];
    int pipeClient = open(namePipe, O_RDONLY), res = 0;

    if(pipeClient < 0){
        perror("Error while openning");
        _exit(0);
    }

    while(read(pipeClient, response+res, 1) > 0){
        res++;
    }

    close(pipeClient);

    return strdup(response);
}

int reachedMax(char* Transform, int nTransforms){
    TRANSFORM t = transforms;
    int entrou = 0;
    while(t != NULL){
        if(strcmp(t->name, Transform) == 0){
            entrou+=1;
            if(nTransforms > t->maxExecutes) return 1;
        }
        t=t->prox;
    }
    if(entrou > 0) return 0;
    return 1;
}

int nTransforms(){
    TRANSFORM current = transforms;
    int count = 0;

    while(current != NULL){
        count += 1;
        current = current->prox;
    }

    return count;
}

void writeTerminal(char* message){
    write(1, message, strlen(message));
}

int maxTransformSimultaneas(){
    TRANSFORM current = transforms;
    int count = 0;
    while(current != NULL){
        count += current->maxExecutes;
        current = current->prox;
    }
    return count;
}

ssize_t readln(int fd, char *line, ssize_t size) {
    ssize_t res = 0;
    ssize_t i = 0;
    while ((res = read(fd, &line[i], size)) > 0 && ((char) line[i] != '\n')) {
        i+=res;
    }
    return i;
}

TRANSFORM loadTransforms(char* config){
    char line[MAX_BUFFER], *token; int i = 0, configFile;
    TRANSFORM main = NULL, current = NULL, previous = NULL;

    if ((configFile = open(config, O_RDONLY)) < 0) {
        perror("Error while opening File");
        _exit(-1);
    }

    i = 0;
    while (readln(configFile, line, 1) > 0) {
        current = malloc(sizeof(struct Transform));
        if (i == 0) main = current;
        else previous->prox = current;

        token = strtok(line, " ");
        strcpy(current->name, token);
        token = strtok(NULL, " ");
        current->maxExecutes = atoi(token);

        current->currentExecutes = 0;
        current->nExecutes = 0;
        current->prox = NULL;
        previous = current;
        i++;
    }
    return main;
}

UTILIZACOES initializeUti(char* command){
    UTILIZACOES initial = NULL; int first = 1, found = 0; //inicializem sempre o found para nao dar merda
    char* token = malloc(sizeof(char)*MAX_BUFFER), *tmp = malloc(sizeof(char) * MAX_BUFFER);
    
    strcpy(tmp, command);
    token = strtok(tmp, " ");

    while(token != NULL){
        if(first == 1){
            initial = malloc(sizeof(UTILIZACOES));
            initial->prox = NULL,
            initial->utilizacoes = 1;
            strcpy(initial->name, token);
            first = 0;
            token = strtok(NULL, " ");
            continue;
        }
        found = 0;
        UTILIZACOES temp = initial;
        while(temp->prox != NULL){
            if(strcmp(temp->name, token) == 0){
                temp->utilizacoes++;
                found = 1;
            }
            temp = temp->prox;
        }
        if(strcmp(temp->name, token) == 0){
            temp->utilizacoes++;
            found = 1;
        }
        if(found == 0){
            temp->prox = malloc(sizeof(UTILIZACOES));
            temp->prox->prox = NULL;
            temp->prox->utilizacoes = 1;
            strcpy(temp->prox->name, token);

        }
        token = strtok(NULL, " ");
    }
    free(token);
    free(tmp);
    return initial;
}

int getProxID(){
    TAREFA t = tasks;
    if(t == NULL)return 1;
    while(t->prox != NULL) t=t->prox;
    return t->id+1;
}

char* getFifoFromPID(char* pid){
    char* nameFicheiro = malloc(sizeof(char)*(9+strlen(pid)));
    strcpy(nameFicheiro, "tmp/pipe_");
    strcpy(nameFicheiro+9, pid);
    return nameFicheiro;
}

//Suposto receber uma TASK(REVER ESTA FUNÇÃO)
void updateExecTransf(TAREFA t, int incrementaOuDecrementa){
    char* token = malloc(sizeof(char)*MAX_BUFFER);
    char* token_initial = token, *tmp = malloc(sizeof(char) * MAX_BUFFER);
  
    strcpy(tmp, t->command);
    token = strtok(tmp, " ");

    while(token != NULL){
        TRANSFORM current = transforms;
        while(current != NULL){
            if(strcmp(current->name, token) == 0){
                if(incrementaOuDecrementa) current->nExecutes++;
                else current->nExecutes--;
            }
            current = current->prox;
        }
        token = strtok(NULL, " ");
    }
    t->status = incrementaOuDecrementa;
    free(tmp);
    free(token_initial);
}


//Finalmente, Habemos o bichu crl
void workingEngine(){
    TAREFA tarefas = tasks;
    while(tarefas != NULL){
        if(tarefas->status == 1) {
            tarefas = tarefas->prox;
            continue;
        }
        updateExecTransf(tarefas, 1);
        if(fork() == 0){
            writeToClient(tarefas->fifoClient, "(processing)\n");
            sleep(1);
            if(fork() == 0){
                int input = open(tarefas->input_file, O_RDONLY);
                int output = open(tarefas->output_file, O_CREAT | O_TRUNC | O_WRONLY, 0777);
                if(DEBUG) sleep(TESTING_TIME);
                executeTask(input, output, tarefas);
                _exit(1);
            }else{
                int status;
                wait(&status);
                kill(atoi(process_pid), SIGUSR1);
                closeSonTask(tarefas);
            }
            _exit(1);
        }
        tarefas = tarefas->prox;
    }
}

void initializeTarefa(char* clientPID, int priority, char* inputFile, char* outputfile, char* command){
    TAREFA new = malloc(sizeof (struct Task));
    new->pipeClient = malloc(sizeof(char)*strlen(clientPID));
    new->command = malloc(sizeof(char)*strlen(command));
    new->input_file = malloc(sizeof(char)*strlen(inputFile));
    new->output_file = malloc(sizeof(char)*strlen(outputfile));
    new->fifoClient = malloc(sizeof(char)* strlen(clientPID)+9);
    new->status = 0;
    new->priority = priority;
    new->id = getProxID();
    int i = 0, numCommands = 0, surpassedMaximum = 0;

    if(DEBUG) {
        writeTerminal("New tarefa -> ID: ");
        char id[MAX_BUFFER];
        itoa(new->id, id);
        i = 0;
        while (id[i] != '\0') write(1, id+(i++), 1);
        writeTerminal("\n");
    }
    new->prox = NULL;

    strcpy(new->pipeClient, clientPID);
    strcpy(new->output_file, outputfile);
    strcpy(new->input_file, inputFile);
    strcpy(new->command, command);
    strcpy(new->fifoClient, getFifoFromPID(clientPID));

    UTILIZACOES uti = initializeUti(command);

    while(uti != NULL){
        numCommands+=uti->utilizacoes;
        if(reachedMax(uti->name, uti->utilizacoes))surpassedMaximum = 1;
        uti = uti->prox;
    }

    new->numcommands = numCommands;

    if(!surpassedMaximum) {
        addTask(new);
        workingEngine();
    }else{
        writeToClient(new->fifoClient, "Max concurrent transforms reached or you have inexistent transforms");
        sleep(1);
        kill(atoi(clientPID), 1);
    }
}

void addTask(TAREFA t){
    TAREFA backup = tasks;
    if(tasks == NULL) tasks = t;
    else if(backup->prox == NULL) {
        if(backup->priority < t->priority){
            tasks = t;
            t->prox = backup;
        }else{
            backup->prox = t;
        }
    }else{
        TAREFA temp = tasks;
        while(temp->prox != NULL && t->priority > temp->prox->priority)temp = temp->prox;
        temp->prox = t;
    }
}

char* createCommand(char* executable, char* command){
    int len = strlen(path_Transforms);
    strcpy(executable, path_Transforms);
    executable[len] = '/';
    strcpy(executable+len+1, command);
    return executable;
}

void writeToFile(char* message){
    int fd = open("mensagens.txt", O_CREAT | O_WRONLY | O_APPEND, 0777);
    write(fd, message, strlen(message));
    close(fd);
}

void executeTask(int input, int output, TAREFA t) {
    signal(SIGCHLD, SIG_DFL);
    int i, status, x, pip[2], f; char **command_order, *tmp = strdup(t->command), *token = strdup(t->command);

    token = strtok(tmp, " ");
    command_order = malloc(sizeof(char *) * t->numcommands);
    for (int i = 0; i < t->numcommands; i++) {
        command_order[i] = malloc(sizeof(char) * strlen(token));
        strcpy(command_order[i], token);
        token = strtok(NULL, " ");
    }
    x = t->numcommands;

    while (i < x) {
        if (i != 0) {
            dup2(pip[0], 0);
            close(pip[0]);
        } else dup2(input, 0);
        if (i == x-1) dup2(output, 1);
        else {
            if (pipe(pip) == 0) {
                dup2(pip[1], 1);
                close(pip[1]);
            } else {
                perror("Pipe");
                _exit(-1);
            }
        }
        if ((f = fork()) == -1) {
            perror("Fork");
            _exit(-1);
        } else if (f == 0) {
            char *executable = malloc(sizeof(char) * MAX_BUFFER);
            executable = createCommand(executable, command_order[i]);
            execlp(executable, executable, NULL);
            perror("Exec");
            _exit(-1);
        }
        i++;
    }
    for(int i = 0; i<x; i++) wait(&status);
}


void term_handler() {
    char pid[MAX_BUFFER];
    itoa(getpid(), pid);
    _exit(0);
}

void status(char* fPipeClient) {
    int f = fork();

    if (f == 0) {
        int pipeClient = open(fPipeClient, O_WRONLY);
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);

        if(pipeClient < 0){
            perror("Error");
            _exit(0);
        }
        TAREFA iterator = tasks;
        while (iterator != NULL && iterator->status == 1) {
            write(pipeClient, "task #", 6);
            char num[MAX_BUFFER];
            itoa(iterator->id, num);
            write(pipeClient, num, strlen(num));
            write(pipeClient, ": ", 2);
            write(pipeClient, iterator->command, strlen(iterator->command));
            write(pipeClient, "\n", 1);
            iterator = iterator->prox;
        }
        TRANSFORM it = transforms;
        while (it != NULL) {
            write(pipeClient, "Transformation ", 16);
            write(pipeClient, it->name, strlen(it->name));
            write(pipeClient, ": ", 2);

            char num[MAX_BUFFER];
            itoa(it->nExecutes,num);
            write(pipeClient, num, strlen(num));
            write(pipeClient, "/", 1);
            itoa(it->maxExecutes, num);
            write(pipeClient, num, strlen(num));
            write(pipeClient, " (running/max)\n", 15);
            it = it->prox;
        }
        char num[MAX_BUFFER];
        itoa(getppid(),num);
        write(pipeClient, "server pid : ", 14);
        write(pipeClient, num, strlen(num));
        write(pipeClient, "\n", 1);
        close(pipeClient);
        _exit(0);
    }
}

void finalizeFork(){
    unlink("tmp/servidor");
    while (tasks != NULL) pause();
    write(1,"\n",1);
    _exit(0);
}

void closeSonTask(TAREFA t){
    if (mkfifo("tmp/fecharTarefa", 0666) == 0) {
        //kill(t->pid, SIGUSR2); //remover isto antes de enviar
        char num[MAX_BUFFER];
        itoa(t->id, num);

        int pipe = open("tmp/fecharTarefa", O_WRONLY);
        int i = 0;
        while (num[i] != '\0') write(pipe, num+(i++), 1);
        close(pipe);
        _exit(0);
    } else {
        sleep(1);
        //closeSonTask(t);//isto tambem
        _exit(-1);
    }
}

void transforma(char* clientPID, char *args){
    char* complete_command = malloc(sizeof(char)*strlen(args));
    strcpy(complete_command, args);

    //Var declaradas em cima e definidas abaixo
    //Vai dar Segfault, porque ? dunno mas nao façam de outra maneira
    int priority = 0, nRemove = 0;
    char *token = strtok(args, " ");

    nRemove += strlen(token);
    token = strtok(NULL, " ");
    nRemove += strlen(token);
    priority = atoi(token);
    token = strtok(NULL, " "); // INPUT file
    char *input_file = malloc(sizeof(char) * strlen(token));
    strcpy(input_file, token);
    nRemove += strlen(token);

    token = strtok(NULL, " "); // OUTPUT file
    char *output_file = malloc(sizeof(char) * strlen(token));
    strcpy(output_file, token);
    nRemove += strlen(token);

    char* complete_command_copy = malloc(sizeof(char)*strlen(complete_command));
    strcpy(complete_command_copy, complete_command);
    complete_command_copy += nRemove+4; // 4 -> num de espacos
    initializeTarefa(clientPID, priority, input_file, output_file, complete_command_copy);
}

void removeTarefa(int id, int warnPipe){
    TAREFA tarefa = tasks;

    if(tarefa != NULL && tarefa->prox == NULL && tarefa->id == id){
        if(warnPipe){
            if(fork() == 0){
                kill(atoi(tarefa->pipeClient), SIGUSR2);
                _exit(0);
            }
        }
        updateExecTransf(tarefa, 0);
        tasks = NULL;
    }
    else if(tarefa != NULL && tarefa->prox != NULL){
        TAREFA previous = tarefa;
        while(tarefa != NULL){
            if(tarefa->id == id){
                if(warnPipe){
                    if(fork() == 0){
                        kill(atoi(tarefa->pipeClient), SIGUSR2);
                        _exit(0);
                    }
                }
                updateExecTransf(tarefa, 0);
                previous->prox = tarefa->prox;
            }
            tarefa = tarefa->prox;

        }
    }

}

void updateTarefa(int sigin){
    if(DEBUG) write(1, "Received update da Tarefa.\n", 35);
    int pipeFechar = open("tmp/fecharTarefa", O_RDONLY), res, num;

    if(pipeFechar > 0){
        char num_string[MAX_BUFFER];
        res = 0;
        while (read(pipeFechar,num_string+res,1) > 0) {res++;}

        if(DEBUG) {
            writeTerminal("Closing tarefa ");
            writeTerminal(num_string);
            writeTerminal("\n");
        }
        num = atoi(num_string);
        removeTarefa(num, 1);
        close(pipeFechar);
        unlink("tmp/fecharTarefa");
    }
}

int main(int argc, char **argv) {
    unlink("tmp/servidor");
    unlink("tmp/fecharTarefa");
    itoa(getpid(), process_pid);

    //initializeUti("nop nop nop nop nop boda nop");

    if (argc != 3) {
        write(1,"./sdstored config-filename transformations-folder\n", 49);
        return -1;
    }

    path_Transforms = malloc(sizeof(char)*strlen(argv[2]));
    strcpy(path_Transforms, argv[2]);
    transforms = loadTransforms(argv[1]);

    if(mkfifo("tmp/servidor", 0666) != 0){
        perror("Error creating initial fifo a fifo initial");
        return -1;
    }
    if (signal(SIGUSR1, updateTarefa) || signal(SIGTERM, term_handler) || signal(SIGINT, term_handler) || signal(SIGCHLD, SIG_IGN)) {
        perror("Signal");
        _exit(-1);
    }
    if(DEBUG){
        write(1, "Servidor iniciado com o PID: ", 29);
        write(1, process_pid, strlen(process_pid));
        write(1, "\n", 1);
    }
    while(1) {
        int pipeServidor = open("tmp/servidor", O_RDONLY) , res = 0;
        char clientPID[MAX_BUFFER];
        char buff_cliente[MAX_BUFFER];

      //  int res = 0;
        while (read(pipeServidor, clientPID+res,1) > 0)  
            res++;
        
        clientPID[res++] = '\0';
        close(pipeServidor);

        char fPipeClient[9+strlen(clientPID)];
        strcpy(fPipeClient, strdup("tmp/pipe_"));
        strcpy(fPipeClient+9, clientPID);

        int pipeClient = open(fPipeClient, O_RDONLY);
        res = 0;
        while (read(pipeClient, buff_cliente+res,1) > 0){
            res++;
        }
        buff_cliente[res] = '\0';
        if(DEBUG){
            write(STDOUT_FILENO, buff_cliente, strlen(buff_cliente));
            write(1, "\n", 1);
        }
        int validOption = 0;
        if(strcmp(buff_cliente, "status") == 0){
            if(DEBUG){
                write(STDOUT_FILENO, "O cliente ", 10);
                write(STDOUT_FILENO, clientPID, strlen(clientPID));
                write(STDOUT_FILENO, " quer status\n", 14);
            }
            validOption = 1;
            status(fPipeClient);
        }
        if(buff_cliente[0] == 'p' && buff_cliente[1] == 'r' && buff_cliente[2] == 'o' && buff_cliente[3] == 'c' &&
           buff_cliente[4] == '-' && buff_cliente[5] == 'f' && buff_cliente[6] == 'i' && buff_cliente[7] == 'l' &&
           buff_cliente[8] == 'e'){

            validOption = 1;
            writeToClient(fPipeClient, "(pending)\n");
            transforma(clientPID, buff_cliente);
        }
        if(validOption == 0){
            writeToClient(fPipeClient, "No valid option found, try later!\n");
            int pid_c = atoi(clientPID);
            if(fork() == 0){
                sleep(1);
                kill(pid_c, 1);
                _exit(0);
            }

        }
    }
    return 0;
}

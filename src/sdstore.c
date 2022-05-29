#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_BUFFER 1024
#define DEBUG 1

void closeFork();

char processPID[MAX_BUFFER];
char communicationFile[MAX_BUFFER];

void reverse(char s[]) {
    int i, j; char c;
    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

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

int writeToServer(char* nomePipe, char* message){
    int clientPipe = open(nomePipe, O_WRONLY), status;
    if(clientPipe < 0) perror("Opening error");
    status = write(clientPipe, message, strlen(message)*sizeof(char));
    close(clientPipe);
    return status;
}

char* readFromServer(char* nomePipe){
    char response[2000];
    int clientPipe = open(nomePipe, O_RDONLY), res = 0;

    for(int i = 0; i<2000; i++) response[i] = '\0';
    if(clientPipe < 0)perror("Opening error");
    
    res = 0;
    while(read(clientPipe, response+res, 1) > 0) res++;
    
    response[res+1] = '\0';
    close(clientPipe);
    return strdup(response);
}

void status(){
    writeToServer(communicationFile, "status");
    char* read = readFromServer(communicationFile);
    if(DEBUG) write(1, "Output do comando:\n", 19);
    write(1, read, strlen(read));

}

void startCommunication(){
    int mainPipe = open("tmp/servidor", O_WRONLY);

    if (mainPipe == -1) {
        perror("Error while opening pipe with server!");
        _exit(-1);
    }

    write(mainPipe, processPID,  strlen(processPID));
    close(mainPipe);

    char fclientPipe[9+strlen(processPID)];
    strcpy(fclientPipe, "tmp/pipe_");
    strcpy(fclientPipe+9, processPID);
    strcpy(communicationFile, fclientPipe);

    if(mkfifo(fclientPipe, 0666) != 0){
        perror("Cant create Pipe to communicate with server");
        _exit(-1);
    }
}

void transformation(int argc, char const *argv[]){
    int communication = open(communicationFile, O_WRONLY);

    if (communication == -1) {
        perror("Error while opening");
        unlink(communicationFile);
        _exit(-1);
    }
    for (int i = 1; i < argc; i++) {
        write(communication, argv[i], strlen(argv[i]));
        write(communication, " ", 1);
    }
    close(communication);
    char* message = readFromServer(communicationFile);
    while(1){
        write(1, message, strlen(message));
        message = readFromServer(communicationFile);

        if(strcmp(message, "(concluido)") == 0){
            write(1, "(concluido)\n", 12);
            unlink(communicationFile);
            exit(1);
        }
        usleep(300);
    }
}

void closeFork() {
    unlink(communicationFile);
    _exit(0);
}

void USR2_HANDLER(){
    write(1, "(concluido)\n", 12);
}

int main(int argc, char const *argv[]) {
    itoa(getpid(), processPID);

    if (signal(SIGINT, closeFork) == SIG_ERR) {
        unlink(communicationFile);
        perror("Signal1");
        _exit(-1);
    }
    if (signal(SIGTERM, closeFork) == SIG_ERR) {
        unlink(communicationFile);
        perror("Signal2");
        _exit(-1);
    }
    if (signal(SIGUSR2, USR2_HANDLER) == SIG_ERR) {
        unlink(communicationFile);
        perror("Signal2");
        _exit(-1);
    }

    if (argc >= 2 && !strcmp(argv[1], "status")) {
        startCommunication();
        status();
        unlink(communicationFile);
    } else if (argc >= 5 && !strcmp(argv[1], "proc-file")) {
        startCommunication();
        transformation(argc, argv);
    } else {
        write(1,"./sdstore status\n", 17);
        write(1,"./sdstore proc-file <priority> <input-filename> <output-filename> transformation-id-1 transformation-id-2 ...\n",105);
    }
    return 0;
}
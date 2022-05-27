#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>   /*chamadas ao sistema: defs e decls essenciais*/
#include <fcntl.h>    /*O_RDONLY, O_WRONLY, O_CREAT, O_* */

#include "sdstore.h"

//Responsavel por ignorar um ctrl+C no menu 
void IGNORA(int x){
	write(1, "\nsdstore$ ", 8);
}

//Responsavel por terminar o sdstore, quando é recebido um SIGQUIT
void SAI_CLIENTE(int x){
	write(1, "\n", 1);
	_exit(0);
}

//Gera nome para um fifo, de maneira aleatória
void get_Name_Fifo(char* nome, int tam){
	srand(time(NULL));
	for (int i = 0; i < (tam-1); ++i)
	{
		nome[i]= (char)(rand()%26) + 'A';
	}
	nome[tam] = '\0';
}
/*
//Responsavel por recuperar o output, e apresentar o mesmo no ecrã
void recover_Output(int pos){
	int d = open("log.idx",O_RDONLY); 
	int d1 = open("log.txt",O_RDONLY);
	char buffer[MAX];

	int nLinhas = 0;
	int encontrou = 0;
	char* temp;
	int n;

	while((n=read(d,buffer,MAX))>0){
		temp = strtok(buffer,"\n");
		nLinhas++;
		if(pos==0){
			encontrou = 1;
			break;
		}
		while(((temp = strtok(NULL,"\n"))!=NULL) && (nLinhas<pos)){
			nLinhas++;
		}

		if(nLinhas==pos && temp!=NULL){
			encontrou = 1;
			break;
		}
	}

	if(encontrou==1){
		int posFich = atoi(strtok(temp," "));
		int tamLer = atoi(strtok(NULL," "));

		int n2 = lseek(d1,posFich,SEEK_CUR);
		
		char bufferTemp[tamLer];
		n = read(d1,bufferTemp,tamLer);
		write(1,bufferTemp,n);
	}
	else{
		printf("Nao existe Output\n");
	}

}
*/
//Responsavel por pedir ao servidor, para executar um commando
void execute(char* command, int tam){
	char nameFIFO[10];
	get_Name_Fifo(nameFIFO,10);

	int opc = mkfifo(nameFIFO,0600);
	
	struct estruturaDadosFifo e;
	strcpy(e.opcao, "execute");
	strcpy(e.fifoResposta, nameFIFO);
	strcpy(e.command, command);

	int fifo = open("fifo1", O_WRONLY);
	write(fifo, &e, sizeof(struct estruturaDadosFifo));
	close(fifo);

	int d  = open("log.idx",O_WRONLY|O_CREAT|O_APPEND, 0600); 
	int d1 = open("log.txt",O_WRONLY|O_CREAT, 0600);
	int n2 = lseek(d1,0,SEEK_END);

	fifo = open(nameFIFO, O_RDONLY);

	int n;
	int nT = 0;
	char buffer[MAX];
	while((n=read(fifo,buffer,MAX))>0){
		nT += n;
		write(1,buffer,n);
		write(d1,buffer,n);
	}

	char resp[MAX];
	sprintf(resp,"%d %d\n",n2,nT);
	write(d,resp,strlen(resp));
	close(d);

	close(fifo);
	close(d1);
}

//Responsavel por mostar ajuda à utilização do programa
void printaAjuda(){
	//write(1, "tempo-inatividade(-i) segundos\n", 32);
	//write(1, "tempo-execucao(-m) segundos\n", 29);
	write(1, "execute(-e) p1 | p2 ...\n", 26);
	write(1, "listar(-l)\n", 12);
	write(1, "terminar(-t) numero\n", 21);
	write(1, "historico(-r)\n", 15);
	write(1, "output(-o) numero\n", 19);
	write(1, "Crtrl+| (SIGQUIT) para sair\n", 29);
}

//Responsavel por pedir ao servidor historico de execuçoes
void printHistorico(){
	char nameFIFO[10];
	get_Name_Fifo(nameFIFO,10);

	int opc = mkfifo(nameFIFO,0600);
	
	struct estruturaDadosFifo e;
	strcpy(e.opcao, "historico");
	strcpy(e.fifoResposta, nameFIFO);
	strcpy(e.command, "Non");

	int fifo = open("fifo1", O_WRONLY);
	write(fifo, &e, sizeof(struct estruturaDadosFifo));
	close(fifo);

	fifo = open(nameFIFO, O_RDONLY);

	int n;
	char buffer[MAX];
	while((n=read(fifo,buffer,MAX))>0){
		write(1,buffer,n);
	}

	close(fifo);
}

//Fresponsavel por defenir no servidor o tempo maximo de inatividade
/*void setTempoInatividade(char* tempo){
	struct estruturaDadosFifo e;
	strcpy(e.opcao, "tempo-inactividade");
	strcpy(e.fifoResposta, "Non");
	strcpy(e.command, tempo);

	int fifo = open("fifo1", O_WRONLY);

	write(fifo, &e, sizeof(struct estruturaDadosFifo));

	close(fifo);
}

//Funcao responsavel pr defenir no servidor o tempo maximo de execução
void setTempoExecucao(char* tempo){
	struct estruturaDadosFifo e;
	strcpy(e.opcao, "tempo-execucao");
	strcpy(e.fifoResposta, "Non");
	strcpy(e.command, tempo);

	int fifo = open("fifo1", O_WRONLY);

	write(fifo, &e, sizeof(struct estruturaDadosFifo));

	close(fifo);
}
*/
//Funcao responsavel por pedir ao servidor a lista de programas que estao a execute 
void list(){
	char nameFIFO[10];
	get_Name_Fifo(nameFIFO,10);

	int opc = mkfifo(nameFIFO,0600);
	
	struct estruturaDadosFifo e;
	strcpy(e.opcao, "list");
	strcpy(e.fifoResposta, nameFIFO);
	strcpy(e.command, "Non");

	int fifo = open("fifo1", O_WRONLY);
	write(fifo, &e, sizeof(struct estruturaDadosFifo));
	close(fifo);

	fifo = open(nameFIFO, O_RDONLY);

	int n;
	char buffer[MAX];
	while((n=read(fifo,buffer,MAX))>0){
		write(1,buffer,n);
	}

	close(fifo);
}

//Ressponsavel por terminar a instrucao a execute no servidor
void terminar(char* command){
	struct estruturaDadosFifo e;
	strcpy(e.opcao, "terminar");
	strcpy(e.fifoResposta, "Non");
	strcpy(e.command, command);

	int fifo = open("fifo1", O_WRONLY);
	write(fifo, &e, sizeof(struct estruturaDadosFifo));
	close(fifo);
}

//Funcao main que vai receber os commands e decidir o que fazer
int main(int argc, char const *argv[]){
	char* optExecute;
	char* command;

    if(argc <= 1){
    	while(1){
    		char buffer[MAX];

    		signal(SIGINT,IGNORA);
    		signal(SIGQUIT,SAI_CLIENTE);
    		
    		write(1, "\nsdstore$ ", 9);
    		read(0, buffer, MAX);
    		
    		char optExecute[MAX];
    		strcpy(optExecute,strtok(buffer," "));
    		int i;
    		for ( i = 0; i<strlen(optExecute); i++){
    			if(optExecute[i]=='\n')
    				optExecute[i] = '\0';
    		}

    		command = buffer + strlen(optExecute)+1;
    		
    		for ( i = 0; i<strlen(command); i++){
    			if(command[i]=='\n')
    				command[i] = '\0';
    		}

    		signal(SIGINT,SIG_IGN);
    		signal(SIGQUIT,SIG_IGN);
    		
    		/*if(strcmp(optExecute,"tempo-inactividade")==0){
    			setTempoInatividade(command);
    		}
    		
    		if(strcmp(optExecute,"tempo-execucao")==0){
    			setTempoExecucao(command);
    		}
    		*/
    		if(strcmp(optExecute,"execute")==0){
    			execute(command,strlen(command));
    		}
    		
    		if(strcmp(optExecute,"list")==0){
    			list();
    		}
    		
    		if(strcmp(optExecute,"terminar")==0){
    			terminar(command);
    		}
    		
    		if(strcmp(optExecute,"historico")==0){
    			printHistorico();
    		}
    		
    		if(strcmp(optExecute,"ajuda")==0){
    			printaAjuda();
    		}
    		
    		if(strcmp(optExecute,"output")==0){
    			int pos = atoi(command);
    			recuperaOutPut(pos);		
    		}
    	}
	}
	else

		optExecute = argv[1];
		
		signal(SIGINT,SIG_IGN);
    	signal(SIGQUIT,SIG_IGN);

		/*if(strcmp(optExecute,"-i")==0){
			setTempoInatividade(argv[2]);
		}

		if(strcmp(optExecute,"-m")==0){
			setTempoExecucao(argv[2]);
		}*/

		if(strcmp(optExecute,"-e")==0){
			command = argv[2];
			execute(command,strlen(command)+1);
		}

		if(strcmp(optExecute,"-l")==0){
			list();
		}

		if(strcmp(optExecute,"-t")==0){
			terminar(argv[2]);
		}

		if(strcmp(optExecute,"-r")==0){
			printHistorico();
		}

		if(strcmp(optExecute,"-h")==0){
			printaAjuda();
		}

		if(strcmp(optExecute,"-o")==0){
			int pos = atoi(argv[2]);
			recuperaOutPut(pos);		
		}
	return 0;
}

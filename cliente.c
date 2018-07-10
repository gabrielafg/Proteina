#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#define NUM_CLIENTE 1
#define TAM_PROTEINA 609

typedef struct {
    uint8_t size;
    char method;
    char payload[5];
} aatp_msg;

pthread_mutex_t mutex;
pthread_t thread_cliente[NUM_CLIENTE];
pthread_t thread_gerenciador;
int indice_thread[NUM_CLIENTE];

bool fim;
char proteina[TAM_PROTEINA + 1];
char proteina_info[75];
int indice_aminoacido;

void *thread_result;

void lerProteina();
void *executarCliente(void *indice);
void comunicarComServidor(int socket_remoto);
aatp_msg criarSolicitacao();
void *executarGerenciador(void *indice);

int main(){
	fim = false;
	
	lerProteina();
	indice_aminoacido = 0;
	/*printf("%s \n", proteina);
	printf("%s \n", proteina_info);*/
	
	pthread_mutex_init(&mutex, NULL);

	for(int i = 0; i<NUM_CLIENTE; i++){
		indice_thread[i] = i;
		(void)pthread_create(&thread_cliente[i], NULL, executarCliente, &indice_thread[i]);
	}
	(void)pthread_create(&thread_gerenciador, NULL, executarGerenciador, 0);
	
	for(int i = 0; i<NUM_CLIENTE; i++){
		(void)pthread_join(thread_cliente[i], &thread_result);
	}
	(void)pthread_join(thread_gerenciador, &thread_result);
	return 0;
}

// ---------------- Funções gerais

void lerProteina(){
	FILE *arquivo;
	arquivo = fopen("Serum albumin.fasta", "r");
	//fread(aminoacido, 20, 1, arquivo);
	fgets(proteina_info, sizeof(proteina_info), arquivo);
	fgets(proteina, sizeof(proteina), arquivo);
	fclose(arquivo);
}

// ---------------- Funções do cliente

void *executarCliente(void *indice){
	while(!fim){
		char *rem_hostname;
		int porta_remota, socket_remoto;
		struct sockaddr_in endereco_remoto; /* Estrutura: familia + endereco IP + porta */
		
		rem_hostname = "192.168.25.37";
		porta_remota = atoi("8000");
		endereco_remoto.sin_family = AF_INET; /* familia do protocolo*/
		endereco_remoto.sin_addr.s_addr = inet_addr(rem_hostname); /* endereco IP local */
		endereco_remoto.sin_port = htons(porta_remota); /* porta local  */
		
		socket_remoto = socket(AF_INET, SOCK_STREAM, 0);
		
		if (socket_remoto < 0) {
			perror("Criando stream socket");
			exit(1);
		}
		printf("> Conectando no servidor '%s:%d'\n", rem_hostname, porta_remota);
		
	   	/* Estabelece uma conexão remota */
		/* parametros(descritor socket, estrutura do endereco remoto, comprimento do endereco) */
		if (connect(socket_remoto, (struct sockaddr *) &endereco_remoto, sizeof(endereco_remoto)) < 0) {
			perror("Conectando stream socket");
			exit(1);
		}
		
		comunicarComServidor(socket_remoto);			
		close(socket_remoto);
	}
}

void comunicarComServidor(int socket_remoto){
	aatp_msg solicitacao = { 0 };
	solicitacao = criarSolicitacao();
	printf("Cliente - Solicitei: tam - %d; %c; %s\n", solicitacao.size, solicitacao.method, solicitacao.payload);
	send(socket_remoto, &solicitacao, sizeof(solicitacao), 0);
	
	aatp_msg resposta;
	size_t tam_resposta = sizeof(resposta);
	recv(socket_remoto, &resposta, tam_resposta, 0);
	printf("Cliente - Recebi: tam - %d; %c; %s\n", resposta.size, resposta.method, resposta.payload);
}

aatp_msg criarSolicitacao(){
	aatp_msg solicitacao = { 0 };
	solicitacao.method = 'S';
	solicitacao.size = rand()%5 + 1;
	memset(&solicitacao.payload, 0, sizeof solicitacao.payload);
	return solicitacao;
}

// ---------------- Funções do gerenciador
void *executarGerenciador(void *indice){
	
}


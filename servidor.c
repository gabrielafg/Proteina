#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

typedef struct {
    uint8_t size;
    char method;
    char payload[5];
} aatp_msg;

bool fim;
char aminoacido[20];

void lerAminoacidos();
void conectarComCliente(int socket_original, struct sockaddr_in endereco);
aatp_msg criarResposta(uint8_t quantidade);

int main(int argc, char *argv[]){
	fim = false;
	
	lerAminoacidos();
	printf("%s\n\n", aminoacido);
		
	int descritor_socket;
	struct sockaddr_in endereco, endereco_cliente;
	
	if (argc != 2) {
		printf("Parametros: <local_port> \n");
		exit(1);
	}
	
   	/* Cria o socket para enviar e receber datagramas - parametros(familia, tipo, protocolo) */
	descritor_socket = socket(AF_INET, SOCK_STREAM, 0);	
	
	if (descritor_socket < 0) {
		perror("Criando stream socket");
		exit(1);
	}

	endereco.sin_family = AF_INET; //familia do protocolo
	endereco.sin_addr.s_addr = INADDR_ANY; //endereco IP local
	endereco.sin_port = htons(atoi(argv[1])); //porta local
	bzero(&(endereco.sin_zero), 8);

   	/* Bind para o endereco local*/
	if (bind(descritor_socket, (struct sockaddr *) &endereco, sizeof(struct sockaddr)) < 0) {
		perror("Ligando stream socket");
		exit(1);
	}
	
	while(!fim){
		listen(descritor_socket, 5);
		/* parametros(descritor socket, numeros de conexões em espera sem serem aceitas pelo accept)*/
		conectarComCliente(descritor_socket, endereco);
		printf("Fechando conexão.\n");
		sleep(2);
	}
	close(descritor_socket);
	
	return 0;
}

void lerAminoacidos(){
	FILE *arquivo;
    arquivo = fopen("aminoacidos.txt", "r");
    fread(aminoacido, 20, 1, arquivo);
    fclose(arquivo);
}

void conectarComCliente(int socket_original, struct sockaddr_in endereco){
	printf("> Aguardando conexão\n");
	
	struct sockaddr_in endereco_cliente;
	int tamanho = sizeof(struct sockaddr_in);
   	/* Aceita um pedido de conexao, devolve um novo "socket" ja ligado ao emissor do pedido e o "socket" original*/
	int novo_socket = accept(socket_original, (struct sockaddr *)&endereco_cliente, &tamanho);

	aatp_msg solicitacao = { 0 };
	size_t tam_solicitacao = sizeof(solicitacao);

	recv(novo_socket, &solicitacao, tam_solicitacao, 0);
	printf("Servidor - Recebi: tam - %d; %c; %s\n", solicitacao.size, solicitacao.method, solicitacao.payload);
	
	aatp_msg resposta = { 0 };
	resposta = criarResposta(solicitacao.size);
	
	send(novo_socket, &resposta, sizeof(resposta), 0);
	printf("Servidor - Enviei: tam - %d; %c; %s\n", resposta.size, resposta.method, resposta.payload);
	
	close(novo_socket);
}

aatp_msg criarResposta(uint8_t quantidade){
	aatp_msg resposta = { 0 };
	resposta.method = 'R';
	resposta.size = quantidade;
	memset(&resposta.payload, 0, sizeof resposta.payload);
	for(int i = 0; i<quantidade; i++){
		int num = rand()%20;
		resposta.payload[i] = aminoacido[num];
	}
	return resposta;
}

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
	
	descritor_socket = socket(AF_INET, SOCK_STREAM, 0);	
	
	if (descritor_socket < 0) {
		perror("Criando stream socket");
		exit(1);
	}

	endereco.sin_family = AF_INET;
	endereco.sin_addr.s_addr = INADDR_ANY;
	endereco.sin_port = htons(atoi(argv[1]));
	bzero(&(endereco.sin_zero), 8);
	
	if (bind(descritor_socket, (struct sockaddr *) &endereco, sizeof(struct sockaddr)) < 0) {
		perror("Ligando stream socket");
		exit(1);
	}
	
	while(!fim){
		listen(descritor_socket, 5);
		conectarComCliente(descritor_socket, endereco);
		printf(">>> Fechando conexão.\n\n");
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
	printf(">>> Aguardando conexão\n");
	int read_size;
	
	struct sockaddr_in endereco_cliente;
	int tamanho = sizeof(struct sockaddr_in);
	int novo_socket = accept(socket_original, (struct sockaddr *)&endereco_cliente, &tamanho);

	aatp_msg solicitacao = { 0 };
	size_t tam_solicitacao = sizeof(solicitacao);

	while((read_size = recv(novo_socket, &solicitacao, tam_solicitacao, 0) > 0)){
		printf("Servidor - Recebi: quantidade - %d; método - %c; payload - %s\n", solicitacao.size, solicitacao.method, solicitacao.payload);
	
		aatp_msg resposta = { 0 };
		resposta = criarResposta(solicitacao.size);
	
		send(novo_socket, &resposta, sizeof(resposta), 0);
		printf("Servidor - Enviei: quantidade - %d; método - %c; payload - %s\n\n", resposta.size, resposta.method, resposta.payload);
	}
	
	close(novo_socket);
}

aatp_msg criarResposta(uint8_t quantidade){
	aatp_msg resposta = { 0 };
	resposta.method = 'R';
	resposta.size = quantidade;
	memset(&resposta.payload, 0, sizeof resposta.payload);
	int i = 0;
	for(i = 0; i<quantidade; i++){
		int aa = rand()%20;
		resposta.payload[i] = aminoacido[aa];
	}
	return resposta;
}

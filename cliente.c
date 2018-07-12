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

#define NUM_CLIENTE 2
#define TAM_PROTEINA 609
#define NUM_IPS 1
#define REFERENCIA 1
#define NOVA 2

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
char nova_proteina[TAM_PROTEINA];
char proteina_auxiliar[TAM_PROTEINA + 1];
char proteina_info[75];
char ip[NUM_IPS][15];
int progresso;
aatp_msg pacote_recebido[NUM_CLIENTE];

void *thread_result;

void lerProteina(int tipo);
void criarNovaProteina();
void lerIPs();
void *executarCliente(void *indice);
int estabelecerConexao();
void comunicarComServidor(int id, int socket_remoto);
aatp_msg criarSolicitacao();
void verificarPacote(int id);
void armazenarAminoacido();
void *executarGerenciador();

int main(){
	fim = false;
	progresso = 0;
	
	lerProteina(REFERENCIA);
	criarNovaProteina();
	lerIPs();
	
	pthread_mutex_init(&mutex, NULL);

	for(int i = 0; i < NUM_CLIENTE; i++){
		indice_thread[i] = i;
		(void)pthread_create(&thread_cliente[i], NULL, executarCliente, &indice_thread[i]);
	}
	(void)pthread_create(&thread_gerenciador, NULL, executarGerenciador, 0);
	
	for(int i = 0; i < NUM_CLIENTE; i++){
		(void)pthread_join(thread_cliente[i], &thread_result);
	}
	(void)pthread_join(thread_gerenciador, &thread_result);
	return 0;
}

// ---------------- Funções gerais

void lerProteina(int tipo){
	if(tipo == REFERENCIA){
		FILE *arquivo;
		arquivo = fopen("Serum albumin.fasta", "r");
		fgets(proteina_info, sizeof(proteina_info), arquivo);
		fgets(proteina, sizeof(proteina), arquivo);
		fclose(arquivo);
	}else
		if(tipo == NOVA){
			FILE *arquivo;
			arquivo = fopen("nova_proteina.txt", "r");
			fgets(proteina_auxiliar, sizeof(proteina_auxiliar), arquivo);
			fclose(arquivo);
		}	
}

void criarNovaProteina(){
	FILE *arq;
	arq = fopen("nova_proteina.txt", "wb");
	if(arq){
		printf("Arquvio criado: nova_proteina.txt");
		memset(&nova_proteina, '_', sizeof(nova_proteina));
		fputs(nova_proteina, arq);
		fclose(arq);
	}else{
		printf("Erro ao criar arquivo.\n");
		exit(1);
	}
}

void lerIPs(){
	FILE *arquivo;
	arquivo = fopen("IPs.txt", "r");
	for(int i = 0; i < NUM_IPS; i++){
		fgets(ip[i], sizeof(ip[i]), arquivo);
	}
	fclose(arquivo);
}

// ---------------- Funções do cliente

void *executarCliente(void *indice){
	int id = *(int*) indice;
	while(!fim){
		int socket_remoto;
		socket_remoto = estabelecerConexao();
		comunicarComServidor(id, socket_remoto);			
		close(socket_remoto);
		
		bool verificado = false;
		while(!verificado){
			pthread_mutex_lock(&mutex);
			verificarPacote(id);
			verificado = true;
			pthread_mutex_unlock(&mutex);
		}
	}
}

int estabelecerConexao(){
	char *rem_hostname;
	int porta_remota, socket_remoto;
	struct sockaddr_in endereco_remoto; /* Estrutura: familia + endereco IP + porta */
	
	rem_hostname = ip[rand()%NUM_IPS];
	porta_remota = atoi("1339");
	endereco_remoto.sin_family = AF_INET; /* familia do protocolo*/
	endereco_remoto.sin_addr.s_addr = inet_addr(rem_hostname); /* endereco IP local */
	endereco_remoto.sin_port = htons(porta_remota); /* porta local  */
	
	socket_remoto = socket(AF_INET, SOCK_STREAM, 0);
	
	if (socket_remoto < 0) {
		perror("Criando stream socket");
		exit(1);
	}
	printf("> Conectando no servidor '%s:%d'\n", rem_hostname, porta_remota);
	
	if (connect(socket_remoto, (struct sockaddr *) &endereco_remoto, sizeof(endereco_remoto)) < 0) {
		perror("Conectando stream socket");
		//exit(1);
	}
	return socket_remoto;
}

void comunicarComServidor(int id, int socket_remoto){
	aatp_msg solicitacao = { 0 };
	solicitacao = criarSolicitacao();
	printf("Cliente %d - Solicitei: %d aminoácido(s)\n", id, solicitacao.size);
	send(socket_remoto, &solicitacao, sizeof(solicitacao), 0);
	
	aatp_msg resposta;
	size_t tam_resposta = sizeof(resposta);
	recv(socket_remoto, &resposta, tam_resposta, 0);
	printf("Cliente %d - Recebi: %s\n\n", id, resposta.payload);
	pacote_recebido[id] = resposta;
}

aatp_msg criarSolicitacao(){
	aatp_msg solicitacao = { 0 };
	solicitacao.method = 'S';
	solicitacao.size = rand()%5 + 1;
	memset(&solicitacao.payload, 0, sizeof solicitacao.payload);
	return solicitacao;
}

void verificarPacote(int id){
	if(toupper(pacote_recebido[id].method) == 'R'){
		int quantidade = pacote_recebido[id].size;
		char sequencia[quantidade];
		char aa[quantidade];
		int aa_aceito = 0;
	
		lerProteina(NOVA);
	
		for(int i = 0; i < quantidade; i++){
			sequencia[i] = toupper(pacote_recebido[id].payload[i]);
		}
		for(int i = 0; i < TAM_PROTEINA && aa_aceito < quantidade; i++){
			for(int j = 0; j < quantidade; j++){
				if((proteina[i] == sequencia[j]) && (proteina_auxiliar[i] == '_')){
					proteina_auxiliar[i] = sequencia[j];
					sequencia[j] = '*';
					aa_aceito++;
					progresso++;
					if(progresso == TAM_PROTEINA)
						fim = true;
				}
			}
		}
		if(aa_aceito > 0){
			armazenarAminoacido();
		}
	}
}

void armazenarAminoacido(){
	char temp[TAM_PROTEINA + 1];
	FILE *arquivo;
	arquivo = fopen("nova_proteina.txt", "wr");
	fseek(arquivo, 0, SEEK_CUR);
	fprintf(arquivo, "%s", proteina_auxiliar);
	fseek(arquivo, 0, SEEK_CUR);
	fclose(arquivo);
}

// ---------------- Funções do gerenciador 
void *executarGerenciador(){
	while(!fim){
		system("clear");
		printf("Progresso: %d/%d \n", progresso, TAM_PROTEINA);
		printf("%s \n", proteina_info);
		printf("Sequência: %s \n", proteina_auxiliar);
		sleep(1);
	}
	printf("A SEQUÊNCIA ESTÁ COMPLETA!!!! \n\n");
}

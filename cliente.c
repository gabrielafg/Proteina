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

#define PORTA 1337
#define NUM_CLIENTE 2
#define TAM_PROTEINA 609
#define NUM_IPS 6
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
char nova_proteina[TAM_PROTEINA + 1];
char proteina_info[75];
char ip[NUM_IPS][15];
int progresso;
int pacotes;
int aminoacidos;
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
	pacotes = 0;
	aminoacidos = 0;
	
	lerProteina(REFERENCIA);
	criarNovaProteina();
	lerProteina(NOVA);
	lerIPs();
	
	pthread_mutex_init(&mutex, NULL);

	int i = 0;
	for(i = 0; i < NUM_CLIENTE; i++){
		indice_thread[i] = i;
		(void)pthread_create(&thread_cliente[i], NULL, executarCliente, &indice_thread[i]);
	}
	(void)pthread_create(&thread_gerenciador, NULL, executarGerenciador, 0);
	
	for(i = 0; i < NUM_CLIENTE; i++){
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
			fgets(nova_proteina, sizeof(nova_proteina), arquivo);
			fclose(arquivo);
		}
}

void criarNovaProteina(){
	FILE *arq;
	arq = fopen("nova_proteina.txt", "wr");
	if(arq){
		printf("Arquvio criado: nova_proteina.txt");
		char preenchimento[TAM_PROTEINA];
		memset(&preenchimento, '_', sizeof(preenchimento));
		fputs(preenchimento, arq);
		fclose(arq);
	}else{
		printf("Erro ao criar arquivo nova_proteina.txt.\n");
		exit(1);
	}
}

void lerIPs(){
	FILE *arquivo;
	arquivo = fopen("IPs.txt", "r");
	if(arquivo){
		int i = 0;
		for(i = 0; i < NUM_IPS; i++){
			fgets(ip[i], sizeof(ip[i]), arquivo);
		}
		fclose(arquivo);
	}else{
		printf("O arquivo de IPs não foi encontrado. \n\n");
		exit(1);
	}
}

// ---------------- Funções do cliente

void *executarCliente(void *indice){
	int id = *(int*) indice;
	while(!fim){
		int socket_remoto;
		socket_remoto = estabelecerConexao();
		if(socket_remoto >= 0){
			comunicarComServidor(id, socket_remoto);			
			close(socket_remoto);
		
			bool verificado = false;
			while(!verificado && !fim){
				pthread_mutex_lock(&mutex);
				verificarPacote(id);
				verificado = true;
				pthread_mutex_unlock(&mutex);
			}
		}else{
			usleep(100);
		}
	
	}

}

int estabelecerConexao(){
	char *servidor;
	int porta_remota, socket_remoto;
	struct sockaddr_in endereco_remoto; /* Estrutura: familia + endereco IP + porta */
	
	servidor = ip[rand()%NUM_IPS];
	porta_remota = PORTA;
	endereco_remoto.sin_family = AF_INET;
	endereco_remoto.sin_addr.s_addr = inet_addr(servidor); 
	endereco_remoto.sin_port = htons(porta_remota);
	
	socket_remoto = socket(AF_INET, SOCK_STREAM, 0);
	
	if (socket_remoto < 0) {
		//printf("Erro ao criar stream socket");
		return -1;
	}else{
		//printf("> Conectando no servidor '%s:%d'\n", servidor, porta_remota);
		int conexao = connect(socket_remoto, (struct sockaddr *) &endereco_remoto, sizeof(endereco_remoto));
		if (conexao < 0) {
			//printf("Erro ao conectar stream socket");
			return -1;
		}else{
			return socket_remoto;
		}
	}
}

void comunicarComServidor(int id, int socket_remoto){
	aatp_msg solicitacao = { 0 };
	solicitacao = criarSolicitacao();
	//printf("Cliente %d - Solicitei: %d aminoácido(s)\n", id, solicitacao.size);
	send(socket_remoto, &solicitacao, sizeof(solicitacao), 0);
	
	aatp_msg resposta;
	size_t tam_resposta = sizeof(resposta);
	recv(socket_remoto, &resposta, tam_resposta, 0);
	//printf("Cliente %d - Recebi: %s\n\n", id, resposta.payload);
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
		FILE *arquivo;
		arquivo = fopen("nova_proteina.txt", "r+");
		fgets(nova_proteina, sizeof(nova_proteina), arquivo);
		int quantidade = pacote_recebido[id].size;
		int aa_aceito = 0;
		char sequencia[quantidade];
		
		int i = 0;
		int j = 0;
		for(i = 0; i < quantidade; i++){
			sequencia[i] = toupper(pacote_recebido[id].payload[i]);
		}		
		
		for(i = 0; (i < TAM_PROTEINA) && (aa_aceito < quantidade); i++){
			for(j = 0; j < quantidade; j++){
				if((proteina[i] == sequencia[j]) && (nova_proteina[i] == '_')){
					fseek(arquivo, i, SEEK_SET);
					putc(sequencia[j], arquivo);
					nova_proteina[i] = sequencia[j];
					aa_aceito++;
					progresso++;
					if(progresso == TAM_PROTEINA)
						fim = true;
				}
			}
		}
		pacotes++;
		aminoacidos += quantidade;
		fclose(arquivo);
	}
}

// ---------------- Funções do gerenciador 
void *executarGerenciador(){
	while(!fim){
		system("clear");
		printf("%s \n", proteina_info);
		printf("Sequência: %s \n", nova_proteina);
		printf("Progresso: %d/%d \n\n", progresso, TAM_PROTEINA);
		sleep(1);
	}
	system("clear");
	printf("%s \n", proteina_info);
	printf("Sequência: %s \n", nova_proteina);
	printf("Progresso: %d/%d \n\n", progresso, TAM_PROTEINA);
	printf("A SEQUÊNCIA ESTÁ COMPLETA!!!! \n\n");
	printf("%d aminoácidos de %d pacotes foram recebidos.\n\n", aminoacidos, pacotes);
}

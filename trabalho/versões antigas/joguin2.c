#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<math.h>

/*               -- REGRAS DO JOGO --
- As threads são ações: ATAQUE / DEFESA / ESQUIVA? / DESARME?
- Vão rolando ações de jogadores aleatórios
    * Ataques contra outros jogadores (duram 0.25 segundos)
    * Defesas (duram 1 segundo)
    * Esquivas (duram 0.5 segundos)
    * Desarme (dura 0.25 segunodos)
- Cada jogador tem vida e dano (agilidade)
- 2 podem atacar um defensor, que se defende dos dois
*/

// PRÓXIMOS PASSOS - CRIAR TODAS AS THREADS DEPOIS INICIAR TODAS AS THREADS

#define A 3 // numero de threads de ataque POR JOGADOR
#define D 2 // numero de threads de defesa POR JOGADOR
// #define A_A 3 // quantos podem atacar ao mesmo tempo
#define D_A 2 // de quantos atcantes um defensor pode se defender ao mesmo tempo
#define P 5 // total de jogadores

long long int vida_maxima = 100;
long long int dano_maximo = 10;

// variaveis do problema
int atacando = 0; // contador de threads atacando
int defendendo = 0; // contador de threads defendendo
int totalRingue = 0; // contador de threads totais (por enquanto nao vou usar isso
                    // pq iria requerer uma entrada no ringue para estar disponível para ataque/defesa)
int estado[P]; // vetor com os estados dos jogadores: 0 = INATIVO // 1 = ATACANDO // 2 = EM FORMAÇÃO DE DEFESA
                    // 3 = DEFENDENDO DE 1 // 4 = DEFENDENDO DE 2

struct Jogador {
    int id_jogador;
    int vida;
    int dano;
};

// variaveis para sincronizacao
pthread_mutex_t mutex;
pthread_cond_t conds[P];

// entrada ataque
void IniciaAtaque (int id, int idAlvo) {
    pthread_mutex_lock(&mutex);
    while(estado[id] != 0) {
        printf("Jogador %d queria atacar mas está fazendo outra ação\n", id);
        pthread_cond_wait(&conds[id], &mutex);
        printf("Jogador %d teve seu ataque desbloqueado\n", id);
    }

    printf("Jogador %d quer atacar\n", id);

    while(estado[idAlvo] >= 4) {
        printf("Jogador %d teve seu ataque bloqueado\n", id);
        pthread_cond_wait(&conds[id], &mutex);
        printf("Jogador %d teve seu ataque desbloqueado\n", id);
    }
    // está insistindo no mesmo alvo
    atacando++;
    if (estado[idAlvo] >= 2) estado[idAlvo]++;
    estado[id] = 1;
    pthread_mutex_unlock(&mutex);
}

// saida ataque
void FimAtaque (int id, int idAlvo) {
    pthread_mutex_lock(&mutex);
    printf("Jogador %d terminou de atacar\n", id);
    atacando--;
    if (estado[idAlvo] == 4) estado[idAlvo] = 3;
    if (estado[idAlvo] == 3) estado[idAlvo] = 2;
    estado[id] = 0;
    pthread_cond_signal(&conds[id]);
    pthread_mutex_unlock(&mutex);
}

// entrada defesa
void IniciaDefesa (int id) {
    pthread_mutex_lock(&mutex);
    while(estado[id] != 0) {
        printf("Jogador %d queria defender mas está fazendo outra ação\n", id);
        pthread_cond_wait(&conds[id], &mutex);
        printf("Jogador %d teve sua defesa desbloqueada\n", id);
    }

    printf("Jogador %d quer defender\n", id);
    defendendo++;
    estado[id] = 2;
    pthread_mutex_unlock(&mutex);
}

// saida defesa
void FimDefesa (int id) {
    pthread_mutex_lock(&mutex);
    printf("Jogador %d terminou de defender\n", id);
    defendendo--;
    estado[id] = 0;
    pthread_cond_signal(&conds[id]);
    pthread_mutex_unlock(&mutex);
}

// thread de ataque
void * atacante (void * arg) {
    struct Jogador *jogador = (struct Jogador *) arg;
    int id_jogador = jogador->id_jogador; //(int)(floor((float)*id_thread/(float)A)) + 1; //id_threadA = (j * A) + i -> j = (id_threadA - i) / A -> id_jogador = floor(id_threadA / A)
    int vaiAtacar = 0;
    while(1) {
        // escolhendo o alvo
        srand(time(NULL));
        int idAlvo = 0;
        while(1) {
            idAlvo = rand() % P;
            // NÃO PODE SER ELE MESMO (OU PODE?)
            printf("Jogador %d mirou no jogador %d\n", id_jogador, idAlvo);
            if(estado[idAlvo] < 3) {
                vaiAtacar = 1;
                break;
            }
            else {
                printf("Jogador %d queria atacar jogador %d mas este já está sendo atacado por 2 jogadores\n", id_jogador, idAlvo);
            }
        }

        if (vaiAtacar == 1) {
            IniciaAtaque(id_jogador, idAlvo);
            printf("Jogador %d esta atacando o jogador %d\n", id_jogador, idAlvo);

            sleep(0.25); // tempo para realizar o ataque
            if (estado[idAlvo] == 0 || estado[idAlvo] == 1) {
                printf("ATAQUE BEM SUCEDIDO - Jogador %d inflingiu %d de dano ao jogador %d\n", id_jogador, jogador->dano, idAlvo);
                // FALTA REALMENTE TIRAR A VIDA DOS JOGADORES E MATAR ELES QUANDO A VIDA CHEGAR A 0
            }
            if (estado[idAlvo] == 2 || estado[idAlvo] == 3) {
                printf("DEFESA - Jogador %d se defendeu do ataque do jogador %d\n", idAlvo, id_jogador);
            }
            if (estado[idAlvo] == 4) {
                printf("ERRO - Jogador %d atacou o jogador %d que estava no estado 4\n", id_jogador, idAlvo);
            }

            FimAtaque(id_jogador, idAlvo);
            sleep(1); //tempo de descanso
        }
    } 
    free(arg);
    pthread_exit(NULL);
}

// thread de defesa
void * defensor (void * arg) {
    struct Jogador *jogador = (struct Jogador *) arg;
    int id_jogador = jogador->id_jogador; // (int)(floor((float)*id_thread/(float)D)) + 1; //id_threadD = (j * D) + i -> j = (id_threadD - i) / D -> id_jogador = floor(id_threadD / D)
    while(1) {
        IniciaDefesa(id_jogador);
        printf("Jogador %d esta defendendo\n", id_jogador);
        sleep(1); //tempo de defesa
        FimDefesa(id_jogador);
        sleep(1); //tempo de descanso
    } 
    free(arg);
    pthread_exit(NULL);
}

// funcao principal
int main(void) {
    // identificadores das threads
    pthread_t tid[(A + D) * P];
    int id[(A + D) * P];

    // inicializa as variaveis de sincronizacao
    printf("Inicializando as variaveis de sincronizacao\n");
    pthread_mutex_init(&mutex, NULL);
    for(int k=0; k<P; k++) {
        pthread_cond_init(&conds[k], NULL);
    }

    // cria os jogadores e seus status
    printf("Criando os jogadores e seus status\n");
    srand(time(NULL));
    struct Jogador lista_jogs[P];
    for(int i=0; i<P; i++){
        lista_jogs[i].id_jogador = i;
        lista_jogs[i].vida = rand() % vida_maxima;
        lista_jogs[i].dano = rand() % dano_maximo;
        printf("Jogador %d criado com %d de vida e %d de dano\n", i, lista_jogs[i].vida, lista_jogs[i].dano);
    }

    // cria as threads atacantes
    for(int j=0; j < P; j++) {
        for(int i=0; i < A; i++) {
            id[(j * A) + i] = (j * A) + i + 1;
            if(pthread_create(&tid[(j * A) + i], NULL, atacante, (void *) &lista_jogs[j])) exit(-1);
            printf("Thread atacante %d criada\n", (j * A) + i);
        } 
    }

    // cria as threads defensoras
    for(int j=0; j < P; j++) {
        for(int i=0; i < D; i++) {
            id[(A * P) - 1 + (j * D) + i ] = (A * P) + (j * D) + i ;
            if(pthread_create(&tid[(A * P) - 1 + (j * D) + i], NULL, defensor, (void *) &lista_jogs[j])) exit(-1);
            printf("Thread defensora %d criada\n", (A * P) - 1 + (j * D) + i);
        }
    }

    pthread_exit(NULL);
    return 0;
}
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <time.h>

/*               -- REGRAS DO JOGO --
- As threads são ações: ATAQUE / DEFESA / ESQUIVA? / DESARME?
- Vão rolando ações de jogadores aleatórios
    * Ataques contra outros jogadores
    * Defesas (duram 1 segundo)
    * Esquivas (duram 0.5 segundos)
    * Desarme (dura 0.25 segunodos)
- Cada jogador tem vida e dano (agilidade)
- 2 podem atacar um defensor, que se defende dos dois
*/

// ATUAL - AJEITAR A DEFESA

#define A 4 // numero de threads de ataque
#define D 2 // numero de threads de defesa
#define A_A 3 // quantos podem atacar ao mesmo tempo
#define D_A 2 // de quantos atcantes um defensor pode se defender ao mesmo tempo
#define P 6 // total de jogadores

long long int vida_maxima = 100;
long long int dano_maximo = 10;

// variaveis do problema
int vida[] = {0}; // vetor com vidas dos jogadores
int estado[] = {0}; // vetor com os estados dos jogadores: 0 = INATIVO // 1 = ATACANDO // 2 = EM FORMAÇÃO DE DEFESA
                    // 3 = DEFENDENDO DE 1 // 4 = DEFENDENDO DE 2
int atacando = 0; // contador de threads atacando
int defendendo = 0; // contador de threads defendendo
int totalRingue = 0; // contador de threads totais (por enquanto nao vou usar isso
                    // pq iria requerer uma entrada no ringue para estar disponível para ataque/defesa)

typedef struct Jogador {
    int id_jogador;
    int vida;
    int dano;
} jogador;

// variaveis para sincronizacao
pthread_mutex_t mutex;
pthread_cond_t cond_ataque, cond_defesa;

// entrada ataque
void IniciaAtaque (int id, int idAlvo) {
    pthread_mutex_lock(&mutex);
    printf("A[%d] quer atacar\n", id);
    while(totalRingue > P && estado[idAlvo] >= 4) {
        printf("A[%d] bloqueou\n", id);
        pthread_cond_wait(&cond_ataque, &mutex);
        printf("A[%d] desbloqueou\n", id);
    }
    atacando++;
    if (estado[idAlvo] >= 2) estado[idAlvo]++;
    estado[id] = 1;
    pthread_mutex_unlock(&mutex);
    return(idAlvo);
}

// saida ataque
void FimAtaque (int id, int idAlvo) {
    pthread_mutex_lock(&mutex);
    printf("A[%d] terminou de atacar\n", id);
    atacando--;
    if (estado[idAlvo] == 4) estado[idAlvo] = 3;
    if (estado[idAlvo] == 3) estado[idAlvo] = 2;
    estado[id] = 0;
    pthread_mutex_unlock(&mutex);
}

// entrada defesa
void IniciaDefesa (int id) {
    pthread_mutex_lock(&mutex);
    printf("D[%d] quer defender\n", id);
    while((atacando>0) || (defendendo>0)) {
        printf("D[%d] bloqueou\n", id);
        pthread_cond_wait(&cond_defesa, &mutex);
        printf("D[%d] desbloqueou\n", id);
    }
    defendendo++;
    pthread_mutex_unlock(&mutex);
}

// saida defesa
void FimDefesa (int id) {
    pthread_mutex_lock(&mutex);
    printf("D[%d] terminou de defender\n", id);
    defendendo--;
    pthread_cond_signal(&cond_defesa);
    pthread_cond_broadcast(&cond_ataque);
    pthread_mutex_unlock(&mutex);
}

// thread de ação
void * acao (void * arg) {
    pthread_mutex_lock(&mutex);
    int *id = (int *) arg;
    srand(time(NULL));
    while(1) {
        int aleatorio = rand() % 100;
        if (aleatorio >= 0 && aleatorio < 40){
            estado[*id] = 1;
        }
        else if (aleatorio >= 40 && aleatorio < 70) {
            estado[*id] = 2;
        }
        else if (aleatorio >= 70 && aleatorio < 90) {
            estado[*id] = 5; // ESQUIVA
        }
        else if (aleatorio >= 90 && aleatorio < 100) {
            estado[*id] = 6; // DESARME
        }
        estado[*id] = rand() % 4;
        pthread_cond_signal(&cond_defesa);
        //printf("Defensora %d esta defendendo\n", *id);
        //sleep(1); //tempo de defesa
        sleep(1); //tempo de descanso
    } 
    free(arg);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

// thread de ataque
void * atacante (void * arg) {
    int *id = (int *) arg;
    int vaiAtacar = 0;
    while(1) {
        int *idAlvo = 0;
        while(idAlvo < P) {
            if(estado[*idAlvo] < 3) {
                vaiAtacar = 1;
                break;
            }
        idAlvo++;
        }
        if (vaiAtacar == 1) {
            IniciaAtaque(*id, *idAlvo);
            printf("Atacante %d esta atacando\n", *id);
            FimAtaque(*id, *idAlvo);
            sleep(1); //tempo de descanso
        }
    } 
    free(arg);
    pthread_exit(NULL);
}

// thread de defesa
void * defensor (void * arg) {
    int *id = (int *) arg;
    while(1) {
        IniciaDefesa(*id);
        printf("Defensora %d esta defendendo\n", *id);
        sleep(1); //tempo de defesa
        FimDefesa(*id);
        sleep(1); //tempo de descanso
    } 
    free(arg);
    pthread_exit(NULL);
}

// funcao principal
int main(void) {
    // identificadores das threads
    pthread_t tid[A+D];
    int id[A+D];

    // inicializa as variaveis de sincronizacao
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_ataque, NULL);
    pthread_cond_init(&cond_defesa, NULL);

    // cria os jogadores e seus status
    srand(time(NULL));
    jogador * lista_jogs[P];
    for(int i=0; i<P; i++){
        lista_jogs[i]->id_jogador = i;
        lista_jogs[i]->vida = rand() % vida_maxima;
        lista_jogs[i]->dano = rand() % dano_maximo;
    }

    // cria as threads de acao
    for(int i=0; i<P; i++) {
    id[i] = i+1;
    if(pthread_create(&tid[i], NULL, acao, (void *) &id[i])) exit(-1);
    } 

    // cria as threads atacantes
    for(int i=0; i<A; i++) {
    id[i] = i+1;
    if(pthread_create(&tid[i], NULL, atacante, (void *) &id[i+P])) exit(-1);
    } 

    // cria as threads defensoras
    for(int i=0; i<D; i++) {
    id[i+A] = i+1;
    if(pthread_create(&tid[i+A], NULL, defensor, (void *) &id[i+P+A])) exit(-1);
    } 

    pthread_exit(NULL);
    return 0;
}
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#define A 4 //numero de threads atacantes
#define D 2 //numero de threads defensoras
#define A_A 2 //quantos podem atacar ao mesmo tempo
#define D_A 2 //de quantos atcantes um defensor pode se defender ao mesmo tempo
#define P 6 //quantas pessoas cabem no ringue

//condições do problema
//2 podem atacar um defensor, que se defende dos dois

//variaveis do problema
int vida[] = {0}; //vetor com vidas dos jogadores
int estado[] = {0}; //vetor com os estados das threads: 0 = PARADA // 1 = ATACANDO/DEFENDENDO DE 1
                   // 2 = DEFENDENDO DE 2
int atacando = 0; //contador de threads atacando
int defendendo = 0; //contador de threads defendendo
int totalRingue = 0; //contador de threads totais (por enquanto nao vou usar isso
                    //pq iria requerer uma entrada no ringue para estar disponível para ataque/defesa)

//variaveis para sincronizacao
pthread_mutex_t mutex;
pthread_cond_t cond_ataque, cond_defesa;

//entrada ataque
int * IniciaAtaque (int id) {
   pthread_mutex_lock(&mutex);
   printf("A[%d] quer atacar\n", id);
   int idDef = 0;
   while(idDef < P) {
     if(estado[idDef] < 2) {
         break;
     }
     idDef++;
   }
   while(totalRingue > P && estado[idDef] >= 2) {
     printf("A[%d] bloqueou\n", id);
     pthread_cond_wait(&cond_ataque, &mutex);
     printf("A[%d] desbloqueou\n", id);
   }
   atacando++;
   estado[idDef]++;
   estado[id]++;
   pthread_mutex_unlock(&mutex);
   return(idDef);
}

//saida ataque
void FimAtaque (int id, int idDef) {
   pthread_mutex_lock(&mutex);
   printf("A[%d] terminou de atacar\n", id);
   atacando--;
   estado[idDef]--;
   estado[id]--;
   if(estado[idDef] == 1) pthread_cond_signal(&cond_ataque);
   if(estado[idDef] == 0) pthread_cond_broadcast(&cond_ataque);
   pthread_mutex_unlock(&mutex);
}

//entrada defesa
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

//saida defesa
void FimDefesa (int id) {
   pthread_mutex_lock(&mutex);
   printf("D[%d] terminou de defender\n", id);
   defendendo--;
   pthread_cond_signal(&cond_defesa);
   pthread_cond_broadcast(&cond_ataque);
   pthread_mutex_unlock(&mutex);
}

//thread atacante
void * atacante (void * arg) {
  int *id = (int *) arg;
  while(1) {
    int *idDef = IniciaAtaque(*id);
    printf("Atacante %d esta atacando\n", *id);
    FimAtaque(*id, *idDef);
    sleep(1);
  } 
  free(arg);
  pthread_exit(NULL);
}

//thread defensora
void * defensor (void * arg) {
  int *id = (int *) arg;
  while(1) {
    IniciaDefesa(*id);
    printf("Defensora %d esta defendendo\n", *id);
    FimDefesa(*id);
    sleep(1);
  } 
  free(arg);
  pthread_exit(NULL);
}

//funcao principal
int main(void) {
  //identificadores das threads
  pthread_t tid[A+D];
  int id[A+D];

  //inicializa as variaveis de sincronizacao
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond_ataque, NULL);
  pthread_cond_init(&cond_defesa, NULL);

  //cria as threads atacantes
  for(int i=0; i<A; i++) {
    id[i] = i+1;
    if(pthread_create(&tid[i], NULL, atacante, (void *) &id[i])) exit(-1);
  } 
  
  //cria as threads defensoras
  for(int i=0; i<D; i++) {
    id[i+A] = i+1;
    if(pthread_create(&tid[i+A], NULL, defensor, (void *) &id[i+A])) exit(-1);
  } 

  pthread_exit(NULL);
  return 0;
}
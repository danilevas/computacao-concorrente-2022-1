#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h>
#include <semaphore.h>

#define NTHREADS 5

// variaveis globais
// semaforos para sincronizar a ordem de execucao das threads
sem_t sem1;
sem_t sem2;
int cont = 0; // contador

// funcao a ser executada pela thread 1
void *t1 () {
  // printf("\nThread : %d esta executando...\n", *tid);
  sem_wait(&sem2);
  printf("Volte sempre!\n");
  // printf("\nThread : %d terminou!\n", *tid);
  pthread_exit(NULL);
}

// funcao a ser executada pelas threads 2, 3 e 4
void *t_alt (void *frase) {
  char *diga = (char*) frase;
  // printf("\nThread : %d esta executando...\n", *tid);
  sem_wait(&sem1); //espera T5 executar
  puts(diga);
  sem_post(&sem1);
  cont++;
  // printf("\nThread : %d terminou!\n", *tid);
  if(cont == 3){
    sem_post(&sem2);
  }
  pthread_exit(NULL);
}

void *t5 () {
    // printf("\nThread : %d esta executando...\n", *tid);
    printf("Seja bem-vindo!\n");
    sem_post(&sem1);
    // printf("\nThread : %d terminou!\n", *tid);
    pthread_exit(NULL);
}

//funcao principal
int main(int argc, char *argv[]) {
  pthread_t tid[NTHREADS];
  int *id[5], t;

  for (t=0; t<NTHREADS; t++) {
    if ((id[t] = malloc(sizeof(int))) == NULL) {
       pthread_exit(NULL); return 1;
    }
    *id[t] = t+1;
  }

  //inicia os semaforos
  sem_init(&sem1, 0, 0);
  sem_init(&sem2, 0, 0);

/* Frases */
  char f2[] = "Fique a vontade";
  char f3[] = "Sente-se por favor";
  char f4[] = "Aceita um copo d'agua?";

  // cria as threads
  if (pthread_create(&tid[0], NULL, t1, NULL)) { printf("--ERRO: pthread_create()\n"); exit(-1); }
  if (pthread_create(&tid[1], NULL, t_alt, (void *)f2)) { printf("--ERRO: pthread_create()\n"); exit(-1); }
  if (pthread_create(&tid[2], NULL, t_alt, (void *)f3)) { printf("--ERRO: pthread_create()\n"); exit(-1); }
  if (pthread_create(&tid[3], NULL, t_alt, (void *)f4)) { printf("--ERRO: pthread_create()\n"); exit(-1); }
  if (pthread_create(&tid[4], NULL, t5, NULL)) { printf("--ERRO: pthread_create()\n"); exit(-1); }

  // espera todas as threads terminarem
  for (t=0; t<NTHREADS; t++) {
    if (pthread_join(tid[t], NULL)) {
         printf("--ERRO: pthread_join() \n"); exit(-1);
    } 
    free(id[t]);
  } 
  pthread_exit(NULL);
}
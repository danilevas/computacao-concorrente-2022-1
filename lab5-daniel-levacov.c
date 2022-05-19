#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NTHREADS 5

/* Variaveis globais */
int controle = 0;
pthread_mutex_t mutex;
pthread_cond_t cond;

/* Thread 1 */
void *thread1 () { 
  pthread_mutex_lock(&mutex);
  while(controle < 4) {
    pthread_cond_wait(&cond, &mutex);
  }
  controle++;
  pthread_mutex_unlock(&mutex);
  printf("Volte sempre!\n");
  pthread_exit(NULL);
}

/* Threads 2 a 4 */
void *threads_alt (void *frase) {
  char* diga = (char*)frase;
  pthread_mutex_lock(&mutex);
  while(controle < 1) {
    pthread_cond_wait(&cond, &mutex);
  }

  controle++;
  puts(diga);
  
  if(controle == 4)
    pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);
  pthread_exit(NULL);
}

/* Thread 5 */
void *thread5 () {
  printf("Seja bem-vindo!\n");
  pthread_mutex_lock(&mutex);
  controle++;

  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex); 
  pthread_exit(NULL);
}

/* Funcao principal */
int main(int argc, char *argv[]) {
  int i; 
  pthread_t threads[NTHREADS];

  /* Inicilaiza o mutex (lock de exclusao mutua) e a variavel de condicao */
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init (&cond, NULL);

  /* Frases */
  char f2[] = "Fique a vontade";
  char f3[] = "Sente-se por favor";
  char f4[] = "Aceita um copo d'agua?";

  /* Cria as threads */
  pthread_create(&threads[0], NULL, thread1, NULL);
  pthread_create(&threads[1], NULL, threads_alt, (void *)f2);
  pthread_create(&threads[2], NULL, threads_alt, (void *)f3);
  pthread_create(&threads[3], NULL, threads_alt, (void *)f4);
  pthread_create(&threads[4], NULL, thread5, NULL);

  /* Espera todas as threads completarem */
  for (i = 0; i < NTHREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  /* Desaloca variaveis e termina */
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
}
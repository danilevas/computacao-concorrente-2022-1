#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 2
#define NUM_ELEMENTOS 10000

//vetor com os elementos
int vec[NUM_ELEMENTOS];

//funcao que calcula o quadrado dos elementos
void * quadrado(void * arg) {
  int *numero = (int *) arg;
  for(int i=0; i<(NUM_ELEMENTOS/2); i++) {
    numero[i] = numero[i] * numero[i];
    // printf("%d\n", i);
    // printf("%d\n", numero[i]);
  }
  pthread_exit(NULL);
}

int checagem() {
  for(int init=0; init<NUM_ELEMENTOS; init++) {
    if(vec[init] != (init*init))
      return 0;
  }
  return 1;
}

int main(void) {
  pthread_t tid[NUM_THREADS]; //id da thread
  int check; //retorno da funcao checagem

  for(int i=0; i<NUM_ELEMENTOS; i++) {
    vec[i] = i;
  }

  if(pthread_create(&tid[0], NULL, quadrado, (void *)&vec[0])) {
    printf("ERRO -- pthread_create: thread 1\n");
  }
  if(pthread_create(&tid[1], NULL, quadrado, (void *)&vec[(NUM_ELEMENTOS/2)])) {
    printf("ERRO -- pthread_create: thread 2\n");
  }

  for(int i=0; i<NUM_THREADS; i++) {
    if (pthread_join(tid[i], NULL)) {
      printf("ERRO -- pthread_join\n");
    }
  }
  
  //confere se os valores estão corretos 
  check = checagem();
  if(check) {
    printf("sucesso!\n");
  } else {
    printf("falhou...\n");
  }
  
  pthread_exit(NULL);
  return 0;
}
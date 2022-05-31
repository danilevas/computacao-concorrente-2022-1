//Soma todos os elementos de um vetor de inteiro
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include"timer.h"
#include <time.h>
#include <math.h>

long long int rand_maximo = 100;
long long int dim; //dimensao do vetor de entrada
int nthreads; //numero de threads
int *vetorEntrada; //vetor de entrada com dimensao dim
float *vetorSaida; //vetor de saida com dimensao dim

pthread_mutex_t mutex; //variavel de lock para exclusao mutua
int iGlobal = 0;

//funcao ehPrimo
int ehPrimo(long long int n){
        if (n<=1) return 0;
        if (n==2) return 1;
        if (n%2==0) return 0;
        for (int i=3; i<sqrt(n) + 1; i+=2){
            if (n%i==0) return 0;
        }
        return 1;
    }

//fluxo das threads
void * tarefa(void * arg) {
    long int id = (long int) arg; //identificador da thread
    //printf("Thread %ld iniciando\n", id);
    int iLocal = 0;

    pthread_mutex_lock(&mutex);
    iLocal = iGlobal;
    iGlobal++;
    pthread_mutex_unlock(&mutex);
    while (iLocal < dim) {
        pthread_mutex_lock(&mutex);
        if (ehPrimo(vetorEntrada[iLocal]) == 1) {
            vetorSaida[iLocal] = (float)(sqrt(vetorEntrada[iLocal]));
            //printf("T%ld: vetorEntrada[%d] = %d / vetorSaida[%d] = %f\n", id, iLocal, vetorEntrada[iLocal], iLocal, vetorSaida[iLocal]);
        }
        else {
            vetorSaida[iLocal] = vetorEntrada[iLocal];
            //printf("T%ld: vetorEntrada[%d] = %d / vetorSaida[%d] = %f\n", id, iLocal, vetorEntrada[iLocal], iLocal, vetorSaida[iLocal]);
        }
        iLocal = iGlobal;
        iGlobal++;
        pthread_mutex_unlock(&mutex);
    }

    //retorna
    pthread_exit(NULL);
}

//teste dos resultados concorrentes
int check() {
    for(int k=0; k<dim; k++) {
        if ((ehPrimo(vetorEntrada[k]) == 1) && (vetorSaida[k] != (float)(sqrt(vetorEntrada[k])))) {
            printf("vetorEntrada[%d] = %d / vetorSaida[%d] = %f - diferente de %lf\n", k, vetorEntrada[k], k,
                vetorSaida[k], sqrt(vetorEntrada[k]));
            return 1;
        }
    }
    return 0;
}

//fluxo principal
int main(int argc, char *argv[]) {
    double ini, fim; //tomada de tempo
    pthread_t *tid; //identificadores das threads no sistema
    double *retorno; //valor de retorno das threads

    //recebe e valida os parametros de entrada (dimensao do vetor, numero de threads)
    if(argc < 3) {
        fprintf(stderr, "Digite: %s <dimensao do vetor> <numero threads>\n", argv[0]);
        return 1; 
    }
    dim = atoll(argv[1]);
    nthreads = atoi(argv[2]);

    //aloca o vetor de entrada e saida
    vetorEntrada = (int*) malloc(sizeof(int)*dim);
    if(vetorEntrada == NULL) {
        fprintf(stderr, "ERRO--malloc\n");
        return 2;
    }
    vetorSaida = (float*) malloc(sizeof(float)*dim);
    if(vetorSaida == NULL) {
        fprintf(stderr, "ERRO--malloc\n");
        return 2;
    }

    //preenche o vetor de entrada e incializa o de saida com 0s
    srand(time(NULL));
    for(long int i=0; i<dim; i++) {
        vetorEntrada[i] = rand() % rand_maximo;
        //printf("%d\n", vetorEntrada[i]);
        vetorSaida[i] = 0;
    }

    //fazer o processo sequencialmente
    GET_TIME(ini);
    int cont = 0;
    for(int k=0; k<dim; k++){
        if (ehPrimo(vetorEntrada[k]) == 1) {
            cont++;
            vetorSaida[k] = (float)(sqrt(vetorEntrada[k]));
        }
        else {
            vetorSaida[k] = vetorEntrada[k];
        }
    }
    GET_TIME(fim);
    printf("Tempo sequencial:  %lf / %d primos\n", fim-ini, cont);
    // for(int j=0; j<dim; j++) {
    //     printf("%lf\n", vetorSaida[j]);
    // }

    //parte concorrente
    GET_TIME(ini);

    //inicilaiza o mutex (lock de exclusao mutua)
    pthread_mutex_init(&mutex, NULL);

    //criar os tids
    tid = (pthread_t *) malloc(sizeof(pthread_t) * nthreads);
    if(tid==NULL) {
        fprintf(stderr, "ERRO--malloc\n");
        return 2;
    }

    //criar as threads
    for(long int i=0; i<nthreads; i++) {
        if(pthread_create(tid+i, NULL, tarefa, (void*) i)) {
            fprintf(stderr, "ERRO--pthread_create\n");
            return 3;
        }
    }

    //aguardar o termino das threads
    for(long int i=0; i<nthreads; i++) {
        if(pthread_join(*(tid+i), (void**) &retorno)) {
            fprintf(stderr, "ERRO--pthread_create\n");
            return 3;
        }
    }

    pthread_mutex_destroy(&mutex);
    GET_TIME(fim);
    printf("Tempo concorrente:  %lf\n", fim-ini);

    //testar os resultados
    if (check(vetorEntrada, vetorSaida) == 1){
        printf("ERRO -- Resultados concorrentes errados.\n");
        return 4;
    }
    else {
        printf("Tudo certinho com os resultados concorrentes!\n");
    }

    //libera as areas de memoria alocadas
    free(vetorEntrada);
    free(vetorSaida);
    free(tid);

    return 0;
}
//Soma todos os elementos de um vetor de inteiro
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include"timer.h"
#include <time.h>

long long int rand_maximo = 100000000;
long long int divisor_rand = 10000;

long long int dim; //dimensao do vetor de entrada
int nthreads; //numero de threads
float *vetor; //vetor de entrada com dimensao dim 

//fluxo das threads
void * tarefa(void * arg) {
    long int id = (long int) arg; //identificador da thread
    double *menorMaior; //vetor local que guarda o menor e maior elemento

    menorMaior = (double*) malloc(sizeof(double)*2);
    if(menorMaior == NULL) {exit(1);}
    long int tamBloco = dim/nthreads;  //tamanho do bloco de cada thread 
    long int ini = id * tamBloco; //elemento inicial do bloco da thread
    long int fim; //elemento final(nao processado) do bloco da thread
    if(id == nthreads-1) fim = dim;
    else fim = ini + tamBloco; //trata o resto se houver

    //acha o maior e menor elemento do bloco da thread
    menorMaior[0] = vetor[ini];
    menorMaior[1] = vetor[ini];
    for(long long int i=ini+1; i<fim; i++) {
        if (vetor[i] < menorMaior[0]) {
            menorMaior[0] = vetor[i];
        }
        if (vetor[i] > menorMaior[1]) {
            menorMaior[1] = vetor[i];
        }
    }

    //retorna o resultado do maior e menor elemento do bloco
    pthread_exit((void *) menorMaior);
}

//fluxo principal
int main(int argc, char *argv[]) {
    float maiorSeq, menorSeq = 0; //maior e menor sequencial
    float maiorConc, menorConc = 0; //maior e menor concorrente
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

    //aloca o vetor de entrada
    vetor = (float*) malloc(sizeof(float)*dim);
    if(vetor == NULL) {
        fprintf(stderr, "ERRO--malloc\n");
        return 2;
    }

    //preenche o vetor de entrada
    srand(time(NULL));
    for(long int i=0; i<dim; i++)
        vetor[i] = (float)(rand() % rand_maximo)/divisor_rand;

    //ver quem eh maior e menor sequencialmente
    GET_TIME(ini);
    maiorSeq = vetor[0];
    menorSeq = vetor[0];
    for(long long int i=1; i<dim; i++) {
        if (vetor[i] < menorSeq) {
            menorSeq = vetor[i];
        }
        if (vetor[i] > maiorSeq) {
            maiorSeq = vetor[i];
        }
    }
    GET_TIME(fim);
    printf("Tempo sequencial:  %lf\n", fim-ini);

    //soma concorrente dos elementos
    GET_TIME(ini);
    tid = (pthread_t *) malloc(sizeof(pthread_t) * nthreads);
    if(tid==NULL) {
        fprintf(stderr, "ERRO--malloc\n");
        return 2;
    }

    //criar as threads
    for(long int i=0; i<nthreads; i++) {
        if(pthread_create(tid+i, NULL, tarefa, (void*) i)){
            fprintf(stderr, "ERRO--pthread_create\n");
            return 3;
        }
    }

    //aguardar o termino das threads

    for(long int i=0; i<nthreads; i++) {
        if(pthread_join(*(tid+i), (void**) &retorno)){
            fprintf(stderr, "ERRO--pthread_create\n");
            return 3;
        }

        //descobrindo maiores e menores globais
        if (retorno[0] < menorConc || i == 0){
            menorConc = retorno[0];
        }
        if (retorno[1] > maiorConc || i == 0){
            maiorConc = retorno[1];
        }
    }
    GET_TIME(fim);
    printf("Tempo concorrente:  %lf\n", fim-ini);

    //exibir os resultados
    printf("Maior seq:  %.12f / Menor seq: %.12f\n", maiorSeq, menorSeq);
    printf("Maior conc:  %.12f / Menor conc: %.12f\n", maiorConc, menorConc);

    //libera as areas de memoria alocadas
    free(vetor);
    free(tid);

    return 0;
}
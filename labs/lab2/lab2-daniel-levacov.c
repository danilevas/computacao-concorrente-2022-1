#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "timer.h"

int rand_maximo = 10000;

int *mat; //matriz de entrada
int *mat2; //matriz de entrada 2
int *saida; //matriz de saida
int nthreads; //numero de threads

typedef struct{
   int id; //identificador do elemento que a thread ira processar
   int dim; //dimensao das estruturas de entrada
} tArgs;

//funcao que as threads executarao
void * tarefa(void *arg) {
    tArgs *args = (tArgs*) arg;
    printf("Thread %d\n", args->id);
    for(int i=args->id; i<args->dim; i+=nthreads) {
        for(int j=0; j<args->dim; j++) {
            for(int k=0; k<args->dim; k++) {
                saida[i*args->dim + j] += mat[i*args->dim + k] * mat2[k*args->dim + j];
            }
        }
   }
   pthread_exit(NULL);
}

//fluxo principal
int main(int argc, char* argv[]) {
    int dim; //dimensao da matriz de entrada
    pthread_t *tid; //identificadores das threads no sistema
    tArgs *args; //identificadores locais das threads e dimensao
    double inicio, fim;
   
    //leitura e avaliacao dos parametros de entrada
    if(argc<3) {
       printf("Digite: %s <dimensao da matriz> <numero de threads>\n", argv[0]);
       return 1;
    }
    dim = atoi(argv[1]);
    nthreads = atoi(argv[2]);
    if (nthreads > dim) nthreads=dim;

    // alocacao de memoria para as estruturas de dados
    mat = (int *) malloc(sizeof(int) * dim * dim);
    if (mat == NULL) {printf("ERRO--malloc\n"); return 2;}
    mat2 = (int *) malloc(sizeof(int) * dim * dim);
    if (mat2 == NULL) {printf("ERRO--malloc\n"); return 2;}
    saida = (int *) malloc(sizeof(int) * dim * dim);
    if (saida == NULL) {printf("ERRO--malloc\n"); return 2;}
   
    //inicializacao das estruturas de dados de entrada e saida
    srand(time(NULL));
    for(int i=0; i<dim; i++) {
        for(int j=0; j<dim; j++) {
            mat[i*dim+j] = rand() % rand_maximo;    //equivalente mat[i][j]
            mat2[i*dim+j] = rand() % rand_maximo;
            saida[i*dim+j] = 0;
        }
    }

    GET_TIME(inicio);
    //multiplicacao das matrizes SEQUENCIAL
    for(int i=0; i<dim; i++) {
        for(int j=0; j<dim; j++) {
            for(int k=0; k<dim; k++) {
                saida[i*dim+j] += mat[i*dim+k] * mat2[k*dim+j];
            }
        }
    }
    GET_TIME(fim);
    double tsequencial = fim - inicio;
    printf("Tempo multiplicacao SEQUENCIAL:%lf\n", tsequencial);

    //checagem dos resultados SEQUENCIAIS
    int result = 0;
    int certo = 0;
    for(int i=0; i<dim; i++) {
        for(int j=0; j<dim; j++) {
            for(int k=0; k<dim; k++) {
                result += mat[i*dim+k] * mat2[k*dim+j];
            }

            if (result == saida[i*dim+j]){
                //printf("Check %d/%d\n", i, j);
                result = 0;
            }
            else {
                //printf("ERRO NA POSICAO %d/%d\n", i, j);
                certo ++;
                result = 0;
            }
        }
    }

    if (certo == 0){
        printf("Multiplicacao SEQUENCIAL correta: 0\n");
    }
    else {
        printf("Multiplicacao SEQUENCIAL com erro: 1\n");
    }

    //reinicializacao da estrutura de dados de saida
    srand(time(NULL));
    for(int i=0; i<dim; i++) {
        for(int j=0; j<dim; j++) {
            saida[i*dim+j] = 0;
        }
    }

    //multiplicacao da matriz pelo vetor CONCORRENTE
    GET_TIME(inicio);

    //alocacao das estruturas
    tid = (pthread_t*) malloc(sizeof(pthread_t)*nthreads);
    if(tid==NULL) {puts("ERRO--malloc"); return 2;}
    args = (tArgs*) malloc(sizeof(tArgs)*nthreads);
    if(args==NULL) {puts("ERRO--malloc"); return 2;}

    //criacao das threads
    for(int i=0; i<nthreads; i++) {
        (args+i)->id = i;
        (args+i)->dim = dim;
        if(pthread_create(tid+i, NULL, tarefa, (void*) (args+i))){
            puts("ERRO--pthread_create"); return 3;
        }
    }

    //espera pelo termino da threads
    for(int i=0; i<nthreads; i++) {
        pthread_join(*(tid+i), NULL);
    }
    GET_TIME(fim)
    double tconcorrente = fim - inicio;
    printf("Tempo multiplicacao CONCORRENTE:%lf\n", tconcorrente);

    //checagem dos resultados CONCORRENTES
    result = 0;
    certo = 0;
    for(int i=0; i<dim; i++) {
        for(int j=0; j<dim; j++) {
            for(int k=0; k<dim; k++) {
                result += mat[i*dim+k] * mat2[k*dim+j];
            }

            if (result == saida[i*dim+j]){
                //printf("Check %d/%d\n", i, j);
                result = 0;
            }
            else {
                //printf("ERRO NA POSICAO %d/%d\n", i, j);
                certo ++;
                result = 0;
            }
        }
    }

    if (certo == 0){
        printf("Multiplicacao CONCORRENTE correta: 0\n");
    }
    else {
        printf("Multiplicacao CONCORRENTE com erro: 1\n");
    }

    //avaliando a diferenca de tempo
    double dif = tsequencial/tconcorrente;
    printf("Diferenca entre tempo de execucao de programa sequencial e concorrente: %lf\n", dif);
    if (dif > 1) {
        printf("CONCORRENTE foi mais rapido\n");
    }
    else {
        printf("SEQUENCIAL foi mais rapido\n");
    }

    //liberacao da memoria
    free(mat);
    free(mat2);
    free(saida);
    free(args);
    free(tid);

    return 0;
}

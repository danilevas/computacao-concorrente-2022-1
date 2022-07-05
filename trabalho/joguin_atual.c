#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "nomes.h"
#include <errno.h> 

/*
NOTAS SILVANA
    - sempre se ligar em quando as threads estão lendo informações globais para não haver problema de corrida
    - ver se uma thread não está alterando algo de outra thread
    - ter bem definidas as situações em que uma thread se bloqueia
    - se ligar nas condições para as threads se bloquearem e só der signal quando de fato as condições mudarem
    - pensar que o signal sinaliza a thread independente da condição que foi usada para entrar no wait

ERROS
    - na hora do sleep do desarme, não é só a thread desarmada que para, mas todas

PRÓXIMOS PASSOS
    - elaborar desenhos legais e compreensíveis
    - balancear os valores de vida e dano, e os tempos // ir testando bastante
    - implementar novas ações
        * ESQUIVA // baseado num status de agilidade x/100, que influencia a chance de um ataque recebido não dar certo // a principio foi!
        * DESARME // baseado num status de inteligência y/100, que influencia a chance de um ataque recebido não dar certo e ainda atordoar o atacante
            * entender o que está acontecendo, colocar vários prints
    
SE DER TEMPO
    - superpoder: jogador ataca 2 outros ao mesmo tempo
    - colocar o suicídio com uma chance bem menor de acontecer (tipo um critical fail) // status de loucura rege isso
    - implementar acerto crítico - dá o dobro de dano // status de precisão
    - cura/poções // ter uma terceira ação CURA que tem uma thread própria com menos chance de ser executada que ATAQUE e DEFESA (tanto por ter menos threads dela por jogador
        quanto por ter um rand dentro da função que determina se vai realmente curar ou não)
    - o jogo ser um jogo de apostas!!!!! Com nomes e descrições de cada jogador e stats pra apostar

*REGRAS DO JOGO*
    - As threads são ações: ATAQUE / DEFESA / ESQUIVA / DESARME
    - Vão rolando ações de jogadores aleatórios
        * Ataques contra outros jogadores (duram 0.25 segundos)
        * Defesas (duram 1 segundo)
        * Esquivas (duram 0.5 segundos)
        * Desarme (dura 0.25 segunodos)
    - Cada jogador tem vida e dano (pode ter agilidade/iniciativa, para ver quem começa primeiro (ordem de liberação das threads))
    - 2 podem atacar um defensor, que se defende dos dois
*/

#define A 2 // numero de threads de ataque POR JOGADOR
#define D 1 // numero de threads de defesa POR JOGADOR
#define D_A 2 // de quantos atacantes um defensor pode se defender ao mesmo tempo (não estou usando essa variável)
#define P 5 // total de jogadores

// TEMPOS
double tempo_defesa = 1.5;
double tempo_descanso = 2.25;
double tempo_penalti = 1.5;
double tempo_acrescimo = 1.5;

int vida_minima = 65; // minimo 65
int vida_maxima = 100; // maximo 99
int dano_minimo = 20; // minimo 20
int dano_maximo = 35; // maximo 34
int iniciativa_max = 100;
int agilidade_max = 100;
int inteligencia_max = 100;
int chance_maxima_esq = 20; // 20% de chance máxima de esquiva
int chance_maxima_des = 10; // 10% de chance máxima de desarme

// variaveis do problema
int estado[P]; // vetor com os estados dos jogadores:
// 0 = INATIVO // 1 = ATACANDO // 2 = EM FORMAÇÃO DE DEFESA
int atacado_por[P]; // por quantos jogadores um jogador está sendo atacado
int vivo[P]; // 0 = morto // 1 = vivo
int aposta = 0;

int threads_ativas = 0; // MAX = (A + D) * P
int espera = 0; // controle para as threads não começarem uma de cada vez
int acabado = 0;

struct Jogador {
    int id_jogador;
    char nome[50];
    char descricao[100];
    int vida;
    int vida_original;
    int dano;
    int iniciativa;
    int agilidade;
    int inteligencia;
};

// cria a struct a ser passada para as threads: a lista de jogadores mais o jogador a que a thread pertence e o id da thread
struct Passa {
    int id_jogador;
    int id_thread;
};

// variaveis para sincronizacao
pthread_mutex_t mutex;
pthread_cond_t conds[P];

struct Jogador lista_jogs[P]; // lista global de jogadores

/* msleep(): Sleep for the requested number of milliseconds. */
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

void acabou () {
    int mortos = 0; int o_vivo = -1;
    for(int k=0; k<P; k++) {
        if (vivo[k] == 0) {
            mortos++;
        }
        else {
            o_vivo = k;
        }
    }
    if (mortos == P - 1) {
        acabado = 1;
        printf("                                  ----- FIM DO JOGO - VITÓRIA DE %s (J%d) -----\n\n", lista_jogs[o_vivo].nome, o_vivo);
        if (o_vivo == aposta) {
            printf("Parabéns viajante! Eu não botei muita fé em você, mas parece que você apostou no cavalo certo. Aqui está seu prêmio, a lendária corrente do poder!\n\n");
            printf("                                  \\\\                         //     \n");
            printf("                                  @@@@@@@@@@@@[IOI]@@@@@@@@@@@@\n");
            printf("                                  //                         \\\\     \n\n");
            printf("Use-a com sabedoria... Até a próxima, viajante!\n");
        }
        else {
            printf("Bem, como eu imaginei, parece que você não entende muito sobre luta. A vitória de %s era clara!", lista_jogs[o_vivo].nome);
            printf("Mas olhe, estude as artes marciais com mais dedicação e quem sabe da próxima vez você não leva a lendária corrente do poder. Até lá!\n");
        }
        exit(0);
    }
}

int IniciaAtaque (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    while (espera == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        // printf("--- LIBEROU O ATAQUE DO J%d (espera)\n\n", id_jogador);
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (ataque)
    while(estado[id_jogador] != 0) {
        // printf("J%d queria atacar com A%d mas está fazendo outra ação\n\n", id_jogador, id_thread);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        // printf("--- LIBEROU O ATAQUE DO J%d (inativo)\n\n", id_jogador);
        // printf("J%d teve seu ataque A%d desbloqueado\n\n", id_jogador, id_thread);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - %s (J%d) MORTO ATACOU!\n\n", lista_jogs[id_jogador].nome, id_jogador);
    }

    // escolha do alvo
    int idAlvo = 0; int cont = 0;
    while(cont < 100) {
        idAlvo = rand() % P;
        // printf("ALVO DE J%d ESCOLHIDO: J%d\n\n", id_jogador, idAlvo);
        if((idAlvo != id_jogador) && (vivo[idAlvo] == 1) && (atacado_por[idAlvo] < 2)) break;
        // else if (atacado_por[idAlvo] >= 2) printf("J%d queria atacar J%d mas este já está sendo atacado por 2 jogadores\n\n", id_jogador, idAlvo);
        // else if (vivo[idAlvo] == 0) printf("J%d queria atacar J%d mas este está morto\n\n", id_jogador, idAlvo);
        cont++;
    }

    atacado_por[idAlvo]++;
    // printf("J%d esta sendo atacado por %d jogadores - novo: J%d\n\n", idAlvo, atacado_por[idAlvo], id_jogador);
    
    estado[id_jogador] = 1;
    pthread_mutex_unlock(&mutex);
    return idAlvo;
}

int ExecutaAtaque (int id_jogador, int idAlvo) {
    // sleep(0.25); // tempo para realizar o ataque
    pthread_mutex_lock(&mutex);

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        // printf("J%d caiu no wait do jogador morto\n\n", id_jogador);
        atacado_por[idAlvo]--;
        // printf("J%d esta sendo atacado por %d jogadores - J%d MORREU\n\n", idAlvo, atacado_por[idAlvo], id_jogador);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - J%d MORTO ATACOU!\n\n", id_jogador);
    }

    if (estado[idAlvo] == 2) {
    printf("J%d ) <=====|=o J%d | %s (J%d) SE DEFENDEU DE %s (J%d)\n\n", idAlvo, id_jogador, lista_jogs[idAlvo].nome, idAlvo, lista_jogs[id_jogador].nome, id_jogador);
    }
    if (atacado_por[idAlvo] > 2) {
        printf("%s (J%d) ERROU, ATACOU %s (J%d) QUE JÁ ESTAVA SENDO ATACADO POR 2!\n\n", lista_jogs[id_jogador].nome, id_jogador, lista_jogs[idAlvo].nome, idAlvo);
        sleep(tempo_penalti); // penalti
    }

    if (vivo[idAlvo] == 0) {
    printf("%s (J%d) ERROU, ATACOU %s (J%d) QUE ESTÁ MORTO!\n\n", lista_jogs[id_jogador].nome, id_jogador, lista_jogs[idAlvo].nome, idAlvo);
    sleep(tempo_penalti); // penalti
    }

    int metrica_esquiva = rand() % 500; int esquivou = 0;
    // (int)(agilidade_max/(chance_maxima_esq/100));
    if (metrica_esquiva > lista_jogs[idAlvo].agilidade || estado[idAlvo] == 2) {
        esquivou = 0; // não esquivou
    }
    else {
        esquivou = 1; // esquivou
        printf("J%d\n^^ <=====|=o J%d | %s (J%d) ESQUIVOU DO ATAQUE DE %s (J%d)\n\n",
            idAlvo, id_jogador, lista_jogs[idAlvo].nome, idAlvo, lista_jogs[id_jogador].nome, id_jogador);
    }

    int metrica_desarme = rand() % 200; int desarmou = 0;
    // (int)(agilidade_max/(chance_maxima_esq/100));
    if (metrica_desarme > lista_jogs[idAlvo].inteligencia || estado[idAlvo] == 2) {
        desarmou = 0; // não desarmou
    }
    else {
        desarmou = 1; // desarmou
        printf("J%d <==\\ \\==|=o J%d | %s (J%d) DESARMOU %s (J%d)\n\n",
            idAlvo, id_jogador, lista_jogs[idAlvo].nome, idAlvo, lista_jogs[id_jogador].nome, id_jogador);
    }

    if ((estado[idAlvo] == 0 || estado[idAlvo] == 1) && vivo[idAlvo] != 0 && atacado_por[idAlvo] <= 2 && esquivou == 0 && desarmou == 0) {
        printf("J%d o=|=====> J%d (%dD) | %s (J%d) ATINGIU %s (J%d)\n",
            id_jogador, idAlvo, lista_jogs[id_jogador].dano, lista_jogs[id_jogador].nome, id_jogador, lista_jogs[idAlvo].nome, idAlvo);

        lista_jogs[idAlvo].vida -= lista_jogs[id_jogador].dano;
        printf("Vida de %s (J%d): (%d/%d)\n\n", lista_jogs[idAlvo].nome, idAlvo, lista_jogs[idAlvo].vida, lista_jogs[idAlvo].vida_original);

        if (lista_jogs[idAlvo].vida <= 0) {
            printf("J%d o=|=====> X J%d | %s (J%d) MORREU\n\n", id_jogador, idAlvo, lista_jogs[idAlvo].nome, idAlvo);
            vivo[idAlvo] = 0;
            printf("// VIVOS:\n");
            for (int v=0; v<P; v++) {
                if (vivo[v] == 1) {
                    printf("// %s (J%d)\n", lista_jogs[v].nome, v);
                }
            }
            puts("");
        }
    }
    pthread_mutex_unlock(&mutex);
    return(desarmou);
}

void FimAtaque (int id_jogador, int idAlvo, int desarmou) {
    pthread_mutex_lock(&mutex);
    // printf("J%d terminou de atacar\n\n", id_jogador);
    atacado_por[idAlvo]--;
    // printf("J%d esta sendo atacado por %d jogadores\n\n", idAlvo, atacado_por[idAlvo]);
    estado[id_jogador] = 0;

    acabou();

    if (desarmou == 1) {
        sleep(tempo_acrescimo + tempo_descanso); //tempo de descanso extra para quem foi desarmado
    }
    else {
        sleep(tempo_descanso); //tempo de descanso
    }
    pthread_cond_signal(&conds[id_jogador]);
    pthread_cond_wait(&conds[id_jogador], &mutex);
    // printf("--- LIBEROU O ATAQUE DO J%d (normal)\n\n", id_jogador);
    pthread_mutex_unlock(&mutex);
}

void IniciaDefesa (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    while (espera == 0) {
        if (id_thread == (A + D) * P - 1) {
            espera = 1;
        }
        pthread_cond_wait(&conds[id_jogador], &mutex);
        // printf("--- LIBEROU A DEFESA DO J%d (espera)\n\n", id_jogador);
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (defesa)
    while(estado[id_jogador] != 0) {
        // printf("J%d queria defender com D%d mas está fazendo outra ação\n\n", id_jogador, id_thread);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        // printf("--- LIBEROU A DEFESA DO J%d (inativo)\n\n", id_jogador);
        // printf("J%d teve sua defesa D%d desbloqueada\n\n", id_jogador, id_thread);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - J%d MORTO DEFENDEU!\n\n", id_jogador);
    }

    // printf("J%d quer defender\n\n", id_jogador);
    estado[id_jogador] = 2;
    pthread_mutex_unlock(&mutex);
}

void FimDefesa (int id_jogador) {
    pthread_mutex_lock(&mutex);
    printf("%s (J%d) ABAIXOU O ESCUDO\n\n", lista_jogs[id_jogador].nome, id_jogador);
    estado[id_jogador] = 0;

    // acabou();

    sleep(tempo_descanso); // tempo de descanso
    pthread_cond_signal(&conds[id_jogador]);
    pthread_cond_wait(&conds[id_jogador], &mutex);
    // printf("--- LIBEROU A DEFESA DO J%d (normal)\n\n", id_jogador);
    pthread_mutex_unlock(&mutex);
}

// thread de ataque
void * atacante (void * arg) {
    struct Passa *passado = (struct Passa *) arg;
    int id_jogador = passado->id_jogador;
    int id_thread = passado->id_thread;

    while(1) {
        while (vivo[id_jogador] == 0) {
            // printf("J%d caiu no wait do jogador morto\n\n", id_jogador);
            pthread_cond_wait(&conds[id_jogador], &mutex);
            printf("ERRO CRÍTICO - J%d MORTO ATACOU!\n\n", id_jogador);
        }

        int idAlvo = IniciaAtaque(id_jogador, id_thread);
        if (acabado == 1) {
            printf("J%d iria atacar mas ja acabou o jogo\n\n", id_jogador);
            break;
        }
        // else {
        //     printf("J%d está atacando J%d\n\n", id_jogador, idAlvo);
        // }
        int desarmou = ExecutaAtaque(id_jogador, idAlvo);
        FimAtaque(id_jogador, idAlvo, desarmou);
    } 
    free(arg);
    pthread_exit(NULL);
}

// thread de defesa
void * defensor (void * arg) {
    struct Passa *passado = (struct Passa *) arg;
    int id_jogador = passado->id_jogador;
    int id_thread = passado->id_thread;

    while(1) {
        while (vivo[id_jogador] == 0) {
            pthread_cond_wait(&conds[id_jogador], &mutex);
            printf("ERRO CRÍTICO - J%d MORTO DEFENDEU!\n\n", id_jogador);
        }

        if (acabado == 1) {
            printf("J%d iria defender mas ja acabou o jogo\n\n", id_jogador);
            break;
        }
        IniciaDefesa(id_jogador, id_thread);
        printf("%s (J%d) LEVANTOU O ESCUDO\n\n", lista_jogs[id_jogador].nome, id_jogador);
        sleep(tempo_defesa); //tempo de defesa
        FimDefesa(id_jogador);
    } 
    free(arg);
    pthread_exit(NULL);
}

// funcao principal
int main(void) {
    // identificadores das threads
    pthread_t tid[(A + D) * P];
    // int id[(A + D) * P];

    // inicializa as variaveis de sincronizacao
    printf("Inicializando as variaveis de sincronizacao\n");
    pthread_mutex_init(&mutex, NULL);
    for(int k=0; k<P; k++) {
        pthread_cond_init(&conds[k], NULL);
    }
    srand(time(NULL));

    // cria os jogadores e seus status
    printf("Criando os jogadores e seus status\n\n");
    srand(time(NULL));
    for(int i=0; i<P; i++){
        lista_jogs[i].id_jogador = i;
        geraNome(lista_jogs[i].nome);
        geraDesc(lista_jogs[i].descricao);
        lista_jogs[i].vida = vida_minima + (rand() % (vida_maxima - vida_minima));
        lista_jogs[i].vida_original = lista_jogs[i].vida;
        lista_jogs[i].dano = dano_minimo + (rand() % (dano_maximo - dano_minimo));
        lista_jogs[i].iniciativa = rand() % iniciativa_max;
        lista_jogs[i].agilidade = rand() % agilidade_max;
        lista_jogs[i].inteligencia = rand() % inteligencia_max;
        estado[i] = 0;
        vivo[i] = 1;
        atacado_por[i] = 0;
        printf("Jogador/a %d: ", i);
        puts(lista_jogs[i].nome);
        puts(lista_jogs[i].descricao);
        printf("VIDA: (%d/%d) / DANO: %d / INICIATIVA: %d / AGILIDADE: %d / INTELIGÊNCIA: %d\n\n",
            lista_jogs[i].vida, vida_maxima, lista_jogs[i].dano, lista_jogs[i].iniciativa, lista_jogs[i].agilidade, lista_jogs[i].inteligencia);
    }

    printf("Olá, viajante. Você se encontra na casa de apostas da CORRENTE DO PODER, o maior campeonato de luta das redondezas.\n");
    printf("Em qual dos nosssos exímios lutadores você gostaria de apostar hoje? (escolha pelo número) ");
    scanf("%d", &aposta);
    printf("\nHmm... %s! Ótima escolha. Vamos à luta!\n\n", lista_jogs[aposta].nome);
    sleep(2.5);

    struct Passa passas[P][A+D];
    
    // cria as threads atacantes
    for(int j=0; j < P; j++) {
        for(int i=0; i < A; i++) {
            // id[(j * A) + i] = (j * A) + i;
            passas[j][i].id_jogador = j;
            passas[j][i].id_thread = (j * A) + i;
            printf("Criando thread A%d - J%d\n", (j * A) + i, j);
            if(pthread_create(&tid[(j * A) + i], NULL, atacante, (void *) &passas[j][i])) exit(-1);
            threads_ativas++;
            //printf("Threads ativas = %d\n\n", threads_ativas);
            //printf("Thread atacante %d criada\n\n", (j * A) + i);
            msleep(500);
        } 
    }

    // cria as threads defensoras
    for(int j=0; j < P; j++) {
        for(int i=0; i < D; i++) {
            // id[(A * P) + (j * D) + i] = (A * P) + (j * D) + i;
            passas[j][A+i].id_jogador = j;
            passas[j][A+i].id_thread = (A * P) + (j * D) + i;
            printf("Criando thread D%d - J%d\n", (A * P) + (j * D) + i, j);
            if(pthread_create(&tid[(A * P) + (j * D) + i], NULL, defensor, (void *) &passas[j][A+i])) exit(-1);
            threads_ativas++;
            //printf("Threads ativas = %d\n\n", threads_ativas);
            //printf("Thread defensora %d criada\n\n", (A * P) + (j * D) + i);
            msleep(500);
        }
    }
    puts("");
    printf("A luta irá começar em 3...\n");
    sleep(1.5);
    printf("2...\n");
    sleep(1.2);
    printf("1...\n");
    sleep(1.2);

    int iniciativas[P];
    for (int k = 0; k < P; k++) {
        iniciativas[k] = lista_jogs[k].iniciativa;
    }

    // criando uma lista das iniciativas para sabermos quem começa primeiro
    int ordem[P];
    int maior = -1; int maior_ind = 0; int j = 0;
    for (int i = 0; i < P; i++) {
        maior = 0;
        maior_ind = 0;
        for (j = 0; j < P; j++) {
            if (iniciativas[j] >= maior) {
                maior = iniciativas[j];
                maior_ind = j;
            }
        }
        ordem[i] = maior_ind;
        iniciativas[maior_ind] = -1;
    }

    sleep(0.25);
    if (espera == 1) {
        puts("");
        for (int pos=0; pos < P; pos++) {
        printf("Threads de %s (J%d) liberadas!\n\n", lista_jogs[ordem[pos]].nome, ordem[pos]);
        pthread_cond_signal(&conds[ordem[pos]]);
        sleep(1);
        }
    }

    // espera todas as threads terminarem
    for (int t=0; t<A+D; t++) {
        if (pthread_join(tid[t], NULL)) {
            printf("--ERRO: pthread_join()\n\n"); exit(-1); 
        } 
    }

    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
    return 0;
}
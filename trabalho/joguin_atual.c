#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "nomes.h"
#include <errno.h> 

#define A 2 // numero de threads de ataque POR JOGADOR
#define D 1 // numero de threads de defesa POR JOGADOR
#define P 5 // total de jogadores

// tempos
double tempo_defesa = 1.5; // tempo durante o qual um jogador fica em estado de defesa
double tempo_descanso = 2.25; // tempo de descanso após um jogador executar uma ação
double tempo_penalti = 1.5; // tempo de penalti após um jogador cometer um erro (atacar um outro jogador morto ou que já está sendo atacado por outros 2 jogadores)
double tempo_atordoa = 1.5; // tempo que um jogador fica atordoado após ser desarmado

int vida_minima = 65; // minimo 65
int vida_maxima = 100; // maximo 99
int dano_minimo = 20; // minimo 20
int dano_maximo = 35; // maximo 34
int iniciativa_max = 100;
int agilidade_max = 100;
int inteligencia_max = 100;

// vetor com os estados dos jogadores
int estado[P]; // 0 = INATIVO // 1 = ATACANDO // 2 = EM FORMAÇÃO DE DEFESA

int atacado_por[P]; // por quantos jogadores um jogador está sendo atacado
int vivo[P]; // 0 = MORTO // 1 = VIVO
int aposta = 0; // em qual jogador o usuário apostou
int espera = 0; // controle para as threads não começarem uma de cada vez
int acabado = 0; // vira 1 quando o jogo acaba

// struct que guarda as informações de cada jogador
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

// struct a ser passada para as threads: o id do jogador a que a thread pertence e o id da thread em si
struct Passa {
    int id_jogador;
    int id_thread;
};

// variaveis para sincronizacao
pthread_mutex_t mutex;
pthread_cond_t conds[P];

struct Jogador lista_jogs[P]; // lista global de jogadores

// msleep(): dá um sleep pela quantidade requisitada de milissegundos
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

// é chamada quando ao fim de cada ataque para checar se o jogo acabou. se sim, emite a devida mensagem e encerra o programa
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
            printf("Parabéns viajante! Eu não botei muita fé em você, mas parece que você apostou no cavalo certo. Aqui está seu prêmio, a lendária CORRENTE DO PODER!\n\n");
            printf("                                  \\\\                         //     \n");
            printf("                                  @@@@@@@@@@@@[IOI]@@@@@@@@@@@@\n");
            printf("                                  //                         \\\\     \n\n");
            printf("Use-a com sabedoria... Até a próxima, viajante!\n");
        }
        else {
            printf("Bem, como eu imaginei, parece que você não entende muito sobre luta. A vitória de %s era clara!\n", lista_jogs[o_vivo].nome);
            printf("Mas olhe, estude as artes marciais com mais dedicação e quem sabe da próxima vez você não leva a lendária CORRENTE DO PODER. Até lá!\n");
        }
        exit(0);
    }
}

// é chamada pela thread atacante para iniciar o ataque
int IniciaAtaque (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    // a thread não sai desse wait até todas iniciarem de fato (controlado pela main)
    while (espera == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (ataque)
    while(estado[id_jogador] != 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - %s (J%d) MORTO ATACOU!\n\n", lista_jogs[id_jogador].nome, id_jogador);
    }

    // escolha do alvo para o ataque
    int idAlvo = 0; int cont = 0;
    while(cont < 100) {
        idAlvo = rand() % P;
        // o alvo só é escolhido se não for o próprio jogador, não estiver morto e não estiver sendo atacado por 2 outros jogadores
        if((idAlvo != id_jogador) && (vivo[idAlvo] == 1) && (atacado_por[idAlvo] < 2)) break;
        cont++;
    }

    atacado_por[idAlvo]++;
    estado[id_jogador] = 1;
    pthread_mutex_unlock(&mutex);
    return idAlvo;
}

// é chamada pela thread atacante para executar de fato o ataque
int ExecutaAtaque (int id_jogador, int idAlvo) {
    pthread_mutex_lock(&mutex);

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        atacado_por[idAlvo]--;
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

    /* metrica_esquiva é um número aleatório de 0 a 500. temos que um jogador realiza a esquiva quando metrica_esquiva é maior que sua agilidade,
       ou seja, quanto maior sua agilidade, maior sua chance de esquivar. como a agilidade máxima é 100, ele/a teria uma chance máxima de esquivar
       de 20%. (100/500) */
    int metrica_esquiva = rand() % 501; int esquivou = 0;
    if (metrica_esquiva > lista_jogs[idAlvo].agilidade || estado[idAlvo] == 2) {
        esquivou = 0; // não esquivou
    }
    else {
        esquivou = 1; // esquivou
        printf("J%d\n^^ <=====|=o J%d | %s (J%d) ESQUIVOU DO ATAQUE DE %s (J%d)\n\n",
            idAlvo, id_jogador, lista_jogs[idAlvo].nome, idAlvo, lista_jogs[id_jogador].nome, id_jogador);
    }

    /* metrica_desarme é um número aleatório de 0 a 1000. temos que um jogador realiza o desarme quando metrica_desarme é maior que sua inteligência,
       ou seja, quanto maior sua inteligência, maior sua chance de desarmar. como a inteligência máxima é 100, ele/a teria uma chance máxima de desarmar
       de 10%. (100/1000) */
    int metrica_desarme = rand() % 1001; int desarmou = 0;
    if (metrica_desarme > lista_jogs[idAlvo].inteligencia || estado[idAlvo] == 2) {
        desarmou = 0; // não desarmou
    }
    else {
        desarmou = 1; // desarmou
        printf("J%d <==\\ \\==|=o J%d | %s (J%d) DESARMOU %s (J%d)\n\n",
            idAlvo, id_jogador, lista_jogs[idAlvo].nome, idAlvo, lista_jogs[id_jogador].nome, id_jogador);
    }

    /* o jogador realiza seu ataque com sucesso se o alvo não estiver se defendendo, não estiver morto, não estiver sendo atacado por 2 outros jogadores,
       e não tiver esquivado ou desarmado seu ataque */
    if ((estado[idAlvo] == 0 || estado[idAlvo] == 1) && vivo[idAlvo] != 0 && atacado_por[idAlvo] <= 2 && esquivou == 0 && desarmou == 0) {
        printf("J%d o=|=====> J%d (%dD) | %s (J%d) ATINGIU %s (J%d)\n",
            id_jogador, idAlvo, lista_jogs[id_jogador].dano, lista_jogs[id_jogador].nome, id_jogador, lista_jogs[idAlvo].nome, idAlvo);

        lista_jogs[idAlvo].vida -= lista_jogs[id_jogador].dano; // tiramos da vida do alvo o dano do jogador
        printf("Vida de %s (J%d): (%d/%d)\n\n", lista_jogs[idAlvo].nome, idAlvo, lista_jogs[idAlvo].vida, lista_jogs[idAlvo].vida_original);

        if (lista_jogs[idAlvo].vida <= 0) {
            printf("J%d o=|=====> X J%d | %s (J%d) MORREU\n\n", id_jogador, idAlvo, lista_jogs[idAlvo].nome, idAlvo);
            vivo[idAlvo] = 0; // o alvo está morto

            // mostrando quais jogadores ainda estão vivos/as
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

// é chamada pela thread atacante para finalizar o ataque
void FimAtaque (int id_jogador, int idAlvo, int desarmou) {
    pthread_mutex_lock(&mutex);
    atacado_por[idAlvo]--;
    estado[id_jogador] = 0; // o/a jogador/a volta ao estado inativo

    acabou(); // checamos se o jogo já acabou (se só há 1 jogador/a vivo/a)

    if (desarmou == 1) {
        sleep(tempo_atordoa + tempo_descanso); //tempo de descanso extra para quem foi desarmado
    }
    else {
        sleep(tempo_descanso); //tempo de descanso normal
    }
    pthread_cond_signal(&conds[id_jogador]); // damos signal para a próxima ação do mesmo jogador
    pthread_cond_wait(&conds[id_jogador], &mutex); // deixamos essa thread de ataque do jogador esperando até ser chamada novamente
    pthread_mutex_unlock(&mutex);
}

void IniciaDefesa (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    // a thread não sai desse wait até todas iniciarem de fato (controlado pela main)
    while (espera == 0) {
        if (id_thread == (A + D) * P - 1) { // se esta for a última thread a ser criada
            espera = 1;
        }
        pthread_cond_wait(&conds[id_jogador], &mutex);
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (defesa)
    while(estado[id_jogador] != 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - J%d MORTO DEFENDEU!\n\n", id_jogador);
    }

    estado[id_jogador] = 2;
    pthread_mutex_unlock(&mutex);
}

void FimDefesa (int id_jogador) {
    pthread_mutex_lock(&mutex);
    printf("%s (J%d) ABAIXOU O ESCUDO\n\n", lista_jogs[id_jogador].nome, id_jogador);
    estado[id_jogador] = 0; // o/a jogador/a volta ao estado inativo

    sleep(tempo_descanso); // tempo de descanso
    pthread_cond_signal(&conds[id_jogador]); // damos signal para a próxima ação do mesmo jogador
    pthread_cond_wait(&conds[id_jogador], &mutex); // deixamos essa thread de defesa do jogador esperando até ser chamada novamente
    pthread_mutex_unlock(&mutex);
}

// thread de ataque
void * atacante (void * arg) {
    // jogando os valores passados para a thread para variáveis locais
    struct Passa *passado = (struct Passa *) arg;
    int id_jogador = passado->id_jogador;
    int id_thread = passado->id_thread;

    while(1) {
        // se o jogador tiver morrido dá um wait eterno
        while (vivo[id_jogador] == 0) {
            pthread_cond_wait(&conds[id_jogador], &mutex);
            printf("ERRO CRÍTICO - J%d MORTO ATACOU!\n\n", id_jogador);
        }

        int idAlvo = IniciaAtaque(id_jogador, id_thread);
        int desarmou = ExecutaAtaque(id_jogador, idAlvo);
        FimAtaque(id_jogador, idAlvo, desarmou);
    } 
    free(arg);
    pthread_exit(NULL);
}

// thread de defesa
void * defensor (void * arg) {
    // jogando os valores passados para a thread para variáveis locais
    struct Passa *passado = (struct Passa *) arg;
    int id_jogador = passado->id_jogador;
    int id_thread = passado->id_thread;

    while(1) {
        // se o jogador tiver morrido dá um wait eterno
        while (vivo[id_jogador] == 0) {
            pthread_cond_wait(&conds[id_jogador], &mutex);
            printf("ERRO CRÍTICO - J%d MORTO DEFENDEU!\n\n", id_jogador);
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

    // inicializa as variaveis de sincronizacao
    printf("Inicializando as variaveis de sincronizacao\n");
    pthread_mutex_init(&mutex, NULL);
    for(int k=0; k<P; k++) {
        pthread_cond_init(&conds[k], NULL); // há um cond para cada jogador
    }
    srand(time(NULL));
    msleep(1250);

    // cria os jogadores e seus status
    printf("Criando os jogadores e seus status\n\n");
    msleep(1250);
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

    printf("--------------------------------------------------------------------------------------------------------------------------------------\n\n");
    printf("Olá, viajante. Você se encontra na casa de apostas da CORRENTE DO PODER, o maior campeonato de luta das redondezas!\n");
    printf("Nossos lutadores destemidos irão brigar até sobrar apenas um/a vivo/a no ringue! O seu papel é apostar em quem vai ser essa pessoa.\n");
    printf("O seu prêmio, se você acertar? A lendária CORRENTE DO PODER, artefato de grandeza inimaginável!\n\n");
    printf("Agora vamos às regras:\n");
    printf("Os jogadores podem executar ações de ATAQUE e DEFESA, e podem reagir a um ataque com uma ESQUIVA ou um DESARME.\n");
    printf("Os jogadores têm 5 status:\n");
    printf("- VIDA: A quantidade de pontos de vida de um jogador\n");
    printf("- DANO: O dano que um jogador desfere a um jogador alvo no momento de ataque (quantos pontos de vida são retirados do alvo)\n");
    printf("- INICIATIVA: Determina a ordem que cada jogador começa a executar ações (a ordem em que as threads são liberadas)\n");
    printf("- AGILIDADE: Determina a chance de um jogador de esquivar de um ataque vindo em sua direção\n");
    printf("- INTELIGÊNCIA: Determina a chance de um jogador de desarmar um jogador que está lhe atacando, o deixando atordoado\n\n");
    printf("Agora que você entende as regras do jogo, diga: em qual de nosssos exímios lutadores você gostaria de apostar hoje? (escolha pelo número) ");
    scanf("%d", &aposta);
    printf("\nHmm... %s! Ótima escolha. Vamos à luta!\n\n", lista_jogs[aposta].nome);
    sleep(2.5);

    struct Passa passas[P][A+D];
    
    // cria as threads atacantes
    for(int j=0; j < P; j++) {
        for(int i=0; i < A; i++) {
            passas[j][i].id_jogador = j;
            passas[j][i].id_thread = (j * A) + i;
            printf("Criando thread de ataque A%d de J%d\n", (j * A) + i, j);
            if(pthread_create(&tid[(j * A) + i], NULL, atacante, (void *) &passas[j][i])) exit(-1);
            msleep(500);
        } 
    }

    // cria as threads defensoras
    for(int j=0; j < P; j++) {
        for(int i=0; i < D; i++) {
            passas[j][A+i].id_jogador = j;
            passas[j][A+i].id_thread = (A * P) + (j * D) + i;
            printf("Criando thread de defesa D%d de J%d\n", (A * P) + (j * D) + i, j);
            if(pthread_create(&tid[(A * P) + (j * D) + i], NULL, defensor, (void *) &passas[j][A+i])) exit(-1);
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

    // criando uma lista das iniciativas para sabermos quem começa primeiro
    int iniciativas[P];
    for (int k = 0; k < P; k++) {
        iniciativas[k] = lista_jogs[k].iniciativa;
    }

    // ordenando a lista na lista ordem
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
    // quando espera == 1 (quando a última thread iniciar) liberamos uma das threads de cada um dos jogadores na ordem de iniciativa (maior até menor)
    if (espera == 1) {
        puts("");
        for (int pos=0; pos < P; pos++) {
        printf("Thread inicial de %s (J%d) liberadas!\n\n", lista_jogs[ordem[pos]].nome, ordem[pos]);
        pthread_cond_signal(&conds[ordem[pos]]);
        msleep(2250);
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
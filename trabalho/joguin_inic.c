#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<math.h>

/*
NOTAS SILVANA
    - sempre se ligar em quando as threads estão lendo informações globais para não haver problema de corrida
    - ver se uma thread não está alterando algo de outra thread
    - ter bem definidas as situações em que uma thread se bloqueia
    - se ligar nas condições para as threads se bloquearem e só der signal quando de fato as condições mudarem
    - pensar que o signal sinaliza a thread independente da condição que foi usada para entrar no wait

PRÓXIMOS PASSOS
    - ajeitar o erro do jogador que ataca o que já está sendo atacado por 2 e que ataca quem tá morto (colocar penalti)
    - JOGADORES ESTÃO ATACANDO E DEFENDENDO DEPOIS DE MORREREM DEPOIS DO JOGO TERMINAR // meio que foi!
    - JOGADORES FAZENDO COISAS ANTES DE LIBERAR AS THREADS CARALHO // hmm vamos ignorar por enquanto kk
    - ataques/defesas demorando demais
    - milhões de defesas seguidas do mesmo atacante e mesmo defensor

PASSOS EVENTUAIS
    - modularizar tudo em funções // não é necessário
    - tirar vantagens/desvantagens de jogadores por terem um certo número (começarem antes, sempre serem atacados, etc.)
        * colocar iniciativa - o com maior iniciativa começa antes
    - colocar o primeiro jogador como 1 // vamos deixar o vetor com a posição 0 vazia
    - balancear os valores de vida e dano, e os tempos // ir testando bastante
    - implementar novas ações
        * ESQUIVA // baseado num status de agilidade x/10, que influencia a chance de um ataque recebido não dar certo
        * DESARME // baseado num status de inteligência y/10, que influencia a chance de um ataque recebido não dar certo e ainda atordoar o atacante
    
SE DER TEMPO
    - superpoder: jogador ataca 2 outros ao mesmo tempo
    - colocar o suicídio com uma chance bem menor de acontecer (tipo um critical fail) // status de loucura rege isso
    - implementar acerto crítico - dá o dobro de dano // status de precisão
    - cura/poções // ter uma terceira ação CURA que tem uma thread própria com menos chance de ser executada que ATAQUE e DEFESA (tanto por ter menos threads dela por jogador
        quanto por ter um rand dentro da função que determina se vai realmente curar ou não)
    - o jogo ser um jogo de apostas!!!!! Com nomes e descrições de cada jogador e stats pra apostar

*REGRAS DO JOGO*
    - As threads são ações: ATAQUE / DEFESA / ESQUIVA? / DESARME?
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
double tempo_defesa = 0.5;
double tempo_descanso = 0.25;
double tempo_penalti = 0.25;

int vida_minima = 60; // minimo 60
int vida_maxima = 100; // maximo 99
int dano_minimo = 15; // minimo 15
int dano_maximo = 30; // maximo 29
int iniciativa_max = 100;
int agilidade_max = 100;
int inteligencia_max = 100;

// variaveis do problema
int estado[P]; // vetor com os estados dos jogadores:
// 0 = INATIVO // 1 = ATACANDO // 2 = EM FORMAÇÃO DE DEFESA
int atacado_por[P]; // por quantos jogadores um jogador está sendo atacado
int vivo[P]; // 0 = morto // 1 = vivo

int threads_ativas = 0; // MAX = (A + D) * P
int espera = 0; // controle para as threads não começarem uma de cada vez
int acabado = 0;

struct Jogador {
    int id_jogador;
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
        printf("----- ACABOU O JOGO - J%d É O GRANDE VENCEDOR ----- \n", o_vivo);
        // vivo[o_vivo] = 0;
        exit(0);
    }
}

int IniciaAtaque (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    while (espera == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (ataque)
    while(estado[id_jogador] != 0) {
        // printf("J%d queria atacar com A%d mas está fazendo outra ação\n", id_jogador, id_thread);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        // printf("J%d teve seu ataque A%d desbloqueado\n", id_jogador, id_thread);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - J%d MORTO ATACOU!\n", id_jogador);
    }

    // escolha do alvo
    int idAlvo = 0; int cont = 0;
    while(cont < 100) {
        idAlvo = rand() % P;
        // if (vivo[idAlvo] == 1) printf("ALVO DE J%d ESCOLHIDO: J%d\n", id_jogador, idAlvo);
        if((idAlvo != id_jogador) && (vivo[idAlvo] == 1) && (atacado_por[idAlvo] < 2)) break;
        // else if (atacado_por[idAlvo] >= 2) printf("J%d queria atacar J%d mas este já está sendo atacado por 2 jogadores\n", id_jogador, idAlvo);
        // else if (vivo[idAlvo] == 0) printf("J%d queria atacar J%d mas este está morto\n", id_jogador, idAlvo);
        cont++;
    }

    atacado_por[idAlvo]++;
    // printf("J%d esta sendo atacado por %d jogadores - novo: J%d\n", idAlvo, atacado_por[idAlvo], id_jogador);
    
    estado[id_jogador] = 1;
    pthread_mutex_unlock(&mutex);
    return idAlvo;
}

void ExecutaAtaque (int id_jogador, int idAlvo) {
    // sleep(0.25); // tempo para realizar o ataque
    pthread_mutex_lock(&mutex);

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        // printf("J%d caiu no wait do jogador morto\n", id_jogador);
        atacado_por[idAlvo]--;
        // printf("J%d esta sendo atacado por %d jogadores - J%d MORREU\n", idAlvo, atacado_por[idAlvo], id_jogador);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - J%d MORTO ATACOU!\n", id_jogador);
    }

    if (estado[idAlvo] == 2) {
    printf("DEFESA - J%d ) <=====|=o J%d\n", idAlvo, id_jogador);
    }
    if (atacado_por[idAlvo] > 2) {
        printf("ERRO - J%d atacou J%d que já estava sendo atacado por 2\n", id_jogador, idAlvo);
        // penalti
    }

    if (vivo[idAlvo] == 0) {
    printf("ERRO - J%d atacou J%d que está morto!\n", id_jogador, idAlvo);
    // penalti
    }

    if ((estado[idAlvo] == 0 || estado[idAlvo] == 1) && vivo[idAlvo] != 0 && atacado_por[idAlvo] <= 2) {
        printf("J%d o=|=====> J%d (%dD)\n", id_jogador, idAlvo, lista_jogs[id_jogador].dano);
        lista_jogs[idAlvo].vida -= lista_jogs[id_jogador].dano;
        printf("J%d: (%d/%d)\n", idAlvo, lista_jogs[idAlvo].vida, lista_jogs[idAlvo].vida_original);

        if (lista_jogs[idAlvo].vida <= 0) {
            printf("MORTE - J%d o=|=====> J%d\n", id_jogador, idAlvo);
            vivo[idAlvo] = 0;
            printf("// VIVOS:\n");
            for (int v=0; v<P; v++) {
                if (vivo[v] == 1) {
                    printf("// J%d\n", v);
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);
}

void FimAtaque (int id_jogador, int idAlvo) {
    pthread_mutex_lock(&mutex);
    printf("J%d terminou de atacar\n", id_jogador);
    atacado_por[idAlvo]--;
    // printf("J%d esta sendo atacado por %d jogadores\n", idAlvo, atacado_por[idAlvo]);
    estado[id_jogador] = 0;

    acabou();

    sleep(tempo_descanso); //tempo de descanso
    pthread_cond_signal(&conds[id_jogador]);
    pthread_mutex_unlock(&mutex);
}

void IniciaDefesa (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    while (espera == 0) {
        if (id_thread == (A + D) * P - 1) {
            espera = 1;
        }
        pthread_cond_wait(&conds[id_jogador], &mutex);
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (defesa)
    while(estado[id_jogador] != 0) {
        // printf("J%d queria defender com D%d mas está fazendo outra ação\n", id_jogador, id_thread);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        // printf("J%d teve sua defesa D%d desbloqueada\n", id_jogador, id_thread);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (vivo[id_jogador] == 0) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("ERRO CRÍTICO - J%d MORTO DEFENDEU!\n", id_jogador);
    }

    printf("J%d quer defender\n", id_jogador);
    estado[id_jogador] = 2;
    pthread_mutex_unlock(&mutex);
}

void FimDefesa (int id_jogador) {
    pthread_mutex_lock(&mutex);
    printf("J%d terminou de defender\n", id_jogador);
    estado[id_jogador] = 0;

    // acabou();

    sleep(tempo_descanso); // tempo de descanso
    pthread_cond_signal(&conds[id_jogador]);
    pthread_mutex_unlock(&mutex);
}

// thread de ataque
void * atacante (void * arg) {
    struct Passa *passado = (struct Passa *) arg;
    int id_jogador = passado->id_jogador;
    int id_thread = passado->id_thread;

    while(1) {
        while (vivo[id_jogador] == 0) {
            // printf("J%d caiu no wait do jogador morto\n", id_jogador);
            pthread_cond_wait(&conds[id_jogador], &mutex);
            printf("ERRO CRÍTICO - J%d MORTO ATACOU!\n", id_jogador);
        }

        int idAlvo = IniciaAtaque(id_jogador, id_thread);
        if (acabado == 1) {
            printf("J%d iria atacar mas ja acabou o jogo\n", id_jogador);
            break;
        }
        else {
            printf("J%d está atacando J%d\n", id_jogador, idAlvo);
        }
        ExecutaAtaque(id_jogador, idAlvo);
        FimAtaque(id_jogador, idAlvo);
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
            printf("ERRO CRÍTICO - J%d MORTO DEFENDEU!\n", id_jogador);
        }

        if (acabado == 1) {
            printf("J%d iria defender mas ja acabou o jogo\n", id_jogador);
            break;
        }
        IniciaDefesa(id_jogador, id_thread);  
        printf("J%d está defendendo\n", id_jogador);
        // sleep(tempo_defesa); //tempo de defesa
        FimDefesa(id_jogador);
    } 
    free(arg);
    pthread_exit(NULL);
}

// funcao principal
int main(void) {
    // identificadores das threads
    pthread_t tid[(A + D) * P];
    int id[(A + D) * P];

    // inicializa as variaveis de sincronizacao
    printf("Inicializando as variaveis de sincronizacao\n");
    pthread_mutex_init(&mutex, NULL);
    for(int k=0; k<P; k++) {
        pthread_cond_init(&conds[k], NULL);
    }
    srand(time(NULL));

    // cria os jogadores e seus status
    printf("Criando os jogadores e seus status\n");
    srand(time(NULL));
    for(int i=0; i<P; i++){
        lista_jogs[i].id_jogador = i;
        lista_jogs[i].vida = vida_minima + (rand() % (vida_maxima - vida_minima));
        lista_jogs[i].vida_original = lista_jogs[i].vida;
        lista_jogs[i].dano = dano_minimo + (rand() % (dano_maximo - dano_minimo));
        lista_jogs[i].iniciativa = rand() % iniciativa_max;
        lista_jogs[i].agilidade = rand() % agilidade_max;
        lista_jogs[i].inteligencia = rand() % inteligencia_max;
        estado[i] = 0;
        vivo[i] = 1;
        atacado_por[i] = 0;
        printf("Jogador %d criado com VIDA: (%d/%d) / DANO: %d / INICIATIVA: %d / AGILIDADE: %d / INTELIGÊNCIA: %d\n",
             i, lista_jogs[i].vida, vida_maxima, lista_jogs[i].dano, lista_jogs[i].iniciativa, lista_jogs[i].agilidade, lista_jogs[i].inteligencia);
    }

    struct Passa passas[P][A+D];
    
    // cria as threads atacantes
    for(int j=0; j < P; j++) {
        for(int i=0; i < A; i++) {
            id[(j * A) + i] = (j * A) + i;
            passas[j][i].id_jogador = j;
            passas[j][i].id_thread = (j * A) + i;
            printf("Criando thread A%d - J%d\n", (j * A) + i, j);
            if(pthread_create(&tid[(j * A) + i], NULL, atacante, (void *) &passas[j][i])) exit(-1);
            threads_ativas++;
            //printf("Threads ativas = %d\n", threads_ativas);
            //printf("Thread atacante %d criada\n", (j * A) + i);
        } 
    }

    // cria as threads defensoras
    for(int j=0; j < P; j++) {
        for(int i=0; i < D; i++) {
            id[(A * P) + (j * D) + i] = (A * P) + (j * D) + i;
            passas[j][A+i].id_jogador = j;
            passas[j][A+i].id_thread = (A * P) + (j * D) + i;
            printf("Criando thread D%d - J%d\n", (A * P) + (j * D) + i, j);
            if(pthread_create(&tid[(A * P) + (j * D) + i], NULL, defensor, (void *) &passas[j][A+i])) exit(-1);
            threads_ativas++;
            //printf("Threads ativas = %d\n", threads_ativas);
            //printf("Thread defensora %d criada\n", (A * P) + (j * D) + i);
        }
    }

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
        for (int pos=0; pos < P; pos++) {
        printf("Threads do J%d liberadas!\n", ordem[pos]);
        pthread_cond_signal(&conds[ordem[pos]]);
        }
    }

    // espera todas as threads terminarem
    for (int t=0; t<A+D; t++) {
        if (pthread_join(tid[t], NULL)) {
            printf("--ERRO: pthread_join()\n"); exit(-1); 
        } 
    }

    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
    return 0;
}
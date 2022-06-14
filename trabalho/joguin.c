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
ESTADO ATUAL
    - vamo la ta bem bagunçado ainda kkkk mas pelo menos eu consegui:
        * fazer as threads só iniciarem depois de todas serem criadas
        * colocar as coisas da thread de ataque nas funções não thread (criei uma ExecutaAtaque) para algumas coisas
        serem feitas dentro do lock
PRÓXIMOS PASSOS
    - ajeitar o erro do jogador que ataca o que já está sendo atacado por 2
    - entender porque a vida tá sendo diminuída errado quando o ataque é bem sucedido
    - entender porque certos prints se repetem:
    - implementar melhor a morte
*REGRAS DO JOGO*
    - As threads são ações: ATAQUE / DEFESA / ESQUIVA? / DESARME?
    - Vão rolando ações de jogadores aleatórios
        * Ataques contra outros jogadores (duram 0.25 segundos)
        * Defesas (duram 1 segundo)
        * Esquivas (duram 0.5 segundos)
        * Desarme (dura 0.25 segunodos)
    - Cada jogador tem vida e dano (agilidade)
    - 2 podem atacar um defensor, que se defende dos dois
*/

#define A 2 // numero de threads de ataque POR JOGADOR
#define D 2 // numero de threads de defesa POR JOGADOR
#define D_A 2 // de quantos atacantes um defensor pode se defender ao mesmo tempo (não estou usando essa variável)
#define P 5 // total de jogadores

int vida_minima = 25; // minimo 25
int vida_maxima = 100; // maximo 99
int dano_minimo = 10; // minimo 10
int dano_maximo = 50; // maximo 49

// variaveis do problema
int atacando = 0; // contador de threads atacando
int defendendo = 0; // contador de threads defendendo
int estado[P]; // vetor com os estados dos jogadores:
// 0 = INATIVO // 1 = ATACANDO // 2 = EM FORMAÇÃO DE DEFESA // 3 = DEFENDENDO DE 1 // 4 = DEFENDENDO DE 2 // 9 = MORTO

int threads_ativas = 0; // MAX = (A + D) * P
int espera = 0; // controle para as threads não começarem uma de cada vez

struct Jogador {
    int id_jogador;
    int vida;
    int dano;
};

// cria a struct a ser passada para as threads: a lista de jogadores mais o jogador a que a thread pertence e o id da thread
struct Passa {
    struct Jogador *jogs;
    int id_jogador;
    int id_thread;
};

// variaveis para sincronizacao
pthread_mutex_t mutex;
pthread_cond_t conds[P];

int IniciaAtaque (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    while (espera == 0) {
        //printf("Thread de ataque do jogador %d diz: threads ativas = %d\n", id_jogador, threads_ativas);
        printf("Thread atacante %d do jogador %d BLOQUEADA\n", id_thread, id_jogador);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("Thread atacante %d do jogador %d LIBERADA!\n", id_thread, id_jogador);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (estado[id_jogador] == 9) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
        //matar a thread caso o jogador morra com pthread_exit();
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (ataque)
    while(estado[id_jogador] != 0) {
        printf("Jogador %d queria atacar mas está fazendo outra ação\n", id_jogador);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("Jogador %d teve seu ataque desbloqueado\n", id_jogador);
    }

    // escolha do alvo
    srand(time(NULL));
    int idAlvo = 0;
    while(1) {
        idAlvo = rand() % P;
        if(estado[idAlvo] < 3 && idAlvo != id_jogador) break;
        else if (estado[idAlvo == 4]) printf("Jogador %d queria atacar jogador %d mas este já está sendo atacado por 2 jogadores\n", id_jogador, idAlvo);
        else if (estado[idAlvo == 9]) printf("Jogador %d queria atacar jogador %d mas este está morto\n", id_jogador, idAlvo);
    }

    printf("Jogador %d mirou no jogador %d\n", id_jogador, idAlvo);

    // while(estado[idAlvo] >= 4) {
    //     printf("Jogador %d teve seu ataque bloqueado\n", id_jogador);
    //     pthread_cond_wait(&conds[id_jogador], &mutex);
    //     printf("Jogador %d teve seu ataque desbloqueado\n", id_jogador);
    // }
    // está insistindo no mesmo alvo

    atacando++;
    estado[id_jogador] = 1;
    if (estado[idAlvo] == 2 || estado[idAlvo] == 3) estado[idAlvo]++;
    pthread_mutex_unlock(&mutex);
    return idAlvo;
}

void ExecutaAtaque (int id_jogador, int idAlvo, int dano_ataque, int vida_alvo) {
    sleep(0.25); // tempo para realizar o ataque

    if (estado[idAlvo] == 0 || estado[idAlvo] == 1) {
        printf("ATAQUE BEM SUCEDIDO - Jogador %d inflingiu %d de dano ao jogador %d\n", id_jogador, dano_ataque, idAlvo);
        vida_alvo -= dano_ataque;
        printf("Vida do Jogador %d: %d/%d\n", idAlvo, vida_alvo, vida_maxima);

        if (vida_alvo <= 0) {
            printf("MORTE - Jogador %d matou o jogador %d\n", id_jogador, idAlvo);
            estado[idAlvo] = 9;
        }
    }
    if (estado[idAlvo] == 2 || estado[idAlvo] == 3) {
        printf("DEFESA - Jogador %d se defendeu do ataque do jogador %d\n", idAlvo, id_jogador);
    }
    if (estado[idAlvo] == 4) {
        printf("ERRO - Jogador %d atacou o jogador %d que estava no estado 4\n", id_jogador, idAlvo);
    }
    if (estado[idAlvo] == 9) {
    printf("ERRO - Jogador %d atacou o jogador %d que está morto!\n", id_jogador, idAlvo);
    }
}

void FimAtaque (int id_jogador, int idAlvo) {
    pthread_mutex_lock(&mutex);
    printf("Jogador %d terminou de atacar\n", id_jogador);
    atacando--;
    estado[id_jogador] = 0;
    if (estado[idAlvo] == 4 || estado[idAlvo] == 3) estado[idAlvo]--;
    pthread_cond_signal(&conds[id_jogador]);
    pthread_mutex_unlock(&mutex);
}

void IniciaDefesa (int id_jogador, int id_thread) {
    pthread_mutex_lock(&mutex);

    printf("ESPERA = %d / ID_THREAD = %d\n", espera, id_thread);
    while (espera == 0) {
        //printf("Thread de ataque do jogador %d diz: threads ativas = %d\n", id_jogador, threads_ativas);
        if (id_thread == (A + D) * P - 1) {
            espera = 1;
        }
        printf("Thread defensora %d do jogador %d BLOQUEADA\n", id_thread, id_jogador);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("Thread defensora %d do jogador %d liberada!\n", id_thread, id_jogador);
    }

    // se o jogador tiver morrido dá um wait eterno
    while (estado[id_jogador] == 9) {
        pthread_cond_wait(&conds[id_jogador], &mutex);
    }

    // enquanto o jogador não estiver inativo ele não pode iniciar uma ação (defesa)
    while(estado[id_jogador] != 0) {
        printf("Jogador %d queria defender mas está fazendo outra ação\n", id_jogador);
        pthread_cond_wait(&conds[id_jogador], &mutex);
        printf("Jogador %d teve sua defesa desbloqueada\n", id_jogador);
    }

    printf("Jogador %d quer defender\n", id_jogador);
    defendendo++;
    estado[id_jogador] = 2;
    pthread_mutex_unlock(&mutex);
}

void FimDefesa (int id_jogador) {
    pthread_mutex_lock(&mutex);
    printf("Jogador %d terminou de defender\n", id_jogador);
    defendendo--;
    estado[id_jogador] = 0;
    pthread_cond_signal(&conds[id_jogador]);
    pthread_mutex_unlock(&mutex);
}

// thread de ataque
void * atacante (void * arg) {
    struct Passa *passado = (struct Passa *) arg;
    struct Jogador *jogadores = passado->jogs;
    int id_jogador = passado->id_jogador;
    int id_thread = passado->id_thread;
    printf("TESTETESTETESTE Thread de ataque %d TESTETESTETESTE\n", id_thread);

    while(1) {
        int idAlvo = IniciaAtaque(id_jogador, id_thread);
        printf("Jogador %d está atacando o jogador %d\n", id_jogador, idAlvo);
        ExecutaAtaque(id_jogador, idAlvo, jogadores[id_jogador].dano, jogadores[idAlvo].vida);
        FimAtaque(id_jogador, idAlvo);
        sleep(1); //tempo de descanso
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
        IniciaDefesa(id_jogador, id_thread);
        printf("Jogador %d está defendendo\n", id_jogador);
        sleep(1); //tempo de defesa
        FimDefesa(id_jogador);
        sleep(1); //tempo de descanso
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

    // cria os jogadores e seus status
    printf("Criando os jogadores e seus status\n");
    srand(time(NULL));
    struct Jogador lista_jogs[P];
    for(int i=0; i<P; i++){
        lista_jogs[i].id_jogador = i;
        lista_jogs[i].vida = vida_minima + (rand() % (vida_maxima - vida_minima));
        lista_jogs[i].dano = dano_minimo + (rand() % (dano_maximo - dano_minimo));
        estado[i] = 0;
        printf("Jogador %d criado com %d de vida e %d de dano\n", i, lista_jogs[i].vida, lista_jogs[i].dano);
    }

    struct Passa passas[P][A+D];
    
    // cria as threads atacantes
    for(int j=0; j < P; j++) {
        for(int i=0; i < A; i++) {
            id[(j * A) + i] = (j * A) + i;
            passas[j][i].jogs = lista_jogs;
            passas[j][i].id_jogador = j;
            passas[j][i].id_thread = (j * A) + i;
            printf("Criando thread atacante %d do jogador %d\n", (j * A) + i, j);
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
            passas[j][A+i].jogs = lista_jogs;
            passas[j][A+i].id_jogador = j;
            passas[j][A+i].id_thread = (A * P) + (j * D) + i;
            printf("Criando thread defensora %d do jogador %d\n", (A * P) + (j * D) + i, j);
            if(pthread_create(&tid[(A * P) + (j * D) + i], NULL, defensor, (void *) &passas[j][A+i])) exit(-1);
            threads_ativas++;
            //printf("Threads ativas = %d\n", threads_ativas);
            //printf("Thread defensora %d criada\n", (A * P) + (j * D) + i);
        }
    }
    
    sleep(1);
    if (espera == 1) {
        for(int jogador=0; jogador < P; jogador++) {
        printf("Threads do jogador %d liberadas!\n", jogador);
        pthread_cond_signal(&conds[jogador]);
        }
    }

    pthread_exit(NULL);
    return 0;
}
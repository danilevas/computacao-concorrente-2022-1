// int *id_thread = (int *) arg;
//     int id_interm = (int)(floor((float)*id_thread/(float)D)); //id_threadD = (j * D) + i -> j = (id_threadD - i) / D -> id_jogador = floor(id_threadD / D)
//     int *id_jogador = (int *) id_interm;

// // thread de ação
// void * acao (void * arg) {
//     pthread_mutex_lock(&mutex);
//     int *id = (int *) arg;
//     srand(time(NULL));
//     while(1) {
//         int aleatorio = rand() % 100;
//         if (aleatorio >= 0 && aleatorio < 60){
//             estado[*id] = 1;
//         }
//         else if (aleatorio >= 60 && aleatorio < 100) {
//             estado[*id] = 2;
//         }
//         // else if (aleatorio >= 70 && aleatorio < 90) {
//         //     estado[*id] = 5; // ESQUIVA
//         // }
//         // else if (aleatorio >= 90 && aleatorio < 100) {
//         //     estado[*id] = 6; // DESARME
//         // }
//         estado[*id] = rand() % 4;
//         pthread_cond_signal(&cond_defesa);
//         //printf("Defensora %d esta defendendo\n", *id);
//         //sleep(1); //tempo de defesa
//         sleep(1); //tempo de descanso
//     } 
//     free(arg);
//     pthread_mutex_unlock(&mutex);
//     pthread_exit(NULL);
// }

// // cria as threads de acao
// for(int i=0; i<P; i++) {
// id[i] = i+1;
// if(pthread_create(&tid[i], NULL, acao, (void *) &id[i])) exit(-1);
// } 

// BLOQUEIO DEFESA
// while((atacando>0) || (defendendo>0)) {
//     printf("D[%d] bloqueou\n", id);
//     pthread_cond_wait(&cond_defesa, &mutex);
//     printf("D[%d] desbloqueou\n", id);
// }

// struct Jogador* lista_jogs[P] = malloc(sizeof(struct Jogador) * P);
//     struct Passa* passa = malloc(sizeof(struct Passa));

//     // cria os jogadores e seus status
//     printf("Criando os jogadores e seus status\n");
//     srand(time(NULL));
//     // struct Jogador lista_jogs[P];
//     for(int i=0; i<P; i++){
//         *lista_jogs[i].id_jogador = i;
//         *lista_jogs[i].vida = rand() % vida_maxima;
//         *lista_jogs[i].dano = rand() % dano_maximo;
//         printf("Jogador %d criado com %d de vida e %d de dano\n", i, lista_jogs[i].vida, lista_jogs[i].dano);
//     }

//     // struct Passa passa;
//     passa->jogs = lista_jogs;
//     // for (int i=0; i<P; i++){
//     //     passa.jogs.jogador = lista_jogs[i];
//     // }

//     // cria as threads atacantes
//     for(int j=0; j < P; j++) {
//         for(int i=0; i < A; i++) {
//             id[(j * A) + i] = (j * A) + i + 1;
//             passa->id_jogador = j;
//             if(pthread_create(&tid[(j * A) + i], NULL, atacante, (void *) &passa)) exit(-1);
//             printf("Thread atacante %d criada\n", (j * A) + i);
//         } 
//     }

//     // cria as threads defensoras
//     for(int j=0; j < P; j++) {
//         for(int i=0; i < D; i++) {
//             id[(A * P) - 1 + (j * D) + i ] = (A * P) + (j * D) + i ;
//             passa->id_jogador = j;
//             if(pthread_create(&tid[(A * P) - 1 + (j * D) + i], NULL, defensor, (void *) &passa)) exit(-1);
//             printf("Thread defensora %d criada\n", (A * P) - 1 + (j * D) + i);
//         }
//     }

//     pthread_exit(NULL);
//     return 0;
// }
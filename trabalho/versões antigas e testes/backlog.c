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
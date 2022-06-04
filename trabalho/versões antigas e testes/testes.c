struct Jogador {
    int id_jogador;
    int vida;
    int dano;
};

// cria a struct a ser passada para as threads: a lista de jogadores mais o jogador a que a thread pertence
struct Passa {
    struct Jogador *jogs;
    int *id_jogador;
};
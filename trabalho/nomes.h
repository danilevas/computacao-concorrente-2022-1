#include<pthread.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<math.h>

void geraNome(char nome[]) {
    char nomes[16][20];
    strcpy(nomes[0], "Joel");
    strcpy(nomes[1], "Maria");
    strcpy(nomes[2], "Jorginho");
    strcpy(nomes[3], "Pamela");
    strcpy(nomes[4], "Charles");
    strcpy(nomes[5], "Gilson");
    strcpy(nomes[6], "Carlinhos");
    strcpy(nomes[7], "Eratóstenes");
    strcpy(nomes[8], "Turing");
    strcpy(nomes[9], "Euler");
    strcpy(nomes[10], "Newton");
    strcpy(nomes[11], "Fermat");
    strcpy(nomes[12], "Fabinho");
    strcpy(nomes[13], "Ada");
    strcpy(nomes[14], "Úrsula");
    strcpy(nomes[15], "Janderson");

    char titulos[13][20];
    strcpy(titulos[0], "O Bárbaro");
    strcpy(titulos[1], "A Destruidora");
    strcpy(titulos[2], "O Pequeno");
    strcpy(titulos[3], "A Mortal");
    strcpy(titulos[4], "O Trapaceiro");
    strcpy(titulos[5], "O Ultrapassado");
    strcpy(titulos[6], "O Preguiçoso");
    strcpy(titulos[7], "A Sanduicheira");
    strcpy(titulos[8], "A Geladeira");
    strcpy(titulos[9], "O Martelo");
    strcpy(titulos[10], "O Raio");
    strcpy(titulos[11], "A Lagartixa");
    strcpy(titulos[12], "O Imbatível");

    strcpy(nome, nomes[rand() % 16]);
    strcat(nome, ", ");
    strcat(nome, titulos[rand() % 13]);
}

void geraDesc(char descricao[]) {
    char desc[100] = "Vindo/a de uma família de ";
    char familia[9][100];
    strcpy(familia[0], "alfaiates ");
    strcpy(familia[1], "guerreiros ");
    strcpy(familia[2], "padeiros ");
    strcpy(familia[3], "líderes ");
    strcpy(familia[4], "reis ");
    strcpy(familia[5], "ladrões ");
    strcpy(familia[6], "assassinos ");
    strcpy(familia[7], "fazendeiros ");
    strcpy(familia[8], "padres ");

    strcat(desc, familia[rand() % 9]);

    char lugar[8][100];
    strcpy(lugar[0], "das planícies");
    strcpy(lugar[1], "das montanhas");
    strcpy(lugar[2], "das florestas");
    strcpy(lugar[3], "da costa");
    strcpy(lugar[4], "da cidade subterrânea");
    strcpy(lugar[5], "do deserto");
    strcpy(lugar[6], "das estepes");
    strcpy(lugar[7], "de terras longínquas");

    strcat(desc, lugar[rand() % 8]);

    strcpy(descricao, desc);
}

// int main(void) {
//     srand(time(NULL));
//     char nome[100] = "";
//     geraNome(nome);
//     puts(nome);

//     char descricao[100] = "";
//     geraDesc(descricao);
//     puts(descricao);
//     return 0;
// }
# Repositório de Trabalhos - Computação Concorrente (2022.1)

Este repositório contém os trabalhos desenvolvidos durante a disciplina de **Computação Concorrente** do Curso de Ciência da Computação da UFRJ, cursada em 2022.1. Durante o curso, foram explorados conceitos fundamentais de concorrência, sincronização e comunicação entre threads, utilizando a linguagem C com as bibliotecas **pthreads** e **semaphore**.

## 🧵 Conceitos e Implementações

Ao longo da disciplina, foram implementadas diversas abordagens de programação concorrente, incluindo:

- **Criação e gerenciamento de threads** com `pthread_create` e `pthread_join`
- **Mecanismos de sincronização** como mutexes e variáveis de condição
- **Compartilhamento seguro de recursos** entre múltiplas threads
- **Controle de concorrência** em diferentes cenários, evitando condições de corrida e deadlocks

## 🎮 Trabalho Final - Corrente do Poder

O projeto final da disciplina consistiu no jogo **Corrente do Poder**, um jogo de apostas e combate totalmente concorrente. O jogador deve escolher um dos cinco personagens e torcer para que ele seja o último sobrevivente na arena.

### 🔥 Mecânica do Jogo

- Cinco personagens lutam em uma arena, cada um com atributos individuais: **vida, dano, iniciativa, agilidade e inteligência**.
- Cada personagem possui **duas threads de ataque** e **uma thread de defesa**, garantindo um ambiente altamente concorrente.
- O jogador aposta em um personagem antes da luta começar. Se seu personagem for o vencedor, ele recebe a lendária **Corrente do Poder**.
- Durante a luta, os personagens podem executar ações de **ataque, defesa, esquiva e desarme**, todas coordenadas por threads independentes.
- A luta continua até restar apenas um personagem vivo, declarando o vencedor.

### 🛠️ Implementação Concorrente

- **Controle de ações**: Mutexes e variáveis de condição garantem que as threads sigam a lógica de jogo corretamente.
- **Escolha de alvos**: Cada personagem escolhe alvos de ataque dinamicamente, evitando ataques inválidos.
- **Mecanismo de apostas**: O jogador seleciona um personagem no início do jogo e acompanha sua performance.
- **Gestão de eventos**: Threads de ataque e defesa coordenam ações simultâneas, criando um ambiente dinâmico e imprevisível.

## 🛠️ Dependências

- Linguagem: **C**
- Bibliotecas: **pthreads** e **semaphore**

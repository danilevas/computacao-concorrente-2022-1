// classe da estrutura de dados (recurso) Cmprt entre as threads
class Cmprt {
    // recurso compartilhado
    private int c_global;
    private int total;

    // construtor
    public Cmprt() {
        this.c_global = 0;
        this.total = 0;
    }

    public synchronized int inc_c_global() {
        this.c_global++;
        return this.c_global-1;
    }

    public synchronized int get_tot() {
        return this.total;
    }

    public synchronized void inc_tot() {
        this.total++;
    }
}

// funcao executada pelas threads
class Find extends Thread {
    Cmprt c_local;
    int[] my_vetor;

    // construtor
    public Find(Cmprt Cmprt, int[] my_vetor) {
        this.c_local = Cmprt;
        this.my_vetor = my_vetor;
    }

    // metodo que as threads executam
    public void run() {
        int c = this.c_local.inc_c_global();
        while(c < my_vetor.length) {
            if (my_vetor[c] % 2 == 0) { // confere se eh par
                c_local.inc_tot();
            }
            c = this.c_local.inc_c_global();
        }
    }
}

// classe da main
class PrincipalThread {
    static final int tamanho_vetor = 3000;
    static final int num_threads = 4;

    public static void main (String[] args) {
        // reserva espaço para um vetor de threads
        Thread[] threads = new Thread[num_threads];
        
        // vetor inicial de tamanho tamanho_vetor
        int[] my_vetor = new int[tamanho_vetor];
        
        // total sequencial para comparar corretude
        int tot_seq = 0;

        // instancia o recurso Cmprt
        Cmprt Cmprt = new Cmprt();

        // popula o vetor
        for(int i=0; i<tamanho_vetor; i++) {
            my_vetor[i] = i;
        }

        // cria as threads
        for (int i=0; i<threads.length; i++) {
            threads[i] = new Find(Cmprt, my_vetor);
        }

        // inicia as threads
        for (int i=0; i<threads.length; i++) {
            threads[i].start();
        }

        // espera pelo termino das threads
        for (int i=0; i<threads.length; i++) {
            try { threads[i].join(); }
            catch (InterruptedException e) { return; }
        }

        // acha o total de numeros pares de forma sequencial
        for(int i=0; i<tamanho_vetor; i++) {
            if(my_vetor[i] % 2 == 0) {
                tot_seq++;
            }
        }

        System.out.println("Total de numeros pares encontrado de forma sequencial: " + tot_seq);
        System.out.println("Total de numeros pares encontrado de forma concorrente: " + Cmprt.get_tot());

        if(tot_seq == Cmprt.get_tot()) {
            System.out.println("Sucesso! O programa rodou certinho.");
        }
        else {
            System.out.println("Os valores totais nao bateram, há algum problema com o programa.");
        }
    }
}
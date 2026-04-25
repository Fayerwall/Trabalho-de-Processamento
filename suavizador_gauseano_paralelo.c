#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h> // Inclusão da biblioteca OpenMP

#define SIGMA 1.0
#define INTERACOES 3
#define TAMANHO 3

void monta_kernel(double kernel[TAMANHO][TAMANHO]){
    int i, j;
    int ax[TAMANHO];
    double sum = 0.0;

    for(i = 0; i < TAMANHO; i++) ax[i] = (-1 * (TAMANHO / 2) + i);

    for(i = 0; i < TAMANHO; i++){
        for(j = 0; j < TAMANHO; j++){
            kernel[i][j] = exp(-1 * (pow(ax[j], 2.0) + pow(ax[i], 2.0)) / (2 * pow(SIGMA, 2.0)));
            sum += kernel[i][j];
        }
    }

    for(i = 0; i < TAMANHO; i++){
        for(j = 0; j < TAMANHO; j++) kernel[i][j] /= sum;
    }
}

double** padding(int largura, int altura, double **imagem){
    int p = TAMANHO / 2;
    int n_largura = largura + 2 * p;
    int n_altura = altura + 2 * p;

    double **imagem_preenchida = (double **)malloc(n_altura * sizeof(double *));
    for(int i = 0; i < n_altura; i++) 
        imagem_preenchida[i] = (double *)malloc(n_largura * sizeof(double));

    // Paralelização do loop externo de preenchimento
    #pragma omp parallel for collapse(2) schedule(static)
    for(int i = 0; i < n_altura; i++){
        for(int j = 0; j < n_largura; j++){
            int ori_i = i - p;
            int ori_j = j - p;

            if (ori_i < 0) ori_i = 0;
            if (ori_i >= altura) ori_i = altura - 1;
            if (ori_j < 0) ori_j = 0;
            if (ori_j >= largura) ori_j = largura - 1;

            imagem_preenchida[i][j] = imagem[ori_i][ori_j];
        }
    }
    return imagem_preenchida;
}

void convolucao(int largura, int altura, double **imagem_preenchida, double kernel[TAMANHO][TAMANHO], double **saida){
    // Paralelização do loop externo. 'soma' deve ser privada para cada thread.
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            double soma = 0.0; // Variável declarada dentro do loop paralelo é privada por padrão
            for (int ki = 0; ki < TAMANHO; ki++) {
                for (int kj = 0; kj < TAMANHO; kj++) {
                    soma += imagem_preenchida[i + ki][j + kj] * kernel[ki][kj];
                }
            }
            saida[i][j] = soma;
        }
    }
}

double** suavizador_gaussiano(int largura, int altura, double **imagem){
    double kernel[TAMANHO][TAMANHO];
    monta_kernel(kernel);

    double **imagem_atual = (double **)malloc(altura * sizeof(double *));
    for (int i = 0; i < altura; i++) {
        imagem_atual[i] = (double *)malloc(largura * sizeof(double));
        #pragma omp parallel for
        for (int j = 0; j < largura; j++) imagem_atual[i][j] = imagem[i][j];
    }

    for (int it = 0; it < INTERACOES; it++) {
        double **com_pad = padding(largura, altura, imagem_atual);
        convolucao(largura, altura, com_pad, kernel, imagem_atual);

        for (int i = 0; i < (altura + (TAMANHO/2)*2); i++) free(com_pad[i]);
        free(com_pad);
    }
    return imagem_atual;
}

int main(void){
    // Exemplo de matriz 5x5 conforme o benchmark
    int altura = 5, largura = 5;
    double **imagem = (double **)malloc(altura * sizeof(double *));
    for(int i=0; i<altura; i++) imagem[i] = (double *)malloc(largura * sizeof(double));
    
    double dados[5][5] = {
        {1, 2, 3, 2, 1},
        {2, 4, 6, 4, 2},
        {3, 6, 9, 6, 3},
        {2, 4, 6, 4, 2},
        {1, 2, 3, 2, 1}
    };
    for(int i=0; i<5; i++) for(int j=0; j<5; j++) imagem[i][j] = dados[i][j];

    printf("Rodando Suavizador com %s threads...\n", getenv("OMP_NUM_THREADS") ? getenv("OMP_NUM_THREADS") : "padrao");

    double inicio = omp_get_wtime();
    double **resultado = suavizador_gaussiano(largura, altura, imagem);
    double fim = omp_get_wtime();

    printf("\n--- Matriz Suavizada em Paralelo ---\n");
    for(int i = 0; i < altura; i++){
        for(int j = 0; j < largura; j++) printf("%.6f ", resultado[i][j]);
        printf("\n");
    }

    printf("\nTempo de execucao (paralelo): %f segundos\n", fim - inicio);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SIGMA 1.0
#define INTERACOES 20
#define TAMANHO 3

// Aloca um bloco contíguo de memória para a matriz (melhora o cache)
double** alocar_matriz(int altura, int largura) {
    double **matriz = (double **)malloc(altura * sizeof(double *));
    double *dados = (double *)malloc(altura * largura * sizeof(double));
    for (int i = 0; i < altura; i++) {
        matriz[i] = &dados[i * largura];
    }
    return matriz;
}

// Libera a matriz contígua de forma segura
void liberar_matriz(double **matriz) {
    if (matriz != NULL) {
        if (matriz[0] != NULL) {
            free(matriz[0]); // Libera o bloco de dados
        }
        free(matriz); // Libera o vetor de ponteiros
    }
}

/*faz a montagem do kernel a ser utilizado*/
void monta_kernel(double kernel[TAMANHO][TAMANHO]){
    int i, j;
    int ax[TAMANHO], xx[TAMANHO][TAMANHO], yy[TAMANHO][TAMANHO];

    for(i = 0; i < TAMANHO; i++){
        ax[i] = (-1 * (TAMANHO / 2) + i);
    }

    for(i = 0; i < TAMANHO; i++){
        for(j = 0; j < TAMANHO; j++){
            xx[i][j] = ax[j];
            yy[i][j] = ax[i];
        }
    }

    double sum = 0.0;

    for(i = 0; i < TAMANHO; i++){
        for(j = 0; j < TAMANHO; j++){
            kernel[i][j] = exp(-1 * (pow(xx[i][j], 2.0) + pow(yy[i][j], 2.0)) / (2 * pow(SIGMA, 2.0)));
            sum += kernel[i][j];
        }
    }

    for(i = 0; i < TAMANHO; i++){
        for(j = 0; j < TAMANHO; j++){
            kernel[i][j] /= sum;
        }
    }
}

/*faz o processo do aumento da borda da imagem original*/
double** padding(int largura, int altura, double **imagem){
    int p = TAMANHO / 2;
    int n_largura = largura + 2 * p;
    int n_altura = altura + 2 * p;

    double **imagem_preenchida = alocar_matriz(n_altura, n_largura);

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

/*realiza a operação de convolução e alteração do pixel da imagem*/
double** convolucao(int largura, int altura, double **imagem_preenchida, double kernel[TAMANHO][TAMANHO], double **saida){
    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            double soma = 0.0;
            for (int ki = 0; ki < TAMANHO; ki++) {
                for (int kj = 0; kj < TAMANHO; kj++) {
                    soma += imagem_preenchida[i + ki][j + kj] * kernel[ki][kj];
                }
            }
            saida[i][j] = soma;
        }
    }
    return saida;
}

double** suavizador_gaussiano(int largura, int altura, double **imagem){
    double kernel[TAMANHO][TAMANHO];
    monta_kernel(kernel);

    // Alocação contígua da imagem atual
    double **imagem_atual = alocar_matriz(altura, largura);
    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            imagem_atual[i][j] = imagem[i][j];
        }
    }

    // Alocação da matriz de saída fora do loop
    double **imagem_saida = alocar_matriz(altura, largura);

    for (int it = 0; it < INTERACOES; it++) {
        double **com_pad = padding(largura, altura, imagem_atual);

        convolucao(largura, altura, com_pad, kernel, imagem_saida);

        // Swap (troca de ponteiros para evitar re-alocações na memória)
        double **temp = imagem_atual;
        imagem_atual = imagem_saida;
        imagem_saida = temp;

        liberar_matriz(com_pad);
    }

    liberar_matriz(imagem_saida);
    return imagem_atual;
} 

double** ler_pgm(const char *nome_arquivo, int *largura, int *altura) {
    FILE *arquivo = fopen(nome_arquivo, "r");
    if (!arquivo) {
        printf("Erro ao abrir a imagem: %s\n", nome_arquivo);
        exit(1);
    }

    char formato[3];
    int max_val;
    if (fscanf(arquivo, "%s", formato) != 1 || 
        fscanf(arquivo, "%d %d", largura, altura) != 2 || 
        fscanf(arquivo, "%d", &max_val) != 1) {
        
        printf("Erro ao ler cabecalho da imagem.\n");
        fclose(arquivo);
        exit(1);
    }

    double **imagem = alocar_matriz(*altura, *largura);
    for (int i = 0; i < *altura; i++) {
        for (int j = 0; j < *largura; j++) {
            int pixel;
            if (fscanf(arquivo, "%d", &pixel) != 1) {
                printf("Erro ao ler pixels.\n");
                liberar_matriz(imagem);
                fclose(arquivo);
                exit(1);
            }
            imagem[i][j] = (double)pixel;
        }
    }
    fclose(arquivo);
    return imagem;
}

void salvar_pgm(const char *nome_arquivo, int largura, int altura, double **imagem) {
    FILE *arquivo = fopen(nome_arquivo, "w");
    if (!arquivo) {
        printf("Erro ao salvar a imagem: %s\n", nome_arquivo);
        exit(1);
    }

    fprintf(arquivo, "P2\n%d %d\n255\n", largura, altura);

    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            int pixel = (int)imagem[i][j];
            if (pixel > 255) pixel = 255;
            if (pixel < 0) pixel = 0;
            fprintf(arquivo, "%d ", pixel);
        }
        fprintf(arquivo, "\n");
    }
    fclose(arquivo);
}

int main(void) {
    int largura, altura;

    double **imagem = ler_pgm("entrada.pgm", &largura, &altura);

    // Início da medição com a biblioteca padrão <time.h>
    clock_t start = clock();

    double **resultado = suavizador_gaussiano(largura, altura, imagem);

    clock_t end = clock();

    double tempo_segundos = (double)(end - start) / CLOCKS_PER_SEC;

    salvar_pgm("saida.pgm", largura, altura, resultado);

    printf("\nTempo de execucao (Serial ): %f segundos\n", tempo_segundos);

    liberar_matriz(imagem);
    liberar_matriz(resultado);

    return 0;
}
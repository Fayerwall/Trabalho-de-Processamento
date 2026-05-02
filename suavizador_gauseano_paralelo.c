#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define SIGMA 1.0
#define INTERACOES 20
#define TAMANHO 3

// Aloca um bloco contíguo de memória para a matriz (melhora o cache e o acesso aos pixels)
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

double** padding(int largura, int altura, double **imagem){
    int p = TAMANHO / 2;
    int n_largura = largura + 2 * p;
    int n_altura = altura + 2 * p;

    double **imagem_preenchida = alocar_matriz(n_altura, n_largura);

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

double** convolucao(int largura, int altura, double **imagem_preenchida, double kernel[TAMANHO][TAMANHO], double **saida){
    #pragma omp parallel for schedule(static)
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

    // Aloque a imagem atual de forma contígua e copie os dados
    double **imagem_atual = alocar_matriz(altura, largura);
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            imagem_atual[i][j] = imagem[i][j];
        }
    }

    // Alocação de buffer auxiliar fora da iteração
    double **imagem_saida = alocar_matriz(altura, largura);

    for (int it = 0; it < INTERACOES; it++) {
        double **com_pad = padding(largura, altura, imagem_atual);

        convolucao(largura, altura, com_pad, kernel, imagem_saida);

        // Swap (troca de ponteiros para evitar re-alocações)
        double **temp = imagem_atual;
        imagem_atual = imagem_saida;
        imagem_saida = temp;

        liberar_matriz(com_pad); // Libera o padding, mas mantém os dados processados
    }

    liberar_matriz(imagem_saida); // Libera o ponteiro do buffer auxiliar restante
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

    // Verificamos o retorno de cada fscanf para evitar o warning e tratar possíveis erros de leitura
    if (fscanf(arquivo, "%s", formato) != 1) {
        printf("Erro ao ler o formato da imagem.\n");
        fclose(arquivo);
        exit(1);
    }
    
    if (fscanf(arquivo, "%d %d", largura, altura) != 2) {
        printf("Erro ao ler as dimensoes da imagem.\n");
        fclose(arquivo);
        exit(1);
    }
    
    if (fscanf(arquivo, "%d", &max_val) != 1) {
        printf("Erro ao ler o valor maximo de tom de cinza.\n");
        fclose(arquivo);
        exit(1);
    }

    double **imagem = alocar_matriz(*altura, *largura);
    for (int i = 0; i < *altura; i++) {
        for (int j = 0; j < *largura; j++) {
            int pixel;
            if (fscanf(arquivo, "%d", &pixel) != 1) {
                printf("Erro ao ler os pixels da imagem.\n");
                // Libera a matriz antes de sair
                for(int k=0; k<i; k++) free(imagem[k]);
                free(imagem);
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

    double inicio = omp_get_wtime();

    double **resultado = suavizador_gaussiano(largura, altura, imagem);

    double fim = omp_get_wtime();

    salvar_pgm("saida.pgm", largura, altura, resultado);

    printf("\nTempo de execucao (Paralelo Otimizado): %f segundos\n", fim - inicio);

    // Liberação da memória
    liberar_matriz(imagem);
    liberar_matriz(resultado);

    return 0;
}
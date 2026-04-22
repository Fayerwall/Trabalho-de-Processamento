#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SIGMA 1.0
#define INTERACOES 3
#define TAMANHO 3

/*faz a montagem do kernel a ser utilizado*/

void monta_kernel(double kernel[TAMANHO][TAMANHO]){
    int i,j;
    int ax[TAMANHO], xx[TAMANHO][TAMANHO],yy[TAMANHO][TAMANHO];


    for( i=0;i<TAMANHO;i++){
        ax[i] = (-1 * (TAMANHO / 2) + i);
    }

    for( i=0; i < TAMANHO; i++){
        for( j=0; j < TAMANHO; j++){
            xx[i][j] = ax[j];
            yy[i][j] = ax[i];
        }
    }

    double sum = 0;

    for( i=0; i <TAMANHO; i++){
        for( j=0; j < TAMANHO; j++){
            kernel[i][j] = exp( -1 *(pow(xx[i][j],2.0) + pow(yy[i][j],2.0))  / (2 * pow(SIGMA,2.0)));
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

    double **imagem_preenchida = (double **)malloc(n_altura * sizeof(double *));

    for(int i = 0; i < n_altura; i++){
        imagem_preenchida[i] = (double *)malloc(n_largura * sizeof(double));

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
}

/*consta esse erro na função abaixo mas não interfere no código*/

double** suavizador_gaussiano( int largura, int altura, double imagem[altura][largura]){
    double kernel[TAMANHO][TAMANHO];
    monta_kernel(kernel);

    double **imagem_atual = (double **)malloc(altura * sizeof(double *));
    for (int i = 0; i < altura; i++) {
        imagem_atual[i] = (double *)malloc(largura * sizeof(double));
        for (int j = 0; j < largura; j++) imagem_atual[i][j] = imagem[i][j];
    }

    int contador = 0;
    for (int it = 0; it < INTERACOES; it++) {

        double **com_pad = padding(largura, altura, imagem_atual);

        convolucao(largura, altura, com_pad, kernel, imagem_atual);

        for (int i = 0; i < (altura + (TAMANHO/2)*2); i++) free(com_pad[i]);
        free(com_pad);
        
    }
    return imagem_atual;
}   

int main(void){
    
    double imagem[5][5] = {
        {1, 2, 3, 2, 1},
        {2, 4, 6, 4, 2},
        {3, 6, 9, 6, 3},
        {2, 4, 6, 4, 2},
        {1, 2, 3, 2, 1}
    };

    int altura = sizeof(imagem) / sizeof(imagem[0]);
    int largura = sizeof(imagem[0]) / sizeof(imagem[0][0]);

    double **resultado = suavizador_gaussiano(largura, altura, imagem);

    for(int i = 0; i < altura; i++){
        for(int j = 0; j < largura; j++){
            printf("%f ", resultado[i][j]);
        }
        printf("\n");
    }

    return 0;
}
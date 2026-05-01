import cv2

img = cv2.imread('saida.pgm')

if img is None:
    print("Erro: Não foi possível encontrar ou ler o arquivo 'saida.pgm'")
else:

    cv2.imwrite('imagem_suavizada.png', img)
    
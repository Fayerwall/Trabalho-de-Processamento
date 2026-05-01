import cv2

img = cv2.imread('imagem2.jpg', cv2.IMREAD_GRAYSCALE)
altura, largura = img.shape

with open('entrada.pgm', 'w') as f:
    f.write(f"P2\n{largura} {altura}\n255\n")
    for linha in img:
        f.write(" ".join(str(pixel) for pixel in linha) + "\n")
        
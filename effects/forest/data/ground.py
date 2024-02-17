from PIL import Image
import numpy as np
import sys


if __name__ == '__main__':
    img = np.array(Image.open(sys.argv[1]))

    print('static __data_chip uint16_t grass[320] = {')
    for row in img:
        count = 0
        aux = 0
        for pixel in row:
            count += 1
            aux = aux | ((0 if pixel[0] > 0 else 1) << (count - 1))
            if count == 16:
                print(hex(aux), end=',')
                count = 0
        print('')
    print('};')

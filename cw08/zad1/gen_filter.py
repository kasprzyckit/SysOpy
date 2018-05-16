from random import randint
import sys
import numpy as np

c = int(sys.argv[1])
fs = np.zeros((c, c))
for i in range(c):
    for j in range(c):
        fs[i][j] = randint(0, 100)
s = np.sum(fs)
for i in range(c):
    for j in range(c):
        fs[i][j] /= s
np.savetxt(sys.argv[2], fs, fmt='%.10f', delimiter='\n')
with open(sys.argv[2], 'r+') as f:
    content = f.read()
    f.seek(0, 0)
    f.write(str(c) + '\n' + content)
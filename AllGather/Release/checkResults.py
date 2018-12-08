from random import randint
import sys

with open("result1.txt",'r+') as f:
    content = f.readlines()
    mat1 = [x.strip().split(' ') for x in content]

with open("result.txt",'r+') as f:
    content = f.readlines()
    mat2 = [x.strip().split(' ') for x in content]

with open("resultDif.txt",'w+') as o:
    for i in range(0, len(mat2)):
	    for j in range (0, len(mat2[0])):
	        o.write(str(int(mat1[i][j])-int(mat2[i][j])) + " ")
	    o.write('\n')

from random import randint
import sys

size = int(sys.argv[1])
with open("matrix_A.txt",'w+') as o:
	o.write(str(size)+ "\n")
	for i in range(0, size):
	    for j in range (0, size):
	        o.write(str(randint(0, 10)) + " ")
	    o.write('\n')

with open("matrix_B.txt",'w+') as o:
	o.write(str(size)+"\n")
	for i in range(0, size):
	    for j in range (0, size):
	        o.write(str(randint(0, 10)) + " ")
	    o.write('\n')

import sys
import os

file1 = sys.argv[1]
file2 = sys.argv[2]
f1=open(file1,"r")
lines=f1.readlines()
f1.close()
count1, count2, n = 0, 0, 97

for line in open(file2):
    #print(line)
    for l in lines:
        if line.split()[1] == l.split()[1][10:] : count1+=1
        #print(line[0:40],l[0:40])
        if line.split()[0] == l.split()[0] : count2+=1
print('name:',count1/n*100,'%')
print('data:',count2/n*100,'%')

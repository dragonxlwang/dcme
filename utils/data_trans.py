import os
import re
import sys
import math
import random


def split_data(file_path, outfile_path, n, r):
    fout_train = open(outfile_path + ".train", "w")
    fout_test = open(outfile_path + ".test", "w")
    with open(file_path, "r") as fin:
        while(1):
            block = []
            for i in range(n):
                block.append(fin.readline())
            if(block[0] == ""):
                break
            if(random.random() < r):
                fout_train.writelines(block)
            else:
                fout_test.writelines(block)
    fout_train.close()
    fout_test.close()
    return


def permute_data(file_path, outfile_path, n):
    fout_train = open(outfile_path + ".train", "w")
    fout_test = open(outfile_path + ".test", "w")
    with open(file_path, "r") as fin:
        lns = fin.readlines()
        with open(outfile_path, "w") as fout:
            ttl = len(lns) / n
            ids = range(ttl)
            for i in range(ttl * 10):
                x = int(random.random() * ttl)
                y = int(random.random() * (ttl - 1))
                if(y >= x):
                    y += 1
                a = ids[x]
                ids[x] = ids[y]
                ids[y] = a
            for i in range(ttl):
                j = ids[i]
                for k in range(n):
                    fout.write(lns[k + j * n].strip() + '\n')
    return

if __name__ == '__main__':
    # split_data('/home/xwang95/data/acm_corpus/proc.txt',
    #            '/home/xwang95/data/acm_corpus/proc', 4, 0.9)
    permute_data('/home/xwang95/data/acm_corpus/proc.train',
               '/home/xwang95/data/acm_corpus/proc-perm.train', 4)

import sys
import numpy as np
import time
import re

wiki_file = '/home/xwang95/data/wiki/sample_100.txt'

window = 5
stop_word_num = 20
vocab = {}

alpha = 1e-3
beta = 1e-3


def gibbs_sample_one_iter(toks, link_tok):
    pvec = np.empty(2 * window + 1)
    for i in range(len(toks)):
        b = max(0, i - window)
        e = min(len(toks), i + window + 1)
        vocab[toks[i]][link_tok[i]] -= 1
        for j in range(b, e):
            pvec[j - b] = vocab[toks[i]].get(toks[j], 0.0) + (beta if(i == j)
                                                              else alpha)
        s = np.random.multinomial(
            1, pvec[0:e - b] / sum(pvec[0:e - b])).argmax()
        link_tok[i] = toks[b + s]
        vocab[toks[i]][link_tok[i]] = vocab[toks[i]].get(link_tok[i], 0.0) + 1
    return


def filter_stop_words(toks_lst, num=20):
    freq = {}
    stop = set()
    for toks in toks_lst:
        for w in toks:
            freq[w] = freq.get(w, 0.0) + 1
    for w in sorted(freq, key=lambda x: freq[x], reverse=True):
        stop.add(w)
        if(len(stop) == num):
            break
    for i in range(len(toks_lst)):
        toks_lst[i] = [w for w in toks_lst[i] if w not in stop]
    return toks_lst


def main_proc():
    nepoch = 10
    niter = 10
    toks_lst = []
    link_lst = []
    t1 = time.time()
    t2 = time.time()
    with open(wiki_file, 'r') as fin:
        for ln in fin:
            toks = ln.strip().split()
            toks_lst.append(toks)
    toks_lst = filter_stop_words(toks_lst, stop_word_num)
    for d in range(len(toks_lst)):
        toks = toks_lst[d]
        rdns = np.random.random(len(toks))
        blst = [max(0, i - window) for i in range(len(toks))]
        elst = [min(len(toks), i + window + 1) for i in range(len(toks))]
        llst = [toks[int(blst[i] + rdns[i] * (elst[i] - blst[i]))]
                for i in range(len(toks))]
        link_lst.append(llst)
        for i in range(len(toks)):  # initialize
            if(toks[i] not in vocab):
                vocab[toks[i]] = {}
            vocab[toks[i]][llst[i]] = vocab[toks[i]].get(llst[i], 0.0) + 1
    for e in range(nepoch):
        print('{0} epoch'.format(e))
        for d in range(len(toks_lst)):
            sys.stdout.write('process {0} doc / {1} sec\r'.format(d, t2 - t1))
            sys.stdout.flush()
            toks = toks_lst[d]
            llst = link_lst[d]
            for i in range(niter):
                gibbs_sample_one_iter(toks, llst)
            t2 = time.time()
    freq = {}
    for w in vocab:
        freq[w] = sum(vocab[w][t] for t in vocab[w])
    with open('phrase_table.txt', 'w') as fout:
        for w in sorted(vocab, key=lambda x: freq[x], reverse=True):
            if(freq[w] == vocab[w].get(w, 0)):
                continue
            fout.write('|= {0}:{1} {2}\n'.format(
                w, freq[w], vocab[w].get(w, 0)))
            for t in sorted(vocab[w], key=lambda y: vocab[w][y], reverse=True):
                if(t == w or vocab[w][t] == 0.0):
                    continue
                fout.write('{0}..{1}:{2}:{3}:{4}\n'.format(w, t, vocab[w][t],
                                                           freq[w], freq[t]))
    return


def filter_infreq_phrase(file_path):
    l = file_path.split('.')
    l[0] = l[0] + '_filter'
    outfile_path = '.'.join(l)
    with open(file_path, 'r') as fin:
        with open(outfile_path, 'w') as fout:
            for ln in fin:
                m = re.match(r"(.*?)\.\.(.*?):(.*):(.*):(.*)", ln)
                if(m and float(m.group(3)) >= 2 and
                    float(m.group(3)) < min(float(m.group(4)),
                                            float(m.group(5)))):
                    fout.write(ln)
    return

if(__name__ == '__main__'):
    if(sys.argv[1] == 'sample'):
        main_proc()
    elif(sys.argv[1] == 'filter'):
        filter_infreq_phrase('phrase_table.txt')

import os
import gigaword
import re
from nltk.tokenize import sent_tokenize, word_tokenize
from multiprocessing import Pool
import math

giga_path = os.path.join(os.path.expanduser('~'), 'data/gigaword')
nyt_path = os.path.join(giga_path, 'gigaword_eng_5_d2/data/nyt_eng/')
wpb_path = os.path.join(giga_path, 'gigaword_eng_5_d2/data/wpb_eng/')
apw_path = os.path.join(giga_path, 'gigaword_eng_5_d1/data/apw_eng/')
PUNCTUATION = set([';', ':', ',', '.', '!', '?', "``", "''", "`", "\"", "'"])

# dirpath = nyt_path
# matchstr=r'nyt_eng_\d+\.gz'
# dump_file_name = 'giga_nyt.txt'
# voc_file_name = 'giga_nyt.vcb'

dirpath = wpb_path
matchstr = r'wpb_eng_\d+\.gz'
dump_file_name = 'giga_wpb.txt'
voc_file_name = 'giga_wpb.vcb'

dirpath = apw_path
matchstr = r'apw_eng_\d+\.gz'
dump_file_name = 'giga_apw.txt'
voc_file_name = 'giga_apw.vcb'


def single_process_work(args):
    beg, end = args[0], args[1]
    proc_sents = []
    unigram = {}
    max_file_num = -1   # -1: no cap
    fn = 0
    flst = sorted(os.listdir(dirpath))[beg:end]
    for f in flst:
        fn += 1
        if(max_file_num >= 0 and fn > max_file_num):
            break
        if(not re.match(matchstr, f)):
            continue
        fp = os.path.join(dirpath, f)
        print('[giga_proc]: reading file {0} {1}-{2}'.format(fp, beg, end))
        t = gigaword.read_file(fp)
        for d in t:
            txt = ''
            for ln in d.text:
                txt += ln.replace("``", "\"").replace("''", "\"")
            txt = ' '.join(txt.split())
            sents = sent_tokenize(txt)
            for s in sents:
                words = [w for w in word_tokenize(s.encode('ascii', 'ignore'))
                         if w not in PUNCTUATION]
                proc_sents.append(words)
                for w in words:
                    unigram[w] = unigram.get(w, 0) + 1
    part_str = '.part_{0}_{1}'.format(beg, end)
    with open(os.path.join(giga_path, voc_file_name + part_str), 'w') as fout:
        print('[giga_proc]: dumping file {0}'.format(fout.name))
        for w in sorted(unigram, key=lambda x: unigram[x], reverse=True):
            fout.write('{0} {1}\n'.format(unigram[w], w))
    with open(os.path.join(giga_path, dump_file_name + part_str), 'w') as fout:
        print('[giga_proc]: dumping file {0}'.format(fout.name))
        for words in proc_sents:
            fout.write(' '.join(words) + '\n')
    print('[giga_proc]: finished {0}-{1}'.format(beg, end))


def main():
    pn = 20
    flst = os.listdir(dirpath)
    arglst = []
    for i in range(pn):
        beg = int(math.ceil(float(len(flst)) / pn * i))
        end = int(math.ceil(float(len(flst)) / pn * (i + 1)))
        if(id == 0):
            beg = 0
        if(id == pn - 1):
            end = (len(flst))
        arglst.append([beg, end])
    pool = Pool(pn)
    pool.map(single_process_work, arglst)
    pool.close()
    pool.join()
    print(80 * '=')
    print('[giga_proc]: map finished')
    print(80 * '=')

    print('[giga_proc]: reducing vocabulary file')
    unigram = {}
    for args in arglst:
        beg, end = args[0], args[1]
        part_str = '.part_{0}_{1}'.format(beg, end)
        fp = os.path.join(giga_path, voc_file_name + part_str)
        with open(fp) as fin:
            for ln in fin:
                c, w = ln.split(' ', 1)
                c = int(c)
                w = w.strip()
                unigram[w] = unigram.get(w, 0) + c
        os.remove(fp)

    with open(os.path.join(giga_path, voc_file_name), 'w') as fout:
        for w in sorted(unigram, key=lambda x: unigram[x], reverse=True):
            fout.write('{0} {1}\n'.format(unigram[w], w))

    print('[giga_proc]: reducing txt file')
    with open(os.path.join(giga_path, dump_file_name), 'w') as fout:
        for args in arglst:
            beg, end = args[0], args[1]
            part_str = '.part_{0}_{1}'.format(beg, end)
            fp = os.path.join(giga_path, dump_file_name + part_str)
            with open(fp) as fin:
                for ln in fin:
                    fout.write(ln)
            os.remove(fp)
    print('[giga_proc]: reduce finished')


def lowercase_vcb_file():
    unigram = {}
    with open(os.path.join(giga_path, voc_file_name)) as fin:
        for ln in fin:
            c, w = ln.split(' ', 1)
            c = int(c)
            w = w.strip().lower()
            unigram[w] = unigram.get(w, 0) + c
    with open(os.path.join(giga_path, voc_file_name), 'w') as fout:
        for w in sorted(unigram, key=lambda x: unigram[x], reverse=True):
            fout.write('{0} {1}\n'.format(unigram[w], w))

if __name__ == '__main__':
    main()
    # lowercase_vcb_file()

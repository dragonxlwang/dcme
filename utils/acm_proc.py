import os
import re
import sys
import math
from nltk.tokenize import sent_tokenize, word_tokenize
from multiprocessing.pool import ThreadPool
try:
    import xml.etree.cElementTree as etree
except ImportError:
    import xml.etree.ElementTree as etree

# this code prepares training/test data for dcme
# collect abstract as features, and predict corresponding journal/year
proc_path = os.path.join(os.path.expanduser('~'),
                         'data/acm_corpus/acmdl/proceeding')
jour_path = os.path.join(os.path.expanduser('~'),
                         'data/acm_corpus/acmdl/periodical')
proc_out_path = '/home/xwang95/data/acm_corpus/proc.txt'
jour_out_path = '/home/xwang95/data/acm_corpus/jour.txt'

absre = re.compile("<abstract>(.*?)</abstract>", re.DOTALL)
xmlre = re.compile("<.*?>")
PUNCTUATION = set([';', ':', ',', '.', '!', '?', "``", "''", "`", "\"", "'"])
jcodere = re.compile("<journal_code>(.*?)</journal_code>")
jnamere = re.compile("<journal_name>(.*?)</journal_name>")
jyearre = re.compile("<issue_date>.*?(\d*)</issue_date>")
jvyearre = re.compile('<volume>(\d*)</volume>')
paperre = re.compile("PROC-(.*?)\d*-(\d*)-(\d*).xml")


def process_one_file(file_path, is_journal, return_dict, pid):
    punc = ''.join(PUNCTUATION)
    punc += "[({})]"
    if(not is_journal):
        basename = os.path.basename(file_path)
        mobj = paperre.match(basename)
        if(mobj is None):
            print("{} abort".format(file_path))
            return
        vcode = mobj.group(1).strip()
        vyear = mobj.group(2).strip()
        vname = mobj.group(3).strip()
        if(len(vcode) == 0 or len(vyear) == 0 or len(vname) == 0):
            print("{} abort because of 0 len field".format(file_path))
            return
    with open(file_path, 'r') as f:
        txt = ' '.join(f.readlines())
        if(is_journal):
            mobj = jcodere.search(txt)
            if(mobj is None):
                print("{} abort because of journal code".format(file_path))
                return
            vcode = mobj.group(1).strip()
            mobj = jnamere.search(txt)
            if(mobj is None):
                print("{} abort because of journal name".format(file_path))
                return
            vname = '-'.join(mobj.group(1).split())

            mobj = jyearre.search(txt)
            if(mobj is None):
                vyear1 = None
            else:
                vyear1 = mobj.group(1)
            mobj = jvyearre.search(txt)
            if(mobj is None):
                vyear2 = None
            else:
                vyear2 = mobj.group(1)
            if(vyear1 is None and vyear2 is None):
                print("{} abort because of journal year".format(file_path))
                return
            elif(vyear1 is None and vyear2 is not None):
                if(len(vyear2) != 4):
                    return
                vyear = vyear2.strip().strip()
            else:
                vyear = vyear1.strip().strip()

        abslst = absre.findall(txt)
        abslst = [x.strip() for x in abslst]  # remove whitespace
        abslst = [x.replace('<par><![CDATA[', '') for x in abslst]  # rm leading
        abslst = [x.replace(']]></par>', '') for x in abslst]  # rm trailing
        abslst = [xmlre.sub('', x) for x in abslst]  # rm xml tags
        abslst = [' '.join([w.strip(punc) for w in x.split()]) for x in abslst]
        abslst = [x.strip() for x in abslst]  # remove whitespace
    # print('[acmdl]: {:8d} {}'.format(len(abslst), file_path))
    if (pid not in return_dict):
        return_dict[pid] = []
    return_dict[pid].append([vcode, vname, vyear, abslst])
    return


def job_map(args):
    dir_path, is_journal, beg, end, pid, return_dict = args
    flst = sorted(os.listdir(dir_path))[beg:end]
    for f in flst:
        process_one_file(os.path.join(dir_path, f),
                         is_journal, return_dict, pid)
    return


def job_reduce(return_dict, outfile_path):
    total_cnt = 0
    thread_cnt = 0
    with open(outfile_path, 'w') as fout:
        for k in return_dict.keys():
            thread_cnt = 0
            l = return_dict[k]
            for vcode, vname, vyear, abslst in l:
                abs_cnt = 0
                for abstxt in abslst:
                    fout.write('{}\n'.format(vcode))
                    fout.write('{}\n'.format(vname))
                    fout.write('{}\n'.format(vyear))
                    fout.write('{}\n'.format(abstxt))
                    abs_cnt += 1
                thread_cnt += abs_cnt
            total_cnt += thread_cnt
            print('[acmdl]: reduce thread {} ({}/{}/{})'.format(
                k, len(l), thread_cnt, total_cnt))
    return


def main(dir_path, outfile_path, is_journal=True):
    pn = 20
    flst = os.listdir(dir_path)
    arglst = []
    ret = dict()
    for i in range(pn):
        beg = int(math.ceil(float(len(flst)) / pn * i))
        end = int(math.ceil(float(len(flst)) / pn * (i + 1)))
        if(id == 0):
            beg = 0
        if(id == pn - 1):
            end = (len(flst))
        arglst.append([dir_path, is_journal, beg, end, i, ret])
    pool = ThreadPool(pn)
    pool.map(job_map, arglst)
    pool.close()
    pool.join()
    print(80 * '=')
    print('[acmdl]: map finished')
    print(80 * '=')
    job_reduce(ret, outfile_path)
    print(80 * '=')
    print('[acmdl]: reduce finished')
    print(80 * '=')
    return

if __name__ == '__main__':
    # main(jour_path, jour_out_path, True)
    # main(proc_path, proc_out_path, False)

import os


def proc():
    file_path = '~/data/MSRPC/msr_paraphrase_train.txt'
    file_path = os.path.expanduser(file_path)
    out_file_path = file_path.replace('.txt', '_3ln.txt')
    file = open(file_path)
    outfile = open(out_file_path, 'w')
    file.readline()
    lns = file.readlines()
    for ln in lns:
        parts = ln.strip().split('\t')
        if(len(parts) != 5):
            print("error in segmantation!")
            return
        label = parts[0]
        sent1 = parts[3]
        sent2 = parts[4]
        outfile.write(label + '\n')
        outfile.write(sent1 + '\n')
        outfile.write(sent2 + '\n')
    file.close()
    outfile.close()
    return

if __name__ == '__main__':
    proc()

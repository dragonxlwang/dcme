import gzip
try:
    import xml.etree.cElementTree as etree
except ImportError:
    import xml.etree.ElementTree as etree
import re
from collections import namedtuple

Token = namedtuple('Token', [
    'id', 'word', 'lemma', 'begin', 'end', 'pos', 'ner'])
Sentence = namedtuple('Sentence', [
    'id', 'tokens'])
Document = namedtuple('Document', [
    'id', 'date', 'type', 'headline', 'dateline',
    'text', 'sentences', 'coreferences'])
Mention = namedtuple('Mention', [
    'representative', 'sentence', 'start', 'end', 'head'])
YMD = namedtuple('YMD', 'year month day')


def parse_ymd(text):
    year = int(text[:4])
    month = int(text[4:6])
    day = int(text[6:])
    return YMD(year, month, day)


def parse_lisp(text):
    text = text.replace('(', ' ( ')
    text = text.replace(')', ' ) ')
    text = re.sub('\\s+', ' ', text).strip()
    stack = [[]]
    for cmd in text.split(' '):
        if cmd == '(':
            stack.append([])
        elif cmd == ')':
            last = tuple(stack[-1])
            del stack[-1]
            stack[-1].append(last)
        else:
            stack[-1].append(cmd)
    return stack[0][0]


def parse_text(xml):
    p = xml.findall('P')
    if len(p) == 0:
        p = [xml.text.strip()]
    else:
        p = [x.text.strip() for x in p]
    return p


def parse_mention(xml):
    return Mention(
        representative='representative' in xml.attrib,
        sentence=int(xml.find('sentence').text),
        start=int(xml.find('start').text),
        end=int(xml.find('end').text),
        head=int(xml.find('head').text))


def parse_token(xml):
    pos_tag = xml.find('POS')
    ner_tag = xml.find('NER')

    return Token(
        id=xml.attrib['id'],
        word=xml.find('word').text,
        lemma=xml.find('lemma').text,
        begin=int(xml.find('CharacterOffsetBegin').text),
        end=int(xml.find('CharacterOffsetEnd').text),
        pos=pos_tag.text if pos_tag is not None else None,
        ner=ner_tag.text if ner_tag is not None else None)


def parse_sentence(xml):
    return Sentence(
        id=xml.attrib['id'],
        tokens=[parse_token(x) for x in xml.find('tokens')])


def read_file(path,
              p_headline=True, p_dateline=True,
              p_coreferences=True, p_sentences=True,
              p_text=True):
    amp = re.compile(r'&amp;', re.IGNORECASE)
    bamp = re.compile(r'&')
    with gzip.open(path) as source:
        # source.readline()
        # file_line = source.readline() + "</FILE>"
        # file_tag = etree.fromstring(file_line)
        # file_id = file_tag.attrib['id']

        lines = []
        for line in source:
            # fix ampersand escape
            lines.append(bamp.sub('&amp;', amp.sub('&', line)))
            # lines.append(line)

            if line.strip() == '</DOC>':
                lines = ['<xml>'] + lines
                lines.append('</xml>')
                # print 80 * '='
                # for ln in lines:
                #     print ln
                # print 80 * '='
                xml = etree.fromstringlist(lines).find('DOC')

                doc_id = xml.attrib['id']
                date_str = doc_id.split('_')[-1].split('.')[0]
                date = parse_ymd(date_str)

                headline_xml = xml.find('HEADLINE')
                if headline_xml is not None and p_headline:
                    headline = headline_xml.text.strip()
                else:
                    headline = None

                dateline_xml = xml.find('DATELINE')
                if dateline_xml is not None and p_dateline:
                    dateline = dateline_xml.text.strip()
                else:
                    dateline = None

                coreferences = xml.find('coreferences')
                if coreferences is not None and p_coreferences:
                    coreferences = [[parse_mention(m) for m in x]
                                    for x in coreferences]
                else:
                    coreferences = []

                sentences = xml.find('sentences')
                if sentences is not None and p_sentences:
                    sentences = [parse_sentence(x)
                                 for x in xml.find('sentences')]
                else:
                    sentences = []

                text = xml.find('TEXT')
                if text is not None and p_text:
                    text = parse_text(text)
                else:
                    text = None

                yield Document(
                    id=xml.attrib['id'],
                    date=date,
                    type=xml.attrib['type'],
                    headline=headline,
                    dateline=dateline,
                    text=text,
                    sentences=sentences,
                    coreferences=coreferences)
                lines = []

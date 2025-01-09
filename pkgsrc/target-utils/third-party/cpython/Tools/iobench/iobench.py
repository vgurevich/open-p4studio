# -*- coding: utf-8 -*-
# This file should be kept compatible with both Python 2.6 and Python >= 3.0.

import itertools
import os
import platform
import re
import sys
import time
from optparse import OptionParser

out = sys.stdout

TEXT_ENCODING = 'utf8'
NEWLINES = 'lf'

# Compatibility
try:
    xrange
except NameError:
    xrange = range

def text_open(fn, mode, encoding=None):
    try:
        return open(fn, mode, encoding=encoding or TEXT_ENCODING)
    except TypeError:
        if 'r' in mode:
            mode += 'U' # 'U' mode is needed only in Python 2.x
        return open(fn, mode)

def get_file_sizes():
    for s in ['20 KiB', '400 KiB', '10 MiB']:
        size, unit = s.split()
        size = int(size) * {'KiB': 1024, 'MiB': 1024 ** 2}[unit]
        yield s.replace(' ', ''), size

def get_binary_files():
    return ((name + ".bin", size) for name, size in get_file_sizes())

def get_text_files():
    return (("%s-%s-%s.txt" % (name, TEXT_ENCODING, NEWLINES), size)
        for name, size in get_file_sizes())

def with_open_mode(mode):
    def decorate(f):
        f.file_open_mode = mode
        return f
    return decorate

def with_sizes(*sizes):
    def decorate(f):
        f.file_sizes = sizes
        return f
    return decorate


# Here begin the tests

@with_open_mode("r")
@with_sizes("medium")
def read_bytewise(f):
    """ read one unit at a time """
    f.seek(0)
    while f.read(1):
        pass

@with_open_mode("r")
@with_sizes("medium")
def read_small_chunks(f):
    """ read 20 units at a time """
    f.seek(0)
    while f.read(20):
        pass

@with_open_mode("r")
@with_sizes("medium")
def read_big_chunks(f):
    """ read 4096 units at a time """
    f.seek(0)
    while f.read(4096):
        pass

@with_open_mode("r")
@with_sizes("small", "medium", "large")
def read_whole_file(f):
    """ read whole contents at once """
    f.seek(0)
    while f.read():
        pass

@with_open_mode("rt")
@with_sizes("medium")
def read_lines(f):
    """ read one line at a time """
    f.seek(0)
    for line in f:
        pass

@with_open_mode("r")
@with_sizes("medium")
def seek_forward_bytewise(f):
    """ seek forward one unit at a time """
    f.seek(0, 2)
    size = f.tell()
    f.seek(0, 0)
    for i in xrange(0, size - 1):
        f.seek(i, 0)

@with_open_mode("r")
@with_sizes("medium")
def seek_forward_blockwise(f):
    """ seek forward 1000 units at a time """
    f.seek(0, 2)
    size = f.tell()
    f.seek(0, 0)
    for i in xrange(0, size - 1, 1000):
        f.seek(i, 0)

@with_open_mode("rb")
@with_sizes("medium")
def read_seek_bytewise(f):
    """ alternate read & seek one unit """
    f.seek(0)
    while f.read(1):
        f.seek(1, 1)

@with_open_mode("rb")
@with_sizes("medium")
def read_seek_blockwise(f):
    """ alternate read & seek 1000 units """
    f.seek(0)
    while f.read(1000):
        f.seek(1000, 1)


@with_open_mode("w")
@with_sizes("small")
def write_bytewise(f, source):
    """ write one unit at a time """
    for i in xrange(0, len(source)):
        f.write(source[i:i+1])

@with_open_mode("w")
@with_sizes("medium")
def write_small_chunks(f, source):
    """ write 20 units at a time """
    for i in xrange(0, len(source), 20):
        f.write(source[i:i+20])

@with_open_mode("w")
@with_sizes("medium")
def write_medium_chunks(f, source):
    """ write 4096 units at a time """
    for i in xrange(0, len(source), 4096):
        f.write(source[i:i+4096])

@with_open_mode("w")
@with_sizes("large")
def write_large_chunks(f, source):
    """ write 1e6 units at a time """
    for i in xrange(0, len(source), 1000000):
        f.write(source[i:i+1000000])


@with_open_mode("w+")
@with_sizes("small")
def modify_bytewise(f, source):
    """ modify one unit at a time """
    f.seek(0)
    for i in xrange(0, len(source)):
        f.write(source[i:i+1])

@with_open_mode("w+")
@with_sizes("medium")
def modify_small_chunks(f, source):
    """ modify 20 units at a time """
    f.seek(0)
    for i in xrange(0, len(source), 20):
        f.write(source[i:i+20])

@with_open_mode("w+")
@with_sizes("medium")
def modify_medium_chunks(f, source):
    """ modify 4096 units at a time """
    f.seek(0)
    for i in xrange(0, len(source), 4096):
        f.write(source[i:i+4096])

@with_open_mode("wb+")
@with_sizes("medium")
def modify_seek_forward_bytewise(f, source):
    """ alternate write & seek one unit """
    f.seek(0)
    for i in xrange(0, len(source), 2):
        f.write(source[i:i+1])
        f.seek(i+2)

@with_open_mode("wb+")
@with_sizes("medium")
def modify_seek_forward_blockwise(f, source):
    """ alternate write & seek 1000 units """
    f.seek(0)
    for i in xrange(0, len(source), 2000):
        f.write(source[i:i+1000])
        f.seek(i+2000)

# XXX the 2 following tests don't work with py3k's text IO
@with_open_mode("wb+")
@with_sizes("medium")
def read_modify_bytewise(f, source):
    """ alternate read & write one unit """
    f.seek(0)
    for i in xrange(0, len(source), 2):
        f.read(1)
        f.write(source[i+1:i+2])

@with_open_mode("wb+")
@with_sizes("medium")
def read_modify_blockwise(f, source):
    """ alternate read & write 1000 units """
    f.seek(0)
    for i in xrange(0, len(source), 2000):
        f.read(1000)
        f.write(source[i+1000:i+2000])


read_tests = [
    read_bytewise, read_small_chunks, read_lines, read_big_chunks,
    None, read_whole_file, None,
    seek_forward_bytewise, seek_forward_blockwise,
    read_seek_bytewise, read_seek_blockwise,
]

write_tests = [
    write_bytewise, write_small_chunks, write_medium_chunks, write_large_chunks,
]

modify_tests = [
    modify_bytewise, modify_small_chunks, modify_medium_chunks,
    None,
    modify_seek_forward_bytewise, modify_seek_forward_blockwise,
    read_modify_bytewise, read_modify_blockwise,
]

def run_during(duration, func):
    _t = time.time
    n = 0
    start = os.times()
    start_timestamp = _t()
    real_start = start[4] or start_timestamp
    while True:
        func()
        n += 1
        if _t() - start_timestamp > duration:
            break
    end = os.times()
    real = (end[4] if start[4] else time.time()) - real_start
    return n, real, sum(end[0:2]) - sum(start[0:2])

def warm_cache(filename):
    with open(filename, "rb") as f:
        f.read()


def run_all_tests(options):
    def print_label(filename, func):
        name = re.split(r'[-.]', filename)[0]
        out.write(
            ("[%s] %s... "
                % (name.center(7), func.__doc__.strip())
            ).ljust(52))
        out.flush()

    def print_results(size, n, real, cpu):
        bw = n * float(size) / 1024 ** 2 / real
        bw = ("%4d MiB/s" if bw > 100 else "%.3g MiB/s") % bw
        out.write(bw.rjust(12) + "\n")
        if cpu < 0.90 * real:
            out.write("   warning: test above used only %d%% CPU, "
                "result may be flawed!\n" % (100.0 * cpu / real))

    def run_one_test(name, size, open_func, test_func, *args):
        mode = test_func.file_open_mode
        print_label(name, test_func)
        if "w" not in mode or "+" in mode:
            warm_cache(name)
        with open_func(name) as f:
            n, real, cpu = run_during(1.5, lambda: test_func(f, *args))
        print_results(size, n, real, cpu)

    def run_test_family(tests, mode_filter, files, open_func, *make_args):
        for test_func in tests:
            if test_func is None:
                out.write("\n")
                continue
            if mode_filter in test_func.file_open_mode:
                continue
            for s in test_func.file_sizes:
                name, size = files[size_names[s]]
                #name += file_ext
                args = tuple(f(name, size) for f in make_args)
                run_one_test(name, size,
                    open_func, test_func, *args)

    size_names = {
        "small": 0,
        "medium": 1,
        "large": 2,
    }

    print("Python %s" % sys.version)
    if sys.version_info < (3, 3):
        if sys.maxunicode > 0xffff:
            text = "UCS-4 (wide build)"
        else:
            text = "UTF-16 (narrow build)"
    else:
        text = "PEP 393"
    print("Unicode: %s" % text)
    print(platform.platform())
    binary_files = list(get_binary_files())
    text_files = list(get_text_files())
    if "b" in options:
        print("Binary unit = one byte")
    if "t" in options:
        print("Text unit = one character (%s-decoded)" % TEXT_ENCODING)

    # Binary reads
    if "b" in options and "r" in options:
        print("\n** Binary input **\n")
        run_test_family(read_tests, "t", binary_files, lambda fn: open(fn, "rb"))

    # Text reads
    if "t" in options and "r" in options:
        print("\n** Text input **\n")
        run_test_family(read_tests, "b", text_files, lambda fn: text_open(fn, "r"))

    # Binary writes
    if "b" in options and "w" in options:
        print("\n** Binary append **\n")
        def make_test_source(name, size):
            with open(name, "rb") as f:
                return f.read()
        run_test_family(write_tests, "t", binary_files,
            lambda fn: open(os.devnull, "wb"), make_test_source)

    # Text writes
    if "t" in options and "w" in options:
        print("\n** Text append **\n")
        def make_test_source(name, size):
            with text_open(name, "r") as f:
                return f.read()
        run_test_family(write_tests, "b", text_files,
            lambda fn: text_open(os.devnull, "w"), make_test_source)

    # Binary overwrites
    if "b" in options and "w" in options:
        print("\n** Binary overwrite **\n")
        def make_test_source(name, size):
            with open(name, "rb") as f:
                return f.read()
        run_test_family(modify_tests, "t", binary_files,
            lambda fn: open(fn, "r+b"), make_test_source)

    # Text overwrites
    if "t" in options and "w" in options:
        print("\n** Text overwrite **\n")
        def make_test_source(name, size):
            with text_open(name, "r") as f:
                return f.read()
        run_test_family(modify_tests, "b", text_files,
            lambda fn: text_open(fn, "r+"), make_test_source)


def prepare_files():
    print("Preparing files...")
    # Binary files
    for name, size in get_binary_files():
        if os.path.isfile(name) and os.path.getsize(name) == size:
            continue
        with open(name, "wb") as f:
            f.write(os.urandom(size))
    # Text files
    chunk = []
    with text_open(__file__, "r", encoding='utf8') as f:
        for line in f:
            if line.startswith("# <iobench text chunk marker>"):
                break
        else:
            raise RuntimeError(
                "Couldn't find chunk marker in %s !" % __file__)
        if NEWLINES == "all":
            it = itertools.cycle(["\n", "\r", "\r\n"])
        else:
            it = itertools.repeat(
                {"cr": "\r", "lf": "\n", "crlf": "\r\n"}[NEWLINES])
        chunk = "".join(line.replace("\n", next(it)) for line in f)
        if isinstance(chunk, bytes):
            chunk = chunk.decode('utf8')
        chunk = chunk.encode(TEXT_ENCODING)
    for name, size in get_text_files():
        if os.path.isfile(name) and os.path.getsize(name) == size:
            continue
        head = chunk * (size // len(chunk))
        tail = chunk[:size % len(chunk)]
        # Adjust tail to end on a character boundary
        while True:
            try:
                tail.decode(TEXT_ENCODING)
                break
            except UnicodeDecodeError:
                tail = tail[:-1]
        with open(name, "wb") as f:
            f.write(head)
            f.write(tail)

def main():
    global TEXT_ENCODING, NEWLINES

    usage = "usage: %prog [-h|--help] [options]"
    parser = OptionParser(usage=usage)
    parser.add_option("-b", "--binary",
                      action="store_true", dest="binary", default=False,
                      help="run binary I/O tests")
    parser.add_option("-t", "--text",
                      action="store_true", dest="text", default=False,
                      help="run text I/O tests")
    parser.add_option("-r", "--read",
                      action="store_true", dest="read", default=False,
                      help="run read tests")
    parser.add_option("-w", "--write",
                      action="store_true", dest="write", default=False,
                      help="run write & modify tests")
    parser.add_option("-E", "--encoding",
                      action="store", dest="encoding", default=None,
                      help="encoding for text tests (default: %s)" % TEXT_ENCODING)
    parser.add_option("-N", "--newlines",
                      action="store", dest="newlines", default='lf',
                      help="line endings for text tests "
                           "(one of: {lf (default), cr, crlf, all})")
    parser.add_option("-m", "--io-module",
                      action="store", dest="io_module", default=None,
                      help="io module to test (default: builtin open())")
    options, args = parser.parse_args()
    if args:
        parser.error("unexpected arguments")
    NEWLINES = options.newlines.lower()
    if NEWLINES not in ('lf', 'cr', 'crlf', 'all'):
        parser.error("invalid 'newlines' option: %r" % NEWLINES)

    test_options = ""
    if options.read:
        test_options += "r"
    if options.write:
        test_options += "w"
    elif not options.read:
        test_options += "rw"
    if options.text:
        test_options += "t"
    if options.binary:
        test_options += "b"
    elif not options.text:
        test_options += "tb"

    if options.encoding:
        TEXT_ENCODING = options.encoding

    if options.io_module:
        globals()['open'] = __import__(options.io_module, {}, {}, ['open']).open

    prepare_files()
    run_all_tests(test_options)

if __name__ == "__main__":
    main()


# -- This part to exercise text reading. Don't change anything! --
# <iobench text chunk marker>

"""
1.
G?ttir allar,
??r gangi fram,
um sko?ask skyli,
um skyggnast skyli,
?v? at ?v?st er at vita,
hvar ?vinir
sitja ? fleti fyrir.

2.
Gefendr heilir!
Gestr er inn kominn,
hvar skal sitja sj??
Mj?k er br??r,
s? er ? br?ndum skal
s?ns of freista frama.

3.
Elds er ??rf,
?eims inn er kominn
ok ? kn? kalinn;
matar ok v??a
er manni ??rf,
?eim er hefr um fjall farit.

4.
Vatns er ??rf,
?eim er til ver?ar kemr,
?erru ok ?j??la?ar,
g??s of ??is,
ef s?r geta m?tti,
or?s ok endr??gu.

5.
Vits er ??rf,
?eim er v??a ratar;
d?lt er heima hvat;
at augabrag?i ver?r,
s? er ekki kann
ok me? snotrum sitr.

6.
At hyggjandi sinni
skyli-t ma?r hr?sinn vera,
heldr g?tinn at ge?i;
?? er horskr ok ??gull
kemr heimisgar?a til,
sjaldan ver?r v?ti v?rum,
?v? at ?brig?ra vin
f?r ma?r aldregi
en mannvit mikit.

7.
Inn vari gestr,
er til ver?ar kemr,
?unnu hlj??i ?egir,
eyrum hl??ir,
en augum sko?ar;
sv? n?sisk fr??ra hverr fyrir.

8.
Hinn er s?ll,
er s?r of getr
lof ok l?knstafi;
?d?lla er vi? ?at,
er ma?r eiga skal
annars brj?stum ?.
"""

"""
C'est revenir tard, je le sens, sur un sujet trop rebattu et d?j? presque oubli?. Mon ?tat, qui ne me permet plus aucun travail suivi, mon aversion pour le genre pol?mique, ont caus? ma lenteur ? ?crire et ma r?pugnance ? publier. J'aurais m?me tout ? fait supprim? ces Lettres, ou plut?t je lie les aurais point ?crites, s'il n'e?t ?t? question que de moi : Mais ma patrie ne m'est pas tellement devenue ?trang?re que je puisse voir tranquillement opprimer ses citoyens, surtout lorsqu'ils n'ont compromis leurs droits qu'en d?fendant ma cause. Je serais le dernier des hommes si dans une telle occasion j'?coutais un sentiment qui n'est plus ni douceur ni patience, mais faiblesse et l?chet?, dans celui qu'il emp?che de remplir son devoir.
Rien de moins important pour le public, j'en conviens, que la mati?re de ces lettres. La constitution d'une petite R?publique, le sort d'un petit particulier, l'expos? de quelques injustices, la r?futation de quelques sophismes ; tout cela n'a rien en soi d'assez consid?rable pour m?riter beaucoup de lecteurs : mais si mes sujets sont petits mes objets sont grands, et dignes de l'attention de tout honn?te homme. Laissons Gen?ve ? sa place, et Rousseau dans sa d?pression ; mais la religion, mais la libert?, la justice ! voil?, qui que vous soyez, ce qui n'est pas au-dessous de vous.
Qu'on ne cherche pas m?me ici dans le style le d?dommagement de l'aridit? de la mati?re. Ceux que quelques traits heureux de ma plume ont si fort irrit?s trouveront de quoi s'apaiser dans ces lettres, L'honneur de d?fendre un opprim? e?t enflamm? mon coeur si j'avais parl? pour un autre. R?duit au triste emploi de me d?fendre moi-m?me, j'ai d? me borner ? raisonner ; m'?chauffer e?t ?t? m'avilir. J'aurai donc trouv? gr?ce en ce point devant ceux qui s'imaginent qu'il est essentiel ? la v?rit? d'?tre dite froidement ; opinion que pourtant j'ai peine ? comprendre. Lorsqu'une vive persuasion nous anime, le moyen d'employer un langage glac? ? Quand Archim?de tout transport? courait nu dans les rues de Syracuse, en avait-il moins trouv? la v?rit? parce qu'il se passionnait pour elle ? Tout au contraire, celui qui la sent ne peut s'abstenir de l'adorer ; celui qui demeure froid ne l'a pas vue.
Quoi qu'il en soit, je prie les lecteurs de vouloir bien mettre ? part mon beau style, et d'examiner seulement si je raisonne bien ou mal ; car enfin, de cela seul qu'un auteur s'exprime en bons termes, je ne vois pas comment il peut s'ensuivre que cet auteur ne sait ce qu'il dit.
"""

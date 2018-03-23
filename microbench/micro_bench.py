#!/usr/bin/env python
import argparse
import sys
import os
import shutil

DBG = 1

__home = os.getcwd()
__nvs_root = '/nethome/pfernando3/nvs/microbench'  # root of fbench script location
__micro_root = '/nethome/pfernando3/nvs/microbench'

__empty = ''
__tmpfs = 'tmpfs'
__pmfs = 'pmfs'
__nvs = 'nvs'
__nvsd = 'nvsd'
__memcpy = 'memcpy'


__workload_l = []
__workload_l.append(__tmpfs)
__workload_l.append(__pmfs)
__workload_l.append(__nvs)
__workload_l.append(__nvsd)
__workload_l.append(__memcpy)


parser = argparse.ArgumentParser(prog="runscript", description="script to run yuma")

parser.add_argument('-b', dest='build',action='store_true', default=False, help="build benchmark")
parser.add_argument('-c', dest='clean',action='store_true', default=False, help="clean benchmark")
parser.add_argument('-r', dest='run',action='store_true', default=False, help="run benchmark")

parser.add_argument('-w', dest='workload', default=__empty, help='', choices=__workload_l)
parser.add_argument('-s', dest='snapsize', default=__empty, help='')
parser.add_argument('-v', dest='varsize', default=__empty, help='')


try:
    args = parser.parse_args()

except:
    sys.exit(0)


def dbg(s):
    if DBG == 1:
        print s


def msg(s):
    print '\n' + '>>>' + s + '\n'


def cd(dirt):
    dbg(dirt)

    if dirt == __home:
        os.chdir(__home);
    else:

        path = __home + '/' + dirt
        dbg(path)

        try:
            os.chdir(path)
        except:
            print 'invalid directory ', path
            sys.exit(0)


def sh(cmd):
    msg(cmd)
    try:
        os.system(cmd)
    except:
        print 'invalid cmd', cmd
        sys.exit(0)


def tonum(str):
    ch = str.strip()[-1]
    num = long(str[:-1])
    if (ch == 'k'):
        return num * 1024
    elif (ch == 'm'):
        return num * 1024 ** 2
    elif (ch == 'g'):
        return num * 1024 ** 3
    else:
        return num


def build_nvs(sysargs):
    args = sysargs
    w = args.workload

    cmd = 'cmake'
    d = '../nvstream/build'
    cd(d)

    if w==__tmpfs:
        cmd = cmd + ' -DFILE_STORE=ON -DTMPFS=ON'
    elif w == __pmfs:
        cmd = cmd + ' -DFILE_STORE=ON -DPMFS=ON'
    elif w == __nvs:
        msg("persistent nvs store")
    elif w == __nvsd:
        msg("delta nvs store")
        cmd = cmd + ' -DDELTA_STORE=ON'
    elif w == __memcpy:
        msg("volatile nvs store")
        cmd = cmd + ' -DMEMCPY=ON'

    msg(cmd)
    cmd = cmd + ' ..'
    sh(cmd)
    cmd = 'make'
    sh(cmd)



def build_micro(sysargs):
    args = sysargs
    w = args.workload

    d = 'src'
    cd(d)
    cmd = 'make micro_writer'
    sh(cmd)


def build_bench(sysargs):
    build_nvs(sysargs)
    build_micro(sysargs)

def run_bench(sysargs):
    args = sysargs
    w = args.workload
    s = args.snapsize
    v = args.varsize

    if w == __pmfs:
        cmd = 'rm -rf /mnt/pmfs/unity'
        sh(cmd)
    elif w == __tmpfs:
        cmd = 'rm -rf /mnt/tmpfs/unity'
        sh(cmd)
    elif w == __nvs or w == __memcpy or w == __nvsd:
        cmd = 'rm -rf /dev/shm/unity_NVS_ROOT*'
        sh(cmd)

    cmd = 'rm -rf /dev/shm/shm'
    sh(cmd)
    cmd = 'rm -rf /dev/shm/unity_NVS_ROOT*'
    sh(cmd)
#format heap
    cmd1 = 'mpirun -np 1 --bind-to core ../nvstream/build/src/tools/nvsformat'
    cmd2 = 'mpirun -np 1 --bind-to core src/micro_writer -v ' + v +' -s ' + s
    sh(cmd1)
    sh(cmd2)

def clean_nvs(sysargs):
    args=sysargs
    w=args.workload

    d = '../nvstream/build'
    cd(d)
    msg('deleting cmake cache..')
    try:
        os.remove('CMakeCache.txt')
    except OSError:
        pass
    cd(__home)


def clean_micro(sysargs):
    args = sysargs
    w = args.workload

    d = 'src'
    cd(d)
    cmd = 'make clean'
    sh(cmd)


def clean_bench(sysargs):
    clean_micro(sysargs)
    clean_nvs(sysargs)


if __name__ == '__main__':

    r = args.run
    c = args.clean
    b = args.build

    if b is True:
        build_bench(args)
    if c is True:
        clean_bench(args)
    if r is True:
        run_bench(args)
    else:
        sys.exit(0)

#!/usr/bin/env python
import argparse
import sys
import os
import shutil

DBG = 1

__home = os.getcwd()
__nvs_root = 'nvstream'  # root of fbench script location

__empty = ''
__tmpfs = 'tmpfs'
__pmfs = 'pmfs'
__memcpy = 'memcpy'
__nvstream = 'nvstream'
__dnvstream = 'dnvstream'


__store_l = []
__store_l.append(__tmpfs)
__store_l.append(__pmfs)
__store_l.append(__memcpy)
__store_l.append(__nvstream)
__store_l.append(__dnvstream)


parser = argparse.ArgumentParser(prog="buildscript", description="script to build nvs under different store configs")
parser.add_argument('-s', dest='store', default=__empty, help='', choices=__store_l)
parser.add_argument('-c', dest='clean', action='store_true', default=False, help="clean")
parser.add_argument('-b', dest='build', action='store_true', default=False, help="build")

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


def build_nvs(args):
    #default configuration is full data copy with streaming writes with crash-consistency
    s = args.sotre
    cmd = 'cmake .. '
    if(s == __tmpfs):
        cmd += '-DFILE_STORE=ON -DTMPFS=ON'
    elif(s == __pmfs):
        cmd += '-DFILE_STORE=ON -DPMFS=ON'
    elif(s == __memcpy):
        cmd += '-DMEMCPY=ON'
    elif(s == __nvstream):
        cmd += '' # nothing to do
    elif(s == __dnvstream):
        cmd += '-DDELTA_STORE=ON'


    d = __nvs_root + '/build'
    cd(d)
    sh(cmd)
    cd(__home)

def clean_nvs(args):
    d = __nvs_root + '/build'
    cd(d)
    msg('deleting cmake generated files')
    try:
        os.remove('CMakeCache.txt')
    except OSError:
        pass
    cd(__home)


def sh(cmd):
    msg(cmd)
    try:
        os.system(cmd)
    except:
        print 'invalid cmd', cmd
        sys.exit(0)





if __name__ == '__main__':

    s = args.store
    c = args.clean
    b = args.build


    if b is True:
        build_nvs(args)


    if c is True:
        clean_nvs(args)


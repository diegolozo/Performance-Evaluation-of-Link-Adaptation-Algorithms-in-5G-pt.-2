__author__      = "Jorge Ignacio Sandoval"
__version__     = "1.0"
__email__       = "jsandova@uchile.cl"

import time
import os , zipfile
from os import listdir
import sys
import gzip
import configparser
import pandas as pd
import threading
import multiprocessing


# CONSTANTS
# Set text colors
TXT_CLEAR = '\033[0m'
TXT_RED = '\033[0;31m'
TXT_GREEN = '\033[0;32m'
TXT_YELLOW = '\033[0;33m'
TXT_BLUE = '\033[0;34m'
TXT_MAGENTA = '\033[0;35m'
TXT_CYAN = '\033[0;36m'

# Set Background colors
BG__RED = '\033[0;41m'
BG__GREEN = '\033[0;42m'
BG__YELLOW= '\033[0;43m'
BG__BLUE = '\033[0;44m'
BG__MAGENTA = '\033[0;45m'
BG__CYAN = '\033[0;46m'


tic = time.time()
myhome = os.path.dirname(os.path.realpath(__file__))+"/"

config={}
VERBOSE=1
total_lines = 0
def main():
    global myhome, config, VERBOSE, total_lines
    DEBUG=0

    title = "Delay and PER"
    if VERBOSE: print(TXT_CYAN + title + TXT_CLEAR, flush = True)

    tic = time.time()
    validate()
    config = configuration()
    total_lines = 0

    groups = [ 0, 1, 2]

    num_processes = len(groups)
    if DEBUG: print(num_processes)
    pool = multiprocessing.Pool(processes=num_processes)
    pool.map(proc_group, groups)
    pool.close()
    pool.join()

    toc = time.time()
    print(f"\nAll Processed in: %.2f" %(toc-tic))

def configuration():
    """Read Configuration"""
    global myhome
    DEBUG=0
    # Read parameters of the simulation
    conf = {}
    conf_file = configparser.ConfigParser()
    conf_file.read(myhome + 'graph.ini')
    serverID1 = conf_file['general']['serverID1']
    serverID2 = conf_file['general']['serverID2']
    serverID3 = conf_file['general']['serverID3']
    serverIDs = [serverID1, serverID2, serverID3]
    conf['serverIDs'] =  serverIDs

    flowType1 = conf_file['general']['flowType1']
    flowType2 = conf_file['general']['flowType2']
    flowType3 = conf_file['general']['flowType3']
    flowTypes = [flowType1, flowType2, flowType3]
    conf['flowTypes'] =  flowTypes
    if DEBUG: print(conf)
    return conf

def validate():
    """Path Validation."""
    global myhome, VERBOSE
    if (len(sys.argv)>1):
        myhome = sys.argv[1] + "/"
        print(f"Processing: {TXT_CYAN}{myhome}{TXT_CLEAR}\n")
    else:
        print(TXT_RED + "No Folder defined bye" + TXT_CLEAR)
        exit()

    if ( not os.path.exists(myhome) ):
        print(f"Folder No Found: {TXT_RED}{myhome}{TXT_CLEAR}")
        exit()

def count_lines( ):
    global myhome, config, VERBOSE
    DEBUG=0

    tic = time.time()
    title = "Count Lines"
    file = "tcp-all-ascii.txt"
    if VERBOSE: print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    if VERBOSE: print('load', end = "", flush = True)
    # Open File
    if os.path.exists(myhome + file):
        if DEBUG: print(".plain.", end = "", flush = True)
        f = open(myhome + file)
    else:
        if DEBUG: print(".gzip.", flush = True)
        f = gzip.open(myhome + file + ".gz", 'rt') 
    if VERBOSE: print('ed', end = ".", flush = True)
    # count lines
    if VERBOSE: print('counting', end = ".", flush = True)
    for count, line in enumerate(f):
        pass
    result = count + 1
    if VERBOSE: print(result, end = ".", flush = True)
    toc = time.time()
    print(f"\tProcessed in: %.2f" %(toc-tic))
    return result

def proc_group( group = 0):
    """Delay and PER"""
    global myhome, config, VERBOSE, total_lines
    DEBUG=0
    OFS="\t"
    tic = time.time()
    if DEBUG: print (config, end = "", flush = True)
    if VERBOSE: print(TXT_CYAN + str(group) + TXT_CLEAR, end = ".", flush = True)

    # Open Server File
    # file = "tcp-all-ascii.txt"
    file = "ip-ascii-" + str(group)+ "-all.txt"
    if VERBOSE: print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        if DEBUG: print(".plain.", end = "", flush = True)
        f = open(myhome + file)
    else:
        if DEBUG: print(".gzip.", flush = True)
        f = gzip.open(myhome + file + ".gz", 'rt') 
    if VERBOSE: print('ed', end = ".", flush = True)

    # if VERBOSE: print()

    # Dictionaries with informations
    T = {}
    TF = {}
    L = {}
    ACK = {}
    Btx = {}
    Bdrop = {}
    Ptx = {}
    Pdrop = {}

    # line counter
    count = 0
    # status bar refresh rate
    update = 10000
    filter="/NodeList/" + config['serverIDs'][group] + "/"
    if DEBUG : print("filter: " + filter , flush = True)
    if config['flowTypes'][group] == "TCP":
        # read and process line by line
        count = 0
        for line in f:
            count += 1
            offset = 0
            if  line.strip().split()[9] == 'ECT': offset = 1
            if VERBOSE:
                if count % update == 0:
                    status_line(count, total_lines, group)

            if line.strip().split()[2].find(filter) < 0:
                # if not matchs
                continue
            else:
                if DEBUG: print("\nline: {}".format(line))
                # Read each register
                reg = {}
                reg['time'] = line.strip().split()[1]
                reg['direction'] = line.strip().split()[0]
                if line.strip().split()[0] == 't':
                    reg['server'] = line.strip().split()[23 + offset]
                    reg['client'] = line.strip().split()[25 + offset][:-1]
                    reg['source_port'] = line.strip().split()[27 + offset][1:]
                    reg['dest_port'] = line.strip().split()[29 + offset]

                elif line.strip().split()[0] == 'r':
                    reg['server'] = line.strip().split()[25 + offset][:-1]
                    reg['client'] = line.strip().split()[23 + offset]
                    reg['source_port'] = line.strip().split()[29 + offset]
                    reg['dest_port'] = line.strip().split()[27 + offset][1:]
                reg['prot'] = line.strip().split()[15 + offset]
                reg['len']  = int(line.strip().split()[22 + offset])
                
                if line.strip().split()[30 + offset].find("SYN") >= 0:
                    reg['size'] = 1
                elif  len(line.strip().split()) < 38 + offset:  
                    reg['size'] = 0
                else:
                    if line.strip().split()[37 + offset].find("size") >= 0:
                        reg['size'] = line.strip().split()[37 + offset][6:-1]
                    elif line.strip().split()[37 + offset].find("Fragment") >= 0:
                        reg['size'] = str(int(line.strip().split()[38 + offset].split(':')[1][:-1])  - int(line.strip().split()[38 + offset].split(':')[0][1:]))
                    else:
                        reg['size'] = 0
                reg['SN'] = str (int(line.strip().split()[31 + offset][4:]) + int(reg['size'])) #Error because size ???????

                if line.strip().split()[0] == 't':
                    index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['source_port'] + '-' + reg['dest_port'] + '-' + reg['SN']
                    T[index] = reg['time']
                    TF[index] = 0
                    L[index] = int(L[index] + reg['len']) if (index in L) else int(reg['len'])
                    ACK[index] = 0

                    index = reg['time']
                    Btx[index] = (Btx[index] + reg['len']) if (index in Btx) else reg['len']
                    Bdrop[index] = int(Bdrop[index] + reg['len']) if (index in Bdrop) else int(reg['len'])
                    Ptx[index] = (Ptx[reg['time']] + 1) if (index in Ptx) else 1
                    Pdrop[index] = (Pdrop[reg['time']] + 1) if (index in Pdrop) else 1
                    if DEBUG: print(reg)

                elif line.strip().split()[0] == 'r':
                    if (line.strip().split()[35 + offset] == "ns3::TcpOptionSack(blocks:"):
                        # SACK Case
                        blocks = line.strip().split()[36]
                        tmp = blocks.split(",")
                        if DEBUG: print("\nLine {}:({}) {}".format(count, len(line.strip().split()), line.strip()))
                        if DEBUG: print(tmp)
                        i = 2 
                        while (i < int(tmp[0])*2 - 1 ):
                            if DEBUG: print (i, end=" -> ")
                            serial = tmp[i].split("]")
                            if DEBUG: print( serial[0])
                            reg['SN'] = serial[0]

                            index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['source_port'] + '-' + reg['dest_port'] + '-' + reg['SN']
                            if (index in ACK) & (ACK[index] == 0):
                                TF[index] = reg['time']
                                ACK[index] = 1
                                Bdrop[T[index]] = int(Bdrop[T[index]]) - int(L[index])
                                Pdrop[T[index]] -= 1
                                if DEBUG: print(reg)

                            i = i +1
                    else:
                        # Normal Case
                        reg['SN'] = line.strip().split()[32 + offset][4:]
                        if DEBUG: print(reg)

                        index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['source_port'] + '-' + reg['dest_port'] + '-' + reg['SN']
                        if (index in ACK) :
                            if (ACK[index] == 0):
                                TF[index] = reg['time']
                                ACK[index] = 1
                                Bdrop[T[index]] = int(Bdrop[T[index]]) - int(L[index])
                                Pdrop[T[index]] -= 1
                                if DEBUG: print(reg)


    elif config['flowTypes'][group] == "UDP":
        # read and process line by line
        count = 0
        for line in f:
            count += 1
            offset = 0
            if  line.strip().split()[9] == 'ECT': offset = 1
            if VERBOSE:
                if count % update == 0:
                    status_line(count, total_lines, group)

            if line.strip().split()[2].find(filter) < 0 or line.strip().split()[0] == 'r' or len(line.strip().split()) < 33 + offset:
                # if not matchs
                continue
            else:
                if DEBUG: print("\nline: {}".format(line))
                # Read each register
                reg = {}
                reg['time'] = line.strip().split()[1]
                reg['direction'] = line.strip().split()[0]
                reg['server'] = line.strip().split()[23 + offset]
                reg['client'] = line.strip().split()[25 + offset][:-1]
                reg['source_port'] = line.strip().split()[29 + offset]
                reg['dest_port'] = line.strip().split()[31 + offset][:-1]
                reg['prot'] = line.strip().split()[15 + offset]
                reg['len']  = int(line.strip().split()[22 + offset])
                reg['SN'] = line.strip().split()[33 + offset][6:]

                if DEBUG: print(reg)

                index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['source_port'] + '-' + reg['dest_port'] + '-' + reg['SN']
                T[index] = reg['time']
                TF[index] = 0
                L[index] = int(L[index] + reg['len']) if (index in L) else int(reg['len'])
                ACK[index] = 0

                index = reg['time']
                Btx[index] = (Btx[index] + reg['len']) if (index in Btx) else reg['len']
                Bdrop[index] = int(Bdrop[index] + reg['len']) if (index in Bdrop) else int(reg['len'])
                Ptx[index] = (Ptx[reg['time']] + 1) if (index in Ptx) else 1
                Pdrop[index] = (Pdrop[reg['time']] + 1) if (index in Pdrop) else 1

        # Open UE File
        file = "ip-ascii-all-ue.txt"
        if VERBOSE: print('load', end = "", flush = True)
        if os.path.exists(myhome + file):
            if DEBUG: print(".plain.", end = "", flush = True)
            f = open(myhome + file)
        else:
            if DEBUG: print(".gzip.", flush = True)
            f = gzip.open(myhome + file + ".gz", 'rt') 
        if VERBOSE: print('ed', end = ".", flush = True)

        # if VERBOSE: print()
        # read and process line by line
        count = 0
        for line in f:
            count += 1
            offset = 0
            if  line.strip().split()[9] == 'ECT': offset = 1
            if DEBUG: print("line: {}".format(line))
            if VERBOSE:
                if count % update == 0:
                    status_line(count, total_lines, group)

            if line.strip().split()[0] == 't' or len(line.strip().split()) < 33 + offset:
                # if not matchs
                continue
            else:
                # Read each register
                reg = {}
                reg['time'] = line.strip().split()[1]
                reg['direction'] = line.strip().split()[0]
                reg['server'] = line.strip().split()[23 + offset]
                reg['client'] = line.strip().split()[25 + offset][:-1]
                reg['source_port'] = line.strip().split()[29 + offset]
                reg['dest_port'] = line.strip().split()[31 + offset][:-1]
                reg['prot'] = line.strip().split()[15 + offset]
                reg['len']  = int(line.strip().split()[22 + offset])
                reg['SN'] = line.strip().split()[33 + offset][6:]

                if DEBUG: print(reg)

                index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['source_port'] + '-' + reg['dest_port'] + '-' + reg['SN']
                if (index in ACK) :
                    if (ACK[index] == 0):
                        TF[index] = reg['time']
                        ACK[index] = 1
                        Bdrop[T[index]] = int(Bdrop[T[index]]) - int(L[index])
                        Pdrop[T[index]] -= 1
    elif config['flowTypes'][group] == "QUIC":
        # read and process line by line
        count = 0
        for line in f:
            count += 1
            offset = 0
            if  line.strip().split()[9] == 'ECT': offset = 1
            if VERBOSE:
                if count % update == 0:
                    status_line(count, total_lines, group)

            if line.strip().split()[2].find(filter) < 0 or len(line.strip().split()) < 33 + offset:
                # if not matchs
                continue
            else:
                if DEBUG: print("\nline {}: {}".format(count, line))
                # Read each register
                reg = {}
                reg['time'] = line.strip().split()[1]
                reg['direction'] = line.strip().split()[0]
                if line.strip().split()[0] == 't':
                    reg['server'] = line.strip().split()[23 + offset]
                    reg['client'] = line.strip().split()[25 + offset][:-1]
                    reg['source_port'] = line.strip().split()[29 + offset]
                    reg['dest_port'] = line.strip().split()[31 + offset][:-1]

                elif line.strip().split()[0] == 'r':
                    reg['server'] = line.strip().split()[25 + offset][:-1]
                    reg['client'] = line.strip().split()[23 + offset]
                    reg['source_port'] = line.strip().split()[31 + offset][:-1]
                    reg['dest_port'] = line.strip().split()[29 + offset]
                reg['prot'] = line.strip().split()[15 + offset]
                reg['len']  = int(line.strip().split()[22 + offset])
                reg['SN']  = line.strip().split()[36 + offset][:-2]
                
                if line.strip().split()[0] == 't':
                    index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['source_port'] + '-' + reg['dest_port'] + '-' + reg['SN']
                    index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['SN']
                    T[index] = reg['time']
                    TF[index] = 0
                    L[index] = int(L[index] + reg['len']) if (index in L) else int(reg['len'])
                    ACK[index] = 0

                    index = reg['time']
                    Btx[index] = (Btx[index] + reg['len']) if (index in Btx) else reg['len']
                    Bdrop[index] = int(Bdrop[index] + reg['len']) if (index in Bdrop) else int(reg['len'])
                    Ptx[index] = (Ptx[reg['time']] + 1) if (index in Ptx) else 1
                    Pdrop[index] = (Pdrop[reg['time']] + 1) if (index in Pdrop) else 1
                    if DEBUG: print(reg)
                elif line.strip().split()[0] == 'r':
                    for i in range(len(line.strip().split())):
                        if ( line.strip().split()[i] == "ns3::QuicSubHeader") :
                            if(line.strip().split()[i + 1].split("|")[1] == "ACK"):
                                reg['SN']  = line.strip().split()[i + 3].split("|")[0]

                                index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['source_port'] + '-' + reg['dest_port'] + '-' + reg['SN']
                                index = reg['client'] + '-' + reg['server'] + '-' + reg['prot'] + '-' + reg['SN']
                                if (index in ACK) & (ACK[index] == 0):
                                    TF[index] = reg['time']
                                    ACK[index] = 1
                                    Bdrop[T[index]] = int(Bdrop[T[index]]) - int(L[index])
                                    Pdrop[T[index]] -= 1
                                    if DEBUG: print(reg)



    # Making Delay File
    outfile = "ip-group-" + str(group) + "-delay.txt"
    fout = open(myhome + outfile, 'w')
    line = "Time" + OFS + "UE" + OFS + "seq" + OFS + "rtt"
    fout.write( line + '\n')
    for key, value in T.items():
        reg = {}
        tmp = key.split("-")
        reg['client'] = tmp[0]
        reg['server'] = tmp[1]
        reg['SN'] = tmp[2]
        reg['UE'] = int(tmp[0].split(".")[3]) - 1 

        if float(TF[key]) > 0 :
            line = str(T[key]) + OFS + str(reg['UE']) + OFS + str(reg['SN']) + OFS + str(float(TF[key]) - float(T[key]))
            # print(line)
            fout.write( line + '\n')

    # Making PER File
    outfile = "ip-group-" + str(group) + "-per.txt"
    fout = open(myhome + outfile, 'w')
    line = "Time" + OFS + "BytesTx" + OFS + "BytesDroped" + OFS + "PacketsTx" + OFS + "PacketsDroped"
    fout.write( line + '\n')
    for key, value in Btx.items():
        line = str(key) + OFS + str(Btx[key]) + OFS + str(Bdrop[key]) + OFS + str(Ptx[key]) + OFS + str(Pdrop[key])
        # print(line)
        fout.write( line + '\n')

    # if VERBOSE: print()
    toc = time.time()
    # if VERBOSE: print(f"\tProcessed in: %.2f" %(toc-tic))
    if VERBOSE: print(TXT_CYAN + "end-" + str(group) + TXT_CLEAR, end = ".", flush = True)

def status_line(now, final, group=0):
    if final > 0:
        # for i in range(group + 1):
        #     sys.stdout.write("\033[F")
        width = 20
        line = '[ '
        for i in range(width):
            if i < now/final*width:
                line += '#'
                
            else:
                line += '.'
        line += ' ]'
        # print( line, end = '\r' )
        print( str(group) + ': ' + line )
    else:
        print(".", end = "", flush = True)


if __name__ == '__main__':
    main()


exit ()
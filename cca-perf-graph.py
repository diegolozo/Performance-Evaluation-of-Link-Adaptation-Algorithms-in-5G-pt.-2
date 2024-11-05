__author__      = "Jorge Ignacio Sandoval"
__version__     = "1.1.0"
__email__       = "jsandova@uchile.cl"

from operator import mod
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.cbook import get_sample_data
from matplotlib.offsetbox import AnnotationBbox,OffsetImage
import matplotlib.patches as mpatches
# from scipy import special
import math
import pandas as pd
from datetime import datetime
import time
import sys
import configparser
import os , zipfile
from os import listdir
import json
from lib.simutils import *

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

colors = ['blue', 'orange', 'green', 'red', 'magenta', 'cyan', 'yellow', 'lightblue', 'pink', 'purple', 'lime']
linestyles = ['solid', 'dashed', 'dashdot', 'dotted']
show_title = 1
show_legend = 1

# Set graph 
plt.rc('font', size = 12)       # Set the default text font size
plt.rc('axes', titlesize = 16)  # Set the axes title font size
plt.rc('axes', labelsize = 16)  # Set the axes labels font size
plt.rc('xtick', labelsize = 14) # Set the font size for x tick labels
plt.rc('ytick', labelsize = 14) # Set the font size for y tick labels
plt.rc('legend', fontsize = 14) # Set the legend font size
plt.rc('figure', titlesize = 18)# Set the font size of the figure title
plt.rc('figure', labelsize = 16)# Set the font size of the figure label
plt.rc('lines', linewidth = 2)
title_font_size = 18
subtitle_font_size = 12


tic = time.time()
# myhome = os.path.dirname(os.path.realpath(__file__)) + "/"
root_dir = os.getcwd().split("out")[0]  # Dir where the git repository was cloned

if (len(sys.argv) > 1):
    myhome = sys.argv[1]
    if myhome[-1] != '/':
        myhome = myhome + '/'
    print(f"Processing: {TXT_CYAN}{myhome}{TXT_CLEAR}\n")
    if ( not os.path.exists(myhome) ):
        print(f"Folder not Found: {TXT_RED}{myhome}{TXT_CLEAR}")
        exit()
else:
    print(TXT_RED + "A path is needed!" + TXT_CLEAR)
    exit()


# Read parameters of the simulation
config = configparser.ConfigParser()
config.read(myhome + 'graph.ini')

NRTrace = config['general']['NRTrace']
TCPTrace = config['general']['TCPTrace']
# flowType = config['general']['flowType']
flowType1 = config['general']['flowType1']
flowType2 = config['general']['flowType2']
flowType3 = config['general']['flowType3']
flowTypes = [flowType1, flowType2, flowType3]
# tcpTypeId = config['general']['tcpTypeId']
tcpTypeId1 = config['general']['tcpTypeId1']
tcpTypeId2 = config['general']['tcpTypeId2']
tcpTypeId3 = config['general']['tcpTypeId3']
tcpTypeIds = [tcpTypeId1, tcpTypeId2, tcpTypeId3]
resamplePeriod=int(config['general']['resamplePeriod'])
simTime = float(config['general']['simTime'])
AppStartTime = float(config['general']['AppStartTime'])
iniTime = float(config['general']['iniTime']) if config.has_option('general', 'iniTime') else AppStartTime
endTime = float(config['general']['endTime']) if config.has_option('general', 'endTime') else simTime
rlcBuffer = float(config['general']['rlcBuffer'])
rlcBufferPerc = int(config['general']['rlcBufferPerc'])
serverType = config['general']['serverType']
myscenario = config['general']['myscenario'] if config.has_option('general', 'myscenario') else 'outdoor'
phyDistro = config['general']['phyDistro'] if config.has_option('general', 'phyDistro') else '1'
# serverID = config['general']['serverID']
serverID1 = config['general']['serverID1']
serverID2 = config['general']['serverID2']
serverID3 = config['general']['serverID3']
serverIDs = [serverID1, serverID2, serverID3]

ip_net_Server = config['general']['ip_net_Server1']
ip_net_Server = '.'.join(ip_net_Server.split('.')[:2])

ip_net_UE = config['gNb']['ip_net_UE']
ip_net_UE = '.'.join(ip_net_UE.split('.')[:2])

UENum = int(config['general']['UENum'])
UEN1 = int(config['general']['UEN1'])
UEN2 = int(config['general']['UEN2'])
UEN3 = int(config['general']['UEN3'])
UENs = [UEN1, UEN2, UEN3]
AQMEnq = config['general']['AQMEnq']
if AQMEnq == "dummy":
    AMQEnq = "None"
AQMDeq = config['general']['AQMDeq']
if AQMDeq == "dummy":
    AQMDeq = "None"

AppStartTimes = []
for ue in range(UENum):
    AppStartTimes.append(float(config['UE-' + str(ue + 1)]['AppStartTime']))

SegmentSize = float(config['general']['SegmentSize']) 
dataRate = float(config['general']['dataRate']) if config.has_option('general', 'dataRate') else 1000
dataRate1 = float(config['general']['dataRate1']) if config.has_option('general', 'dataRate1') else dataRate
dataRate2 = float(config['general']['dataRate2']) if config.has_option('general', 'dataRate2') else dataRate
dataRate3 = float(config['general']['dataRate3']) if config.has_option('general', 'dataRate3') else dataRate
dataRates = [dataRate1, dataRate2, dataRate3]
gNbNum = int(config['gNb']['gNbNum'])
gNbX = float(config['gNb']['gNbX'])
gNbY = float(config['gNb']['gNbY'])
gNbD = float(config['gNb']['gNbD'])
enableBuildings = int(config['building']['enableBuildings'])
gridWidth = int(config['building']['gridWidth'])
buildN = int(config['building']['buildN'])
buildX = float(config['building']['buildX'])
buildY = float(config['building']['buildY'])
buildDx = float(config['building']['buildDx'])
buildDy = float(config['building']['buildDy'])
buildLx = float(config['building']['buildLx'])
buildLy = float(config['building']['buildLy'])


# Max limit in plot
thr_limit = max(dataRates)*1.2
rtt_limit = 200
cwd_limit = 2000
cwd_limit = rlcBuffer/ rlcBufferPerc / UENum
cwd_limit = max(dataRates) * rlcBufferPerc / 100 * 2.5
inflight_limit = 2 * max(dataRates)
inflight_limit = rlcBuffer/ rlcBufferPerc / UENum
inflight_limit = max(dataRates) * rlcBufferPerc / 100 * 1.5
qdelay_limit = 100


points = np.array([])
parts = np.array([[]])
myscenario = config['general']['myscenario'] if config.has_option('general', 'myscenario') else 'outdoor'
circlepath = int(config['general']['circlepath']) if config.has_option('general', 'circlepath') else 0

if phyDistro == "3":
    parts=np.array([[ 0, 3, "0-3"],
                    [ 3, 7,"3-7"],
                    [ 7, 17.4,"7-17"],
                    [ 17.4, 19.9,"17-20"],
                    [ 19.9, 28.4,"20-28"],
                    [ 28.4, 31,"28-31"],
                    [ 31, 39.4,"32-39"],
                    [ 39.4, 43.6,"39-44"],
                    [ 43.6, 51,"44-51"],
                    [ 51, 60,"51-60"],
                    [ 0, 60,"0-60"],
                ])
else:
    if circlepath:
        print('circlepath')
        parts=np.array([[ 0, 100,"0-100"]])
        if (iniTime < 100):
            parts = np.vstack([[ 86.5, 100,"86-100"], parts])
        if (iniTime < 86.5):
            parts = np.vstack([[ 61, 86.5,"61-86"], parts])
        if (iniTime < 61):
            parts = np.vstack([[ 39, 60,"39-61"], parts])
        if (iniTime < 39):
            parts = np.vstack([[ 12.5, 39,"12-39"], parts])
        if (iniTime < 12.5):
            parts = np.vstack([[ 0, 12.5, "0-12"], parts])
        parts=np.array([
                [ 0, 12.5, "0-12"],
                [ 12.5, 39,"12-39"],
                [ 39, 60,"39-61"],
                [ 61, 86.5,"61-86"],
                [ 86.5, 100,"86-100"],
                [ 0, 100,"0-100"]
            ])

    else:
        parts=np.array([[ 0, 60,"0-60"]])
        if (iniTime < 60):
            parts = np.vstack([[ 40, 60,"40-60"], parts])
        if (iniTime < 40):
            parts = np.vstack([[ 20, 40,"20-40"], parts])
        if (iniTime < 20):
            parts = np.vstack([[ 0, 20, "0-20"], parts])


prefix = ''
subtitle = ''
for i in range(len(tcpTypeIds)):
    if tcpTypeIds[i] == "TcpCubic" or tcpTypeIds[i] == "TcpBbr" :
        tcpTypeIds[i] = tcpTypeIds[i][0:3]+tcpTypeIds[i][3:].upper()
    
    if tcpTypeIds[i][:2] == "Tcp" :
        prefix += tcpTypeIds[i][3:] +'-'
        subtitle += str(UENs[i]) + ' UE: ' + tcpTypeIds[i][3:] + '; '
    else:
        prefix += tcpTypeIds[i] +'-'
        subtitle += str(UENs[i]) + ' UE: ' + tcpTypeIds[i] + '; '

prefix += serverType + '-' + str(rlcBufferPerc) + '-' + 'UE'
subtitle += "Enq: " + AQMEnq + "; Deq: " + AQMDeq

for i in range(len(tcpTypeIds)):
    prefix += '_' + str(UENs[i] )
prefix += '-AQMEnq_' + AQMEnq + '-AQMDeq_' + AQMDeq +'-' + str(int(iniTime)) + '_' + str(int(endTime)) +'-'

# print(prefix)
# exit()
def main():
    global parts, myhome
    tic = time.time()
    parts = find_zones(myhome)
    remove_graph(myhome)
    graph_mobility(myhome)
    sdelay = graph_rlc_sdelay(myhome)
    [sinr_control, sinr_data] = graph_SINR(myhome)
    # [cqi, bler ] = graph_CQI_BLER(myhome)
    path_loss = graph_path_loss(myhome)
    graph_thr_tx(myhome)
    graph_tcp(myhome)
    rtt = graph_rtt(myhome)
    rlc = graph_thr_rlcbuffer2(myhome)
    drp = graph_thr_packetdrop(myhome)
    jain_index = graph_jain_index(myhome)
    calculate_metrics(myhome)
    results = configparser.ConfigParser()
    results.read(myhome + 'results.ini')

    res_summary = {'RTT'        : rtt,
                   'BUFFER_RLC' : rlc,
                   'SDELAY_RLC' : sdelay,
                   'SINR_CONTROL' : sinr_control,
                   'SINR_DATA'  : sinr_data,
                #    'CQI'        : cqi,
                #    'BLER'       : bler,
                   'PATH_LOSS'  : path_loss,
                   }
    for key, value in res_summary.items():
        for p in range(parts.shape[0]):
            value_part = value[p]
            
            for ue in range(len(value_part)):
                results['part-' + str((p+1) % parts.shape[0])][key + '_MEAN-' + str((ue+1) % len(value_part))] = str(value_part[ue][0]) if not math.isnan(value_part[ue][0]) else '0'
                results['part-' + str((p+1) % parts.shape[0])][key + '_STD-' + str((ue+1) % len(value_part))] = str(value_part[ue][1]) if not math.isnan(value_part[ue][1]) else '0'
                if len(value_part[ue]) > 2:
                    results['part-' + str((p+1) % parts.shape[0])][key + '_BINS-' + str((ue+1) % len(value_part))] = np.array2string(value_part[ue][2], separator=',')
                    results['part-' + str((p+1) % parts.shape[0])][key + '_CDF-'  + str((ue+1) % len(value_part))] = np.array2string(value_part[ue][3], separator=',')
                

    for p in range(parts.shape[0]):
        results['part-' + str((p+1) % parts.shape[0])]['THR_DRP_MEAN'] = str(drp[p][0]) if not math.isnan(drp[p][0]) else '0'
        results['part-' + str((p+1) % parts.shape[0])]['THR_DRP_STD'] = str(drp[p][1])  if not math.isnan(drp[p][1]) else '0'

        results['part-' + str((p+1) % parts.shape[0])]['JAIN_MEAN'] = str(jain_index[p][0]) if not math.isnan(jain_index[p][0]) else '0'
        results['part-' + str((p+1) % parts.shape[0])]['JAIN_STD'] = str(jain_index[p][1])  if not math.isnan(jain_index[p][1]) else '0'
        
    with open(myhome + 'results.ini', 'w') as configfile:
        results.write(configfile)

    toc = time.time()
    print(f"\nAll Processed in: %.2f" %(toc-tic))

@info_n_time_decorator("Find Zones")
def find_zones(myhome):
    """Graph the path loss of the UEs"""
    DEBUG=0

    file = "DlPathlossTrace.txt"
    title = "Path Loss"

    if os.path.exists(myhome + file):
        PLOSS = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        PLOSS = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )


    PLOSS.set_index('Time(sec)', inplace = True)
    PLOSS = PLOSS.loc[PLOSS['IMSI'] != 0]
    PLOSS = PLOSS[PLOSS['pathLoss(dB)'] < 0]
    mydata = abs(PLOSS[PLOSS['IMSI'] == 1].diff())
    if DEBUG: print(mydata.sort_values(by=['pathLoss(dB)'], ascending=False).head(5).sort_index().index.tolist())

    if DEBUG: print(mydata['pathLoss(dB)'].std(), mydata['pathLoss(dB)'].mean(), mydata['pathLoss(dB)'].median() )
    if DEBUG: print(mydata[mydata['pathLoss(dB)'] > 6 * mydata['pathLoss(dB)'].std()]['pathLoss(dB)'])
    zones=mydata[mydata['pathLoss(dB)'] > 6 * mydata['pathLoss(dB)'].std()]['pathLoss(dB)'].index.tolist()
    zones.append(mydata.index.max())
    if DEBUG: print(zones)
    if DEBUG: print(type(zones))
    # check distance, remove changes < 1 seg
    i = 0
    while i <  len(zones) - 1:
        if zones[i + 1] - zones[i] < 1:
            zones.pop(i)
        else:
            i+=1
    # first zone start at 0
    if len(zones) > 0 and zones[0] < 1:
        zones[0] = 0
    # last zone finsh at simTime
    if len(zones) > 1 :
        zones[-1] = max(zones[-1], simTime)
    if DEBUG: print('zones:', zones)

    # result=np.array([[ 0, max(mydata.index.max(), simTime), "00-"+f"{round(max(mydata.index.max(), simTime)):02}"]])
    result=[]
    if DEBUG: print('result: ', result)
    for i in range(len(zones) - 1):
        result.append( [zones[i], zones[i + 1], f"{round(zones[i]):02}" + '-' + f"{round(zones[i + 1]):02}"])
        if DEBUG: print(i, 'result: ', result)
    result.append([ 0, max(mydata.index.max(), simTime), "00-"+f"{round(max(mydata.index.max(), simTime)):02}"])
     
    if DEBUG: print('result: ', result)
    return np.array(result)

@info_n_time_decorator("RLC Buffer Stats")
def graph_rlc_sdelay(myhome):
    DEBUG=0
    title = "RLC Buffer Sojourn Delay"

    DF = pd.DataFrame()
    ue = 0
    for z in range(len(UENs)):
        for u in range(UENs[z]):
            file = "buffer_" + str(ue) + "_aqm_stats.txt"
            ue +=1
            
            if os.path.exists(myhome + file):
                if DEBUG: print(myhome + file)
                TMP = pd.read_csv(myhome + file, sep = ";", on_bad_lines = 'skip' )
            else:
                file =  file + ".gz"
                if DEBUG: print(myhome + file)
                TMP = pd.read_csv(myhome + file, compression = 'gzip', sep = ";", on_bad_lines = 'skip' )
            TMP['rnti'] = ue
            DF = pd.concat([DF,TMP], sort=False)
    DF['Simulation Time [ns]'] = DF['Simulation Time [ns]'] / 1E9
    DF = DF.rename(columns={'Simulation Time [ns]': 'Time' })
    DF.index = pd.to_datetime(DF['Time'], unit='s')
    DF = DF.groupby('rnti').resample(str(resamplePeriod)+'ms')['Delay [ms]'].max().ffill()
    DF = DF.reset_index(level = 0)
    DF.index = DF.index.astype(int) / 1e9
    if DEBUG: print(DF)
    if DEBUG: print(parts)
    result = []
    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        if DEBUG: print(p, z)
        fig, ax = plt.subplots()
        if DEBUG: print(DF[(DF.index >= float(x)) & (DF.index <= float(y))])
        DF[(DF.index >= float(x)) & (DF.index <= float(y))].groupby(['rnti'])['Delay [ms]'].plot()
        ax.set_xlabel("Time [s]")
        ax.set_ylabel("Sojourn Delay [ms]")
        ax.set_ylim([0 , qdelay_limit])
        ax.set_xlim([float(x) , float(y)])
        if show_title:
            plt.suptitle(title, y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        if show_legend:
            text_legend = DF['rnti'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, 
                        loc = 'upper center',
                        ncol = max([1, len(DF['rnti'].unique()) // 2 ])
                        )

        fig.savefig(myhome + prefix + 'RLCsdelay-' + str(z) + '.png')


        fig, ax = plt.subplots()
        cdf_metric = []
        for ue in DF['rnti'].drop_duplicates().sort_values().unique():
            data = np.array(DF[(DF.index >= float(x)) & (DF.index <= float(y)) & (DF['rnti'] == ue)]['Delay [ms]'].values)
            count, bins_count = np.histogram(data, bins=100) 
            pdf = count / sum(count) 
            cdf = np.cumsum(pdf)
            
            plt.plot(bins_count[1:], cdf)

            cdf_metric.append([bins_count[1:], cdf])
        data = np.array(DF[(DF.index >= float(x)) & (DF.index <= float(y)) ]['Delay [ms]'].values)
        count, bins_count = np.histogram(data, bins=100) 
        pdf = count / sum(count) 
        cdf = np.cumsum(pdf)
        cdf_metric.append([bins_count[1:], cdf])

        ax.set_ylim([0 , 1.1])
        ax.set_xlim([0 , qdelay_limit])
        ax.set_ylabel("CDF")
        ax.set_xlabel("Sojourn Delay [ms]")

        if show_title:
            plt.suptitle(title + ' CDF', y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        if show_legend:
            text_legend = DF['rnti'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, 
                        loc = 'upper center',
                        ncol = max([1, len(DF['rnti'].unique()) // 2 ])
                        )
        fig.savefig(myhome + prefix + 'RLCsdelayCDF-' + str(z) + '.png')

        
        # Metrics
        metric = []
        for ue in DF['rnti'].drop_duplicates().sort_values().unique():
            metric.append([ DF[(DF.index >= float(x)) & (DF.index <= float(y)) & (DF['rnti'] == ue)]['Delay [ms]'].mean(),
                              DF[(DF.index >= float(x)) & (DF.index <= float(y)) & (DF['rnti'] == ue)]['Delay [ms]'].std(),
                              cdf_metric[int(ue)-1][0],
                              cdf_metric[int(ue)-1][1] ])
        metric.append([ DF[(DF.index >= float(x)) & (DF.index <= float(y)) ]['Delay [ms]'].mean(),
                            DF[(DF.index >= float(x)) & (DF.index <= float(y)) ]['Delay [ms]'].std(),
                            cdf_metric[-1][0],
                            cdf_metric[-1][1] ])
            
        result.append(metric)
    if DEBUG: print(result)
    plt.close()
    return result

@info_n_time_decorator("Jain's fairness Index")
def graph_jain_index(myhome):
    """Graph Jain's fairness Index."""
    # tic = time.time()
    title = "Jain's fairness Index"
    file = "NrDlPdcpRxStats.txt"
    # print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    # print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        thrrx = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        thrrx = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    # print('ed', end = ".", flush = True)

    thrrx = thrrx.groupby(['time(s)','rnti'])['packetSize'].sum().reset_index()
    thrrx.index = pd.to_datetime(thrrx['time(s)'],unit='s')

    thrrx = pd.DataFrame(thrrx.groupby('rnti').resample(str(resamplePeriod)+'ms').packetSize.sum().replace(np.nan, 0))
    thrrx = thrrx.reset_index(level = 0)

    thrrx['InsertedDate'] = thrrx.index
    thrrx['deltaTime'] = thrrx['InsertedDate'].astype(int) / 1e9
    thrrx['Time'] = thrrx['deltaTime']

    thrrx['deltaTime'] = thrrx.groupby('rnti').diff()['deltaTime']

    thrrx.loc[~thrrx['deltaTime'].notnull(),'deltaTime'] = thrrx.loc[~thrrx['deltaTime'].notnull(),'InsertedDate'].astype(int) / 1e9
    thrrx['throughput'] =  thrrx['packetSize']*8 / thrrx['deltaTime'] / 1e6
    thrrx['throughput'] = thrrx['throughput'].astype(float)

    thrrx = thrrx.set_index('Time')
    thrrx['rnti'] = UENum - thrrx['rnti'] + 1


    thrrx['sqrt_thr']=thrrx['throughput']*thrrx['throughput']
    jain_num = pow(thrrx.groupby(['Time' ])['throughput'].sum(),2)
    jain_den = thrrx.groupby(['Time' ])['rnti'].count() * thrrx.groupby(['Time' ])['sqrt_thr'].sum()
    jain = jain_num / jain_den
    # print('plotting', end = ".", flush = True)
    fig, ax = plt.subplots()
    jain.plot()

    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Jain's fairness Index")
    ax.set_ylim([0 , 1.1])
    ax.set_xlim([0 , simTime])
    if show_title:
        plt.suptitle(title, y = 0.99, fontsize = title_font_size)
        plt.title(subtitle, fontsize = subtitle_font_size)
    
    fig.savefig(myhome + prefix + 'JAIN' + '.png')
    plt.close()

    jain_index = []
    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        jain_part = [jain[(jain.index >= max([float(x), max(AppStartTimes)])) & (jain.index <= float(y))   ].mean(), jain[(jain.index >= max([float(x), max(AppStartTimes)])) & (jain.index <= float(y))   ].std()]
        jain_index.append(jain_part)

    # toc = time.time()
    # print(f"\tProcessed in: %.2f" %(toc-tic))
    return jain_index

@info_n_time_decorator("Removing existing files")
def remove_graph(myhome):
    """Delete all png files."""
    for item in os.listdir(myhome):
        if item.endswith(".png"):
            os.remove(os.path.join(myhome, item))

    return True

@info_n_time_decorator("Mobility")
def graph_mobility(myhome):
    """Plots the displacement of the UEs."""
    file = "mobilityPosition.txt"
    jsonfile = "PhysicalDistribution.json"
    title = "Mobility"
    
    if os.path.exists(myhome + file):
        mob = pd.read_csv(myhome + file, sep="\t", on_bad_lines='skip')
    else:
        mob = pd.read_csv(myhome + file + ".gz", compression='gzip', sep="\t", on_bad_lines='skip')
    mob.set_index('Time', inplace=True)

    fig, ax = plt.subplots(figsize=(6, 6))
    for ue in mob['UE'].unique():
        ax1 = mob[mob['UE'] == ue].plot.scatter(x='x', y='y', ax=ax, c=colors[(ue-1) % len(colors)])

    gNbicon = plt.imread(get_sample_data(os.path.join(root_dir, "img", "gNb.png")))
    gNbbox = OffsetImage(gNbicon, zoom=0.25)

    house_icon = plt.imread(get_sample_data(os.path.join(root_dir, "img", "house.png")))
    house_box = OffsetImage(house_icon, zoom=0.05)
    tree_icon = plt.imread(get_sample_data(os.path.join(root_dir, "img", "tree.png")))
    tree_box = OffsetImage(tree_icon, zoom=0.05)

    if os.path.exists(myhome + jsonfile):
        with open(myhome + jsonfile) as json_file:
            data = json.load(json_file)

        # Set gnb
        for g in data['gnb']:
            gNbPos = [g['x'], g['y'] * 1.1]
            gNbab = AnnotationBbox(gNbbox, gNbPos, frameon=False, zorder = 3)
            ax.add_artist(gNbab)

        # Set buildings and trees
        if enableBuildings:
            for b in data['Buildings']:
                if ("ExternalWallsType" in b) and (b['ExternalWallsType'] == 0):
                    tree_ab = AnnotationBbox(tree_box, (b['xmin'], b['ymin']), frameon=False, zorder = 1)
                    ax.add_artist(tree_ab)
                else:
                    house_ab = AnnotationBbox(house_box, (b['xmin'], b['ymin']), frameon=False, zorder = 1)
                    ax.add_artist(house_ab)

    else:
        # Default position for buildings and trees if JSON doesn't exist
        if enableBuildings:
            for b in range(buildN):
                row, col = divmod(b, gridWidth)
                x = buildX + (buildLx + buildDx) * col
                y = buildY + (buildLy + buildDy) * row
                if b % 2 == 0:
                    tree_ab = AnnotationBbox(tree_box, (x, y), frameon=False, zorder = 1)
                    ax.add_artist(tree_ab)
                else:
                    house_ab = AnnotationBbox(house_box, (x, y), frameon=False, zorder = 1)
                    ax.add_artist(house_ab)

        # Set gnb
        for g in range(gNbNum):
            gNbPos = [gNbX, (gNbY + g * gNbD) * 1.1]
            gNbab = AnnotationBbox(gNbbox, gNbPos, frameon=False, zorder = 3)
            ax.add_artist(gNbab)

    # Set UE
    UEicon = plt.imread(get_sample_data(os.path.join(root_dir, "img", "UE.png")))
    UEbox = OffsetImage(UEicon, zoom=0.02)
    for ue in mob['UE'].unique():
        UEPos = mob[mob['UE'] == ue][['x', 'y']].iloc[-1:].values[0] * 1.01
        UEab = AnnotationBbox(UEbox, UEPos, frameon=False, zorder = 2)
        ax.add_artist(UEab)

    plt.xlim([0, 100])
    plt.ylim([0, 100])
    ax.set_xlabel("Distance [m]")
    ax.set_ylabel("Distance [m]")

    if show_title:
        plt.suptitle(title, y=0.99, fontsize=title_font_size)
        plt.title(subtitle, fontsize=subtitle_font_size)

    fig.savefig(myhome + prefix + 'MOBILITY' + '.png', dpi = 600)
    plt.close()    
    return True

@info_n_time_decorator("SINR of all UEs")
def graph_SINR(myhome):
    """Graph the SINR of the UEs"""
    DEBUG=0
    files={'Control':'DlCtrlSinr.txt',
            'Data': 'DlDataSinr.txt'}
    result = []
    for key, value in files.items():
        #tic = time.time()
        title = "SINR " + key
        file = value
        # print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
        if DEBUG: print('load', end = "", flush = True)
        if os.path.exists(myhome + file):
            SINR = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
        else:
            SINR = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip')
        if DEBUG: print('ed', end = ".", flush = True)
        SINR = SINR.rename(columns={'RNTI': 'rnti', 'SINR(dB)': 'sinr'})

        SINR = SINR[SINR['rnti'] != 0]
        SINR['rnti'] = UENum - SINR['rnti'] +1
        if DEBUG: print(SINR)

        SINR.index = pd.to_datetime(SINR['Time'],unit = 's')
        if DEBUG: print(SINR)
        SINR = pd.DataFrame(SINR.groupby('rnti').resample(str(resamplePeriod)+'ms').sinr.mean().ffill())
        if DEBUG: print ('Resampled')
        if DEBUG: print (SINR)
        SINR = SINR.reset_index(level = 0)
        if DEBUG: print (SINR)
        SINR['Time'] = SINR.index.astype(int) / 1e9
        SINR = SINR.set_index('Time')
        if DEBUG: print (SINR)

        # PLOT
        fig, ax = plt.subplots()
        SINR.groupby('rnti')['sinr'].plot()
        plt.ylim([min(15, SINR['sinr'].min()) , max(30,SINR['sinr'].max())])
        ax.set_ylabel("SINR(dB)")
        ax.set_xlabel("Time(s)")

        if show_title:
            plt.suptitle(title, y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)
        if show_legend:
            text_legend = SINR['rnti'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, 
                      loc = 'lower center',
                      ncol = max([1, len(SINR['rnti'].unique()) // 3 ])
                      )

        # Metrics
        res = []
        for p in range(parts.shape[0] ):
            [x, y, z] = parts[p,:]
            metric = []
            for ue in SINR['rnti'].drop_duplicates().sort_values().unique():
                metric.append([ SINR[(SINR.index >= float(x)) & (SINR.index <= float(y)) & (SINR['rnti'] == ue)]['sinr'].mean(),
                                SINR[(SINR.index >= float(x)) & (SINR.index <= float(y)) & (SINR['rnti'] == ue)]['sinr'].std() ])
            metric.append([ SINR[(SINR.index >= float(x)) & (SINR.index <= float(y)) ]['sinr'].mean(),
                                SINR[(SINR.index >= float(x)) & (SINR.index <= float(y)) ]['sinr'].std() ])
                
            res.append(metric)
        result.append(res)
        fig.savefig(myhome + prefix +'SINR-' +key+'.png')
    plt.close()
    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return result

@info_n_time_decorator("CQI and BLER")
def graph_CQI_BLER(myhome):
    """Graph the CQI and BLER of the UEs"""
    DEBUG=1
    #tic = time.time()
    file = "RxPacketTrace.txt"
    title = "CQI"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    #print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        TMP = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        TMP = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)
    if DEBUG: print ("Loaded DataFrame", TMP)

    if DEBUG: print("direction: ", TMP['direction'].drop_duplicates().sort_values().unique())
    if DEBUG: print("rnti: ", TMP['rnti'].drop_duplicates().sort_values().unique())
    TMP.set_index('Time', inplace = True)
    TMP = TMP[TMP['rnti'] != 0]
    if DEBUG: print ("only rnti <> 0", TMP)
    TMP = TMP[TMP['direction'] =='DL']
    if DEBUG: print ("only DL", TMP)
    TMP['rnti'] = UENum - TMP['rnti'] +1
    if DEBUG: print ("Filtered", TMP)


    TMP.index = pd.to_datetime(TMP.index,unit = 's')
    if DEBUG: print (TMP)
    CQI = pd.DataFrame(TMP.groupby('rnti').resample(str(resamplePeriod)+'ms').CQI.mean().round().ffill())
    CQI = CQI.reset_index(level = 0)
    if DEBUG: print (CQI)

    CQI.index = CQI.index.astype(int) / 1e9
    if DEBUG: print (CQI)

    #print('plotting', end = ".", flush = True)
    fig, ax = plt.subplots()
    CQI.groupby('rnti')['CQI'].plot()
    plt.ylim([0, 16])
    plt.xlim([0, simTime])
    ax.set_ylabel("CQI")
    ax.set_xlabel("Time(s)")

    if show_title:
        plt.suptitle(title, y = 0.99, fontsize = title_font_size)
        plt.title(subtitle, fontsize = subtitle_font_size)
    if show_legend:
        text_legend = CQI['rnti'].drop_duplicates().sort_values().unique()
        text_legend = [ 'UE ' + str(i)  for i in text_legend]
        ax.legend(text_legend, ncol = max([1, len(CQI['rnti'].unique()) // 3 ]))

    fig.savefig(myhome + prefix +'CQI' +'.png')
    plt.close()
    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    # Metrics
    result_cqi = []
    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        if DEBUG: print(x,y)
        metric = []
        if DEBUG: print(CQI['rnti'].drop_duplicates().sort_values().unique())
        for ue in CQI['rnti'].drop_duplicates().sort_values().unique():
            metric.append([ CQI[(CQI.index >= float(x)) & (CQI.index <= float(y)) & (CQI['rnti'] == ue)]['CQI'].mean(),
                            CQI[(CQI.index >= float(x)) & (CQI.index <= float(y)) & (CQI['rnti'] == ue)]['CQI'].std() ])
        metric.append([ CQI[(CQI.index >= float(x)) & (CQI.index <= float(y)) ]['CQI'].mean(),
                            CQI[(CQI.index >= float(x)) & (CQI.index <= float(y)) ]['CQI'].std() ])
            
        result_cqi.append(metric)
    if DEBUG: print("result_cqi:", result_cqi)

    # BLER
    title = "BLER"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    CQI = TMP
    CQI['Time'] = CQI.index
    CQI.index = pd.to_datetime(CQI['Time'],unit = 's')
    BLER = pd.DataFrame(TMP.groupby('rnti').resample(str(resamplePeriod)+'ms').TBler.mean().replace(np.nan, 0))
    BLER = BLER.reset_index(level = 0)
    if DEBUG: print (BLER)
    BLER = BLER[~BLER['TBler'].isna()]

    BLER['Time'] = BLER.index
    BLER['Time'] = BLER['Time'].astype(int) / 1e9
    BLER = BLER.set_index('Time')
    # BLER['rnti'] = UENum - BLER['rnti'] +1

    #print('plotting', end = ".", flush = True)
    fig, ax = plt.subplots()

    for ue in BLER['rnti'].drop_duplicates().sort_values().unique():
        #print(ue, end = ".", flush = True)
        plt.semilogy(BLER[BLER['rnti'] == ue].index.values, BLER[BLER['rnti'] == ue]['TBler'].values, label='UE ' + str(ue))

    plt.xlabel("Time(s)")
    plt.ylabel("BLER")
    plt.grid(True, which="both", ls="-")
    plt.yticks(fontsize = 10)

    ax.set_ylim([10e-30, 1])
    
    if show_title:
        plt.suptitle(title, y = 0.99, fontsize = title_font_size)
        plt.title(subtitle, fontsize = subtitle_font_size)
    if show_legend:
        plt.legend(ncol = max([1, len(BLER['rnti'].unique()) // 3 ]))

    fig.savefig(myhome + prefix +'BLER' +'.png')
    plt.close()
    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))

    # Metrics

    result_bler = []
    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        metric = []
        for ue in BLER['rnti'].drop_duplicates().sort_values().unique():
            metric.append([ BLER[(BLER.index >= float(x)) & (BLER.index <= float(y)) & (BLER['rnti'] == ue)]['TBler'].mean(),
                            BLER[(BLER.index >= float(x)) & (BLER.index <= float(y)) & (BLER['rnti'] == ue)]['TBler'].std() ])
        metric.append([ BLER[(BLER.index >= float(x)) & (BLER.index <= float(y)) ]['TBler'].mean(),
                            BLER[(BLER.index >= float(x)) & (BLER.index <= float(y)) ]['TBler'].std() ])
            
        result_bler.append(metric)
    if DEBUG: print("result_bler:", result_bler)
        
    results = [result_cqi, result_bler]
    if DEBUG: print(results)
    return results

@info_n_time_decorator("Path Loss")
def graph_path_loss(myhome):
    """Graph the path loss of the UEs"""
    DEBUG=0
    #tic = time.time()
    file = "DlPathlossTrace.txt"
    title = "Path Loss"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    #print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        PLOSS = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        PLOSS = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)

    PLOSS = PLOSS.rename(columns={'IMSI': 'rnti', 'Time(sec)': 'Time', 'pathLoss(dB)': 'pathLoss'})

    PLOSS.set_index('Time', inplace = True)
    PLOSS = PLOSS.loc[PLOSS['rnti'] != 0]
    PLOSS = PLOSS[PLOSS['pathLoss'] < 0]

    PLOSS.index = pd.to_datetime(PLOSS.index,unit = 's')
    if DEBUG: print (PLOSS)
    PLOSS = pd.DataFrame(PLOSS.groupby('rnti').resample(str(resamplePeriod)+'ms').pathLoss.mean().ffill())
    PLOSS = PLOSS.reset_index(level = 0)
    if DEBUG: print (PLOSS)

    PLOSS.index = PLOSS.index.astype(int) / 1e9
    if DEBUG: print (PLOSS)


    # PLOT
    fig, ax = plt.subplots()
    PLOSS.groupby(['rnti'])['pathLoss'].plot()
    ax.set_xlabel("Time [s]")
    ax.set_ylabel("pathLoss [dB]")

    plt.yticks(fontsize = 10)
    if show_title:
        plt.suptitle(title, y = 0.99, fontsize = title_font_size)
        plt.title(subtitle, fontsize = subtitle_font_size)
    if show_legend:
        text_legend = PLOSS['rnti'].drop_duplicates().sort_values().unique()
        text_legend = [ 'UE ' + str(i)  for i in text_legend]
        ax.legend(text_legend, ncol = max([1, len(PLOSS['rnti'].unique()) // 3 ]), loc='lower center')

    fig.savefig(myhome + prefix +'PATH_LOSS' +'.png')
    plt.close()
    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    results = []
    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        metric = []
        for ue in PLOSS['rnti'].drop_duplicates().sort_values().unique():
            metric.append([ PLOSS[(PLOSS.index >= float(x)) & (PLOSS.index <= float(y)) & (PLOSS['rnti'] == ue)]['pathLoss'].mean(),
                            PLOSS[(PLOSS.index >= float(x)) & (PLOSS.index <= float(y)) & (PLOSS['rnti'] == ue)]['pathLoss'].std() ])
        metric.append([ PLOSS[(PLOSS.index >= float(x)) & (PLOSS.index <= float(y)) ]['pathLoss'].mean(),
                            PLOSS[(PLOSS.index >= float(x)) & (PLOSS.index <= float(y)) ]['pathLoss'].std() ])
            
        results.append(metric)

    return results

@info_n_time_decorator(f"{tcpTypeIds[0]} Throughput TX")
def graph_thr_tx(myhome):
    """Graph the throughput transmmited"""
    #tic = time.time()

    title = tcpTypeIds[0] + " " + "Throughput TX"
    file = "NrDlPdcpTxStats.txt"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    #print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        thrtx = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        thrtx = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)

    thrtx = thrtx.groupby(['time(s)','rnti'])['packetSize'].sum().reset_index()
    thrtx.index = pd.to_datetime(thrtx['time(s)'],unit='s')

    thrtx = pd.DataFrame(thrtx.groupby('rnti').resample(str(resamplePeriod)+'ms').packetSize.sum().replace(np.nan, 0))
    thrtx = thrtx.reset_index(level = 0)

    thrtx['InsertedDate'] = thrtx.index
    thrtx['deltaTime'] = thrtx['InsertedDate'].astype(int) / 1e9
    thrtx['Time'] = thrtx['deltaTime']
    thrtx['deltaTime'] = thrtx.groupby('rnti').diff()['deltaTime']

    thrtx.loc[~thrtx['deltaTime'].notnull(),'deltaTime'] = thrtx.loc[~thrtx['deltaTime'].notnull(),'InsertedDate'].astype(int) / 1e9
    thrtx['throughput'] = thrtx['packetSize']*8 / thrtx['deltaTime'] / 1e6
    thrtx = thrtx.set_index('Time')
    thrtx['rnti'] = UENum - thrtx['rnti'] +1

    #print('plotting', end = ".", flush = True)
    fig, ax = plt.subplots()
    thrtx.groupby(['rnti'])['throughput'].plot()

    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Throughput [Mb/s]")
    ax.set_ylim([0 , max([thr_limit ,thrtx['throughput'].max()*1.1])])
    ax.set_xlim([0 , simTime])

    if show_title:
        plt.suptitle(title, y = 0.99, fontsize = title_font_size)
        plt.title(subtitle, fontsize = subtitle_font_size)
    if show_legend:
        text_legend = thrtx['rnti'].drop_duplicates().sort_values().unique()
        text_legend = [ 'UE ' + str(i)  for i in text_legend]
        ax.legend(text_legend, ncol = max([1, len(thrtx['rnti'].unique()) // 3 ]))


    fig.savefig(myhome + prefix + 'ThrTx' + '.png')
    plt.close()
    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return True

@info_n_time_decorator("Throughput vs RLC Sub Buffer")
def graph_thr_rlcsubbuffer():
    """Graph the throughput and RLC subbuffer"""
    #tic = time.time()
    title = "Throughput vs RLC Sub Buffer"
    file = "NrDlPdcpRxStats.txt"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    #print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        thrrx = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        thrrx = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)

    thrrx = thrrx.groupby(['time(s)','rnti'])['packetSize'].sum().reset_index()
    thrrx.index = pd.to_datetime(thrrx['time(s)'],unit='s')

    thrrx = pd.DataFrame(thrrx.groupby('rnti').resample(str(resamplePeriod)+'ms').packetSize.sum().replace(np.nan, 0))
    thrrx = thrrx.reset_index(level = 0)

    thrrx['InsertedDate'] = thrrx.index
    thrrx['deltaTime'] = thrrx['InsertedDate'].astype(int) / 1e9
    thrrx['Time'] = thrrx['deltaTime']

    thrrx['deltaTime'] = thrrx.groupby('rnti').diff()['deltaTime']

    thrrx.loc[~thrrx['deltaTime'].notnull(),'deltaTime'] = thrrx.loc[~thrrx['deltaTime'].notnull(),'InsertedDate'].astype(int) / 1e9
    thrrx['throughput'] =  thrrx['packetSize']*8 / thrrx['deltaTime'] / 1e6
    thrrx['throughput'] = thrrx['throughput'].astype(float)
    thrrx['rnti'] = UENum - thrrx['rnti'] +1

    thrrx = thrrx.set_index('Time')

    #print('load', end = "", flush = True)
    file = "RlcSubBufferStat.txt"
    if os.path.exists(myhome + file):
        rlc = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        rlc = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)

    rlc['Time'] = pd.to_datetime(rlc['Time'],unit='s')
    rlc = rlc.set_index('Time')
    
    # Filter gNb RLCbuffer
    rlc = rlc[rlc['ID'].str[:len(ip_net_UE)] == ip_net_UE]
    
    # Normalize by rlcBuffer
    rlc['NumOfBytes'] = rlc['NumOfBytes'] / rlcBuffer

    #print('Resampling', end = ".", flush = True)
    # Resampling Total Buffer
    rlcB = pd.DataFrame(rlc.resample(str(resamplePeriod)+'ms').NumOfBytes.max())
    # rlcB = rlcB.reset_index(level = 0).sort_index(axis = 0)
    rlcB['Time'] = rlcB.index.astype(int) / 1e9
    rlcB = rlcB.set_index('Time')

    # Resampling Subbuffer
    rlcSB = pd.DataFrame(rlc.groupby('ID').resample(str(resamplePeriod)+'ms').NumOfBytes.max().replace(np.nan, 0))
    rlcSB = rlcSB.reset_index(level = 0).sort_index(axis = 0)
    rlcSB['Time'] = rlcSB.index.astype(int) / 1e9
    rlcSB = rlcSB.set_index('Time')

    # print (rlcSB)
    
    # Convert ID to UE
    rlcSB['rnti'] = rlcSB['ID'].str.split(".").str[3].astype(int) - 1
    # print (rlcSB)

    #print('plotting', end = ".", flush = True)
    thrrx['t'] = thrrx.index
    rlcB['t'] = rlcB.index
    rlcSB['t'] = rlcSB.index





    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        #print(z, end = ".", flush = True)

        fig, ax = plt.subplots()
        # Total RLC Buffer per UE
        # print(rlcB[(rlcB['t'] >= float(x)) & (rlcB['t'] <= float(y))])
        # ax3 = rlcB[(rlcB['t'] >= float(x)) & (rlcB['t'] <= float(y))]['NumOfBytes'].plot.area( secondary_y=True,
        #                                                                                     ax = ax,  
        #                                                                                     alpha = 0.1,
        #                                                                                     color = "black",
        #                                                                                     legend = False)
        for ue in thrrx['rnti'].drop_duplicates().sort_values().unique():
            # Throughput per UE
            ax1 = thrrx[(thrrx['t'] >= float(x)) & (thrrx['t'] <= float(y)) & (thrrx['rnti'] == ue)]['throughput'].plot( ax = ax,  
                                                                                            color = colors[(ue-1) % len(colors)], 
                                                                                            legend = False)
            # SubBuffer per UE
            # print(rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue)])
            ax2 = rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue)]['NumOfBytes'].plot( secondary_y=True,
                                                                                            ax = ax,  
                                                                                            alpha = 0.5, 
                                                                                            color = colors[(ue-1) % len(colors)], 
                                                                                            linestyle = 'dashed',
                                                                                            legend = False)

        # # SubBuffer per UE
        # for ue in rlcSB['rnti'].drop_duplicates().sort_values().unique():
        #     ax2 = rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue)]['NumOfBytes'].plot.area( secondary_y=True,
        #                                                                                     ax=ax,  
        #                                                                                     alpha = 0.2, 
        #                                                                                     color = colors[(ue-1) % len(colors)], 
        #                                                                                     legend=False)



        ax.set_xlabel("Time [s]")
        ax.set_ylabel("Throughput [Mb/s]")
        # ax.set_ylim([0 , dataRate * 1.2 * UENum])
        ax.set_ylim([0 , thr_limit ])
        
        ax.set_xlim([float(x) , float(y)])

        ax2.set_ylabel("RLC Buffer [%]", loc = 'bottom')
        ax2.set_ylim(0,4)
        ax2.set_yticks([0,0.5,1])
        ax2.set_yticklabels(['0','50','100' ], fontsize = 12)

        # ax3.set_ylim(0,4)


        if show_title:
            plt.suptitle('Throughput vs RLC Buffer and Sub-Buffer', y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        if show_legend:
            text_legend = thrrx['rnti'].drop_duplicates().sort_values().unique()
            text_legend1 = [ 'UE ' + str(i)  for i in text_legend]
            text_legend2 = [ 'SubBuffer UE ' + str(i)  for i in text_legend]
            # text_legend = np.array(text_legend1 + text_legend2)
            # text_legend = text_legend.reshape(2,int(len(text_legend)/2)).reshape([len(text_legend)], order='F').tolist()
            # ax.legend(text_legend1+ text_legend2, loc = 'upper left', ncol = max([1, len(text_legend) // 3 ]))
            ax1.legend(text_legend1, loc = 'upper left', ncol = max([1, len(text_legend1) // 3 ]))
            # ax2.legend(text_legend2, loc = 'upper right', ncol = max([1, len(text_legend2) // 3 ]))
            

        plotfile = myhome + prefix + 'ThrSubBuffer' +'-' + z + '.png'
        fig.savefig(plotfile)
        plt.close()

    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return True

@info_n_time_decorator("Throughput vs RLC Buffer (2)", debug=True)
def graph_thr_rlcbuffer2(myhome):
    """Graph the throughput and RLC buffer"""
    DEBUG=0
    tic = time.time()
    title = "Throughput vs RLC Buffer"
    file = "NrDlPdcpRxStats.txt"
    if DEBUG: print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    if DEBUG: print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        thrrx = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        thrrx = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    if DEBUG: print('ed', end = ".", flush = True)

    thrrx = thrrx.groupby(['time(s)','rnti'])['packetSize'].sum().reset_index()
    thrrx.index = pd.to_datetime(thrrx['time(s)'],unit='s')

    thrrx = pd.DataFrame(thrrx.groupby('rnti').resample(str(resamplePeriod)+'ms').packetSize.sum().replace(np.nan, 0))
    thrrx = thrrx.reset_index(level = 0)

    thrrx['InsertedDate'] = thrrx.index
    thrrx['deltaTime'] = thrrx['InsertedDate'].astype(int) / 1e9
    thrrx['Time'] = thrrx['deltaTime']

    thrrx['deltaTime'] = thrrx.groupby('rnti').diff()['deltaTime']

    thrrx.loc[~thrrx['deltaTime'].notnull(),'deltaTime'] = thrrx.loc[~thrrx['deltaTime'].notnull(),'InsertedDate'].astype(int) / 1e9
    thrrx['throughput'] =  thrrx['packetSize']*8 / thrrx['deltaTime'] / 1e6
    thrrx['throughput'] = thrrx['throughput'].astype(float)
    thrrx['rnti'] = UENum - thrrx['rnti'] +1

    thrrx = thrrx.set_index('Time')

    if DEBUG: print('load', end = "", flush = True)
    file = "RlcBufferStat.txt"
    if os.path.exists(myhome + file):
        rlc = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        rlc = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    if DEBUG: print('ed', flush = True)

    # Filter gNb RLCbuffer
    rlc = rlc[rlc['dstIP'].str[:len(ip_net_UE)] == ip_net_UE]
    rlc.index = pd.to_datetime(rlc['Time'],unit='s')

    # Resampling Subbuffer
    rlcSB = pd.DataFrame(rlc.groupby('dstIP').resample(str(resamplePeriod)+'ms').agg({'dropSize': "sum", 'txBufferSize': "max"}).replace(np.nan, 0))
    rlcSB = rlcSB.reset_index(level = 0).sort_index(axis = 0)
    rlcSB['Time'] = rlcSB.index.astype(int) / 1e9
    rlcSB = rlcSB.set_index('Time')
    rlcSB['state'] = 0
    rlcSB.loc[rlcSB['dropSize'] > 0,'state'] = 1
    # Normalize
    if DEBUG: print('Normalize: ', rlcBuffer)
    rlcSB['txBufferSize'] = rlcSB['txBufferSize'] / rlcBuffer

    # Convert ID to UE
    rlcSB['rnti'] = rlcSB['dstIP'].str.split(".").str[3].astype(int) - 1

    if DEBUG: print(rlcSB)

    

    if DEBUG: print('plotting', flush = True)
    thrrx['t'] = thrrx.index
    rlcSB['t'] = rlcSB.index
    
    rlc_result = []



    if DEBUG: print (rlcSB)
    if DEBUG: print ('zones: ', parts)

    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        if DEBUG: print(z, flush = True)

        # Throughput per UE
        if DEBUG: print ('Throughput per UE')
        fig, ax = plt.subplots()
        for ue in thrrx['rnti'].drop_duplicates().sort_values().unique():
            ax1 = thrrx[(thrrx['t'] >= float(x)) & (thrrx['t'] <= float(y)) & (thrrx['rnti'] == ue)]['throughput'].plot( ax = ax,  
                                                                                            color = colors[(ue-1) % len(colors)], 
                                                                                            legend = False)
                        
            ax2 = rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue) ]['txBufferSize'].plot.area( secondary_y=True,
                                                                                                ax=ax,  
                                                                                                color = colors[(ue-1) % len(colors)], 
                                                                                                alpha = 0.2,
                                                                                                linestyle = linestyles[(ue-1) % len(linestyles)],
                                                                                                legend=False)
            ax3 = rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue) ]['txBufferSize'].plot( secondary_y=True,
                                                                                                ax=ax,  
                                                                                                color = colors[(ue-1) % len(colors)], 
                                                                                                linestyle = linestyles[(ue-1) % len(linestyles)],
                                                                                                legend=False)
                
            
        ax2.set_ylabel("RLC Buffer [%]", loc = 'bottom')
        ax2.set_ylim(0,4)
        ax2.set_yticks([0,0.5,1])
        ax2.set_yticklabels(['0','50','100' ], fontsize = 12)
        

        ax3.set_ylim(0, 4)

        ax.set_xlabel("Time [s]")
        ax.set_ylabel("Throughput [Mb/s]")
        ax.set_ylim([0 , max([thr_limit, thrrx[(thrrx['t'] >= float(x)) & (thrrx['t'] <= float(y))]['throughput'].max()*1.05])])
        ax.set_xlim([float(x) , float(y)])

        if show_title:
            plt.suptitle(title, y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)
        if show_legend:
            text_legend = thrrx['rnti'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, loc = 'upper left', ncol = max([1, len(thrrx['rnti'].unique()) // 3 ]))

        plotfile = myhome + prefix + 'ThrRx' +'-' + z + '.png'
        fig.savefig(plotfile)
        plt.close()

        # Aggregated Throughput
        if DEBUG: print ('Aggregated Throughput')
        fig, ax = plt.subplots()
        ax1 = thrrx[(thrrx['t'] >= float(x)) & (thrrx['t'] <= float(y))].groupby(['t'])['throughput'].sum().plot( ax=ax)

        if DEBUG: print ('points')
        if p<parts.shape[0]-1:
            for d in range(points.shape[0]):
                if (points[d] >= float(x)) & (points[d] <= float(y)):
                    mytext = "("+ str(points[d])+","+ str(round(thrrx.loc[points[d]]['throughput'],2)) +")"
                    ax.annotate( mytext, (points[d] , thrrx.loc[points[d]]['throughput']))

        if DEBUG: print ('buffer')
        if DEBUG: print ('for in ', thrrx['rnti'].drop_duplicates().sort_values().unique())
        for ue in thrrx['rnti'].drop_duplicates().sort_values().unique():
            if DEBUG: print ('ue: ', ue)
            if len(rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue) & (rlcSB['state'] >= 0 )]['txBufferSize'].index) > 0:
                ax2 = rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue) & (rlcSB['state'] >= 0 )]['txBufferSize'].plot.area( secondary_y=True,
                                                                                                ax=ax,  
                                                                                                color = colors[(ue-1) % len(colors)], 
                                                                                                alpha = 0.2,
                                                                                                linestyle = linestyles[(ue-1) % len(linestyles)],
                                                                                                legend=False)
            
            if len(rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue) & (rlcSB['state'] > 0 )]['txBufferSize'].index) > 0:
                ax3 = rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue) & (rlcSB['state'] > 0 )]['txBufferSize'].plot( secondary_y=True,
                                                                                                ax=ax,  
                                                                                                color = colors[(ue-1) % len(colors)], 
                                                                                                linestyle = linestyles[(ue-1) % len(linestyles)],
                                                                                                legend=False)
        if DEBUG: print ('labels and legend')
        ax2.set_ylabel("RLC Buffer [%]", loc = 'bottom')
        ax2.set_ylim(0,4)
        ax2.set_yticks([0,0.5,1])
        ax2.set_yticklabels(['0','50','100' ], fontsize = 12)
        
        ax3.set_ylim(0, 4)

        ax.set_xlabel("Time [s]")
        ax.set_ylabel("Throughput [Mb/s]")
        ax.set_ylim([0 , thr_limit * UENum])
        ax.set_xlim([float(x) , float(y)])

        if show_title:
            plt.suptitle('Agg. Throughput vs RLC Buffer', y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        plotfile = myhome + prefix + 'ThrAgg' +'-' + z + '.png'
        fig.savefig(plotfile)
        plt.close()

        # Summary
        if DEBUG: print ('Metrics Summary')
        rlc_part = []
        for ue in rlcSB['rnti'].drop_duplicates().sort_values().unique():
            rlc_part.append([ rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue)]['txBufferSize'].mean(),
                              rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) & (rlcSB['rnti'] == ue)]['txBufferSize'].std() ])
        rlc_part.append([ rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) ]['txBufferSize'].mean(),
                            rlcSB[(rlcSB['t'] >= float(x)) & (rlcSB['t'] <= float(y)) ]['txBufferSize'].std() ])
            
        rlc_result.append(rlc_part) 

        toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return rlc_result

@info_n_time_decorator("Throughput vs RLC Buffer")
def graph_thr_rlcbuffer(myhome):
    """Graph the throughput and RLC buffer"""

    #tic = time.time()
    title = "Throughput vs RLC Buffer"
    file = "NrDlPdcpRxStats.txt"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    #print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        thrrx = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        thrrx = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)

    thrrx = thrrx.groupby(['time(s)','rnti'])['packetSize'].sum().reset_index()
    thrrx.index = pd.to_datetime(thrrx['time(s)'],unit='s')

    thrrx = pd.DataFrame(thrrx.groupby('rnti').resample(str(resamplePeriod)+'ms').packetSize.sum().replace(np.nan, 0))
    thrrx = thrrx.reset_index(level = 0)

    thrrx['InsertedDate'] = thrrx.index
    thrrx['deltaTime'] = thrrx['InsertedDate'].astype(int) / 1e9
    thrrx['Time'] = thrrx['deltaTime']

    thrrx['deltaTime'] = thrrx.groupby('rnti').diff()['deltaTime']

    thrrx.loc[~thrrx['deltaTime'].notnull(),'deltaTime'] = thrrx.loc[~thrrx['deltaTime'].notnull(),'InsertedDate'].astype(int) / 1e9
    thrrx['throughput'] =  thrrx['packetSize']*8 / thrrx['deltaTime'] / 1e6
    thrrx['throughput'] = thrrx['throughput'].astype(float)
    thrrx['rnti'] = UENum - thrrx['rnti'] +1

    thrrx = thrrx.set_index('Time')

    #print('load', end = "", flush = True)
    file = "RlcBufferStat.txt"
    if os.path.exists(myhome + file):
        rlcdrop = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        rlcdrop = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)

    rlcdrop['direction'] = 'UL'
    
    # rlcdrop.loc[rlcdrop['PacketSize'] > 1500,'direction'] = 'DL'
    rlcdrop.loc[rlcdrop['srcIP'].str[:len(ip_net_Server)] == ip_net_Server,'direction'] = 'DL'

    rlc = rlcdrop[rlcdrop['direction'] == 'DL']
    rlc.index = pd.to_datetime(rlc['Time'],unit='s')
    #print(".", end = "", flush = True)
    rlcdrop = pd.DataFrame(rlc.resample(str(resamplePeriod)+'ms').dropSize.sum().replace(np.nan, 0))
    rlcdrop['Time'] = rlcdrop.index.astype(int) / 1e9
    rlcdrop = rlcdrop.set_index('Time')
    rlcdrop['state'] = 0
    rlcdrop.loc[rlcdrop['dropSize'] > 0,'state'] = 1

    rlcB = pd.DataFrame(rlc.resample(str(resamplePeriod)+'ms').txBufferSize.max().replace(np.nan, 0))
    rlcB['Time'] = rlcB.index.astype(int) / 1e9
    rlcB = rlcB.set_index('Time')
    # Normalize
    rlcB['txBufferSize'] = rlcB['txBufferSize'] / rlcBuffer

    # print("RlcBuffer Size: " + str(rlcBuffer) )

    rlcdrop['state'] = rlcdrop['state']*rlcB['txBufferSize']
    rlcB.loc[rlcdrop['state'] > 0,'txBufferSize'] = 0

    #print('plotting', end = ".", flush = True)
    thrrx['t'] = thrrx.index
    rlcdrop['t'] = rlcdrop.index
    rlcB['t'] = rlcB.index
    rlc_result = []



    # print (rlcB)

    for p in range(parts.shape[0] ):
        [x, y, z] = parts[p,:]
        #print(z, end = ".", flush = True)

        # Throughput per UE
        fig, ax = plt.subplots()
        ax1 = thrrx[(thrrx['t'] >= float(x)) & (thrrx['t'] <= float(y))].groupby(['rnti'])['throughput'].plot( ax=ax)

        if p<parts.shape[0]-1:
            for d in range(points.shape[0]):
                if (points[d] >= float(x)) & (points[d] <= float(y)):
                    mytext = "("+ str(points[d])+","+ str(round(thrrx.loc[points[d]]['throughput'],2)) +")"
                    ax.annotate( mytext, (points[d] , thrrx.loc[points[d]]['throughput']))

        ax2 = rlcdrop[(rlcdrop['t'] >= float(x)) & (rlcdrop['t'] <= float(y))]['state'].plot.area( secondary_y=True,
                                                                                        ax=ax,
                                                                                        alpha = 0.2,
                                                                                        color="red",
                                                                                        legend=False)
        ax2.set_ylabel("RLC Buffer [%]", loc = 'bottom')
        ax2.set_ylim(0,4)
        ax2.set_yticks([0,0.5,1])
        ax2.set_yticklabels(['0','50','100' ], fontsize = 12)
        
        ax3 = rlcB[(rlcB['t'] >= float(x)) & (rlcB['t'] <= float(y))]['txBufferSize'].plot.area( secondary_y=True,
                                                                                            ax=ax,  
                                                                                            alpha = 0.2, 
                                                                                            color="green", 
                                                                                            legend=False)
        rlc_part = [ rlcB[(rlcB['t'] >= float(x)) & (rlcB['t'] <= float(y))]['txBufferSize'].mean(), rlcB[(rlcB['t'] >= float(x)) & (rlcB['t'] <= float(y))]['txBufferSize'].std()]
        rlc_result.append(rlc_part) 
        ax3.set_ylim(0, 4)

        ax.set_xlabel("Time [s]")
        ax.set_ylabel("Throughput [Mb/s]")
        ax.set_ylim([0 , max([thr_limit, thrrx[(thrrx['t'] >= float(x)) & (thrrx['t'] <= float(y))]['throughput'].max()*1.05])])
        ax.set_xlim([float(x) , float(y)])

        if show_title:
            plt.suptitle(title, y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)
        if show_legend:
            text_legend = thrrx['rnti'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, loc = 'upper left', ncol = max([1, len(thrrx['rnti'].unique()) // 3 ]))

        plotfile = myhome + prefix + 'ThrRx' +'-' + z + '.png'
        fig.savefig(plotfile)
        plt.close()

        # Aggregated Throughput
        fig, ax = plt.subplots()
        ax1 = thrrx[(thrrx['t'] >= float(x)) & (thrrx['t'] <= float(y))].groupby(['t'])['throughput'].sum().plot( ax=ax)

        if p<parts.shape[0]-1:
            for d in range(points.shape[0]):
                if (points[d] >= float(x)) & (points[d] <= float(y)):
                    mytext = "("+ str(points[d])+","+ str(round(thrrx.loc[points[d]]['throughput'],2)) +")"
                    ax.annotate( mytext, (points[d] , thrrx.loc[points[d]]['throughput']))

        ax2 = rlcdrop[(rlcdrop['t'] >= float(x)) & (rlcdrop['t'] <= float(y))]['state'].plot.area( secondary_y=True,
                                                                                        ax=ax,
                                                                                        alpha = 0.2,
                                                                                        color="red",
                                                                                        legend=False)
        ax2.set_ylabel("RLC Buffer [%]", loc = 'bottom')
        ax2.set_ylim(0,4)
        ax2.set_yticks([0,0.5,1])
        ax2.set_yticklabels(['0','50','100' ], fontsize = 12)
        
        ax3 = rlcB[(rlcB['t'] >= float(x)) & (rlcB['t'] <= float(y))]['txBufferSize'].plot.area( secondary_y=True,
                                                                                            ax=ax,  
                                                                                            alpha = 0.2, 
                                                                                            color="green", 
                                                                                            legend=False)
        ax3.set_ylim(0, 4)

        ax.set_xlabel("Time [s]")
        ax.set_ylabel("Throughput [Mb/s]")
        ax.set_ylim([0 , thr_limit * UENum])
        ax.set_xlim([float(x) , float(y)])

        if show_title:
            plt.suptitle('Agg. Throughput vs RLC Buffer', y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        plotfile = myhome + prefix + 'ThrAgg' +'-' + z + '.png'
        fig.savefig(plotfile)
        plt.close()
        #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return rlc_result

@info_n_time_decorator("Throughput vs PER")
def graph_thr_packetdrop(myhome):
    """Graph the throughput and packets drop"""

    #tic = time.time()
    title = "Throughput vs PER"
    file = "tcp-all-per.txt"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    #print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        tx_ = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
    else:
        tx_ = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)
    tx_.index = pd.to_datetime(tx_['Time'],unit='s')
    tx_thr = pd.DataFrame(tx_.resample(str(resamplePeriod)+'ms').BytesTx.sum().replace(np.nan, 0))
    tx_drp = pd.DataFrame(tx_.resample(str(resamplePeriod)+'ms').BytesDroped.sum().replace(np.nan, 0))

    tx_drp['PacketsTx'] = pd.DataFrame(tx_.resample(str(resamplePeriod)+'ms').PacketsTx.sum().replace(np.nan, 0))
    tx_drp['PacketsDroped'] = pd.DataFrame(tx_.resample(str(resamplePeriod)+'ms').PacketsDroped.sum().replace(np.nan, 0))

    tx_thr = tx_thr.reset_index(level = 0)
    tx_thr['Throughput'] =  tx_thr['BytesTx']*8 / (resamplePeriod/1000) / 1e6
    tx_thr['Time'] = tx_thr['Time'].astype(int) / 1e9
    tx_thr = tx_thr.set_index('Time')

    tx_drp = tx_drp.reset_index(level = 0)
    tx_drp['Throughput'] =  tx_drp['BytesDroped']*8 / (resamplePeriod/1000)  / 1e6
    tx_drp['PER'] = tx_drp['PacketsDroped'] / tx_drp['PacketsTx']
    tx_drp['Time'] = tx_drp['Time'].astype(int) / 1e9
    tx_drp = tx_drp.set_index('Time')
    drp_result = []
    for p in range(parts.shape[0] ):
        
        [x, y, z] = parts[p,:]
        fig, ax = plt.subplots()
        if p<parts.shape[0]-1:
            # plt.plot(tx_thr.loc[x:y].index, tx_thr['Throughput'].loc[x:y], '-o', markevery=tx_thr.loc[x:y].index.get_indexer(points, method='nearest'))
            ax1 = tx_thr['Throughput'].loc[x:y].plot(ax=ax, markevery=tx_thr.loc[x:y].index.get_indexer(points, method='nearest'))
            for d in range(points.shape[0]):
                if (points[d] >= float(x)) & (points[d] <= float(y)):
                    mytext = "("+ str(points[d])+","+ str(round(tx_thr.loc[points[d]]['Throughput'],2)) +")"
                    ax.annotate( mytext, (points[d] , tx_thr.loc[points[d]]['Throughput']))
        else:
            ax1 = tx_thr['Throughput'].loc[x:y].plot(ax=ax)
        ax2 = tx_drp['PER'].loc[x:y].plot.area(  secondary_y=True, ax=ax,  alpha = 0.2, color="red")
        ax3 = tx_drp['Throughput'].plot(  ax=ax,  color="red")

        drp_part = [ tx_drp['Throughput'].loc[x:y].mean(),
                    tx_drp['Throughput'].loc[x:y].std()]
        drp_result.append(drp_part)

        ax.set_xlabel("Time [s]")
        ax.set_ylabel("Throughput [Mb/s]")
        ax.set_ylim([0 , max([thr_limit * UENum, tx_thr['Throughput'].loc[x:y].max()*1.1])])

        ax2.set_ylabel("PER [%]", loc = 'bottom')
        ax2.set_ylim(0,4)

        ax2.set_yticks([0,0.5,1])
        ax2.set_yticklabels(['0','50','100' ], fontsize = 12)

        ax.set_xlim([float(x) , float(y)])

        if show_title:
            plt.suptitle(title, y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)
        fig.savefig(myhome + prefix + 'ThrDrp' +'-' + z+ '.png')
        plt.close()
    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return drp_result

@info_n_time_decorator("RTT", debug=True)
def graph_rtt(myhome):
    """Graph the total RTT"""
    DEBUG=0
    #tic = time.time()
    title = "RTT"
    file = "tcp-all-delay.txt"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
    #print('load', end = "", flush = True)
    if os.path.exists(myhome + file):
        ret = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip'  )
    else:
         ret = pd.read_csv(myhome + file + ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end = ".", flush = True)

    ret = ret.groupby(['Time','UE'])['rtt'].mean().reset_index()

    ret.index = pd.to_datetime(ret['Time'],unit='s')

    # ret = ret[(ret['Time'] >= AppStartTime) & (ret['Time'] <= simTime - AppStartTime)]

    ret = pd.DataFrame(ret.groupby(['UE']).resample(str(resamplePeriod)+'ms').rtt.mean().replace(np.nan, 0))

    ret = ret.reset_index(level = 0)
    ret['Time'] = ret.index.astype(int) / 1e9

    ret = ret.set_index('Time')
    ret['rtt'] = ret['rtt']*1000
    ret['t'] = ret.index
    ret = ret.astype({'UE':'int'})
    rtt = []

    for p in range(parts.shape[0] ):
        
        [x, y, z] = parts[p,:]


        # RTT per UE
        fig, ax = plt.subplots()
      
        if p<parts.shape[0]:
            if len(ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))]['UE'].unique()>1):
                ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))].groupby('UE')['rtt'].plot()
            else:
                ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))]['rtt'].plot()
            # plt.plot(ret.loc[x:y].index, ret['rtt'].loc[x:y], '-o',markevery = ret.loc[x:y].index.get_indexer(points, method='nearest'))
            
            for d in range(points.shape[0]):
                if (points[d] >= float(x)) & (points[d] <= float(y)):
                    try:
                        mytext = "("+ str(points[d])+","+ str(round(ret.loc[points[d]]['rtt'],2)) +")"
                        ax.annotate( mytext, (points[d] , ret.loc[points[d]]['rtt']))
                    except KeyError:
                        print(d)
 

            rtt_part = []
            for ue in ret['UE'].drop_duplicates().sort_values().unique():
                data = np.array(ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))  & (ret['UE'] == ue) ]['rtt'])
                if DEBUG: print(data)
                count, bins_count = np.histogram(data, bins=100) 
                pdf = count / sum(count) 
                cdf = np.cumsum(pdf)                
                rtt_part.append([data.mean(), 
                                data.std() ,
                                bins_count[1:],
                                cdf])
            data = np.array(ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))  ]['rtt'])
            count, bins_count = np.histogram(data, bins=100) 
            pdf = count / sum(count) 
            cdf = np.cumsum(pdf)                
            rtt_part.append([data.mean(), 
                            data.std() ,
                            bins_count[1:],
                            cdf])
        
            rtt.append(rtt_part)

        else:
            ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))].groupby('UE')['rtt'].plot()
            # ret['rtt'].loc[x:y].plot()

        ax.set_ylabel("RTT [ms]")
        ax.set_ylim(0, max([rtt_limit, ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))]['rtt'].mean()*2]))
        ax.set_xlim([float(x) , float(y)])

        if show_title:
            plt.suptitle(title, y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)
        if show_legend:
            text_legend = ret['UE'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, ncol = max([1, len(ret['UE'].unique()) // 3 ]))

        fig.savefig(myhome + prefix + 'RTT' +'-' + z+ '.png')
        plt.close()

        # RTT Total
        fig, ax = plt.subplots()
      
        if p<parts.shape[0]:
            if len(ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))]['UE'].unique()>1):
                ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))].groupby('t')['rtt'].mean().plot()
            else:
                ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))]['rtt'].plot()
            
            for d in range(points.shape[0]):
                if (points[d] >= float(x)) & (points[d] <= float(y)):
                    try:
                        mytext = "("+ str(points[d])+","+ str(round(ret.loc[points[d]]['rtt'],2)) +")"
                        ax.annotate( mytext, (points[d] , ret.loc[points[d]]['rtt']))
                    except KeyError:
                        print(f"Error with key: {d}")
        else:
            ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))].groupby('t')['t'].mean().plot()

        ax.set_ylabel("RTT [ms]")
        ax.set_ylim(0, max([rtt_limit, ret[(ret['t'] >= float(x)) & (ret['t'] <= float(y))]['rtt'].mean()*2]))
        ax.set_xlim([float(x) , float(y)])

        if show_title:
            plt.suptitle(title, y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        fig.savefig(myhome + prefix + 'RTTmean' +'-' + z+ '.png')
        plt.close()

        # RTT CDF
        fig, ax = plt.subplots()
        for ue in ret['UE'].drop_duplicates().sort_values().unique():
            bins = rtt[-1][ue][2]
            cdf  = rtt[-1][ue][3]
            
            plt.plot(bins, cdf)



        ax.set_ylim([0 , 1.1])
        ax.set_xlim([0 , rtt_limit])
        ax.set_ylabel("CDF")
        ax.set_xlabel("RTT [ms]")

        if show_title:
            plt.suptitle(title + ' CDF', y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        if show_legend:
            text_legend = ret['UE'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, ncol = max([1, len(ret['UE'].unique()) // 3 ]))





        fig.savefig(myhome + prefix + 'RTTcdf' +'-' + z+ '.png')
        plt.close()        
    toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return rtt

@info_n_time_decorator("Congestion Window & Inflight bytes")
def graph_tcp(myhome):
    """Graph the cwnd and inflight bytes."""
    DEBUG=0
    
    files={'Congestion Window':'tcp-cwnd-',
           'Inflight Bytes':'tcp-inflight-',}
    for key, value in files.items():
        #tic = time.time()
        title = key
        #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)
        #print('load', end = "", flush = True)

        DF = pd.DataFrame()
        UEN=0
        for i in range(len(flowTypes)):
            if DEBUG: print("i: "+ str(i) + ", flowtype: " +  flowTypes[i])
            if flowTypes[i] == 'TCP':
                for u in range(UENs[i]):
                    file = value + serverIDs[i] + "-" + str(u+1) + ".txt"
                    if os.path.exists(myhome + file):
                        TMP = pd.read_csv(myhome + file, sep = "\t", on_bad_lines = 'skip' )
                    else:
                        file =  file + ".gz"
                        TMP = pd.read_csv(myhome + file, compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
                    TMP['rnti'] = UEN + u +1
                    DF = pd.concat([DF,TMP], sort=False)
            UEN += UENs[i]
        if len(DF) == 0: break
        #print("ed", end = ".", flush = True)
        DF.index = pd.to_datetime(DF['Time'],unit='s')

        # DF = DF[(DF['Time'] >= AppStartTime) & (DF['Time'] <= simTime - AppStartTime)]
        DF = pd.DataFrame(DF.groupby('rnti').resample(str(resamplePeriod)+'ms' ).newval.mean().ffill())
        DF = DF.reset_index(level = 0)

        DF['Time'] = DF.index.astype(int) / 1e9

        DF = DF.set_index('Time')
        DF['newval'] = DF['newval'] / 1024
        DF['t'] = DF.index
        
        for p in range(parts.shape[0] ):
            [x, y, z] = parts[p,:]
            if (len(DF[(DF['t'] >= float(x)) & (DF['t'] <= float(y))].index)>0):
                fig, ax = plt.subplots()
                ax1 = DF[(DF['t'] >= float(x)) & (DF['t'] <= float(y))].groupby(['rnti'])['newval'].plot( ax = ax, )

                ax.set_ylabel(key+" [KBytes]")
                ax.set_xlim([float(x) , float(y)])
                if (value == 'tcp-cwnd-'):
                    # ax.set_ylim([0 , max([cwd_limit, DF['newval'].max()*1.1])])
                    ax.set_ylim([0 , cwd_limit])
                elif (value == 'tcp-inflight-'):
                    # ax.set_ylim([0 ,  max([inflight_limit, DF['newval'].max()*1.1])])
                    ax.set_ylim([0 ,  inflight_limit])

                plt.yticks(fontsize = 10)
                if show_title:
                    plt.suptitle(title, y = 0.99, fontsize = title_font_size)
                    plt.title(subtitle, fontsize = subtitle_font_size)
                if show_legend:
                    text_legend = DF['rnti'].drop_duplicates().sort_values().unique()
                    text_legend = [ 'UE ' + str(i)  for i in text_legend]
                    ax.legend(text_legend, ncol = max([1, len(DF['rnti'].unique()) // 3 ]))

                fig.savefig(myhome + prefix + value + z + '.png')

                plt.close()
        #toc = time.time()
        #print(f"\tProcessed in: %.2f" %(toc-tic))
    return True

@info_n_time_decorator("Metrics")
def calculate_metrics(myhome):
    """Calculates the metrics."""
    DEBUG=0
    conv_thresh=0.10
    #tic = time.time()
    title = "Metrics"
    file = "NrDlPdcpRxStats.txt"
    #print(TXT_CYAN + title + TXT_CLEAR, end = "...", flush = True)

    #print('load', end="", flush=True)
    if os.path.exists(myhome+file):
        tmp = pd.read_csv(myhome+file, sep = "\t", on_bad_lines = 'skip' )
    else:
        tmp = pd.read_csv(myhome+file+ ".gz", compression = 'gzip', sep = "\t", on_bad_lines = 'skip' )
    #print('ed', end=".", flush=True)

    tmp=tmp.groupby(['time(s)','rnti'])['packetSize'].sum().reset_index()
    tmp.index=pd.to_datetime(tmp['time(s)'],unit='s')
    tmp=pd.DataFrame(tmp.groupby('rnti').resample(str(resamplePeriod)+'ms').packetSize.sum().replace(np.nan, 0))
    tmp=tmp.reset_index(level=0)
    tmp['Time']=tmp.index.astype(int) / 1e9

    tmp['rnti'] = UENum - tmp['rnti'] +1

    # results file
    results = configparser.ConfigParser()

    # DEFAULT Section
    results['DEFAULT'] = {'UENum': UENum,
                            'Sec': parts.shape[0] - 1,
                            # 'Date': myhome[-13:-1]
                            }
    # Aggregated Throughput
    #print("agg-thr", end=".", flush=True)
    for p in range(parts.shape[0]):
        [x, y, z] = parts[p,:]
        # start and end of each section
        results['part-' + str((p+1) % parts.shape[0])]= {'x':x,
                                    'y':y}
        
        thrrx = pd.DataFrame( tmp.groupby('Time').packetSize.sum() )
        thrrx = thrrx.reset_index(level=0)
        if DEBUG: print(thrrx)
        thrrx = thrrx[(thrrx['Time'] >= float(x)) & (thrrx['Time'] <= float(y))]
        if DEBUG: print(thrrx)
        
        thrrx['throughput'] = thrrx['packetSize'] * 8 / (resamplePeriod / 1000)  / 1e6
        if DEBUG: print(thrrx)
        thrrx=thrrx.set_index('Time')
        results['part-' + str((p+1) % parts.shape[0])]['THR_MEAN-0'] = str(thrrx['throughput'].mean()) if not math.isnan(thrrx['throughput'].mean()) else '0'
        results['part-' + str((p+1) % parts.shape[0])]['THR_STD-0'] = str(thrrx['throughput'].std()) if not math.isnan(thrrx['throughput'].std()) else '0'
        if len(thrrx['throughput']) == 0:
            promedio = 0
        else:
            promedio = np.nanmean(thrrx['throughput'].values)
        if DEBUG: print(f'mean: {promedio}')
    # Per UE Throughput, goal, smooth    
    #print("convergence", end=".", flush=True)
    DEBUG=0
    for ue in tmp['rnti'].drop_duplicates().sort_values().unique():
        # thrrx=pd.DataFrame(tmp.groupby('rnti').resample(str(resamplePeriod)+'ms').packetSize.sum())
        # thrrx=thrrx.reset_index(level=0)
        thrrx = tmp.copy()
        thrrx=thrrx[thrrx['rnti']==ue]
        thrrx['InsertedDate']=thrrx.index
        thrrx['deltaTime']=thrrx['InsertedDate'].astype(int) / 1e9
        thrrx['Time']=thrrx['deltaTime']

        thrrx['deltaTime']=abs(thrrx.diff()['deltaTime'])

        thrrx.loc[~thrrx['deltaTime'].notnull(),'deltaTime']=thrrx.loc[~thrrx['deltaTime'].notnull(),'InsertedDate'].astype(int) / 1e9
        thrrx['throughput']= thrrx['packetSize'] * 8 / thrrx['deltaTime'] / 1e6

        thrrx=thrrx.set_index('Time')
        if (DEBUG==1): print(thrrx)
        #print('prepare', end=".", flush=True)

            
        thr=thrrx
        thr['Time']=thr.index
        if (DEBUG==1): print(thr)
        for p in range(parts.shape[0]-1):
            #print(f"p{p}",  end=":", flush=True)

            [x, y, z] = parts[p,:]
            if (DEBUG==1): print(TXT_GREEN + "Range: ["+ x+","+ y+"]" + TXT_CLEAR)
            deltat=max([1.6, (float(y)-float(x)) / 5])
            
            # Throughput > Zero
            xi = float(x)
            if (DEBUG==1): print("["+ str(xi)+","+ str(xi+deltat)+"]")
            if (DEBUG==1): print(TXT_MAGENTA + "UE:"+ str(ue) + " Finding Throughput Zero in: ["+ str(xi)+","+ str(xi+deltat)+"]" + TXT_CLEAR)
            if (DEBUG==1): print(f"First xi: %f" % xi)
            # Min time in this range when thr>0
            if (len(thr[((thr['Time'] >= float(x) ) & (thr['Time'] <=float(y)) & (thr['throughput']>0) )]['throughput'].index)>0):
                xi = thr[((thr['Time'] >= float(x) ) & (thr['Time'] <=float(y))  & (thr['throughput']>0) )]['throughput'].index.values.min()
            else:
                xi = float(y)
            if (DEBUG==1): print(f"Sec xi: %f" % xi)

            # Throughput mean a nd std
            throughput=np.array(thr[(thr['Time'] >= float(x) ) & (thr['Time'] <= float(y))]['throughput'].values)
            if (len(throughput)==0):
                thr_mean = 0
                thr_std = 0
                bins = [0]
                cdf = [0]
            else:
                # included when all UEs are running
                thr_mean = thr[(thr['Time'] >= max([float(x), max(AppStartTimes)]) ) & (thr['Time'] <= float(y))]['throughput'].mean()
                thr_std  = thr[(thr['Time'] >= max([float(x), max(AppStartTimes)]) ) & (thr['Time'] <= float(y))]['throughput'].std()
                            
                data = np.array(thr[(thr['Time'] >= max([float(x), max(AppStartTimes)]) ) & (thr['Time'] <= float(y))]['throughput'].values)
                count, bins_count = np.histogram(data, bins=100)
                pdf = count / sum(count) 
                cdf = np.cumsum(pdf)
                thr_bins = bins_count[1:]
                thr_cdf = cdf
            
            
            # Threshhold
            while xi < float(y)-deltat:
                if (DEBUG==1): print(TXT_MAGENTA + "UE:"+ str(ue) + " Finding stability in: ["+ str(xi)+","+ str(xi+deltat)+"]" + TXT_CLEAR)

                # range to analize
                throughput=np.array(thr[(thr['Time'] >= xi ) & (thr['Time'] <=xi+deltat)]['throughput'].values)
                mytime=np.array(thr[(thr['Time'] >= xi ) & (thr['Time'] <=xi+deltat )]['throughput'].index.values)
                if (DEBUG==1): print(mytime)
                if (DEBUG==1): print(throughput)
                

                # Criteria thr.min <= conv_thresh * thr.mean
                if len(throughput)>1:
                    goal = throughput.mean()
                    if (DEBUG==1): print(f"THR: [%f - %f]" % (goal*(1 - conv_thresh), goal*(1 + conv_thresh)))
                    if(throughput.min()<=goal*(1 - conv_thresh)):
                        i = min([np.where(throughput==throughput[throughput<=goal*(1 - conv_thresh)].max())[0][0] +1 , len(throughput) -1])
                        xi = mytime[i]
                    elif (throughput.max()>=goal*(1 + conv_thresh)):
                        xi = xi + resamplePeriod / 1000
                    else:
                        if (DEBUG==1): print(TXT_RED + "Convergence" + TXT_CLEAR)
                        break
                else:
                    xi = xi + resamplePeriod / 1000
            if (DEBUG==1): print(f"Final xi: %f" % xi)
            if (DEBUG==1): print(f"Final: %f" % (float(y)-deltat))
            if xi>=float(y)-deltat:
                if (DEBUG==1): print(TXT_CYAN +  "UE:"+ str(ue) + " NO Convergence in: ["+ str(x)+","+ str(y)+"]" + TXT_CLEAR)
                throughput=np.array(thr[(thr['Time'] >= float(x) ) & (thr['Time'] <=float(y) )]['throughput'].values)
                mytime=np.array(thr[(thr['Time'] >= float(x) ) & (thr['Time'] <=float(y) )]['throughput'].index.values)
                
                ct0 = float(y)
                convergence_time = float(y)
                if len(throughput)>1:
                    goal = throughput.mean()
                    if throughput.mean() == 0:
                        smooth = 0
                    else:
                        smooth = abs(throughput.max()-throughput.min()) / throughput.mean()
                else:
                    goal = 0
                    smooth = 0
            else:
                if (DEBUG==1): print(TXT_CYAN +  "UE:"+ str(ue) + " Convergence in: ["+ str(xi)+","+ str(xi+deltat)+"]" + TXT_CLEAR)

                throughput = np.array(thr[(thr['Time'] >= xi ) & (thr['Time'] <=xi+deltat)]['throughput'].values)
                mytime = np.array(thr[(thr['Time'] >= xi ) & (thr['Time'] <=xi+deltat )]['throughput'].index.values)
                goal = throughput.mean()
                if (DEBUG==1): print(mytime)
                if (DEBUG==1): print(throughput)
                if (DEBUG==1): print(f"Goal: %f" % goal)
                # convergence time
                if len(throughput)>1:
                    if throughput[0]< goal:
                        i = np.where (throughput== throughput[throughput <= goal].max() )[0][0]
                    else:
                        i = np.where (throughput== throughput[throughput >= goal].max() )[0][0]
                    convergence_time = mytime[i] 
                    
                else:
                    if (DEBUG==1): print(TXT_RED+ "Does Not converge" + TXT_CLEAR)
                    throughput=np.array(thr['throughput'].values[-1:])
                    mytime=np.array(thr.index.values[-1:])
                    if (DEBUG==1): print(mytime)
                    if (DEBUG==1): print(throughput)
                    convergence_time = float(y)

                if (DEBUG==1): print(mytime)
                if (DEBUG==1): print(throughput)

    
                # usamos como referencia para calcular el throughput objetivo
                if(convergence_time >= mytime.max()):
                    i = len(mytime)-1
                else:
                    i = np.where(mytime==mytime[mytime>convergence_time][0])[0][0]

                if (DEBUG==1): print(f"convergence_time: %f" % convergence_time)
                if (DEBUG==1): print(f"i: %f" % i)

                if(i == 0):
                    goal_max=0
                    goal_max=0
                else:
                    goal_max = throughput[i:].max()
                    goal_min = throughput[i:].min()

                if (DEBUG==1): print(f"i: {i} - {i+round(deltat/(resamplePeriod/1000))}")
                if (DEBUG==1): print(TXT_MAGENTA + "Goal: " + str(goal) +"\t" + "["+ str(goal_min) +","+ str(goal_max) + "]"+ TXT_CLEAR )

                # finding contact point
                if(goal>=throughput.max()):
                    convergence_time = mytime.max()
                else:
                    if throughput[:i].mean() <= goal :
                        j = np.where(throughput == throughput[throughput>goal][0])[0][0] - 1
                    else:
                        j = np.where(throughput == throughput[throughput<=goal][0])[0][0] - 1
                    if (j<0): j=0
                    while (throughput[j] == throughput[j+1]): j = j + 1
                    
                    convergence_time = mytime[j] + abs((mytime[j+1]-mytime[j])/(throughput[j+1]-throughput[j])*(goal-throughput[j]))

                if (DEBUG==1): print(TXT_MAGENTA + "CT:" + str(convergence_time) + TXT_CLEAR)

                # smoothness
                if goal==0:
                    smooth = 0
                else:
                    smooth = abs(goal_max-goal_min) / goal            

                # print(f"El tiempo de convergencia de {t} en {p} es: {convergence_time-float(x)}")
                
            if (DEBUG==1): print("Plotting")
            if (DEBUG==1): print("CT: %f" %convergence_time)
            if (DEBUG==1): print("smooth: %f" %smooth)

            # Graphics
            fig, ax = plt.subplots()
            title = "Convergence Time"
            title += " UE " + str(ue)

            throughput = np.array(thr[(thr['Time'] >= float(x) ) & (thr['Time'] <= float(y)) ]['throughput'].values)
            mytime = np.array(thr[(thr['Time'] >= float(x) ) & (thr['Time'] <= float(y) ) ]['throughput'].index.values)

            # Original Curve
            ax.plot(mytime, throughput)

            # Polynomial
            # time_curve = np.linspace(float(x), min([ max([ct0, convergence_time])+ deltat, float(y) ]), 100)
            # throughput_curve = polynomial(time_curve)
            
            # # Crear el grafico comparado con los datos originales recortados
            # ax.plot(time_curve, throughput_curve)
            
            # Goal Line
            # ax.plot(time_curve,time_curve*0+goal, 'r--')
            # ax.annotate( "Goal", (max([0, time_curve[0]-10]) , goal ))
            # Goal Line
            if convergence_time< float(y):
                time_curve = np.linspace(float(x), convergence_time, 100)
                ax.plot(time_curve, time_curve*0 + goal, 'r--')
                ax.annotate( "Goal", (time_curve[0] , goal*(1 + conv_thresh/4) ))

            # Convergence Threshhold
            if (DEBUG==1): print(goal*(1 - conv_thresh))
            ax.plot(mytime, mytime*0 + goal*(1 - conv_thresh), '--', color='orange')
            ax.plot(mytime, mytime*0 + goal*(1 + conv_thresh), '--', color='orange')
            # ax.annotate( "conv_thresh_min", (max([float(x), mytime[0]-10]) , goal*(1 - conv_thresh) ))
            # ax.annotate( "conv_thresh_max", (max([float(x), mytime[0]-10]) , goal*(1 + conv_thresh) ))

            # Min/max Line
            # time_curve = np.linspace(ct0, min([ max([ct0, convergence_time])+ deltat, float(y) ]), 100)
            # ax.plot(time_curve,time_curve*0+goal_min, color='red')
            # ax.plot(time_curve,time_curve*0+goal_max, color='red')
            
            # Convergence Time
            if convergence_time < float(y):
                if (float(x) < AppStartTime):
                    convergence_time = convergence_time - AppStartTime
                ax.plot([convergence_time]*10, np.linspace(goal, goal*(1 - conv_thresh) *0.9, 10), '--', color='red')
                ax.annotate( "CT: " + str(round(convergence_time-float(x),2)) + "[s]", (convergence_time , goal*(1 - conv_thresh)*0.9 ))
                rect = mpatches.Rectangle((xi, goal*(1 - conv_thresh)), deltat, goal*(conv_thresh)*2, alpha=0.2, facecolor="orange")
                plt.gca().add_patch(rect)

            ax.set_xlabel('Time [s]')
            ax.set_ylabel('Throughput [Mb/s]')
            # fig.set_tight_layout(True)
            if ((len(throughput) > 0) and (throughput.max() != 0)):
                ax.set_ylim([0, max([0, throughput.max()*1.1])])

            if show_title:
                plt.suptitle(title, y = 0.99, fontsize = title_font_size)
                plt.title(subtitle, fontsize = subtitle_font_size)

            plotfile = myhome + prefix + 'CT-' + str(z) + '-' + 'UE' + str(ue) + '.png'
            fig.savefig(plotfile)
            if (DEBUG==1): print(plotfile)
            plt.close()

            convergence_time = min([convergence_time-float(x), float(y)])
            if (convergence_time >= float(y) - float(x)):
                if len(throughput)>1:
                    if throughput.mean() == 0:
                        smooth = 0
                    else:
                        smooth = abs(throughput.max() - throughput.min()) / throughput.mean()
                else:
                    smooth=0

            # Summary
            if (DEBUG==1): print("CT: %f" %convergence_time)
            if (DEBUG==1): print("smooth: %f" %smooth)
            if (DEBUG==1): print("goal: %f" %goal)
            results['part-' + str(p+1)]['CT-' + str(ue)] = str(convergence_time)
            results['part-' + str(p+1)]['SMOOTH-' + str(ue)] = str(smooth)
            results['part-' + str(p+1)]['GOAL-' + str(ue)] = str(goal)
            results['part-' + str(p+1)]['THR_MEAN-' + str(ue)] = str(thr_mean) if not math.isnan(thr_mean) else '0'
            results['part-' + str(p+1)]['THR_STD-' + str(ue)]  = str(thr_std) if not math.isnan(thr_std)  else '0'
            results['part-' + str(p+1)]['THR_BINS-' + str(ue)] = np.array2string(thr_bins, separator=',') 
            results['part-' + str(p+1)]['THR_CDF-' + str(ue)]  = np.array2string(thr_cdf, separator=',') 

    with open(myhome + 'results.ini', 'w') as configfile:
        results.write(configfile)

    #Plot THR CDF
    title = "Throughput"
    for p in range(parts.shape[0]-1):
        [x, y, z] = parts[p,:]
        if DEBUG: print('zone: ', z)
        fig, ax = plt.subplots()
        for ue in tmp['rnti'].drop_duplicates().sort_values().unique():    
            if DEBUG: print('ue: ', ue)
            bins = results['part-' + str(p+1)]['THR_BINS-' + str(ue)]
            if DEBUG: print('bins: ', type(bins), bins)
            bins = eval('np.array(' + bins + ')')
            if DEBUG: print('bins: ', type(bins), bins)
            
            cdf =  results['part-' + str(p+1)]['THR_CDF-' + str(ue)]
            cdf = eval('np.array(' + cdf + ')')
            if DEBUG: print('bins: ', type(bins), bins)
            plt.plot(bins, cdf)

        ax.set_ylim([0 , 1.1])
        ax.set_xlim([0 , thr_limit])
        ax.set_ylabel("CDF")
        ax.set_xlabel("Throughput [Mb/s]")

        if show_title:
            plt.suptitle(title + ' CDF', y = 0.99, fontsize = title_font_size)
            plt.title(subtitle, fontsize = subtitle_font_size)

        if show_legend:
            text_legend = tmp['rnti'].drop_duplicates().sort_values().unique()
            text_legend = [ 'UE ' + str(i)  for i in text_legend]
            ax.legend(text_legend, 
                        loc = 'upper center',
                        ncol = max([1, len(tmp['rnti'].unique()) // 2 ])
                        )
        fig.savefig(myhome + prefix + 'ThrRxCDF-' + str(z) + '.png')           



    #toc = time.time()
    #print(f"\tProcessed in: %.2f" %(toc-tic))
    return True



if __name__ == '__main__':
    main()
    exit ()


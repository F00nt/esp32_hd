# coding=cp1251
'''
���������� ������� ���������� �� ���� 
���������� 3 ����
1� � 3� - ������ ���, �� ���� ������� �����
2� ���� ����������  ������ ��������� 80 (��� ������� ���������� -t) ����� ���� 

��������� �������:
log2graph.py {-p[N]} {-t[N]} {filename}
-p[N]  - ������ � ���. ������������� ����� ���� � ���������� ��������. ���� �� ������ - ������ ����� 2 ���
-p0 - �������� �������������/����������. ������������ ��� �������� ������ ��������� �����
-t - ���������� ����� ���� ������������ �� ������ �������. ������� 2. �� ��������� 80 (��������� TAIL_LEN)
-h - help, ������� ����� � ������ �������
filename - ��� �����. ���� �� �����, �� ������������ esp32hd.log

������� �������:
>log2graph.py
������ ���� esp32hd.log � �������� 2 �������, ���� ���� �������� - ����� ��������� �������

>log2graph.py ������.log
����� ������ ���� ������.log � �������� 2 �������, ���� ���� �������� - ����� ��������� �������

>log2graph.py -p15 ������.log
����� ������ ���� ������.log � �������� 15 ������

>log2graph.py -p15 -t100 ������.log
����� ������ ���� ������.log � �������� 15 ������, ������ ������ ����� ���������� ������ 100 ��������� ����� ����

>log2graph.py -p0 -t99999999999 ������������.log
���������� ����� ������ � ������� ��� �� ����� ������������.log. ���� ���� ���������� - ������� �������� �� �����
������ ������ ����� ���������� ������ ���, ���� ��� ����� ������ 99999999999 �����

===========================================================
������ ����: csv , ����������� ';'
� ������ ������ �.���� ����� ����� ����, ����������� ';'

(!!!)������ ����� � ���� ����� uptime - ����� � ������� HH:MI:SS �� ������ ��������

������ ����:
time;uptime;MainMode;MainStatus;waitStr;CurPower;SetPower;Alarm;V0open;V1open;V2open;V2pwm;V2period;V2prc;V3open;A0val;A1val;A2val;A3val;bmpTruePressure;bmpTemperature;
10:29;00:24:17;2;0;;-1;0;128;0;0;0;0;0;0;0;172;4798;4810;4822;741.61;24.2;
10:30;00:24:22;2;0;;-1;0;128;0;0;0;0;0;0;0;178;4788;4820;4842;741.64;24.2;
10:30;00:24:27;2;0;;-1;0;128;0;0;0;0;0;0;0;172;4798;4830;4834;741.61;24.3;
...

����� ���� �������� ��� ������� ������ ��������� SENSORS_NAME
����� ���� ���� ���������� � �������� ������ ��������� GRAPH_LOGS
'''
import csv
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.animation as anim
from matplotlib import mlab
import datetime, time
import matplotlib.dates as mdates
from matplotlib.dates import DateFormatter

#������ ��������������� ����� ���� �� ������� ������� ������ ��� ��������
 
SENSORS_NAME = "TCube;TTube 20%;TTube;TDeflegmator;TWater IN;TWater Out;TAlarm;".split(';')
#time;uptime;MainMode;MainStatus;waitStr;CurPower;SetPower;Alarm;TCube;TTube 20%;TTube;TDeflegmator;TWater IN;TWater Out;TAlarm;V0open;V1open;V2open;V2pwm;V2period;V2prc;V3open;bmpTruePressure;bmpTemperature;

# ������ ����� ���������� �� ������ ���� � ����� ���� ����������. 
# ������ ������ - ������ ����, ������ - ������, ������ - ������
# ����� � ������� -  ������ ����������  � ���������� ������
#GRAPH_LOGS  = [[0,1,2,4],[1,2,4],[5,6,7]]
GRAPH_LOGS  = [[0,1,2,3],[1,2,3],[4,5,6]]

TAIL_LEN = 80
PERIOD_SEC = 2

pos_sensor = []
period = PERIOD_SEC
tail_len = TAIL_LEN

def is_digit(string):
    if string.isdigit():
       return True
    else:
        try:
            float(string)
            return True
        except ValueError:
            return False

def read_header(fname):
    pos_sensor = []
    arr = []
    try:
        with open(fname, "r", newline="") as file:
          #������ ���� �������
          reader = csv.reader(file)
          line_no=0
          for row in reader:
            #import ipdb; ipdb.set_trace()
            if row:
              fields = row[0].split(';')
              if fields[0]=='time':
                for i in range(len(SENSORS_NAME)):
                  arr.append([])
                  if SENSORS_NAME[i] in fields:
                    pos_sensor.append(fields.index(SENSORS_NAME[i]))
                  else:
                    pos_sensor.append(-1)
                return pos_sensor
    except e:
        print(f'���� {fname} �� ������')
        return []

from datetime import datetime

def getSecUptime(uptime_str):
    # uptime_str �.���� � ������� HH24:MI:SS
    try:
        if len(uptime_str.split(':'))==2:
            t = datetime.strptime(uptime_str,'%M:%S')
        else:
            t = datetime.strptime(uptime_str,'%H:%M:%S')
    except ValueError:
        print(f'error uptime "{uptime_str}"')
    #print(f' str:{uptime_str} t:{t}')
    return t
    '''    
    datetime.strptime("21/11/06 16:30", "%d/%m/%y %H:%M")
    #��������� ������ �� ����������� ':'
    uptime_arr = uptime_str.split(':')
    # ������ ���������� ��� ���������� ������
    m = [1,60,3600]
    ret  = 0
    try:
      k = 0
      for i in reversed(uptime_arr):
        ret += int(i)*m[k];
        k +=1
    except ValueError:
      #���� uptime �� �����������
      ret = -1
    return ret
    '''
        
def readCsv(fname):
    arr = []
    # ���� ������� �� �������, ���������� ������ ������
    if (len(pos_sensor)==0):
      return arr
      
    # ������� ������ �������� ��� ����������� �������� �������� + 1 �� ��� X
    for i in range(len(pos_sensor)+1): 
      arr.append([])
    
    #������ ������ �� �����
    nrow=1
    with open(fname, "r", newline="") as file:
        #������ ���� �������
        reader = csv.reader(file)
        for row in reader:
            if row:
              nrow += 1
              # ����������� ������ � ������
              fields = row[0].split(';') 
              if is_digit(fields[0].split(':')[0]): #���� ������  �������� �����, �������� ��������� ������ 
                #�������� �������� �� ������� ���� uptime � ��������. ��� ������ ������� -1
                uptime = getSecUptime(fields[0])
                #if (uptime< 0):
                if (uptime==None):
                #���� uptime �� ����������� - �� ��.������
                  print(f'������ ������� ���� uptime "{fields[0]}"')
                  continue
                #������� uptime � ������ X
                arr[len(pos_sensor)].append(uptime)
                
                #���� � ������ ���� �������� � ��������� �� � ��������������� ������� Y
                for i in range(len(pos_sensor)): # ��� ���� ��������
                  if pos_sensor[i] != -1: # ���� ������ �������� � ��������� �����
                    # ��������������� � �����
                    try:
                        v = float(fields[pos_sensor[i]])
                    except ValueError:
                        v = -127.0 # ���� ������ �������������� �  �����
                    # ��������� ��������, ���� ��������� - �������� �� "���������" 22.22 
                    if (v ==-127.0):
                        v = 22.22
                  else:
                    v = 0;
                  #��������� � ������ ��������
                  arr[i].append(v)
    return arr

def getTailStart(arr):
    ret = len(arr[0]) - tail_len
    if ret <0:
        ret = 0
    return ret

def plot_cont(filepath):
    
    from pylab import rcParams        
    rcParams['figure.subplot.left'] = 0.04 # ����� �������
    rcParams['figure.subplot.right'] = 0.96 # ������ �������
    rcParams['figure.subplot.bottom'] = 0.1 # ������ �������
    rcParams['figure.subplot.top'] = 0.95 # ������� �������
    
    fig = plt.figure()
    fig.set_size_inches(16,6)
    ax  = fig.add_subplot(1,3,1)
    ax2 = fig.add_subplot(1,3,2)
    ax3 = fig.add_subplot(1,3,3)

    def update(i):
        global pos_sensor
        arr = readCsv(filepath)
        xlist = arr[len(arr)-1]
        ax.clear()
        #SENSORS_NAME = "TCube;TTube 20%;TTube 80%;TDeflegmator;TTSA;TWater IN;TWater Out".split(';')
        color = ['b-','g--','r-','y--','c-','m-', 'k-', 'w-']
        c = 0
        for i in GRAPH_LOGS[0]:
          if pos_sensor[i]>0:
            ax.plot_date (xlist, arr[i], color[c], label = SENSORS_NAME[i])
          c+=1
        ax.grid(True)
        ax.legend ()
        ax.set_xlabel('Uptime, ���')
        ax.xaxis.set_major_formatter( DateFormatter('%H:%M:%S') )
        #ax.set_xticks(df.index)
        #ax.set_xticklabels(df.manufacturer.str.upper(), rotation=60, fontdict={'horizontalalignment': 'right', 'size':12})
        #ax.set_ylabel('�����������, �')
        #ax.set_title ('������ ������')
        
        #-------------------------------------
        tail_start = getTailStart(arr)
        xlist2 = xlist[tail_start:]
        ax2.clear()
        c = 0
        for i in GRAPH_LOGS[1]:
          if pos_sensor[i]>0:
            ax2.plot (xlist2, arr[i][tail_start:], color[c], label = SENSORS_NAME[i])
            #print(arr[i][tail_start:])
          c+=1
        ax2.grid(True)
        ax2.legend ()
        ax2.set_xlabel('Uptime, ���')
        ax2.xaxis.set_major_formatter( DateFormatter('%H:%M:%S') )
        #ax2.set_ylabel('T����������,�')
        ax2.set_title(f'{len(xlist2)} ��������')
        #-------------------------------------
        ax3.clear()
        c = 0
        for i in GRAPH_LOGS[2]:
          if pos_sensor[i]>0:
            ax3.plot (xlist, arr[i], color[c], label = SENSORS_NAME[i])
          c+=1
        ax3.grid(True)
        ax3.legend ()
        ax3.set_xlabel('Uptime, ���')
        ax3.xaxis.set_major_formatter( DateFormatter('%H:%M:%S') )
        ax3.set_ylabel('T,�')
        ax3.set_title('����')
        fig.autofmt_xdate(rotation=45)
        '''
        # Plot Line2 (Right Y Axis)
        ax4 = ax3.twinx()  # instantiate a second axes that shares the same x-axis
        ax4.plot(x, y2, color='tab:blue')
        '''
    if period >0:
        a = anim.FuncAnimation(fig, update, interval=period*1000, repeat=True)
    else:
      update(1)
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='log2graph')
    parser.add_argument(
        "filename",
        nargs='?',
        help="filename of log [esp32_hd.log by default]",
        default="esp32_hd.log")
    parser.add_argument("-period", "-p", nargs='?', type=int, help=f"re-draw  period,sec {PERIOD_SEC} by default. 0-no redraw", default=PERIOD_SEC)
    parser.add_argument("-tail", "-t", nargs='?', type=int, help=f'point number of the 2d plot, min 2. By default {TAIL_LEN}', default=TAIL_LEN)
    args = parser.parse_args()

    period = args.period
    filepath = args.filename
    tail_len = args.tail
    if tail_len<2:
      tail_len = 2
    try:
        f = open(filepath)
        f.close
    except FileNotFoundError:
        print(f'���� {filepath} �� ������')
        exit()

    print(f"logfile:{filepath} re-draw period:{period} sec tail:{tail_len}")
    pos_sensor = read_header(filepath)
    if (pos_sensor.count(-1)>10):
        print(f'������� ����������� � ���� "{filepath}"�� �������')
        exit()
    plot_cont(filepath)

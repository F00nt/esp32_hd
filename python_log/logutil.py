# coding=cp1251
def get_header(filename, key_field_name):
    try:
        with open(filename, "r", newline="") as file:
            line_no=0
            for line in file:
                header = line.split(';')
                if (header.count(key_field_name)>0): #���� � ������ ���� �������� ����
                    return header; # ��� ���������
    except Exception:
        print(f'������ ������ ����� {filename}')
    return None;  

def  findSensor(header, sensor_name_list):
    ret_list = []
    for sensor in sensor_name_list:
        try:
            index =header.index(sensor)
        except Exception:
            index = -1
        ret_list.append(index)
    return ret_list

from datetime import datetime
def getSecUptime(uptime_str):
    # uptime_str �.���� � ������� HH24:MI:SS
    try:
        if len(uptime_str.split(':'))==2:
            return datetime.strptime(uptime_str,'%M:%S')
        else:
            return datetime.strptime(uptime_str,'%H:%M:%S')
    except ValueError:
        #print(f'error uptime "{uptime_str}"')
        pass
    return None

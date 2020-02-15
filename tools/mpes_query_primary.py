#! /usr/bin/env python
import pymysql
import argparse
import os


#included_panels = [1111, 1112, 1113, 1114, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1211, 1212, 1213, 1214, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1311, 1312, 1313, 1314, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1411, 1412, 1413, 1414, 1421, 1422, 1423, 1424, 1425, 1426, 1427, 1428] # all
#included_panels = [1111, 1112, 1113, 1114, 1211, 1212, 1213, 1214, 1311, 1312, 1313, 1314, 1411, 1412, 1413, 1414] # only inner ring
included_panels = [1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1421, 1422, 1423, 1424, 1425, 1426, 1427, 1428] # only outter ring
#included_panels = [1121, 1123, 1125, 1127, 1221, 1223, 1225, 1227, 1321, 1323, 1325, 1327, 1421, 1423, 1425, 1427] # only outter ring odd
#included_panels = [1122, 1124, 1126, 1128, 1222, 1224, 1226, 1228, 1322, 1324, 1326, 1328, 1422, 1424, 1426, 1428] # only outter ring even

#included_panels = [1312,1322] # no.2
#included_panels = [1213,1225,1214] # no.3
#included_panels = [1112,1113,1123,1124,1125,1126] # for 10323 and 10047 calibration
#included_panels = [1114,1211,1128] # for 10344 calibration
#included_panels = [1425,1426,1427,1413] # for 10343 and 10205 calibration

KNOWN_BAD_SENSORS=[] # permanent dead
#KNOWN_BAD_SENSORS+=[10031] # calibrated
#KNOWN_BAD_SENSORS+=[10323] # calibrated
#KNOWN_BAD_SENSORS+=[10344] # calibrated
#KNOWN_BAD_SENSORS+=[10047] # calibrated
#KNOWN_BAD_SENSORS+=[10187] # calibrated
KNOWN_BAD_SENSORS+=[10046] # needs calibration
KNOWN_BAD_SENSORS+=[10003] # needs calibration
KNOWN_BAD_SENSORS+=[10343,10030] # calibrated, but keep tracking it
#KNOWN_BAD_SENSORS+=[10281] # did not initialize

KNOWN_BAD_SENSORS+=[10005] # edge aligned, do not move. need clockangle adjust
KNOWN_BAD_SENSORS+=[10029] # usb port swapped. not calibrated yet.
KNOWN_BAD_SENSORS+=[10009] # needs calibration
#KNOWN_BAD_SENSORS+=[10181] # did not initialize
KNOWN_BAD_SENSORS+=[10008] # needs calibration
KNOWN_BAD_SENSORS+=[10013,10016] # suspects, largely misaligned

KNOWN_BAD_SENSORS+=[10177] # laser intensity too low

def print_query_db_MPESes(panels, KNOWN_BAD_SENSORS=[], verbose=True):
    str1_ = ""
    i=0
    for p in panels:
        if i==0:
            str1_ = "(w_panel={} or l_panel={}".format(p,p)
        else:
            str1_ = str1_+" or w_panel={} or l_panel={}".format(p,p)
        i+=1
    str1_ = str1_+")"
    if KNOWN_BAD_SENSORS:
        i=0
        for bads in KNOWN_BAD_SENSORS:
            if i==0:
                str1_ = str1_+" and (serial_number != {}".format(bads)
            else:
                str1_ = str1_+" and serial_number != {}".format(bads)
            i+=1
        str1_ = str1_+" )"
        
    str1_ = str1_ + ' and end_date IS NULL'
    str_="select serial_number from Opt_MPESMapping where "+str1_+";"
    if verbose:
        print(str_)
    return str_

    
class pSCTDB_readonly:
    def __init__(self, host='romulus.ucsc.edu'):
        self.DB_HOST=host
        print(self.DB_HOST)
        #self.DB_USER='CTAreadonly'
        #self.DB_PASSWD='readCTAdb'
        #self.DB_ONLINE='CTAonline'
        #self.DB_OFFLINE='CTAoffline'
        #self.DB_PORT=3406
        self.DB_USER=os.getenv('CTADBUSERREADONLY')
        self.DB_PASSWD=os.getenv('CTADBPASSREADONLY')
        self.DB_ONLINE=os.getenv('CTAONLINEDB')
        self.DB_OFFLINE='CTAoffline'
        self.DB_PORT=int(os.getenv('CTADBPORT'))

        
    def connect(self):
        try:
            self.conn = pymysql.connect(host=self.DB_HOST,
                                    user=self.DB_USER,
                                    passwd=self.DB_PASSWD,
                                    db=self.DB_ONLINE, 
                                    port=self.DB_PORT)
            self.cur = self.conn.cursor()
        except:
            print("Cannot connect to {}, consider changing this".format(self.DB_HOST))
            exit()

    def close_connect(self):
        self.cur.close()
        self.conn.close()

    def do_query(self, query):
        self.connect()
        nentries=self.cur.execute(query)
        #self.close_connect()
        return self.cur


    def query_mpeses(self, panels, KNOWN_BAD_SENSORS=[], verbose=True):
        query = print_query_db_MPESes(panels, KNOWN_BAD_SENSORS=KNOWN_BAD_SENSORS, verbose=False)
        self.connect()
        nentries=self.cur.execute(query)
        mpeses = []
        for row in self.cur:
            mpeses.append(row[0])
            if verbose:
                print(row[0])
        self.close_connect()
        return mpeses



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Print out ALL MPESes on the provided panels, excluding specified panels (currently hard coded)')
    parser.add_argument('-H', '--host', default=os.getenv('CTADBHOST'), help="Host for DB", type=str)
    args = parser.parse_args()

    print(args.host)
    testDB = pSCTDB_readonly(args.host)
    mpes_q_res = testDB.query_mpeses(included_panels, KNOWN_BAD_SENSORS)
    
    print('+++++++ included panels ++++++++')
    for p in range(0,len(included_panels)):
        print included_panels[p]




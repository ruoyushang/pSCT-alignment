#! /usr/bin/env python
import pymysql
import argparse
import os


#included_panels = [1111, 1112, 1113, 1114, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1211, 1212, 1213, 1214, 1221, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1311, 1312, 1313, 1314, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1411, 1412, 1413, 1414, 1421, 1422, 1423, 1424, 1425, 1426, 1427, 1428]
included_panels = [1112, 1113, 1114, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1211, 1212, 1213, 1214, 1222, 1223, 1224, 1225, 1226, 1227, 1228, 1311, 1313, 1314, 1321, 1322, 1323, 1324, 1325, 1326, 1327, 1328, 1411, 1412, 1413, 1414, 1421, 1422, 1423, 1424, 1425, 1426, 1427] # excluded non-functioning panels 1111, 1221, 1312, 1428 
KNOWN_BAD_SENSORS=[] # permanent dead
KNOWN_BAD_SENSORS+= [10302, 10295, 10086, 10076, 10301, 10182, 10084, 10078, 10028, 10211, 10206, 10281, 10013, 10016, 10207, 10280, 10263, 10314, 10256, 10253, 10235, 10334, 10257, 10245, 10243, 10247] # working sensors deliberately excluded around non-functioning panels 1221, 1312, 1428
KNOWN_BAD_SENSORS+=[10050, 10130] # working but need alignment before useable
KNOWN_BAD_SENSORS+=[] # These are temporarily laser lost 
#KNOWN_BAD_SENSORS+=[10219] # working but need alignment before useable

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




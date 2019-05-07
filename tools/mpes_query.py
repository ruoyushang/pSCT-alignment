#!/usr/bin/env python
import pymysql
import argparse
import os


#included_panels=[2121,2122,2123,2124,2221,2222,2223,2224,2321,2322,2323,2324,2421,2422,2423,2424]
included_panels=[2121,2122,2123,2124,2221,2321,2322,2323,2324,2421,2422,2423,2424]
KNOWN_BAD_SENSORS=[10351, 10332, 10358, 10152, 10164, 10153, 10143, 10146,10046,10278]
KNOWN_BAD_SENSORS+=[10172,10165,10176,10162,10048,10296,10174,10154]#For panel 2223

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




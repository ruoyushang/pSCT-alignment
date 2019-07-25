#!/usr/bin/env python
import argparse
import os

import pymysql


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
    
    def query_panel_info(self, panels):
        if not isinstance(panels, list):
            panels = [str(panels)]
        else:
            panels = [str(i) for i in panels]
        self.connect()
        for p_ in panels:
            print("====")
            query = "select mpcb_id, mpcb_ip_address, pedb_channel from Opt_MPMMapping where position = {}".format(p_)
            nentries=self.cur.execute(query)
            for i, row in enumerate(self.cur):
                #ips.append(row[0])
                print("Panel {} CBC {}".format(panels[i], row[0]))
                print("Panel {} IP {}".format(panels[i], row[1]))
                print("Panel {} PEDB Channel {} port {}".format(panels[i],row[2],(int(row[2])+2)/3)) 

            print("====")

            q2 = "select * from Opt_MPESMapping where (w_panel = {} or l_panel = {}) and end_date IS NULL".format(p_,p_)
            nentries2=self.cur.execute(q2)
            print("Panel {}".format(p_))
            print("MPES mapping")
            for i, row in enumerate(self.cur):
                print("serial_number {} w_panel {} l_panel {} w_position {} port {} comment {}".format(row[2],row[3],row[4],
                                                                                                      row[5],row[6],row[7]))
            print("====")


            q3="select serial_number, port,comment from Opt_ActuatorMapping where end_date is NULL and panel in ({});".format(p_)
            nentries2=self.cur.execute(q3)
            print("Panel {}".format(p_))
            print("Actuator mapping")
            for i, row in enumerate(self.cur):
                print("serial_number {} port {} comment {}".format(row[0],row[1],row[2]))
            
            print("====")
           
        self.close_connect()


    def query_panel_ip(self, panels, verbose=True):
        if not isinstance(panels, list):
            panels = [str(panels)]
        else:
            panels = [str(i) for i in panels]
        query = "select mpcb_ip_address from Opt_MPMMapping where position in ({})".format(','.join(panels))
        self.connect()
        nentries=self.cur.execute(query)
        ips = []
        i=0
        for row in self.cur:
            ips.append(row[0])
            if verbose:
                print("Panel {} IP {}".format(panels[i], row[0]))
            i+=1
        self.close_connect()
        return ips



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Print out ALL mapping for the provided panels')
    parser.add_argument('panel', default='1111', help="Panel ID", type=str)
    parser.add_argument('-H', '--host', default='172.17.10.10', help="Host for DB", type=str)
    args = parser.parse_args()

    testDB = pSCTDB_readonly(args.host)
    testDB.query_panel_info(args.panel)
    




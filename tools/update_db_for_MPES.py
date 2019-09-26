#! /usr/bin/env python
import pymysql
import argparse
import os
import re
from time import gmtime, strftime

currenttime=strftime("%Y-%m-%d", gmtime())
calibration_params= "nominal_reading, l_response_actuator1, w_response_actuator1, l_response_actuator2, w_response_actuator2, l_response_actuator3, w_response_actuator3, l_response_actuator4, w_response_actuator4, l_response_actuator5, w_response_actuator5,l_response_actuator6, w_response_actuator6, plate_scale "

def query_get_old_sensor_calibration(old_sensor_serial,coord):

    execute_part_str="select {} from ".format(calibration_params)
    tablename = "Opt_MPESConfigurationAndCalibration "
    identifier="where serial_number={} and coord={} and end_date is NULL ;".format(old_sensor_serial,coord)
    query=execute_part_str+tablename+identifier
    return query

def query_add_new_sensor_to_mapping(new_mpes_serial, old_mpes_serial, w_panel, l_panel, position, port):
    tablename = "Opt_MPESMapping"
    execute_part_str = "INSERT INTO {table} ".format(table=tablename)
    id_names = "(start_date, end_date, serial_number, w_panel, l_panel, w_position, port, comment) "
    start_date=currenttime
    newInfo="VALUES (\"{start_date}\", {end_date}, {serial_number}, {w_panel}, {l_panel}, {w_position}, {port}, \'{comment}\' ) ;".format(start_date=start_date,end_date="NULL",serial_number=new_mpes_serial,w_panel=w_panel,l_panel=l_panel,w_position=position,port=port, comment="New sensor added to replace {}, will need calibration".format(old_mpes_serial))
    query=execute_part_str+id_names+newInfo
    return query

def query_disable_old_sensor(old_sensor,table, comment):
    execute_part_str="UPDATE "
    end_date=currenttime
    if table == "Opt_MPESMapping":
        set_part=" SET end_date = \"{}\", comment = {} ".format(end_date,comment)
    elif table == "Opt_MPESConfigurationAndCalibration":
        set_part=" SET end_date = \"{}\" ".format(end_date)
    identifier = " WHERE serial_number={} ;".format(old_sensor)
    query = execute_part_str + table + set_part + identifier
    return query

def query_add_new_sensor_to_config(calib_data, new_mpes_serial,coord,targets=None):
    tablename = "Opt_MPESConfigurationAndCalibration"

    execute_part_str = "INSERT INTO {table} ".format(table=tablename)
    start_date = currenttime
    id_params = "start_date, end_date, serial_number, coord, "
    data_str=" "

    if targets is not None:
        if coord =="\"x\"":
            data_str = data_str + str(targets[0]) + ", "
        elif coord == "\"y\"":
            data_str = data_str + str(targets[1]) + ", "
        for i in range(1,len(calib_data)-1):
            data_str = data_str + str(calib_data[i]) + ", "
    else:
        for i in range(0,len(calib_data)-1):
            data_str = data_str + str(calib_data[i]) + ", "
    data_str = data_str + str(calib_data[-1])

    newInfo = "VALUES (\"{start_date}\", {end_date}, {serial_number}, {coord}, {data}) ;".format(start_date=start_date, end_date="NULL", serial_number=new_mpes_serial, data=data_str,coord=coord)

    query = execute_part_str + " (" + id_params + calibration_params + ") " + newInfo
    return query

class pSCTDB_readonly:
    def __init__(self, host='romulus.ucsc.edu'):
        self.DB_HOST=host
        # print(self.DB_HOST)
        self.DB_USER=os.getenv('MYSQL_USER')
        self.DB_PASSWD=os.getenv('MYSQL_PASSWORD')
        self.DB_ONLINE=os.getenv('MYSQL_DATABASE')
        self.DB_OFFLINE='CTAOnline'
        self.DB_PORT=int(os.getenv('MYSQL_PORT'))


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

    def disable_old_sensor(self, old_sensor, table, comment):
        self.connect()
        query_disable_mapping = query_disable_old_sensor(old_sensor, table, comment)
        print("\n"+query_disable_mapping)
        # self.cur.execute(query_add_mapping)
        # if self.cur.lastrowid:lastrowid
        #     print('last insert id', self.cur.lastrowid)
        # else:
        #     print('last insert id not found')
        self.close_connect()


    def add_new_sensor(self, new_mpes_serial, old_mpes_serial, panel, l_panel, position, port, targets=None):
        self.connect()

        query_add_mapping = query_add_new_sensor_to_mapping(new_mpes_serial, old_mpes_serial, panel, l_panel, position, port)
        print("\n"+query_add_mapping)
        # self.cur.execute(query_add_mapping)
        # if self.cur.lastrowid:lastrowid
        #     print('last insert id', self.cur.lastrowid)
        # else:
        #     print('last insert id not found')

        coords=["\"x\"", "\"y\""]
        for c in coords:
            calib_data = self.collect_calibration_data(old_mpes_serial,c)
            query_add_config = query_add_new_sensor_to_config(calib_data, new_mpes_serial,c, targets)
            print("\n"+query_add_config)
        # self.cur.execute(query_add_config)
        # if self.cur.lastrowid:
        #     print('last insert id', self.cur.lastrowid)
        # else:
        #     print('last insert id not found')

        self.close_connect()

    def collect_calibration_data(self,old_sensor_serial, coord):
        query_calib_data = query_get_old_sensor_calibration(old_sensor_serial, coord)
        self.cur.execute(query_calib_data)
        rows = self.cur.fetchall()
        if len(rows) > 1:
            print("Too many possible candidates, something must be wrong with this data.")
        else:
            return rows[0]



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Print out ALL MPESes on the provided panels')
    #example -h 172.18.0.2 10005 10337 1322 1311 2

    parser.add_argument('newMPES',metavar="new",type=int)
    parser.add_argument('oldMPES',metavar="old",type=int)
    parser.add_argument('w_panel')
    parser.add_argument('l_panel')
    parser.add_argument('position')
    parser.add_argument('port')
    parser.add_argument('-T', '--targetCentroid',dest="target", nargs=2,type=int, help="Target centroid values for newly calibrated sensors, done in situ.")

    parser.add_argument('-H', '--host', default=os.getenv('MYSQL_HOST'), help="Host for DB", type=str)
    args = parser.parse_args()

    # print(args.host)
    testDB = pSCTDB_readonly(args.host)

    print("\tNew MPES: {}\n\tOld MPES: {}\n\tw_panel: {}\n\tl_panel: {}\n\tposition: {}\n\tport: {}".format(args.newMPES, args.oldMPES, args.w_panel, args.l_panel, args.position, args.port))

    print("Target coords: {}".format(args.target))
    print("\n")
    print("Inserting {} into Opt_MPESMapping".format(args.newMPES))
    print("Inserting {} into Opt_MPESConfigurationAndCalibration, using {} calibration constants".format(args.newMPES, args.oldMPES))
    print("\n")
    print("Printing out commands for you to insert into DB. Confirm the following lines first, then copy/paste into the database.")
    print("##############################################################")
    print("################## Commands to copy ##########################")
    print("##############################################################")

    mpes_add_res = testDB.add_new_sensor(args.newMPES, args.oldMPES, args.w_panel, args.l_panel, args.position, args.port,args.target)

    tablename = "Opt_MPESMapping"
    comment = "\'Replaced this sensor with {} \'".format(args.newMPES)
    mpes_disable_res = testDB.disable_old_sensor(args.oldMPES, tablename, comment)

    tablename = "Opt_MPESConfigurationAndCalibration"
    comment = ""
    testDB.disable_old_sensor(args.oldMPES, tablename, comment)




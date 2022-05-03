#! /usr/bin/env python
from __future__ import print_function
import pymysql
import argparse
import os
import re
from time import gmtime, strftime
import logging
import numpy as np
import pandas as pd

currenttime = strftime("%Y-%m-%d %H:%M:%S", gmtime())
calibration_params = ["nominal_reading",
                      "l_response_actuator1", "w_response_actuator1",
                      "l_response_actuator2", "w_response_actuator2",
                      "l_response_actuator3", "w_response_actuator3",
                      "l_response_actuator4", "w_response_actuator4",
                      "l_response_actuator5", "w_response_actuator5",
                      "l_response_actuator6", "w_response_actuator6",
                      "plate_scale"]
target_params = "nominal_reading"

list_of_sql_commands = []


def setup_logging(logfile='file.log', debug=False):
    global logger
    # Create a custom logger
    logger = logging.getLogger(__name__)

    # Create handlers
    c_handler = logging.StreamHandler()
    f_handler = logging.FileHandler(logfile)

    if debug:
        c_handler.setLevel(logging.DEBUG)
    else:
        c_handler.setLevel(logging.INFO)
    f_handler.setLevel(logging.DEBUG)

    # Create formatters and add it to handlers
    c_format = logging.Formatter('%(levelname)s - %(message)s')
    f_format = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    c_handler.setFormatter(c_format)
    f_handler.setFormatter(f_format)

    # Add handlers to the logger
    logger.addHandler(c_handler)
    logger.addHandler(f_handler)
    logger.setLevel(logging.DEBUG)

    return logger


def read_mpes_file(filepath):
    file = open(filepath, 'r')
    lines = file.readlines()

    in_mirror_chunk = False
    recording_mpes_data = False
    mpes = 0
    mpes_data = []
    all_mpes = {}
    for line in lines:
        # get mirror stuff to skip
        if line.find("Az:") >= 0:
            continue
        if line.find("Mirror: ") >= 0:
            in_mirror_chunk = True
            continue
        if line.find("****") >= 0:
            in_mirror_chunk = False
            continue
        if in_mirror_chunk:
            continue

        if line.find("MPES_") >= 0:
            mpes_data = []
            mpes_str = line[line.find("MPES_"):line.find("MPES_") + 10]
            mpes = mpes_str.split("_")[1]
            recording_mpes_data = True
            continue

        if recording_mpes_data:
            value = line.split('\n')[0]
            # if float(value) < 0:
            #     mpes_data.append(np.nan)
            # else:
            #     mpes_data.append(float(value))
            mpes_data.append(float(value))
            all_mpes[mpes] = mpes_data
        else:
            continue

    all_mpes.items()
    return all_mpes


def query_get_sensor_calibration(old_sensor_serial, coord):
    execute_part_str = "select {} from ".format(", ".join(calibration_params))
    tablename = "Opt_MPESConfigurationAndCalibration "
    identifier = "where serial_number={} and coord={} and end_date is NULL ;".format(old_sensor_serial, coord)
    query = execute_part_str + tablename + identifier

    return query


def query_get_sensor_targets(sensor_serial, coord):
    execute_part_str = "select {} from ".format(target_params)
    tablename = "Opt_MPESConfigurationAndCalibration "
    identifier = "where serial_number={} and coord={} and end_date is NULL ;".format(sensor_serial, coord)
    query = execute_part_str + tablename + identifier

    return query


def query_get_all_sensor_targets(coord):
    execute_part_str = "select serial_number, {} from ".format(target_params)
    tablename = "Opt_MPESConfigurationAndCalibration "
    identifier = "where coord={} and end_date is NULL ;".format(coord)
    query = execute_part_str + tablename + identifier

    return query


def query_add_new_sensor_to_mapping(new_mpes_serial, old_mpes_serial, w_panel, l_panel, position, port):
    tablename = "Opt_MPESMapping"
    execute_part_str = "INSERT INTO {table} ".format(table=tablename)
    id_names = "(start_date, end_date, serial_number, w_panel, l_panel, w_position, port, comment) "
    start_date = currenttime
    new_info = "VALUES (\"{start_date}\", {end_date}, {serial_number}, {w_panel}, {l_panel}, " \
               "{w_position}, {port}, \'{comment}\' ) ;".format(start_date=start_date, end_date="NULL",
                                                                serial_number=new_mpes_serial,
                                                                w_panel=w_panel, l_panel=l_panel, w_position=position,
                                                                port=port,
                                                                comment="New sensor added to replace {}, will "
                                                                        "need calibration".format(old_mpes_serial))
    query = execute_part_str + id_names + new_info

    global list_of_sql_commands
    list_of_sql_commands.append(query)
    return query


def query_disable_old_sensor(old_sensor, table, comment):
    execute_part_str = "UPDATE "
    end_date = currenttime
    set_part = " SET end_date = \"{}\" ".format(end_date)
    if comment is None:
        if table == "Opt_MPESMapping":
            set_part = " SET end_date = \"{}\"".format(end_date)
    else:
        if table == "Opt_MPESMapping":
            set_part = " SET end_date = \"{}\", comment = {} ".format(end_date, comment)
        elif table == "Opt_MPESConfigurationAndCalibration":
            set_part = " SET end_date = \"{}\" ".format(end_date)
    identifier = " WHERE serial_number={} and start_date != \"{}\";".format(old_sensor, end_date)
    query = execute_part_str + table + set_part + identifier

    global list_of_sql_commands
    list_of_sql_commands.append(query)
    return query


def query_add_new_sensor_to_config(calib_data, new_mpes_serial, coord, targets=None):
    tablename = "Opt_MPESConfigurationAndCalibration"

    execute_part_str = "INSERT INTO {table} ".format(table=tablename)
    start_date = currenttime
    id_params = "start_date, end_date, serial_number, coord, "
    data_str = " "

    if targets is not None:
        if coord == "\"x\"":
            data_str = data_str + str(targets[0]) + ", "
        elif coord == "\"y\"":
            data_str = data_str + str(targets[1]) + ", "
        for i in range(1, len(calib_data) - 1):
            data_str = data_str + str(calib_data[i]) + ", "
    else:
        for i in range(0, len(calib_data) - 1):
            data_str = data_str + str(calib_data[i]) + ", "
    data_str = data_str + str(calib_data[-1])

    new_info = "VALUES (\"{start_date}\", {end_date}, {serial_number}, {coord}, {data}) ;".format(start_date=start_date,
                                                                                                  end_date="NULL",
                                                                                                  serial_number=new_mpes_serial,
                                                                                                  data=data_str,
                                                                                                  coord=coord)

    query = execute_part_str + " (" + id_params + ", ".join(calibration_params) + ") " + new_info
    global list_of_sql_commands
    list_of_sql_commands.append(query)

    return query


class pSCTDB:
    def __init__(self, host='romulus.ucsc.edu'):
        self.cur = None
        self.conn = None
        self.DB_HOST = host
        # print(self.DB_HOST)
        self.DB_USER = os.getenv('MYSQL_USER')
        self.DB_PASSWD = os.getenv('MYSQL_PASSWORD')
        self.DB_ONLINE = os.getenv('MYSQL_DATABASE')
        self.DB_OFFLINE = 'CTAOnline'
        self.DB_PORT = int(os.getenv('MYSQL_PORT'))

    def connect(self):
        try:
            self.conn = pymysql.connect(host=self.DB_HOST,
                                        user=self.DB_USER,
                                        passwd=self.DB_PASSWD,
                                        db=self.DB_ONLINE,
                                        port=self.DB_PORT)
            self.cur = self.conn.cursor()
        except (pymysql.Error, pymysql.Warning) as e:
            logger.error(e)
            logger.error("Cannot connect to {}, consider changing this".format(self.DB_HOST))
            exit()

    def close_connect(self, doCommit):
        if doCommit:
            self.conn.commit()
        self.cur.close()
        self.conn.close()


def disable_old_sensor(database, old_sensor, table, comment):
    query_disable_mapping = query_disable_old_sensor(old_sensor, table, comment)
    database.cur.execute(query_disable_mapping)


def add_new_sensor(database, new_mpes_serial, old_mpes_serial, panel, l_panel, position, port, targets=None):
    query_add_mapping = query_add_new_sensor_to_mapping(new_mpes_serial, old_mpes_serial, panel, l_panel, position,
                                                        port)
    database.cur.execute(query_add_mapping)

    coords = ["\"x\"", "\"y\""]
    for c in coords:
        calib_data = collect_calibration_data(database, old_mpes_serial, c)
        if len(calib_data) == 0:
            logger.error('Calibration data for {old_mpes_serial} missing, value set to -1'.format(
                old_mpes_serial=old_mpes_serial))
            calib_data = [-1] * len(calibration_params)
        query_add_config = query_add_new_sensor_to_config(calib_data, new_mpes_serial, c, targets)
        database.cur.execute(query_add_config)


def collect_calibration_data(database, old_sensor_serial, coord):
    query_calib_data = query_get_sensor_calibration(old_sensor_serial, coord)
    database.cur.execute(query_calib_data)
    rows = database.cur.fetchall()
    if len(rows) > 1:
        logger.error("Too many possible candidates, something must be wrong with this data.")
    elif len(rows) == 0:
        logger.error('No data found for this sensor. Either sensor {old_sensor_serial} is missing from original '
                     'Opt_MPESConfigurationAndCalibration table or end_date is expired.'.format(old_sensor_serial=old_sensor_serial))
        # exit()
        return rows
    else:
        return rows[0]


def collect_sensor_targets(database, sensor_serial, coord):
    query_targets = query_get_sensor_targets(sensor_serial, coord)
    database.cur.execute(query_targets)
    rows = database.cur.fetchall()
    if len(rows) > 1:
        logger.error("Too many possible candidates, something must be wrong with this data.")
    elif len(rows) == 0:
        logger.error('No data found. Either sensor {sensor_serial} is missing from '
                     'Opt_MPESConfigurationAndCalibration table or end_date is expired.'.format(sensor_serial=sensor_serial))
        return rows
    else:
        return rows[0]


def collect_all_sensor_targets(database, coord):
    query_targets = query_get_all_sensor_targets(coord)
    database.cur.execute(query_targets)
    rows = np.array(database.cur.fetchall())
    return rows


def update_sensor_list(database, mpes_list_path):
    logger.info('Updating sensor targets with list in {mpes_list_path}'.format(mpes_list_path=mpes_list_path))
    mpes_list = read_mpes_file(mpes_list_path)
    for mpes, targets in mpes_list.items():
        update_sensor_target(database, mpes, targets)


def update_sensor_target(database, sensor_serial, targets):
    logger.debug('Updating targets for {sensor_serial}'.format(sensor_serial=sensor_serial))
    logger.debug('\tTarget values ({targets})'.format(targets=targets))

    coords = ["\"x\"", "\"y\""]
    old_targets = []
    for c in coords:
        old_targets.append(collect_sensor_targets(database, sensor_serial, c))
    if len(old_targets) == 0:
        logger.error(
            'Sensor targets for {sensor_serial} is missing, values set to -1'.format(sensor_serial=sensor_serial))
        old_targets = [-1] * len(old_targets)
    logger.debug('Old targets: {old_targets}'.format(old_targets=old_targets))

    coords = ["\"x\"", "\"y\""]
    for c in coords:
        calib_data = collect_calibration_data(database, sensor_serial, c)
        if len(calib_data) == 0:
            logger.error(
                'Calibration data for {sensor_serial} is missing, value set to -1'.format(sensor_serial=sensor_serial))
            calib_data = [-1] * len(calibration_params)
        query_add_config = query_add_new_sensor_to_config(calib_data, sensor_serial, c, targets)
        database.cur.execute(query_add_config)

    tablename = "Opt_MPESConfigurationAndCalibration"
    comment = None
    disable_old_sensor(database, sensor_serial, tablename, comment)


def replace_MPES():
    add_new_sensor(db, args.newMPES, args.oldMPES, args.w_panel, args.l_panel, args.position,
                   args.port, args.target)
    tablename = "Opt_MPESMapping"
    comment = "\'Replaced this sensor with {} \'".format(args.newMPES)
    disable_old_sensor(db, args.oldMPES, tablename, comment)

    tablename = "Opt_MPESConfigurationAndCalibration"
    comment = ""
    disable_old_sensor(db, args.oldMPES, tablename, comment)

    print_replace_summary(args.newMPES, args.oldMPES, args.w_panel, args.l_panel, args.position, args.port, args.target)


def get_all_targets(database):
    logger.info('Collecting all targets for all active sensors')

    coords = ["x", "y"]
    coords_dict = {}
    for c in coords:
        targets = collect_all_sensor_targets(database, "\"" + c + "\"")
        coords_dict["sensor_" + c] = targets[:, 0]
        coords_dict[c] = targets[:, 1]

    is_aligned = np.array_equal(coords_dict['sensor_x'], coords_dict['sensor_y'])

    if not is_aligned:
        logger.error("Sensor list is not equal so a sensor must be missing a target coordinate.")
        df = pd.DataFrame.from_dict(coords_dict)
        df.to_csv(args.csvfile, index=False)
    else:
        df = pd.DataFrame.from_dict(coords_dict)
        df.pop("sensor_y")
        df.rename(columns={"sensor_x": "sensor"}, inplace=True)
        df.sensor = df.sensor.astype(int)
        df.to_csv(args.csvfile, index=False)


def print_replace_summary(newMPES, oldMPES, w_panel, l_panel, position, port, target):
    logger.info(
        "\n\tNew MPES: {}\n\tOld MPES: {}\n\tw_panel: {}\n\tl_panel: {}\n\tposition: {}\n\tport: {}".format(
            newMPES, oldMPES, w_panel, l_panel, position, port))
    logger.info("Target coords: {}".format(target))
    logger.info("\n")
    logger.info("Inserting {} into Opt_MPESMapping".format(newMPES))
    logger.info("Inserting {} into Opt_MPESConfigurationAndCalibration, using {} calibration constants".format(newMPES,
                                                                                                               oldMPES))

    logger.warning("##############################################################")
    logger.warning(
        "!!! If the MPES was not replaced, DO NOT update the mapping table, "
        "\n\ti.e., only copy and paste line 2, 3, and 5 "
        "\n\twhich update Opt_MPESConfigurationAndCalibration!!!")
    logger.warning("##############################################################")
    logger.warning("")


def parsing():
    global args
    parser = argparse.ArgumentParser(description='Update MPES properties in database')
    # example -h 172.18.0.2 10005 10337 1322 1311 2 2

    sub_parsers = parser.add_subparsers(dest='subparser_name')

    # Replace a single sensor
    replace_parser = sub_parsers.add_parser('replace', help='Replace MPES single calibration data')
    replace_parser.add_argument('newMPES', metavar="new", type=int, help='New MPES')
    replace_parser.add_argument('oldMPES', metavar="old", type=int, help='Old MPES')
    replace_parser.add_argument('w_panel', help='Panel position of webcam side')
    replace_parser.add_argument('l_panel', help='Panel position of laser side')
    replace_parser.add_argument('position', help='Position of webcam within panel')
    replace_parser.add_argument('port', help='Webcam USB port number')
    replace_parser.add_argument('-T', '--targetCentroid', dest="target", nargs=2, type=float,
                                help="Target centroid values for newly calibrated sensors, done in situ.")

    # Update target for a single sensor
    update_mpes_target_parser = sub_parsers.add_parser('update_mpes_target',
                                                       help='Update calibration targets for a single MPES')
    update_mpes_target_parser.add_argument('mpes', type=int, help='MPES serial number.')
    update_mpes_target_parser.add_argument('-T', '--targetCentroid', dest="newTargets", nargs=2, type=float,
                                           help="Target centroid values for newly calibrated sensors, done in situ.")

    # Update targets for a list of sensors
    update_mpes_list_parser = sub_parsers.add_parser('update_list',
                                                     help='Update calibration targets for a list of MPES')
    update_mpes_list_parser.add_argument('mpes_list_path', type=str,
                                         help='Path to MPES file generated by p2pas_client')
    update_mpes_list_parser.add_argument("--csv", dest="csvfile", help="CSV file to save dump of all targets",
                                         default="all_mpes_targets.csv")

    # Collect targets for all active sensors
    dump_mpes_target_parser = sub_parsers.add_parser('dump',
                                                     help='Print targets for all active MPES')
    dump_mpes_target_parser.add_argument("--csv", dest="csvfile", help="CSV file to save dump of all targets",
                                         default="all_mpes_targets.csv")

    # General arguments
    parser.add_argument('-v', dest="debug", action='store_true')
    parser.add_argument('-H', '--host', default=os.getenv('MYSQL_HOST'), help="Host for DB", type=str)
    parser.add_argument('-l', '--log', dest="log", default='MPES_db_update.log',
                        type=str, help='Path to logfile')
    parser.add_argument('-o', '--output', dest="out", default="MPES_updates_commands.sql",
                        type=str, help='Path to output SQL file')
    args = parser.parse_args()

    return args


if __name__ == '__main__':
    args = parsing()

    setup_logging(args.log, args.debug)
    fout = open(args.out, 'w')
    fout.close()

    db = pSCTDB(args.host)
    db.connect()

    needConfirm = True
    if args.subparser_name == 'replace':
        replace_MPES()
    elif args.subparser_name == 'update_mpes_target':
        update_sensor_target(db, args.mpes, args.newTargets)
    elif args.subparser_name == 'update_list':
        get_all_targets(db)
        update_sensor_list(db, args.mpes_list_path)
    elif args.subparser_name == 'dump':
        get_all_targets(db)
        needConfirm = False

    if needConfirm:
        for sql in list_of_sql_commands:
            print(sql, file=open(args.out, 'a+'))
        doCommit = bool(int(input("Confirm to edit DB? 1 for yes, 0 for no: ")))
        db.close_connect(doCommit)
    else:
        db.close_connect(False)

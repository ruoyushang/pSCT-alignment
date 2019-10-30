
import os
import sys
import pymysql
import argparse

import numpy as np
try:
    from matplotlib import pyplot as plt
    plot=True
except:
    plot=False
    print("Cannot import matplotlib, skip plotting")

import socket

pix2mm = 1.0 / 19.75

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

    
    def query_mpes(self, mpes_id, verbose=True):
        self.connect()
        q = "select * from Opt_MPESMapping where serial_number = {} and end_date is NULL".format(mpes_id)
        nentries=self.cur.execute(q)
        for i, row in enumerate(self.cur):
            #print("MPES {}".format(mpes_id))
            #print("w_panel {} l_panel {}".format(row[3], row[4]))
            #print("w_position {} port {}".format(row[5], row[6])) 
            return "%s"%(row[3]), "%s"%(row[4]), "%s"%(row[5]), "%s"%(row[6])


def read_misalignment(inputfilename="logs/p2pasclient_log_20190703184623.txt"):
    #KNOWN_BAD_SENSORS = [10332, 10358, 10152, 10146]  # permanent dead

    outputfile = open(inputfilename, "r")

    misalignment = []
    sensor_serial = []
    sensor_misalign_x = []
    sensor_misalign_y = []

    line_counts = -1
    for line in outputfile:
        #print(line)
        line = line.strip()
        if "MPES" in line:
            line_counts = 0
            serial_ = line.split(" ")[1].strip("(").strip(",")
            sensor_serial += [serial_]
        else:
            if line_counts>=0:
                line_counts += 1
                if line_counts==1: sensor_misalign_x += [float(line.strip())]
                if line_counts==2: sensor_misalign_y += [float(line.strip())]
            else: continue

    misalignment = [sensor_serial, sensor_misalign_x, sensor_misalign_y]

    return misalignment

def diff_misalignment(misalignment_1, misalignment_2):

    misalignment = []
    sensor_serial = []
    sensor_misalign_x = []
    sensor_misalign_y = []

    for sensor1 in range(0,len(misalignment_1[0])):
        for sensor2 in range(0,len(misalignment_2[0])):
            if misalignment_1[0][sensor1]==misalignment_2[0][sensor2]: # serial numbers must match
                sensor_serial += [misalignment_1[0][sensor1]]
                sensor_misalign_x += [misalignment_1[1][sensor1]-misalignment_2[1][sensor2]]
                sensor_misalign_y += [misalignment_1[2][sensor1]-misalignment_2[2][sensor2]]

    misalignment = [sensor_serial, sensor_misalign_x, sensor_misalign_y]

    return misalignment

def get_misalignment_chi2(misalignment):

    chi2 = 0.
    for sensor in range(0,len(misalignment[0])):
        chi2 += pow(misalignment[1][sensor],2)
        chi2 += pow(misalignment[2][sensor],2)
    return chi2

def FindPanelPosition(panel_id):
    radius = 0.
    phase = 0.
    is_primary = True
    is_inner_ring = True
    if list(panel_id)[0] == "1":
        is_primary = True
    else:
        is_primary = False
    if list(panel_id)[2] == "1":
        is_inner_ring = True
    else:
        is_inner_ring = False
    quadrant = float(list(panel_id)[1])
    segment = float(list(panel_id)[3])
    total_segment = 0.
    if is_primary and is_inner_ring:
        total_segment = 4.
    elif is_primary and not is_inner_ring:
        total_segment = 8.
    elif not is_primary and is_inner_ring:
        total_segment = 2.
    elif not is_primary and not is_inner_ring:
        total_segment = 4.
    phase = (quadrant - 1) * 0.5 * np.pi + 0.5 * np.pi * (segment - 0.5) / total_segment
    phase_width = 0.5 * np.pi * (1.0) / total_segment
    if is_inner_ring:
        radius = 1.5
    else:
        radius = 3.0
    return radius, phase, phase_width


def FindEdgeSensorPosition(mpes_id):

    testDB = pSCTDB_readonly()
    w_panel, l_panel, position, port = testDB.query_mpes(mpes_id)

    w_radius, w_phase, w_phase_width = FindPanelPosition(w_panel)
    if list(w_panel)[0] == "1":  # primary
        if list(w_panel)[2] == "1":  # inner
            if position == "3":
                return w_radius + 0.75 , w_phase - 0.375*w_phase_width
            if position == "4":
                return w_radius + 0.75 , w_phase + 0.125*w_phase_width
            if position == "2":
                return w_radius + 0.5 , w_phase - 0.5*w_phase_width
            if position == "1":
                return w_radius - 0.5 , w_phase - 0.5*w_phase_width
            if position == "5":
                return w_radius + 0.0 , w_phase + 0.5*w_phase_width
        if list(w_panel)[2] == "2":  # outer
            if position == "2":
                return w_radius - 0.75 , w_phase + 0.25*w_phase_width
            if position == "3":
                return w_radius - 0.5 , w_phase - 0.5*w_phase_width
            if position == "4":
                return w_radius + 0.5 , w_phase - 0.5*w_phase_width
            if position == "1":
                return w_radius + 0.0 , w_phase + 0.5*w_phase_width
    if list(w_panel)[0] == "2":  # secondary
        if list(w_panel)[2] == "1":  # inner
            if position == "4":
                return w_radius + 0.75 , w_phase - 0.125*w_phase_width
            if position == "3":
                return w_radius + 0.75 , w_phase + 0.375*w_phase_width
            if position == "2":
                return w_radius + 0.5 , w_phase + 0.5*w_phase_width
            if position == "1":
                return w_radius - 0.5 , w_phase + 0.5*w_phase_width
            if position == "5":
                return w_radius + 0.0 , w_phase - 0.5*w_phase_width
        if list(w_panel)[2] == "2":  # outer
            if position == "2":
                return w_radius - 0.75 , w_phase - 0.25*w_phase_width
            if position == "4":
                return w_radius + 0.5 , w_phase + 0.5*w_phase_width
            if position == "3":
                return w_radius - 0.5 , w_phase + 0.5*w_phase_width
            if position == "1":
                return w_radius + 0.0 , w_phase - 0.5*w_phase_width

    print 'MPES %s is missing'%(mpes_id)
    print 'w_panel %s, position %s'%(w_panel,position)
    return 0, 0

def PlotMisalignment(misalignment, zmax=1.0, outfilename="misalignment.png"):

    all_misalignments = {}
    misalignment_array = np.empty([])
    for sensor in range(0,len(misalignment[0])):
        mpes_id = misalignment[0][sensor]
        misalign = 0.
        misalign += pow(misalignment[1][sensor],2)
        misalign += pow(misalignment[2][sensor],2)
        misalign = pix2mm*pow(misalign,0.5)
        print '%s, %s'%(mpes_id,misalign)
        misalignment_array = np.append(misalignment_array, misalign)
        radius, phase = FindEdgeSensorPosition(mpes_id)
        coor_x = radius * np.cos(phase)
        coor_y = radius * np.sin(phase)
        all_misalignments.update({mpes_id: {"misalignment": misalign, "coor_x": coor_x, "coor_y": coor_y}})

    fig, ax = plt.subplots(figsize=(6, 5), ncols=1)
    if len(all_misalignments)>104:
        #primary
        s=15
    else:
        s=20
    try:
        for sensor in all_misalignments:
            marker = 'o'
            if all_misalignments[sensor]["misalignment"] <= 0:
                marker = '+'
            if all_misalignments[sensor]["misalignment"] > zmax:
                edge = 'red'
            else:
                edge = 'face'
            if zmax is None:
                zmax=3
            scatter = ax.scatter(all_misalignments[sensor]["coor_x"], all_misalignments[sensor]["coor_y"],
                                 label=str(sensor), c=all_misalignments[sensor]["misalignment"], cmap='viridis',
                                 marker=marker, edgecolors=edge, vmin=0, vmax=zmax, s=s)
        cb = fig.colorbar(scatter, ax=ax)
        cb.set_label("Misalignment [mm]")
        ax.axis('off')
        ax.set_yticklabels([])
        ax.set_xticklabels([])
        #plt.tightlayout()
        plt.savefig(outfilename)

        plt.figure(figsize=(8, 3))
        plt.hist(misalignment_array, bins=20, range=[0, 2])
        plt.xlabel("Misalignment [mm]")
        plt.tight_layout()
        plt.savefig(outfilename.split('.')[0]+"_hist.png")

    except UnboundLocalError:
        print("No MPES found")

    print '%s is created.'%(outfilename.split('.')[0]+"_hist.png")
    print '%s is created.'%(outfilename)
    return ax


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Analyze MPES misalignments file to plot misalignment and histogram.")

    output_folder = "alignment_plots"

    if len(sys.argv) == 2:
        outputfilename = sys.argv[1]
        plot_name = outputfilename.split('/')[len(outputfilename.split('/'))-1].strip('.png')
        misalignment = read_misalignment(outputfilename)
        PlotMisalignment(misalignment, 1., "%s/misalignment_%s.png"%(output_folder,plot_name))
    if len(sys.argv) == 3:
        outputfilename_1 = sys.argv[1]
        plot_name_1 = outputfilename_1.split('/')[len(outputfilename_1.split('/'))-1].strip('.png')
        misalignment_1 = read_misalignment(outputfilename_1)
        print 'file1 chi2 = %s'%(get_misalignment_chi2(misalignment_1))
        outputfilename_2 = sys.argv[2]
        plot_name_2 = outputfilename_2.split('/')[len(outputfilename_2.split('/'))-1].strip('.png')
        misalignment_2 = read_misalignment(outputfilename_2)
        print 'file2 chi2 = %s'%(get_misalignment_chi2(misalignment_2))
        misalignment = diff_misalignment(misalignment_1,misalignment_2)
        print 'diff chi2 = %s'%(get_misalignment_chi2(misalignment))
        PlotMisalignment(misalignment, 0.75, "%s/diff_%s_%s.png"%(output_folder,plot_name_1,plot_name_2))


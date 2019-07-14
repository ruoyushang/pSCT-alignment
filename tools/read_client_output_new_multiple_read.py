#! /usr/bin/env python2
import os
import sys

import numpy as np
try:
    from matplotlib import pyplot as plt
    plot=True
except:
    plot=False
    print("Cannot import matplotlib, skip plotting")

import socket

this_home = socket.gethostname()
if this_home == "alignment.psct":
        this_db =  "172.17.10.10"
else:
        this_db = 'romulus.ucsc.edu'


try:
    import pymysql

    query_= "select serial_number from Opt_MPESMapping where w_position=2 and (w_panel like \"%22\" or  w_panel like \"%24\") ; "
    testDB = pSCTDB_readonly(this_db)
    testDB.connect()
    nentries = testDB.cur.execute(query_)
    mpeses_port6_airquote = []
    for row in testDB.cur:
        mpeses_port6_airquote.append(row[0])

    testDB.close_connect()
    print("Query")
    print(mpeses_port6_airquote)
except:
    print("Failed to connect to DB {}".format(this_db))
    mpeses_port6_airquote = np.array([10043,10050,10192,10183,10074,10032,10337,10247,10124,10142,10167,10085,
                                      10164,10342,10225,10113,10323,10333], dtype=int)

print(mpeses_port6_airquote)

pix2mm = 1.0 / 19.75
KNOWN_BAD_SENSORS = []

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
    if is_inner_ring:
        radius = 1.5
    else:
        radius = 3.0
    return radius, phase


def FindEdgeSensorPosition(edge_id, port):
    is_long_edge = False
    n_expected_mpes = len(edge_id.split("+")) + 1
    if list(edge_id.split("+")[0])[2] == "1" and list(edge_id.split("+")[1])[2] == "2":
        if n_expected_mpes == 4:
            is_long_edge = True
        else:
            print "Not long edge but should be."
            print(edge_id)
            p_id=int((edge_id.split("+")[0]))%100
            s_id=int((edge_id.split("+")[1]))%100
            if p_id*2==s_id:
                edge_id=edge_id+'+'+str(int(edge_id.split("+")[1])-1)
            elif p_id*2==s_id+1:
                edge_id=edge_id.split("+")[0]+'+'+str(int(edge_id.split("+")[1])+1)+'+'+edge_id.split("+")[1]
            print("recover edge_id {}".format(edge_id))
            is_long_edge = True
            #return 0, 0
    if is_long_edge:
        panel_id_0 = edge_id.split("+")[0]
        panel_id_1 = edge_id.split("+")[1]
        panel_id_2 = edge_id.split("+")[2]
        radius_0, phase_0 = FindPanelPosition(panel_id_0)
        radius_1, phase_1 = FindPanelPosition(panel_id_1)
        radius_2, phase_2 = FindPanelPosition(panel_id_2)
        final_phase = phase_0
        delta_phase = 0.3 * (phase_2 - phase_1)
        final_radius = 1. / 3. * (radius_0 + radius_1 + radius_2)
        if delta_phase >= np.pi: final_phase = 0
        if list(panel_id_0)[0] == "1":  # primary
            if port == "3":
                return final_radius, final_phase - 2. * delta_phase
            if port == "2":
                return final_radius, final_phase - 1. * delta_phase
            if port == "4":
                return final_radius, final_phase + 1. * delta_phase
            if port == "6":
                return final_radius, final_phase + 2. * delta_phase
        else:
            if port == "2":
                return final_radius, final_phase - 2. * delta_phase
            if port == "4":
                return final_radius, final_phase - 1. * delta_phase
            if port == "6":
                return final_radius, final_phase + 1. * delta_phase
            if port == "3":
                return final_radius, final_phase + 2. * delta_phase
    else:
        panel_id_0 = edge_id.split("+")[0]
        panel_id_1 = edge_id.split("+")[1]
        radius_0, phase_0 = FindPanelPosition(panel_id_0)
        radius_1, phase_1 = FindPanelPosition(panel_id_1)
        delta_phase = phase_1 - phase_0
        final_phase = 0.5 * delta_phase + phase_0
        if delta_phase >= np.pi: final_phase = 0
        if list(panel_id_0)[0] == "1":  # primary
            if list(panel_id_0)[2] == "2":  # outer
                if port == "1":
                    return 0.5 * (radius_0 + radius_1), final_phase
                if port == "3":
                    return 0.5 * (radius_0 + radius_1) - 0.25, final_phase
                if port == "4":
                    return 0.5 * (radius_0 + radius_1) + 0.25, final_phase
            else:
                if port == "5":
                    return 0.5 * (radius_0 + radius_1), final_phase
                if port == "1":
                    return 0.5 * (radius_0 + radius_1) - 0.25, final_phase
                if port == "2":
                    return 0.5 * (radius_0 + radius_1) + 0.25, final_phase
        else:
            if list(panel_id_0)[2] == "2":  # outer
                if port == "1":
                    return 0.5 * (radius_0 + radius_1), final_phase
                if port == "3":
                    return 0.5 * (radius_0 + radius_1) - 0.25, final_phase
                if port == "4":
                    return 0.5 * (radius_0 + radius_1) + 0.25, final_phase
            else:
                if port == "5":
                    return 0.5 * (radius_0 + radius_1), final_phase
                if port == "1":
                    return 0.5 * (radius_0 + radius_1) - 0.25, final_phase
                if port == "2":
                    return 0.5 * (radius_0 + radius_1) + 0.25, final_phase

    print 'Could not find sensor on edge %s port %s' % (edge_id, port)
    return 0, 0


def read_misalignment(inputfilename="logs/p2pasclient_log_20190703184623.txt", out_text="mpes_readings_new.txt"):
    #KNOWN_BAD_SENSORS = [10332, 10358, 10152, 10146]  # permanent dead

    outputfile = open(inputfilename, "r")

    num_read = 0 #keep track of the number of reads
    all_misalignment_list = []
    misalignment_array_list = []

    all_misalignment = {}
    gt1mm = 0
    gt2mm = 0
    gt3mm = 0

    which_sensor = 0
    reading_x = True
    is_an_edge_block = False
    max_misalign = 0.
    n_expected_mpes = 0
    n_detected_mpes = 0
    edge_id = ""
    sensor_ports = []
    sensor_serial = []
    sensor_misalign = []
    sensor_FoV = []

    misalignment_array=[]
    x_array=[]
    dx_array=[]
    y_array=[]
    dy_array=[]

    empty_sensor=True

    for line in outputfile:
        #print(line)
        line = line.strip()
        if len(line.split(" ")) < 4:
            continue
        if "readSensorsParallel" in line:
            if num_read > 0: #not first read
                misalignment_array = np.array(misalignment_array)
                print("========================================================")
                print("==== Summary of the {}th parallel read: ====".format(num_read))
                print("{num} > 1mm misalignment".format(num=gt1mm))
                print("{num} > 2mm misalignment".format(num=gt2mm))
                print("{num} > 3mm misalignment".format(num=gt3mm))
                print("Total number of MPESs read attempted: {}".format(misalignment_array.shape[0]))
                print("Total number of MPESs read successfully: {}".format(
                    misalignment_array[misalignment_array > 0].shape[0]))
                print("Excluding non-readable MPES from calculations below")
                print("Mean {} std {}".format(np.mean(misalignment_array[misalignment_array > 0]),
                                              np.std(misalignment_array[misalignment_array > 0])))
                total_chi2mm = np.sum(misalignment_array[misalignment_array > 0] ** 2)
                print("Total misalignment chi^2 = {} mm^2 for {} MPESs.".format(total_chi2mm, misalignment_array[
                    misalignment_array > 0].shape[0]))
                print("========================================================")

                #return all_misalignment, misalignment_array
                all_misalignment_list.append(all_misalignment)
                misalignment_array_list.append((misalignment_array))
                if plot:
                    plotfile = "alignment_plot_" + outputfilename.split('.')[0].split('_')[-1] + "reading_"+ str(num_read)+ ".png"
                    plot_misalignments(all_misalignment, comparison, zmax=1, outfilename=plotfile)
                    print("Saving plot to {}".format(plotfile))
                    plotfile2 = "alignment_plot_" + outputfilename.split('.')[0].split('_')[-1] + "reading_" + str(
                        num_read) + "_hist.png"
                    plt.figure(figsize=(8,3))
                    plt.hist(misalignment_array, bins=30)
                    plt.xlabel("Misalignment [mm]")
                    plt.tight_layout()
                    print("Saving histogram plot to {}".format(plotfile2))
                    plt.savefig(plotfile2)

            num_read += 1

            #initialize for next read
            all_misalignment = {}
            gt1mm = 0
            gt2mm = 0
            gt3mm = 0

            which_sensor = 0
            reading_x = True
            is_an_edge_block = False
            max_misalign = 0.
            n_expected_mpes = 0
            n_detected_mpes = 0
            edge_id = ""
            sensor_ports = []
            sensor_serial = []
            sensor_misalign = []
            sensor_FoV = []

            misalignment_array = []
            x_array = []
            dx_array = []
            y_array = []
            dy_array = []

        #if re.match("(.*)Readings(.*)", line): # and "Edge" in line.split(" ")[-2]:
        if "Readings" in line:
        #if "Readings" in line and "Edge" in line.split(" ")[-2]:

            print("========================================================")
            which_sensor = 0
            is_an_edge_block = True
            edge_id = line.split(", ")[-3]
            print(line)
            print(edge_id)
            sensor_ports = []
            sensor_serial = []
            sensor_misalign = []
            sensor_FoV = []
            n_expected_mpes = len(edge_id.split("+")) + 1
            n_detected_mpes = 0

            empty_sensor = False

        elif is_an_edge_block == True:
            if "MPES_" in line.split(" ")[2]:
                serial_ = line.split(", ")[0].strip("(")
                sensor_serial += [serial_]
                if int(serial_) in mpeses_port6_airquote:
                    # assert([line.split(", ")[-1].split(")")[0]] == '2')
                    print("Changing MPES {} port to 6 just for plotting".format(serial_))
                    sensor_ports.append("6")  # to avoid two port-2 sensors
                else:
                    sensor_ports += [line.split(", ")[-1].split(")")[0]]
                reading_x = True
                if empty_sensor:
                    which_sensor += 1
                empty_sensor = True

                continue
            elif "Current" in line.split(" ")[0]:
                continue
            elif "+/-" in line and reading_x:
                # these line look like  171.061 +/- 0.0316871 [155.7] (15.3605)
                # colums: Current position +/- Spot width [Aligned position] (Misalignment)
                #print("x line")
                #print(line)
                # print(line.split(" ")[0])
                x_val = float(line.split(" ")[0])
                x_rms = float(line.split(" ")[2])
                x_target = float(line.split(" ")[3].strip('[]'))
                x_misalign = float(line.split(" ")[4].strip('()'))
                x_array.append(x_val)
                dx_array.append(x_rms)
                if x_val == -1:
                    sensor_FoV.append(False)
                else:
                    sensor_FoV.append(True)
                # sensor_misalign.append(x_val**2)
                reading_x = False
                empty_sensor = False
                continue
            elif "+/-" in line and not reading_x:
                # these line look like  171.061 +/- 0.0316871 [155.7] (15.3605)
                # colums: Current position +/- Spot width [Aligned position] (Misalignment)
                #print("y line")
                #print(line)
                y_val = float(line.split(" ")[0])
                y_rms = float(line.split(" ")[2])
                y_target = float(line.split(" ")[3].strip('[]'))
                y_misalign = float(line.split(" ")[4].strip('()'))
                y_array.append(y_val)
                dy_array.append(y_rms)
                empty_sensor = False

                # sensor_misalign[which_sensor] = np.sqrt(sensor_misalign[which_sensor]+(y_val**2))
                if y_val < 0 and sensor_FoV[which_sensor]:
                    print(x_val, y_val)
                    print("This should never happen")
                    sensor_FoV[which_sensor] = False
                    misalignment = -1
                    sensor_misalign.append(-1)
                elif y_val == -1:
                    sensor_FoV[which_sensor] = False
                    misalignment = -1
                    sensor_misalign.append(-1)
                else:
                    misalignment = pix2mm * np.sqrt(x_misalign ** 2 + y_misalign ** 2)
                    sensor_misalign.append(misalignment)
                    n_detected_mpes += 1
                misalignment_array.append(misalignment)
                reading_x = True
                max_misalign = max(max_misalign, misalignment)
                if misalignment > 1:
                    gt1mm += 1
                if misalignment > 2:
                    gt2mm += 1
                if misalignment > 3:
                    gt3mm += 1
                #print('edge %s, port %s, serial %s, misalignment = %0.2f' % (
                #    edge_id, sensor_ports[which_sensor], sensor_serial[which_sensor], misalignment))
                print('edge %s, port %s, serial %s, misalignment = %0.2f, x = %0.2f y = %0.2f' % (
                    edge_id, sensor_ports[which_sensor], sensor_serial[which_sensor], misalignment, x_val, y_val))
                # with open(out_text, 'a') as outf:
                #    outf.write("{},{}\n".format(sensor_serial[sensor], misalignment))
                if int(sensor_serial[which_sensor]) in KNOWN_BAD_SENSORS:
                    which_sensor += 1
                    continue
                radius, phase = FindEdgeSensorPosition(edge_id, sensor_ports[which_sensor])
                coor_x = radius * np.cos(phase)
                coor_y = radius * np.sin(phase)
                #print(x_val, y_val, misalignment)
                all_misalignment.update(
                    {sensor_serial[which_sensor]: {"edge": edge_id, "port": sensor_ports[which_sensor],
                                                   "misalignment": misalignment, "coor_x": coor_x,
                                                   "coor_y": coor_y}})
                which_sensor += 1

            if line == "\n":
                is_an_edge_block = False
                # is_an_alignment_block = False
                which_sensor = 0
                missed_mpes = n_expected_mpes - n_detected_mpes
                max_misalign = 0.
                if n_expected_mpes == 4:
                    for i in range(4):
                        serial_ = sensor_serial[i]
                        if int(serial_) in mpeses_port6_airquote:
                            print("Changing MPES {} port to 6 just for plotting".format(serial_))
                            sensor_ports[i] = "6"  # to avoid two port-2 sensors

    misalignment_array = np.array(misalignment_array)
    print("========================================================")
    print("==== Summary of the {}th parallel read: ====".format(num_read))
    print("{num} > 1mm misalignment".format(num=gt1mm))
    print("{num} > 2mm misalignment".format(num=gt2mm))
    print("{num} > 3mm misalignment".format(num=gt3mm))
    print("Total number of MPESs read attempted: {}".format(misalignment_array.shape[0]))
    print("Total number of MPESs read successfully: {}".format(misalignment_array[misalignment_array>0].shape[0]))
    print("Excluding non-readable MPES from calculations below")
    print("Mean {} std {}".format(np.mean(misalignment_array[misalignment_array>0]), np.std(misalignment_array[misalignment_array>0])))
    total_chi2mm = np.sum(misalignment_array[misalignment_array>0]**2)
    print("Total misalignment chi^2 = {} mm^2 for {} MPESs.".format(total_chi2mm, misalignment_array[misalignment_array>0].shape[0]))
    print("========================================================")
    # return all_misalignment, misalignment_array
    all_misalignment_list.append(all_misalignment)
    misalignment_array_list.append((misalignment_array))
    if plot:
        plotfile = "alignment_plot_" + outputfilename.split('.')[0].split('_')[-1] + "reading_" + str(num_read) + ".png"
        plot_misalignments(all_misalignment, comparison, zmax=1, outfilename=plotfile)
        print("Saving plot to {}".format(plotfile))
        plotfile2 = "alignment_plot_" + outputfilename.split('.')[0].split('_')[-1] + "reading_" + str(
            num_read) + "_hist.png"
        plt.figure(figsize=(8, 3))
        plt.hist(misalignment_array, bins=30)
        plt.xlabel("Misalignment [mm]")
        plt.tight_layout()
        print("Saving histogram plot to {}".format(plotfile2))
        plt.savefig(plotfile2)

    return all_misalignment_list, misalignment_array_list




def plot_misalignments(all_misalignments, comparison=False, zmax=None,outfilename="misalignment.png"):
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
            if all_misalignments[sensor]["misalignment"] > 1:
                edge = 'red'
            else:
                edge = 'face'
            if comparison:
                if zmax is None:
                    zmax=0.6
                scatter = ax.scatter(all_misalignments[sensor]["coor_x"], all_misalignments[sensor]["coor_y"],
                                     label=str(sensor), c=all_misalignments[sensor]["misalignment"], cmap='seismic',
                                     marker=marker, vmin=-.6, vmax=zmax, s=s)
            else:
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
    except UnboundLocalError:
        print("No MPES found")
    return ax


def compare_misalignments(misalignments_1, misalignments_2):
    difference = misalignments_1.copy()
    for sensor in misalignments_1:
        try:
            difference[sensor]['misalignment'] = misalignments_2[sensor]['misalignment'] - misalignments_1[sensor][
                'misalignment']
        except KeyError:
            difference[sensor]['misalignment'] = 0
    return difference


if __name__ == "__main__":
    # outputfile = open("/home/ctauser/logs/p2pasclient_log_201905171353.txt", "r") # primary inner
    # outputfile = open("/home/ctauser/logs/p2pasclient_log_201905171535.txt", "r") # secondary whole
    # outputfile = open("/home/ctauser/logs/p2pasclient_log_201905201028.txt", "r")  # whole primary mirror
    # outputfile = open("/home/ctauser/logs/p2pasclient_log_201905201820.txt", "r")  # secondary
    # outputfile = open("/home/ctauser/logs/p2pasclient_log_201905201933.txt", "r")  # secondary
    # outputfile = open("/home/ctauser/logs/p2pasclient_log_201905201942.txt", "r")  # whole primary mirror
    # outputfile = open("/home/ctauser/logs/p2pasclient_log_201905211358.txt", "r")  # whole primary mirror

    comparison = False
    if len(sys.argv) == 1:
        outputfilename = "/home/ctauser/logs/p2pasclient_log_201905262012.txt"
        all_misalignment, mis_arr = read_misalignment(outputfilename)
    elif len(sys.argv) == 2:
        outputfilename = sys.argv[1]
        all_misalignment, mis_arr  = read_misalignment(outputfilename)
    elif len(sys.argv) == 3:
        print("Not implemented for multiple reads yet")
        exit(0)
        log_1 = sys.argv[1]
        log_2 = sys.argv[2]
        print(log_1.split('.')[0])
        print(log_2.split('.')[0])
        print(log_2.split('.')[0].split('/')[-1])
        outputfilename = log_1.split('.')[0]+'_'+log_2.split('.')[0].split('_')[-1]+'.'
        misalignments_1, mis_arr1  = read_misalignment(log_1, out_text="/dev/null")
        misalignments_2, mis_arr2  = read_misalignment(log_2, out_text="/dev/null")
        all_misalignment = compare_misalignments(misalignments_1, misalignments_2)
        comparison = True
    else:
        print("Usage:")
        print("python " + sys.argv[0] + " your_log.txt")
        print("or")
        print("python " + sys.argv[0] + "         (hardcoded your_log.txt)")
        print("or, to compare log 1 to log 2")
        print("python " + sys.argv[0] + " log_1.txt log_2.txt")
        exit()

    # for sensor in all_misalignment:
    #     print("{sensor}: {misalignment}".format(sensor=sensor, misalignment=all_misalignment[sensor]["misalignment"]))

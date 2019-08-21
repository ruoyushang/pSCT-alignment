#! /usr/bin/env python2
import os
import sys

from datetime import datetime

import numpy as np
import pandas as pd
#import csv

pix2mm = 1.0 / 19.75
print("Please add the read divider RFP to log file")



def read_pad(inputfilename="p2pasclient_log_201907120030", out_text=None,
                      plot=True,  zmax=0.5 ):
    outputfile = open(inputfilename, "r")


    num_read = 0 #keep track of the number of reads

    n_panels = 0

    #out_file = out_text.split(".")[0]+"_"+str(num_read+1)+"."+out_text.split(".")[-1]
    #print(out_file)
    #with open(out_file, 'w') as outf:
    #    outf.write("Panel_id,pad_PRF1,pad_PRF2,pad_PRF3,pad_PRF4,pad_PRF5,pad_PRF6,pad_PRF7,pad_PRF8,pad_PRF9,"
    #               "pad_TRF1,pad_TRF2,pad_TRF3,pad_TRF4,pad_TRF5,pad_TRF6,pad_TRF7,pad_TRF8,pad_TRF9\n")
    #print("Panel_id,pad_PRF1,pad_PRF2,pad_PRF3,pad_PRF4,pad_PRF5,pad_PRF6,pad_PRF7,pad_PRF8,pad_PRF9,"
    #               "pad_TRF1,pad_TRF2,pad_TRF3,pad_TRF4,pad_TRF5,pad_TRF6,pad_TRF7,pad_TRF8,pad_TRF9\n")

    readMode = "newpad"
    is_an_data_block = False
    is_an_PRF_block = False
    is_an_TRF_block = False

    line_read = 0
    line2write = ""

    list_of_outfiles = []

    for line in outputfile:
        #if "readPositionAll" in line and not is_an_data_block:
        if "readPosition()" in line and not is_an_data_block:

            print("========================================================")
            print("============== reading in {} ===============".format(readMode))
            
            num_read+=1

            if out_text is not None:
                out_file = out_text.split(".")[0] + "_" + str(num_read) + "." + out_text.split(".")[-1]
            else:
                timetag = '_'.join('_'.join(line.split('] [')[0][1:].split()).split(':'))
                #timenow = datetime.today().strftime('%Y%m%d_%H%M')

                #out_file = "RFP_" + inputfilename.split('.')[0].split('_')[-1] + "_for_vvv_" + str(
                #num_read) + ".csv"
                out_file = "RFP_" + timetag + "_for_vvv_" + str(
                    num_read) + ".csv"

            print(out_file)
            if os.path.exists(out_file):
                yes = {'yes', 'y', 'ye', ''}
                no = {'no', 'n'}

                print("We found a file named {}, are you sure we can overwrite it???? ".format(out_file))
                choice = raw_input().lower()
                if choice in yes:
                    print("good")
                elif choice in no:
                    print("Please rename your file or provide a second argument to this script, exiting... ")
                    exit(0)
                else:
                    sys.stdout.write("Please respond with 'y' or 'n'")

            list_of_outfiles.append(out_file)

            with open(out_file, 'w') as outf:
                outf.write("pad_PRF1,pad_PRF4,pad_PRF7,pad_PRF2,pad_PRF5,pad_PRF8,pad_PRF3,pad_PRF6,pad_PRF9,"
                           "pad_TRF1,pad_TRF4,pad_TRF7,pad_TRF2,pad_TRF5,pad_TRF8,pad_TRF3,pad_TRF6,pad_TRF9,Panel_id\n")

            is_an_data_block = True

            is_an_PRF_block = False
            is_an_TRF_block = False


        elif "Computed mirror coordinates" in line and is_an_data_block:
            if num_read > 0: #not first read
                print("========================================================")
                print("==== Done with the {}th read in {}. ====".format(num_read, readMode))
                print("========================================================")


            is_an_data_block = False
            is_an_PRF_block = False
            is_an_TRF_block = False
            line_read = 0
            line2write = ""

        if "Panel_" in line and is_an_data_block:
            if 'Current panel coordinates' in line:
                n_panels +=1
                #print(line, line.split(", ")[-2])
                #panel_id = line.split(", ")[-2].split("_")[1]
                panel_id = line.split("Panel_")[1].split(",")[0]
                is_an_PRF_block = False
                is_an_TRF_block = False
                line_read = 0
                #line2write += "{}\n".format(panel_id)

        if "Panel frame pad" in line:
            is_an_PRF_block = True
            is_an_TRF_block = False
            line_read = 0
            continue

        if is_an_PRF_block and is_an_data_block:
            if line_read <3:
                #print("PRF {} line read".format(line_read))
                #print(line)
                line2write += ",".join(line.split())+","
                line_read +=1

        if "Telescope frame pad" in line:
            is_an_TRF_block = True
            is_an_PRF_block = False
            line_read = 0
            continue

        if is_an_TRF_block and is_an_data_block:
            if line_read <3:
                line2write += ",".join(line.split())+","
                line_read += 1
            else:
                is_an_TRF_block = False
                is_an_PRF_block = False
                line2write += "{}\n".format(panel_id)
                print("what the f**k is this: {}\n".format(line2write))
                print line2write
                with open(out_file, 'a') as outf:
                    outf.write("{}\n".format(line2write))
                line2write = ""


    print("{} panel pad coords read".format(n_panels))
    if num_read<1:
        print("Please add the read divider RFP to log file")
    else:
        for f_ in list_of_outfiles:
            df = pd.read_csv(f_)
            cols= ["Panel_id", "pad_PRF1", "pad_PRF2", "pad_PRF3", "pad_PRF4", "pad_PRF5",
                    "pad_PRF6", "pad_PRF7", "pad_PRF8", "pad_PRF9", "pad_TRF1", "pad_TRF2", "pad_TRF3",
                    "pad_TRF4", "pad_TRF5", "pad_TRF6", "pad_TRF7", "pad_TRF8", "pad_TRF9"]

            df_new = df[cols]

            df_new.columns=["Panel_id", "pad_PRF1x", "pad_PRF1y", "pad_PRF1z", "pad_PRF2x", "pad_PRF2y",
                    "pad_PRF2z", "pad_PRF3x", "pad_PRF3y", "pad_PRF3z", "pad_TRF1x", "pad_TRF1y", "pad_TRF1z",
                    "pad_TRF2x","pad_TRF2y", "pad_TRF2z", "pad_TRF3x", "pad_TRF3y", "pad_TRF3z"]
            df_new.to_csv(f_, index=False, float_format='%.3f')

    return




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
        print("Usage:")
        print("python " + sys.argv[0] + " your_log.txt")
        print("or")
        print("python " + sys.argv[0] + " your_log.txt output.csv")
        exit()
        #outputfilename = "/home/ctauser/logs/p2pasclient_log_201905262012.txt"
        #read_pad(outputfilename, out_text="pad_readings.csv")
    elif len(sys.argv) == 2:
        if sys.argv[1] == '-h' or sys.argv[1] == '-help':
            print("Usage:")
            print("python " + sys.argv[0] + " your_log.txt")
            print("or")
            print("python " + sys.argv[0] + " your_log.txt output.csv")
            exit()
        outputfilename = sys.argv[1]
        read_pad(outputfilename, out_text=None)
    elif len(sys.argv) == 3:
        log_1 = sys.argv[1]
        log_2 = sys.argv[2]
        print(log_1.split('.')[0])
        print(log_2.split('.')[0])
        print(log_2.split('.')[0].split('/')[-1])
        outputfilename = sys.argv[1]
        out_textfile = sys.argv[2]
        read_pad(outputfilename, out_text=out_textfile)

    else:
        print("Usage:")
        print("python " + sys.argv[0] + " your_log.txt")
        print("or")
        print("python " + sys.argv[0] + " your_log.txt output.csv")
        exit()


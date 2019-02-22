#import sys
#sys.path.insert(0, "..")

from opcua import Client
#import errno
import sys
import argparse
import os


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Measure astrometry for an images')
    parser.add_argument('-o', '--output', default=None, help='Output text file to hold the data (timestamp, RA and Dec) served by the server; default is None so that everything prints to stdout. ')
    args = parser.parse_args()

    skycam_client = Client("opc.tcp://10.0.50.113:48800")
    if args.output is not None: 
        if os.path.exists(args.output):
            append_write = 'a' # append if already exists
        else:
            append_write = 'w' # make a new file if not
        outf = open(args.output, append_write)
    try:
        skycam_client.connect()
        previous_timestamp=-1
        print("Timestamp, RA, Dec") 
        if args.output is not None and append_write == 'w':
            outf.write("Timestamp, RA, Dec\n") 
        while True:
            # Client has a few methods to get proxy to UA nodes that should always be in address space such as Root or Objects
            root = skycam_client.get_root_node()
            #print("Objects node is: ", root)

            # Node objects have methods to read and write node attributes as well as browse or populate address space
            #print("Children of root are: ", root.get_children())

            # Now getting a variable node using its browse path
            timestamp = root.get_child(["0:Objects", "2:skycam_pointing_data", "2:Timestamp"]).get_value()
            RA = root.get_child(["0:Objects", "2:skycam_pointing_data", "2:RA"]).get_value()
            Dec = root.get_child(["0:Objects", "2:skycam_pointing_data", "2:Dec"]).get_value()
            skycam_pointing_data = root.get_child(["0:Objects", "2:skycam_pointing_data"])
            #Let's not print when no data are served
            if timestamp!=0 and timestamp != previous_timestamp:
            #if timestamp != previous_timestamp:
                #print("skycam_pointing_data obj is: {}".format(skycam_pointing_data))
                #print("timestamp is: {}".format(timestamp))
                #print("RA is: {}".format(RA))
                #print("Dec is: {}".format(Dec))
                print("{}, {}, {}".format(timestamp, RA, Dec))
                if args.output is not None:
                    outf.write("{}, {}, {}\n".format(timestamp, RA, Dec))
            previous_timestamp=timestamp

    except KeyboardInterrupt:
        print("Client interrupted.")
        pass
    finally:
        if args.output is not None:
            outf.close()
        print("Disconnecting client")
        try:
            skycam_client.disconnect()
        except AttributeError:
            print("Server probably isn't up. Exiting.")
            sys.exit()
        except Exception as ex:
            if 'Transport endpoint is not connected' in ex.args:
                print("Server probably stopped. Exiting")
                sys.exit()
            else:
                template = "An exception of type {0} occurred. Arguments:\n{1!r}"
                message = template.format(type(ex).__name__, ex.args)
                print(message)


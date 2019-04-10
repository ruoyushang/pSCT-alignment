#include "common/globalalignment/rangefinder/dls.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <fcntl.h> // For file handling
#include <getopt.h>  // Argument parsing


int main(int argc, char* argv[]) {

    DLS dls;
    if (argc==1) return dls.help();
    char portname [] = "/dev/ttyUSB0";
    int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
   

    if (fd>0)
        dls.setFD(fd);

    int c;

    static struct option long_options[] = {
        {"port"       , required_argument , 0    , 'p'} ,
        {"user"       , no_argument       , 0    , 'u'} ,
        {"measure"    , no_argument       , 0    , 'm'} ,
        {"continuous" , optional_argument , 0    , 'c'} ,
        {"laser"      , required_argument , 0    , 'l'} ,
        {"temp"       , no_argument       , 0    , 't'} ,
        {"offset"     , required_argument , 0    , 'o'} ,
        {"speed"      , required_argument , 0    , 's'} ,
        {"gain"       , required_argument , 0    , 'g'} ,
        {"quality"    , no_argument       , 0    , 'q'} ,
        {"average"    , required_argument , 0    , 'a'} ,
        {"nspikes"    , required_argument , 0    , 'n'} ,
        {"errors"     , required_argument , 0    , 'e'} ,
        {"help"       , no_argument       , 0    , 'h'} ,
        {NULL         , 0                 , NULL ,  0 }
    };

    int option_index = 0;
    while (true) {
        c = getopt_long(argc, argv, "p:umc::l:to:g:hs:qa:n:e:", long_options, &option_index);
//	printf ("?? getopt returned character code %c ??\n", c);
        if (c == (-1)) {
            break;
        }

        switch (c) {
            case 0:
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
            case 'p':
                if (strcmp(portname, optarg)) {
                    sprintf(portname, "%s", optarg);
                    int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
                    if (fd < 0) {
                        printf ("error %d opening %s: %s", errno, portname, strerror (errno));
                        return 0;
                    }
                    dls.setFD(fd);
		    dls.stopTracking(); 
                }
            case 'u':
                dls.setUserCalibrated(true);
                break;

            case 'm':
		dls.stopTracking(); 
                printf("%6.1f mm\n", dls.measureDistance() / 10.0);
                break;

            case 'c':
                dls.stopTracking();

                if (optarg != NULL) {
                    int delay_ms = atoi (optarg);
                    int delay_10ms = delay_ms / 10;

                    dls.startTrackingDelay(delay_10ms);
                }
                else {
                    dls.startTracking();
                }
                while ( !dls.kbhit()) {
                    printf("%6.1f mm\n", dls.readTracking() / 10.0);
                }
                dls.stopTracking();
                break;

            case 'l':
                if (!strcmp(optarg, "on"))
                    dls.laserOn();
                if (!strcmp(optarg, "off"))
                    dls.laserOff();
                break;

            case 't':
                printf("%4.1f C\n", dls.readTemperature() / 10.0);
                break;

            case 's':
                if (!strcmp(optarg, "normal"))
                    dls.setMeasuringCharacteristic(0,0);
                else if (!strcmp(optarg,"fast"))
                    dls.setMeasuringCharacteristic(0,1);
                else if (!strcmp(optarg,"precise"))
                    dls.setMeasuringCharacteristic(0,2);
                else if (!strcmp(optarg,"natural"))
                    dls.setMeasuringCharacteristic(0,3);
                else if (!strcmp(optarg,"timed"))
                    dls.setMeasuringCharacteristic(1,1);
                else if (!strcmp(optarg,"movingerror"))
                    dls.setMeasuringCharacteristic(2,0);
                else if (!strcmp(optarg,"movingnoerror"))
                    dls.setMeasuringCharacteristic(2,1);
                else
                    fprintf(stderr, "ERROR: Invalid Measuring Characteristic\n");
                break;

            case 'o':
                {
                    float offset_mm = atof(optarg);
                    int offset = static_cast <int> (offset_mm * 10.0);
                    dls.setOffset(offset);
                }
                break;

            case 'g':
                {
                    float gain = atof(optarg);
                    dls.setGain(gain);
                }
                break;

            case 'q':
                {
                    int quality = dls.getSignalQuality();
                    printf("%i\n", quality);
                }
                break;

            case 'a':
                dls.setOutputFilter(atoi(optarg), -1, -1);
                break;
            case 'n':
                dls.setOutputFilter(-1, atoi(optarg), -1);
                break;
            case 'e':
                dls.setOutputFilter(-1, -1, atoi(optarg));
                break;

            case 'h':
                dls.help();
                break;

            case '?':
                break;

            default:
                dls.help();
                //printf ("?? getopt returned character code 0%o ??\n", c);
                break;
        }
    }
    if (optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        printf ("\n");
    }
    exit (0);

    //
    //if (optindex < argc) {
    //    printf ("non-option ARGV-elements: ");
    //    while (optindex < argc)
    //        printf ("%s ", argv[optindex++]);
    //    printf ("\n");
    //}
    //exit(0);
}

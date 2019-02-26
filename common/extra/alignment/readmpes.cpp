#include "MPESImageSet.h"
#include <cstdio>
#include <stdio.h>
#include <cstring>
#include <string.h>
#include <signal.h>

using namespace std;

FILE *file;
    
void my_handler(int s)
{
    fclose(file);
    exit(1); 
}

int main(int argc,char **argv)
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    if (argc < 3)
    {
        std::cerr << "Specify the Video Device and the Output File!" 
            << endl;
        return -1;
    }

    int ID = atoi(argv[1]);
    string Result = string(argv[2]);

    file = fopen(Result.c_str(),"a");
    if(!file) file = fopen(Result.c_str(), "w");
    if(!file)
    {
        std::perror("File opening failed");
        return -1;
    }
    
    while (1)
    {
        MPESImageSet images(ID, 9, 0.0, NULL, 320, 500, 50, 0.0, 0.0, 0);

        images.simpleAverage();
       
        if (file!=NULL)
        {
            cout << "printing shit out" << endl;
            fprintf(file, "%e %e %f %f %f %f %f %f\n", 
                   images.SetData.CleanedIntensity, images.SetData.CleanedIntensitySD,
                   images.SetData.xCentroid, images.SetData.yCentroid, 
                   images.SetData.xCentroidSD, images.SetData.yCentroidSD, 
                   images.SetData.xSpotSD, images.SetData.ySpotSD);
        }
    }
    return 0;
}

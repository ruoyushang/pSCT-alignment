#include <iostream>
#include <string>
#include <csignal>
#include <sstream>
#include "actuator.cpp"
#include "platform.cpp"
#include "mpesclass.cpp"

using namespace std;

void sighandler(int)
{
throw 99;
}

int main()
{
signal(SIGINT, sighandler);

try{
int cbc_id=31;
int actuator_ids[6]={463,465,466,467,468,469};
int mpes_ids[6]={0,1,2,-1,-1,-1};
Actuator::DBstruct HDB_ID;
HDB_ID.ip="romulus.ucsc.edu";
HDB_ID.user="CTAreadwrite";
HDB_ID.password="write2db4pSCT";
HDB_ID.dbname="CTAoffline";
Actuator::ASFstruct ASF_ID;
ASF_ID.directory="/home/root/actuator/";

Platform platform(HDB_ID, ASF_ID, cbc_id, actuator_ids, mpes_ids);
for(int i=0; i<6; i++)
{
    platform.actuator[i].SetSPPortNumber(i+1);
    platform.actuator[i].SetRecordingInterval(70);
}
int quit = 0;
std::string input;
int actuator_number;
int mpes_number;
int stepsize;
float length;
float returnlength;
Actuator::position position;
MPES::Position mpesPosition;
std::cout << "Platform Initialized! " << std::endl;
while(quit != 1)
{
std::cout << "Enter Command: ";
std::cin >> input;
if (input == "help")
{
std::cout << std::endl << std::endl << "Commands: " << std::endl << "intstep: Steps integer number of steps." << std::endl << "deltastep: Steps distance in mm from current position, rounded to nearest step." << std::endl << "lengthstep: Steps actuator to absolute length." << std::endl << "findhome: Re-acquires home position via hardware extend limit." << std::endl << "gohome: Moves to home position." << std::endl << "gohomecounterpart: Moves to software retract limit." << std::endl << "readpos: Outputs revolution and angle." << std::endl << "readlength: Outputs length of actuator." << std::endl << "quit: Deconstructs platform object and closes program." << std::endl << "findmatrix: Finds MPES Alignment Matrix and saves to local text file." << std::endl << "invertmatrix: Inverts local alignment matrix text file and saves in local textfile." << std::endl << "readmatrix: Loads matrix from local textfile" << std::endl << "readinverse: Loads inverse matrix from local textfile" << std::endl << "align: Aligns platform to position where all sensor beams are centered using loaded inverse matrix" << std::endl << "readmpes: Reads mpes" << std::endl << "intstepall: Step all motors" << std::endl << "lengthstepall: step all motors" << std::endl << std::endl;
}
if (input == "quit")
{
    quit=1;
}

if (input == "intstepall")
{
int steps [6];
for (int j=0; j<6; j++)
{
std::cout << "Enter steps for actuator "<< j << ": ";
std::cin >> steps[j];
}
platform.StepAll(steps[0],steps[1],steps[2],steps[3],steps[4],steps[5]);
}


if (input == "lengthstepall")
{
int steps [6];
float length;

for (int j=0; j<6; j++)
{
std::cout << "Enter length for actuator "<< j << ": ";
std::cin >> length;
float current_shaft_length=platform.actuator[j].GetShaftLength();
float relative_shaft_length=length-current_shaft_length;
steps[j]=(int) floor( (relative_shaft_length/platform.actuator[j].MM_per_Step)+0.5);
}

platform.StepAll(steps[0],steps[1],steps[2],steps[3],steps[4],steps[5]);
}


//step absolute steps 
//step relative steps intstep
//step absolute length lengthstep
//step relative length deltastep
//get location readpos
//get relative length from home
//get absolute length readlength
if (input == "findmatrix")
{
    int stepsize;
    std::cout << "Enter stepsize: ";
    std::cin >> stepsize;
    platform.FindMatrix(stepsize);
}
if (input == "invertmatrix")
{
    platform.InvertMatrix();
}
if (input == "readmatrix")
{
    platform.ReadMatrix();
}
if (input == "align")
{
    platform.AlignToPosition();
}
if (input == "readinverse")
{
    platform.ReadInverseMatrix();
}
if (input == "readmpes")
{
    std::cout << "Enter MPES Number: ";
    std::cin >> mpes_number;
    if (mpes_number < 0 || mpes_number > 2)
    {
        std::cout << "mpes number must be between 0-2" << std::endl;
    }
    else
    {
        mpesPosition=platform.ReadMPES(mpes_number);
        std::cout << mpesPosition.xCenter << ", " << mpesPosition.yCenter << std::endl;
    }
}
if (input == "intstep")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
        std::cout << "Enter Integer Number of Steps: ";
        std::cin >> stepsize;
        //int stepsremaining=platform.StepActuator(actuator_number-1, stepsize);
        int stepsremaining=platform.actuator[actuator_number-1].MoveSteps(stepsize);
        std::cout << "Steps remaining from target position: " << stepsremaining << std::endl;
    }
}
if (input == "lengthstep")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
        std::cout << "Enter Desired Length of Actuator: ";
        std::cin >> length;
        returnlength=platform.actuator[actuator_number-1].MoveAbsolute(length);
        std::cout << "Remainder length is:" << returnlength << std::endl;
    }
}
if (input == "deltastep")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
        std::cout << "Enter Length of Step: ";
        std::cin >> length;
        returnlength=platform.actuator[actuator_number-1].MoveRelative(length);
        std::cout << "Remainder length is:" << returnlength << std::endl;
 
    }
}
if (input == "findhome")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
    platform.actuator[actuator_number-1].FindHome();
    Actuator::position extendstop=platform.actuator[actuator_number-1].Extend_Stop;
    platform.actuator[actuator_number-1].FindExtendStop();
    int difference = (extendstop.Revolution*200+extendstop.Angle-5)-(platform.actuator[actuator_number-1].Extend_Stop.Revolution*200+platform.actuator[actuator_number-1].Extend_Stop.Angle);
    int absdifference=abs(difference);
    std::cout << "Number of Steps difference between calibration-recorded and current hardware extension stop is" << absdifference << std::endl;
if (absdifference > 30)
{
std::cout << "Warning! Home position may be defined differently than during calibration." << std::endl;
}
    }
}
if (input == "findextendstop")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
	platform.actuator[actuator_number-1].FindExtendStop();
    }
}
if (input == "gohome")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
    int stepsremaining=platform.actuator[actuator_number-1].GoHome();
    std::cout << "Steps Remaining from home: " << stepsremaining << std::endl;
    }
}
if (input == "gohomecounterpart")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
    int stepsremaining=platform.actuator[actuator_number-1].GoHomeCounterpart();
    std::cout << "Steps Remaining from home counterpart: " << stepsremaining << std::endl;
    }
}

if (input == "readpos")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
    position=platform.actuator[actuator_number-1].GetPosition();
    std::cout << "Actuator Revolution: " << position.Revolution << ", Angle: " << position.Angle << std::endl;
    }
}

if (input == "readlength")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
        length=platform.actuator[actuator_number-1].GetShaftLength();
        std::cout << "Actuator Length is: " << length << std::endl;
    }
}

}

}//endtry

catch(int ex)
{
std::cout << "exception number thrown: " << ex << "\n";
if ( ex == 99 )
{
exit;
}
}

return 0;
}


#include "actuator.cpp"
#include "platform.cpp"
#include <iostream>
#include <string>
#include <csignal>
#include <sstream>


using namespace std;

void sighandler(int)
{
throw 99;
}

int main()
{
signal(SIGINT, sighandler);

try{
int cbc_id=1;
int actuator_ids[6]={1,2,3,4,5,6};
int mpes_ids[6]={1,2,3,4,5,6};
Actuator::DBstruct HDB_ID;
HDB_ID.ip="romulus.ucsc.edu";
HDB_ID.user="CTAreadwrite";
HDB_ID.password="write2db4pSCT";
HDB_ID.dbname="CTAoffline";
Actuator::ASFstruct ASF_ID;
ASF_ID.directory="/home/root/actuator/";

Platform platform(HDB_ID, ASF_ID, cbc_ID, actuator_ids, mpes_ids);
for(int i=0; i<6; i++)
{
    platform.actuator[i].SetSPPort(i+1);
    platform.actuator[i].SetRecordingInterval(70);
}
//cbc load parameters

/*
Actuator actuator1(cbc, act1_id, HDB_ID, ASF_ID);
Actuator actuator2(cbc, act2_id, HDB_ID, ASF_ID);
Actuator actuator3(cbc, act3_id, HDB_ID, ASF_ID);
Actuator actuator4(cbc, act4_id, HDB_ID, ASF_ID);
Actuator actuator5(cbc, act5_id, HDB_ID, ASF_ID);
Actuator actuator6(cbc, act6_id, HDB_ID, ASF_ID);
*/

Actuator actuator1(cbc);
Actuator actuator2(cbc);
Actuator actuator3(cbc);
Actuator actuator4(cbc);
Actuator actuator5(cbc);
Actuator actuator6(cbc);

actuator1.SetActuatorID(1);
actuator2.SetActuatorID(2);
actuator3.SetActuatorID(3);
actuator4.SetActuatorID(4);
actuator5.SetActuatorID(5);
actuator6.SetActuatorID(6);

actuator1.SetASF(ASF_ID);
actuator2.SetASF(ASF_ID);
actuator3.SetASF(ASF_ID);
actuator4.SetASF(ASF_ID);
actuator5.SetASF(ASF_ID);
actuator6.SetASF(ASF_ID);


actuator1.SetSPPortNumber(1);
actuator2.SetSPPortNumber(2);
actuator3.SetSPPortNumber(3);
actuator4.SetSPPortNumber(4);
actuator5.SetSPPortNumber(5);
actuator6.SetSPPortNumber(6);


actuator1.SetRecordingInterval(70);
actuator2.SetRecordingInterval(70);
actuator3.SetRecordingInterval(70);
actuator4.SetRecordingInterval(70);
actuator5.SetRecordingInterval(70);
actuator6.SetRecordingInterval(70);


int quit = 0;
std::string input;
int actuator_number;
int stepsize;
while(quit != 1)
{
std::cout << "Enter Command: ";
std::cin >> input;
if (input == "help")
{
std::cout << "display help options here" << std::endl;
}
if (input == "quit")
{
    quit=1;
}
//step absolute steps
//step relative steps
//step absolute length
//step relative length
//get location
//get relative length from home
//get absolute length
if (input == "step")
{
    std::cout << "Enter Actuator Number: ";
    std::cin >> actuator_number;
    if (actuator_number < 1 || actuator_number > 6)
    {
        std::cout << "Error! Actuator Number must be between 1 and 6!" << std::endl;
    }
    else
    {
        std::cout << "Enter Step Size: ";
        std::cin >> stepsize;
        if (actuator_number == 1)
        {
        actuator1.MoveSteps(stepsize);
        }
        else if (actuator_number == 2)
        {
            actuator2.MoveSteps(stepsize);
        }
        else if (actuator_number == 3)
        {
            actuator3.MoveSteps(stepsize);
        }
        else if (actuator_number == 4)
        {
            actuator4.MoveSteps(stepsize);
        }
        else if (actuator_number == 5)
        {
            actuator5.MoveSteps(stepsize);
        }
        else if (actuator_number == 6)
        {
            actuator6.MoveSteps(stepsize);
        }

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


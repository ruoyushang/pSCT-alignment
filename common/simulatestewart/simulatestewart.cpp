#include "stewartplatform.hpp"

#include <iostream>
#include <iomanip>

#include <sstream>
#include <string>

int main(int argc, char *argv[])
{
    double actL[6];
    double panelCoords[6];
    double padCoords[3][3];
    unsigned rotOrder = 0;
    StewartPlatform SP;
    StewartPlatform::PanelType paneltype;

    std::cout << std::fixed << std::setprecision(5);
    if (argc == 1) {
        std::cout << "There are three ways to run this program:\n"
            << "\t1. specify 6 actuator lengths in mm to print out the pad coordinates;\n"
            << "\t2. specify coords of the 3 pads to print out the actuator lengths that\n"
            << "\t   will get you there. All units in mm.\n"
            << "\t3. specify 6 coords of the panel (x,y,z,alpha(Rx),beta(Ry),gamma(Rz))\n"
            << "\t   together with the  order of the axes around which the rotations\n"
            << "\t   were applied (e.g., 321 for gamma(z)->beta(y)->alpha(x)) to print\n"
            << "\t   the actuator lengths that will get you there.\n"
            << "\t   All units in mm -- for the angles, it's the angle in radians*320mm.\n"
            << "\t   \n"
            << "\tYou also need to specify the type of panel for which to perform the computation\n"
            << "\tas the first argument. Avaialbale panel types are P1, P2, S1, S2 or OPT."
            << std::endl;
        return 1;
    }
    else if (argc > 1) {
        std::string tst = argv[1];
        if (tst == "p1" || tst == "P1")
            paneltype = StewartPlatform::PanelType::P1;
        else if (tst == "p2" || tst == "P2")
            paneltype = StewartPlatform::PanelType::P2;
        else if (tst == "s1" || tst == "S1")
            paneltype = StewartPlatform::PanelType::S1;
        else if (tst == "s2" || tst == "S2")
            paneltype = StewartPlatform::PanelType::S2;
        else if (tst == "opt" || tst == "OPT")
            paneltype = StewartPlatform::PanelType::OPT;
        else {
            std::cout << "You need to choose a proper panel type (P1, P2, S1, S2, OPT).\n";
            return 1;
        }

        const char *types[] = {"P1", "P2", "S1", "S2", "OPT"};
        std::cout << "Chosen panel type is " << types[paneltype] << std::endl;
    }

    if (argc == 8) {
        for (int i = 2; i < 8; i++) {
            std::istringstream ss(argv[i]);
            if (!(ss >> actL[i-2])) {
                std::cerr << "Invalid argument " << argv[i] << std::endl;
                return -1;
            }
        }
        
        std::cout << "Pad coords with actuator lengths ( "; 
        for (int i = 0; i < 6; i++) std::cout << actL[i] << " ";
        std::cout << "):" << std::endl;
    }
    else if (argc == 9) {
        for (int i = 2; i < 8; i++) {
            std::istringstream ss(argv[i]);
            if (!(ss >> panelCoords[i-2])) {
                std::cerr << "Invalid argument " << argv[i] << std::endl;
                return -1;
            }
        }
        std::istringstream ss(argv[8]);
        if (!(ss >> rotOrder)) {
            std::cerr << "Invalid argument " << argv[8] << std::endl;
            return -1;
        }
        
        std::cout << "Actuator lengths for the panel position ( "; 
        for (int i = 0; i < 6; i++) std::cout << panelCoords[i] << " ";
        std::cout << "):" << std::endl;
    }
    else if (argc == 11) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                std::istringstream ss(argv[i*3 + j + 2]);
                if (!(ss >> padCoords[i][j])) {
                    std::cerr << "Invalid argument " << argv[i*3 + j + 2] << std::endl;
                    return -1;
                }
            }
        }

        std::cout << "Actuator lengths for the pad coords ( ";
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                std::cout << padCoords[i][j] << " ";
        std::cout << "):" << std::endl;
    }
    else {
        std::cout << "Wrong number of arguments!" << std::endl;
        return -1;
    }

    SP.SetPanelType(paneltype);
    if (rotOrder) {
        // calling order of the panel coords: x, y, z, Rx, Ry, Rz
        // and the order of rotations is given by fRotOrder
        // internally, the coords are taken w.r.t. base origin. Caller gives coords
        // w.r.t. the base triangle position, which is (0, 0, 0, 0, 0, 0).
        SP.ComputeActsFromPanel(panelCoords);
        for (int i = 0; i < 6; i++)
            std::cout << SP.GetActLengths()[i] << " ";
        std::cout << std::endl;
        
        /*
        SP.ComputeStewart(SP.GetActLengths());
        std::cout << "input vs self-consistency (platform coords):" << std::endl;
        for (int i = 0; i < 3; i++)
            std::cout << panelCoords[i] << " -- " << SP.GetPanelCoords()[i] << std::endl;
        for (int i = 0; i < 3; i++)
            std::cout << panelCoords[3 + SP.fRotOrder[2-i] - 1] << " -- " << SP.GetPanelCoords()[i + 3] << std::endl;
        */
    }
    else if (argc == 8) {
        SP.ComputeStewart(actL);
        std::cout << "panel coordinates: " << std::endl;
        for (int i = 0; i < 6; i++)
            std::cout << SP.GetPanelCoords()[i] << " ";
        std::cout << std::endl;

        std::cout << "pad coordinates: " << std::endl;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++)
                std::cout << SP.GetPadCoords(i)[j] << " ";
            std::cout << std::endl;
        }

        /*
        SP.ComputeActsFromPads(SP.GetPadCoords());
        std::cout << "input vs self-consistency (actuator lengths):" << std::endl;
        for (int i = 0; i < 6; i++)
            std::cout << actL[i] << " -- " << SP.GetActLengths()[i] << std::endl;
        */
    }
    else {
        SP.ComputeActsFromPads(padCoords);
        for (int i = 0; i < 6; i++)
            std::cout << SP.GetActLengths()[i] << " ";
        std::cout << std::endl;

        /*
        SP.ComputeStewart(SP.GetActLengths());
        std::cout << "input vs self-consistency (pad coords):" << std::endl;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                 std::cout << padCoords[i][j] << " -- " << SP.GetPadCoords()[i][j] << std::endl;
        */
    }
    SP.SetPanelType(StewartPlatform::PanelType::P1);
        
    return 0;
}

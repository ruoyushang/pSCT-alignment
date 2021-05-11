//
// Created by Deivid Ribeiro on 5/11/21.
//
#include "stewartplatform.hpp"
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "AGeoAsphericDisk.h" // ROBAST dependency
#include "TMinuit.h" // ROOT's implementation of MINUIT for chiSq minimization

#include "client/controllers/mirrorcontroller.hpp"

#include "common/alignment/device.hpp"
#include "common/simulatestewart/mathtools.hpp"
#include "common/simulatestewart/mirrordefinitions.hpp" // definitions of the mirror surfaces

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

const std::string SAVEFILE_DELIMITER = "****************************************";

int main(int argc, char* argv[]){
    std::string usage = "<ACT file>";

    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " " << usage << std::endl;
        return -1;
    }

    std::vector<std::string> ACTpaths;
    for (int i = optind; i < argc; i++) {
        ACTpaths.push_back(argv[i]);
    }

    std::string loadFilePath;
    std::string saveFilePath;
    Device::Identity m_Identity;

    for (const auto& path : ACTpaths) {
        loadFilePath = path;
        saveFilePath = path + "_physicalPos";

        //Check if file already exists
        struct stat buf{};
        if (stat(loadFilePath.c_str(), &buf) == -1) {
            spdlog::error("File {} not found. Please make sure the selected file path is valid.",
                          loadFilePath);
            return 0;
        }

        std::ostringstream os;

        // Open file stream
        std::ifstream infile(loadFilePath);
        if (infile.bad()) {
            spdlog::error("File {} cannot be read. Please check it and try again.", loadFilePath);
            return 0;
        }

        // Check to make sure it matches this mirror
        std::string line;
        getline(infile, line);
        unsigned s = line.find("(");
        unsigned e = line.find(")");
        Device::Identity mirrorId = Device::parseIdentity(line.substr(s, e - s + 2));

        m_Identity = mirrorId;

        // Print Mirror Info
        std::map<Device::Identity, Eigen::VectorXd> panelPositions;
        while (getline(infile, line) && (line != SAVEFILE_DELIMITER)) {
            os << line << std::endl;
        }
        spdlog::info("Mirror Info:\n{}", os.str());

        Eigen::VectorXd deltaActLengths(6);
        Eigen::VectorXd targetActLengths(6);
        Eigen::VectorXd currentActLengths(6);
        Eigen::VectorXd physicalCoords(6);
        unsigned j = 0;

        // Parse all target actuator lengths
        Device::Identity panelId;
        int i = 0;
        StewartPlatform SP;

        // Will save all panel positions, including OT, if it is a child of this mirror.
        spdlog::info("{}: Attempting to write Mirror position to file {}...", m_Identity, saveFilePath);

        //Check if file already exists
        struct stat buf1{};
        if (stat(saveFilePath.c_str(), &buf1) != -1) {
            spdlog::error(
                    "File {} already exists. Please select a different path, or manually delete/move/rename the file in your system.",
                    saveFilePath);
            return OpcUa_Bad;
        }

        // Create output file stream
        std::ofstream f(saveFilePath);

        if (f.bad()) {
            spdlog::error("Cannot write to file at {}. Aborting...", saveFilePath);
            f.close();
            return OpcUa_Bad;
        }

        // Place mirror name/Type and
        // other information at top of file
        f << "Mirror: " << m_Identity << std::endl;
        std::time_t now = std::time(0);
        f << "Timestamp: " << std::ctime(&now) << std::endl;
        f << "Converted from original file " << loadFilePath << std::endl;
        f << SAVEFILE_DELIMITER << std::endl;

        while (infile.peek() != EOF) {
            getline(infile, line);
            std::size_t found = line.find("Az:");
            if (found != std::string::npos) {
                continue;
            }
            s = line.find("(");
            e = line.find(")");
            panelId = Device::parseIdentity(line.substr(s, e - s + 2));
            i = 0;
            while (getline(infile, line) && line != SAVEFILE_DELIMITER) {
                targetActLengths(i) = std::stod(line);
                i++;
            }
            panelPositions[panelId] = targetActLengths;
            spdlog::info("Found position for Panel {}:\n{}\n", panelId, targetActLengths);

            SP.SetPanelType(StewartPlatform::PanelType::OPT);
            // update current coordinates
            SP.ComputeStewart(targetActLengths.data());
            // panel coords
            for (int i = 0; i < 6; i++)
                physicalCoords[i] = SP.GetPanelCoords()[i];

            f << "Panel: " << panelId << std::endl;
            f << physicalCoords << std::endl;
            f << SAVEFILE_DELIMITER << std::endl;
        }

        f.close();
        spdlog::info("{}: Done writing Mirror position to file {}.", m_Identity, saveFilePath);
    }


}

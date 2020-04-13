//
// Created by Deivid Ribeiro on 3/11/20.
//

#include "focalplane.hpp"

#include <utility>

const std::vector<Device::ErrorDefinition> focalplane::ERROR_DEFINITIONS = {
        {"Could not find image.", Device::ErrorState::OperableError},//error 0
};

focalplane::focalplane(Device::Identity identity) :
        Device::Device(std::move(identity)) {
    spdlog::debug("Created Focal Plane....");
    spdlog::debug("Verbosity: {}, ImageFile: {}", std::to_string(m_verbosity), m_ImageFile);
}

void focalplane::set_image_file(std::string imageFile) {
    m_ImageFile = std::move(imageFile);
}

std::string focalplane::get_image_file() {
    return m_ImageFile;
}

void focalplane::set_verbosity(bool verbosity) {
    m_verbosity = verbosity;
}

bool focalplane::get_verbosity() {
    return m_verbosity;
}

void focalplane::turnOn() {
    spdlog::debug("Is on");
}

void focalplane::turnOff() {
    spdlog::debug("Is off");
}

bool focalplane::isOn() {
    return true;
}

Device::ErrorState focalplane::getErrorState() {
    return Device::getErrorState();
}

std::string focalplane::analyzeSinglePanelCommand() {
    std::string command;
    command = m_pModPath + m_pFocalPlane_PY + " " + m_ImageFile
              + " --DETECT_MINAREA " + std::to_string(m_imgAnalysisParams.m_DetectMinArea)
              + " --DEBLEND_MINCONT " + std::to_string(m_imgAnalysisParams.m_DeblendMinCont)
              + " --THRESH " + std::to_string(m_imgAnalysisParams.m_Thresh)
              + " --search_xs " + m_imgAnalysisParams.m_SearchXs
              + " --search_ys " + m_imgAnalysisParams.m_SearchYs;
    if (m_verbosity) {
        command += " -v";
    }

    return command;
}

std::string focalplane::analyzePatternCommand() {
    std::string command;
    command = m_pModPath + m_pFocalPlane_PY + " " + m_ImageFile
              + " --DETECT_MINAREA " + std::to_string(m_imgAnalysisParams.m_DetectMinArea)
              + " --DEBLEND_MINCONT " + std::to_string(m_imgAnalysisParams.m_DeblendMinCont)
              + " --THRESH " + std::to_string(m_imgAnalysisParams.m_Thresh)
              + " -r --ring_rad " + std::to_string(m_PatternRadius)
              + " --ring_frac " + std::to_string(m_RingFrac)
              + " -p " + m_PatternCenter
              + " --ring_tol " + std::to_string(m_RingTol)
              ;

    if (m_verbosity) {
        command += " -v";
    }

    return command;
}

std::string focalplane::CalcMotionPatternToCenter(std::string &sector, std::string &respFile) {
    std::string command;
    command = m_pModPath + m_pCalcMotion_PY + " --p2c "
              + " --sector " + sector
              + " --pattern_radius " + std::to_string(m_PatternRadius)
              + " --resp_file " + respFile;
    if (m_verbosity) {
        command += " -v";
    }
//    "python calc_motion.py --p2c --sector 'P1' --pattern_radius 280 --resp_file M1_matirx_fast.yaml"
    return command;
}

std::string focalplane::CalcMotionCenterToPattern(std::string &sector, std::string &respFile) {
    std::string command;
    command = m_pModPath + m_pCalcMotion_PY + " --c2p "
              + " --sector " + sector
              + " --pattern_radius " + std::to_string(m_PatternRadius)
              + " --resp_file " + respFile;
    if (m_verbosity) {
        command += " -v";
    }
//    "python calc_motion.py --p2c --sector 'P1' --pattern_radius 280 --resp_file M1_matirx_fast.yaml"
    return command;
}

std::string focalplane::CalcMotionSinglePanel2center(int panel, double current_x, double currentY) {
    std::string command;
    command = m_pModPath + m_pCalcMotion_PY + " " + std::to_string(panel) + " -c "
              + " -x " + std::to_string(current_x)
              + " -y " + std::to_string(currentY);
    if (m_verbosity) {
        command += " -v";
    }
//    "python calc_motion.py 1328 -c -x 1444 -y 877"
    return command;
}

std::string focalplane::CalcMotionSinglePanel2pattern(int panel, double currentX, double currentY) {
    std::string command;
    command = m_pModPath + m_pCalcMotion_PY + " " + std::to_string(panel) + " -p "
              + " -x " + std::to_string(currentX)
              + " -y " + std::to_string(currentY);
    if (m_verbosity) {
        command += " -v";
    }
//    "python calc_motion.py 1328 -p -x 1444 -y 877"
    return command;
}

std::string focalplane::getResponseMatrixPatternFast() {
    return std::string();
}

std::string focalplane::getResponseMatrixSinglePanel() {
    return std::string();
}

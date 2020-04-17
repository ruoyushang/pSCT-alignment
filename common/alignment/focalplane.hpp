//
// Created by Deivid Ribeiro on 3/11/20.
//

#ifndef ALIGNMENT_FOCALPLANE_HPP
#define ALIGNMENT_FOCALPLANE_HPP

#include "common/alignment/device.hpp"

#include "spdlog/spdlog.h"

class focalplane : public Device {

public:
    explicit focalplane(Device::Identity identity);

    std::string analyzeSinglePanelCommand();

    std::string analyzePatternCommand();

    std::string CalcMotionPatternToCenter(std::string &sector, std::string &respFile);

    std::string CalcMotionCenterToPattern(std::string &sector, std::string &respFile);

    std::string CalcMotionSinglePanel2center(int panel, double current_x, double currentY);

    std::string CalcMotionSinglePanel2pattern(int panel, double currentX, double currentY);

    std::string getResponseMatrixPatternFast();

    std::string getResponseMatrixSinglePanel();

    void set_image_file(std::string imageFile);

    std::string get_image_file();

    void set_verbosity(bool verbosity);

    bool get_verbosity();

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) {
        return focalplane::ERROR_DEFINITIONS.at(errorCode);
    }

    int getNumErrors() override { return focalplane::ERROR_DEFINITIONS.size(); }

    bool initialize() override {
        return true;
    }

    void turnOn() override;

    void turnOff() override;

    bool isOn() override;

    Device::ErrorState getErrorState() override;

    std::string m_ImageFile;

    bool m_verbosity{};

    struct image_parameters {
        double m_DeblendMinCont = 0.01;
        int m_Thresh = 6;
        double m_DetectMinArea = 30;
        std::string m_SearchXs = "0 0";
        std::string m_SearchYs = "0 0";
    };

    image_parameters m_imgAnalysisParams = image_parameters();
    double m_PatternRadius;
    std::string m_PatternCenter = "1913 1010";
    double m_PhaseOffsetRad;
    double m_RingTol;
    double m_RingFrac;
    double m_MinDist;

    std::string exec(const char* cmd);

protected:
    std::string m_pModPath = "";
    std::string m_pFocalPlane_PY = "focal_plane.py";
    std::string m_pFindMotionFocalPlane_PY = "find_motion_focal_plane.py";
    std::string m_pCalcMotion_PY = "calc_motion.py";
};


#endif //ALIGNMENT_FOCALPLANE_HPP

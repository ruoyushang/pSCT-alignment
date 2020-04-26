//
// Created by Deivid Ribeiro on 3/11/20.
//

#ifndef ALIGNMENT_FOCALPLANE_HPP
#define ALIGNMENT_FOCALPLANE_HPP

#include "common/alignment/device.hpp"
#include "common/utilities/CSVReader.hpp"
#include <boost/regex.hpp>
#include <utility>
#include <libgen.h>

#include "spdlog/spdlog.h"

class focalplane : public Device {

public:
    explicit focalplane(Device::Identity identity);

    std::string analyzeSinglePanelCommand();

    std::string analyzePatternCommand();

    std::string CalcMotionPatternToCenter(std::string &sector, std::string &respFile);

    std::string CalcMotionCenterToPattern(std::string &sector, std::string &respFile);

    std::string CalcMotionSinglePanel2center(int panel, double current_x, double currentY, std::string &respFile);

    std::string CalcMotionSinglePanel2pattern(int panel, double currentX, double currentY, std::string &respFile);

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
        int m_DetectMinArea = 30;
        std::string m_SearchXs = "0 0";
        std::string m_SearchYs = "0 0";
    };

    image_parameters m_imgAnalysisParams = image_parameters();
    int m_PatternRadius;
    std::string m_PatternCenter = "1913 1010";
    double m_PhaseOffsetRad;
    double m_RingTol;
    double m_RingFrac;
    int m_MinDist;
    bool m_show = false;

    static std::string exec(const char* cmd);

    std::string getCSVFilepathFromImageName(std::string image_filepath);

    static std::vector<std::vector<std::string>> getCSVData(std::string data_file);

    std::map<int, std::vector<double>> makePanelCoordinateMap(std::vector<std::vector<std::string>> dataList);

    static std::string getDatetimeFromRAWname(const std::string& raw_name);

    void setDataDir(std::string data_dir) {m_data_dir = std::move(data_dir);};

protected:
    std::string m_pModPath = "/home/ctauser/focal_plane/";
    std::string m_data_dir = "/home/ctauser/focal_plane/data/";
    std::string m_pFocalPlane_PY = "focal_plane.py";
    std::string m_pFindMotionFocalPlane_PY = "find_motion_focal_plane.py";
    std::string m_pCalcMotion_PY = "calc_motion.py";
};


#endif //ALIGNMENT_FOCALPLANE_HPP

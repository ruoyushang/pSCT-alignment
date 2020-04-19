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
    if (m_show){
        command += " --show";
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
              + " --min_dist " + std::to_string(m_MinDist)
              + " --phase_offset_rad " + std::to_string(m_PhaseOffsetRad)
              ;

    if (m_verbosity) {
        command += " -v";
    }
    if (m_show){
        command += " --show";
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

std::string focalplane::exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}
std::string focalplane::getDatetimeFromRAWname(const std::string& raw_name){
    std::string output;
    spdlog::debug("Filename to get datetime string: \'{}\'", raw_name);
    try {
        const char *raw_literal_expr = R"rgx((\d{1,4})-(\d{1,2})-(\d{1,2})-(\d{1,2}):(\d{1,2}):(\d{1,2}))rgx";
        spdlog::trace("Regex search pattern: {}", raw_literal_expr);

        boost::regex expr(raw_literal_expr);
        boost::smatch what;

        if (boost::regex_search(raw_name, what, expr))
        {
            spdlog::trace("Found something during regex");
            spdlog::trace("Response: {}", what[0].str());
            for (int i = 1; i < (int) what.size(); i ++){
                output += what[i] + "_";
            }
        }
        else{
            spdlog::warn("Nothing came out of regex search for this RAW image path.");
        }
    }
    catch (const boost::regex_error& e) {
        spdlog::error("Error for regex: {}", e.what());
        if (e.code() == boost::regex_constants::error_brack) {
            spdlog::error("The code was error_brack");
        }
        else if(e.code() == boost::regex_constants::error_escape){
            spdlog::error("The code was error_escape");
        }
        else{
            spdlog::error("Error code for regex: {}", e.code());
        }
    }
    return output;
}

std::string focalplane::getCSVFilepathFromImageName(std::string image_filepath) {
    char *dirc, *basec, *bname, *dname;
    char *path = const_cast<char *>(image_filepath.c_str());

    spdlog::trace("Path to decode: {} ", path);

    dirc = strdup(path);
    basec = strdup(path);
    dname = dirname(dirc);
    bname = basename(basec);

    std::string s = bname;
    std::string dir = m_data_dir;

    std::string dt_match = getDatetimeFromRAWname(s);
    spdlog::trace("Found a datetime string: {}",dt_match);
    std::string image_filename_prefix = dir + "res_focal_plane_" + dt_match;
    std::string final_path = image_filename_prefix + "ring_search_vvv.csv";

    spdlog::debug("CSV path: {}", final_path);

    return final_path;
}

std::vector<std::vector<std::string>> focalplane::getCSVData(std::string data_file) {
    CSVReader reader(std::move(data_file));
    std::vector<std::vector<std::string> > dataList = reader.getData();
    return dataList;
}

std::map<int, std::vector<double>> focalplane::makePanelCoordinateMap(std::vector<std::vector<std::string>> dataList) {
    std::map <int , std::vector<double>>  m_coordinates_per_panel;

    int i = 0;
    for (const std::vector<std::string> &line : dataList) {
        if (i==0) {
            i++;
            continue;
        }
        else{
            i++;
        }
        std::string panel = line[0];
        std::string x_coord = line[2];
        std::string y_coord = line[3];
        spdlog::debug("{}: ({}, {})", panel, x_coord, y_coord);
        int p = std::stoi(panel);
        double x = std::stod(x_coord);
        double y = std::stod(y_coord);
        m_coordinates_per_panel[p] = {x, y};
    }

    return m_coordinates_per_panel;
}

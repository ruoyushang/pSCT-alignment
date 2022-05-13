#include "common/alignment/mpes.hpp"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unistd.h>

#include "common/alignment/platform.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


const int MPESBase::DEFAULT_IMAGES_TO_CAPTURE = 9;
const std::string MPESBase::DEFAULT_IMAGES_SAVE_DIR_PATH = "/home/root/mpesimages";
const std::string MPESBase::MATRIX_CONSTANTS_DIR_PATH = "/home/root/mpesCalibration/";
const std::string MPESBase::CAL2D_CONSTANTS_DIR_PATH = "/home/root/mpesCalibration/";
const std::string MPESBase::BACKUP_IMAGE_FILEPATH = "doxygen/cta_logo.png";

const std::vector<Device::ErrorDefinition> MPESBase::ERROR_DEFINITIONS = {
    {"Bad connection. No device found",                                                                            Device::ErrorState::FatalError},//error 0
    {"Failed to read data, possible select timeout.",                                          Device::ErrorState::FatalError},//error 1
    {"Intensity of the image is insufficient to process confidently.",                              Device::ErrorState::FatalError},//error 2
    {"Intensity of the image is too bright to produce reliable measurement. Likely cause: no tube or no lid.",             Device::ErrorState::FatalError},//error 3
    {"Intensity of the image is bright enough to perform calculations but the spot width is extensively large > 20px.",      Device::ErrorState::OperableError},//error 4
    {"Image is severely uneven. Likely due to being in the reflection region, too close to webcam edges, or a bad laser. More than 30% deviation.",Device::ErrorState::FatalError},//error 5
    {"Image is mildly uneven. More than 20% but less than 30% deviation.",                                           Device::ErrorState::OperableError},//error 6
    {"Intensity of the image is zero, no pixels pass threshold value.",Device::ErrorState::FatalError}//error 7,
};

MPESBase::MPESBase(Device::Identity identity, Device::DBInfo DBInfo) : Device::Device(std::move(identity)),
                                                                                          m_Calibrate(false){
    if (!DBInfo.empty()) {
        setDBInfo(DBInfo);
    } else {
        spdlog::warn("{} : MPES: No DB info provided.", m_Identity);
    }
}

void MPESBase::setDBInfo(Device::DBInfo DBInfo) {
    m_DBInfo = std::move(DBInfo);
}

bool MPESBase::initialize() {
    if (isBusy()) {
        spdlog::error("{} : MPES::initialize() : Busy, cannot initialize.", m_Identity);
        return false;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    bool status;
    status = __initialize();
    return status;
}

// find and set optimal exposure -- assume I(e) is linear
// returns measured intensity -- check this value to see if things work fine
int MPESBase::setExposure() {
    if (isBusy()) {
        spdlog::error("{} : MPES::setExposure() : Busy, cannot set exposure.", m_Identity);
        return -1;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    int intensity;
    intensity = __setExposure();
    return intensity;
}

// returns intensity of the beam -- 0 if no beam/device.
// -1 if busy
// so check the return value to know if things work fine
int MPESBase::updatePosition() {
    if (isBusy()) {
        spdlog::error("{} : MPES::updatePosition() : Busy, cannot read webcam.", m_Identity);
        return -1;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    spdlog::info("{} : MPES::updatePosition() : Reading webcam...", m_Identity);
    int intensity;
    intensity = __updatePosition();
    spdlog::info("{} : MPES::updatePosition() : Done.", m_Identity);
    saveMPESStatustoDB();
    return intensity;
}

void MPESBase::turnOn() {
    if (isBusy()) {
        spdlog::error("{} : MPES::turnOn() : Busy, cannot turn on MPES.", m_Identity);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    if (__initialize()) {
        __setExposure();
    }
    else {
        spdlog::error("{} : MPES::turnOn() : Did not initialize after turnOn. Will not reset exposure.", m_Identity);
    }
}

void MPESBase::saveMPESStatustoDB() {
    try {
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        driver = get_driver_instance();
        std::string dbAddress = "tcp://" + m_DBInfo.host + ":" + m_DBInfo.port;
        con = driver->connect(dbAddress, m_DBInfo.user, m_DBInfo.password);
        con->setSchema(m_DBInfo.dbname);
        stmt = con->createStatement();

        struct tm tstruct{};
        char buf[80];
        if (m_Position.cleanedIntensity < -1){
            time_t t = time(0);
            tstruct = *localtime(&t);
        }
        else {
            tstruct = *localtime(&m_Position.timestamp);
        }
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

        std::stringstream sql_stmt;
        sql_stmt << "INSERT INTO Opt_MPESStatus (date, serial_number, x_coord, y_coord, "
                    "x_err, y_err, x_width, y_width, intensity";
        for (int e = 0; e < getNumErrors(); e++) {
            sql_stmt << ", error" << e;
        }
        sql_stmt << ") ";

        sql_stmt << " SELECT * from (SELECT ";

        sql_stmt << "'" << buf << "' as `date`, "
                 << m_Identity.serialNumber << " as serial_number, "
                 << m_Position.xCentroid << " as x_coord, "
                 << m_Position.yCentroid << " as y_coord, "
                 << m_Position.xCentroidErr << " as x_err, "
                 << m_Position.yCentroidErr << " as y_err, "
                 << m_Position.xSpotWidth << " as x_width, "
                 << m_Position.ySpotWidth << " as y_width, "
                 << m_Position.cleanedIntensity << " as intensity";
        for (int e = 0; e < getNumErrors(); e++) {
            sql_stmt << ", " << getError(e) << " as error" << e;
        }
        sql_stmt << ") as temp";

        sql_stmt << " WHERE NOT EXISTS( "
                 << " SELECT * from Opt_MPESStatus WHERE (`date`='"
                 << buf << "' AND serial_number="
                 << m_Identity.serialNumber
                 << ") ) LIMIT 1";

        spdlog::trace("{} : Recorded MPES measurement DB ", m_Identity);

        spdlog::trace(sql_stmt.str());
        stmt->execute(sql_stmt.str());

        delete res;
        delete stmt;
        delete con;
    }
    catch (sql::SQLException &e) {
        spdlog::error("# ERR: SQLException in {}"
                      "({}) on line {}\n"
                      "# ERR: {}\n"
                      " (MySQL error code: {}"
                      ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(), e.getSQLState());
        spdlog::error("{} : Operable Error: SQL communication error.", m_Identity);
        return;
    }
}

#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"
#include "common/mpescode/MPESImage.h"
#include "common/mpescode/MPESImageSet.h"
#include "common/mpescode/MPESDevice.h"

void MPES::turnOff() {
    if (isBusy()) {
        spdlog::error("{} : MPES::turnOff() : Busy, cannot turn off MPES.", m_Identity);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_pCBC->usb.disable(getPortNumber());
}

bool MPES::isOn() {
    return m_pCBC->usb.isEnabled(getPortNumber());
}

// returns intensity of the sensor image.
// check this value to see if everything is working fine
bool MPES::__initialize() {
    spdlog::info("{} : MPES::initialize() : Initializing...", m_Identity);
    // we toggle the usb port, checking the video devices when it's off and again when it's on.
    // the new video device is the ID of the newly created MPES.

    m_Errors.assign(getNumErrors(), false);

    m_pCBC->usb.disable(getPortNumber()); // make sure our USB is off
    std::set<int> oldVideoDevices = getVideoDevices(); // count video devices

    m_pCBC->usb.enable(getPortNumber()); // switch the usb back on and wait for the video device to show up
    sleep(4);

    std::set<int> newVideoDevices = getVideoDevices(); // check all video devices again

    std::set<int> toggledDevices;
    std::set_difference(newVideoDevices.begin(), newVideoDevices.end(), oldVideoDevices.begin(), oldVideoDevices.end(),
                        std::inserter(toggledDevices, toggledDevices.begin()));

    int newVideoDeviceId;
    if (toggledDevices.size() == 1) {
        newVideoDeviceId = *toggledDevices.begin(); // get the only element in the set -- this is the new device ID
        if (newVideoDeviceId == -1) {
            return false; // make sure this is a valid video device -- i.e., not -1
        }
    } else {
        spdlog::error("{} : MPES::initialize() : Found {} devices, should be exactly 1.", m_Identity,
                      toggledDevices.size());
        setError(0); // fatal
        return false; // the list should be just one device at this point
    }

    spdlog::debug("MPES::initialize(): Detected new video device {}.", newVideoDeviceId);
    m_pDevice = std::unique_ptr<MPESDevice>(new MPESDevice(newVideoDeviceId));
    m_pImageSet = std::unique_ptr<MPESImageSet>(new MPESImageSet(m_pDevice.get(), DEFAULT_IMAGES_TO_CAPTURE,DEFAULT_IMAGES_SAVE_DIR_PATH.c_str()));

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(6) << getSerialNumber(); // pad serial number to 6 digits with zeros
    std::string MPES_IDstring = oss.str();

    std::string matFileFullPath = MATRIX_CONSTANTS_DIR_PATH + MPES_IDstring + "_MatConstants.txt";
    spdlog::trace("{} : MPES::initialize(): Trying to access {} for Matrix File", m_Identity, matFileFullPath);
    std::string calFileFullPath = CAL2D_CONSTANTS_DIR_PATH + MPES_IDstring + "_2D_Constants.txt";
    spdlog::trace("{} : MPES::initialize(): Trying to access {} for Cal2D File", m_Identity, calFileFullPath);

    std::ifstream matFile(matFileFullPath);
    std::ifstream calFile(calFileFullPath);

    if (matFile.good() && calFile.good()) { // Check if files exist and can be read
        matFile.close();
        calFile.close();

        m_pDevice->LoadCalibration(calFileFullPath.c_str());
        m_pDevice->LoadMatrixTransform(matFileFullPath.c_str());
        spdlog::debug("{} : Read calibration data -- using calibrated values.", m_Identity);
        m_Calibrate = true;
    } else {
        spdlog::debug("{} : Did not read calibration data -- using raw values.", m_Identity);
    }

    spdlog::debug("{} : MPES::initialize() : Done.", m_Identity);
    return true;
}

int MPES::__setExposure() {
    spdlog::info("{} : MPES::setExposure() : Resetting exposure...", m_Identity);

    m_pDevice->SetTolerance(INTENSITY_RATIO_TOLERANCE);

    int currentExposure;
    int tempExposure = m_pDevice->GetExposure();
    bool tryExtremeExposure = true ;
    bool ratio_error = false;

    do{
        spdlog::info("Trying read() again to maintain inside constraints");
        unsetError(4);
        unsetError(5);
        unsetError(6);

        currentExposure = m_pDevice->GetExposure();
        if (std::abs(m_Position.xSpotWidth / m_Position.ySpotWidth - 1) > 0.40) {
            spdlog::error("Image is severely uneven and outside physical expectations. Exiting setExposure loop.");
            ratio_error = true;
            break;
        }

        spdlog::info("Condition 1: {} <= {} * {}/{} = {}",m_Position.cleanedIntensity, m_pDevice->GetTargetIntensity(), currentExposure,std::to_string(MAX_EXPOSURE),std::to_string(m_pDevice->GetTargetIntensity() * ((float)tempExposure/MAX_EXPOSURE)));
        spdlog::info("Condition 2: {} >= {} * {}/{} = {}",m_Position.cleanedIntensity, m_pDevice->GetTargetIntensity(), currentExposure,std::to_string(MIN_EXPOSURE),std::to_string(m_pDevice->GetTargetIntensity() * ((float)tempExposure/MIN_EXPOSURE)));

        __updatePosition();


        spdlog::info("After updating position: ");
        spdlog::info("Condition 1: {} <= {} * {}/{} = {}",m_Position.cleanedIntensity, m_pDevice->GetTargetIntensity(), currentExposure,std::to_string(MAX_EXPOSURE),std::to_string(m_pDevice->GetTargetIntensity() * ((float)tempExposure/MAX_EXPOSURE)));
        spdlog::info("Condition 2: {} >= {} * {}/{} = {}",m_Position.cleanedIntensity, m_pDevice->GetTargetIntensity(), currentExposure,std::to_string(MIN_EXPOSURE),std::to_string(m_pDevice->GetTargetIntensity() * ((float)tempExposure/MIN_EXPOSURE)));

        spdlog::info("{} : MPES::setExposure() : Intensity {} ({}). Exposure: {}.", m_Identity, m_Position.cleanedIntensity,
                      m_pDevice->GetTargetIntensity(), currentExposure);

        tempExposure = m_pDevice->GetExposure();

        // Case 1: intensity is less than target (or even is zero), multiply exposure by precision and try again.
        if (m_Position.cleanedIntensity <= (m_pDevice->GetTargetIntensity() * ((float)tempExposure/MAX_EXPOSURE))){
            if (tryExtremeExposure) {
                tryExtremeExposure = false;
                m_pDevice->SetExposure(MAX_EXPOSURE);
            } else {
                m_pDevice->SetExposure((int) ((float) tempExposure * PRECISION));
            }
            spdlog::warn("Image is too dim, resetting exposure up to {}, from {} ",m_pDevice->GetExposure(), currentExposure);
        }
        // Case 2: intensity is greater than target (or is totally bright, e.g. tube open), divide exposure by precision and try again.
        else if (m_Position.cleanedIntensity >=(m_pDevice->GetTargetIntensity() * ((float)tempExposure/MIN_EXPOSURE))) {
            if (tryExtremeExposure)
            {
                tryExtremeExposure = false;
                m_pDevice->SetExposure(MIN_EXPOSURE);
            }
            else
            {
                m_pDevice->SetExposure((int)((float)tempExposure / PRECISION));
                spdlog::warn("Image is too bright, resetting exposure up to {}, from {}",m_pDevice->GetExposure(), currentExposure);
            }
        }
        // Case 3: normal operation, just increment to get closer to target.
        else {
            m_pDevice->SetExposure((int)((float)tempExposure * (m_pDevice->GetTargetIntensity() / m_Position.cleanedIntensity )));
            spdlog::info("Normal operation, incrementing exposure up to {} from {}",m_pDevice->GetExposure(), currentExposure);
        }
    }
    while (((m_Position.cleanedIntensity < (m_pDevice->GetTargetIntensity()/PRECISION)) || (m_Position.cleanedIntensity > (m_pDevice->GetTargetIntensity() * PRECISION))) && (m_pDevice->GetExposure() <= MAX_EXPOSURE ) && (m_pDevice->GetExposure() >= MIN_EXPOSURE));

    if (m_pDevice->GetExposure() > MAX_EXPOSURE){
        if (m_Position.cleanedIntensity <= (m_pDevice->GetTargetIntensity()/PRECISION)){
        spdlog::error("{} : MPES::setExposure() : Failed to set exposure, reached maximum limit of {}. Setting Error 2 (too dim)...",
                      m_Identity, std::to_string(MPESBase::MAX_EXPOSURE));
        setError(2); //fatal
        }
        m_pDevice->SetExposure(tempExposure);
    }
    else if (m_pDevice->GetExposure() < MIN_EXPOSURE){
        if (m_Position.cleanedIntensity >= (m_pDevice->GetTargetIntensity() * PRECISION)){
        spdlog::error("{} : MPES::setExposure() : Failed to set exposure, reached minimum limit of {}. Setting Error 3 (too bright)...",
                      m_Identity, std::to_string(MPESBase::MIN_EXPOSURE));
        setError(3); //fatal
        }
        m_pDevice->SetExposure(tempExposure);
    }
    else{
        if (! ratio_error) {
            spdlog::info("Normal exit from do-while loop. Intensity and exposure within the bound.");
        }
    }

    spdlog::info("{} : MPES::setExposure() : Done.", m_Identity);

    return (int)m_Position.cleanedIntensity;
}

int MPES::__updatePosition() {
    // initialize to something obvious in case of failure
    m_Position.xCentroid = -3.;
    m_Position.yCentroid = -3.;
    m_Position.xCentroidErr = -3.;
    m_Position.yCentroidErr = -3.;
    m_Position.xSpotWidth = -3.;
    m_Position.ySpotWidth = -3.;
    m_Position.cleanedIntensity = -3.;
    m_Position.timestamp = -3;
    m_Position.exposure = -3;
    m_Position.nSat = -3;
    m_Position.last_img = "-";

    // read sensor
    if (int(m_pImageSet->Capture()) > 0) {
        if (m_Calibrate) {
            m_pImageSet->Matrix_Transform();
            m_pImageSet->Calibrate2D();
        } else {
            m_pImageSet->simpleAverage();
        }

        m_Position.xCentroid = m_pImageSet->SetData.xCentroid;
        m_Position.yCentroid = m_pImageSet->SetData.yCentroid;
        m_Position.xCentroidErr = m_pImageSet->SetData.xCentroidSD;
        m_Position.yCentroidErr = m_pImageSet->SetData.yCentroidSD;
        m_Position.xSpotWidth = m_pImageSet->SetData.xSpotSD;
        m_Position.ySpotWidth = m_pImageSet->SetData.ySpotSD;
        m_Position.cleanedIntensity = m_pImageSet->SetData.CleanedIntensity;
        m_Position.nSat = m_pImageSet->SetData.nSat;
    }
    m_Position.last_img = m_pImageSet->SetData.last_img;
    if (int(m_Position.cleanedIntensity) == -1 ){
        // Real image possible, but no pixels pass threshold
        setError(7);
        spdlog::error("{}: [7] [Fatal] Cleaned Intensity = -1. Intensity of the image is zero, no pixels pass threshold value.",m_Identity.serialNumber);
    }
    if (int(m_Position.cleanedIntensity) == -2 ){
        // No image, no data. Possible select timeout error
        setError(1);
        spdlog::error("{}: [1] [Fatal] Cleaned Intensity = -2. Failed to read data, possible select timeout.",m_Identity.serialNumber);
    }
    else if (int(m_Position.cleanedIntensity) == -3 ){
        // No device, data still has default values -3.
        setError(0);
        spdlog::error("{}: [0] [Fatal] Cleaned Intensity = -3. Device not found. Default values unchanged. ",m_Identity.serialNumber);
    }
    else if (m_Position.cleanedIntensity > 5e5 && m_Position.cleanedIntensity < 1e6) {
        setError(4);
        spdlog::warn("{}: [4] [Operable] Intensity of the image is bright to perform calculations but the spot width is extensively large > 20px.", m_Identity.serialNumber);
    }
    else if (m_Position.cleanedIntensity > 1e6) {
        setError(3);
        spdlog::warn("{}: [3] [Fatal] Intensity of the image is too bright to process confidently. Likely cause: no tube or no lid.", m_Identity.serialNumber);
    }

    if (std::abs(m_Position.xSpotWidth / m_Position.ySpotWidth - 1) > 0.40) {
        spdlog::warn("{}: [5] [Fatal] Image is severely uneven. Likely due to being in the reflection region, too close to webcam edges, or a bad laser. More than 40% deviation.", m_Identity.serialNumber);
        setError(5);
    }
    else if (std::abs(m_Position.xSpotWidth / m_Position.ySpotWidth - 1) > 0.20) {
        spdlog::error("{}: [6] [Operable] Image is mildly uneven. More than 20% but less than 40% deviation", m_Identity.serialNumber);
        setError(6);
    }

    m_Position.exposure = m_pDevice->GetExposure(); 
    m_Position.timestamp = std::time(0); 

    return static_cast<int>(m_Position.cleanedIntensity);
}

std::set<int> MPES::getVideoDevices() {
    std::set<int> videoDevices;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("/dev")) != nullptr) { // Open /dev device directory in filesystem
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != nullptr) {
            std::string currentEntry = ent->d_name;
            size_t pos;
            std::string substringToFind = "video"; // Locate video devices (name including the string "video")
            if ((pos = currentEntry.find(substringToFind)) != std::string::npos) { // Found video device
                pos += substringToFind.length(); // go to immediately after the substring "video"
                int deviceNumber = std::stoi(
                        currentEntry.substr(pos)); // grab remaining part of device name to get the device number
                videoDevices.insert(deviceNumber);
            }
        }
        closedir(dir);
    } else {
        videoDevices = {-1}; // output signalling failure
    }

    return videoDevices;
}

#endif

void DummyMPES::turnOff() {
    m_On = false;
}

bool DummyMPES::isOn() {
    return m_On;
}

bool DummyMPES::__initialize() {
    spdlog::debug("{} : DummyMPES::initialize(): Initializing...", m_Identity);

    m_Errors.assign(getNumErrors(), false);

    m_On = true;
    const double chance = 0.75; // this is the chance of getting true, between 0 and 1;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::bernoulli_distribution dist(chance);
    bool result = dist(mt);
    if (!result){
        setError(0);
    }

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(6) << getSerialNumber(); // pad serial number to 6 digits with zeros
    std::string MPES_IDstring = oss.str();

    std::string matFileFullPath = MATRIX_CONSTANTS_DIR_PATH + MPES_IDstring + "_MatConstants.txt";
    spdlog::trace("{} : DummyMPES::initialize(): Trying to access {} for Matrix File", m_Identity, matFileFullPath);
    std::string calFileFullPath = CAL2D_CONSTANTS_DIR_PATH + MPES_IDstring + "_2D_Constants.txt";
    spdlog::trace("{} : DummyMPES::initialize(): Trying to access {} for Cal2D File", m_Identity, calFileFullPath);

    std::ifstream matFile(matFileFullPath);
    std::ifstream calFile(calFileFullPath);

    if (matFile.good() && calFile.good()) { // Check if files exist and can be read
        matFile.close();
        calFile.close();

        spdlog::debug("{} : Read calibration data -- using calibrated values.", m_Identity);
        m_Calibrate = true;
    } else {
        spdlog::debug("{} : Did not read calibration data -- using raw values.", m_Identity);
    }

    spdlog::debug("{} : DummyMPES::initialize() attempted: Done.", m_Identity);

    // Load default

    return result;
}

int DummyMPES::__setExposure() {
    spdlog::debug("{} : DummyMPES::setExposure() : Setting exposure...", m_Identity);
    sleep(5);
    int intensity = MPESBase::NOMINAL_INTENSITY; // dummy value
    spdlog::debug("{} : DummyMPES::setExposure() : Done.", m_Identity);
    return intensity;
}

int DummyMPES::__updatePosition() {
    // Set internal position variable to dummy values
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<float> xCentroidDistribution(m_Position.xNominal, 5.0);
    std::normal_distribution<float> yCentroidDistribution(m_Position.yNominal, 5.0);
	std::normal_distribution<float> xCentroidErrDistribution(0.2, 0.15);
	std::normal_distribution<float> yCentroidErrDistribution(0.2, 0.15);

    m_Position.xCentroid = xCentroidDistribution(generator);
    m_Position.yCentroid = yCentroidDistribution(generator);
	m_Position.xCentroidErr = xCentroidErrDistribution(generator);
	m_Position.yCentroidErr = yCentroidErrDistribution(generator);
    m_Position.xSpotWidth = MPESBase::NOMINAL_SPOT_WIDTH;
    m_Position.ySpotWidth = MPESBase::NOMINAL_SPOT_WIDTH;
    m_Position.cleanedIntensity = MPESBase::NOMINAL_INTENSITY;
    m_Position.last_img = MPESBase::BACKUP_IMAGE_FILEPATH;
    m_Position.nSat = 0;

    m_Position.exposure = 500.0;
    m_Position.timestamp = std::time(0);

    if (m_Position.cleanedIntensity > 5e5 && m_Position.cleanedIntensity < 1e6) {
        setError(4);
        spdlog::warn("{}: [4] [Operable] Intensity of the image is bright to perform calculations but the spot width is extensively large > 20px.", m_Identity.serialNumber);
    }
    else if (m_Position.cleanedIntensity > 1e6) {
        setError(3);
        spdlog::warn("{}: [3] [Fatal] Intensity of the image is too bright to process confidently. Likely cause: no tube or no lid.", m_Identity.serialNumber);
    }

    if (std::abs(m_Position.xSpotWidth / m_Position.ySpotWidth - 1) > 0.40) {
        spdlog::warn("{}: [5] [Fatal] Image is severely uneven. Likely due to being in the reflection region, too close to webcam edges, or a bad laser. More than 40% deviation.", m_Identity.serialNumber);
        setError(5);
    }
    else if (std::abs(m_Position.xSpotWidth / m_Position.ySpotWidth - 1) > 0.20) {
        spdlog::error("{}: [6] [Operable] Image is mildly uneven. More than 20% but less than 40% deviation", m_Identity.serialNumber);
        setError(6);
    }

    return static_cast<int>(m_Position.cleanedIntensity);
}

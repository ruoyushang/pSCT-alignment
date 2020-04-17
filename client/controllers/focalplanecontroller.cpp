#include <string>
#include <vector>

#include "focalplanecontroller.hpp"
#include "panelcontroller.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

FocalPlaneController::FocalPlaneController(Device::Identity identity, Client *pClient)
        : PasController(std::move(identity), pClient, 500) {
    m_pFP = std::shared_ptr<focalplane>(new focalplane(m_Identity));
    spdlog::debug("Created Focal Plane controller.");
}

/* ----------------------------------------------------------------------------
    Class        FocalPlaneController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus FocalPlaneController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_State;
    spdlog::trace("{} : Read device state => ({})", m_Identity, (int) state);
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        FocalPlaneController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus FocalPlaneController::setState(Device::DeviceState state) {
    m_State = state;
    spdlog::trace("{} : Setting device state => ({})", m_Identity, (int) state);
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        FocalPlaneController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus FocalPlaneController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    switch (offset) {
        case PAS_FocalPlaneType_ImageFile: {
            std::string imageFile;
            imageFile = m_pFP->get_image_file();
            value.setString(imageFile.c_str());
            spdlog::trace("{} : Read ImageFile value => ({})", m_Identity, imageFile);
            break;
        }
        case PAS_FocalPlaneType_Verbosity: {
            bool verbosity;
            verbosity = m_pFP->get_verbosity();
            value.setBool(verbosity);
            spdlog::trace("{} : Read Verbosity value => ({})", m_Identity, verbosity);
            break;
        }
        case PAS_FocalPlaneType_DETECT_MINAREA: {
            double detect_minArea;
            detect_minArea = m_pFP->m_imgAnalysisParams.m_DetectMinArea;
            value.setDouble(detect_minArea);
            spdlog::trace("{} : Read detect_minArea value => ({})", m_Identity, detect_minArea);
            break;
        }
        case PAS_FocalPlaneType_DEBLEND_MINCONT: {
            double deblend_mincont;
            deblend_mincont = m_pFP->m_imgAnalysisParams.m_DeblendMinCont;
            value.setDouble(deblend_mincont);
            spdlog::trace("{} : Read deblend_mincont value => ({})", m_Identity, deblend_mincont);
            break;
        }
        case PAS_FocalPlaneType_THRESH: {
            OpcUa_Int16 thresh;
            thresh = m_pFP->m_imgAnalysisParams.m_Thresh;
            value.setInt16(thresh);
            spdlog::trace("{} : Read thresh value => ({})", m_Identity, thresh);
            break;
        }
        case PAS_FocalPlaneType_SEARCH_XS: {
            std::string search_Xs;
            search_Xs = m_pFP->m_imgAnalysisParams.m_SearchXs;
            value.setString(search_Xs.c_str());
            spdlog::trace("{} : Read Search_Xs value => ({})", m_Identity, search_Xs);
            break;
        }
        case PAS_FocalPlaneType_SEARCH_YS: {
            std::string search_Ys;
            search_Ys = m_pFP->m_imgAnalysisParams.m_SearchYs;
            value.setString(search_Ys.c_str());
            spdlog::trace("{} : Read Search_Ys value => ({})", m_Identity, search_Ys);
            break;
        }
        case PAS_FocalPlaneType_PatternRadius: {
            OpcUa_Double valDouble;
            valDouble = m_pFP->m_PatternRadius;
            value.setDouble(valDouble);
            spdlog::trace("{} : Read PatternRadius value => ({})", m_Identity, valDouble);
            break;
        }
        case PAS_FocalPlaneType_PhaseOffsetRad: {
            OpcUa_Double valDouble;
            valDouble = m_pFP->m_PhaseOffsetRad;
            value.setDouble(valDouble);
            spdlog::trace("{} : Read PhaseOffsetRad value => ({})", m_Identity, valDouble);
            break;
        }
        case PAS_FocalPlaneType_RingFrac: {
            OpcUa_Double valDouble;
            valDouble = m_pFP->m_RingFrac;
            value.setDouble(valDouble);
            spdlog::trace("{} : Read RingFrac value => ({})", m_Identity, valDouble);
            break;
        }
        case PAS_FocalPlaneType_RingTol: {
            OpcUa_Double valDouble;
            valDouble = m_pFP->m_RingTol;
            value.setDouble(valDouble);
            spdlog::trace("{} : Read RingTol value => ({})", m_Identity, valDouble);
            break;
        }
        case PAS_FocalPlaneType_MinDist: {
            OpcUa_Double valDouble;
            valDouble = m_pFP->m_MinDist;
            value.setDouble(valDouble);
            spdlog::trace("{} : Read MinDist value => ({})", m_Identity, valDouble);
            break;
        }
        case PAS_FocalPlaneType_PatternCenter: {
            std::string PatternCenter;
            PatternCenter = m_pFP->m_PatternCenter;
            value.setString(PatternCenter.c_str());
            spdlog::trace("{} : Read Search_Ys value => ({})", m_Identity, PatternCenter);
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
            break;
    }
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        FocalPlaneController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus FocalPlaneController::setData(OpcUa_UInt32 offset, UaVariant value) {
    UaStatus status;

    switch (offset) {
        case PAS_FocalPlaneType_ImageFile: {
            std::string val;
            val = value.toString().toUtf8();
            m_pFP->set_image_file(val);
            spdlog::trace("{} : Setting ImageFile value... value => ({})", m_Identity, val);
            break;
        }
        case PAS_FocalPlaneType_Verbosity: {
            OpcUa_Boolean val;
            value.toBool(val);
            m_pFP->set_verbosity(val);
            spdlog::trace("{} : Setting Verbosity value... value => ({})", m_Identity, val);
            break;
        }
        case PAS_FocalPlaneType_DEBLEND_MINCONT: {
            double val;
            value.toDouble(val);
            m_pFP->m_imgAnalysisParams.m_DeblendMinCont = val;
            spdlog::trace("{} : Setting DeblendMinCont value... value => ({})", m_Identity, val);
            break;
        }
        case PAS_FocalPlaneType_DETECT_MINAREA: {
            double val;
            value.toDouble(val);
            m_pFP->m_imgAnalysisParams.m_DetectMinArea = val;
            spdlog::trace("{} : Setting DetectMinArea value... value => ({})", m_Identity, val);
            break;
        }
        case PAS_FocalPlaneType_THRESH: {
            OpcUa_Int16 val;
            value.toInt16(val);
            m_pFP->m_imgAnalysisParams.m_Thresh = val;
            spdlog::trace("{} : Setting tresh value... value => ({})", m_Identity, val);
            break;
        }
        case PAS_FocalPlaneType_SEARCH_XS: {
            std::string val;
            val = value.toString().toUtf8();
            m_pFP->m_imgAnalysisParams.m_SearchXs = val;
            spdlog::trace("{} : Setting SearchXs value... value => ({})", m_Identity, val);
            break;
        }
        case PAS_FocalPlaneType_SEARCH_YS: {
            std::string val;
            val = value.toString().toUtf8();
            m_pFP->m_imgAnalysisParams.m_SearchYs = val;
            spdlog::trace("{} : Setting SearchYs value... value => ({})", m_Identity, val);
            break;
        }
        default:
            return OpcUa_BadNotWritable;
            break;
    }


    return OpcUa_Good;
}

UaStatus FocalPlaneController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    spdlog::trace("Offset: {}", offset);
    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : FocalPlaneController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }
    switch (offset) {
        case PAS_FocalPlaneType_AnalyzeSinglePanelImage: {
            spdlog::info("FocalPlaneController::operate() :  Calling AnalyzeSinglePanelImage...");
            if (!m_pFP->get_image_file().empty()){
                std::string command;
                command = m_pFP->analyzeSinglePanelCommand();

                spdlog::trace("Calling focal plane command:");
                spdlog::info(command);

                std::string ret;
                ret = m_pFP->exec(command.c_str());
                spdlog::info(ret);

                status = OpcUa_Good;
            }
            else{
                spdlog::error("No Image file to analyze");
                status = OpcUa_Bad;
            }
            break;
        }
        case PAS_FocalPlaneType_AnalyzePatternImage: {
            spdlog::info("FocalPlaneController::operate() :  Calling AnalyzePatternImage...");
            m_pFP->m_PatternRadius = args[0].Value.Double;
            m_pFP->m_PhaseOffsetRad = args[1].Value.Double;
            m_pFP->m_RingFrac = args[2].Value.Double;
            m_pFP->m_MinDist = args[3].Value.Double;
            m_pFP->m_PatternCenter = UaString(args[4].Value.String).toUtf8();
            m_pFP->m_RingTol = args[5].Value.Double;


            if (!m_pFP->get_image_file().empty()){
                std::string command;
                command = m_pFP->analyzePatternCommand();

                spdlog::trace("Calling focal plane command:");
                spdlog::info(command);
                std::string ret;
                ret = m_pFP->exec(command.c_str());
                spdlog::info(ret);

                status = OpcUa_Good;
            }
            else{
                spdlog::error("No Image file to analyze");
                status = OpcUa_Bad;
            }
            break;
        }
        case PAS_FocalPlaneType_SavePanelMotion: {
            spdlog::info("FocalPlaneController::operate() :  Calling SavePanelMotion...");
            spdlog::info("Does nothing for now...");

//            deltaArgs;
//            panelId;
//            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(panelId), UaString("MoveDeltaCoords"), deltaArgs);;
            status = OpcUa_Bad;
            break;
        }
        case PAS_FocalPlaneType_Pattern2Center: {
            spdlog::info("FocalPlaneController::operate() :  Calling CalcPatternToCenter...");
            std::string sector = UaString(args[0].Value.String).toUtf8();
            std::string respfile = UaString(args[1].Value.String).toUtf8();
            m_pFP->m_PatternRadius = args[2].Value.Double;

            std::string command;
            command = m_pFP->CalcMotionPatternToCenter(sector, respfile);

            spdlog::trace("Calling calc motion command:");
            spdlog::info(command);
            std::string ret;
            ret = m_pFP->exec(command.c_str());
            spdlog::info(ret);

            status = OpcUa_Good;
            break;
        }
        case PAS_FocalPlaneType_Center2Pattern: {
            spdlog::info("FocalPlaneController::operate() :  Calling CalcCenterToPattern...");
            std::string sector = UaString(args[0].Value.String).toUtf8();
            std::string respfile = UaString(args[1].Value.String).toUtf8();
            m_pFP->m_PatternRadius = args[2].Value.Double;

            std::string command;
            command = m_pFP->CalcMotionCenterToPattern(sector, respfile);

            spdlog::trace("Calling calc motion command:");
            spdlog::info(command);
            std::string ret;
            ret = m_pFP->exec(command.c_str());
            spdlog::info(ret);

            status = OpcUa_Good;
            break;
        }
        case PAS_FocalPlaneType_Panel2Center: {
            spdlog::info("FocalPlaneController::operate() :  Calling CalcPanelToCenter...");
            int panel = args[0].Value.Int16;
            double x = args[1].Value.Double;
            double y = args[2].Value.Double;

            std::string command;
            command = m_pFP->CalcMotionSinglePanel2center(panel, x, y);

            spdlog::trace("Calling calc motion command:");
            spdlog::info(command);
            std::string ret;
            ret = m_pFP->exec(command.c_str());
            spdlog::info(ret);

            status = OpcUa_Good;
            break;
        }
        case PAS_FocalPlaneType_Panel2Pattern: {
            spdlog::info("FocalPlaneController::operate() :  Calling CalcPanelToPattern...");
            int panel = args[0].Value.Int16;
            double x = args[1].Value.Double;
            double y = args[2].Value.Double;

            std::string command;
            command = m_pFP->CalcMotionSinglePanel2pattern(panel, x, y);

            spdlog::trace("Calling calc motion command:");
            spdlog::info(command);
            std::string ret;
            ret = m_pFP->exec(command.c_str());
            spdlog::info(ret);

            status = OpcUa_Good;
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}


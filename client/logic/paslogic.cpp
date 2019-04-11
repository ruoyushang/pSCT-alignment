#include "paslogic.h"
#include "passervertypeids.h"
#include <uastring.h>
#include <uavariant.h>
#include <statuscode.h>
#include <cmath>

#define TIME __timeStamp()

PasLogic::PasLogic(PasCommunicationInterface *pCommIf) : m_pCommIf {pCommIf}, m_terminate {false},
    m_aligned(false),
    m_kPSDDistance {514.61},
    m_kMisalignmentCriterion {0.3},
    m_kAngularScale {StewartPlatform::kMirrorDistance, m_kPSDDistance},
    m_nominalPSDReadings {0., 0., 0., 0.},
    m_kInitTime {std::chrono::system_clock::now()},
    m_logstream {"paslogic.log", std::ofstream::app},
    tee {m_logstream}
{
    // doing just OPT
    m_SP.SetPanelType(StewartPlatform::PanelType::OPT);

    UaString sName; 
    m_pCommIf->getDeviceConfig(GLOB_PositionerType, 0, sName, m_positionerId);
    m_pCommIf->getDeviceConfig(PAS_PSDType, 0, sName, m_psdId);
    m_optId[0].position = 1001; // primary OPT
    m_optId[1].position = 2001; // secondary OPT
}

PasLogic::~PasLogic()
{
}

void PasLogic::run()
{
    while (__PSDmisaligned(0, false) || __PSDmisaligned(1, false)) {
        for (int ot = 0; ot < 2; ot++) {
            tee << TIME << "--- PasLogic: Initial Alignment of OT" << ot+1 << " ---" << std::endl;
            while (__PSDmisaligned(ot)) {
                __align_PSD(ot);
                __PSDmisaligned();
            }
            tee << TIME << "--- PasLogic: DONE Initial Alignment of OT" << ot + 1 << " ---" << std::endl;
        }
    }

    UaVariant val;
    double panelCoords[6];
    for (int i = 0; i < 2; i++) {
        tee << TIME << "--- PasLogic: Initial OPT" <<i+1<<" coords: ";
        for (int j = 0; j < 6; j++) {
            m_pCommIf->getDeviceData(PAS_PanelType, m_optId[i], PAS_PanelType_curCoords_x + j, val);
            val.toDouble(panelCoords[j]);
            tee << panelCoords[j] << " ";
        }
        tee << std::endl;
    }

    bool postalign;

    float cur_elevation;
    while (!m_terminate) {
        postalign = false;
        while (__telescopeMoving()) {
            postalign = true;
            cur_elevation = __telescopeEl();
            tee << TIME << "elevation: " << cur_elevation << std::endl;

            for (int ot = 0; ot < 2; ot++) {
                if (__PSDmisaligned(ot, false)) {
                    tee << TIME << "PSD"<< ot +1 << " misaligned! Aligning OT" << std::endl;
                    __align_PSD(ot);
                    tee << TIME << "DONE" << std::endl;
                }
            }
            __PSDmisaligned();
        }

        // align after the telescope stopped moving
        if (postalign) {
            tee << TIME << "Postalignment:" << std::endl;
            while(!m_aligned) {
                for (int ot = 0; ot < 2; ot++) {
                    if (__PSDmisaligned(ot, false)) {
                        tee << TIME << "PSD"<< ot +1 << " misaligned! Aligning OT" << std::endl;
                        __align_PSD(ot);
                        tee << TIME << "DONE" << std::endl;
                    }
                }
            }
            tee << TIME << "DONE Postalignment" << std::endl;
            __PSDmisaligned();
        }

        UaThread::msleep(1000);
    }

    tee << TIME << "PasLogic: Received terminate signal. Shutting down..." << std::endl;
}

std::string PasLogic::__timeStamp()
{
    using namespace std::chrono;
    char buf[20];

    time_point<system_clock> cur_time = system_clock::now();
    int MSS = duration_cast<milliseconds>(cur_time - m_kInitTime).count();

    int SS = MSS / 1000;
    int MM = SS / 60;
    int HH = MM / 60;
    MSS %= 1000;
    SS %= 60;
    MM %= 60;

    sprintf(buf, "(%02d:%02d:%02d.%03d) ", HH, MM, SS, MSS); 
    std::string timestamp(buf);

    return timestamp;
}

bool PasLogic::__telescopeMoving()
{
    OpcUa_Boolean isMoving = false;
    UaVariant val;

    UaStatus status = m_pCommIf->getDeviceData(GLOB_PositionerType, m_positionerId, GLOB_PositionerType_isMoving, val);
    UA_ASSERT(status.isGood());

    val.toBool(isMoving);

    return static_cast<bool>(isMoving);
}

float PasLogic::__telescopeEl()
{
    float elevation = -1.;
    UaVariant val;

    UaStatus status = m_pCommIf->getDeviceData(GLOB_PositionerType, m_positionerId, GLOB_PositionerType_curEl, val);
    UA_ASSERT(status.isGood());

    val.toFloat(elevation);

    return elevation;
}

bool PasLogic::__PSDmisaligned(unsigned PSDno, bool verbose)
{
    m_aligned = true;

    double curReadings[4];

    __readPSD(curReadings);

    if (verbose)
        tee << TIME << "PSD readings: (" << curReadings[0] << ", " << curReadings[1] << ") (" 
            << curReadings[2] << ", " <<curReadings[3] << ")" << std::endl;

    double misalignment[2] = {0, 0};
    bool is_misaligned[2];
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            misalignment[i] += (m_nominalPSDReadings[2*i + j] - curReadings[2*i + j])*(m_nominalPSDReadings[2*i + j] - curReadings[2*i + j]);
        }
        misalignment[i] = sqrt(misalignment[i]);
        is_misaligned[i] = (misalignment[i] > m_kMisalignmentCriterion);
        m_aligned &= !is_misaligned[i];
    }

    if (verbose)
        tee << TIME << "misalignment: (" << misalignment[0] << ", " << misalignment[1] << ")" << std::endl;

    return is_misaligned[PSDno];
}

void PasLogic::__readPSD(double readings[4])
{
    UaVariant val;
    for (OpcUa_UInt32 i = 0; i < 4; i++) {
        UaStatus status = m_pCommIf->getDeviceData(PAS_PSDType, m_psdId, PAS_PSDType_x1 + i, val);
        UA_ASSERT(status.isGood());
        val.toDouble(readings[i]);
    }
}

void PasLogic::__align_PSD(unsigned psdNo)
{
    double curReadings[4];
    __readPSD(curReadings);

    /* OT AND PSD AXES ORIENTATIONS */
    // looking from the Primary OT towards the secondary:
    // primary OT x up
    // primary OT y to the right
    // secondary OT x down
    // secondary OT y to the right
    // PSD x to the right
    // PSD y up
    double x_rot = (curReadings[2*psdNo] - m_nominalPSDReadings[2*psdNo])/m_kAngularScale[psdNo];
    double y_rot = (curReadings[2*psdNo + 1] - m_nominalPSDReadings[2*psdNo + 1])/m_kAngularScale[psdNo];
    // adjust the sign for the y rotation of the primary OT:
    y_rot *= (static_cast<double>(psdNo) - 0.5)*2;

    tee << TIME << "rotations to be adjusted by: ";
    tee << x_rot*m_SP.kRp << " " << y_rot*m_SP.kRp << std::endl;

    // get current panel coordinates
    UaVariant val;
    double panelCoords[6];
    for (int i = 0; i < 6; i++) {
        m_pCommIf->getDeviceData(PAS_PanelType, m_optId[psdNo], PAS_PanelType_curCoords_x + i, val);
        val.toDouble(panelCoords[i]);
    }
    
    // new ones are just an additional rotation
    panelCoords[PAS_PanelType_curCoords_xRot - PAS_PanelType_curCoords_x] += x_rot*m_SP.kRp;
    panelCoords[PAS_PanelType_curCoords_yRot - PAS_PanelType_curCoords_x] += y_rot*m_SP.kRp;

    tee << TIME << "New OPT" << psdNo + 1 << " coords: ";
    for (int i = 0; i < 6; i++)
        tee << TIME << panelCoords[i] << " ";
    tee << std::endl;

    UaStatus status = __move_OT(psdNo, panelCoords);
    __PSDmisaligned(0, false);
}

UaStatus PasLogic::__move_OT(unsigned otNo, const double coords[6])
{
    UaVariant val;
    for (int i = 0; i < 6; i++) {
        val.setDouble(coords[i]);
        m_pCommIf->setDeviceData(PAS_PanelType, m_optId[otNo], PAS_PanelType_inCoords_x + i, val);
    }

    tee << TIME << "set new panel coords for OPT" << otNo+1 << "; moving..." << std::endl;
    UaStatus status = m_pCommIf->operateDevice(PAS_PanelType, m_optId[otNo], PAS_PanelType_MoveTo_Coords);
    tee << TIME << "done moving" << std::endl;
    if (!status.isGood())
        tee << TIME << "!!! THERE WAS A PROBLEM MOVING THE PANEL !!!" << std::endl;

    return status;
}

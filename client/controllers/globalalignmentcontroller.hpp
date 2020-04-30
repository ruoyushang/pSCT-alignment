#ifndef ALIGNMENT_GLOBALALIGNMENTCONTROLLER_H
#define ALIGNMENT_GLOBALALIGNMENTCONTROLLER_H

#include <fstream>
#include "client/controllers/pascontroller.hpp"

#include "uathread.h"

class GlobalAlignmentController : public PasCompositeController, UaThread {
    UA_DISABLE_COPY(GlobalAlignmentController);
public:
    explicit GlobalAlignmentController(Device::Identity identity, Client *pClient);

    // initialize and precompute everything
    bool initialize() override;

    // same as before -- get/set status/data and Operate:
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

    // own implementation
    void addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController) override;

    void run() override ;

protected:
    //

private:
    std::string m_Mode;
    OpcUa_Boolean m_tracking;
    bool m_processing;

    // this tells when to stop!
    bool m_terminate;
    // this keep track if aligned
    bool m_aligned;

    // this is to compute platform motion
    StewartPlatform m_SP;

    // align primary OT through PSD readings
    void __align_PSD(unsigned psdNo = 0);

    UaStatus __move_OT(unsigned otNo, const double coords[6]);

    bool __PSDmisaligned(unsigned int psdNo = 0);

    const double m_kPSDDistance;
    const double m_kMisalignmentCriterion;
    const double m_kAngularScale[2];
    double m_nominalPSDReadings[4];
    // align secondary OT through PSD readings, after the primary has been aligned
    bool m_secondaryAligned;

    bool __telescopeMoving();

    void __readPSD(int psdNo, double readings[4]);

    float __telescopeEl();

    const std::chrono::time_point<std::chrono::system_clock> m_kInitTime;

    std::string __timeStamp();

    std::ofstream m_logstream;
    Device::Identity m_positionerId;
    Device::Identity m_psdId[2];
    Device::Identity m_optId[2];

};

#endif //ALIGNMENT_GLOBALALIGNMENTCONTROLLER_H

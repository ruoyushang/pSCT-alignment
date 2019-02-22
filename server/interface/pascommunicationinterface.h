/******************************************************************************
** Project: MPES server
**
** Description: Communication interface to access the sensors.
******************************************************************************/
#ifndef __PASCOMMUNICATIONINTERFACE_H__
#define __PASCOMMUNICATIONINTERFACE_H__

#include "uabase.h"
#include "uastring.h"
#include "uathread.h"
#include "uamutex.h"
#include "statuscode.h"
#include "uaarraytemplates.h"
#include "pascominterfacecommon.h"
#include "passervertypeids.h"
#include <map>
#include <string>

class Platform;
class PasController;

struct Identity;

class PasCommunicationInterface : public PasComInterfaceCommon
{
    UA_DISABLE_COPY(PasCommunicationInterface);
public:

    /* construction / destruction */
    PasCommunicationInterface();
    virtual ~PasCommunicationInterface();

    virtual UaStatusCode Initialize();
    void setServerUrl(const std::string& serverUrl) { m_serverUrl = serverUrl; }

    OpcUa_Int32 getDevices(OpcUa_UInt32 deviceType);
   
    UaStatusCode getDeviceConfig(
        OpcUa_UInt32 type,
        OpcUa_UInt32 deviceIndex,
        UaString& sName,
        Identity& identity);

    /* Get device status and data */
    UaStatusCode getDeviceState(
        OpcUa_UInt32 type,
        const Identity& identity,
        PASState& state);

    UaStatusCode getDeviceData(
        OpcUa_UInt32 type,
        const Identity& identity,
        OpcUa_UInt32 offset,
        UaVariant& value);

    /* Set device status and data*/
    UaStatusCode setDeviceState(
        OpcUa_UInt32 type,
        const Identity& identity,
        PASState state);

    UaStatusCode setDeviceData(
        OpcUa_UInt32 type,
        const Identity& identity,
        OpcUa_UInt32 offset,
        UaVariant value);

    /* operate a device -- run a method */
    UaStatusCode OperateDevice(
        OpcUa_UInt32 type,
        const Identity& identity,
        OpcUa_UInt32 offset = 0,
        const UaVariantArray& args = UaVariantArray());
 
private:
    // Communication Interface Thread main function -- inherited from
    // protected UaThread::run()
    // void run();
    UaMutex m_mutex;

    // keeps the URL of the server this communication interface represents.
    // this is needed to fetch all the relevant info from the DB
    std::string m_serverUrl;

    // maps deviceType -> VectorOfDevices
    std::map< OpcUa_UInt32, std::vector<PasController *> > m_pControllers;

    // maps deviceType -> {USB -> idx in the m_pController[deviceType] vector}
    std::map< OpcUa_UInt32, std::map<int, int> > m_DeviceUsbMap;

    // maps deviceType -> number of devices
    // This is needed in case we're expecting a non-zero number of, say, MPES,
    // but all of them malfunction. m_pPlatform->getMPESCount() returns non-zero, but
    // Initialize() detected 0 MPES, resulting in zero size of m_pControllers[MPESType]
    std::map< OpcUa_UInt32, OpcUa_UInt32> m_DeviceCounts;

    // all currently handled devices and their names
    std::map< OpcUa_UInt32, std::string> m_DeviceTypeNames {{PAS_PanelType, "Panel"},
                                                            {PAS_MPESType, "MPES"},
                                                            {PAS_ACTType, "ACT"},
                                                            {PAS_PSDType, "PSD"}};
    // stop the the thread
    OpcUa_Boolean m_stop;
    
    // physical platform
    Platform *m_platform;
};

#endif // #ifndef __PASCOMMUNICATIONINTERFACE_H__

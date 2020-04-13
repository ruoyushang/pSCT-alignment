/******************************************************************************
** Project: P2PAS server/client
**
** Description: Communication interface to access the devices.
******************************************************************************/
#ifndef __PASCOMMUNICATIONINTERFACE_H__
#define __PASCOMMUNICATIONINTERFACE_H__

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uaarraytemplates.h"
#include "uabase/uabase.h"
#include "uabase/uastring.h"

#include "common/alignment/device.hpp"
#include "common/opcua/pascominterfacecommon.hpp"

#include "client/clienthelper.hpp"
#include "client/utilities/configuration.hpp"


class PasController;

class PasCommunicationInterface: public PasComInterfaceCommon
{
    UA_DISABLE_COPY(PasCommunicationInterface);
public:

    /* construction / destruction */
    PasCommunicationInterface();

    ~PasCommunicationInterface() override;

    UaStatus initialize() override;

    UaStatus initializeCCDs();

    void setConfiguration(std::shared_ptr<Configuration> pConfiguration) {
        m_pConfiguration = std::move(pConfiguration);
    }

    const Device::Identity
    addDevice(Client *pClient, OpcUa_UInt32 deviceType, const Device::Identity &identity, std::string mode = "client");

    void addMirrorControllers();

    void addEdgeControllers();

    void addFocalPlaneController();

    void addGlobalAlignmentController();
    void addOpticalAlignmentController();

    void addParentChildRelations();

    /* Get device status and data */
    UaStatus getDeviceState(
        OpcUa_UInt32 type,
        const Device::Identity &identity,
        Device::DeviceState &state) override;

    UaStatus getDeviceData(
        OpcUa_UInt32 type,
        const Device::Identity &identity,
        OpcUa_UInt32 offset,
        UaVariant &value) override;

    /* Set device status and data*/
    UaStatus setDeviceState(
        OpcUa_UInt32 type,
        const Device::Identity &identity,
        Device::DeviceState state) override;

    UaStatus setDeviceData(
        OpcUa_UInt32 type,
        const Device::Identity &identity,
        OpcUa_UInt32 offset,
        UaVariant value) override;

    /* operate a device -- run a method */
    UaStatus operateDevice(OpcUa_UInt32 type, const Device::Identity &identity,
                           OpcUa_UInt32 offset = 0,
                           const UaVariantArray &args = UaVariantArray()) override;

    std::shared_ptr<PasController> getDeviceFromId(OpcUa_UInt32 type, const Device::Identity &identity);

    static std::map<OpcUa_UInt32, std::string> deviceTypeNames;

private:
    std::shared_ptr<Configuration> m_pConfiguration;
    OpcUa_Boolean m_stop;
};

#endif // #ifndef __PASCOMMUNICATIONINTERFACE_H__

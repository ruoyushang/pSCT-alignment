#ifndef __PASPANEL_H__
#define __PASPANEL_H__

#include "uaserver/methodmanager.h"
#include "components.h"
#include "pasobject.h"

#include "common/opcua/pascominterfacecommon.h"
#include "common/opcua/pasnodemanagercommon.h"

#include "client/pasnodemanager.h"
#include "client/pascommunicationinterface.h"


class UaMethodGeneric;

struct Identity;

class PanelObject : public PasObject {
    UA_DISABLE_COPY(PanelObject);

public:
    PanelObject(
            const UaString &name,
            const UaNodeId &newNodeId,
            const UaString &defaultLocaleId,
            PasNodeManager *pNodeManager,
            Identity identity,
            PasCommunicationInterface *pCommIf) : PasObject(name, newNodeId, defaultLocaleId,
                                                            dynamic_cast<PasNodeManagerCommon *>(pNodeManager),
                                                            identity, dynamic_cast<PasComInterfaceCommon *>(pCommIf)) {}

    UaNodeId typeDefinitionId() const;

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() { return PanelObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() { return PanelObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() { return PanelObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;
};

#endif

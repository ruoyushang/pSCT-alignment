#ifndef __PASEDGE_H__
#define __PASEDGE_H__

#include "uaserver/methodmanager.h"
#include "common/alignment/device.hpp"
#include "pasobject.hpp"

#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasnodemanagercommon.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"


class UaMethodGeneric;

class EdgeObject : public PasObject {
    UA_DISABLE_COPY(EdgeObject);

public:
    EdgeObject(
            const UaString &name,
            const UaNodeId &newNodeId,
            const UaString &defaultLocaleId,
            PasClientNodeManager *pNodeManager,
            Device::Identity identity,
            PasClientCommunicationInterface *pCommIf) : PasObject(name, newNodeId, defaultLocaleId,
                                                                  dynamic_cast<PasNodeManagerCommon *>(pNodeManager),
                                                                  std::move(identity),
                                                                  dynamic_cast<PasComInterfaceCommon *>(pCommIf)) { initialize(); };

    UaNodeId typeDefinitionId() const override { return UaNodeId(PAS_EdgeType, browseName().namespaceIndex()); }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() override { return EdgeObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() override { return EdgeObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() override { return EdgeObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;
};

#endif


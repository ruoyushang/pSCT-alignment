#ifndef __PASMIRROR_H__
#define __PASMIRROR_H__

#include "uaserver/methodmanager.h"
#include "components.hpp"
#include "pasobject.hpp"

class PasNodeManager;
class PasCommunicationInterface;
class UaMethodGeneric;
struct Identity;

class MirrorObject : public PasObject
{
    UA_DISABLE_COPY(MirrorObject);
public:
    MirrorObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManager* pNodeManager,
        Identity identity,
        PasCommunicationInterface *pCommIf);

    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() { return MirrorObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() { return MirrorObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() { return MirrorObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;

};

#endif


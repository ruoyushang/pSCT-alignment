/**
 * @file positioner.h
 * @brief Header file for PositionerObject class inheriting from the PasObject class.
 */

#ifndef __POSITIONER_H__
#define __POSITIONER_H__

#include "uaserver/methodmanager.h"
#include "components.hpp"
#include "pasobject.hpp"

#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasnodemanagercommon.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

class UaMethodGeneric;
struct Identity;

/// @brief Class representing the OPC UA object interface of a positioner device.
class PositionerObject : public PasObject
{
    UA_DISABLE_COPY(PositionerObject); // Prevents copying of object constructor
public:
    /// @brief Constructor for a PositionerObject object.
    /// @param name String name for the underlying OPC UA BaseObjectType node.
    /// @param newNodeId Node id for the object node.
    /// @param defaultLocaleId String indicating the session's locale id/
    /// language setting (i.e. "en" for English).
    /// @param pNodeManager Pointer  PasNodeManager to control the creation of OPC UA
    /// nodes and references.
    /// @param identity Identity object describing the positioner's serial,
    /// location, IP address, etc.
    /// @param pCommIf Pointer to a PasCommunicationInterface which controls
    /// device operation/reading.
    PositionerObject(
            const UaString &name,
            const UaNodeId &newNodeId,
            const UaString &defaultLocaleId,
            PasNodeManager *pNodeManager,
            Identity identity,
            PasCommunicationInterface *pCommIf) : PasObject(name, newNodeId, defaultLocaleId,
                                                            dynamic_cast<PasNodeManagerCommon *>(pNodeManager),
                                                            std::move(identity),
                                                            dynamic_cast<PasComInterfaceCommon *>(pCommIf)) { initialize(); }

    /// @brief Method returning the UaNodeId for the Positioner object type
    /// definition node.
    /// @return A UaNodeId uniquely identifying the Positioner object type node.
    UaNodeId typeDefinitionId() const override;

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() override { return PositionerObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() override { return PositionerObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() override { return PositionerObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;
};

#endif

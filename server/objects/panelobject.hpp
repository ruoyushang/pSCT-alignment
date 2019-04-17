/**
 * @file panelobject.hpp
 * @brief Header file for OPC UA panel object class.
 */

#ifndef SERVER_PANELOBJECT_HPP
#define SERVER_PANELOBJECT_HPP

#include <vector>
#include <map>
#include <tuple>
#include <utility>

#include "uabase/uavariant.h"

#include "uaserver/methodmanager.h"

#include "common/opcua/components.h"
#include "common/opcua/pasobject.h"

class PasNodeManager;

class PasCommunicationInterface;

class UaMethodGeneric;

struct Identity;

/// @brief Class representing an OPC UA panel object.
class PanelObject : public PasObject {
    UA_DISABLE_COPY(PanelObject); // Disables copy construction and copy assignment
public:
    /// @brief Instantiate an OPC UA Panel object.
    /// @param name Name for the underlying OPC UA BaseObjectType node.
    /// @param newNodeId OPC UA node id for the object node.
    /// @param defaultLocaleId The session's locale id/language setting (i.e. "en" for English).
    /// @param pNodeManager Pointer to the node manager used to control the creation of OPC UA
    /// nodes and references.
    /// @param identity Identity object describing the panel's serial,
    /// location, IP address, etc.
    /// @param pCommIf Pointer to a PasCommunicationInterface which controls
    /// device operation/reading.
    PanelObject(
            const UaString &name,
            const UaNodeId &newNodeId,
            const UaString &defaultLocaleId,
            PasNodeManager *pNodeManager,
            Identity identity,
            PasCommunicationInterface *pCommIf) : PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity,
                                                            pCommIf) {}

    /// @brief Destroy an OPC UA panel object.
    ~PanelObject() : ~PasObject();

    /// @brief Return the UaNodeId for the Panel object type definition node.
    /// @return The UaNodeId uniquely identifying the Panel object type node.
    UaNodeId typeDefinitionId() const;

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> getVariableDefs() { return PanelObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> getErrorDefs() { return PanelObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> getMethodDefs() { return PanelObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;
};

#endif //SERVER_PANELOBJECT_HPP

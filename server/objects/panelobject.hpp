/**
 * @file panelobject.hpp
 * @brief Header file for OPC UA panel object class.
 */

#ifndef __PANELOBJECT_H__
#define __PANELOBJECT_H__

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
class PanelObject : public PasObject
{
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
        const UaString&            name,
        const UaNodeId&            newNodeId,
        const UaString&            defaultLocaleId,
        PasNodeManager*            pNodeManager,
        Identity                   identity,
        PasCommunicationInterface* pCommIf);
    /// @brief Destroy an OPC UA panel object.
    virtual ~PanelObject(void);

    /// @brief Call an OPC UA method on a Panel object.
    /// @warning The /p outputArguments and /p inputArgumentDiag parameters are
    /// unused.
    /// @param serviceContext Object containing various client
    /// settings as well as the current Session object.
    /// @param pMethodHandle Pointer to a MethodHandle instance containing
    /// information about the object and method being called.
    /// @param inputArguments Array of method arguments of different types,
    /// represented as OPC UA Variants.
    /// @param outputArguments Array of OPC UA variants to hold the method
    /// outputs [UNUSED].
    /// @param inputArgumentResults Array of OPC UA status codes indicating
    /// which inputs caused an error. Any invalid inputs will cause a
    /// OpcUa_BadInvalidArgument code to be placed in that position in the array.
    /// @param inputArgumentDiag Array of diagnostic info objects describing
    /// the inputs [UNUSED].
    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    /// @brief Return the UaNodeId for the Panel object type definition node.
    /// @return The UaNodeId uniquely identifying the Panel object type node.
    UaNodeId typeDefinitionId() const;

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> variables;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, int>> methods;

private:
    /// @brief Map from the OPC UA node IDs for all supported method nodes to the corresponding method node and the OPC UA type ID for the method type node.
    std::map<UaNodeId, std::pair<UaMethodGeneric *, OpcUa_UInt32>> m_MethodMap;
};

#endif

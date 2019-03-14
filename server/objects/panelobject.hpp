/**
 * @file panelobject.hpp
 * @brief Header file for OPC UA panel object class.
 */

#ifndef __PanelController_H__
#define __PanelController_H__

#include "uabase/uavariant.h"

#include "uaserver/methodmanager.h"

#include "common/opcua/components.h"
#include "common/opcua/pasobject.h"

#include <vector>
#include <map>
#include <tuple>
#include <utility>

class PasNodeManager;
class PasCommunicationInterface;
class UaMethodGeneric;
struct Identity;

/// @brief Class representing an OPC UA panel object.
class PanelObject : public PasObject
{
    UA_DISABLE_COPY(PanelObject); // Disables copy construction and copy assignment
public:
    /// @brief Constructor for a Panel object.
    /// @param name String name for the underlying OPC UA BaseObjectType node.
    /// @param newNodeId Node id for the object node.
    /// @param defaultLocaleId String indicating the session's locale id/
    /// language setting (i.e. "en" for English).
    /// @param pNodeManager Pointer  PasNodeManager to control the creation of OPC UA
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
    /// @brief Destructor for a PositionerObject object.
    virtual ~PanelObject(void);

    /// @brief Call an OPC UA method from a Panel object.
    /// @warning The /p outputArguments and /p inputArgumentDiag parameters are
    /// unused.
    /// @param serviceContext ServiceContext object containing various client
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

    /// @brief Return the UaNodeId for the Panel object type
    /// definition node.
    /// @return A UaNodeId uniquely identifying the Panel object type node.
    UaNodeId typeDefinitionId() const;

    /// @brief
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> variables;
    
    /// @brief
    static const std::map<OpcUa_UInt32, std::pair<std::string, int>> methods;

private:
    std::map<UaNodeId, std::pair<UaMethodGeneric *, OpcUa_UInt32>> m_MethodMap;
};

#endif

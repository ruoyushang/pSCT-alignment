/**
 * @file positioner.h
 * @brief Header file for PositionerObject class inheriting from the PasObject class.
 */

#ifndef __POSITIONER_H__
#define __POSITIONER_H__

#include "uaserver/methodmanager.h"
#include "components.h"
#include "pasobject.h"

class PasNodeManager;
class PasCommunicationInterface;
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
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManager* pNodeManager,
        Identity identity,
        PasCommunicationInterface *pCommIf);
    /// @brief Destructor for a PositionerObject object.
    virtual ~PositionerObject();

    /// @brief General function to call an OPC UA method from a Positioner
    /// object.
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

    /// @brief Method returning the UaNodeId for the Positioner object type
    /// definition node.
    /// @return A UaNodeId uniquely identifying the Positioner object type node.
    UaNodeId typeDefinitionId() const;

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() { return PositionerObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() { return PositionerObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() { return PositionerObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;
};

#endif

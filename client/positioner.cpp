/**
 * @file positioner.cpp
 * @brief Source file for PositionerObject class inheriting from the PasObject class.
 */

#include <iostream>
#include "positioner.h"
#include "pasnodemanager.h"
#include "passervertypeids.h"
#include "pascommunicationinterface.h"
#include "components.h"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

/// @details Inherits from the PasObject constructor. Uses a UaMutexRefCounted
/// shared mutex to allow only one thread to access the object at a time and to
/// clean up the object once all references to it are deleted. Gets the
/// namespace index from the PasNodeManager, then creates all OPC UA variable
///and method nodes as children of the object using the PasNodeManager.
PositionerObject::PositionerObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManager *pNodeManager,
    Identity identity,
    PasCommunicationInterface *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
{
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    OpcUa::DataItemType*         pDataItem            = NULL;
    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the optical table components
     **************************************************************/
    // Add Variable "curAz"
    pDataItem = addVariable(pNodeManager, GLOB_PositionerType, GLOB_PositionerType_isMoving);
    pDataItem = addVariable(pNodeManager, GLOB_PositionerType, GLOB_PositionerType_curAz);
    pDataItem = addVariable(pNodeManager, GLOB_PositionerType, GLOB_PositionerType_curEl);
    pDataItem = addVariable(pNodeManager, GLOB_PositionerType, GLOB_PositionerType_inAz);
    pDataItem = addVariable(pNodeManager, GLOB_PositionerType, GLOB_PositionerType_inEl);
    pDataItem = addVariable(pNodeManager, GLOB_PositionerType, GLOB_PositionerType_EnergyLevel);

    // Add Method "initialize"
    UaString sName = "InitializeDrives";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodInit = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodInit, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveTo"
    sName = "MoveTo";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodMoveTo = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodMoveTo, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    sName = "Stop";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStop = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStop, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "ToggleEnergyLevel"
    sName = "ToggleEnergyLevel";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodSetEnergy = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodSetEnergy, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

}

/// @details If the shared mutex esists (the object still exists), releases the
/// reference before deleting the object.
PositionerObject::~PositionerObject()
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

/// @details Uses the PositionerType id number defined in passervertypeids.h
/// along with the namspace index to construct the UaNodeId for the Positioner
/// type node.
UaNodeId PositionerObject::typeDefinitionId() const
{
    UaNodeId ret(GLOB_PositionerType, browseName().namespaceIndex());
    return ret;
}

/// @details Casts the MethodHandle containing all information about the method
/// call to a UaNode (MethodHandleUaNode), then gets the actual method node
/// from it using pUaMethod(). Based on the UaNodeId of this method node,
/// determines which method it is (by checking against the private pointers
/// to the allowed method nodes), checks that at least one input argument was
/// provided, and calls the method using
/// PasCommunicationInterface.OperateDevice(). OperateDevice() receives the
/// Positioner object type id, its Identity object, and the method node's id.
/// If no input arguments were provided, or the method handle is invalid,
/// returns OpcUa_BadInvalidArgument. Else, returns the relevant UaStatus.

UaStatus PositionerObject::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    MethodHandleUaNode* pMethodHandleUaNode = static_cast<MethodHandleUaNode*>(pMethodHandle);
    UaMethod*           pMethod             = NULL;

    if(pMethodHandleUaNode)
    {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if(pMethod)
        {
            if ( pMethod->nodeId() == m_pMethodInit->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(GLOB_PositionerType, m_Identity, GLOB_PositionerType_Init);
            }

            if ( pMethod->nodeId() == m_pMethodMoveTo->nodeId())
            {
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(GLOB_PositionerType, m_Identity, GLOB_PositionerType_Move);
            }

            if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(GLOB_PositionerType, m_Identity, GLOB_PositionerType_Stop);
            }

            if ( pMethod->nodeId() == m_pMethodSetEnergy->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(GLOB_PositionerType, m_Identity, GLOB_PositionerType_SetEnergy);
            }
        }
        else
        {
            assert(false);
            ret = OpcUa_BadInvalidArgument;
        }
    }
    else
    {
        assert(false);
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}

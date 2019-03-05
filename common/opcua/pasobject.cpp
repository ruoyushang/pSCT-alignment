#include "pasobject.h"
#include "pasnodemanagercommon.h"
#include "passervertypeids.h"
#include "pascominterfacecommon.h"
#include "pasnodemanager.h"
#include "pascommunicationinterface.h"
#include "mpeseventdata.h"
#include "uaserver/methodhandleuanode.h"

#include <iostream>

// ----------------------------------------------------------------
// PasObject implementation
PasObject::PasObject(const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManagerCommon *pNodeManager,
        Identity identity,
        PasComInterfaceCommon *pCommIf) :
                  BaseObjectType(newNodeId, name, pNodeManager->getNameSpaceIndex(),
                         pNodeManager->getNodeManagerConfig()),
                  m_defaultLocaleId(defaultLocaleId),
                  m_pSharedMutex(NULL),
                  m_Identity(identity),
                  m_pCommIf(pCommIf),
                  m_pNodeManager(pNodeManager)
{

    // Store information needed to access device
    //PasUserData* pUserData = new PasUserData(isState, ParentType, m_Identity, VarType);
    //pDataItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    //pDataItem->setValueHandling(UaVariable_Value_Cache);

}

PasObject::~PasObject() {}

OpcUa_Byte PasObject::eventNotifier() const
{
    return Ua_EventNotifier_SubscribeToEvents;
}


MethodManager* PasObject::getMethodManager(UaMethod* pMethod) const
{
    OpcUa_ReferenceParameter(pMethod);
    return (MethodManager*)this;
}


// generic begin call -- calls worker thread that needs to be implemented for device;
// that is, only <ObjectType>::call() method needs to be implemented for asynchronous calls
// to work through UaSession::beginCall() on the client side
UaStatus PasObject::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    UaStatus ret;

    OpcUa::MethodCallJob* pCallJob = new OpcUa::MethodCallJob;
    pCallJob->initialize(this, pCallback, serviceContext, callbackHandle, pMethodHandle, inputArguments);
    ret = NodeManagerRoot::CreateRootNodeManager()->pServerManager()->getThreadPool()->addJob(pCallJob);
    if ( ret.isBad() )
    {
        delete pCallJob;
    }

    return ret;
}

OpcUa::DataItemType* PasObject::addVariable(PasNodeManagerCommon *pNodeManager, OpcUa_UInt32 ParentType, OpcUa_UInt32 VarType, OpcUa_Boolean isState, bool addReference)
{
    // Get the instance declaration node used as base for this variable instance
    UaVariable* pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(VarType);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    OpcUa::DataItemType* pDataItem = new OpcUa::DataItemType(this, pInstanceDeclaration, pNodeManager, m_pSharedMutex);
    
    UaStatus addStatus;
    if (addReference)
        addStatus = pNodeManager->addNodeAndReference(this, pDataItem, OpcUaId_HasComponent);
    else
        addStatus = pNodeManager->addUaNode(pDataItem->getUaNode());
    UA_ASSERT(addStatus.isGood());

    // Store information needed to access device
    PasUserData* pUserData = new PasUserData(isState, ParentType, m_Identity, VarType);
    pDataItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pDataItem->setValueHandling(UaVariable_Value_Cache);

    return pDataItem;
}

// -------------------------------------------------------------------
// Specialization: MPESObject Implementation
MPESObject::MPESObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManagerCommon *pNodeManager,
    Identity identity,
    PasComInterfaceCommon *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
  {
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    OpcUa::DataItemType*         pDataItem            = NULL;
    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the sensor components
     **************************************************************/
    // Add Variable "State"
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_State, OpcUa_True);

    // Add state alarm condition "StateCondition" as component to the object
    m_pStateOffNormalAlarm = new OpcUa::OffNormalAlarmType(
        UaNodeId(UaString("%1.StateCondition").arg(this->nodeId().toString()), nsIdx),  // NodeId
        "StateCondition",           // Name of the node used for browse name and display name
        nsIdx,                      // Namespace index of the browse name
        pNodeManager,               // Node manager responsible for this node
        this->nodeId(),             // NodeId of the source node
        this->browseName().name()); // Name of the source node
    addStatus = pNodeManager->addNodeAndReference(this, m_pStateOffNormalAlarm, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Create HasEventSource reference from object to state variable
    addStatus = pNodeManager->addUaReference(this, pDataItem, OpcUaId_HasEventSource);
    UA_ASSERT(addStatus.isGood());
    // Create HasCondition reference from state variable to condition
    addStatus = pNodeManager->addUaReference(pDataItem, m_pStateOffNormalAlarm, OpcUaId_HasCondition);
    UA_ASSERT(addStatus.isGood());
    m_pStateOffNormalAlarm->setConditionName("StateCondition");
    // Set the initial states of the condition
    m_pStateOffNormalAlarm->setAckedState(OpcUa_True);
    m_pStateOffNormalAlarm->setEnabledState(OpcUa_True);
    /* =====================================================================*/

    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_xCentroidAvg);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_yCentroidAvg);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_xCentroidSD);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_yCentroidSD);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_CleanedIntensity);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_xCentroidNominal);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_yCentroidNominal);

    // Add Method "Start"
    UaString sName = "Start";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStart = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStart, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    sName = "Stop";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStop = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStop, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    sName = "Read";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodRead = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodRead, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "SetExposure"
    sName = "SetExposure";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodSetExposure = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodSetExposure, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

}



MPESObject::~MPESObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}



UaNodeId MPESObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_MPESType, browseName().namespaceIndex());
    return ret;
}


/*==========================================================================*/
UaStatus MPESObject::call(
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
            // Check if we have the start method
            if ( pMethod->nodeId() == m_pMethodStart->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_MPESType, m_Identity,
                        PASState::PAS_On );
            }
            // Check if we have the stop method
            else if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_MPESType, m_Identity,
                        PASState::PAS_Off );
            }
            // Check if we have the read method
            else if ( pMethod->nodeId() == m_pMethodRead->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MPESType, m_Identity, PAS_MPESType_Read);
            }
            // Check if we have the SetExposure method
            else if ( pMethod->nodeId() == m_pMethodSetExposure->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MPESType, m_Identity, PAS_MPESType_SetExposure);
            }
            // Create Sensor Events if state change succeeded
            if ( ret.isGood() )
            {
                //--------------------------------------------------------
                // Create event
                MPESEventTypeData eventData(browseName().namespaceIndex());

                // Handle MPESEventType specific fields
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_xCentroidAvg, eventData.m_xCentroidAvg);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_yCentroidAvg, eventData.m_yCentroidAvg);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_xCentroidSD, eventData.m_xCentroidSD);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_yCentroidSD, eventData.m_yCentroidSD);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_CleanedIntensity, eventData.m_CleanedIntensity);
                PASState state;
                m_pCommIf->getDeviceState(PAS_MPESType, m_Identity, state);
                eventData.m_State.setUInt32(static_cast<unsigned>(state));

                // Fill all default event fields
                eventData.m_SourceNode.setNodeId(nodeId());
                eventData.m_SourceName.setString(browseName().toString());
                UaString messageText;
                if ( state == PASState::PAS_Off )
                {
                    messageText = UaString("State of %1 changed to OFF").arg(browseName().toString());
                }
                else
                {
                    messageText = UaString("State of %1 changed to ON").arg(browseName().toString());
                }
                eventData.m_Message.setLocalizedText(UaLocalizedText("en", messageText));
                eventData.m_Severity.setUInt16(500);
                // Set timestamps and unique EventId
                eventData.prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());

                // Fire the event
                m_pNodeManager->fireEvent(&eventData);

                //--------------------------------------------------------
                // Change state of alarm condition
                if ( state == PASState::PAS_Off )
                {
                    m_pStateOffNormalAlarm->setAckedState(OpcUa_False);
                    m_pStateOffNormalAlarm->setActiveState(OpcUa_True);
                    m_pStateOffNormalAlarm->setRetain(OpcUa_True);
                }
                else
                {
                    m_pStateOffNormalAlarm->setAckedState(OpcUa_True);
                    m_pStateOffNormalAlarm->setActiveState(OpcUa_False);
                    m_pStateOffNormalAlarm->setRetain(OpcUa_False);
                }
                m_pStateOffNormalAlarm->setMessage(UaLocalizedText("en", messageText));
                m_pStateOffNormalAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
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


// -------------------------------------------------------------------
// Specialization: ACTObject Implementation
ACTObject::ACTObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManagerCommon *pNodeManager,
    Identity identity,
    PasComInterfaceCommon *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
{
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    OpcUa::DataItemType*         pDataItem            = NULL;
    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the sensor components
     **************************************************************/
    //Create the folder for the Errors
    UaFolder *pErrorFolder = new UaFolder("Errors", UaNodeId("Errors", nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, pErrorFolder, OpcUaId_Organizes);
    UA_ASSERT(addStatus.isGood());
    
    // Add Variable "State"
    pDataItem = addVariable(pNodeManager, PAS_ACTType, PAS_ACTType_State, OpcUa_True);

    // Add Variable "Steps"
    pDataItem = addVariable(pNodeManager, PAS_ACTType, PAS_ACTType_Steps);
    pDataItem = addVariable(pNodeManager, PAS_ACTType, PAS_ACTType_curLength_mm);
    pDataItem = addVariable(pNodeManager, PAS_ACTType, PAS_ACTType_inLength_mm);

    // Add Method "Start"
    UaString sName = "Start";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStart = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStart, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    sName = "Stop";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStop = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStop, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Step"
    sName = "Step";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStep = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStep, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
}

ACTObject::~ACTObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId ACTObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_ACTType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus ACTObject::call(
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
            // Check if we have the step() method
            if ( pMethod->nodeId() == m_pMethodStep->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_ACTType, m_Identity);
            }
            // Check if we have the stop method
            else if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_ACTType, m_Identity,
                        PASState::PAS_Off );
            }
            // Check if we have the start method
            else if ( pMethod->nodeId() == m_pMethodStart->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_ACTType, m_Identity,
                        PASState::PAS_On );
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

// -------------------------------------------------------------------
// Specialization: PSDObject Implementation
PSDObject::PSDObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManagerCommon *pNodeManager,
    Identity identity,
    PasComInterfaceCommon *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
{
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();
    OpcUa::DataItemType*         pDataItem            = NULL;

    /**************************************************************
     * Create the PSD components
     **************************************************************/
    // Add Variable "State"
    // Get the instance declaration node used as base for this variable instance
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_State, OpcUa_True);

    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_x1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_y1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_x2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_y2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dx1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dy1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dx2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dy2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_Temp);

    // Add Method "StepAll"
    UaString sName = "Update";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodUpdate = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodUpdate, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
}

PSDObject::~PSDObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId PSDObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_PSDType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus PSDObject::call(
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
            // Check if we have the step() method
            if ( pMethod->nodeId() == m_pMethodUpdate->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_PSDType, m_Identity);
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

#include <iostream>
#include "gasobject.h"
#include "pasnodemanager.h"
#include "passervertypeids.h"
#include "pascommunicationinterface.h"
#include "components.h"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

// -------------------------------------------------------------------
// Specialization: OptTableObject Implementation
OptTableObject::OptTableObject(
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
    // Add Variable "State"
    pDataItem = addVariable(pNodeManager, PAS_OptTableType, PAS_OptTableType_State, OpcUa_True);

    // Add Variable "StepSize"
    pDataItem = addVariable(pNodeManager, PAS_OptTableType, PAS_OptTableType_StepSize);

    // Add Method "FindMatrix"
    UaString sName = "FindMatrix";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodFindMatrix = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodFindMatrix, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    sName = "Align";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodAlign = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodAlign, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "StepAll"
    sName = "StepAll";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStepAll = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStepAll, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
}

OptTableObject::~OptTableObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId OptTableObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_OptTableType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus OptTableObject::call(
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
            if ( pMethod->nodeId() == m_pMethodFindMatrix->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_OptTableType, m_Identity, PAS_OptTableType_FindMatrix);
            }
            else if ( pMethod->nodeId() == m_pMethodAlign->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_OptTableType, m_Identity, PAS_OptTableType_Align);
            }
            else if ( pMethod->nodeId() == m_pMethodStepAll->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_OptTableType, m_Identity, PAS_OptTableType_StepAll);
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
// ----------------------------------------------------------------
//  CCDObject Implementation
CCDObject::CCDObject(
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
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_State, OpcUa_True);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_xFromLED);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_yFromLED);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_zFromLED);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_psiFromLED);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_thetaFromLED);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_phiFromLED);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_xNominal);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_yNominal);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_zNominal);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_psiNominal);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_thetaNominal);
    pDataItem = addVariable(pNodeManager, PAS_CCDType, PAS_CCDType_phiNominal);

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
}



CCDObject::~CCDObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}



UaNodeId CCDObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_CCDType, browseName().namespaceIndex());
    return ret;
}


/*==========================================================================*/
UaStatus CCDObject::call(
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

 std::cout << "calling method ";
        if(pMethod)
        {
            // Check if we have the start method
            if ( pMethod->nodeId() == m_pMethodStart->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_CCDType, m_Identity,
                                                    PASState::On);
            }
            // Check if we have the stop method
            else if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_CCDType, m_Identity,
                                                    PASState::Off);
            }
            // Check if we have the read method
            else if ( pMethod->nodeId() == m_pMethodRead->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_CCDType, m_Identity);
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

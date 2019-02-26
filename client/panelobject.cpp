#include "panelobject.h"
#include "pasnodemanager.h"
#include "passervertypeids.h"
#include "pascommunicationinterface.h"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

// -------------------------------------------------------------------
// Specialization: PanelObject Implementation
PanelObject::PanelObject(
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
     * Create the panel components
     **************************************************************/

    pDataItem = addVariable(pNodeManager, PAS_PanelType, PAS_PanelType_State, OpcUa_True);

    unsigned vars[] = {
        PAS_PanelType_curCoords_x,
        PAS_PanelType_curCoords_y,
        PAS_PanelType_curCoords_z,
        PAS_PanelType_curCoords_xRot,
        PAS_PanelType_curCoords_yRot,
        PAS_PanelType_curCoords_zRot,
        PAS_PanelType_inCoords_x,
        PAS_PanelType_inCoords_y,
        PAS_PanelType_inCoords_z,
        PAS_PanelType_inCoords_xRot,
        PAS_PanelType_inCoords_yRot,
        PAS_PanelType_inCoords_zRot,
        PAS_PanelType_IntTemperature,
        PAS_PanelType_ExtTemperature};
    for (auto &var : vars)
        pDataItem = addVariable(pNodeManager, PAS_PanelType, var);

    // Add Method "MoveToActs"
    UaString sName = "MoveToActs";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodMoveToActs = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodMoveToActs, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveToCoords"
    sName = "MoveToCoords";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodMoveToCoords = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodMoveToCoords, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    sName = "Read";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodRead = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodRead, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    sName = "Stop";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStop = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStop, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

}

PanelObject::~PanelObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId PanelObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_PanelType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus PanelObject::call(
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
            if ( pMethod->nodeId() == m_pMethodMoveToActs->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_PanelType, m_Identity, PAS_PanelType_MoveTo_Acts);
            }
            else if ( pMethod->nodeId() == m_pMethodMoveToCoords->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_PanelType, m_Identity, PAS_PanelType_MoveTo_Coords);
            }
            else if ( pMethod->nodeId() == m_pMethodRead->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_PanelType, m_Identity, PAS_PanelType_Read);
            }
            else if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_PanelType, m_Identity, PAS_PanelType_Stop);
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

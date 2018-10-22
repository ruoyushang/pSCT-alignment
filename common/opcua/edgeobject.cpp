#include "edgeobject.h"
#include "pasnodemanager.h"
#include "passervertypeids.h"
#include "pascommunicationinterface.h"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

// -------------------------------------------------------------------
// Specialization: EdgeObject Implementation
EdgeObject::EdgeObject(
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

    OpcUa::DataItemType*         pDataItem = NULL;
    UaStatus                     addStatus;
    UaPropertyMethodArgument*    pPropertyArg = NULL;
    UaUInt32Array                nullarray;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the edge components
     **************************************************************/
    // Add Variable "State"
    pDataItem = addVariable(pNodeManager, PAS_EdgeType, PAS_EdgeType_State, OpcUa_True);

    // Add Variable "StepSize"
    pDataItem = addVariable(pNodeManager, PAS_EdgeType, PAS_EdgeType_StepSize);

    // Add Method "FindMatrix"
    UaString sName = "FindMatrix";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodFindMatrix = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodFindMatrix, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    sName = "Stop";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStop = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStop, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Move"
    sName = "Move";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodMove = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodMove, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    sName = "Read";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodRead = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodRead, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    sName = "Align";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodAlign = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodAlign, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Property "inPanel"
    sName = "inPanel";
    sNodeId = UaString("%1.%2").arg(m_pMethodAlign->nodeId().toString()).arg(sName);
    pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(sNodeId, nsIdx),             // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            2,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument inPanel
    pPropertyArg->setArgument(
            0,                       // Index of the argument
            "movePanel",             // Name of the argument
            UaNodeId(OpcUaId_UInt32),// Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "Position of the panel to move during alignment")); // Description
    pPropertyArg->setArgument(
            1,                       // Index of the argument
            "fixPanel",              // Name of the argument
            UaNodeId(OpcUaId_UInt32),// Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "Position of the panel to keep fixed during alignment")); // Description
    // Add property to method
    addStatus = pNodeManager->addNodeAndReference(m_pMethodAlign, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());
}

EdgeObject::~EdgeObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId EdgeObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_EdgeType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus EdgeObject::call(
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
            // Check which method we have
            if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_EdgeType, m_Identity, PAS_EdgeType_Stop);
            }
            else if ( pMethod->nodeId() == m_pMethodMove->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_EdgeType, m_Identity, PAS_EdgeType_Move);
            }
            else if ( pMethod->nodeId() == m_pMethodFindMatrix->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_EdgeType, m_Identity, PAS_EdgeType_FindMatrix);
            }
            else if ( pMethod->nodeId() == m_pMethodRead->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_EdgeType, m_Identity, PAS_EdgeType_Read);
            }
            else if ( pMethod->nodeId() == m_pMethodAlign->nodeId())
            {
                // Number of input arguments must be 2
                if ( inputArguments.length() != 2 )
                    ret = OpcUa_BadInvalidArgument;
                else {
                    inputArgumentResults.create(inputArguments.length());

                    // validate input parameters
                    if ( inputArguments[0].Datatype != OpcUaType_UInt32 ) {
                        ret = OpcUa_BadInvalidArgument;
                        inputArgumentResults[0] = OpcUa_BadTypeMismatch;
                    }
                    if ( inputArguments[1].Datatype != OpcUaType_UInt32 ) {
                        ret = OpcUa_BadInvalidArgument;
                        inputArgumentResults[1] = OpcUa_BadTypeMismatch;
                    }
                    if (ret.isGood())
                        ret = m_pCommIf->OperateDevice(PAS_EdgeType, m_Identity, PAS_EdgeType_Align, inputArguments);
                }
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


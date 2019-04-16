#include "mirrorobject.h"
#include "pasnodemanager.h"
#include "passervertypeids.h"
#include "pascommunicationinterface.h"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

// -------------------------------------------------------------------
// Specialization: MirrorObject Implementation
MirrorObject::MirrorObject(
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
    pDataItem = addVariable(pNodeManager, PAS_MirrorType, PAS_MirrorType_State, OpcUa_True);
 
    unsigned vars[] = {
        PAS_EdgeType_AlignFrac,
        PAS_MirrorType_curCoords_x,
        PAS_MirrorType_curCoords_y,
        PAS_MirrorType_curCoords_z,
        PAS_MirrorType_curCoords_xRot,
        PAS_MirrorType_curCoords_yRot,
        PAS_MirrorType_curCoords_zRot,
        PAS_MirrorType_selectedPanels,
        PAS_MirrorType_selectedEdges,
        PAS_MirrorType_selectedMPES};


    for (auto &var : vars)
        pDataItem = addVariable(pNodeManager, PAS_MirrorType, var);

    UaFolder *pSysFolder = NULL;

    //Create the folder for the Systematic Offsets
    pSysFolder = new UaFolder("SysOffsets", UaNodeId("SysOffsets", nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, pSysFolder, OpcUaId_Organizes);
    UA_ASSERT(addStatus.isGood());


    // add systematic offset variables
    unsigned sysOffsets[] = {
        PAS_MirrorType_sysOffsetsMPES_x1,
        PAS_MirrorType_sysOffsetsMPES_y1,
        PAS_MirrorType_sysOffsetsMPES_x2,
        PAS_MirrorType_sysOffsetsMPES_y2,
        PAS_MirrorType_sysOffsetsMPES_x3,
        PAS_MirrorType_sysOffsetsMPES_y3};
    for (auto &var : sysOffsets) {
        // add data variable without creating a reference
        pDataItem = addVariable(pNodeManager, PAS_MirrorType, var, false, false);
        addStatus = pNodeManager->addUaReference(pSysFolder->nodeId(), pDataItem->nodeId(), OpcUaId_Organizes);
    }


    // Add Method "Move"
    UaString sName = "Move";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodMove = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodMove, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveSector"
    sName = "MoveSector";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodMoveSector = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodMoveSector, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "ReadPos"
    sName = "ReadPosition";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodReadPos = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodReadPos, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "ReadAlign"
    sName = "ReadAlignment";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodReadAlign = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodReadAlign, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "AlignSector"
    sName = "AlignSector";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodAlignSector = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodAlignSector, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    sName = "Align";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodAlign = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodAlign, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Property "inEdge"
    sName = "inEdge";
    sNodeId = UaString("%1.%2").arg(m_pMethodAlign->nodeId().toString()).arg(sName);
    pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(sNodeId, nsIdx),             // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            2,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument inPanel
    pPropertyArg->setArgument(
            0,                       // Index of the argument
            "startEdge",             // Name of the argument
            UaNodeId(OpcUaId_String),      // Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "The edge from which to start sequential alignment")); // Description
    pPropertyArg->setArgument(
            1,                       // Index of the argument
            "direction",             // Name of the argument
            UaNodeId(OpcUaId_UInt32),// Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "Direction in which to move: 0 for +ZRot, 1 for -ZRot")); // Description
    // Add property to method
    addStatus = pNodeManager->addNodeAndReference(m_pMethodAlign, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());


    // Add Method "GlobalAlign"
    sName = "GlobalAlign";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodGlobalAlign = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodGlobalAlign, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Property "inPanel" to this method
    sName = "inPanel";
    sNodeId = UaString("%1.%2").arg(m_pMethodAlign->nodeId().toString()).arg(sName);
    pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(sNodeId, nsIdx),             // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            1,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument inPanel
    pPropertyArg->setArgument(
            0,                       // Index of the argument
            "fixPanel",             // Name of the argument
            UaNodeId(OpcUaId_UInt32),      // Data type of the argument
            OpcUa_ValueRanks_Scalar, // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "This panel will stay fixed. It's the constraining panel for the global fit")); // Description
    addStatus = pNodeManager->addNodeAndReference(m_pMethodGlobalAlign, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());


    // Add Method "Stop"
    sName = "Stop";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStop = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStop, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

}

MirrorObject::~MirrorObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId MirrorObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_MirrorType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus MirrorObject::call(
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
                    ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_Stop);
            }
            else if ( pMethod->nodeId() == m_pMethodMove->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_MoveToCoords);
            }
            else if ( pMethod->nodeId() == m_pMethodMoveSector->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_SimulateAlignSector);
            }
            else if ( pMethod->nodeId() == m_pMethodAlignSector->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_AlignSector);
            }

            else if ( pMethod->nodeId() == m_pMethodAlign->nodeId())
            {
                // Number of input arguments must be 2
                if ( inputArguments.length() != 2 )
                    ret = OpcUa_BadInvalidArgument;
                else {
                    inputArgumentResults.create(inputArguments.length());

                    // validate input parameters
                    if ( inputArguments[0].Datatype != OpcUaType_String ) {
                        ret = OpcUa_BadInvalidArgument;
                        inputArgumentResults[0] = OpcUa_BadTypeMismatch;
                    }
                    if ( inputArguments[1].Datatype != OpcUaType_UInt32 ) {
                        ret = OpcUa_BadInvalidArgument;
                        inputArgumentResults[1] = OpcUa_BadTypeMismatch;
                    }
                    if (ret.isGood())
                        ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_Align, inputArguments);
                }
            }
            else if ( pMethod->nodeId() == m_pMethodGlobalAlign->nodeId())
            {
                // Number of input arguments must be 1
                if ( inputArguments.length() != 1 )
                    ret = OpcUa_BadInvalidArgument;
                else {
                    inputArgumentResults.create(inputArguments.length());

                    // validate input parameters
                    if ( inputArguments[0].Datatype != OpcUaType_UInt32 ) {
                        ret = OpcUa_BadInvalidArgument;
                        inputArgumentResults[0] = OpcUa_BadTypeMismatch;
                    }
                    if (ret.isGood())
                        ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_GlobalAlign, inputArguments);
                }
            }

            else if ( pMethod->nodeId() == m_pMethodReadPos->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_ReadPos);
            }
            else if ( pMethod->nodeId() == m_pMethodReadAlign->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MirrorType, m_Identity, PAS_MirrorType_ReadAlign);
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


#include "furnacecontrollerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"
#include "bacommunicationinterface.h"

FurnaceControllerObject::FurnaceControllerObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    NmBuildingAutomation* pNodeManager,
    OpcUa_UInt32 deviceAddress,
    BaCommunicationInterface *pCommIf)
: ControllerObject(name, newNodeId, defaultLocaleId, pNodeManager, deviceAddress, pCommIf)
{
    UaVariable*                  pInstanceDeclaration = NULL;
    OpcUa::DataItemType*         pDataItem            = NULL;
    BaUserData*                  pUserData            = NULL;
    UaStatus                     addStatus;
    // Method helpers
    UaPropertyMethodArgument*    pPropertyArg = NULL;
    UaUInt32Array                nullarray;
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the FurnaceController components
     **************************************************************/
    // Add Variable "GasFlow"
    // Get the instance declaration node used as base for this variable instance
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_FurnaceControllerType_GasFlow);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    pDataItem = new OpcUa::DataItemType(
        this,           // Parent node
        pInstanceDeclaration, // Instance declaration variable this variable instance is based on
        pNodeManager,   // Node manager responsible for this variable
        m_pSharedMutex);  // Shared mutex used across all variables of this object
    addStatus = pNodeManager->addNodeAndReference(this, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_False, deviceAddress, 3);
    pDataItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pDataItem->setValueHandling(UaVariable_Value_Cache);
    
    // Add Method "StartWithSetpoint"
    UaString sName = "StartWithSetpoint";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStartWithSetpoint = new UaMethodGeneric(
        sName,
        UaNodeId(sNodeId, nsIdx),
        m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStartWithSetpoint, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    sName = "StartWithSetpoint";
    sNodeId = UaString("%1.%2").arg(m_pMethodStartWithSetpoint->nodeId().toString()).arg(sName);
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(sNodeId, nsIdx),             // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        1,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetPoint
    pPropertyArg->setArgument(
        0,                       // Index of the argument
        "TemperatureSetPoint",   // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        OpcUa_ValueRanks_Scalar, // Array rank of the argument
        nullarray,               // Array dimensions of the argument
        UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Add property to method
    addStatus = pNodeManager->addNodeAndReference(m_pMethodStartWithSetpoint, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());
}

FurnaceControllerObject::~FurnaceControllerObject(void)
{
}

UaNodeId FurnaceControllerObject::typeDefinitionId() const
{
    UaNodeId ret(Ba_FurnaceControllerType, browseName().namespaceIndex());
    return ret;
}

UaStatus FurnaceControllerObject::call(
    UaMethod*             pMethod,
    const UaVariantArray& inputArguments,
    UaVariantArray&       /*outputArguments*/,
    UaStatusCodeArray&    inputArgumentResults,
    UaDiagnosticInfos&    /*inputArgumentDiag*/)
{
    UaStatus ret;

    if(pMethod)
    {
        // Check if we have the start method
        if ( pMethod->nodeId() == m_pMethodStartWithSetpoint->nodeId())
        {
            // Number of input arguments must be 1
            if ( inputArguments.length() != 1 )
            {
                ret = OpcUa_BadInvalidArgument;
            }
            else
            {
                inputArgumentResults.create(1);
                // validate input parameter
                if ( inputArguments[0].Datatype != OpcUaType_Double )
                {
                    ret = OpcUa_BadInvalidArgument;
                    inputArgumentResults[0] = OpcUa_BadTypeMismatch;
                }
                else
                {
                    UaVariant    vTemp;
                    OpcUa_Double value;

                    ret = m_pCommIf->setControllerState(
                        m_deviceAddress,
                        BaCommunicationInterface::Ba_ControllerState_On );
                    if ( ret.isGood() )
                    {
                        // Setting TemperatureSetPoint
                        vTemp = inputArguments[0];
                        vTemp.toDouble(value);
                        ret = m_pCommIf->setControllerData(
                            m_deviceAddress,
                            1,
                            value);
                    }
                }
            }
        }
        else
        {
            ret = OpcUa_BadMethodInvalid;
        }
    }
    return ret;
}


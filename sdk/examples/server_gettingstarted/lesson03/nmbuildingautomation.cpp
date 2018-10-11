#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "airconditionercontrollerobject.h"
#include "furnacecontrollerobject.h"
#include "opcua_analogitemtype.h"
#include "bacommunicationinterface.h"

NmBuildingAutomation::NmBuildingAutomation()
: NodeManagerBase("urn:UnifiedAutomation:CppDemoServer:BuildingAutomation")
{
    m_pCommIf = new BaCommunicationInterface;
}

NmBuildingAutomation::~NmBuildingAutomation()
{
    delete m_pCommIf;
}

UaStatus NmBuildingAutomation::afterStartUp()
{
    UaStatus ret;
    UaFolder                       *pFolder = NULL;
    AirConditionerControllerObject *pAirConditioner = NULL;
    FurnaceControllerObject        *pFurnace = NULL;
    UaString       sControllerName;
    OpcUa_UInt32   count = m_pCommIf->getCountControllers();
    OpcUa_UInt32   i;
    OpcUa_UInt32   controllerAddress;
    BaCommunicationInterface::ControllerType controllerType;

    createTypeNodes();

    /**************************************************************
     Create a folder for the controller objects and add the folder to the ObjectsFolder
    ***************************************************************/
    pFolder = new UaFolder("BuildingAutomation", UaNodeId("BuildingAutomation", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pFolder, OpcUaId_Organizes);

    /**************************************************************
     * Create the Controller Object Instances
     **************************************************************/
    for ( i=0; i<count; i++ )
    {
        ret = m_pCommIf->getControllerConfig(
            i,
            controllerType,
            sControllerName,
            controllerAddress);

        if ( controllerType == BaCommunicationInterface::AIR_CONDITIONER )
        {
            pAirConditioner = new AirConditionerControllerObject(
                sControllerName,
                UaNodeId(sControllerName, getNameSpaceIndex()),
                m_defaultLocaleId,
                this,
                controllerAddress,
                m_pCommIf);
            ret = addNodeAndReference(pFolder, pAirConditioner, OpcUaId_Organizes);
            UA_ASSERT(ret.isGood());
        }
        else
        {
            pFurnace = new FurnaceControllerObject(
                sControllerName,
                UaNodeId(sControllerName, getNameSpaceIndex()),
                m_defaultLocaleId,
                this,
                controllerAddress,
                m_pCommIf);
            ret = addNodeAndReference(pFolder, pFurnace, OpcUaId_Organizes);
            UA_ASSERT(ret.isGood());
        }
    }

    return ret;
}

UaStatus NmBuildingAutomation::beforeShutDown()
{
    UaStatus ret;

    return ret;
}

UaStatus NmBuildingAutomation::readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues)
{
    UaStatus ret;
    OpcUa_UInt32 i;
    OpcUa_UInt32 count = arrUaVariables.length();
    UaDateTime timeStamp = UaDateTime::now();

    // Create result array
    arrDataValues.create(count);

    for (i=0; i<count; i++)
    {
        // Set timestamps
        arrDataValues[i].setSourceTimestamp(timeStamp);
        arrDataValues[i].setServerTimestamp(timeStamp);

        // Cast UaVariable to BaControllerVariable
        UaVariable* pVariable = arrUaVariables[i];
        if (pVariable)
        {
            BaUserData* pUserData = (BaUserData*)pVariable->getUserData();

            if ( pUserData )
            {
                UaVariant    vTemp;
                UaStatusCode status;

                if ( pUserData->isState() == OpcUa_False )
                {
                    // Read of a data variable
                    OpcUa_Double value;

                    // Get the data for the controller from the communication interface
                    status = m_pCommIf->getControllerData(
                        pUserData->deviceAddress(),
                        pUserData->variableOffset(),
                        value);
                    if ( status.isGood() )
                    {
                        // Set value
                        vTemp.setDouble(value);
                        arrDataValues[i].setValue(vTemp, OpcUa_True, OpcUa_False);
                    }
                    else
                    {
                        // Set Error
                        arrDataValues[i].setStatusCode(status.statusCode());
                    }
                }
                else
                {
                    // Read of a state variable
                    // We need to get the state of the controller
                    BaCommunicationInterface::ControllerState state;

                    // Get the data for the controller from the communication interface
                    status = m_pCommIf->getControllerState(
                        pUserData->deviceAddress(),
                        state);
                    if ( status.isGood() )
                    {
                        // Set value
                        vTemp.setUInt32(state);
                        arrDataValues[i].setValue(vTemp, OpcUa_True, OpcUa_False);
                    }
                    else
                    {
                        // Set Error
                        arrDataValues[i].setStatusCode(status.statusCode());
                    }
                }
            }
            else
            {
                arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
            }
        }
        else
        {
            arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
        }
    }

    return ret;
}

UaStatus NmBuildingAutomation::writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes)
{
    UaStatus ret;
    OpcUa_UInt32 i;
    OpcUa_UInt32 count = arrUaVariables.length();

    // Create result array
    arrStatusCodes.create(count);

    for ( i=0; i<count; i++ )
    {
        // Cast UaVariable to BaControllerVariable
        UaVariable* pVariable = arrUaVariables[i];
        if ( pVariable )
        {
            BaUserData* pUserData = (BaUserData*)pVariable->getUserData();

            if ( pUserData )
            {
                if ( pUserData->isState() == OpcUa_False )
                {
                    UaVariant    vTemp(arrpDataValues[i]->Value);
                    UaStatusCode status;
                    OpcUa_Double value;

                    status = vTemp.toDouble(value);
                    if ( status.isGood() )
                    {
                        // Get the data for the controller from the communication interface
                        status = m_pCommIf->setControllerData(
                            pUserData->deviceAddress(),
                            pUserData->variableOffset(),
                            value);
                    }
                    arrStatusCodes[i] = status.statusCode();
                }
                else
                {
                    // State variable can not be written
                    arrStatusCodes[i] = OpcUa_BadNotWritable;
                }
            }
            else
            {
                arrStatusCodes[i] = OpcUa_BadInternalError;
            }
        }
        else
        {
            arrStatusCodes[i] = OpcUa_BadInternalError;
        }
    }

    return ret;
}

UaStatus NmBuildingAutomation::createTypeNodes()
{
    UaStatus ret;
    UaStatus addStatus;

    UaVariant                    defaultValue;
    UaObjectTypeSimple*          pControllerType = NULL;
    UaObjectTypeSimple*          pAirConditionerControllerType = NULL;
    UaObjectTypeSimple*          pFurnaceControllerType = NULL;
    OpcUa::BaseDataVariableType* pDataVariable;
    OpcUa::DataItemType*         pDataItem;
    OpcUa::AnalogItemType*       pAnalogItem;

    /**************************************************************
     * Create the Controller Type
     **************************************************************/
    // Add ObjectType "ControllerType"
    pControllerType = new UaObjectTypeSimple(
        "ControllerType",    // Used as string in browse name and display name
        UaNodeId(Ba_ControllerType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pControllerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Controller Type Instance declaration
     ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataVariable = new OpcUa::BaseDataVariableType(
        UaNodeId(Ba_ControllerType_State, getNameSpaceIndex()), // NodeId of the Variable
        "State",                // Name of the Variable
        getNameSpaceIndex(),    // Namespace index of the browse name (same like NodeId)
        defaultValue,           // Initial value
        Ua_AccessLevel_CurrentRead, // Access level
        this);                  // Node manager for this variable
    // Set Modelling Rule to Mandatory
    pDataVariable->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pDataVariable, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "Temperature" as AnalogItem
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_ControllerType_Temperature, getNameSpaceIndex()),
        "Temperature",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    UaRange tempRange(0, 100);
    pAnalogItem->setEURange(tempRange);
    UaEUInformation tempEUInformation("", -1, UaLocalizedText("en", "\xc2\xb0\x46") /* °F */, UaLocalizedText("en", "Degrees Fahrenheit"));
    pAnalogItem->setEngineeringUnits(tempEUInformation);

    // Add Variable "TemperatureSetPoint" as AnalogItem
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_ControllerType_TemperatureSetPoint, getNameSpaceIndex()),
        "TemperatureSetPoint",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    pAnalogItem->setEngineeringUnits(tempEUInformation);

    // Add Variable "PowerConsumption"
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(Ba_ControllerType_PowerConsumption, getNameSpaceIndex()),
        "PowerConsumption",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the AirConditionerController Type
     **************************************************************/
    pAirConditionerControllerType = new UaObjectTypeSimple(
        "AirConditionerControllerType", // Used as string in browse name and display name
        UaNodeId(Ba_AirConditionerControllerType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,
        OpcUa_False);
    // Add Object Type node to address space and create reference to Controller type
    addStatus = addNodeAndReference(pControllerType, pAirConditionerControllerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the AirConditionerController Type Instance declaration
     ***************************************************************/
    // Add Variable "Humidity"
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_AirConditionerControllerType_Humidity, getNameSpaceIndex()),
        "Humidity",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pAirConditionerControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    UaEUInformation tempEUPercent("", -1, UaLocalizedText("en", "%"), UaLocalizedText("en", "Percent"));
    pAnalogItem->setEngineeringUnits(tempEUPercent);

    // Add Variable "HumiditySetpoint"
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_AirConditionerControllerType_HumiditySetpoint, getNameSpaceIndex()),
        "HumiditySetpoint",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pAirConditionerControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    pAnalogItem->setEngineeringUnits(tempEUPercent);

    /**************************************************************
     * Create the FurnaceController Type
     **************************************************************/
    pFurnaceControllerType = new UaObjectTypeSimple(
        "FurnaceControllerType", // Used as string in browse name and display name
        UaNodeId(Ba_FurnaceControllerType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,
        OpcUa_False);
    // Add Object Type node to address space and create reference to Controller type
    addStatus = addNodeAndReference(pControllerType, pFurnaceControllerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the FurnaceController Type Instance declaration
     **************************************************************/
    // Add Variable "GasFlow"
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(Ba_FurnaceControllerType_GasFlow, getNameSpaceIndex()),
        "GasFlow",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pFurnaceControllerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    return ret;
}

UaVariable* NmBuildingAutomation::getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier)
{
    // Try to find the instance declaration node with the numeric identifier
    // and the namespace index of this node manager
    UaNode* pNode = findNode(UaNodeId(numericIdentifier, getNameSpaceIndex()));
    if ( (pNode != NULL) && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        // Return the node if valid and a variable
        return (UaVariable*)pNode;
    }
    else
    {
        return NULL;
    }
}

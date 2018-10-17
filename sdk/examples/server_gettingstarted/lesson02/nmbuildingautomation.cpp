#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "airconditionercontrollerobject.h"
#include "furnacecontrollerobject.h"
#include "opcua_analogitemtype.h"

NmBuildingAutomation::NmBuildingAutomation()
: NodeManagerBase("urn:UnifiedAutomation:CppDemoServer:BuildingAutomation")
{
}

NmBuildingAutomation::~NmBuildingAutomation()
{
}

UaStatus NmBuildingAutomation::afterStartUp()
{
    UaStatus ret;
    AirConditionerControllerObject *pAirConditioner;
    FurnaceControllerObject        *pFurnace;
    UaString sName;

    createTypeNodes();

    sName = "AirConditioner1";
    pAirConditioner = new AirConditionerControllerObject(
                sName,
                UaNodeId(sName, getNameSpaceIndex()),
                m_defaultLocaleId,
                this
                );

    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pAirConditioner, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    sName = "Furnace1";
    pFurnace = new FurnaceControllerObject(
                sName,
                UaNodeId(sName, getNameSpaceIndex()),
                m_defaultLocaleId,
                this
                );

    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pFurnace, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    return ret;
}

UaStatus NmBuildingAutomation::beforeShutDown()
{
    UaStatus ret;

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
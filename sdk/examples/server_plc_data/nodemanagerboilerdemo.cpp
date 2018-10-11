#include "nodemanagerboilerdemo.h"
#include "opcuaplc_nodemanagerplcopen.h"
#include "opcuadi_nodemanagerdevices.h"
#include "opcuaplc_ctrlconfigurationtype.h"
#include "opcuaplc_ctrlresourcetype.h"
#include "opcuaplc_ctrlprogramtype.h"
#include "opcuaplc_ctrlfunctionblocktype.h"
#include "opcuadi_identifiers.h"

NodeManagerBoilerDemo::NodeManagerBoilerDemo()
: NodeManagerBase("urn:UnifiedAutomation:PLCopen:BoilerDemo"),
  m_bStopThread(false),
  m_pB1_FillLevel(NULL),
  m_pB1_FillLevelSP(NULL),
  m_pB2_FillLevel(NULL),
  m_pB2_FillLevelSP(NULL)
{
}

NodeManagerBoilerDemo::~NodeManagerBoilerDemo()
{
}

UaStatus NodeManagerBoilerDemo::afterStartUp()
{
    UaStatus             ret;
    UaFolder*            pFolder1        = NULL;
    UaDataVariableCache* pDataVariable   = NULL;
    UaVariant            defaultValue;

    //###############################################################################################################
    //###############################################################################################################
    // PLCopen boiler demo example BEGIN

    //##################################################
    // Handle Configuration
    // Create folder for configuration types
    pFolder1 = new UaFolder("Configurations", UaNodeId("Configurations", getNameSpaceIndex()), "");
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlTypes, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pFolder1, OpcUaId_Organizes);

    // Create user specific configuration type -> add type hirarchy and to Configuration folder
    UaObjectTypeSimple* pObjectType = new UaObjectTypeSimple(
        "DefaultConfigurationType",
        UaNodeId("DefaultConfigurationType", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlConfigurationType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);

    // Create configuration instance
    OpcUaPlc::CtrlConfigurationType* pMyConfig = new OpcUaPlc::CtrlConfigurationType(UaNodeId("MyConfiguration", getNameSpaceIndex()), "MyConfiguration", getNameSpaceIndex(), this);
    pMyConfig->setTypeDefinition(UaNodeId("DefaultConfigurationType", getNameSpaceIndex()));
    addNodeAndReference(UaNodeId(OpcUaDiId_DeviceSet, OpcUaDi::NodeManagerDevices::getTypeNamespace()), pMyConfig, OpcUaId_Organizes);

    //##################################################
    // Handle Resource
    // Create folder for resource types
    pFolder1 = new UaFolder("Resources", UaNodeId("Resources", getNameSpaceIndex()), "");
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlTypes, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pFolder1, OpcUaId_Organizes);

    // Create user specific resource type -> add to type hirarchy, to Resources folder and to MyConfiguration.Resources.SupportedTypes
    pObjectType = new UaObjectTypeSimple(
        "UnifiedPlcType",
        UaNodeId("UnifiedPlcType", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlResourceType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    pMyConfig->addResourceType(pObjectType);

    // Create resource instance -> add to MyConfiguration.Resources
    OpcUaPlc::CtrlResourceType* pMyResource = new OpcUaPlc::CtrlResourceType(UaNodeId("PLC1", getNameSpaceIndex()), "PLC1", getNameSpaceIndex(), this);
    pMyResource->setTypeDefinition(UaNodeId("UnifiedPlcType", getNameSpaceIndex()));
    pMyConfig->addResourceInstance(pMyResource);
    addUaReference(OpcUaId_ObjectsFolder, pMyResource->nodeId(), OpcUaId_Organizes);

    //##################################################
    // Handle Program
    OpcUaPlc::CtrlProgramType* pMyProgram = new OpcUaPlc::CtrlProgramType(UaNodeId("MAIN", getNameSpaceIndex()), "MAIN", getNameSpaceIndex(), this);
    pMyResource->addProgramInstance(pMyProgram);

    // Add GlobalVars
    defaultValue.setBool(OpcUa_True);
    pDataVariable = new UaDataVariableCache( "bAutoMode", UaNodeId(".bAutoMode", getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    pMyResource->addGlobalVar(pDataVariable);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rMaxFillLevel", UaNodeId(".rMaxFillLevel", getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    pMyResource->addGlobalVar(pDataVariable);
    pDataVariable = new UaDataVariableCache( "rMaxFeedwaterPressure", UaNodeId(".rMaxFeedwaterPressure", getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    pMyResource->addGlobalVar(pDataVariable);
    pDataVariable = new UaDataVariableCache( "rMaxValveState", UaNodeId(".rMaxValveState", getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    pMyResource->addGlobalVar(pDataVariable);
    defaultValue.setInt32(0);
    pDataVariable = new UaDataVariableCache( "counter", UaNodeId(".counter", getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    pMyResource->addGlobalVar(pDataVariable);

    //##################################################
    // Handle Function Blocks
    // Create folder for FunctionBlock types
    pFolder1 = new UaFolder("FunctionBlocks", UaNodeId("FunctionBlocks", getNameSpaceIndex()), "");
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlTypes, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pFolder1, OpcUaId_Organizes);

    // Create user specific resource type -> add to type hirarchy and to FunctionBlocks folder
    pObjectType = new UaObjectTypeSimple(
        "FB_LevelController",
        UaNodeId("FB_LevelController", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "Measurement", UaNodeId(UaString("%1.Measurement").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Setpoint", UaNodeId(UaString("%1.Setpoint").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_ControlComputation",
        UaNodeId("FB_ControlComputation", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "Input1", UaNodeId(UaString("%1.Input1").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input2", UaNodeId(UaString("%1.Input2").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input3", UaNodeId(UaString("%1.Input3").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rEstimatedFeedwater", UaNodeId(UaString("%1.rEstimatedFeedwater").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowDifference", UaNodeId(UaString("%1.rFlowDifference").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_Valve",
        UaNodeId("FB_Valve", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetValveOpening", UaNodeId(UaString("%1.rSetValveOpening").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetValveOpening", UaNodeId(UaString("%1.rGetValveOpening").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_InputPipe",
        UaNodeId("FB_InputPipe", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rValveOpening", UaNodeId(UaString("%1.rValveOpening").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFeedwaterRate", UaNodeId(UaString("%1.rFeedwaterRate").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    // FB_Valve - Valve1001
    OpcUaPlc::CtrlFunctionBlockType* pValve1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Valve1001").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "Valve1001", getNameSpaceIndex(), this);
    pValve1001->setTypeDefinition(UaNodeId("FB_Valve", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pValve1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetValveOpening", UaNodeId(UaString("%1.rSetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetValveOpening", UaNodeId(UaString("%1.rGetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_FillLevel",
        UaNodeId("FB_FillLevel", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetFillLevel", UaNodeId(UaString("%1.rGetFillLevel").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_Drum",
        UaNodeId("FB_Drum", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rFlowIn", UaNodeId(UaString("%1.rFlowIn").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowOut", UaNodeId(UaString("%1.rFlowOut").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFillLevel", UaNodeId(UaString("%1.rFillLevel").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    // FB_FillLevel - LI1001
    OpcUaPlc::CtrlFunctionBlockType* pLI1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.LI1001").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "LI1001", getNameSpaceIndex(), this);
    pLI1001->setTypeDefinition(UaNodeId("FB_FillLevel", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pLI1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetFillLevel", UaNodeId(UaString("%1.rGetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_SteamFlow",
        UaNodeId("FB_SteamFlow", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetSteamFlow", UaNodeId(UaString("%1.rSetSteamFlow").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetSteamFlow", UaNodeId(UaString("%1.rGetSteamFlow").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_OutputPipe",
        UaNodeId("FB_OutputPipe", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSteamDemand", UaNodeId(UaString("%1.rSteamDemand").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rSteamFlow", UaNodeId(UaString("%1.rSteamFlow").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    // FB_SteamFlow - FT1002
    OpcUaPlc::CtrlFunctionBlockType* pFT1002 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.FT1002").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "FT1002", getNameSpaceIndex(), this);
    pFT1002->setTypeDefinition(UaNodeId("FB_SteamFlow", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pFT1002, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetSteamFlow", UaNodeId(UaString("%1.rSetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetSteamFlow", UaNodeId(UaString("%1.rGetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);

    pObjectType = new UaObjectTypeSimple(
        "FB_Boiler",
        UaNodeId("FB_Boiler", getNameSpaceIndex()),
        "",
        OpcUa_False);
    addNodeAndReference(UaNodeId(OpcUaPlcId_CtrlFunctionBlockType, OpcUaPlc::NodeManagerPLCopen::getTypeNamespace()), pObjectType, OpcUaId_HasSubtype);
    addUaReference(pFolder1, pObjectType, OpcUaId_Organizes);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rSteamDemand", UaNodeId(UaString("%1.rSteamDemand").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pObjectType, pDataVariable, OpcUaId_HasComponent);
    // FB_LevelController - LC1001
    OpcUaPlc::CtrlFunctionBlockType* pLC1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.LC1001").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "LC1001", getNameSpaceIndex(), this);
    pLC1001->setTypeDefinition(UaNodeId("FB_LevelController", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pLC1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "Measurement", UaNodeId(UaString("%1.Measurement").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Setpoint", UaNodeId(UaString("%1.Setpoint").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, pDataVariable, OpcUaId_HasComponent);
    // FB_ControlComputation - CC1001
    OpcUaPlc::CtrlFunctionBlockType* pCC1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.CC1001").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "CC1001", getNameSpaceIndex(), this);
    pCC1001->setTypeDefinition(UaNodeId("FB_ControlComputation", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pCC1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "Input1", UaNodeId(UaString("%1.Input1").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input2", UaNodeId(UaString("%1.Input2").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input3", UaNodeId(UaString("%1.Input3").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rEstimatedFeedwater", UaNodeId(UaString("%1.rEstimatedFeedwater").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowDifference", UaNodeId(UaString("%1.rFlowDifference").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    // FB_InputPipe - Pipe1001
    OpcUaPlc::CtrlFunctionBlockType* pPipe1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Pipe1001").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "Pipe1001", getNameSpaceIndex(), this);
    pPipe1001->setTypeDefinition(UaNodeId("FB_InputPipe", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pPipe1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rValveOpening", UaNodeId(UaString("%1.rValveOpening").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFeedwaterRate", UaNodeId(UaString("%1.rFeedwaterRate").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1001, pDataVariable, OpcUaId_HasComponent);
    // FB_Valve - Valve1001
    pValve1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Valve1001").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), "Valve1001", getNameSpaceIndex(), this);
    pValve1001->setTypeDefinition(UaNodeId("FB_Valve", getNameSpaceIndex()));
    addNodeAndReference(pPipe1001, pValve1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetValveOpening", UaNodeId(UaString("%1.rSetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetValveOpening", UaNodeId(UaString("%1.rGetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    // FB_Drum - Drum1001
    OpcUaPlc::CtrlFunctionBlockType* pDrum1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Drum1001").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "Drum1001", getNameSpaceIndex(), this);
    pDrum1001->setTypeDefinition(UaNodeId("FB_Drum", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pDrum1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rFlowIn", UaNodeId(UaString("%1.rFlowIn").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowOut", UaNodeId(UaString("%1.rFlowOut").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFillLevel", UaNodeId(UaString("%1.rFillLevel").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    // FB_FillLevel - LI1001
    pLI1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.LI1001").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), "LI1001", getNameSpaceIndex(), this);
    pLI1001->setTypeDefinition(UaNodeId("FB_FillLevel", getNameSpaceIndex()));
    addNodeAndReference(pDrum1001, pLI1001, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetFillLevel", UaNodeId(UaString("%1.rGetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    // FB_OutputPipe - Pipe1002
    OpcUaPlc::CtrlFunctionBlockType* pPipe1002 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Pipe1002").arg(pObjectType->nodeId().toString()), getNameSpaceIndex()), "Pipe1002", getNameSpaceIndex(), this);
    pPipe1002->setTypeDefinition(UaNodeId("FB_OutputPipe", getNameSpaceIndex()));
    addNodeAndReference(pObjectType, pPipe1002, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSteamDemand", UaNodeId(UaString("%1.rSteamDemand").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rSteamFlow", UaNodeId(UaString("%1.rSteamFlow").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1002, pDataVariable, OpcUaId_HasComponent);
    // FB_SteamFlow - FT1002
    pFT1002 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.FT1002").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), "FT1002", getNameSpaceIndex(), this);
    pFT1002->setTypeDefinition(UaNodeId("FB_SteamFlow", getNameSpaceIndex()));
    addNodeAndReference(pPipe1002, pFT1002, OpcUaId_HasComponent);
    defaultValue.setFloat(0);
    pDataVariable = new UaDataVariableCache( "rSetSteamFlow", UaNodeId(UaString("%1.rSetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetSteamFlow", UaNodeId(UaString("%1.rGetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);

    // Create FB instance -> add to Program
    OpcUaPlc::CtrlFunctionBlockType* pFbBoiler1 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId("MAIN.fbBoiler1", getNameSpaceIndex()), "fbBoiler1", getNameSpaceIndex(), this);
    pFbBoiler1->setTypeDefinition(UaNodeId("FB_Boiler", getNameSpaceIndex()));
    addNodeAndReference(pMyProgram, pFbBoiler1, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFbBoiler1, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rSteamDemand", UaNodeId(UaString("%1.rSteamDemand").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFbBoiler1, pDataVariable, OpcUaId_HasComponent);
    // FB_LevelController - LC1001
    pLC1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.LC1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "LC1001", getNameSpaceIndex(), this);
    pLC1001->setTypeDefinition(UaNodeId("FB_LevelController", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pLC1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "Measurement", UaNodeId(UaString("%1.Measurement").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, pDataVariable, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)300);
    m_pB1_FillLevelSP = new UaDataVariableCache( "Setpoint", UaNodeId(UaString("%1.Setpoint").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, m_pB1_FillLevelSP, OpcUaId_HasComponent);
    // FB_ControlComputation - CC1001
    pCC1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.CC1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "CC1001", getNameSpaceIndex(), this);
    pCC1001->setTypeDefinition(UaNodeId("FB_ControlComputation", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pCC1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "Input1", UaNodeId(UaString("%1.Input1").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input2", UaNodeId(UaString("%1.Input2").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input3", UaNodeId(UaString("%1.Input3").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rEstimatedFeedwater", UaNodeId(UaString("%1.rEstimatedFeedwater").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowDifference", UaNodeId(UaString("%1.rFlowDifference").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    // FB_InputPipe - Pipe1001
    pPipe1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Pipe1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "Pipe1001", getNameSpaceIndex(), this);
    pPipe1001->setTypeDefinition(UaNodeId("FB_InputPipe", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pPipe1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rValveOpening", UaNodeId(UaString("%1.rValveOpening").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFeedwaterRate", UaNodeId(UaString("%1.rFeedwaterRate").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1001, pDataVariable, OpcUaId_HasComponent);
    // FB_Valve - Valve1001
    pValve1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Valve1001").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), "Valve1001", getNameSpaceIndex(), this);
    pValve1001->setTypeDefinition(UaNodeId("FB_Valve", getNameSpaceIndex()));
    addNodeAndReference(pPipe1001, pValve1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetValveOpening", UaNodeId(UaString("%1.rSetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetValveOpening", UaNodeId(UaString("%1.rGetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    // FB_Drum - Drum1001
    pDrum1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Drum1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "Drum1001", getNameSpaceIndex(), this);
    pDrum1001->setTypeDefinition(UaNodeId("FB_Drum", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pDrum1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rFlowIn", UaNodeId(UaString("%1.rFlowIn").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowOut", UaNodeId(UaString("%1.rFlowOut").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFillLevel", UaNodeId(UaString("%1.rFillLevel").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    // FB_FillLevel - LI1001
    pLI1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.LI1001").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), "LI1001", getNameSpaceIndex(), this);
    pLI1001->setTypeDefinition(UaNodeId("FB_FillLevel", getNameSpaceIndex()));
    addNodeAndReference(pDrum1001, pLI1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetFillLevel", UaNodeId(UaString("%1.rGetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)300);
    m_pB1_FillLevel = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, m_pB1_FillLevel, OpcUaId_HasComponent);
    // FB_OutputPipe - Pipe1002
    pPipe1002 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Pipe1002").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "Pipe1002", getNameSpaceIndex(), this);
    pPipe1002->setTypeDefinition(UaNodeId("FB_OutputPipe", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pPipe1002, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSteamDemand", UaNodeId(UaString("%1.rSteamDemand").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rSteamFlow", UaNodeId(UaString("%1.rSteamFlow").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1002, pDataVariable, OpcUaId_HasComponent);
    // FB_SteamFlow - FT1002
    pFT1002 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.FT1002").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), "FT1002", getNameSpaceIndex(), this);
    pFT1002->setTypeDefinition(UaNodeId("FB_SteamFlow", getNameSpaceIndex()));
    addNodeAndReference(pPipe1002, pFT1002, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetSteamFlow", UaNodeId(UaString("%1.rSetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetSteamFlow", UaNodeId(UaString("%1.rGetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);

    // Create FB instance -> add to Program
    pFbBoiler1 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId("MAIN.fbBoiler2", getNameSpaceIndex()), "fbBoiler2", getNameSpaceIndex(), this);
    pFbBoiler1->setTypeDefinition(UaNodeId("FB_Boiler", getNameSpaceIndex()));
    addNodeAndReference(pMyProgram, pFbBoiler1, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFbBoiler1, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rSteamDemand", UaNodeId(UaString("%1.rSteamDemand").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFbBoiler1, pDataVariable, OpcUaId_HasComponent);
    // FB_LevelController - LC1001
    pLC1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.LC1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "LC1001", getNameSpaceIndex(), this);
    pLC1001->setTypeDefinition(UaNodeId("FB_LevelController", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pLC1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "Measurement", UaNodeId(UaString("%1.Measurement").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, pDataVariable, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)300);
    m_pB2_FillLevelSP = new UaDataVariableCache( "Setpoint", UaNodeId(UaString("%1.Setpoint").arg(pLC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLC1001, m_pB2_FillLevelSP, OpcUaId_HasComponent);
    // FB_ControlComputation - CC1001
    pCC1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.CC1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "CC1001", getNameSpaceIndex(), this);
    pCC1001->setTypeDefinition(UaNodeId("FB_ControlComputation", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pCC1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "Input1", UaNodeId(UaString("%1.Input1").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input2", UaNodeId(UaString("%1.Input2").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "Input3", UaNodeId(UaString("%1.Input3").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "ControlOut", UaNodeId(UaString("%1.ControlOut").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rEstimatedFeedwater", UaNodeId(UaString("%1.rEstimatedFeedwater").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowDifference", UaNodeId(UaString("%1.rFlowDifference").arg(pCC1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pCC1001, pDataVariable, OpcUaId_HasComponent);
    // FB_InputPipe - Pipe1001
    pPipe1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Pipe1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "Pipe1001", getNameSpaceIndex(), this);
    pPipe1001->setTypeDefinition(UaNodeId("FB_InputPipe", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pPipe1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rValveOpening", UaNodeId(UaString("%1.rValveOpening").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFeedwaterRate", UaNodeId(UaString("%1.rFeedwaterRate").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1001, pDataVariable, OpcUaId_HasComponent);
    // FB_Valve - Valve1001
    pValve1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Valve1001").arg(pPipe1001->nodeId().toString()), getNameSpaceIndex()), "Valve1001", getNameSpaceIndex(), this);
    pValve1001->setTypeDefinition(UaNodeId("FB_Valve", getNameSpaceIndex()));
    addNodeAndReference(pPipe1001, pValve1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetValveOpening", UaNodeId(UaString("%1.rSetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetValveOpening", UaNodeId(UaString("%1.rGetValveOpening").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pValve1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pValve1001, pDataVariable, OpcUaId_HasComponent);
    // FB_Drum - Drum1001
    pDrum1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Drum1001").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "Drum1001", getNameSpaceIndex(), this);
    pDrum1001->setTypeDefinition(UaNodeId("FB_Drum", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pDrum1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rFlowIn", UaNodeId(UaString("%1.rFlowIn").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFlowOut", UaNodeId(UaString("%1.rFlowOut").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rFillLevel", UaNodeId(UaString("%1.rFillLevel").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pDrum1001, pDataVariable, OpcUaId_HasComponent);
    // FB_FillLevel - LI1001
    pLI1001 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.LI1001").arg(pDrum1001->nodeId().toString()), getNameSpaceIndex()), "LI1001", getNameSpaceIndex(), this);
    pLI1001->setTypeDefinition(UaNodeId("FB_FillLevel", getNameSpaceIndex()));
    addNodeAndReference(pDrum1001, pLI1001, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetFillLevel", UaNodeId(UaString("%1.rSetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetFillLevel", UaNodeId(UaString("%1.rGetFillLevel").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, pDataVariable, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)300);
    m_pB2_FillLevel = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pLI1001->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pLI1001, m_pB2_FillLevel, OpcUaId_HasComponent);
    // FB_OutputPipe - Pipe1002
    pPipe1002 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.Pipe1002").arg(pFbBoiler1->nodeId().toString()), getNameSpaceIndex()), "Pipe1002", getNameSpaceIndex(), this);
    pPipe1002->setTypeDefinition(UaNodeId("FB_OutputPipe", getNameSpaceIndex()));
    addNodeAndReference(pFbBoiler1, pPipe1002, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSteamDemand", UaNodeId(UaString("%1.rSteamDemand").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rSteamFlow", UaNodeId(UaString("%1.rSteamFlow").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pPipe1002, pDataVariable, OpcUaId_HasComponent);
    // FB_SteamFlow - FT1002
    pFT1002 = new OpcUaPlc::CtrlFunctionBlockType(UaNodeId(UaString("%1.FT1002").arg(pPipe1002->nodeId().toString()), getNameSpaceIndex()), "FT1002", getNameSpaceIndex(), this);
    pFT1002->setTypeDefinition(UaNodeId("FB_SteamFlow", getNameSpaceIndex()));
    addNodeAndReference(pPipe1002, pFT1002, OpcUaId_HasComponent);
    defaultValue.setFloat((OpcUa_Float)4.2);
    pDataVariable = new UaDataVariableCache( "rSetSteamFlow", UaNodeId(UaString("%1.rSetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "rGetSteamFlow", UaNodeId(UaString("%1.rGetSteamFlow").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    pDataVariable = new UaDataVariableCache( "DataItem", UaNodeId(UaString("%1.DataItem").arg(pFT1002->nodeId().toString()), getNameSpaceIndex()), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, m_defaultLocaleId);
    addNodeAndReference(pFT1002, pDataVariable, OpcUaId_HasComponent);
    // PLCopen example END
    //###############################################################################################################
    //###############################################################################################################
    
    // Start worker thread for simulation
    start();

    return ret;
}

UaStatus NodeManagerBoilerDemo::beforeShutDown()
{
    UaStatus ret;

    // Stop worker thread and wait for threat termination
    m_bStopThread = true;
    wait();

    return ret;
}

// Main method of worker thread simulating values
void NodeManagerBoilerDemo::run()
{
    UaDataValue      tempDataValue;
    UaVariant        tempValue;
    OpcUa_Float      tempFloat;
    OpcUa_Float      tempSetPoint;
    OpcUa_Float      tempChange;

    while( m_bStopThread == false )
    {
        //-------------------------------------------------
        // Simulate fill level boiler 1
        tempDataValue = m_pB1_FillLevel->value(NULL);
        tempValue = *tempDataValue.value();
        tempValue.toFloat(tempFloat);
        tempDataValue = m_pB1_FillLevelSP->value(NULL);
        tempValue = *tempDataValue.value();
        tempValue.toFloat(tempSetPoint);
        if ( tempFloat > tempSetPoint )
        {
            tempChange = (tempFloat - tempSetPoint)/8;
            if ( tempChange < 5 ) tempChange = 5;
            tempFloat = tempFloat - tempChange;
        }
        else
        {
            tempChange = (tempSetPoint - tempFloat)/6;
            if ( tempChange < 4 ) tempChange = 4;
            tempFloat = tempFloat + tempChange;
        }
        tempValue.setFloat(tempFloat);
        tempDataValue.setValue(tempValue, OpcUa_True, OpcUa_True);
        m_pB1_FillLevel->setValue(NULL, tempDataValue, OpcUa_False);
        //-------------------------------------------------

        //-------------------------------------------------
        // Simulate fill level boiler 2
        tempDataValue = m_pB2_FillLevel->value(NULL);
        tempValue = *tempDataValue.value();
        tempValue.toFloat(tempFloat);
        tempDataValue = m_pB2_FillLevelSP->value(NULL);
        tempValue = *tempDataValue.value();
        tempValue.toFloat(tempSetPoint);
        if ( tempFloat > tempSetPoint )
        {
            tempChange = (tempFloat - tempSetPoint)/7;
            if ( tempChange < 6 ) tempChange = 6;
            tempFloat = tempFloat - tempChange;
        }
        else
        {
            tempChange = (tempSetPoint - tempFloat)/9;
            if ( tempChange < 5 ) tempChange = 5;
            tempFloat = tempFloat + tempChange;
        }
        tempValue.setFloat(tempFloat);
        tempDataValue.setValue(tempValue, OpcUa_True, OpcUa_True);
        m_pB2_FillLevel->setValue(NULL, tempDataValue, OpcUa_False);
        //-------------------------------------------------

        msleep(500);
    }
}
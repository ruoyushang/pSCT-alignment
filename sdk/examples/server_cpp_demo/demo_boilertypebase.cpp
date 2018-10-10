/******************************************************************************
** demo_boilertypebase.cpp
**
** Copyright (c) 2006-2015 Unified Automation GmbH All rights reserved.
**
** Software License Agreement ("SLA") Version 2.5
**
** Unless explicitly acquired and licensed from Licensor under another
** license, the contents of this file are subject to the Software License
** Agreement ("SLA") Version 2.5, or subsequent versions
** as allowed by the SLA, and You may not copy or use this file in either
** source code or executable form, except in compliance with the terms and
** conditions of the SLA.
**
** All software distributed under the SLA is provided strictly on an
** "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
** AND LICENSOR HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
** LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
** PURPOSE, QUIET ENJOYMENT, OR NON-INFRINGEMENT. See the SLA for specific
** language governing rights and limitations under the SLA.
**
** The complete license agreement can be found here:
** http://unifiedautomation.com/License/SLA/2.5/
**
** Project: C++ OPC Server SDK information model for namespace http://www.unifiedautomation.com/DemoServer/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#include "demo_boilertypebase.h"
#include "demo_boilertype.h"
#include "uagenericnodes.h"
#include "nodemanagerroot.h"
#include "methodhandleuanode.h"
#include "demo_nodemanagerdemo.h"

#include "demo_filllevelsensortype.h"
#include "demo_instancefactory_demo.h"
#include "demo_temperaturesensortype.h"
#include "instancefactory.h"
// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

bool BoilerTypeBase::s_typeNodesCreated = false;
OpcUa::BaseMethod* BoilerTypeBase::s_pFillMethod = NULL;
OpcUa::BaseMethod* BoilerTypeBase::s_pHeatMethod = NULL;
Demo::FillLevelSensorType* BoilerTypeBase::s_pFillLevelSensor = NULL;
Demo::TemperatureSensorType* BoilerTypeBase::s_pTemperatureSensor = NULL;
OpcUa::BaseDataVariableType* BoilerTypeBase::s_pFillLevelSetPoint = NULL;
OpcUa::BaseDataVariableType* BoilerTypeBase::s_pHeaterStatus = NULL;
OpcUa::BaseDataVariableType* BoilerTypeBase::s_pTemperatureSetPoint = NULL;

/** Constructs an BoilerType object using an instance declaration node as base
*/
BoilerTypeBase::BoilerTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, pInstanceDeclarationObject, pNodeConfig, pSharedMutex)
{
    initialize();
}

/** Constructs an BoilerType object
*/
BoilerTypeBase::BoilerTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, name, browseNameNameSpaceIndex, pNodeConfig, pSharedMutex)
{
    initialize();
}

/**  Constructs an instance of the class BoilerTypeBase with all components
*/
BoilerTypeBase::BoilerTypeBase(
    UaBase::Object*    pBaseNode,    //!< [in] The "tree" of nodes that specifies the attributes of the instance and its children
    XmlUaNodeFactoryManager* pFactory, //!< [in] The factory to create the children
    NodeManagerConfig* pNodeConfig,  //!< [in] Interface pointer to the NodeManagerConfig interface used to add and delete node and references in the address space
    UaMutexRefCounted* pSharedMutex) //!< [in] Shared mutex object used to synchronize access to the variable. Can be NULL if no shared mutex is provided
: OpcUa::BaseObjectType(pBaseNode, pFactory, pNodeConfig, pSharedMutex)
{
    UaStatus      addStatus;

    if ( s_typeNodesCreated == false )
    {
        createTypes();
    }

    m_pFillLevelSensor = NULL;
    m_pTemperatureSensor = NULL;
    m_pFillLevelSetPoint = NULL;
    m_pHeaterStatus = NULL;
    m_pTemperatureSetPoint = NULL;
    m_pFillMethod = NULL;
    m_pHeatMethod = NULL;

    std::list<UaBase::BaseNode*> lstReferencedNodes = pBaseNode->hierarchicallyReferencedNodes();
    for (std::list<UaBase::BaseNode*>::const_iterator it = lstReferencedNodes.begin(); it != lstReferencedNodes.end(); it++)
    {
        UaBase::BaseNode *pChild = *it;
        if (pChild->browseName() == UaQualifiedName("FillLevelSetPoint", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pFillLevelSetPoint)
            {
                m_pFillLevelSetPoint = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pFillLevelSetPoint, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Variable*)pChild)->modellingRuleId().isNull())
                {
                    m_pFillLevelSetPoint->setModellingRuleId(((UaBase::Variable*)pChild)->modellingRuleId());
                }
            }
        }
        else if (pChild->browseName() == UaQualifiedName("HeaterStatus", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pHeaterStatus)
            {
                m_pHeaterStatus = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pHeaterStatus, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Variable*)pChild)->modellingRuleId().isNull())
                {
                    m_pHeaterStatus->setModellingRuleId(((UaBase::Variable*)pChild)->modellingRuleId());
                }
            }
        }
        else if (pChild->browseName() == UaQualifiedName("TemperatureSetPoint", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pTemperatureSetPoint)
            {
                m_pTemperatureSetPoint = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pTemperatureSetPoint, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Variable*)pChild)->modellingRuleId().isNull())
                {
                    m_pTemperatureSetPoint->setModellingRuleId(((UaBase::Variable*)pChild)->modellingRuleId());
                }
            }
        }
        else if (pChild->browseName() == UaQualifiedName("FillLevelSensor", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pFillLevelSensor)
            {
                m_pFillLevelSensor = (Demo::FillLevelSensorType*) pFactory->createObject((UaBase::Object*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pFillLevelSensor, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Object*)pChild)->modellingRuleId().isNull())
                {
                    m_pFillLevelSensor->setModellingRuleId(((UaBase::Object*)pChild)->modellingRuleId());
                }
            }
        }
        else if (pChild->browseName() == UaQualifiedName("TemperatureSensor", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pTemperatureSensor)
            {
                m_pTemperatureSensor = (Demo::TemperatureSensorType*) pFactory->createObject((UaBase::Object*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pTemperatureSensor, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Object*)pChild)->modellingRuleId().isNull())
                {
                    m_pTemperatureSensor->setModellingRuleId(((UaBase::Object*)pChild)->modellingRuleId());
                }
            }
        }
        else if (s_pFillMethod && pChild->browseName() == s_pFillMethod->browseName())
        {
            if (!m_pFillMethod)
            {
                m_pFillMethod = (OpcUa::BaseMethod*) pFactory->createMethod((UaBase::Method*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pFillMethod, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Method*)pChild)->modellingRuleId().isNull())
                {
                    m_pFillMethod->setModellingRuleId(((UaBase::Method*)pChild)->modellingRuleId());
                }
            }
        }
        else if (s_pHeatMethod && pChild->browseName() == s_pHeatMethod->browseName())
        {
            if (!m_pHeatMethod)
            {
                m_pHeatMethod = (OpcUa::BaseMethod*) pFactory->createMethod((UaBase::Method*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pHeatMethod, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Method*)pChild)->modellingRuleId().isNull())
                {
                    m_pHeatMethod->setModellingRuleId(((UaBase::Method*)pChild)->modellingRuleId());
                }
            }
        }
    }
    UA_ASSERT(m_pFillLevelSensor);
    UA_ASSERT(m_pTemperatureSensor);
    UA_ASSERT(m_pFillLevelSetPoint);
    UA_ASSERT(m_pHeaterStatus);
    UA_ASSERT(m_pTemperatureSetPoint);
    UA_ASSERT(m_pFillMethod);
    UA_ASSERT(m_pHeatMethod);
}

/** Initialize the object with all member nodes
*/
void BoilerTypeBase::initialize()
{
    OpcUa_Int16 nsIdx = m_pNodeConfig->getNameSpaceIndex();
    UaStatus      addStatus;
    UaVariant     defaultValue;
    UaUInt32Array nullarray;
    UaPropertyMethodArgument* pPropertyArg = NULL;

    if ( s_typeNodesCreated == false )
    {
        createTypes();
    }
    // Object FillLevelSensor
    m_pFillLevelSensor = new Demo::FillLevelSensorType(UaNodeId(UaString("%1.FillLevelSensor").arg(nodeId().toString()), nsIdx), s_pFillLevelSensor, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pFillLevelSensor, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Mandatory variable FillLevelSetPoint
    m_pFillLevelSetPoint = new OpcUa::BaseDataVariableType(this, s_pFillLevelSetPoint, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pFillLevelSetPoint, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Mandatory variable HeaterStatus
    m_pHeaterStatus = new OpcUa::BaseDataVariableType(this, s_pHeaterStatus, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pHeaterStatus, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    m_pHeaterStatus->setDataType(UaNodeId(DemoId_HeaterStatus, Demo::NodeManagerDemo::getTypeNamespace()));

    // Object TemperatureSensor
    m_pTemperatureSensor = new Demo::TemperatureSensorType(UaNodeId(UaString("%1.TemperatureSensor").arg(nodeId().toString()), nsIdx), s_pTemperatureSensor, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pTemperatureSensor, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Mandatory variable TemperatureSetPoint
    m_pTemperatureSetPoint = new OpcUa::BaseDataVariableType(this, s_pTemperatureSetPoint, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pTemperatureSetPoint, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    // Method Fill
    m_pFillMethod = new OpcUa::BaseMethod(this, s_pFillMethod, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pFillMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // ------------------------------------------------------------------------
    // Add InputArguments
    // Create property
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(UaString("%1.Fill.InputArguments").arg(nodeId().toString()), nsIdx), // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        1,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument SetPoint
    pPropertyArg->setArgument(
        0,                      // Index of the argument
        "SetPoint",             // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                     // Array rank of the argument
        nullarray,        // Array dimension
        UaLocalizedText("en-US", "FillLevel Setpoint [l]")); // Description
    // Add property to method
    addStatus = m_pNodeConfig->addNodeAndReference(m_pFillMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // Method Heat
    m_pHeatMethod = new OpcUa::BaseMethod(this, s_pHeatMethod, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pHeatMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // ------------------------------------------------------------------------
    // Add InputArguments
    // Create property
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(UaString("%1.Heat.InputArguments").arg(nodeId().toString()), nsIdx), // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        1,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument SetPoint
    pPropertyArg->setArgument(
        0,                      // Index of the argument
        "SetPoint",             // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                     // Array rank of the argument
        nullarray,        // Array dimension
        UaLocalizedText("en-US", "Temperature Setpoint [\302\260C]")); // Description
    // Add property to method
    addStatus = m_pNodeConfig->addNodeAndReference(m_pHeatMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

}

/** Destruction
*/
BoilerTypeBase::~BoilerTypeBase()
{
}

/** Create the related type nodes
*/
void BoilerTypeBase::createTypes()
{
    if ( s_typeNodesCreated == false )
    {
        s_typeNodesCreated = true;

        UaStatus      addStatus;
        UaVariant     defaultValue;
        UaUInt32Array nullarray;
        UaPropertyMethodArgument* pPropertyArg = NULL;
        NodeManagerRoot* pNodeManagerRoot = NodeManagerRoot::CreateRootNodeManager();
        OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
        OpcUa_Int16 nsSuperTypeIdx = NodeManagerRoot::getTypeNamespace();
        NodeManagerConfig* pTypeNodeConfig = pNodeManagerRoot->getNodeManagerByNamespace(nsTypeIdx)->getNodeManagerConfig();

        OpcUa::GenericObjectType* pObjectType;
        pObjectType = new OpcUa::GenericObjectType(
            UaNodeId(DemoId_BoilerType, nsTypeIdx),
            UaQualifiedName("BoilerType", nsTypeIdx),
            UaLocalizedText("", "BoilerType"),
            UaLocalizedText("", ""),
            OpcUa_False,
            &BoilerType::clearStaticMembers);
        pTypeNodeConfig->addNodeAndReference(UaNodeId(OpcUaId_BaseObjectType, nsSuperTypeIdx), pObjectType, OpcUaId_HasSubtype);
        {
            std::list<UaBase::BaseNode*> lstBaseNodes;
            UaBase::Object *pChild = new UaBase::Object(UaNodeId(DemoId_BoilerType_FillLevelSensor, nsTypeIdx));
            lstBaseNodes.push_back(pChild);
            pChild->setBrowseName(UaQualifiedName("FillLevelSensor", Demo::NodeManagerDemo::getTypeNamespace()));
            pChild->setDisplayName(UaLocalizedText("", "FillLevelSensor"));
            {
                UaBase::BaseNode *pParent = pChild;
                UaBase::BaseNode *pChild2;
                UaBase::FullReference reference;
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_FillLevelSensor_FillLevel, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("FillLevel", Demo::NodeManagerDemo::getTypeNamespace()));
                pChild2->setDisplayName(UaLocalizedText("", "FillLevel"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(3);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(3);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Double);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_AnalogItemType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_FillLevelSensor_FillLevel_Definition, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Definition", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Definition"));
                    pChild3->setDescription(UaLocalizedText("", "A vendor-specific, human readable string that specifies how the value of this DataItem is calculated."));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_String);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_FillLevelSensor_FillLevel_EngineeringUnits, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("EngineeringUnits", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "EngineeringUnits"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_EUInformation);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_FillLevelSensor_FillLevel_EURange, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("EURange", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "EURange"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Range);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_FillLevelSensor_FillLevel_InstrumentRange, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InstrumentRange", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InstrumentRange"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Range);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_FillLevelSensor_FillLevel_ValuePrecision, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("ValuePrecision", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "ValuePrecision"));
                    pChild3->setDescription(UaLocalizedText("", "The maximum precision that the server can maintain for the item based on restrictions in the target environment."));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Double);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
            }

            XmlUaNodeFactoryManager factory;
        Demo::InstanceFactoryDemo factoryDemo(Demo::NodeManagerDemo::getTypeNamespace());
        factory.addNamespace(&factoryDemo);

            s_pFillLevelSensor = new Demo::FillLevelSensorType(pChild, &factory, pTypeNodeConfig, NULL);
            addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pFillLevelSensor, OpcUaId_HasComponent);
            UA_ASSERT(addStatus.isGood());
            s_pFillLevelSensor->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
            std::list<UaBase::BaseNode*>::iterator it = lstBaseNodes.begin();
            while (it != lstBaseNodes.end())
            {
                delete *it;
                it++;
            }
        }

        {
            std::list<UaBase::BaseNode*> lstBaseNodes;
            UaBase::Object *pChild = new UaBase::Object(UaNodeId(DemoId_BoilerType_TemperatureSensor, nsTypeIdx));
            lstBaseNodes.push_back(pChild);
            pChild->setBrowseName(UaQualifiedName("TemperatureSensor", Demo::NodeManagerDemo::getTypeNamespace()));
            pChild->setDisplayName(UaLocalizedText("", "TemperatureSensor"));
            {
                UaBase::BaseNode *pParent = pChild;
                UaBase::BaseNode *pChild2;
                UaBase::FullReference reference;
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_TemperatureSensor_Temperature, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Temperature", Demo::NodeManagerDemo::getTypeNamespace()));
                pChild2->setDisplayName(UaLocalizedText("", "Temperature"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(3);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(3);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Double);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_AnalogItemType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_TemperatureSensor_Temperature_Definition, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Definition", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Definition"));
                    pChild3->setDescription(UaLocalizedText("", "A vendor-specific, human readable string that specifies how the value of this DataItem is calculated."));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_String);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_TemperatureSensor_Temperature_EngineeringUnits, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("EngineeringUnits", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "EngineeringUnits"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_EUInformation);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_TemperatureSensor_Temperature_EURange, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("EURange", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "EURange"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Range);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_TemperatureSensor_Temperature_InstrumentRange, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InstrumentRange", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InstrumentRange"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Range);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_BoilerType_TemperatureSensor_Temperature_ValuePrecision, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("ValuePrecision", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "ValuePrecision"));
                    pChild3->setDescription(UaLocalizedText("", "The maximum precision that the server can maintain for the item based on restrictions in the target environment."));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Double);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
            }

            XmlUaNodeFactoryManager factory;
        Demo::InstanceFactoryDemo factoryDemo(Demo::NodeManagerDemo::getTypeNamespace());
        factory.addNamespace(&factoryDemo);

            s_pTemperatureSensor = new Demo::TemperatureSensorType(pChild, &factory, pTypeNodeConfig, NULL);
            addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pTemperatureSensor, OpcUaId_HasComponent);
            UA_ASSERT(addStatus.isGood());
            s_pTemperatureSensor->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
            std::list<UaBase::BaseNode*>::iterator it = lstBaseNodes.begin();
            while (it != lstBaseNodes.end())
            {
                delete *it;
                it++;
            }
        }

        // ------------------------------------------------------------------------
        // Method Fill
        // ------------------------------------------------------------------------
        s_pFillMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_BoilerType_Fill, nsTypeIdx), "Fill", Demo::NodeManagerDemo::getTypeNamespace());
        s_pFillMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pFillMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
        // ------------------------------------------------------------------------
        // Add InputArguments
        // Create property
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(DemoId_BoilerType_Fill_InputArguments, nsTypeIdx),       // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            1,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        // Argument SetPoint
        pPropertyArg->setArgument(
            0,                      // Index of the argument
            "SetPoint",             // Name of the argument
            UaNodeId(OpcUaId_Double),// Data type of the argument
            -1,                     // Array rank of the argument
            nullarray,        // Array dimension
            UaLocalizedText("en-US", "FillLevel Setpoint [l]")); // Description
        // Add property to method
        pTypeNodeConfig->addNodeAndReference(s_pFillMethod, pPropertyArg, OpcUaId_HasProperty);

        // ------------------------------------------------------------------------
        // Method Heat
        // ------------------------------------------------------------------------
        s_pHeatMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_BoilerType_Heat, nsTypeIdx), "Heat", Demo::NodeManagerDemo::getTypeNamespace());
        s_pHeatMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pHeatMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
        // ------------------------------------------------------------------------
        // Add InputArguments
        // Create property
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(DemoId_BoilerType_Heat_InputArguments, nsTypeIdx),       // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            1,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        // Argument SetPoint
        pPropertyArg->setArgument(
            0,                      // Index of the argument
            "SetPoint",             // Name of the argument
            UaNodeId(OpcUaId_Double),// Data type of the argument
            -1,                     // Array rank of the argument
            nullarray,        // Array dimension
            UaLocalizedText("en-US", "Temperature Setpoint [\302\260C]")); // Description
        // Add property to method
        pTypeNodeConfig->addNodeAndReference(s_pHeatMethod, pPropertyArg, OpcUaId_HasProperty);


        // Mandatory variable FillLevelSetPoint
        defaultValue.setDouble(0.0);
        s_pFillLevelSetPoint = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_BoilerType_FillLevelSetPoint, nsTypeIdx), "FillLevelSetPoint", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 1, pTypeNodeConfig);
        s_pFillLevelSetPoint->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pFillLevelSetPoint->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pFillLevelSetPoint, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable HeaterStatus
        defaultValue.setInt32((OpcUa_Int32) Demo::HeaterStatus_Off);
        s_pHeaterStatus = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_BoilerType_HeaterStatus, nsTypeIdx), "HeaterStatus", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 1, pTypeNodeConfig);
        s_pHeaterStatus->setDataType(UaNodeId(DemoId_HeaterStatus, Demo::NodeManagerDemo::getTypeNamespace()));
        s_pHeaterStatus->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pHeaterStatus->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pHeaterStatus, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable TemperatureSetPoint
        defaultValue.setDouble(0.0);
        s_pTemperatureSetPoint = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_BoilerType_TemperatureSetPoint, nsTypeIdx), "TemperatureSetPoint", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 1, pTypeNodeConfig);
        s_pTemperatureSetPoint->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pTemperatureSetPoint->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pTemperatureSetPoint, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

    }
}

/** Clear the static members of the class
*/
void BoilerTypeBase::clearStaticMembers()
{
    s_typeNodesCreated = false;

    s_pFillLevelSensor = NULL;
    s_pTemperatureSensor = NULL;
    s_pFillLevelSetPoint = NULL;
    s_pHeaterStatus = NULL;
    s_pTemperatureSetPoint = NULL;
}

/** Returns the type definition NodeId for the BoilerType
*/
UaNodeId BoilerTypeBase::typeDefinitionId() const
{
    UaNodeId ret(DemoId_BoilerType, NodeManagerDemo::getTypeNamespace());
    return ret;
}

/** Implementation of the MethodManager interface method beginCall
*/
UaStatus BoilerTypeBase::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    UaStatus            ret;

    OpcUa::MethodCallJob* pCallJob = new OpcUa::MethodCallJob;
    pCallJob->initialize(this, pCallback, serviceContext, callbackHandle, pMethodHandle, inputArguments);
    ret = NodeManagerRoot::CreateRootNodeManager()->pServerManager()->getThreadPool()->addJob(pCallJob);
    if ( ret.isBad() )
    {
        delete pCallJob;
    }

    return ret;
}

/** Synchronous version of the MethodManager method call executed in a worker thread
*/
UaStatus BoilerTypeBase::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    MethodHandleUaNode* pMethodHandleUaNode = (MethodHandleUaNode*)pMethodHandle;
    UaMethod*           pMethod             = NULL;

    if (pMethodHandleUaNode)
    {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if (pMethod)
        {
            if ( ( (m_pFillMethod != NULL) && (pMethod->nodeId() == m_pFillMethod->nodeId()) )
                 || ( (s_pFillMethod != NULL) && (pMethod->nodeId() == s_pFillMethod->nodeId()) ) )
            {
                if ( inputArguments.length() < 1 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else if ( inputArguments.length() > 1 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    inputArgumentResults.create(1);

                    UaVariant value;
                    UaStatus tmpRet;

                    OpcUa_Double SetPoint;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toDouble(SetPoint);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->Fill(serviceContext, SetPoint);
                    }
                }

            }
            else if ( ( (m_pHeatMethod != NULL) && (pMethod->nodeId() == m_pHeatMethod->nodeId()) )
                      || ( (s_pHeatMethod != NULL) && (pMethod->nodeId() == s_pHeatMethod->nodeId()) ) )
            {
                if ( inputArguments.length() < 1 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else if ( inputArguments.length() > 1 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    inputArgumentResults.create(1);

                    UaVariant value;
                    UaStatus tmpRet;

                    OpcUa_Double SetPoint;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toDouble(SetPoint);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->Heat(serviceContext, SetPoint);
                    }
                }

            }
            else
            {
                return BaseObjectType::call(serviceContext, pMethodHandle, inputArguments, outputArguments, inputArgumentResults, inputArgumentDiag);
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

/**
 *  Sets the FillLevelSetPoint value
 */
void BoilerTypeBase::setFillLevelSetPoint(OpcUa_Double FillLevelSetPoint)
{
    UaVariant value;
    value.setDouble(FillLevelSetPoint);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pFillLevelSetPoint->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of FillLevelSetPoint
 */
OpcUa_Double BoilerTypeBase::getFillLevelSetPoint() const
{
    UaVariant defaultValue;
    OpcUa_Double ret = 0;
    UaDataValue dataValue(m_pFillLevelSetPoint->value(NULL));
    defaultValue = *dataValue.value();
    defaultValue.toDouble(ret);
    return ret;
}

/**
 *  Sets the HeaterStatus value
 */
void BoilerTypeBase::setHeaterStatus(Demo::HeaterStatus HeaterStatus)
{
    UaVariant value;
    value.setInt32(HeaterStatus);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pHeaterStatus->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of HeaterStatus
 */
Demo::HeaterStatus BoilerTypeBase::getHeaterStatus() const
{
    UaVariant defaultValue;
    Demo::HeaterStatus ret = Demo::HeaterStatus_Off;
    UaDataValue dataValue(m_pHeaterStatus->value(NULL));
    defaultValue = *dataValue.value();
    OpcUa_Int32 i = (OpcUa_Int32) Demo::HeaterStatus_Off;
    defaultValue.toInt32(i);
    ret = (Demo::HeaterStatus) i;
    return ret;
}

/**
 *  Sets the TemperatureSetPoint value
 */
void BoilerTypeBase::setTemperatureSetPoint(OpcUa_Double TemperatureSetPoint)
{
    UaVariant value;
    value.setDouble(TemperatureSetPoint);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pTemperatureSetPoint->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of TemperatureSetPoint
 */
OpcUa_Double BoilerTypeBase::getTemperatureSetPoint() const
{
    UaVariant defaultValue;
    OpcUa_Double ret = 0;
    UaDataValue dataValue(m_pTemperatureSetPoint->value(NULL));
    defaultValue = *dataValue.value();
    defaultValue.toDouble(ret);
    return ret;
}

/** Returns the FillLevelSensor node.
 */
Demo::FillLevelSensorType* BoilerTypeBase::getFillLevelSensor()
{
    return m_pFillLevelSensor;
}

/** Returns the FillLevelSetPoint node.
 */
OpcUa::BaseDataVariableType* BoilerTypeBase::getFillLevelSetPointNode()
{
    return m_pFillLevelSetPoint;
}

/** Returns the HeaterStatus node.
 */
OpcUa::BaseDataVariableType* BoilerTypeBase::getHeaterStatusNode()
{
    return m_pHeaterStatus;
}

/** Returns the TemperatureSensor node.
 */
Demo::TemperatureSensorType* BoilerTypeBase::getTemperatureSensor()
{
    return m_pTemperatureSensor;
}

/** Returns the TemperatureSetPoint node.
 */
OpcUa::BaseDataVariableType* BoilerTypeBase::getTemperatureSetPointNode()
{
    return m_pTemperatureSetPoint;
}

/** Returns the Fill node.
  */
OpcUa::BaseMethod* BoilerTypeBase::getFill()
{
    return m_pFillMethod;
}
/** Returns the Heat node.
  */
OpcUa::BaseMethod* BoilerTypeBase::getHeat()
{
    return m_pHeatMethod;
}

/** Applys the NodeAccessInfo set at the BoilerTypeBase and its children. */
void BoilerTypeBase::useAccessInfoFromType()
{
    OpcUa::BaseObjectType::useAccessInfoFromType();

    // instance node
    OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
    UaNode *pTypeNode = NodeManagerRoot::CreateRootNodeManager()->getNode(UaNodeId(DemoId_BoilerType, nsTypeIdx));
    if (pTypeNode)
    {
        useAccessInfoFrom(pTypeNode);
        pTypeNode->releaseReference();
    }

    // children
    m_pFillLevelSensor->useAccessInfoFromInstance(s_pFillLevelSensor);
    m_pTemperatureSensor->useAccessInfoFromInstance(s_pTemperatureSensor);
    m_pFillLevelSetPoint->useAccessInfoFromInstance(s_pFillLevelSetPoint);
    m_pHeaterStatus->useAccessInfoFromInstance(s_pHeaterStatus);
    m_pTemperatureSetPoint->useAccessInfoFromInstance(s_pTemperatureSetPoint);
#if SUPPORT_Method_Server_Facet
    m_pFillMethod->useAccessInfoFrom(s_pFillMethod);
    m_pHeatMethod->useAccessInfoFrom(s_pHeatMethod);
#endif // SUPPORT_Method_Server_Facet
}

/** Uses the NodeAccessInfo of pOther and its children. */
void BoilerTypeBase::useAccessInfoFromInstance(BoilerTypeBase *pOther)
{
    OpcUa::BaseObjectType::useAccessInfoFromInstance(pOther);

    // children
    m_pFillLevelSensor->useAccessInfoFromInstance(pOther->m_pFillLevelSensor);
    m_pTemperatureSensor->useAccessInfoFromInstance(pOther->m_pTemperatureSensor);
    m_pFillLevelSetPoint->useAccessInfoFromInstance(pOther->m_pFillLevelSetPoint);
    m_pHeaterStatus->useAccessInfoFromInstance(pOther->m_pHeaterStatus);
    m_pTemperatureSetPoint->useAccessInfoFromInstance(pOther->m_pTemperatureSetPoint);
#if SUPPORT_Method_Server_Facet
    m_pFillMethod->useAccessInfoFrom(pOther->m_pFillMethod);
    m_pHeatMethod->useAccessInfoFrom(pOther->m_pHeatMethod);
#endif // SUPPORT_Method_Server_Facet
}

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/



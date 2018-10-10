/******************************************************************************
** demo_machinetypebase.cpp
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

#include "demo_machinetypebase.h"
#include "demo_machinetype.h"
#include "uagenericnodes.h"
#include "nodemanagerroot.h"
#include "methodhandleuanode.h"
#include "demo_nodemanagerdemo.h"

#include "demo_instancefactory_demo.h"
#include "demo_temperaturesensortype.h"
#include "instancefactory.h"
// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

bool MachineTypeBase::s_typeNodesCreated = false;
Demo::TemperatureSensorType* MachineTypeBase::s_pTemperatureSensor = NULL;
OpcUa::BaseDataVariableType* MachineTypeBase::s_pHeaterSwitch = NULL;

/** Constructs an MachineType object using an instance declaration node as base
*/
MachineTypeBase::MachineTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, pInstanceDeclarationObject, pNodeConfig, pSharedMutex)
{
    initialize();
}

/** Constructs an MachineType object
*/
MachineTypeBase::MachineTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, name, browseNameNameSpaceIndex, pNodeConfig, pSharedMutex)
{
    initialize();
}

/**  Constructs an instance of the class MachineTypeBase with all components
*/
MachineTypeBase::MachineTypeBase(
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

    m_pTemperatureSensor = NULL;
    m_pHeaterSwitch = NULL;

    std::list<UaBase::BaseNode*> lstReferencedNodes = pBaseNode->hierarchicallyReferencedNodes();
    for (std::list<UaBase::BaseNode*>::const_iterator it = lstReferencedNodes.begin(); it != lstReferencedNodes.end(); it++)
    {
        UaBase::BaseNode *pChild = *it;
        if (pChild->browseName() == UaQualifiedName("HeaterSwitch", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pHeaterSwitch)
            {
                m_pHeaterSwitch = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pHeaterSwitch, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Variable*)pChild)->modellingRuleId().isNull())
                {
                    m_pHeaterSwitch->setModellingRuleId(((UaBase::Variable*)pChild)->modellingRuleId());
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
    }
    UA_ASSERT(m_pTemperatureSensor);
    UA_ASSERT(m_pHeaterSwitch);
}

/** Initialize the object with all member nodes
*/
void MachineTypeBase::initialize()
{
    OpcUa_Int16 nsIdx = m_pNodeConfig->getNameSpaceIndex();
    UaStatus      addStatus;
    UaVariant     defaultValue;

    if ( s_typeNodesCreated == false )
    {
        createTypes();
    }
    // Mandatory variable HeaterSwitch
    m_pHeaterSwitch = new OpcUa::BaseDataVariableType(this, s_pHeaterSwitch, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pHeaterSwitch, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Object TemperatureSensor
    m_pTemperatureSensor = new Demo::TemperatureSensorType(UaNodeId(UaString("%1.TemperatureSensor").arg(nodeId().toString()), nsIdx), s_pTemperatureSensor, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pTemperatureSensor, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

}

/** Destruction
*/
MachineTypeBase::~MachineTypeBase()
{
}

/** Create the related type nodes
*/
void MachineTypeBase::createTypes()
{
    if ( s_typeNodesCreated == false )
    {
        s_typeNodesCreated = true;

        UaStatus      addStatus;
        UaVariant     defaultValue;
        NodeManagerRoot* pNodeManagerRoot = NodeManagerRoot::CreateRootNodeManager();
        OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
        OpcUa_Int16 nsSuperTypeIdx = NodeManagerRoot::getTypeNamespace();
        NodeManagerConfig* pTypeNodeConfig = pNodeManagerRoot->getNodeManagerByNamespace(nsTypeIdx)->getNodeManagerConfig();

        OpcUa::GenericObjectType* pObjectType;
        pObjectType = new OpcUa::GenericObjectType(
            UaNodeId(DemoId_MachineType, nsTypeIdx),
            UaQualifiedName("MachineType", nsTypeIdx),
            UaLocalizedText("", "MachineType"),
            UaLocalizedText("", ""),
            OpcUa_False,
            &MachineType::clearStaticMembers);
        pTypeNodeConfig->addNodeAndReference(UaNodeId(OpcUaId_BaseObjectType, nsSuperTypeIdx), pObjectType, OpcUaId_HasSubtype);
        {
            std::list<UaBase::BaseNode*> lstBaseNodes;
            UaBase::Object *pChild = new UaBase::Object(UaNodeId(DemoId_MachineType_TemperatureSensor, nsTypeIdx));
            lstBaseNodes.push_back(pChild);
            pChild->setBrowseName(UaQualifiedName("TemperatureSensor", Demo::NodeManagerDemo::getTypeNamespace()));
            pChild->setDisplayName(UaLocalizedText("", "TemperatureSensor"));
            {
                UaBase::BaseNode *pParent = pChild;
                UaBase::BaseNode *pChild2;
                UaBase::FullReference reference;
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_MachineType_TemperatureSensor_Temperature, nsTypeIdx));
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
                    pChild3 = new UaBase::Variable(UaNodeId(DemoId_MachineType_TemperatureSensor_Temperature_EURange, nsTypeIdx));
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


        // Mandatory variable HeaterSwitch
        defaultValue.setBool(OpcUa_False);
        s_pHeaterSwitch = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_MachineType_HeaterSwitch, nsTypeIdx), "HeaterSwitch", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        s_pHeaterSwitch->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pHeaterSwitch->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pHeaterSwitch, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

    }
}

/** Clear the static members of the class
*/
void MachineTypeBase::clearStaticMembers()
{
    s_typeNodesCreated = false;

    s_pTemperatureSensor = NULL;
    s_pHeaterSwitch = NULL;
}

/** Returns the type definition NodeId for the MachineType
*/
UaNodeId MachineTypeBase::typeDefinitionId() const
{
    UaNodeId ret(DemoId_MachineType, NodeManagerDemo::getTypeNamespace());
    return ret;
}

/**
 *  Sets the HeaterSwitch value
 */
void MachineTypeBase::setHeaterSwitch(OpcUa_Boolean HeaterSwitch)
{
    UaVariant value;
    value.setBool(HeaterSwitch);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pHeaterSwitch->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of HeaterSwitch
 */
OpcUa_Boolean MachineTypeBase::getHeaterSwitch() const
{
    UaVariant defaultValue;
    OpcUa_Boolean ret = OpcUa_False;
    UaDataValue dataValue(m_pHeaterSwitch->value(NULL));
    defaultValue = *dataValue.value();
    defaultValue.toBool(ret);
    return ret;
}

/** Returns the HeaterSwitch node.
 */
OpcUa::BaseDataVariableType* MachineTypeBase::getHeaterSwitchNode()
{
    return m_pHeaterSwitch;
}

/** Returns the TemperatureSensor node.
 */
Demo::TemperatureSensorType* MachineTypeBase::getTemperatureSensor()
{
    return m_pTemperatureSensor;
}


/** Applys the NodeAccessInfo set at the MachineTypeBase and its children. */
void MachineTypeBase::useAccessInfoFromType()
{
    OpcUa::BaseObjectType::useAccessInfoFromType();

    // instance node
    OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
    UaNode *pTypeNode = NodeManagerRoot::CreateRootNodeManager()->getNode(UaNodeId(DemoId_MachineType, nsTypeIdx));
    if (pTypeNode)
    {
        useAccessInfoFrom(pTypeNode);
        pTypeNode->releaseReference();
    }

    // children
    m_pTemperatureSensor->useAccessInfoFromInstance(s_pTemperatureSensor);
    m_pHeaterSwitch->useAccessInfoFromInstance(s_pHeaterSwitch);
}

/** Uses the NodeAccessInfo of pOther and its children. */
void MachineTypeBase::useAccessInfoFromInstance(MachineTypeBase *pOther)
{
    OpcUa::BaseObjectType::useAccessInfoFromInstance(pOther);

    // children
    m_pTemperatureSensor->useAccessInfoFromInstance(pOther->m_pTemperatureSensor);
    m_pHeaterSwitch->useAccessInfoFromInstance(pOther->m_pHeaterSwitch);
}

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/



/******************************************************************************
** demo_filllevelsensortypebase.cpp
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

#include "demo_filllevelsensortypebase.h"
#include "demo_filllevelsensortype.h"
#include "uagenericnodes.h"
#include "nodemanagerroot.h"
#include "methodhandleuanode.h"
#include "demo_nodemanagerdemo.h"

#include "instancefactory.h"
// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

bool FillLevelSensorTypeBase::s_typeNodesCreated = false;
OpcUa::AnalogItemType* FillLevelSensorTypeBase::s_pFillLevel = NULL;

/** Constructs an FillLevelSensorType object using an instance declaration node as base
*/
FillLevelSensorTypeBase::FillLevelSensorTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, pInstanceDeclarationObject, pNodeConfig, pSharedMutex)
{
    initialize();
}

/** Constructs an FillLevelSensorType object
*/
FillLevelSensorTypeBase::FillLevelSensorTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, name, browseNameNameSpaceIndex, pNodeConfig, pSharedMutex)
{
    initialize();
}

/**  Constructs an instance of the class FillLevelSensorTypeBase with all components
*/
FillLevelSensorTypeBase::FillLevelSensorTypeBase(
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

    m_pFillLevel = NULL;

    std::list<UaBase::BaseNode*> lstReferencedNodes = pBaseNode->hierarchicallyReferencedNodes();
    for (std::list<UaBase::BaseNode*>::const_iterator it = lstReferencedNodes.begin(); it != lstReferencedNodes.end(); it++)
    {
        UaBase::BaseNode *pChild = *it;
        if (pChild->browseName() == UaQualifiedName("FillLevel", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pFillLevel)
            {
                m_pFillLevel = (OpcUa::AnalogItemType*) pFactory->createVariable((UaBase::Variable*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pFillLevel, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Variable*)pChild)->modellingRuleId().isNull())
                {
                    m_pFillLevel->setModellingRuleId(((UaBase::Variable*)pChild)->modellingRuleId());
                }
            }
        }
    }
    UA_ASSERT(m_pFillLevel);
}

/** Initialize the object with all member nodes
*/
void FillLevelSensorTypeBase::initialize()
{
    UaStatus      addStatus;
    UaVariant     defaultValue;

    if ( s_typeNodesCreated == false )
    {
        createTypes();
    }
    // Mandatory variable FillLevel
    m_pFillLevel = new OpcUa::AnalogItemType(this, s_pFillLevel, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pFillLevel, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


}

/** Destruction
*/
FillLevelSensorTypeBase::~FillLevelSensorTypeBase()
{
}

/** Create the related type nodes
*/
void FillLevelSensorTypeBase::createTypes()
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
            UaNodeId(DemoId_FillLevelSensorType, nsTypeIdx),
            UaQualifiedName("FillLevelSensorType", nsTypeIdx),
            UaLocalizedText("", "FillLevelSensorType"),
            UaLocalizedText("", ""),
            OpcUa_False,
            &FillLevelSensorType::clearStaticMembers);
        pTypeNodeConfig->addNodeAndReference(UaNodeId(OpcUaId_BaseObjectType, nsSuperTypeIdx), pObjectType, OpcUaId_HasSubtype);

        // Mandatory variable FillLevel
        defaultValue.setDouble(0.0);
        {
            std::list<UaBase::BaseNode*> lstBaseNodes;
            UaBase::Variable *pChild = new UaBase::Variable(UaNodeId(DemoId_FillLevelSensorType_FillLevel, nsTypeIdx));
            lstBaseNodes.push_back(pChild);
            pChild->setBrowseName(UaQualifiedName("FillLevel", Demo::NodeManagerDemo::getTypeNamespace()));
            pChild->setDisplayName(UaLocalizedText("", "FillLevel"));
            pChild->setAccessLevel(3);
            pChild->setUserAccessLevel(3);
            pChild->setValueRank(-1);
            pChild->setDataTypeId(OpcUaId_Double);
            pChild->setValue(defaultValue);
            pChild->setValueRank(-1);
            {
                UaBase::BaseNode *pParent = pChild;
                UaBase::BaseNode *pChild2;
                UaBase::FullReference reference;
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_FillLevelSensorType_FillLevel_Definition, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Definition", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Definition"));
                pChild2->setDescription(UaLocalizedText("", "A vendor-specific, human readable string that specifies how the value of this DataItem is calculated."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_FillLevelSensorType_FillLevel_EngineeringUnits, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EngineeringUnits", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EngineeringUnits"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_EUInformation);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_FillLevelSensorType_FillLevel_EURange, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EURange", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EURange"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Range);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_FillLevelSensorType_FillLevel_InstrumentRange, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("InstrumentRange", 0));
                pChild2->setDisplayName(UaLocalizedText("", "InstrumentRange"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Range);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(DemoId_FillLevelSensorType_FillLevel_ValuePrecision, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ValuePrecision", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ValuePrecision"));
                pChild2->setDescription(UaLocalizedText("", "The maximum precision that the server can maintain for the item based on restrictions in the target environment."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Double);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Optional);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
            }

            XmlUaNodeFactoryManager factory;
            s_pFillLevel = new OpcUa::AnalogItemType(pChild, &factory, pTypeNodeConfig, NULL);
            addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pFillLevel, OpcUaId_HasComponent);
            UA_ASSERT(addStatus.isGood());
            s_pFillLevel->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
            std::list<UaBase::BaseNode*>::iterator it = lstBaseNodes.begin();
            while (it != lstBaseNodes.end())
            {
                delete *it;
                it++;
            }
        }

    }
}

/** Clear the static members of the class
*/
void FillLevelSensorTypeBase::clearStaticMembers()
{
    s_typeNodesCreated = false;

    s_pFillLevel = NULL;
}

/** Returns the type definition NodeId for the FillLevelSensorType
*/
UaNodeId FillLevelSensorTypeBase::typeDefinitionId() const
{
    UaNodeId ret(DemoId_FillLevelSensorType, NodeManagerDemo::getTypeNamespace());
    return ret;
}

/**
 *  Sets the FillLevel value
 */
void FillLevelSensorTypeBase::setFillLevel(OpcUa_Double FillLevel)
{
    UaVariant value;
    value.setDouble(FillLevel);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pFillLevel->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of FillLevel
 */
OpcUa_Double FillLevelSensorTypeBase::getFillLevel() const
{
    UaVariant defaultValue;
    OpcUa_Double ret = 0;
    UaDataValue dataValue(m_pFillLevel->value(NULL));
    defaultValue = *dataValue.value();
    defaultValue.toDouble(ret);
    return ret;
}

/** Returns the FillLevel node.
 */
OpcUa::AnalogItemType* FillLevelSensorTypeBase::getFillLevelNode()
{
    return m_pFillLevel;
}


/** Applys the NodeAccessInfo set at the FillLevelSensorTypeBase and its children. */
void FillLevelSensorTypeBase::useAccessInfoFromType()
{
    OpcUa::BaseObjectType::useAccessInfoFromType();

    // instance node
    OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
    UaNode *pTypeNode = NodeManagerRoot::CreateRootNodeManager()->getNode(UaNodeId(DemoId_FillLevelSensorType, nsTypeIdx));
    if (pTypeNode)
    {
        useAccessInfoFrom(pTypeNode);
        pTypeNode->releaseReference();
    }

    // children
    m_pFillLevel->useAccessInfoFromInstance(s_pFillLevel);
}

/** Uses the NodeAccessInfo of pOther and its children. */
void FillLevelSensorTypeBase::useAccessInfoFromInstance(FillLevelSensorTypeBase *pOther)
{
    OpcUa::BaseObjectType::useAccessInfoFromInstance(pOther);

    // children
    m_pFillLevel->useAccessInfoFromInstance(pOther->m_pFillLevel);
}

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/



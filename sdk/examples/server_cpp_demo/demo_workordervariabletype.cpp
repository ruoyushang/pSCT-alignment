/******************************************************************************
** demo_workordervariabletype.cpp
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

#include "demo_workordervariabletype.h"
#include "uagenericnodes.h"
#include "nodemanagerroot.h"
#include "instancefactory.h"
#include "demo_nodemanagerdemo.h"


// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

bool WorkOrderVariableType::s_typeNodesCreated = false;
OpcUa::BaseDataVariableType* WorkOrderVariableType::s_pAssetID = NULL;
OpcUa::BaseDataVariableType* WorkOrderVariableType::s_pID = NULL;
OpcUa::BaseDataVariableType* WorkOrderVariableType::s_pStartTime = NULL;
OpcUa::BaseDataVariableType* WorkOrderVariableType::s_pStatusComments = NULL;

/** Constructs an instance of the class WorkOrderVariableType based on an instance declaration variable.

 This constructor is used if the variable is created based on an instance declaration of an object or variable type. Since the only difference between the
 instance declaration variable and the instance variable are the NodeId and the Value, the pointer to the instance declaration node is used to get the attribute
 values other than the attributes NodeId and Value from the instance declaration node.<br>
 The variable is reference counted and can not be deleted directly. The reference counter is set to one after the variable is created.
 If the variable was successfully added to a NodeManager using the methods NodeManagerConfig::addUaNode or NodeManagerConfig::addNodeAndReference the
 NodeManager will release this reference during shut down. If the variable was not added to a NodeManager, it must be deleted using the releaseReference
 method. If the interface pointer is used in other places, the availability of the variable must be ensured by incrementing the reference counter with
 addReference when the pointer is stored somewhere and decremented with releaseReference if the interface pointer is not longer needed.
*/
WorkOrderVariableType::WorkOrderVariableType(
    UaNode*            pParentNode,  //!< [in] Parent node of the new variable
    UaVariable*        pInstanceDeclarationVariable, //!< [in] UaVariable interface of the instance declaration node used to provide attribute values other than NodeId and Value
    NodeManagerConfig* pNodeConfig,  //!< [in] Interface pointer to the NodeManagerConfig interface used to add and delete node and references in the address space
    UaMutexRefCounted* pSharedMutex) //!< [in] Shared mutex object used to synchronize access to the variable. Can be NULL if no shared mutex is provided
: OpcUa::BaseDataVariableType(pParentNode, pInstanceDeclarationVariable, pNodeConfig, pSharedMutex)
{
    initialize(pNodeConfig);
}

/** Constructs an instance of the class WorkOrderVariableType with the passed attribute values.

 This constructor is used if the variable is not created based on an instance declaration. It defines the name, nodeId, initial value and access level
 of the variable. Additional language specific names and other attribute values can be set with set methods provided by the class.
 The variable is reference counted and can not be deleted directly. The reference counter is set to one after the variable is created.
 If the variable was successfully added to a NodeManager using the methods NodeManagerConfig::addUaNode or NodeManagerConfig::addNodeAndReference the
 NodeManager will release this reference during shut down. If the variable was not added to a NodeManager, it must be deleted using the releaseReference
 method. If the interface pointer is used in other places, the availability of the variable must be ensured by incrementing the reference counter with
 addReference when the pointer is stored somewhere and decremented with releaseReference if the interface pointer is not longer needed.
*/
WorkOrderVariableType::WorkOrderVariableType(
    const UaNodeId&    nodeId,       //!< [in] NodeId of the new variable
    const UaString&    name,         //!< [in] Name of the new variable. Used as browse name and also as display name if no additional language specific names are set.
    OpcUa_UInt16       browseNameNameSpaceIndex, //!< [in] Namespace index used for the browse name
    const UaVariant&   initialValue, //!< [in] Initial value for the Variable
    OpcUa_Byte         accessLevel,  //!< [in] Access level for the Variable
    NodeManagerConfig* pNodeConfig,  //!< [in] Interface pointer to the NodeManagerConfig interface used to add and delete node and references in the address space
    UaMutexRefCounted* pSharedMutex) //!< [in] Shared mutex object used to synchronize access to the variable. Can be NULL if no shared mutex is provided
: OpcUa::BaseDataVariableType(nodeId, name, browseNameNameSpaceIndex, initialValue, accessLevel, pNodeConfig, pSharedMutex)
{
    initialize(pNodeConfig);
}

/**  Constructs an instance of the class WorkOrderVariableType with all components
*/
WorkOrderVariableType::WorkOrderVariableType(
    UaBase::Variable*  pBaseNode,    //!< [in] The "tree" of nodes that specifies the attributes of the instance and its children
    XmlUaNodeFactoryManager* pFactory, //!< [in] The factory to create the children
    NodeManagerConfig* pNodeConfig,  //!< [in] Interface pointer to the NodeManagerConfig interface used to add and delete node and references in the address space
    UaMutexRefCounted* pSharedMutex) //!< [in] Shared mutex object used to synchronize access to the variable. Can be NULL if no shared mutex is provided
: OpcUa::BaseDataVariableType(pBaseNode, pFactory, pNodeConfig, pSharedMutex)
{
    UaStatus      addStatus;

    if ( s_typeNodesCreated == false )
    {
        createTypes();
    }

    m_pAssetID = NULL;
    m_pID = NULL;
    m_pStartTime = NULL;
    m_pStatusComments = NULL;

    std::list<UaBase::BaseNode*> lstReferencedNodes = pBaseNode->hierarchicallyReferencedNodes();
    for (std::list<UaBase::BaseNode*>::const_iterator it = lstReferencedNodes.begin(); it != lstReferencedNodes.end(); it++)
    {
        UaBase::BaseNode *pChild = *it;
        UaBase::Variable *pVariable = (UaBase::Variable*) pChild;
        if (pChild->browseName() == UaQualifiedName("AssetID", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pAssetID)
            {
                m_pAssetID = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)*it, pNodeConfig, pSharedMutex);
                if (!pVariable->modellingRuleId().isNull())
                {
                    m_pAssetID->setModellingRuleId(pVariable->modellingRuleId());
                }
                addStatus = pNodeConfig->addNodeAndReference(this, m_pAssetID, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
            }
        }
        else if (pChild->browseName() == UaQualifiedName("ID", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pID)
            {
                m_pID = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)*it, pNodeConfig, pSharedMutex);
                if (!pVariable->modellingRuleId().isNull())
                {
                    m_pID->setModellingRuleId(pVariable->modellingRuleId());
                }
                addStatus = pNodeConfig->addNodeAndReference(this, m_pID, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
            }
        }
        else if (pChild->browseName() == UaQualifiedName("StartTime", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pStartTime)
            {
                m_pStartTime = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)*it, pNodeConfig, pSharedMutex);
                if (!pVariable->modellingRuleId().isNull())
                {
                    m_pStartTime->setModellingRuleId(pVariable->modellingRuleId());
                }
                addStatus = pNodeConfig->addNodeAndReference(this, m_pStartTime, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
            }
        }
        else if (pChild->browseName() == UaQualifiedName("StatusComments", Demo::NodeManagerDemo::getTypeNamespace()))
        {
            if (!m_pStatusComments)
            {
                m_pStatusComments = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)*it, pNodeConfig, pSharedMutex);
                if (!pVariable->modellingRuleId().isNull())
                {
                    m_pStatusComments->setModellingRuleId(pVariable->modellingRuleId());
                }
                addStatus = pNodeConfig->addNodeAndReference(this, m_pStatusComments, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
            }
        }
    }
    UA_ASSERT(m_pAssetID);
    UA_ASSERT(m_pID);
    UA_ASSERT(m_pStartTime);
    UA_ASSERT(m_pStatusComments);
}

/** Initialize the variable with all member nodes
*/
void WorkOrderVariableType::initialize(NodeManagerConfig* pNodeConfig)
{
    UaStatus      addStatus;

    if ( s_typeNodesCreated == false )
    {
        createTypes();
    }

    // String variable AssetID
    m_pAssetID = new OpcUa::BaseDataVariableType(this, s_pAssetID, pNodeConfig, getSharedMutex());
    addStatus = pNodeConfig->addNodeAndReference(this, m_pAssetID, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Guid variable ID
    m_pID = new OpcUa::BaseDataVariableType(this, s_pID, pNodeConfig, getSharedMutex());
    addStatus = pNodeConfig->addNodeAndReference(this, m_pID, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // DateTime variable StartTime
    m_pStartTime = new OpcUa::BaseDataVariableType(this, s_pStartTime, pNodeConfig, getSharedMutex());
    addStatus = pNodeConfig->addNodeAndReference(this, m_pStartTime, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // WorkOrderStatusType variable StatusComments
    m_pStatusComments = new OpcUa::BaseDataVariableType(this, s_pStatusComments, pNodeConfig, getSharedMutex());
    addStatus = pNodeConfig->addNodeAndReference(this, m_pStatusComments, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    m_pStatusComments->setDataType(UaNodeId(DemoId_WorkOrderStatusType, Demo::NodeManagerDemo::getTypeNamespace()));
}

/** Destroys the WorkOrderVariableType object
*
* The destructor is protected since the object is reference counted. It is not possible to delete the object directly.
*
* If the node was added successfully to a NodeManager, it can only be deleted by deleting the node from the NodeManager.
* If the node was not added to a NodeManager or adding the node failed, the node can be deleted by releasing the reference to the node.
* In addition potential children should be deleted before releasing the reference. The two variations are shown in the following sample code.
* @code
* WorkOrderVariableType* pWorkOrderVariableType;
* MyNodeManager*  pNodeManager;
*
* // Delete children and release reference if the node was not added to a NodeManager
* pWorkOrderVariableType->deleteAllChildren(pNodeManager);
* pWorkOrderVariableType->releaseReference();
*
* // Delete the node from the NodeManager after it was added to the NodeManager
* pNodeManager->deleteUaNode(pWorkOrderVariableType, OpcUa_True, OpcUa_True, OpcUa_True);
* @endcode
*/
WorkOrderVariableType::~WorkOrderVariableType()
{
}

/** Create the related type nodes
*/
void WorkOrderVariableType::createTypes()
{
    if ( s_typeNodesCreated == false )
    {
        OpcUa::BaseDataVariableType::createTypes();
        s_typeNodesCreated = true;
        UaStatus      addStatus;
        UaVariant     defaultValue;
        NodeManagerRoot* pNodeManagerRoot = NodeManagerRoot::CreateRootNodeManager();
        OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
        NodeManagerConfig* pTypeNodeConfig = pNodeManagerRoot->getNodeManagerByNamespace(nsTypeIdx)->getNodeManagerConfig();

        UaVariant                   nullValue;
        OpcUa::GenericVariableType* pVariableType;
        pVariableType = new OpcUa::GenericVariableType(
            UaNodeId(DemoId_WorkOrderVariableType, nsTypeIdx),
            UaQualifiedName("WorkOrderVariableType", nsTypeIdx),
            UaLocalizedText("", "WorkOrderVariableType"),
            UaLocalizedText("", ""),
            nullValue,
            UaNodeId(DemoId_WorkOrderType, Demo::NodeManagerDemo::getTypeNamespace()),
            -1,
            OpcUa_False,
            &clearStaticMembers);
        addStatus = pTypeNodeConfig->addNodeAndReference(UaNodeId(OpcUaId_BaseDataVariableType, NodeManagerRoot::getTypeNamespace()), pVariableType, OpcUaId_HasSubtype);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable AssetID
        defaultValue.setString("");
        s_pAssetID = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_WorkOrderVariableType_AssetID, nsTypeIdx), "AssetID", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        s_pAssetID->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pAssetID->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pVariableType, s_pAssetID, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable ID
        defaultValue.setGuid(UaGuid());
        s_pID = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_WorkOrderVariableType_ID, nsTypeIdx), "ID", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        s_pID->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pID->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pVariableType, s_pID, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable StartTime
        defaultValue.setDateTime(UaDateTime());
        s_pStartTime = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_WorkOrderVariableType_StartTime, nsTypeIdx), "StartTime", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        s_pStartTime->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pStartTime->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pVariableType, s_pStartTime, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable StatusComments
        Demo::WorkOrderStatusTypes valStatusComments;
        valStatusComments.toVariant(defaultValue);
        s_pStatusComments = new OpcUa::BaseDataVariableType(UaNodeId(DemoId_WorkOrderVariableType_StatusComments, nsTypeIdx), "StatusComments", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        s_pStatusComments->setDataType(UaNodeId(DemoId_WorkOrderStatusType, Demo::NodeManagerDemo::getTypeNamespace()));
        s_pStatusComments->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        s_pStatusComments->setValueRank(1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pVariableType, s_pStatusComments, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

    }
}

/** Clear the static members of the class
*/
void WorkOrderVariableType::clearStaticMembers()
{
    s_typeNodesCreated = false;

    s_pAssetID = NULL;
    s_pID = NULL;
    s_pStartTime = NULL;
    s_pStatusComments = NULL;
}

/** Returns the type definition NodeId for the WorkOrderVariableType
*/
UaNodeId WorkOrderVariableType::typeDefinitionId() const
{
    UaNodeId ret(DemoId_WorkOrderVariableType, NodeManagerDemo::getTypeNamespace());
    return ret;
}

/**
 *  Sets the AssetID value
 */
void WorkOrderVariableType::setAssetID(const UaString& AssetID)
{
    UaVariant value;
    value.setString(AssetID);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pAssetID->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of AssetID
 */
UaString WorkOrderVariableType::getAssetID() const
{
    UaVariant defaultValue;
    UaString ret;
    UaDataValue dataValue(m_pAssetID->value(NULL));
    defaultValue = *dataValue.value();
    ret = defaultValue.toString();
    return ret;
}


/**
 *  Sets the ID value
 */
void WorkOrderVariableType::setID(const UaGuid& ID)
{
    UaVariant value;
    value.setGuid(ID);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pID->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of ID
 */
UaGuid WorkOrderVariableType::getID() const
{
    UaVariant defaultValue;
    UaGuid ret;
    UaDataValue dataValue(m_pID->value(NULL));
    defaultValue = *dataValue.value();
    defaultValue.toGuid(ret);
    return ret;
}


/**
 *  Sets the StartTime value
 */
void WorkOrderVariableType::setStartTime(const UaDateTime& StartTime)
{
    UaVariant value;
    value.setDateTime(StartTime);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pStartTime->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of StartTime
 */
UaDateTime WorkOrderVariableType::getStartTime() const
{
    UaVariant defaultValue;
    UaDateTime ret = UaDateTime();
    UaDataValue dataValue(m_pStartTime->value(NULL));
    defaultValue = *dataValue.value();
    defaultValue.toDateTime(ret);
    return ret;
}


/**
 *  Sets the StatusComments
 */
void WorkOrderVariableType::setStatusComments(const Demo::WorkOrderStatusTypes& StatusComments)
{
    UaDataValue dataValue;
    StatusComments.toDataValue(dataValue, OpcUa_True);
    m_pStatusComments->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Gets the StatusComments
 */
void WorkOrderVariableType::getStatusComments(Demo::WorkOrderStatusTypes& StatusComments) const
{
    StatusComments.clear();
    if ( m_pStatusComments != NULL )
    {
        UaDataValue dataValue(m_pStatusComments->value(NULL));
        UaVariant value;
        value.attach(dataValue.value());
        StatusComments.setWorkOrderStatusTypes(value);
        value.detach();
    }
}

/** Returns the AssetID node.
 */
OpcUa::BaseDataVariableType* WorkOrderVariableType::getAssetIDNode()
{
    return m_pAssetID;
}

/** Returns the ID node.
 */
OpcUa::BaseDataVariableType* WorkOrderVariableType::getIDNode()
{
    return m_pID;
}

/** Returns the StartTime node.
 */
OpcUa::BaseDataVariableType* WorkOrderVariableType::getStartTimeNode()
{
    return m_pStartTime;
}

/** Returns the StatusComments node.
 */
OpcUa::BaseDataVariableType* WorkOrderVariableType::getStatusCommentsNode()
{
    return m_pStatusComments;
}


/** Applys the NodeAccessInfo set at the Base and its children. */
void WorkOrderVariableType::useAccessInfoFromType()
{
    // instance node
    OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
    UaNode *pTypeNode = NodeManagerRoot::CreateRootNodeManager()->getNode(UaNodeId(DemoId_WorkOrderVariableType, nsTypeIdx));
    if (pTypeNode)
    {
        useAccessInfoFrom(pTypeNode);
        pTypeNode->releaseReference();
    }

    // children
    m_pAssetID->useAccessInfoFromInstance(s_pAssetID);
    m_pID->useAccessInfoFromInstance(s_pID);
    m_pStartTime->useAccessInfoFromInstance(s_pStartTime);
    m_pStatusComments->useAccessInfoFromInstance(s_pStatusComments);
}

/** Uses the NodeAccessInfo of pOther and its children. */
void WorkOrderVariableType::useAccessInfoFromInstance(WorkOrderVariableType *pOther)
{
    OpcUa::BaseDataVariableType::useAccessInfoFromInstance(pOther);

    // children
    m_pAssetID->useAccessInfoFromInstance(pOther->m_pAssetID);
    m_pID->useAccessInfoFromInstance(pOther->m_pID);
    m_pStartTime->useAccessInfoFromInstance(pOther->m_pStartTime);
    m_pStatusComments->useAccessInfoFromInstance(pOther->m_pStatusComments);
}


} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/


/******************************************************************************
** access_accesspermissionobjecttypebase.cpp
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
** Project: C++ OPC Server SDK information model for namespace http://www.unifiedautomation.com/DemoServer/AccessPermission
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#include "access_accesspermissionobjecttypebase.h"
#include "access_accesspermissionobjecttype.h"
#include "uagenericnodes.h"
#include "nodemanagerroot.h"
#include "methodhandleuanode.h"
#include "access_nodemanageraccess.h"

#include "instancefactory.h"
#include "uaargument.h"
// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
namespace Access {

bool AccessPermissionObjectTypeBase::s_typeNodesCreated = false;
OpcUa::BaseMethod* AccessPermissionObjectTypeBase::s_pSetValue_AllMethod = NULL;
OpcUa::BaseMethod* AccessPermissionObjectTypeBase::s_pSetValue_JohnMethod = NULL;
OpcUa::BaseMethod* AccessPermissionObjectTypeBase::s_pSetValue_OperatorsMethod = NULL;
#if SUPPORT_Event_Subscription_Server_Facet
OpcUa::ExclusiveLimitAlarmType* AccessPermissionObjectTypeBase::s_pLimitAlarm_All = NULL;
OpcUa::ExclusiveLimitAlarmType* AccessPermissionObjectTypeBase::s_pLimitAlarm_John = NULL;
OpcUa::ExclusiveLimitAlarmType* AccessPermissionObjectTypeBase::s_pLimitAlarm_Operators = NULL;
#endif // SUPPORT_Event_Subscription_Server_Facet
OpcUa::BaseDataVariableType* AccessPermissionObjectTypeBase::s_pValue = NULL;

/** Constructs an AccessPermissionObjectType object using an instance declaration node as base
*/
AccessPermissionObjectTypeBase::AccessPermissionObjectTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, pInstanceDeclarationObject, pNodeConfig, pSharedMutex)
{
    initialize();
}

/** Constructs an AccessPermissionObjectType object
*/
AccessPermissionObjectTypeBase::AccessPermissionObjectTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex)
: OpcUa::BaseObjectType(nodeId, name, browseNameNameSpaceIndex, pNodeConfig, pSharedMutex)
{
    initialize();
}

/**  Constructs an instance of the class AccessPermissionObjectTypeBase with all components
*/
AccessPermissionObjectTypeBase::AccessPermissionObjectTypeBase(
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

#if SUPPORT_Event_Subscription_Server_Facet
    m_pLimitAlarm_All = NULL;
    m_pLimitAlarm_John = NULL;
    m_pLimitAlarm_Operators = NULL;
#endif // SUPPORT_Event_Subscription_Server_Facet
    m_pValue = NULL;
    m_pSetValue_AllMethod = NULL;
    m_pSetValue_JohnMethod = NULL;
    m_pSetValue_OperatorsMethod = NULL;

    std::list<UaBase::BaseNode*> lstReferencedNodes = pBaseNode->hierarchicallyReferencedNodes();
    for (std::list<UaBase::BaseNode*>::const_iterator it = lstReferencedNodes.begin(); it != lstReferencedNodes.end(); it++)
    {
        UaBase::BaseNode *pChild = *it;
        if (pChild->browseName() == UaQualifiedName("Value", Access::NodeManagerAccess::getTypeNamespace()))
        {
            if (!m_pValue)
            {
                m_pValue = (OpcUa::BaseDataVariableType*) pFactory->createVariable((UaBase::Variable*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pValue, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Variable*)pChild)->modellingRuleId().isNull())
                {
                    m_pValue->setModellingRuleId(((UaBase::Variable*)pChild)->modellingRuleId());
                }
            }
        }
#if SUPPORT_Event_Subscription_Server_Facet
        else if (pChild->browseName() == UaQualifiedName("LimitAlarm_All", Access::NodeManagerAccess::getTypeNamespace()))
        {
            if (!m_pLimitAlarm_All)
            {
                m_pLimitAlarm_All = (OpcUa::ExclusiveLimitAlarmType*) pFactory->createObject((UaBase::Object*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pLimitAlarm_All, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Object*)pChild)->modellingRuleId().isNull())
                {
                    m_pLimitAlarm_All->setModellingRuleId(((UaBase::Object*)pChild)->modellingRuleId());
                }
            }
        }
        else if (pChild->browseName() == UaQualifiedName("LimitAlarm_John", Access::NodeManagerAccess::getTypeNamespace()))
        {
            if (!m_pLimitAlarm_John)
            {
                m_pLimitAlarm_John = (OpcUa::ExclusiveLimitAlarmType*) pFactory->createObject((UaBase::Object*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pLimitAlarm_John, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Object*)pChild)->modellingRuleId().isNull())
                {
                    m_pLimitAlarm_John->setModellingRuleId(((UaBase::Object*)pChild)->modellingRuleId());
                }
            }
        }
        else if (pChild->browseName() == UaQualifiedName("LimitAlarm_Operators", Access::NodeManagerAccess::getTypeNamespace()))
        {
            if (!m_pLimitAlarm_Operators)
            {
                m_pLimitAlarm_Operators = (OpcUa::ExclusiveLimitAlarmType*) pFactory->createObject((UaBase::Object*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pLimitAlarm_Operators, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Object*)pChild)->modellingRuleId().isNull())
                {
                    m_pLimitAlarm_Operators->setModellingRuleId(((UaBase::Object*)pChild)->modellingRuleId());
                }
            }
        }
#endif // SUPPORT_Event_Subscription_Server_Facet
        else if (s_pSetValue_AllMethod && pChild->browseName() == s_pSetValue_AllMethod->browseName())
        {
            if (!m_pSetValue_AllMethod)
            {
                m_pSetValue_AllMethod = (OpcUa::BaseMethod*) pFactory->createMethod((UaBase::Method*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pSetValue_AllMethod, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Method*)pChild)->modellingRuleId().isNull())
                {
                    m_pSetValue_AllMethod->setModellingRuleId(((UaBase::Method*)pChild)->modellingRuleId());
                }
            }
        }
        else if (s_pSetValue_JohnMethod && pChild->browseName() == s_pSetValue_JohnMethod->browseName())
        {
            if (!m_pSetValue_JohnMethod)
            {
                m_pSetValue_JohnMethod = (OpcUa::BaseMethod*) pFactory->createMethod((UaBase::Method*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pSetValue_JohnMethod, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Method*)pChild)->modellingRuleId().isNull())
                {
                    m_pSetValue_JohnMethod->setModellingRuleId(((UaBase::Method*)pChild)->modellingRuleId());
                }
            }
        }
        else if (s_pSetValue_OperatorsMethod && pChild->browseName() == s_pSetValue_OperatorsMethod->browseName())
        {
            if (!m_pSetValue_OperatorsMethod)
            {
                m_pSetValue_OperatorsMethod = (OpcUa::BaseMethod*) pFactory->createMethod((UaBase::Method*)pChild, pNodeConfig, pSharedMutex);
                addStatus = pNodeConfig->addNodeAndReference(this, m_pSetValue_OperatorsMethod, OpcUaId_HasComponent);
                UA_ASSERT(addStatus.isGood());
                if (!((UaBase::Method*)pChild)->modellingRuleId().isNull())
                {
                    m_pSetValue_OperatorsMethod->setModellingRuleId(((UaBase::Method*)pChild)->modellingRuleId());
                }
            }
        }
    }
#if SUPPORT_Event_Subscription_Server_Facet
    UA_ASSERT(m_pLimitAlarm_All);
    UA_ASSERT(m_pLimitAlarm_John);
    UA_ASSERT(m_pLimitAlarm_Operators);
#endif // SUPPORT_Event_Subscription_Server_Facet
    UA_ASSERT(m_pValue);
    UA_ASSERT(m_pSetValue_AllMethod);
    UA_ASSERT(m_pSetValue_JohnMethod);
    UA_ASSERT(m_pSetValue_OperatorsMethod);
}

/** Initialize the object with all member nodes
*/
void AccessPermissionObjectTypeBase::initialize()
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
#if SUPPORT_Event_Subscription_Server_Facet
    // Condition LimitAlarm_All
    m_pLimitAlarm_All = new OpcUa::ExclusiveLimitAlarmType(UaNodeId(UaString("%1.LimitAlarm_All").arg(nodeId().toString()), nsIdx), s_pLimitAlarm_All, m_pNodeConfig, nodeId(), browseName().toString(), m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pLimitAlarm_All, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Condition LimitAlarm_John
    m_pLimitAlarm_John = new OpcUa::ExclusiveLimitAlarmType(UaNodeId(UaString("%1.LimitAlarm_John").arg(nodeId().toString()), nsIdx), s_pLimitAlarm_John, m_pNodeConfig, nodeId(), browseName().toString(), m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pLimitAlarm_John, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Condition LimitAlarm_Operators
    m_pLimitAlarm_Operators = new OpcUa::ExclusiveLimitAlarmType(UaNodeId(UaString("%1.LimitAlarm_Operators").arg(nodeId().toString()), nsIdx), s_pLimitAlarm_Operators, m_pNodeConfig, nodeId(), browseName().toString(), m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pLimitAlarm_Operators, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
#endif // SUPPORT_Event_Subscription_Server_Facet
    // Mandatory variable Value
    m_pValue = new OpcUa::BaseDataVariableType(this, s_pValue, m_pNodeConfig, m_pSharedMutex);
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pValue, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    // Method SetValue_All
    m_pSetValue_AllMethod = new OpcUa::BaseMethod(this, s_pSetValue_AllMethod, m_pSharedMutex);
    m_pSetValue_AllMethod->setDescription(UaLocalizedText("", "This method is executable for every user."));
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pSetValue_AllMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // ------------------------------------------------------------------------
    // Add InputArguments
    // Create property
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(UaString("%1.SetValue_All.InputArguments").arg(nodeId().toString()), nsIdx), // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        1,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument Value
    pPropertyArg->setArgument(
        0,                      // Index of the argument
        "Value",             // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                     // Array rank of the argument
        nullarray,        // Array dimension
        UaLocalizedText("", "")); // Description
    // Add property to method
    addStatus = m_pNodeConfig->addNodeAndReference(m_pSetValue_AllMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // Method SetValue_John
    m_pSetValue_JohnMethod = new OpcUa::BaseMethod(this, s_pSetValue_JohnMethod, m_pSharedMutex);
    m_pSetValue_JohnMethod->setDescription(UaLocalizedText("", "This method is executable only for user john."));
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pSetValue_JohnMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // ------------------------------------------------------------------------
    // Add InputArguments
    // Create property
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(UaString("%1.SetValue_John.InputArguments").arg(nodeId().toString()), nsIdx), // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        1,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument Value
    pPropertyArg->setArgument(
        0,                      // Index of the argument
        "Value",             // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                     // Array rank of the argument
        nullarray,        // Array dimension
        UaLocalizedText("", "")); // Description
    // Add property to method
    addStatus = m_pNodeConfig->addNodeAndReference(m_pSetValue_JohnMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // Method SetValue_Operators
    m_pSetValue_OperatorsMethod = new OpcUa::BaseMethod(this, s_pSetValue_OperatorsMethod, m_pSharedMutex);
    m_pSetValue_OperatorsMethod->setDescription(UaLocalizedText("", "This method is executable only for members of the operators group."));
    addStatus = m_pNodeConfig->addNodeAndReference(this, m_pSetValue_OperatorsMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // ------------------------------------------------------------------------
    // Add InputArguments
    // Create property
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(UaString("%1.SetValue_Operators.InputArguments").arg(nodeId().toString()), nsIdx), // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        1,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument Value
    pPropertyArg->setArgument(
        0,                      // Index of the argument
        "Value",             // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                     // Array rank of the argument
        nullarray,        // Array dimension
        UaLocalizedText("", "")); // Description
    // Add property to method
    addStatus = m_pNodeConfig->addNodeAndReference(m_pSetValue_OperatorsMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    m_pNodeConfig->addUaReference(this, m_pValue, OpcUaId_HasEventSource);
    UA_ASSERT(addStatus.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    m_pNodeConfig->addUaReference(m_pValue, m_pLimitAlarm_All, OpcUaId_HasCondition);
    UA_ASSERT(addStatus.isGood());
    m_pNodeConfig->addUaReference(m_pValue, m_pLimitAlarm_John, OpcUaId_HasCondition);
    UA_ASSERT(addStatus.isGood());
    m_pNodeConfig->addUaReference(m_pValue, m_pLimitAlarm_Operators, OpcUaId_HasCondition);
    UA_ASSERT(addStatus.isGood());
#endif // SUPPORT_Event_Subscription_Server_Facet
}

/** Destruction
*/
AccessPermissionObjectTypeBase::~AccessPermissionObjectTypeBase()
{
}

/** Create the related type nodes
*/
void AccessPermissionObjectTypeBase::createTypes()
{
    if ( s_typeNodesCreated == false )
    {
        s_typeNodesCreated = true;

        UaStatus      addStatus;
        UaVariant     defaultValue;
        UaUInt32Array nullarray;
        UaPropertyMethodArgument* pPropertyArg = NULL;
        NodeManagerRoot* pNodeManagerRoot = NodeManagerRoot::CreateRootNodeManager();
        OpcUa_Int16 nsTypeIdx = NodeManagerAccess::getTypeNamespace();
        OpcUa_Int16 nsSuperTypeIdx = NodeManagerRoot::getTypeNamespace();
        NodeManagerConfig* pTypeNodeConfig = pNodeManagerRoot->getNodeManagerByNamespace(nsTypeIdx)->getNodeManagerConfig();

        OpcUa::GenericObjectType* pObjectType;
        pObjectType = new OpcUa::GenericObjectType(
            UaNodeId(AccessId_AccessPermissionObjectType, nsTypeIdx),
            UaQualifiedName("AccessPermissionObjectType", nsTypeIdx),
            UaLocalizedText("", "AccessPermissionObjectType"),
            UaLocalizedText("", ""),
            OpcUa_False,
            &AccessPermissionObjectType::clearStaticMembers);
        pTypeNodeConfig->addNodeAndReference(UaNodeId(OpcUaId_BaseObjectType, nsSuperTypeIdx), pObjectType, OpcUaId_HasSubtype);
#if SUPPORT_Event_Subscription_Server_Facet
        {
            std::list<UaBase::BaseNode*> lstBaseNodes;
            UaBase::Object *pChild = new UaBase::Object(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All, nsTypeIdx));
            lstBaseNodes.push_back(pChild);
            pChild->setBrowseName(UaQualifiedName("LimitAlarm_All", Access::NodeManagerAccess::getTypeNamespace()));
            pChild->setDisplayName(UaLocalizedText("", "LimitAlarm_All"));
            pChild->setDescription(UaLocalizedText("", "Every user can receive events for this alarm object."));
            {
                UaBase::BaseNode *pParent = pChild;
                UaBase::BaseNode *pChild2;
                UaBase::FullReference reference;
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_AckedState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("AckedState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "AckedState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_AckedState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Acknowledge, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Acknowledge", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Acknowledge"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Acknowledge_InputArguments, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InputArguments", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InputArguments"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Argument);
                    {
                        UaExtensionObjectArray extensionObjectArray;
                        extensionObjectArray.create(2);
                        UaArgument argument;
                        argument.setArgument("EventId", OpcUaId_ByteString, -1, UaUInt32Array(), UaLocalizedText("", "The identifier for the event to comment."));
                        argument.toExtensionObject(extensionObjectArray[ 0 ]);
                        argument.setArgument("Comment", OpcUaId_LocalizedText, -1, UaUInt32Array(), UaLocalizedText("", "The comment to add to the condition."));
                        argument.toExtensionObject(extensionObjectArray[ 1 ]);
                        UaVariant value;
                        value.setExtensionObjectArray(extensionObjectArray);
                        ((UaBase::Variable*)pChild3)->setValue(value);
                    }
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_ActiveState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ActiveState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ActiveState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_ActiveState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_AddComment, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("AddComment", 0));
                pChild2->setDisplayName(UaLocalizedText("", "AddComment"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_AddComment_InputArguments, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InputArguments", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InputArguments"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Argument);
                    {
                        UaExtensionObjectArray extensionObjectArray;
                        extensionObjectArray.create(2);
                        UaArgument argument;
                        argument.setArgument("EventId", OpcUaId_ByteString, -1, UaUInt32Array(), UaLocalizedText("", "The identifier for the event to comment."));
                        argument.toExtensionObject(extensionObjectArray[ 0 ]);
                        argument.setArgument("Comment", OpcUaId_LocalizedText, -1, UaUInt32Array(), UaLocalizedText("", "The comment to add to the condition."));
                        argument.toExtensionObject(extensionObjectArray[ 1 ]);
                        UaVariant value;
                        value.setExtensionObjectArray(extensionObjectArray);
                        ((UaBase::Variable*)pChild3)->setValue(value);
                    }
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_BranchId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("BranchId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "BranchId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_ClientUserId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ClientUserId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ClientUserId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Comment, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Comment", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Comment"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Comment_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_ConditionClassId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionClassId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionClassId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_ConditionClassName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionClassName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionClassName"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_ConditionName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionName"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Disable, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Disable", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Disable"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Enable, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Enable", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Enable"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_EnabledState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EnabledState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EnabledState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_EnabledState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_EventId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EventId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EventId"));
                pChild2->setDescription(UaLocalizedText("", "A globally unique identifier for the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_ByteString);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_EventType, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EventType", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EventType"));
                pChild2->setDescription(UaLocalizedText("", "The identifier for the event type."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_InputNode, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("InputNode", 0));
                pChild2->setDisplayName(UaLocalizedText("", "InputNode"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_LastSeverity, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LastSeverity", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LastSeverity"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UInt16);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_LastSeverity_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Object(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_LimitState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LimitState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LimitState"));
                ((UaBase::Object*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Object*)pChild2)->setTypeDefinitionId(OpcUaId_ExclusiveLimitStateMachineType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_LimitState_CurrentState, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("CurrentState", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "CurrentState"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_LocalizedText);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_FiniteStateVariableType);
                    reference.setReferenceTypeId(OpcUaId_HasComponent);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    {
                        UaBase::BaseNode *pChild4;
                        pChild4 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_LimitState_CurrentState_Id, nsTypeIdx));
                        lstBaseNodes.push_back(pChild4);
                        pChild4->setBrowseName(UaQualifiedName("Id", 0));
                        pChild4->setDisplayName(UaLocalizedText("", "Id"));
                        ((UaBase::Variable*)pChild4)->setAccessLevel(1);
                        ((UaBase::Variable*)pChild4)->setUserAccessLevel(1);
                        ((UaBase::Variable*)pChild4)->setValueRank(-1);
                        ((UaBase::Variable*)pChild4)->setDataTypeId(OpcUaId_NodeId);
                        ((UaBase::Variable*)pChild4)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                        ((UaBase::Variable*)pChild4)->setTypeDefinitionId(OpcUaId_PropertyType);
                        reference.setReferenceTypeId(OpcUaId_HasProperty);
                        reference.setSourceNode(pChild3);
                        reference.setTargetNode(pChild4);
                        pChild3->addFullReference(reference);
                        pChild4->addFullReference(reference);
                        pChild4->setParentNodeId(pChild3->nodeId());
                    }
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_LocalTime, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LocalTime", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LocalTime"));
                pChild2->setDescription(UaLocalizedText("", "Information about the local time where the event originated."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_TimeZoneDataType);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Message, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Message", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Message"));
                pChild2->setDescription(UaLocalizedText("", "A localized description of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Quality, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Quality", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Quality"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_StatusCode);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Quality_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_ReceiveTime, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ReceiveTime", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ReceiveTime"));
                pChild2->setDescription(UaLocalizedText("", "When the server received the event from the underlying system."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UtcTime);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Retain, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Retain", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Retain"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Boolean);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Severity, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Severity", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Severity"));
                pChild2->setDescription(UaLocalizedText("", "Indicates how urgent an event is."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UInt16);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_SourceName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SourceName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SourceName"));
                pChild2->setDescription(UaLocalizedText("", "A description of the source of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_SourceNode, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SourceNode", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SourceNode"));
                pChild2->setDescription(UaLocalizedText("", "The source of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_SuppressedOrShelved, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SuppressedOrShelved", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SuppressedOrShelved"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Boolean);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All_Time, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Time", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Time"));
                pChild2->setDescription(UaLocalizedText("", "When the event occurred."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UtcTime);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
            }

            XmlUaNodeFactoryManager factory;

            s_pLimitAlarm_All = new OpcUa::ExclusiveLimitAlarmType(pChild, &factory, pTypeNodeConfig, NULL);
            addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pLimitAlarm_All, OpcUaId_HasComponent);
            UA_ASSERT(addStatus.isGood());
            s_pLimitAlarm_All->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
            std::list<UaBase::BaseNode*>::iterator it = lstBaseNodes.begin();
            while (it != lstBaseNodes.end())
            {
                delete *it;
                it++;
            }
        }

        {
            std::list<UaBase::BaseNode*> lstBaseNodes;
            UaBase::Object *pChild = new UaBase::Object(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John, nsTypeIdx));
            lstBaseNodes.push_back(pChild);
            pChild->setBrowseName(UaQualifiedName("LimitAlarm_John", Access::NodeManagerAccess::getTypeNamespace()));
            pChild->setDisplayName(UaLocalizedText("", "LimitAlarm_John"));
            pChild->setDescription(UaLocalizedText("", "Only user john can receive events for this alarm object."));
            {
                UaBase::BaseNode *pParent = pChild;
                UaBase::BaseNode *pChild2;
                UaBase::FullReference reference;
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_AckedState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("AckedState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "AckedState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_AckedState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Acknowledge, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Acknowledge", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Acknowledge"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Acknowledge_InputArguments, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InputArguments", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InputArguments"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Argument);
                    {
                        UaExtensionObjectArray extensionObjectArray;
                        extensionObjectArray.create(2);
                        UaArgument argument;
                        argument.setArgument("EventId", OpcUaId_ByteString, -1, UaUInt32Array(), UaLocalizedText("", "The identifier for the event to comment."));
                        argument.toExtensionObject(extensionObjectArray[ 0 ]);
                        argument.setArgument("Comment", OpcUaId_LocalizedText, -1, UaUInt32Array(), UaLocalizedText("", "The comment to add to the condition."));
                        argument.toExtensionObject(extensionObjectArray[ 1 ]);
                        UaVariant value;
                        value.setExtensionObjectArray(extensionObjectArray);
                        ((UaBase::Variable*)pChild3)->setValue(value);
                    }
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_ActiveState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ActiveState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ActiveState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_ActiveState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_AddComment, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("AddComment", 0));
                pChild2->setDisplayName(UaLocalizedText("", "AddComment"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_AddComment_InputArguments, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InputArguments", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InputArguments"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Argument);
                    {
                        UaExtensionObjectArray extensionObjectArray;
                        extensionObjectArray.create(2);
                        UaArgument argument;
                        argument.setArgument("EventId", OpcUaId_ByteString, -1, UaUInt32Array(), UaLocalizedText("", "The identifier for the event to comment."));
                        argument.toExtensionObject(extensionObjectArray[ 0 ]);
                        argument.setArgument("Comment", OpcUaId_LocalizedText, -1, UaUInt32Array(), UaLocalizedText("", "The comment to add to the condition."));
                        argument.toExtensionObject(extensionObjectArray[ 1 ]);
                        UaVariant value;
                        value.setExtensionObjectArray(extensionObjectArray);
                        ((UaBase::Variable*)pChild3)->setValue(value);
                    }
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_BranchId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("BranchId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "BranchId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_ClientUserId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ClientUserId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ClientUserId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Comment, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Comment", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Comment"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Comment_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_ConditionClassId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionClassId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionClassId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_ConditionClassName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionClassName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionClassName"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_ConditionName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionName"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Disable, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Disable", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Disable"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Enable, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Enable", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Enable"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_EnabledState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EnabledState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EnabledState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_EnabledState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_EventId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EventId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EventId"));
                pChild2->setDescription(UaLocalizedText("", "A globally unique identifier for the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_ByteString);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_EventType, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EventType", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EventType"));
                pChild2->setDescription(UaLocalizedText("", "The identifier for the event type."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_InputNode, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("InputNode", 0));
                pChild2->setDisplayName(UaLocalizedText("", "InputNode"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_LastSeverity, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LastSeverity", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LastSeverity"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UInt16);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_LastSeverity_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Object(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_LimitState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LimitState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LimitState"));
                ((UaBase::Object*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Object*)pChild2)->setTypeDefinitionId(OpcUaId_ExclusiveLimitStateMachineType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_LimitState_CurrentState, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("CurrentState", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "CurrentState"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_LocalizedText);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_FiniteStateVariableType);
                    reference.setReferenceTypeId(OpcUaId_HasComponent);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    {
                        UaBase::BaseNode *pChild4;
                        pChild4 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_LimitState_CurrentState_Id, nsTypeIdx));
                        lstBaseNodes.push_back(pChild4);
                        pChild4->setBrowseName(UaQualifiedName("Id", 0));
                        pChild4->setDisplayName(UaLocalizedText("", "Id"));
                        ((UaBase::Variable*)pChild4)->setAccessLevel(1);
                        ((UaBase::Variable*)pChild4)->setUserAccessLevel(1);
                        ((UaBase::Variable*)pChild4)->setValueRank(-1);
                        ((UaBase::Variable*)pChild4)->setDataTypeId(OpcUaId_NodeId);
                        ((UaBase::Variable*)pChild4)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                        ((UaBase::Variable*)pChild4)->setTypeDefinitionId(OpcUaId_PropertyType);
                        reference.setReferenceTypeId(OpcUaId_HasProperty);
                        reference.setSourceNode(pChild3);
                        reference.setTargetNode(pChild4);
                        pChild3->addFullReference(reference);
                        pChild4->addFullReference(reference);
                        pChild4->setParentNodeId(pChild3->nodeId());
                    }
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_LocalTime, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LocalTime", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LocalTime"));
                pChild2->setDescription(UaLocalizedText("", "Information about the local time where the event originated."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_TimeZoneDataType);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Message, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Message", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Message"));
                pChild2->setDescription(UaLocalizedText("", "A localized description of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Quality, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Quality", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Quality"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_StatusCode);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Quality_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_ReceiveTime, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ReceiveTime", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ReceiveTime"));
                pChild2->setDescription(UaLocalizedText("", "When the server received the event from the underlying system."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UtcTime);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Retain, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Retain", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Retain"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Boolean);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Severity, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Severity", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Severity"));
                pChild2->setDescription(UaLocalizedText("", "Indicates how urgent an event is."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UInt16);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_SourceName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SourceName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SourceName"));
                pChild2->setDescription(UaLocalizedText("", "A description of the source of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_SourceNode, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SourceNode", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SourceNode"));
                pChild2->setDescription(UaLocalizedText("", "The source of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_SuppressedOrShelved, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SuppressedOrShelved", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SuppressedOrShelved"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Boolean);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John_Time, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Time", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Time"));
                pChild2->setDescription(UaLocalizedText("", "When the event occurred."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UtcTime);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
            }

            XmlUaNodeFactoryManager factory;

            s_pLimitAlarm_John = new OpcUa::ExclusiveLimitAlarmType(pChild, &factory, pTypeNodeConfig, NULL);
            addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pLimitAlarm_John, OpcUaId_HasComponent);
            UA_ASSERT(addStatus.isGood());
            s_pLimitAlarm_John->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
            std::list<UaBase::BaseNode*>::iterator it = lstBaseNodes.begin();
            while (it != lstBaseNodes.end())
            {
                delete *it;
                it++;
            }
        }

        {
            std::list<UaBase::BaseNode*> lstBaseNodes;
            UaBase::Object *pChild = new UaBase::Object(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators, nsTypeIdx));
            lstBaseNodes.push_back(pChild);
            pChild->setBrowseName(UaQualifiedName("LimitAlarm_Operators", Access::NodeManagerAccess::getTypeNamespace()));
            pChild->setDisplayName(UaLocalizedText("", "LimitAlarm_Operators"));
            pChild->setDescription(UaLocalizedText("", "Only members of the operators group can receive events for this alarm object."));
            {
                UaBase::BaseNode *pParent = pChild;
                UaBase::BaseNode *pChild2;
                UaBase::FullReference reference;
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AckedState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("AckedState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "AckedState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AckedState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Acknowledge, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Acknowledge", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Acknowledge"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Acknowledge_InputArguments, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InputArguments", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InputArguments"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Argument);
                    {
                        UaExtensionObjectArray extensionObjectArray;
                        extensionObjectArray.create(2);
                        UaArgument argument;
                        argument.setArgument("EventId", OpcUaId_ByteString, -1, UaUInt32Array(), UaLocalizedText("", "The identifier for the event to comment."));
                        argument.toExtensionObject(extensionObjectArray[ 0 ]);
                        argument.setArgument("Comment", OpcUaId_LocalizedText, -1, UaUInt32Array(), UaLocalizedText("", "The comment to add to the condition."));
                        argument.toExtensionObject(extensionObjectArray[ 1 ]);
                        UaVariant value;
                        value.setExtensionObjectArray(extensionObjectArray);
                        ((UaBase::Variable*)pChild3)->setValue(value);
                    }
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ActiveState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ActiveState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ActiveState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ActiveState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AddComment, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("AddComment", 0));
                pChild2->setDisplayName(UaLocalizedText("", "AddComment"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AddComment_InputArguments, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("InputArguments", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "InputArguments"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Argument);
                    {
                        UaExtensionObjectArray extensionObjectArray;
                        extensionObjectArray.create(2);
                        UaArgument argument;
                        argument.setArgument("EventId", OpcUaId_ByteString, -1, UaUInt32Array(), UaLocalizedText("", "The identifier for the event to comment."));
                        argument.toExtensionObject(extensionObjectArray[ 0 ]);
                        argument.setArgument("Comment", OpcUaId_LocalizedText, -1, UaUInt32Array(), UaLocalizedText("", "The comment to add to the condition."));
                        argument.toExtensionObject(extensionObjectArray[ 1 ]);
                        UaVariant value;
                        value.setExtensionObjectArray(extensionObjectArray);
                        ((UaBase::Variable*)pChild3)->setValue(value);
                    }
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_BranchId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("BranchId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "BranchId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ClientUserId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ClientUserId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ClientUserId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Comment, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Comment", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Comment"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Comment_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ConditionClassId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionClassId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionClassId"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ConditionClassName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionClassName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionClassName"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ConditionName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ConditionName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ConditionName"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Disable, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Disable", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Disable"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Method(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Enable, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Enable", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Enable"));
                ((UaBase::Method*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EnabledState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EnabledState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EnabledState"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_TwoStateVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EnabledState_Id, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("Id", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "Id"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_Boolean);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EventId, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EventId", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EventId"));
                pChild2->setDescription(UaLocalizedText("", "A globally unique identifier for the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_ByteString);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EventType, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("EventType", 0));
                pChild2->setDisplayName(UaLocalizedText("", "EventType"));
                pChild2->setDescription(UaLocalizedText("", "The identifier for the event type."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_InputNode, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("InputNode", 0));
                pChild2->setDisplayName(UaLocalizedText("", "InputNode"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LastSeverity, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LastSeverity", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LastSeverity"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UInt16);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LastSeverity_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Object(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LimitState, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LimitState", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LimitState"));
                ((UaBase::Object*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Object*)pChild2)->setTypeDefinitionId(OpcUaId_ExclusiveLimitStateMachineType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LimitState_CurrentState, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("CurrentState", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "CurrentState"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_LocalizedText);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_FiniteStateVariableType);
                    reference.setReferenceTypeId(OpcUaId_HasComponent);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                    {
                        UaBase::BaseNode *pChild4;
                        pChild4 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LimitState_CurrentState_Id, nsTypeIdx));
                        lstBaseNodes.push_back(pChild4);
                        pChild4->setBrowseName(UaQualifiedName("Id", 0));
                        pChild4->setDisplayName(UaLocalizedText("", "Id"));
                        ((UaBase::Variable*)pChild4)->setAccessLevel(1);
                        ((UaBase::Variable*)pChild4)->setUserAccessLevel(1);
                        ((UaBase::Variable*)pChild4)->setValueRank(-1);
                        ((UaBase::Variable*)pChild4)->setDataTypeId(OpcUaId_NodeId);
                        ((UaBase::Variable*)pChild4)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                        ((UaBase::Variable*)pChild4)->setTypeDefinitionId(OpcUaId_PropertyType);
                        reference.setReferenceTypeId(OpcUaId_HasProperty);
                        reference.setSourceNode(pChild3);
                        reference.setTargetNode(pChild4);
                        pChild3->addFullReference(reference);
                        pChild4->addFullReference(reference);
                        pChild4->setParentNodeId(pChild3->nodeId());
                    }
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LocalTime, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("LocalTime", 0));
                pChild2->setDisplayName(UaLocalizedText("", "LocalTime"));
                pChild2->setDescription(UaLocalizedText("", "Information about the local time where the event originated."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_TimeZoneDataType);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Message, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Message", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Message"));
                pChild2->setDescription(UaLocalizedText("", "A localized description of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_LocalizedText);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Quality, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Quality", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Quality"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_StatusCode);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_ConditionVariableType);
                reference.setReferenceTypeId(OpcUaId_HasComponent);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                {
                    UaBase::BaseNode *pChild3;
                    pChild3 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Quality_SourceTimestamp, nsTypeIdx));
                    lstBaseNodes.push_back(pChild3);;
                    pChild3->setBrowseName(UaQualifiedName("SourceTimestamp", 0));
                    pChild3->setDisplayName(UaLocalizedText("", "SourceTimestamp"));
                    ((UaBase::Variable*)pChild3)->setAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setUserAccessLevel(1);
                    ((UaBase::Variable*)pChild3)->setValueRank(-1);
                    ((UaBase::Variable*)pChild3)->setDataTypeId(OpcUaId_UtcTime);
                    ((UaBase::Variable*)pChild3)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                    ((UaBase::Variable*)pChild3)->setTypeDefinitionId(OpcUaId_PropertyType);
                    reference.setReferenceTypeId(OpcUaId_HasProperty);
                    reference.setSourceNode(pChild2);
                    reference.setTargetNode(pChild3);
                    pChild2->addFullReference(reference);
                    pChild3->addFullReference(reference);
                    pChild3->setParentNodeId(pChild2->nodeId());
                }
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ReceiveTime, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("ReceiveTime", 0));
                pChild2->setDisplayName(UaLocalizedText("", "ReceiveTime"));
                pChild2->setDescription(UaLocalizedText("", "When the server received the event from the underlying system."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UtcTime);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Retain, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Retain", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Retain"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Boolean);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Severity, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Severity", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Severity"));
                pChild2->setDescription(UaLocalizedText("", "Indicates how urgent an event is."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UInt16);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_SourceName, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SourceName", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SourceName"));
                pChild2->setDescription(UaLocalizedText("", "A description of the source of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_String);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_SourceNode, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SourceNode", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SourceNode"));
                pChild2->setDescription(UaLocalizedText("", "The source of the event."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_NodeId);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_SuppressedOrShelved, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("SuppressedOrShelved", 0));
                pChild2->setDisplayName(UaLocalizedText("", "SuppressedOrShelved"));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_Boolean);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
                pChild2 = new UaBase::Variable(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Time, nsTypeIdx));
                lstBaseNodes.push_back(pChild2);
                pChild2->setBrowseName(UaQualifiedName("Time", 0));
                pChild2->setDisplayName(UaLocalizedText("", "Time"));
                pChild2->setDescription(UaLocalizedText("", "When the event occurred."));
                ((UaBase::Variable*)pChild2)->setAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setUserAccessLevel(1);
                ((UaBase::Variable*)pChild2)->setValueRank(-1);
                ((UaBase::Variable*)pChild2)->setDataTypeId(OpcUaId_UtcTime);
                ((UaBase::Variable*)pChild2)->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
                ((UaBase::Variable*)pChild2)->setTypeDefinitionId(OpcUaId_PropertyType);
                reference.setReferenceTypeId(OpcUaId_HasProperty);
                reference.setSourceNode(pParent);
                reference.setTargetNode(pChild2);
                pParent->addFullReference(reference);
                pChild2->addFullReference(reference);
                pChild2->setParentNodeId(pParent->nodeId());
            }

            XmlUaNodeFactoryManager factory;

            s_pLimitAlarm_Operators = new OpcUa::ExclusiveLimitAlarmType(pChild, &factory, pTypeNodeConfig, NULL);
            addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pLimitAlarm_Operators, OpcUaId_HasComponent);
            UA_ASSERT(addStatus.isGood());
            s_pLimitAlarm_Operators->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
            std::list<UaBase::BaseNode*>::iterator it = lstBaseNodes.begin();
            while (it != lstBaseNodes.end())
            {
                delete *it;
                it++;
            }
        }
#endif // SUPPORT_Event_Subscription_Server_Facet

        // ------------------------------------------------------------------------
        // Method SetValue_All
        // ------------------------------------------------------------------------
        s_pSetValue_AllMethod = new OpcUa::BaseMethod(UaNodeId(AccessId_AccessPermissionObjectType_SetValue_All, nsTypeIdx), "SetValue_All", Access::NodeManagerAccess::getTypeNamespace());
        s_pSetValue_AllMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pSetValue_AllMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
        // ------------------------------------------------------------------------
        // Add InputArguments
        // Create property
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(AccessId_AccessPermissionObjectType_SetValue_All_InputArguments, nsTypeIdx),       // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            1,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        // Argument Value
        pPropertyArg->setArgument(
            0,                      // Index of the argument
            "Value",             // Name of the argument
            UaNodeId(OpcUaId_Double),// Data type of the argument
            -1,                     // Array rank of the argument
            nullarray,        // Array dimension
            UaLocalizedText("", "")); // Description
        // Add property to method
        pTypeNodeConfig->addNodeAndReference(s_pSetValue_AllMethod, pPropertyArg, OpcUaId_HasProperty);

        // ------------------------------------------------------------------------
        // Method SetValue_John
        // ------------------------------------------------------------------------
        s_pSetValue_JohnMethod = new OpcUa::BaseMethod(UaNodeId(AccessId_AccessPermissionObjectType_SetValue_John, nsTypeIdx), "SetValue_John", Access::NodeManagerAccess::getTypeNamespace());
        s_pSetValue_JohnMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pSetValue_JohnMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
        // ------------------------------------------------------------------------
        // Add InputArguments
        // Create property
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(AccessId_AccessPermissionObjectType_SetValue_John_InputArguments, nsTypeIdx),       // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            1,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        // Argument Value
        pPropertyArg->setArgument(
            0,                      // Index of the argument
            "Value",             // Name of the argument
            UaNodeId(OpcUaId_Double),// Data type of the argument
            -1,                     // Array rank of the argument
            nullarray,        // Array dimension
            UaLocalizedText("", "")); // Description
        // Add property to method
        pTypeNodeConfig->addNodeAndReference(s_pSetValue_JohnMethod, pPropertyArg, OpcUaId_HasProperty);

        // ------------------------------------------------------------------------
        // Method SetValue_Operators
        // ------------------------------------------------------------------------
        s_pSetValue_OperatorsMethod = new OpcUa::BaseMethod(UaNodeId(AccessId_AccessPermissionObjectType_SetValue_Operators, nsTypeIdx), "SetValue_Operators", Access::NodeManagerAccess::getTypeNamespace());
        s_pSetValue_OperatorsMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pSetValue_OperatorsMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
        // ------------------------------------------------------------------------
        // Add InputArguments
        // Create property
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(AccessId_AccessPermissionObjectType_SetValue_Operators_InputArguments, nsTypeIdx),       // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            1,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        // Argument Value
        pPropertyArg->setArgument(
            0,                      // Index of the argument
            "Value",             // Name of the argument
            UaNodeId(OpcUaId_Double),// Data type of the argument
            -1,                     // Array rank of the argument
            nullarray,        // Array dimension
            UaLocalizedText("", "")); // Description
        // Add property to method
        pTypeNodeConfig->addNodeAndReference(s_pSetValue_OperatorsMethod, pPropertyArg, OpcUaId_HasProperty);


        // Mandatory variable Value
        defaultValue.setDouble(0);
        s_pValue = new OpcUa::BaseDataVariableType(UaNodeId(AccessId_AccessPermissionObjectType_Value, nsTypeIdx), "Value", Access::NodeManagerAccess::getTypeNamespace(), defaultValue, 1, pTypeNodeConfig);
        s_pValue->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, s_pValue, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
        addStatus = pTypeNodeConfig->addUaReference(pObjectType, s_pValue, OpcUaId_HasEventSource);
        UA_ASSERT(addStatus.isGood());

        pTypeNodeConfig->addUaReference(s_pValue->nodeId(), UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All, nsTypeIdx), OpcUaId_HasCondition);
        pTypeNodeConfig->addUaReference(s_pValue->nodeId(), UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John, nsTypeIdx), OpcUaId_HasCondition);
        pTypeNodeConfig->addUaReference(s_pValue->nodeId(), UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators, nsTypeIdx), OpcUaId_HasCondition);
    }
}

/** Clear the static members of the class
*/
void AccessPermissionObjectTypeBase::clearStaticMembers()
{
    s_typeNodesCreated = false;

#if SUPPORT_Event_Subscription_Server_Facet
    s_pLimitAlarm_All = NULL;
    s_pLimitAlarm_John = NULL;
    s_pLimitAlarm_Operators = NULL;
#endif // SUPPORT_Event_Subscription_Server_Facet
    s_pValue = NULL;
}

/** Returns the type definition NodeId for the AccessPermissionObjectType
*/
UaNodeId AccessPermissionObjectTypeBase::typeDefinitionId() const
{
    UaNodeId ret(AccessId_AccessPermissionObjectType, NodeManagerAccess::getTypeNamespace());
    return ret;
}

/** Implementation of the MethodManager interface method beginCall
*/
UaStatus AccessPermissionObjectTypeBase::beginCall(
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
UaStatus AccessPermissionObjectTypeBase::call(
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
            if ( ( (m_pSetValue_AllMethod != NULL) && (pMethod->nodeId() == m_pSetValue_AllMethod->nodeId()) )
                 || ( (s_pSetValue_AllMethod != NULL) && (pMethod->nodeId() == s_pSetValue_AllMethod->nodeId()) ) )
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

                    OpcUa_Double Value;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toDouble(Value);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->SetValue_All(serviceContext, Value);
                    }
                }

            }
            else if ( ( (m_pSetValue_JohnMethod != NULL) && (pMethod->nodeId() == m_pSetValue_JohnMethod->nodeId()) )
                      || ( (s_pSetValue_JohnMethod != NULL) && (pMethod->nodeId() == s_pSetValue_JohnMethod->nodeId()) ) )
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

                    OpcUa_Double Value;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toDouble(Value);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->SetValue_John(serviceContext, Value);
                    }
                }

            }
            else if ( ( (m_pSetValue_OperatorsMethod != NULL) && (pMethod->nodeId() == m_pSetValue_OperatorsMethod->nodeId()) )
                      || ( (s_pSetValue_OperatorsMethod != NULL) && (pMethod->nodeId() == s_pSetValue_OperatorsMethod->nodeId()) ) )
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

                    OpcUa_Double Value;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toDouble(Value);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->SetValue_Operators(serviceContext, Value);
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
 *  Sets the Value value
 */
void AccessPermissionObjectTypeBase::setValue(OpcUa_Double Value)
{
    UaVariant value;
    value.setDouble(Value);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    m_pValue->setValue(NULL, dataValue, OpcUa_False);
}

/**
 *  Returns the value of Value
 */
OpcUa_Double AccessPermissionObjectTypeBase::getValue() const
{
    UaVariant defaultValue;
    OpcUa_Double ret = 0;
    UaDataValue dataValue(m_pValue->value(NULL));
    defaultValue = *dataValue.value();
    defaultValue.toDouble(ret);
    return ret;
}

#if SUPPORT_Event_Subscription_Server_Facet
/** Returns the LimitAlarm_All node.
 */
OpcUa::ExclusiveLimitAlarmType* AccessPermissionObjectTypeBase::getLimitAlarm_All()
{
    return m_pLimitAlarm_All;
}

/** Returns the LimitAlarm_John node.
 */
OpcUa::ExclusiveLimitAlarmType* AccessPermissionObjectTypeBase::getLimitAlarm_John()
{
    return m_pLimitAlarm_John;
}

/** Returns the LimitAlarm_Operators node.
 */
OpcUa::ExclusiveLimitAlarmType* AccessPermissionObjectTypeBase::getLimitAlarm_Operators()
{
    return m_pLimitAlarm_Operators;
}
#endif // SUPPORT_Event_Subscription_Server_Facet

/** Returns the Value node.
 */
OpcUa::BaseDataVariableType* AccessPermissionObjectTypeBase::getValueNode()
{
    return m_pValue;
}

/** Returns the SetValue_All node.
  */
OpcUa::BaseMethod* AccessPermissionObjectTypeBase::getSetValue_All()
{
    return m_pSetValue_AllMethod;
}
/** Returns the SetValue_John node.
  */
OpcUa::BaseMethod* AccessPermissionObjectTypeBase::getSetValue_John()
{
    return m_pSetValue_JohnMethod;
}
/** Returns the SetValue_Operators node.
  */
OpcUa::BaseMethod* AccessPermissionObjectTypeBase::getSetValue_Operators()
{
    return m_pSetValue_OperatorsMethod;
}

/** Applys the NodeAccessInfo set at the AccessPermissionObjectTypeBase and its children. */
void AccessPermissionObjectTypeBase::useAccessInfoFromType()
{
    OpcUa::BaseObjectType::useAccessInfoFromType();

    // instance node
    OpcUa_Int16 nsTypeIdx = NodeManagerAccess::getTypeNamespace();
    UaNode *pTypeNode = NodeManagerRoot::CreateRootNodeManager()->getNode(UaNodeId(AccessId_AccessPermissionObjectType, nsTypeIdx));
    if (pTypeNode)
    {
        useAccessInfoFrom(pTypeNode);
        pTypeNode->releaseReference();
    }

    // children
#if SUPPORT_Event_Subscription_Server_Facet
    m_pLimitAlarm_All->useAccessInfoFromInstance(s_pLimitAlarm_All);
    m_pLimitAlarm_John->useAccessInfoFromInstance(s_pLimitAlarm_John);
    m_pLimitAlarm_Operators->useAccessInfoFromInstance(s_pLimitAlarm_Operators);
#endif // SUPPORT_Event_Subscription_Server_Facet
    m_pValue->useAccessInfoFromInstance(s_pValue);
    m_pValue->useAccessInfoFromInstance(s_pValue);
#if SUPPORT_Method_Server_Facet
    m_pSetValue_AllMethod->useAccessInfoFrom(s_pSetValue_AllMethod);
    m_pSetValue_JohnMethod->useAccessInfoFrom(s_pSetValue_JohnMethod);
    m_pSetValue_OperatorsMethod->useAccessInfoFrom(s_pSetValue_OperatorsMethod);
#endif // SUPPORT_Method_Server_Facet
}

/** Uses the NodeAccessInfo of pOther and its children. */
void AccessPermissionObjectTypeBase::useAccessInfoFromInstance(AccessPermissionObjectTypeBase *pOther)
{
    OpcUa::BaseObjectType::useAccessInfoFromInstance(pOther);

    // children
#if SUPPORT_Event_Subscription_Server_Facet
    m_pLimitAlarm_All->useAccessInfoFromInstance(pOther->m_pLimitAlarm_All);
    m_pLimitAlarm_John->useAccessInfoFromInstance(pOther->m_pLimitAlarm_John);
    m_pLimitAlarm_Operators->useAccessInfoFromInstance(pOther->m_pLimitAlarm_Operators);
#endif // SUPPORT_Event_Subscription_Server_Facet
    m_pValue->useAccessInfoFromInstance(pOther->m_pValue);
    m_pValue->useAccessInfoFromInstance(pOther->m_pValue);
#if SUPPORT_Method_Server_Facet
    m_pSetValue_AllMethod->useAccessInfoFrom(pOther->m_pSetValue_AllMethod);
    m_pSetValue_JohnMethod->useAccessInfoFrom(pOther->m_pSetValue_JohnMethod);
    m_pSetValue_OperatorsMethod->useAccessInfoFrom(pOther->m_pSetValue_OperatorsMethod);
#endif // SUPPORT_Method_Server_Facet
}

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission



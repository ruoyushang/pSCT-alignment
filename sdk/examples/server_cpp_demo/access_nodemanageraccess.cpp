/******************************************************************************
** access_nodemanageraccess.cpp
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

#include "access_nodemanageraccess.h"
#include "nodemanagerroot.h"
#include "demo_nodemanagerdemo.h"
#if SUPPORT_Event_Subscription_Server_Facet
#include "opcua_basemodelchangeeventtypedata.h"
#endif // SUPPORT_Event_Subscription_Server_Facet
#include "nodeaccessinfo.h"
#include "myservercallback.h"
#include "access_accesspermissionobjecttype.h"

// Include type headers

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
namespace Access {

/** Construction of the class NodeManagerAccess.
 */
NodeManagerAccess::NodeManagerAccess(
    OpcUa_Boolean firesEvents,  //!< [in] Flag indicating if the NodeManager should activate the EventManager
    OpcUa_Int32 nHashTableSize) //!< [in] Size of the hash table. The default value is 10.007.<br>
                                //   The size can be smaller than the total number of nodes managed
                                //   by the node manager but it is better for the performance to avoid
                                //   collisions with a large size.
                                //   Prefered sizes are 1.009, 10.007, 100.003, 1.000.003, 10.000.019.
: NodeManagerAccessBase(firesEvents, nHashTableSize)
{
#if SUPPORT_Method_Server_Facet
    m_bAdvancedNodesCreated = false;
#endif // SUPPORT_Method_Server_Facet

    m_userIdRoot = 0;
    m_userIdJoe = UserIdJoe;
    m_userIdJohn = UserIdJohn;
    m_userIdSue = UserIdSue;

    m_groupIdSecurityAdmin = 0;
    m_groupIdConfigAdmin = 0;
    m_groupIdUsers = GroupIdUsers;
    m_groupIdOperators = GroupIdOperators;
}

/** Destroys the NodeManagerAccess object and all nodes managed by the node manager
 */
NodeManagerAccess::~NodeManagerAccess()
{
}

/** Finish start up in derived class after starting up base class.
 */
UaStatus NodeManagerAccess::afterStartUp()
{
    // read default roles from configuration
    m_pServerManager->getServerConfig()->getIdsForDefaultRoles(
        m_userIdRoot,
        m_groupIdSecurityAdmin,
        m_groupIdConfigAdmin);

    // The address space is created by the base class
    UaStatus ret = NodeManagerAccessBase::afterStartUp();
    // Add custom start up code here
    return ret;
}

/** Start shut down in derived class before shutting down base class.
*/
UaStatus NodeManagerAccess::beforeShutDown()
{
    UaStatus ret;
    return ret;
}

#if SUPPORT_Event_Subscription_Server_Facet
/** Event handlers for AcknowledgeableConditionType method Acknowledge called from UA client
 *  @return Status code.
 */
UaStatus NodeManagerAccess::OnAcknowledge(
    const ServiceContext&  serviceContext, //!< [in] The client service context including the session object used for the call
    OpcUa::AcknowledgeableConditionType* pCondition, //!< [in] The affected acknowledgeable condition object
    const UaByteString&    EventId,        //!< [in] The EventId of the event the acknowledge is related to
    const UaLocalizedText& Comment)        //!< [in] The comment provied by the client
{
    OpcUa_ReferenceParameter(EventId);

    UaByteString userEventId;
    EventManagerBase::getUserEventId(EventId, userEventId);
    if ( userEventId.length() > 0 )
    {
        OpcUa::AcknowledgeableConditionTypeData* pBranch = (OpcUa::AcknowledgeableConditionTypeData*)pCondition->getBranchByEventId(EventId, userEventId);
        if ( pBranch == NULL )
        {
            return OpcUa_BadInvalidState;
        }

        // Lock the event data for change since a refresh may access the data in parallel
        pBranch->lockEventData();
        pBranch->setAckedState(OpcUa_True);
        pBranch->setComment(Comment);
        pBranch->setClientUserId(serviceContext.pSession()->getClientUserId());
        pBranch->setMessage(UaLocalizedText("en", "Branch state acknowledged by UA client"));
        // ToDo Check active state
        pBranch->setRetain(OpcUa_False);
        pBranch->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        // Unlock event data before fire event
        pBranch->unlockEventData();
        fireEvent(pBranch);

        pCondition->deleteBranch(pBranch->getBranchId());
    }
    else
    {
        // Check if the condition is unacknowledged
        if ( pCondition->getAckedStateBool() != OpcUa_False )
        {
            return OpcUa_BadInvalidState;
        }

        // Chance condition to acknowledged
        pCondition->setAckedState(OpcUa_True);
        pCondition->setComment(Comment);
        pCondition->setClientUserId(serviceContext.pSession()->getClientUserId());
        pCondition->setMessage(UaLocalizedText("en", "Condition state acknowledged by UA client"));

        OpcUa::AlarmConditionType* pAlarmCondition = (OpcUa::AlarmConditionType*)pCondition;
        if ( pAlarmCondition->getActiveStateBool() == OpcUa_False )
        {
            pCondition->setRetain(OpcUa_False);
        }

        // Trigger state change event
        pCondition->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
    }

    return OpcUa_Good;
}

/** Event handlers for AcknowledgeableConditionType method Confirm called from UA client
 *  @return Status code.
 */
UaStatus NodeManagerAccess::OnConfirm(
    const ServiceContext&  serviceContext, //!< [in] The client service context including the session object used for the call
    OpcUa::AcknowledgeableConditionType* pCondition, //!< [in] The affected acknowledgeable condition object
    const UaByteString&    EventId,        //!< [in] The EventId of the event the acknowledge is related to
    const UaLocalizedText& Comment)        //!< [in] The comment provied by the client
{
    OpcUa_ReferenceParameter(EventId);

    // Check if the condition is unacknowledged
    if ( pCondition->getConfirmedStateBool() != OpcUa_False )
    {
        return OpcUa_BadInvalidState;
    }

    // Chance condition to acknowledged
    pCondition->setConfirmedState(OpcUa_True);
    pCondition->setComment(Comment);
    pCondition->setClientUserId(serviceContext.pSession()->getClientUserId());
    pCondition->setMessage(UaLocalizedText("en", "Condition state confirmed by UA client"));

    // Trigger state change event
    pCondition->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());

    return OpcUa_Good;
}
#endif // SUPPORT_Event_Subscription_Server_Facet

#if SUPPORT_Method_Server_Facet
/** Method AddAdvancedNodes
  */
UaStatus NodeManagerAccess::Demo_AccessRights_Options_AddAdvancedNodes(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;
    if (m_bAdvancedNodesCreated)
    {
        ret = OpcUa_BadInvalidState;
    }
    else
    {
        m_bAdvancedNodesCreated = true;
        NodeManagerAccessBase::createAdvancedNodes();
        createAdditionalReferences();
        setPermissionsForLimitedNodes();
        // Create model change event
#if SUPPORT_Event_Subscription_Server_Facet
        UaModelChangeStructureDataTypes changes;
        UaModelChangeStructureDataType change;

        // Prepare model change structure event data
        changes.create(6);
        // Only a node with a NodeVersion property (DemoId_Demo_AccessRights)
        // ToDo: Add node version property
        // Folder Access_All
        change.setModelChangeStructureDataType(
            UaNodeId(DemoId_Demo_AccessRights_Access_All, Demo::NodeManagerDemo::getTypeNamespace()),
            OpcUaId_FolderType,
            OpcUa_ModelChangeStructureVerbMask_ReferenceAdded);
        change.copyTo(&changes[0]);
        // Folder Access_John
        change.setModelChangeStructureDataType(
            UaNodeId(DemoId_Demo_AccessRights_Access_John, Demo::NodeManagerDemo::getTypeNamespace()),
            OpcUaId_FolderType,
            OpcUa_ModelChangeStructureVerbMask_ReferenceAdded);
        change.copyTo(&changes[1]);
        // Folder Access_Operators
        change.setModelChangeStructureDataType(
            UaNodeId(DemoId_Demo_AccessRights_Access_Operators, Demo::NodeManagerDemo::getTypeNamespace()),
            OpcUaId_FolderType,
            OpcUa_ModelChangeStructureVerbMask_ReferenceAdded);
        change.copyTo(&changes[2]);
        // Folder Browse_All
        change.setModelChangeStructureDataType(
            UaNodeId(DemoId_Demo_AccessRights_Browse_All, Demo::NodeManagerDemo::getTypeNamespace()),
            OpcUaId_FolderType,
            OpcUa_ModelChangeStructureVerbMask_ReferenceAdded);
        change.copyTo(&changes[3]);
        // Folder Browse_John
        change.setModelChangeStructureDataType(
            UaNodeId(DemoId_Demo_AccessRights_Browse_John, Demo::NodeManagerDemo::getTypeNamespace()),
            OpcUaId_FolderType,
            OpcUa_ModelChangeStructureVerbMask_ReferenceAdded);
        change.copyTo(&changes[4]);
        // Folder Browse_Operators
        change.setModelChangeStructureDataType(
            UaNodeId(DemoId_Demo_AccessRights_Browse_Operators, Demo::NodeManagerDemo::getTypeNamespace()),
            OpcUaId_FolderType,
            OpcUa_ModelChangeStructureVerbMask_ReferenceAdded);
        change.copyTo(&changes[5]);

        // Prepare model change event
        OpcUa::GeneralModelChangeEventTypeData eventData;
        eventData.setMessage(UaLocalizedText("", "Node added"));
        eventData.setChanges(changes);

        // Fire model change event
        fireEvent(&eventData);
#endif // SUPPORT_Event_Subscription_Server_Facet
    }
    return ret;
}
#endif // SUPPORT_Method_Server_Facet

void NodeManagerAccess::createAdditionalReferences()
{
    UaStatus ret;
    ret = addUaReference(
        UaNodeId(DemoId_Demo_AccessRights_Access_John, Demo::NodeManagerDemo::getTypeNamespace()),
        UaNodeId(AccessId_Demo_AccessRights_Browse_All_John_ReadAttributes, getNameSpaceIndex()),
        OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    ret = addUaReference(
        UaNodeId(DemoId_Demo_AccessRights_Access_Operators, Demo::NodeManagerDemo::getTypeNamespace()),
        UaNodeId(AccessId_Demo_AccessRights_Browse_All_Operators_ReadAttributes, getNameSpaceIndex()),
        OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    ret = addUaReference(
        UaNodeId(DemoId_Demo_AccessRights_Browse_John, Demo::NodeManagerDemo::getTypeNamespace()),
        UaNodeId(AccessId_Demo_AccessRights_Browse_All_John_ReadAttributes, getNameSpaceIndex()),
        OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    ret = addUaReference(
        UaNodeId(DemoId_Demo_AccessRights_Browse_John, Demo::NodeManagerDemo::getTypeNamespace()),
        UaNodeId(AccessId_Demo_AccessRights_Browse_All_Operators_ReadAttributes, getNameSpaceIndex()),
        OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    ret = addUaReference(
        UaNodeId(DemoId_Demo_AccessRights_Browse_Operators, Demo::NodeManagerDemo::getTypeNamespace()),
        UaNodeId(AccessId_Demo_AccessRights_Browse_All_John_ReadAttributes, getNameSpaceIndex()),
        OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    ret = addUaReference(
        UaNodeId(DemoId_Demo_AccessRights_Browse_Operators, Demo::NodeManagerDemo::getTypeNamespace()),
        UaNodeId(AccessId_Demo_AccessRights_Browse_All_Operators_ReadAttributes, getNameSpaceIndex()),
        OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
}

void NodeManagerAccess::setPermissionsForLimitedNodes()
{
    UaNode* pNode;

    // configure variables
    pNode = findNode(UaNodeId(AccessId_Demo_AccessRights_Access_All_All_RequireEncryption, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OPERATION |
            (OpcUa_UInt32)NodeAccessInfo::UA_ALL_ENCRYPTION_REQUIRED,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(AccessId_Demo_AccessRights_Access_All_All_RequireSigning, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OPERATION |
            NodeAccessInfo::UA_ALL_SIGNING_REQUIRED,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(AccessId_Demo_AccessRights_Browse_All_John_ReadAttributes, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(AccessId_Demo_AccessRights_Browse_All_Operators_ReadAttributes, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    // configure Access permission on the AccessPermissionObjectType - in the next step we copy he access info to the instances
#if SUPPORT_Method_Server_Facet
    // configure methods with limited access on AccessPermissionObjectType
    pNode = findNode(UaNodeId(AccessId_AccessPermissionObjectType_SetValue_All, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OPERATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(AccessId_AccessPermissionObjectType_SetValue_John, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(AccessId_AccessPermissionObjectType_SetValue_Operators, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }
#endif // SUPPORT_Method_Server_Facet

// set the access permissions of the alarms
#if SUPPORT_Event_Subscription_Server_Facet
    pNode = findNode(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_All, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OPERATION,
            m_userIdJohn,
            m_groupIdOperators));

        OpcUa::ExclusiveLimitAlarmType* pAlarm = (OpcUa::ExclusiveLimitAlarmType*) pNode;
        pAlarm->setEventAccessInfo((NodeAccessInfo*)pNode->getAccessInfo());
    }

    pNode = findNode(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_John, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE,
            m_userIdJohn,
            m_groupIdOperators));

        OpcUa::ExclusiveLimitAlarmType* pAlarm = (OpcUa::ExclusiveLimitAlarmType*) pNode;
        pAlarm->setEventAccessInfo((NodeAccessInfo*)pNode->getAccessInfo());
    }

    pNode = findNode(UaNodeId(AccessId_AccessPermissionObjectType_LimitAlarm_Operators, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE,
            m_userIdJohn,
            m_groupIdOperators));

        OpcUa::ExclusiveLimitAlarmType* pAlarm = (OpcUa::ExclusiveLimitAlarmType*) pNode;
        pAlarm->setEventAccessInfo((NodeAccessInfo*)pNode->getAccessInfo());
    }
#endif // SUPPORT_Event_Subscription_Server_Facet

    // apply the access permissions of the type to the instances
    pNode = findNode(UaNodeId(AccessId_Server_Object_All, getNameSpaceIndex()));
    if ( pNode )
    {
        AccessPermissionObjectType* pObject = (AccessPermissionObjectType*)pNode;
        if ( pObject )
        {
            // copy access info
            pObject->useAccessInfoFromType();
        }
        else
        {
            UA_ASSERT(OpcUa_False);
        }
    }

    pNode = findNode(UaNodeId(AccessId_Server_Object_John, getNameSpaceIndex()));
    if ( pNode )
    {
        AccessPermissionObjectType* pObject = (AccessPermissionObjectType*)pNode;
        if ( pObject )
        {
            // copy access info
            pObject->useAccessInfoFromType();
        }
        else
        {
            UA_ASSERT(OpcUa_False);
        }
    }

    pNode = findNode(UaNodeId(AccessId_Server_Object_Operators, getNameSpaceIndex()));
    if ( pNode )
    {
        AccessPermissionObjectType* pObject = (AccessPermissionObjectType*)pNode;
        if ( pObject )
        {
            // copy access info
            pObject->useAccessInfoFromType();
        }
        else
        {
            UA_ASSERT(OpcUa_False);
        }
    }

    // configure objects with limited access to restrict use of methods on this object
    pNode = findNode(UaNodeId(AccessId_Server_Object_All, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OPERATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(AccessId_Server_Object_John, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(AccessId_Server_Object_Operators, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }
}

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission

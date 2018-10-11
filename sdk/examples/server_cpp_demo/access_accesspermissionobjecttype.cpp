/******************************************************************************
** access_accesspermissionobjecttype.cpp
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

#include "access_accesspermissionobjecttype.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
namespace Access {

/** Construction of the class AccessPermissionObjectType.
 This constructor is used if the object is created based on an instance declaration of an object type. Since the only difference between the
 instance declaration object and the instance object are the node id, the pointer to the instance declaration node is used to get the attribute
 values other than NodeId from the instance declaration node.

 The object is reference counted and can not be deleted directly. The reference counter is set to one after the object is created.
 If the object was successfully added to a NodeManager using the methods NodeManagerConfig::addUaNode or NodeManagerConfig::addNodeAndReference the
 NodeManager will release this reference during shut down. If the object was not added to a NodeManager, it must be deleted using the releaseReference
 method. If the interface pointer is used in other places, the availability of the object must be ensured by incrementing the reference counter with
 addReference when the pointer is stored somewhere and decremented with releaseReference if the interface pointer is not longer needed.
 */
AccessPermissionObjectType::AccessPermissionObjectType(
    const UaNodeId& nodeId,          //!< [in] NodeId of the new object
    UaObject* pInstanceDeclarationObject, //!< [in] UaObject interface of the instance declaration node used to provide attribute values other than NodeId
    NodeManagerConfig* pNodeConfig,  //!< [in] Interface pointer to the NodeManagerConfig interface used to add and delete node and references in the address space
    UaMutexRefCounted* pSharedMutex) //!< [in] Shared mutex object used to synchronize access to the object. Can be NULL if no shared mutex is provided
: AccessPermissionObjectTypeBase(nodeId, pInstanceDeclarationObject, pNodeConfig, pSharedMutex)
{
    initialize();
}

/** Construction of the class AccessPermissionObjectType.
 This constructor is used if the object is not created based on an instance declaration. It defines the name and node id of the object. Additional language specific
 names and other attribute values can be set with set methods provided by the class.

 The object is reference counted and can not be deleted directly. The reference counter is set to one after the object is created.
 If the object was successfully added to a NodeManager using the methods NodeManagerConfig::addUaNode or NodeManagerConfig::addNodeAndReference the
 NodeManager will release this reference during shut down. If the object was not added to a NodeManager, it must be deleted using the releaseReference
 method. If the interface pointer is used in other places, the availability of the object must be ensured by incrementing the reference counter with
 addReference when the pointer is stored somewhere and decremented with releaseReference if the interface pointer is not longer needed.
 */
AccessPermissionObjectType::AccessPermissionObjectType(
    const UaNodeId& nodeId,          //!< [in] NodeId of the new object
    const UaString& name,            //!< [in] Name of the new object. Used as browse name and also as display name if no additional language specific names are set.
    OpcUa_UInt16 browseNameNameSpaceIndex, //!< [in] Namespace index used for the browse name
    NodeManagerConfig* pNodeConfig,  //!< [in] Interface pointer to the NodeManagerConfig interface used to add and delete node and references in the address space
    UaMutexRefCounted* pSharedMutex) //!< [in] Shared mutex object used to synchronize access to the object. Can be NULL if no shared mutex is provided
: AccessPermissionObjectTypeBase(nodeId, name, browseNameNameSpaceIndex, pNodeConfig, pSharedMutex)
{
    initialize();
}

/**  Constructs an instance of the class AccessPermissionObjectType with all components.
*/
AccessPermissionObjectType::AccessPermissionObjectType(
    UaBase::Object*    pBaseNode,    //!< [in] The "tree" of nodes that specifies the attributes of the instance and its children
    XmlUaNodeFactoryManager*   pFactory,     //!< [in] The factory to create the children
    NodeManagerConfig* pNodeConfig,  //!< [in] Interface pointer to the NodeManagerConfig interface used to add and delete node and references in the address space
    UaMutexRefCounted* pSharedMutex) //!< [in] Shared mutex object used to synchronize access to the variable. Can be NULL if no shared mutex is provided
: AccessPermissionObjectTypeBase(pBaseNode, pFactory, pNodeConfig, pSharedMutex)
{
    initialize();
}

/** Destruction
*/
AccessPermissionObjectType::~AccessPermissionObjectType()
{
}

/** Create the type related static members of the class
*/
void AccessPermissionObjectType::createTypes()
{
    // Call base class
    AccessPermissionObjectTypeBase::createTypes();

    // Create your static members here
    // This method may be called several times
}

/** Clear the static members of the class
*/
void AccessPermissionObjectType::clearStaticMembers()
{
    // Clear your static members here
    // Call base class
    AccessPermissionObjectTypeBase::clearStaticMembers();
}

/**
 * This method is executable for every user.
 */
UaStatus AccessPermissionObjectType::SetValue_All(
    const ServiceContext& serviceContext /**<General context for the service calls containing
                                             information like the session object,
                                             return diagnostic mask and timeout hint.*/,
    OpcUa_Double Value)
{
    OpcUa_ReferenceParameter(serviceContext);
    setValue(Value);
    return OpcUa_Good;
}

/**
 * This method is executable only for user john.
 */
UaStatus AccessPermissionObjectType::SetValue_John(
    const ServiceContext& serviceContext /**<General context for the service calls containing
                                             information like the session object,
                                             return diagnostic mask and timeout hint.*/,
    OpcUa_Double Value)
{
    OpcUa_ReferenceParameter(serviceContext);
    setValue(Value);
    return OpcUa_Good;
}

/**
 * This method is executable only for members of the operators group.
 */
UaStatus AccessPermissionObjectType::SetValue_Operators(
    const ServiceContext& serviceContext /**<General context for the service calls containing
                                             information like the session object,
                                             return diagnostic mask and timeout hint.*/,
    OpcUa_Double Value)
{
    OpcUa_ReferenceParameter(serviceContext);
    setValue(Value);
    return OpcUa_Good;
}

void AccessPermissionObjectType::setValue(OpcUa_Double Value)
{
    AccessPermissionObjectTypeBase::setValue(Value);

#if SUPPORT_Event_Subscription_Server_Facet
    // LimitAlarm_All
    if ( Value <= m_pLimitAlarm_All->getLowLowLimit() )
    {
        if ( m_pLimitAlarm_All->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::LowLow )
        {
            m_pLimitAlarm_All->setMessage(UaLocalizedText("en", "LimitAlarm_All active in LowLow"));
            m_pLimitAlarm_All->setActiveState(OpcUa_True);
            m_pLimitAlarm_All->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::LowLow);
            m_pLimitAlarm_All->setAckedState(OpcUa_False);
            m_pLimitAlarm_All->setRetain(OpcUa_True);
            m_pLimitAlarm_All->setLastSeverity(m_pLimitAlarm_All->getSeverity());
            m_pLimitAlarm_All->setSeverity(800);
            m_pLimitAlarm_All->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value <= m_pLimitAlarm_All->getLowLimit() )
    {
        if ( m_pLimitAlarm_All->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::Low )
        {
            m_pLimitAlarm_All->setMessage(UaLocalizedText("en", "LimitAlarm_All active in Low"));
            m_pLimitAlarm_All->setActiveState(OpcUa_True);
            m_pLimitAlarm_All->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Low);
            m_pLimitAlarm_All->setAckedState(OpcUa_False);
            m_pLimitAlarm_All->setRetain(OpcUa_True);
            m_pLimitAlarm_All->setLastSeverity(m_pLimitAlarm_All->getSeverity());
            m_pLimitAlarm_All->setSeverity(400);
            m_pLimitAlarm_All->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value >= m_pLimitAlarm_All->getHighHighLimit() )
    {
        if ( m_pLimitAlarm_All->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::HighHigh )
        {
            m_pLimitAlarm_All->setMessage(UaLocalizedText("en", "LimitAlarm_All active in HighHigh"));
            m_pLimitAlarm_All->setActiveState(OpcUa_True);
            m_pLimitAlarm_All->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::HighHigh);
            m_pLimitAlarm_All->setAckedState(OpcUa_False);
            m_pLimitAlarm_All->setRetain(OpcUa_True);
            m_pLimitAlarm_All->setLastSeverity(m_pLimitAlarm_All->getSeverity());
            m_pLimitAlarm_All->setSeverity(800);
            m_pLimitAlarm_All->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value >= m_pLimitAlarm_All->getHighLimit() )
    {
        if ( m_pLimitAlarm_All->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::High )
        {
            m_pLimitAlarm_All->setMessage(UaLocalizedText("en", "LimitAlarm_All active in High"));
            m_pLimitAlarm_All->setActiveState(OpcUa_True);
            m_pLimitAlarm_All->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::High);
            m_pLimitAlarm_All->setAckedState(OpcUa_False);
            m_pLimitAlarm_All->setRetain(OpcUa_True);
            m_pLimitAlarm_All->setLastSeverity(m_pLimitAlarm_All->getSeverity());
            m_pLimitAlarm_All->setSeverity(400);
            m_pLimitAlarm_All->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else
    {
        if ( m_pLimitAlarm_All->getActiveStateBool() != OpcUa_False )
        {
            m_pLimitAlarm_All->setMessage(UaLocalizedText("en", "LimitAlarm_All inactive"));
            m_pLimitAlarm_All->setActiveState(OpcUa_False);
            m_pLimitAlarm_All->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Disabled);
            m_pLimitAlarm_All->setAckedState(OpcUa_True);
            m_pLimitAlarm_All->setRetain(OpcUa_False);
            m_pLimitAlarm_All->setLastSeverity(m_pLimitAlarm_All->getSeverity());
            m_pLimitAlarm_All->setSeverity(400);
            m_pLimitAlarm_All->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }

    // LimitAlarm_John
    if ( Value <= m_pLimitAlarm_John->getLowLowLimit() )
    {
        if ( m_pLimitAlarm_John->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::LowLow )
        {
            m_pLimitAlarm_John->setMessage(UaLocalizedText("en", "LimitAlarm_John active in LowLow"));
            m_pLimitAlarm_John->setActiveState(OpcUa_True);
            m_pLimitAlarm_John->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::LowLow);
            m_pLimitAlarm_John->setAckedState(OpcUa_False);
            m_pLimitAlarm_John->setRetain(OpcUa_True);
            m_pLimitAlarm_John->setLastSeverity(m_pLimitAlarm_John->getSeverity());
            m_pLimitAlarm_John->setSeverity(800);
            m_pLimitAlarm_John->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value <= m_pLimitAlarm_John->getLowLimit() )
    {
        if ( m_pLimitAlarm_John->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::Low )
        {
            m_pLimitAlarm_John->setMessage(UaLocalizedText("en", "LimitAlarm_John active in Low"));
            m_pLimitAlarm_John->setActiveState(OpcUa_True);
            m_pLimitAlarm_John->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Low);
            m_pLimitAlarm_John->setAckedState(OpcUa_False);
            m_pLimitAlarm_John->setRetain(OpcUa_True);
            m_pLimitAlarm_John->setLastSeverity(m_pLimitAlarm_John->getSeverity());
            m_pLimitAlarm_John->setSeverity(400);
            m_pLimitAlarm_John->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value >= m_pLimitAlarm_John->getHighHighLimit() )
    {
        if ( m_pLimitAlarm_John->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::HighHigh )
        {
            m_pLimitAlarm_John->setMessage(UaLocalizedText("en", "LimitAlarm_John active in HighHigh"));
            m_pLimitAlarm_John->setActiveState(OpcUa_True);
            m_pLimitAlarm_John->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::HighHigh);
            m_pLimitAlarm_John->setAckedState(OpcUa_False);
            m_pLimitAlarm_John->setRetain(OpcUa_True);
            m_pLimitAlarm_John->setLastSeverity(m_pLimitAlarm_John->getSeverity());
            m_pLimitAlarm_John->setSeverity(800);
            m_pLimitAlarm_John->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value >= m_pLimitAlarm_John->getHighLimit() )
    {
        if ( m_pLimitAlarm_John->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::High )
        {
            m_pLimitAlarm_John->setMessage(UaLocalizedText("en", "LimitAlarm_John active in High"));
            m_pLimitAlarm_John->setActiveState(OpcUa_True);
            m_pLimitAlarm_John->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::High);
            m_pLimitAlarm_John->setAckedState(OpcUa_False);
            m_pLimitAlarm_John->setRetain(OpcUa_True);
            m_pLimitAlarm_John->setLastSeverity(m_pLimitAlarm_John->getSeverity());
            m_pLimitAlarm_John->setSeverity(400);
            m_pLimitAlarm_John->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else
    {
        if ( m_pLimitAlarm_John->getActiveStateBool() != OpcUa_False )
        {
            m_pLimitAlarm_John->setMessage(UaLocalizedText("en", "LimitAlarm_John inactive"));
            m_pLimitAlarm_John->setActiveState(OpcUa_False);
            m_pLimitAlarm_John->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Disabled);
            m_pLimitAlarm_John->setAckedState(OpcUa_True);
            m_pLimitAlarm_John->setRetain(OpcUa_False);
            m_pLimitAlarm_John->setLastSeverity(m_pLimitAlarm_John->getSeverity());
            m_pLimitAlarm_John->setSeverity(400);
            m_pLimitAlarm_John->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }

    // LimitAlarm_Operators
    if ( Value <= m_pLimitAlarm_Operators->getLowLowLimit() )
    {
        if ( m_pLimitAlarm_Operators->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::LowLow )
        {
            m_pLimitAlarm_Operators->setMessage(UaLocalizedText("en", "LimitAlarm_Operators active in LowLow"));
            m_pLimitAlarm_Operators->setActiveState(OpcUa_True);
            m_pLimitAlarm_Operators->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::LowLow);
            m_pLimitAlarm_Operators->setAckedState(OpcUa_False);
            m_pLimitAlarm_Operators->setRetain(OpcUa_True);
            m_pLimitAlarm_Operators->setLastSeverity(m_pLimitAlarm_Operators->getSeverity());
            m_pLimitAlarm_Operators->setSeverity(800);
            m_pLimitAlarm_Operators->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value <= m_pLimitAlarm_Operators->getLowLimit() )
    {
        if ( m_pLimitAlarm_Operators->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::Low )
        {
            m_pLimitAlarm_Operators->setMessage(UaLocalizedText("en", "LimitAlarm_Operators active in Low"));
            m_pLimitAlarm_Operators->setActiveState(OpcUa_True);
            m_pLimitAlarm_Operators->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Low);
            m_pLimitAlarm_Operators->setAckedState(OpcUa_False);
            m_pLimitAlarm_Operators->setRetain(OpcUa_True);
            m_pLimitAlarm_Operators->setLastSeverity(m_pLimitAlarm_Operators->getSeverity());
            m_pLimitAlarm_Operators->setSeverity(400);
            m_pLimitAlarm_Operators->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value >= m_pLimitAlarm_Operators->getHighHighLimit() )
    {
        if ( m_pLimitAlarm_Operators->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::HighHigh )
        {
            m_pLimitAlarm_Operators->setMessage(UaLocalizedText("en", "LimitAlarm_Operators active in HighHigh"));
            m_pLimitAlarm_Operators->setActiveState(OpcUa_True);
            m_pLimitAlarm_Operators->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::HighHigh);
            m_pLimitAlarm_Operators->setAckedState(OpcUa_False);
            m_pLimitAlarm_Operators->setRetain(OpcUa_True);
            m_pLimitAlarm_Operators->setLastSeverity(m_pLimitAlarm_Operators->getSeverity());
            m_pLimitAlarm_Operators->setSeverity(800);
            m_pLimitAlarm_Operators->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else if ( Value >= m_pLimitAlarm_Operators->getHighLimit() )
    {
        if ( m_pLimitAlarm_Operators->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::High )
        {
            m_pLimitAlarm_Operators->setMessage(UaLocalizedText("en", "LimitAlarm_Operators active in High"));
            m_pLimitAlarm_Operators->setActiveState(OpcUa_True);
            m_pLimitAlarm_Operators->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::High);
            m_pLimitAlarm_Operators->setAckedState(OpcUa_False);
            m_pLimitAlarm_Operators->setRetain(OpcUa_True);
            m_pLimitAlarm_Operators->setLastSeverity(m_pLimitAlarm_Operators->getSeverity());
            m_pLimitAlarm_Operators->setSeverity(400);
            m_pLimitAlarm_Operators->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
    else
    {
        if ( m_pLimitAlarm_Operators->getActiveStateBool() != OpcUa_False )
        {
            m_pLimitAlarm_Operators->setMessage(UaLocalizedText("en", "LimitAlarm_Operators inactive"));
            m_pLimitAlarm_Operators->setActiveState(OpcUa_False);
            m_pLimitAlarm_Operators->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Disabled);
            m_pLimitAlarm_Operators->setAckedState(OpcUa_True);
            m_pLimitAlarm_Operators->setRetain(OpcUa_False);
            m_pLimitAlarm_Operators->setLastSeverity(m_pLimitAlarm_Operators->getSeverity());
            m_pLimitAlarm_Operators->setSeverity(400);
            m_pLimitAlarm_Operators->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }
#endif // SUPPORT_Event_Subscription_Server_Facet
}

void AccessPermissionObjectType::initialize()
{
    UaVariant value;
    value.setDouble(50);
    m_pValue->setValue(NULL, UaDataValue(value, OpcUa_Good, UaDateTime::now(), UaDateTime::now()), OpcUa_False);

#if SUPPORT_Event_Subscription_Server_Facet
    m_pLimitAlarm_All->setConditionName("ExclusiveLimitAlarm_All");
    m_pLimitAlarm_All->setAckedState(OpcUa_True);
    m_pLimitAlarm_All->setActiveState(OpcUa_False);
    m_pLimitAlarm_All->setEnabledState(OpcUa_True);
    m_pLimitAlarm_All->setHighHighLimit(80);
    m_pLimitAlarm_All->setHighLimit(60);
    m_pLimitAlarm_All->setLowLimit(40);
    m_pLimitAlarm_All->setLowLowLimit(20);

    m_pLimitAlarm_John->setConditionName("ExclusiveLimitAlarm_John");
    m_pLimitAlarm_John->setAckedState(OpcUa_True);
    m_pLimitAlarm_John->setActiveState(OpcUa_False);
    m_pLimitAlarm_John->setEnabledState(OpcUa_True);
    m_pLimitAlarm_John->setHighHighLimit(80);
    m_pLimitAlarm_John->setHighLimit(60);
    m_pLimitAlarm_John->setLowLimit(40);
    m_pLimitAlarm_John->setLowLowLimit(20);

    m_pLimitAlarm_Operators->setConditionName("ExclusiveLimitAlarm_Operators");
    m_pLimitAlarm_Operators->setAckedState(OpcUa_True);
    m_pLimitAlarm_Operators->setActiveState(OpcUa_False);
    m_pLimitAlarm_Operators->setEnabledState(OpcUa_True);
    m_pLimitAlarm_Operators->setHighHighLimit(80);
    m_pLimitAlarm_Operators->setHighLimit(60);
    m_pLimitAlarm_Operators->setLowLimit(40);
    m_pLimitAlarm_Operators->setLowLowLimit(20);
#endif // SUPPORT_Event_Subscription_Server_Facet
}

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission


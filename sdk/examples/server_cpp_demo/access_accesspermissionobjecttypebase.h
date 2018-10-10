/******************************************************************************
** access_accesspermissionobjecttypebase.h
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

#ifndef __ACCESS_ACCESSPERMISSIONOBJECTTYPEBASE_H__
#define __ACCESS_ACCESSPERMISSIONOBJECTTYPEBASE_H__

#include "opcua_baseobjecttype.h"
#include "access_datatypes.h"
#include "access_identifiers.h"
#include "access_instancefactory_access.h"
#include "basenodes.h"
#include "opcua_basedatavariabletype.h"
#if SUPPORT_Event_Subscription_Server_Facet
#include "opcua_conditionvariabletype.h"
#include "opcua_exclusivelimitalarmtype.h"
#endif // SUPPORT_Event_Subscription_Server_Facet
#if SUPPORT_Method_Server_Facet
#include "opcua_exclusivelimitstatemachinetype.h"
#include "opcua_twostatevariabletype.h"
#endif // SUPPORT_Method_Server_Facet
#include "opcua_propertytype.h"
#include "uaargument.h"
#include "uatimezonedatatype.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
namespace Access {


/** Generated base class for a AccessPermissionObjectType.
 *
 *  This class contains the auto generated code for the object type AccessPermissionObjectType
 *  representing an OPC UA ObjectType. This class is used to create the object type and to
 *  create and represent the object in the server address space.
 *
 *  **Object members of the AccessPermissionObjectType:**
 *
 *  Browse Name          | TypeDefinition          | Modelling Rule | See Also
 *  ---------------------|-------------------------|----------------|-----------------------------
 *  LimitAlarm_All       | ExclusiveLimitAlarmType | Mandatory      | \ref getLimitAlarm_All
 *  LimitAlarm_John      | ExclusiveLimitAlarmType | Mandatory      | \ref getLimitAlarm_John
 *  LimitAlarm_Operators | ExclusiveLimitAlarmType | Mandatory      | \ref getLimitAlarm_Operators
 *
 *  **Variable members of the AccessPermissionObjectType:**
 *
 *  Browse Name | DataType | TypeDefinition       | Modelling Rule | See Also
 *  ------------|----------|----------------------|----------------|-----------------------------
 *  Value       | Double   | BaseDataVariableType | Mandatory      | \ref getValue, \ref setValue
 *  Value       | Double   | BaseDataVariableType | Mandatory      | \ref getValue, \ref setValue
 *
 */
class CPP_ACCESS_EXPORT AccessPermissionObjectTypeBase:
    public OpcUa::BaseObjectType
{
    UA_DISABLE_COPY(AccessPermissionObjectTypeBase);
protected:
    virtual ~AccessPermissionObjectTypeBase();
public:
    // construction / destruction
    AccessPermissionObjectTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    AccessPermissionObjectTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    AccessPermissionObjectTypeBase(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    virtual UaNodeId       typeDefinitionId() const;


    //- Interface MethodManager -----------------------------------------------------------
    /** Call method of an UA object. */
    virtual UaStatus beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments);
    virtual UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);
    //- Interface MethodManager -----------------------------------------------------------

    virtual UaStatus SetValue_All(
        const ServiceContext& serviceContext /**<General context for the service calls containing
                                                 information like the session object,
                                                 return diagnostic mask and timeout hint.*/,
        OpcUa_Double Value) = 0;
    virtual UaStatus SetValue_John(
        const ServiceContext& serviceContext /**<General context for the service calls containing
                                                 information like the session object,
                                                 return diagnostic mask and timeout hint.*/,
        OpcUa_Double Value) = 0;
    virtual UaStatus SetValue_Operators(
        const ServiceContext& serviceContext /**<General context for the service calls containing
                                                 information like the session object,
                                                 return diagnostic mask and timeout hint.*/,
        OpcUa_Double Value) = 0;

    virtual void setValue(OpcUa_Double Value);
    virtual OpcUa_Double getValue() const;


#if SUPPORT_Event_Subscription_Server_Facet
    virtual OpcUa::ExclusiveLimitAlarmType* getLimitAlarm_All();
    virtual OpcUa::ExclusiveLimitAlarmType* getLimitAlarm_John();
    virtual OpcUa::ExclusiveLimitAlarmType* getLimitAlarm_Operators();
#endif // SUPPORT_Event_Subscription_Server_Facet
    virtual OpcUa::BaseDataVariableType* getValueNode();
    virtual OpcUa::BaseMethod* getSetValue_All();
    virtual OpcUa::BaseMethod* getSetValue_John();
    virtual OpcUa::BaseMethod* getSetValue_Operators();

    // NodeAccessInfo management
    virtual void useAccessInfoFromType();
    void useAccessInfoFromInstance(AccessPermissionObjectTypeBase *pInstance);

protected:
    // Object nodes
    // Object LimitAlarm_All
#if SUPPORT_Event_Subscription_Server_Facet
    static OpcUa::ExclusiveLimitAlarmType*  s_pLimitAlarm_All;
    OpcUa::ExclusiveLimitAlarmType*  m_pLimitAlarm_All;
    // Object LimitAlarm_John
    static OpcUa::ExclusiveLimitAlarmType*  s_pLimitAlarm_John;
    OpcUa::ExclusiveLimitAlarmType*  m_pLimitAlarm_John;
    // Object LimitAlarm_Operators
    static OpcUa::ExclusiveLimitAlarmType*  s_pLimitAlarm_Operators;
    OpcUa::ExclusiveLimitAlarmType*  m_pLimitAlarm_Operators;
#endif // SUPPORT_Event_Subscription_Server_Facet

    // Variable nodes
    // Variable Value
    static OpcUa::BaseDataVariableType*  s_pValue;
    OpcUa::BaseDataVariableType*  m_pValue;


    // Method nodes
    static OpcUa::BaseMethod* s_pSetValue_AllMethod;
    OpcUa::BaseMethod*  m_pSetValue_AllMethod;
    static OpcUa::BaseMethod* s_pSetValue_JohnMethod;
    OpcUa::BaseMethod*  m_pSetValue_JohnMethod;
    static OpcUa::BaseMethod* s_pSetValue_OperatorsMethod;
    OpcUa::BaseMethod*  m_pSetValue_OperatorsMethod;


private:
    void initialize();

private:
    static bool s_typeNodesCreated;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission

#endif // #ifndef __ACCESSACCESSPERMISSIONOBJECTTYPEBASE_H__



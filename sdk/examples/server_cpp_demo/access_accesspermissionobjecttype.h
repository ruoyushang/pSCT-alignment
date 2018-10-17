/******************************************************************************
** access_accesspermissionobjecttype.h
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

#ifndef __ACCESS_ACCESSPERMISSIONOBJECTTYPE_H__
#define __ACCESS_ACCESSPERMISSIONOBJECTTYPE_H__

#include "access_accesspermissionobjecttypebase.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
namespace Access {


/** @brief Class implementing the UaObject interface for the AccessPermissionObjectType.
 *
 * OPC UA Objects are used to represent systems, system components, real-world objects and software
 * objects. They have the NodeClass @ref L3UaNodeClassObject. The detailed description of Objects and their attributes
 * can be found in the general description of the @ref L3UaNodeClassObject node class.
 *
 *  **Object members of the AccessPermissionObjectType:**
 *
 *  Browse Name          | Modelling Rule | See Also
 *  ---------------------|----------------|---------------------------------------------------------------------------------------
 *  LimitAlarm_All       | Mandatory      | \ref AccessPermissionObjectTypeBase::getLimitAlarm_All "getLimitAlarm_All"
 *  LimitAlarm_John      | Mandatory      | \ref AccessPermissionObjectTypeBase::getLimitAlarm_John "getLimitAlarm_John"
 *  LimitAlarm_Operators | Mandatory      | \ref AccessPermissionObjectTypeBase::getLimitAlarm_Operators "getLimitAlarm_Operators"
 *
 *  **Variable members of the AccessPermissionObjectType:**
 *
 *  Browse Name | Modelling Rule | See Also
 *  ------------|----------------|-------------------------------------------------------------------------------------------------------------------
 *  Value       | Mandatory      | \ref AccessPermissionObjectTypeBase::getValue "getValue", \ref AccessPermissionObjectTypeBase::setValue "setValue"
 *  Value       | Mandatory      | \ref AccessPermissionObjectTypeBase::getValue "getValue", \ref AccessPermissionObjectTypeBase::setValue "setValue"
 *
 *  **Method members of the AccessPermissionObjectType:**
 *
 *  Browse Name             | Modelling Rule
 *  ------------------------|---------------
 *  \ref SetValue_All       | Mandatory
 *  \ref SetValue_John      | Mandatory
 *  \ref SetValue_Operators | Mandatory
 *
 */
class CPP_ACCESS_EXPORT AccessPermissionObjectType:
    public AccessPermissionObjectTypeBase
{
    UA_DISABLE_COPY(AccessPermissionObjectType);
protected:
    // destruction
    virtual ~AccessPermissionObjectType();
public:
    // construction
    AccessPermissionObjectType(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    AccessPermissionObjectType(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    AccessPermissionObjectType(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    // UaAccessPermissionObjectType method SetValue_All
    virtual UaStatus SetValue_All(
        const ServiceContext& serviceContext,
        OpcUa_Double Value);
    // UaAccessPermissionObjectType method SetValue_John
    virtual UaStatus SetValue_John(
        const ServiceContext& serviceContext,
        OpcUa_Double Value);
    // UaAccessPermissionObjectType method SetValue_Operators
    virtual UaStatus SetValue_Operators(
        const ServiceContext& serviceContext,
        OpcUa_Double Value);

protected:
    virtual void setValue(OpcUa_Double Value);

private:
    virtual void initialize();
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission

#endif // #ifndef __ACCESSACCESSPERMISSIONOBJECTTYPE_H__


/******************************************************************************
** access_instancefactory_access.cpp
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


#include "access_instancefactory_access.h"
#include "access_identifiers.h"
#include "access_accesspermissionobjecttype.h"
#include "access_datatypes.h"


namespace Access
{

InstanceFactoryAccess::InstanceFactoryAccess(OpcUa_UInt16 namespaceIndex)
: XmlUaNodeFactoryNamespace(namespaceIndex)
{
}

OpcUa::BaseVariableType* InstanceFactoryAccess::createVariable(
    UaBase::Variable *pVariable,
    XmlUaNodeFactoryManager *pFactory,
    NodeManagerConfig* pNodeConfig,
    UaMutexRefCounted* pSharedMutex)
{
    UA_ASSERT(pVariable->typeDefinitionId().namespaceIndex() == namespaceIndex());
    OpcUa_ReferenceParameter(pVariable);
    OpcUa_ReferenceParameter(pFactory);
    OpcUa_ReferenceParameter(pNodeConfig);
    OpcUa_ReferenceParameter(pSharedMutex);
    return NULL;
}

OpcUa::BaseObjectType* InstanceFactoryAccess::createObject(
    UaBase::Object *pObject,
    XmlUaNodeFactoryManager *pFactory,
    NodeManagerConfig* pNodeConfig,
    UaMutexRefCounted* pSharedMutex)
{
    UA_ASSERT(pObject->typeDefinitionId().namespaceIndex() == namespaceIndex());
    if (pObject->typeDefinitionId().namespaceIndex() == namespaceIndex())
    {
        switch (pObject->typeDefinitionId().identifierNumeric())
        {
        case AccessId_AccessPermissionObjectType:
            return new Access::AccessPermissionObjectType(pObject, pFactory, pNodeConfig, pSharedMutex);
        default:
            UA_ASSERT(false);
            return createGenericObject(pObject, pFactory, pNodeConfig, pSharedMutex);
        }
    }
    return NULL;
}

/**Returns default values for all data types provided by the Namespace http://www.unifiedautomation.com/DemoServer/AccessPermission */
UaVariant InstanceFactoryAccess::defaultValue(const UaNodeId &dataTypeId, OpcUa_Int32 valueRank) const
{
    UA_ASSERT(dataTypeId.namespaceIndex() == namespaceIndex());
    UaVariant value;
    OpcUa_ReferenceParameter(dataTypeId);
    OpcUa_ReferenceParameter(valueRank);
    return value;
}

};

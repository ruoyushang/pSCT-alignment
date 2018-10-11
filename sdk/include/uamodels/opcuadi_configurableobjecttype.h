/******************************************************************************
** opcuadi_configurableobjecttype.h
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
** Project: C++ OPC Server SDK information model for namespace http://opcfoundation.org/UA/DI/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#ifndef __OPCUADI_CONFIGURABLEOBJECTTYPE_H__
#define __OPCUADI_CONFIGURABLEOBJECTTYPE_H__

#include "opcuadi_configurableobjecttypebase.h"

// Namespace for the UA information model http://opcfoundation.org/UA/DI/
namespace OpcUaDi {


/** @brief Class implementing the UaObject interface for the ConfigurableObjectType.
 *
 * OPC UA Objects are used to represent systems, system components, real-world objects and software
 * objects. They have the NodeClass @ref L3UaNodeClassObject. The detailed description of Objects and their attributes
 * can be found in the general description of the @ref L3UaNodeClassObject node class.
 *
 *  **Object members of the ConfigurableObjectType:**
 *
 *  Browse Name          | TypeDefinition | Modelling Rule      | See Also
 *  ---------------------|----------------|---------------------|-----------------------------------------------------------------------
 *  \<ObjectIdentifier\> | BaseObjectType | OptionalPlaceholder | &nbsp;
 *  SupportedTypes       | FolderType     | Mandatory           | \ref ConfigurableObjectTypeBase::getSupportedTypes "getSupportedTypes"
 *
 */
class UAMODELS_EXPORT ConfigurableObjectType:
    public ConfigurableObjectTypeBase
{
    UA_DISABLE_COPY(ConfigurableObjectType);
protected:
    // destruction
    virtual ~ConfigurableObjectType();
public:
    // construction
    ConfigurableObjectType(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    ConfigurableObjectType(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    ConfigurableObjectType(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    UaStatus addSupportedType(UaReferenceLists* pOjectType);

protected:

private:
};

} // End namespace for the UA information model http://opcfoundation.org/UA/DI/

#endif // #ifndef __OPCUADICONFIGURABLEOBJECTTYPE_H__


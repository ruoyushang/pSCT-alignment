/******************************************************************************
** opcuaplc_ctrlconfigurationtype.h
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
** Project: C++ OPC Server SDK information model for namespace http://PLCopen.org/OpcUa/IEC61131-3/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#ifndef __OPCUAPLC_CTRLCONFIGURATIONTYPE_H__
#define __OPCUAPLC_CTRLCONFIGURATIONTYPE_H__

#include "opcuaplc_ctrlconfigurationtypebase.h"

// Namespace for the UA information model http://PLCopen.org/OpcUa/IEC61131-3/
namespace OpcUaPlc {


/** @brief Class implementing the UaObject interface for the CtrlConfigurationType.
 *
 * OPC UA Objects are used to represent systems, system components, real-world objects and software
 * objects. They have the NodeClass @ref L3UaNodeClassObject. The detailed description of Objects and their attributes
 * can be found in the general description of the @ref L3UaNodeClassObject node class.
 *
 *  **Object members of the CtrlConfigurationType:**
 *
 *  Browse Name   | TypeDefinition         | Modelling Rule | See Also
 *  --------------|------------------------|----------------|--------------------------------------------------------------------
 *  AccessVars    | FunctionalGroupType    | Optional       | \ref CtrlConfigurationTypeBase::getAccessVars "getAccessVars"
 *  Configuration | FunctionalGroupType    | Optional       | \ref CtrlConfigurationTypeBase::getConfiguration "getConfiguration"
 *  ConfigVars    | FunctionalGroupType    | Optional       | \ref CtrlConfigurationTypeBase::getConfigVars "getConfigVars"
 *  GlobalVars    | FunctionalGroupType    | Optional       | \ref CtrlConfigurationTypeBase::getGlobalVars "getGlobalVars"
 *  MethodSet     | BaseObjectType         | Optional       | \ref CtrlConfigurationTypeBase::getMethodSet "getMethodSet"
 *  Resources     | ConfigurableObjectType | Mandatory      | \ref CtrlConfigurationTypeBase::getResources "getResources"
 *
 */
class UAMODELS_EXPORT CtrlConfigurationType:
    public CtrlConfigurationTypeBase
{
    UA_DISABLE_COPY(CtrlConfigurationType);
protected:
    // destruction
    virtual ~CtrlConfigurationType();
public:
    // construction
    CtrlConfigurationType(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    CtrlConfigurationType(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    CtrlConfigurationType(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    virtual UaNodeId typeDefinitionId() const;
    void setTypeDefinition (const UaNodeId& typeDefinition);

    UaStatus addResourceInstance(UaReferenceLists* pResourceOject, NodeManagerConfig* pNodeConfig = NULL);
    UaStatus addResourceType(UaReferenceLists* pResourceOjectType);

    UaStatus addGlobalVar(UaReferenceLists* pGlobalVarNode, NodeManagerConfig* pNodeConfig = NULL);

    // UaCtrlConfigurationType method Start
    virtual UaStatus MethodSet_Start(
        const ServiceContext&  serviceContext );
    // UaCtrlConfigurationType method Stop
    virtual UaStatus MethodSet_Stop(
        const ServiceContext&  serviceContext );

protected:

private:
    UaNodeId m_typeDefinition;
};

} // End namespace for the UA information model http://PLCopen.org/OpcUa/IEC61131-3/

#endif // #ifndef __OPCUAPLCCTRLCONFIGURATIONTYPE_H__


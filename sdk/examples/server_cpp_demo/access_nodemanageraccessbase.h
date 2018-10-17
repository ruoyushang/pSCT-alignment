/******************************************************************************
** access_nodemanageraccessbase.h
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

#ifndef _ACCESS_NODEMANAGERACCESSBASE_H__
#define _ACCESS_NODEMANAGERACCESSBASE_H__

#include "uamutex.h"
#include "uabasenodes.h"
#include "nodemanagerbase.h"
#include "uaobjecttypes.h"
#include "access_identifiers.h"
#include "methodmanager.h"

/** Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
*/
namespace Access {

/** Generated base class for NodeManager for Access information model.
 *
 *  This abstract class contains the auto generated code for the NodeManagerAccess.
 *  This class is used to create the nodes defined for the namespace http://www.unifiedautomation.com/DemoServer/AccessPermission
 *  and to select the methods for UA Method calls if the UA Methods are not defined at an ObjectType.
 *  Methods that are defined at an ObjectType are selected at the <ObjectTypeName>Base class.
 *
 */
class CPP_ACCESS_EXPORT NodeManagerAccessBase : public NodeManagerBase
#if SUPPORT_Method_Server_Facet
    , public MethodManager
#endif
{
    UA_DISABLE_COPY(NodeManagerAccessBase);
    NodeManagerAccessBase();
public:
    /* construction / destruction */
    NodeManagerAccessBase(OpcUa_Boolean firesEvents, OpcUa_Int32 nHashTableSize = 10007);
    virtual ~NodeManagerAccessBase(){};

    //- Interface NodeManagerUaNode -------------------------------------------------------
    virtual UaStatus   afterStartUp();
    //- Interface NodeManagerUaNode -------------------------------------------------------

    static OpcUa_UInt16 getTypeNamespace();

#if SUPPORT_Method_Server_Facet

    //- Method management -----------------------------------------------------------------
    virtual MethodHandle* getMethodHandle(
        Session* pSession,
        OpcUa_NodeId* pObjectNodeId,
        OpcUa_NodeId* pMethodNodeId,
        UaStatus& result) const;
    virtual UaStatus beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments);
    virtual UaStatus call(
        const ServiceContext&  /*serviceContext*/,
        MethodHandle*          /*pMethodHandle*/,
        const UaVariantArray&  /*inputArguments*/,
        UaVariantArray&        /*outputArguments*/,
        UaStatusCodeArray&     /*inputArgumentResults*/,
        UaDiagnosticInfos&     /*inputArgumentDiag*/);
    virtual UaStatus Demo_AccessRights_Options_AddAdvancedNodes(const ServiceContext& serviceContext) = 0;
    //- Method management -----------------------------------------------------------------
#endif // SUPPORT_Method_Server_Facet
protected:
    UaStatus createAdvancedNodes();
    UaStatus createAdditionalReferences();
private:
    UaStatus createDataTypeNodes();
    UaStatus createReferenceTypeNodes();
    UaStatus addObjectsFolderDemoAccessRightsAccess_AllAll_RequireEncryption(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_AllAll_RequireSigning(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_AllObject_All(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsBrowse_AllJohn_ReadAttributes(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_JohnObject_John(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_OperatorsObject_Operators(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsBrowse_AllOperators_ReadAttributes(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsOptions(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsOptionsAddAdvancedNodes(const XmlUaNodeFactoryManager &factory);


protected:
    UaMutex             m_mutex;
    static OpcUa_UInt16 s_namespaceIndex;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission

#endif // #ifndef __NODEMANAGERACCESSBASE_H__


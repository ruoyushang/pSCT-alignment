/******************************************************************************
** access_nodemanageraccessbase.cpp
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

#include "access_nodemanageraccessbase.h"
#include "nodemanagerroot.h"

// Include type headers
#include <uabasenodes.h>
#include "access_accesspermissionobjecttype.h"
#include "access_nodemanageraccess.h"
#include "demo_nodemanagerdemo.h"
#include "opcua_basedatavariabletype.h"
#include "opcua_foldertype.h"
#include "nodemanagerroot.h"
#include "opcua_foldertype.h"
#if SUPPORT_Method_Server_Facet
#  include <uathreadpool.h>
#endif

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
namespace Access {
#if SUPPORT_Method_Server_Facet
class CPP_ACCESS_EXPORT MethodCallJob : public UaThreadPoolJob
{
    UA_DISABLE_COPY(MethodCallJob);
public:
    MethodCallJob();
    virtual ~MethodCallJob();
    void execute();
    void initialize(
        NodeManagerAccessBase* pNodeManager,
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments);

private:
    MethodManagerCallback* m_pCallback;
    ServiceContext         m_serviceContext;
    OpcUa_UInt32           m_callbackHandle;
    MethodHandle*          m_pMethodHandle;
    UaVariantArray         m_inputArguments;
    NodeManagerAccessBase* m_pNodeManager;
};

/** Constructs a helper object for executing methods in a worker thread pool job.
*/
MethodCallJob::MethodCallJob()
: m_pCallback(NULL),
  m_callbackHandle(0),
  m_pMethodHandle(NULL),
  m_pNodeManager(NULL)
{
}

/** Destroys the helper object
*/
MethodCallJob::~MethodCallJob()
{
    if ( m_pMethodHandle )
    {
        m_pMethodHandle->releaseReference();
        m_pMethodHandle = NULL;
    }
    m_inputArguments.detach();
}

/** Execute call from worker thread.
*/
void MethodCallJob::execute()
{
    UaStatus            ret;
    UaVariantArray      outputArguments;
    UaStatusCodeArray   inputArgumentResults;
    UaDiagnosticInfos   inputArgumentDiag;

    ret = m_pNodeManager->call(
        m_serviceContext,
        m_pMethodHandle,
        m_inputArguments,
        outputArguments,
        inputArgumentResults,
        inputArgumentDiag);

    m_pCallback->finishCall(
        m_callbackHandle,
        inputArgumentResults,
        inputArgumentDiag,
        outputArguments,
        ret);
}

/** Initialize thread pool job object
*/
void MethodCallJob::initialize(
    NodeManagerAccessBase* pNodeManager,
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    m_pCallback       = pCallback;
    m_serviceContext  = serviceContext;
    m_callbackHandle  = callbackHandle;
    m_pMethodHandle   = pMethodHandle;
    if ( m_pMethodHandle )
    {
        m_pMethodHandle->addReference();
    }
    // This is not a deep copy of the data since we will detach the data in the destructor
    // The array data will be managed and deleted with the request structure
    // It is also passed on as const to BaseObjectType::call()
    m_inputArguments.attach(inputArguments.length(), (OpcUa_Variant*) inputArguments.rawData());
    m_pNodeManager = pNodeManager;
}

#endif
OpcUa_UInt16 NodeManagerAccessBase::s_namespaceIndex = 0;

/** Construction of the class NodeManagerAccessBase.
*/
NodeManagerAccessBase::NodeManagerAccessBase(
    OpcUa_Boolean firesEvents,  //!< [in] Flag indicating if the NodeManager should activate the EventManager
    OpcUa_Int32 nHashTableSize) //!< [in] Size of the hash table. The default value is 10.007.<br>
                                //   The size can be smaller than the total number of nodes managed
                                //   by the node manager but it is better for the performance to avoid
                                //   collisions with a large size.
                                //   Prefered sizes are 1.009, 10.007, 100.003, 1.000.003, 10.000.019.
: NodeManagerBase("http://www.unifiedautomation.com/DemoServer/AccessPermission", firesEvents, nHashTableSize)
{
    m_defaultLocaleId         = "en";
}

/** Finish start up in derived class after starting up base class.
*/
UaStatus NodeManagerAccessBase::afterStartUp()
{
    UaStatus ret;

    s_namespaceIndex = getNameSpaceIndex();

    ret = createDataTypeNodes();
    UA_ASSERT(ret.isGood());
    ret = createReferenceTypeNodes();
    UA_ASSERT(ret.isGood());

    // ------------------------------------------------------------
    // Create object type nodes
    // ------------------------------------------------------------
    AccessPermissionObjectType::createTypes();
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // Create variable type nodes
    // ------------------------------------------------------------
    // ------------------------------------------------------------

    XmlUaNodeFactoryManager factory;
    Access::InstanceFactoryAccess factoryAccess(getTypeNamespace());
    factory.addNamespace(&factoryAccess);

    ret = addObjectsFolderDemoAccessRightsOptions(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsOptionsAddAdvancedNodes(factory);
    UA_ASSERT(ret.isGood());

    return ret;
}

/** Create additional nodes that are not visible after startup.
*/
UaStatus NodeManagerAccessBase::createAdvancedNodes()
{
    XmlUaNodeFactoryManager factory;
    Access::InstanceFactoryAccess factoryAccess(getTypeNamespace());
    factory.addNamespace(&factoryAccess);

    UaStatus ret;
    ret = addObjectsFolderDemoAccessRightsAccess_AllAll_RequireEncryption(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_AllAll_RequireSigning(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_AllObject_All(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsBrowse_AllJohn_ReadAttributes(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_JohnObject_John(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_OperatorsObject_Operators(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsBrowse_AllOperators_ReadAttributes(factory);
    UA_ASSERT(ret.isGood());

    return ret;
}

/** Static method to get the namespace index for the type namespace managed by this class.
 */
OpcUa_UInt16 NodeManagerAccessBase::getTypeNamespace()
{
    return s_namespaceIndex;
}

/** Helper function to create data type related nodes.
*/
UaStatus NodeManagerAccessBase::createDataTypeNodes()
{
    UaStatus ret;
    return ret;
}

/** Helper function to create reference type related nodes.
*/
UaStatus NodeManagerAccessBase::createReferenceTypeNodes()
{
    UaStatus ret;
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsAccess_AllAll_RequireEncryption(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_RequireEncryption = new UaBase::Variable(UaNodeId(AccessId_Demo_AccessRights_Access_All_All_RequireEncryption, getTypeNamespace()));
    pAll_RequireEncryption->setBrowseName(UaQualifiedName("All_RequireEncryption", getTypeNamespace()));
    pAll_RequireEncryption->setDisplayName(UaLocalizedText("", "All_RequireEncryption"));
    pAll_RequireEncryption->setDescription(UaLocalizedText("", "Access to this node is only allowed when the message security mode includes encryption."));
    pAll_RequireEncryption->setAccessLevel(3);
    pAll_RequireEncryption->setUserAccessLevel(3);
    pAll_RequireEncryption->setValueRank(-1);
    pAll_RequireEncryption->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pAll_RequireEncryption->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_RequireEncryption, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_All, Demo::NodeManagerDemo::getTypeNamespace()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_RequireEncryption;
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsAccess_AllAll_RequireSigning(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_RequireSigning = new UaBase::Variable(UaNodeId(AccessId_Demo_AccessRights_Access_All_All_RequireSigning, getTypeNamespace()));
    pAll_RequireSigning->setBrowseName(UaQualifiedName("All_RequireSigning", getTypeNamespace()));
    pAll_RequireSigning->setDisplayName(UaLocalizedText("", "All_RequireSigning"));
    pAll_RequireSigning->setDescription(UaLocalizedText("", "Access to this node is only allowed when the message security mode includes signing."));
    pAll_RequireSigning->setAccessLevel(3);
    pAll_RequireSigning->setUserAccessLevel(3);
    pAll_RequireSigning->setValueRank(-1);
    pAll_RequireSigning->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pAll_RequireSigning->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_RequireSigning, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_All, Demo::NodeManagerDemo::getTypeNamespace()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_RequireSigning;
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsAccess_AllObject_All(const XmlUaNodeFactoryManager &factory)
{
    OpcUa_ReferenceParameter(factory);
    UaStatus ret;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    Access::AccessPermissionObjectType* pObject_All = new Access::AccessPermissionObjectType(
        UaNodeId(AccessId_Server_Object_All, getNameSpaceIndex()),
        "Object_All",
        getNameSpaceIndex(),
        this);
    pObject_All->setDisplayName(UaLocalizedText("", "Object_All"));
    pObject_All->setDescription(UaLocalizedText("", "Methods on this object instance are executable for everybody unless limited by method configuration."));
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_All, Demo::NodeManagerDemo::getTypeNamespace()), pObject_All, referenceTypeId);
    UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    pObject_All->setEventNotifier(1);
    parentEventNodeId = OpcUaId_Server;
    ret = addUaReference(parentEventNodeId, pObject_All->nodeId(), OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());
    registerEventNotifier(parentEventNodeId, pObject_All->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsBrowse_AllJohn_ReadAttributes(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pJohn_ReadAttributes = new UaBase::Variable(UaNodeId(AccessId_Demo_AccessRights_Browse_All_John_ReadAttributes, getTypeNamespace()));
    pJohn_ReadAttributes->setBrowseName(UaQualifiedName("John_ReadAttributes", getTypeNamespace()));
    pJohn_ReadAttributes->setDisplayName(UaLocalizedText("", "John_ReadAttributes"));
    pJohn_ReadAttributes->setDescription(UaLocalizedText("", "Full access to user John. Other users cannot read any attribute."));
    pJohn_ReadAttributes->setAccessLevel(3);
    pJohn_ReadAttributes->setUserAccessLevel(3);
    pJohn_ReadAttributes->setValueRank(-1);
    pJohn_ReadAttributes->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pJohn_ReadAttributes->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pJohn_ReadAttributes, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Browse_All, Demo::NodeManagerDemo::getTypeNamespace()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pJohn_ReadAttributes;
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsAccess_JohnObject_John(const XmlUaNodeFactoryManager &factory)
{
    OpcUa_ReferenceParameter(factory);
    UaStatus ret;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    Access::AccessPermissionObjectType* pObject_John = new Access::AccessPermissionObjectType(
        UaNodeId(AccessId_Server_Object_John, getNameSpaceIndex()),
        "Object_John",
        getNameSpaceIndex(),
        this);
    pObject_John->setDisplayName(UaLocalizedText("", "Object_John"));
    pObject_John->setDescription(UaLocalizedText("", "Methods on this object instance are executable for John unless limited by method configuration."));
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_John, Demo::NodeManagerDemo::getTypeNamespace()), pObject_John, referenceTypeId);
    UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    pObject_John->setEventNotifier(1);
    parentEventNodeId = OpcUaId_Server;
    ret = addUaReference(parentEventNodeId, pObject_John->nodeId(), OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());
    registerEventNotifier(parentEventNodeId, pObject_John->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsAccess_OperatorsObject_Operators(const XmlUaNodeFactoryManager &factory)
{
    OpcUa_ReferenceParameter(factory);
    UaStatus ret;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    Access::AccessPermissionObjectType* pObject_Operators = new Access::AccessPermissionObjectType(
        UaNodeId(AccessId_Server_Object_Operators, getNameSpaceIndex()),
        "Object_Operators",
        getNameSpaceIndex(),
        this);
    pObject_Operators->setDisplayName(UaLocalizedText("", "Object_Operators"));
    pObject_Operators->setDescription(UaLocalizedText("", "Methods on this object instance are executable for members of the operators group unless limited by method configuration."));
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_Operators, Demo::NodeManagerDemo::getTypeNamespace()), pObject_Operators, referenceTypeId);
    UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    pObject_Operators->setEventNotifier(1);
    parentEventNodeId = OpcUaId_Server;
    ret = addUaReference(parentEventNodeId, pObject_Operators->nodeId(), OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());
    registerEventNotifier(parentEventNodeId, pObject_Operators->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsBrowse_AllOperators_ReadAttributes(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pOperators_ReadAttributes = new UaBase::Variable(UaNodeId(AccessId_Demo_AccessRights_Browse_All_Operators_ReadAttributes, getTypeNamespace()));
    pOperators_ReadAttributes->setBrowseName(UaQualifiedName("Operators_ReadAttributes", getTypeNamespace()));
    pOperators_ReadAttributes->setDisplayName(UaLocalizedText("", "Operators_ReadAttributes"));
    pOperators_ReadAttributes->setDescription(UaLocalizedText("", "All attributes are only readable for members of the operator group."));
    pOperators_ReadAttributes->setAccessLevel(3);
    pOperators_ReadAttributes->setUserAccessLevel(3);
    pOperators_ReadAttributes->setValueRank(-1);
    pOperators_ReadAttributes->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pOperators_ReadAttributes->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pOperators_ReadAttributes, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Browse_All, Demo::NodeManagerDemo::getTypeNamespace()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pOperators_ReadAttributes;
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsOptions(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pOptions = new UaBase::Object(UaNodeId(AccessId_Demo_AccessRights_Options, getTypeNamespace()));
    pOptions->setBrowseName(UaQualifiedName("Options", getTypeNamespace()));
    pOptions->setDisplayName(UaLocalizedText("", "Options"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pOptions, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights, Demo::NodeManagerDemo::getTypeNamespace()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pOptions;
    return ret;
}

UaStatus NodeManagerAccessBase::addObjectsFolderDemoAccessRightsOptionsAddAdvancedNodes(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(AccessId_Demo_AccessRights_Options_AddAdvancedNodes, getTypeNamespace()), "AddAdvancedNodes", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(AccessId_Demo_AccessRights_Options, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}


#if SUPPORT_Method_Server_Facet
MethodHandle* NodeManagerAccessBase::getMethodHandle(
    Session*       pSession,
    OpcUa_NodeId*  pObjectNodeId,
    OpcUa_NodeId*  pMethodNodeId,
    UaStatus&      result) const
{
    if (pObjectNodeId && pMethodNodeId)
    {
        UaNodeId objectId(*pObjectNodeId);
        UaNodeId methodId(*pMethodNodeId);
        if ( (methodId == UaNodeId(AccessId_Demo_AccessRights_Options_AddAdvancedNodes, getNameSpaceIndex()))
        )
        {
            MethodHandleNodeId *pRet = new MethodHandleNodeId();
            pRet->m_methodNodeId = methodId;
            pRet->m_objectNodeId = objectId;
            pRet->m_pMethodManager = (MethodManager*)this;
            return pRet;
        }
    }

    MethodHandle* pMethodHandle = NodeManagerUaNode::getMethodHandle(pSession, pObjectNodeId, pMethodNodeId, result);
    return pMethodHandle;
}

/** Implementation of the MethodManager interface method beginCall
*/
UaStatus NodeManagerAccessBase::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    UaStatus            ret;

    MethodCallJob* pCallJob = new MethodCallJob;
    pCallJob->initialize(this, pCallback, serviceContext, callbackHandle, pMethodHandle, inputArguments);
    ret = NodeManagerRoot::CreateRootNodeManager()->pServerManager()->getThreadPool()->addJob(pCallJob);
    if ( ret.isBad() )
    {
        delete pCallJob;
    }

    return ret;
}

UaStatus NodeManagerAccessBase::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    OpcUa_ReferenceParameter(inputArgumentResults);
    OpcUa_ReferenceParameter(outputArguments);

    UaStatus            ret;
    OpcUa_ReferenceParameter(inputArgumentDiag);
    if (pMethodHandle->getHandleImplementation() == MethodHandle::NODE_ID)
    {
        MethodHandleNodeId* pMethodHandleNodeId = (MethodHandleNodeId*)pMethodHandle;
        if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(AccessId_Demo_AccessRights_Options_AddAdvancedNodes, getNameSpaceIndex()))
        {
                if ( inputArguments.length() != 0 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else
                {
                    ret = this->Demo_AccessRights_Options_AddAdvancedNodes(serviceContext);
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

#endif // SUPPORT_Method_Server_Facet

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission




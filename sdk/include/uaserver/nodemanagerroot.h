/******************************************************************************
** nodemanagerroot.h
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
** Project: C++ OPC Server SDK core module
**
** Description: OPC server core module Root NodeManager class.
**
******************************************************************************/
#ifndef NODEMANAGERROOT_H
#define NODEMANAGERROOT_H

#include "nodemanager.h"
#include <vector>
#include <map>
#include "nodemanagerbase.h"
#include "version_coremodule.h"

class SERVER_CORE_EXPORT Session;
class SERVER_CORE_EXPORT ServerManager;
class SERVER_CORE_EXPORT UaObjectTypeSimple;
class SERVER_CORE_EXPORT UaObjectServer;
class NamespaceContext;

/** NodeManagerRoot
 *  @brief Class definition of the NodeManagerRoot.
 *  Create Sessions, starts up the Server. This class controls the session and gives
 *  access to all possible browse possibilities. It is also possible to add new nodes/references,
 *  or to delete old nodes/references. Here also the shutdown of the server will be prepared.
 *  Derived from NodeManagerUaNode, IOManagerUaNode and NodeManagerList
 *  It is not possible to create shallow copies of this Class.
 */
class SERVER_CORE_EXPORT NodeManagerRoot : public NodeManagerBase, public NodeManagerList
{
    UA_DISABLE_COPY(NodeManagerRoot);
    NodeManagerRoot(); // singleton
public:
    /** construction */
    static NodeManagerRoot* CreateRootNodeManager();
    /** destruction */
    virtual ~NodeManagerRoot();

    //- Interface NodeManagerList ----------------------------------------------------------
    // This interface is used to manage the list of NodeManagers and the namespace table
    NodeManager* getNodeManagerByNamespace(OpcUa_UInt16 NsIdx) const;
    UaStatus addNodeManager(NodeManager* pNodeManager, const UaString& sNamespaceUri, OpcUa_UInt16& assignedNamespaceIndex);
    UaStatus removeNodeManager(OpcUa_UInt16 namespaceIndex);
    //- Interface NodeManagerList ----------------------------------------------------------

    /** Get the server object
     *  @return the new created server object.
     */
    inline UaObjectServer* pServerObject() const {return m_pServerObject;};

    // Returns a namespace index for a namespace URI.
    OpcUa_StatusCode getNamespaceIndexFromUri(const UaString& sNamespaceUri, OpcUa_UInt16& namespaceIndex);
    // Returns a namespace URI for a namespace index.
    OpcUa_StatusCode getNamespaceUriFromIndex(OpcUa_UInt16 namespaceIndex, UaString& sNamespaceUri ) const;

    // Returns a namespace index for a COM alias.
    OpcUa_StatusCode getNamespaceIndexFromComAlias(const UaString& comAlias, OpcUa_UInt16& namespaceIndex);
    // Returns a COM alias for a namespace index.
    OpcUa_StatusCode getComAliasFromIndex(OpcUa_UInt16 namespaceIndex, UaString& sComAlias ) const;

    // Inform the node manager about the shutdown of the server.
    void startServerShutDown(
        OpcUa_Int32            secondsTillShutdown,
        const UaLocalizedText& shutdownReason);
    void startServerShutDown(
        OpcUa_Int32            secondsTillShutdown,
        const UaLocalizedTextArray& shutdownReason);

    // Update the server manager about the change of the server state.
    void changeServerState(OpcUa_ServerState newState);
    void changeServiceLevel(OpcUa_Byte serviceLevel, OpcUa_Byte changeMask);

    // Get a UaNode pointer for a requested nodeId and increments reference count. The caller must release the reference when the node is not longer needed.
    UaNode* getNode(const UaNodeId& nodeId);

    // Internal browse methods
    UaStatus browse(
        const UaNodeId& startingNode,
        const UaNodeId& referenceTypeFilter,
        ContinuationPointWrapper& continuationPoint,
        UaReferenceDescriptions& references);
    UaStatus browse(
        const UaNodeId& startingNode,
        OpcUa_Boolean   isInverse,
        const UaNodeId& referenceTypeFilter,
        OpcUa_UInt32    nodeClassMask,
        ContinuationPointWrapper& continuationPoint,
        UaReferenceDescriptions& references);
    UaStatus browse(
        Session*        pSession,
        const UaNodeId& startingNode,
        OpcUa_Boolean   isInverse,
        const UaNodeId& referenceTypeFilter,
        OpcUa_UInt32    nodeClassMask,
        ContinuationPointWrapper& continuationPoint,
        UaReferenceDescriptions& references);

    // Returns the list of super types for a passed OPC UA type node
    UaStatus getListOfSuperTypes(
        const UaNodeId& startingNode,
        UaNodeIdArray&  superTypeList);

    /** Get the server manager. */
    inline ServerManager* pServerManager(){return m_pServerManager;}

    /** Get the type namespace index. */
    static OpcUa_Int16 getTypeNamespace(){return 0;}

    //- Interface NodeManager ------------------------------------------------------------------
    UaStatus startUp(ServerManager* pServerManager);
    UaStatus sessionOpened(Session* session);
    UaStatus sessionActivated(Session* session);
    UaStatus sessionClosed(Session* session);
    UaStatus browse(const ServiceContext& serviceContext, BrowseContext& browseContext, UaReferenceDescriptions& references);
    UaStatus translateBrowsePathToNodeId(const ServiceContext& serviceContext, const UaNodeId& startingNode, UaRelativePath& relativePath, UaBrowsePathTargets& browsePathTargets);
    VariableHandle* getVariableHandle(Session* pSession, VariableHandle::ServiceType serviceType, OpcUa_NodeId* pNodeId, OpcUa_Int32 AttributeId) const;
#if SUPPORT_Historical_Access
    HistoryVariableHandle* getHistoryVariableHandle(Session* pSession, HistoryVariableHandle::ServiceType serviceType, OpcUa_NodeId* pNodeId, UaStatus& result) const;
#endif // SUPPORT_Historical_Access
#if SUPPORT_Event_Subscription_Server_Facet
    UaStatus getEventManagers(Session* pSession, OpcUa_NodeId* pNodeId, OpcUa_EventFilter* eventFilter, EventManagerArray& eventManagers) const;
#endif // SUPPORT_Event_Subscription_Server_Facet
#if SUPPORT_Method_Server_Facet
    MethodHandle* getMethodHandle(Session* pSession, OpcUa_NodeId* pObjectNodeId, OpcUa_NodeId* pMethodNodeId, UaStatus& result) const;
#endif // SUPPORT_Method_Server_Facet
    UaStatus addNode(const ServiceContext& serviceContext, const UaNodeId& parentNodeId, const UaNodeId& referenceTypeId, const UaNodeId& requestedNewNodeId,
        const UaQualifiedName& browseName, OpcUa_NodeClass nodeClass, const UaExtensionObject& nodeAttributes, const UaNodeId& typeDefinition, UaNodeId& addedNodeId);
    UaStatus addReference(const ServiceContext& serviceContext, const UaNodeId& sourceNodeId, const UaNodeId& referenceTypeId,
        OpcUa_Boolean isForward, const UaNodeId& targetNodeId, OpcUa_NodeClass targetNodeClass);
    UaStatus deleteNode(const ServiceContext& serviceContext, const UaNodeId& nodeId, OpcUa_Boolean deleteTargetReference);
    UaStatus deleteReference(const ServiceContext& serviceContext, const UaNodeId& sourceNodeId, const UaNodeId& referenceTypeId, OpcUa_Boolean isForward,
        const UaNodeId& targetNodeId, OpcUa_Boolean deleteBidirectional);
    NodeManagerCrossReferences* getNodeManagerCrossReferences();
    NodeManagerConfig* getNodeManagerConfig();
    //- Interface NodeManager --------------------------------------------------------------

    //- Interface NodeManagerUaNode --------------------------------------------------------
    UaStatus   afterStartUp();
    UaStatus   beforeShutDown();
    //- Interface NodeManagerUaNode --------------------------------------------------------

    //- Interface IOManagerUaNode ----------------------------------------------------------
    UaStatus readValues(const UaVariableArray& arrUaVariables, UaDataValueArray& arrDataValues);
    UaStatus writeValues(const UaVariableArray& arrUaVariables, const PDataValueArray& arrpDataValues, UaStatusCodeArray& arrStatusCodes);
    //- Interface IOManagerUaNode ----------------------------------------------------------

    void decrementSecondsTillShutdown(OpcUa_Int32 secondsTillShutdown);

    inline static OpcUa_UInt32 maxBrowseResults(){return s_maxBrowseResults;}

private:
    UaStatus createTypes(UaReferenceLists* pParentNode);
    UaStatus createPart5_ObjectTypes(UaReferenceLists* pParentNode);
    UaStatus createPart5_VariableTypes(UaReferenceLists* pParentNode);
    UaStatus createPart5_ReferenceTypes(UaReferenceLists* pParentNode);
    UaStatus createPart5_DataTypes(UaReferenceLists* pParentNode);
    void updateNamespaceTable();

private:
    UaFolder*                  m_pRootNode;
    UaObjectServer*            m_pServerObject;
    UaObjectTypeSimple*        m_pBaseObjectType;
    static NodeManagerRoot*    s_pSingleton;
    static OpcUa_UInt32        s_maxBrowseResults;

    mutable UaMutex            m_mutexNodeManagers;
    OpcUa_Boolean              m_bIsShuttingDown;
    OpcUa_Boolean              m_bIsSecondsTillShudownActive;
    OpcUa_UInt16               m_countNodeManagers;
    OpcUa_UInt16               m_sizeNodeManagers;
    NodeManager**              m_pNodeManagers;
    std::vector<UaString>      m_vNamespaceUris;
    std::map<UaString,OpcUa_UInt16> m_mapNamespaceUris;
    std::vector<NamespaceContext*>  m_vNamespaceContext;
};

#endif // NODEMANAGERROOT_H

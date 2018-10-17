#ifndef __NODEMANAGERTREE_H__
#define __NODEMANAGERTREE_H__

#include "nodemanager.h"

class IOManagerTree;
class TreeInterface;
class TreeNodeInterface;

/** Implements the NodeManager interface for a tree based OPC UA server address space
  *
  * This NodeManager implementation is not using the toolkit functionality. It implements the SDK interface NodeManager directly.
  *
  * The NodeManager is able to represent a tree of nodes with a simple parent / child relationship. The tree is access through the interfaces
  * TreeInterface and TreeNodeInterface. The implementation is able to handle all node classes and also different hierarchical reference types
  * but is not able to represent a full mesh network of nodes.
  */
class NodeManagerTree :
    public NodeManager
{
    UA_DISABLE_COPY(NodeManagerTree);
    NodeManagerTree();
public:
    NodeManagerTree(
        const UaString& sNamespaceUri,
        TreeInterface*  pTree);
    virtual ~NodeManagerTree();

    //- Mandatory methods interface NodeManager -----------------------------------------------
    UaStatus startUp(ServerManager* pServerManager);
    UaStatus shutDown();
    UaStatus browse(const ServiceContext& serviceContext, BrowseContext& browseContext, UaReferenceDescriptions& References);
    UaStatus translateBrowsePathToNodeId(const ServiceContext& serviceContext, const UaNodeId& startingNode, UaRelativePath& relativePath, UaBrowsePathTargets& browsePathTargets);
    VariableHandle* getVariableHandle(Session* pSession, VariableHandle::ServiceType serviceType, OpcUa_NodeId* pNodeId, OpcUa_Int32 AttributeId) const;
    //- Interface NodeManager ------------------------------------------------------------------

private:
    UaMutex                 m_mutex;
    OpcUa_Boolean           m_isStarted;

    UaString                m_sNamespaceUri;
    OpcUa_UInt16            m_nNamespaceIndex;
    UaNodeId                m_rootNodeId;

    ServerManager*          m_pServerManager;

    TreeInterface*          m_pTree;
    IOManagerTree*          m_pIOManagerConfig;
};



#endif // __NODEMANAGERTREE_H__
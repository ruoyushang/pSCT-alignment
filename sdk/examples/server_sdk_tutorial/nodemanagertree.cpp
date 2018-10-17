#include "nodemanagertree.h"
#include "iomanagertree.h"
#include "nodemanagerroot.h"
#include "treeinterface.h"

/** Continuation point user data for this NodeManager
 */
class CPNodeManagerTree : public ContinuationPointUserDataBase
{
    UA_DISABLE_COPY(CPNodeManagerTree);
public:
    CPNodeManagerTree() : m_nextChild(0){}
    virtual ~CPNodeManagerTree(){}

    OpcUa_UInt32 m_nextChild;
};

/** Creates and initializes a NodeManagerTree object
  */
NodeManagerTree::NodeManagerTree(
    const UaString& sNamespaceUri,
    TreeInterface* pTree)
: m_isStarted(OpcUa_False),
  m_sNamespaceUri(sNamespaceUri),
  m_nNamespaceIndex(0),
  m_pServerManager(NULL),
  m_pTree(pTree),
  m_pIOManagerConfig(NULL)
{
}

/** Destroys the NodeManagerTree object
 */
NodeManagerTree::~NodeManagerTree()
{
    if ( m_pIOManagerConfig )
    {
        delete m_pIOManagerConfig;
        m_pIOManagerConfig = NULL;
    }
}

/** Start up NodeManagerTree.
 *  @param pServerManager Interface to the central ServerManager object. This interface pointer should be stored
 *  @return Error code.
 */
UaStatus NodeManagerTree::startUp(ServerManager* pServerManager)
{
    if ( pServerManager == NULL )
    {
        return OpcUa_BadInvalidArgument;
    }
    if ( (m_isStarted != OpcUa_False) ||
         (m_pTree == NULL) )
    {
        return OpcUa_BadInternalError;
    }

    UaStatus      ret;
    UaUInt32Array availableSamplingRates;

    m_pServerManager = pServerManager;

    //--------------------------------------------------------------------------------------
    // The following code is necessary to register the node manager in the Server SDK
    //--------------------------------------------------------------------------------------
    // Get to NodeManager root to register the node manager
    NodeManagerRoot *pNodeManagerRoot = (NodeManagerRoot*)m_pServerManager->getNodeManagerRoot();

    // Add this node manager to the list of node managers in the root node manager
    ret = pNodeManagerRoot->addNodeManager(this, m_sNamespaceUri, m_nNamespaceIndex);

    if ( ret.isGood() )
    {
        m_pIOManagerConfig = new IOManagerTree(m_pTree, m_nNamespaceIndex);

        // Our root node is the Objects folder
        m_rootNodeId =  UaNodeId(OpcUaId_ObjectsFolder, 0);

        // Connect to NodeManager root for instance node entry point
        // We will adding nodes to the ObjectsFolder -> connect to this node in the root NodeManager
        ret = pNodeManagerRoot->connectStartingNode(m_rootNodeId, this, OpcUa_True, OpcUa_False);

        m_isStarted = OpcUa_True;
    }
    //--------------------------------------------------------------------------------------

    if ( ret.isGood() )
    {
        // Start up tree manager
        ret = m_pTree->startUp(pServerManager);
    }

    return ret;
}

/** Shut down NodeManagerTree.
 *  @return Error code.
 */
UaStatus NodeManagerTree::shutDown()
{
    if (m_isStarted == OpcUa_False)
    {
        return OpcUa_BadInternalError;
    }
    m_isStarted = OpcUa_False;

    UaStatus ret;

    //--------------------------------------------------------------------------------------
    // The following code is necessary to unregister the node manager from the Server SDK
    //--------------------------------------------------------------------------------------
    // Get to NodeManager root to unregister the node manager
    NodeManagerRoot *pNodeManagerRoot = (NodeManagerRoot*)m_pServerManager->getNodeManagerRoot();

    // Remove this node manager from the list of node managers in the root node manager
    pNodeManagerRoot->removeNodeManager(m_nNamespaceIndex);

    // Disconnect from browse starting node
    pNodeManagerRoot->disconnectStartingNode(m_rootNodeId, this);
    //--------------------------------------------------------------------------------------

    // Shut down tree manager
    ret = m_pTree->shutDown();

    return ret;
}

/** Discover the references of a specified node of a view.
 *  @param serviceContext   General context for the service calls containing information like the session object,
 *                          return diagnostic mask and timeout hint.
 *  @param browseContext    Browse context for the first browse or the following browse with continuation point.
 *  @param References       Array of ReferenceDescription results for the browsed node.
 *  @return                 Error code.
 */
UaStatus NodeManagerTree::browse(
    const ServiceContext&    serviceContext,
    BrowseContext&           browseContext,
    UaReferenceDescriptions& References)
{
    OpcUa_ReferenceParameter(serviceContext);
    // ****************************************************************
    // Error handling
    if (m_isStarted == OpcUa_False)
    {
        return OpcUa_BadInternalError;
    }

    if ( (browseContext.pView()->ViewId.NamespaceIndex != 0) ||
         (browseContext.pView()->ViewId.IdentifierType != OpcUa_IdentifierType_Numeric) ||
         (browseContext.pView()->ViewId.Identifier.Numeric != 0) )
    {
        // We have no views
        return OpcUa_BadViewIdUnknown;
    }

    if ( browseContext.BrowseDirection() == OpcUa_BrowseDirection_Inverse )
    {
        // We support only forward browsing here
        return OpcUa_BadBrowseDirectionInvalid;
    }
    // ****************************************************************

    // ****************************************************************
    // Prepare result mask handling for OPC UA browse results
    bool returnReferenceTypeId = false;
    bool returnIsForward       = false;
    bool returnNodeClass       = false;
    bool returnBrowseName      = false;
    bool returnDisplayName     = false;
    bool returnTypeDefinition  = false;
    if ( browseContext.uResultMask() & OpcUa_BrowseResultMask_ReferenceTypeId ) returnReferenceTypeId = true;
    if ( browseContext.uResultMask() & OpcUa_BrowseResultMask_IsForward ) returnIsForward = true;
    if ( browseContext.uResultMask() & OpcUa_BrowseResultMask_NodeClass ) returnNodeClass = true;
    if ( browseContext.uResultMask() & OpcUa_BrowseResultMask_BrowseName ) returnBrowseName = true;
    if ( browseContext.uResultMask() & OpcUa_BrowseResultMask_DisplayName ) returnDisplayName = true;
    if ( browseContext.uResultMask() & OpcUa_BrowseResultMask_TypeDefinition ) returnTypeDefinition = true;
    // ****************************************************************

    // ****************************************************************
    // Check if a HasTypeDefinition Reference is requested for OPC UA browse results
    bool typeDefinitionRequested = false;
    UaRefHasTypeDefinition refHasTypeDefinition(NULL, NULL);
    UaNodeId               refTypeId(*browseContext.pReferenceTypeId());
    OpcUa_NodeId*          pRefTypeId = browseContext.pReferenceTypeId();
    if ( (browseContext.BrowseDirection() != OpcUa_BrowseDirection_Inverse) &&
            ( (refTypeId.isNull() != OpcUa_False) ||
            refHasTypeDefinition.referenceTypeId().nodeIdCompare(pRefTypeId) ||
            ( (browseContext.bIncludeSubtype() != OpcUa_False) && (refHasTypeDefinition.isSubtypeOf(pRefTypeId)) ) ) )
    {
        typeDefinitionRequested = true;
    }
    // ****************************************************************

    UaStatus ret;
    UaString sIdentifier;

    // ****************************************************************
    // Prepare browse for underlying system
    // * get starting node
    // * get browse filter
    if ( (browseContext.pNodeToBrowse()->NamespaceIndex == m_nNamespaceIndex) &&
         (browseContext.pNodeToBrowse()->IdentifierType == OpcUa_IdentifierType_String))
    {
        // Namespace and identifier type string is handled by this node manager
        // May be a node below root of this node manager

        // Parse the NodeId to check if the node id is valid
        sIdentifier = &browseContext.pNodeToBrowse()->Identifier.String;
    }
    else if ( m_rootNodeId == UaNodeId(*browseContext.pNodeToBrowse()) )
    {
        // Root of this node manager
        sIdentifier = "";

        // Type definition for root is handled by SDK
        typeDefinitionRequested = false;
    }
    else
    {
        // This is not a node from this node manager
        return OpcUa_BadNodeIdUnknown;
    }

    OpcUa_UInt32       i             = 0;
    OpcUa_UInt32       startingChild = 0;
    OpcUa_UInt32       childCount    = 0;
    OpcUa_UInt32       browseCount   = 0;
    TreeNodeInterface* pNodeToBrowse = NULL;
    TreeNodeInterface* pChildNode    = NULL;

    // Lock access to tree
    m_pTree->lockTree();

    // Try to find the requested node
    pNodeToBrowse = m_pTree->findNode(sIdentifier);
    if ( pNodeToBrowse == NULL )
    {
        // Unlock tree
        m_pTree->unlockTree();

        // This node is not known
        return OpcUa_BadNodeIdUnknown;
    }
    else if ( typeDefinitionRequested )
    {
        // Check if the node class of the node to browse has a type definition
        if ( (pNodeToBrowse->getNodeClass() != OpcUa_NodeClass_Object) &&
             (pNodeToBrowse->getNodeClass() != OpcUa_NodeClass_Variable) )
        {
            typeDefinitionRequested = false;
        }
    }

    // ****************************************************************
    // Check if we have a continuation point from OPC UA browse
    if ( browseContext.isFirstBrowse() == OpcUa_False )
    {
        CPNodeManagerTree* pCP = (CPNodeManagerTree*)browseContext.detachUserData();
        if ( pCP )
        {
            startingChild = pCP->m_nextChild;
            delete pCP;
        }
    }
    // ****************************************************************

    childCount = pNodeToBrowse->getChildCount();

    if ( typeDefinitionRequested )
    {
        // Check if the continuation point is valid
        if ( childCount+1 <= startingChild )
        {
            // Unlock tree
            m_pTree->unlockTree();

            // The starting child from continuation point does not mathc current child count
            return OpcUa_BadContinuationPointInvalid;
        }

        // Return one more reference for the type definition
        References.create(childCount+1);
    }
    else
    {
        // Check if there is anything to return
        if ( childCount == 0 )
        {
            // Unlock tree
            m_pTree->unlockTree();

            // Nothing to-do
            return OpcUa_Good;
        }

        // Check if the continuation point is valid
        if ( childCount <= startingChild )
        {
            // Unlock tree
            m_pTree->unlockTree();

            // The starting child from continuation point does not mathc current child count
            return OpcUa_BadContinuationPointInvalid;
        }

        References.create(childCount);
    }

    // ****************************************************************
    // Fill OPC UA browse results with information from children
    pChildNode = pNodeToBrowse->getFirstChildNode();
    for ( i=0; i<childCount; i++ )
    {
        if (pChildNode == NULL)
        {
            break;
        }
        if ( i < startingChild )
        {
            pChildNode = pNodeToBrowse->getNextChildNode();
            // Skip until we reached the continuation point
            continue;
        }

        // Prepare filtering
        OpcUa_NodeClass nodeClass = pChildNode->getNodeClass();
        UaNodeId typeDefinition = pChildNode->getTypeDefinition(m_nNamespaceIndex);
        UaNodeId referenceTypeId(OpcUaId_HasComponent);
        if ( typeDefinition == OpcUaId_FolderType )
        {
            referenceTypeId = OpcUaId_Organizes;
        }
        else if ( typeDefinition == OpcUaId_PropertyType )
        {
            referenceTypeId = OpcUaId_HasProperty;
        }

        if ( !checkReferenceTypeMatch(browseContext.pReferenceTypeId(), referenceTypeId.identifierNumeric(), browseContext.bIncludeSubtype()) )
        {
            // The reference type does not match the reference type filter
            continue;
        }
        if ( (browseContext.uNodeClassMask() != 0) &&
             ((browseContext.uNodeClassMask()&nodeClass) == 0) )
        {
            // The node class not match the node class filter
            continue;
        }

        // Set NodeId - is always returned
        UaNodeId nodeId(pChildNode->getNodeIdentifier(), m_nNamespaceIndex);
        nodeId.copyTo(&References[browseCount].NodeId.NodeId);
        if ( returnBrowseName )
        {
            // Set BrowseName
            UaQualifiedName browseName(pChildNode->getName(), m_nNamespaceIndex);
            browseName.copyTo(&References[browseCount].BrowseName);
        }
        if ( returnDisplayName )
        {
            // Set DisplayName
            UaLocalizedText displayName("", pChildNode->getName());
            displayName.copyTo(&References[browseCount].DisplayName);
        }
        if ( returnNodeClass )
        {
            References[browseCount].NodeClass = nodeClass;
        }
        if ( returnReferenceTypeId )
        {
            referenceTypeId.copyTo(&References[browseCount].ReferenceTypeId);
        }
        if ( returnTypeDefinition )
        {
            typeDefinition.copyTo(&References[browseCount].TypeDefinition.NodeId);
        }
        References[browseCount].IsForward = OpcUa_True;

        browseCount++;
        pChildNode = pNodeToBrowse->getNextChildNode();

        // ****************************************************************
        // Check if need to create a continuation point and stop browsing
        if ( browseCount >= browseContext.uMaxResultsToReturn() )
        {
            // Create continuation point object for browse context
            CPNodeManagerTree* pCP = new CPNodeManagerTree;
            // Set continuation point data for next browse
            pCP->m_nextChild = i+1;
            // Assign continuation point to browse context
            browseContext.setUserData(pCP);
            break;
        }
        // ****************************************************************
    }
    // ****************************************************************

    if ( typeDefinitionRequested && browseCount < browseContext.uMaxResultsToReturn() )
    {
        // Provide type definition if requested
        // Get the node class of the browsed node
        OpcUa_NodeClass nodeClass = pNodeToBrowse->getNodeClass();

        if ( nodeClass == OpcUa_NodeClass_Object )
        {
            // ToDo handle types other than Folder;
            References[browseCount].NodeId.NodeId.Identifier.Numeric = OpcUaId_FolderType;
            if ( returnBrowseName )
            {
                UaQualifiedName browseName("FolderType", 0);
                browseName.copyTo(&References[browseCount].BrowseName);
            }
            if ( returnDisplayName )
            {
                UaLocalizedText displayName("", "FolderType");
                displayName.copyTo(&References[browseCount].DisplayName);
            }
            if ( returnNodeClass != OpcUa_False )
            {
                References[browseCount].NodeClass = OpcUa_NodeClass_ObjectType;
            }
            if ( returnReferenceTypeId )
            {
                References[browseCount].ReferenceTypeId.Identifier.Numeric = OpcUaId_HasTypeDefinition;
            }
            if ( returnTypeDefinition )
            {
                References[browseCount].IsForward   = OpcUa_True;
            }
        }
        else
        {
            // ToDo handle types other than BaseDataVariable
            References[browseCount].NodeId.NodeId.Identifier.Numeric = OpcUaId_BaseDataVariableType;
            if ( returnBrowseName )
            {
                UaQualifiedName browseName("BaseDataVariableType", 0);
                browseName.copyTo(&References[browseCount].BrowseName);
            }
            if ( returnDisplayName )
            {
                UaLocalizedText displayName("", "BaseDataVariableType");
                displayName.copyTo(&References[browseCount].DisplayName);
            }
            if ( returnNodeClass != OpcUa_False )
            {
                References[browseCount].NodeClass = OpcUa_NodeClass_VariableType;
            }
            if ( returnReferenceTypeId )
            {
                References[browseCount].ReferenceTypeId.Identifier.Numeric = OpcUaId_HasTypeDefinition;
            }
            if ( returnTypeDefinition )
            {
                References[browseCount].IsForward   = OpcUa_True;
            }
        }
        browseCount++;
    }

    if ( References.length() > browseCount )
    {
        References.resize(browseCount);
    }

    // Unlock tree
    m_pTree->unlockTree();

    return ret;
}

/** Translates a relative browse path to a node id.
 *  @param serviceContext   General context for the service calls containing information like the session object,
 *                          return diagnostic mask and timeout hint.
 *  @param startingNode     NodeId of the node that needs to be connected to the other NodeManager.
 *  @param relativePath     Relative path from the starting node to the searched node.
 *  @param resultNodeIds    ToDoDoc
 *  @return                 Error code.
 */
UaStatus NodeManagerTree::translateBrowsePathToNodeId(
    const ServiceContext& serviceContext,
    const UaNodeId&       startingNode,
    UaRelativePath&       relativePath,
    UaBrowsePathTargets&  browsePathTargets)
{
    OpcUa_ReferenceParameter(serviceContext);
    if (m_isStarted != OpcUa_False)
    {
        return OpcUa_BadInternalError;
    }

    UaString strNodeId;

    // Root?
    if ((startingNode.namespaceIndex() == 0) &&
        (startingNode.identifierType() == OpcUa_IdentifierType_Numeric) &&
        (startingNode.identifierNumeric() == OpcUaId_ObjectsFolder))
    {
        // Starting node is objects folder -> Root -> Path is empty
        strNodeId = "";
    }
    // Inside our address space?
    else if ((startingNode.namespaceIndex() == m_nNamespaceIndex) && (startingNode.identifierType() == OpcUa_IdentifierType_String))
    {
        // Use node identifier string, this is the current browse path
        strNodeId = startingNode.identifierString();
    }
    else
    {
        return OpcUa_BadNodeIdUnknown;
    }


    OpcUa_ReferenceParameter(startingNode);
    OpcUa_ReferenceParameter(relativePath);
    OpcUa_ReferenceParameter(browsePathTargets);
/*
    ToDo Finish implementation

    // This implementation assumes the ItemId can be build
    // by composing the ItemId of the path and a defined path separator
    // ToDo Provide an alternative implementation that works without this expectation (e.g. using browse)
    while (relativePath.getCurrentElement() != NULL)
    {
        if (strNodeId.length() > 0)
        {
            strNodeId += COM_ITEM_ID_PATH_SEPARATOR;
        }
        // ToDo -> Check all other parameters of the relativePath
        UaString usBrowseName(&relativePath.getCurrentElement()->TargetName.Name);
        strNodeId += usBrowseName;
        relativePath.incrementPosition();
    }

    // Validate the string id
    UaByteArray szItemId = strNodeId.toUtf16();
    if (FAILED(m_pComDaAddressSpace->ValidateFullName((LPWSTR)(const UaUShort*)szItemId)))
    {
        return OpcUa_BadNoMatch;
    }

    // Valid string id -> return the NodeId
    browsePathTargets.create (1);
    UaNodeId tempNode (strNodeId, m_nNamespaceIndex);
    tempNode.copyTo (&browsePathTargets[0].TargetId.NodeId);
    browsePathTargets[0].RemainingPathIndex = OpcUa_UInt32_Max;
*/
    return OpcUa_Good;
}

/** Get the variable handle for the passed node Id.
 *  @param pSession     Interface to the Session context for the method call
 *  @param serviceType  Service type enumerator with the following possible settings
 *  @param pNodeId      NodeId of the requested node
 *  @param AttributeId  AttributeId of the requested node.
 *  @return             a pointer to the VariableHandle.
 */
VariableHandle* NodeManagerTree::getVariableHandle(
    Session*                    pSession,
    VariableHandle::ServiceType serviceType,
    OpcUa_NodeId*               pNodeId,
    OpcUa_Int32                 AttributeId) const
{
    OpcUa_ReferenceParameter(pSession);
    OpcUa_ReferenceParameter(serviceType);
    if ( m_isStarted == OpcUa_False )
    {
        return NULL;
    }

    if ( (pNodeId == NULL) || (pNodeId->NamespaceIndex != m_nNamespaceIndex) )
    {
        return NULL;
    }

    VariableHandle* pVariableHandle = NULL;

    if ( pNodeId->IdentifierType != OpcUa_IdentifierType_String )
    {
        return NULL;
    }

    if ( AttributeId == OpcUa_Attributes_Value )
    {
        // The value attribute is requested - we need to request the VariableHandle from the tree interface
        pVariableHandle = m_pTree->getValueVariableHandle(&pNodeId->Identifier.String);
    }
    else
    {
        pVariableHandle = new VariableHandleTreeNode;
        pVariableHandle->m_AttributeID = AttributeId;
        ((VariableHandleTreeNode*)pVariableHandle)->m_sNodeIdentifier = &pNodeId->Identifier.String;

        // A non value attribute is requested - we are using the IOManagerTree
        pVariableHandle->m_pIOManager  = m_pIOManagerConfig;
    }

    return pVariableHandle;
}


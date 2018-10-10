#include "mytree.h"
#include "mytreenodebase.h"
#include "myfolder.h"
#include "myiomanagervalues.h"
#include "myfolder.h"
#include "mydatavariable.h"

/** Creates a MyTree object with default settings.*/
MyTree::MyTree()
: m_pRootNode(NULL),
  m_pIOManagerValues(NULL)
{
    m_pRootNode = new MyFolder(NULL, "");
    m_pIOManagerValues = new MyIOManagerValues(this);
}

/** Destroys the MyTree object. */
MyTree::~MyTree()
{
    delete m_pIOManagerValues;
    m_pIOManagerValues = NULL;

    delete m_pRootNode;
    m_pRootNode = NULL;
}

UaStatus MyTree::startUp(ServerManager* pServerManager)
{
    OpcUa_ReferenceParameter(pServerManager);

    UaStatus ret;

    MyFolder* pFolder0;
    MyFolder* pFolder1;
    MyFolder* pFolder2;
    MyDataVariable* pTreeVariable;
    pFolder0 = new MyFolder(NULL, "SdkTreeTest");
    addTreeNode(pFolder0);

    int i,j,k;
    int inum = 5;
    int jnum = 5;
    int knum = 500;

    // We create the following number of Int32 variables
    int totalNumberVariables = inum * jnum * knum;

    // Create a simulation of a shared memory as Int32 array
    m_sharedMemorySimulation.create(totalNumberVariables);
    totalNumberVariables = 0;

    for ( i=0; i<inum; i++ )
    {
        // Create first level of folders
        pFolder1 = new MyFolder(pFolder0, UaString("Folder_%1").arg(i));
        addTreeNode(pFolder1);
        for ( j=0; j<jnum; j++ )
        {
            // Create second level of folders
            pFolder2 = new MyFolder(pFolder1, UaString("Folder_%1_%2").arg(i).arg(j));
            addTreeNode(pFolder2);
            for ( k=0; k<knum; k++ )
            {
                // Create variables on the second level of folders
                pTreeVariable = new MyDataVariable(
                    pFolder2,
                    UaString("Variable_%1_%2_%3").arg(i).arg(j).arg(k),
                    OpcUaType_Int32,
                    OpcUa_AccessLevels_CurrentReadOrWrite );
                addTreeNode(pTreeVariable);

                // Set the offset for shared memory simulation
                pTreeVariable->setOffset(totalNumberVariables);
                totalNumberVariables++;
            }
        }
    }

    return ret;
}

UaStatus MyTree::shutDown()
{
    UaStatus ret;


    return ret;
}

/** Find a tree node based on the string identifier
 *  @param nodeIdentifier String indentifier for the node to find
 *  @return Pointer to TreeNodeInterface for the found node or NULL if no node was found
 */
TreeNodeInterface* MyTree::findNode(const UaString& nodeIdentifier)
{
    // Tree is locked by caller

    if ( nodeIdentifier.length() < 1 )
    {
        return m_pRootNode;
    }

    std::map<UaString, TreeNodeInterface*>::iterator it;
    it = m_mapTreeNodes.find(nodeIdentifier);
    if ( it != m_mapTreeNodes.end() )
    {
        return it->second;
    }

    return NULL;
}

/** Get a variable handle for the value attribute of a tree node
 *  @param nodeIdentifier String indentifier for the node to get a handle for
 *  @return Pointer to VariableHandle for the node or NULL if node was found
 */
VariableHandle* MyTree::getValueVariableHandle(const UaString& nodeIdentifier)
{
    if ( nodeIdentifier.length() < 1 )
    {
        return NULL;
    }

    // Lock tree
    UaMutexLocker lock(&m_mutex);

    std::map<UaString, TreeNodeInterface*>::iterator it;
    it = m_mapTreeNodes.find(nodeIdentifier);
    if ( (it != m_mapTreeNodes.end()) &&
         (it->second->getNodeClass() == OpcUa_NodeClass_Variable) )
    {
        // We have a node derived from MyTreeVariable
        MyTreeVariable* pVariable = (MyTreeVariable*)it->second;

        // Get variable handle from node
        MyVariableHandleOffset* pVariableHandle = pVariable->getVariableHandle();

        // Value attribute is requested - we are using our value IOManager
        pVariableHandle->m_AttributeID = OpcUa_Attributes_Value;
        pVariableHandle->m_pIOManager  = m_pIOManagerValues;

        return pVariableHandle;
    }

    return NULL;
}

/** Lock the tree during the time a treenode pointer is used
 */
void MyTree::lockTree()
{
    m_mutex.lock();
}

/** Unlock the tree
 */
void MyTree::unlockTree()
{
    m_mutex.unlock();
}

/** Adds a new tree node to the tree and to the parent node
 */
UaStatus MyTree::addTreeNode(MyTreeNodeBase* pNewNode)
{
    UaMutexLocker lock(&m_mutex);

    // Add to tree
    MyTreeNodeBase* pParentNode = pNewNode->getParentNode();
    if ( pParentNode )
    {
        if ( pParentNode->supportsChildren() )
        {
            ((MyTreeNodeWithChildren*)pParentNode)->addChildNode(pNewNode);
        }
        else
        {
            return OpcUa_BadInvalidArgument;
        }
    }
    else
    {
        m_pRootNode->addChildNode(pNewNode);
    }

    // Add to map
    m_mapTreeNodes[pNewNode->getNodeIdentifier()] = pNewNode;

    return OpcUa_Good;
}

UaStatus MyTree::read(OpcUa_UInt32 offset, UaVariant& value)
{
    // The caller must lock the shared mutex

    if ( offset >= m_sharedMemorySimulation.length() )
    {
        return OpcUa_BadInvalidArgument;
    }

    // Get value from shared memory simulation
    value.setInt32(m_sharedMemorySimulation[offset]);

    return OpcUa_Good;
}

UaStatus MyTree::write(OpcUa_UInt32 offset, const UaVariant& value)
{
    // The caller must lock the shared mutex

    if ( offset >= m_sharedMemorySimulation.length() )
    {
        return OpcUa_BadInvalidArgument;
    }

    // We support only Int32 at the moment
    if ( value.type() != OpcUaType_Int32 )
    {
        return OpcUa_BadTypeMismatch;
    }

    // Update value in shared memory simulation
    return value.toInt32(m_sharedMemorySimulation[offset]);
}

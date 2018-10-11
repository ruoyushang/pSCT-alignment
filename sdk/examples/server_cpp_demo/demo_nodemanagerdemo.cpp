/******************************************************************************
** demo_nodemanagerdemo.cpp
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
** Project: C++ OPC Server SDK information model for namespace http://www.unifiedautomation.com/DemoServer/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#include "demo_nodemanagerdemo.h"
#include "nodemanagerroot.h"

// Include type headers
#include "demo_datatypes.h"
#include "demo_accessrights.h"
#include "demo_structurewithoptionalfields.h"
#include "demo_uniontest.h"
#include "demo_workordertype.h"
#include "demo_workordervariabletype.h"
#include "demo_workorderstatustype.h"
#include <uaunistring.h>
#include "opcua_foldertype.h"
#include "nodeaccessinfo.h"
#if SUPPORT_Method_Server_Facet
#include "opcua_filetype.h"
#endif // SUPPORT_Method_Server_Facet
#include "opcua_basedatavariabletype.h"
#include "opcua_propertytype.h"
#include "opcua_analogitemtype.h"
#include "opcua_multistatediscretetype.h"
#include "opcua_multistatevaluediscretetype.h"
#include "methodhandleuanode.h"
#if SUPPORT_Historical_Access
#include "historymanagercache.h"
#endif // SUPPORT_Historical_Access
#if SUPPORT_Event_Subscription_Server_Facet
#include "opcua_basemodelchangeeventtypedata.h"
#include "demo_sampleeventtypedata.h"
#endif // SUPPORT_Event_Subscription_Server_Facet
#include <uagenericunionvalue.h>
#include "myservercallback.h"
#include "demo_userdata.h"
#include "uafile.h"

#define Demo_PersonStruct               543210
#define Demo_PersonStruct_DefaultBinary 543211
#define Demo_PersonStruct_DefaultXml    543212

#define Demo_Measurement               543213
#define Demo_Measurement_DefaultBinary 543214
#define Demo_Measurement_DefaultXml    543215

#define Demo_StatusSymbols               543216
#define Demo_StatusSymbols_DefaultBinary 543217
#define Demo_StatusSymbols_DefaultXml    543218

#define Demo_CarExtras               543229
#define Demo_CarExtras_DefaultBinary 543220
#define Demo_CarExtras_DefaultXml    543221


// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

/** Construction of the class NodeManagerDemo.
 */
NodeManagerDemo::NodeManagerDemo(
    OpcUa_Boolean firesEvents,  //!< [in] Flag indicating if the NodeManager should activate the EventManager
    const UaString& sAppPath,   //!< [in] Application path
    OpcUa_Int32 nHashTableSize) //!< [in] Size of the hash table. The default value is 10.007.<br>
                                //   The size can be smaller than the total number of nodes managed
                                //   by the node manager but it is better for the performance to avoid
                                //   collisions with a large size.
                                //   Prefered sizes are 1.009, 10.007, 100.003, 1.000.003, 10.000.019.
: NodeManagerDemoBase(firesEvents, nHashTableSize),
m_stopThread(false),
m_sAppPath(sAppPath),
m_nodeVersion(0),
m_pDynamicNode(NULL),
m_totalMassNodesStatic(0),
m_totalMassNodesDynamic(0)
{
    m_defaultLocaleId         = "en";
    m_pSharedNodeMutex = new UaMutexRefCounted;

    m_userIdRoot = 0;
    m_userIdJoe = UserIdJoe;
    m_userIdJohn = UserIdJohn;
    m_userIdSue = UserIdSue;

    m_groupIdSecurityAdmin = 0;
    m_groupIdConfigAdmin = 0;
    m_groupIdUsers = GroupIdUsers;
    m_groupIdOperators = GroupIdOperators;

#if SUPPORT_Historical_Access
    // Create the HistoryManager responsible for this NodeManager
    m_pHistoryManagerCache = new HistoryManagerCache;
    // Register the HistoryManager with the NodeManager base class
    setHistoryManager(m_pHistoryManagerCache);
#endif // SUPPORT_Historical_Access

#if SUPPORT_Event_Subscription_Server_Facet
    m_pUaOffNormalAlarm = NULL;
    m_pUaOffNormalAlarmData = NULL;
    m_pUaNonExclusiveLevelAlarm = NULL;
    m_pUaNonExclusiveLevelAlarmData = NULL;
    m_pUaExclusiveLevelAlarm = NULL;
    m_pUaExclusiveLevelAlarmData = NULL;
#endif // SUPPORT_Event_Subscription_Server_Facet
}

/** Destroys the NodeManagerDemo object and all nodes managed by the node manager
 */
NodeManagerDemo::~NodeManagerDemo()
{
#if SUPPORT_Historical_Access
    setHistoryManager(NULL);
    delete m_pHistoryManagerCache;
#endif // SUPPORT_Historical_Access
    m_pSharedNodeMutex->releaseReference();

#if SUPPORT_Event_Subscription_Server_Facet
    // Delete condition data objects
    // Make sure EventManagerBase::shutDownEM() was called before
    // The EventManagerBase stores condition data object pointers if retain is TRUE
    if ( m_pUaOffNormalAlarmData )
    {
        m_pUaOffNormalAlarmData->releaseReference();
        m_pUaOffNormalAlarmData = NULL;
    }
    if ( m_pUaNonExclusiveLevelAlarmData )
    {
        m_pUaNonExclusiveLevelAlarmData->releaseReference();
        m_pUaNonExclusiveLevelAlarmData = NULL;
    }
    if ( m_pUaExclusiveLevelAlarmData )
    {
        m_pUaExclusiveLevelAlarmData->releaseReference();
        m_pUaExclusiveLevelAlarmData = NULL;
    }
#endif // SUPPORT_Event_Subscription_Server_Facet

    if ( m_pDynamicNode )
    {
        m_pDynamicNode->releaseReference();
        m_pDynamicNode = NULL;
    }
}

/** Finish start up in derived class after starting up base class.
 */
UaStatus NodeManagerDemo::afterStartUp()
{
    // read default roles from configuration
    m_pServerManager->getServerConfig()->getIdsForDefaultRoles(
        m_userIdRoot,
        m_groupIdSecurityAdmin,
        m_groupIdConfigAdmin);

#if SUPPORT_Historical_Access
    // We support historical data access
    m_pHistoryManagerCache->pHistoryServerCapabilities()->setAccessHistoryDataCapability(OpcUa_True);
#endif // SUPPORT_Historical_Access

    s_namespaceIndex = getNameSpaceIndex();

    UaStatus ret =  NodeManagerDemoBase::afterStartUp();

    if ( ret.isGood() )
    {
        createTypeNodes();

        // Create mass variables
        createMassNodes();

        // Create file objects
        createFileNodes();

        // Create special nodes
        createSpecialNodes();

        // Create alarm nodes
        createAlarmNodes();

        // initialize static scalar types
        initializeScalar();

        // Initialize static arrays
        initializeArrays();

        // Set special permissions for certain nodes
        setPermissionsForLimitedNodes();

#if SUPPORT_Historical_Access
        // add variables to historizing
        UaNode* pNode = findNode(UaNodeId(DemoId_Demo_History_ByteWithHistory, getNameSpaceIndex()));
        if ( pNode )
        {
            m_pHistoryManagerCache->addVariableToHistorize(pNode);
        }
        pNode = findNode(UaNodeId(DemoId_Demo_History_DoubleWithHistory, getNameSpaceIndex()));
        if ( pNode )
        {
            m_pHistoryManagerCache->addVariableToHistorize(pNode);
        }
#endif // SUPPORT_Historical_Access
    }

#if SUPPORT_Historical_Access
    m_pHistoryManagerCache->startUp(m_pServerManager);
#endif // SUPPORT_Historical_Access

    // Start worker thread for simulation
    if ( ret.isGood() )
    {
        start();
    }

    return ret;
}

/** Start shut down in derived class before shutting down base class.
*/
UaStatus NodeManagerDemo::beforeShutDown()
{
#if SUPPORT_Historical_Access
    m_pHistoryManagerCache->stopLogging();
    m_pHistoryManagerCache->shutDown();
#endif // SUPPORT_Historical_Access

    UaMutexLocker lock(&m_mutex);
    m_stopThread = true;
    lock.unlock();
    // Wait for worker thread completion
    wait();

    UaMutexLocker lockMassVar(&m_massVarMutex);
    std::list<UaVariable*>::iterator it = m_massVariablesDynamic.begin();
    while ( it != m_massVariablesDynamic.end() )
    {
        // Release reference for the variable in the simulation list
        (*it)->releaseReference();
        it++;
    }
    m_massVariablesDynamic.clear();
    lockMassVar.unlock();

    return OpcUa_Good;
}

#if SUPPORT_Method_Server_Facet

/** Method StartLogging
  */
UaStatus NodeManagerDemo::Demo_History_StartLogging(const ServiceContext& serviceContext)
{
#if SUPPORT_Historical_Access
    UaStatus ret;
    UaNode* pNode = findNode(UaNodeId(DemoId_Demo_History_DataLoggerActive, getNameSpaceIndex()));
    if ( pNode && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        // Get current active value and check state
        UaVariable* pLoggerActive = (UaVariable*)pNode;
        UaDataValue dataValue = pLoggerActive->value(serviceContext.pSession());
        UaVariant value(*dataValue.value());
        OpcUa_Boolean isActive = OpcUa_False;
        if ( OpcUa_IsNotGood(value.toBool(isActive)) )
        {
            return OpcUa_BadInternalError;
        }
        if ( isActive != OpcUa_False )
        {
            return OpcUa_BadInvalidState;
        }

        // Set active value
        value.setBool(OpcUa_True);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        ret = pLoggerActive->setValue(serviceContext.pSession(), dataValue, OpcUa_False);
        if ( ret.isBad() )
        {
            return ret;
        }

        // Start logging
        if (m_pHistoryManagerCache)
        {
            m_pHistoryManagerCache->startLogging();
            return OpcUa_Good;
        }
        else
        {
            return OpcUa_BadInternalError;
        }
    }
    else
    {
        return OpcUa_BadInternalError;
    }
#else
    return OpcUa_BadNotSupported;
#endif // SUPPORT_Historical_Access
}

/** Method StopLogging
  */
UaStatus NodeManagerDemo::Demo_History_StopLogging(const ServiceContext& serviceContext)
{
#if SUPPORT_Historical_Access
    UaStatus ret;
    UaNode* pNode = findNode(UaNodeId(DemoId_Demo_History_DataLoggerActive, getNameSpaceIndex()));
    if ( pNode && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        // Get current active value and check state
        UaVariable* pLoggerActive = (UaVariable*)pNode;
        UaDataValue dataValue = pLoggerActive->value(serviceContext.pSession());
        UaVariant value(*dataValue.value());
        OpcUa_Boolean isActive = OpcUa_False;
        if ( OpcUa_IsNotGood(value.toBool(isActive)) )
        {
            return OpcUa_BadInternalError;
        }
        if ( isActive == OpcUa_False )
        {
            return OpcUa_BadInvalidState;
        }

        value.setBool(OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pLoggerActive->setValue(serviceContext.pSession(), dataValue, OpcUa_False);
        ret = pLoggerActive->setValue(serviceContext.pSession(), dataValue, OpcUa_False);
        if ( ret.isBad() )
        {
            return ret;
        }

        // Stop logging
        if (m_pHistoryManagerCache)
        {
            m_pHistoryManagerCache->stopLogging();
            return OpcUa_Good;
        }
        else
        {
            return OpcUa_BadInternalError;
        }
    }
    else
    {
        return OpcUa_BadInternalError;
    }
#else
    return OpcUa_BadNotSupported;
#endif // SUPPORT_Historical_Access
}

/** Method DoSomethingAfter10s
  * Wait 10 seconds before this method returns
  */
UaStatus NodeManagerDemo::Demo_Method_DoSomethingAfter10s(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaThread::msleep(10000);
    return OpcUa_Good;
}

/** Method EnumTest
  */
UaStatus NodeManagerDemo::Demo_Method_EnumTest(const ServiceContext& serviceContext,
        /*IN*/ Demo::HeaterStatus HeaterStatus,
        /*IN*/ Demo::Priority Priority,
        /*OUT*/ Demo::HeaterStatus& RetHeaterStatus,
        /*OUT*/ Demo::Priority& RetPriority)
{
    OpcUa_ReferenceParameter(serviceContext);
    RetHeaterStatus = HeaterStatus;
    RetPriority = Priority;
    UaStatus ret;
    return ret;
}

/** Method Multiply
  * Returns the product of the two given numbers
  */
UaStatus NodeManagerDemo::Demo_Method_Multiply(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_Double a /**<parameter a */,
        /*IN*/ OpcUa_Double b /**<parameter b */,
        /*OUT*/ OpcUa_Double& result /**<result = a * b.*/)
{
    OpcUa_ReferenceParameter(serviceContext);
    result = a * b;
    return OpcUa_Good;
}

/** Method VectorAdd
  */
UaStatus NodeManagerDemo::Demo_Method_VectorAdd(const ServiceContext& serviceContext,
        /*IN*/ const Demo::Vector& v1,
        /*IN*/ const Demo::Vector& v2,
        /*OUT*/ Demo::Vector& result)
{
    OpcUa_ReferenceParameter(serviceContext);
    result.setX(v1.getX() + v2.getX());
    result.setY(v1.getY() + v2.getY());
    result.setZ(v1.getZ() + v2.getZ());
    return OpcUa_Good;
}

/** Method CreateDynamicNode
  */
UaStatus NodeManagerDemo::Demo_DynamicNodes_CreateDynamicNode(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;

    if ( m_pDynamicNode != NULL )
    {
        // The node is already created
        return OpcUa_BadInvalidState;
    }

    UaNode* pDynamicNodes = findNode(UaNodeId(DemoId_Demo_DynamicNodes, getNameSpaceIndex()));
    UaNode* pNodeVersion = findNode(UaNodeId(DemoId_Demo_DynamicNodes_NodeVersion, getNameSpaceIndex()));

    if ( (pDynamicNodes == NULL) || (pNodeVersion == NULL) || (pNodeVersion->nodeClass() != OpcUa_NodeClass_Variable) )
    {
        // The necessary nodes for the execution of the method are not present
        return OpcUa_BadInvalidState;
    }

    m_pDynamicNode = new OpcUa::FolderType(
        UaNodeId(UaGuid::create(), getNameSpaceIndex()),
        "New Dynamic Node",
        getNameSpaceIndex(),
        this);
    ret = addNodeAndReference(pDynamicNodes->getUaReferenceLists(), m_pDynamicNode, OpcUaId_Organizes);
    if ( ret.isNotGood() )
    {
        // Adding the node failed
        // Release the node since the NodeManager did not take over ownership
        m_pDynamicNode->releaseReference();
        m_pDynamicNode = NULL;
        return ret;
    }
    else
    {
        UaVariant defaultValue;
        OpcUa::DataItemType* pVariable = NULL;

        defaultValue.setUInt32(0);
        pVariable = new OpcUa::DataItemType(
            UaNodeId("Demo.DynamicNodes.Variable1", getNameSpaceIndex()),
            "Variable1",
            getNameSpaceIndex(),
            defaultValue,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this);
        ret = addNodeAndReference(m_pDynamicNode, pVariable, OpcUaId_HasComponent);

        defaultValue.setDouble(0.0);
        pVariable = new OpcUa::DataItemType(
            UaNodeId("Demo.DynamicNodes.Variable2", getNameSpaceIndex()),
            "Variable2",
            getNameSpaceIndex(),
            defaultValue,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this);
        ret = addNodeAndReference(m_pDynamicNode, pVariable, OpcUaId_HasComponent);
    }

    // First keep our own reference to the dynamic node
    m_pDynamicNode->addReference();

    // Increment node version property
    m_nodeVersion++;
    UaVariant value(m_nodeVersion);
    value.changeType(OpcUaType_String, OpcUa_False);
    UaDataValue dataValue;
    dataValue.setValue(value, OpcUa_True, OpcUa_True);
    ((UaVariable*)pNodeVersion)->setValue(serviceContext.pSession(), dataValue, OpcUa_False);

    // Create model change event
#if SUPPORT_Event_Subscription_Server_Facet
    UaModelChangeStructureDataTypes changes;
    UaModelChangeStructureDataType change;

    // Prepare model change structure event data
    changes.create(1);
    // Only a node with a NodeVersion property (DemoId_Demo_DynamicNodes)
    // indicates its changes -> we added a reference to this node
    change.setModelChangeStructureDataType(
        pDynamicNodes->nodeId(),
        pDynamicNodes->typeDefinitionId(),
        OpcUa_ModelChangeStructureVerbMask_ReferenceAdded);
    change.copyTo(&changes[0]);

    // Prepare model change event
    OpcUa::GeneralModelChangeEventTypeData eventData;
    eventData.setMessage(UaLocalizedText("", "Node added"));
    eventData.setChanges(changes);

    // Fire model change event
    fireEvent(&eventData);
#endif // SUPPORT_Event_Subscription_Server_Facet

    return ret;
}

/** Method DeleteDynamicNode
  */
UaStatus NodeManagerDemo::Demo_DynamicNodes_DeleteDynamicNode(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;

    if ( m_pDynamicNode == NULL )
    {
        // The node is not created yet
        return OpcUa_BadInvalidState;
    }

    UaNode* pDynamicNodes = findNode(UaNodeId(DemoId_Demo_DynamicNodes, getNameSpaceIndex()));
    UaNode* pNodeVersion = findNode(UaNodeId(DemoId_Demo_DynamicNodes_NodeVersion, getNameSpaceIndex()));

    if ( (pDynamicNodes == NULL) || (pNodeVersion == NULL) || (pNodeVersion->nodeClass() != OpcUa_NodeClass_Variable) )
    {
        // The necessary nodes for the execution of the method are not present
        return OpcUa_BadInvalidState;
    }

    ret = deleteUaNode(m_pDynamicNode, OpcUa_True, OpcUa_True, OpcUa_True);
    // Release our reference to the node
    m_pDynamicNode->releaseReference();
    m_pDynamicNode = NULL;

    if ( ret.isGood() )
    {
        // Increment node version property
        m_nodeVersion++;
        UaVariant value(m_nodeVersion);
        value.changeType(OpcUaType_String, OpcUa_False);
        UaDataValue dataValue;
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        ((UaVariable*)pNodeVersion)->setValue(serviceContext.pSession(), dataValue, OpcUa_False);

        // Create model change event
#if SUPPORT_Event_Subscription_Server_Facet
        UaModelChangeStructureDataTypes changes;
        UaModelChangeStructureDataType change;

        // Prepare model change structure event data
        changes.create(1);
        // Only a node with a NodeVersion property (DemoId_Demo_DynamicNodes)
        // indicates its changes -> we remvoed a reference from this node
        change.setModelChangeStructureDataType(
            pDynamicNodes->nodeId(),
            pDynamicNodes->typeDefinitionId(),
            OpcUa_ModelChangeStructureVerbMask_ReferenceDeleted);
        change.copyTo(&changes[0]);

        // Prepare model change event
        OpcUa::GeneralModelChangeEventTypeData eventData;
        eventData.setMessage(UaLocalizedText("", "Node removed"));
        eventData.setChanges(changes);

        // Fire model change event
        fireEvent(&eventData);
#endif // SUPPORT_Event_Subscription_Server_Facet
    }

    return ret;
}

/** Method MethodIO
  * Adds 2 unsigned integers. This method is added for compliance testing only.
  */
UaStatus NodeManagerDemo::Demo_CTT_Methods_MethodIO(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_UInt32 Summand1,
        /*IN*/ OpcUa_UInt32 Summand2,
        /*OUT*/ OpcUa_UInt32& Sum)
{
    OpcUa_ReferenceParameter(serviceContext);
    Sum = Summand1 + Summand2;
    UaStatus ret;
    return ret;
}

/** Method MethodNoArgs
  * Method without Arguments and without any functionality. This method is added for compliance testing only.
  */
UaStatus NodeManagerDemo::Demo_CTT_Methods_MethodNoArgs(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;
    return ret;
}

/** Method Method
  */
UaStatus NodeManagerDemo::Demo_CTT_NodeClasses_Method(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;
    return ret;
}

/** Method Method1
  */
UaStatus NodeManagerDemo::Demo_CTT_References_Has3ForwardReferences2_Method1(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;
    return ret;
}

/** Method Method2
  */
UaStatus NodeManagerDemo::Demo_CTT_References_Has3ForwardReferences2_Method2(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;
    return ret;
}

/** Method Method3
  */
UaStatus NodeManagerDemo::Demo_CTT_References_Has3ForwardReferences2_Method3(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;
    return ret;
}

/** Method ReferencedNode3
  */
UaStatus NodeManagerDemo::Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3(const ServiceContext& serviceContext)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus ret;
    return ret;
}

/** Method SetAnimationSpeed
  */
UaStatus NodeManagerDemo::Demo_Images_Dynamic_SetAnimationSpeed(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_UInt32 AnimationSpeed /**<Animation simulation interval [ms]. */)
{
    OpcUa_ReferenceParameter(serviceContext);
    OpcUa_ReferenceParameter(AnimationSpeed);
    UaStatus ret;
    return ret;
}

/** Method SetSimulationSpeed
  */
UaStatus NodeManagerDemo::Demo_SetSimulationSpeed(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_UInt32 speed /**<Simulation interval [ms]. */)
{
    UaNode* pSimulationSpeed = findNode(UaNodeId(DemoId_Demo_SimulationSpeed, getNameSpaceIndex()));

    if ( pSimulationSpeed && (pSimulationSpeed->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        UaDataValue dataValue;
        UaVariant uvValue;
        uvValue.setUInt32(speed);
        dataValue.setValue(uvValue, OpcUa_False, OpcUa_True);
        return ((UaVariable*)pSimulationSpeed)->setValue(serviceContext.pSession(), dataValue, OpcUa_False);
    }
    else
    {
        return OpcUa_BadInternalError;
    }
}

/** Method StartSimulation
  */
UaStatus NodeManagerDemo::Demo_StartSimulation(const ServiceContext& serviceContext)
{
    UaNode* pSimulationActive = findNode(UaNodeId(DemoId_Demo_SimulationActive, getNameSpaceIndex()));

    if ( pSimulationActive && (pSimulationActive->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        UaDataValue dataValue;
        UaVariant uvValue;
        uvValue.setBool(OpcUa_True);
        dataValue.setValue(uvValue, OpcUa_False, OpcUa_True);
        return ((UaVariable*)pSimulationActive)->setValue(serviceContext.pSession(), dataValue, OpcUa_False);
    }
    else
    {
        return OpcUa_BadInternalError;
    }
}

/** Method StopSimulation
  */
UaStatus NodeManagerDemo::Demo_StopSimulation(const ServiceContext& serviceContext)
{
    UaNode* pSimulationActive = findNode(UaNodeId(DemoId_Demo_SimulationActive, getNameSpaceIndex()));

    if ( pSimulationActive && (pSimulationActive->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        UaDataValue dataValue;
        UaVariant uvValue;
        uvValue.setBool(OpcUa_False);
        dataValue.setValue(uvValue, OpcUa_False, OpcUa_True);
        return ((UaVariable*)pSimulationActive)->setValue(serviceContext.pSession(), dataValue, OpcUa_False);
    }
    else
    {
        return OpcUa_BadInternalError;
    }
}

UaStatus NodeManagerDemo::Demo_MassFolder_AddNodes(const UaNodeId& objectId, OpcUa_UInt32 totalCount)
{
    UaStatus ret;
    OpcUa_UInt32 startCount = 0;
    UaNode* pMassFolder = NULL;
    UaString sParentName;
    bool isDynamic = false;
    OpcUa_Byte accessLevel = OpcUa_AccessLevels_CurrentRead;

    if ( totalCount > 100000 )
    {
        return OpcUa_BadInvalidArgument;
    }

    if ( objectId == UaNodeId("Demo.Massfolder_Static", getNameSpaceIndex()) )
    {
        sParentName = "Demo.Massfolder_Static";
        if ( m_totalMassNodesStatic >= totalCount )
        {
            return OpcUa_BadNothingToDo;
        }
        startCount = m_totalMassNodesStatic;
        pMassFolder = findNode(UaNodeId("Demo.Massfolder_Static", getNameSpaceIndex()));
        if ( pMassFolder == NULL )
        {
            return OpcUa_BadInternalError;
        }
        m_totalMassNodesStatic = totalCount;
        accessLevel = OpcUa_AccessLevels_CurrentReadOrWrite;
    }
    else if ( objectId == UaNodeId("Demo.Massfolder_Dynamic", getNameSpaceIndex()) )
    {
        isDynamic = true;
        sParentName = "Demo.Massfolder_Dynamic";
        if ( m_totalMassNodesDynamic >= totalCount )
        {
            return OpcUa_BadNothingToDo;
        }
        startCount = m_totalMassNodesDynamic;
        pMassFolder = findNode(UaNodeId("Demo.Massfolder_Dynamic", getNameSpaceIndex()));
        if ( pMassFolder == NULL )
        {
            return OpcUa_BadInternalError;
        }
        m_totalMassNodesDynamic = totalCount;
        accessLevel = OpcUa_AccessLevels_CurrentReadOrWrite;
    }
    else
    {
        return OpcUa_BadInternalError;
    }


    UaStatus addResult;
    UaVariant value;
    value.setUInt32(0);
    OpcUa::BaseDataVariableType* pVariable = NULL;

    for ( OpcUa_UInt32 i=startCount; i<totalCount; i++ )
    {
        UaString sName(UaString("Variable%1").arg((int)i, 4, 10, UaChar('0')));
        // Dynamic
        UaString sNodeIdMass(UaString("%1.%2").arg(sParentName).arg(sName));
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sNodeIdMass, getTypeNamespace()), sName, getNameSpaceIndex(), value, accessLevel, this, m_pSharedNodeMutex);
        addNodeAndReference(pMassFolder->getUaReferenceLists(), pVariable, OpcUaId_HasComponent);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            if ( isDynamic )
            {
                // Add a reference since we store the variable for simulation
                pVariable->addReference();
                UaMutexLocker lockMassVar(&m_massVarMutex);
                m_massVariablesDynamic.push_back(pVariable);
                lockMassVar.unlock();
            }
        }
    }

    return ret;
}
#endif // SUPPORT_Method_Server_Facet

void NodeManagerDemo::createTypeNodes()
{
    addTypeDictionary(UaNodeId("Demo2", getNameSpaceIndex()), "Demo2", "http://www.unifiedautomation.com/DemoServer/Dictionaries/Demo2");

    // Controller configuration DataType
    UaStructureDefinition personStructure;
    personStructure.setName("Person");
    personStructure.setDataTypeId(UaNodeId(Demo_PersonStruct, getNameSpaceIndex()));
    personStructure.setBinaryEncodingId(UaNodeId(Demo_PersonStruct_DefaultBinary, getNameSpaceIndex()));
    personStructure.setXmlEncodingId(UaNodeId(Demo_PersonStruct_DefaultXml, getNameSpaceIndex()));

    UaStructureField personField;
    personField.setName("Name");
    personField.setDataTypeId(OpcUaId_String);
    personField.setArrayType(UaStructureField::ArrayType_Scalar);
    personStructure.addChild(personField);
    personField.setName("Height");
    personField.setDataTypeId(OpcUaId_UInt16);
    personStructure.addChild(personField);
    personField.setName("Weight");
    personField.setDataTypeId(OpcUaId_Float);
    personStructure.addChild(personField);
    addStructuredType(personStructure);

    UaStructureDefinition analogValueUnion;
    analogValueUnion.setName("AnalogMeasurementValue");
    analogValueUnion.setDataTypeId(UaNodeId(Demo_Measurement, getNameSpaceIndex()));
    analogValueUnion.setBinaryEncodingId(UaNodeId(Demo_Measurement_DefaultBinary, getNameSpaceIndex()));
    analogValueUnion.setXmlEncodingId(UaNodeId(Demo_Measurement_DefaultXml, getNameSpaceIndex()));

    UaStructureField analogValueField;
    analogValueField.setName("Temperature");
    analogValueField.setDataTypeId(OpcUaId_Float);
    analogValueField.setArrayType(UaStructureField::ArrayType_Scalar);
    analogValueUnion.addChild(analogValueField);

    analogValueField.setName("Voltage");
    analogValueField.setDataTypeId(OpcUaId_Int32);
    analogValueUnion.addChild(analogValueField);

    analogValueField.setName("Preassure");
    analogValueField.setDataTypeId(OpcUaId_UInt16);
    analogValueUnion.addChild(analogValueField);

    analogValueUnion.setUnion(true);

    addStructuredType(analogValueUnion);

    //create struct with optional fields
    UaStructureDefinition statusSymbolsStruct;
    statusSymbolsStruct.setName("StatusSymbols");
    statusSymbolsStruct.setDataTypeId(UaNodeId(Demo_StatusSymbols, getNameSpaceIndex()));
    statusSymbolsStruct.setBinaryEncodingId(UaNodeId(Demo_StatusSymbols_DefaultBinary, getNameSpaceIndex()));
    statusSymbolsStruct.setXmlEncodingId(UaNodeId(Demo_StatusSymbols_DefaultXml, getNameSpaceIndex()));

    UaStructureField statusSymbolField;
    statusSymbolField.setName("Job");
    statusSymbolField.setDataTypeId(OpcUaId_String);
    statusSymbolField.setArrayType(UaStructureField::ArrayType_Scalar);
    statusSymbolField.setOptional(true);
    statusSymbolsStruct.addChild(statusSymbolField);

    statusSymbolField.setName("House");
    statusSymbolField.setDataTypeId(OpcUaId_String);
    statusSymbolField.setOptional(true);
    statusSymbolsStruct.addChild(statusSymbolField);

    statusSymbolField.setName("Car");
    statusSymbolField.setDataTypeId(OpcUaId_String);
    statusSymbolField.setOptional(true);

    statusSymbolsStruct.addChild(statusSymbolField);

    addStructuredType(statusSymbolsStruct);

    //create option set definition for car extras
    UaOptionSetDefinition carExtraDefinition;
    carExtraDefinition.setName("CarExtras");
    carExtraDefinition.setDataTypeId(UaNodeId(Demo_CarExtras, getNameSpaceIndex()));
    carExtraDefinition.setBinaryEncodingId(UaNodeId(Demo_CarExtras_DefaultBinary, getNameSpaceIndex()));
    carExtraDefinition.setXmlEncodingId(UaNodeId(Demo_CarExtras_DefaultXml, getNameSpaceIndex()));

    carExtraDefinition.addChild(UaLocalizedText("en", "air conditioner"));
    carExtraDefinition.addChild(UaLocalizedText("en", "super charged"));
    carExtraDefinition.addChild(UaLocalizedText("en", "launch control"));
    carExtraDefinition.addChild(UaLocalizedText("en", "navigation system"));
    carExtraDefinition.setBaseType(UaOptionSetDefinition::BaseType_OptionSet);

    addOptionSetType(carExtraDefinition);
}

void NodeManagerDemo::createMassNodes()
{
    UaStatus addResult;

    // Create mass variables
    UaNode* pMassStatic = findNode(UaNodeId(DemoId_Demo_Massfolder_Static, getNameSpaceIndex()));
    UaNode* pMassDynamic = findNode(UaNodeId(DemoId_Demo_Massfolder_Dynamic, getNameSpaceIndex()));

    if ( pMassStatic && pMassDynamic )
    {
        UaVariant value;

#if SUPPORT_Method_Server_Facet
        OpcUa::PropertyType* pProperty = NULL;
        OpcUa::BaseMethod* pMethod = NULL;

        // Massfolder_Static.AddNodes
        pMethod = new OpcUa::BaseMethod(UaNodeId("Demo.Massfolder_Static.AddNodes", getNameSpaceIndex()), "AddNodes", getNameSpaceIndex());
        addResult = addNodeAndReference(pMassStatic->getUaReferenceLists(), pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addResult.isGood());
        //InputArguments
        {
            UaArguments valInputArgumentsArray;
            valInputArgumentsArray.create(1);
            UaArgument argument;
            argument.setName("TotalNumber");
            argument.setDataType(UaNodeId(OpcUaId_UInt32));
            argument.setDescription(UaLocalizedText("", "Total number of mass nodes in the folder"));
            argument.setValueRank(-1);
            argument.copyTo(&valInputArgumentsArray[ 0 ]);
            valInputArgumentsArray.toVariant(value);
        }
        pProperty = new OpcUa::PropertyType(UaNodeId("Demo.Massfolder_Static.AddNodes.InputArguments", getNameSpaceIndex()), "InputArguments", NodeManagerRoot::getTypeNamespace(), value, 1, this);
        pProperty->setDataType(UaNodeId(OpcUaId_Argument, NodeManagerRoot::getTypeNamespace()));
        addResult = addNodeAndReference(pMethod, pProperty, OpcUaId_HasProperty);

        UA_ASSERT(addResult.isGood());
        // Massfolder_Static.AddNodes
        pMethod = new OpcUa::BaseMethod(UaNodeId("Demo.Massfolder_Dynamic.AddNodes", getNameSpaceIndex()), "AddNodes", getNameSpaceIndex());
        addResult = addNodeAndReference(pMassDynamic->getUaReferenceLists(), pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addResult.isGood());
        //InputArguments
        {
            UaArguments valInputArgumentsArray;
            valInputArgumentsArray.create(1);
            UaArgument argument;
            argument.setName("TotalNumber");
            argument.setDataType(UaNodeId(OpcUaId_UInt32));
            argument.setDescription(UaLocalizedText("", "Total number of mass nodes in the folder"));
            argument.setValueRank(-1);
            argument.copyTo(&valInputArgumentsArray[ 0 ]);
            valInputArgumentsArray.toVariant(value);
        }
        pProperty = new OpcUa::PropertyType(UaNodeId("Demo.Massfolder_Dynamic.AddNodes.InputArguments", getNameSpaceIndex()), "InputArguments", NodeManagerRoot::getTypeNamespace(), value, 1, this);
        pProperty->setDataType(UaNodeId(OpcUaId_Argument, NodeManagerRoot::getTypeNamespace()));
        addResult = addNodeAndReference(pMethod, pProperty, OpcUaId_HasProperty);
        UA_ASSERT(addResult.isGood());
#endif // SUPPORT_Method_Server_Facet

        value.setUInt32(0);
        OpcUa::BaseDataVariableType* pVariable = NULL;

        m_totalMassNodesStatic = 1000;
        m_totalMassNodesDynamic = 1000;
        for ( int i=0; i<1000; i++ )
        {
            UaString sName(UaString("Variable%1").arg(i, 4, 10, UaChar('0')));
            // Static
            UaString sNodeIdStatic(UaString("%1.%2").arg(DemoId_Demo_Massfolder_Static).arg(sName));
            pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sNodeIdStatic, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
            addResult = addNodeAndReference(pMassStatic->getUaReferenceLists(), pVariable, OpcUaId_HasComponent);
            if ( addResult.isBad() )
            {
                pVariable->deleteAllChildren(this);
                pVariable->releaseReference();
            }
            // Dynamic
            UaString sNodeIdDynamic(UaString("%1.%2").arg(DemoId_Demo_Massfolder_Dynamic).arg(sName));
            pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sNodeIdDynamic, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
            addResult = addNodeAndReference(pMassDynamic->getUaReferenceLists(), pVariable, OpcUaId_HasComponent);
            if ( addResult.isBad() )
            {
                pVariable->deleteAllChildren(this);
                pVariable->releaseReference();
            }
            else
            {
                // Add a reference since we store the variable for simulation
                pVariable->addReference();
                m_massVariablesDynamic.push_back(pVariable);
            }
        }
    }
}

void NodeManagerDemo::createFileNodes()
{
    UaStatus addResult;

    // Create file nodes
    OpcUa::FolderType* pFolder = (OpcUa::FolderType*)findNode(UaNodeId(DemoId_Demo_Files, getTypeNamespace()));

    if (pFolder)
    {
#if SUPPORT_Method_Server_Facet
        OpcUa::FileType* pFileType = NULL;
        UaString sFilePath;
        // Create a FileType object for server configuration file
        sFilePath = UaString("%1/ServerConfig.xml").arg(m_sAppPath);
        pFileType = new OpcUa::FileType(UaNodeId("Demo.Files.serverconfig.xml", getNameSpaceIndex()), "serverconfig.xml", getNameSpaceIndex(), this);
        pFileType->setFilePath(sFilePath);
        pFileType->setWritable(OpcUa_False);
        pFileType->setUserWritable(OpcUa_False);
        addResult = addNodeAndReference(pFolder, pFileType, OpcUaId_HasComponent);
        UA_ASSERT(addResult.isGood());

        // Create a FileType object for nodeset import file
        sFilePath = UaString("%1/uanodesetimport.xml").arg(m_sAppPath);
        pFileType = new OpcUa::FileType(UaNodeId("Demo.Files.uanodesetimport.xml", getNameSpaceIndex()), "uanodesetimport.xml", getNameSpaceIndex(), this);
        pFileType->setFilePath(sFilePath);
        pFileType->setWritable(OpcUa_False);
        pFileType->setUserWritable(OpcUa_False);
        addResult = addNodeAndReference(pFolder, pFileType, OpcUaId_HasComponent);
        UA_ASSERT(addResult.isGood());
#endif // SUPPORT_Method_Server_Facet
    }
}

void NodeManagerDemo::initializeScalar()
{
    // Demo.CTT.AllProfiles.Scalar.ByteString
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ByteString, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.DateTime
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_DateTime, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.Guid
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Guid, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.LocalizedText
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_LocalizedText, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.NodeId
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_NodeId, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.QualifiedName
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_QualifiedName, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.String
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_String, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.Variant
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Variant, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Scalar.XmlElement
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_XmlElement, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.DataItem.DateTime
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_DateTime, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.DataItem.String
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_String, getNameSpaceIndex()));

    // DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscreteXX
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete1, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete2, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete3, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete4, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete5, getNameSpaceIndex()));

    // DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_<DataType>
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Byte, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int16, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int32, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int64, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_SByte, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt16, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt32, getNameSpaceIndex()));
    initializeMultiState(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt64, getNameSpaceIndex()));

    // Demo.Static.Scalar.BaseDataType
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_BaseDataType, getNameSpaceIndex()));

    // Demo.Static.Scalar.ByteString
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_ByteString, getNameSpaceIndex()));

    // Demo.Static.Scalar.ExpandedNodeId
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_ExpandedNodeId, getNameSpaceIndex()));

    // Demo.Static.Scalar.DateTime
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_DateTime, getNameSpaceIndex()));

    // Demo.Static.Scalar.Guid
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Guid, getNameSpaceIndex()));

    // Demo.Static.Scalar.ImagePNG
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_ImagePNG, getNameSpaceIndex()));

    // Demo.Static.Scalar.Integer
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Integer, getNameSpaceIndex()));

    // Demo.Static.Scalar.LocalizedText
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_LocalizedText, getNameSpaceIndex()));

    // Demo.Static.Scalar.NodeId
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_NodeId, getNameSpaceIndex()));

    // Demo.Static.Scalar.Number
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Number, getNameSpaceIndex()));

    // Demo.Static.Scalar.NumericRange
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_NumericRange, getNameSpaceIndex()));

    // Demo.Static.Scalar.OptionSet
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Structures_OptionSet, getNameSpaceIndex()));

    // Demo.Static.Scalar.OptionalFields
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Structures_OptionalFields, getNameSpaceIndex()));

    // Demo.Static.Scalar.Number
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Number, getNameSpaceIndex()));

    // Demo.Static.Scalar.Structure
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Structures_Structure, getNameSpaceIndex()));

    // Demo.Static.Scalar.UInteger
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_UInteger, getNameSpaceIndex()));

    // Demo.Static.Scalar.UtcTime
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_UtcTime, getNameSpaceIndex()));

    // Demo.Static.Scalar.QualifiedName
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_QualifiedName, getNameSpaceIndex()));

    // Demo.Static.Scalar.String
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_String, getNameSpaceIndex()));

    // Demo.Static.Scalar.Union
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Structures_Union, getNameSpaceIndex()));

    // Demo.Static.Scalar.XmlElement
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_XmlElement, getNameSpaceIndex()));

    // Demo.Static.Scalar.Vector
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Scalar_Structures_Vector, getNameSpaceIndex()));

    // Demo.Static.Scalar.WorkOrder
    initializeWorkOrder(UaNodeId(DemoId_Demo_Static_Scalar_Structures_WorkOrder, getNameSpaceIndex()));

    // Demo.WorkOrder.WorkOrderVariable
    initializeWorkOrder(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable, getNameSpaceIndex()));

    // Demo.WorkOrder.WorkOrderVariable2
    initializeWorkOrder(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable2, getNameSpaceIndex()));
}

void NodeManagerDemo::initializeArrays()
{
    // Demo.CTT.AllProfiles.Arrays.Bool
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Bool, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Byte
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Byte, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.ByteString
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_ByteString, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.DateTime
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_DateTime, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Guid
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Guid, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Double
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Double, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Duration
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Duration, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Float
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Float, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Int16
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int16, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Int32
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int32, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Int64
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int64, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.LocalizedText
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_LocalizedText, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.QualifiedName
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_QualifiedName, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.SByte
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_SByte, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.String
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_String, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.UInt16
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt16, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.UInt32
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt32, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.UInt64
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt64, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.Variant
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Variant, getNameSpaceIndex()));

    // Demo.CTT.AllProfiles.Arrays.XmlElement
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_XmlElement, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.AnalogTypeArrays.Double
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Double, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.AnalogTypeArrays.Float
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Float, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.AnalogTypeArrays.Int16
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int16, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.AnalogTypeArrays.Int32
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int32, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.AnalogTypeArrays.UInt16
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt16, getNameSpaceIndex()));

    // Demo.CTT.DAProfile.AnalogTypeArrays.UInt32
    initializeVariableValue(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt32, getNameSpaceIndex()));

    // Demo.Static.Arrays.BaseDataType
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_BaseDataType, getNameSpaceIndex()));

    // Demo.Static.Arrays.Boolean
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Boolean, getNameSpaceIndex()));

    // Demo.Static.Arrays.Byte
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Byte, getNameSpaceIndex()));

    // Demo.Static.Arrays.ByteString
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_ByteString, getNameSpaceIndex()));

    // Demo.Static.Arrays.DateTime
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_DateTime, getNameSpaceIndex()));

    // Demo.Static.Arrays.Double
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Double, getNameSpaceIndex()));

    // Demo.Static.Arrays.Duration
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Duration, getNameSpaceIndex()));

    // Demo.Static.Arrays.ExpandedNodeId
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_ExpandedNodeId, getNameSpaceIndex()));

    // Demo.Static.Arrays.Float
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Float, getNameSpaceIndex()));

    // Demo.Static.Arrays.Guid
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Guid, getNameSpaceIndex()));

    // Demo.Static.Arrays.ImagePNG
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_ImagePNG, getNameSpaceIndex()));

    // Demo.Static.Arrays.Int16
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Int16, getNameSpaceIndex()));

    // Demo.Static.Arrays.Int32
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Int32, getNameSpaceIndex()));

    // Demo.Static.Arrays.Int64
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Int64, getNameSpaceIndex()));

    // Demo.Static.Arrays.Integer
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Integer, getNameSpaceIndex()));

    // Demo.Static.Arrays.LocalizedText
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_LocalizedText, getNameSpaceIndex()));

    // Demo.Static.Arrays.NodeId
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_NodeId, getNameSpaceIndex()));

    // Demo.Static.Arrays.Number
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Number, getNameSpaceIndex()));

    // Demo.Static.Arrays.NumericRange
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_NumericRange, getNameSpaceIndex()));

    // Demo.Static.Arrays.QualifiedName
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_QualifiedName, getNameSpaceIndex()));

    // Demo.Static.Arrays.SByte
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_SByte, getNameSpaceIndex()));

    // Demo.Static.Arrays.ServerState
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_ServerState, getNameSpaceIndex()));

    // Demo.Static.Arrays.StatusCode
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_StatusCode, getNameSpaceIndex()));

    // Demo.Static.Arrays.String
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_String, getNameSpaceIndex()));

    // Demo.Static.Arrays.Structure
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Structure, getNameSpaceIndex()));

    // Demo.Static.Arrays.UInt16
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_UInt16, getNameSpaceIndex()));

    // Demo.Static.Arrays.UInt32
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_UInt32, getNameSpaceIndex()));

    // Demo.Static.Arrays.UInt64
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_UInt64, getNameSpaceIndex()));

    // Demo.Static.Arrays.UInteger
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_UInteger, getNameSpaceIndex()));

    // Demo.Static.Arrays.UtcTime
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_UtcTime, getNameSpaceIndex()));

    // Demo.Static.Arrays.Vector
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_Vector, getNameSpaceIndex()));

    // Demo.Static.Arrays.WorkOrder
    initializeWorkOrder(UaNodeId(DemoId_Demo_Static_Arrays_WorkOrder, getNameSpaceIndex()));

    // Demo.Static.Arrays.XmlElement
    initializeVariableValue(UaNodeId(DemoId_Demo_Static_Arrays_XmlElement, getNameSpaceIndex()));
}

void NodeManagerDemo::initializeVariableValue(const UaNodeId& nodeId)
{
    OpcUa_Int32 i,j;
    OpcUa_Int32 count = 10;
    UaDataValue dataValue;
    UaVariant uvValue;
    UaNode* pNode = findNode(nodeId);
    UaVariable* pVariable = NULL;
    UaNodeId dataType;

    UaByteString byteStringImagePNG = UaByteString(4276, (OpcUa_Byte*) "\211\120\116\107\015\012\032\012\000\000\000\015\111\110\104\122\000\000\000\127\000\000\000\127\010\006\000\000\000\161\310\331"
        "\154\000\000\000\001\163\122\107\102\000\256\316\034\351\000\000\000\006\142\113\107\104\000\377\000\377\000\377\240\275\247\223"
        "\000\000\000\011\160\110\131\163\000\000\015\327\000\000\015\327\001\102\050\233\170\000\000\000\007\164\111\115\105\007\333\012"
        "\025\013\060\073\373\102\311\166\000\000\020\064\111\104\101\124\170\332\325\135\175\224\024\325\225\377\335\252\352\356\351\031"
        "\140\306\010\014\013\042\240\046\220\210\213\233\250\213\021\115\334\204\354\256\311\131\326\370\201\270\156\320\343\061\147\235"
        "\131\305\030\016\203\236\125\070\350\012\146\217\211\223\354\014\204\234\263\206\020\021\202\161\027\217\046\033\046\021\162\200"
        "\060\012\050\004\230\001\135\020\006\031\232\257\141\076\373\273\337\335\077\252\152\272\272\273\272\373\275\356\231\351\366\315"
        "\171\247\252\077\246\352\127\277\372\275\373\356\275\357\325\153\102\031\226\372\372\072\064\065\065\073\137\023\200\152\000\143"
        "\000\214\141\346\351\104\064\035\300\024\000\127\003\030\013\340\063\326\347\076\000\136\000\004\040\012\040\002\140\000\300\105"
        "\000\135\000\216\003\350\000\160\024\100\033\200\113\000\372\001\364\064\065\065\307\263\141\050\244\120\231\223\072\007\300\035"
        "\000\156\002\060\321\252\325\000\300\314\040\032\022\370\375\000\002\000\072\001\374\031\300\157\001\374\306\306\121\014\311\045"
        "\047\267\256\356\121\064\067\257\266\057\304\000\120\013\140\061\200\177\005\140\130\137\143\027\254\074\004\370\163\035\143\003"
        "\200\377\000\160\270\251\251\071\366\251\043\327\126\105\175\175\235\027\300\303\000\026\000\270\065\355\342\263\341\034\156\162"
        "\355\162\000\300\146\000\315\115\115\315\227\352\353\353\000\100\112\315\124\006\304\176\035\300\106\313\146\122\332\105\247\157"
        "\221\345\363\142\210\225\071\006\003\210\003\130\334\324\324\374\143\331\023\150\245\040\325\052\023\353\353\353\066\003\150\001"
        "\160\171\332\005\122\226\055\362\274\137\110\253\225\075\206\007\100\143\175\175\335\273\165\165\217\136\017\000\113\226\054\056"
        "\037\345\072\324\372\105\000\333\254\336\235\313\261\163\315\246\140\146\046\042\012\011\041\026\257\136\275\246\371\226\133\276"
        "\214\135\273\376\124\132\162\035\304\316\001\260\243\030\122\323\075\005\131\257\201\231\063\130\052\224\141\000\324\327\327\367"
        "\350\372\365\277\134\123\122\345\072\210\275\014\300\307\226\073\245\144\057\155\062\064\115\203\246\151\040\002\172\173\373\142"
        "\335\335\075\241\140\160\040\021\014\206\242\241\120\220\205\020\224\110\010\146\146\030\206\116\000\151\176\177\205\136\131\131"
        "\251\373\375\176\243\272\272\332\137\135\135\255\151\232\006\041\004\204\020\005\023\315\314\150\157\157\177\172\333\266\355\053"
        "\313\301\054\264\063\363\014\042\122\355\210\070\026\213\045\102\241\120\354\344\311\216\323\307\216\035\373\344\364\351\323\027"
        "\000\174\002\340\043\053\060\070\017\340\054\200\136\053\170\210\132\175\212\327\252\143\001\214\267\352\147\153\153\153\277\164"
        "\315\065\127\137\075\165\352\324\053\252\252\252\056\327\165\335\243\151\232\222\377\314\314\234\110\044\242\055\055\277\377\332"
        "\361\343\307\167\225\322\054\074\000\140\275\254\142\231\231\165\135\247\213\027\273\372\016\036\374\363\305\063\147\316\354\277"
        "\160\341\342\026\000\207\000\234\262\210\054\266\030\000\046\115\233\066\365\363\223\047\137\071\267\266\166\334\003\265\265\023"
        "\306\263\131\040\243\146\146\106\133\133\333\037\267\157\377\343\127\113\102\156\135\335\243\006\021\375\217\025\155\111\235\223"
        "\210\260\167\357\336\377\153\155\175\367\050\200\225\000\166\215\000\124\375\346\233\147\057\235\065\153\326\363\272\256\113\373"
        "\322\335\335\335\211\127\137\335\160\007\200\255\043\356\212\021\121\025\200\131\262\212\215\307\343\361\267\336\172\253\275\265"
        "\365\335\325\000\356\034\041\142\001\040\261\173\167\353\277\167\165\165\135\307\314\347\144\205\060\166\354\130\175\324\250\121"
        "\337\055\225\237\353\005\160\205\344\215\240\366\366\043\347\116\234\070\271\006\300\017\001\304\060\202\345\366\333\277\212\137"
        "\375\152\363\041\042\172\304\351\135\344\022\003\063\143\312\224\053\257\261\134\313\021\047\127\123\120\071\372\372\172\317\000"
        "\170\245\024\216\354\266\155\333\155\322\366\310\164\154\266\135\366\371\174\166\136\244\164\021\232\224\341\323\215\050\000\275"
        "\224\030\210\110\127\021\204\145\101\264\364\336\162\370\145\253\151\212\027\126\372\033\254\202\231\210\134\061\227\235\162\211"
        "\010\022\246\256\354\060\273\365\175\306\310\235\074\063\004\225\371\176\151\011\223\307\234\315\211\056\011\340\162\046\166\250"
        "\260\214\224\237\013\073\264\114\007\156\157\315\174\101\371\020\353\206\327\211\331\211\267\344\066\227\231\141\047\135\354\316"
        "\302\271\265\057\044\033\320\221\056\066\336\364\316\315\336\267\163\020\271\104\241\114\356\222\226\371\170\354\067\337\124\126"
        "\156\072\200\164\345\332\337\161\373\156\051\115\102\072\076\267\353\311\206\171\320\346\076\271\365\056\124\032\076\074\377\067"
        "\033\260\270\345\336\317\022\360\035\042\272\023\300\164\000\141\006\016\201\371\125\001\336\374\223\073\336\076\373\375\226\173"
        "\360\322\334\315\043\142\273\076\215\366\066\105\271\076\130\304\156\275\173\031\200\017\100\364\157\014\134\313\200\301\300\050"
        "\000\263\101\364\023\000\207\026\157\275\373\366\227\346\156\306\075\057\336\072\142\156\315\247\332\025\133\371\215\015\370\376"
        "\357\356\276\027\240\345\126\300\354\152\212\000\134\016\322\336\271\243\341\206\277\332\334\260\143\377\160\004\021\345\322\241"
        "\025\253\162\055\151\026\276\155\200\360\040\023\203\211\311\332\246\125\230\357\103\340\262\311\143\176\006\240\252\034\233\143"
        "\051\224\353\006\333\110\366\216\320\100\074\016\071\375\145\206\245\177\146\301\127\303\034\265\035\030\152\302\312\201\137\365"
        "\233\234\303\054\020\021\300\000\233\004\346\033\074\244\104\202\075\262\311\225\241\000\132\306\344\146\035\177\113\052\027\054"
        "\175\115\314\246\151\220\367\031\213\157\142\043\357\347\252\211\041\267\131\020\102\115\072\012\047\327\064\162\371\247\134\023"
        "\152\262\227\105\213\026\241\261\261\021\000\120\137\137\067\236\210\374\212\301\101\254\251\251\271\023\000\236\170\142\021\136"
        "\176\271\061\007\146\067\174\031\223\177\262\212\334\050\244\045\232\243\366\205\044\063\334\046\316\220\222\053\326\330\330\210"
        "\307\036\173\174\052\021\257\047\242\251\060\107\071\124\112\374\361\307\037\273\004\340\173\057\277\334\330\342\234\010\230\216"
        "\303\126\157\152\246\216\122\304\221\014\040\162\330\134\301\102\261\331\260\102\123\327\362\310\237\034\012\311\135\236\174\362"
        "\173\063\141\116\216\323\120\330\134\061\006\360\027\000\266\326\327\327\375\155\123\123\363\326\232\232\032\164\167\167\347\154"
        "\352\346\076\271\340\226\311\212\061\113\134\040\251\333\004\105\045\347\353\110\164\135\177\241\010\142\007\117\152\235\347\115"
        "\000\323\273\273\273\117\252\165\254\224\041\236\234\066\127\020\233\004\223\204\241\145\100\050\020\254\032\103\144\343\367\276"
        "\373\026\300\060\214\277\267\104\100\251\071\127\014\066\123\346\314\143\070\047\173\130\373\354\363\371\174\137\370\302\347\137"
        "\150\153\153\377\247\142\060\023\041\167\156\141\320\250\160\166\135\222\245\154\126\356\116\111\001\150\156\233\153\030\306\120"
        "\345\240\311\060\014\370\375\376\111\000\374\000\102\305\141\316\225\025\343\144\145\153\113\126\115\176\306\326\147\074\044\246"
        "\041\233\102\335\336\137\260\340\176\154\334\370\032\014\303\070\156\030\036\014\101\145\237\257\002\207\017\267\165\015\217\211"
        "\163\232\005\010\020\310\326\045\334\350\343\002\375\100\267\070\335\376\177\367\346\227\011\364\265\327\066\330\307\372\117\135"
        "\327\177\310\014\266\063\200\366\361\062\115\101\352\315\162\116\162\324\165\215\016\034\070\020\010\207\303\037\146\252\066\173"
        "\156\301\355\230\122\312\145\346\024\005\147\253\146\020\301\105\253\326\015\123\256\016\155\371\362\345\170\346\231\147\177\324"
        "\323\323\263\336\347\363\222\327\353\045\257\327\213\364\352\361\170\340\365\172\341\363\231\325\371\231\317\347\205\337\137\101"
        "\035\035\035\135\157\274\361\106\073\200\025\305\340\116\365\042\262\166\150\012\256\130\062\114\056\312\046\251\076\221\263\174"
        "\371\162\254\133\367\163\054\134\370\340\167\256\273\156\346\007\063\147\316\274\317\343\361\136\246\142\040\243\321\110\374\360"
        "\341\266\300\301\203\007\017\001\170\036\100\120\055\374\315\164\122\064\015\271\275\005\145\041\162\161\066\326\331\234\210\234"
        "\315\072\267\073\266\160\341\203\130\270\360\237\261\156\335\372\037\035\074\170\350\025\000\065\120\037\121\211\370\174\336\100"
        "\044\022\115\310\342\316\014\042\234\357\347\011\042\244\162\005\051\256\260\120\266\271\116\073\355\044\224\331\274\373\311\317"
        "\163\337\271\165\353\326\333\273\335\126\125\056\221\110\064\057\346\164\274\104\200\020\311\375\124\101\024\253\134\112\272\152"
        "\205\131\334\364\321\137\116\261\133\066\300\362\311\351\332\176\063\073\132\031\273\012\045\147\207\306\062\177\154\157\205\262"
        "\315\265\107\112\223\323\356\063\137\347\212\323\163\225\177\330\064\103\352\173\363\066\316\120\302\354\304\351\166\015\116\354"
        "\022\146\101\045\374\035\236\071\107\052\252\375\356\133\163\261\366\133\055\370\114\305\270\133\036\332\122\353\317\235\013\021"
        "\241\237\377\343\216\135\057\166\076\204\206\211\257\110\341\220\315\237\144\303\154\244\207\217\062\315\232\031\140\121\110\204"
        "\046\227\162\224\345\167\355\267\132\360\320\233\137\171\116\047\355\151\146\304\123\175\317\264\255\246\033\367\257\377\362\117"
        "\033\046\276\122\047\227\230\222\116\071\146\045\070\065\131\256\142\215\024\132\156\062\237\053\363\254\236\232\131\320\110\273"
        "\323\034\242\202\227\035\311\052\073\017\065\230\217\042\100\304\360\010\314\147\172\167\312\345\103\264\054\370\122\257\041\133"
        "\300\141\024\342\133\015\157\156\101\165\014\215\023\311\173\344\120\032\071\267\366\110\013\107\141\076\325\063\244\230\363\052"
        "\127\244\064\363\374\331\074\265\241\233\341\261\271\231\270\235\042\111\335\222\106\351\343\203\103\212\331\315\076\073\106\042"
        "\130\076\112\140\024\035\376\146\167\322\025\125\043\075\042\302\103\202\071\023\253\324\320\272\120\074\215\172\020\221\253\025"
        "\330\137\161\113\300\344\376\137\247\347\222\043\231\157\215\156\027\213\331\306\227\232\014\312\347\212\051\336\324\241\164\304"
        "\234\144\046\073\077\025\162\035\116\177\066\154\212\043\326\262\346\302\046\072\167\204\106\054\075\160\302\005\347\163\311\325"
        "\256\233\300\270\100\263\060\350\040\132\051\123\333\144\121\212\303\127\230\040\310\105\116\224\101\162\376\040\302\122\200\254"
        "\165\120\317\347\262\053\360\344\161\264\301\160\123\115\271\002\356\356\044\247\007\334\112\240\063\061\247\016\242\312\034\312"
        "\160\155\136\062\353\146\050\067\061\112\013\124\050\053\341\205\230\205\174\215\316\234\343\306\105\140\246\074\341\075\044\203"
        "\010\011\014\205\030\205\124\307\234\323\062\205\354\332\344\144\174\000\031\114\266\311\120\065\011\232\306\110\016\066\303\065"
        "\273\231\377\151\036\146\045\342\324\346\055\220\204\223\356\076\153\173\250\354\023\063\053\171\104\311\254\135\276\210\221\362"
        "\233\005\221\352\057\246\130\357\214\006\254\050\133\025\262\224\203\010\111\077\227\120\310\114\113\222\041\221\211\334\247\342"
        "\271\346\163\071\113\077\231\236\236\034\276\242\144\027\024\102\010\245\326\246\251\145\305\044\274\005\231\270\207\325\315\202"
        "\164\173\324\064\015\341\160\050\041\157\025\130\372\176\251\140\016\207\303\347\375\176\377\340\172\066\371\310\035\030\350\217"
        "\147\134\113\122\333\042\131\141\325\054\257\115\117\101\111\271\175\000\275\147\215\217\161\372\223\073\126\145\115\323\020\215"
        "\106\343\373\366\275\177\022\346\062\052\022\331\071\007\106\312\136\141\135\203\114\131\271\362\005\054\133\266\074\304\214\077"
        "\230\015\336\025\257\135\051\030\014\305\366\355\173\277\033\346\262\205\231\344\246\014\237\247\277\227\366\232\131\315\360\066"
        "\064\054\015\003\374\220\246\221\365\010\175\146\042\203\210\210\231\171\315\232\237\036\210\104\042\133\221\145\124\326\275\031"
        "\101\142\072\200\074\344\247\236\172\332\076\370\102\042\072\143\105\215\256\311\027\042\302\353\257\157\076\041\204\330\004\240"
        "\307\225\134\241\360\307\020\112\136\356\017\176\360\042\032\032\226\266\005\002\201\332\256\256\256\075\375\375\175\102\010\141"
        "\307\357\034\014\006\343\035\035\035\275\215\215\077\336\171\352\324\251\015\000\176\041\237\341\260\376\050\037\146\365\360\267"
        "\241\141\351\351\143\307\076\236\325\325\165\151\367\300\300\100\314\201\031\341\160\070\336\331\171\146\140\355\332\237\275\267"
        "\177\377\201\065\000\126\113\165\150\356\241\150\232\351\121\260\012\113\226\064\340\331\147\237\301\212\025\317\235\003\160\333"
        "\234\071\163\156\350\351\351\236\027\213\305\077\027\215\106\252\202\301\320\047\201\100\140\007\314\325\361\072\012\113\162\160"
        "\216\220\165\160\066\226\122\131\272\164\051\126\255\132\165\036\300\155\263\147\317\276\061\030\014\176\063\032\215\136\027\213"
        "\105\053\203\301\320\331\100\040\260\213\231\377\267\272\272\372\104\117\117\117\256\040\102\144\341\314\045\353\124\100\022\144"
        "\305\212\347\006\373\212\235\073\167\356\204\304\150\200\034\267\042\015\035\147\035\114\122\055\253\126\255\262\167\343\255\255"
        "\255\273\001\354\166\373\236\033\261\012\131\061\067\207\277\074\026\105\340\074\201\045\247\270\142\142\104\261\251\115\012\161"
        "\040\056\227\005\047\230\205\224\054\235\223\014\113\100\056\043\157\006\304\361\031\227\213\162\235\251\122\316\145\013\270\164"
        "\344\246\264\170\221\026\122\247\201\056\027\142\263\032\126\027\201\050\172\217\103\254\134\347\243\122\204\274\023\164\325\207"
        "\205\206\111\271\116\334\234\075\054\046\015\045\264\271\131\246\024\270\265\266\222\310\040\117\137\313\030\372\307\141\212\055"
        "\311\010\115\210\301\312\216\175\141\275\146\347\076\213\222\167\150\363\254\371\141\102\010\026\016\174\302\161\015\354\270\026"
        "\266\037\071\050\005\271\114\000\023\022\154\331\132\353\165\322\366\122\252\035\046\003\232\121\101\074\351\346\321\245\126\356"
        "\170\023\057\017\342\113\307\315\144\366\023\042\041\270\064\312\005\022\000\316\310\066\305\061\123\174\225\137\132\064\351\246"
        "\323\273\373\112\242\330\055\363\217\140\336\246\031\213\140\076\260\307\371\362\265\104\100\337\351\110\077\106\160\155\310\101"
        "\162\267\314\077\222\000\360\276\254\311\322\075\032\152\257\257\372\345\274\215\323\037\111\277\360\341\044\166\313\374\043\366"
        "\376\123\000\136\222\011\300\230\231\065\017\241\143\133\117\100\071\274\056\276\073\030\004\077\023\300\176\270\317\100\313\000"
        "\154\145\262\000\140\017\021\335\267\145\376\221\343\351\044\014\003\301\067\000\170\003\346\252\246\362\317\102\014\044\242\277"
        "\175\370\243\235\140\174\155\304\311\265\011\231\267\151\306\106\000\363\025\102\162\347\367\132\001\374\032\300\136\000\037\156"
        "\231\177\244\163\010\310\234\002\340\163\000\146\003\270\013\346\072\274\200\302\012\322\104\104\037\376\367\205\316\366\215\027"
        "\236\200\371\203\032\043\354\212\361\340\170\374\003\104\370\153\006\246\222\272\372\147\133\065\002\040\070\157\323\214\040\200"
        "\143\000\366\301\374\241\241\116\230\113\142\007\141\076\373\025\005\120\141\325\112\060\256\004\341\012\230\053\102\335\010\140"
        "\032\314\037\062\252\104\352\323\351\122\153\011\330\304\106\172\343\221\243\277\276\270\315\122\074\112\142\026\000\340\333\257"
        "\317\100\074\312\067\202\360\007\322\150\164\061\113\376\253\347\011\062\046\205\024\374\013\046\314\314\244\021\305\006\022\321"
        "\167\026\177\334\032\276\030\177\322\272\311\045\361\026\314\334\132\034\170\363\376\243\173\172\077\211\314\215\207\004\150\004"
        "\237\376\160\071\125\121\304\106\373\022\321\035\313\116\036\015\137\214\057\033\151\142\263\202\277\376\137\046\140\377\232\000"
        "\246\375\135\315\265\323\357\032\373\073\357\050\175\022\151\204\221\124\161\301\244\022\021\063\270\367\144\270\167\373\322\023"
        "\177\002\343\005\014\121\356\130\265\270\056\000\024\330\333\217\232\253\052\020\330\323\177\276\257\063\272\121\063\250\272\242"
        "\306\370\242\247\122\043\026\154\217\035\225\035\311\272\117\243\110\167\074\162\350\027\347\072\016\254\015\274\016\140\031\314"
        "\205\057\112\031\231\147\057\125\265\136\014\234\215\102\257\240\333\376\362\341\332\377\232\174\153\365\125\116\162\163\250\171"
        "\070\177\371\051\345\264\314\314\054\300\307\336\356\072\336\366\352\371\016\000\315\226\327\122\016\151\017\371\122\163\215\377"
        "\236\151\337\250\131\120\065\301\363\365\232\151\025\243\075\125\072\070\301\140\341\112\364\260\374\146\031\151\000\351\204\150"
        "\177\042\176\351\243\120\377\331\367\373\273\073\266\367\274\023\033\020\277\007\260\005\262\043\307\345\106\256\355\302\215\236"
        "\354\035\037\351\115\334\073\361\246\321\017\117\272\145\314\225\343\256\255\034\003\262\246\240\362\040\045\105\223\073\170\303"
        "\310\044\125\304\231\317\037\032\270\320\261\255\367\334\331\017\372\173\105\214\337\026\061\336\000\363\247\144\142\345\144\246"
        "\206\312\156\136\013\340\346\011\067\216\272\151\334\314\312\253\252\046\170\247\172\374\332\170\303\257\371\014\277\116\106\005"
        "\151\232\107\323\064\017\221\156\220\351\243\020\071\122\230\346\150\102\042\306\054\142\314\042\046\104\074\314\211\170\130\210"
        "\130\177\042\021\352\212\207\172\117\105\072\273\332\203\307\057\036\011\235\260\174\347\075\000\336\103\031\027\032\206\343\125"
        "\002\250\324\075\030\125\071\301\073\125\323\351\052\315\240\011\314\230\110\032\252\101\164\031\004\127\222\116\136\042\170\314"
        "\014\046\307\000\304\070\301\041\026\350\141\301\227\104\234\073\023\141\016\204\057\305\155\062\203\216\232\300\247\240\374\077"
        "\154\317\357\235\043\045\062\236\000\000\000\000\111\105\116\104\256\102\140\202");

    if ( pNode && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        pVariable = (UaVariable*)pNode;
    }

    if ( pVariable )
    {
        dataType = pVariable->dataType();
    }
    else
    {
        return;
    }

    // namespace 0 datatypes
    if (dataType.namespaceIndex() == 0 && dataType.identifierType() == OpcUa_IdentifierType_Numeric)
    {
        // scalar
        if (pVariable->valueRank() == OpcUa_ValueRanks_Scalar)
        {
            switch (dataType.identifierNumeric())
            {
            case OpcUaId_ByteString:
                {
                    UaByteString bsTmp;
                    UaByteArray valueByteStringHelper;
                    valueByteStringHelper.resize(count);

                    for ( i=0; i<count; i++ )
                    {
                        valueByteStringHelper[i] = (char)(i * 0x10);
                    }
                    bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
                    uvValue.setByteString(bsTmp, OpcUa_True);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_DateTime:
            case OpcUaId_UtcTime:
                uvValue.setDateTime(UaDateTime::now());
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
                break;
            case OpcUaId_Guid:
                uvValue.setGuid(UaGuid::create());
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
                break;
            case OpcUaId_String:
                uvValue.setString("Hello world");
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
                break;
            case OpcUaId_XmlElement:
                {
                    UaByteString bsTmp;
                    UaByteArray valueByteStringHelper;
                    UaString sTmp("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                    valueByteStringHelper = sTmp.toLocal8Bit();
                    bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
                    uvValue.setXmlElement(bsTmp, OpcUa_True);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_NodeId:
                {
                    UaNodeId tempNodeId("MyStringNodeId",1);
                    uvValue.setNodeId(tempNodeId);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_ExpandedNodeId:
                {
                    UaExpandedNodeId tempNodeId(UaNodeId("ExpandedNodeId", 2), UaString(""), 3);
                    uvValue.setExpandedNodeId(tempNodeId);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_QualifiedName:
                {
                    UaQualifiedName tempQualifiedName("QualifiedName", 2);
                    uvValue.setQualifiedName(tempQualifiedName);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_LocalizedText:
                {
                    UaLocalizedText tempLocalizedText("en", "LocalizedText");
                    uvValue.setLocalizedText(tempLocalizedText);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Structure:
                {
                    UaRange tmpRange;
                    UaExtensionObject tmpExtensionObject;
                    tmpRange.setRange(1.00, 28.43);
                    tmpRange.toExtensionObject(tmpExtensionObject);
                    uvValue.setExtensionObject(tmpExtensionObject, OpcUa_True);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_BaseDataType:
                {
                    uvValue.setByte(123);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Number:
                {
                    uvValue.setFloat((OpcUa_Float)12.554);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Integer:
                {
                    uvValue.setInt16(-1234);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_UInteger:
                {
                    uvValue.setUInt16(1234);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_NumericRange:
                {
                    uvValue.setString("1:2,0:1");
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_ImagePNG:
                {
                    uvValue.setByteString(byteStringImagePNG, OpcUa_False);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            default:
                break;
            }
        }
        // array
        else if (pVariable->valueRank() == OpcUa_ValueRanks_OneDimension)
        {
            switch (dataType.identifierNumeric())
            {
            case OpcUaId_Boolean:
                {
                    UaBooleanArray tempArray;
                    tempArray.create(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = OpcUa_True;
                    }
                    uvValue.setBoolArray(tempArray, OpcUa_True);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Byte:
                {
                    UaByteArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = (OpcUa_Byte)i;
                    }
                    uvValue.setByteArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_BaseDataType:
                {
                    UaVariantArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        switch (i%5)
                        {
                        case 0:
                            uvValue.setBool(OpcUa_True);
                            break;
                        case 1:
                            uvValue.setByte(21);
                            break;
                        case 2:
                            uvValue.setInt32(234);
                            break;
                        case 3:
                            uvValue.setFloat(123.45f);
                            break;
                        default:
                            uvValue.setString("Hello world");
                        }
                        uvValue.copyTo(&tempArray[i]);
                    }
                    tempArray.toDataValue(dataValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_ByteString:
                {
                    UaByteStringArray tempArray;
                    tempArray.resize(count);
                    UaByteString bsTmp;
                    UaByteArray valueByteStringHelper;
                    for ( i=0; i<count; i++ )
                    {
                        OpcUa_ByteString_Initialize(&tempArray[i]);
                        valueByteStringHelper.resize(8);
                        for ( j=0; j<valueByteStringHelper.size(); j++ )
                        {
                            valueByteStringHelper[j] = (char)( (i+j) * 0x10);
                        }
                        bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
                        bsTmp.copyTo(&tempArray[i]);
                    }
                    uvValue.setByteStringArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_DateTime:
            case OpcUaId_UtcTime:
                {
                    UaDateTimeArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = UaDateTime::now();
                    }
                    uvValue.setDateTimeArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Double:
            case OpcUaId_Duration:
                {
                    UaDoubleArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = i*0.2+10;
                    }
                    uvValue.setDoubleArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Float:
            case OpcUaId_Number:
                {
                    UaFloatArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = (OpcUa_Float)(i*0.1+20);
                    }
                    uvValue.setFloatArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Guid:
                {
                    UaGuidArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        UaGuid::create().copyTo(&tempArray[i]);
                    }
                    uvValue.setGuidArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Int16:
                {
                    UaInt16Array tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = (OpcUa_Int16)i-5;
                    }
                    uvValue.setInt16Array(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Int32:
            case OpcUaId_Integer:
                {
                    UaInt32Array tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = i-5;
                    }
                    uvValue.setInt32Array(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Int64:
                {
                    UaInt64Array tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = i-5;
                    }
                    uvValue.setInt64Array(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_SByte:
                {
                    UaSByteArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = (OpcUa_SByte)i-5;
                    }
                    uvValue.setSByteArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_String:
                {
                    UaStringArray tempArray;
                    tempArray.resize(count);
                    UaString sTemp;
                    for ( i=0; i<count; i++ )
                    {
                        OpcUa_String_Initialize(&tempArray[i]);
                        sTemp = UaString("Hello_%1")
                            .arg((int)i);
                        sTemp.copyTo(&tempArray[i]);
                    }
                    uvValue.setStringArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_UInt16:
                {
                    UaUInt16Array tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = (OpcUa_UInt16)i;
                    }
                    uvValue.setUInt16Array(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_UInt32:
            case OpcUaId_UInteger:
                {
                    UaUInt32Array tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = i;
                    }
                    uvValue.setUInt32Array(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_UInt64:
                {
                    UaUInt64Array tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = i;
                    }
                    uvValue.setUInt64Array(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_XmlElement:
                {
                    UaByteStringArray tempArray;
                    tempArray.resize(count);
                    UaString sTmp("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                    UaByteString bsTmp;
                    UaByteArray valueByteStringHelper = sTmp.toLocal8Bit();
                    bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());

                    for ( i=0; i<count; i++ )
                    {
                        OpcUa_ByteString_Initialize(&tempArray[i]);
                        bsTmp.copyTo(&tempArray[i]);
                    }
                    uvValue.setXmlElementArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_NodeId:
                {
                    UaNodeIdArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        UaString sTmp;
                        UaNodeId tempNodeId;
                        switch (i%4)
                        {
                            // numeric
                        case 0:
                            tempNodeId = UaNodeId(i, i%5);
                            break;
                            //String
                        case 1:
                            if (i < 10)
                            {
                                sTmp = UaString("NodeId_0%1").arg(i);
                            }
                            else
                            {
                                sTmp = UaString("NodeId_%1").arg(i);
                            }
                            tempNodeId = UaNodeId(sTmp, i%5);
                            break;
                            // Guid
                        case 2:
                            tempNodeId = UaNodeId(UaGuid::create(), i%5);
                            break;
                            // ByteString
                        case 3:
                            UaByteString bsTmp;
                            UaByteArray valueByteStringHelper;
                            valueByteStringHelper.resize(8);
                            for ( j=0; j<valueByteStringHelper.size(); j++ )
                            {
                                valueByteStringHelper[j] = (char)( (i+j) * 0x10);
                            }
                            bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
                            tempNodeId = UaNodeId(bsTmp, i%5);
                            break;
                        }
                        tempNodeId.copyTo(&tempArray[i]);
                    }
                    uvValue.setNodeIdArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_ExpandedNodeId:
                {
                    UaExpandedNodeIdArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        UaString sTmp;
                        if (i < 10)
                        {
                            sTmp = UaString("ExpandedNodeId_0%1").arg(i);
                        }
                        else
                        {
                            sTmp = UaString("ExpandedNodeId_%1").arg(i);
                        }
                        UaExpandedNodeId tempNodeId(UaNodeId(sTmp, (OpcUa_UInt16)i%3), UaString(""), i%5);
                        tempNodeId.copyTo(&tempArray[i]);
                    }
                    uvValue.setExpandedNodeIdArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_StatusCode:
                {
                    UaStatusCodeArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        switch (i%6)
                        {
                        case 0:
                            tempArray[i] = OpcUa_Good;
                            break;
                        case 1:
                            tempArray[i] = OpcUa_BadNoCommunication;
                            break;
                        case 2:
                            tempArray[i] = OpcUa_BadNodeIdInvalid;
                            break;
                        case 3:
                            tempArray[i] = OpcUa_BadIndexRangeInvalid;
                            break;
                        case 4:
                            tempArray[i] = OpcUa_BadNotReadable;
                            break;
                        case 5:
                            tempArray[i] = OpcUa_BadNotImplemented;
                            break;
                        }
                    }
                    uvValue.setStatusCodeArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_QualifiedName:
                {
                    UaQualifiedNameArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        UaString sTmp;
                        if (i < 10)
                        {
                            sTmp = UaString("QualifiedName_0%1").arg(i);
                        }
                        else
                        {
                            sTmp = UaString("QualifiedName_%1").arg(i);
                        }
                        UaQualifiedName tempQualifiedName(sTmp, i%5);
                        tempQualifiedName.copyTo(&tempArray[i]);
                    }
                    uvValue.setQualifiedNameArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_LocalizedText:
                {
                    UaLocalizedTextArray tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        UaString sTmp;
                        if (i < 10)
                        {
                            sTmp = UaString("LocalizedText_0%1").arg(i);
                        }
                        else
                        {
                            sTmp = UaString("LocalizedText_%1").arg(i);
                        }
                        UaLocalizedText tempLocalizedText;
                        if (i%2)
                        {
                            tempLocalizedText = UaLocalizedText("en", sTmp);
                        }
                        else
                        {
                            tempLocalizedText = UaLocalizedText("de", sTmp);
                        }
                        tempLocalizedText.copyTo(&tempArray[i]);
                    }
                    uvValue.setLocalizedTextArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_Structure:
                {
                    UaRange tmpRange;
                    UaExtensionObject tmpExtensionObject;
                    UaExtensionObjectArray tempArray;
                    tempArray.create(count);
                    for ( i=0; i<count; i++ )
                    {
                        tmpRange.setLow(i*1.2);
                        tmpRange.setHigh(i*2.3+1);
                        tmpRange.toExtensionObject(tmpExtensionObject);
                        tmpExtensionObject.copyTo(&tempArray[i]);
                    }
                    uvValue.setExtensionObjectArray(tempArray, OpcUa_True);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_NumericRange:
                {
                    UaStringArray tempArray;
                    tempArray.resize(count);
                    UaString sTemp;
                    for ( i=0; i<count; i++ )
                    {
                        OpcUa_String_Initialize(&tempArray[i]);
                        sTemp = UaString("%1:%2,0:1")
                            .arg((int)i)
                            .arg((int)count-i);
                        sTemp.copyTo(&tempArray[i]);
                    }
                    uvValue.setStringArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_ServerState:
                {
                    UaInt32Array tempArray;
                    tempArray.resize(count);
                    for ( i=0; i<count; i++ )
                    {
                        tempArray[i] = i%8;
                    }
                    uvValue.setInt32Array(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            case OpcUaId_ImagePNG:
                {
                    UaByteStringArray tempArray;
                    tempArray.resize(3);
                    for ( i=0; i<3; i++ )
                    {
                        byteStringImagePNG.copyTo(&tempArray[i]);
                    }
                    uvValue.setByteStringArray(tempArray);
                    dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                    pVariable->setValue(NULL, dataValue, OpcUa_False);
                }
                break;
            default:
                break;
            }
        }
    }
    // types from this namespace
    else if (dataType.namespaceIndex() == getNameSpaceIndex() && dataType.identifierType() == OpcUa_IdentifierType_Numeric)
    {
        // scalar
        if (pVariable->valueRank() == OpcUa_ValueRanks_Scalar)
        {
            // Vector
            if (dataType.identifierNumeric() == DemoId_Vector)
            {
                Vector vectorVal;
                vectorVal.setVector(12.34, 23.45, 34.56);
                vectorVal.toVariant(uvValue);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
            }
            // OptionSet
            if (dataType.identifierNumeric() == DemoId_AccessRights)
            {
                AccessRights valOptionSet(AccessRights::Bit_Read);
                valOptionSet |= Demo::AccessRights::Bit_Write;
                valOptionSet.toVariant(uvValue);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
                // change value handling because we want to check values before writing
                pVariable->setValueHandling(UaVariable_Value_CacheIsSource | UaVariable_Value_CacheIsUpdatedOnRequest);
            }
            // OptionalFields
            if (dataType.identifierNumeric() == DemoId_StructureWithOptionalFields)
            {
                StructureWithOptionalFields valOptionalFields;
                valOptionalFields.setMandatoryInt32(123);
                UaString sTemp;
                UaStringArray tempArray;
                tempArray.resize(3);
                sTemp = UaString("Hello");
                sTemp.copyTo(&tempArray[0]);
                sTemp = UaString("OPC UA");
                sTemp.copyTo(&tempArray[1]);
                sTemp = UaString("world.");
                sTemp.copyTo(&tempArray[2]);
                valOptionalFields.setMandatoryStringArray(tempArray);
                valOptionalFields.toVariant(uvValue);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
            }
            // Union
            if (dataType.identifierNumeric() == DemoId_UnionTest)
            {
                UnionTest valUnion;
                valUnion.setString("This is a string.");
                valUnion.toVariant(uvValue);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
            }
        }
        else if (pVariable->valueRank() == OpcUa_ValueRanks_OneDimension)
        {
            // Vector
            if (dataType.identifierNumeric() == DemoId_Vector)
            {
                Vectors vectorArray;
                vectorArray.create(count);
                for ( i=0; i<count; i++ )
                {
                    vectorArray[i].X = i*1.1;
                    vectorArray[i].Y = i*2.2;
                    vectorArray[i].Z = i*3.3;
                }
                vectorArray.toVariant(uvValue);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pVariable->setValue(NULL, dataValue, OpcUa_False);
            }
        }
    }
}

void NodeManagerDemo::initializeMultiState(const UaNodeId& nodeId)
{
    UaNode* pNode = findNode(nodeId);

    if (!pNode)
    {
        return;
    }

    if (pNode->typeDefinitionId() == UaNodeId(OpcUaId_MultiStateDiscreteType) ||
        pNode->typeDefinitionId() == UaNodeId(OpcUaId_MultiStateValueDiscreteType))
    {
        OpcUa::BaseVariableType* pVariable = (OpcUa::BaseVariableType*)pNode;

        // change value handling because we want to check values before writing
        pVariable->setValueHandling(UaVariable_Value_CacheIsSource | UaVariable_Value_CacheIsUpdatedOnRequest);
    }

    if (pNode->typeDefinitionId() == UaNodeId(OpcUaId_MultiStateValueDiscreteType))
    {
        OpcUa::MultiStateValueDiscreteType *pMultiState = (OpcUa::MultiStateValueDiscreteType*) pNode;
        UaEnumValueTypes enumValues;
        pMultiState->getEnumValues(enumValues);
        if (enumValues.length() > 0)
        {
            UaDataValue dv = pMultiState->value(0);
            UaVariant v = *dv.value();
            switch (v.type())
            {
            case OpcUaType_Byte:
                v.setByte((OpcUa_Byte) enumValues[0].Value);
                break;
            case OpcUaType_SByte:
                v.setSByte((OpcUa_SByte) enumValues[0].Value);
                break;
            case OpcUaType_UInt16:
                v.setUInt16((OpcUa_UInt16) enumValues[0].Value);
                break;
            case OpcUaType_Int16:
                v.setInt16((OpcUa_Int16) enumValues[0].Value);
                break;
            case OpcUaType_UInt32:
                v.setUInt32((OpcUa_UInt32) enumValues[0].Value);
                break;
            case OpcUaType_Int32:
                v.setInt32((OpcUa_Int32) enumValues[0].Value);
                break;
            case OpcUaType_UInt64:
                v.setUInt64((OpcUa_UInt64) enumValues[0].Value);
                break;
            case OpcUaType_Int64:
                v.setInt64((OpcUa_Int64) enumValues[0].Value);
                break;
            default:
                break;
            }
            dv.setValue(v, OpcUa_True, OpcUa_True);
            pMultiState->setValue(NULL, dv, OpcUa_False);
            pMultiState->setValueAsText(enumValues[0].DisplayName);
        }
    }
}

void NodeManagerDemo::initializeWorkOrder(const UaNodeId& nodeId)
{
    UaNode* pNode = findNode(nodeId);
    UaVariable* pVariable = NULL;

    if (!pNode)
    {
        return;
    }

    if ( pNode && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        pVariable = (UaVariable*)pNode;
    }
    else
    {
        return;
    }

    // prepare sample data
    WorkOrderStatusType wostTemp;
    WorkOrderStatusTypes wostArray;
    wostArray.resize(3);
    wostTemp.setWorkOrderStatusType("Wendy Terry", UaDateTime::now(), UaLocalizedText("en-US", "Mission accomplished!"));
    wostTemp.copyTo(&wostArray[0]);
    wostTemp.setWorkOrderStatusType("Gavin Mackenzie", UaDateTime::now(), UaLocalizedText("en-US", "I think clients would love this."));
    wostTemp.copyTo(&wostArray[1]);
    wostTemp.setWorkOrderStatusType("Phil Taylor", UaDateTime::now(), UaLocalizedText("en-US", "And justice for all."));
    wostTemp.copyTo(&wostArray[2]);

    WorkOrderType valWorkOrder;
    valWorkOrder.setID(UaGuid::create());
    valWorkOrder.setAssetID("123-X-Y-Z");
    valWorkOrder.setStartTime(UaDateTime::now());
    valWorkOrder.setStatusComments(wostArray);

    // WorkOrderVariableType
    if (pNode->typeDefinitionId() == UaNodeId(DemoId_WorkOrderVariableType, getNameSpaceIndex()))
    {
        UaVariant value;
        OpcUa::BaseDataVariableType* pChildNode = NULL;
        WorkOrderVariableType* pWorkOrderVar = (WorkOrderVariableType*)pNode;

        // we save the value in a shared object instead of the variable
        DemoSharedData* pSharedData = new DemoSharedData();
        valWorkOrder.toVariant(value);
        pSharedData->setValue(value);
        pSharedData->setSourceTimestamp(UaDateTime::now());
        UserDataWorkOrder* pUserData = new UserDataWorkOrder(UserDataWorkOrder::CompleteStructure, pSharedData);
        pWorkOrderVar->setUserData(pUserData);
        pSharedData->releaseReference();

        // change value handling because we want to apply written values to the structure or child nodes
        pWorkOrderVar->setValueHandling(UaVariable_Value_Cache);
        // change value handling and update values for children
        pChildNode = pWorkOrderVar->getAssetIDNode();
        if (pChildNode)
        {
            pChildNode->setValueHandling(UaVariable_Value_Cache);
            pChildNode->setUserData(new UserDataWorkOrder(UserDataWorkOrder::AssetID, pSharedData));
         }
         pChildNode = pWorkOrderVar->getIDNode();
         if (pChildNode)
         {
             pChildNode->setValueHandling(UaVariable_Value_Cache);
             pChildNode->setUserData(new UserDataWorkOrder(UserDataWorkOrder::ID, pSharedData));
         }
         pChildNode = pWorkOrderVar->getStartTimeNode();
         if (pChildNode)
         {
             pChildNode->setValueHandling(UaVariable_Value_Cache);
             pChildNode->setUserData(new UserDataWorkOrder(UserDataWorkOrder::StartTime, pSharedData));
         }
         pChildNode = pWorkOrderVar->getStatusCommentsNode();
         if (pChildNode)
         {
             pChildNode->setValueHandling(UaVariable_Value_Cache);
             pChildNode->setUserData(new UserDataWorkOrder(UserDataWorkOrder::StatusComments, pSharedData));
         }
    }
    // WorkOrder DataType
    else if (pVariable->dataType() == UaNodeId(DemoId_WorkOrderType, getNameSpaceIndex()))
    {
        UaVariant value;
        UaDataValue dataValue;
        int count = 10;

        if (pVariable->valueRank() == OpcUa_ValueRanks_Scalar)
        {
            valWorkOrder.toVariant(value);
            dataValue.setValue(value, OpcUa_True, OpcUa_True);
            pVariable->setValue(NULL, dataValue, OpcUa_False);
        }
        else if (pVariable->valueRank() == OpcUa_ValueRanks_OneDimension)
        {
            WorkOrderTypes workOrderArray;
            workOrderArray.create(count);
            OpcUa_Int32 i = 0;
            for ( i=0; i<count; i++ )
            {
                valWorkOrder.setID(UaGuid::create());
                valWorkOrder.setAssetID(UaString("%1%2%3-X-Y-Z").arg((int)i+1).arg((int)i+2).arg((int)i+3));
                valWorkOrder.copyTo(&workOrderArray[i]);
            }
            workOrderArray.toVariant(value);
            dataValue.setValue(value, OpcUa_True, OpcUa_True);
            pVariable->setValue(NULL, dataValue, OpcUa_False);
        }
    }
}

void NodeManagerDemo::createSpecialNodes()
{
    OpcUa::BaseDataVariableType* pVariable;
    UaStatus addResult;
    UaString sName;
    UaVariant value;
    OpcUa_UInt32 i;

    UaNode* pStatic = findNode(UaNodeId(DemoId_Demo_Static, getNameSpaceIndex()));
    UaNode* pDynamic = findNode(UaNodeId(DemoId_Demo_Dynamic, getNameSpaceIndex()));

    // Add variables with all NodeId types
    if ( pStatic && pDynamic)
    {
        value.setUInt32(0);

        // Variable with numeric NodeId
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId(OpcUa_UInt32_Max-1, getTypeNamespace()),
            "VariableWithNumericNodeId",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pStatic->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId(OpcUa_UInt32_Max, getTypeNamespace()),
            "VariableWithNumericNodeId",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentRead,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pDynamic->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            // Add a reference since we store the variable for simulation
            pVariable->addReference();
            m_massVariablesDynamic.push_back(pVariable);
        }

        // Variable with Guid NodeId
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId(UaGuid::create(), getTypeNamespace()),
            "VariableWithGuidNodeId",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pStatic->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId(UaGuid::create(), getTypeNamespace()),
            "VariableWithGuidNodeId",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentRead,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pDynamic->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            // Add a reference since we store the variable for simulation
            pVariable->addReference();
            m_massVariablesDynamic.push_back(pVariable);
        }

        // Variable with ByteString NodeId
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId(UaByteString(4, (OpcUa_Byte*)"abcd"), getTypeNamespace()),
            "VariableWithByteStringNodeId",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pStatic->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId(UaByteString(4, (OpcUa_Byte*)"efgh"), getTypeNamespace()),
            "VariableWithByteStringNodeId",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentRead,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pDynamic->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            // Add a reference since we store the variable for simulation
            pVariable->addReference();
            m_massVariablesDynamic.push_back(pVariable);
        }
    }

    // Add variables with matrix for all data types
    if ( pStatic )
    {
        UaString sNodeIdFolder("Demo.Static.Matrix");
        OpcUa::FolderType* pFolder = new OpcUa::FolderType(UaNodeId(sNodeIdFolder, getTypeNamespace()), "Matrix", getTypeNamespace(), this);
        addResult = addNodeAndReference(pStatic->getUaReferenceLists(), pFolder, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pFolder->deleteAllChildren(this);
            pFolder->releaseReference();
            return;
        }

        UaString sIdentifier;
        UaInt32Array tmpDimensions;
        tmpDimensions.create(3);
        tmpDimensions[0] = 2;
        tmpDimensions[1] = 4;
        tmpDimensions[2] = 3;

        sName = "Boolean";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaBoolArray tmpBoolArr;
        tmpBoolArr.create(24);
        for (i = 0; i < tmpBoolArr.length(); i++) {tmpBoolArr[i] = (i%2) ? OpcUa_True : OpcUa_False;}
        value.setBoolMatrix(tmpBoolArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "SByte";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaSByteArray tmpSByteArr;
        tmpSByteArr.create(24);
        for ( i = 0; i < tmpSByteArr.length(); i++) {tmpSByteArr[i] = (OpcUa_SByte)i;}
        value.setSByteMatrix(tmpSByteArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "Int16";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaInt16Array tmpInt16Arr;
        tmpInt16Arr.create(24);
        for ( i = 0; i < tmpInt16Arr.length(); i++) {tmpInt16Arr[i] = (OpcUa_Int16)i;}
        value.setInt16Matrix(tmpInt16Arr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "UInt16";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaUInt16Array tmpUInt16Arr;
        tmpUInt16Arr.create(24);
        for ( i = 0; i < tmpUInt16Arr.length(); i++) {tmpUInt16Arr[i] = (OpcUa_UInt16)i;}
        value.setUInt16Matrix(tmpUInt16Arr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "Int32";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaInt32Array tmpInt32Arr;
        tmpInt32Arr.create(24);
        for ( i = 0; i < tmpInt32Arr.length(); i++) {tmpInt32Arr[i] = i;}
        value.setInt32Matrix(tmpInt32Arr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "UInt32";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaUInt32Array tmpUInt32Arr;
        tmpUInt32Arr.create(24);
        for ( i = 0; i < tmpUInt32Arr.length(); i++) {tmpUInt32Arr[i] = i;}
        value.setUInt32Matrix(tmpUInt32Arr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "Int64";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaInt64Array tmpInt64Arr;
        tmpInt64Arr.create(24);
        for ( i = 0; i < tmpInt64Arr.length(); i++) {tmpInt64Arr[i] = i;}
        value.setInt64Matrix(tmpInt64Arr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "UInt64";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaUInt64Array tmpUInt64Arr;
        tmpUInt64Arr.create(24);
        for ( i = 0; i < tmpUInt64Arr.length(); i++) {tmpUInt64Arr[i] = i;}
        value.setUInt64Matrix(tmpUInt64Arr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "Float";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaFloatArray tmpFloatArr;
        tmpFloatArr.create(24);
        for ( i = 0; i < tmpFloatArr.length(); i++) {tmpFloatArr[i] = (OpcUa_Float)i*(OpcUa_Float)3.14159265;}
        value.setFloatMatrix(tmpFloatArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "Double";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaDoubleArray tmpDoubleArr;
        tmpDoubleArr.create(24);
        for ( i = 0; i < tmpDoubleArr.length(); i++) {tmpDoubleArr[i] = (OpcUa_Double)i*3.14159265;}
        value.setDoubleMatrix(tmpDoubleArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "String";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaStringArray tmpStringArr;
        tmpStringArr.create(24);
        for ( i = 0; i < tmpStringArr.length(); i++) {UaString("String %1").arg((unsigned int)i).copyTo(&tmpStringArr[i]);}
        value.setStringMatrix(tmpStringArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "DateTime";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaDateTimeArray tmpDateTimeArr;
        tmpDateTimeArr.create(24);
        for ( i = 0; i < tmpDateTimeArr.length(); i++) {tmpDateTimeArr[i] = UaDateTime::now();}
        value.setDateTimeMatrix(tmpDateTimeArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "Guid";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaGuidArray tmpGuidArr;
        tmpGuidArr.create(24);
        for ( i = 0; i < tmpGuidArr.length(); i++) {UaGuid::create().copyTo(&tmpGuidArr[i]);}
        value.setGuidMatrix(tmpGuidArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "NodeId";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaNodeIdArray tmpNodeIdArr;
        tmpNodeIdArr.create(24);
        for ( i = 0; i < tmpNodeIdArr.length(); i++)
        {
            UaNodeId tempNodeId(UaString("%1").arg((unsigned int)i), (OpcUa_UInt16)i);
            tempNodeId.copyTo(&tmpNodeIdArr[i]);
        }
        value.setNodeIdMatrix(tmpNodeIdArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "ExpandedNodeId";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaExpandedNodeIdArray tmpExpandedNodeIdArr;
        tmpExpandedNodeIdArr.create(24);
        for ( i = 0; i < tmpExpandedNodeIdArr.length(); i++)
        {
            UaExpandedNodeId tempNodeId(UaNodeId("ExpandedNodeId", (OpcUa_UInt16)1), UaString(""), 0);
            tempNodeId.copyTo(&tmpExpandedNodeIdArr[i]);
        }
        value.setExpandedNodeIdMatrix(tmpExpandedNodeIdArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "StatusCode";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaStatusCodeArray tmpStatusCodeArr;
        tmpStatusCodeArr.create(24);
        for ( i = 0; i < tmpStatusCodeArr.length(); i++)
        {
            tmpStatusCodeArr[i] = i;
        }
        value.setStatusCodeMatrix(tmpStatusCodeArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "QualifiedName";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaQualifiedNameArray tmpQualifiedNameArr;
        tmpQualifiedNameArr.create(24);
        for ( i = 0; i < tmpQualifiedNameArr.length(); i++)
        {
            UaQualifiedName tempQName(UaString("QualifiedName %1").arg((unsigned int)i), (OpcUa_UInt16)i);
            tempQName.copyTo(&tmpQualifiedNameArr[i]);
        }
        value.setQualifiedNameMatrix(tmpQualifiedNameArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);

        sName = "LocalizedText";
        sIdentifier = UaString("%1.%2").arg(sNodeIdFolder).arg(sName);
        UaLocalizedTextArray tmpLocalizedTextArr;
        tmpLocalizedTextArr.create(24);
        for ( i = 0; i < tmpLocalizedTextArr.length(); i++)
        {
            UaLocalizedText tempLText("de", UaString("LocalizedText %1").arg((unsigned int)i));
            tempLText.copyTo(&tmpLocalizedTextArr[i]);
        }
        value.setLocalizedTextMatrix(tmpLocalizedTextArr, tmpDimensions, OpcUa_True);
        pVariable = new OpcUa::BaseDataVariableType(UaNodeId(sIdentifier, getTypeNamespace()), sName, getNameSpaceIndex(), value, 3, this, m_pSharedNodeMutex);
        addNodeAndReference(pFolder, pVariable, OpcUaId_Organizes);
    }

    // Add variables with structure datatype
    UaNode* pStructures = findNode(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getNameSpaceIndex()));
    if (pStructures)
    {
        UaGenericStructureValue person(structureDefinition(UaNodeId(Demo_PersonStruct, getNameSpaceIndex())));
        value.setString("Fat Boy");
        person.setField("Name", value, OpcUa_True);
        value.setUInt16(171);
        person.setField("Height", value, OpcUa_True);
        value.setFloat(132.6f);
        person.setField("Weight", value, OpcUa_True);

        // Variable with person structure
        person.toVariant(value);
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId("Person1", getTypeNamespace()),
            "Person1",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pStructures->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            pVariable->setDataType(UaNodeId(Demo_PersonStruct, getNameSpaceIndex()));
        }

        value.setString("Long John");
        person.setField("Name", value, OpcUa_True);
        value.setUInt16(213);
        person.setField("Height", value, OpcUa_True);
        value.setFloat(82.1f);
        person.setField("Weight", value, OpcUa_True);

        // Variable with person structure
        person.toVariant(value);
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId("Person2", getTypeNamespace()),
            "Person2",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pStructures->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            pVariable->setDataType(UaNodeId(Demo_PersonStruct, getNameSpaceIndex()));
        }

        UaGenericUnionValue analogMeasurement(structureDefinition(UaNodeId(Demo_Measurement, getNameSpaceIndex())));
        value.setFloat(23.4f);
        analogMeasurement.setValue("Temperature", value, OpcUa_True);

        // Variable with person structure
        analogMeasurement.toVariant(value);
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId("Demo.Static.Scalar.Structures.AnalogMeasurement", getTypeNamespace()),
            "AnalogMeasurement",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pStructures->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            pVariable->setDataType(UaNodeId(Demo_Measurement, getNameSpaceIndex()));
        }

        UaGenericStructureValue StatusSymbols(structureDefinition(UaNodeId(Demo_StatusSymbols, getNameSpaceIndex())));
        value.setString("Blacksmith");
        StatusSymbols.setField("Job", value, OpcUa_True);
        value.setString("flat");
        StatusSymbols.setField("House", value, OpcUa_True);
        value.setString("to slow");
        StatusSymbols.setField("Car", value, OpcUa_True);

        // Variable with person structure
        StatusSymbols.toVariant(value);

        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId("Demo.Static.Scalar.Structures.StatusSymbols", getTypeNamespace()),
            "StatusSymbols",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pStructures->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            pVariable->setDataType(UaNodeId(Demo_StatusSymbols, getNameSpaceIndex()));
        }

        //variable with CarExtras
        UaGenericOptionSetValue carExtras(optionSetDefinition(UaNodeId(Demo_CarExtras, getNameSpaceIndex())));
        carExtras.setValue("air conditioner", false);
        carExtras.setValue("super charged", true);
        carExtras.setValue("launch control", true);
        carExtras.setValue("navigation system", false);
        carExtras.toVariant(value);

        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId("Demo.Static.Scalar.CarExtras", getTypeNamespace()),
            "CarExtras",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);

        // change value handling because we want to check values before writing
        pVariable->setValueHandling(UaVariable_Value_CacheIsSource | UaVariable_Value_CacheIsUpdatedOnRequest);
        addResult = addNodeAndReference(pStructures->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            pVariable->setDataType(UaNodeId(Demo_CarExtras, getNameSpaceIndex()));
        }
     }

     UaNode* pArrays = findNode(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()));
     if (pArrays)
     {
        //create array of unions
        UaGenericUnionArray analogMeasurementArray(structureDefinition(UaNodeId(Demo_Measurement, getNameSpaceIndex())));
        analogMeasurementArray.create(3);
        value.setFloat(37.5f);
        analogMeasurementArray[0].setValue("Temperature", value, OpcUa_True);
        value.setInt32(0x6C00);
        analogMeasurementArray[1].setValue("Voltage", value, OpcUa_True);
        value.setUInt16(0x7FFF);
        analogMeasurementArray[2].setValue("Preassure", value, OpcUa_True);

        // Variable with person structure
        analogMeasurementArray.toVariant(value);
        pVariable = new OpcUa::BaseDataVariableType(
            UaNodeId("Demo.Static.Arrays.AnalogMeasurement", getTypeNamespace()),
            "AnalogMeasurement",
            getNameSpaceIndex(),
            value,
            OpcUa_AccessLevels_CurrentReadOrWrite,
            this,
            m_pSharedNodeMutex);
        addResult = addNodeAndReference(pArrays->getUaReferenceLists(), pVariable, OpcUaId_Organizes);
        if ( addResult.isBad() )
        {
            pVariable->deleteAllChildren(this);
            pVariable->releaseReference();
        }
        else
        {
            pVariable->setDataType(UaNodeId(Demo_Measurement, getNameSpaceIndex()));
        }
    }
}

void NodeManagerDemo::createAlarmNodes()
{
#if SUPPORT_Event_Subscription_Server_Facet
    UaMutexRefCounted* pSharedMutex = new UaMutexRefCounted;
    OpcUa::BaseDataVariableType* pTrigger = NULL;
    OpcUa::AnalogItemType* pTriggerAnalog = NULL;
    UserDataDemoAlarm* pUserDataAlarm = NULL;
    UaVariant initialValue;

    //************************************************************************
    // Event notifier
    //************************************************************************
    OpcUa::BaseObjectType* pAlarmsWithNodes = new OpcUa::BaseObjectType(
        UaNodeId("AlarmsWithNodes", getNameSpaceIndex()),
        "AlarmsWithNodes",
        getNameSpaceIndex(),
        this,
        pSharedMutex);
    addNodeAndReference(UaNodeId(DemoId_Demo_Events, getNameSpaceIndex()), pAlarmsWithNodes, OpcUaId_Organizes);
    pAlarmsWithNodes->setEventNotifier(OpcUa_EventNotifiers_SubscribeToEvents);

    registerEventNotifier(OpcUaId_Server, pAlarmsWithNodes->nodeId());
    addUaReference(OpcUaId_Server, pAlarmsWithNodes->nodeId(), OpcUaId_HasNotifier);

    OpcUa::BaseObjectType* pAlarmsNoNodes = new OpcUa::BaseObjectType(
        UaNodeId("AlarmsNoNodes", getNameSpaceIndex()),
        "AlarmsNoNodes",
        getNameSpaceIndex(),
        this,
        pSharedMutex);
    addNodeAndReference(UaNodeId(DemoId_Demo_Events, getNameSpaceIndex()), pAlarmsNoNodes, OpcUaId_Organizes);
    pAlarmsNoNodes->setEventNotifier(OpcUa_EventNotifiers_SubscribeToEvents);

    registerEventNotifier(OpcUaId_Server, pAlarmsNoNodes->nodeId());
    addUaReference(OpcUaId_Server, pAlarmsNoNodes->nodeId(), OpcUaId_HasNotifier);


    //************************************************************************
    // OffNormalAlarm with nodes + Trigger
    //************************************************************************
    initialValue.setBool(OpcUa_False);
    pTrigger = new OpcUa::BaseDataVariableType(
        UaNodeId("AlarmsWithNodes.OffNormalAlarmTrigger", getNameSpaceIndex()),
        "OffNormalAlarmTrigger",
        getNameSpaceIndex(),
        initialValue,
        OpcUa_AccessLevels_CurrentReadOrWrite,
        this,
        pSharedMutex);
    addNodeAndReference(pAlarmsWithNodes, pTrigger, OpcUaId_HasComponent);
    registerEventSource(pAlarmsWithNodes->nodeId(), pTrigger->nodeId());

    m_pUaOffNormalAlarm = new OpcUa::OffNormalAlarmType(
        UaNodeId("AlarmsWithNodes.OffNormalAlarm", getNameSpaceIndex()),
        "OffNormalAlarm",
        getNameSpaceIndex(),
        this,
        pTrigger->nodeId(),
        pTrigger->browseName().toString(),
        pSharedMutex);
    addNodeAndReference(pAlarmsWithNodes, m_pUaOffNormalAlarm, OpcUaId_HasComponent);
    addUaReference(pAlarmsWithNodes, m_pUaOffNormalAlarm, OpcUaId_HasCondition);
    m_pUaOffNormalAlarm->setConditionName("OffNormalAlarm");
    m_pUaOffNormalAlarm->setInputNode(pTrigger->nodeId());
    m_pUaOffNormalAlarm->setAckedState(OpcUa_True);
    m_pUaOffNormalAlarm->setActiveState(OpcUa_False);
    m_pUaOffNormalAlarm->setEnabledState(OpcUa_True);

    // add user data to the trigger
    // we access this user data to do alarm processing when the trigger variable changes its value
    pUserDataAlarm = new UserDataDemoAlarmOffNormalNode(m_pUaOffNormalAlarm);
    pTrigger->setUserData(pUserDataAlarm);
    pUserDataAlarm = NULL;
    //************************************************************************

    //************************************************************************
    // No nodes OffNormalAlarm + Trigger
    //************************************************************************
    initialValue.setBool(OpcUa_False);
    pTrigger = new OpcUa::BaseDataVariableType(
        UaNodeId("AlarmsNoNodes.OffNormalAlarmTrigger", getNameSpaceIndex()),
        "NoNodesOffNormalAlarmTrigger",
        getNameSpaceIndex(),
        initialValue,
        OpcUa_AccessLevels_CurrentReadOrWrite,
        this,
        pSharedMutex);
    addNodeAndReference(pAlarmsNoNodes, pTrigger, OpcUaId_HasComponent);
    registerEventSource(pAlarmsNoNodes->nodeId(), pTrigger->nodeId());

    m_pUaOffNormalAlarmData = new OpcUa::OffNormalAlarmTypeData;
    m_pUaOffNormalAlarmData->setSource(pTrigger);
    m_pUaOffNormalAlarmData->setConditionNodeId(UaNodeId("AlarmsNoNodes.OffNormalAlarm", getNameSpaceIndex()));
    m_pUaOffNormalAlarmData->setConditionName("NoNodesOffNormalAlarm");
    m_pUaOffNormalAlarmData->setConditionClassId(OpcUaId_ProcessConditionClassType);
    m_pUaOffNormalAlarmData->setConditionClassName(UaLocalizedText("", "ProcessConditionClassType"));
    m_pUaOffNormalAlarmData->setClientUserId("");
    m_pUaOffNormalAlarmData->setInputNode(pTrigger->nodeId());
    m_pUaOffNormalAlarmData->setComment(UaLocalizedText("", ""));
    m_pUaOffNormalAlarmData->setBranchId(UaNodeId());
    m_pUaOffNormalAlarmData->setSeverity(500);
    m_pUaOffNormalAlarmData->setAckedState(OpcUa_True);
    m_pUaOffNormalAlarmData->setActiveState(OpcUa_False);
    m_pUaOffNormalAlarmData->setEnabledState(OpcUa_True);

    // add user data to the trigger
    // we access this user data to do alarm processing when the trigger variable changes its value
    pUserDataAlarm = new UserDataDemoAlarmOffNormalNoNode(m_pUaOffNormalAlarmData);
    pTrigger->setUserData(pUserDataAlarm);
    pUserDataAlarm = NULL;
    //************************************************************************

    //************************************************************************
    // NonExclusiveLevelAlarm + Trigger
    //************************************************************************
    initialValue.setDouble(50);
    pTriggerAnalog = new OpcUa::AnalogItemType(
        UaNodeId("AlarmsWithNodes.NonExclusiveLevelAlarmTrigger", getNameSpaceIndex()),
        "NonExclusiveLevelAlarmTrigger",
        getNameSpaceIndex(),
        initialValue,
        OpcUa_AccessLevels_CurrentReadOrWrite,
        this,
        pSharedMutex);
    addNodeAndReference(pAlarmsWithNodes, pTriggerAnalog, OpcUaId_HasComponent);
    registerEventSource(pAlarmsWithNodes->nodeId(), pTriggerAnalog->nodeId());
    pTriggerAnalog->setEURange(UaRange(0,100));
    pTriggerAnalog->setEngineeringUnits(UaEUInformation(UaEUInformation::EngineeringUnit_percent));

    m_pUaNonExclusiveLevelAlarm = new OpcUa::NonExclusiveLevelAlarmType(
        UaNodeId("AlarmsWithNodes.NonExclusiveLevelAlarm", getNameSpaceIndex()),
        "NonExclusiveLevelAlarm",
        getNameSpaceIndex(),
        this,
        pTriggerAnalog->nodeId(),
        pTriggerAnalog->browseName().toString(),
        pSharedMutex);
    addNodeAndReference(pAlarmsWithNodes, m_pUaNonExclusiveLevelAlarm, OpcUaId_HasComponent);
    addUaReference(pAlarmsWithNodes, m_pUaNonExclusiveLevelAlarm, OpcUaId_HasCondition);
    m_pUaNonExclusiveLevelAlarm->setConditionName("NonExclusiveLevelAlarm");
    m_pUaNonExclusiveLevelAlarm->setAckedState(OpcUa_True);
    m_pUaNonExclusiveLevelAlarm->setActiveState(OpcUa_False);
    m_pUaNonExclusiveLevelAlarm->setEnabledState(OpcUa_True);
    m_pUaNonExclusiveLevelAlarm->setHighHighLimit(90);
    m_pUaNonExclusiveLevelAlarm->setHighLimit(70);
    m_pUaNonExclusiveLevelAlarm->setLowLimit(30);
    m_pUaNonExclusiveLevelAlarm->setLowLowLimit(10);

    // add user data to the trigger
    // we access this user data to do alarm processing when the trigger variable changes its value
    pUserDataAlarm = new UserDataDemoAlarmLevelNonExNode(m_pUaNonExclusiveLevelAlarm);
    pTriggerAnalog->setUserData(pUserDataAlarm);
    pUserDataAlarm = NULL;
    //************************************************************************

    //************************************************************************
    // No nodes NonExclusiveLevelAlarm + Trigger
    //************************************************************************
    initialValue.setDouble(50);
    pTriggerAnalog = new OpcUa::AnalogItemType(
        UaNodeId("AlarmsNoNodes.NonExclusiveLevelAlarmTrigger", getNameSpaceIndex()),
        "NoNodesNonExclusiveLevelAlarmTrigger",
        getNameSpaceIndex(),
        initialValue,
        OpcUa_AccessLevels_CurrentReadOrWrite,
        this,
        pSharedMutex);
    addNodeAndReference(pAlarmsNoNodes, pTriggerAnalog, OpcUaId_HasComponent);
    registerEventSource(pAlarmsNoNodes->nodeId(), pTriggerAnalog->nodeId());
    pTriggerAnalog->setEURange(UaRange(0,100));
    pTriggerAnalog->setEngineeringUnits(UaEUInformation(UaEUInformation::EngineeringUnit_percent));

    m_pUaNonExclusiveLevelAlarmData = new OpcUa::NonExclusiveLevelAlarmTypeData;
    m_pUaNonExclusiveLevelAlarmData->setSource(pTriggerAnalog);
    m_pUaNonExclusiveLevelAlarmData->setConditionNodeId(UaNodeId("AlarmsNoNodes.NonExclusiveLevelAlarm", getNameSpaceIndex()));
    m_pUaNonExclusiveLevelAlarmData->setConditionName("NoNodesNonExclusiveLevelAlarm");
    m_pUaNonExclusiveLevelAlarmData->setConditionClassId(OpcUaId_ProcessConditionClassType);
    m_pUaNonExclusiveLevelAlarmData->setConditionClassName(UaLocalizedText("", "ProcessConditionClassType"));
    m_pUaNonExclusiveLevelAlarmData->setClientUserId("");
    m_pUaNonExclusiveLevelAlarmData->setInputNode(pTrigger->nodeId());
    m_pUaNonExclusiveLevelAlarmData->setComment(UaLocalizedText("", ""));
    m_pUaNonExclusiveLevelAlarmData->setBranchId(UaNodeId());
    m_pUaNonExclusiveLevelAlarmData->setSeverity(500);
    m_pUaNonExclusiveLevelAlarmData->setAckedState(OpcUa_True);
    m_pUaNonExclusiveLevelAlarmData->setActiveState(OpcUa_False);
    m_pUaNonExclusiveLevelAlarmData->setEnabledState(OpcUa_True);
    m_pUaNonExclusiveLevelAlarmData->setHighHighLimit(90);
    m_pUaNonExclusiveLevelAlarmData->setHighLimit(70);
    m_pUaNonExclusiveLevelAlarmData->setLowLimit(30);
    m_pUaNonExclusiveLevelAlarmData->setLowLowLimit(10);

    // add user data to the trigger
    // we access this user data to do alarm processing when the trigger variable changes its value
    pUserDataAlarm = new UserDataDemoAlarmLevelNonExNoNode(m_pUaNonExclusiveLevelAlarmData);
    pTriggerAnalog->setUserData(pUserDataAlarm);
    pUserDataAlarm = NULL;
    //************************************************************************

    //************************************************************************
    // ExclusiveLevelAlarm + Trigger
    //************************************************************************
    initialValue.setDouble(50);
    pTriggerAnalog = new OpcUa::AnalogItemType(
        UaNodeId("AlarmsWithNodes.ExclusiveLevelAlarmTrigger", getNameSpaceIndex()),
        "ExclusiveLevelAlarmTrigger",
        getNameSpaceIndex(),
        initialValue,
        OpcUa_AccessLevels_CurrentReadOrWrite,
        this,
        pSharedMutex);
    addNodeAndReference(pAlarmsWithNodes, pTriggerAnalog, OpcUaId_HasComponent);
    registerEventSource(pAlarmsWithNodes->nodeId(), pTriggerAnalog->nodeId());
    pTriggerAnalog->setEURange(UaRange(0,100));
    pTriggerAnalog->setEngineeringUnits(UaEUInformation(UaEUInformation::EngineeringUnit_percent));

    m_pUaExclusiveLevelAlarm = new OpcUa::ExclusiveLevelAlarmType(
        UaNodeId("AlarmsWithNodes.ExclusiveLevelAlarm", getNameSpaceIndex()),
        "ExclusiveLevelAlarm",
        getNameSpaceIndex(),
        this,
        pTriggerAnalog->nodeId(),
        pTriggerAnalog->browseName().toString(),
        pSharedMutex);
    addNodeAndReference(pAlarmsWithNodes, m_pUaExclusiveLevelAlarm, OpcUaId_HasComponent);
    addUaReference(pAlarmsWithNodes, m_pUaExclusiveLevelAlarm, OpcUaId_HasCondition);
    m_pUaExclusiveLevelAlarm->setConditionName("ExclusiveLevelAlarm");
    m_pUaExclusiveLevelAlarm->setAckedState(OpcUa_True);
    m_pUaExclusiveLevelAlarm->setActiveState(OpcUa_False);
    m_pUaExclusiveLevelAlarm->setEnabledState(OpcUa_True);
    m_pUaExclusiveLevelAlarm->setHighHighLimit(90);
    m_pUaExclusiveLevelAlarm->setHighLimit(70);
    m_pUaExclusiveLevelAlarm->setLowLimit(30);
    m_pUaExclusiveLevelAlarm->setLowLowLimit(10);
    m_pUaExclusiveLevelAlarm->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Disabled);

    // add user data to the trigger
    // we access this user data to do alarm processing when the trigger variable changes its value
    pUserDataAlarm = new UserDataDemoAlarmLevelExclusiveNode(m_pUaExclusiveLevelAlarm);
    pTriggerAnalog->setUserData(pUserDataAlarm);
    pUserDataAlarm = NULL;
    //************************************************************************

    //************************************************************************
    // No nodes ExclusiveLevelAlarm + Trigger
    //************************************************************************
    initialValue.setDouble(50);
    pTriggerAnalog = new OpcUa::AnalogItemType(
        UaNodeId("AlarmsNoNodes.ExclusiveLevelAlarmTrigger", getNameSpaceIndex()),
        "NoNodesExclusiveLevelAlarmTrigger",
        getNameSpaceIndex(),
        initialValue,
        OpcUa_AccessLevels_CurrentReadOrWrite,
        this,
        pSharedMutex);
    addNodeAndReference(pAlarmsNoNodes, pTriggerAnalog, OpcUaId_HasComponent);
    registerEventSource(pAlarmsNoNodes->nodeId(), pTriggerAnalog->nodeId());
    pTriggerAnalog->setEURange(UaRange(0,100));
    pTriggerAnalog->setEngineeringUnits(UaEUInformation(UaEUInformation::EngineeringUnit_percent));

    m_pUaExclusiveLevelAlarmData = new OpcUa::ExclusiveLevelAlarmTypeData;
    m_pUaExclusiveLevelAlarmData->setSource(pTriggerAnalog);
    m_pUaExclusiveLevelAlarmData->setConditionNodeId(UaNodeId("AlarmsNoNodes.ExclusiveLevelAlarm", getNameSpaceIndex()));
    m_pUaExclusiveLevelAlarmData->setConditionName("NoNodesExclusiveLevelAlarm");
    m_pUaExclusiveLevelAlarmData->setConditionClassId(OpcUaId_ProcessConditionClassType);
    m_pUaExclusiveLevelAlarmData->setConditionClassName(UaLocalizedText("", "ProcessConditionClassType"));
    m_pUaExclusiveLevelAlarmData->setClientUserId("");
    m_pUaExclusiveLevelAlarmData->setInputNode(pTrigger->nodeId());
    m_pUaExclusiveLevelAlarmData->setComment(UaLocalizedText("", ""));
    m_pUaExclusiveLevelAlarmData->setBranchId(UaNodeId());
    m_pUaExclusiveLevelAlarmData->setSeverity(500);
    m_pUaExclusiveLevelAlarmData->setAckedState(OpcUa_True);
    m_pUaExclusiveLevelAlarmData->setActiveState(OpcUa_False);
    m_pUaExclusiveLevelAlarmData->setEnabledState(OpcUa_True);
    m_pUaExclusiveLevelAlarmData->setHighHighLimit(90);
    m_pUaExclusiveLevelAlarmData->setHighLimit(70);
    m_pUaExclusiveLevelAlarmData->setLowLimit(30);
    m_pUaExclusiveLevelAlarmData->setLowLowLimit(10);
    m_pUaExclusiveLevelAlarmData->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Disabled);

    // add user data to the trigger
    // we access this user data to do alarm processing when the trigger variable changes its value
    pUserDataAlarm = new UserDataDemoAlarmLevelExclusiveNoNode(m_pUaExclusiveLevelAlarmData);
    pTriggerAnalog->setUserData(pUserDataAlarm);
    pUserDataAlarm = NULL;
    //************************************************************************

    // set user data for BaseEvent and SampleEvent trigger
    UaNode* pEventTrigger = findNode(UaNodeId(DemoId_Demo_Events_Trigger_BaseEvent, getNameSpaceIndex()));
    UA_ASSERT(pEventTrigger);
    if (pEventTrigger)
    {
        UserDataDemoEvent* pUserDataEvent = new UserDataDemoEvent(UserDataDemoEvent::EventType_Base);
        pEventTrigger->setUserData(pUserDataEvent);
    }
    pEventTrigger = findNode(UaNodeId(DemoId_Demo_Events_Trigger_SampleEvent, getNameSpaceIndex()));
    UA_ASSERT(pEventTrigger);
    if (pEventTrigger)
    {
        UserDataDemoEvent* pUserDataEvent = new UserDataDemoEvent(UserDataDemoEvent::EventType_Sample);
        pEventTrigger->setUserData(pUserDataEvent);
    }

    pSharedMutex->releaseReference();
#endif // SUPPORT_Event_Subscription_Server_Facet
}

#if SUPPORT_Event_Subscription_Server_Facet
void NodeManagerDemo::afterSetAttributeValue(
    Session*           pSession,
    UaNode*            pNode,
    OpcUa_Int32        attributeId,
    const UaDataValue& dataValue)
{
    OpcUa_ReferenceParameter(pSession);
    if ( attributeId != OpcUa_Attributes_Value )
    {
        return;
    }

    if (pNode->nodeClass() != OpcUa_NodeClass_Variable)
    {
        return;
    }
    UaVariable* pVariable = (UaVariable*)pNode;

    if (pVariable->getUserData() == NULL)
    {
        return;
    }
    UserDataDemo* pUserDataDemo = (UserDataDemo*)pVariable->getUserData();

    UaVariant uaValue(*dataValue.value());

    // Handle Alarms
    if (pUserDataDemo->getUserDataDemoType() == UserDataDemo::UserDataDemoType_Alarm)
    {
        UserDataDemoAlarm* pUserDataDemoAlarm = (UserDataDemoAlarm*)pUserDataDemo;

        switch (pUserDataDemoAlarm->getAlarmType())
        {
            // OffNormalAlarm
        case UserDataDemoAlarm::AlarmType_OffNormal:
            {
                UserDataDemoAlarmOffNormal* pUserDataDemoAlarmOffNormal = (UserDataDemoAlarmOffNormal*)pUserDataDemoAlarm;
                OpcUa_Boolean isActive = OpcUa_False;
                uaValue.toBool(isActive);
                pUserDataDemoAlarmOffNormal->setActiveState(isActive, getEventManagerUaNode());
            }
            break;
            // NonExclusiveLevelAlarm
        case UserDataDemoAlarm::AlarmType_NonExclusiveLevel:
            {
                UserDataDemoAlarmLevelNonEx* pUserDataDemoAlarmNonExLevel = (UserDataDemoAlarmLevelNonEx*)pUserDataDemoAlarm;
                OpcUa_Double level;
                uaValue.toDouble(level);
                pUserDataDemoAlarmNonExLevel->setLevel(level, getEventManagerUaNode());
            }
            break;
            // ExclusiveLevelAlarm
        case UserDataDemoAlarm::AlarmType_ExclusiveLevel:
            {
                UserDataDemoAlarmLevelExclusive* pUserDataDemoAlarmLevelExclusive = (UserDataDemoAlarmLevelExclusive*)pUserDataDemoAlarm;
                OpcUa_Double level;
                uaValue.toDouble(level);
                pUserDataDemoAlarmLevelExclusive->setLevel(level, getEventManagerUaNode());
            }
            break;
        default:
            break;
        }
    }
    // Handle Events
    else if (pUserDataDemo->getUserDataDemoType() == UserDataDemo::UserDataDemoType_Event)
    {
        UserDataDemoEvent* pUserDataDemoEvent = (UserDataDemoEvent*)pUserDataDemo;
        pUserDataDemoEvent->fireEvent(pNode, getEventManagerUaNode());
    }
    else
    {
        // Nothing to do. We handle the SharedData in writeValues().
    }
}
#endif // SUPPORT_Event_Subscription_Server_Facet

UaStatus NodeManagerDemo::readValues(
    const UaVariableArray& arrUaVariables,
    UaDataValueArray&      arrDataValues)
{
    OpcUa_UInt32         count = arrUaVariables.length();

    arrDataValues.create(count);
    for (OpcUa_UInt32 i=0; i<count; i++)
    {
        UaVariable* pVariable = arrUaVariables[i];
        if (pVariable)
        {
            if (pVariable->getUserData())
            {
                UserDataDemo* pUserData = (UserDataDemo*)pVariable->getUserData();

                // handle WorkOrder
                if (pUserData->getUserDataDemoType() == UserDataDemo::UserDataDemoType_Shared && ((UserDataDemoShared*)pUserData)->sharedDataId() == SharedDataWorkOrder)
                {
                    UserDataWorkOrder* pUserDataWorkOrder = (UserDataWorkOrder*)pUserData;
                    if (pUserDataWorkOrder->pSharedData())
                    {
                        UaVariant value = pUserDataWorkOrder->pSharedData()->getValue();
                        UaExtensionObject extensionObject;
                        value.toExtensionObject(extensionObject);
                        WorkOrderType workOrder(extensionObject, OpcUa_True);

                        switch (pUserDataWorkOrder->getField())
                        {
                        case UserDataWorkOrder::AssetID:
                            value.setString(workOrder.getAssetID());
                            break;
                        case UserDataWorkOrder::ID:
                            value.setGuid(workOrder.getID());
                            break;
                        case UserDataWorkOrder::StartTime:
                            value.setDateTime(workOrder.getStartTime());
                            break;
                        case UserDataWorkOrder::StatusComments:
                            {
                                WorkOrderStatusTypes statusComments;
                                workOrder.getStatusComments(statusComments);
                                statusComments.toVariant(value, OpcUa_True);
                            }
                            break;
                        default:
                            break;
                        }
                        arrDataValues[i].setDataValue(value, OpcUa_False, OpcUa_Good, pUserDataWorkOrder->pSharedData()->getSourceTimestamp(), UaDateTime::now());
                    }
                    else
                    {
                        arrDataValues[i].clear();
                        arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
                    }
                }
            }
            else
            {
                UaVariant value(*pVariable->value(NULL).value());
                arrDataValues[i].setValue(value, OpcUa_False);
                arrDataValues[i].setStatusCode(OpcUa_Good);
                arrDataValues[i].setSourceTimestamp(pVariable->value(NULL).sourceTimestamp());
            }
        }
        else
        {
            arrDataValues[i].clear();
            arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
        }

        arrDataValues[i].setServerTimestamp(UaDateTime::now());
    }
    return OpcUa_Good;
}

UaStatus NodeManagerDemo::writeValues(
    const UaVariableArray& arrUaVariables,
    const PDataValueArray& arrpDataValues,
    UaStatusCodeArray&     arrStatusCodes)
{
    UaStatus ret;
    OpcUa_UInt32 count = arrUaVariables.length();

    arrStatusCodes.create(count);

    for (OpcUa_UInt32 i=0; i<count; i++ )
    {
        UaVariable* pVariable = arrUaVariables[i];
        if (pVariable)
        {
            // user data is set
            if (pVariable->getUserData())
            {
                UserDataDemo* pUserData = (UserDataDemo*)pVariable->getUserData();

                // DemoShared
                if (pUserData->getUserDataDemoType() == UserDataDemo::UserDataDemoType_Shared)
                {
                    UserDataDemoShared* pUserDataDemoShared = (UserDataDemoShared*)pUserData;

                    // Handle WorkOrder
                    if (pUserDataDemoShared->sharedDataId() == SharedDataWorkOrder)
                    {
                        UserDataWorkOrder* pUserDataWorkOrder = (UserDataWorkOrder*)pUserData;

                        // Update the UaVariant value in sharedData
                        if (pUserDataWorkOrder->pSharedData())
                        {
                            UaVariant oldValue;
                            UaVariant newValue;
                            oldValue = pUserDataWorkOrder->pSharedData()->getValue();
                            UaExtensionObject extensionObject;
                            oldValue.toExtensionObject(extensionObject);
                            WorkOrderType workOrder(extensionObject, OpcUa_True);
                            newValue = UaVariant(arrpDataValues[i]->Value);

                            switch (pUserDataWorkOrder->getField())
                            {
                            case UserDataWorkOrder::AssetID:
                                if (newValue.dataType() == UaNodeId(OpcUaId_String) && newValue.arrayType() == OpcUa_VariantArrayType_Scalar)
                                {
                                    workOrder.setAssetID(newValue.toString());
                                }
                                else
                                {
                                    arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                                }
                                break;
                            case UserDataWorkOrder::ID:
                                if (newValue.dataType() == UaNodeId(OpcUaId_Guid) && newValue.arrayType() == OpcUa_VariantArrayType_Scalar)
                                {
                                    UaGuid guid;
                                    newValue.toGuid(guid);
                                    workOrder.setID(guid);
                                }
                                else
                                {
                                    arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                                }
                                break;
                            case UserDataWorkOrder::StartTime:
                                if (newValue.dataType() == UaNodeId(OpcUaId_DateTime) && newValue.arrayType() == OpcUa_VariantArrayType_Scalar)
                                {
                                    UaDateTime startTime;
                                    newValue.toDateTime(startTime);
                                    workOrder.setStartTime(startTime);
                                }
                                else
                                {
                                    arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                                }
                                break;
                            case UserDataWorkOrder::StatusComments:
                                if (newValue.dataType() == UaNodeId(DemoId_WorkOrderStatusType, getNameSpaceIndex()) && newValue.arrayType() == OpcUa_VariantArrayType_Array)
                                {
                                    WorkOrderStatusTypes statusComments;
                                    statusComments.setWorkOrderStatusTypes(newValue, OpcUa_True);
                                    workOrder.setStatusComments(statusComments);
                                }
                                else
                                {
                                    arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                                }
                                break;
                            case UserDataWorkOrder::CompleteStructure:
                                if (newValue.dataType() == UaNodeId(DemoId_WorkOrderType, getNameSpaceIndex()))
                                {
                                    newValue.toExtensionObject(extensionObject);
                                    workOrder.setWorkOrderType(extensionObject);
                                }
                                else
                                {
                                    arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                                }
                                break;
                            default:
                                arrStatusCodes[i] = OpcUa_BadInternalError;
                                break;
                            }

                            workOrder.toVariant(oldValue, OpcUa_True);
                            pUserDataWorkOrder->pSharedData()->setValue(oldValue);
                            pUserDataWorkOrder->pSharedData()->setSourceTimestamp(UaDateTime::now());
                        }
                        else
                        {
                            arrStatusCodes[i] = OpcUa_BadInternalError;
                        }
                    }
                    // can add code for other types here
                    else
                    {}
                }
                // Handle Alarms - we added special handling for alarm in the afterSetAttributeValue() method
                else if (pUserData->getUserDataDemoType() == UserDataDemo::UserDataDemoType_Alarm)
                {
                    // nothing to do here
                }
                else
                {
                    arrStatusCodes[i] = OpcUa_BadInternalError;
                }
            }
            // check variable type and datatype
            else
            {
                //****************************************************************************************
                //  Special handling for particular VariableTypes
                //****************************************************************************************
                // check valid values for MultiStateDiscreteType
                if (pVariable->typeDefinitionId() == UaNodeId(OpcUaId_MultiStateDiscreteType))
                {
                    OpcUa::MultiStateDiscreteType* pMultiStateVar = (OpcUa::MultiStateDiscreteType*)pVariable;
                    UaLocalizedTextArray enumStrings;
                    pMultiStateVar->getEnumStrings(enumStrings);
                    UaVariant varTmp(arrpDataValues[i]->Value);

                    // check for correct datatype
                    if (pVariable->dataType() == varTmp.dataType())
                    {
                        OpcUa_UInt64 newValue;
                        if (OpcUa_IsGood(varTmp.toUInt64(newValue)))
                        {
                            // Check for valid range
                            if (newValue < enumStrings.length())
                            {
                                UaDataValue dataValue(*arrpDataValues[i]);
                                dataValue.setSourceTimestamp(UaDateTime::now());
                                pVariable->setValue(NULL, dataValue);
                            }
                            else
                            {
                                arrStatusCodes[i] = OpcUa_BadOutOfRange;
                            }
                        }
                        else
                        {
                            arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                        }
                    }
                    else
                    {
                        arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                    }
                }
                // check valid values for MultiStateValueDiscreteType
                else if (pVariable->typeDefinitionId() == UaNodeId(OpcUaId_MultiStateValueDiscreteType))
                {
                    OpcUa::MultiStateValueDiscreteType* pMultiStateVar = (OpcUa::MultiStateValueDiscreteType*)pVariable;
                    UaEnumValueTypes enumValues;
                    pMultiStateVar->getEnumValues(enumValues);
                    UaVariant varTmp(arrpDataValues[i]->Value);

                    // check for correct datatype
                    if (pVariable->dataType() == varTmp.dataType())
                    {
                        OpcUa_Int64 newValue;
                        if (OpcUa_IsGood(varTmp.toInt64(newValue)))
                        {
                            // Check for valid value
                            bool bFound = false;
                            UaLocalizedText valueAsText;
                            for (OpcUa_UInt32 j=0; j<enumValues.length(); j++)
                            {
                                if (newValue == enumValues[j].Value)
                                {
                                    bFound = true;
                                    valueAsText = enumValues[j].DisplayName;
                                    break;
                                }
                            }
                            if (bFound)
                            {
                                UaDataValue dataValue(*arrpDataValues[i]);
                                dataValue.setSourceTimestamp(UaDateTime::now());
                                pVariable->setValue(NULL, dataValue);
                                pMultiStateVar->setValueAsText(valueAsText);
                            }
                            else
                            {
                                arrStatusCodes[i] = OpcUa_BadOutOfRange;
                            }
                        }
                        else
                        {
                            arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                        }
                    }
                    else
                    {
                        arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                    }
                }

                //****************************************************************************************
                //  Special handling for particular DataTypes
                //****************************************************************************************
                // special handling for car extras
                else if (pVariable->dataType() == UaNodeId(Demo_CarExtras, getNameSpaceIndex()))
                {
                    UaOptionSetDefinition carExtrasDefinition = optionSetDefinition(UaNodeId(Demo_CarExtras, getNameSpaceIndex()));
                    UaGenericOptionSetValue carExtrasIn, carExtrasResult;

                    carExtrasIn.setOptionSetValue(arrpDataValues[i]->Value, carExtrasDefinition);
                    carExtrasResult.setOptionSetValue(*pVariable->value(NULL).value(), carExtrasDefinition);

                    //set all valid fields to new value
                    int childrenCount = carExtrasDefinition.childrenCount();
                    for (int a = 0; a < childrenCount; a++)
                    {
                        if (carExtrasIn.validBit(a))
                        {
                            carExtrasResult.setValue(a, carExtrasIn.value(a));
                        }
                    }

                    //convert back to UaVariable
                    UaDataValue dataValue;
                    carExtrasResult.toDataValue(dataValue, OpcUa_True);
                    pVariable->setValue(NULL, dataValue);
                }
                // special handling for access rights
                else if (pVariable->dataType() == UaNodeId(DemoId_AccessRights, getNameSpaceIndex()))
                {
                    AccessRights accessRightsResult(*pVariable->value(NULL).value());
                    accessRightsResult.apply(AccessRights(arrpDataValues[i]->Value));

                    //convert back to UaVariable
                    UaDataValue dataValue;
                    accessRightsResult.toDataValue(dataValue, OpcUa_True);
                    pVariable->setValue(NULL, dataValue);
                }
                // just write
                else
                {
                    UaDataValue dataValue(*arrpDataValues[i]);
                    dataValue.setSourceTimestamp(UaDateTime::now());
                    pVariable->setValue(NULL, dataValue);
                }
            }
        }
        else
        {
            arrStatusCodes[i] = OpcUa_BadInternalError;
        }
    }
    return ret;
}
#if SUPPORT_Event_Subscription_Server_Facet
/** Event handlers for AcknowledgeableConditionType method Acknowledge called from UA client
 *  @return Status code.
 */
UaStatus NodeManagerDemo::OnAcknowledge(
    const ServiceContext&  serviceContext, //!< [in] The client service context including the session object used for the call
    OpcUa::AcknowledgeableConditionType* pCondition, //!< [in] The affected acknowledgeable condition object
    const UaByteString&    EventId,        //!< [in] The EventId of the event the acknowledge is related to
    const UaLocalizedText& Comment)        //!< [in] The comment provied by the client
{
    OpcUa_ReferenceParameter(EventId);

    UaByteString userEventId;
    EventManagerBase::getUserEventId(EventId, userEventId);
    if ( userEventId.length() > 0 )
    {
        OpcUa::AcknowledgeableConditionTypeData* pBranch = (OpcUa::AcknowledgeableConditionTypeData*)pCondition->getBranchByEventId(EventId, userEventId);
        if ( pBranch == NULL )
        {
            return OpcUa_BadInvalidState;
        }

        // Lock the event data for change since a refresh may access the data in parallel
        pBranch->lockEventData();
        pBranch->setAckedState(OpcUa_True);
        pBranch->setComment(Comment);
        pBranch->setClientUserId(serviceContext.pSession()->getClientUserId());
        pBranch->setMessage(UaLocalizedText("en", "Branch state acknowledged by UA client"));
        // ToDo Check active state
        pBranch->setRetain(OpcUa_False);
        pBranch->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        // Unlock event data before fire event
        pBranch->unlockEventData();
        fireEvent(pBranch);

        pCondition->deleteBranch(pBranch->getBranchId());
    }
    else
    {
        // Check if the condition is unacknowledged
        if ( pCondition->getAckedStateBool() != OpcUa_False )
        {
            return OpcUa_BadInvalidState;
        }

        // Chance condition to acknowledged
        pCondition->setAckedState(OpcUa_True);
        pCondition->setComment(Comment);
        pCondition->setClientUserId(serviceContext.pSession()->getClientUserId());
        pCondition->setMessage(UaLocalizedText("en", "Condition state acknowledged by UA client"));

        OpcUa::AlarmConditionType* pAlarmCondition = (OpcUa::AlarmConditionType*)pCondition;
        if ( pAlarmCondition->getActiveStateBool() == OpcUa_False )
        {
            pCondition->setRetain(OpcUa_False);
        }

        // Trigger state change event
        pCondition->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
    }

    return OpcUa_Good;
}

/** Event handlers for AcknowledgeableConditionType method Confirm called from UA client
 *  @return Status code.
 */
UaStatus NodeManagerDemo::OnConfirm(
    const ServiceContext&  serviceContext, //!< [in] The client service context including the session object used for the call
    OpcUa::AcknowledgeableConditionType* pCondition, //!< [in] The affected acknowledgeable condition object
    const UaByteString&    EventId,        //!< [in] The EventId of the event the acknowledge is related to
    const UaLocalizedText& Comment)        //!< [in] The comment provied by the client
{
    OpcUa_ReferenceParameter(EventId);

    // Check if the condition is unacknowledged
    if ( pCondition->getConfirmedStateBool() != OpcUa_False )
    {
        return OpcUa_BadInvalidState;
    }

    // Chance condition to acknowledged
    pCondition->setConfirmedState(OpcUa_True);
    pCondition->setComment(Comment);
    pCondition->setClientUserId(serviceContext.pSession()->getClientUserId());
    pCondition->setMessage(UaLocalizedText("en", "Condition state confirmed by UA client"));

    // Trigger state change event
    pCondition->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());

    return OpcUa_Good;
}
#endif // SUPPORT_Event_Subscription_Server_Facet

#if SUPPORT_Method_Server_Facet
MethodHandle* NodeManagerDemo::getMethodHandle(
    Session*       pSession,
    OpcUa_NodeId*  pObjectNodeId,
    OpcUa_NodeId*  pMethodNodeId,
    UaStatus&      result) const
{
    if (pObjectNodeId && pMethodNodeId)
    {
        // We need to handle the methods of the conditions without nodes
        UaNodeId objectId(*pObjectNodeId);
        UaNodeId methodId(*pMethodNodeId);
#if SUPPORT_Event_Subscription_Server_Facet
        if ( (objectId == m_pUaOffNormalAlarmData->getConditionNodeId() )
          || (objectId == m_pUaNonExclusiveLevelAlarmData->getConditionNodeId())
          || (objectId == m_pUaExclusiveLevelAlarmData->getConditionNodeId())
        )
        {
            MethodHandleNodeId *pRet = new MethodHandleNodeId();
            pRet->m_methodNodeId = methodId;
            pRet->m_objectNodeId = objectId;
            pRet->m_pMethodManager = (MethodManager*)this;
            return pRet;
        }
#endif // SUPPORT_Event_Subscription_Server_Facet

        // We need to handle the AddNodes methods here too
        if ( (objectId == UaNodeId("Demo.Massfolder_Static", getNameSpaceIndex()))
          || (objectId == UaNodeId("Demo.Massfolder_Dynamic", getNameSpaceIndex()))
        )
        {
            MethodHandleNodeId *pRet = new MethodHandleNodeId();
            pRet->m_methodNodeId = methodId;
            pRet->m_objectNodeId = objectId;
            pRet->m_pMethodManager = (MethodManager*)this;
            return pRet;
        }
    }

    MethodHandle* pMethodHandle = NodeManagerDemoBase::getMethodHandle(pSession, pObjectNodeId, pMethodNodeId, result);
    return pMethodHandle;
}

/** Implementation of the MethodManager interface method beginCall
*/
UaStatus NodeManagerDemo::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    UaStatus            ret;
    UaStatusCodeArray   inputArgumentResults;
    UaDiagnosticInfos   inputArgumentDiag;
    UaVariantArray      outputArguments;
    if (pMethodHandle->getHandleImplementation() == MethodHandle::NODE_ID)
    {
        MethodHandleNodeId* pMethodHandleNodeId = (MethodHandleNodeId*)pMethodHandle;
        // Check it we have Acknowledge or AddComment method
        if ( (pMethodHandleNodeId->m_methodNodeId == UaNodeId("Demo.Massfolder_Static.AddNodes", getNameSpaceIndex())) ||
             ( pMethodHandleNodeId->m_methodNodeId == UaNodeId("Demo.Massfolder_Dynamic.AddNodes", getNameSpaceIndex()) ) )
        {
            OpcUa_UInt32 totalCount = 0;
            if ( inputArguments.length() != 1 )
            {
                ret = OpcUa_BadInvalidArgument;
            }
            else
            {
                UaVariant    value;
                // Extract and check input parameters
                value = inputArguments[ 0 ];
                ret = value.toUInt32(totalCount);
            }

            if ( ret.isGood() )
            {
                ret = Demo_MassFolder_AddNodes(pMethodHandleNodeId->m_objectNodeId, totalCount);
            }

            pCallback->finishCall(
                callbackHandle,
                inputArgumentResults,
                inputArgumentDiag,
                outputArguments,
                ret);

            return OpcUa_Good;
        }
#if SUPPORT_Event_Subscription_Server_Facet
        else if ( (pMethodHandleNodeId->m_methodNodeId == OpcUaId_AcknowledgeableConditionType_Acknowledge) ||
                  (pMethodHandleNodeId->m_methodNodeId == OpcUaId_ConditionType_AddComment) )
        {
            bool isAck = false;
            if ( pMethodHandleNodeId->m_methodNodeId == OpcUaId_AcknowledgeableConditionType_Acknowledge )
            {
                isAck = true;
            }
            OpcUa::AcknowledgeableConditionTypeData* pConditionData = NULL;
            if ( pMethodHandleNodeId->m_objectNodeId == m_pUaOffNormalAlarmData->getConditionNodeId() )
            {
                pConditionData = m_pUaOffNormalAlarmData;
            }
            else if ( pMethodHandleNodeId->m_objectNodeId == m_pUaNonExclusiveLevelAlarmData->getConditionNodeId() )
            {
                pConditionData = m_pUaNonExclusiveLevelAlarmData;
            }
            else if ( pMethodHandleNodeId->m_objectNodeId == m_pUaExclusiveLevelAlarmData->getConditionNodeId() )
            {
                pConditionData = m_pUaExclusiveLevelAlarmData;
            }
            // Process method if we have a condition
            if ( pConditionData )
            {
                UaByteString eventId;
                UaLocalizedText comment;
                if ( inputArguments.length() != 2 )
                {
                    ret = OpcUa_BadInvalidArgument;
                }
                else
                {
                    UaVariant    value;
                    // Extract and check input parameters
                    if ( ret.isGood() )
                    {
                        value = inputArguments[ 0 ];
                        ret = value.toByteString(eventId);
                    }
                    if ( ret.isGood() )
                    {
                        value = inputArguments[ 1 ];
                        ret = value.toLocalizedText(comment);
                    }
                }

                if ( ret.isGood() )
                {
                    UaByteString storedEventId;
                    pConditionData->m_EventId.toByteString(storedEventId);
                    if ( eventId == storedEventId )
                    {
                        if ( isAck )
                        {
                            if ( pConditionData->getAckedState() == OpcUa_False )
                            {
                                // Update state for acknowledge and fire event
                                // Lock the event data for change since a refresh may access the data in parallel
                                pConditionData->lockEventData();
                                pConditionData->setAckedState(OpcUa_True);
                                pConditionData->setComment(comment);
                                pConditionData->setClientUserId(serviceContext.pSession()->getClientUserId());
                                pConditionData->setMessage(UaLocalizedText("en", "Condition state acknowledged by UA client"));
                                pConditionData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                                // Unlock event data before fire event
                                pConditionData->unlockEventData();
                                fireEvent(pConditionData);
                            }
                            else
                            {
                                ret = OpcUa_BadConditionBranchAlreadyAcked;
                            }
                        }
                        else
                        {
                            // Update state for comment and fire event
                            // Lock the event data for change since a refresh may access the data in parallel
                            pConditionData->lockEventData();
                            pConditionData->setComment(comment);
                            pConditionData->setClientUserId(serviceContext.pSession()->getClientUserId());
                            pConditionData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                            // Unlock event data before fire event
                            pConditionData->unlockEventData();
                            fireEvent(pConditionData);
                        }
                    }
                    else
                    {
                        ret = OpcUa_BadInvalidArgument;
                    }
                }

                pCallback->finishCall(
                    callbackHandle,
                    inputArgumentResults,
                    inputArgumentDiag,
                    outputArguments,
                    ret);

                return OpcUa_Good;
            }
        }
#endif // SUPPORT_Event_Subscription_Server_Facet
    }

    return NodeManagerDemoBase::beginCall(pCallback, serviceContext, callbackHandle, pMethodHandle, inputArguments);
}
#endif // SUPPORT_Method_Server_Facet


/** Limit access to nodes with restrictions.
*/
void NodeManagerDemo::setPermissionsForLimitedNodes()
{
    UaNode* pNode = NULL;

    // configure folders with limited access for browsing
    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Browse_All, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OBSERVATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Browse_John, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Browse_Operators, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OBSERVATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    // configure variables with limited access
    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_John_All_RO_John_RW, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_John_All_WO_John_RW, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_GROUP_WRITABLE |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_WRITABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_John_John_RO, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OBSERVATION |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_John_John_RW, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_John_John_WO, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_ATTRREADABLE |
            NodeAccessInfo::UA_OWNER_WRITABLE |
            NodeAccessInfo::UA_OWNER_BROWSEABLE |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_All_RO_Operators_RW, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_All_WO_Operators_RW, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_WRITABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_Operators_RO, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OBSERVATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_Operators_RW, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_Operators_WO, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_ATTRREADABLE |
            NodeAccessInfo::UA_OWNER_WRITABLE |
            NodeAccessInfo::UA_OWNER_BROWSEABLE |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_GROUP_WRITABLE |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    // configure CTT variables
    pNode = findNode(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OPERATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead_NotCurrentWrite, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OBSERVATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead_NotUser, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OBSERVATION |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OPERATION |
            NodeAccessInfo::UA_OTHER_OPERATION,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite_NotCurrentRead, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_ATTRREADABLE |
            NodeAccessInfo::UA_OWNER_WRITABLE |
            NodeAccessInfo::UA_OWNER_BROWSEABLE |
            NodeAccessInfo::UA_GROUP_ATTRREADABLE |
            NodeAccessInfo::UA_GROUP_WRITABLE |
            NodeAccessInfo::UA_GROUP_BROWSEABLE |
            NodeAccessInfo::UA_OTHER_ATTRREADABLE |
            NodeAccessInfo::UA_OTHER_WRITABLE |
            NodeAccessInfo::UA_OTHER_BROWSEABLE,
            m_userIdJohn,
            m_groupIdOperators));
    }

    pNode = findNode(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite_NotUser, getNameSpaceIndex()));
    if ( pNode )
    {
        pNode->attachAccessInfo(new NodeAccessInfo(
            (OpcUa_UInt32)
            NodeAccessInfo::UA_OWNER_OPERATION |
            NodeAccessInfo::UA_GROUP_OBSERVATION |
            NodeAccessInfo::UA_OTHER_OBSERVATION,
            m_userIdJohn,
            m_groupIdOperators));
    }
}

UaVariable* NodeManagerDemo::findSimulationVariable(const char *szIdentifier)
{
    UaVariable *pRet = OpcUa_Null;
    UaNode *pNode = findNode(UaNodeId(szIdentifier, getNameSpaceIndex()));
    if ( pNode && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        pRet = (UaVariable*)pNode;
        pRet->addReference();
    }
    return pRet;
}

/* Worker thread main method for simulation. */
void NodeManagerDemo::run()
{
    UaDataValue   dataValue;
    UaVariant     value;
    OpcUa_UInt32  valueSpeed       = 0;
    OpcUa_Boolean valueActive      = OpcUa_False;
    UaNode*       pNode            = NULL;
    UaVariable*   pSimulationSpeed = NULL;
    UaVariable*   pSimulationActive = NULL;

#define ARRAY_LENGTH 10
    OpcUa_Boolean           valueBoolean        = OpcUa_False;
    OpcUa_Byte              valueByte           = 0;
    UaByteString            valueByteString;
    OpcUa_DateTime          valueDateTime       = OpcUa_DateTime_UtcNow();
    OpcUa_Double            valueDouble         = 0;
    OpcUa_Double            valueDoubleHist     = 0;
    UaExpandedNodeId        valueExpandedNodeId;
    OpcUa_Float             valueFloat          = 0;
    UaGuid                  valueGuid           = UaGuid::create();
    OpcUa_Int16             valueInt16          = 0;
    OpcUa_Int32             valueInt32          = 0;
    OpcUa_Int64             valueInt64          = 0;
    UaLocalizedText         valueLocalizedText;
    UaNodeId                valueNodeId;
    UaQualifiedName         valueQualifiedName;
    OpcUa_SByte             valueSByte          = 0;
    UaStatusCodeArray       valuesStatusCode;
    OpcUa_Byte              valuesStatusIndex   = 0;
    UaString                valueString;
    UaString                valueXmlString;
    OpcUa_UInt16            valueUInt16         = 0;
    OpcUa_UInt32            valueUInt32         = 0;
    OpcUa_UInt64            valueUInt64         = 0;
    UaByteString            valueXmlElement;
    OpcUa_UInt16            indexAnimatedGIF    = 0;

    int                     i                   = 0;
    OpcUa_UInt32            uIndex              = 0;
    UaBooleanArray          valueBooleanArray;
    UaByteArray             valueByteArray;
    UaByteStringArray       valueByteStringArray;
    UaDateTimeArray         valueDateTimeArray;
    UaDoubleArray           valueDoubleArray;
    UaExpandedNodeIdArray   valueExpandedNodeIdArray;
    UaFloatArray            valueFloatArray;
    UaGuidArray             valueGuidArray;
    UaInt16Array            valueInt16Array;
    UaInt32Array            valueInt32Array;
    UaInt64Array            valueInt64Array;
    UaLocalizedTextArray    valueLocalizedTextArray;
    UaNodeIdArray           valueNodeIdArray;
    UaQualifiedNameArray    valueQualifiedNameArray;
    UaSByteArray            valueSByteArray;
    UaStatusCodeArray       valueStatusCodeArray;
    UaStringArray           valueStringArray;
    UaUInt16Array           valueUInt16Array;
    UaUInt32Array           valueUInt32Array;
    UaUInt64Array           valueUInt64Array;
    UaByteStringArray       valueXmlElementArray;
    UaByteStringArray       valueAnimatedGIF;

    UaVariable*   pScalarBoolean        = NULL;
    UaVariable*   pScalarByte           = NULL;
    UaVariable*   pScalarByteString     = NULL;
    UaVariable*   pScalarDateTime       = NULL;
    UaVariable*   pScalarDouble         = NULL;
    UaVariable*   pScalarExpandedNodeId = NULL;
    UaVariable*   pScalarFloat          = NULL;
    UaVariable*   pScalarGuid           = NULL;
    UaVariable*   pScalarInt16          = NULL;
    UaVariable*   pScalarInt32          = NULL;
    UaVariable*   pScalarInt64          = NULL;
    UaVariable*   pScalarLocalizedText  = NULL;
    UaVariable*   pScalarNodeId         = NULL;
    UaVariable*   pScalarQualifiedName  = NULL;
    UaVariable*   pScalarQualDynVal     = NULL;
    UaVariable*   pScalarQualStatVal    = NULL;
    UaVariable*   pScalarSByte          = NULL;
    UaVariable*   pScalarStatusCode     = NULL;
    UaVariable*   pScalarString         = NULL;
    UaVariable*   pScalarUInt16         = NULL;
    UaVariable*   pScalarUInt32         = NULL;
    UaVariable*   pScalarUInt64         = NULL;
    UaVariable*   pScalarXmlElement     = NULL;

    UaVariable*   pHistoryByte          = NULL;
    UaVariable*   pHistoryDouble        = NULL;

    UaVariable*   pImageGIF             = NULL;

    UaVariable*   pArrayBoolean         = NULL;
    UaVariable*   pArrayByte            = NULL;
    UaVariable*   pArrayByteString      = NULL;
    UaVariable*   pArrayDateTime        = NULL;
    UaVariable*   pArrayDouble          = NULL;
    UaVariable*   pArrayExpandedNodeId  = NULL;
    UaVariable*   pArrayFloat           = NULL;
    UaVariable*   pArrayGuid            = NULL;
    UaVariable*   pArrayInt16           = NULL;
    UaVariable*   pArrayInt32           = NULL;
    UaVariable*   pArrayInt64           = NULL;
    UaVariable*   pArrayLocalizedText   = NULL;
    UaVariable*   pArrayNodeId          = NULL;
    UaVariable*   pArrayQualifiedName   = NULL;
    UaVariable*   pArraySByte           = NULL;
    UaVariable*   pArrayStatusCode      = NULL;
    UaVariable*   pArrayString          = NULL;
    UaVariable*   pArrayUInt16          = NULL;
    UaVariable*   pArrayUInt32          = NULL;
    UaVariable*   pArrayUInt64          = NULL;
    UaVariable*   pArrayXmlElement      = NULL;

    // initialize status code array
    valuesStatusCode.create(ARRAY_LENGTH);
    for (i = 0; i < ARRAY_LENGTH; i++)
    {
        switch (i%ARRAY_LENGTH)
        {
        case 0:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_Good;
            break;
        case 1:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_UncertainEngineeringUnitsExceeded;
            break;
        case 2:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_BadNodeIdUnknown;
            break;
        case 3:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_BadAttributeIdInvalid;
            break;
        case 4:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_BadUserAccessDenied;
            break;
        case 5:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_BadDataEncodingInvalid;
            break;
        case 6:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_BadWaitingForInitialData;
            break;
        case 7:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_BadSecurityChecksFailed;
            break;
        case 8:
            valuesStatusCode[i%ARRAY_LENGTH] = OpcUa_UncertainInitialValue;
            break;
        case 9:
            valuesStatusCode[i%ARRAY_LENGTH] = 0x12345678;   // invalid status code to test clients
            break;
        }
    }

    // initialize ByteStringArray for animation
    int animationCount = 0;
    valueAnimatedGIF.create(100);
    OpcUa_Int64 readCount = 0;
    char buffer[128*1024];
    OpcUa_Int64 bufsize = 128*1024;
    while (animationCount < 100)
    {
        UaFile file(UaString("%1/animation/animation_%2.gif")
            .arg(m_sAppPath)
            .arg(animationCount));
        if (file.open(UaIODevice::OpenModeRead))
        {
            readCount = file.read(buffer, bufsize);
            if (readCount > 0)
            {
                UaByteString bsTmp((OpcUa_Int32)readCount, (OpcUa_Byte*)buffer);
                bsTmp.copyTo(&valueAnimatedGIF[animationCount]);
                animationCount++;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    valueAnimatedGIF.resize(animationCount);


    std::list<UaVariable*>::iterator it;

    // Demo_SimulationSpeed
    pNode = findNode(UaNodeId(DemoId_Demo_SimulationSpeed, getNameSpaceIndex()));
    if ( pNode && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        pSimulationSpeed = (UaVariable*)pNode;
        pSimulationSpeed->addReference();
        // Set default simulation speed to 80ms
        value.setUInt32(80);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pSimulationSpeed->setValue(NULL, dataValue, OpcUa_False);
    }
    else m_stopThread = true;

    // Demo_SimulationActive
    pNode = findNode(UaNodeId(DemoId_Demo_SimulationActive, getNameSpaceIndex()));
    if ( pNode && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        pSimulationActive = (UaVariable*)pNode;
        pSimulationActive->addReference();
        // Set default simulation speed to 80ms
        value.setBool(OpcUa_True);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pSimulationActive->setValue(NULL, dataValue, OpcUa_False);
    }
    else m_stopThread = true;

    /////////////////////////////////////////////////////////
    // Scalar
    // DemoId_Demo_Dynamic_Scalar_Boolean
    pScalarBoolean = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Boolean);
    if (!pScalarBoolean) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Byte
    pScalarByte = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Byte);
    if (!pScalarByte) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_ByteString
    pScalarByteString = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_ByteString);
    if (!pScalarByteString) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_DateTime
    pScalarDateTime = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_DateTime);
    if (!pScalarDateTime) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Double
    pScalarDouble = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Double);
    if (!pScalarDouble) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_ExpandedNodeId
    pScalarExpandedNodeId = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_ExpandedNodeId);
    if (!pScalarExpandedNodeId) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Float
    pScalarFloat = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Float);
    if (!pScalarFloat) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Guid
    pScalarGuid = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Guid);
    if (!pScalarGuid) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Int16
    pScalarInt16 = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Int16);
    if (!pScalarInt16) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Int32
    pScalarInt32 = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Int32);
    if (!pScalarInt32) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Int64
    pScalarInt64 = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Int64);
    if (!pScalarInt64) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_LocalizedText
    pScalarLocalizedText = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_LocalizedText);
    if (!pScalarLocalizedText) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_NodeId
    pScalarNodeId = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_NodeId);
    if (!pScalarNodeId) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_QualifiedName
    pScalarQualifiedName = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_QualifiedName);
    if (!pScalarQualifiedName) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Quality_DynamicValue
    pScalarQualDynVal = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Quality_DynamicValue);
    if (!pScalarQualDynVal) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_Quality_StaticValue
    pScalarQualStatVal = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_Quality_StaticValue);
    if (!pScalarQualStatVal) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_SByte
    pScalarSByte = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_SByte);
    if (!pScalarSByte) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_StatusCode
    pScalarStatusCode = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_StatusCode);
    if (!pScalarStatusCode) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_String
    pScalarString = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_String);
    if (!pScalarString) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_UInt16
    pScalarUInt16 = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_UInt16);
    if (!pScalarUInt16) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_UInt32
    pScalarUInt32 = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_UInt32);
    if (!pScalarUInt32) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_UInt64
    pScalarUInt64 = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_UInt64);
    if (!pScalarUInt64) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Scalar_XmlElement
    pScalarXmlElement = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_XmlElement);
    if (!pScalarXmlElement) {m_stopThread = true;}

    pHistoryByte = findSimulationVariable(DemoId_Demo_History_ByteWithHistory);
    if (!pHistoryByte) {m_stopThread = true;}

    pHistoryDouble = findSimulationVariable(DemoId_Demo_History_DoubleWithHistory);
    if (!pHistoryDouble) {m_stopThread = true;}

    pImageGIF = findSimulationVariable(DemoId_Demo_Dynamic_Scalar_ImageGIF);
    if (!pImageGIF) {m_stopThread = true;}

    /////////////////////////////////////////////////////////
    // Arrays
    // DemoId_Demo_Dynamic_Arrays_Boolean
    pArrayBoolean = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Boolean);
    if (!pArrayBoolean) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_Byte
    pArrayByte = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Byte);
    if (!pArrayByte) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_ByteString
    pArrayByteString = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_ByteString);
    if (!pArrayByteString) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_DateTime
    pArrayDateTime = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_DateTime);
    if (!pArrayDateTime) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_Double
    pArrayDouble = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Double);
    if (!pArrayDouble) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_ExpandedNodeId
    pArrayExpandedNodeId = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_ExpandedNodeId);
    if (!pArrayExpandedNodeId) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_Float
    pArrayFloat = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Float);
    if (!pArrayFloat) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_Guid
    pArrayGuid = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Guid);
    if (!pArrayGuid) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_Int16
    pArrayInt16 = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Int16);
    if (!pArrayInt16) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_Int32
    pArrayInt32 = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Int32);
    if (!pArrayInt32) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_Int64
    pArrayInt64 = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_Int64);
    if (!pArrayInt64) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_LocalizedText
    pArrayLocalizedText = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_LocalizedText);
    if (!pArrayLocalizedText) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_NodeId
    pArrayNodeId = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_NodeId);
    if (!pArrayNodeId) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_QualifiedName
    pArrayQualifiedName = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_QualifiedName);
    if (!pArrayQualifiedName) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_SByte
    pArraySByte = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_SByte);
    if (!pArraySByte) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_StatusCode
    pArrayStatusCode = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_StatusCode);
    if (!pArrayStatusCode) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_String
    pArrayString = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_String);
    if (!pArrayString) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_UInt16
    pArrayUInt16 = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_UInt16);
    if (!pArrayUInt16) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_UInt32
    pArrayUInt32 = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_UInt32);
    if (!pArrayUInt32) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_UInt64
    pArrayUInt64 = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_UInt64);
    if (!pArrayUInt64) {m_stopThread = true;}

    // DemoId_Demo_Dynamic_Arrays_XmlElement
    pArrayXmlElement = findSimulationVariable(DemoId_Demo_Dynamic_Arrays_XmlElement);
    if (!pArrayXmlElement) {m_stopThread = true;}

    valueBooleanArray.create(ARRAY_LENGTH);
    valueByteArray.resize(ARRAY_LENGTH);
    valueByteStringArray.create(ARRAY_LENGTH);
    valueDateTimeArray.create(ARRAY_LENGTH);
    valueDoubleArray.create(ARRAY_LENGTH);
    valueExpandedNodeIdArray.create(ARRAY_LENGTH);
    valueFloatArray.create(ARRAY_LENGTH);
    valueGuidArray.create(ARRAY_LENGTH);
    valueInt16Array.create(ARRAY_LENGTH);
    valueInt32Array.create(ARRAY_LENGTH);
    valueInt64Array.create(ARRAY_LENGTH);
    valueLocalizedTextArray.create(ARRAY_LENGTH);
    valueNodeIdArray.create(ARRAY_LENGTH);
    valueQualifiedNameArray.create(ARRAY_LENGTH);
    valueSByteArray.create(ARRAY_LENGTH);
    valueStatusCodeArray.create(ARRAY_LENGTH);
    valueStringArray.create(ARRAY_LENGTH);
    valueUInt16Array.create(ARRAY_LENGTH);
    valueUInt32Array.create(ARRAY_LENGTH);
    valueUInt64Array.create(ARRAY_LENGTH);
    valueXmlElementArray.create(ARRAY_LENGTH);

    /////////////////////////////////////////////////////////
    // Helper values
    UaByteArray valueByteStringHelper;
    valueByteStringHelper.resize(16);
    for (i = 0; i < valueByteStringHelper.size(); i++)
    {
        valueByteStringHelper[i] = (char)(i * 0x10);
    }

    UaUniString valueUniString = "*** stone - paper - scissors - lizard - spock ";

    while ( m_stopThread == false )
    {
        /////////////////////////////////////////////////////////
        // Simulation active
        dataValue = pSimulationActive->value(NULL);
        value = *dataValue.value();
        value.toBool(valueActive);
        if ( valueActive == OpcUa_False )
        {
            UaThread::msleep(valueSpeed);
            continue;
        }

        valueByteString.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
        valueString = valueUniString.toUtf16();
        valueXmlString = UaString("<a>%1</a>").arg(valueString);

        /////////////////////////////////////////////////////////
        // Scalar
        // DemoId_Demo_Dynamic_Scalar_Boolean
        valueBoolean = (valueBoolean == OpcUa_False) ? OpcUa_True : OpcUa_False;
        value.setBool(valueBoolean);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarBoolean->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Byte
        valueByte++;
        value.setByte(valueByte);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarByte->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_ByteString
        value.setByteString(valueByteString, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarByteString->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_DateTime
        valueDateTime = OpcUa_DateTime_UtcNow();
        value.setDateTime(valueDateTime);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarDateTime->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Double
        valueDouble += 1.1;
        value.setDouble(valueDouble);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarDouble->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_ExpandedNodeId
        valueExpandedNodeId.setExpandedNodeId(valueNodeId, valueExpandedNodeId.namespaceUri(), valueExpandedNodeId.serverIndex() + 1);
        value.setExpandedNodeId(valueExpandedNodeId);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarExpandedNodeId->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Float
        valueFloat += (OpcUa_Float)1.1;
        value.setFloat(valueFloat);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarFloat->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Guid
        valueGuid = UaGuid::create();
        value.setGuid(valueGuid);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarGuid->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Int16
        valueInt16++;
        value.setInt16(valueInt16);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarInt16->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Int32
        valueInt32++;
        value.setInt32(valueInt32);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarInt32->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Int64
        valueInt64++;
        value.setInt64(valueInt64);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarInt64->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_LocalizedText
        valueLocalizedText.setText(valueString);
        value.setLocalizedText(valueLocalizedText);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarLocalizedText->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_NodeId
        switch (valueNodeId.identifierType())
        {
        case OpcUa_IdentifierType_Numeric:
            valueNodeId.setNodeId(valueString, valueNodeId.namespaceIndex() + 1);
            break;
        case OpcUa_IdentifierType_String:
            valueNodeId.setNodeId(valueGuid, valueNodeId.namespaceIndex() + 1);
            break;
        case OpcUa_IdentifierType_Guid:
            valueNodeId.setNodeId(valueByteString, valueNodeId.namespaceIndex() + 1);
            break;
        case OpcUa_IdentifierType_Opaque:
            valueNodeId.setNodeId(valueUInt32, valueNodeId.namespaceIndex() + 1);
            break;
        default:
            break;
        }
        value.setNodeId(valueNodeId);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarNodeId->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_QualifiedName
        valueQualifiedName.setQualifiedName(valueString, valueQualifiedName.namespaceIndex() + 1);
        value.setQualifiedName(valueQualifiedName);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarQualifiedName->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Quality_DynamicValue
        // we check for good and uncertain because we also need to handle the invalid status 0x12345678 here
        if (OpcUa_IsGood(valueStatusCodeArray[valuesStatusIndex]) || OpcUa_IsUncertain(valueStatusCodeArray[valuesStatusIndex]))
        {
            value.setDouble(valueDouble);
        }
        else
        {
            value.clear();
        }
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        dataValue.setStatusCode(valueStatusCodeArray[valuesStatusIndex]);
        pScalarQualDynVal->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_Quality_StaticValue
        // we check for good and uncertain because we also need to handle the invalid status 0x12345678 here
        if (OpcUa_IsGood(valueStatusCodeArray[valuesStatusIndex]) || OpcUa_IsUncertain(valueStatusCodeArray[valuesStatusIndex]))
        {
            value.setDouble(12.34);
        }
        else
        {
            value.clear();
        }
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        dataValue.setStatusCode(valueStatusCodeArray[valuesStatusIndex]);
        pScalarQualStatVal->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_SByte
        valueSByte++;
        value.setSByte(valueSByte);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarSByte->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_StatusCode
        valuesStatusIndex++;
        if (valuesStatusIndex >= ARRAY_LENGTH)
        {
            valuesStatusIndex = 0;
        }
        value.setStatusCode(valuesStatusCode[valuesStatusIndex]);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarStatusCode->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_String
        value.setString(valueString);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarString->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_UInt16
        valueUInt16++;
        value.setUInt16(valueUInt16);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarUInt16->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_UInt32
        valueUInt32++;
        value.setUInt32(valueUInt32);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarUInt32->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_UInt64
        valueUInt64++;
        value.setUInt64(valueUInt64);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarUInt64->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Scalar_XmlElement
        valueXmlElement.setByteString(valueXmlString.size(), (OpcUa_Byte*)valueXmlString.toUtf8());
        value.setXmlElement(valueXmlElement);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pScalarXmlElement->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_History_ByteWithHistory
        value.setByte(valueByte);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pHistoryByte->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_History_DoubleWithHistory - create sin from 0 - 255
        value.setDouble(128 * sin(valueDoubleHist) + 128);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pHistoryDouble->setValue(NULL, dataValue, OpcUa_False);
        valueDoubleHist += 0.01;
        if (valueDoubleHist > 3.1415927*2)
        {
            valueDoubleHist = 0.0;
        }

        if (animationCount > 0)
        {
            // DemoId_Demo_Dynamic_Scalar_ImageGIF
            UaByteString nextPicture(valueAnimatedGIF[indexAnimatedGIF]);
            value.setByteString(nextPicture, OpcUa_True);
            dataValue.setValue(value, OpcUa_True, OpcUa_True);
            pImageGIF->setValue(NULL, dataValue, OpcUa_False);
            indexAnimatedGIF++;
            if (indexAnimatedGIF >= valueAnimatedGIF.length())
            {
                indexAnimatedGIF = 0;
            }
        }

        /////////////////////////////////////////////////////////
        // Mass variables
        value.setUInt32(valueUInt32);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        UaMutexLocker lockMassVar(&m_massVarMutex);
        it = m_massVariablesDynamic.begin();
        while ( it != m_massVariablesDynamic.end() )
        {
            (*it)->setValue(NULL, dataValue, OpcUa_False);
            it++;
        }
        lockMassVar.unlock();

        /////////////////////////////////////////////////////////
        // Array
        // DemoId_Demo_Dynamic_Arrays_Boolean
        for (uIndex = 0; uIndex < valueBooleanArray.length(); uIndex++)
        {
            valueBooleanArray[uIndex] = valueBoolean;
        }
        value.setBoolArray(valueBooleanArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayBoolean->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_Byte
        for (i = 0; i < valueByteArray.size(); i++)
        {
            valueByteArray[i] = valueByte;
        }
        value.setByteArray(valueByteArray);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayByte->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_ByteString
        for (uIndex = 0; uIndex < valueByteStringArray.length(); uIndex++)
        {
            OpcUa_ByteString_Clear(&valueByteStringArray[uIndex]);
            valueByteString.copyTo(&valueByteStringArray[uIndex]);
        }
        value.setByteStringArray(valueByteStringArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayByteString->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_DateTime
        for (uIndex = 0; uIndex < valueDateTimeArray.length(); uIndex++)
        {
            valueDateTimeArray[uIndex] = valueDateTime;
        }
        value.setDateTimeArray(valueDateTimeArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayDateTime->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_Double
        for (uIndex = 0; uIndex < valueDoubleArray.length(); uIndex++)
        {
            valueDoubleArray[uIndex] = valueDouble;
        }
        value.setDoubleArray(valueDoubleArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayDouble->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_ExpandedNodeId
        for (uIndex = 0; uIndex < valueExpandedNodeIdArray.length(); uIndex++)
        {
            OpcUa_ExpandedNodeId_Clear(&valueExpandedNodeIdArray[uIndex]);
            valueExpandedNodeId.copyTo(&valueExpandedNodeIdArray[uIndex]);
        }
        value.setExpandedNodeIdArray(valueExpandedNodeIdArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayExpandedNodeId->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_Float
        for (uIndex = 0; uIndex < valueFloatArray.length(); uIndex++)
        {
            valueFloatArray[uIndex] = valueFloat;
        }
        value.setFloatArray(valueFloatArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayFloat->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_Guid
        for (uIndex = 0; uIndex < valueGuidArray.length(); uIndex++)
        {
            UaGuid::create().copyTo(&valueGuidArray[uIndex]);
        }
        value.setGuidArray(valueGuidArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayGuid->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_Int16
        for (uIndex = 0; uIndex < valueInt16Array.length(); uIndex++)
        {
            valueInt16Array[uIndex] = valueInt16;
        }
        value.setInt16Array(valueInt16Array, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayInt16->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_Int32
        for (uIndex = 0; uIndex < valueInt32Array.length(); uIndex++)
        {
            valueInt32Array[uIndex] = valueInt32;
        }
        value.setInt32Array(valueInt32Array, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayInt32->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_Int64
        for (uIndex = 0; uIndex < valueInt64Array.length(); uIndex++)
        {
            valueInt64Array[uIndex] = valueInt64;
        }
        value.setInt64Array(valueInt64Array, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayInt64->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_LocalizedText
        for (uIndex = 0; uIndex < valueLocalizedTextArray.length(); uIndex++)
        {
            OpcUa_LocalizedText_Clear(&valueLocalizedTextArray[uIndex]);
            valueLocalizedText.copyTo(&valueLocalizedTextArray[uIndex]);
        }
        value.setLocalizedTextArray(valueLocalizedTextArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayLocalizedText->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_NodeId
        for (uIndex = 0; uIndex < valueNodeIdArray.length(); uIndex++)
        {
            OpcUa_NodeId_Clear(&valueNodeIdArray[uIndex]);
            valueNodeId.copyTo(&valueNodeIdArray[uIndex]);
        }
        value.setNodeIdArray(valueNodeIdArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayNodeId->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_QualifiedName
        for (uIndex = 0; uIndex < valueQualifiedNameArray.length(); uIndex++)
        {
            OpcUa_QualifiedName_Clear(&valueQualifiedNameArray[uIndex]);
            valueQualifiedName.copyTo(&valueQualifiedNameArray[uIndex]);
        }
        value.setQualifiedNameArray(valueQualifiedNameArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayQualifiedName->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_SByte
        for (uIndex = 0; uIndex < valueSByteArray.length(); uIndex++)
        {
            valueSByteArray[uIndex] = valueSByte;
        }
        value.setSByteArray(valueSByteArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArraySByte->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_StatusCode
        for (uIndex = 0; uIndex < valueStatusCodeArray.length(); uIndex++)
        {
            valueStatusCodeArray[uIndex] = valuesStatusCode[valuesStatusIndex];
        }
        value.setStatusCodeArray(valueStatusCodeArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayStatusCode->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_String
        for (uIndex = 0; uIndex < valueStringArray.length(); uIndex++)
        {
            OpcUa_String_Clear(&valueStringArray[uIndex]);
            valueString.copyTo(&valueStringArray[uIndex]);
        }
        value.setStringArray(valueStringArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayString->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_UInt16
        for (uIndex = 0; uIndex < valueUInt16Array.length(); uIndex++)
        {
            valueUInt16Array[uIndex] = valueUInt16;
        }
        value.setUInt16Array(valueUInt16Array, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayUInt16->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_UInt32
        for (uIndex = 0; uIndex < valueUInt32Array.length(); uIndex++)
        {
            valueUInt32Array[uIndex] = valueUInt32;
        }
        value.setUInt32Array(valueUInt32Array, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayUInt32->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_UInt64
        for (uIndex = 0; uIndex < valueUInt64Array.length(); uIndex++)
        {
            valueUInt64Array[uIndex] = valueUInt64;
        }
        value.setUInt64Array(valueUInt64Array, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayUInt64->setValue(NULL, dataValue, OpcUa_False);

        // DemoId_Demo_Dynamic_Arrays_XmlElement
        for (uIndex = 0; uIndex < valueXmlElementArray.length(); uIndex++)
        {
            OpcUa_XmlElement_Clear(&valueXmlElementArray[uIndex]);
            valueXmlElement.copyTo(&valueXmlElementArray[uIndex]);
        }
        value.setXmlElementArray(valueXmlElementArray, OpcUa_False);
        dataValue.setValue(value, OpcUa_True, OpcUa_True);
        pArrayXmlElement->setValue(NULL, dataValue, OpcUa_False);

        /////////////////////////////////////////////////////////
        // Helper values
        for (i = 0; i < valueByteStringHelper.size(); i++)
        {
            valueByteStringHelper[i] = valueByteStringHelper[i] + 1;
        }

        valueUniString.append(valueUniString.at(0));
        valueUniString.replace(0, 1, "");

        /////////////////////////////////////////////////////////
        // Simulation speed
        dataValue = pSimulationSpeed->value(NULL);
        value = *dataValue.value();
        value.toUInt32(valueSpeed);
        UaThread::msleep(valueSpeed);
    }

    if ( pSimulationSpeed ) pSimulationSpeed->releaseReference();
    if ( pSimulationActive ) pSimulationActive->releaseReference();

    if ( pScalarBoolean ) pScalarBoolean->releaseReference();
    if ( pScalarByte ) pScalarByte->releaseReference();
    if ( pScalarByteString ) pScalarByteString->releaseReference();
    if ( pScalarDateTime ) pScalarDateTime->releaseReference();
    if ( pScalarDouble ) pScalarDouble->releaseReference();
    if ( pScalarExpandedNodeId ) pScalarExpandedNodeId->releaseReference();
    if ( pScalarFloat ) pScalarFloat->releaseReference();
    if ( pScalarGuid ) pScalarGuid->releaseReference();
    if ( pScalarInt16 ) pScalarInt16->releaseReference();
    if ( pScalarInt32 ) pScalarInt32->releaseReference();
    if ( pScalarInt64 ) pScalarInt64->releaseReference();
    if ( pScalarLocalizedText ) pScalarLocalizedText->releaseReference();
    if ( pScalarNodeId ) pScalarNodeId->releaseReference();
    if ( pScalarQualifiedName ) pScalarQualifiedName->releaseReference();
    if ( pScalarQualDynVal ) pScalarQualDynVal->releaseReference();
    if ( pScalarQualStatVal ) pScalarQualStatVal->releaseReference();
    if ( pScalarSByte ) pScalarSByte->releaseReference();
    if ( pScalarStatusCode ) pScalarStatusCode->releaseReference();
    if ( pScalarString ) pScalarString->releaseReference();
    if ( pScalarUInt16 ) pScalarUInt16->releaseReference();
    if ( pScalarUInt32 ) pScalarUInt32->releaseReference();
    if ( pScalarUInt64 ) pScalarUInt64->releaseReference();
    if ( pScalarXmlElement ) pScalarXmlElement->releaseReference();

    if ( pHistoryByte ) pHistoryByte->releaseReference();
    if ( pHistoryDouble ) pHistoryDouble->releaseReference();

    if ( pImageGIF ) pImageGIF->releaseReference();

    if ( pArrayBoolean ) pArrayBoolean->releaseReference();
    if ( pArrayByte ) pArrayByte->releaseReference();
    if ( pArrayByteString ) pArrayByteString->releaseReference();
    if ( pArrayDateTime ) pArrayDateTime->releaseReference();
    if ( pArrayDouble ) pArrayDouble->releaseReference();
    if ( pArrayExpandedNodeId ) pArrayExpandedNodeId->releaseReference();
    if ( pArrayFloat ) pArrayFloat->releaseReference();
    if ( pArrayGuid ) pArrayGuid->releaseReference();
    if ( pArrayInt16 ) pArrayInt16->releaseReference();
    if ( pArrayInt32 ) pArrayInt32->releaseReference();
    if ( pArrayInt64 ) pArrayInt64->releaseReference();
    if ( pArrayLocalizedText ) pArrayLocalizedText->releaseReference();
    if ( pArrayNodeId ) pArrayNodeId->releaseReference();
    if ( pArrayQualifiedName ) pArrayQualifiedName->releaseReference();
    if ( pArraySByte ) pArraySByte->releaseReference();
    if ( pArrayStatusCode ) pArrayStatusCode->releaseReference();
    if ( pArrayString ) pArrayString->releaseReference();
    if ( pArrayUInt16 ) pArrayUInt16->releaseReference();
    if ( pArrayUInt32 ) pArrayUInt32->releaseReference();
    if ( pArrayUInt64 ) pArrayUInt64->releaseReference();
    if ( pArrayXmlElement ) pArrayXmlElement->releaseReference();
}


} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

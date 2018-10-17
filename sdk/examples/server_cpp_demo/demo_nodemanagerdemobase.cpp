/******************************************************************************
** demo_nodemanagerdemobase.cpp
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

#include "demo_nodemanagerdemobase.h"

// Include type headers
#include "demo_datatypes.h"
#include <uabasenodes.h>
#include "demo_accessrights.h"
#include "demo_boilertype.h"
#include "demo_filllevelsensortype.h"
#include "demo_machinetype.h"
#if SUPPORT_Event_Subscription_Server_Facet
# include "demo_sampleeventtypedata.h"
#endif
#include "demo_structurewithoptionalfields.h"
#include "demo_temperaturesensortype.h"
#include "demo_uniontest.h"
#include "demo_vector.h"
#include "demo_workordertype.h"
#include "demo_workordervariabletype.h"
#include "opcua_analogitemtype.h"
#include "opcua_basedatavariabletype.h"
#include "opcua_baseobjecttype.h"
#include "opcua_dataitemtype.h"
#include "opcua_foldertype.h"
#include "opcua_multistatediscretetype.h"
#include "opcua_multistatevaluediscretetype.h"
#include "opcua_propertytype.h"
#include "opcua_twostatediscretetype.h"
#include "uaargument.h"
#include "instancefactory.h"
#include "nodemanagerroot.h"
#include "opcua_foldertype.h"
#if SUPPORT_Method_Server_Facet
#  include <uathreadpool.h>
#endif

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {
#if SUPPORT_Method_Server_Facet
class CPP_DEMO_EXPORT MethodCallJob : public UaThreadPoolJob
{
    UA_DISABLE_COPY(MethodCallJob);
public:
    MethodCallJob();
    virtual ~MethodCallJob();
    void execute();
    void initialize(
        NodeManagerDemoBase* pNodeManager,
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
    NodeManagerDemoBase* m_pNodeManager;
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
    NodeManagerDemoBase* pNodeManager,
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
OpcUa_UInt16 NodeManagerDemoBase::s_namespaceIndex = 0;

/** Construction of the class NodeManagerDemoBase.
*/
NodeManagerDemoBase::NodeManagerDemoBase(
    OpcUa_Boolean firesEvents,  //!< [in] Flag indicating if the NodeManager should activate the EventManager
    OpcUa_Int32 nHashTableSize) //!< [in] Size of the hash table. The default value is 10.007.<br>
                                //   The size can be smaller than the total number of nodes managed
                                //   by the node manager but it is better for the performance to avoid
                                //   collisions with a large size.
                                //   Prefered sizes are 1.009, 10.007, 100.003, 1.000.003, 10.000.019.
: NodeManagerBase("http://www.unifiedautomation.com/DemoServer/", firesEvents, nHashTableSize)
{
    m_defaultLocaleId         = "en";
}

/** Finish start up in derived class after starting up base class.
*/
UaStatus NodeManagerDemoBase::afterStartUp()
{
    UaStatus ret;

    s_namespaceIndex = getNameSpaceIndex();

    Demo::DataTypes::registerStructuredTypes();

    ret = createDataTypeNodes();
    UA_ASSERT(ret.isGood());
    ret = createReferenceTypeNodes();
    UA_ASSERT(ret.isGood());

    // ------------------------------------------------------------
    // Create object type nodes
    // ------------------------------------------------------------
#if SUPPORT_Event_Subscription_Server_Facet
    SampleEventTypeData::createTypes();
#endif
    BoilerType::createTypes();
    FillLevelSensorType::createTypes();
    MachineType::createTypes();
    TemperatureSensorType::createTypes();
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // Create variable type nodes
    // ------------------------------------------------------------
    WorkOrderVariableType::createTypes();
    // ------------------------------------------------------------

    XmlUaNodeFactoryManager factory;
    Demo::InstanceFactoryDemo factoryDemo(getTypeNamespace());
    factory.addNamespace(&factoryDemo);

    ret = addObjectsFolderDemo(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStatic(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArrays(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysBaseDataType(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysBoolean(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysByteString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysDateTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysDuration(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysExpandedNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysGuid(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysImagePNG(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysInteger(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysLocalizedText(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysNumber(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysNumericRange(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysQualifiedName(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysServerState(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysStatusCode(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysStructure(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysUInteger(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysUtcTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysVector(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysWorkOrder(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticArraysXmlElement(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalar(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarBaseDataType(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarBoolean(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarByteString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarDateTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarDuration(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarExpandedNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarGuid(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarHeaterStatus(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarImagePNG(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarInteger(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarLocalizedText(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarNumber(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarNumericRange(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarPriority(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarQualifiedName(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarServerState(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStatusCode(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStructures(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStructuresOptionalFields(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStructuresOptionSet(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStructuresStructure(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStructuresUnion(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStructuresVector(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarStructuresWorkOrder(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarUInteger(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarUtcTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStaticScalarXmlElement(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamic(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArrays(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysBoolean(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysByteString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysDateTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysExpandedNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysGuid(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysLocalizedText(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysQualifiedName(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysStatusCode(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicArraysXmlElement(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalar(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarBoolean(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarByteString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarDateTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarExpandedNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarGuid(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarImageGIF(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarLocalizedText(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarQualifiedName(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarQuality_DynamicValue(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarQuality_StaticValue(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarStatusCode(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicScalarXmlElement(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoHistory(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoHistoryByteWithHistory(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoHistoryDataLoggerActive(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoHistoryDoubleWithHistory(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoHistoryStartLogging(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoHistoryStopLogging(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoMethod(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoMethodDoSomethingAfter10s(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoMethodEnumTest(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoMethodMultiply(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoMethodVectorAdd(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoEvents(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoEventsMachine(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoEventsSampleEventNotifier(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoEventsTrigger_BaseEvent(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoEventsTrigger_SampleEvent(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRights(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_All(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_AllAll_RO(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_AllAll_RW(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_AllAll_WO(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_John(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_JohnAll_RO_John_RW(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_JohnAll_WO_John_RW(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_JohnJohn_RO(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_JohnJohn_RW(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_JohnJohn_WO(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_Operators(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_OperatorsAll_RO_Operators_RW(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_OperatorsAll_WO_Operators_RW(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_RO(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_RW(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_WO(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsBrowse_All(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsBrowse_John(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoAccessRightsBrowse_Operators(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoMassfolder_Static(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoMassfolder_Dynamic(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicNodes(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicNodesCreateDynamicNode(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicNodesDeleteDynamicNode(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoDynamicNodesNodeVersion(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoBoilerDemo(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoBoilerDemoBoiler1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTT(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTMethods(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTMethodsMethodIO(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTMethodsMethodNoArgs(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTNodeClasses(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTNodeClassesMethod(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTNodeClassesObject(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTNodeClassesVariable(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPaths(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8folder9(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8folder9folder10(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferences(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences2BaseDataVariable(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Object(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Property(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3InverseReferences(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences4(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences5(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHasInverseAndForwardReferences(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHasInverseAndForwardReferencesReferencedNode(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypes(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode4(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTSecurityAccess(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead_NotCurrentWrite(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead_NotUser(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite_NotCurrentRead(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite_NotUser(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStatic(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_Profiles(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArrays(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysBool(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysByteString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysDateTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysDuration(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysGuid(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysLocalizedText(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysQualifiedName(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysVariant(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesArraysXmlElement(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalar(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarBool(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarByteString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarDateTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarDuration(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarEnumeration(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarGuid(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarImage(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageBMP(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageGIF(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageJPG(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarImagePNG(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarInteger(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarLocalizedText(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarNodeId(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarNumber(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarQualifiedName(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInteger(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarVariant(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticAll_ProfilesScalarXmlElement(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_Profile(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeVariableWithEngineeringUnits(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeVariableWithInstrumentRange(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_Arrays(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItem(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemDateTime(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemDouble(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemFloat(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemString(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteType(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete4(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete5(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete1(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete3(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete4(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete5(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteType(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeSByte(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt16(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt32(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt64(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoUnicodeTest(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoUnicodeTestDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoUnicodeTestFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoUnicodeTestThe_quick_brown_fox_jumps_over_the_lazy_dog(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoUnicodeTestVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoUnicodeTestNode5016(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoIcon(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoIconIcon(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoImages(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoImagesDynamic(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoImagesStatic(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoFiles(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoWorkOrder(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoWorkOrderWorkOrderVariable(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoWorkOrderWorkOrderVariable2(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoSetSimulationSpeed(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoSimulationActive(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoSimulationSpeed(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStartSimulation(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addObjectsFolderDemoStopSimulation(factory);
    UA_ASSERT(ret.isGood());
    if (ret.isBad()) return ret;
    ret = addAdditionalHierarchicalReferences();

    return ret;
}

/** Static method to get the namespace index for the type namespace managed by this class.
 */
OpcUa_UInt16 NodeManagerDemoBase::getTypeNamespace()
{
    return s_namespaceIndex;
}

/** Helper function to create data type related nodes.
*/
UaStatus NodeManagerDemoBase::createDataTypeNodes()
{
    UaStatus               ret;
    UaVariant              defaultValue;
    UaGenericDataType*     pDataType;
    OpcUa::BaseObjectTypeGeneric *pObject;
    UaLocalizedTextArray   ltEnumStrings;
    UaExtensionObjectArray arEnumValues;
    OpcUa_EnumValueType*   pEnumValue;
    UaString               sTemp;
    OpcUa::PropertyType*   pEnumString;
    UaLocalizedTextArray   ltOptionSetValues;
    UaString               sOptionSetTemp;
    OpcUa::PropertyType*   pOptionSetValues;

    // AccessRights
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_AccessRights, getNameSpaceIndex()),
                    UaQualifiedName("AccessRights", getNameSpaceIndex()),
                    UaLocalizedText("", "AccessRights"),
                    UaLocalizedText("", ""),
                    OpcUa_False);
    ret = addNodeAndReference(UaNodeId(OpcUaId_OptionSet), pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());
    ltOptionSetValues.create(3);
    sOptionSetTemp = "Read";
    sOptionSetTemp.copyTo(&ltOptionSetValues[0].Text);
    sOptionSetTemp = "Write";
    sOptionSetTemp.copyTo(&ltOptionSetValues[1].Text);
    sOptionSetTemp = "Execute";
    sOptionSetTemp.copyTo(&ltOptionSetValues[2].Text);

    defaultValue.setLocalizedTextArray(ltOptionSetValues);
    pOptionSetValues = new OpcUa::PropertyType(UaNodeId(DemoId_AccessRights_OptionSetValues, getNameSpaceIndex()), "OptionSetValues", 0, defaultValue, 1, this);
    defaultValue.clear();
    ret = addNodeAndReference(pDataType, pOptionSetValues, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());

    pObject = new OpcUa::BaseObjectTypeGeneric(
                    UaNodeId(DemoId_AccessRights_Encoding_DefaultBinary, getNameSpaceIndex()),
                    "Default Binary",
                    0,
                    this);
    pObject->setTypeDefinitionId(OpcUaId_DataTypeEncodingType);

    ret = addNodeAndReference(pDataType, pObject, OpcUaId_HasEncoding);
    UA_ASSERT(ret.isGood());
    // Enumerations

    // HeaterStatus
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_HeaterStatus, getNameSpaceIndex()),
                    UaQualifiedName("HeaterStatus", getNameSpaceIndex()),
                    UaLocalizedText("", "HeaterStatus"),
                    UaLocalizedText("", ""),
                    OpcUa_False);
    ret = addNodeAndReference(OpcUaId_Enumeration, pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());
    // EnumStrings
    ltEnumStrings.create(3);
    sTemp = "Off";
    sTemp.copyTo(&ltEnumStrings[0].Text);
    sTemp = "Heating";
    sTemp.copyTo(&ltEnumStrings[1].Text);
    sTemp = "Cooling";
    sTemp.copyTo(&ltEnumStrings[2].Text);

    defaultValue.setLocalizedTextArray(ltEnumStrings);
    pEnumString = new OpcUa::PropertyType(UaNodeId(DemoId_HeaterStatus_EnumStrings, getNameSpaceIndex()), "EnumStrings", 0, defaultValue, 1, this);
    defaultValue.clear();
    ret = addNodeAndReference(pDataType, pEnumString, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());

    // Priority
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_Priority, getNameSpaceIndex()),
                    UaQualifiedName("Priority", getNameSpaceIndex()),
                    UaLocalizedText("", "Priority"),
                    UaLocalizedText("", "This enumerated value defines its possible values in an EnumValues property"),
                    OpcUa_False);
    ret = addNodeAndReference(OpcUaId_Enumeration, pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());
    // EnumValues
    arEnumValues.create(5);
    OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                           &arEnumValues[0],
                                           (OpcUa_Void**)&pEnumValue);
    pEnumValue->Value = 10;
    sTemp = "Low";
    sTemp.copyTo(&pEnumValue->DisplayName.Text);
    OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                           &arEnumValues[1],
                                           (OpcUa_Void**)&pEnumValue);
    pEnumValue->Value = 40;
    sTemp = "Normal";
    sTemp.copyTo(&pEnumValue->DisplayName.Text);
    OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                           &arEnumValues[2],
                                           (OpcUa_Void**)&pEnumValue);
    pEnumValue->Value = 70;
    sTemp = "High";
    sTemp.copyTo(&pEnumValue->DisplayName.Text);
    OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                           &arEnumValues[3],
                                           (OpcUa_Void**)&pEnumValue);
    pEnumValue->Value = 90;
    sTemp = "Urgent";
    sTemp.copyTo(&pEnumValue->DisplayName.Text);
    OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                           &arEnumValues[4],
                                           (OpcUa_Void**)&pEnumValue);
    pEnumValue->Value = 100;
    sTemp = "Immediate";
    sTemp.copyTo(&pEnumValue->DisplayName.Text);

    defaultValue.setExtensionObjectArray(arEnumValues, OpcUa_True);
    pEnumString = new OpcUa::PropertyType(UaNodeId(DemoId_Priority_EnumValues, getNameSpaceIndex()), "EnumValues", 0, defaultValue, 1, this);
    defaultValue.clear();
    ret = addNodeAndReference(pDataType, pEnumString, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());

    // Structured Types

    // StructureWithOptionalFields
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_StructureWithOptionalFields, getNameSpaceIndex()),
                    UaQualifiedName("StructureWithOptionalFields", getNameSpaceIndex()),
                    UaLocalizedText("", "StructureWithOptionalFields"),
                    UaLocalizedText("", ""),
                    OpcUa_False);
    ret = addNodeAndReference(UaNodeId(OpcUaId_Structure), pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());

    pObject = new OpcUa::BaseObjectTypeGeneric(
                    UaNodeId(DemoId_StructureWithOptionalFields_Encoding_DefaultBinary, getNameSpaceIndex()),
                    "Default Binary",
                    0,
                    this);
    pObject->setTypeDefinitionId(OpcUaId_DataTypeEncodingType);

    ret = addNodeAndReference(pDataType, pObject, OpcUaId_HasEncoding);
    UA_ASSERT(ret.isGood());

    // UnionTest
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_UnionTest, getNameSpaceIndex()),
                    UaQualifiedName("UnionTest", getNameSpaceIndex()),
                    UaLocalizedText("", "UnionTest"),
                    UaLocalizedText("", ""),
                    OpcUa_False);
    ret = addNodeAndReference(UaNodeId(OpcUaId_Union), pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());

    pObject = new OpcUa::BaseObjectTypeGeneric(
                    UaNodeId(DemoId_UnionTest_Encoding_DefaultBinary, getNameSpaceIndex()),
                    "Default Binary",
                    0,
                    this);
    pObject->setTypeDefinitionId(OpcUaId_DataTypeEncodingType);

    ret = addNodeAndReference(pDataType, pObject, OpcUaId_HasEncoding);
    UA_ASSERT(ret.isGood());

    // Vector
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_Vector, getNameSpaceIndex()),
                    UaQualifiedName("Vector", getNameSpaceIndex()),
                    UaLocalizedText("", "Vector"),
                    UaLocalizedText("", ""),
                    OpcUa_False);
    ret = addNodeAndReference(UaNodeId(OpcUaId_Structure), pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());

    pObject = new OpcUa::BaseObjectTypeGeneric(
                    UaNodeId(DemoId_Vector_Encoding_DefaultBinary, getNameSpaceIndex()),
                    "Default Binary",
                    0,
                    this);
    pObject->setTypeDefinitionId(OpcUaId_DataTypeEncodingType);

    ret = addNodeAndReference(pDataType, pObject, OpcUaId_HasEncoding);
    UA_ASSERT(ret.isGood());

    // WorkOrderStatusType
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_WorkOrderStatusType, getNameSpaceIndex()),
                    UaQualifiedName("WorkOrderStatusType", getNameSpaceIndex()),
                    UaLocalizedText("", "WorkOrderStatusType"),
                    UaLocalizedText("", ""),
                    OpcUa_False);
    ret = addNodeAndReference(UaNodeId(OpcUaId_Structure), pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());

    pObject = new OpcUa::BaseObjectTypeGeneric(
                    UaNodeId(DemoId_WorkOrderStatusType_Encoding_DefaultBinary, getNameSpaceIndex()),
                    "Default Binary",
                    0,
                    this);
    pObject->setTypeDefinitionId(OpcUaId_DataTypeEncodingType);

    ret = addNodeAndReference(pDataType, pObject, OpcUaId_HasEncoding);
    UA_ASSERT(ret.isGood());

    // WorkOrderType
    pDataType = new UaGenericDataType(
                    UaNodeId(DemoId_WorkOrderType, getNameSpaceIndex()),
                    UaQualifiedName("WorkOrderType", getNameSpaceIndex()),
                    UaLocalizedText("", "WorkOrderType"),
                    UaLocalizedText("", ""),
                    OpcUa_False);
    ret = addNodeAndReference(UaNodeId(OpcUaId_Structure), pDataType, OpcUaId_HasSubtype);
    UA_ASSERT(ret.isGood());

    pObject = new OpcUa::BaseObjectTypeGeneric(
                    UaNodeId(DemoId_WorkOrderType_Encoding_DefaultBinary, getNameSpaceIndex()),
                    "Default Binary",
                    0,
                    this);
    pObject->setTypeDefinitionId(OpcUaId_DataTypeEncodingType);

    ret = addNodeAndReference(pDataType, pObject, OpcUaId_HasEncoding);
    UA_ASSERT(ret.isGood());

    // Data Type Dictionary
    UaByteString bsDictionary(3409, (OpcUa_Byte*)"\074\157\160\143\072\124\171\160\145\104\151\143\164\151\157\156\141\162\171\040\170\155\154\156\163\072\170\163\151\075\042\150"
        "\164\164\160\072\057\057\167\167\167\056\167\063\056\157\162\147\057\062\060\060\061\057\130\115\114\123\143\150\145\155\141\055"
        "\151\156\163\164\141\156\143\145\042\040\170\155\154\156\163\072\164\156\163\075\042\150\164\164\160\072\057\057\167\167\167\056"
        "\165\156\151\146\151\145\144\141\165\164\157\155\141\164\151\157\156\056\143\157\155\057\104\145\155\157\123\145\162\166\145\162"
        "\057\042\040\104\145\146\141\165\154\164\102\171\164\145\117\162\144\145\162\075\042\114\151\164\164\154\145\105\156\144\151\141"
        "\156\042\040\170\155\154\156\163\072\157\160\143\075\042\150\164\164\160\072\057\057\157\160\143\146\157\165\156\144\141\164\151"
        "\157\156\056\157\162\147\057\102\151\156\141\162\171\123\143\150\145\155\141\057\042\040\170\155\154\156\163\072\165\141\075\042"
        "\150\164\164\160\072\057\057\157\160\143\146\157\165\156\144\141\164\151\157\156\056\157\162\147\057\125\101\057\042\040\124\141"
        "\162\147\145\164\116\141\155\145\163\160\141\143\145\075\042\150\164\164\160\072\057\057\167\167\167\056\165\156\151\146\151\145"
        "\144\141\165\164\157\155\141\164\151\157\156\056\143\157\155\057\104\145\155\157\123\145\162\166\145\162\057\042\076\012\040\074"
        "\157\160\143\072\111\155\160\157\162\164\040\116\141\155\145\163\160\141\143\145\075\042\150\164\164\160\072\057\057\157\160\143"
        "\146\157\165\156\144\141\164\151\157\156\056\157\162\147\057\125\101\057\042\057\076\012\040\074\157\160\143\072\123\164\162\165"
        "\143\164\165\162\145\144\124\171\160\145\040\102\141\163\145\124\171\160\145\075\042\165\141\072\105\170\164\145\156\163\151\157"
        "\156\117\142\152\145\143\164\042\040\116\141\155\145\075\042\101\143\143\145\163\163\122\151\147\150\164\163\042\076\012\040\040"
        "\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\102\171\164\145\123\164\162"
        "\151\156\147\042\040\116\141\155\145\075\042\126\141\154\165\145\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144"
        "\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\102\171\164\145\123\164\162\151\156\147\042\040\116\141\155\145\075"
        "\042\126\141\154\151\144\102\151\164\163\042\057\076\012\040\074\057\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124"
        "\171\160\145\076\012\040\074\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124\171\160\145\040\102\141\163\145\124\171"
        "\160\145\075\042\165\141\072\105\170\164\145\156\163\151\157\156\117\142\152\145\143\164\042\040\116\141\155\145\075\042\123\164"
        "\162\165\143\164\165\162\145\127\151\164\150\117\160\164\151\157\156\141\154\106\151\145\154\144\163\042\076\012\040\040\074\157"
        "\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\102\151\164\042\040\116\141\155\145"
        "\075\042\117\160\164\151\157\156\141\154\111\156\164\063\062\123\160\145\143\151\146\151\145\144\042\057\076\012\040\040\074\157"
        "\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\102\151\164\042\040\116\141\155\145"
        "\075\042\117\160\164\151\157\156\141\154\123\164\162\151\156\147\101\162\162\141\171\123\160\145\143\151\146\151\145\144\042\057"
        "\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\114\145\156\147\164\150\075\042\063\060\042\040\124\171\160\145\116"
        "\141\155\145\075\042\157\160\143\072\102\151\164\042\040\116\141\155\145\075\042\122\145\163\145\162\166\145\144\061\042\057\076"
        "\012\040\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\111\156\164\063"
        "\062\042\040\116\141\155\145\075\042\115\141\156\144\141\164\157\162\171\111\156\164\063\062\042\057\076\012\040\040\074\157\160"
        "\143\072\106\151\145\154\144\040\123\167\151\164\143\150\106\151\145\154\144\075\042\117\160\164\151\157\156\141\154\111\156\164"
        "\063\062\123\160\145\143\151\146\151\145\144\042\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\111\156\164\063\062"
        "\042\040\116\141\155\145\075\042\117\160\164\151\157\156\141\154\111\156\164\063\062\042\057\076\012\040\040\074\157\160\143\072"
        "\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\111\156\164\063\062\042\040\116\141\155\145\075"
        "\042\116\157\117\146\115\141\156\144\141\164\157\162\171\123\164\162\151\156\147\101\162\162\141\171\042\057\076\012\040\040\074"
        "\157\160\143\072\106\151\145\154\144\040\114\145\156\147\164\150\106\151\145\154\144\075\042\116\157\117\146\115\141\156\144\141"
        "\164\157\162\171\123\164\162\151\156\147\101\162\162\141\171\042\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\103"
        "\150\141\162\101\162\162\141\171\042\040\116\141\155\145\075\042\115\141\156\144\141\164\157\162\171\123\164\162\151\156\147\101"
        "\162\162\141\171\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\123\167\151\164\143\150\106\151\145\154\144"
        "\075\042\117\160\164\151\157\156\141\154\123\164\162\151\156\147\101\162\162\141\171\123\160\145\143\151\146\151\145\144\042\040"
        "\124\171\160\145\116\141\155\145\075\042\157\160\143\072\111\156\164\063\062\042\040\116\141\155\145\075\042\116\157\117\146\117"
        "\160\164\151\157\156\141\154\123\164\162\151\156\147\101\162\162\141\171\042\057\076\012\040\040\074\157\160\143\072\106\151\145"
        "\154\144\040\114\145\156\147\164\150\106\151\145\154\144\075\042\116\157\117\146\117\160\164\151\157\156\141\154\123\164\162\151"
        "\156\147\101\162\162\141\171\042\040\123\167\151\164\143\150\106\151\145\154\144\075\042\117\160\164\151\157\156\141\154\123\164"
        "\162\151\156\147\101\162\162\141\171\123\160\145\143\151\146\151\145\144\042\040\124\171\160\145\116\141\155\145\075\042\157\160"
        "\143\072\103\150\141\162\101\162\162\141\171\042\040\116\141\155\145\075\042\117\160\164\151\157\156\141\154\123\164\162\151\156"
        "\147\101\162\162\141\171\042\057\076\012\040\074\057\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124\171\160\145\076"
        "\012\040\074\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124\171\160\145\040\102\141\163\145\124\171\160\145\075\042"
        "\165\141\072\125\156\151\157\156\042\040\116\141\155\145\075\042\125\156\151\157\156\124\145\163\164\042\076\012\040\040\074\157"
        "\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\125\111\156\164\063\062\042\040\116"
        "\141\155\145\075\042\123\167\151\164\143\150\106\151\145\154\144\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144"
        "\040\123\167\151\164\143\150\106\151\145\154\144\075\042\123\167\151\164\143\150\106\151\145\154\144\042\040\124\171\160\145\116"
        "\141\155\145\075\042\157\160\143\072\111\156\164\063\062\042\040\123\167\151\164\143\150\126\141\154\165\145\075\042\061\042\040"
        "\116\141\155\145\075\042\111\156\164\063\062\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\123\167\151\164"
        "\143\150\106\151\145\154\144\075\042\123\167\151\164\143\150\106\151\145\154\144\042\040\124\171\160\145\116\141\155\145\075\042"
        "\157\160\143\072\103\150\141\162\101\162\162\141\171\042\040\123\167\151\164\143\150\126\141\154\165\145\075\042\062\042\040\116"
        "\141\155\145\075\042\123\164\162\151\156\147\042\057\076\012\040\074\057\157\160\143\072\123\164\162\165\143\164\165\162\145\144"
        "\124\171\160\145\076\012\040\074\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124\171\160\145\040\102\141\163\145\124"
        "\171\160\145\075\042\165\141\072\105\170\164\145\156\163\151\157\156\117\142\152\145\143\164\042\040\116\141\155\145\075\042\126"
        "\145\143\164\157\162\042\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157"
        "\160\143\072\104\157\165\142\154\145\042\040\116\141\155\145\075\042\130\042\057\076\012\040\040\074\157\160\143\072\106\151\145"
        "\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\104\157\165\142\154\145\042\040\116\141\155\145\075\042\131"
        "\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\104"
        "\157\165\142\154\145\042\040\116\141\155\145\075\042\132\042\057\076\012\040\074\057\157\160\143\072\123\164\162\165\143\164\165"
        "\162\145\144\124\171\160\145\076\012\040\074\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124\171\160\145\040\102\141"
        "\163\145\124\171\160\145\075\042\165\141\072\105\170\164\145\156\163\151\157\156\117\142\152\145\143\164\042\040\116\141\155\145"
        "\075\042\127\157\162\153\117\162\144\145\162\123\164\141\164\165\163\124\171\160\145\042\076\012\040\040\074\157\160\143\072\106"
        "\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\103\150\141\162\101\162\162\141\171\042\040\116\141"
        "\155\145\075\042\101\143\164\157\162\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141"
        "\155\145\075\042\157\160\143\072\104\141\164\145\124\151\155\145\042\040\116\141\155\145\075\042\124\151\155\145\163\164\141\155"
        "\160\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\165\141\072\114"
        "\157\143\141\154\151\172\145\144\124\145\170\164\042\040\116\141\155\145\075\042\103\157\155\155\145\156\164\042\057\076\012\040"
        "\074\057\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124\171\160\145\076\012\040\074\157\160\143\072\123\164\162\165"
        "\143\164\165\162\145\144\124\171\160\145\040\102\141\163\145\124\171\160\145\075\042\165\141\072\105\170\164\145\156\163\151\157"
        "\156\117\142\152\145\143\164\042\040\116\141\155\145\075\042\127\157\162\153\117\162\144\145\162\124\171\160\145\042\076\012\040"
        "\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\107\165\151\144\042\040"
        "\116\141\155\145\075\042\111\104\042\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155"
        "\145\075\042\157\160\143\072\103\150\141\162\101\162\162\141\171\042\040\116\141\155\145\075\042\101\163\163\145\164\111\104\042"
        "\057\076\012\040\040\074\157\160\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\104\141"
        "\164\145\124\151\155\145\042\040\116\141\155\145\075\042\123\164\141\162\164\124\151\155\145\042\057\076\012\040\040\074\157\160"
        "\143\072\106\151\145\154\144\040\124\171\160\145\116\141\155\145\075\042\157\160\143\072\111\156\164\063\062\042\040\116\141\155"
        "\145\075\042\116\157\117\146\123\164\141\164\165\163\103\157\155\155\145\156\164\163\042\057\076\012\040\040\074\157\160\143\072"
        "\106\151\145\154\144\040\114\145\156\147\164\150\106\151\145\154\144\075\042\116\157\117\146\123\164\141\164\165\163\103\157\155"
        "\155\145\156\164\163\042\040\124\171\160\145\116\141\155\145\075\042\164\156\163\072\127\157\162\153\117\162\144\145\162\123\164"
        "\141\164\165\163\124\171\160\145\042\040\116\141\155\145\075\042\123\164\141\164\165\163\103\157\155\155\145\156\164\163\042\057"
        "\076\012\040\074\057\157\160\143\072\123\164\162\165\143\164\165\162\145\144\124\171\160\145\076\012\040\074\157\160\143\072\105"
        "\156\165\155\145\162\141\164\145\144\124\171\160\145\040\114\145\156\147\164\150\111\156\102\151\164\163\075\042\063\062\042\040"
        "\116\141\155\145\075\042\110\145\141\164\145\162\123\164\141\164\165\163\042\076\012\040\040\074\157\160\143\072\105\156\165\155"
        "\145\162\141\164\145\144\126\141\154\165\145\040\116\141\155\145\075\042\117\146\146\042\040\126\141\154\165\145\075\042\060\042"
        "\057\076\012\040\040\074\157\160\143\072\105\156\165\155\145\162\141\164\145\144\126\141\154\165\145\040\116\141\155\145\075\042"
        "\110\145\141\164\151\156\147\042\040\126\141\154\165\145\075\042\061\042\057\076\012\040\040\074\157\160\143\072\105\156\165\155"
        "\145\162\141\164\145\144\126\141\154\165\145\040\116\141\155\145\075\042\103\157\157\154\151\156\147\042\040\126\141\154\165\145"
        "\075\042\062\042\057\076\012\040\074\057\157\160\143\072\105\156\165\155\145\162\141\164\145\144\124\171\160\145\076\012\040\074"
        "\157\160\143\072\105\156\165\155\145\162\141\164\145\144\124\171\160\145\040\114\145\156\147\164\150\111\156\102\151\164\163\075"
        "\042\063\062\042\040\116\141\155\145\075\042\120\162\151\157\162\151\164\171\042\076\012\040\040\074\157\160\143\072\104\157\143"
        "\165\155\145\156\164\141\164\151\157\156\076\124\150\151\163\040\145\156\165\155\145\162\141\164\145\144\040\166\141\154\165\145"
        "\040\144\145\146\151\156\145\163\040\151\164\163\040\160\157\163\163\151\142\154\145\040\166\141\154\165\145\163\040\151\156\040"
        "\141\156\040\105\156\165\155\126\141\154\165\145\163\040\160\162\157\160\145\162\164\171\074\057\157\160\143\072\104\157\143\165"
        "\155\145\156\164\141\164\151\157\156\076\012\040\040\074\157\160\143\072\105\156\165\155\145\162\141\164\145\144\126\141\154\165"
        "\145\040\116\141\155\145\075\042\114\157\167\042\040\126\141\154\165\145\075\042\061\060\042\057\076\012\040\040\074\157\160\143"
        "\072\105\156\165\155\145\162\141\164\145\144\126\141\154\165\145\040\116\141\155\145\075\042\116\157\162\155\141\154\042\040\126"
        "\141\154\165\145\075\042\064\060\042\057\076\012\040\040\074\157\160\143\072\105\156\165\155\145\162\141\164\145\144\126\141\154"
        "\165\145\040\116\141\155\145\075\042\110\151\147\150\042\040\126\141\154\165\145\075\042\067\060\042\057\076\012\040\040\074\157"
        "\160\143\072\105\156\165\155\145\162\141\164\145\144\126\141\154\165\145\040\116\141\155\145\075\042\125\162\147\145\156\164\042"
        "\040\126\141\154\165\145\075\042\071\060\042\057\076\012\040\040\074\157\160\143\072\105\156\165\155\145\162\141\164\145\144\126"
        "\141\154\165\145\040\116\141\155\145\075\042\111\155\155\145\144\151\141\164\145\042\040\126\141\154\165\145\075\042\061\060\060"
        "\042\057\076\012\040\074\057\157\160\143\072\105\156\165\155\145\162\141\164\145\144\124\171\160\145\076\012\074\057\157\160\143"
        "\072\124\171\160\145\104\151\143\164\151\157\156\141\162\171\076\012");
    defaultValue.setByteString(bsDictionary, OpcUa_True);
    OpcUa::BaseVariableTypeGeneric *pBaseVariableType = new OpcUa::BaseVariableTypeGeneric(
        UaNodeId(DemoId_BinarySchema, getNameSpaceIndex()),
        "Demo",
        getNameSpaceIndex(),
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();
    pBaseVariableType->setTypeDefinitionId(OpcUaId_DataTypeDictionaryType);

    ret = addNodeAndReference(OpcUaId_OPCBinarySchema_TypeSystem, pBaseVariableType, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());

    defaultValue.setString("AccessRights");
    OpcUa::BaseVariableTypeGeneric *pDemoId_BinarySchema_AccessRights = new OpcUa::BaseVariableTypeGeneric(
        UaNodeId(DemoId_BinarySchema_AccessRights, getNameSpaceIndex()),
        "AccessRights",
        getNameSpaceIndex(),
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();
    pDemoId_BinarySchema_AccessRights->setTypeDefinitionId(OpcUaId_DataTypeDescriptionType);

    ret = addNodeAndReference(pBaseVariableType, pDemoId_BinarySchema_AccessRights, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_AccessRights_Encoding_DefaultBinary, getNameSpaceIndex()), pDemoId_BinarySchema_AccessRights->nodeId(), OpcUaId_HasDescription);
    UA_ASSERT(ret.isGood());

    defaultValue.setString("StructureWithOptionalFields");
    OpcUa::BaseVariableTypeGeneric *pDemoId_BinarySchema_StructureWithOptionalFields = new OpcUa::BaseVariableTypeGeneric(
        UaNodeId(DemoId_BinarySchema_StructureWithOptionalFields, getNameSpaceIndex()),
        "StructureWithOptionalFields",
        getNameSpaceIndex(),
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();
    pDemoId_BinarySchema_StructureWithOptionalFields->setTypeDefinitionId(OpcUaId_DataTypeDescriptionType);

    ret = addNodeAndReference(pBaseVariableType, pDemoId_BinarySchema_StructureWithOptionalFields, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_StructureWithOptionalFields_Encoding_DefaultBinary, getNameSpaceIndex()), pDemoId_BinarySchema_StructureWithOptionalFields->nodeId(), OpcUaId_HasDescription);
    UA_ASSERT(ret.isGood());

    defaultValue.setString("UnionTest");
    OpcUa::BaseVariableTypeGeneric *pDemoId_BinarySchema_UnionTest = new OpcUa::BaseVariableTypeGeneric(
        UaNodeId(DemoId_BinarySchema_UnionTest, getNameSpaceIndex()),
        "UnionTest",
        getNameSpaceIndex(),
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();
    pDemoId_BinarySchema_UnionTest->setTypeDefinitionId(OpcUaId_DataTypeDescriptionType);

    ret = addNodeAndReference(pBaseVariableType, pDemoId_BinarySchema_UnionTest, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_UnionTest_Encoding_DefaultBinary, getNameSpaceIndex()), pDemoId_BinarySchema_UnionTest->nodeId(), OpcUaId_HasDescription);
    UA_ASSERT(ret.isGood());

    defaultValue.setString("Vector");
    OpcUa::BaseVariableTypeGeneric *pDemoId_BinarySchema_Vector = new OpcUa::BaseVariableTypeGeneric(
        UaNodeId(DemoId_BinarySchema_Vector, getNameSpaceIndex()),
        "Vector",
        getNameSpaceIndex(),
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();
    pDemoId_BinarySchema_Vector->setTypeDefinitionId(OpcUaId_DataTypeDescriptionType);

    ret = addNodeAndReference(pBaseVariableType, pDemoId_BinarySchema_Vector, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Vector_Encoding_DefaultBinary, getNameSpaceIndex()), pDemoId_BinarySchema_Vector->nodeId(), OpcUaId_HasDescription);
    UA_ASSERT(ret.isGood());

    defaultValue.setString("WorkOrderStatusType");
    OpcUa::BaseVariableTypeGeneric *pDemoId_BinarySchema_WorkOrderStatusType = new OpcUa::BaseVariableTypeGeneric(
        UaNodeId(DemoId_BinarySchema_WorkOrderStatusType, getNameSpaceIndex()),
        "WorkOrderStatusType",
        getNameSpaceIndex(),
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();
    pDemoId_BinarySchema_WorkOrderStatusType->setTypeDefinitionId(OpcUaId_DataTypeDescriptionType);

    ret = addNodeAndReference(pBaseVariableType, pDemoId_BinarySchema_WorkOrderStatusType, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_WorkOrderStatusType_Encoding_DefaultBinary, getNameSpaceIndex()), pDemoId_BinarySchema_WorkOrderStatusType->nodeId(), OpcUaId_HasDescription);
    UA_ASSERT(ret.isGood());

    defaultValue.setString("WorkOrderType");
    OpcUa::BaseVariableTypeGeneric *pDemoId_BinarySchema_WorkOrderType = new OpcUa::BaseVariableTypeGeneric(
        UaNodeId(DemoId_BinarySchema_WorkOrderType, getNameSpaceIndex()),
        "WorkOrderType",
        getNameSpaceIndex(),
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();
    pDemoId_BinarySchema_WorkOrderType->setTypeDefinitionId(OpcUaId_DataTypeDescriptionType);

    ret = addNodeAndReference(pBaseVariableType, pDemoId_BinarySchema_WorkOrderType, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_WorkOrderType_Encoding_DefaultBinary, getNameSpaceIndex()), pDemoId_BinarySchema_WorkOrderType->nodeId(), OpcUaId_HasDescription);
    UA_ASSERT(ret.isGood());

    defaultValue.setString("http://www.unifiedautomation.com/DemoServer/");
    OpcUa::PropertyType *pDemoId_BinarySchema_NamespaceUri = new OpcUa::PropertyType(
        UaNodeId(DemoId_BinarySchema_NamespaceUri, getNameSpaceIndex()),
        "NamespaceUri",
        0,
        defaultValue,
        OpcUa_AccessLevels_CurrentRead,
        this);
    defaultValue.clear();

    ret = addNodeAndReference(pBaseVariableType, pDemoId_BinarySchema_NamespaceUri, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    return ret;
}

/** Helper function to create reference type related nodes.
*/
UaStatus NodeManagerDemoBase::createReferenceTypeNodes()
{
    UaStatus                ret;

    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemo(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDemo = new UaBase::Object(UaNodeId(DemoId_Demo, getTypeNamespace()));
    pDemo->setBrowseName(UaQualifiedName("Demo", getTypeNamespace()));
    pDemo->setDisplayName(UaLocalizedText("", "Demo"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pDemo, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDemo;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStatic(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pStatic = new UaBase::Object(UaNodeId(DemoId_Demo_Static, getTypeNamespace()));
    pStatic->setBrowseName(UaQualifiedName("Static", getTypeNamespace()));
    pStatic->setDisplayName(UaLocalizedText("", "000_Static"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pStatic, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStatic;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArrays(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pArrays = new UaBase::Object(UaNodeId(DemoId_Demo_Static_Arrays, getTypeNamespace()));
    pArrays->setBrowseName(UaQualifiedName("Arrays", getTypeNamespace()));
    pArrays->setDisplayName(UaLocalizedText("", "Arrays"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pArrays, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pArrays;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysBaseDataType(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBaseDataType = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_BaseDataType, getTypeNamespace()));
    pBaseDataType->setBrowseName(UaQualifiedName("BaseDataType", getTypeNamespace()));
    pBaseDataType->setDisplayName(UaLocalizedText("", "BaseDataType"));
    pBaseDataType->setAccessLevel(3);
    pBaseDataType->setUserAccessLevel(3);
    pBaseDataType->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pBaseDataType->setArrayDimensions(arrayDimensions);
    pBaseDataType->setDataTypeId(OpcUaId_BaseDataType);
    {
        UaVariantArray valBaseDataTypeArray;
        defaultValue.setVariantArray(valBaseDataTypeArray);
    }
    pBaseDataType->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBaseDataType, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBaseDataType;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysBoolean(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBoolean = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Boolean, getTypeNamespace()));
    pBoolean->setBrowseName(UaQualifiedName("Boolean", getTypeNamespace()));
    pBoolean->setDisplayName(UaLocalizedText("", "Boolean"));
    pBoolean->setAccessLevel(3);
    pBoolean->setUserAccessLevel(3);
    pBoolean->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pBoolean->setArrayDimensions(arrayDimensions);
    pBoolean->setDataTypeId(OpcUaId_Boolean);
    {
        UaBooleanArray valBooleanArray;
        defaultValue.setBoolArray(valBooleanArray);
    }
    pBoolean->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBoolean, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBoolean;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pByte->setArrayDimensions(arrayDimensions);
    pByte->setDataTypeId(OpcUaId_Byte);
    {
        UaByteArray valByteArray;
        defaultValue.setByteArray(valByteArray);
    }
    pByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysByteString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByteString = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_ByteString, getTypeNamespace()));
    pByteString->setBrowseName(UaQualifiedName("ByteString", getTypeNamespace()));
    pByteString->setDisplayName(UaLocalizedText("", "ByteString"));
    pByteString->setAccessLevel(3);
    pByteString->setUserAccessLevel(3);
    pByteString->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pByteString->setArrayDimensions(arrayDimensions);
    pByteString->setDataTypeId(OpcUaId_ByteString);
    {
        UaByteStringArray valByteStringArray;
        defaultValue.setByteStringArray(valByteStringArray);
    }
    pByteString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByteString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByteString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysDateTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDateTime = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_DateTime, getTypeNamespace()));
    pDateTime->setBrowseName(UaQualifiedName("DateTime", getTypeNamespace()));
    pDateTime->setDisplayName(UaLocalizedText("", "DateTime"));
    pDateTime->setAccessLevel(3);
    pDateTime->setUserAccessLevel(3);
    pDateTime->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDateTime->setArrayDimensions(arrayDimensions);
    pDateTime->setDataTypeId(OpcUaId_DateTime);
    {
        UaDateTimeArray valDateTimeArray;
        defaultValue.setDateTimeArray(valDateTimeArray);
    }
    pDateTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDateTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDateTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDouble->setArrayDimensions(arrayDimensions);
    pDouble->setDataTypeId(OpcUaId_Double);
    {
        UaDoubleArray valDoubleArray;
        defaultValue.setDoubleArray(valDoubleArray);
    }
    pDouble->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysDuration(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDuration = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Duration, getTypeNamespace()));
    pDuration->setBrowseName(UaQualifiedName("Duration", getTypeNamespace()));
    pDuration->setDisplayName(UaLocalizedText("", "Duration"));
    pDuration->setAccessLevel(3);
    pDuration->setUserAccessLevel(3);
    pDuration->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDuration->setArrayDimensions(arrayDimensions);
    pDuration->setDataTypeId(OpcUaId_Duration);
    {
        UaDoubleArray valDurationArray;
        defaultValue.setDoubleArray(valDurationArray);
    }
    pDuration->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDuration, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDuration;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysExpandedNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pExpandedNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_ExpandedNodeId, getTypeNamespace()));
    pExpandedNodeId->setBrowseName(UaQualifiedName("ExpandedNodeId", getTypeNamespace()));
    pExpandedNodeId->setDisplayName(UaLocalizedText("", "ExpandedNodeId"));
    pExpandedNodeId->setAccessLevel(3);
    pExpandedNodeId->setUserAccessLevel(3);
    pExpandedNodeId->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pExpandedNodeId->setArrayDimensions(arrayDimensions);
    pExpandedNodeId->setDataTypeId(OpcUaId_ExpandedNodeId);
    {
        UaExpandedNodeIdArray valExpandedNodeIdArray;
        defaultValue.setExpandedNodeIdArray(valExpandedNodeIdArray);
    }
    pExpandedNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pExpandedNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pExpandedNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pFloat->setArrayDimensions(arrayDimensions);
    pFloat->setDataTypeId(OpcUaId_Float);
    {
        UaFloatArray valFloatArray;
        defaultValue.setFloatArray(valFloatArray);
    }
    pFloat->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysGuid(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pGuid = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Guid, getTypeNamespace()));
    pGuid->setBrowseName(UaQualifiedName("Guid", getTypeNamespace()));
    pGuid->setDisplayName(UaLocalizedText("", "Guid"));
    pGuid->setAccessLevel(3);
    pGuid->setUserAccessLevel(3);
    pGuid->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pGuid->setArrayDimensions(arrayDimensions);
    pGuid->setDataTypeId(OpcUaId_Guid);
    {
        UaGuidArray valGuidArray;
        defaultValue.setGuidArray(valGuidArray);
    }
    pGuid->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pGuid, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pGuid;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysImagePNG(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImagePNG = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_ImagePNG, getTypeNamespace()));
    pImagePNG->setBrowseName(UaQualifiedName("ImagePNG", getTypeNamespace()));
    pImagePNG->setDisplayName(UaLocalizedText("", "ImagePNG"));
    pImagePNG->setAccessLevel(3);
    pImagePNG->setUserAccessLevel(3);
    pImagePNG->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pImagePNG->setArrayDimensions(arrayDimensions);
    pImagePNG->setDataTypeId(OpcUaId_ImagePNG);
    {
        UaByteStringArray valImagePNGArray;
        defaultValue.setByteStringArray(valImagePNGArray);
    }
    pImagePNG->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImagePNG, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImagePNG;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt16->setArrayDimensions(arrayDimensions);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        UaInt16Array valInt16Array;
        defaultValue.setInt16Array(valInt16Array);
    }
    pInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt32->setArrayDimensions(arrayDimensions);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        UaInt32Array valInt32Array;
        defaultValue.setInt32Array(valInt32Array);
    }
    pInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt64->setArrayDimensions(arrayDimensions);
    pInt64->setDataTypeId(OpcUaId_Int64);
    {
        UaInt64Array valInt64Array;
        defaultValue.setInt64Array(valInt64Array);
    }
    pInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysInteger(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInteger = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Integer, getTypeNamespace()));
    pInteger->setBrowseName(UaQualifiedName("Integer", getTypeNamespace()));
    pInteger->setDisplayName(UaLocalizedText("", "Integer"));
    pInteger->setAccessLevel(3);
    pInteger->setUserAccessLevel(3);
    pInteger->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInteger->setArrayDimensions(arrayDimensions);
    pInteger->setDataTypeId(OpcUaId_Integer);
    {
        UaVariantArray valIntegerArray;
        defaultValue.setVariantArray(valIntegerArray);
    }
    pInteger->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInteger, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInteger;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysLocalizedText(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pLocalizedText = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_LocalizedText, getTypeNamespace()));
    pLocalizedText->setBrowseName(UaQualifiedName("LocalizedText", getTypeNamespace()));
    pLocalizedText->setDisplayName(UaLocalizedText("", "LocalizedText"));
    pLocalizedText->setAccessLevel(3);
    pLocalizedText->setUserAccessLevel(3);
    pLocalizedText->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pLocalizedText->setArrayDimensions(arrayDimensions);
    pLocalizedText->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray valLocalizedTextArray;
        defaultValue.setLocalizedTextArray(valLocalizedTextArray);
    }
    pLocalizedText->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pLocalizedText, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pLocalizedText;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_NodeId, getTypeNamespace()));
    pNodeId->setBrowseName(UaQualifiedName("NodeId", getTypeNamespace()));
    pNodeId->setDisplayName(UaLocalizedText("", "NodeId"));
    pNodeId->setAccessLevel(3);
    pNodeId->setUserAccessLevel(3);
    pNodeId->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pNodeId->setArrayDimensions(arrayDimensions);
    pNodeId->setDataTypeId(OpcUaId_NodeId);
    {
        UaNodeIdArray valNodeIdArray;
        defaultValue.setNodeIdArray(valNodeIdArray);
    }
    pNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysNumber(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNumber = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Number, getTypeNamespace()));
    pNumber->setBrowseName(UaQualifiedName("Number", getTypeNamespace()));
    pNumber->setDisplayName(UaLocalizedText("", "Number"));
    pNumber->setAccessLevel(3);
    pNumber->setUserAccessLevel(3);
    pNumber->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pNumber->setArrayDimensions(arrayDimensions);
    pNumber->setDataTypeId(OpcUaId_Number);
    pNumber->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNumber, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNumber;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysNumericRange(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNumericRange = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_NumericRange, getTypeNamespace()));
    pNumericRange->setBrowseName(UaQualifiedName("NumericRange", getTypeNamespace()));
    pNumericRange->setDisplayName(UaLocalizedText("", "NumericRange"));
    pNumericRange->setAccessLevel(3);
    pNumericRange->setUserAccessLevel(3);
    pNumericRange->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pNumericRange->setArrayDimensions(arrayDimensions);
    pNumericRange->setDataTypeId(OpcUaId_NumericRange);
    {
        UaStringArray valNumericRangeArray;
        defaultValue.setStringArray(valNumericRangeArray);
    }
    pNumericRange->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNumericRange, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNumericRange;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysQualifiedName(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQualifiedName = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_QualifiedName, getTypeNamespace()));
    pQualifiedName->setBrowseName(UaQualifiedName("QualifiedName", getTypeNamespace()));
    pQualifiedName->setDisplayName(UaLocalizedText("", "QualifiedName"));
    pQualifiedName->setAccessLevel(3);
    pQualifiedName->setUserAccessLevel(3);
    pQualifiedName->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pQualifiedName->setArrayDimensions(arrayDimensions);
    pQualifiedName->setDataTypeId(OpcUaId_QualifiedName);
    {
        UaQualifiedNameArray valQualifiedNameArray;
        defaultValue.setQualifiedNameArray(valQualifiedNameArray);
    }
    pQualifiedName->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQualifiedName, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQualifiedName;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pSByte->setArrayDimensions(arrayDimensions);
    pSByte->setDataTypeId(OpcUaId_SByte);
    {
        UaSByteArray valSByteArray;
        defaultValue.setSByteArray(valSByteArray);
    }
    pSByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysServerState(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pServerState = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_ServerState, getTypeNamespace()));
    pServerState->setBrowseName(UaQualifiedName("ServerState", getTypeNamespace()));
    pServerState->setDisplayName(UaLocalizedText("", "ServerState"));
    pServerState->setAccessLevel(3);
    pServerState->setUserAccessLevel(3);
    pServerState->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pServerState->setArrayDimensions(arrayDimensions);
    pServerState->setDataTypeId(OpcUaId_ServerState);
    {
        UaInt32Array valServerStateArray;
        defaultValue.setInt32Array(valServerStateArray);
    }
    pServerState->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pServerState, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pServerState;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysStatusCode(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pStatusCode = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_StatusCode, getTypeNamespace()));
    pStatusCode->setBrowseName(UaQualifiedName("StatusCode", getTypeNamespace()));
    pStatusCode->setDisplayName(UaLocalizedText("", "StatusCode"));
    pStatusCode->setAccessLevel(3);
    pStatusCode->setUserAccessLevel(3);
    pStatusCode->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pStatusCode->setArrayDimensions(arrayDimensions);
    pStatusCode->setDataTypeId(OpcUaId_StatusCode);
    {
        UaStatusCodeArray valStatusCodeArray;
        defaultValue.setStatusCodeArray(valStatusCodeArray);
    }
    pStatusCode->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pStatusCode, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStatusCode;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pString = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_String, getTypeNamespace()));
    pString->setBrowseName(UaQualifiedName("String", getTypeNamespace()));
    pString->setDisplayName(UaLocalizedText("", "String"));
    pString->setAccessLevel(3);
    pString->setUserAccessLevel(3);
    pString->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pString->setArrayDimensions(arrayDimensions);
    pString->setDataTypeId(OpcUaId_String);
    {
        UaStringArray valStringArray;
        defaultValue.setStringArray(valStringArray);
    }
    pString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysStructure(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pStructure = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Structure, getTypeNamespace()));
    pStructure->setBrowseName(UaQualifiedName("Structure", getTypeNamespace()));
    pStructure->setDisplayName(UaLocalizedText("", "Structure"));
    pStructure->setAccessLevel(3);
    pStructure->setUserAccessLevel(3);
    pStructure->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pStructure->setArrayDimensions(arrayDimensions);
    pStructure->setDataTypeId(OpcUaId_Structure);
    {
        UaExtensionObjectArray valStructureArray;
        defaultValue.setExtensionObjectArray(valStructureArray);
    }
    pStructure->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pStructure, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStructure;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt16->setArrayDimensions(arrayDimensions);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        UaUInt16Array valUInt16Array;
        defaultValue.setUInt16Array(valUInt16Array);
    }
    pUInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt32->setArrayDimensions(arrayDimensions);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        UaUInt32Array valUInt32Array;
        defaultValue.setUInt32Array(valUInt32Array);
    }
    pUInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt64->setArrayDimensions(arrayDimensions);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
    {
        UaUInt64Array valUInt64Array;
        defaultValue.setUInt64Array(valUInt64Array);
    }
    pUInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysUInteger(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInteger = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_UInteger, getTypeNamespace()));
    pUInteger->setBrowseName(UaQualifiedName("UInteger", getTypeNamespace()));
    pUInteger->setDisplayName(UaLocalizedText("", "UInteger"));
    pUInteger->setAccessLevel(3);
    pUInteger->setUserAccessLevel(3);
    pUInteger->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInteger->setArrayDimensions(arrayDimensions);
    pUInteger->setDataTypeId(OpcUaId_UInteger);
    {
        UaVariantArray valUIntegerArray;
        defaultValue.setVariantArray(valUIntegerArray);
    }
    pUInteger->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInteger, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInteger;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysUtcTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUtcTime = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_UtcTime, getTypeNamespace()));
    pUtcTime->setBrowseName(UaQualifiedName("UtcTime", getTypeNamespace()));
    pUtcTime->setDisplayName(UaLocalizedText("", "UtcTime"));
    pUtcTime->setAccessLevel(3);
    pUtcTime->setUserAccessLevel(3);
    pUtcTime->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUtcTime->setArrayDimensions(arrayDimensions);
    pUtcTime->setDataTypeId(OpcUaId_UtcTime);
    {
        UaDateTimeArray valUtcTimeArray;
        defaultValue.setDateTimeArray(valUtcTimeArray);
    }
    pUtcTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUtcTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUtcTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysVector(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pVector = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_Vector, getTypeNamespace()));
    pVector->setBrowseName(UaQualifiedName("Vector", getTypeNamespace()));
    pVector->setDisplayName(UaLocalizedText("", "Vector"));
    pVector->setAccessLevel(3);
    pVector->setUserAccessLevel(3);
    pVector->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pVector->setArrayDimensions(arrayDimensions);
    pVector->setDataTypeId(UaNodeId(DemoId_Vector, getNameSpaceIndex()));
    {
        Demo::Vectors valVectorArray;
        valVectorArray.toVariant(defaultValue);
    }
    pVector->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pVector, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pVector;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysWorkOrder(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pWorkOrder = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_WorkOrder, getTypeNamespace()));
    pWorkOrder->setBrowseName(UaQualifiedName("WorkOrder", getTypeNamespace()));
    pWorkOrder->setDisplayName(UaLocalizedText("", "WorkOrder"));
    pWorkOrder->setAccessLevel(3);
    pWorkOrder->setUserAccessLevel(3);
    pWorkOrder->setValueRank(1);
    pWorkOrder->setDataTypeId(UaNodeId(DemoId_WorkOrderType, getNameSpaceIndex()));
    {
        Demo::WorkOrderTypes valWorkOrderArray;
        valWorkOrderArray.toVariant(defaultValue);
    }
    pWorkOrder->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pWorkOrder, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pWorkOrder;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticArraysXmlElement(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pXmlElement = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Arrays_XmlElement, getTypeNamespace()));
    pXmlElement->setBrowseName(UaQualifiedName("XmlElement", getTypeNamespace()));
    pXmlElement->setDisplayName(UaLocalizedText("", "XmlElement"));
    pXmlElement->setAccessLevel(3);
    pXmlElement->setUserAccessLevel(3);
    pXmlElement->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pXmlElement->setArrayDimensions(arrayDimensions);
    pXmlElement->setDataTypeId(OpcUaId_XmlElement);
    {
        UaXmlElementArray valXmlElementArray;
        defaultValue.setXmlElementArray(valXmlElementArray);
    }
    pXmlElement->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pXmlElement, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pXmlElement;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalar(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pScalar = new UaBase::Object(UaNodeId(DemoId_Demo_Static_Scalar, getTypeNamespace()));
    pScalar->setBrowseName(UaQualifiedName("Scalar", getTypeNamespace()));
    pScalar->setDisplayName(UaLocalizedText("", "Scalar"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pScalar, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pScalar;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarBaseDataType(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBaseDataType = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_BaseDataType, getTypeNamespace()));
    pBaseDataType->setBrowseName(UaQualifiedName("BaseDataType", getTypeNamespace()));
    pBaseDataType->setDisplayName(UaLocalizedText("", "BaseDataType"));
    pBaseDataType->setAccessLevel(3);
    pBaseDataType->setUserAccessLevel(3);
    pBaseDataType->setValueRank(-1);
    pBaseDataType->setDataTypeId(OpcUaId_BaseDataType);
        defaultValue.clear();
    pBaseDataType->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBaseDataType, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBaseDataType;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarBoolean(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBoolean = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Boolean, getTypeNamespace()));
    pBoolean->setBrowseName(UaQualifiedName("Boolean", getTypeNamespace()));
    pBoolean->setDisplayName(UaLocalizedText("", "Boolean"));
    pBoolean->setAccessLevel(3);
    pBoolean->setUserAccessLevel(3);
    pBoolean->setValueRank(-1);
    pBoolean->setDataTypeId(OpcUaId_Boolean);
        defaultValue.setBool(OpcUa_False);
    pBoolean->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBoolean, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBoolean;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(-1);
    pByte->setDataTypeId(OpcUaId_Byte);
        defaultValue.setByte(0);
    pByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarByteString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByteString = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_ByteString, getTypeNamespace()));
    pByteString->setBrowseName(UaQualifiedName("ByteString", getTypeNamespace()));
    pByteString->setDisplayName(UaLocalizedText("", "ByteString"));
    pByteString->setAccessLevel(3);
    pByteString->setUserAccessLevel(3);
    pByteString->setValueRank(-1);
    pByteString->setDataTypeId(OpcUaId_ByteString);
        { UaByteString tmpByteString; defaultValue.setByteString(tmpByteString, OpcUa_False); };
    pByteString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByteString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByteString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarDateTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDateTime = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_DateTime, getTypeNamespace()));
    pDateTime->setBrowseName(UaQualifiedName("DateTime", getTypeNamespace()));
    pDateTime->setDisplayName(UaLocalizedText("", "DateTime"));
    pDateTime->setAccessLevel(3);
    pDateTime->setUserAccessLevel(3);
    pDateTime->setValueRank(-1);
    pDateTime->setDataTypeId(OpcUaId_DateTime);
        defaultValue.setDateTime(UaDateTime());
    pDateTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDateTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDateTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(-1);
    pDouble->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pDouble->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarDuration(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDuration = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Duration, getTypeNamespace()));
    pDuration->setBrowseName(UaQualifiedName("Duration", getTypeNamespace()));
    pDuration->setDisplayName(UaLocalizedText("", "Duration"));
    pDuration->setAccessLevel(3);
    pDuration->setUserAccessLevel(3);
    pDuration->setValueRank(-1);
    pDuration->setDataTypeId(OpcUaId_Duration);
        defaultValue.setDouble(0.0);
    pDuration->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDuration, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDuration;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarExpandedNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pExpandedNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_ExpandedNodeId, getTypeNamespace()));
    pExpandedNodeId->setBrowseName(UaQualifiedName("ExpandedNodeId", getTypeNamespace()));
    pExpandedNodeId->setDisplayName(UaLocalizedText("", "ExpandedNodeId"));
    pExpandedNodeId->setAccessLevel(3);
    pExpandedNodeId->setUserAccessLevel(3);
    pExpandedNodeId->setValueRank(-1);
    pExpandedNodeId->setDataTypeId(OpcUaId_ExpandedNodeId);
        defaultValue.setExpandedNodeId(UaExpandedNodeId());
    pExpandedNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pExpandedNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pExpandedNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(-1);
    pFloat->setDataTypeId(OpcUaId_Float);
        defaultValue.setFloat(0.0);
    pFloat->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarGuid(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pGuid = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Guid, getTypeNamespace()));
    pGuid->setBrowseName(UaQualifiedName("Guid", getTypeNamespace()));
    pGuid->setDisplayName(UaLocalizedText("", "Guid"));
    pGuid->setAccessLevel(3);
    pGuid->setUserAccessLevel(3);
    pGuid->setValueRank(-1);
    pGuid->setDataTypeId(OpcUaId_Guid);
        defaultValue.setGuid(UaGuid());
    pGuid->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pGuid, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pGuid;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarHeaterStatus(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pHeaterStatus = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_HeaterStatus, getTypeNamespace()));
    pHeaterStatus->setBrowseName(UaQualifiedName("HeaterStatus", getTypeNamespace()));
    pHeaterStatus->setDisplayName(UaLocalizedText("", "HeaterStatus"));
    pHeaterStatus->setAccessLevel(3);
    pHeaterStatus->setUserAccessLevel(3);
    pHeaterStatus->setValueRank(-1);
    pHeaterStatus->setDataTypeId(UaNodeId(DemoId_HeaterStatus, getNameSpaceIndex()));
        defaultValue.setInt32((OpcUa_Int32) Demo::HeaterStatus_Off);
    pHeaterStatus->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pHeaterStatus, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHeaterStatus;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarImagePNG(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImagePNG = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_ImagePNG, getTypeNamespace()));
    pImagePNG->setBrowseName(UaQualifiedName("ImagePNG", getTypeNamespace()));
    pImagePNG->setDisplayName(UaLocalizedText("", "ImagePNG"));
    pImagePNG->setAccessLevel(3);
    pImagePNG->setUserAccessLevel(3);
    pImagePNG->setValueRank(-1);
    pImagePNG->setDataTypeId(OpcUaId_ImagePNG);
        { UaByteString tmpImagePNG; defaultValue.setByteString(tmpImagePNG, OpcUa_False); };
    pImagePNG->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImagePNG, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImagePNG;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(-1);
    pInt16->setDataTypeId(OpcUaId_Int16);
        defaultValue.setInt16(0);
    pInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(-1);
    pInt32->setDataTypeId(OpcUaId_Int32);
        defaultValue.setInt32(0);
    pInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(-1);
    pInt64->setDataTypeId(OpcUaId_Int64);
        defaultValue.setInt64(0);
    pInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarInteger(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInteger = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Integer, getTypeNamespace()));
    pInteger->setBrowseName(UaQualifiedName("Integer", getTypeNamespace()));
    pInteger->setDisplayName(UaLocalizedText("", "Integer"));
    pInteger->setAccessLevel(3);
    pInteger->setUserAccessLevel(3);
    pInteger->setValueRank(-1);
    pInteger->setDataTypeId(OpcUaId_Integer);
        defaultValue.clear();
    pInteger->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInteger, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInteger;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarLocalizedText(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pLocalizedText = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_LocalizedText, getTypeNamespace()));
    pLocalizedText->setBrowseName(UaQualifiedName("LocalizedText", getTypeNamespace()));
    pLocalizedText->setDisplayName(UaLocalizedText("", "LocalizedText"));
    pLocalizedText->setAccessLevel(3);
    pLocalizedText->setUserAccessLevel(3);
    pLocalizedText->setValueRank(-1);
    pLocalizedText->setDataTypeId(OpcUaId_LocalizedText);
        defaultValue.setLocalizedText(UaLocalizedText());
    pLocalizedText->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pLocalizedText, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pLocalizedText;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_NodeId, getTypeNamespace()));
    pNodeId->setBrowseName(UaQualifiedName("NodeId", getTypeNamespace()));
    pNodeId->setDisplayName(UaLocalizedText("", "NodeId"));
    pNodeId->setAccessLevel(3);
    pNodeId->setUserAccessLevel(3);
    pNodeId->setValueRank(-1);
    pNodeId->setDataTypeId(OpcUaId_NodeId);
        defaultValue.setNodeId(UaNodeId());
    pNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarNumber(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNumber = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Number, getTypeNamespace()));
    pNumber->setBrowseName(UaQualifiedName("Number", getTypeNamespace()));
    pNumber->setDisplayName(UaLocalizedText("", "Number"));
    pNumber->setAccessLevel(3);
    pNumber->setUserAccessLevel(3);
    pNumber->setValueRank(-1);
    pNumber->setDataTypeId(OpcUaId_Number);
    defaultValue.clear();
    pNumber->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNumber, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNumber;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarNumericRange(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNumericRange = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_NumericRange, getTypeNamespace()));
    pNumericRange->setBrowseName(UaQualifiedName("NumericRange", getTypeNamespace()));
    pNumericRange->setDisplayName(UaLocalizedText("", "NumericRange"));
    pNumericRange->setAccessLevel(3);
    pNumericRange->setUserAccessLevel(3);
    pNumericRange->setValueRank(-1);
    pNumericRange->setDataTypeId(OpcUaId_NumericRange);
        defaultValue.setString("");
    pNumericRange->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNumericRange, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNumericRange;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarPriority(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pPriority = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Priority, getTypeNamespace()));
    pPriority->setBrowseName(UaQualifiedName("Priority", getTypeNamespace()));
    pPriority->setDisplayName(UaLocalizedText("", "Priority"));
    pPriority->setAccessLevel(3);
    pPriority->setUserAccessLevel(3);
    pPriority->setValueRank(-1);
    pPriority->setDataTypeId(UaNodeId(DemoId_Priority, getNameSpaceIndex()));
        defaultValue.setInt32((OpcUa_Int32) Demo::Priority_Low);
    pPriority->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pPriority, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pPriority;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarQualifiedName(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQualifiedName = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_QualifiedName, getTypeNamespace()));
    pQualifiedName->setBrowseName(UaQualifiedName("QualifiedName", getTypeNamespace()));
    pQualifiedName->setDisplayName(UaLocalizedText("", "QualifiedName"));
    pQualifiedName->setAccessLevel(3);
    pQualifiedName->setUserAccessLevel(3);
    pQualifiedName->setValueRank(-1);
    pQualifiedName->setDataTypeId(OpcUaId_QualifiedName);
        defaultValue.setQualifiedName(UaQualifiedName());
    pQualifiedName->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQualifiedName, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQualifiedName;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(-1);
    pSByte->setDataTypeId(OpcUaId_SByte);
        defaultValue.setSByte(0);
    pSByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarServerState(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pServerState = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_ServerState, getTypeNamespace()));
    pServerState->setBrowseName(UaQualifiedName("ServerState", getTypeNamespace()));
    pServerState->setDisplayName(UaLocalizedText("", "ServerState"));
    pServerState->setAccessLevel(3);
    pServerState->setUserAccessLevel(3);
    pServerState->setValueRank(-1);
    pServerState->setDataTypeId(OpcUaId_ServerState);
        defaultValue.setInt32((OpcUa_Int32) OpcUa_ServerState_Running);
    pServerState->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pServerState, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pServerState;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStatusCode(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pStatusCode = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_StatusCode, getTypeNamespace()));
    pStatusCode->setBrowseName(UaQualifiedName("StatusCode", getTypeNamespace()));
    pStatusCode->setDisplayName(UaLocalizedText("", "StatusCode"));
    pStatusCode->setAccessLevel(3);
    pStatusCode->setUserAccessLevel(3);
    pStatusCode->setValueRank(-1);
    pStatusCode->setDataTypeId(OpcUaId_StatusCode);
        defaultValue.setStatusCode(0);
    pStatusCode->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pStatusCode, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStatusCode;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pString = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_String, getTypeNamespace()));
    pString->setBrowseName(UaQualifiedName("String", getTypeNamespace()));
    pString->setDisplayName(UaLocalizedText("", "String"));
    pString->setAccessLevel(3);
    pString->setUserAccessLevel(3);
    pString->setValueRank(-1);
    pString->setDataTypeId(OpcUaId_String);
        defaultValue.setString("");
    pString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStructures(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pStructures = new UaBase::Object(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getTypeNamespace()));
    pStructures->setBrowseName(UaQualifiedName("Structures", getTypeNamespace()));
    pStructures->setDisplayName(UaLocalizedText("", "Structures"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pStructures, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStructures;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStructuresOptionalFields(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pOptionalFields = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Structures_OptionalFields, getTypeNamespace()));
    pOptionalFields->setBrowseName(UaQualifiedName("OptionalFields", getTypeNamespace()));
    pOptionalFields->setDisplayName(UaLocalizedText("", "OptionalFields"));
    pOptionalFields->setAccessLevel(3);
    pOptionalFields->setUserAccessLevel(3);
    pOptionalFields->setValueRank(-1);
    pOptionalFields->setDataTypeId(UaNodeId(DemoId_StructureWithOptionalFields, getNameSpaceIndex()));
    {
        Demo::StructureWithOptionalFields valOptionalFields;
        valOptionalFields.toVariant(defaultValue);
    }
    pOptionalFields->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pOptionalFields, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pOptionalFields;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStructuresOptionSet(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pOptionSet = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Structures_OptionSet, getTypeNamespace()));
    pOptionSet->setBrowseName(UaQualifiedName("OptionSet", getTypeNamespace()));
    pOptionSet->setDisplayName(UaLocalizedText("", "OptionSet"));
    pOptionSet->setAccessLevel(3);
    pOptionSet->setUserAccessLevel(3);
    pOptionSet->setValueRank(-1);
    pOptionSet->setDataTypeId(UaNodeId(DemoId_AccessRights, getNameSpaceIndex()));
    {
        Demo::AccessRights valOptionSet;
        valOptionSet.toVariant(defaultValue);
    }
    pOptionSet->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pOptionSet, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pOptionSet;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStructuresStructure(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pStructure = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Structures_Structure, getTypeNamespace()));
    pStructure->setBrowseName(UaQualifiedName("Structure", getTypeNamespace()));
    pStructure->setDisplayName(UaLocalizedText("", "Structure"));
    pStructure->setAccessLevel(3);
    pStructure->setUserAccessLevel(3);
    pStructure->setValueRank(-1);
    pStructure->setDataTypeId(OpcUaId_Structure);
        { UaExtensionObject tmpStructure; defaultValue.setExtensionObject(tmpStructure, OpcUa_False); };
    pStructure->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pStructure, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStructure;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStructuresUnion(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUnion = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Structures_Union, getTypeNamespace()));
    pUnion->setBrowseName(UaQualifiedName("Union", getTypeNamespace()));
    pUnion->setDisplayName(UaLocalizedText("", "Union"));
    pUnion->setAccessLevel(3);
    pUnion->setUserAccessLevel(3);
    pUnion->setValueRank(-1);
    pUnion->setDataTypeId(UaNodeId(DemoId_UnionTest, getNameSpaceIndex()));
    {
        Demo::UnionTest valUnion;
        valUnion.toVariant(defaultValue);
    }
    pUnion->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUnion, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUnion;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStructuresVector(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pVector = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Structures_Vector, getTypeNamespace()));
    pVector->setBrowseName(UaQualifiedName("Vector", getTypeNamespace()));
    pVector->setDisplayName(UaLocalizedText("", "Vector"));
    pVector->setAccessLevel(3);
    pVector->setUserAccessLevel(3);
    pVector->setValueRank(-1);
    pVector->setDataTypeId(UaNodeId(DemoId_Vector, getNameSpaceIndex()));
    {
        Demo::Vector valVector;
        valVector.toVariant(defaultValue);
    }
    pVector->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pVector, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pVector;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarStructuresWorkOrder(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pWorkOrder = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_Structures_WorkOrder, getTypeNamespace()));
    pWorkOrder->setBrowseName(UaQualifiedName("WorkOrder", getTypeNamespace()));
    pWorkOrder->setDisplayName(UaLocalizedText("", "WorkOrder"));
    pWorkOrder->setAccessLevel(3);
    pWorkOrder->setUserAccessLevel(3);
    pWorkOrder->setValueRank(-1);
    pWorkOrder->setDataTypeId(UaNodeId(DemoId_WorkOrderType, getNameSpaceIndex()));
    {
        Demo::WorkOrderType valWorkOrder;
        valWorkOrder.toVariant(defaultValue);
    }
    pWorkOrder->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pWorkOrder, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar_Structures, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pWorkOrder;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(-1);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
        defaultValue.setUInt16(0);
    pUInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(-1);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
        defaultValue.setUInt32(0);
    pUInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(-1);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
        defaultValue.setUInt64(0);
    pUInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarUInteger(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInteger = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_UInteger, getTypeNamespace()));
    pUInteger->setBrowseName(UaQualifiedName("UInteger", getTypeNamespace()));
    pUInteger->setDisplayName(UaLocalizedText("", "UInteger"));
    pUInteger->setAccessLevel(3);
    pUInteger->setUserAccessLevel(3);
    pUInteger->setValueRank(-1);
    pUInteger->setDataTypeId(OpcUaId_UInteger);
        defaultValue.clear();
    pUInteger->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInteger, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInteger;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarUtcTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUtcTime = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_UtcTime, getTypeNamespace()));
    pUtcTime->setBrowseName(UaQualifiedName("UtcTime", getTypeNamespace()));
    pUtcTime->setDisplayName(UaLocalizedText("", "UtcTime"));
    pUtcTime->setAccessLevel(3);
    pUtcTime->setUserAccessLevel(3);
    pUtcTime->setValueRank(-1);
    pUtcTime->setDataTypeId(OpcUaId_UtcTime);
        defaultValue.setDateTime(UaDateTime());
    pUtcTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUtcTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUtcTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStaticScalarXmlElement(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pXmlElement = new UaBase::Variable(UaNodeId(DemoId_Demo_Static_Scalar_XmlElement, getTypeNamespace()));
    pXmlElement->setBrowseName(UaQualifiedName("XmlElement", getTypeNamespace()));
    pXmlElement->setDisplayName(UaLocalizedText("", "XmlElement"));
    pXmlElement->setAccessLevel(3);
    pXmlElement->setUserAccessLevel(3);
    pXmlElement->setValueRank(-1);
    pXmlElement->setDataTypeId(OpcUaId_XmlElement);
        { UaByteString tmpXmlElement; defaultValue.setXmlElement(tmpXmlElement, OpcUa_False); };
    pXmlElement->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pXmlElement, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Static_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pXmlElement;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamic(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDynamic = new UaBase::Object(UaNodeId(DemoId_Demo_Dynamic, getTypeNamespace()));
    pDynamic->setBrowseName(UaQualifiedName("Dynamic", getTypeNamespace()));
    pDynamic->setDisplayName(UaLocalizedText("", "001_Dynamic"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pDynamic, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDynamic;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArrays(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pArrays = new UaBase::Object(UaNodeId(DemoId_Demo_Dynamic_Arrays, getTypeNamespace()));
    pArrays->setBrowseName(UaQualifiedName("Arrays", getTypeNamespace()));
    pArrays->setDisplayName(UaLocalizedText("", "Arrays"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pArrays, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pArrays;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysBoolean(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBoolean = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Boolean, getTypeNamespace()));
    pBoolean->setBrowseName(UaQualifiedName("Boolean", getTypeNamespace()));
    pBoolean->setDisplayName(UaLocalizedText("", "Boolean"));
    pBoolean->setAccessLevel(3);
    pBoolean->setUserAccessLevel(3);
    pBoolean->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pBoolean->setArrayDimensions(arrayDimensions);
    pBoolean->setDataTypeId(OpcUaId_Boolean);
    {
        UaBooleanArray valBooleanArray;
        defaultValue.setBoolArray(valBooleanArray);
    }
    pBoolean->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBoolean, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBoolean;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pByte->setArrayDimensions(arrayDimensions);
    pByte->setDataTypeId(OpcUaId_Byte);
    {
        UaByteArray valByteArray;
        defaultValue.setByteArray(valByteArray);
    }
    pByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysByteString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByteString = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_ByteString, getTypeNamespace()));
    pByteString->setBrowseName(UaQualifiedName("ByteString", getTypeNamespace()));
    pByteString->setDisplayName(UaLocalizedText("", "ByteString"));
    pByteString->setAccessLevel(3);
    pByteString->setUserAccessLevel(3);
    pByteString->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pByteString->setArrayDimensions(arrayDimensions);
    pByteString->setDataTypeId(OpcUaId_ByteString);
    {
        UaByteStringArray valByteStringArray;
        defaultValue.setByteStringArray(valByteStringArray);
    }
    pByteString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByteString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByteString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysDateTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDateTime = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_DateTime, getTypeNamespace()));
    pDateTime->setBrowseName(UaQualifiedName("DateTime", getTypeNamespace()));
    pDateTime->setDisplayName(UaLocalizedText("", "DateTime"));
    pDateTime->setAccessLevel(3);
    pDateTime->setUserAccessLevel(3);
    pDateTime->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDateTime->setArrayDimensions(arrayDimensions);
    pDateTime->setDataTypeId(OpcUaId_DateTime);
    {
        UaDateTimeArray valDateTimeArray;
        defaultValue.setDateTimeArray(valDateTimeArray);
    }
    pDateTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDateTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDateTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDouble->setArrayDimensions(arrayDimensions);
    pDouble->setDataTypeId(OpcUaId_Double);
    {
        UaDoubleArray valDoubleArray;
        defaultValue.setDoubleArray(valDoubleArray);
    }
    pDouble->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysExpandedNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pExpandedNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_ExpandedNodeId, getTypeNamespace()));
    pExpandedNodeId->setBrowseName(UaQualifiedName("ExpandedNodeId", getTypeNamespace()));
    pExpandedNodeId->setDisplayName(UaLocalizedText("", "ExpandedNodeId"));
    pExpandedNodeId->setAccessLevel(3);
    pExpandedNodeId->setUserAccessLevel(3);
    pExpandedNodeId->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pExpandedNodeId->setArrayDimensions(arrayDimensions);
    pExpandedNodeId->setDataTypeId(OpcUaId_ExpandedNodeId);
    {
        UaExpandedNodeIdArray valExpandedNodeIdArray;
        defaultValue.setExpandedNodeIdArray(valExpandedNodeIdArray);
    }
    pExpandedNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pExpandedNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pExpandedNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pFloat->setArrayDimensions(arrayDimensions);
    pFloat->setDataTypeId(OpcUaId_Float);
    {
        UaFloatArray valFloatArray;
        defaultValue.setFloatArray(valFloatArray);
    }
    pFloat->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysGuid(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pGuid = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Guid, getTypeNamespace()));
    pGuid->setBrowseName(UaQualifiedName("Guid", getTypeNamespace()));
    pGuid->setDisplayName(UaLocalizedText("", "Guid"));
    pGuid->setAccessLevel(3);
    pGuid->setUserAccessLevel(3);
    pGuid->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pGuid->setArrayDimensions(arrayDimensions);
    pGuid->setDataTypeId(OpcUaId_Guid);
    {
        UaGuidArray valGuidArray;
        defaultValue.setGuidArray(valGuidArray);
    }
    pGuid->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pGuid, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pGuid;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt16->setArrayDimensions(arrayDimensions);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        UaInt16Array valInt16Array;
        defaultValue.setInt16Array(valInt16Array);
    }
    pInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt32->setArrayDimensions(arrayDimensions);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        UaInt32Array valInt32Array;
        defaultValue.setInt32Array(valInt32Array);
    }
    pInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt64->setArrayDimensions(arrayDimensions);
    pInt64->setDataTypeId(OpcUaId_Int64);
    {
        UaInt64Array valInt64Array;
        defaultValue.setInt64Array(valInt64Array);
    }
    pInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysLocalizedText(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pLocalizedText = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_LocalizedText, getTypeNamespace()));
    pLocalizedText->setBrowseName(UaQualifiedName("LocalizedText", getTypeNamespace()));
    pLocalizedText->setDisplayName(UaLocalizedText("", "LocalizedText"));
    pLocalizedText->setAccessLevel(3);
    pLocalizedText->setUserAccessLevel(3);
    pLocalizedText->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pLocalizedText->setArrayDimensions(arrayDimensions);
    pLocalizedText->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray valLocalizedTextArray;
        defaultValue.setLocalizedTextArray(valLocalizedTextArray);
    }
    pLocalizedText->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pLocalizedText, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pLocalizedText;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_NodeId, getTypeNamespace()));
    pNodeId->setBrowseName(UaQualifiedName("NodeId", getTypeNamespace()));
    pNodeId->setDisplayName(UaLocalizedText("", "NodeId"));
    pNodeId->setAccessLevel(3);
    pNodeId->setUserAccessLevel(3);
    pNodeId->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pNodeId->setArrayDimensions(arrayDimensions);
    pNodeId->setDataTypeId(OpcUaId_NodeId);
    {
        UaNodeIdArray valNodeIdArray;
        defaultValue.setNodeIdArray(valNodeIdArray);
    }
    pNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysQualifiedName(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQualifiedName = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_QualifiedName, getTypeNamespace()));
    pQualifiedName->setBrowseName(UaQualifiedName("QualifiedName", getTypeNamespace()));
    pQualifiedName->setDisplayName(UaLocalizedText("", "QualifiedName"));
    pQualifiedName->setAccessLevel(3);
    pQualifiedName->setUserAccessLevel(3);
    pQualifiedName->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pQualifiedName->setArrayDimensions(arrayDimensions);
    pQualifiedName->setDataTypeId(OpcUaId_QualifiedName);
    {
        UaQualifiedNameArray valQualifiedNameArray;
        defaultValue.setQualifiedNameArray(valQualifiedNameArray);
    }
    pQualifiedName->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQualifiedName, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQualifiedName;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pSByte->setArrayDimensions(arrayDimensions);
    pSByte->setDataTypeId(OpcUaId_SByte);
    {
        UaSByteArray valSByteArray;
        defaultValue.setSByteArray(valSByteArray);
    }
    pSByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysStatusCode(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pStatusCode = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_StatusCode, getTypeNamespace()));
    pStatusCode->setBrowseName(UaQualifiedName("StatusCode", getTypeNamespace()));
    pStatusCode->setDisplayName(UaLocalizedText("", "StatusCode"));
    pStatusCode->setAccessLevel(3);
    pStatusCode->setUserAccessLevel(3);
    pStatusCode->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pStatusCode->setArrayDimensions(arrayDimensions);
    pStatusCode->setDataTypeId(OpcUaId_StatusCode);
    {
        UaStatusCodeArray valStatusCodeArray;
        defaultValue.setStatusCodeArray(valStatusCodeArray);
    }
    pStatusCode->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pStatusCode, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStatusCode;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pString = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_String, getTypeNamespace()));
    pString->setBrowseName(UaQualifiedName("String", getTypeNamespace()));
    pString->setDisplayName(UaLocalizedText("", "String"));
    pString->setAccessLevel(3);
    pString->setUserAccessLevel(3);
    pString->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pString->setArrayDimensions(arrayDimensions);
    pString->setDataTypeId(OpcUaId_String);
    {
        UaStringArray valStringArray;
        defaultValue.setStringArray(valStringArray);
    }
    pString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt16->setArrayDimensions(arrayDimensions);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        UaUInt16Array valUInt16Array;
        defaultValue.setUInt16Array(valUInt16Array);
    }
    pUInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt32->setArrayDimensions(arrayDimensions);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        UaUInt32Array valUInt32Array;
        defaultValue.setUInt32Array(valUInt32Array);
    }
    pUInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt64->setArrayDimensions(arrayDimensions);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
    {
        UaUInt64Array valUInt64Array;
        defaultValue.setUInt64Array(valUInt64Array);
    }
    pUInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicArraysXmlElement(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pXmlElement = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Arrays_XmlElement, getTypeNamespace()));
    pXmlElement->setBrowseName(UaQualifiedName("XmlElement", getTypeNamespace()));
    pXmlElement->setDisplayName(UaLocalizedText("", "XmlElement"));
    pXmlElement->setAccessLevel(3);
    pXmlElement->setUserAccessLevel(3);
    pXmlElement->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pXmlElement->setArrayDimensions(arrayDimensions);
    pXmlElement->setDataTypeId(OpcUaId_XmlElement);
    {
        UaXmlElementArray valXmlElementArray;
        defaultValue.setXmlElementArray(valXmlElementArray);
    }
    pXmlElement->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pXmlElement, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pXmlElement;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalar(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pScalar = new UaBase::Object(UaNodeId(DemoId_Demo_Dynamic_Scalar, getTypeNamespace()));
    pScalar->setBrowseName(UaQualifiedName("Scalar", getTypeNamespace()));
    pScalar->setDisplayName(UaLocalizedText("", "Scalar"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pScalar, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pScalar;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarBoolean(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBoolean = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Boolean, getTypeNamespace()));
    pBoolean->setBrowseName(UaQualifiedName("Boolean", getTypeNamespace()));
    pBoolean->setDisplayName(UaLocalizedText("", "Boolean"));
    pBoolean->setAccessLevel(3);
    pBoolean->setUserAccessLevel(3);
    pBoolean->setValueRank(-1);
    pBoolean->setDataTypeId(OpcUaId_Boolean);
        defaultValue.setBool(OpcUa_False);
    pBoolean->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBoolean, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBoolean;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(-1);
    pByte->setDataTypeId(OpcUaId_Byte);
        defaultValue.setByte(0);
    pByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarByteString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByteString = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_ByteString, getTypeNamespace()));
    pByteString->setBrowseName(UaQualifiedName("ByteString", getTypeNamespace()));
    pByteString->setDisplayName(UaLocalizedText("", "ByteString"));
    pByteString->setAccessLevel(3);
    pByteString->setUserAccessLevel(3);
    pByteString->setValueRank(-1);
    pByteString->setDataTypeId(OpcUaId_ByteString);
        { UaByteString tmpByteString; defaultValue.setByteString(tmpByteString, OpcUa_False); };
    pByteString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByteString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByteString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarDateTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDateTime = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_DateTime, getTypeNamespace()));
    pDateTime->setBrowseName(UaQualifiedName("DateTime", getTypeNamespace()));
    pDateTime->setDisplayName(UaLocalizedText("", "DateTime"));
    pDateTime->setAccessLevel(3);
    pDateTime->setUserAccessLevel(3);
    pDateTime->setValueRank(-1);
    pDateTime->setDataTypeId(OpcUaId_DateTime);
        defaultValue.setDateTime(UaDateTime());
    pDateTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDateTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDateTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(-1);
    pDouble->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pDouble->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarExpandedNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pExpandedNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_ExpandedNodeId, getTypeNamespace()));
    pExpandedNodeId->setBrowseName(UaQualifiedName("ExpandedNodeId", getTypeNamespace()));
    pExpandedNodeId->setDisplayName(UaLocalizedText("", "ExpandedNodeId"));
    pExpandedNodeId->setAccessLevel(3);
    pExpandedNodeId->setUserAccessLevel(3);
    pExpandedNodeId->setValueRank(-1);
    pExpandedNodeId->setDataTypeId(OpcUaId_ExpandedNodeId);
        defaultValue.setExpandedNodeId(UaExpandedNodeId());
    pExpandedNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pExpandedNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pExpandedNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(-1);
    pFloat->setDataTypeId(OpcUaId_Float);
        defaultValue.setFloat(0.0);
    pFloat->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarGuid(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pGuid = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Guid, getTypeNamespace()));
    pGuid->setBrowseName(UaQualifiedName("Guid", getTypeNamespace()));
    pGuid->setDisplayName(UaLocalizedText("", "Guid"));
    pGuid->setAccessLevel(3);
    pGuid->setUserAccessLevel(3);
    pGuid->setValueRank(-1);
    pGuid->setDataTypeId(OpcUaId_Guid);
        defaultValue.setGuid(UaGuid());
    pGuid->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pGuid, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pGuid;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarImageGIF(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImageGIF = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_ImageGIF, getTypeNamespace()));
    pImageGIF->setBrowseName(UaQualifiedName("ImageGIF", getTypeNamespace()));
    pImageGIF->setDisplayName(UaLocalizedText("", "ImageGIF"));
    pImageGIF->setAccessLevel(3);
    pImageGIF->setUserAccessLevel(3);
    pImageGIF->setValueRank(-1);
    pImageGIF->setDataTypeId(OpcUaId_ImageGIF);
    {
        UaByteString tmpImageGIF = UaByteString(0, (OpcUa_Byte*) "");
        defaultValue.setByteString(tmpImageGIF, OpcUa_False);
    }
    pImageGIF->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImageGIF, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImageGIF;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(-1);
    pInt16->setDataTypeId(OpcUaId_Int16);
        defaultValue.setInt16(0);
    pInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(-1);
    pInt32->setDataTypeId(OpcUaId_Int32);
        defaultValue.setInt32(0);
    pInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(-1);
    pInt64->setDataTypeId(OpcUaId_Int64);
        defaultValue.setInt64(0);
    pInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarLocalizedText(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pLocalizedText = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_LocalizedText, getTypeNamespace()));
    pLocalizedText->setBrowseName(UaQualifiedName("LocalizedText", getTypeNamespace()));
    pLocalizedText->setDisplayName(UaLocalizedText("", "LocalizedText"));
    pLocalizedText->setAccessLevel(3);
    pLocalizedText->setUserAccessLevel(3);
    pLocalizedText->setValueRank(-1);
    pLocalizedText->setDataTypeId(OpcUaId_LocalizedText);
        defaultValue.setLocalizedText(UaLocalizedText());
    pLocalizedText->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pLocalizedText, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pLocalizedText;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_NodeId, getTypeNamespace()));
    pNodeId->setBrowseName(UaQualifiedName("NodeId", getTypeNamespace()));
    pNodeId->setDisplayName(UaLocalizedText("", "NodeId"));
    pNodeId->setAccessLevel(3);
    pNodeId->setUserAccessLevel(3);
    pNodeId->setValueRank(-1);
    pNodeId->setDataTypeId(OpcUaId_NodeId);
        defaultValue.setNodeId(UaNodeId());
    pNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarQualifiedName(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQualifiedName = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_QualifiedName, getTypeNamespace()));
    pQualifiedName->setBrowseName(UaQualifiedName("QualifiedName", getTypeNamespace()));
    pQualifiedName->setDisplayName(UaLocalizedText("", "QualifiedName"));
    pQualifiedName->setAccessLevel(3);
    pQualifiedName->setUserAccessLevel(3);
    pQualifiedName->setValueRank(-1);
    pQualifiedName->setDataTypeId(OpcUaId_QualifiedName);
        defaultValue.setQualifiedName(UaQualifiedName());
    pQualifiedName->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQualifiedName, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQualifiedName;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarQuality_DynamicValue(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQuality_DynamicValue = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Quality_DynamicValue, getTypeNamespace()));
    pQuality_DynamicValue->setBrowseName(UaQualifiedName("Quality_DynamicValue", getTypeNamespace()));
    pQuality_DynamicValue->setDisplayName(UaLocalizedText("", "Quality_DynamicValue"));
    pQuality_DynamicValue->setDescription(UaLocalizedText("", "The statuc code and the value changes."));
    pQuality_DynamicValue->setAccessLevel(3);
    pQuality_DynamicValue->setUserAccessLevel(3);
    pQuality_DynamicValue->setValueRank(-1);
    pQuality_DynamicValue->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pQuality_DynamicValue->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQuality_DynamicValue, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQuality_DynamicValue;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarQuality_StaticValue(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQuality_StaticValue = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_Quality_StaticValue, getTypeNamespace()));
    pQuality_StaticValue->setBrowseName(UaQualifiedName("Quality_StaticValue", getTypeNamespace()));
    pQuality_StaticValue->setDisplayName(UaLocalizedText("", "Quality_StaticValue"));
    pQuality_StaticValue->setDescription(UaLocalizedText("", "The statuc code changes, the value is constant"));
    pQuality_StaticValue->setAccessLevel(3);
    pQuality_StaticValue->setUserAccessLevel(3);
    pQuality_StaticValue->setValueRank(-1);
    pQuality_StaticValue->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pQuality_StaticValue->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQuality_StaticValue, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQuality_StaticValue;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(-1);
    pSByte->setDataTypeId(OpcUaId_SByte);
        defaultValue.setSByte(0);
    pSByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarStatusCode(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pStatusCode = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_StatusCode, getTypeNamespace()));
    pStatusCode->setBrowseName(UaQualifiedName("StatusCode", getTypeNamespace()));
    pStatusCode->setDisplayName(UaLocalizedText("", "StatusCode"));
    pStatusCode->setAccessLevel(3);
    pStatusCode->setUserAccessLevel(3);
    pStatusCode->setValueRank(-1);
    pStatusCode->setDataTypeId(OpcUaId_StatusCode);
        defaultValue.setStatusCode(0);
    pStatusCode->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pStatusCode, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStatusCode;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pString = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_String, getTypeNamespace()));
    pString->setBrowseName(UaQualifiedName("String", getTypeNamespace()));
    pString->setDisplayName(UaLocalizedText("", "String"));
    pString->setAccessLevel(3);
    pString->setUserAccessLevel(3);
    pString->setValueRank(-1);
    pString->setDataTypeId(OpcUaId_String);
        defaultValue.setString("");
    pString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(-1);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
        defaultValue.setUInt16(0);
    pUInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(-1);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
        defaultValue.setUInt32(0);
    pUInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(-1);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
        defaultValue.setUInt64(0);
    pUInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicScalarXmlElement(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pXmlElement = new UaBase::Variable(UaNodeId(DemoId_Demo_Dynamic_Scalar_XmlElement, getTypeNamespace()));
    pXmlElement->setBrowseName(UaQualifiedName("XmlElement", getTypeNamespace()));
    pXmlElement->setDisplayName(UaLocalizedText("", "XmlElement"));
    pXmlElement->setAccessLevel(3);
    pXmlElement->setUserAccessLevel(3);
    pXmlElement->setValueRank(-1);
    pXmlElement->setDataTypeId(OpcUaId_XmlElement);
        { UaByteString tmpXmlElement; defaultValue.setXmlElement(tmpXmlElement, OpcUa_False); };
    pXmlElement->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pXmlElement, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Dynamic_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pXmlElement;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoHistory(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHistory = new UaBase::Object(UaNodeId(DemoId_Demo_History, getTypeNamespace()));
    pHistory->setBrowseName(UaQualifiedName("History", getTypeNamespace()));
    pHistory->setDisplayName(UaLocalizedText("", "002_History"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHistory, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHistory;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoHistoryByteWithHistory(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByteWithHistory = new UaBase::Variable(UaNodeId(DemoId_Demo_History_ByteWithHistory, getTypeNamespace()));
    pByteWithHistory->setBrowseName(UaQualifiedName("ByteWithHistory", getTypeNamespace()));
    pByteWithHistory->setDisplayName(UaLocalizedText("", "ByteWithHistory"));
    pByteWithHistory->setAccessLevel(7);
    pByteWithHistory->setUserAccessLevel(7);
    pByteWithHistory->setValueRank(-1);
    pByteWithHistory->setDataTypeId(OpcUaId_Byte);
    {
        defaultValue.setByte(0);
    }
    pByteWithHistory->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByteWithHistory, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_History, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByteWithHistory;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoHistoryDataLoggerActive(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDataLoggerActive = new UaBase::Variable(UaNodeId(DemoId_Demo_History_DataLoggerActive, getTypeNamespace()));
    pDataLoggerActive->setBrowseName(UaQualifiedName("DataLoggerActive", getTypeNamespace()));
    pDataLoggerActive->setDisplayName(UaLocalizedText("", "DataLoggerActive"));
    pDataLoggerActive->setAccessLevel(3);
    pDataLoggerActive->setUserAccessLevel(3);
    pDataLoggerActive->setValueRank(-1);
    pDataLoggerActive->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pDataLoggerActive->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDataLoggerActive, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_History, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDataLoggerActive;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoHistoryDoubleWithHistory(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDoubleWithHistory = new UaBase::Variable(UaNodeId(DemoId_Demo_History_DoubleWithHistory, getTypeNamespace()));
    pDoubleWithHistory->setBrowseName(UaQualifiedName("DoubleWithHistory", getTypeNamespace()));
    pDoubleWithHistory->setDisplayName(UaLocalizedText("", "DoubleWithHistory"));
    pDoubleWithHistory->setAccessLevel(7);
    pDoubleWithHistory->setUserAccessLevel(7);
    pDoubleWithHistory->setValueRank(-1);
    pDoubleWithHistory->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pDoubleWithHistory->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDoubleWithHistory, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_History, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDoubleWithHistory;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoHistoryStartLogging(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_History_StartLogging, getTypeNamespace()), "StartLogging", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_History, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoHistoryStopLogging(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_History_StopLogging, getTypeNamespace()), "StopLogging", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_History, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoMethod(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pMethod = new UaBase::Object(UaNodeId(DemoId_Demo_Method, getTypeNamespace()));
    pMethod->setBrowseName(UaQualifiedName("Method", getTypeNamespace()));
    pMethod->setDisplayName(UaLocalizedText("", "003_Method"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pMethod, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pMethod;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoMethodDoSomethingAfter10s(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_Method_DoSomethingAfter10s, getTypeNamespace()), "DoSomethingAfter10s", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Method, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoMethodEnumTest(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_Method_EnumTest, getTypeNamespace()), "EnumTest", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Method, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    UaArgument argument;

    // InputArguments
    UaBase::Variable *pInputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_Method_EnumTest_InputArguments, getTypeNamespace()));
    pInputArguments->setBrowseName(UaQualifiedName("InputArguments", 0));
    pInputArguments->setDisplayName(UaLocalizedText("", "InputArguments"));
    pInputArguments->setAccessLevel(1);
    pInputArguments->setUserAccessLevel(1);
    pInputArguments->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 2;
    pInputArguments->setArrayDimensions(arrayDimensions);
    pInputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valInputArguments;
    valInputArguments.create(2);
    argument.setName("HeaterStatus");
    argument.setDataType(UaNodeId(DemoId_HeaterStatus, getNameSpaceIndex()));
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 0 ]);
    argument.setName("Priority");
    argument.setDataType(UaNodeId(DemoId_Priority, getNameSpaceIndex()));
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 1 ]);
    valInputArguments.toVariant(defaultValue);
    pInputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pInputArgumentsProperty = new OpcUa::PropertyType(pInputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pInputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pInputArguments;

    // OutputArguments
    UaBase::Variable *pOutputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_Method_EnumTest_OutputArguments, getTypeNamespace()));
    pOutputArguments->setBrowseName(UaQualifiedName("OutputArguments", 0));
    pOutputArguments->setDisplayName(UaLocalizedText("", "OutputArguments"));
    pOutputArguments->setAccessLevel(1);
    pOutputArguments->setUserAccessLevel(1);
    pOutputArguments->setValueRank(1);
    arrayDimensions.create(1);
    arrayDimensions[0] = 2;
    pOutputArguments->setArrayDimensions(arrayDimensions);
    pOutputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valOutputArguments;
    valOutputArguments.create(2);
    argument.setName("RetHeaterStatus");
    argument.setDataType(UaNodeId(DemoId_HeaterStatus, getNameSpaceIndex()));
    argument.setValueRank(-1);
    argument.copyTo(&valOutputArguments[ 0 ]);
    argument.setName("RetPriority");
    argument.setDataType(UaNodeId(DemoId_Priority, getNameSpaceIndex()));
    argument.setValueRank(-1);
    argument.copyTo(&valOutputArguments[ 1 ]);
    valOutputArguments.toVariant(defaultValue);
    pOutputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pOutputArgumentsProperty = new OpcUa::PropertyType(pOutputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pOutputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pOutputArguments;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoMethodMultiply(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_Method_Multiply, getTypeNamespace()), "Multiply", getTypeNamespace());
    pMethod->setDescription(UaLocalizedText("", "Returns the product of the two given numbers"));
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Method, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    UaArgument argument;

    // InputArguments
    UaBase::Variable *pInputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_Method_Multiply_InputArguments, getTypeNamespace()));
    pInputArguments->setBrowseName(UaQualifiedName("InputArguments", 0));
    pInputArguments->setDisplayName(UaLocalizedText("", "InputArguments"));
    pInputArguments->setAccessLevel(1);
    pInputArguments->setUserAccessLevel(1);
    pInputArguments->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 2;
    pInputArguments->setArrayDimensions(arrayDimensions);
    pInputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valInputArguments;
    valInputArguments.create(2);
    argument.setName("a");
    argument.setDataType(OpcUaId_Double);
    argument.setDescription(UaLocalizedText("", "parameter a"));
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 0 ]);
    argument.setName("b");
    argument.setDataType(OpcUaId_Double);
    argument.setDescription(UaLocalizedText("", "parameter b"));
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 1 ]);
    valInputArguments.toVariant(defaultValue);
    pInputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pInputArgumentsProperty = new OpcUa::PropertyType(pInputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pInputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pInputArguments;

    // OutputArguments
    UaBase::Variable *pOutputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_Method_Multiply_OutputArguments, getTypeNamespace()));
    pOutputArguments->setBrowseName(UaQualifiedName("OutputArguments", 0));
    pOutputArguments->setDisplayName(UaLocalizedText("", "OutputArguments"));
    pOutputArguments->setAccessLevel(1);
    pOutputArguments->setUserAccessLevel(1);
    pOutputArguments->setValueRank(1);
    arrayDimensions.create(1);
    arrayDimensions[0] = 1;
    pOutputArguments->setArrayDimensions(arrayDimensions);
    pOutputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valOutputArguments;
    valOutputArguments.create(1);
    argument.setName("result");
    argument.setDataType(OpcUaId_Double);
    argument.setDescription(UaLocalizedText("", "result = a * b"));
    argument.setValueRank(-1);
    argument.copyTo(&valOutputArguments[ 0 ]);
    valOutputArguments.toVariant(defaultValue);
    pOutputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pOutputArgumentsProperty = new OpcUa::PropertyType(pOutputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pOutputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pOutputArguments;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoMethodVectorAdd(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_Method_VectorAdd, getTypeNamespace()), "VectorAdd", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Method, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    UaArgument argument;

    // InputArguments
    UaBase::Variable *pInputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_Method_VectorAdd_InputArguments, getTypeNamespace()));
    pInputArguments->setBrowseName(UaQualifiedName("InputArguments", 0));
    pInputArguments->setDisplayName(UaLocalizedText("", "InputArguments"));
    pInputArguments->setAccessLevel(1);
    pInputArguments->setUserAccessLevel(1);
    pInputArguments->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 2;
    pInputArguments->setArrayDimensions(arrayDimensions);
    pInputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valInputArguments;
    valInputArguments.create(2);
    argument.setName("v1");
    argument.setDataType(UaNodeId(DemoId_Vector, getNameSpaceIndex()));
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 0 ]);
    argument.setName("v2");
    argument.setDataType(UaNodeId(DemoId_Vector, getNameSpaceIndex()));
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 1 ]);
    valInputArguments.toVariant(defaultValue);
    pInputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pInputArgumentsProperty = new OpcUa::PropertyType(pInputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pInputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pInputArguments;

    // OutputArguments
    UaBase::Variable *pOutputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_Method_VectorAdd_OutputArguments, getTypeNamespace()));
    pOutputArguments->setBrowseName(UaQualifiedName("OutputArguments", 0));
    pOutputArguments->setDisplayName(UaLocalizedText("", "OutputArguments"));
    pOutputArguments->setAccessLevel(1);
    pOutputArguments->setUserAccessLevel(1);
    pOutputArguments->setValueRank(1);
    arrayDimensions.create(1);
    arrayDimensions[0] = 1;
    pOutputArguments->setArrayDimensions(arrayDimensions);
    pOutputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valOutputArguments;
    valOutputArguments.create(1);
    argument.setName("result");
    argument.setDataType(UaNodeId(DemoId_Vector, getNameSpaceIndex()));
    argument.setValueRank(-1);
    argument.copyTo(&valOutputArguments[ 0 ]);
    valOutputArguments.toVariant(defaultValue);
    pOutputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pOutputArgumentsProperty = new OpcUa::PropertyType(pOutputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pOutputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pOutputArguments;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoEvents(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pEvents = new UaBase::Object(UaNodeId(DemoId_Demo_Events, getTypeNamespace()));
    pEvents->setBrowseName(UaQualifiedName("Events", getTypeNamespace()));
    pEvents->setDisplayName(UaLocalizedText("", "004_Events"));
    pEvents->setDescription(UaLocalizedText("", "In this folder you find objects which demonstrate OPC UA event features."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pEvents, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEvents;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoEventsMachine(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pMachine = new UaBase::Object(UaNodeId(DemoId_Server_Machine, getTypeNamespace()));
    pMachine->setBrowseName(UaQualifiedName("Machine", getTypeNamespace()));
    pMachine->setDisplayName(UaLocalizedText("", "Machine"));
    pMachine->setEventNotifier(1);

    UaBase::FullReference reference;

    // HeaterSwitch
    UaBase::Variable *pHeaterSwitch = new UaBase::Variable(UaNodeId(DemoId_Server_Machine_HeaterSwitch, getTypeNamespace()));
    pHeaterSwitch->setBrowseName(UaQualifiedName("HeaterSwitch", getTypeNamespace()));
    pHeaterSwitch->setDisplayName(UaLocalizedText("", "HeaterSwitch"));
    pHeaterSwitch->setAccessLevel(3);
    pHeaterSwitch->setUserAccessLevel(3);
    pHeaterSwitch->setValueRank(-1);
    pHeaterSwitch->setDataTypeId(OpcUaId_Boolean);
    defaultValue.setBool(OpcUa_False);
    pHeaterSwitch->setValue(defaultValue);
    pHeaterSwitch->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pMachine);
    reference.setTargetNode(pHeaterSwitch);
    pMachine->addFullReference(reference);
    pHeaterSwitch->addFullReference(reference);
    pHeaterSwitch->setParentNodeId(pMachine->nodeId());

    // TemperatureSensor
    UaBase::Object *pTemperatureSensor = new UaBase::Object(UaNodeId(DemoId_Server_Machine_TemperatureSensor, getTypeNamespace()));
    pTemperatureSensor->setBrowseName(UaQualifiedName("TemperatureSensor", getTypeNamespace()));
    pTemperatureSensor->setDisplayName(UaLocalizedText("", "TemperatureSensor"));
    pTemperatureSensor->setTypeDefinitionId(UaNodeId(DemoId_TemperatureSensorType, getNameSpaceIndex()));
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pMachine);
    reference.setTargetNode(pTemperatureSensor);
    pMachine->addFullReference(reference);
    pTemperatureSensor->addFullReference(reference);
    pTemperatureSensor->setParentNodeId(pMachine->nodeId());

    // TemperatureSensor -> Temperature
    UaBase::Variable *pTemperatureSensorTemperature = new UaBase::Variable(UaNodeId(DemoId_Server_Machine_TemperatureSensor_Temperature, getTypeNamespace()));
    pTemperatureSensorTemperature->setBrowseName(UaQualifiedName("Temperature", getTypeNamespace()));
    pTemperatureSensorTemperature->setDisplayName(UaLocalizedText("", "Temperature"));
    pTemperatureSensorTemperature->setAccessLevel(3);
    pTemperatureSensorTemperature->setUserAccessLevel(3);
    pTemperatureSensorTemperature->setValueRank(-1);
    pTemperatureSensorTemperature->setDataTypeId(OpcUaId_Double);
    defaultValue.setDouble(0.0);
    pTemperatureSensorTemperature->setValue(defaultValue);
    pTemperatureSensorTemperature->setTypeDefinitionId(OpcUaId_AnalogItemType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pTemperatureSensor);
    reference.setTargetNode(pTemperatureSensorTemperature);
    pTemperatureSensor->addFullReference(reference);
    pTemperatureSensorTemperature->addFullReference(reference);
    pTemperatureSensorTemperature->setParentNodeId(pTemperatureSensor->nodeId());

    // TemperatureSensor -> Temperature -> EURange
    UaBase::Variable *pTemperatureSensorTemperatureEURange = new UaBase::Variable(UaNodeId(DemoId_Server_Machine_TemperatureSensor_Temperature_EURange, getTypeNamespace()));
    pTemperatureSensorTemperatureEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pTemperatureSensorTemperatureEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pTemperatureSensorTemperatureEURange->setAccessLevel(1);
    pTemperatureSensorTemperatureEURange->setUserAccessLevel(1);
    pTemperatureSensorTemperatureEURange->setValueRank(-1);
    pTemperatureSensorTemperatureEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 0);
        valEURange.toVariant(defaultValue);
    }
    pTemperatureSensorTemperatureEURange->setValue(defaultValue);
    pTemperatureSensorTemperatureEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTemperatureSensorTemperature);
    reference.setTargetNode(pTemperatureSensorTemperatureEURange);
    pTemperatureSensorTemperature->addFullReference(reference);
    pTemperatureSensorTemperatureEURange->addFullReference(reference);
    pTemperatureSensorTemperatureEURange->setParentNodeId(pTemperatureSensorTemperature->nodeId());
    Demo::MachineType *pObjectType = new Demo::MachineType(pMachine, (XmlUaNodeFactoryManager*) &factory, this);
    pObjectType->setEventNotifier(1);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Events, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    parentEventNodeId = OpcUaId_Server;
    ret = addUaReference(parentEventNodeId, pObjectType->nodeId(), OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());
    registerEventNotifier(parentEventNodeId, pObjectType->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
    delete pHeaterSwitch;
    delete pTemperatureSensor;
    delete pTemperatureSensorTemperature;
    delete pTemperatureSensorTemperatureEURange;
    delete pMachine;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoEventsSampleEventNotifier(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pSampleEventNotifier = new UaBase::Object(UaNodeId(DemoId_Server_SampleEventNotifier, getTypeNamespace()));
    pSampleEventNotifier->setBrowseName(UaQualifiedName("SampleEventNotifier", getTypeNamespace()));
    pSampleEventNotifier->setDisplayName(UaLocalizedText("", "SampleEventNotifier"));
    pSampleEventNotifier->setEventNotifier(1);

    UaBase::FullReference reference;
    OpcUa::BaseObjectType *pObjectType = new OpcUa::BaseObjectType(pSampleEventNotifier, (XmlUaNodeFactoryManager*) &factory, this);
    pObjectType->setEventNotifier(1);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Events, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    parentEventNodeId = OpcUaId_Server;
    ret = addUaReference(parentEventNodeId, pObjectType->nodeId(), OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());
    registerEventNotifier(parentEventNodeId, pObjectType->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
    delete pSampleEventNotifier;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoEventsTrigger_BaseEvent(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pTrigger_BaseEvent = new UaBase::Variable(UaNodeId(DemoId_Demo_Events_Trigger_BaseEvent, getTypeNamespace()));
    pTrigger_BaseEvent->setBrowseName(UaQualifiedName("Trigger_BaseEvent", getTypeNamespace()));
    pTrigger_BaseEvent->setDisplayName(UaLocalizedText("", "Trigger_BaseEvent"));
    pTrigger_BaseEvent->setAccessLevel(3);
    pTrigger_BaseEvent->setUserAccessLevel(3);
    pTrigger_BaseEvent->setValueRank(-1);
    pTrigger_BaseEvent->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pTrigger_BaseEvent->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pTrigger_BaseEvent, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Events, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    parentEventNodeId = UaNodeId(DemoId_Server_SampleEventNotifier, getNameSpaceIndex());
    ret = addUaReference(parentEventNodeId, pVariableType->nodeId(), OpcUaId_HasEventSource);
    UA_ASSERT(ret.isGood());
    registerEventSource(parentEventNodeId, pVariableType->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
    delete pTrigger_BaseEvent;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoEventsTrigger_SampleEvent(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pTrigger_SampleEvent = new UaBase::Variable(UaNodeId(DemoId_Demo_Events_Trigger_SampleEvent, getTypeNamespace()));
    pTrigger_SampleEvent->setBrowseName(UaQualifiedName("Trigger_SampleEvent", getTypeNamespace()));
    pTrigger_SampleEvent->setDisplayName(UaLocalizedText("", "Trigger_SampleEvent"));
    pTrigger_SampleEvent->setAccessLevel(3);
    pTrigger_SampleEvent->setUserAccessLevel(3);
    pTrigger_SampleEvent->setValueRank(-1);
    pTrigger_SampleEvent->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pTrigger_SampleEvent->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pTrigger_SampleEvent, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Events, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
    parentEventNodeId = UaNodeId(DemoId_Server_SampleEventNotifier, getNameSpaceIndex());
    ret = addUaReference(parentEventNodeId, pVariableType->nodeId(), OpcUaId_HasEventSource);
    UA_ASSERT(ret.isGood());
    registerEventSource(parentEventNodeId, pVariableType->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
    delete pTrigger_SampleEvent;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRights(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pAccessRights = new UaBase::Object(UaNodeId(DemoId_Demo_AccessRights, getTypeNamespace()));
    pAccessRights->setBrowseName(UaQualifiedName("AccessRights", getTypeNamespace()));
    pAccessRights->setDisplayName(UaLocalizedText("", "005_AccessRights"));
    pAccessRights->setDescription(UaLocalizedText("", "This folder contains variables with different access rights to demonstrates user authorization."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pAccessRights, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccessRights;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_All(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pAccess_All = new UaBase::Object(UaNodeId(DemoId_Demo_AccessRights_Access_All, getTypeNamespace()));
    pAccess_All->setBrowseName(UaQualifiedName("Access_All", getTypeNamespace()));
    pAccess_All->setDisplayName(UaLocalizedText("", "Access_All"));
    pAccess_All->setDescription(UaLocalizedText("", "Contains elements with full access to attributes for all users."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pAccess_All, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccess_All;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_AllAll_RO(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_RO = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_All_All_RO, getTypeNamespace()));
    pAll_RO->setBrowseName(UaQualifiedName("All_RO", getTypeNamespace()));
    pAll_RO->setDisplayName(UaLocalizedText("", "All_RO"));
    pAll_RO->setAccessLevel(1);
    pAll_RO->setUserAccessLevel(1);
    pAll_RO->setValueRank(-1);
    pAll_RO->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pAll_RO->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_RO, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_All, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_RO;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_AllAll_RW(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_RW = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_All_All_RW, getTypeNamespace()));
    pAll_RW->setBrowseName(UaQualifiedName("All_RW", getTypeNamespace()));
    pAll_RW->setDisplayName(UaLocalizedText("", "All_RW"));
    pAll_RW->setAccessLevel(3);
    pAll_RW->setUserAccessLevel(3);
    pAll_RW->setValueRank(-1);
    pAll_RW->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pAll_RW->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_RW, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_All, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_RW;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_AllAll_WO(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_WO = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_All_All_WO, getTypeNamespace()));
    pAll_WO->setBrowseName(UaQualifiedName("All_WO", getTypeNamespace()));
    pAll_WO->setDisplayName(UaLocalizedText("", "All_WO"));
    pAll_WO->setAccessLevel(2);
    pAll_WO->setUserAccessLevel(2);
    pAll_WO->setValueRank(-1);
    pAll_WO->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pAll_WO->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_WO, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_All, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_WO;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_John(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pAccess_John = new UaBase::Object(UaNodeId(DemoId_Demo_AccessRights_Access_John, getTypeNamespace()));
    pAccess_John->setBrowseName(UaQualifiedName("Access_John", getTypeNamespace()));
    pAccess_John->setDisplayName(UaLocalizedText("", "Access_John"));
    pAccess_John->setDescription(UaLocalizedText("", "Contains elements with full access to attributes for user John."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pAccess_John, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccess_John;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_JohnAll_RO_John_RW(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_RO_John_RW = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_John_All_RO_John_RW, getTypeNamespace()));
    pAll_RO_John_RW->setBrowseName(UaQualifiedName("All_RO_John_RW", getTypeNamespace()));
    pAll_RO_John_RW->setDisplayName(UaLocalizedText("", "All_RO_John_RW"));
    pAll_RO_John_RW->setDescription(UaLocalizedText("", "Read and Write access to value attribute for user John. Read only access for other users."));
    pAll_RO_John_RW->setAccessLevel(3);
    pAll_RO_John_RW->setUserAccessLevel(3);
    pAll_RO_John_RW->setValueRank(-1);
    pAll_RO_John_RW->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pAll_RO_John_RW->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_RO_John_RW, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_John, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_RO_John_RW;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_JohnAll_WO_John_RW(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_WO_John_RW = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_John_All_WO_John_RW, getTypeNamespace()));
    pAll_WO_John_RW->setBrowseName(UaQualifiedName("All_WO_John_RW", getTypeNamespace()));
    pAll_WO_John_RW->setDisplayName(UaLocalizedText("", "All_WO_John_RW"));
    pAll_WO_John_RW->setDescription(UaLocalizedText("", "Read and Write access to value attribute for user John. Write only access for other users."));
    pAll_WO_John_RW->setAccessLevel(3);
    pAll_WO_John_RW->setUserAccessLevel(3);
    pAll_WO_John_RW->setValueRank(-1);
    pAll_WO_John_RW->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pAll_WO_John_RW->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_WO_John_RW, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_John, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_WO_John_RW;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_JohnJohn_RO(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pJohn_RO = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_John_John_RO, getTypeNamespace()));
    pJohn_RO->setBrowseName(UaQualifiedName("John_RO", getTypeNamespace()));
    pJohn_RO->setDisplayName(UaLocalizedText("", "John_RO"));
    pJohn_RO->setDescription(UaLocalizedText("", "Read only access to value attribute for user John. No access to value attribute for other users."));
    pJohn_RO->setAccessLevel(1);
    pJohn_RO->setUserAccessLevel(1);
    pJohn_RO->setValueRank(-1);
    pJohn_RO->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pJohn_RO->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pJohn_RO, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_John, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pJohn_RO;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_JohnJohn_RW(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pJohn_RW = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_John_John_RW, getTypeNamespace()));
    pJohn_RW->setBrowseName(UaQualifiedName("John_RW", getTypeNamespace()));
    pJohn_RW->setDisplayName(UaLocalizedText("", "John_RW"));
    pJohn_RW->setDescription(UaLocalizedText("", "Read and Write access to value attribute for user John. No access to value attribute for other users."));
    pJohn_RW->setAccessLevel(3);
    pJohn_RW->setUserAccessLevel(3);
    pJohn_RW->setValueRank(-1);
    pJohn_RW->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pJohn_RW->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pJohn_RW, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_John, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pJohn_RW;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_JohnJohn_WO(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pJohn_WO = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_John_John_WO, getTypeNamespace()));
    pJohn_WO->setBrowseName(UaQualifiedName("John_WO", getTypeNamespace()));
    pJohn_WO->setDisplayName(UaLocalizedText("", "John_WO"));
    pJohn_WO->setDescription(UaLocalizedText("", "Write only access to value attribute for user John. No access to value attribute for other users."));
    pJohn_WO->setAccessLevel(2);
    pJohn_WO->setUserAccessLevel(2);
    pJohn_WO->setValueRank(-1);
    pJohn_WO->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pJohn_WO->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pJohn_WO, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_John, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pJohn_WO;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_Operators(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pAccess_Operators = new UaBase::Object(UaNodeId(DemoId_Demo_AccessRights_Access_Operators, getTypeNamespace()));
    pAccess_Operators->setBrowseName(UaQualifiedName("Access_Operators", getTypeNamespace()));
    pAccess_Operators->setDisplayName(UaLocalizedText("", "Access_Operators"));
    pAccess_Operators->setDescription(UaLocalizedText("", "Contains elements with full access to attributes for users of group Operators."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pAccess_Operators, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccess_Operators;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_OperatorsAll_RO_Operators_RW(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_RO_Operators_RW = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_All_RO_Operators_RW, getTypeNamespace()));
    pAll_RO_Operators_RW->setBrowseName(UaQualifiedName("All_RO_Operators_RW", getTypeNamespace()));
    pAll_RO_Operators_RW->setDisplayName(UaLocalizedText("", "All_RO_Operators_RW"));
    pAll_RO_Operators_RW->setDescription(UaLocalizedText("", "Read and Write access to value attribute for members of Operators group. Read only access for other users."));
    pAll_RO_Operators_RW->setAccessLevel(3);
    pAll_RO_Operators_RW->setUserAccessLevel(3);
    pAll_RO_Operators_RW->setValueRank(-1);
    pAll_RO_Operators_RW->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pAll_RO_Operators_RW->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_RO_Operators_RW, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_Operators, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_RO_Operators_RW;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_OperatorsAll_WO_Operators_RW(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAll_WO_Operators_RW = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_All_WO_Operators_RW, getTypeNamespace()));
    pAll_WO_Operators_RW->setBrowseName(UaQualifiedName("All_WO_Operators_RW", getTypeNamespace()));
    pAll_WO_Operators_RW->setDisplayName(UaLocalizedText("", "All_WO_Operators_RW"));
    pAll_WO_Operators_RW->setDescription(UaLocalizedText("", "Read and Write access to value attribute for members of Operators group. Write only access for other users."));
    pAll_WO_Operators_RW->setAccessLevel(3);
    pAll_WO_Operators_RW->setUserAccessLevel(3);
    pAll_WO_Operators_RW->setValueRank(-1);
    pAll_WO_Operators_RW->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pAll_WO_Operators_RW->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAll_WO_Operators_RW, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_Operators, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_WO_Operators_RW;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_RO(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pOperators_RO = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_Operators_RO, getTypeNamespace()));
    pOperators_RO->setBrowseName(UaQualifiedName("Operators_RO", getTypeNamespace()));
    pOperators_RO->setDisplayName(UaLocalizedText("", "Operators_RO"));
    pOperators_RO->setDescription(UaLocalizedText("", "Read only access to value attribute for members of Operators group. No access to value attribute for other users."));
    pOperators_RO->setAccessLevel(1);
    pOperators_RO->setUserAccessLevel(1);
    pOperators_RO->setValueRank(-1);
    pOperators_RO->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pOperators_RO->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pOperators_RO, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_Operators, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pOperators_RO;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_RW(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pOperators_RW = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_Operators_RW, getTypeNamespace()));
    pOperators_RW->setBrowseName(UaQualifiedName("Operators_RW", getTypeNamespace()));
    pOperators_RW->setDisplayName(UaLocalizedText("", "Operators_RW"));
    pOperators_RW->setDescription(UaLocalizedText("", "Read and Write access to value attribute for members of Operators group. No access to value attribute for other users."));
    pOperators_RW->setAccessLevel(3);
    pOperators_RW->setUserAccessLevel(3);
    pOperators_RW->setValueRank(-1);
    pOperators_RW->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pOperators_RW->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pOperators_RW, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_Operators, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pOperators_RW;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_WO(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pOperators_WO = new UaBase::Variable(UaNodeId(DemoId_Demo_AccessRights_Access_Operators_Operators_WO, getTypeNamespace()));
    pOperators_WO->setBrowseName(UaQualifiedName("Operators_WO", getTypeNamespace()));
    pOperators_WO->setDisplayName(UaLocalizedText("", "Operators_WO"));
    pOperators_WO->setDescription(UaLocalizedText("", "Write only access to value attribute for members of Operators group. No access to value attribute for other users."));
    pOperators_WO->setAccessLevel(2);
    pOperators_WO->setUserAccessLevel(2);
    pOperators_WO->setValueRank(-1);
    pOperators_WO->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pOperators_WO->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pOperators_WO, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights_Access_Operators, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pOperators_WO;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsBrowse_All(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pBrowse_All = new UaBase::Object(UaNodeId(DemoId_Demo_AccessRights_Browse_All, getTypeNamespace()));
    pBrowse_All->setBrowseName(UaQualifiedName("Browse_All", getTypeNamespace()));
    pBrowse_All->setDisplayName(UaLocalizedText("", "Browse_All"));
    pBrowse_All->setDescription(UaLocalizedText("", "Browse access for all users."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pBrowse_All, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBrowse_All;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsBrowse_John(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pBrowse_John = new UaBase::Object(UaNodeId(DemoId_Demo_AccessRights_Browse_John, getTypeNamespace()));
    pBrowse_John->setBrowseName(UaQualifiedName("Browse_John", getTypeNamespace()));
    pBrowse_John->setDisplayName(UaLocalizedText("", "Browse_John"));
    pBrowse_John->setDescription(UaLocalizedText("", "Browse access for user John. Other users cannot browse this node."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pBrowse_John, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBrowse_John;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoAccessRightsBrowse_Operators(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pBrowse_Operators = new UaBase::Object(UaNodeId(DemoId_Demo_AccessRights_Browse_Operators, getTypeNamespace()));
    pBrowse_Operators->setBrowseName(UaQualifiedName("Browse_Operators", getTypeNamespace()));
    pBrowse_Operators->setDisplayName(UaLocalizedText("", "Browse_Operators"));
    pBrowse_Operators->setDescription(UaLocalizedText("", "Browse access for members of Operators group. Other users cannot browse this node."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pBrowse_Operators, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_AccessRights, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBrowse_Operators;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoMassfolder_Static(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pMassfolder_Static = new UaBase::Object(UaNodeId(DemoId_Demo_Massfolder_Static, getTypeNamespace()));
    pMassfolder_Static->setBrowseName(UaQualifiedName("Massfolder_Static", getTypeNamespace()));
    pMassfolder_Static->setDisplayName(UaLocalizedText("", "006_Massfolder_Static"));
    pMassfolder_Static->setDescription(UaLocalizedText("", "This folder contains static mass variables for performance and stress testing."));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pMassfolder_Static, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pMassfolder_Static;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoMassfolder_Dynamic(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pMassfolder_Dynamic = new UaBase::Object(UaNodeId(DemoId_Demo_Massfolder_Dynamic, getTypeNamespace()));
    pMassfolder_Dynamic->setBrowseName(UaQualifiedName("Massfolder_Dynamic", getTypeNamespace()));
    pMassfolder_Dynamic->setDisplayName(UaLocalizedText("", "007_Massfolder_Dynamic"));
    pMassfolder_Dynamic->setDescription(UaLocalizedText("", "This folder contains dynamic mass variables for performance and stress testing."));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pMassfolder_Dynamic, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pMassfolder_Dynamic;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicNodes(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDynamicNodes = new UaBase::Object(UaNodeId(DemoId_Demo_DynamicNodes, getTypeNamespace()));
    pDynamicNodes->setBrowseName(UaQualifiedName("DynamicNodes", getTypeNamespace()));
    pDynamicNodes->setDisplayName(UaLocalizedText("", "008_DynamicNodes"));
    pDynamicNodes->setDescription(UaLocalizedText("", "This folder demonstrates creating nodes dynamically and sending ModelChangeEvents."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pDynamicNodes, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDynamicNodes;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicNodesCreateDynamicNode(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_DynamicNodes_CreateDynamicNode, getTypeNamespace()), "CreateDynamicNode", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_DynamicNodes, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicNodesDeleteDynamicNode(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_DynamicNodes_DeleteDynamicNode, getTypeNamespace()), "DeleteDynamicNode", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_DynamicNodes, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoDynamicNodesNodeVersion(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNodeVersion = new UaBase::Variable(UaNodeId(DemoId_Demo_DynamicNodes_NodeVersion, getTypeNamespace()));
    pNodeVersion->setBrowseName(UaQualifiedName("NodeVersion", 0));
    pNodeVersion->setDisplayName(UaLocalizedText("", "NodeVersion"));
    pNodeVersion->setAccessLevel(3);
    pNodeVersion->setUserAccessLevel(3);
    pNodeVersion->setValueRank(-1);
    pNodeVersion->setDataTypeId(OpcUaId_String);
        defaultValue.setString("");
    pNodeVersion->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pNodeVersion, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_DynamicNodes, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNodeVersion;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoBoilerDemo(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pBoilerDemo = new UaBase::Object(UaNodeId(DemoId_Demo_BoilerDemo, getTypeNamespace()));
    pBoilerDemo->setBrowseName(UaQualifiedName("BoilerDemo", getTypeNamespace()));
    pBoilerDemo->setDisplayName(UaLocalizedText("", "009_BoilerDemo"));
    pBoilerDemo->setDescription(UaLocalizedText("", "In this folder you find a boiler demo which uses all OPC UA features."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pBoilerDemo, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBoilerDemo;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoBoilerDemoBoiler1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pBoiler1 = new UaBase::Object(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1, getTypeNamespace()));
    pBoiler1->setBrowseName(UaQualifiedName("Boiler1", getTypeNamespace()));
    pBoiler1->setDisplayName(UaLocalizedText("", "Boiler1"));

    UaBase::FullReference reference;

    // Fill
    UaBase::Method *pFill = new UaBase::Method(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_Fill, getTypeNamespace()));
    pFill->setBrowseName(UaQualifiedName("Fill", getTypeNamespace()));
    pFill->setDisplayName(UaLocalizedText("", "Fill"));
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pBoiler1);
    reference.setTargetNode(pFill);
    pBoiler1->addFullReference(reference);
    pFill->addFullReference(reference);
    pFill->setParentNodeId(pBoiler1->nodeId());

    // Fill -> InputArguments
    UaBase::Variable *pFillInputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_Fill_InputArguments, getTypeNamespace()));
    pFillInputArguments->setBrowseName(UaQualifiedName("InputArguments", 0));
    pFillInputArguments->setDisplayName(UaLocalizedText("", "InputArguments"));
    pFillInputArguments->setAccessLevel(1);
    pFillInputArguments->setUserAccessLevel(1);
    pFillInputArguments->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 1;
    pFillInputArguments->setArrayDimensions(arrayDimensions);
    pFillInputArguments->setDataTypeId(OpcUaId_Argument);
    {
        UaArguments valInputArgumentsArray;
        valInputArgumentsArray.create(1);
        UaArgument argument;
        argument.setName("SetPoint");
        argument.setDataType(OpcUaId_Double);
        argument.setDescription(UaLocalizedText("", "FillLevel Setpoint [l]"));
        argument.setValueRank(-1);
        argument.copyTo(&valInputArgumentsArray[ 0 ]);
        valInputArgumentsArray.toVariant(defaultValue);
    }
    pFillInputArguments->setValue(defaultValue);
    pFillInputArguments->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pFill);
    reference.setTargetNode(pFillInputArguments);
    pFill->addFullReference(reference);
    pFillInputArguments->addFullReference(reference);
    pFillInputArguments->setParentNodeId(pFill->nodeId());

    // FillLevelSensor
    UaBase::Object *pFillLevelSensor = new UaBase::Object(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor, getTypeNamespace()));
    pFillLevelSensor->setBrowseName(UaQualifiedName("FillLevelSensor", getTypeNamespace()));
    pFillLevelSensor->setDisplayName(UaLocalizedText("", "FillLevelSensor"));
    pFillLevelSensor->setTypeDefinitionId(UaNodeId(DemoId_FillLevelSensorType, getNameSpaceIndex()));
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pBoiler1);
    reference.setTargetNode(pFillLevelSensor);
    pBoiler1->addFullReference(reference);
    pFillLevelSensor->addFullReference(reference);
    pFillLevelSensor->setParentNodeId(pBoiler1->nodeId());

    // FillLevelSensor -> FillLevel
    UaBase::Variable *pFillLevelSensorFillLevel = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor_FillLevel, getTypeNamespace()));
    pFillLevelSensorFillLevel->setBrowseName(UaQualifiedName("FillLevel", getTypeNamespace()));
    pFillLevelSensorFillLevel->setDisplayName(UaLocalizedText("", "FillLevel"));
    pFillLevelSensorFillLevel->setAccessLevel(3);
    pFillLevelSensorFillLevel->setUserAccessLevel(3);
    pFillLevelSensorFillLevel->setValueRank(-1);
    pFillLevelSensorFillLevel->setDataTypeId(OpcUaId_Double);
    defaultValue.setDouble(0.0);
    pFillLevelSensorFillLevel->setValue(defaultValue);
    pFillLevelSensorFillLevel->setTypeDefinitionId(OpcUaId_AnalogItemType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pFillLevelSensor);
    reference.setTargetNode(pFillLevelSensorFillLevel);
    pFillLevelSensor->addFullReference(reference);
    pFillLevelSensorFillLevel->addFullReference(reference);
    pFillLevelSensorFillLevel->setParentNodeId(pFillLevelSensor->nodeId());

    // FillLevelSensor -> FillLevel -> EngineeringUnits
    UaBase::Variable *pFillLevelSensorFillLevelEngineeringUnits = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor_FillLevel_EngineeringUnits, getTypeNamespace()));
    pFillLevelSensorFillLevelEngineeringUnits->setBrowseName(UaQualifiedName("EngineeringUnits", 0));
    pFillLevelSensorFillLevelEngineeringUnits->setDisplayName(UaLocalizedText("", "EngineeringUnits"));
    pFillLevelSensorFillLevelEngineeringUnits->setAccessLevel(1);
    pFillLevelSensorFillLevelEngineeringUnits->setUserAccessLevel(1);
    pFillLevelSensorFillLevelEngineeringUnits->setValueRank(-1);
    pFillLevelSensorFillLevelEngineeringUnits->setDataTypeId(OpcUaId_EUInformation);
    {
        UaEUInformation valEngineeringUnits = UaEUInformation(
            "",
            -1,
            UaLocalizedText("", ""),
            UaLocalizedText("", ""));
        valEngineeringUnits.toVariant(defaultValue);
    }
    pFillLevelSensorFillLevelEngineeringUnits->setValue(defaultValue);
    pFillLevelSensorFillLevelEngineeringUnits->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pFillLevelSensorFillLevel);
    reference.setTargetNode(pFillLevelSensorFillLevelEngineeringUnits);
    pFillLevelSensorFillLevel->addFullReference(reference);
    pFillLevelSensorFillLevelEngineeringUnits->addFullReference(reference);
    pFillLevelSensorFillLevelEngineeringUnits->setParentNodeId(pFillLevelSensorFillLevel->nodeId());

    // FillLevelSensor -> FillLevel -> EURange
    UaBase::Variable *pFillLevelSensorFillLevelEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor_FillLevel_EURange, getTypeNamespace()));
    pFillLevelSensorFillLevelEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pFillLevelSensorFillLevelEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pFillLevelSensorFillLevelEURange->setAccessLevel(1);
    pFillLevelSensorFillLevelEURange->setUserAccessLevel(1);
    pFillLevelSensorFillLevelEURange->setValueRank(-1);
    pFillLevelSensorFillLevelEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 0);
        valEURange.toVariant(defaultValue);
    }
    pFillLevelSensorFillLevelEURange->setValue(defaultValue);
    pFillLevelSensorFillLevelEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pFillLevelSensorFillLevel);
    reference.setTargetNode(pFillLevelSensorFillLevelEURange);
    pFillLevelSensorFillLevel->addFullReference(reference);
    pFillLevelSensorFillLevelEURange->addFullReference(reference);
    pFillLevelSensorFillLevelEURange->setParentNodeId(pFillLevelSensorFillLevel->nodeId());

    // FillLevelSetPoint
    UaBase::Variable *pFillLevelSetPoint = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_FillLevelSetPoint, getTypeNamespace()));
    pFillLevelSetPoint->setBrowseName(UaQualifiedName("FillLevelSetPoint", getTypeNamespace()));
    pFillLevelSetPoint->setDisplayName(UaLocalizedText("", "FillLevelSetPoint"));
    pFillLevelSetPoint->setAccessLevel(1);
    pFillLevelSetPoint->setUserAccessLevel(1);
    pFillLevelSetPoint->setValueRank(-1);
    pFillLevelSetPoint->setDataTypeId(OpcUaId_Double);
    defaultValue.setDouble(0.0);
    pFillLevelSetPoint->setValue(defaultValue);
    pFillLevelSetPoint->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pBoiler1);
    reference.setTargetNode(pFillLevelSetPoint);
    pBoiler1->addFullReference(reference);
    pFillLevelSetPoint->addFullReference(reference);
    pFillLevelSetPoint->setParentNodeId(pBoiler1->nodeId());

    // Heat
    UaBase::Method *pHeat = new UaBase::Method(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_Heat, getTypeNamespace()));
    pHeat->setBrowseName(UaQualifiedName("Heat", getTypeNamespace()));
    pHeat->setDisplayName(UaLocalizedText("", "Heat"));
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pBoiler1);
    reference.setTargetNode(pHeat);
    pBoiler1->addFullReference(reference);
    pHeat->addFullReference(reference);
    pHeat->setParentNodeId(pBoiler1->nodeId());

    // Heat -> InputArguments
    UaBase::Variable *pHeatInputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_Heat_InputArguments, getTypeNamespace()));
    pHeatInputArguments->setBrowseName(UaQualifiedName("InputArguments", 0));
    pHeatInputArguments->setDisplayName(UaLocalizedText("", "InputArguments"));
    pHeatInputArguments->setAccessLevel(1);
    pHeatInputArguments->setUserAccessLevel(1);
    pHeatInputArguments->setValueRank(1);
    arrayDimensions.create(1);
    arrayDimensions[0] = 1;
    pHeatInputArguments->setArrayDimensions(arrayDimensions);
    pHeatInputArguments->setDataTypeId(OpcUaId_Argument);
    {
        UaArguments valInputArgumentsArray;
        valInputArgumentsArray.create(1);
        UaArgument argument;
        argument.setName("SetPoint");
        argument.setDataType(OpcUaId_Double);
        argument.setDescription(UaLocalizedText("", "Temperature Setpoint [\302\260C]"));
        argument.setValueRank(-1);
        argument.copyTo(&valInputArgumentsArray[ 0 ]);
        valInputArgumentsArray.toVariant(defaultValue);
    }
    pHeatInputArguments->setValue(defaultValue);
    pHeatInputArguments->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pHeat);
    reference.setTargetNode(pHeatInputArguments);
    pHeat->addFullReference(reference);
    pHeatInputArguments->addFullReference(reference);
    pHeatInputArguments->setParentNodeId(pHeat->nodeId());

    // HeaterStatus
    UaBase::Variable *pHeaterStatus = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_HeaterStatus, getTypeNamespace()));
    pHeaterStatus->setBrowseName(UaQualifiedName("HeaterStatus", getTypeNamespace()));
    pHeaterStatus->setDisplayName(UaLocalizedText("", "HeaterStatus"));
    pHeaterStatus->setAccessLevel(1);
    pHeaterStatus->setUserAccessLevel(1);
    pHeaterStatus->setValueRank(-1);
    pHeaterStatus->setDataTypeId(UaNodeId(DemoId_HeaterStatus, getNameSpaceIndex()));
    defaultValue.setInt32((OpcUa_Int32) Demo::HeaterStatus_Off);
    pHeaterStatus->setValue(defaultValue);
    pHeaterStatus->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pBoiler1);
    reference.setTargetNode(pHeaterStatus);
    pBoiler1->addFullReference(reference);
    pHeaterStatus->addFullReference(reference);
    pHeaterStatus->setParentNodeId(pBoiler1->nodeId());

    // TemperatureSensor
    UaBase::Object *pTemperatureSensor = new UaBase::Object(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor, getTypeNamespace()));
    pTemperatureSensor->setBrowseName(UaQualifiedName("TemperatureSensor", getTypeNamespace()));
    pTemperatureSensor->setDisplayName(UaLocalizedText("", "TemperatureSensor"));
    pTemperatureSensor->setTypeDefinitionId(UaNodeId(DemoId_TemperatureSensorType, getNameSpaceIndex()));
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pBoiler1);
    reference.setTargetNode(pTemperatureSensor);
    pBoiler1->addFullReference(reference);
    pTemperatureSensor->addFullReference(reference);
    pTemperatureSensor->setParentNodeId(pBoiler1->nodeId());

    // TemperatureSensor -> Temperature
    UaBase::Variable *pTemperatureSensorTemperature = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor_Temperature, getTypeNamespace()));
    pTemperatureSensorTemperature->setBrowseName(UaQualifiedName("Temperature", getTypeNamespace()));
    pTemperatureSensorTemperature->setDisplayName(UaLocalizedText("", "Temperature"));
    pTemperatureSensorTemperature->setAccessLevel(3);
    pTemperatureSensorTemperature->setUserAccessLevel(3);
    pTemperatureSensorTemperature->setValueRank(-1);
    pTemperatureSensorTemperature->setDataTypeId(OpcUaId_Double);
    defaultValue.setDouble(0.0);
    pTemperatureSensorTemperature->setValue(defaultValue);
    pTemperatureSensorTemperature->setTypeDefinitionId(OpcUaId_AnalogItemType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pTemperatureSensor);
    reference.setTargetNode(pTemperatureSensorTemperature);
    pTemperatureSensor->addFullReference(reference);
    pTemperatureSensorTemperature->addFullReference(reference);
    pTemperatureSensorTemperature->setParentNodeId(pTemperatureSensor->nodeId());

    // TemperatureSensor -> Temperature -> EngineeringUnits
    UaBase::Variable *pTemperatureSensorTemperatureEngineeringUnits = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor_Temperature_EngineeringUnits, getTypeNamespace()));
    pTemperatureSensorTemperatureEngineeringUnits->setBrowseName(UaQualifiedName("EngineeringUnits", 0));
    pTemperatureSensorTemperatureEngineeringUnits->setDisplayName(UaLocalizedText("", "EngineeringUnits"));
    pTemperatureSensorTemperatureEngineeringUnits->setAccessLevel(1);
    pTemperatureSensorTemperatureEngineeringUnits->setUserAccessLevel(1);
    pTemperatureSensorTemperatureEngineeringUnits->setValueRank(-1);
    pTemperatureSensorTemperatureEngineeringUnits->setDataTypeId(OpcUaId_EUInformation);
    {
        UaEUInformation valEngineeringUnits = UaEUInformation(
            "",
            -1,
            UaLocalizedText("", ""),
            UaLocalizedText("", ""));
        valEngineeringUnits.toVariant(defaultValue);
    }
    pTemperatureSensorTemperatureEngineeringUnits->setValue(defaultValue);
    pTemperatureSensorTemperatureEngineeringUnits->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTemperatureSensorTemperature);
    reference.setTargetNode(pTemperatureSensorTemperatureEngineeringUnits);
    pTemperatureSensorTemperature->addFullReference(reference);
    pTemperatureSensorTemperatureEngineeringUnits->addFullReference(reference);
    pTemperatureSensorTemperatureEngineeringUnits->setParentNodeId(pTemperatureSensorTemperature->nodeId());

    // TemperatureSensor -> Temperature -> EURange
    UaBase::Variable *pTemperatureSensorTemperatureEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor_Temperature_EURange, getTypeNamespace()));
    pTemperatureSensorTemperatureEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pTemperatureSensorTemperatureEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pTemperatureSensorTemperatureEURange->setAccessLevel(1);
    pTemperatureSensorTemperatureEURange->setUserAccessLevel(1);
    pTemperatureSensorTemperatureEURange->setValueRank(-1);
    pTemperatureSensorTemperatureEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 0);
        valEURange.toVariant(defaultValue);
    }
    pTemperatureSensorTemperatureEURange->setValue(defaultValue);
    pTemperatureSensorTemperatureEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTemperatureSensorTemperature);
    reference.setTargetNode(pTemperatureSensorTemperatureEURange);
    pTemperatureSensorTemperature->addFullReference(reference);
    pTemperatureSensorTemperatureEURange->addFullReference(reference);
    pTemperatureSensorTemperatureEURange->setParentNodeId(pTemperatureSensorTemperature->nodeId());

    // TemperatureSetPoint
    UaBase::Variable *pTemperatureSetPoint = new UaBase::Variable(UaNodeId(DemoId_Demo_BoilerDemo_Boiler1_TemperatureSetPoint, getTypeNamespace()));
    pTemperatureSetPoint->setBrowseName(UaQualifiedName("TemperatureSetPoint", getTypeNamespace()));
    pTemperatureSetPoint->setDisplayName(UaLocalizedText("", "TemperatureSetPoint"));
    pTemperatureSetPoint->setAccessLevel(1);
    pTemperatureSetPoint->setUserAccessLevel(1);
    pTemperatureSetPoint->setValueRank(-1);
    pTemperatureSetPoint->setDataTypeId(OpcUaId_Double);
    defaultValue.setDouble(0.0);
    pTemperatureSetPoint->setValue(defaultValue);
    pTemperatureSetPoint->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pBoiler1);
    reference.setTargetNode(pTemperatureSetPoint);
    pBoiler1->addFullReference(reference);
    pTemperatureSetPoint->addFullReference(reference);
    pTemperatureSetPoint->setParentNodeId(pBoiler1->nodeId());
    Demo::BoilerType *pObjectType = new Demo::BoilerType(pBoiler1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_BoilerDemo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFill;
    delete pFillInputArguments;
    delete pFillLevelSensor;
    delete pFillLevelSensorFillLevel;
    delete pFillLevelSensorFillLevelEngineeringUnits;
    delete pFillLevelSensorFillLevelEURange;
    delete pFillLevelSetPoint;
    delete pHeat;
    delete pHeatInputArguments;
    delete pHeaterStatus;
    delete pTemperatureSensor;
    delete pTemperatureSensorTemperature;
    delete pTemperatureSensorTemperatureEngineeringUnits;
    delete pTemperatureSensorTemperatureEURange;
    delete pTemperatureSetPoint;
    delete pBoiler1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTT(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pCTT = new UaBase::Object(UaNodeId(DemoId_Demo_CTT, getTypeNamespace()));
    pCTT->setBrowseName(UaQualifiedName("CTT", getTypeNamespace()));
    pCTT->setDisplayName(UaLocalizedText("", "010_ComplianceTest"));
    pCTT->setDescription(UaLocalizedText("", "This folder contains variable for the OPC UA Compliance Test Tool."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pCTT, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pCTT;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTMethods(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pMethods = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Methods, getTypeNamespace()));
    pMethods->setBrowseName(UaQualifiedName("Methods", getTypeNamespace()));
    pMethods->setDisplayName(UaLocalizedText("", "Methods"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pMethods, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pMethods;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTMethodsMethodIO(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_CTT_Methods_MethodIO, getTypeNamespace()), "MethodIO", getTypeNamespace());
    pMethod->setDescription(UaLocalizedText("", "Adds 2 unsigned integers. This method is added for compliance testing only."));
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Methods, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    UaArgument argument;

    // InputArguments
    UaBase::Variable *pInputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Methods_MethodIO_InputArguments, getTypeNamespace()));
    pInputArguments->setBrowseName(UaQualifiedName("InputArguments", 0));
    pInputArguments->setDisplayName(UaLocalizedText("", "InputArguments"));
    pInputArguments->setAccessLevel(1);
    pInputArguments->setUserAccessLevel(1);
    pInputArguments->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 2;
    pInputArguments->setArrayDimensions(arrayDimensions);
    pInputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valInputArguments;
    valInputArguments.create(2);
    argument.setName("Summand1");
    argument.setDataType(OpcUaId_UInt32);
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 0 ]);
    argument.setName("Summand2");
    argument.setDataType(OpcUaId_UInt32);
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 1 ]);
    valInputArguments.toVariant(defaultValue);
    pInputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pInputArgumentsProperty = new OpcUa::PropertyType(pInputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pInputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pInputArguments;

    // OutputArguments
    UaBase::Variable *pOutputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Methods_MethodIO_OutputArguments, getTypeNamespace()));
    pOutputArguments->setBrowseName(UaQualifiedName("OutputArguments", 0));
    pOutputArguments->setDisplayName(UaLocalizedText("", "OutputArguments"));
    pOutputArguments->setAccessLevel(1);
    pOutputArguments->setUserAccessLevel(1);
    pOutputArguments->setValueRank(1);
    arrayDimensions.create(1);
    arrayDimensions[0] = 1;
    pOutputArguments->setArrayDimensions(arrayDimensions);
    pOutputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valOutputArguments;
    valOutputArguments.create(1);
    argument.setName("Sum");
    argument.setDataType(OpcUaId_UInt32);
    argument.setValueRank(-1);
    argument.copyTo(&valOutputArguments[ 0 ]);
    valOutputArguments.toVariant(defaultValue);
    pOutputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pOutputArgumentsProperty = new OpcUa::PropertyType(pOutputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pOutputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pOutputArguments;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTMethodsMethodNoArgs(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_CTT_Methods_MethodNoArgs, getTypeNamespace()), "MethodNoArgs", getTypeNamespace());
    pMethod->setDescription(UaLocalizedText("", "Method without Arguments and without any functionality. This method is added for compliance testing only."));
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Methods, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTNodeClasses(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pNodeClasses = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_NodeClasses, getTypeNamespace()));
    pNodeClasses->setBrowseName(UaQualifiedName("NodeClasses", getTypeNamespace()));
    pNodeClasses->setDisplayName(UaLocalizedText("", "NodeClasses"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pNodeClasses, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(pObjectType->nodeId(), OpcUaId_BaseDataVariableType, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(pObjectType->nodeId(), OpcUaId_BaseObjectType, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(pObjectType->nodeId(), OpcUaId_HierarchicalReferences, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(pObjectType->nodeId(), OpcUaId_String, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    delete pNodeClasses;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTNodeClassesMethod(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_CTT_NodeClasses_Method, getTypeNamespace()), "Method", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_NodeClasses, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTNodeClassesObject(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pObject = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_NodeClasses_Object, getTypeNamespace()));
    pObject->setBrowseName(UaQualifiedName("Object", getTypeNamespace()));
    pObject->setDisplayName(UaLocalizedText("", "Object"));

    OpcUa::BaseObjectType *pObjectType = new OpcUa::BaseObjectType(pObject, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_NodeClasses, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pObject;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTNodeClassesVariable(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pVariable = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_NodeClasses_Variable, getTypeNamespace()));
    pVariable->setBrowseName(UaQualifiedName("Variable", getTypeNamespace()));
    pVariable->setDisplayName(UaLocalizedText("", "Variable"));
    pVariable->setAccessLevel(3);
    pVariable->setUserAccessLevel(3);
    pVariable->setValueRank(-1);
    pVariable->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pVariable->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pVariable, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_NodeClasses, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pVariable;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPaths(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pPaths = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths, getTypeNamespace()));
    pPaths->setBrowseName(UaQualifiedName("Paths", getTypeNamespace()));
    pPaths->setDisplayName(UaLocalizedText("", "Paths"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pPaths, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pPaths;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder1 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1, getTypeNamespace()));
    pfolder1->setBrowseName(UaQualifiedName("folder1", getTypeNamespace()));
    pfolder1->setDisplayName(UaLocalizedText("", "folder1"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder2 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2, getTypeNamespace()));
    pfolder2->setBrowseName(UaQualifiedName("folder2", getTypeNamespace()));
    pfolder2->setDisplayName(UaLocalizedText("", "folder2"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder3 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3, getTypeNamespace()));
    pfolder3->setBrowseName(UaQualifiedName("folder3", getTypeNamespace()));
    pfolder3->setDisplayName(UaLocalizedText("", "folder3"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder4 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4, getTypeNamespace()));
    pfolder4->setBrowseName(UaQualifiedName("folder4", getTypeNamespace()));
    pfolder4->setDisplayName(UaLocalizedText("", "folder4"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder4, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder4;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder5 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5, getTypeNamespace()));
    pfolder5->setBrowseName(UaQualifiedName("folder5", getTypeNamespace()));
    pfolder5->setDisplayName(UaLocalizedText("", "folder5"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder5, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder5;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder6 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6, getTypeNamespace()));
    pfolder6->setBrowseName(UaQualifiedName("folder6", getTypeNamespace()));
    pfolder6->setDisplayName(UaLocalizedText("", "folder6"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder6, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder6;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder7 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7, getTypeNamespace()));
    pfolder7->setBrowseName(UaQualifiedName("folder7", getTypeNamespace()));
    pfolder7->setDisplayName(UaLocalizedText("", "folder7"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder7, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder7;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder8 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8, getTypeNamespace()));
    pfolder8->setBrowseName(UaQualifiedName("folder8", getTypeNamespace()));
    pfolder8->setDisplayName(UaLocalizedText("", "folder8"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder8, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder8;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8folder9(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder9 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8_folder9, getTypeNamespace()));
    pfolder9->setBrowseName(UaQualifiedName("folder9", getTypeNamespace()));
    pfolder9->setDisplayName(UaLocalizedText("", "folder9"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder9, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder9;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8folder9folder10(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pfolder10 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8_folder9_folder10, getTypeNamespace()));
    pfolder10->setBrowseName(UaQualifiedName("folder10", getTypeNamespace()));
    pfolder10->setDisplayName(UaLocalizedText("", "folder10"));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pfolder10, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8_folder9, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pfolder10;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferences(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pReferences = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References, getTypeNamespace()));
    pReferences->setBrowseName(UaQualifiedName("References", getTypeNamespace()));
    pReferences->setDisplayName(UaLocalizedText("", "References"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pReferences, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferences;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHas3ForwardReferences1 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences1, getTypeNamespace()));
    pHas3ForwardReferences1->setBrowseName(UaQualifiedName("Has3ForwardReferences1", getTypeNamespace()));
    pHas3ForwardReferences1->setDisplayName(UaLocalizedText("", "Has3ForwardReferences1"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHas3ForwardReferences1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHas3ForwardReferences1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode1 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences1_ReferencedNode1, getTypeNamespace()));
    pReferencedNode1->setBrowseName(UaQualifiedName("ReferencedNode1", getTypeNamespace()));
    pReferencedNode1->setDisplayName(UaLocalizedText("", "ReferencedNode1"));
    pReferencedNode1->setAccessLevel(3);
    pReferencedNode1->setUserAccessLevel(3);
    pReferencedNode1->setValueRank(-1);
    pReferencedNode1->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pReferencedNode1->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pReferencedNode1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences1, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode2 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences1_ReferencedNode2, getTypeNamespace()));
    pReferencedNode2->setBrowseName(UaQualifiedName("ReferencedNode2", getTypeNamespace()));
    pReferencedNode2->setDisplayName(UaLocalizedText("", "ReferencedNode2"));
    pReferencedNode2->setAccessLevel(3);
    pReferencedNode2->setUserAccessLevel(3);
    pReferencedNode2->setValueRank(-1);
    pReferencedNode2->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pReferencedNode2->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pReferencedNode2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences1, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode3 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences1_ReferencedNode3, getTypeNamespace()));
    pReferencedNode3->setBrowseName(UaQualifiedName("ReferencedNode3", getTypeNamespace()));
    pReferencedNode3->setDisplayName(UaLocalizedText("", "ReferencedNode3"));
    pReferencedNode3->setAccessLevel(3);
    pReferencedNode3->setUserAccessLevel(3);
    pReferencedNode3->setValueRank(-1);
    pReferencedNode3->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pReferencedNode3->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pReferencedNode3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences1, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHas3ForwardReferences2 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2, getTypeNamespace()));
    pHas3ForwardReferences2->setBrowseName(UaQualifiedName("Has3ForwardReferences2", getTypeNamespace()));
    pHas3ForwardReferences2->setDisplayName(UaLocalizedText("", "Has3ForwardReferences2"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHas3ForwardReferences2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHas3ForwardReferences2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences2BaseDataVariable(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBaseDataVariable = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_BaseDataVariable, getTypeNamespace()));
    pBaseDataVariable->setBrowseName(UaQualifiedName("BaseDataVariable", getTypeNamespace()));
    pBaseDataVariable->setDisplayName(UaLocalizedText("", "BaseDataVariable"));
    pBaseDataVariable->setAccessLevel(3);
    pBaseDataVariable->setUserAccessLevel(3);
    pBaseDataVariable->setValueRank(-1);
    pBaseDataVariable->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pBaseDataVariable->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBaseDataVariable, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBaseDataVariable;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method1, getTypeNamespace()), "Method1", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method2, getTypeNamespace()), "Method2", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method3, getTypeNamespace()), "Method3", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Object(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pObject = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Object, getTypeNamespace()));
    pObject->setBrowseName(UaQualifiedName("Object", getTypeNamespace()));
    pObject->setDisplayName(UaLocalizedText("", "Object"));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pObject, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pObject;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Property(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pProperty = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Property, getTypeNamespace()));
    pProperty->setBrowseName(UaQualifiedName("Property", getTypeNamespace()));
    pProperty->setDisplayName(UaLocalizedText("", "Property"));
    pProperty->setAccessLevel(3);
    pProperty->setUserAccessLevel(3);
    pProperty->setValueRank(-1);
    pProperty->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pProperty->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pProperty, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pProperty;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHas3ForwardReferences3 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3, getTypeNamespace()));
    pHas3ForwardReferences3->setBrowseName(UaQualifiedName("Has3ForwardReferences3", getTypeNamespace()));
    pHas3ForwardReferences3->setDisplayName(UaLocalizedText("", "Has3ForwardReferences3"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHas3ForwardReferences3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHas3ForwardReferences3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pReferencedNode1 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1, getTypeNamespace()));
    pReferencedNode1->setBrowseName(UaQualifiedName("ReferencedNode1", getTypeNamespace()));
    pReferencedNode1->setDisplayName(UaLocalizedText("", "ReferencedNode1"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pReferencedNode1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3InverseReferences(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHas3InverseReferences = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1_Has3InverseReferences, getTypeNamespace()));
    pHas3InverseReferences->setBrowseName(UaQualifiedName("Has3InverseReferences", getTypeNamespace()));
    pHas3InverseReferences->setDisplayName(UaLocalizedText("", "Has3InverseReferences"));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHas3InverseReferences, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHas3InverseReferences;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pReferencedNode2 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode2, getTypeNamespace()));
    pReferencedNode2->setBrowseName(UaQualifiedName("ReferencedNode2", getTypeNamespace()));
    pReferencedNode2->setDisplayName(UaLocalizedText("", "ReferencedNode2"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pReferencedNode2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pReferencedNode3 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode3, getTypeNamespace()));
    pReferencedNode3->setBrowseName(UaQualifiedName("ReferencedNode3", getTypeNamespace()));
    pReferencedNode3->setDisplayName(UaLocalizedText("", "ReferencedNode3"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pReferencedNode3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences4(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHas3ForwardReferences4 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences4, getTypeNamespace()));
    pHas3ForwardReferences4->setBrowseName(UaQualifiedName("Has3ForwardReferences4", getTypeNamespace()));
    pHas3ForwardReferences4->setDisplayName(UaLocalizedText("", "Has3ForwardReferences4"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHas3ForwardReferences4, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHas3ForwardReferences4;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode1 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences4_ReferencedNode1, getTypeNamespace()));
    pReferencedNode1->setBrowseName(UaQualifiedName("ReferencedNode1", getTypeNamespace()));
    pReferencedNode1->setDisplayName(UaLocalizedText("", "ReferencedNode1"));
    pReferencedNode1->setAccessLevel(3);
    pReferencedNode1->setUserAccessLevel(3);
    pReferencedNode1->setValueRank(-1);
    pReferencedNode1->setDataTypeId(OpcUaId_Byte);
        defaultValue.setByte(0);
    pReferencedNode1->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pReferencedNode1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences4, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode2 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences4_ReferencedNode2, getTypeNamespace()));
    pReferencedNode2->setBrowseName(UaQualifiedName("ReferencedNode2", getTypeNamespace()));
    pReferencedNode2->setDisplayName(UaLocalizedText("", "ReferencedNode2"));
    pReferencedNode2->setAccessLevel(3);
    pReferencedNode2->setUserAccessLevel(3);
    pReferencedNode2->setValueRank(-1);
    pReferencedNode2->setDataTypeId(OpcUaId_Byte);
        defaultValue.setByte(0);
    pReferencedNode2->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pReferencedNode2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences4, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode3 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences4_ReferencedNode3, getTypeNamespace()));
    pReferencedNode3->setBrowseName(UaQualifiedName("ReferencedNode3", getTypeNamespace()));
    pReferencedNode3->setDisplayName(UaLocalizedText("", "ReferencedNode3"));
    pReferencedNode3->setAccessLevel(3);
    pReferencedNode3->setUserAccessLevel(3);
    pReferencedNode3->setValueRank(-1);
    pReferencedNode3->setDataTypeId(OpcUaId_Byte);
        defaultValue.setByte(0);
    pReferencedNode3->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pReferencedNode3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences4, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences5(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHas3ForwardReferences5 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5, getTypeNamespace()));
    pHas3ForwardReferences5->setBrowseName(UaQualifiedName("Has3ForwardReferences5", getTypeNamespace()));
    pHas3ForwardReferences5->setDisplayName(UaLocalizedText("", "Has3ForwardReferences5"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHas3ForwardReferences5, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHas3ForwardReferences5;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pReferencedNode1 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode1, getTypeNamespace()));
    pReferencedNode1->setBrowseName(UaQualifiedName("ReferencedNode1", getTypeNamespace()));
    pReferencedNode1->setDisplayName(UaLocalizedText("", "ReferencedNode1"));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pReferencedNode1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode2 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode2, getTypeNamespace()));
    pReferencedNode2->setBrowseName(UaQualifiedName("ReferencedNode2", getTypeNamespace()));
    pReferencedNode2->setDisplayName(UaLocalizedText("", "ReferencedNode2"));
    pReferencedNode2->setAccessLevel(3);
    pReferencedNode2->setUserAccessLevel(3);
    pReferencedNode2->setValueRank(-1);
    pReferencedNode2->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pReferencedNode2->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pReferencedNode2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3, getTypeNamespace()), "ReferencedNode3", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHasInverseAndForwardReferences(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHasInverseAndForwardReferences = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_HasInverseAndForwardReferences, getTypeNamespace()));
    pHasInverseAndForwardReferences->setBrowseName(UaQualifiedName("HasInverseAndForwardReferences", getTypeNamespace()));
    pHasInverseAndForwardReferences->setDisplayName(UaLocalizedText("", "HasInverseAndForwardReferences"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHasInverseAndForwardReferences, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHasInverseAndForwardReferences;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHasInverseAndForwardReferencesReferencedNode(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_HasInverseAndForwardReferences_ReferencedNode, getTypeNamespace()));
    pReferencedNode->setBrowseName(UaQualifiedName("ReferencedNode", getTypeNamespace()));
    pReferencedNode->setDisplayName(UaLocalizedText("", "ReferencedNode"));
    pReferencedNode->setAccessLevel(3);
    pReferencedNode->setUserAccessLevel(3);
    pReferencedNode->setValueRank(-1);
    pReferencedNode->setDataTypeId(OpcUaId_UInt32);
        defaultValue.setUInt32(0);
    pReferencedNode->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pReferencedNode, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_HasInverseAndForwardReferences, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypes(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pHasReferencesWithDifferentParentTypes = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes, getTypeNamespace()));
    pHasReferencesWithDifferentParentTypes->setBrowseName(UaQualifiedName("HasReferencesWithDifferentParentTypes", getTypeNamespace()));
    pHasReferencesWithDifferentParentTypes->setDisplayName(UaLocalizedText("", "HasReferencesWithDifferentParentTypes"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pHasReferencesWithDifferentParentTypes, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pHasReferencesWithDifferentParentTypes;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode1 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode1, getTypeNamespace()));
    pReferencedNode1->setBrowseName(UaQualifiedName("ReferencedNode1", getTypeNamespace()));
    pReferencedNode1->setDisplayName(UaLocalizedText("", "ReferencedNode1"));
    pReferencedNode1->setAccessLevel(3);
    pReferencedNode1->setUserAccessLevel(3);
    pReferencedNode1->setValueRank(-1);
    pReferencedNode1->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pReferencedNode1->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pReferencedNode1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode2 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode2, getTypeNamespace()));
    pReferencedNode2->setBrowseName(UaQualifiedName("ReferencedNode2", getTypeNamespace()));
    pReferencedNode2->setDisplayName(UaLocalizedText("", "ReferencedNode2"));
    pReferencedNode2->setAccessLevel(3);
    pReferencedNode2->setUserAccessLevel(3);
    pReferencedNode2->setValueRank(-1);
    pReferencedNode2->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pReferencedNode2->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pReferencedNode2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pReferencedNode3 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode3, getTypeNamespace()));
    pReferencedNode3->setBrowseName(UaQualifiedName("ReferencedNode3", getTypeNamespace()));
    pReferencedNode3->setDisplayName(UaLocalizedText("", "ReferencedNode3"));
    pReferencedNode3->setAccessLevel(3);
    pReferencedNode3->setUserAccessLevel(3);
    pReferencedNode3->setValueRank(-1);
    pReferencedNode3->setDataTypeId(OpcUaId_Double);
        defaultValue.setDouble(0.0);
    pReferencedNode3->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pReferencedNode3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode4(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pReferencedNode4 = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode4, getTypeNamespace()));
    pReferencedNode4->setBrowseName(UaQualifiedName("ReferencedNode4", getTypeNamespace()));
    pReferencedNode4->setDisplayName(UaLocalizedText("", "ReferencedNode4"));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pReferencedNode4, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pReferencedNode4;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTSecurityAccess(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pSecurityAccess = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_SecurityAccess, getTypeNamespace()));
    pSecurityAccess->setBrowseName(UaQualifiedName("SecurityAccess", getTypeNamespace()));
    pSecurityAccess->setDisplayName(UaLocalizedText("", "SecurityAccess"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pSecurityAccess, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSecurityAccess;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAccessLevel_CurrentRead = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead, getTypeNamespace()));
    pAccessLevel_CurrentRead->setBrowseName(UaQualifiedName("AccessLevel_CurrentRead", getTypeNamespace()));
    pAccessLevel_CurrentRead->setDisplayName(UaLocalizedText("", "AccessLevel_CurrentRead"));
    pAccessLevel_CurrentRead->setDescription(UaLocalizedText("", "A Node whose AccessLevel attribute conatins 'CurrentRead'. The UserAccessLevel should match the AccessLevel so that it is not more restrictive."));
    pAccessLevel_CurrentRead->setAccessLevel(3);
    pAccessLevel_CurrentRead->setUserAccessLevel(3);
    pAccessLevel_CurrentRead->setValueRank(-1);
    pAccessLevel_CurrentRead->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pAccessLevel_CurrentRead->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAccessLevel_CurrentRead, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_SecurityAccess, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccessLevel_CurrentRead;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead_NotCurrentWrite(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAccessLevel_CurrentRead_NotCurrentWrite = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead_NotCurrentWrite, getTypeNamespace()));
    pAccessLevel_CurrentRead_NotCurrentWrite->setBrowseName(UaQualifiedName("AccessLevel_CurrentRead_NotCurrentWrite", getTypeNamespace()));
    pAccessLevel_CurrentRead_NotCurrentWrite->setDisplayName(UaLocalizedText("", "AccessLevel_CurrentRead_NotCurrentWrite"));
    pAccessLevel_CurrentRead_NotCurrentWrite->setDescription(UaLocalizedText("", "A Node whose UserAccessLevel contains 'CurrentRead', but explicitly does not include 'CurrentWrite'."));
    pAccessLevel_CurrentRead_NotCurrentWrite->setAccessLevel(3);
    pAccessLevel_CurrentRead_NotCurrentWrite->setUserAccessLevel(3);
    pAccessLevel_CurrentRead_NotCurrentWrite->setValueRank(-1);
    pAccessLevel_CurrentRead_NotCurrentWrite->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pAccessLevel_CurrentRead_NotCurrentWrite->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAccessLevel_CurrentRead_NotCurrentWrite, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_SecurityAccess, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccessLevel_CurrentRead_NotCurrentWrite;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead_NotUser(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAccessLevel_CurrentRead_NotUser = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead_NotUser, getTypeNamespace()));
    pAccessLevel_CurrentRead_NotUser->setBrowseName(UaQualifiedName("AccessLevel_CurrentRead_NotUser", getTypeNamespace()));
    pAccessLevel_CurrentRead_NotUser->setDisplayName(UaLocalizedText("", "AccessLevel_CurrentRead_NotUser"));
    pAccessLevel_CurrentRead_NotUser->setDescription(UaLocalizedText("", "A Node whose AccessLevel attribute contains 'CurrentRead' but the 'UserAccessLevel' does not contain 'CurrentRead'."));
    pAccessLevel_CurrentRead_NotUser->setAccessLevel(3);
    pAccessLevel_CurrentRead_NotUser->setUserAccessLevel(3);
    pAccessLevel_CurrentRead_NotUser->setValueRank(-1);
    pAccessLevel_CurrentRead_NotUser->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pAccessLevel_CurrentRead_NotUser->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAccessLevel_CurrentRead_NotUser, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_SecurityAccess, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccessLevel_CurrentRead_NotUser;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAccessLevel_CurrentWrite = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite, getTypeNamespace()));
    pAccessLevel_CurrentWrite->setBrowseName(UaQualifiedName("AccessLevel_CurrentWrite", getTypeNamespace()));
    pAccessLevel_CurrentWrite->setDisplayName(UaLocalizedText("", "AccessLevel_CurrentWrite"));
    pAccessLevel_CurrentWrite->setDescription(UaLocalizedText("", "A Node whose AccessLevel attribute contains 'CurrentWrite'. The UserAccessLevel should match the AccessLevel so that it is not more restrictive."));
    pAccessLevel_CurrentWrite->setAccessLevel(3);
    pAccessLevel_CurrentWrite->setUserAccessLevel(3);
    pAccessLevel_CurrentWrite->setValueRank(-1);
    pAccessLevel_CurrentWrite->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pAccessLevel_CurrentWrite->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAccessLevel_CurrentWrite, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_SecurityAccess, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccessLevel_CurrentWrite;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite_NotCurrentRead(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAccessLevel_CurrentWrite_NotCurrentRead = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite_NotCurrentRead, getTypeNamespace()));
    pAccessLevel_CurrentWrite_NotCurrentRead->setBrowseName(UaQualifiedName("AccessLevel_CurrentWrite_NotCurrentRead", getTypeNamespace()));
    pAccessLevel_CurrentWrite_NotCurrentRead->setDisplayName(UaLocalizedText("", "AccessLevel_CurrentWrite_NotCurrentRead"));
    pAccessLevel_CurrentWrite_NotCurrentRead->setDescription(UaLocalizedText("", "A Node whose UserAccessLevel contains CurrentWrite, but explicitly does not include 'CurrentRead'."));
    pAccessLevel_CurrentWrite_NotCurrentRead->setAccessLevel(3);
    pAccessLevel_CurrentWrite_NotCurrentRead->setUserAccessLevel(3);
    pAccessLevel_CurrentWrite_NotCurrentRead->setValueRank(-1);
    pAccessLevel_CurrentWrite_NotCurrentRead->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pAccessLevel_CurrentWrite_NotCurrentRead->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAccessLevel_CurrentWrite_NotCurrentRead, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_SecurityAccess, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccessLevel_CurrentWrite_NotCurrentRead;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite_NotUser(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pAccessLevel_CurrentWrite_NotUser = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite_NotUser, getTypeNamespace()));
    pAccessLevel_CurrentWrite_NotUser->setBrowseName(UaQualifiedName("AccessLevel_CurrentWrite_NotUser", getTypeNamespace()));
    pAccessLevel_CurrentWrite_NotUser->setDisplayName(UaLocalizedText("", "AccessLevel_CurrentWrite_NotUser"));
    pAccessLevel_CurrentWrite_NotUser->setDescription(UaLocalizedText("", "A Node whose AccessLevel attribute contains 'CurrentWrite' but the UserAccessLevel does NOT contain 'CurrentWrite'"));
    pAccessLevel_CurrentWrite_NotUser->setAccessLevel(3);
    pAccessLevel_CurrentWrite_NotUser->setUserAccessLevel(3);
    pAccessLevel_CurrentWrite_NotUser->setValueRank(-1);
    pAccessLevel_CurrentWrite_NotUser->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pAccessLevel_CurrentWrite_NotUser->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pAccessLevel_CurrentWrite_NotUser, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_SecurityAccess, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAccessLevel_CurrentWrite_NotUser;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStatic(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pStatic = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static, getTypeNamespace()));
    pStatic->setBrowseName(UaQualifiedName("Static", getTypeNamespace()));
    pStatic->setDisplayName(UaLocalizedText("", "Static"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pStatic, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStatic;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_Profiles(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pAll_Profiles = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles, getTypeNamespace()));
    pAll_Profiles->setBrowseName(UaQualifiedName("All_Profiles", getTypeNamespace()));
    pAll_Profiles->setDisplayName(UaLocalizedText("", "All Profiles"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pAll_Profiles, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAll_Profiles;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArrays(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pArrays = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getTypeNamespace()));
    pArrays->setBrowseName(UaQualifiedName("Arrays", getTypeNamespace()));
    pArrays->setDisplayName(UaLocalizedText("", "Arrays"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pArrays, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pArrays;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysBool(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBool = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Bool, getTypeNamespace()));
    pBool->setBrowseName(UaQualifiedName("Bool", getTypeNamespace()));
    pBool->setDisplayName(UaLocalizedText("", "Bool"));
    pBool->setAccessLevel(3);
    pBool->setUserAccessLevel(3);
    pBool->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pBool->setArrayDimensions(arrayDimensions);
    pBool->setDataTypeId(OpcUaId_Boolean);
    {
        UaBooleanArray valBoolArray;
        defaultValue.setBoolArray(valBoolArray);
    }
    pBool->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBool, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBool;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pByte->setArrayDimensions(arrayDimensions);
    pByte->setDataTypeId(OpcUaId_Byte);
    {
        UaByteArray valByteArray;
        defaultValue.setByteArray(valByteArray);
    }
    pByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysByteString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByteString = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_ByteString, getTypeNamespace()));
    pByteString->setBrowseName(UaQualifiedName("ByteString", getTypeNamespace()));
    pByteString->setDisplayName(UaLocalizedText("", "ByteString"));
    pByteString->setAccessLevel(3);
    pByteString->setUserAccessLevel(3);
    pByteString->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pByteString->setArrayDimensions(arrayDimensions);
    pByteString->setDataTypeId(OpcUaId_ByteString);
    {
        UaByteStringArray valByteStringArray;
        defaultValue.setByteStringArray(valByteStringArray);
    }
    pByteString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByteString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByteString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysDateTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDateTime = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_DateTime, getTypeNamespace()));
    pDateTime->setBrowseName(UaQualifiedName("DateTime", getTypeNamespace()));
    pDateTime->setDisplayName(UaLocalizedText("", "DateTime"));
    pDateTime->setAccessLevel(3);
    pDateTime->setUserAccessLevel(3);
    pDateTime->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDateTime->setArrayDimensions(arrayDimensions);
    pDateTime->setDataTypeId(OpcUaId_DateTime);
    {
        UaDateTimeArray valDateTimeArray;
        defaultValue.setDateTimeArray(valDateTimeArray);
    }
    pDateTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDateTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDateTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDouble->setArrayDimensions(arrayDimensions);
    pDouble->setDataTypeId(OpcUaId_Double);
    {
        UaDoubleArray valDoubleArray;
        defaultValue.setDoubleArray(valDoubleArray);
    }
    pDouble->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysDuration(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDuration = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Duration, getTypeNamespace()));
    pDuration->setBrowseName(UaQualifiedName("Duration", getTypeNamespace()));
    pDuration->setDisplayName(UaLocalizedText("", "Duration"));
    pDuration->setAccessLevel(3);
    pDuration->setUserAccessLevel(3);
    pDuration->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDuration->setArrayDimensions(arrayDimensions);
    pDuration->setDataTypeId(OpcUaId_Duration);
    {
        UaDoubleArray valDurationArray;
        defaultValue.setDoubleArray(valDurationArray);
    }
    pDuration->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDuration, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDuration;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pFloat->setArrayDimensions(arrayDimensions);
    pFloat->setDataTypeId(OpcUaId_Float);
    {
        UaFloatArray valFloatArray;
        defaultValue.setFloatArray(valFloatArray);
    }
    pFloat->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysGuid(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pGuid = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Guid, getTypeNamespace()));
    pGuid->setBrowseName(UaQualifiedName("Guid", getTypeNamespace()));
    pGuid->setDisplayName(UaLocalizedText("", "Guid"));
    pGuid->setAccessLevel(3);
    pGuid->setUserAccessLevel(3);
    pGuid->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pGuid->setArrayDimensions(arrayDimensions);
    pGuid->setDataTypeId(OpcUaId_Guid);
    {
        UaGuidArray valGuidArray;
        defaultValue.setGuidArray(valGuidArray);
    }
    pGuid->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pGuid, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pGuid;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt16->setArrayDimensions(arrayDimensions);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        UaInt16Array valInt16Array;
        defaultValue.setInt16Array(valInt16Array);
    }
    pInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt32->setArrayDimensions(arrayDimensions);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        UaInt32Array valInt32Array;
        defaultValue.setInt32Array(valInt32Array);
    }
    pInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt64->setArrayDimensions(arrayDimensions);
    pInt64->setDataTypeId(OpcUaId_Int64);
    {
        UaInt64Array valInt64Array;
        defaultValue.setInt64Array(valInt64Array);
    }
    pInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysLocalizedText(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pLocalizedText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_LocalizedText, getTypeNamespace()));
    pLocalizedText->setBrowseName(UaQualifiedName("LocalizedText", getTypeNamespace()));
    pLocalizedText->setDisplayName(UaLocalizedText("", "LocalizedText"));
    pLocalizedText->setAccessLevel(3);
    pLocalizedText->setUserAccessLevel(3);
    pLocalizedText->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pLocalizedText->setArrayDimensions(arrayDimensions);
    pLocalizedText->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray valLocalizedTextArray;
        defaultValue.setLocalizedTextArray(valLocalizedTextArray);
    }
    pLocalizedText->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pLocalizedText, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pLocalizedText;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysQualifiedName(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQualifiedName = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_QualifiedName, getTypeNamespace()));
    pQualifiedName->setBrowseName(UaQualifiedName("QualifiedName", getTypeNamespace()));
    pQualifiedName->setDisplayName(UaLocalizedText("", "QualifiedName"));
    pQualifiedName->setAccessLevel(3);
    pQualifiedName->setUserAccessLevel(3);
    pQualifiedName->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pQualifiedName->setArrayDimensions(arrayDimensions);
    pQualifiedName->setDataTypeId(OpcUaId_QualifiedName);
    {
        UaQualifiedNameArray valQualifiedNameArray;
        defaultValue.setQualifiedNameArray(valQualifiedNameArray);
    }
    pQualifiedName->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQualifiedName, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQualifiedName;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pSByte->setArrayDimensions(arrayDimensions);
    pSByte->setDataTypeId(OpcUaId_SByte);
    {
        UaSByteArray valSByteArray;
        defaultValue.setSByteArray(valSByteArray);
    }
    pSByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pString = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_String, getTypeNamespace()));
    pString->setBrowseName(UaQualifiedName("String", getTypeNamespace()));
    pString->setDisplayName(UaLocalizedText("", "String"));
    pString->setAccessLevel(3);
    pString->setUserAccessLevel(3);
    pString->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pString->setArrayDimensions(arrayDimensions);
    pString->setDataTypeId(OpcUaId_String);
    {
        UaStringArray valStringArray;
        defaultValue.setStringArray(valStringArray);
    }
    pString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt16->setArrayDimensions(arrayDimensions);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        UaUInt16Array valUInt16Array;
        defaultValue.setUInt16Array(valUInt16Array);
    }
    pUInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt32->setArrayDimensions(arrayDimensions);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        UaUInt32Array valUInt32Array;
        defaultValue.setUInt32Array(valUInt32Array);
    }
    pUInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt64->setArrayDimensions(arrayDimensions);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
    {
        UaUInt64Array valUInt64Array;
        defaultValue.setUInt64Array(valUInt64Array);
    }
    pUInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysVariant(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pVariant = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_Variant, getTypeNamespace()));
    pVariant->setBrowseName(UaQualifiedName("Variant", getTypeNamespace()));
    pVariant->setDisplayName(UaLocalizedText("", "Variant"));
    pVariant->setAccessLevel(3);
    pVariant->setUserAccessLevel(3);
    pVariant->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pVariant->setArrayDimensions(arrayDimensions);
    pVariant->setDataTypeId(OpcUaId_BaseDataType);
    {
        UaVariantArray valVariantArray;
        defaultValue.setVariantArray(valVariantArray);
    }
    pVariant->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pVariant, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pVariant;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesArraysXmlElement(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pXmlElement = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays_XmlElement, getTypeNamespace()));
    pXmlElement->setBrowseName(UaQualifiedName("XmlElement", getTypeNamespace()));
    pXmlElement->setDisplayName(UaLocalizedText("", "XmlElement"));
    pXmlElement->setAccessLevel(3);
    pXmlElement->setUserAccessLevel(3);
    pXmlElement->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pXmlElement->setArrayDimensions(arrayDimensions);
    pXmlElement->setDataTypeId(OpcUaId_XmlElement);
    {
        UaXmlElementArray valXmlElementArray;
        defaultValue.setXmlElementArray(valXmlElementArray);
    }
    pXmlElement->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pXmlElement, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pXmlElement;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalar(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pScalar = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getTypeNamespace()));
    pScalar->setBrowseName(UaQualifiedName("Scalar", getTypeNamespace()));
    pScalar->setDisplayName(UaLocalizedText("", "Scalar"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pScalar, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pScalar;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarBool(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pBool = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Bool, getTypeNamespace()));
    pBool->setBrowseName(UaQualifiedName("Bool", getTypeNamespace()));
    pBool->setDisplayName(UaLocalizedText("", "Bool"));
    pBool->setAccessLevel(3);
    pBool->setUserAccessLevel(3);
    pBool->setValueRank(-1);
    pBool->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pBool->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pBool, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pBool;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(-1);
    pByte->setDataTypeId(OpcUaId_Byte);
    {
        defaultValue.setByte(0);
    }
    pByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarByteString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByteString = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ByteString, getTypeNamespace()));
    pByteString->setBrowseName(UaQualifiedName("ByteString", getTypeNamespace()));
    pByteString->setDisplayName(UaLocalizedText("", "ByteString"));
    pByteString->setAccessLevel(3);
    pByteString->setUserAccessLevel(3);
    pByteString->setValueRank(-1);
    pByteString->setDataTypeId(OpcUaId_ByteString);
    {
        UaByteString tmpByteString = UaByteString(0, (OpcUa_Byte*) "");
        defaultValue.setByteString(tmpByteString, OpcUa_False);
    }
    pByteString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pByteString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByteString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarDateTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDateTime = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_DateTime, getTypeNamespace()));
    pDateTime->setBrowseName(UaQualifiedName("DateTime", getTypeNamespace()));
    pDateTime->setDisplayName(UaLocalizedText("", "DateTime"));
    pDateTime->setAccessLevel(3);
    pDateTime->setUserAccessLevel(3);
    pDateTime->setValueRank(-1);
    pDateTime->setDataTypeId(OpcUaId_DateTime);
        defaultValue.setDateTime(UaDateTime());
    pDateTime->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDateTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDateTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(-1);
    pDouble->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pDouble->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarDuration(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDuration = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Duration, getTypeNamespace()));
    pDuration->setBrowseName(UaQualifiedName("Duration", getTypeNamespace()));
    pDuration->setDisplayName(UaLocalizedText("", "Duration"));
    pDuration->setAccessLevel(3);
    pDuration->setUserAccessLevel(3);
    pDuration->setValueRank(-1);
    pDuration->setDataTypeId(OpcUaId_Duration);
        defaultValue.setDouble(0.0);
    pDuration->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pDuration, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDuration;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarEnumeration(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pEnumeration = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Enumeration, getTypeNamespace()));
    pEnumeration->setBrowseName(UaQualifiedName("Enumeration", getTypeNamespace()));
    pEnumeration->setDisplayName(UaLocalizedText("", "Enumeration"));
    pEnumeration->setAccessLevel(3);
    pEnumeration->setUserAccessLevel(3);
    pEnumeration->setValueRank(-1);
    pEnumeration->setDataTypeId(UaNodeId(DemoId_HeaterStatus, getNameSpaceIndex()));
    {
        defaultValue.setInt32(Demo::HeaterStatus_Heating);
    }
    pEnumeration->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pEnumeration, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumeration;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(-1);
    pFloat->setDataTypeId(OpcUaId_Float);
    {
        defaultValue.setFloat(0);
    }
    pFloat->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarGuid(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pGuid = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Guid, getTypeNamespace()));
    pGuid->setBrowseName(UaQualifiedName("Guid", getTypeNamespace()));
    pGuid->setDisplayName(UaLocalizedText("", "Guid"));
    pGuid->setAccessLevel(3);
    pGuid->setUserAccessLevel(3);
    pGuid->setValueRank(-1);
    pGuid->setDataTypeId(OpcUaId_Guid);
        defaultValue.setGuid(UaGuid());
    pGuid->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pGuid, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pGuid;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarImage(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImage = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Image, getTypeNamespace()));
    pImage->setBrowseName(UaQualifiedName("Image", getTypeNamespace()));
    pImage->setDisplayName(UaLocalizedText("", "Image"));
    pImage->setAccessLevel(3);
    pImage->setUserAccessLevel(3);
    pImage->setValueRank(-1);
    pImage->setDataTypeId(OpcUaId_Image);
    {
        UaByteString tmpImage = UaByteString(10370, (OpcUa_Byte*) "\211\120\116\107\015\012\032\012\000\000\000\015\111\110\104\122\000\000\001\000\000\000\001\000\010\006\000\000\000\134\162\250"
        "\146\000\000\000\001\163\122\107\102\000\256\316\034\351\000\000\000\004\147\101\115\101\000\000\261\217\013\374\141\005\000\000"
        "\000\011\160\110\131\163\000\000\016\304\000\000\016\304\001\225\053\016\033\000\000\050\027\111\104\101\124\170\136\355\235\207"
        "\137\025\307\372\377\177\177\316\115\057\067\345\346\246\122\244\203\010\026\020\025\120\261\040\140\101\024\354\202\150\120\354"
        "\306\353\115\064\326\010\166\243\211\061\032\143\215\306\236\330\153\064\326\330\053\026\104\237\337\176\146\367\050\236\314\367"
        "\312\071\234\235\231\303\171\076\171\275\137\226\200\354\331\147\347\275\063\263\263\273\377\217\070\234\006\344\311\223\047\124"
        "\133\133\113\367\357\337\247\033\067\156\320\251\123\247\150\357\336\275\264\145\313\026\132\273\166\015\175\373\355\162\252\252"
        "\232\117\063\146\314\240\051\123\046\323\350\321\243\251\254\254\214\206\015\033\112\203\006\015\242\222\222\142\052\052\052\242"
        "\076\175\172\123\101\101\001\345\345\365\170\216\374\374\074\352\325\253\047\365\355\133\110\375\373\367\243\201\003\007\320\220"
        "\041\203\251\264\164\070\215\034\131\116\343\306\215\243\151\323\246\321\234\071\163\150\321\242\105\364\375\367\337\323\372\365"
        "\077\323\257\277\376\112\207\016\035\242\213\027\057\320\335\273\167\351\341\303\207\124\127\127\347\154\065\347\105\141\001\160"
        "\236\006\215\034\015\374\352\325\253\164\364\350\021\332\270\161\043\055\136\274\210\246\117\377\212\052\053\307\320\320\241\103"
        "\250\260\260\217\150\260\075\172\344\032\007\044\002\321\100\030\223\047\117\242\271\163\347\010\121\354\336\275\213\316\236\075"
        "\113\267\156\335\142\071\170\205\005\020\242\101\143\277\166\355\032\035\074\170\200\126\257\136\115\263\146\315\262\316\332\025"
        "\342\314\333\263\147\201\264\201\005\063\220\026\172\027\303\206\015\023\075\024\364\042\320\173\071\175\372\264\220\136\250\206"
        "\005\020\042\271\167\357\236\070\253\257\135\273\326\072\243\117\027\147\363\336\275\173\111\033\113\050\201\136\003\206\046\350"
        "\341\054\134\270\120\014\051\060\234\170\374\370\261\263\347\232\166\130\000\115\060\070\273\243\301\037\071\162\330\032\233\177"
        "\053\306\343\005\005\371\306\166\335\115\003\373\011\363\020\030\372\154\332\264\211\056\135\272\104\217\036\075\162\366\156\323"
        "\012\013\240\211\004\023\164\350\316\242\073\077\151\322\104\061\126\227\035\334\214\357\100\010\230\314\304\234\302\216\035\333"
        "\351\346\315\233\102\262\115\041\054\200\040\016\316\112\047\117\236\244\245\113\227\212\331\162\164\147\145\007\060\023\130\040"
        "\327\057\276\230\042\346\020\256\137\277\346\124\043\070\303\002\010\262\340\314\163\342\304\011\061\211\205\161\074\067\172\275"
        "\340\262\346\304\211\023\305\120\341\366\355\333\116\225\202\047\054\200\040\311\235\073\167\150\303\206\365\124\136\076\202\307"
        "\362\206\202\111\325\231\063\147\322\261\143\307\202\146\022\221\005\140\160\352\352\354\056\076\306\236\074\246\017\056\260\026"
        "\341\347\237\177\026\363\005\046\207\005\140\140\320\360\177\373\355\067\032\073\266\222\317\366\101\116\121\121\137\132\270\160"
        "\001\135\271\162\305\251\256\131\141\001\030\024\164\033\261\274\266\242\342\163\156\370\115\014\364\340\252\253\253\214\023\001"
        "\013\300\200\240\341\357\331\263\233\106\215\032\311\015\277\211\203\171\202\157\276\231\047\226\133\233\020\026\200\346\234\075"
        "\173\206\046\114\030\057\075\130\230\246\013\256\036\140\315\206\356\145\310\054\000\115\301\235\153\313\226\055\345\345\270\041"
        "\016\256\352\034\070\160\100\333\302\042\026\200\342\240\320\270\073\015\327\360\145\007\004\023\172\140\055\007\156\243\306\155"
        "\326\252\303\002\120\030\334\253\216\361\037\217\363\031\031\003\006\224\320\301\203\007\235\243\105\115\130\000\212\162\376\374"
        "\171\032\065\152\224\264\360\014\343\001\067\155\255\134\271\102\334\333\241\042\054\000\227\203\056\377\366\355\333\305\055\247"
        "\262\202\063\214\014\334\320\165\375\372\165\347\050\162\057\054\000\027\203\313\173\313\227\057\347\365\372\214\137\014\032\064"
        "\120\074\172\315\315\260\000\134\012\356\324\233\073\167\056\217\367\231\106\201\247\030\355\334\271\323\071\252\002\037\026\200"
        "\013\301\370\015\263\272\262\202\062\214\077\374\360\303\017\256\134\052\144\001\004\070\070\363\317\236\075\113\132\104\206\361"
        "\227\356\335\273\321\202\005\013\002\376\120\123\026\100\200\263\164\351\022\151\001\031\246\261\100\002\170\054\172\040\157\065"
        "\146\001\004\060\233\067\157\346\061\177\003\300\201\014\272\165\353\112\135\273\166\021\277\342\317\271\271\335\245\137\317\074"
        "\003\373\153\336\274\171\001\033\016\260\000\002\224\323\247\117\205\314\262\136\064\124\064\332\116\235\072\122\207\016\355\051"
        "\043\243\055\265\151\323\206\132\266\114\245\344\344\144\112\114\114\244\330\330\130\212\212\212\242\210\210\010\101\170\170\070"
        "\205\205\205\075\307\147\237\175\366\024\357\377\347\371\276\310\310\110\212\211\211\246\204\204\170\152\336\274\071\265\150\321"
        "\202\132\265\152\111\351\351\351\324\256\135\006\145\145\145\122\227\056\071\041\045\017\174\146\274\357\040\020\141\001\004\040"
        "\065\065\167\305\133\160\144\305\012\146\160\126\316\311\351\114\231\231\035\050\055\055\115\064\076\064\104\064\354\372\215\367"
        "\377\002\015\071\046\046\206\222\222\222\050\065\065\325\152\270\255\204\050\354\306\333\216\262\263\263\255\177\037\215\027\313"
        "\141\363\255\237\327\135\374\071\053\053\113\374\177\174\035\276\036\337\207\357\207\130\242\243\243\305\277\353\375\263\040\213"
        "\330\130\373\147\101\104\220\122\166\166\226\070\143\312\076\133\260\223\236\236\106\173\366\354\161\216\100\377\303\002\150\144"
        "\320\025\133\262\244\151\214\373\161\046\155\337\276\275\150\100\361\361\361\342\354\053\153\154\000\177\217\063\162\247\116\235"
        "\304\253\277\046\115\232\044\306\247\170\014\071\336\050\204\007\232\374\361\307\037\164\361\342\105\361\124\034\074\246\334\363"
        "\332\256\206\166\137\361\165\230\124\305\367\341\373\261\126\376\302\205\013\342\051\111\070\370\327\257\137\117\313\226\055\023"
        "\057\065\231\060\141\002\025\027\027\013\171\044\044\044\074\335\116\364\074\040\054\154\053\032\115\307\216\331\115\242\267\000"
        "\071\243\126\215\275\255\230\005\320\310\240\353\217\167\332\311\212\144\072\150\010\070\273\343\354\212\056\073\032\113\375\106"
        "\356\001\147\376\276\175\373\212\106\006\331\141\145\043\226\066\343\071\205\246\275\152\013\302\300\303\071\317\234\071\103\133"
        "\267\156\025\063\347\225\225\225\324\263\147\117\321\203\200\270\040\066\374\036\102\200\364\144\373\046\030\350\320\241\203\220"
        "\157\143\152\300\002\150\104\160\206\302\231\117\126\034\123\301\231\003\007\016\306\352\221\221\021\177\073\303\067\153\326\114"
        "\164\315\361\244\333\237\176\372\211\316\235\073\047\032\125\260\007\265\302\372\014\254\254\133\265\152\225\170\131\112\106\106"
        "\206\220\001\344\327\272\165\353\240\234\113\300\220\147\315\232\065\316\247\364\075\054\200\106\004\257\332\222\025\305\104\060"
        "\141\207\256\075\032\170\375\006\017\001\244\244\244\120\171\171\071\255\133\267\116\274\005\047\124\002\051\100\160\020\002\316"
        "\244\220\042\206\017\230\134\204\050\145\373\321\064\060\317\321\272\165\053\277\357\033\140\001\064\042\170\311\244\254\050\246"
        "\200\263\031\306\274\315\233\047\375\255\173\217\061\161\105\105\005\355\330\261\203\152\152\152\234\117\024\332\301\333\203\161"
        "\051\167\350\320\241\142\330\223\226\326\106\134\355\220\355\133\223\200\264\360\276\107\177\302\002\360\063\170\201\244\311\157"
        "\321\305\354\075\032\176\375\106\017\060\323\216\063\236\351\217\253\326\235\313\227\057\323\342\305\213\305\044\147\333\266\155"
        "\215\036\032\140\062\020\163\032\376\364\002\130\000\176\006\167\371\311\212\241\033\034\250\270\154\126\377\214\217\337\027\026"
        "\026\322\256\135\273\214\233\264\063\075\230\377\330\260\141\203\230\350\105\157\112\266\317\165\203\232\143\150\207\253\060\276"
        "\206\005\340\107\320\210\312\312\112\245\305\320\011\316\126\161\161\161\117\033\076\306\367\235\073\167\026\263\366\234\306\005"
        "\313\157\061\051\212\305\136\046\366\006\060\144\301\232\011\137\037\044\302\002\360\043\147\317\236\025\117\156\221\025\102\027"
        "\231\231\231\142\106\333\323\370\041\202\352\352\152\172\360\340\201\263\325\234\100\004\017\163\305\143\335\114\173\306\003\056"
        "\347\102\370\277\376\372\253\263\245\015\013\013\300\217\140\241\213\254\010\272\300\162\334\210\210\147\135\176\134\306\073\162"
        "\344\210\263\265\234\100\007\127\017\160\217\076\036\355\055\253\207\016\060\131\011\001\214\031\063\306\331\312\206\205\005\340"
        "\107\146\315\232\051\055\202\016\160\171\257\376\170\037\013\136\060\233\315\161\077\273\167\357\066\252\047\210\145\322\030\006"
        "\140\345\144\103\303\002\360\061\030\013\342\305\217\262\002\250\006\353\334\353\257\313\307\122\130\054\231\345\250\313\212\025"
        "\053\244\265\321\101\162\162\163\061\014\304\022\354\206\206\005\340\143\320\300\112\112\212\245\005\120\015\026\366\170\032\077"
        "\046\000\371\322\236\372\340\315\076\246\274\343\001\013\202\160\054\340\351\101\015\015\013\300\307\240\221\231\060\001\204\263"
        "\277\247\353\217\245\254\276\130\237\023\330\154\334\270\101\132\043\325\340\016\110\034\017\123\247\116\165\266\354\305\141\001"
        "\370\030\334\221\046\333\371\252\301\155\263\236\263\377\177\376\363\037\147\353\070\072\202\067\072\313\152\244\032\054\141\306"
        "\361\200\145\315\015\015\013\300\307\230\050\000\236\364\323\033\323\004\320\265\153\127\147\313\136\034\026\200\217\201\000\360"
        "\330\057\335\340\316\065\217\000\232\302\335\172\301\034\010\100\126\043\325\340\041\052\070\036\260\370\253\241\141\001\370\030"
        "\026\000\307\073\277\375\146\206\000\332\267\147\001\270\036\026\000\307\073\054\200\020\012\013\200\343\035\026\100\010\305\024"
        "\001\340\062\040\013\300\214\260\000\102\050\067\157\336\020\353\000\164\303\002\060\047\170\000\252\254\106\252\301\163\001\130"
        "\000\056\207\005\300\361\016\013\040\204\302\002\340\170\207\005\020\102\141\001\160\274\303\002\010\241\340\136\000\334\002\252"
        "\033\334\377\315\002\060\043\020\200\254\106\252\301\163\041\130\000\056\207\005\300\361\016\013\040\204\302\002\340\170\207\005"
        "\020\102\141\001\160\274\303\002\010\241\100\000\170\037\300\377\102\126\234\100\203\067\327\260\000\314\310\357\277\377\046\075"
        "\016\352\043\253\141\240\301\203\101\131\000\056\247\041\002\120\001\013\300\234\064\104\000\052\140\001\050\010\013\200\343\035"
        "\026\100\010\205\005\300\361\016\013\040\204\302\002\340\170\207\005\020\102\201\000\360\236\070\335\340\045\237\054\000\063\362"
        "\373\357\277\113\153\244\232\254\254\114\026\200\333\141\001\160\274\303\002\010\241\260\000\070\336\141\001\204\120\130\000\034"
        "\357\260\000\102\050\020\000\136\021\255\033\274\242\232\005\140\106\040\000\131\215\124\303\002\120\020\026\000\307\073\346\010"
        "\040\313\034\001\074\261\376\253\255\173\100\065\265\167\350\162\315\005\072\165\343\010\235\270\176\200\056\334\071\115\267\037"
        "\336\240\007\165\367\350\361\223\307\316\127\007\117\130\000\034\357\260\000\234\074\172\134\113\047\257\037\244\065\177\054\242"
        "\351\173\312\251\164\123\016\365\137\227\116\375\176\112\373\033\203\326\147\322\244\035\045\264\354\310\014\332\373\327\057\164"
        "\267\366\266\363\257\230\035\026\000\307\073\041\057\200\333\017\157\322\306\077\127\322\270\137\213\254\006\217\006\336\306\147"
        "\040\213\245\107\246\323\331\133\047\105\357\301\324\100\000\175\372\364\326\016\013\300\234\354\333\367\273\264\106\252\311\316"
        "\126\054\000\164\341\267\237\137\107\145\233\272\112\033\265\077\014\370\271\035\055\072\064\315\330\036\201\051\002\300\273\340"
        "\130\000\146\044\044\005\160\343\376\025\372\317\356\141\126\243\115\243\042\253\341\006\232\122\113\052\007\056\357\064\256\067"
        "\300\002\340\170\047\344\004\160\361\316\237\124\261\265\247\264\341\006\222\222\237\063\150\333\271\265\106\111\200\005\300\361"
        "\116\110\011\340\352\275\277\104\227\277\150\155\153\045\364\377\051\235\266\235\135\353\374\164\375\261\005\320\107\073\054\000"
        "\163\262\157\337\076\151\215\124\223\235\235\355\256\000\060\313\077\165\327\020\151\103\165\223\222\265\031\264\377\364\036\147"
        "\053\364\006\002\050\054\354\243\035\274\012\212\005\140\106\040\000\131\215\124\323\261\243\313\002\130\373\307\022\352\153\065"
        "\110\035\364\131\224\116\277\355\333\353\154\211\276\260\000\070\336\011\011\001\334\255\275\105\303\067\166\266\032\143\053\075"
        "\254\151\105\271\245\131\164\365\352\125\147\213\364\204\005\300\361\116\110\010\140\317\305\315\362\206\251\220\316\123\223\250"
        "\254\254\214\036\077\326\267\202\220\005\300\361\116\110\010\140\301\301\251\124\150\065\102\235\024\054\113\241\350\270\110\072"
        "\176\374\270\263\125\352\003\001\364\355\133\250\235\274\274\074\026\200\041\201\000\144\065\122\115\307\216\035\335\023\300\027"
        "\273\006\133\215\260\245\126\372\254\116\245\350\026\021\064\155\332\064\147\253\324\307\024\001\340\135\160\054\000\063\022\022"
        "\002\030\367\153\077\151\243\124\312\232\226\024\223\032\101\135\273\166\245\047\117\364\254\015\140\001\160\274\023\032\002\330"
        "\126\044\032\240\156\142\132\106\120\174\174\274\266\171\000\026\000\307\073\041\041\200\261\333\372\122\237\065\251\332\201\000"
        "\302\302\302\064\013\240\257\166\130\000\346\304\026\200\274\116\052\351\324\051\104\004\200\017\251\123\000\105\105\175\265\223"
        "\237\237\317\002\060\044\373\367\357\223\326\110\065\006\011\040\305\065\130\000\066\054\000\163\022\042\002\050\224\066\110\325"
        "\260\000\154\130\000\346\044\044\004\120\151\011\240\267\325\000\165\143\206\000\212\264\303\002\060\047\373\367\357\227\326\110"
        "\065\235\072\165\012\166\001\264\170\041\046\010\240\137\277\042\355\340\165\320\054\000\063\002\001\310\152\244\032\167\005\260"
        "\265\017\365\376\321\152\204\232\301\072\000\026\000\013\300\244\260\000\024\321\313\202\005\140\303\002\060\047\041\041\200\061"
        "\226\000\320\000\165\303\002\260\051\050\050\140\001\030\222\020\022\100\262\166\164\013\340\326\255\233\324\277\177\077\355\340"
        "\165\320\052\005\000\361\155\335\372\013\125\125\315\247\311\223\047\123\171\371\010\052\053\053\245\322\322\341\332\301\166\214"
        "\036\135\101\063\146\114\247\345\313\227\321\261\143\307\250\256\256\316\331\162\367\003\001\310\152\244\032\026\200\202\204\222"
        "\000\160\277\305\371\363\347\351\313\057\277\024\317\235\307\260\043\030\300\276\031\064\150\040\255\133\267\216\036\076\174\350"
        "\174\032\367\142\212\000\072\167\166\131\000\075\255\006\250\233\150\026\200\300\155\001\324\326\326\212\263\051\036\066\211\237"
        "\025\254\240\167\160\362\344\011\347\123\271\223\020\021\200\165\040\374\330\134\073\054\000\033\034\334\156\011\240\246\246\206"
        "\376\373\337\151\177\173\363\114\260\122\134\334\237\266\157\377\325\265\073\110\131\000\012\321\057\200\133\326\316\356\257\235"
        "\236\075\173\272\042\000\234\371\321\370\275\037\071\035\354\140\222\354\367\337\177\163\076\145\140\143\013\100\136\047\225\270"
        "\057\200\325\126\043\324\214\011\002\300\031\105\067\170\037\174\240\005\200\063\344\262\145\313\110\166\253\151\123\140\340\300"
        "\001\164\354\330\121\347\323\006\056\007\016\354\227\326\110\065\150\370\256\011\140\364\326\136\124\260\072\111\073\054\000\033"
        "\067\004\160\346\314\237\124\122\122\114\262\165\346\115\005\364\004\116\237\076\355\174\342\300\204\005\240\020\026\200\115\240"
        "\005\200\375\371\365\327\137\077\167\135\271\051\202\261\162\357\336\275\105\035\003\225\020\021\100\117\253\001\046\152\207\005"
        "\140\023\150\001\134\277\176\135\234\375\145\077\253\251\321\245\113\016\115\237\076\335\371\344\215\017\013\100\041\046\010\000"
        "\015\105\067\230\335\016\244\000\266\156\335\052\375\071\115\021\014\005\222\223\223\351\257\277\376\162\076\175\343\162\340\300"
        "\001\351\317\121\115\116\216\213\002\300\313\100\363\255\006\250\033\026\200\115\240\005\120\125\125\105\003\006\224\204\004\230"
        "\014\114\114\114\244\352\352\152\347\323\067\056\054\000\205\104\247\206\263\000\054\002\055\000\054\243\105\303\010\025\122\122"
        "\132\210\113\251\201\070\216\102\103\000\277\024\120\376\017\011\332\141\001\330\004\132\000\143\307\216\245\301\203\007\205\014"
        "\031\031\031\024\027\027\107\167\357\336\165\366\200\377\011\011\001\174\156\011\040\317\152\200\272\061\103\000\045\332\301\114"
        "\166\040\005\060\165\352\124\032\062\144\160\310\320\272\165\153\261\357\056\137\276\354\354\001\377\143\013\100\136\047\225\344"
        "\344\344\260\000\334\016\004\040\033\127\252\006\253\333\002\051\200\352\352\052\032\072\164\110\310\320\274\171\163\261\357\216"
        "\036\155\374\302\040\010\100\126\043\325\340\352\206\313\002\210\327\016\013\300\046\320\002\130\263\146\015\015\037\076\054\044"
        "\200\000\142\143\143\305\276\073\163\346\214\263\007\374\017\013\100\041\054\000\233\100\013\000\253\000\145\367\332\067\105\260"
        "\054\030\373\055\062\062\122\324\263\261\011\011\001\214\262\004\320\303\152\200\272\061\103\000\003\376\007\362\342\004\232\100"
        "\013\000\377\306\344\311\223\304\353\327\233\072\231\231\035\304\176\303\104\140\040\236\027\140\013\100\166\054\170\220\327\060"
        "\320\270\054\200\174\253\001\306\151\307\004\001\310\056\053\251\246\117\237\076\001\025\000\262\147\317\036\361\244\237\246\014"
        "\046\000\043\042\354\265\044\023\046\114\160\076\171\343\162\360\340\001\151\215\124\323\245\113\027\026\200\333\061\105\000\205"
        "\205\201\027\000\156\005\236\061\143\006\215\034\131\336\044\051\057\057\247\166\355\062\304\076\013\017\017\027\302\013\104\102"
        "\103\000\133\362\051\167\125\234\166\130\000\066\156\010\000\301\145\261\111\223\046\322\347\237\217\012\012\106\215\152\050\043"
        "\051\057\057\117\274\130\026\373\014\057\323\014\324\163\003\103\104\000\171\126\003\214\325\016\013\300\306\055\001\040\047\117"
        "\236\244\261\143\053\251\242\342\363\046\003\326\377\143\322\017\373\253\131\263\146\126\243\075\350\174\332\306\207\005\240\020"
        "\063\004\060\120\073\205\205\366\114\266\033\002\100\216\037\077\156\065\234\012\361\264\335\140\006\215\037\147\176\317\270\037"
        "\075\200\305\213\027\073\237\062\060\201\114\144\065\122\115\327\256\054\000\327\003\001\340\211\263\272\361\134\312\162\113\000"
        "\310\246\115\233\104\117\003\335\347\061\143\106\007\035\230\360\113\113\113\173\272\237\060\356\307\043\315\003\035\010\100\126"
        "\043\325\270\052\200\221\133\172\120\367\125\061\332\141\001\330\250\020\000\262\153\327\056\312\314\314\024\007\025\176\056\316"
        "\250\225\225\143\304\020\301\044\052\053\053\105\243\037\061\142\204\270\104\232\236\236\376\364\254\017\242\243\243\305\231\337"
        "\215\343\206\005\240\020\026\200\215\052\001\040\230\030\034\067\156\034\105\105\105\211\206\204\125\164\270\237\076\065\065\205"
        "\132\266\114\325\016\266\043\076\076\236\142\142\142\236\216\363\075\240\313\337\257\137\077\072\174\370\260\153\117\005\146\001"
        "\050\044\072\325\236\305\325\053\200\101\332\121\051\000\117\260\154\166\316\234\071\326\201\326\125\310\000\215\015\335\152\200"
        "\206\246\033\154\007\316\372\230\344\153\333\266\255\350\356\343\211\275\156\037\053\266\000\344\165\122\011\352\342\242\000\162"
        "\255\006\030\355\016\337\067\034\023\004\040\273\275\124\065\230\325\126\055\200\372\171\360\340\001\135\274\170\121\334\114\203"
        "\263\353\241\103\207\264\203\355\070\173\366\154\100\156\361\365\045\020\200\254\106\252\351\326\315\105\001\224\157\316\245\156"
        "\126\003\324\015\013\300\106\267\000\070\317\302\002\120\010\013\300\206\005\140\116\102\110\000\121\332\141\001\330\260\000\314"
        "\011\013\100\041\046\010\240\376\223\145\164\121\124\124\304\002\060\044\207\016\035\224\326\110\065\335\272\165\163\117\000\043"
        "\054\001\164\265\032\240\156\130\000\066\054\000\163\142\212\000\272\167\167\125\000\335\255\006\330\114\073\054\000\033\026\200"
        "\071\141\001\050\204\005\140\303\002\060\047\241\041\200\115\226\000\276\263\032\241\146\242\123\364\013\300\373\001\223\072\300"
        "\173\356\130\000\146\004\153\020\144\065\122\215\253\002\050\333\324\215\272\174\027\251\035\026\200\015\013\300\234\230\043\200"
        "\356\054\000\267\303\002\340\170\207\005\240\020\026\200\015\013\300\234\204\210\000\272\122\316\167\021\332\211\322\054\200\333"
        "\267\157\321\260\141\103\005\262\042\250\002\357\271\147\001\230\021\010\300\204\143\042\067\227\005\340\172\352\013\100\047\046"
        "\011\240\366\361\103\072\167\353\017\072\170\145\027\035\274\354\056\207\256\354\246\363\267\117\323\243\307\265\316\117\327\237"
        "\372\002\320\111\210\010\300\076\350\131\000\146\010\340\310\325\275\064\174\143\216\250\115\347\357\302\225\220\143\015\005\313"
        "\267\344\322\251\033\107\234\255\320\233\220\020\100\351\246\056\324\171\245\125\000\315\260\000\154\114\020\300\351\233\107\051"
        "\177\165\202\325\040\321\050\325\323\353\307\144\272\164\367\234\263\065\372\302\002\120\210\176\001\334\266\166\366\060\355\364"
        "\357\337\137\253\000\236\130\377\115\334\136\054\255\221\112\206\176\333\215\056\135\272\344\154\225\236\330\002\220\327\111\045"
        "\271\271\271\054\000\267\003\001\310\136\070\251\232\342\142\275\002\250\173\374\110\274\253\121\126\043\225\144\315\377\214\262"
        "\262\262\304\325\031\135\201\000\144\065\122\115\217\036\256\013\040\114\073\054\000\033\335\002\170\144\011\100\126\037\325\164"
        "\134\142\117\012\317\237\077\337\331\062\365\011\011\001\014\267\004\320\311\332\341\272\141\001\330\004\223\000\144\165\014\024"
        "\331\113\354\175\200\367\044\350\012\013\100\041\054\000\233\342\342\142\355\002\220\325\107\065\036\001\340\201\230\272\022\042"
        "\002\310\261\166\370\147\332\061\101\000\262\167\316\253\246\244\044\224\005\360\354\170\360\010\000\257\305\322\225\303\207\017"
        "\111\153\244\232\036\075\172\260\000\334\016\013\300\216\055\000\171\215\124\222\275\344\123\026\200\203\253\002\030\266\061\207"
        "\072\256\370\114\073\054\000\033\023\004\040\253\217\152\262\026\263\000\074\270\054\200\316\326\016\377\124\073\054\000\033\026"
        "\200\015\013\340\031\054\000\005\261\005\120\252\235\222\222\022\315\002\250\225\326\107\065\131\213\077\061\100\000\207\245\065"
        "\122\115\136\036\013\300\365\100\000\145\145\245\332\031\060\200\005\000\114\021\200\254\106\252\161\125\000\103\067\166\242\354"
        "\025\237\150\207\005\140\143\202\000\144\365\121\115\046\013\340\051\054\000\005\141\001\330\061\107\000\037\263\000\034\102\104"
        "\000\366\244\217\136\001\224\005\020\171\061\137\004\013\300\306\034\001\310\152\353\017\362\172\067\204\274\274\074\067\005\320"
        "\321\332\341\037\153\247\231\001\002\030\061\242\114\073\003\007\016\060\100\000\362\032\251\304\024\001\310\152\244\232\374\174"
        "\067\005\260\241\043\145\175\373\261\166\130\000\066\046\010\100\126\037\325\164\130\364\021\013\300\201\005\240\040\054\000\073"
        "\054\200\147\011\021\001\144\133\073\374\043\355\260\000\154\314\020\200\274\106\052\141\001\074\303\125\001\014\261\004\220\151"
        "\355\160\335\230\041\200\021\332\031\070\160\040\013\300\302\034\001\310\353\244\222\374\374\174\026\200\333\201\000\312\313\107"
        "\150\147\320\040\375\002\220\325\107\065\355\015\020\300\221\043\207\245\065\122\215\002\001\174\250\035\026\200\215\031\002\220"
        "\327\110\045\355\027\175\310\002\160\140\001\050\010\013\300\016\013\340\131\102\102\000\203\327\147\123\207\345\037\152\307\014"
        "\001\224\053\100\136\144\017\046\010\100\126\037\325\264\133\150\202\000\216\130\065\221\325\060\220\310\217\203\372\270\054\200"
        "\054\153\207\377\133\073\046\010\140\344\310\162\355\014\032\064\310\000\001\310\153\244\222\166\013\377\155\204\000\144\065\122"
        "\015\013\100\101\114\021\300\340\301\054\000\300\002\170\206\253\002\030\144\011\240\275\265\303\165\303\002\260\141\001\330\260"
        "\000\236\341\262\000\062\255\006\370\201\166\130\000\066\046\010\100\126\037\325\144\054\374\200\005\340\300\002\120\020\010\140"
        "\324\250\221\317\061\162\244\172\006\017\036\314\002\260\060\105\000\046\034\023\005\005\054\000\327\043\023\200\016\130\000\066"
        "\246\012\100\007\256\012\140\340\372\016\324\156\371\277\264\303\002\260\061\101\000\262\372\250\046\143\341\277\130\000\016\356"
        "\012\340\147\113\000\313\254\235\256\031\026\200\215\021\002\220\324\107\065\155\027\260\000\074\260\000\024\304\026\300\050\355"
        "\014\031\302\002\000\346\010\100\136\047\225\024\024\024\270\051\200\366\326\016\177\137\073\046\010\340\363\317\107\151\307\014"
        "\001\310\153\244\222\266\013\336\327\056\200\243\107\217\110\153\244\232\236\075\135\026\100\206\265\303\165\023\311\002\020\230"
        "\040\000\131\175\124\223\316\002\170\012\013\100\101\130\000\166\130\000\317\302\002\360\211\367\032\005\013\300\146\310\220\041"
        "\054\000\213\364\005\357\261\000\034\024\010\100\336\050\003\101\333\006\242\133\000\167\356\334\246\212\212\317\265\063\164\250"
        "\011\002\220\327\122\045\146\010\340\250\264\106\252\351\331\263\247\173\002\030\140\011\240\355\122\253\021\152\206\005\140\143"
        "\202\000\144\365\121\115\132\065\013\300\103\023\027\300\273\002\026\200\015\013\300\046\255\372\135\026\200\203\313\002\150\147"
        "\355\160\273\021\352\204\005\140\143\206\000\344\065\122\211\011\002\070\166\354\230\264\106\252\161\165\016\240\304\022\100\272"
        "\265\303\165\243\133\000\167\357\336\245\261\143\307\322\350\321\025\132\061\341\251\300\262\372\250\306\004\001\234\072\165\112"
        "\132\043\325\270\372\156\300\222\237\063\254\035\376\216\166\164\013\240\266\266\226\276\374\362\277\322\002\250\244\127\257\136"
        "\142\077\204\207\207\153\024\200\274\106\052\151\123\375\216\166\001\334\274\171\123\132\043\325\144\145\145\371\274\057\202\120"
        "\000\366\353\240\165\011\000\131\271\162\205\264\000\052\151\337\276\235\330\017\211\211\211\124\127\127\347\154\231\272\260\000"
        "\236\345\311\223\047\364\325\127\137\112\353\244\022\034\013\330\027\205\205\205\316\226\275\070\054\000\077\162\362\344\111\032"
        "\063\146\264\264\010\052\300\003\040\243\243\243\304\176\300\033\141\161\000\252\016\013\340\371\374\362\313\057\322\132\251\002"
        "\103\302\260\260\060\261\057\046\117\236\354\154\325\213\323\140\001\024\377\334\226\322\226\376\123\073\046\010\000\077\273\252"
        "\252\312\332\361\220\200\172\160\260\143\037\200\231\063\147\072\133\245\066\020\200\254\076\252\151\123\375\117\043\004\120\123"
        "\123\103\123\247\176\041\255\227\333\140\002\060\055\055\115\354\007\110\140\333\266\155\316\126\275\070\015\027\300\072\113\000"
        "\113\254\235\256\031\023\004\200\134\273\166\315\062\355\044\321\023\120\011\336\011\030\021\021\041\366\101\164\164\064\235\071"
        "\163\306\331\042\265\021\002\220\324\107\065\255\253\314\020\000\162\374\370\061\032\073\266\122\132\067\067\301\073\001\075\147"
        "\377\214\214\014\272\167\357\236\263\105\057\016\013\240\021\301\013\041\124\026\034\135\377\204\204\170\361\371\301\304\211\023"
        "\265\164\377\021\026\200\074\033\067\156\220\326\316\055\360\176\210\310\310\110\261\017\040\201\325\253\127\073\133\322\260\370"
        "\040\200\164\153\207\277\255\035\223\004\200\340\305\220\023\047\116\220\026\047\220\014\037\076\354\351\044\017\310\311\311\021"
        "\227\044\165\305\177\001\310\353\352\057\255\253\336\066\112\000\020\362\346\315\233\251\262\162\214\264\216\201\244\270\270\277"
        "\350\005\172\216\011\274\040\324\327\023\102\203\005\320\337\022\100\033\153\207\353\306\064\001\040\177\374\361\207\230\170\221"
        "\025\051\020\224\224\024\123\134\134\334\323\102\247\244\244\320\211\023\047\234\237\256\047\020\200\254\076\252\151\145\230\000"
        "\074\331\271\163\047\215\033\067\126\132\317\306\122\121\121\041\126\375\171\316\374\000\047\004\334\250\346\153\130\000\001\312"
        "\137\177\375\345\114\312\124\010\373\007\002\164\371\063\063\073\210\153\375\236\102\247\247\247\013\341\350\016\013\340\305\071"
        "\164\350\020\225\225\225\211\106\053\253\257\077\140\005\150\253\126\255\236\216\371\101\357\336\275\351\306\215\033\316\117\365"
        "\055\076\012\340\055\355\230\052\000\004\105\300\214\154\217\036\271\204\347\264\313\012\370\042\160\260\014\033\066\224\072\166"
        "\314\246\250\050\373\122\037\100\301\213\212\212\350\322\245\113\316\117\323\033\026\100\303\202\113\306\045\045\045\324\253\127"
        "\117\161\313\256\254\346\057\002\307\304\200\001\045\142\202\317\063\001\014\360\373\361\343\307\373\064\351\347\235\240\023\100"
        "\204\301\002\100\060\006\303\065\341\354\354\154\161\151\046\067\267\273\270\106\013\041\100\016\365\173\010\350\312\341\240\300"
        "\231\276\177\377\176\242\033\207\356\175\175\273\003\234\365\177\372\351\047\243\076\263\055\000\171\215\124\322\252\352\055\261"
        "\217\114\025\000\202\205\132\213\027\057\246\066\155\332\120\273\166\031\342\341\235\070\223\343\101\236\070\046\074\075\004\374"
        "\212\343\003\177\137\126\126\112\205\205\175\254\023\101\107\112\112\112\172\356\170\100\217\260\153\327\256\264\177\377\176\347"
        "\047\370\237\006\013\240\337\117\351\324\172\361\133\332\061\135\000\236\334\277\177\237\066\154\330\100\175\373\366\025\147\162"
        "\214\327\142\143\143\305\104\136\213\026\055\054\222\051\041\041\101\374\135\175\253\173\300\327\143\234\267\146\315\032\161\215"
        "\331\264\100\000\255\255\006\250\233\226\363\315\027\200\047\350\041\056\137\276\134\064\136\064\142\034\027\230\333\361\034\023"
        "\150\350\370\163\114\114\314\163\303\076\017\070\126\206\016\035\112\333\267\157\027\113\322\003\221\000\011\340\115\145\004\213"
        "\000\074\101\217\340\374\371\363\264\166\355\132\232\060\141\002\165\353\326\115\234\011\320\370\121\144\234\355\343\343\343\305"
        "\337\341\040\256\254\254\024\227\162\160\175\337\344\317\050\004\040\075\026\324\022\114\002\360\004\367\156\140\150\260\142\305"
        "\012\361\106\237\116\235\072\211\161\075\032\070\216\007\234\020\040\005\364\374\260\322\363\213\057\276\240\215\033\067\322\345"
        "\313\227\003\176\331\327\007\001\244\131\073\134\336\050\125\022\154\002\360\016\266\373\341\303\207\142\334\206\131\333\133\267"
        "\156\211\337\343\357\164\254\351\367\067\266\000\344\065\122\111\313\371\157\006\235\000\274\203\272\077\170\360\100\364\364\160"
        "\074\334\271\163\347\351\061\341\366\161\356\223\000\132\131\073\134\067\301\056\200\246\022\010\100\126\037\325\244\066\001\001"
        "\350\214\017\002\150\143\355\360\067\264\023\221\362\061\013\300\200\330\002\220\327\110\045\251\363\337\140\001\064\042\054\000"
        "\216\137\141\001\064\215\030\055\200\226\022\130\000\146\204\005\320\064\322\140\001\024\375\324\332\152\200\257\153\207\005\140"
        "\106\040\000\131\175\124\223\072\377\165\026\100\043\322\160\001\254\265\004\260\310\332\351\232\141\001\230\021\041\000\111\175"
        "\124\223\372\015\013\240\061\141\001\160\374\012\013\240\151\244\301\002\350\273\266\025\245\056\172\115\073\054\000\063\002\001"
        "\310\352\243\232\024\026\100\243\302\002\340\370\025\163\004\360\032\013\240\021\141\001\160\374\012\013\240\151\304\107\001\274"
        "\252\035\026\200\031\261\005\040\257\221\112\130\000\215\113\203\005\120\150\011\040\005\073\134\063\054\000\063\002\001\310\352"
        "\243\232\026\054\200\106\245\341\002\130\143\011\140\241\265\323\065\023\321\202\005\140\102\204\000\044\365\121\115\213\171\054"
        "\200\306\204\005\300\361\053\054\200\246\021\037\004\320\322\332\341\257\150\207\005\140\106\154\001\310\153\244\222\344\171\257"
        "\262\000\032\221\006\013\240\217\045\200\026\326\016\327\015\013\300\214\100\000\262\372\250\206\005\320\270\370\040\200\124\153"
        "\207\277\254\235\210\026\037\261\000\014\210\055\000\171\215\124\222\074\357\025\026\100\043\302\002\340\370\025\026\100\323\010"
        "\013\200\343\127\114\021\100\163\026\100\243\342\203\000\122\050\171\341\113\332\141\001\230\021\010\100\126\037\325\064\237\367"
        "\062\013\240\021\361\115\000\013\254\235\256\031\026\200\031\021\002\220\324\107\065\315\347\262\000\032\023\026\000\307\257\260"
        "\000\232\106\130\000\034\277\142\212\000\222\130\000\215\112\203\005\320\173\115\013\152\276\340\037\332\141\001\230\021\010\100"
        "\126\037\325\044\315\175\211\005\320\210\064\130\000\170\046\240\254\000\252\211\310\370\067\013\300\200\134\256\071\057\255\217"
        "\152\222\146\163\017\240\061\151\260\000\106\157\355\055\055\200\152\342\307\277\116\141\021\054\000\335\031\377\153\077\151\175"
        "\124\223\060\205\357\005\150\114\032\054\200\271\373\306\111\013\240\203\250\156\357\213\367\244\161\364\344\367\113\333\010\017"
        "\343\220\325\106\065\161\043\355\067\003\365\350\321\303\331\072\216\057\151\260\000\066\237\131\045\055\200\016\222\346\275\114"
        "\343\146\217\164\266\214\243\062\027\356\374\111\035\127\174\046\255\213\016\242\372\274\043\004\120\132\132\352\154\041\307\227"
        "\064\130\000\347\157\237\022\057\142\220\025\101\007\051\163\336\244\337\116\357\160\266\216\243\042\127\357\135\064\146\062\330"
        "\103\244\063\047\064\175\372\164\147\053\071\276\244\301\002\250\173\374\210\162\127\305\112\213\240\213\266\213\337\243\003\227"
        "\166\072\133\310\161\063\070\001\164\373\076\112\132\007\135\240\047\030\326\354\123\041\200\365\353\327\073\133\312\361\045\015"
        "\026\000\362\365\336\317\245\205\320\111\233\045\157\323\167\307\346\012\101\161\002\237\047\117\036\323\226\063\077\120\373\345"
        "\377\226\356\177\235\304\175\156\217\377\243\242\242\350\334\271\163\316\026\163\174\211\117\002\070\164\145\027\045\343\006\014"
        "\111\061\164\202\005\041\043\267\364\240\363\167\116\073\133\312\011\104\256\335\273\104\123\166\016\022\017\336\220\355\167\335"
        "\104\165\171\137\010\240\103\207\016\364\350\021\237\000\374\211\117\002\170\374\244\216\362\127\047\112\213\141\002\351\113\337"
        "\245\157\366\117\244\232\332\333\316\026\163\374\111\355\343\207\264\342\350\154\312\374\366\103\351\176\066\201\304\231\126\367"
        "\077\332\356\376\117\235\072\325\331\162\216\257\361\111\000\310\367\307\277\221\026\304\044\160\340\126\037\374\202\156\076\270"
        "\352\154\065\247\041\201\070\127\036\233\103\135\276\153\046\335\257\046\021\063\340\155\321\370\303\303\303\151\337\276\175\316"
        "\047\340\370\032\237\005\120\123\173\207\072\256\370\124\132\024\323\150\153\365\010\246\356\032\112\107\256\356\025\113\127\071"
        "\177\117\235\325\253\073\165\343\010\175\375\133\005\145\175\373\221\164\077\032\307\334\227\050\274\271\275\044\274\123\247\116"
        "\274\050\254\021\361\131\000\310\212\143\263\345\205\061\024\314\133\364\372\061\231\026\035\372\257\165\260\037\026\007\175\050"
        "\007\023\173\347\156\377\041\352\130\274\056\103\074\137\137\266\337\114\045\146\220\175\366\007\313\227\057\167\076\025\307\237"
        "\370\045\200\173\217\356\032\075\027\360\277\300\012\266\374\325\011\342\214\267\373\302\106\272\363\360\246\363\251\232\166\356"
        "\077\252\021\053\370\060\107\122\270\266\245\121\153\072\174\041\151\326\313\024\036\157\077\030\066\065\065\225\152\152\152\234"
        "\117\310\361\047\176\011\000\331\173\161\113\320\235\071\274\301\325\203\066\113\336\242\376\353\322\205\020\160\271\013\067\271"
        "\340\222\342\023\353\277\140\014\266\033\075\234\353\367\057\323\366\363\353\150\336\276\011\064\170\103\026\245\057\175\307\310"
        "\053\070\276\322\254\373\173\117\317\376\213\027\057\166\076\065\307\337\370\055\000\034\150\323\166\017\227\026\051\230\201\024"
        "\262\276\375\230\112\176\316\240\311\073\006\322\322\303\323\151\333\271\265\364\347\315\343\164\373\341\015\143\346\022\040\251"
        "\073\017\157\211\005\072\073\317\257\247\345\107\147\212\371\216\301\353\263\250\363\312\160\361\071\144\237\057\230\211\037\363"
        "\006\205\205\331\215\077\073\073\233\356\335\273\347\354\015\216\277\361\133\000\010\206\002\246\055\015\165\023\234\105\173\254"
        "\212\243\101\353\063\305\335\221\020\140\325\201\311\342\312\010\356\225\100\027\033\023\152\130\057\177\351\356\071\272\122\163"
        "\121\234\211\161\065\002\362\300\054\073\272\342\017\353\356\133\277\336\023\177\306\020\004\377\037\137\207\245\266\370\276\213"
        "\326\367\377\171\363\030\035\270\274\203\176\071\273\232\176\070\061\237\026\034\234\112\137\356\031\101\225\333\012\151\310\206"
        "\154\061\214\311\130\366\176\223\154\350\062\022\147\130\135\377\204\117\104\343\217\214\214\244\235\073\171\005\150\040\322\050"
        "\001\040\147\156\235\240\166\313\376\045\055\132\050\202\006\331\162\321\353\324\172\361\233\142\225\042\326\046\144\054\173\217"
        "\332\057\377\200\072\054\377\120\364\056\160\025\005\277\342\162\045\376\036\015\031\137\227\266\344\237\342\373\360\375\115\241"
        "\273\036\060\276\171\231\042\333\333\153\376\301\270\161\343\350\311\223\340\034\242\231\226\106\013\000\331\163\161\163\320\116"
        "\052\061\206\123\365\022\105\165\267\127\374\201\316\235\073\363\304\137\000\023\020\001\040\077\036\137\104\055\026\006\367\244"
        "\040\143\026\311\325\057\121\164\241\175\273\057\110\112\112\242\077\377\374\323\071\342\070\201\110\300\004\200\054\375\175\246"
        "\125\064\063\327\215\063\301\205\150\374\105\377\244\317\234\111\277\350\350\150\332\261\203\157\377\016\164\002\052\000\254\310"
        "\232\263\171\012\045\315\017\215\211\051\306\045\252\376\101\321\170\320\207\323\370\233\065\153\106\077\374\360\203\163\224\161"
        "\002\231\200\012\000\201\004\146\374\060\221\222\346\260\004\030\337\301\311\003\327\372\075\227\373\042\042\042\150\345\312\225"
        "\074\351\347\122\002\056\000\104\364\004\276\235\116\311\323\336\224\026\231\141\144\044\176\365\012\105\244\075\233\355\307\175"
        "\376\077\376\370\243\163\124\161\334\210\053\002\100\140\354\157\127\055\243\344\221\377\242\346\326\170\116\126\160\206\021\124"
        "\377\203\342\053\336\240\260\130\373\072\077\300\204\337\266\155\333\234\243\211\343\126\134\023\200\047\273\167\357\242\166\103"
        "\022\171\110\300\110\101\227\077\272\317\273\364\131\204\175\157\077\310\311\311\241\023\047\116\070\107\020\307\315\270\056\000"
        "\344\312\225\053\324\177\124\157\212\037\373\272\364\040\140\102\223\204\051\257\122\144\372\207\117\047\373\300\350\321\243\351"
        "\316\235\073\316\221\303\161\073\112\004\200\340\221\115\263\147\317\242\370\334\217\051\161\006\137\052\014\145\360\066\237\250"
        "\202\167\305\013\136\074\015\077\056\056\216\126\255\132\305\223\175\212\243\114\000\236\354\335\273\227\272\344\165\242\330\201"
        "\157\123\322\067\074\054\010\051\346\377\203\142\113\337\174\272\246\037\340\211\076\205\205\205\164\362\344\111\347\010\341\250"
        "\214\162\001\040\367\357\337\247\252\252\052\152\321\061\206\342\312\337\244\244\052\026\101\223\306\252\157\174\345\033\024\321"
        "\366\337\117\057\357\201\326\255\133\213\131\176\176\240\247\276\150\021\200\047\347\317\237\247\341\245\303\051\262\325\107\102"
        "\004\070\103\110\017\040\046\050\301\152\076\334\302\353\075\316\307\335\174\023\047\116\244\233\067\103\343\141\054\046\107\253"
        "\000\020\214\371\366\354\331\103\275\172\365\244\310\324\217\105\027\221\127\022\006\071\226\310\343\107\277\111\021\155\076\174"
        "\356\214\217\206\077\144\310\020\072\176\374\270\123\175\216\356\150\027\200\047\020\301\356\335\273\305\170\260\131\162\030\105"
        "\367\175\207\022\277\174\105\134\043\226\036\144\214\161\140\162\067\246\370\155\212\110\261\037\330\351\001\013\172\206\015\033"
        "\306\015\337\300\030\043\000\117\260\212\020\023\205\375\372\365\243\146\061\021\024\231\371\201\170\003\014\117\030\032\212\065"
        "\276\117\030\367\006\105\345\374\213\302\243\236\135\313\007\061\061\061\064\142\304\010\321\360\171\166\337\314\030\047\000\117"
        "\040\002\054\006\231\062\145\012\045\267\110\246\360\244\217\050\252\327\273\024\077\361\165\161\263\210\364\140\144\324\140\365"
        "\312\160\015\077\246\357\273\024\216\263\175\275\156\076\110\117\117\247\231\063\147\212\071\036\156\370\146\307\130\001\324\017"
        "\236\375\266\172\365\152\312\313\313\023\067\207\204\045\174\114\121\371\226\014\306\277\306\127\020\124\201\063\375\224\327\304"
        "\252\275\210\026\177\157\364\030\337\227\224\224\320\057\277\374\102\017\037\076\164\052\307\061\075\101\041\200\372\101\257\140"
        "\306\214\031\224\225\225\045\016\072\134\123\156\326\355\075\212\035\361\246\170\156\034\367\016\002\104\365\113\224\370\365\313"
        "\142\370\025\225\377\336\323\027\161\324\007\143\373\356\335\273\323\374\371\363\351\302\205\013\116\205\070\301\224\240\023\200"
        "\047\165\165\165\164\354\330\061\232\065\153\026\165\351\322\105\310\000\353\311\043\132\132\103\005\253\167\020\127\141\011\301"
        "\072\200\171\022\261\341\044\315\302\365\372\327\355\263\174\233\017\351\263\310\347\307\364\000\017\346\310\317\317\247\205\013"
        "\027\322\331\263\147\271\213\037\344\011\132\001\324\017\144\160\352\324\051\161\120\242\033\332\274\171\163\161\260\206\105\175"
        "\102\341\251\037\122\124\267\367\051\166\310\133\224\060\311\032\062\314\345\041\003\300\244\052\306\361\261\303\255\063\174\336"
        "\173\024\321\352\103\373\145\233\136\135\173\320\252\125\053\032\076\174\270\270\057\037\343\172\314\317\160\232\106\232\204\000"
        "\352\007\147\044\254\064\074\160\340\000\315\236\075\233\172\365\352\045\316\132\141\141\141\342\340\306\114\165\104\153\113\012"
        "\075\254\141\303\240\267\305\031\117\334\233\200\271\204\246\330\133\100\127\176\346\313\024\077\376\165\212\035\372\266\130\203"
        "\217\205\071\141\321\237\074\167\215\336\003\366\023\326\345\027\027\027\013\241\142\006\377\301\203\007\316\336\345\064\265\064"
        "\071\001\310\342\021\302\222\045\113\250\274\274\234\062\063\063\051\066\066\326\226\202\163\340\013\061\130\147\101\134\316\212"
        "\056\172\107\234\031\161\367\142\302\264\127\305\315\053\111\363\060\277\140\130\357\001\317\131\370\306\332\056\153\373\022\277"
        "\174\325\156\344\145\157\122\114\377\167\304\273\363\043\261\020\307\152\350\365\033\170\175\360\371\343\343\343\305\223\166\053"
        "\053\053\305\031\036\303\252\332\132\176\221\152\250\044\044\004\340\035\364\022\256\136\275\052\026\036\125\127\127\213\133\120"
        "\161\205\001\153\323\321\133\370\133\143\211\260\004\021\377\261\030\116\104\266\377\100\064\056\114\214\105\027\376\223\142\007"
        "\274\055\144\201\311\062\064\300\204\057\136\245\104\113\032\211\377\265\370\012\274\042\046\047\223\146\276\142\141\375\072\013"
        "\277\177\366\053\346\051\320\003\111\234\156\201\257\267\276\017\322\111\264\376\235\370\011\257\211\007\145\304\225\275\045\156"
        "\236\202\230\160\006\027\215\273\303\007\226\260\076\022\223\240\141\221\136\333\053\001\302\303\345\071\364\210\360\134\375\245"
        "\113\227\322\376\375\373\351\326\255\133\316\136\341\204\142\102\122\000\377\127\320\123\270\164\351\222\170\337\374\367\337\177"
        "\117\323\246\115\243\322\322\122\052\050\050\240\366\355\333\123\142\142\342\163\275\206\377\211\325\275\026\267\273\132\204\065"
        "\373\124\364\060\104\267\333\233\146\300\032\173\343\353\360\365\222\156\171\103\300\135\165\311\311\311\242\167\203\106\216\236"
        "\316\364\351\323\305\315\066\207\017\037\026\317\144\340\313\163\034\357\260\000\032\020\364\030\320\055\306\013\051\256\135\273"
        "\106\107\217\036\245\055\133\266\010\111\240\007\201\206\066\176\374\170\052\053\053\023\053\030\163\163\163\305\145\312\166\355"
        "\332\121\233\066\155\050\045\045\105\074\342\012\147\141\074\341\026\022\361\006\227\324\060\366\306\004\046\336\172\233\226\226"
        "\046\276\077\073\073\133\364\116\060\271\211\106\215\233\150\276\376\372\153\132\264\150\221\170\122\056\036\233\205\113\243\067"
        "\156\334\020\353\045\370\316\072\116\303\103\364\377\001\001\016\121\026\237\213\202\166\000\000\000\000\111\105\116\104\256\102"
        "\140\202");
        defaultValue.setByteString(tmpImage, OpcUa_False);
    }
    pImage->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImage, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImage;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageBMP(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImageBMP = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageBMP, getTypeNamespace()));
    pImageBMP->setBrowseName(UaQualifiedName("ImageBMP", getTypeNamespace()));
    pImageBMP->setDisplayName(UaLocalizedText("", "ImageBMP"));
    pImageBMP->setAccessLevel(3);
    pImageBMP->setUserAccessLevel(3);
    pImageBMP->setValueRank(-1);
    pImageBMP->setDataTypeId(OpcUaId_ImageBMP);
    {
        UaByteString tmpImageBMP = UaByteString(8254, (OpcUa_Byte*) "\102\115\076\040\000\000\000\000\000\000\076\000\000\000\050\000\000\000\000\001\000\000\000\001\000\000\001\000\001\000\000\000"
        "\000\000\000\040\000\000\304\016\000\000\304\016\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\000\377\377"
        "\377\377\377\377\377\377\377\377\360\000\000\037\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\200\000\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\300\000\000\000\000\000\000\007\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\370\000\000\000\000\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\000\000\000\000\000\000\000\000\000\001\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\360\000\000\000\000\000\000\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\200\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\374\000\000\000\000\000\000\000\000\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\340\000\000\000\000\000\000\000\000\000\000\000\000\017\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\200\000\000\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\374\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\360\000\000\000\000\000\000\000\000\000\000\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\300\000\000\000\000\000\000\000\000\000\000\000\000\000\000\007\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\200\000\000\000\000\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\376\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\370\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\360\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\300\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\017\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\200\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\376\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\374\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\377"
        "\370\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\377\377"
        "\360\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\377\377"
        "\340\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\017\377\377\377\377\377\377\377\377\377\377\377\377"
        "\300\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\007\377\377\377\377\377\377\377\377\377\377\377\377"
        "\200\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\007\377\377\377\377\377\377\377\377\377\377\377\377"
        "\000\000\000\000\000\000\000\007\377\377\377\374\000\000\000\000\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\377"
        "\000\000\000\000\000\000\003\377\377\377\377\377\370\000\000\000\000\000\000\001\377\377\377\377\377\377\377\377\377\377\377\376"
        "\000\000\000\000\000\000\177\377\377\377\377\377\377\300\000\000\000\000\000\001\377\377\377\377\377\377\377\377\377\377\377\374"
        "\000\000\000\000\000\003\377\377\377\377\377\377\377\374\000\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\374"
        "\000\000\000\000\000\037\377\377\377\377\377\377\377\377\200\000\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\370"
        "\000\000\000\000\000\377\377\377\377\377\377\377\377\377\340\000\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\370"
        "\000\000\000\000\003\377\377\377\377\377\377\377\377\377\370\000\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\360"
        "\000\000\000\000\017\377\377\377\377\377\377\377\377\377\376\000\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\360"
        "\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\200\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\360"
        "\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\300\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\000\377\377\377\377\377\377\377\377\377\377\377\340\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\001\377\377\377\377\377\377\377\377\377\377\377\360\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\370\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\374\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\300"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\300"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\300"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\300"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\300"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\360\017\377\377\377\377\376\000\000\000\000\037\377\377\377\377\377\200\177\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\000\000\377\377\377\377\376\000\000\000\000\037\377\377\377\377\370\000\007\377\377\377\340"
        "\000\000\000\003\377\377\377\377\374\077\300\077\377\377\377\376\000\000\000\000\037\377\377\377\377\341\376\001\377\377\377\340"
        "\000\000\000\003\377\377\377\377\373\377\374\017\377\377\377\376\000\000\000\000\037\377\377\377\377\237\377\300\377\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\007\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\360\077\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\203\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\374\037\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\341\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\376\037\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\361\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\017\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\360\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\207\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\370\377\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\307\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\307\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\037\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\077\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\376\000\000\000\000\077\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\377\000\000\000\000\377\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\377\000\000\000\000\377\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\377\300\000\000\003\377\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\377\377\377\377\374\177\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\343\377\377\340"
        "\000\000\000\003\377\377\377\376\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\360\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\370\000\000\000\000\001\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\340\000\000\000\000\000\077\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\217\377\377\377\377\370\037\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\177\377\377\377\377\376\017\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\207\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\303\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\341\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\361\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\361\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\370\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\370\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\370\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\340\000\000\000\000\000\000\000\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\340\000\000\000\000\000\000\000\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\340\000\000\000\000\000\000\000\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\340\000\000\000\000\000\000\000\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\340\000\000\000\000\000\000\001\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\340\000\000\000\000\000\000\001\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\340\000\000\000\000\000\000\003\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\360\000\000\000\000\000\000\003\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\360\000\000\000\000\000\000\007\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\370\000\000\000\000\000\000\017\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\374\000\000\000\000\000\000\037\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\376\000\000\000\000\000\000\077\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\000\000\000\000\000\000\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\300\000\000\000\000\001\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\340"
        "\000\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\370\000\000\003\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\363\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\363\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\363\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\363\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\347\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\347\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\357\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\357\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\337\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\177\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\003\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\077\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\007\377\377\377"
        "\377\377\377\377\377\377\377\370\000\000\000\000\037\377\377\377\377\377\377\377\377\377\377\377\200\000\000\000\007\377\377\377"
        "\377\377\377\377\377\377\377\374\000\000\000\000\017\377\377\377\377\377\377\377\377\377\377\377\200\000\000\000\007\377\377\377"
        "\377\377\377\377\377\377\377\374\000\000\000\000\007\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\007\377\377\377"
        "\377\377\377\377\377\377\377\374\000\000\000\000\003\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\017\377\377\377"
        "\377\377\377\377\377\377\377\374\000\000\000\000\001\377\377\377\377\377\377\377\377\377\377\374\000\000\000\000\017\377\377\377"
        "\377\377\377\377\377\377\377\376\000\000\000\000\000\177\377\377\377\377\377\377\377\377\377\360\000\000\000\000\017\377\377\377"
        "\377\377\377\377\377\377\377\376\000\000\000\000\000\037\377\377\377\377\377\377\377\377\377\300\000\000\000\000\037\377\377\377"
        "\377\377\377\377\377\377\377\376\000\000\000\000\000\007\377\377\377\377\377\377\377\377\377\000\000\000\000\000\037\377\377\377"
        "\377\377\377\377\377\377\377\377\000\000\000\000\000\001\377\377\377\377\377\377\377\377\370\000\000\000\000\000\077\377\377\377"
        "\377\377\377\377\377\377\377\377\000\000\000\000\000\000\077\377\377\377\377\377\377\377\300\000\000\000\000\000\077\377\377\377"
        "\377\377\377\377\377\377\377\377\200\000\000\000\000\000\003\377\377\377\377\377\377\376\000\000\000\000\000\000\177\377\377\377"
        "\377\377\377\377\377\377\377\377\200\000\000\000\000\000\000\037\377\377\377\377\377\300\000\000\000\000\000\000\177\377\377\377"
        "\377\377\377\377\377\377\377\377\300\000\000\000\000\000\000\000\077\377\377\377\340\000\000\000\000\000\000\000\377\377\377\377"
        "\377\377\377\377\377\377\377\377\340\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\377\377\377\377"
        "\377\377\377\377\377\377\377\377\340\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377"
        "\377\377\377\377\377\377\377\377\360\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\007\377\377\377\377"
        "\377\377\377\377\377\377\377\377\370\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\017\377\377\377\377"
        "\377\377\377\377\377\377\377\377\374\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\037\377\377\377\377"
        "\377\377\377\377\377\377\377\377\376\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\077\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\177\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\200\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\300\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\360\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\007\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\370\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\017\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\374\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\077\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\300\000\000\000\000\000\000\000\000\000\000\000\000\000\000\001\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\360\000\000\000\000\000\000\000\000\000\000\000\000\000\000\007\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\374\000\000\000\000\000\000\000\000\000\000\000\000\000\000\037\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\177\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\370\000\000\000\000\000\000\000\000\000\000\000\000\017\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\376\000\000\000\000\000\000\000\000\000\000\000\000\177\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\340\000\000\000\000\000\000\000\000\000\000\003\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\374\000\000\000\000\000\000\000\000\000\000\077\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\377\300\000\000\000\000\000\000\000\000\001\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\377\374\000\000\000\000\000\000\000\000\077\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\360\000\000\000\000\000\000\017\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\340\000\000\000\000\007\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\200\000\377\377\377\377\377\377\377\377\377\377\377");
        defaultValue.setByteString(tmpImageBMP, OpcUa_False);
    }
    pImageBMP->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImageBMP, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImageBMP;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageGIF(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImageGIF = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageGIF, getTypeNamespace()));
    pImageGIF->setBrowseName(UaQualifiedName("ImageGIF", getTypeNamespace()));
    pImageGIF->setDisplayName(UaLocalizedText("", "ImageGIF"));
    pImageGIF->setAccessLevel(3);
    pImageGIF->setUserAccessLevel(3);
    pImageGIF->setValueRank(-1);
    pImageGIF->setDataTypeId(OpcUaId_ImageGIF);
    {
        UaByteString tmpImageGIF = UaByteString(12549, (OpcUa_Byte*) "\107\111\106\070\071\141\000\001\000\001\367\000\000\000\000\000\000\000\063\000\000\146\000\000\231\000\000\314\000\000\377\000"
        "\053\000\000\053\063\000\053\146\000\053\231\000\053\314\000\053\377\000\125\000\000\125\063\000\125\146\000\125\231\000\125\314"
        "\000\125\377\000\200\000\000\200\063\000\200\146\000\200\231\000\200\314\000\200\377\000\252\000\000\252\063\000\252\146\000\252"
        "\231\000\252\314\000\252\377\000\325\000\000\325\063\000\325\146\000\325\231\000\325\314\000\325\377\000\377\000\000\377\063\000"
        "\377\146\000\377\231\000\377\314\000\377\377\063\000\000\063\000\063\063\000\146\063\000\231\063\000\314\063\000\377\063\053\000"
        "\063\053\063\063\053\146\063\053\231\063\053\314\063\053\377\063\125\000\063\125\063\063\125\146\063\125\231\063\125\314\063\125"
        "\377\063\200\000\063\200\063\063\200\146\063\200\231\063\200\314\063\200\377\063\252\000\063\252\063\063\252\146\063\252\231\063"
        "\252\314\063\252\377\063\325\000\063\325\063\063\325\146\063\325\231\063\325\314\063\325\377\063\377\000\063\377\063\063\377\146"
        "\063\377\231\063\377\314\063\377\377\146\000\000\146\000\063\146\000\146\146\000\231\146\000\314\146\000\377\146\053\000\146\053"
        "\063\146\053\146\146\053\231\146\053\314\146\053\377\146\125\000\146\125\063\146\125\146\146\125\231\146\125\314\146\125\377\146"
        "\200\000\146\200\063\146\200\146\146\200\231\146\200\314\146\200\377\146\252\000\146\252\063\146\252\146\146\252\231\146\252\314"
        "\146\252\377\146\325\000\146\325\063\146\325\146\146\325\231\146\325\314\146\325\377\146\377\000\146\377\063\146\377\146\146\377"
        "\231\146\377\314\146\377\377\231\000\000\231\000\063\231\000\146\231\000\231\231\000\314\231\000\377\231\053\000\231\053\063\231"
        "\053\146\231\053\231\231\053\314\231\053\377\231\125\000\231\125\063\231\125\146\231\125\231\231\125\314\231\125\377\231\200\000"
        "\231\200\063\231\200\146\231\200\231\231\200\314\231\200\377\231\252\000\231\252\063\231\252\146\231\252\231\231\252\314\231\252"
        "\377\231\325\000\231\325\063\231\325\146\231\325\231\231\325\314\231\325\377\231\377\000\231\377\063\231\377\146\231\377\231\231"
        "\377\314\231\377\377\314\000\000\314\000\063\314\000\146\314\000\231\314\000\314\314\000\377\314\053\000\314\053\063\314\053\146"
        "\314\053\231\314\053\314\314\053\377\314\125\000\314\125\063\314\125\146\314\125\231\314\125\314\314\125\377\314\200\000\314\200"
        "\063\314\200\146\314\200\231\314\200\314\314\200\377\314\252\000\314\252\063\314\252\146\314\252\231\314\252\314\314\252\377\314"
        "\325\000\314\325\063\314\325\146\314\325\231\314\325\314\314\325\377\314\377\000\314\377\063\314\377\146\314\377\231\314\377\314"
        "\314\377\377\377\000\000\377\000\063\377\000\146\377\000\231\377\000\314\377\000\377\377\053\000\377\053\063\377\053\146\377\053"
        "\231\377\053\314\377\053\377\377\125\000\377\125\063\377\125\146\377\125\231\377\125\314\377\125\377\377\200\000\377\200\063\377"
        "\200\146\377\200\231\377\200\314\377\200\377\377\252\000\377\252\063\377\252\146\377\252\231\377\252\314\377\252\377\377\325\000"
        "\377\325\063\377\325\146\377\325\231\377\325\314\377\325\377\377\377\000\377\377\063\377\377\146\377\377\231\377\377\314\377\377"
        "\377\000\000\000\000\000\000\000\000\000\000\000\000\041\371\004\001\000\000\374\000\054\000\000\000\000\000\001\000\001\000\010"
        "\377\000\367\011\034\110\260\240\301\203\010\023\052\134\310\260\241\303\207\020\015\056\123\246\154\030\261\114\030\047\241\321"
        "\250\121\014\032\217\040\077\242\111\043\211\344\044\214\027\211\121\324\027\261\245\313\227\060\143\312\234\111\063\246\262\213"
        "\231\072\202\334\051\046\144\117\221\076\321\060\002\372\061\150\117\217\047\063\251\124\126\263\251\323\247\120\243\326\274\231"
        "\223\250\121\221\150\254\212\034\352\221\253\326\253\074\103\156\314\104\121\252\331\263\150\323\102\244\132\364\253\333\237\127"
        "\307\210\224\013\366\155\326\220\134\101\142\124\126\117\255\337\277\200\145\052\253\272\363\356\133\111\135\265\162\105\374\065"
        "\215\030\304\151\274\076\006\351\330\056\320\243\150\224\006\336\314\271\063\325\272\076\031\113\006\032\031\015\135\323\104\275"
        "\216\246\314\232\061\110\306\247\307\114\316\252\225\154\347\333\270\235\016\236\004\332\265\353\271\300\203\216\076\015\132\350"
        "\127\111\304\077\062\276\374\265\247\346\334\320\243\063\334\175\165\165\145\061\225\211\377\256\353\065\371\354\342\241\175\172"
        "\377\224\353\266\255\230\347\322\323\107\127\306\333\256\307\106\356\255\212\116\255\330\064\354\340\100\267\177\274\216\065\357"
        "\374\302\126\331\246\336\200\200\015\006\127\175\336\375\367\037\151\254\355\047\234\133\252\135\127\231\152\301\031\305\337\152"
        "\227\115\302\024\201\034\112\145\040\204\361\161\307\037\136\104\171\027\337\160\020\216\130\242\145\316\165\350\042\115\354\141"
        "\106\142\154\133\061\210\132\203\340\325\010\226\176\071\046\026\332\151\243\141\050\136\146\057\026\031\121\214\130\365\170\243"
        "\161\140\251\310\210\143\074\062\151\337\170\016\116\051\337\153\124\126\367\026\224\076\046\331\242\221\140\042\204\144\170\150"
        "\300\207\245\224\135\372\270\240\220\023\362\147\342\167\163\101\026\124\162\117\306\245\134\217\036\025\045\006\061\141\366\051"
        "\020\061\171\052\171\146\135\157\226\031\042\232\127\046\112\347\241\131\336\207\335\153\155\141\265\241\237\056\052\303\234\117"
        "\043\146\207\337\243\125\332\371\040\170\121\356\310\135\225\213\365\066\244\125\347\121\332\141\046\007\046\372\135\224\222\255"
        "\377\211\132\220\053\162\325\136\110\164\335\172\135\250\122\056\212\351\240\362\221\047\333\157\105\115\252\052\164\110\002\265"
        "\130\241\300\222\050\350\222\115\012\331\353\246\244\122\373\046\227\137\015\225\132\240\174\036\233\233\245\205\005\352\026\263"
        "\320\132\245\042\234\316\062\012\142\232\072\036\146\324\255\136\002\107\024\032\336\336\306\152\176\343\031\205\334\235\125\216"
        "\241\037\215\024\146\071\253\271\350\242\271\335\202\234\216\072\133\167\041\346\351\157\270\155\031\133\257\132\367\142\050\324"
        "\271\364\071\373\233\264\005\167\205\261\144\156\122\253\245\167\041\247\333\032\166\026\243\053\361\304\146\301\353\335\214\041"
        "\142\074\247\253\212\342\027\353\314\135\232\111\360\161\015\162\234\032\161\267\166\313\262\131\231\030\206\045\224\356\032\312"
        "\356\321\270\042\274\242\240\043\376\233\064\255\115\066\333\356\133\231\014\055\025\253\077\275\371\251\126\344\056\335\351\204"
        "\065\002\331\256\325\013\067\066\262\272\257\321\010\327\271\131\153\255\333\117\114\352\327\346\257\074\033\027\166\335\317\126"
        "\377\135\056\145\001\377\255\143\151\151\323\265\357\125\072\156\027\267\334\063\051\023\250\276\040\141\110\165\247\355\312\305"
        "\060\130\107\341\020\206\030\067\160\161\203\030\061\200\216\303\026\243\167\176\103\351\141\150\056\306\346\062\306\334\167\175"
        "\223\201\067\014\343\063\351\164\270\215\342\342\050\047\277\070\042\372\335\121\234\203\176\103\014\067\014\077\174\014\232\343"
        "\020\174\353\300\253\176\203\014\306\017\237\072\353\032\263\153\342\336\076\301\373\250\153\070\254\114\373\103\367\012\167\056"
        "\332\207\276\254\171\364\061\244\377\271\030\223\234\264\324\076\364\270\124\017\064\067\135\324\021\016\303\007\257\074\354\114"
        "\106\335\145\235\202\003\215\030\276\047\277\034\015\305\137\224\043\024\277\126\303\071\317\015\317\163\316\121\111\137\374\122"
        "\017\352\010\357\100\011\113\034\216\374\127\056\063\361\150\200\004\204\010\046\100\162\053\136\221\111\147\007\344\035\252\214"
        "\007\275\055\234\207\030\321\120\017\061\076\262\276\327\151\251\152\256\271\201\320\102\270\020\307\251\315\167\374\302\336\165"
        "\377\304\340\271\030\100\120\022\073\354\120\075\064\041\011\325\115\246\144\031\003\242\317\166\002\015\036\062\244\043\166\301"
        "\136\265\304\303\071\343\171\304\173\106\232\241\030\366\167\063\040\132\050\173\140\271\301\342\254\210\020\007\301\006\126\123"
        "\332\225\311\056\123\074\342\355\111\153\303\300\340\330\262\124\031\235\241\155\073\067\230\040\033\015\022\276\274\165\354\151"
        "\045\102\314\360\110\307\010\060\326\313\161\242\122\341\240\056\264\244\274\254\161\220\003\321\036\330\104\166\060\243\340\100"
        "\175\142\160\344\320\306\144\102\266\015\361\006\230\054\210\017\147\363\060\106\235\353\126\141\210\101\352\222\310\303\102\336"
        "\060\201\216\372\112\014\104\111\100\160\065\046\160\357\241\031\110\076\371\071\136\322\116\031\146\042\034\341\156\344\032\355"
        "\155\214\050\061\230\104\052\063\371\254\311\171\347\006\133\270\301\030\246\111\220\125\232\222\114\342\101\045\067\261\270\075"
        "\353\045\020\166\062\210\045\275\270\111\020\061\256\253\174\164\064\046\343\316\126\311\055\055\215\163\241\143\247\101\370\146"
        "\212\245\162\056\020\227\073\211\001\055\255\010\036\071\376\023\157\262\014\244\076\013\342\316\260\071\351\055\061\270\244\025"
        "\365\161\062\327\331\310\053\320\223\346\102\273\331\067\025\371\261\050\067\110\303\064\353\001\066\131\151\021\237\033\125\045"
        "\202\364\306\117\277\005\157\232\321\020\233\211\050\144\271\155\165\056\245\034\125\132\051\333\226\061\304\274\164\232\347\014"
        "\311\355\004\026\236\312\020\017\247\003\361\146\357\340\344\244\163\345\063\225\364\300\013\322\040\307\273\335\371\144\170\110"
        "\025\210\122\311\206\235\070\211\105\200\237\377\343\346\274\320\150\243\052\221\346\062\061\310\352\076\034\107\270\116\366\024"
        "\145\206\001\340\117\061\131\217\063\076\163\152\100\301\152\126\175\371\125\231\275\062\167\141\005\252\307\012\106\052\154\001"
        "\020\137\172\105\252\062\274\062\260\307\270\121\105\074\233\353\040\143\252\225\033\151\322\141\106\323\124\117\322\272\327\375"
        "\010\351\256\165\332\027\356\236\232\312\217\164\244\044\211\111\034\276\064\370\221\243\356\225\114\232\065\314\114\261\105\103"
        "\020\142\062\252\361\321\324\150\340\223\044\032\162\101\255\076\014\316\162\036\243\235\362\354\044\260\245\305\121\107\112\110"
        "\231\375\020\045\231\212\344\254\142\111\302\276\334\041\312\067\120\232\252\322\220\213\311\304\350\147\136\001\223\204\133\134"
        "\253\130\272\075\021\062\042\071\034\306\152\012\122\333\166\167\044\224\171\043\122\250\064\334\312\124\206\111\240\003\356\130"
        "\167\226\247\241\354\152\123\334\145\343\062\210\062\334\312\162\012\205\027\243\022\171\161\352\270\366\170\360\116\363\061\333"
        "\177\175\104\132\114\032\107\256\110\103\140\377\115\277\152\264\034\112\227\301\206\261\221\175\373\233\261\373\322\045\300\126"
        "\304\355\175\107\123\043\373\334\251\075\367\065\116\072\201\113\334\213\051\247\077\167\111\203\151\105\142\330\273\110\166\220"
        "\206\121\257\170\037\306\037\171\025\216\206\037\116\051\075\212\334\035\253\326\227\130\160\375\061\033\365\350\123\034\227\006"
        "\201\315\355\152\142\220\107\143\064\354\116\273\065\016\146\110\064\102\036\217\070\006\305\126\124\316\124\165\374\230\365\032"
        "\347\315\332\222\313\026\222\274\121\160\335\367\073\154\316\112\201\341\124\052\051\363\160\031\342\332\160\145\366\065\144\361"
        "\232\231\217\216\105\351\153\015\055\132\056\241\026\065\204\336\325\210\133\353\136\040\023\372\320\342\065\264\131\023\215\030"
        "\106\003\345\223\152\035\060\175\273\224\037\305\344\271\323\265\235\046\156\107\015\234\241\210\206\315\260\076\264\322\142\111"
        "\143\361\362\330\314\204\246\013\165\167\275\025\315\156\116\254\033\111\364\176\226\043\254\270\276\271\323\201\112\054\116\243"
        "\232\035\306\360\110\323\342\172\122\157\375\377\034\102\100\143\005\201\074\326\226\253\331\347\334\033\351\130\151\243\323\257"
        "\260\257\354\145\004\126\106\043\117\322\226\232\207\302\033\152\207\320\070\312\114\167\225\046\254\240\220\054\070\245\312\170"
        "\364\062\367\114\054\317\336\371\056\150\156\012\064\346\247\214\052\346\106\134\136\316\323\215\275\342\030\373\172\031\326\017"
        "\137\035\235\027\352\103\142\147\105\056\373\042\163\271\155\254\156\336\232\051\340\066\371\204\047\060\341\006\067\264\001\023"
        "\155\060\071\046\076\121\360\315\144\005\076\165\142\163\107\144\376\146\336\124\171\303\037\001\365\153\355\273\155\115\247\067"
        "\313\167\331\263\273\133\242\017\120\140\202\016\044\237\203\033\072\101\007\067\040\275\351\047\167\103\062\014\256\226\272\322"
        "\006\151\136\151\117\222\030\243\143\124\073\246\075\367\256\363\205\057\016\137\352\266\007\343\306\341\215\216\055\107\022\370"
        "\126\330\046\113\167\203\322\347\160\164\067\140\202\356\163\077\272\322\335\220\011\226\250\045\301\206\076\167\172\375\025\031"
        "\035\163\151\355\157\376\010\255\263\252\142\377\110\173\144\167\172\176\324\251\333\214\330\112\273\104\037\165\167\203\047\116"
        "\116\207\224\167\242\344\164\270\204\346\123\176\367\272\313\123\060\101\107\073\063\377\075\033\244\071\267\116\317\003\056\266"
        "\104\373\170\103\221\107\274\376\215\066\342\207\356\220\246\207\276\363\116\157\203\047\234\036\367\316\067\235\351\162\277\273"
        "\324\323\122\370\034\073\126\332\217\107\112\126\264\013\237\341\056\136\261\351\035\064\267\313\274\361\256\272\232\360\011\067"
        "\064\310\033\002\015\067\210\136\351\107\277\004\322\333\020\172\247\213\176\370\244\337\373\360\351\220\214\263\340\026\316\250"
        "\121\263\367\371\365\365\034\157\167\342\372\324\140\327\026\144\334\246\155\376\207\051\016\062\176\014\121\167\303\247\174\300"
        "\007\174\233\027\174\243\327\164\352\047\174\051\167\172\057\101\166\152\027\164\223\207\170\066\247\034\030\027\175\072\207\124"
        "\044\045\022\037\370\044\264\127\166\056\346\137\310\201\173\011\247\200\012\221\014\311\307\176\354\207\162\303\367\171\235\167"
        "\203\023\250\174\022\270\171\030\330\022\302\377\226\160\130\204\202\330\201\152\262\226\025\322\126\031\331\104\143\130\324\160"
        "\375\101\135\056\210\136\216\345\134\164\361\166\020\241\171\045\247\164\021\270\171\120\007\177\120\367\205\044\367\171\074\210"
        "\006\077\010\021\250\301\033\033\050\175\132\006\137\043\341\152\044\101\036\333\066\202\040\066\137\121\310\155\230\345\204\333"
        "\147\050\306\241\110\226\347\020\320\220\162\164\027\167\162\147\167\302\147\162\303\247\176\066\150\162\306\127\203\164\327\006"
        "\032\365\024\216\343\163\151\227\047\250\326\151\337\166\032\066\366\050\330\304\204\267\206\170\073\326\163\157\066\205\166\170"
        "\146\175\330\020\237\220\203\354\247\210\016\130\210\112\147\174\203\270\171\015\110\203\045\267\113\121\341\204\055\030\143\360"
        "\201\170\171\346\152\231\250\021\321\025\152\071\226\151\216\325\202\043\221\177\206\307\155\375\127\204\251\346\022\044\027\201"
        "\237\027\201\355\007\174\245\027\167\074\230\167\023\030\006\022\065\023\024\025\170\110\030\154\247\365\174\044\241\213\241\010"
        "\137\141\040\003\300\305\156\316\345\160\336\377\125\032\352\046\212\201\267\072\245\310\020\236\360\200\022\370\171\354\207\210"
        "\112\047\213\173\047\206\213\330\164\060\350\022\303\366\146\167\046\136\170\270\207\347\365\206\342\025\166\024\307\037\226\070"
        "\024\350\150\142\017\047\155\340\207\145\377\170\020\141\150\176\305\047\210\262\170\210\353\027\201\112\207\210\047\327\006\251"
        "\003\025\044\165\166\301\046\136\033\310\213\271\210\032\135\327\127\012\031\200\050\251\207\171\262\213\345\146\153\267\107\142"
        "\057\307\173\011\061\217\203\230\217\325\330\214\064\330\164\255\130\203\251\230\212\164\240\154\115\021\220\026\007\157\371\267"
        "\207\114\242\021\205\207\030\327\307\140\230\130\204\245\001\037\032\167\161\056\050\027\033\310\133\072\211\020\044\247\176\166"
        "\067\167\346\047\224\046\127\226\242\247\174\016\130\201\233\207\011\305\003\025\003\306\215\032\121\175\072\366\201\150\250\202"
        "\032\161\143\250\301\145\173\205\030\163\231\160\323\027\164\047\131\166\302\010\230\221\361\225\007\301\205\347\347\164\050\047"
        "\224\066\150\224\367\070\210\025\110\162\064\377\230\137\120\001\031\176\251\160\122\102\135\264\361\174\046\350\170\123\211\157"
        "\171\026\230\350\146\050\234\211\064\162\071\022\266\206\230\006\061\174\222\251\212\047\067\224\355\147\201\024\370\205\104\171"
        "\224\000\150\023\155\110\033\055\050\027\341\310\025\010\351\201\172\370\150\320\163\216\033\141\050\300\351\145\040\170\222\015"
        "\107\204\313\311\033\025\111\110\132\230\212\237\107\226\311\127\162\325\131\162\105\331\171\107\127\231\271\031\023\274\365\206"
        "\323\207\154\201\051\155\173\270\142\347\246\150\330\247\143\376\165\234\165\002\225\312\231\177\253\171\234\234\311\232\005\001"
        "\165\356\327\205\206\010\210\203\210\226\250\330\235\202\030\006\342\344\024\024\245\123\245\121\214\167\131\022\066\126\032\012"
        "\332\166\206\026\232\165\206\216\375\243\234\203\345\213\205\307\155\355\041\155\274\141\205\016\201\011\260\230\162\262\370\171"
        "\203\150\217\134\230\162\021\250\217\206\210\224\065\321\166\307\231\025\066\207\166\354\105\214\252\371\104\217\042\207\242\051"
        "\022\271\230\151\134\251\232\056\252\160\130\274\244\240\253\031\235\005\341\205\026\010\240\201\230\162\321\310\237\324\030\210"
        "\371\310\071\160\351\131\366\101\036\171\211\100\250\225\227\160\345\233\231\046\024\052\112\161\050\370\155\000\144\150\147\307"
        "\030\037\050\236\325\125\032\061\260\116\055\301\205\147\011\174\366\150\174\213\131\210\025\150\226\371\070\174\133\072\023\231"
        "\326\165\172\146\132\123\210\155\204\107\220\131\112\170\143\364\235\323\064\140\335\050\155\125\072\236\264\061\245\313\221\227"
        "\101\032\217\013\341\176\063\370\200\347\147\244\212\230\162\213\071\251\354\007\241\215\143\070\375\207\155\072\026\236\274\010"
        "\127\246\311\150\165\172\044\366\363\162\323\307\202\135\025\151\152\250\250\273\311\236\046\041\040\060\322\245\063\332\166\354"
        "\370\251\073\246\232\127\226\161\377\366\111\020\237\340\171\131\270\237\203\310\237\312\047\226\037\351\212\064\150\243\065\361"
        "\160\021\347\247\007\152\163\302\111\237\266\142\032\232\272\026\105\103\125\163\124\060\322\022\040\060\062\175\036\323\254\307"
        "\170\247\027\303\165\266\146\245\102\112\020\224\052\226\221\371\173\374\111\203\074\270\256\242\047\240\121\161\245\004\271\030"
        "\342\011\244\301\146\240\055\312\071\346\010\023\134\003\230\130\226\143\332\202\151\230\065\241\211\006\057\120\302\166\145\150"
        "\020\310\244\234\171\151\250\067\246\161\176\132\204\373\367\060\034\332\020\036\131\251\312\047\213\221\311\244\225\232\261\354"
        "\127\252\061\121\256\056\132\262\203\326\222\355\201\257\055\331\206\042\253\020\365\300\052\342\211\155\135\045\263\360\265\034"
        "\332\127\214\006\333\243\313\141\030\003\365\020\371\226\214\171\251\166\045\201\231\161\064\261\100\112\212\057\061\174\215\270"
        "\210\243\127\174\355\332\176\371\030\251\361\212\003\160\151\207\373\042\155\350\030\114\165\351\142\010\224\243\071\267\005\057"
        "\161\057\073\013\127\003\033\225\377\146\206\234\054\071\255\267\152\161\150\070\205\013\233\124\265\247\141\142\212\153\056\326"
        "\166\346\366\252\313\230\246\153\232\226\050\012\242\360\007\262\320\310\237\055\373\022\044\333\242\360\071\264\305\350\154\135"
        "\107\036\273\046\011\057\271\020\254\302\215\316\251\232\330\266\143\066\133\230\344\124\214\016\123\230\336\312\033\157\273\126"
        "\301\146\253\176\312\243\346\126\150\273\311\133\066\327\253\003\221\237\374\171\254\300\232\217\300\347\261\035\153\176\370\063"
        "\257\245\161\273\332\222\213\104\010\225\221\046\136\250\213\144\055\141\051\015\207\173\337\366\227\146\046\157\303\326\112\073"
        "\052\175\245\371\227\147\150\050\266\126\046\157\313\036\075\332\270\024\153\267\243\053\272\206\253\272\002\001\177\075\171\226"
        "\260\153\251\154\132\251\221\312\237\325\032\023\321\220\275\316\212\152\070\233\262\274\226\257\355\046\250\007\221\011\124\272"
        "\252\112\123\262\321\373\233\264\161\234\212\073\033\371\372\160\236\206\032\136\351\022\076\304\133\107\313\254\210\153\275\320"
        "\233\053\171\033\021\045\212\162\377\161\267\264\157\072\254\316\250\261\371\370\226\117\141\165\127\173\170\307\351\057\104\170"
        "\232\276\013\300\131\361\270\010\101\122\207\272\057\155\147\274\146\153\253\362\266\265\245\053\272\154\327\301\342\312\033\331"
        "\330\103\101\073\130\250\165\073\355\261\134\127\027\223\075\161\256\003\161\210\337\013\242\261\153\301\344\033\276\203\150\231"
        "\004\032\216\206\313\277\135\367\304\314\312\303\372\306\010\312\332\103\204\226\270\155\307\155\000\234\302\073\306\273\104\026"
        "\223\342\151\250\213\273\305\157\166\003\145\150\051\115\334\304\103\313\305\364\111\214\250\313\275\373\020\174\334\171\217\300"
        "\127\201\161\307\261\303\272\261\021\050\257\120\021\175\114\074\137\360\053\175\050\330\242\010\324\021\233\010\021\105\163\234"
        "\130\311\274\366\321\222\117\154\153\210\353\304\340\350\304\033\247\240\135\132\022\214\220\052\153\001\163\341\230\303\331\005"
        "\157\103\141\300\132\014\305\360\230\264\147\331\267\105\214\242\044\212\307\112\067\270\000\311\021\102\250\306\117\254\252\260"
        "\072\272\215\340\312\004\101\277\377\011\034\223\277\213\241\336\330\206\342\011\313\235\214\247\277\014\313\153\114\171\075\161"
        "\044\007\334\146\076\314\273\211\013\176\267\310\156\160\254\230\105\154\307\155\052\172\041\011\174\176\353\256\042\111\265\031"
        "\074\154\312\170\213\155\326\236\262\134\223\305\310\227\017\141\241\307\351\314\026\212\270\101\053\313\314\274\265\223\320\311"
        "\224\314\316\307\351\244\153\141\232\131\253\275\036\207\300\114\074\272\323\174\312\167\174\304\340\073\254\375\010\242\203\210"
        "\313\020\221\276\242\073\274\172\366\155\245\201\156\326\213\275\022\027\021\355\263\316\343\072\314\153\274\321\032\255\321\277"
        "\314\321\033\015\176\260\014\072\202\324\020\312\200\247\274\313\302\115\114\146\012\072\137\273\006\307\103\374\275\330\074\320"
        "\121\073\323\064\350\221\347\013\023\044\305\155\356\334\310\050\070\264\130\332\320\030\115\302\363\213\321\260\174\324\155\114"
        "\317\110\275\324\353\214\214\033\275\316\356\134\074\077\270\130\270\072\256\167\073\234\055\332\010\075\035\216\001\035\233\162"
        "\027\222\103\274\210\312\107\226\377\366\070\254\125\074\023\030\155\207\352\334\240\042\114\271\050\071\205\062\040\277\014\305"
        "\324\164\015\322\035\155\327\170\335\321\033\101\022\243\263\014\371\374\316\171\031\221\235\014\250\055\212\270\365\013\304\002"
        "\261\261\005\075\007\010\315\330\346\327\256\367\010\210\104\174\011\067\020\006\120\101\017\032\130\262\314\051\315\100\055\243"
        "\171\211\220\012\055\020\320\140\324\244\115\327\245\175\332\107\235\327\222\340\213\352\351\020\061\342\311\267\350\260\370\233"
        "\302\135\105\227\301\206\330\373\240\264\162\147\307\136\335\210\061\215\304\170\147\162\147\055\023\363\054\011\176\332\247\010"
        "\132\262\201\255\240\034\241\305\143\220\323\010\161\321\223\240\332\032\315\021\251\155\022\033\061\264\246\075\335\330\315\021"
        "\110\233\317\314\135\267\230\134\313\220\266\243\266\334\300\127\330\256\103\334\272\340\153\307\110\334\006\231\052\327\021\061"
        "\140\322\126\175\300\314\313\075\012\313\130\213\321\150\200\316\076\273\316\326\255\335\046\061\340\045\301\021\005\276\021\336"
        "\175\340\322\155\335\173\275\332\377\335\135\022\261\364\210\256\155\337\355\354\142\051\375\324\010\311\243\320\351\250\012\241"
        "\333\220\035\323\155\300\333\217\335\171\104\234\162\030\274\304\042\114\317\215\333\242\066\246\316\214\372\334\362\135\020\231"
        "\340\340\230\120\334\157\340\340\253\235\335\004\316\340\047\201\340\231\061\264\105\323\343\102\316\335\153\074\343\216\101\074"
        "\147\214\240\164\073\314\314\252\305\306\214\253\206\314\341\073\011\242\037\031\224\330\114\226\341\253\315\045\032\167\320\055"
        "\077\333\373\316\130\231\337\030\035\225\215\234\220\003\012\021\057\273\316\230\220\006\363\134\343\067\236\011\151\060\343\157"
        "\236\006\065\376\343\341\270\346\027\135\025\006\176\347\161\076\011\067\136\334\072\066\074\022\356\332\334\255\310\205\335\265"
        "\000\054\267\217\074\243\270\255\264\354\015\242\334\114\345\046\247\336\155\220\217\155\060\334\043\173\337\156\255\341\121\154"
        "\271\272\132\150\161\375\022\354\041\011\071\141\022\071\056\335\005\316\346\045\121\352\323\375\006\355\263\332\156\040\335\071"
        "\341\347\244\136\334\326\035\353\377\231\300\010\246\263\260\230\155\271\263\014\305\030\156\153\203\014\250\270\175\226\025\050"
        "\331\114\113\226\257\213\354\045\316\307\116\061\140\056\116\171\120\251\241\031\315\334\322\076\302\226\375\022\321\160\347\176"
        "\336\355\222\340\352\337\076\335\101\316\021\170\016\353\363\334\352\104\376\347\044\001\347\244\136\340\321\245\103\004\074\006"
        "\323\315\243\071\372\233\260\152\350\200\056\311\215\076\254\374\176\345\223\136\251\045\216\320\227\220\217\230\016\023\343\115"
        "\306\075\115\355\367\214\324\230\134\360\013\061\103\016\076\335\223\020\353\023\337\076\343\216\353\010\116\352\154\056\347\022"
        "\037\347\021\217\347\065\276\361\064\224\121\242\316\254\136\273\326\114\074\310\352\174\311\160\214\162\064\330\272\034\333\171"
        "\155\132\351\052\067\167\057\117\007\135\336\022\113\226\350\370\315\305\117\275\326\310\115\324\323\361\023\354\156\361\104\076"
        "\353\022\017\347\345\076\356\112\337\076\256\136\343\257\316\346\167\101\074\241\063\073\242\336\316\235\254\214\117\076\272\175"
        "\052\272\323\034\265\125\176\312\377\307\376\325\010\115\363\360\055\320\047\336\024\351\073\346\107\213\231\133\233\161\314\251"
        "\240\145\022\332\010\021\015\231\020\113\301\163\072\323\243\074\172\217\077\313\363\071\237\163\076\171\277\367\200\317\071\331"
        "\104\074\134\100\206\066\161\341\116\335\174\324\316\304\025\116\354\122\216\020\131\050\351\214\275\210\352\155\363\105\174\371"
        "\360\255\304\115\101\122\267\373\326\130\117\214\114\156\317\032\161\261\055\261\014\254\202\115\054\024\113\254\237\077\124\077"
        "\074\245\203\370\306\123\104\262\104\074\320\203\074\233\043\253\060\041\200\215\037\336\372\035\234\012\032\346\072\061\371\211"
        "\311\354\247\174\366\057\077\254\066\135\366\041\033\343\020\361\363\046\273\313\310\115\227\303\177\234\351\003\025\321\100\017"
        "\313\100\017\024\121\101\023\121\101\312\300\375\344\377\375\174\141\376\346\037\376\343\317\027\272\301\360\076\337\320\075\275"
        "\313\267\155\374\006\301\357\360\215\345\137\015\360\075\111\363\163\020\247\000\321\246\115\030\034\373\014\036\104\230\120\341"
        "\102\203\313\320\110\112\003\221\377\121\104\212\020\055\242\231\204\021\115\305\210\032\055\212\301\021\203\341\110\222\045\115"
        "\236\104\231\022\241\062\217\024\321\064\172\230\106\143\306\214\032\043\216\261\130\021\215\230\033\142\124\042\364\344\246\215"
        "\233\071\155\350\014\025\050\124\151\121\241\163\204\042\125\232\164\350\015\221\077\117\162\234\371\160\214\326\254\027\063\246"
        "\231\230\023\115\230\252\126\315\236\105\253\122\131\106\210\061\045\155\374\170\363\055\333\216\165\333\116\342\351\323\054\246"
        "\066\114\057\065\045\332\327\350\245\241\116\005\036\075\352\264\150\142\114\067\302\244\145\110\017\243\230\226\055\305\102\024"
        "\363\165\056\032\233\222\100\226\205\034\132\064\132\145\231\137\146\305\113\063\346\144\325\064\305\344\364\334\363\254\047\250"
        "\213\005\362\175\052\364\350\141\244\275\163\267\241\072\072\141\153\215\141\165\022\317\010\123\265\362\261\240\205\077\207\036"
        "\231\243\044\234\307\121\263\276\216\131\266\131\072\201\175\333\116\214\164\167\337\300\210\215\016\045\030\235\045\362\353\311"
        "\055\117\327\070\046\170\164\372\377\365\015\256\207\373\036\266\106\376\304\225\347\235\115\074\362\230\022\250\066\001\177\223"
        "\252\250\371\240\243\153\063\315\144\332\117\065\257\262\042\313\076\013\241\133\053\053\227\064\203\153\272\216\152\122\015\100"
        "\263\000\223\012\060\305\022\134\252\274\302\220\342\211\076\061\050\062\355\042\346\100\354\360\256\027\067\373\354\102\035\105"
        "\303\157\102\266\252\243\016\066\271\074\024\043\014\275\254\242\355\251\335\014\373\055\260\246\012\134\012\052\340\234\023\055"
        "\032\041\257\364\020\113\041\053\334\261\313\263\226\321\062\113\375\340\303\153\073\253\210\322\115\052\301\226\214\062\115\301"
        "\236\164\212\313\347\350\371\260\075\267\052\302\151\114\355\250\364\262\317\222\326\063\315\265\060\205\324\260\255\030\216\374"
        "\051\250\002\317\073\112\111\244\212\122\263\321\022\005\302\341\006\372\210\013\323\270\101\073\022\203\117\077\077\125\210\316"
        "\115\305\122\315\062\214\076\062\363\047\022\237\052\214\125\046\117\164\324\066\242\344\024\016\232\100\343\173\113\314\326\142"
        "\344\210\221\005\101\005\126\041\377\374\330\153\020\117\041\047\232\111\104\044\143\075\057\051\303\232\355\155\267\250\012\014"
        "\211\276\121\353\044\266\070\212\176\015\266\333\175\224\261\316\307\256\262\344\110\131\125\011\354\015\132\126\321\204\164\305"
        "\001\205\012\303\122\350\352\261\063\333\142\261\364\125\006\373\240\121\206\137\177\373\005\370\137\201\003\356\027\232\013\225"
        "\271\326\301\172\335\102\343\320\331\114\154\325\131\106\241\164\127\122\152\075\205\314\336\205\065\346\166\064\150\130\135\064"
        "\144\221\107\056\020\023\145\354\003\223\334\204\071\162\055\125\225\100\256\230\142\211\311\113\327\260\243\342\215\016\232\215"
        "\167\146\257\221\033\266\120\157\122\047\127\154\223\150\051\235\322\247\276\141\171\036\127\134\064\270\100\043\100\233\325\064"
        "\214\051\110\247\245\331\273\066\252\235\127\143\257\355\354\070\264\117\252\026\354\321\004\317\356\255\250\147\335\130\206\276"
        "\014\277\206\173\062\227\123\302\332\350\164\323\124\073\052\002\211\372\231\076\246\343\246\051\006\171\237\133\064\052\151\233"
        "\165\023\357\151\015\103\124\070\126\204\125\206\174\120\163\125\362\004\122\363\050\126\114\353\305\323\236\012\143\264\240\001"
        "\374\157\236\074\077\113\300\262\005\272\372\364\262\101\176\266\215\030\062\121\057\164\270\047\247\073\135\336\332\055\034\322"
        "\324\047\005\256\240\171\377\006\176\222\030\174\047\034\312\127\243\074\060\346\247\014\233\073\064\160\127\136\271\341\306\377"
        "\123\352\356\315\071\210\050\061\011\344\243\122\121\167\067\150\025\116\166\246\301\027\155\300\002\347\030\242\300\364\333\040"
        "\242\250\365\223\020\150\010\241\322\107\377\251\346\041\173\073\370\215\151\107\211\057\247\326\247\131\273\352\327\227\041\100"
        "\012\176\363\173\022\160\006\127\053\330\264\006\172\036\022\134\164\344\047\020\354\245\210\154\151\212\322\324\000\023\203\250"
        "\141\310\042\342\243\311\007\117\325\260\016\132\145\074\021\153\303\004\247\305\075\333\061\251\015\235\242\117\344\144\250\245"
        "\030\220\316\054\306\103\335\242\320\245\103\065\215\354\176\151\111\331\225\102\030\221\041\112\242\210\323\221\317\364\120\342"
        "\246\166\055\251\154\002\104\235\233\336\027\066\310\320\153\022\203\052\142\026\117\305\221\276\101\347\164\054\134\036\232\316"
        "\023\306\061\062\311\141\030\162\340\014\145\210\206\055\050\361\044\264\241\030\314\334\240\302\041\064\152\202\051\104\321\013"
        "\155\150\026\344\210\120\166\022\341\332\163\224\242\075\301\024\260\015\007\134\037\000\013\070\107\365\305\117\040\077\044\377"
        "\315\003\303\264\053\110\222\004\115\206\134\144\033\224\220\103\325\171\357\213\217\134\240\307\044\251\106\052\246\005\200\024"
        "\224\022\157\304\310\252\106\151\020\070\156\114\313\333\050\162\305\064\320\144\226\030\271\345\044\052\102\023\114\310\204\203"
        "\147\031\117\335\244\224\270\273\261\257\154\060\214\116\032\163\142\313\017\022\221\145\020\121\015\371\102\103\064\335\025\350"
        "\204\072\204\231\310\052\151\026\207\034\321\217\337\044\342\026\241\051\067\130\226\204\057\116\324\144\016\061\031\077\367\025"
        "\245\175\071\234\343\034\112\211\226\224\321\122\213\264\024\022\046\170\111\021\175\142\044\202\136\014\231\336\360\030\230\167"
        "\142\257\057\330\273\136\000\335\160\306\347\040\014\204\030\161\003\115\042\072\102\031\224\223\044\107\111\044\031\177\343\102"
        "\333\331\256\105\321\271\116\046\370\311\117\135\112\342\015\313\264\223\014\366\370\023\201\154\322\015\360\043\140\321\260\111"
        "\062\203\156\323\052\371\273\147\110\205\230\054\233\056\044\115\350\244\237\310\054\107\263\115\322\154\112\176\223\050\076\377"
        "\125\006\267\171\236\345\224\035\145\047\012\063\231\230\047\271\241\247\152\051\251\110\115\232\021\255\166\325\231\315\034\047"
        "\076\111\030\040\221\221\310\050\213\114\037\043\361\030\262\365\005\362\071\107\264\047\064\303\212\113\043\142\204\046\167\155"
        "\116\164\312\266\033\102\136\157\207\051\154\335\132\117\171\325\224\074\157\227\365\102\251\067\323\220\011\376\235\044\067\210"
        "\134\324\035\071\111\040\246\364\125\050\216\211\235\134\017\173\305\223\162\065\204\235\215\250\111\353\232\021\031\204\262\174"
        "\363\043\131\201\010\071\323\127\015\045\175\025\125\317\150\301\071\132\064\144\342\124\047\115\103\057\045\241\125\042\066\326"
        "\044\341\201\347\000\123\267\121\143\372\160\245\077\211\010\156\105\233\334\115\041\247\042\115\275\141\372\166\063\135\012\266"
        "\056\223\211\044\031\141\121\222\041\336\062\263\226\166\245\153\070\047\321\032\256\372\326\234\305\315\146\124\261\313\303\164"
        "\236\247\122\321\321\107\056\275\313\314\175\206\025\202\217\001\150\043\327\032\062\213\121\123\272\322\175\145\354\146\062\377"
        "\321\114\100\104\267\235\235\211\170\305\031\021\220\130\164\044\343\161\337\041\237\122\324\334\130\355\216\006\165\244\064\103"
        "\043\333\315\312\025\267\364\025\247\126\205\167\334\224\254\357\131\215\072\040\244\024\031\341\036\026\110\273\047\221\114\111"
        "\145\074\113\373\032\161\242\045\245\210\155\047\301\330\027\033\244\051\030\136\336\242\140\372\124\364\105\161\121\357\205\316"
        "\062\302\351\325\335\142\104\267\265\324\154\057\047\021\132\147\242\341\006\372\362\242\132\141\032\131\167\255\255\220\157\112"
        "\012\103\035\067\145\157\036\121\237\316\074\163\077\361\151\336\213\026\356\164\151\075\033\121\356\170\302\072\036\025\072\372"
        "\260\347\202\347\212\117\035\373\161\227\032\116\013\047\221\262\076\357\244\065\250\204\046\120\001\323\327\343\077\101\131\227"
        "\122\336\052\056\165\113\254\271\146\042\211\147\341\013\375\322\227\126\003\252\017\265\154\125\035\243\113\262\021\065\177\365"
        "\256\021\315\155\056\211\210\352\136\066\314\264\241\041\164\120\345\034\262\166\075\111\221\152\302\201\203\157\112\351\210\377"
        "\350\030\307\223\230\064\135\145\314\231\066\002\123\135\236\116\112\120\117\227\320\144\043\005\272\174\264\245\232\131\115\143"
        "\152\067\171\330\127\164\353\150\134\133\266\123\376\367\323\160\116\141\002\037\251\353\237\070\064\325\234\305\353\170\037\362"
        "\006\174\076\232\253\116\256\045\233\107\302\252\103\343\356\152\353\063\050\375\030\031\341\154\363\110\274\246\006\157\272\043"
        "\232\156\315\306\000\150\000\335\341\211\151\355\052\116\346\333\165\045\034\363\300\375\210\152\150\052\225\314\377\266\161\203"
        "\003\004\140\102\243\156\275\213\272\146\310\160\026\035\150\206\366\321\137\135\254\215\307\153\140\222\346\226\046\317\146\051"
        "\166\047\270\336\365\016\371\323\157\022\365\110\300\245\232\114\020\034\264\064\351\363\054\237\074\011\137\221\133\041\162\044"
        "\040\172\313\126\124\354\016\220\220\060\377\311\251\116\156\304\124\357\166\317\104\364\356\070\207\047\101\366\252\016\132\276"
        "\071\137\374\236\042\346\321\254\205\266\223\370\260\220\332\115\304\264\343\126\172\306\116\372\176\033\251\326\047\102\125\377"
        "\217\061\224\162\170\215\130\351\175\172\266\266\232\005\064\132\044\246\126\354\311\257\310\306\074\274\242\253\273\250\234\063"
        "\104\031\007\156\067\313\343\372\341\033\213\163\307\362\146\310\272\332\365\316\145\343\321\341\200\115\244\074\137\015\031\172"
        "\144\365\266\070\076\363\150\225\151\372\214\374\363\071\233\016\173\354\145\177\310\221\045\062\327\261\233\061\104\174\275\125"
        "\321\312\370\256\027\127\315\057\367\122\344\320\173\072\243\001\115\335\263\240\256\022\123\023\261\367\253\056\151\147\161\353"
        "\153\360\042\263\170\052\306\143\342\121\013\173\126\016\024\122\215\137\310\333\164\053\142\017\361\374\266\036\066\042\346\027"
        "\022\362\117\353\316\304\351\203\037\354\361\330\326\203\077\007\164\177\167\371\054\271\212\365\201\003\134\307\271\225\017\022"
        "\103\211\214\332\241\175\303\041\107\002\240\303\173\270\330\311\045\003\163\067\062\333\055\031\143\067\077\232\062\061\260\300"
        "\263\230\037\214\122\300\217\173\252\256\003\245\113\241\272\222\033\257\033\153\040\374\333\055\123\203\210\300\053\235\332\377"
        "\223\273\165\132\053\354\163\044\253\062\272\302\002\266\136\323\075\315\272\253\315\302\010\035\363\065\365\123\210\220\063\062"
        "\252\101\240\201\222\277\247\303\262\347\260\222\132\112\275\041\052\260\305\242\057\227\263\253\201\233\250\255\203\216\115\043"
        "\276\047\002\040\172\303\250\060\072\045\262\343\021\135\122\267\060\344\275\252\113\071\047\234\211\123\153\275\031\104\010\172"
        "\123\053\330\133\247\123\252\046\333\121\301\263\370\056\167\333\101\200\333\261\023\244\272\035\013\070\135\232\103\253\220\031"
        "\144\023\054\332\003\265\156\013\060\064\222\204\151\073\101\316\062\242\074\274\266\010\254\055\240\363\301\204\330\015\145\063"
        "\061\375\112\072\067\024\210\370\113\241\033\030\275\264\240\207\061\264\103\076\044\305\135\022\257\336\323\010\327\023\216\246"
        "\303\060\074\302\050\056\003\254\310\172\226\333\103\043\107\253\045\361\302\061\210\162\067\166\013\270\213\220\001\210\173\031"
        "\120\373\037\107\032\064\222\351\100\353\173\216\045\013\301\153\353\054\051\073\051\333\152\102\317\072\051\056\020\300\223\377"
        "\120\047\131\024\031\070\333\062\142\164\044\360\023\226\046\034\077\225\133\256\063\273\042\061\374\256\046\354\271\111\104\210"
        "\166\242\251\116\213\037\015\244\067\102\364\104\244\232\300\374\073\263\002\263\247\177\273\253\271\132\276\001\334\057\103\102"
        "\300\067\053\220\302\253\304\105\361\102\347\331\307\131\252\303\060\224\262\201\373\073\166\123\260\142\033\221\106\202\275\055"
        "\353\300\133\003\310\213\001\301\034\134\106\134\202\100\036\154\304\210\172\306\275\353\267\151\232\275\113\234\277\316\043\306"
        "\010\363\306\204\240\227\236\253\055\047\014\255\236\213\050\352\033\301\361\162\064\150\314\010\106\073\041\255\341\106\370\203"
        "\105\261\203\075\314\202\016\320\161\271\210\352\045\175\352\054\137\133\054\050\143\256\225\273\305\326\303\257\327\363\274\252"
        "\134\253\126\324\304\240\334\032\065\074\011\263\113\071\274\022\261\050\044\105\256\142\271\020\124\307\203\330\276\224\274\065"
        "\065\071\245\155\164\261\152\274\212\075\254\270\136\123\306\321\222\062\232\370\071\111\050\111\310\110\055\114\164\247\154\377"
        "\044\304\302\073\304\206\052\103\017\053\107\273\352\054\064\033\042\234\334\255\113\343\216\276\044\304\053\164\101\166\002\040"
        "\044\173\016\074\163\271\061\014\101\221\242\045\235\354\065\273\272\301\210\120\305\321\160\303\042\343\304\200\244\114\310\074"
        "\110\374\351\114\164\203\046\166\233\264\237\323\245\236\223\115\142\343\112\203\050\110\116\324\236\265\244\265\224\104\300\077"
        "\374\211\145\300\307\132\042\113\221\132\102\137\163\067\045\054\103\165\333\313\100\363\272\220\161\313\266\114\111\116\154\111"
        "\204\130\006\110\233\100\167\333\114\173\042\113\134\254\311\263\364\061\310\374\107\311\024\306\107\072\302\267\172\106\273\262"
        "\105\075\124\316\106\214\104\305\024\115\270\054\011\115\054\315\106\262\110\310\034\002\132\274\314\361\302\116\173\242\062\317"
        "\202\264\074\204\315\060\004\317\175\320\100\372\034\104\271\163\244\155\134\264\117\114\213\141\220\100\253\033\120\227\213\106"
        "\235\334\242\150\304\204\116\271\315\222\120\320\102\004\265\351\364\315\352\104\010\014\165\312\341\054\260\354\044\242\377\377"
        "\153\104\266\013\203\137\134\301\336\344\044\006\235\277\170\254\314\372\153\050\164\204\246\346\003\072\315\072\321\162\324\061"
        "\117\334\120\222\010\075\240\224\077\020\125\320\156\223\110\350\040\206\345\204\264\257\144\073\366\314\303\305\212\104\177\272"
        "\201\111\060\066\004\055\122\305\213\321\026\264\263\350\350\271\035\105\073\043\032\120\025\015\123\250\344\101\061\120\051\330"
        "\361\042\103\374\120\267\364\270\027\134\037\021\035\121\247\004\266\062\115\321\076\204\074\252\323\247\321\071\231\211\374\320"
        "\004\015\310\156\123\300\171\254\017\062\333\321\345\334\301\010\034\270\316\142\114\053\043\010\172\320\053\337\304\043\031\125"
        "\313\177\024\054\375\364\240\236\103\316\147\374\312\147\004\103\063\245\272\321\011\322\334\324\057\231\343\106\111\215\037\144"
        "\104\304\345\224\253\050\345\310\035\113\265\075\255\041\053\325\253\247\272\265\373\334\104\255\214\323\040\065\211\307\011\311"
        "\270\132\110\167\353\045\164\214\210\237\271\001\142\020\074\113\145\261\026\214\060\103\214\123\007\165\236\057\165\071\330\227"
        "\224\322\035\343\303\002\023\061\346\024\034\136\025\322\134\375\270\004\305\310\270\213\237\044\245\017\142\020\051\137\353\271"
        "\141\275\326\076\004\115\064\010\011\034\240\125\270\033\304\103\373\262\123\115\311\042\353\307\233\112\327\365\034\057\362\102"
        "\103\167\335\202\236\350\323\110\135\061\337\344\100\113\115\111\071\025\226\106\134\256\303\134\071\343\014\323\163\005\211\200"
        "\235\112\014\004\124\203\105\115\240\304\104\205\125\210\044\254\101\154\245\120\001\165\325\321\351\211\172\260\017\123\105\130"
        "\102\224\316\123\035\002\216\115\010\160\321\047\376\164\102\150\344\327\214\210\027\301\031\377\203\320\150\227\041\343\322\043"
        "\105\115\074\312\327\263\140\254\162\044\116\062\124\052\236\060\326\106\060\030\373\140\331\067\035\117\214\015\121\156\105\011"
        "\175\330\212\230\154\067\231\325\324\235\020\034\056\250\122\321\270\271\160\315\076\053\074\330\070\225\117\263\120\206\134\343"
        "\014\253\033\057\267\263\100\252\040\013\061\100\326\013\021\202\237\245\071\114\034\104\026\133\115\372\140\054\013\124\127\022"
        "\154\133\252\320\320\201\205\014\117\250\333\265\074\330\111\215\054\340\164\234\266\055\055\034\170\133\307\160\333\137\324\204"
        "\056\131\311\254\344\131\224\255\133\227\035\011\306\212\001\262\350\211\317\055\255\033\010\011\312\020\134\223\364\315\146\045"
        "\306\303\015\121\342\351\023\150\060\327\266\035\135\013\314\004\115\110\032\057\101\331\122\115\335\172\245\071\341\363\223\150"
        "\130\322\266\355\334\134\023\203\114\210\133\350\170\105\212\124\055\334\235\327\106\012\132\157\241\217\042\205\132\303\315\335"
        "\265\322\134\347\125\010\365\141\203\364\311\336\024\332\336\066\230\133\356\305\243\355\145\377\003\111\045\124\353\355\226\361"
        "\315\112\371\053\134\160\035\127\251\065\337\223\070\136\350\315\130\107\272\302\066\330\336\041\120\334\367\245\017\301\102\137"
        "\361\015\337\364\101\202\370\031\137\373\065\244\357\135\064\367\325\137\222\300\335\115\053\122\236\005\127\226\204\326\004\266"
        "\017\003\316\125\157\375\311\371\025\054\336\225\340\350\070\336\271\165\237\001\366\136\360\035\202\376\135\253\271\115\244\374"
        "\335\140\155\023\341\351\135\137\366\035\314\024\056\236\101\244\140\123\135\257\031\066\117\030\356\223\032\026\140\360\205\037"
        "\023\036\137\364\041\341\021\306\243\274\305\341\053\105\337\101\014\140\100\105\120\002\356\322\042\226\333\112\155\141\214\315"
        "\312\352\115\340\200\074\142\376\345\336\364\371\336\345\115\241\221\163\342\013\011\337\022\366\315\043\136\037\023\136\140\015"
        "\376\342\264\240\203\160\245\141\212\334\336\357\245\340\346\115\343\022\313\335\104\102\137\330\273\137\004\044\341\027\236\343"
        "\025\034\141\073\136\341\350\315\335\354\365\342\076\216\216\373\355\136\045\356\136\355\325\377\342\171\045\127\103\076\023\107"
        "\312\143\371\203\143\356\115\274\112\155\116\110\066\245\324\175\143\074\062\143\056\066\125\052\326\137\272\355\337\365\315\336"
        "\361\235\333\105\106\141\115\076\011\361\255\333\105\316\342\043\106\344\054\106\143\126\106\211\335\060\140\033\316\342\020\256"
        "\124\061\056\137\133\026\015\115\254\144\031\306\340\042\025\345\367\245\133\201\360\141\231\203\345\051\056\133\140\126\210\111"
        "\206\142\130\336\345\077\146\342\364\041\142\150\376\101\041\156\144\153\146\141\001\146\344\153\136\145\155\136\010\061\126\346"
        "\035\146\346\062\146\337\143\066\137\111\106\320\145\316\143\050\046\140\321\045\347\320\360\341\024\262\143\330\113\145\314\355"
        "\144\154\106\340\057\176\012\074\126\347\127\266\146\127\306\243\161\256\147\204\310\312\170\156\343\112\236\342\177\166\142\117"
        "\116\111\012\066\143\063\156\350\116\174\346\204\356\347\173\376\343\116\156\346\163\016\341\041\250\345\204\126\210\116\300\056"
        "\130\066\340\131\356\146\124\316\260\010\046\351\221\120\142\171\236\350\135\116\130\210\056\377\142\241\260\350\220\376\146\153"
        "\246\317\354\225\143\222\236\133\070\136\151\021\356\147\075\346\146\327\261\151\034\236\277\123\136\352\225\356\146\046\056\152"
        "\204\176\351\364\025\343\254\254\344\031\036\137\166\266\136\067\270\342\106\226\150\201\066\352\324\245\347\227\066\213\116\336"
        "\147\235\076\345\247\166\303\111\316\146\222\136\343\220\316\343\234\106\144\257\166\152\251\046\351\243\336\064\151\106\131\216"
        "\336\145\255\166\336\100\325\351\020\146\150\235\066\343\355\375\351\204\316\153\365\145\351\216\256\346\153\046\343\107\046\353"
        "\065\044\141\212\256\324\240\376\152\235\306\337\214\256\347\300\266\354\304\276\154\236\216\153\076\216\154\264\124\353\264\206"
        "\153\252\306\056\023\376\345\321\356\325\317\246\151\236\276\346\231\336\264\266\116\150\117\260\350\301\256\350\130\356\150\330"
        "\166\035\326\116\011\332\310\155\330\276\355\106\266\354\302\036\202\063\360\153\157\371\204\306\136\146\164\216\155\141\016\141"
        "\227\366\155\145\000\153\156\316\143\275\346\344\115\103\356\244\206\141\145\130\144\203\176\377\152\271\146\344\146\226\004\337"
        "\056\011\150\010\341\340\346\137\201\066\355\316\016\341\036\210\201\170\065\357\175\100\157\330\366\354\360\046\354\252\336\336"
        "\322\235\157\203\240\207\200\256\346\264\236\153\203\275\356\115\213\027\065\355\357\175\130\352\364\156\352\306\026\156\007\347"
        "\157\363\216\206\163\006\355\312\226\350\346\126\353\051\051\336\376\136\356\234\006\137\033\236\144\275\066\355\117\110\360\175"
        "\110\006\174\176\355\135\246\340\020\057\355\021\206\157\061\130\332\004\247\007\317\166\157\342\226\150\203\256\161\145\066\031"
        "\363\206\006\114\030\202\335\276\360\025\266\157\062\166\356\314\346\356\042\016\153\247\006\147\013\377\337\111\376\204\332\045"
        "\353\345\116\320\234\326\347\246\076\342\342\376\152\252\100\360\022\117\006\374\156\361\006\067\343\036\220\346\263\376\204\010"
        "\267\345\345\116\154\323\116\037\036\340\336\063\060\341\063\370\143\070\117\041\063\320\356\115\243\363\161\213\001\063\067\157"
        "\301\072\203\054\356\363\067\027\142\041\220\163\365\316\161\330\363\004\075\117\143\145\240\377\215\100\147\362\322\346\145\361"
        "\216\355\271\005\334\022\117\210\016\147\360\236\136\353\024\072\203\356\375\144\106\077\353\364\051\163\103\126\006\051\267\157"
        "\270\216\345\006\207\156\014\017\351\063\020\234\141\240\364\204\010\124\226\246\354\167\166\155\350\266\346\103\217\161\011\326"
        "\007\105\007\160\317\346\164\013\127\347\102\377\154\066\030\236\043\117\164\153\246\362\000\337\155\225\326\355\113\047\362\365"
        "\371\204\144\100\164\057\341\027\105\371\361\354\305\162\320\256\362\007\137\353\361\106\353\105\043\213\055\177\165\203\100\363"
        "\122\126\163\046\006\166\067\347\154\013\237\144\000\302\204\150\237\166\036\341\227\117\300\204\371\141\166\171\066\143\066\117"
        "\037\061\367\362\053\246\362\373\145\165\114\035\367\203\220\147\040\037\352\224\126\354\114\377\367\335\146\340\265\162\203\167"
        "\207\167\200\331\165\372\256\007\203\251\370\157\371\226\135\347\227\144\370\204\114\360\204\172\307\136\057\127\361\017\157\203"
        "\036\120\313\256\146\342\341\226\155\357\105\156\252\210\367\376\326\207\054\066\003\074\222\055\363\065\107\165\331\276\163\106"
        "\257\162\012\266\362\300\376\143\157\356\100\344\165\152\144\257\365\320\336\367\046\307\153\323\066\171\074\322\367\071\267\146"
        "\070\067\354\103\221\377\357\201\137\211\200\264\154\243\177\157\323\036\360\156\237\151\141\177\352\312\376\161\174\246\153\151"
        "\076\171\100\276\155\244\307\356\234\316\166\007\347\345\314\116\156\047\266\364\144\337\347\076\367\354\231\166\172\164\327\171"
        "\337\304\362\334\136\173\006\157\160\006\137\373\122\307\372\052\027\360\014\257\156\002\216\353\170\161\334\266\241\372\221\100"
        "\363\257\036\363\266\077\355\151\036\202\234\057\171\301\056\365\373\046\360\156\337\172\171\236\153\065\067\372\147\237\362\107"
        "\047\371\140\057\171\031\040\213\227\037\370\114\220\374\176\146\161\211\006\174\314\276\364\223\047\352\033\267\161\257\126\171"
        "\242\267\156\315\067\371\012\247\361\221\147\163\265\007\166\240\245\012\034\330\360\306\337\334\336\217\147\341\367\354\336\137"
        "\336\041\327\375\310\327\374\335\377\337\260\276\354\117\166\350\115\077\172\033\116\037\313\207\075\176\267\362\052\367\304\200"
        "\165\165\344\077\011\326\067\141\357\147\151\077\157\366\316\226\345\243\326\164\341\317\375\347\036\166\240\007\175\315\307\360"
        "\320\016\376\360\137\377\177\027\007\010\066\103\332\364\150\063\104\040\301\060\061\160\154\121\266\357\041\304\210\022\047\122"
        "\254\150\361\042\306\214\032\067\112\324\024\306\240\101\201\154\012\016\074\130\262\215\020\220\007\011\206\054\211\360\144\312"
        "\224\003\133\252\154\203\220\346\300\230\050\125\276\004\051\263\046\110\203\147\114\006\205\271\363\244\317\242\074\221\346\154"
        "\043\364\006\303\033\016\071\122\255\152\365\052\326\211\312\206\262\251\051\163\350\116\231\075\177\236\371\051\366\244\311\233"
        "\072\135\032\105\353\364\347\112\270\042\323\322\115\252\363\146\323\265\157\151\232\015\353\364\340\215\030\201\305\104\313\152"
        "\370\060\342\304\222\172\260\335\233\122\051\316\243\073\173\040\044\331\325\150\301\244\056\101\212\344\153\323\055\334\277\054"
        "\117\366\164\113\264\046\321\320\247\321\236\271\161\103\341\244\304\262\147\323\316\230\051\314\112\242\154\347\036\275\034\071"
        "\244\344\224\140\351\202\025\062\334\114\111\256\103\206\227\105\171\006\141\163\204\072\375\346\226\036\034\270\352\256\217\363"
        "\072\167\035\203\013\377\016\142\265\307\223\037\257\114\314\015\320\250\361\152\356\215\366\045\157\243\324\333\237\135\177\222"
        "\307\122\324\305\275\022\054\035\226\271\156\117\335\300\305\015\150\114\125\036\202\011\032\066\314\015\134\271\307\337\152\276"
        "\325\327\330\114\167\251\027\032\147\115\005\050\127\133\163\101\267\235\127\315\025\025\237\163\075\050\064\230\170\012\252\270"
        "\242\125\223\334\320\303\160\177\231\061\331\130\100\031\005\142\200\215\201\210\235\144\115\265\027\341\100\043\211\206\232\137"
        "\376\351\265\231\211\202\311\160\103\154\054\072\371\144\106\347\065\170\044\177\370\321\325\331\100\373\365\265\141\113\376\371"
        "\330\130\144\252\121\347\241\151\332\331\127\131\140\120\155\061\311\201\120\272\371\146\104\312\214\021\106\203\032\142\051\132"
        "\162\357\005\330\133\176\104\352\324\327\131\170\111\370\131\215\156\121\026\031\167\131\366\260\305\140\061\114\002\015\234\221"
        "\112\372\220\062\150\274\146\331\122\130\132\027\132\112\044\211\006\342\132\075\345\147\332\174\236\122\110\050\116\233\266\001"
        "\226\200\112\206\301\377\346\244\261\112\272\314\044\164\206\301\130\217\374\215\006\334\225\103\040\047\324\127\046\235\261\223"
        "\225\154\034\027\222\162\303\165\126\137\230\077\342\071\022\235\202\061\052\106\046\205\311\172\255\244\232\240\301\150\235\252"
        "\356\226\153\231\104\025\252\131\241\326\221\372\153\247\221\301\327\134\247\255\245\271\005\016\151\244\210\055\275\222\326\223"
        "\211\030\202\335\072\024\253\142\246\345\236\227\237\215\032\224\230\031\232\013\141\177\244\232\230\146\014\061\210\041\206\046"
        "\364\324\073\361\265\370\006\266\257\225\002\247\224\061\151\063\305\225\352\116\135\352\106\246\221\042\373\126\043\017\356\052"
        "\171\003\274\214\150\242\017\305\061\143\253\014\061\150\344\353\332\213\147\160\274\027\227\024\226\306\236\206\112\345\044\042"
        "\251\146\235\261\160\264\067\344\373\152\046\062\073\115\061\315\142\050\204\303\153\015\222\244\134\217\343\162\312\353\136\017"
        "\146\312\353\321\356\236\270\244\014\016\113\222\311\062\117\253\355\064\075\231\114\042\165\140\202\271\266\057\214\276\166\074"
        "\144\135\034\076\230\175\350\321\013\173\167\261\253\142\114\102\114\233\153\033\356\164\064\304\014\363\066\334\202\351\073\267"
        "\153\046\366\320\203\031\165\037\315\122\101\370\131\016\043\347\164\126\315\062\340\215\206\201\303\303\231\020\176\070\352\251"
        "\113\104\263\333\017\217\256\257\334\214\072\356\232\354\047\312\075\373\354\236\207\101\366\350\017\117\142\172\341\252\013\077"
        "\174\104\365\050\103\263\046\156\117\142\363\303\017\053\175\303\222\004\022\330\074\365\223\244\361\273\046\303\034\117\074\367"
        "\020\005\004\000\073");
        defaultValue.setByteString(tmpImageGIF, OpcUa_False);
    }
    pImageGIF->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImageGIF, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImageGIF;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageJPG(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImageJPG = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageJPG, getTypeNamespace()));
    pImageJPG->setBrowseName(UaQualifiedName("ImageJPG", getTypeNamespace()));
    pImageJPG->setDisplayName(UaLocalizedText("", "ImageJPG"));
    pImageJPG->setAccessLevel(3);
    pImageJPG->setUserAccessLevel(3);
    pImageJPG->setValueRank(-1);
    pImageJPG->setDataTypeId(OpcUaId_ImageJPG);
    {
        UaByteString tmpImageJPG = UaByteString(11105, (OpcUa_Byte*) "\377\330\377\340\000\020\112\106\111\106\000\001\001\001\000\140\000\140\000\000\377\333\000\103\000\002\001\001\002\001\001\002"
        "\002\002\002\002\002\002\002\003\005\003\003\003\003\003\006\004\004\003\005\007\006\007\007\007\006\007\007\010\011\013\011\010"
        "\010\012\010\007\007\012\015\012\012\013\014\014\014\014\007\011\016\017\015\014\016\013\014\014\014\377\333\000\103\001\002\002"
        "\002\003\003\003\006\003\003\006\014\010\007\010\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014"
        "\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\014\377\300"
        "\000\021\010\001\000\001\000\003\001\042\000\002\021\001\003\021\001\377\304\000\037\000\000\001\005\001\001\001\001\001\001\000"
        "\000\000\000\000\000\000\000\001\002\003\004\005\006\007\010\011\012\013\377\304\000\265\020\000\002\001\003\003\002\004\003\005"
        "\005\004\004\000\000\001\175\001\002\003\000\004\021\005\022\041\061\101\006\023\121\141\007\042\161\024\062\201\221\241\010\043"
        "\102\261\301\025\122\321\360\044\063\142\162\202\011\012\026\027\030\031\032\045\046\047\050\051\052\064\065\066\067\070\071\072"
        "\103\104\105\106\107\110\111\112\123\124\125\126\127\130\131\132\143\144\145\146\147\150\151\152\163\164\165\166\167\170\171\172"
        "\203\204\205\206\207\210\211\212\222\223\224\225\226\227\230\231\232\242\243\244\245\246\247\250\251\252\262\263\264\265\266\267"
        "\270\271\272\302\303\304\305\306\307\310\311\312\322\323\324\325\326\327\330\331\332\341\342\343\344\345\346\347\350\351\352\361"
        "\362\363\364\365\366\367\370\371\372\377\304\000\037\001\000\003\001\001\001\001\001\001\001\001\001\000\000\000\000\000\000\001"
        "\002\003\004\005\006\007\010\011\012\013\377\304\000\265\021\000\002\001\002\004\004\003\004\007\005\004\004\000\001\002\167\000"
        "\001\002\003\021\004\005\041\061\006\022\101\121\007\141\161\023\042\062\201\010\024\102\221\241\261\301\011\043\063\122\360\025"
        "\142\162\321\012\026\044\064\341\045\361\027\030\031\032\046\047\050\051\052\065\066\067\070\071\072\103\104\105\106\107\110\111"
        "\112\123\124\125\126\127\130\131\132\143\144\145\146\147\150\151\152\163\164\165\166\167\170\171\172\202\203\204\205\206\207\210"
        "\211\212\222\223\224\225\226\227\230\231\232\242\243\244\245\246\247\250\251\252\262\263\264\265\266\267\270\271\272\302\303\304"
        "\305\306\307\310\311\312\322\323\324\325\326\327\330\331\332\342\343\344\345\346\347\350\351\352\362\363\364\365\366\367\370\371"
        "\372\377\332\000\014\003\001\000\002\021\003\021\000\077\000\375\374\242\212\050\000\242\212\050\000\242\212\050\000\242\212\050"
        "\000\242\212\050\000\242\212\305\370\203\361\033\101\370\121\341\113\255\163\304\232\265\216\211\244\131\256\351\256\256\345\021"
        "\306\276\203\047\253\036\312\062\111\340\003\100\033\125\237\342\177\026\151\176\011\321\245\324\165\235\112\303\111\323\340\031"
        "\222\346\362\341\040\206\077\253\261\000\176\046\277\071\077\152\337\370\056\314\206\153\255\037\341\046\222\252\212\114\177\333"
        "\372\254\131\055\376\324\066\375\275\103\112\117\272\012\370\037\342\347\307\237\031\174\171\327\177\264\274\141\342\135\137\304"
        "\067\131\045\015\334\345\243\207\075\104\161\375\310\307\262\200\050\003\365\347\343\027\374\026\103\340\237\302\231\146\267\263"
        "\326\065\017\030\136\305\220\143\321\055\174\310\263\377\000\135\244\051\031\036\352\315\137\064\174\112\377\000\203\200\274\101"
        "\172\362\107\341\017\207\372\116\236\234\204\237\126\275\222\355\230\172\230\343\021\205\076\333\232\277\074\050\240\017\247\274"
        "\153\377\000\005\205\370\365\343\023\042\307\342\313\135\022\031\077\345\236\233\246\101\036\337\243\072\263\217\373\352\274\277"
        "\304\277\266\247\305\357\027\156\376\320\370\231\343\211\225\376\362\056\263\074\121\237\370\012\060\037\245\171\215\024\001\271"
        "\253\174\115\361\046\275\237\267\170\203\134\274\335\327\317\277\226\114\377\000\337\114\153\036\173\331\256\217\357\046\222\117"
        "\367\230\265\107\105\000\111\015\334\326\307\061\311\044\147\375\226\042\265\264\257\210\376\042\320\216\154\165\355\152\314\216"
        "\206\013\331\143\307\344\302\261\150\240\017\112\360\337\355\223\361\153\302\033\177\263\376\045\170\342\335\127\242\177\155\134"
        "\074\143\376\002\314\127\364\257\116\360\137\374\025\347\343\347\203\166\053\170\312\075\142\024\377\000\226\172\216\235\157\066"
        "\357\253\204\017\377\000\217\127\314\364\120\007\350\067\303\177\370\057\377\000\212\264\326\216\077\026\170\017\102\325\243\340"
        "\074\272\135\334\266\057\217\135\257\346\202\175\262\007\322\276\225\370\077\377\000\005\243\370\055\361\066\130\155\365\113\355"
        "\133\301\267\222\140\155\325\355\077\161\273\332\130\213\250\036\357\267\360\257\306\112\050\003\372\072\360\157\216\364\117\210"
        "\272\044\172\227\207\365\215\063\134\323\345\373\227\066\027\111\161\023\177\300\220\221\370\126\265\177\072\177\014\176\060\370"
        "\253\340\277\210\027\125\360\237\210\065\157\017\352\013\214\313\143\162\321\171\200\166\160\016\035\177\331\140\107\265\175\331"
        "\373\053\177\301\166\065\155\026\113\135\047\342\306\222\272\265\256\102\035\163\113\215\143\271\214\177\172\130\070\107\367\051"
        "\260\200\076\353\032\000\375\100\242\271\277\205\077\030\074\063\361\307\301\326\372\377\000\204\365\253\035\167\111\271\341\147"
        "\266\223\166\306\352\121\327\357\043\214\362\254\001\035\305\164\224\000\121\105\024\000\121\105\024\000\121\105\024\000\121\105"
        "\024\000\121\105\024\000\121\105\126\326\165\213\137\016\350\367\132\205\364\361\332\331\130\302\367\027\023\110\166\244\061\242"
        "\226\146\047\260\000\022\176\224\001\347\077\265\237\355\143\341\177\330\373\341\134\376\046\361\044\315\044\216\114\072\166\237"
        "\023\017\264\152\123\343\042\064\317\100\072\263\036\024\163\311\300\077\212\337\265\227\355\233\343\157\333\023\306\315\252\170"
        "\242\371\223\117\267\166\072\176\223\156\305\154\364\365\077\335\137\342\162\072\273\145\217\260\300\027\277\156\277\332\343\122"
        "\375\261\076\074\352\036\041\236\111\243\320\354\331\255\064\073\046\070\133\133\120\170\044\177\317\107\373\354\175\116\072\050"
        "\307\214\320\001\105\024\120\001\105\025\203\253\374\111\322\364\255\101\054\322\111\057\265\011\134\107\035\245\234\146\151\235"
        "\217\105\000\167\366\353\100\033\324\127\246\374\037\377\000\202\171\176\322\237\264\024\120\334\170\177\341\075\366\201\246\315"
        "\312\337\370\242\341\164\305\301\350\336\124\205\146\043\335\121\253\350\137\004\177\301\274\177\030\374\103\034\155\342\217\213"
        "\036\011\360\336\356\136\075\033\112\233\122\051\355\231\174\232\000\370\266\212\375\044\360\377\000\374\033\161\341\321\032\377"
        "\000\156\174\150\370\201\174\330\371\316\237\151\155\142\017\323\042\114\176\265\325\131\377\000\301\271\137\007\142\215\105\307"
        "\216\376\063\134\060\352\177\267\255\320\037\300\133\120\007\345\215\025\372\262\377\000\360\156\277\301\103\030\124\361\167\306"
        "\050\375\327\304\120\363\371\333\221\131\332\257\374\033\237\360\306\110\277\342\135\361\043\342\325\224\203\241\232\376\316\345"
        "\177\020\155\207\363\240\017\313\172\053\364\123\305\137\360\156\225\344\120\226\360\357\306\313\305\145\373\261\353\036\032\212"
        "\341\133\330\274\122\306\107\327\006\274\223\342\017\374\020\317\343\367\202\041\171\264\231\274\003\343\150\124\174\261\330\352"
        "\062\351\367\157\377\000\000\270\117\053\363\230\120\007\310\324\127\125\361\147\340\067\217\176\000\334\371\176\073\360\117\211"
        "\274\043\363\154\023\337\331\237\261\310\336\211\162\205\240\143\354\256\115\162\240\346\200\012\050\242\200\075\033\366\151\375"
        "\252\174\151\373\047\170\362\075\173\301\372\243\332\263\025\027\166\122\345\355\065\010\301\373\222\307\234\060\353\202\060\313"
        "\223\202\053\366\143\366\042\375\274\074\047\373\153\370\045\256\164\266\376\313\361\046\236\212\165\115\026\151\003\115\152\117"
        "\036\142\036\074\310\211\350\340\161\320\200\170\257\302\032\351\276\016\374\143\361\037\300\037\210\372\137\213\174\047\176\332"
        "\176\267\243\313\346\302\347\046\051\227\370\242\225\177\212\067\031\126\136\340\360\101\000\200\017\350\252\212\363\157\331\043"
        "\366\231\321\177\153\257\200\172\027\216\264\125\373\074\172\234\146\073\313\066\155\322\151\327\161\235\223\333\271\365\107\004"
        "\003\201\271\166\260\341\205\172\115\000\024\121\105\000\024\121\105\000\024\121\105\000\024\121\105\000\025\362\357\374\026\033"
        "\342\304\337\013\177\141\335\176\033\131\032\033\257\024\134\301\241\243\051\301\011\041\057\050\377\000\201\105\034\212\175\232"
        "\276\242\257\206\277\340\276\103\077\262\237\205\177\354\154\207\377\000\110\356\350\003\362\122\212\166\312\066\120\003\151\262"
        "\312\260\306\316\354\025\124\026\142\172\000\052\115\225\217\361\015\161\340\075\143\257\374\171\113\377\000\240\032\000\365\257"
        "\330\063\366\001\361\317\374\024\363\306\227\155\246\336\115\341\037\205\172\055\307\221\251\353\346\074\315\172\343\004\301\156"
        "\274\156\220\251\004\347\012\200\202\331\045\120\376\316\176\310\377\000\360\116\217\204\077\261\066\211\014\036\005\360\215\214"
        "\032\242\306\022\175\156\365\105\326\251\164\173\226\235\206\124\037\356\107\265\075\024\124\077\360\114\117\005\351\176\004\377"
        "\000\202\171\174\027\264\322\054\241\261\267\272\360\166\231\250\314\221\214\171\227\027\066\321\334\117\041\365\147\226\107\143"
        "\356\325\356\324\000\121\105\024\000\121\105\024\000\121\105\024\000\121\105\024\001\016\243\246\333\353\026\023\132\336\133\303"
        "\165\153\160\206\071\141\231\004\221\310\247\250\145\074\020\175\015\174\133\373\141\377\000\301\027\274\007\361\147\110\276\326"
        "\076\034\332\333\370\037\305\012\215\042\131\333\015\232\115\363\365\332\141\351\001\075\003\105\205\031\311\103\326\276\330\242"
        "\200\077\233\337\024\170\147\120\360\127\211\065\015\037\125\265\232\307\123\322\356\044\264\272\267\224\141\340\225\030\253\051"
        "\367\004\021\124\153\337\277\340\250\021\052\376\336\337\022\160\061\377\000\023\010\317\036\366\361\127\201\354\240\006\321\116"
        "\331\106\312\000\373\253\376\015\375\370\361\077\206\077\150\117\035\374\057\270\231\277\263\174\115\245\247\211\354\043\143\362"
        "\305\165\003\055\275\300\121\353\044\117\011\077\365\302\277\131\053\360\247\376\011\031\167\045\247\374\025\163\341\232\106\354"
        "\253\163\245\353\021\310\007\361\250\262\225\260\177\340\112\247\360\257\335\152\000\050\242\212\000\050\242\212\000\050\242\212"
        "\000\050\242\212\000\053\341\337\370\057\130\335\373\052\170\127\376\306\310\177\364\216\356\276\342\257\210\077\340\274\243\077"
        "\262\277\205\277\354\153\207\377\000\110\356\350\003\362\143\313\243\313\253\036\136\151\074\257\245\000\101\345\326\057\304\144"
        "\307\200\065\257\372\362\227\377\000\100\065\320\371\137\112\304\370\222\233\176\037\153\177\365\343\067\376\200\150\003\367\343"
        "\376\011\343\377\000\046\007\360\077\376\304\035\013\377\000\115\360\127\261\127\216\377\000\301\074\177\344\300\376\007\377\000"
        "\330\203\241\177\351\276\012\366\052\000\050\242\212\000\050\242\212\000\050\242\212\000\050\242\212\000\050\242\212\000\374\063"
        "\377\000\202\237\046\177\157\077\211\037\366\020\217\377\000\111\342\257\005\362\353\350\017\370\051\322\347\366\360\370\217\377"
        "\000\137\361\177\351\074\125\340\336\137\322\200\040\362\350\362\352\177\053\351\107\225\364\240\017\147\377\000\202\115\056\337"
        "\370\053\027\302\277\373\007\153\077\372\101\065\176\354\127\341\137\374\022\201\166\377\000\301\130\276\025\177\330\073\131\377"
        "\000\322\011\253\367\122\200\012\206\376\376\015\052\312\153\233\251\241\267\267\267\103\044\262\312\341\022\065\034\222\111\340"
        "\001\352\152\152\370\217\376\012\163\361\353\120\237\305\326\376\003\261\231\355\364\333\110\043\272\324\002\061\123\163\053\362"
        "\210\336\252\253\265\261\320\226\366\025\344\347\131\264\062\354\053\304\315\137\242\135\333\074\374\313\060\216\016\203\255\055"
        "\172\045\335\236\231\361\107\376\012\143\340\277\006\137\113\151\241\331\336\370\236\170\211\123\054\114\055\355\111\036\216\300"
        "\263\175\102\343\320\232\340\355\377\000\340\254\223\175\253\367\276\007\207\311\317\104\325\216\340\077\357\326\017\351\137\034"
        "\357\243\175\176\113\133\215\263\151\317\232\063\121\135\224\125\277\024\337\342\174\015\116\046\307\112\127\214\224\174\222\137"
        "\255\317\322\117\202\337\267\277\201\176\060\152\020\351\362\115\067\207\365\151\316\330\355\365\015\253\034\315\330\044\240\355"
        "\044\366\015\264\223\320\032\366\352\374\155\022\021\137\150\177\301\075\277\155\313\217\032\170\312\017\205\336\050\231\246\325"
        "\277\263\144\276\320\357\134\345\357\041\201\221\146\201\311\352\350\044\215\224\365\145\337\237\271\223\366\074\055\305\325\061"
        "\265\276\251\213\113\235\354\326\227\266\255\065\336\332\351\241\364\131\037\020\117\023\123\352\365\355\315\321\256\266\356\175"
        "\205\105\024\127\337\237\130\025\361\037\374\027\175\167\176\313\076\027\377\000\261\252\037\375\044\273\257\267\053\342\137\370"
        "\056\312\356\375\227\074\057\377\000\143\124\077\372\111\165\100\037\224\136\127\322\217\053\351\123\371\136\324\171\136\324\001"
        "\007\225\364\254\077\211\261\343\341\336\271\323\376\074\146\377\000\320\015\164\176\127\265\141\174\117\217\037\016\165\336\077"
        "\345\302\157\375\000\320\007\357\137\374\023\307\376\114\017\340\177\375\210\072\027\376\233\340\257\142\257\035\377\000\202\170"
        "\377\000\311\201\374\017\377\000\261\007\102\377\000\323\174\025\354\124\000\121\105\024\000\121\105\024\000\121\105\024\000\121"
        "\105\024\000\121\105\024\001\370\177\377\000\005\066\217\077\267\157\304\156\237\361\377\000\037\376\223\305\136\021\345\175\053"
        "\337\177\340\246\121\356\375\272\276\042\377\000\327\374\177\372\117\025\170\117\225\355\100\020\171\137\112\074\257\245\117\345"
        "\173\121\345\173\120\007\257\377\000\301\051\327\157\374\025\217\341\117\375\203\165\237\375\040\232\277\164\153\360\307\376\011"
        "\132\273\177\340\254\237\012\177\354\033\255\177\351\004\325\373\235\100\005\176\153\377\000\301\105\137\037\265\227\210\077\353"
        "\215\247\376\223\107\137\245\025\371\243\377\000\005\031\070\375\255\174\103\377\000\134\055\077\364\232\072\370\137\020\135\262"
        "\330\377\000\215\176\122\076\137\213\277\334\243\376\045\371\063\305\174\317\172\074\317\172\203\161\243\161\257\306\171\217\315"
        "\311\374\317\172\326\375\224\156\244\213\376\012\225\373\075\052\073\052\314\376\042\107\000\375\341\375\217\063\140\376\040\037"
        "\302\260\267\032\327\375\224\016\177\340\251\337\263\277\375\165\361\027\376\231\247\257\247\340\331\177\302\305\037\373\173\377"
        "\000\111\221\355\360\357\374\214\151\374\377\000\364\226\176\304\121\105\025\373\311\372\250\127\305\037\360\135\065\335\373\057"
        "\170\137\376\306\230\277\364\222\352\276\327\257\212\377\000\340\271\253\273\366\140\360\307\375\215\061\177\351\045\325\000\176"
        "\125\171\137\132\074\257\255\117\262\215\224\001\007\225\365\254\037\212\121\343\341\266\275\327\376\074\046\377\000\320\015\164"
        "\273\053\003\342\252\143\341\236\277\377\000\140\371\277\364\003\100\037\273\377\000\360\117\037\371\060\077\201\377\000\366\040"
        "\350\137\372\157\202\275\212\274\167\376\011\343\377\000\046\007\360\077\376\304\035\013\377\000\115\360\127\261\120\001\105\162"
        "\077\033\376\070\370\147\366\167\370\167\171\342\217\025\352\051\247\151\166\170\134\343\164\227\022\034\355\212\065\352\316\330"
        "\070\003\320\223\200\011\037\237\077\023\277\340\275\236\040\233\132\231\074\033\340\075\046\337\116\106\042\051\165\253\211\046"
        "\232\145\354\305\042\050\020\237\100\315\217\123\100\037\246\224\127\345\037\374\077\207\342\257\375\012\176\001\377\000\300\173"
        "\277\376\110\243\376\037\303\361\127\376\205\077\000\377\000\340\075\337\377\000\044\120\007\352\345\025\371\107\377\000\017\341"
        "\370\253\377\000\102\237\200\177\360\036\357\377\000\222\050\377\000\207\360\374\125\377\000\241\117\300\077\370\017\167\377\000"
        "\311\024\001\372\271\105\176\141\374\077\377\000\202\366\170\256\333\130\217\376\022\257\001\350\067\232\173\066\044\072\125\304"
        "\266\263\040\365\002\103\040\142\075\016\063\352\072\327\350\007\354\353\373\110\370\117\366\243\370\167\017\211\074\045\174\156"
        "\254\331\274\273\210\045\135\227\026\062\340\023\024\251\223\265\206\173\022\010\344\022\071\240\016\362\212\050\240\017\304\257"
        "\370\051\164\171\375\271\376\042\165\377\000\217\370\377\000\364\236\052\360\277\053\353\136\365\377\000\005\052\134\376\334\237"
        "\021\077\353\376\077\375\047\212\274\063\145\000\101\345\175\150\362\276\265\076\312\066\120\007\254\177\301\054\327\157\374\025"
        "\227\341\077\375\203\165\257\375\040\232\277\162\253\360\337\376\011\164\273\177\340\254\337\011\377\000\354\033\255\177\351\004"
        "\325\373\221\100\005\176\146\377\000\301\107\233\037\265\307\210\277\353\205\237\376\223\107\137\246\125\371\221\377\000\005\043"
        "\077\361\227\176\042\377\000\256\026\177\372\115\035\174\037\210\177\362\054\217\370\327\345\043\345\170\277\375\312\077\342\137"
        "\223\074\103\175\033\352\035\324\156\257\305\356\176\157\314\115\276\266\277\144\246\317\374\025\073\366\167\377\000\256\276\042"
        "\377\000\323\065\305\163\373\253\163\366\104\071\377\000\202\247\376\317\077\365\327\304\137\372\146\270\257\250\340\317\371\034"
        "\321\377\000\267\277\364\226\173\134\070\377\000\341\106\237\317\377\000\111\147\354\205\024\121\137\276\037\254\005\174\137\377"
        "\000\005\306\033\277\146\057\014\377\000\330\321\027\376\222\135\127\332\025\361\217\374\027\001\167\176\314\236\031\377\000\261"
        "\242\057\375\044\272\240\017\313\077\056\217\056\247\362\276\224\171\137\112\000\203\313\256\177\342\302\143\341\217\210\077\354"
        "\037\077\376\200\153\247\362\276\225\317\174\132\217\037\013\274\103\323\376\101\323\377\000\350\006\200\077\164\277\340\236\077"
        "\362\140\177\003\377\000\354\101\320\277\364\337\005\173\025\170\357\374\023\307\376\114\017\340\177\375\210\072\027\376\233\340"
        "\257\142\240\017\312\337\370\055\067\305\113\377\000\032\376\322\026\176\021\363\244\135\047\302\166\021\070\200\037\225\256\147"
        "\121\043\310\175\374\263\022\214\364\301\365\065\361\267\366\107\261\257\273\377\000\340\261\077\001\157\164\157\215\266\076\070"
        "\206\335\344\322\174\105\151\035\254\323\005\342\053\250\106\335\254\173\156\214\041\031\353\265\375\053\343\257\354\157\366\150"
        "\003\225\376\310\366\064\177\144\173\032\352\277\261\277\331\243\373\033\375\232\000\345\177\262\075\215\037\331\036\306\272\257"
        "\354\157\366\150\376\306\377\000\146\200\071\137\354\217\143\136\235\373\047\176\325\372\247\354\123\361\157\115\361\104\067\123"
        "\057\207\046\273\202\327\304\066\205\277\165\163\144\362\052\073\221\323\174\101\314\212\172\345\010\316\031\201\346\277\261\375"
        "\253\205\370\331\241\135\170\331\064\077\000\350\261\375\253\304\236\075\324\355\264\233\013\164\005\233\347\225\101\162\077\272"
        "\016\001\076\376\306\200\077\242\265\140\352\031\171\126\031\004\167\245\250\164\313\077\354\355\066\336\337\163\077\331\343\130"
        "\367\061\311\154\000\062\152\152\000\374\126\377\000\202\223\046\177\156\037\210\137\365\375\037\376\223\305\136\035\345\327\273"
        "\177\301\110\243\317\355\275\361\013\247\374\177\107\377\000\244\361\127\210\171\137\112\000\203\313\243\313\251\374\257\245\036"
        "\127\322\200\075\077\376\011\200\273\177\340\255\037\011\277\354\031\255\177\351\004\325\373\211\137\207\377\000\360\114\165\333"
        "\377\000\005\150\370\113\377\000\140\315\153\377\000\110\046\257\334\012\000\053\363\017\376\012\117\046\077\153\357\021\177\327"
        "\013\077\375\046\216\277\117\053\362\377\000\376\012\126\330\375\260\074\107\377\000\134\054\377\000\364\232\052\370\037\021\277"
        "\344\131\037\361\257\312\107\312\161\207\373\224\177\304\277\046\170\147\233\364\243\315\372\124\073\205\033\205\176\047\314\176"
        "\150\115\346\375\053\240\375\217\233\167\374\025\077\366\171\377\000\256\276\041\377\000\323\065\305\163\073\205\164\177\261\311"
        "\317\374\025\077\366\173\377\000\256\276\041\377\000\323\065\305\175\117\005\313\376\026\150\377\000\333\337\372\114\217\163\206"
        "\377\000\344\145\117\347\377\000\244\263\366\132\212\050\257\350\023\365\240\257\214\377\000\340\267\153\273\366\146\360\317\375"
        "\214\361\177\351\055\325\175\231\137\033\377\000\301\155\027\167\354\323\341\257\373\031\242\377\000\322\133\252\000\374\273\362"
        "\275\250\362\275\252\307\225\355\107\225\355\100\025\374\257\152\347\176\056\307\217\205\236\042\343\376\141\323\377\000\350\263"
        "\135\127\225\355\134\337\306\010\361\360\247\304\174\177\314\066\177\375\026\150\003\367\033\376\011\343\377\000\046\007\360\077"
        "\376\304\035\013\377\000\115\360\127\261\127\216\377\000\301\074\177\344\300\376\007\377\000\330\203\241\177\351\276\012\366\052"
        "\000\301\370\233\360\323\106\370\277\340\175\103\303\272\365\232\136\351\172\224\106\071\120\217\231\017\147\103\374\056\247\004"
        "\060\350\100\257\314\137\332\043\376\011\165\361\257\341\106\277\160\336\007\320\164\217\212\032\003\271\066\315\016\253\026\225"
        "\251\102\237\335\232\051\361\023\020\060\067\107\047\315\311\330\275\053\365\142\212\000\374\116\077\262\137\355\054\017\374\233"
        "\336\275\377\000\205\026\235\377\000\307\051\077\341\223\077\151\137\372\067\275\173\377\000\012\035\073\377\000\216\127\355\225"
        "\024\001\370\233\377\000\014\231\373\112\377\000\321\275\353\337\370\120\351\337\374\162\217\370\144\317\332\127\376\215\357\136"
        "\377\000\302\207\116\377\000\343\225\373\145\105\000\176\045\257\354\125\373\126\170\242\165\263\322\376\005\377\000\144\111\041"
        "\013\366\315\127\304\226\036\104\000\377\000\023\052\310\030\343\321\162\175\215\175\215\377\000\004\316\377\000\202\073\067\354"
        "\275\361\003\376\026\227\305\055\156\317\306\137\024\144\211\242\262\133\145\047\116\360\372\060\303\171\073\200\057\046\322\313"
        "\277\152\205\014\300\002\111\143\367\165\024\000\121\105\024\001\370\303\377\000\005\036\217\167\355\267\361\003\376\277\243\377"
        "\000\321\021\127\210\371\136\325\356\237\360\121\250\363\373\154\174\100\343\376\137\243\377\000\321\021\127\211\371\136\324\001"
        "\137\312\366\243\312\366\253\036\127\265\036\127\265\000\172\067\374\023\065\166\377\000\301\132\176\022\177\330\063\132\377\000"
        "\322\011\253\366\372\277\021\177\340\232\213\267\376\012\323\360\217\376\301\232\337\376\220\115\137\267\124\000\127\345\317\374"
        "\024\302\115\277\266\037\211\077\353\205\237\376\223\105\137\250\325\371\155\377\000\005\064\174\176\330\336\044\377\000\256\026"
        "\177\372\113\025\174\007\210\356\331\134\177\306\277\364\231\037\047\306\137\356\061\377\000\022\374\231\341\036\165\036\165\103"
        "\276\215\365\370\207\061\371\217\061\067\235\135\067\354\144\373\277\340\252\037\263\337\375\165\361\017\376\231\256\053\223\337"
        "\135\127\354\134\331\377\000\202\250\176\317\237\365\327\304\077\372\146\270\257\252\340\231\177\302\325\017\373\173\377\000\111"
        "\221\357\160\314\277\341\112\227\317\377\000\111\147\354\345\024\121\137\320\207\353\201\137\034\377\000\301\153\227\167\354\325"
        "\341\277\373\031\242\377\000\322\133\232\373\032\276\075\377\000\202\323\214\376\315\236\033\377\000\261\226\057\375\045\271\240"
        "\017\314\117\053\353\107\225\365\251\366\121\262\200\040\362\276\265\315\174\143\217\037\011\374\111\327\376\101\267\037\372\055"
        "\253\254\331\134\327\306\144\307\302\117\023\177\330\062\343\377\000\105\265\000\176\334\377\000\301\074\177\344\300\376\007\377"
        "\000\330\203\241\177\351\276\012\366\052\361\337\370\047\217\374\230\037\300\377\000\373\020\164\057\375\067\301\136\305\100\005"
        "\122\361\057\210\155\074\043\341\335\103\126\277\223\311\261\323\055\244\273\270\223\031\362\343\215\113\061\374\000\065\166\270"
        "\137\332\204\343\366\151\370\205\377\000\142\326\243\377\000\244\322\126\070\252\216\235\031\324\133\244\337\334\214\353\124\160"
        "\247\051\256\211\263\345\115\167\376\013\035\052\152\323\256\233\340\170\244\261\127\042\027\271\324\212\312\353\330\262\254\144"
        "\051\076\200\234\172\232\251\377\000\017\220\325\077\350\105\323\377\000\360\146\377\000\374\156\276\051\337\106\372\374\002\134"
        "\155\235\067\177\155\377\000\222\307\377\000\221\077\046\174\115\230\267\177\151\370\107\374\217\265\277\341\362\032\247\375\010"
        "\272\177\376\014\337\377\000\215\325\315\003\376\013\035\043\352\366\353\252\170\036\070\354\031\300\231\355\265\022\322\242\367"
        "\052\255\030\014\107\241\043\076\242\276\035\337\106\372\043\306\331\322\167\366\337\371\054\177\371\020\217\023\146\111\337\332"
        "\176\021\377\000\043\366\257\341\327\304\175\017\342\327\203\154\274\101\341\315\112\327\126\321\365\005\046\033\230\037\162\261"
        "\126\052\312\175\031\131\131\131\116\012\262\220\100\040\212\333\257\207\177\340\201\167\323\135\376\311\236\070\216\111\036\104"
        "\266\370\221\255\307\022\261\310\215\117\220\330\036\203\163\061\372\223\137\161\127\357\370\152\216\245\030\324\173\264\237\336"
        "\217\326\150\315\316\234\146\372\244\302\212\050\255\215\017\306\337\370\050\262\156\375\265\174\177\377\000\137\321\377\000\350"
        "\210\253\305\074\257\255\173\217\374\024\111\063\373\151\170\373\376\277\143\377\000\321\021\127\212\354\240\010\074\257\255\036"
        "\127\326\247\331\106\312\000\364\017\370\046\322\355\377\000\202\265\174\042\377\000\260\146\267\377\000\244\063\127\355\265\176"
        "\046\177\301\070\106\077\340\255\177\010\177\354\031\255\377\000\351\014\325\373\147\100\005\176\131\177\301\115\344\333\373\144"
        "\170\223\376\270\131\177\351\054\125\372\233\137\225\277\360\123\307\307\355\227\342\137\372\341\145\377\000\244\261\127\347\376"
        "\044\377\000\310\256\077\343\137\372\114\217\222\343\077\367\030\377\000\211\176\114\360\177\067\374\346\217\067\374\346\253\371"
        "\224\171\225\370\157\061\371\201\143\315\377\000\071\256\257\366\051\175\337\360\125\017\331\367\376\272\370\207\377\000\114\327"
        "\025\306\171\225\327\376\304\155\273\376\012\241\373\077\177\327\137\020\177\351\232\342\276\253\202\037\374\055\320\377\000\267"
        "\277\364\231\036\367\014\377\000\310\316\227\317\377\000\111\147\355\035\024\121\137\321\007\353\301\137\037\377\000\301\150\106"
        "\357\331\277\303\237\366\062\305\377\000\244\267\065\366\005\174\205\377\000\005\233\135\337\263\217\207\077\354\144\213\377\000"
        "\111\156\150\003\363\073\313\243\313\251\374\257\245\036\127\322\200\040\362\353\231\370\320\230\370\105\342\157\373\006\134\177"
        "\350\266\256\267\312\372\127\061\361\256\074\174\040\361\107\117\371\005\334\177\350\266\240\017\332\317\370\047\217\374\230\037"
        "\300\377\000\373\020\164\057\375\067\301\136\305\136\073\377\000\004\361\377\000\223\003\370\037\377\000\142\016\205\377\000\246"
        "\370\053\330\250\000\256\023\366\243\070\375\232\076\041\377\000\330\265\250\377\000\351\054\225\335\327\007\373\122\234\176\314"
        "\277\021\077\354\131\324\177\364\226\112\343\314\077\335\152\177\206\137\223\071\361\177\300\237\243\374\217\307\037\067\374\346"
        "\217\067\374\346\253\371\224\171\225\374\263\314\176\026\130\363\177\316\150\363\177\316\152\277\231\107\231\107\060\037\165\177"
        "\301\000\116\177\144\377\000\037\177\331\113\326\277\364\033\152\373\242\276\027\377\000\203\177\316\177\144\337\037\177\331\113"
        "\326\277\364\013\152\373\242\277\251\360\037\356\324\377\000\302\277\044\176\351\205\376\004\075\027\344\024\121\105\165\235\007"
        "\343\317\374\024\065\063\373\147\370\367\376\277\143\377\000\321\021\127\213\371\165\355\277\360\120\250\363\373\146\170\363\247"
        "\374\176\307\377\000\242\043\257\031\362\276\224\001\007\227\107\227\123\371\137\112\074\257\245\000\166\377\000\360\116\205\333"
        "\377\000\005\153\370\101\377\000\140\275\157\377\000\110\145\257\333\012\374\123\377\000\202\167\056\337\370\053\157\301\377\000"
        "\373\005\353\177\372\103\065\176\326\120\001\137\225\077\360\123\367\333\373\147\170\233\376\270\131\177\351\054\125\372\255\137"
        "\224\237\360\124\127\307\355\241\342\157\372\367\262\377\000\322\130\253\363\337\022\377\000\344\125\017\372\370\277\364\231\037"
        "\043\306\237\356\021\377\000\022\374\244\170\037\233\364\243\315\372\124\033\350\337\137\205\163\037\226\334\237\315\372\127\143"
        "\373\017\066\357\370\052\207\354\377\000\377\000\135\174\101\377\000\246\153\232\341\367\327\153\373\013\266\177\340\252\037\000"
        "\077\353\257\210\077\364\315\163\137\131\300\362\377\000\205\272\037\366\367\376\221\043\336\341\177\371\031\322\371\377\000\351"
        "\054\375\251\242\212\053\372\054\375\210\053\344\137\370\054\250\335\373\071\370\167\376\306\110\277\364\226\346\276\272\257\222"
        "\177\340\261\220\064\277\263\227\207\330\051\145\117\021\304\130\372\177\243\134\212\000\374\325\362\275\250\362\275\252\307\225"
        "\355\107\225\355\100\025\374\257\152\345\376\067\107\217\203\276\050\377\000\260\135\307\376\213\152\354\074\257\152\345\076\072"
        "\005\213\340\317\212\131\276\121\375\227\160\071\075\314\144\012\000\375\241\377\000\202\170\377\000\311\201\374\017\377\000\261"
        "\007\102\377\000\323\174\025\354\125\343\277\360\117\057\371\060\117\201\377\000\366\040\350\137\372\157\202\275\212\200\012\340"
        "\377\000\152\163\217\331\223\342\057\375\213\072\227\376\222\311\135\345\160\077\265\111\307\354\303\361\033\376\305\215\113\377"
        "\000\111\144\256\074\303\375\326\247\370\145\371\063\237\027\374\011\372\077\310\374\146\363\176\224\171\277\112\203\175\033\353"
        "\371\123\230\374\036\344\376\157\322\217\067\351\120\157\243\175\034\301\163\357\137\370\067\360\347\366\112\361\347\375\224\255"
        "\147\377\000\100\266\257\272\153\341\137\370\067\354\347\366\111\361\347\375\224\235\147\377\000\100\266\257\272\253\372\263\057"
        "\377\000\165\247\376\025\371\043\367\214\057\360\041\350\277\040\242\212\053\254\350\077\041\077\340\240\321\347\366\310\361\347"
        "\037\362\373\037\376\210\216\274\153\312\366\257\155\377\000\202\203\133\064\177\266\107\216\267\051\122\156\342\156\175\015\274"
        "\104\176\206\274\153\312\366\240\012\376\127\265\036\127\265\130\362\275\250\362\275\250\003\256\377\000\202\172\056\337\370\053"
        "\167\301\377\000\373\005\353\177\372\103\055\176\325\127\342\307\374\023\350\201\377\000\005\162\370\076\277\304\272\126\266\110"
        "\364\006\306\157\360\257\332\172\000\053\362\177\376\012\221\046\337\333\117\304\337\365\357\145\377\000\244\261\127\353\005\176"
        "\115\377\000\301\123\133\037\266\267\211\277\353\336\313\377\000\111\142\257\316\374\115\377\000\221\124\077\353\342\377\000\322"
        "\144\174\177\033\177\270\107\374\153\362\221\363\377\000\233\357\107\233\357\120\156\064\156\065\370\075\317\312\311\374\337\172"
        "\356\077\141\047\335\377\000\005\120\370\003\377\000\135\165\377\000\375\063\134\327\001\270\327\167\373\006\034\377\000\301\124"
        "\176\000\377\000\327\115\177\377\000\114\327\065\365\234\013\377\000\043\312\037\366\367\376\221\043\350\070\137\376\106\224\276"
        "\177\372\113\077\154\050\242\212\376\220\077\144\012\340\377\000\151\077\201\166\077\264\147\301\355\127\302\327\262\175\235\256"
        "\324\113\153\161\214\375\226\341\071\215\361\334\147\202\073\253\060\357\135\345\024\001\370\251\361\167\340\356\275\360\067\307"
        "\127\136\035\361\035\223\331\352\026\277\060\376\050\347\214\222\026\110\333\370\221\260\160\175\210\070\040\201\314\371\137\132"
        "\375\224\370\375\373\063\170\053\366\233\360\272\351\176\061\321\143\324\022\002\132\326\352\051\036\336\362\305\216\062\320\317"
        "\031\022\107\234\014\200\160\330\303\002\070\257\215\076\041\177\301\012\057\256\257\144\157\006\374\161\361\026\213\146\304\354"
        "\203\131\360\375\246\254\361\347\240\022\041\200\234\173\202\175\350\003\343\122\233\106\115\171\304\336\034\327\177\156\037\213"
        "\172\147\301\237\206\161\266\245\175\254\116\247\132\324\343\122\366\232\115\242\260\062\111\043\216\066\257\004\236\347\010\062"
        "\315\212\373\307\102\377\000\203\170\027\304\327\212\076\042\174\163\361\207\211\064\334\203\045\216\217\245\303\243\307\060\356"
        "\254\167\312\010\077\356\347\336\276\332\375\227\277\143\257\206\377\000\261\257\202\033\101\370\167\341\173\035\002\326\154\065"
        "\324\353\231\156\357\230\164\151\246\162\136\102\062\160\011\302\344\200\000\342\200\073\057\206\036\000\261\370\117\360\327\303"
        "\336\025\322\303\056\233\341\255\062\333\112\264\015\367\204\120\104\261\046\175\366\250\255\312\050\240\002\270\037\332\254\343"
        "\366\137\370\215\377\000\142\306\245\377\000\244\262\127\175\134\007\355\134\161\373\056\374\110\377\000\261\137\122\377\000\322"
        "\131\053\217\060\377\000\165\253\376\031\176\114\347\306\177\002\176\217\362\077\026\274\337\172\074\337\172\207\175\033\353\371"
        "\076\347\340\144\336\157\275\036\157\275\103\276\215\364\134\017\320\017\370\067\344\347\366\107\361\337\375\224\235\147\377\000"
        "\100\266\257\272\253\341\077\370\067\333\376\115\027\307\137\366\122\065\237\375\002\332\276\354\257\353\014\277\375\326\237\370"
        "\143\371\043\367\314\047\360\041\350\277\040\242\212\053\260\350\076\057\377\000\202\233\176\305\072\217\304\113\303\361\017\302"
        "\266\257\171\250\133\333\210\365\173\030\206\145\236\064\030\131\243\035\131\225\176\126\136\245\125\110\034\034\376\176\252\156"
        "\134\216\101\350\107\172\375\322\257\231\377\000\151\137\370\045\157\303\237\332\013\130\271\326\264\373\215\153\300\076\043\273"
        "\143\044\327\336\037\226\064\206\351\317\045\245\265\221\136\006\142\171\146\124\127\143\325\215\000\176\143\171\137\132\312\361"
        "\227\214\264\277\207\372\034\272\216\261\171\025\225\254\103\357\071\345\317\367\124\165\146\076\203\232\372\367\132\377\000\202"
        "\013\170\342\366\360\255\257\355\025\065\275\221\044\141\274\023\156\323\143\375\361\160\006\175\366\212\356\076\005\177\301\277"
        "\137\011\274\013\342\233\157\020\174\102\326\274\115\361\163\130\265\145\170\343\326\345\021\151\252\303\237\370\367\117\274\271"
        "\376\007\166\103\335\115\000\170\127\374\020\237\366\151\361\037\306\177\332\037\126\375\242\274\101\247\134\151\076\027\323\354"
        "\045\321\074\043\025\302\025\222\364\271\333\055\302\372\242\246\365\334\070\055\053\000\117\226\153\365\232\253\351\132\125\256"
        "\205\246\133\331\130\333\133\331\331\332\106\260\301\004\021\210\342\205\024\141\125\124\140\052\200\000\000\160\000\253\024\000"
        "\127\344\267\374\025\112\115\277\266\317\212\077\353\336\307\377\000\111\042\257\326\232\374\221\377\000\202\252\237\370\315\317"
        "\024\177\327\275\217\376\222\105\137\234\370\237\377\000\042\230\177\327\305\377\000\244\310\370\356\070\377\000\160\217\370\327"
        "\345\043\347\277\067\374\346\217\067\374\346\241\335\106\352\374\022\347\345\067\046\363\177\316\153\276\375\201\233\167\374\025"
        "\107\340\037\375\164\327\377\000\364\315\163\136\167\272\275\013\366\001\071\377\000\202\251\174\003\377\000\256\232\377\000\376"
        "\231\256\153\353\270\015\377\000\302\355\017\373\173\377\000\110\221\364\034\053\377\000\043\112\137\077\375\045\237\266\324\121"
        "\105\177\111\237\263\205\024\121\100\005\024\121\100\005\024\121\100\005\024\121\100\005\171\377\000\355\140\161\373\055\374\110"
        "\377\000\261\137\123\377\000\322\131\153\320\053\317\377\000\153\057\371\065\237\211\137\366\053\152\177\372\111\055\161\346\077"
        "\356\225\177\303\057\311\234\370\317\340\117\321\376\107\342\157\233\376\163\107\233\376\163\120\356\243\165\177\045\134\374\002"
        "\344\336\157\371\315\036\157\371\315\103\272\215\324\134\056\176\206\177\301\276\277\362\150\236\072\377\000\262\221\254\177\350"
        "\026\325\367\145\174\045\377\000\006\371\034\376\310\076\070\377\000\262\217\254\177\350\026\325\367\155\177\132\345\377\000\356"
        "\264\377\000\303\037\311\037\277\341\077\201\017\105\371\005\024\121\135\207\100\121\105\024\000\121\105\024\000\121\105\024\000"
        "\127\344\217\374\025\202\031\055\377\000\155\257\021\263\051\125\232\322\305\320\377\000\170\175\232\065\317\346\244\176\025\372"
        "\335\137\235\277\360\133\037\203\223\132\170\257\302\336\074\267\205\232\326\362\334\350\367\216\007\021\312\205\244\210\237\166"
        "\126\220\177\333\072\370\037\022\060\263\255\223\071\103\354\112\062\176\232\257\324\371\076\063\241\052\231\153\224\176\314\223"
        "\371\152\277\123\341\077\062\217\062\233\105\177\074\237\220\134\167\231\136\225\377\000\004\370\260\232\137\370\052\007\300\273"
        "\305\214\375\232\332\343\133\212\107\354\214\372\065\336\300\176\273\110\372\327\231\326\207\205\174\121\177\340\237\023\351\372"
        "\316\227\163\045\236\245\245\134\307\167\153\072\175\350\145\106\014\254\063\307\004\016\017\006\275\216\037\315\026\135\230\122"
        "\306\265\165\027\252\362\151\247\157\073\075\074\317\107\051\307\375\117\027\014\113\127\121\172\257\046\254\377\000\006\176\370"
        "\121\137\042\176\312\177\360\126\157\010\374\123\260\265\322\374\167\045\277\204\174\105\200\206\345\362\064\333\306\376\360\177"
        "\371\144\117\122\037\345\035\230\364\257\255\164\335\116\337\130\261\212\352\316\342\033\253\151\224\074\163\102\342\110\344\007"
        "\241\014\070\043\351\137\323\131\136\161\203\314\051\173\134\044\324\227\156\253\325\156\217\333\160\071\216\037\031\017\151\207"
        "\222\222\374\127\252\335\023\121\105\025\351\035\241\105\024\120\001\105\024\120\001\105\024\120\001\136\137\373\153\370\242\303"
        "\302\137\262\137\304\113\215\102\346\073\130\256\064\013\313\050\213\237\365\223\115\013\105\022\017\166\167\121\370\324\177\264"
        "\007\355\235\360\367\366\155\260\230\370\213\136\267\155\111\027\061\351\126\154\056\057\245\075\207\226\017\311\237\357\071\125"
        "\367\257\314\137\333\077\366\347\361\027\355\171\342\030\343\232\077\354\177\013\351\362\027\261\322\243\175\337\067\043\315\225"
        "\277\216\114\034\172\050\044\001\311\047\342\170\263\213\060\170\014\064\350\051\051\125\222\151\105\153\153\253\136\135\255\333"
        "\167\370\237\063\304\034\101\206\302\121\225\044\324\252\064\322\113\245\372\276\337\233\074\077\314\243\314\246\321\137\316\047"
        "\343\127\035\346\121\346\123\153\053\307\036\051\207\301\136\021\324\065\131\212\354\262\201\244\000\377\000\033\177\012\376\055"
        "\201\370\326\224\151\116\255\110\322\202\273\223\111\057\067\242\056\234\045\071\250\103\126\335\221\372\115\377\000\006\366\110"
        "\046\375\216\374\153\042\374\313\047\304\155\135\224\372\217\056\332\276\357\257\223\277\340\211\037\005\257\076\012\177\301\070"
        "\174\007\036\245\033\305\252\170\241\147\361\045\320\141\206\077\153\220\311\021\076\346\017\046\276\261\257\353\254\075\057\145"
        "\112\064\377\000\225\045\367\043\372\022\215\076\112\161\207\144\220\121\105\025\261\240\121\105\024\000\121\105\024\000\121\105"
        "\024\000\127\033\361\373\340\246\223\373\103\174\045\326\074\047\254\056\333\135\122\054\107\062\256\136\326\125\346\071\127\335"
        "\130\003\216\343\043\241\065\331\121\131\326\243\012\324\345\112\252\274\144\232\153\272\173\231\324\247\032\220\164\346\256\232"
        "\263\136\107\341\137\306\337\203\032\357\300\017\211\072\227\205\374\103\152\155\365\015\075\360\030\017\335\334\306\176\344\261"
        "\236\350\303\220\176\240\340\202\007\047\137\262\377\000\266\237\354\141\242\376\327\276\001\026\263\074\172\157\211\164\325\146"
        "\322\265\115\233\274\246\075\142\220\016\132\046\075\107\120\171\034\344\037\311\017\214\277\006\174\115\373\076\370\372\157\014"
        "\370\273\113\227\111\325\243\006\110\203\035\320\336\304\016\074\330\044\351\044\147\216\107\040\234\060\126\312\217\347\076\052"
        "\340\334\116\125\121\325\246\234\250\275\245\333\312\135\275\166\176\272\037\216\347\334\067\133\001\067\122\013\232\237\107\333"
        "\311\377\000\236\317\360\071\172\050\242\276\054\371\220\256\277\341\207\307\357\032\374\027\271\363\074\055\342\215\147\104\031"
        "\334\321\133\334\260\205\317\373\121\234\243\176\040\327\041\105\151\106\265\112\122\347\245\047\026\272\247\147\367\242\351\324"
        "\235\071\163\101\264\373\255\017\254\074\011\377\000\005\214\370\261\341\170\326\075\122\037\015\370\221\027\202\367\126\106\011"
        "\217\343\013\042\377\000\343\246\275\113\303\077\360\134\145\362\302\353\037\017\033\167\171\054\265\156\017\374\005\242\377\000"
        "\331\253\363\366\212\372\154\067\033\147\164\025\243\210\157\374\126\227\346\233\075\272\074\115\231\322\126\215\126\375\154\377"
        "\000\064\317\323\015\063\376\013\151\340\031\223\375\063\302\236\056\267\157\110\205\274\243\363\062\057\362\255\050\377\000\340"
        "\264\337\013\131\176\155\027\306\312\175\076\307\156\177\366\275\176\136\321\136\224\174\110\316\226\362\213\377\000\267\121\333"
        "\036\063\314\326\362\137\162\077\116\256\277\340\265\277\015\142\123\345\170\167\306\262\267\140\155\355\227\077\371\032\271\335"
        "\157\376\013\205\240\102\215\375\233\340\035\142\345\277\207\355\072\204\160\017\307\152\275\176\163\321\121\123\304\134\356\113"
        "\112\211\172\105\176\251\221\056\062\314\336\323\113\376\335\137\255\317\265\074\137\377\000\005\264\361\276\242\216\272\037\204"
        "\274\063\245\006\341\132\352\111\257\035\077\043\030\317\324\127\210\374\120\377\000\202\201\174\135\370\265\034\220\352\136\063"
        "\324\255\054\344\340\333\151\241\154\143\307\241\362\202\263\017\367\211\257\031\242\274\114\147\024\146\330\245\313\132\274\232"
        "\354\235\227\334\254\217\067\021\236\143\353\253\125\253\053\166\116\313\356\126\035\064\317\161\053\111\043\064\222\071\334\314"
        "\307\045\217\251\064\332\050\257\004\362\102\212\052\015\113\123\267\321\254\144\272\273\236\033\153\170\106\347\226\127\012\250"
        "\075\311\252\214\134\237\054\167\034\142\333\262\047\251\177\145\017\331\252\373\376\012\117\373\130\151\236\005\261\111\233\341"
        "\337\204\347\217\122\361\206\245\036\104\157\032\267\313\154\257\375\371\010\050\270\071\373\355\202\043\254\337\331\347\340\037"
        "\304\217\370\051\057\216\346\360\277\303\033\071\164\277\010\332\312\042\327\074\137\167\033\045\255\242\037\274\221\364\057\041"
        "\136\210\277\063\144\147\142\345\253\366\327\366\063\375\215\274\031\373\015\174\022\261\360\117\202\354\331\055\341\076\175\365"
        "\364\300\033\255\126\344\200\036\171\230\165\143\200\000\034\052\200\007\002\277\150\340\136\011\251\207\250\263\034\301\132\113"
        "\341\213\335\177\171\371\366\135\067\172\237\245\360\267\014\316\214\326\063\026\254\376\314\173\171\277\076\313\247\251\352\126"
        "\026\020\351\126\020\332\332\303\035\275\265\264\153\024\121\106\241\122\064\121\205\120\007\000\000\000\002\246\242\212\375\150"
        "\375\000\050\242\212\000\050\242\212\000\050\242\212\000\050\242\212\000\050\242\212\000\053\217\370\333\360\013\301\377\000\264"
        "\137\203\144\320\174\145\241\131\353\232\173\035\361\171\240\254\326\262\143\036\144\062\251\017\024\203\373\310\300\366\316\053"
        "\260\242\246\121\122\134\262\330\122\111\253\063\362\367\366\226\377\000\202\065\170\353\341\253\334\152\177\014\265\010\374\175"
        "\243\051\056\064\175\102\130\354\365\213\165\376\354\163\035\260\134\143\375\277\045\261\335\317\065\361\347\212\045\270\370\177"
        "\342\206\320\274\121\247\352\236\023\327\227\376\141\332\335\243\330\334\067\272\011\000\022\057\243\041\145\075\211\257\350\026"
        "\271\377\000\211\077\011\274\057\361\227\303\222\150\376\056\360\356\207\342\155\056\117\275\151\252\131\107\167\016\175\102\270"
        "\040\037\161\315\174\046\155\341\336\127\214\156\164\123\245\057\356\355\377\000\200\275\076\353\037\051\230\160\176\007\020\334"
        "\251\376\356\136\133\175\337\345\143\360\176\212\375\056\370\253\377\000\004\054\370\075\342\331\245\271\360\215\367\214\076\033"
        "\335\267\314\261\350\332\221\270\260\015\357\155\162\044\120\277\354\241\101\364\257\237\074\177\377\000\004\067\370\313\341\103"
        "\043\170\117\342\027\200\374\143\002\344\254\172\316\237\076\217\160\303\323\164\046\144\047\334\205\037\112\370\054\157\205\371"
        "\225\067\174\074\343\121\174\342\376\347\247\342\174\246\047\201\361\260\326\214\243\045\367\077\307\117\304\371\122\212\364\237"
        "\027\376\300\037\264\247\200\003\065\367\301\273\215\136\030\372\317\241\153\326\167\273\375\304\145\222\117\303\155\171\327\210"
        "\174\027\361\033\301\233\277\266\276\015\374\140\322\325\176\364\222\170\126\342\110\207\374\015\003\057\353\137\073\133\202\363"
        "\272\137\026\036\137\053\077\311\263\307\251\303\071\235\075\350\277\225\237\344\331\035\025\313\352\177\025\354\264\023\215\113"
        "\114\361\076\226\335\326\357\104\272\210\217\315\053\056\177\332\127\301\266\247\022\352\223\104\336\217\145\072\237\375\002\270"
        "\145\303\171\262\337\015\123\377\000\000\227\371\034\257\045\314\027\374\270\237\376\002\377\000\310\357\050\256\016\037\332\127"
        "\302\027\137\352\157\256\256\075\243\260\235\263\377\000\216\126\226\233\361\117\373\177\037\331\136\027\361\266\261\273\247\330"
        "\264\051\345\335\364\342\252\034\061\233\313\154\064\377\000\360\026\277\064\070\344\171\204\266\243\057\374\005\235\125\024\150"
        "\077\017\176\060\170\330\057\366\017\300\137\213\227\252\377\000\162\113\255\006\133\070\233\376\006\343\030\367\257\102\360\237"
        "\374\023\157\366\262\370\215\263\354\377\000\014\174\077\341\010\145\351\076\275\257\300\373\007\251\110\130\270\377\000\276\177"
        "\012\365\060\374\007\235\326\177\301\345\135\333\212\374\057\177\300\356\243\302\231\245\117\371\167\157\126\227\353\177\300\363"
        "\332\317\361\017\213\164\277\011\333\171\332\236\241\147\143\037\143\064\241\167\175\001\344\376\025\365\227\200\177\340\337\157"
        "\212\136\061\170\337\342\017\306\315\063\103\267\353\055\227\205\264\267\225\230\167\121\074\206\042\277\122\215\364\257\244\176"
        "\005\177\301\006\077\147\217\203\167\361\152\032\227\207\365\117\210\132\274\170\143\167\342\253\343\170\254\336\360\040\110\130"
        "\173\072\065\175\116\137\341\135\171\064\361\265\224\127\150\246\337\336\355\157\271\236\366\023\200\352\267\174\115\104\274\243"
        "\257\342\355\371\063\362\237\341\326\265\343\037\332\107\304\022\150\277\010\174\005\342\057\036\352\012\333\036\352\013\146\216"
        "\302\324\236\206\131\133\012\203\375\366\101\357\137\152\376\313\177\360\157\246\251\343\215\116\307\304\177\264\127\212\277\264"
        "\226\026\023\107\341\015\012\123\035\242\036\273\147\270\030\055\350\104\100\037\111\110\257\324\017\012\370\107\111\360\056\205"
        "\157\245\350\232\136\237\243\351\226\253\266\033\113\033\144\267\202\021\350\250\200\050\374\005\150\127\350\371\067\011\345\271"
        "\147\275\207\205\347\374\322\326\137\344\276\111\037\145\226\344\030\054\016\264\143\357\167\172\277\370\037\053\030\177\016\276"
        "\032\370\177\341\017\203\154\174\073\341\175\027\115\360\376\207\246\247\227\155\143\141\156\260\103\010\366\125\000\144\365\047"
        "\251\074\234\232\334\242\212\372\103\332\012\050\242\200\012\050\242\200\012\050\242\200\012\050\242\200\012\050\242\200\077\377"
        "\331");
        defaultValue.setByteString(tmpImageJPG, OpcUa_False);
    }
    pImageJPG->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImageJPG, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImageJPG;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarImagePNG(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pImagePNG = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImagePNG, getTypeNamespace()));
    pImagePNG->setBrowseName(UaQualifiedName("ImagePNG", getTypeNamespace()));
    pImagePNG->setDisplayName(UaLocalizedText("", "ImagePNG"));
    pImagePNG->setAccessLevel(3);
    pImagePNG->setUserAccessLevel(3);
    pImagePNG->setValueRank(-1);
    pImagePNG->setDataTypeId(OpcUaId_ImagePNG);
    {
        UaByteString tmpImagePNG = UaByteString(10370, (OpcUa_Byte*) "\211\120\116\107\015\012\032\012\000\000\000\015\111\110\104\122\000\000\001\000\000\000\001\000\010\006\000\000\000\134\162\250"
        "\146\000\000\000\001\163\122\107\102\000\256\316\034\351\000\000\000\004\147\101\115\101\000\000\261\217\013\374\141\005\000\000"
        "\000\011\160\110\131\163\000\000\016\304\000\000\016\304\001\225\053\016\033\000\000\050\027\111\104\101\124\170\136\355\235\207"
        "\137\025\307\372\377\177\177\316\115\057\067\345\346\246\122\244\203\010\026\020\025\120\261\040\140\101\024\354\202\150\120\354"
        "\306\353\115\064\326\010\166\243\211\061\032\143\215\306\236\330\153\064\326\330\053\026\104\237\337\176\146\367\050\236\314\367"
        "\312\071\234\235\231\303\171\076\171\275\137\226\200\354\331\147\347\275\063\263\263\273\377\217\070\234\006\344\311\223\047\124"
        "\133\133\113\367\357\337\247\033\067\156\320\251\123\247\150\357\336\275\264\145\313\026\132\273\166\015\175\373\355\162\252\252"
        "\232\117\063\146\314\240\051\123\046\323\350\321\243\251\254\254\214\206\015\033\112\203\006\015\242\222\222\142\052\052\052\242"
        "\076\175\172\123\101\101\001\345\345\365\170\216\374\374\074\352\325\253\047\365\355\133\110\375\373\367\243\201\003\007\320\220"
        "\041\203\251\264\164\070\215\034\131\116\343\306\215\243\151\323\246\321\234\071\163\150\321\242\105\364\375\367\337\323\372\365"
        "\077\323\257\277\376\112\207\016\035\242\213\027\057\320\335\273\167\351\341\303\207\124\127\127\347\154\065\347\105\141\001\160"
        "\236\006\215\034\015\374\352\325\253\164\364\350\021\332\270\161\043\055\136\274\210\246\117\377\212\052\053\307\320\320\241\103"
        "\250\260\260\217\150\260\075\172\344\032\007\044\002\321\100\030\223\047\117\242\271\163\347\010\121\354\336\275\213\316\236\075"
        "\113\267\156\335\142\071\170\205\005\020\242\101\143\277\166\355\032\035\074\170\200\126\257\136\115\263\146\315\262\316\332\025"
        "\342\314\333\263\147\201\264\201\005\063\220\026\172\027\303\206\015\023\075\024\364\042\320\173\071\175\372\264\220\136\250\206"
        "\005\020\042\271\167\357\236\070\253\257\135\273\326\072\243\117\027\147\363\336\275\173\111\033\113\050\201\136\003\206\046\350"
        "\341\054\134\270\120\014\051\060\234\170\374\370\261\263\347\232\166\130\000\115\060\070\273\243\301\037\071\162\330\032\233\177"
        "\053\306\343\005\005\371\306\166\335\115\003\373\011\363\020\030\372\154\332\264\211\056\135\272\104\217\036\075\162\366\156\323"
        "\012\013\240\211\004\023\164\350\316\242\073\077\151\322\104\061\126\227\035\334\214\357\100\010\230\314\304\234\302\216\035\333"
        "\351\346\315\233\102\262\115\041\054\200\040\016\316\112\047\117\236\244\245\113\227\212\331\162\164\147\145\007\060\023\130\040"
        "\327\057\276\230\042\346\020\256\137\277\346\124\043\070\303\002\010\262\340\314\163\342\304\011\061\211\205\161\074\067\172\275"
        "\340\262\346\304\211\023\305\120\341\366\355\333\116\225\202\047\054\200\040\311\235\073\167\150\303\206\365\124\136\076\202\307"
        "\362\206\202\111\325\231\063\147\322\261\143\307\202\146\022\221\005\140\160\352\352\354\056\076\306\236\074\246\017\056\260\026"
        "\341\347\237\177\026\363\005\046\207\005\140\140\320\360\177\373\355\067\032\073\266\222\317\366\101\116\121\121\137\132\270\160"
        "\001\135\271\162\305\251\256\131\141\001\030\024\164\033\261\274\266\242\342\163\156\370\115\014\364\340\252\253\253\214\023\001"
        "\013\300\200\240\341\357\331\263\233\106\215\032\311\015\277\211\203\171\202\157\276\231\047\226\133\233\020\026\200\346\234\075"
        "\173\206\046\114\030\057\075\130\230\246\013\256\036\140\315\206\356\145\310\054\000\115\301\235\153\313\226\055\345\345\270\041"
        "\016\256\352\034\070\160\100\333\302\042\026\200\342\240\320\270\073\015\327\360\145\007\004\023\172\140\055\007\156\243\306\155"
        "\326\252\303\002\120\030\334\253\216\361\037\217\363\031\031\003\006\224\320\301\203\007\235\243\105\115\130\000\212\162\376\374"
        "\171\032\065\152\224\264\360\014\343\001\067\155\255\134\271\102\334\333\241\042\054\000\227\203\056\377\366\355\333\305\055\247"
        "\262\202\063\214\014\334\320\165\375\372\165\347\050\162\057\054\000\027\203\313\173\313\227\057\347\365\372\214\137\014\032\064"
        "\120\074\172\315\315\260\000\134\012\356\324\233\073\167\056\217\367\231\106\201\247\030\355\334\271\323\071\252\002\037\026\200"
        "\013\301\370\015\263\272\262\202\062\214\077\374\360\303\017\256\134\052\144\001\004\070\070\363\317\236\075\113\132\104\206\361"
        "\227\356\335\273\321\202\005\013\002\376\120\123\026\100\200\263\164\351\022\151\001\031\246\261\100\002\170\054\172\040\157\065"
        "\146\001\004\060\233\067\157\346\061\177\003\300\201\014\272\165\353\112\135\273\166\021\277\342\317\271\271\335\245\137\317\074"
        "\003\373\153\336\274\171\001\033\016\260\000\002\224\323\247\117\205\314\262\136\064\124\064\332\116\235\072\122\207\016\355\051"
        "\043\243\055\265\151\323\206\132\266\114\245\344\344\144\112\114\114\244\330\330\130\212\212\212\242\210\210\010\101\170\170\070"
        "\205\205\205\075\307\147\237\175\366\024\357\377\347\371\276\310\310\110\212\211\211\246\204\204\170\152\336\274\071\265\150\321"
        "\202\132\265\152\111\351\351\351\324\256\135\006\145\145\145\122\227\056\071\041\045\017\174\146\274\357\040\020\141\001\004\040"
        "\065\065\167\305\133\160\144\305\012\146\160\126\316\311\351\114\231\231\035\050\055\055\115\064\076\064\104\064\354\372\215\367"
        "\377\002\015\071\046\046\206\222\222\222\050\065\065\325\152\270\255\204\050\354\306\333\216\262\263\263\255\177\037\215\027\313"
        "\141\363\255\237\327\135\374\071\053\053\113\374\177\174\035\276\036\337\207\357\207\130\242\243\243\305\277\353\375\263\040\213"
        "\330\130\373\147\101\104\220\122\166\166\226\070\143\312\076\133\260\223\236\236\106\173\366\354\161\216\100\377\303\002\150\144"
        "\320\025\133\262\244\151\214\373\161\046\155\337\276\275\150\100\361\361\361\342\354\053\153\154\000\177\217\063\162\247\116\235"
        "\304\253\277\046\115\232\044\306\247\170\014\071\336\050\204\007\232\374\361\307\037\164\361\342\105\361\124\034\074\246\334\363"
        "\332\256\206\166\137\361\165\230\124\305\367\341\373\261\126\376\302\205\013\342\051\111\070\370\327\257\137\117\313\226\055\023"
        "\057\065\231\060\141\002\025\027\027\013\171\044\044\044\074\335\116\364\074\040\054\154\053\032\115\307\216\331\115\242\267\000"
        "\071\243\126\215\275\255\230\005\320\310\240\353\217\167\332\311\212\144\072\150\010\070\273\343\354\212\056\073\032\113\375\106"
        "\356\001\147\376\276\175\373\212\106\006\331\141\145\043\226\066\343\071\205\246\275\152\013\302\300\303\071\317\234\071\103\133"
        "\267\156\025\063\347\225\225\225\324\263\147\117\321\203\200\270\040\066\374\036\102\200\364\144\373\046\030\350\320\241\203\220"
        "\157\143\152\300\002\150\104\160\206\302\231\117\126\034\123\301\231\003\007\016\306\352\221\221\021\177\073\303\067\153\326\114"
        "\164\315\361\244\333\237\176\372\211\316\235\073\047\032\125\260\007\265\302\372\014\254\254\133\265\152\225\170\131\112\106\106"
        "\206\220\001\344\327\272\165\353\240\234\113\300\220\147\315\232\065\316\247\364\075\054\200\106\004\257\332\222\025\305\104\060"
        "\141\207\256\075\032\170\375\006\017\001\244\244\244\120\171\171\071\255\133\267\116\274\005\047\124\002\051\100\160\020\002\316"
        "\244\220\042\206\017\230\134\204\050\145\373\321\064\060\317\321\272\165\053\277\357\033\140\001\064\042\170\311\244\254\050\246"
        "\200\263\031\306\274\315\233\047\375\255\173\217\061\161\105\105\005\355\330\261\203\152\152\152\234\117\024\332\301\333\203\161"
        "\051\167\350\320\241\142\330\223\226\326\106\134\355\220\355\133\223\200\264\360\276\107\177\302\002\360\063\170\201\244\311\157"
        "\321\305\354\075\032\176\375\106\017\060\323\216\063\236\351\217\253\326\235\313\227\057\323\342\305\213\305\044\147\333\266\155"
        "\215\036\032\140\062\020\163\032\376\364\002\130\000\176\006\167\371\311\212\241\033\034\250\270\154\126\377\214\217\337\027\026"
        "\026\322\256\135\273\214\233\264\063\075\230\377\330\260\141\203\230\350\105\157\112\266\317\165\203\232\143\150\207\253\060\276"
        "\206\005\340\107\320\210\312\312\112\245\305\320\011\316\126\161\161\161\117\033\076\306\367\235\073\167\026\263\366\234\306\005"
        "\313\157\061\051\212\305\136\046\366\006\060\144\301\232\011\137\037\044\302\002\360\043\147\317\236\025\117\156\221\025\102\027"
        "\231\231\231\142\106\333\323\370\041\202\352\352\152\172\360\340\201\263\325\234\100\004\017\163\305\143\335\114\173\306\003\056"
        "\347\102\370\277\376\372\253\263\245\015\013\013\300\217\140\241\213\254\010\272\300\162\334\210\210\147\135\176\134\306\073\162"
        "\344\210\263\265\234\100\007\127\017\160\217\076\036\355\055\253\207\016\060\131\011\001\214\031\063\306\331\312\206\205\005\340"
        "\107\146\315\232\051\055\202\016\160\171\257\376\170\037\013\136\060\233\315\161\077\273\167\357\066\252\047\210\145\322\030\006"
        "\140\345\144\103\303\002\360\061\030\013\342\305\217\262\002\250\006\353\334\353\257\313\307\122\130\054\231\345\250\313\212\025"
        "\053\244\265\321\101\162\162\163\061\014\304\022\354\206\206\005\340\143\320\300\112\112\212\245\005\120\015\026\366\170\032\077"
        "\046\000\371\322\236\372\340\315\076\246\274\343\001\013\202\160\054\340\351\101\015\015\013\300\307\240\221\231\060\001\204\263"
        "\277\247\353\217\245\254\276\130\237\023\330\154\334\270\101\132\043\325\340\016\110\034\017\123\247\116\165\266\354\305\141\001"
        "\370\030\334\221\046\333\371\252\301\155\263\236\263\377\177\376\363\037\147\353\070\072\202\067\072\313\152\244\032\054\141\306"
        "\361\200\145\315\015\015\013\300\307\230\050\000\236\364\323\033\323\004\320\265\153\127\147\313\136\034\026\200\217\201\000\360"
        "\330\057\335\340\316\065\217\000\232\302\335\172\301\034\010\100\126\043\325\340\041\052\070\036\260\370\253\241\141\001\370\030"
        "\026\000\307\073\277\375\146\206\000\332\267\147\001\270\036\026\000\307\073\054\200\020\012\013\200\343\035\026\100\010\305\024"
        "\001\340\062\040\013\300\214\260\000\102\050\067\157\336\020\353\000\164\303\002\060\047\170\000\252\254\106\252\301\163\001\130"
        "\000\056\207\005\300\361\016\013\040\204\302\002\340\170\207\005\020\102\141\001\160\274\303\002\010\241\340\136\000\334\002\252"
        "\033\334\377\315\002\060\043\020\200\254\106\252\301\163\041\130\000\056\207\005\300\361\016\013\040\204\302\002\340\170\207\005"
        "\020\102\141\001\160\274\303\002\010\241\100\000\170\037\300\377\102\126\234\100\203\067\327\260\000\314\310\357\277\377\046\075"
        "\016\352\043\253\141\240\301\203\101\131\000\056\247\041\002\120\001\013\300\234\064\104\000\052\140\001\050\010\013\200\343\035"
        "\026\100\010\205\005\300\361\016\013\040\204\302\002\340\170\207\005\020\102\201\000\360\236\070\335\340\045\237\054\000\063\362"
        "\373\357\277\113\153\244\232\254\254\114\026\200\333\141\001\160\274\303\002\010\241\260\000\070\336\141\001\204\120\130\000\034"
        "\357\260\000\102\050\020\000\136\021\255\033\274\242\232\005\140\106\040\000\131\215\124\303\002\120\020\026\000\307\073\346\010"
        "\040\313\034\001\074\261\376\253\255\173\100\065\265\167\350\162\315\005\072\165\343\010\235\270\176\200\056\334\071\115\267\037"
        "\336\240\007\165\367\350\361\223\307\316\127\007\117\130\000\034\357\260\000\234\074\172\134\113\047\257\037\244\065\177\054\242"
        "\351\173\312\251\164\123\016\365\137\227\116\375\176\112\373\033\203\326\147\322\244\035\045\264\354\310\014\332\373\327\057\164"
        "\267\366\266\363\257\230\035\026\000\307\073\041\057\200\333\017\157\322\306\077\127\322\270\137\213\254\006\217\006\336\306\147"
        "\040\213\245\107\246\323\331\133\047\105\357\301\324\100\000\175\372\364\326\016\013\300\234\354\333\367\273\264\106\252\311\316"
        "\126\054\000\164\341\267\237\137\107\145\233\272\112\033\265\077\014\370\271\035\055\072\064\315\330\036\201\051\002\300\273\340"
        "\130\000\146\044\044\005\160\343\376\025\372\317\356\141\126\243\115\243\042\253\341\006\232\122\113\052\007\056\357\064\256\067"
        "\300\002\340\170\047\344\004\160\361\316\237\124\261\265\247\264\341\006\222\222\237\063\150\333\271\265\106\111\200\005\300\361"
        "\116\110\011\340\352\275\277\104\227\277\150\155\153\045\364\377\051\235\266\235\135\353\374\164\375\261\005\320\107\073\054\000"
        "\163\262\157\337\076\151\215\124\223\235\235\355\256\000\060\313\077\165\327\020\151\103\165\223\222\265\031\264\377\364\036\147"
        "\053\364\006\002\050\054\354\243\035\274\012\212\005\140\106\040\000\131\215\124\323\261\243\313\002\130\373\307\022\352\153\065"
        "\110\035\364\131\224\116\277\355\333\353\154\211\276\260\000\070\336\011\011\001\334\255\275\105\303\067\166\266\032\143\053\075"
        "\254\151\105\271\245\131\164\365\352\125\147\213\364\204\005\300\361\116\110\010\140\317\305\315\362\206\251\220\316\123\223\250"
        "\254\254\214\036\077\326\267\202\220\005\300\361\116\110\010\140\301\301\251\124\150\065\102\235\024\054\113\241\350\270\110\072"
        "\176\374\270\263\125\352\003\001\364\355\133\250\235\274\274\074\026\200\041\201\000\144\065\122\115\307\216\035\335\023\300\027"
        "\273\006\133\215\260\245\126\372\254\116\245\350\026\021\064\155\332\064\147\253\324\307\024\001\340\135\160\054\000\063\022\022"
        "\002\030\367\153\077\151\243\124\312\232\226\024\223\032\101\135\273\166\245\047\117\364\254\015\140\001\160\274\023\032\002\330"
        "\126\044\032\240\156\142\132\106\120\174\174\274\266\171\000\026\000\307\073\041\041\200\261\333\372\122\237\065\251\332\201\000"
        "\302\302\302\064\013\240\257\166\130\000\346\304\026\200\274\116\052\351\324\051\104\004\200\017\251\123\000\105\105\175\265\223"
        "\237\237\317\002\060\044\373\367\357\223\326\110\065\006\011\040\305\065\130\000\066\054\000\163\022\042\002\050\224\066\110\325"
        "\260\000\154\130\000\346\044\044\004\120\151\011\240\267\325\000\165\143\206\000\212\264\303\002\060\047\373\367\357\227\326\110"
        "\065\235\072\165\012\166\001\264\170\041\046\010\240\137\277\042\355\340\165\320\054\000\063\002\001\310\152\244\032\167\005\260"
        "\265\017\365\376\321\152\204\232\301\072\000\026\000\013\300\244\260\000\024\321\313\202\005\140\303\002\060\047\041\041\200\061"
        "\226\000\320\000\165\303\002\260\051\050\050\140\001\030\222\020\022\100\262\166\164\013\340\326\255\233\324\277\177\077\355\340"
        "\165\320\052\005\000\361\155\335\372\013\125\125\315\247\311\223\047\123\171\371\010\052\053\053\245\322\322\341\332\301\166\214"
        "\036\135\101\063\146\114\247\345\313\227\321\261\143\307\250\256\256\316\331\162\367\003\001\310\152\244\032\026\200\202\204\222"
        "\000\160\277\305\371\363\347\351\313\057\277\024\317\235\307\260\043\030\300\276\031\064\150\040\255\133\267\216\036\076\174\350"
        "\174\032\367\142\212\000\072\167\166\131\000\075\255\006\250\233\150\026\200\300\155\001\324\326\326\212\263\051\036\066\211\237"
        "\025\254\240\167\160\362\344\011\347\123\271\223\020\021\200\165\040\374\330\134\073\054\000\033\034\334\156\011\240\246\246\206"
        "\376\373\337\151\177\173\363\114\260\122\134\334\237\266\157\377\325\265\073\110\131\000\012\321\057\200\133\326\316\356\257\235"
        "\236\075\173\272\042\000\234\371\321\370\275\037\071\035\354\140\222\354\367\337\177\163\076\145\140\143\013\100\136\047\225\270"
        "\057\200\325\126\043\324\214\011\002\300\031\105\067\170\037\174\240\005\200\063\344\262\145\313\110\166\253\151\123\140\340\300"
        "\001\164\354\330\121\347\323\006\056\007\016\354\227\326\110\065\150\370\256\011\140\364\326\136\124\260\072\111\073\054\000\033"
        "\067\004\160\346\314\237\124\122\122\114\262\165\346\115\005\364\004\116\237\076\355\174\342\300\204\005\240\020\026\200\115\240"
        "\005\200\375\371\365\327\137\077\167\135\271\051\202\261\162\357\336\275\105\035\003\225\020\021\100\117\253\001\046\152\207\005"
        "\140\023\150\001\134\277\176\135\234\375\145\077\253\251\321\245\113\016\115\237\076\335\371\344\215\017\013\100\041\046\010\000"
        "\015\105\067\230\335\016\244\000\266\156\335\052\375\071\115\021\014\005\222\223\223\351\257\277\376\162\076\175\343\162\340\300"
        "\001\351\317\121\115\116\216\213\002\300\313\100\363\255\006\250\033\026\200\115\240\005\120\125\125\105\003\006\224\204\004\230"
        "\014\114\114\114\244\352\352\152\347\323\067\056\054\000\205\104\247\206\263\000\054\002\055\000\054\243\105\303\010\025\122\122"
        "\132\210\113\251\201\070\216\102\103\000\277\024\120\376\017\011\332\141\001\330\004\132\000\143\307\216\245\301\203\007\205\014"
        "\031\031\031\024\027\027\107\167\357\336\165\366\200\377\011\011\001\174\156\011\040\317\152\200\272\061\103\000\045\332\301\114"
        "\166\040\005\060\165\352\124\032\062\144\160\310\320\272\165\153\261\357\056\137\276\354\354\001\377\143\013\100\136\047\225\344"
        "\344\344\260\000\334\016\004\040\033\127\252\006\253\333\002\051\200\352\352\052\032\072\164\110\310\320\274\171\163\261\357\216"
        "\036\155\374\302\040\010\100\126\043\325\340\352\206\313\002\210\327\016\013\300\046\320\002\130\263\146\015\015\037\076\054\044"
        "\200\000\142\143\143\305\276\073\163\346\214\263\007\374\017\013\100\041\054\000\233\100\013\000\253\000\145\367\332\067\105\260"
        "\054\030\373\055\062\062\122\324\263\261\011\011\001\214\262\004\320\303\152\200\272\061\103\000\003\376\007\362\342\004\232\100"
        "\013\000\377\306\344\311\223\304\353\327\233\072\231\231\035\304\176\303\104\140\040\236\027\140\013\100\166\054\170\220\327\060"
        "\320\270\054\200\174\253\001\306\151\307\004\001\310\056\053\251\246\117\237\076\001\025\000\262\147\317\036\361\244\237\246\014"
        "\046\000\043\042\354\265\044\023\046\114\160\076\171\343\162\360\340\001\151\215\124\323\245\113\027\026\200\333\061\105\000\205"
        "\205\201\027\000\156\005\236\061\143\006\215\034\131\336\044\051\057\057\247\166\355\062\304\076\013\017\017\027\302\013\104\102"
        "\103\000\133\362\051\167\125\234\166\130\000\066\156\010\000\301\145\261\111\223\046\322\347\237\217\012\012\106\215\152\050\043"
        "\051\057\057\117\274\130\026\373\014\057\323\014\324\163\003\103\104\000\171\126\003\214\325\016\013\300\306\055\001\040\047\117"
        "\236\244\261\143\053\251\242\342\363\046\003\326\377\143\322\017\373\253\131\263\146\126\243\075\350\174\332\306\207\005\240\020"
        "\063\004\060\120\073\205\205\366\114\266\033\002\100\216\037\077\156\065\234\012\361\264\335\140\006\215\037\147\176\317\270\037"
        "\075\200\305\213\027\073\237\062\060\201\114\144\065\122\115\327\256\054\000\327\003\001\340\211\263\272\361\134\312\162\113\000"
        "\310\246\115\233\104\117\003\335\347\061\143\106\007\035\230\360\113\113\113\173\272\237\060\356\307\043\315\003\035\010\100\126"
        "\043\325\270\052\200\221\133\172\120\367\125\061\332\141\001\330\250\020\000\262\153\327\056\312\314\314\024\007\025\176\056\316"
        "\250\225\225\143\304\020\301\044\052\053\053\105\243\037\061\142\204\270\104\232\236\236\376\364\254\017\242\243\243\305\231\337"
        "\215\343\206\005\240\020\026\200\215\052\001\040\230\030\034\067\156\034\105\105\105\211\206\204\125\164\270\237\076\065\065\205"
        "\132\266\114\325\016\266\043\076\076\236\142\142\142\236\216\363\075\240\313\337\257\137\077\072\174\370\260\153\117\005\146\001"
        "\050\044\072\325\236\305\325\053\200\101\332\121\051\000\117\260\154\166\316\234\071\326\201\326\125\310\000\215\015\335\152\200"
        "\206\246\033\154\007\316\372\230\344\153\333\266\255\350\356\343\211\275\156\037\053\266\000\344\165\122\011\352\342\242\000\162"
        "\255\006\030\355\016\337\067\034\023\004\040\273\275\124\065\230\325\126\055\200\372\171\360\340\001\135\274\170\121\334\114\203"
        "\263\353\241\103\207\264\203\355\070\173\366\154\100\156\361\365\045\020\200\254\106\252\351\326\315\105\001\224\157\316\245\156"
        "\126\003\324\015\013\300\106\267\000\070\317\302\002\120\010\013\300\206\005\140\116\102\110\000\121\332\141\001\330\260\000\314"
        "\011\013\100\041\046\010\240\376\223\145\164\121\124\124\304\002\060\044\207\016\035\224\326\110\065\335\272\165\163\117\000\043"
        "\054\001\164\265\032\240\156\130\000\066\054\000\163\142\212\000\272\167\167\125\000\335\255\006\330\114\073\054\000\033\026\200"
        "\071\141\001\050\204\005\140\303\002\060\047\241\041\200\115\226\000\276\263\032\241\146\242\123\364\013\300\373\001\223\072\300"
        "\173\356\130\000\146\004\153\020\144\065\122\215\253\002\050\333\324\215\272\174\027\251\035\026\200\015\013\300\234\230\043\200"
        "\356\054\000\267\303\002\340\170\207\005\240\020\026\200\015\013\300\234\204\210\000\272\122\316\167\021\332\211\322\054\200\333"
        "\267\157\321\260\141\103\005\262\042\250\002\357\271\147\001\230\021\010\300\204\143\042\067\227\005\340\172\352\013\100\047\046"
        "\011\240\366\361\103\072\167\353\017\072\170\145\027\035\274\354\056\207\256\354\246\363\267\117\323\243\307\265\316\117\327\237"
        "\372\002\320\111\210\010\300\076\350\131\000\146\010\340\310\325\275\064\174\143\216\250\115\347\357\302\225\220\143\015\005\313"
        "\267\344\322\251\033\107\234\255\320\233\220\020\100\351\246\056\324\171\245\125\000\315\260\000\154\114\020\300\351\233\107\051"
        "\177\165\202\325\040\321\050\325\323\353\307\144\272\164\367\234\263\065\372\302\002\120\210\176\001\334\266\166\366\060\355\364"
        "\357\337\137\253\000\236\130\377\115\334\136\054\255\221\112\206\176\333\215\056\135\272\344\154\225\236\330\002\220\327\111\045"
        "\271\271\271\054\000\267\003\001\310\136\070\251\232\342\142\275\002\250\173\374\110\274\253\121\126\043\225\144\315\377\214\262"
        "\262\262\304\325\031\135\201\000\144\065\122\115\217\036\256\013\040\114\073\054\000\033\335\002\170\144\011\100\126\037\325\164"
        "\134\142\117\012\317\237\077\337\331\062\365\011\011\001\014\267\004\320\311\332\341\272\141\001\330\004\223\000\144\165\014\024"
        "\331\113\354\175\200\367\044\350\012\013\100\041\054\000\233\342\342\142\355\002\220\325\107\065\036\001\340\201\230\272\022\042"
        "\002\310\261\166\370\147\332\061\101\000\262\167\316\253\246\244\044\224\005\360\354\170\360\010\000\257\305\322\225\303\207\017"
        "\111\153\244\232\036\075\172\260\000\334\016\013\300\216\055\000\171\215\124\222\275\344\123\026\200\203\253\002\030\266\061\207"
        "\072\256\370\114\073\054\000\033\023\004\040\253\217\152\262\026\263\000\074\270\054\200\316\326\016\377\124\073\054\000\033\026"
        "\200\015\013\340\031\054\000\005\261\005\120\252\235\222\222\022\315\002\250\225\326\107\065\131\213\077\061\100\000\207\245\065"
        "\122\115\136\036\013\300\365\100\000\145\145\245\332\031\060\200\005\000\114\021\200\254\106\252\161\125\000\103\067\166\242\354"
        "\025\237\150\207\005\140\143\202\000\144\365\121\115\046\013\340\051\054\000\005\141\001\330\061\107\000\037\263\000\034\102\104"
        "\000\366\244\217\136\001\224\005\020\171\061\137\004\013\300\306\034\001\310\152\353\017\362\172\067\204\274\274\074\067\005\320"
        "\321\332\341\037\153\247\231\001\002\030\061\242\114\073\003\007\016\060\100\000\362\032\251\304\024\001\310\152\244\232\374\174"
        "\067\005\260\241\043\145\175\373\261\166\130\000\066\046\010\100\126\037\325\164\130\364\021\013\300\201\005\240\040\054\000\073"
        "\054\200\147\011\021\001\144\133\073\374\043\355\260\000\154\314\020\200\274\106\052\141\001\074\303\125\001\014\261\004\220\151"
        "\355\160\335\230\041\200\021\332\031\070\160\040\013\300\302\034\001\310\353\244\222\374\374\174\026\200\333\201\000\312\313\107"
        "\150\147\320\040\375\002\220\325\107\065\355\015\020\300\221\043\207\245\065\122\215\002\001\174\250\035\026\200\215\031\002\220"
        "\327\110\045\355\027\175\310\002\160\140\001\050\010\013\300\016\013\340\131\102\102\000\203\327\147\123\207\345\037\152\307\014"
        "\001\224\053\100\136\144\017\046\010\100\126\037\325\264\133\150\202\000\216\130\065\221\325\060\220\310\217\203\372\270\054\200"
        "\054\153\207\377\133\073\046\010\140\344\310\162\355\014\032\064\310\000\001\310\153\244\222\166\013\377\155\204\000\144\065\122"
        "\015\013\100\101\114\021\300\340\301\054\000\300\002\170\206\253\002\030\144\011\240\275\265\303\165\303\002\260\141\001\330\260"
        "\000\236\341\262\000\062\255\006\370\201\166\130\000\066\046\010\100\126\037\325\144\054\374\200\005\340\300\002\120\020\010\140"
        "\324\250\221\317\061\162\244\172\006\017\036\314\002\260\060\105\000\046\034\023\005\005\054\000\327\043\023\200\016\130\000\066"
        "\246\012\100\007\256\012\140\340\372\016\324\156\371\277\264\303\002\260\061\101\000\262\372\250\046\143\341\277\130\000\016\356"
        "\012\340\147\113\000\313\254\235\256\031\026\200\215\021\002\220\324\107\065\155\027\260\000\074\260\000\024\304\026\300\050\355"
        "\014\031\302\002\000\346\010\100\136\047\225\024\024\024\270\051\200\366\326\016\177\137\073\046\010\340\363\317\107\151\307\014"
        "\001\310\153\244\222\266\013\336\327\056\200\243\107\217\110\153\244\232\236\075\135\026\100\206\265\303\165\023\311\002\020\230"
        "\040\000\131\175\124\223\316\002\170\012\013\100\101\130\000\166\130\000\317\302\002\360\211\367\032\005\013\300\146\310\220\041"
        "\054\000\213\364\005\357\261\000\034\024\010\100\336\050\003\101\333\006\242\133\000\167\356\334\246\212\212\317\265\063\164\250"
        "\011\002\220\327\122\045\146\010\340\250\264\106\252\351\331\263\247\173\002\030\140\011\240\355\122\253\021\152\206\005\140\143"
        "\202\000\144\365\121\115\132\065\013\300\103\023\027\300\273\002\026\200\015\013\300\046\255\372\135\026\200\203\313\002\150\147"
        "\355\160\273\021\352\204\005\140\143\206\000\344\065\122\211\011\002\070\166\354\230\264\106\252\161\165\016\240\304\022\100\272"
        "\265\303\165\243\133\000\167\357\336\245\261\143\307\322\350\321\025\132\061\341\251\300\262\372\250\306\004\001\234\072\165\112"
        "\132\043\325\270\372\156\300\222\237\063\254\035\376\216\166\164\013\240\266\266\226\276\374\362\277\322\002\250\244\127\257\136"
        "\142\077\204\207\207\153\024\200\274\106\052\151\123\375\216\166\001\334\274\171\123\132\043\325\144\145\145\371\274\057\202\120"
        "\000\366\353\240\165\011\000\131\271\162\205\264\000\052\151\337\276\235\330\017\211\211\211\124\127\127\347\154\231\272\260\000"
        "\236\345\311\223\047\364\325\127\137\112\353\244\022\034\013\330\027\205\205\205\316\226\275\070\054\000\077\162\362\344\111\032"
        "\063\146\264\264\010\052\300\003\040\243\243\243\304\176\300\033\141\161\000\252\016\013\340\371\374\362\313\057\322\132\251\002"
        "\103\302\260\260\060\261\057\046\117\236\354\154\325\213\323\140\001\024\377\334\226\322\226\376\123\073\046\010\000\077\273\252"
        "\252\312\332\361\220\200\172\160\260\143\037\200\231\063\147\072\133\245\066\020\200\254\076\252\151\123\375\117\043\004\120\123"
        "\123\103\123\247\176\041\255\227\333\140\002\060\055\055\115\354\007\110\140\333\266\155\316\126\275\070\015\027\300\072\113\000"
        "\113\254\235\256\031\023\004\200\134\273\166\315\062\355\044\321\023\120\011\336\011\030\021\021\041\366\101\164\164\064\235\071"
        "\163\306\331\042\265\021\002\220\324\107\065\255\253\314\020\000\162\374\370\061\032\073\266\122\132\067\067\301\073\001\075\147"
        "\377\214\214\014\272\167\357\236\263\105\057\016\013\240\021\301\013\041\124\026\034\135\377\204\204\170\361\371\301\304\211\023"
        "\265\164\377\021\026\200\074\033\067\156\220\326\316\055\360\176\210\310\310\110\261\017\040\201\325\253\127\073\133\322\260\370"
        "\040\200\164\153\207\277\255\035\223\004\200\340\305\220\023\047\116\220\026\047\220\014\037\076\354\351\044\017\310\311\311\021"
        "\227\044\165\305\177\001\310\353\352\057\255\253\336\066\112\000\020\362\346\315\233\251\262\162\214\264\216\201\244\270\270\277"
        "\350\005\172\216\011\274\040\324\327\023\102\203\005\320\337\022\100\033\153\207\353\306\064\001\040\177\374\361\207\230\170\221"
        "\025\051\020\224\224\024\123\134\134\334\323\102\247\244\244\320\211\023\047\234\237\256\047\020\200\254\076\252\151\145\230\000"
        "\074\331\271\163\047\215\033\067\126\132\317\306\122\121\121\041\126\375\171\316\374\000\047\004\334\250\346\153\130\000\001\312"
        "\137\177\375\345\114\312\124\010\373\007\002\164\371\063\063\073\210\153\375\236\102\247\247\247\013\341\350\016\013\340\305\071"
        "\164\350\020\225\225\225\211\106\053\253\257\077\140\005\150\253\126\255\236\216\371\101\357\336\275\351\306\215\033\316\117\365"
        "\055\076\012\340\055\355\230\052\000\004\105\300\214\154\217\036\271\204\347\264\313\012\370\042\160\260\014\033\066\224\072\166"
        "\314\246\250\050\373\122\037\100\301\213\212\212\350\322\245\113\316\117\323\033\026\100\303\202\113\306\045\045\045\324\253\127"
        "\117\161\313\256\254\346\057\002\307\304\200\001\045\142\202\317\063\001\014\360\373\361\343\307\373\064\351\347\235\240\023\100"
        "\204\301\002\100\060\006\303\065\341\354\354\154\161\151\046\067\267\273\270\106\013\041\100\016\365\173\010\350\312\341\240\300"
        "\231\276\177\377\176\242\033\207\356\175\175\273\003\234\365\177\372\351\047\243\076\263\055\000\171\215\124\322\252\352\055\261"
        "\217\114\025\000\202\205\132\213\027\057\246\066\155\332\120\273\166\031\342\341\235\070\223\343\101\236\070\046\074\075\004\374"
        "\212\343\003\177\137\126\126\112\205\205\175\254\023\101\107\112\112\112\172\356\170\100\217\260\153\327\256\264\177\377\176\347"
        "\047\370\237\006\013\240\337\117\351\324\172\361\133\332\061\135\000\236\334\277\177\237\066\154\330\100\175\373\366\025\147\162"
        "\214\327\142\143\143\305\104\136\213\026\055\054\222\051\041\041\101\374\135\175\253\173\300\327\143\234\267\146\315\032\161\215"
        "\331\264\100\000\255\255\006\250\233\226\363\315\027\200\047\350\041\056\137\276\134\064\136\064\142\034\027\230\333\361\034\023"
        "\150\350\370\163\114\114\314\163\303\076\017\070\126\206\016\035\112\333\267\157\027\113\322\003\221\000\011\340\115\145\004\213"
        "\000\074\101\217\340\374\371\363\264\166\355\132\232\060\141\002\165\353\326\115\234\011\320\370\121\144\234\355\343\343\343\305"
        "\337\341\040\256\254\254\024\227\162\160\175\337\344\317\050\004\040\075\026\324\022\114\002\360\004\367\156\140\150\260\142\305"
        "\012\361\106\237\116\235\072\211\161\075\032\070\216\007\234\020\040\005\364\374\260\322\363\213\057\276\240\215\033\067\322\345"
        "\313\227\003\176\331\327\007\001\244\131\073\134\336\050\125\022\154\002\360\016\266\373\341\303\207\142\334\206\131\333\133\267"
        "\156\211\337\343\357\164\254\351\367\067\266\000\344\065\122\111\313\371\157\006\235\000\274\203\272\077\170\360\100\364\364\160"
        "\074\334\271\163\347\351\061\341\366\161\356\223\000\132\131\073\134\067\301\056\200\246\022\010\100\126\037\325\244\066\001\001"
        "\350\214\017\002\150\143\355\360\067\264\023\221\362\061\013\300\200\330\002\220\327\110\045\251\363\337\140\001\064\042\054\000"
        "\216\137\141\001\064\215\030\055\200\226\022\130\000\146\204\005\320\064\322\140\001\024\375\324\332\152\200\257\153\207\005\140"
        "\106\040\000\131\175\124\223\072\377\165\026\100\043\322\160\001\254\265\004\260\310\332\351\232\141\001\230\021\041\000\111\175"
        "\124\223\372\015\013\240\061\141\001\160\374\012\013\240\151\244\301\002\350\273\266\025\245\056\172\115\073\054\000\063\002\001"
        "\310\352\243\232\024\026\100\243\302\002\340\370\025\163\004\360\032\013\240\021\141\001\160\374\012\013\240\151\304\107\001\274"
        "\252\035\026\200\031\261\005\040\257\221\112\130\000\215\113\203\005\120\150\011\040\005\073\134\063\054\000\063\002\001\310\352"
        "\243\232\026\054\200\106\245\341\002\130\143\011\140\241\265\323\065\023\321\202\005\140\102\204\000\044\365\121\115\213\171\054"
        "\200\306\204\005\300\361\053\054\200\246\021\037\004\320\322\332\341\257\150\207\005\140\106\154\001\310\153\244\222\344\171\257"
        "\262\000\032\221\006\013\240\217\045\200\026\326\016\327\015\013\300\214\100\000\262\372\250\206\005\320\270\370\040\200\124\153"
        "\207\277\254\235\210\026\037\261\000\014\210\055\000\171\215\124\222\074\357\025\026\100\043\302\002\340\370\025\026\100\323\010"
        "\013\200\343\127\114\021\100\163\026\100\243\342\203\000\122\050\171\341\113\332\141\001\230\021\010\100\126\037\325\064\237\367"
        "\062\013\240\021\361\115\000\013\254\235\256\031\026\200\031\021\002\220\324\107\065\315\347\262\000\032\023\026\000\307\257\260"
        "\000\232\106\130\000\034\277\142\212\000\222\130\000\215\112\203\005\320\173\115\013\152\276\340\037\332\141\001\230\021\010\100"
        "\126\037\325\044\315\175\211\005\320\210\064\130\000\170\046\240\254\000\252\211\310\370\067\013\300\200\134\256\071\057\255\217"
        "\152\222\146\163\017\240\061\151\260\000\106\157\355\055\055\200\152\342\307\277\116\141\021\054\000\335\031\377\153\077\151\175"
        "\124\223\060\205\357\005\150\114\032\054\200\271\373\306\111\013\240\203\250\156\357\213\367\244\161\364\344\367\113\333\010\017"
        "\343\220\325\106\065\161\043\355\067\003\365\350\321\303\331\072\216\057\151\260\000\066\237\131\045\055\200\016\222\346\275\114"
        "\343\146\217\164\266\214\243\062\027\356\374\111\035\127\174\046\255\213\016\242\372\274\043\004\120\132\132\352\154\041\307\227"
        "\064\130\000\347\157\237\022\057\142\220\025\101\007\051\163\336\244\337\116\357\160\266\216\243\042\127\357\135\064\146\062\330"
        "\103\244\063\047\064\175\372\164\147\053\071\276\244\301\002\250\173\374\210\162\127\305\112\213\240\213\266\213\337\243\003\227"
        "\166\072\133\310\161\063\070\001\164\373\076\112\132\007\135\240\047\030\326\354\123\041\200\365\353\327\073\133\312\361\045\015"
        "\026\000\362\365\336\317\245\205\320\111\233\045\157\323\167\307\346\012\101\161\002\237\047\117\036\323\226\063\077\120\373\345"
        "\377\226\356\177\235\304\175\156\217\377\243\242\242\350\334\271\163\316\026\163\174\211\117\002\070\164\145\027\045\343\006\014"
        "\111\061\164\202\005\041\043\267\364\240\363\167\116\073\133\312\011\104\256\335\273\104\123\166\016\022\017\336\220\355\167\335"
        "\104\165\171\137\010\240\103\207\016\364\350\021\237\000\374\211\117\002\170\374\244\216\362\127\047\112\213\141\002\351\113\337"
        "\245\157\366\117\244\232\332\333\316\026\163\374\111\355\343\207\264\342\350\154\312\374\366\103\351\176\066\201\304\231\126\367"
        "\077\332\356\376\117\235\072\325\331\162\216\257\361\111\000\310\367\307\277\221\026\304\044\160\340\126\037\374\202\156\076\270"
        "\352\154\065\247\041\201\070\127\036\233\103\135\276\153\046\335\257\046\021\063\340\155\321\370\303\303\303\151\337\276\175\316"
        "\047\340\370\032\237\005\120\123\173\207\072\256\370\124\132\024\323\150\153\365\010\246\356\032\112\107\256\356\025\113\127\071"
        "\177\117\235\325\253\073\165\343\010\175\375\133\005\145\175\373\221\164\077\032\307\334\227\050\274\271\275\044\274\123\247\116"
        "\274\050\254\021\361\131\000\310\212\143\263\345\205\061\024\314\133\364\372\061\231\026\035\372\257\165\260\037\026\007\175\050"
        "\007\023\173\347\156\377\041\352\130\274\056\103\074\137\137\266\337\114\045\146\220\175\366\007\313\227\057\167\076\025\307\237"
        "\370\045\200\173\217\356\032\075\027\360\277\300\012\266\374\325\011\342\214\267\373\302\106\272\363\360\246\363\251\232\166\356"
        "\077\252\021\053\370\060\107\122\270\266\245\121\153\072\174\041\151\326\313\024\036\157\077\030\066\065\065\225\152\152\152\234"
        "\117\310\361\047\176\011\000\331\173\161\113\320\235\071\274\301\325\203\066\113\336\242\376\353\322\205\020\160\271\013\067\271"
        "\340\222\342\023\353\277\140\014\266\033\075\234\353\367\057\323\366\363\353\150\336\276\011\064\170\103\026\245\057\175\307\310"
        "\053\070\276\322\254\373\173\117\317\376\213\027\057\166\076\065\307\337\370\055\000\034\150\323\166\017\227\026\051\230\201\024"
        "\262\276\375\230\112\176\316\240\311\073\006\322\322\303\323\151\333\271\265\364\347\315\343\164\373\341\015\143\346\022\040\251"
        "\073\017\157\211\005\072\073\317\257\247\345\107\147\212\371\216\301\353\263\250\363\312\160\361\071\144\237\057\230\211\037\363"
        "\006\205\205\331\215\077\073\073\233\356\335\273\347\354\015\216\277\361\133\000\010\206\002\246\055\015\165\023\234\105\173\254"
        "\212\243\101\353\063\305\335\221\020\140\325\201\311\342\312\010\356\225\100\027\033\023\152\130\057\177\351\356\071\272\122\163"
        "\121\234\211\161\065\002\362\300\054\073\272\342\017\353\356\133\277\336\023\177\306\020\004\377\037\137\207\245\266\370\276\213"
        "\326\367\377\171\363\030\035\270\274\203\176\071\273\232\176\070\061\237\026\034\234\112\137\356\031\101\225\333\012\151\310\206"
        "\154\061\214\311\130\366\176\223\154\350\062\022\147\130\135\377\204\117\104\343\217\214\214\244\235\073\171\005\150\040\322\050"
        "\001\040\147\156\235\240\166\313\376\045\055\132\050\202\006\331\162\321\353\324\172\361\233\142\225\042\326\046\144\054\173\217"
        "\332\057\377\200\072\054\377\120\364\056\160\025\005\277\342\162\045\376\036\015\031\137\227\266\344\237\342\373\360\375\115\241"
        "\273\036\060\276\171\231\042\333\333\153\376\301\270\161\343\350\311\223\340\034\242\231\226\106\013\000\331\163\161\163\320\116"
        "\052\061\206\123\365\022\105\165\267\127\374\201\316\235\073\363\304\137\000\023\020\001\040\077\036\137\104\055\026\006\367\244"
        "\040\143\026\311\325\057\121\164\241\175\273\057\110\112\112\242\077\377\374\323\071\342\070\201\110\300\004\200\054\375\175\246"
        "\125\064\063\327\215\063\301\205\150\374\105\377\244\317\234\111\277\350\350\150\332\261\203\157\377\016\164\002\052\000\254\310"
        "\232\263\171\012\045\315\017\215\211\051\306\045\252\376\101\321\170\320\207\323\370\233\065\153\106\077\374\360\203\163\224\161"
        "\002\231\200\012\000\201\004\146\374\060\221\222\346\260\004\030\337\301\311\003\327\372\075\227\373\042\042\042\150\345\312\225"
        "\074\351\347\122\002\056\000\104\364\004\276\235\116\311\323\336\224\026\231\141\144\044\176\365\012\105\244\075\233\355\307\175"
        "\376\077\376\370\243\163\124\161\334\210\053\002\100\140\354\157\127\055\243\344\221\377\242\346\326\170\116\126\160\206\021\124"
        "\377\203\342\053\336\240\260\130\373\072\077\300\204\337\266\155\333\234\243\211\343\126\134\023\200\047\273\167\357\242\166\103"
        "\022\171\110\300\110\101\227\077\272\317\273\364\131\204\175\157\077\310\311\311\241\023\047\116\070\107\020\307\315\270\056\000"
        "\344\312\225\053\324\177\124\157\212\037\373\272\364\040\140\102\223\204\051\257\122\144\372\207\117\047\373\300\350\321\243\351"
        "\316\235\073\316\221\303\161\073\112\004\200\340\221\115\263\147\317\242\370\334\217\051\161\006\137\052\014\145\360\066\237\250"
        "\202\167\305\013\136\074\015\077\056\056\216\126\255\132\305\223\175\212\243\114\000\236\354\335\273\227\272\344\165\242\330\201"
        "\157\123\322\067\074\054\010\051\346\377\203\142\113\337\174\272\246\037\340\211\076\205\205\205\164\362\344\111\347\010\341\250"
        "\214\162\001\040\367\357\337\247\252\252\052\152\321\061\206\342\312\337\244\244\052\026\101\223\306\252\157\174\345\033\024\321"
        "\366\337\117\057\357\201\326\255\133\213\131\176\176\240\247\276\150\021\200\047\347\317\237\247\341\245\303\051\262\325\107\102"
        "\004\070\103\110\017\040\046\050\301\152\076\334\302\353\075\316\307\335\174\023\047\116\244\233\067\103\343\141\054\046\107\253"
        "\000\020\214\371\366\354\331\103\275\172\365\244\310\324\217\105\027\221\127\022\006\071\226\310\343\107\277\111\021\155\076\174"
        "\356\214\217\206\077\144\310\020\072\176\374\270\123\175\216\356\150\027\200\047\020\301\356\335\273\305\170\260\131\162\030\105"
        "\367\175\207\022\277\174\105\134\043\226\036\144\214\161\140\162\067\246\370\155\212\110\261\037\330\351\001\013\172\206\015\033"
        "\306\015\337\300\030\043\000\117\260\212\020\023\205\375\372\365\243\146\061\021\024\231\371\201\170\003\014\117\030\032\212\065"
        "\276\117\030\367\006\105\345\374\213\302\243\236\135\313\007\061\061\061\064\142\304\010\321\360\171\166\337\314\030\047\000\117"
        "\040\002\054\006\231\062\145\012\045\267\110\246\360\244\217\050\252\327\273\024\077\361\165\161\263\210\364\140\144\324\140\365"
        "\312\160\015\077\246\357\273\024\216\263\175\275\156\076\110\117\117\247\231\063\147\212\071\036\156\370\146\307\130\001\324\017"
        "\236\375\266\172\365\152\312\313\313\023\067\207\204\045\174\114\121\371\226\014\306\277\306\127\020\124\201\063\375\224\327\304"
        "\252\275\210\026\177\157\364\030\337\227\224\224\320\057\277\374\102\017\037\076\164\052\307\061\075\101\041\200\372\101\257\140"
        "\306\214\031\224\225\225\045\016\072\134\123\156\326\355\075\212\035\361\246\170\156\034\367\016\002\104\365\113\224\370\365\313"
        "\142\370\025\225\377\336\323\027\161\324\007\143\373\356\335\273\323\374\371\363\351\302\205\013\116\205\070\301\224\240\023\200"
        "\047\165\165\165\164\354\330\061\232\065\153\026\165\351\322\105\310\000\353\311\043\132\132\103\005\253\167\020\127\141\011\301"
        "\072\200\171\022\261\341\044\315\302\365\372\327\355\263\174\233\017\351\263\310\347\307\364\000\017\346\310\317\317\247\205\013"
        "\027\322\331\263\147\271\213\037\344\011\132\001\324\017\144\160\352\324\051\161\120\242\033\332\274\171\163\161\260\206\105\175"
        "\102\341\251\037\122\124\267\367\051\166\310\133\224\060\311\032\062\314\345\041\003\300\244\052\306\361\261\303\255\063\174\336"
        "\173\024\321\352\103\373\145\233\136\135\173\320\252\125\053\032\076\174\270\270\057\037\343\172\314\317\160\232\106\232\204\000"
        "\352\007\147\044\254\064\074\160\340\000\315\236\075\233\172\365\352\045\316\132\141\141\141\342\340\306\114\165\104\153\113\012"
        "\075\254\141\303\240\267\305\031\117\334\233\200\271\204\246\330\133\100\127\176\346\313\024\077\376\165\212\035\372\266\130\203"
        "\217\205\071\141\321\237\074\167\215\336\003\366\023\326\345\027\027\027\013\241\142\006\377\301\203\007\316\336\345\064\265\064"
        "\071\001\310\342\021\302\222\045\113\250\274\274\234\062\063\063\051\066\066\326\226\202\163\340\013\061\130\147\101\134\316\212"
        "\056\172\107\234\031\161\367\142\302\264\127\305\315\053\111\363\060\277\140\130\357\001\317\131\370\306\332\056\153\373\022\277"
        "\174\325\156\344\145\157\122\114\377\167\304\273\363\043\261\020\307\152\350\365\033\170\175\360\371\343\343\343\305\223\166\053"
        "\053\053\305\031\036\303\252\332\132\176\221\152\250\044\044\004\340\035\364\022\256\136\275\052\026\036\125\127\127\213\133\120"
        "\161\205\001\153\323\321\133\370\133\143\211\260\004\021\377\261\030\116\104\266\377\100\064\056\114\214\105\027\376\223\142\007"
        "\274\055\144\201\311\062\064\300\204\057\136\245\104\113\032\211\377\265\370\012\274\042\046\047\223\146\276\142\141\375\072\013"
        "\277\177\366\053\346\051\320\003\111\234\156\201\257\267\276\017\322\111\264\376\235\370\011\257\211\007\145\304\225\275\045\156"
        "\236\202\230\160\006\027\215\273\303\007\226\260\076\022\223\240\141\221\136\333\053\001\302\303\345\071\364\210\360\134\375\245"
        "\113\227\322\376\375\373\351\326\255\133\316\136\341\204\142\102\122\000\377\127\320\123\270\164\351\222\170\337\374\367\337\177"
        "\117\323\246\115\243\322\322\122\052\050\050\240\366\355\333\123\142\142\342\163\275\206\377\211\325\275\026\267\273\132\204\065"
        "\373\124\364\060\104\267\333\233\146\300\032\173\343\353\360\365\222\156\171\103\300\135\165\311\311\311\242\167\203\106\216\236"
        "\316\364\351\323\305\315\066\207\017\037\026\317\144\340\313\163\034\357\260\000\032\020\364\030\320\055\306\013\051\256\135\273"
        "\106\107\217\036\245\055\133\266\010\111\240\007\201\206\066\176\374\170\052\053\053\023\053\030\163\163\163\305\145\312\166\355"
        "\332\121\233\066\155\050\045\045\105\074\342\012\147\141\074\341\026\022\361\006\227\324\060\366\306\004\046\336\172\233\226\226"
        "\046\276\077\073\073\133\364\116\060\271\211\106\215\233\150\276\376\372\153\132\264\150\221\170\122\056\036\233\205\113\243\067"
        "\156\334\020\353\045\370\316\072\116\303\103\364\377\001\001\016\121\026\237\213\202\166\000\000\000\000\111\105\116\104\256\102"
        "\140\202");
        defaultValue.setByteString(tmpImagePNG, OpcUa_False);
    }
    pImagePNG->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pImagePNG, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImagePNG;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(-1);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(-1);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        defaultValue.setInt32(0);
    }
    pInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(-1);
    pInt64->setDataTypeId(OpcUaId_Int64);
    {
        defaultValue.setInt64(0);
    }
    pInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarInteger(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInteger = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Integer, getTypeNamespace()));
    pInteger->setBrowseName(UaQualifiedName("Integer", getTypeNamespace()));
    pInteger->setDisplayName(UaLocalizedText("", "Integer"));
    pInteger->setAccessLevel(3);
    pInteger->setUserAccessLevel(3);
    pInteger->setValueRank(-1);
    pInteger->setDataTypeId(OpcUaId_Integer);
        defaultValue.clear();
    pInteger->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pInteger, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInteger;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarLocalizedText(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pLocalizedText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_LocalizedText, getTypeNamespace()));
    pLocalizedText->setBrowseName(UaQualifiedName("LocalizedText", getTypeNamespace()));
    pLocalizedText->setDisplayName(UaLocalizedText("", "LocalizedText"));
    pLocalizedText->setAccessLevel(3);
    pLocalizedText->setUserAccessLevel(3);
    pLocalizedText->setValueRank(-1);
    pLocalizedText->setDataTypeId(OpcUaId_LocalizedText);
        defaultValue.setLocalizedText(UaLocalizedText());
    pLocalizedText->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pLocalizedText, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pLocalizedText;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarNodeId(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNodeId = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_NodeId, getTypeNamespace()));
    pNodeId->setBrowseName(UaQualifiedName("NodeId", getTypeNamespace()));
    pNodeId->setDisplayName(UaLocalizedText("", "NodeId"));
    pNodeId->setAccessLevel(3);
    pNodeId->setUserAccessLevel(3);
    pNodeId->setValueRank(-1);
    pNodeId->setDataTypeId(OpcUaId_NodeId);
        defaultValue.setNodeId(UaNodeId());
    pNodeId->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNodeId, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNodeId;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarNumber(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pNumber = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Number, getTypeNamespace()));
    pNumber->setBrowseName(UaQualifiedName("Number", getTypeNamespace()));
    pNumber->setDisplayName(UaLocalizedText("", "Number"));
    pNumber->setAccessLevel(3);
    pNumber->setUserAccessLevel(3);
    pNumber->setValueRank(-1);
    pNumber->setDataTypeId(OpcUaId_Number);
    defaultValue.clear();
    pNumber->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pNumber, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNumber;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarQualifiedName(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pQualifiedName = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_QualifiedName, getTypeNamespace()));
    pQualifiedName->setBrowseName(UaQualifiedName("QualifiedName", getTypeNamespace()));
    pQualifiedName->setDisplayName(UaLocalizedText("", "QualifiedName"));
    pQualifiedName->setAccessLevel(3);
    pQualifiedName->setUserAccessLevel(3);
    pQualifiedName->setValueRank(-1);
    pQualifiedName->setDataTypeId(OpcUaId_QualifiedName);
        defaultValue.setQualifiedName(UaQualifiedName());
    pQualifiedName->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pQualifiedName, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pQualifiedName;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(-1);
    pSByte->setDataTypeId(OpcUaId_SByte);
    {
        defaultValue.setSByte(0);
    }
    pSByte->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pString = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_String, getTypeNamespace()));
    pString->setBrowseName(UaQualifiedName("String", getTypeNamespace()));
    pString->setDisplayName(UaLocalizedText("", "String"));
    pString->setAccessLevel(3);
    pString->setUserAccessLevel(3);
    pString->setValueRank(-1);
    pString->setDataTypeId(OpcUaId_String);
        defaultValue.setString("");
    pString->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(-1);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        defaultValue.setUInt16(0);
    }
    pUInt16->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(-1);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pUInt32->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(-1);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
    {
        defaultValue.setUInt64(0);
    }
    pUInt64->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInteger(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInteger = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInteger, getTypeNamespace()));
    pUInteger->setBrowseName(UaQualifiedName("UInteger", getTypeNamespace()));
    pUInteger->setDisplayName(UaLocalizedText("", "UInteger"));
    pUInteger->setAccessLevel(3);
    pUInteger->setUserAccessLevel(3);
    pUInteger->setValueRank(-1);
    pUInteger->setDataTypeId(OpcUaId_UInteger);
        defaultValue.clear();
    pUInteger->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pUInteger, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInteger;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarVariant(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pVariant = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Variant, getTypeNamespace()));
    pVariant->setBrowseName(UaQualifiedName("Variant", getTypeNamespace()));
    pVariant->setDisplayName(UaLocalizedText("", "Variant"));
    pVariant->setAccessLevel(3);
    pVariant->setUserAccessLevel(3);
    pVariant->setValueRank(-1);
    pVariant->setDataTypeId(OpcUaId_BaseDataType);
        defaultValue.clear();
    pVariant->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pVariant, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pVariant;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticAll_ProfilesScalarXmlElement(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pXmlElement = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_XmlElement, getTypeNamespace()));
    pXmlElement->setBrowseName(UaQualifiedName("XmlElement", getTypeNamespace()));
    pXmlElement->setDisplayName(UaLocalizedText("", "XmlElement"));
    pXmlElement->setAccessLevel(3);
    pXmlElement->setUserAccessLevel(3);
    pXmlElement->setValueRank(-1);
    pXmlElement->setDataTypeId(OpcUaId_XmlElement);
        { UaByteString tmpXmlElement; defaultValue.setXmlElement(tmpXmlElement, OpcUa_False); };
    pXmlElement->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pXmlElement, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pXmlElement;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_Profile(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDA_Profile = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile, getTypeNamespace()));
    pDA_Profile->setBrowseName(UaQualifiedName("DA_Profile", getTypeNamespace()));
    pDA_Profile->setDisplayName(UaLocalizedText("", "DA Profile"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pDA_Profile, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDA_Profile;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pAnalogType = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getTypeNamespace()));
    pAnalogType->setBrowseName(UaQualifiedName("AnalogType", getTypeNamespace()));
    pAnalogType->setDisplayName(UaLocalizedText("", "AnalogType"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pAnalogType, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAnalogType;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(-1);
    pByte->setDataTypeId(OpcUaId_Byte);
    {
        defaultValue.setByte(0);
    }
    pByte->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Byte_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pByte);
    reference.setTargetNode(pEURange);
    pByte->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pByte->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(-1);
    pDouble->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pDouble->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Double_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pDouble);
    reference.setTargetNode(pEURange);
    pDouble->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pDouble->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(-1);
    pFloat->setDataTypeId(OpcUaId_Float);
    {
        defaultValue.setFloat(0);
    }
    pFloat->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Float_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pFloat);
    reference.setTargetNode(pEURange);
    pFloat->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pFloat->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(-1);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pInt16->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int16_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt16);
    reference.setTargetNode(pEURange);
    pInt16->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pInt16->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(-1);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        defaultValue.setInt32(0);
    }
    pInt32->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int32_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt32);
    reference.setTargetNode(pEURange);
    pInt32->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pInt32->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(-1);
    pInt64->setDataTypeId(OpcUaId_Int64);
    {
        defaultValue.setInt64(0);
    }
    pInt64->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int64_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt64);
    reference.setTargetNode(pEURange);
    pInt64->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pInt64->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(-1);
    pSByte->setDataTypeId(OpcUaId_SByte);
    {
        defaultValue.setSByte(0);
    }
    pSByte->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_SByte_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pSByte);
    reference.setTargetNode(pEURange);
    pSByte->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pSByte->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(-1);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        defaultValue.setUInt16(0);
    }
    pUInt16->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt16_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt16);
    reference.setTargetNode(pEURange);
    pUInt16->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pUInt16->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(-1);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pUInt32->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt32_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt32);
    reference.setTargetNode(pEURange);
    pUInt32->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pUInt32->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(-1);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
    {
        defaultValue.setUInt64(0);
    }
    pUInt64->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt64_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt64);
    reference.setTargetNode(pEURange);
    pUInt64->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pUInt64->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeVariableWithEngineeringUnits(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pVariableWithEngineeringUnits = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithEngineeringUnits, getTypeNamespace()));
    pVariableWithEngineeringUnits->setBrowseName(UaQualifiedName("VariableWithEngineeringUnits", getTypeNamespace()));
    pVariableWithEngineeringUnits->setDisplayName(UaLocalizedText("", "VariableWithEngineeringUnits"));
    pVariableWithEngineeringUnits->setAccessLevel(3);
    pVariableWithEngineeringUnits->setUserAccessLevel(3);
    pVariableWithEngineeringUnits->setValueRank(-1);
    pVariableWithEngineeringUnits->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pVariableWithEngineeringUnits->setValue(defaultValue);

    UaBase::FullReference reference;

    // EngineeringUnits
    UaBase::Variable *pEngineeringUnits = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithEngineeringUnits_EngineeringUnits, getTypeNamespace()));
    pEngineeringUnits->setBrowseName(UaQualifiedName("EngineeringUnits", 0));
    pEngineeringUnits->setDisplayName(UaLocalizedText("", "EngineeringUnits"));
    pEngineeringUnits->setAccessLevel(1);
    pEngineeringUnits->setUserAccessLevel(1);
    pEngineeringUnits->setValueRank(-1);
    pEngineeringUnits->setDataTypeId(OpcUaId_EUInformation);
    {
        UaEUInformation valEngineeringUnits = UaEUInformation(
            "http://www.opcfoundation.org/UA/units/un/cefact",
            4408652,
            UaLocalizedText("en-US", "\302\260C"),
            UaLocalizedText("en-US", "degree Celsius"));
        valEngineeringUnits.toVariant(defaultValue);
    }
    pEngineeringUnits->setValue(defaultValue);
    pEngineeringUnits->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pVariableWithEngineeringUnits);
    reference.setTargetNode(pEngineeringUnits);
    pVariableWithEngineeringUnits->addFullReference(reference);
    pEngineeringUnits->addFullReference(reference);
    pEngineeringUnits->setParentNodeId(pVariableWithEngineeringUnits->nodeId());

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithEngineeringUnits_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pVariableWithEngineeringUnits);
    reference.setTargetNode(pEURange);
    pVariableWithEngineeringUnits->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pVariableWithEngineeringUnits->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pVariableWithEngineeringUnits, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEngineeringUnits;
    delete pEURange;
    delete pVariableWithEngineeringUnits;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeVariableWithInstrumentRange(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pVariableWithInstrumentRange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithInstrumentRange, getTypeNamespace()));
    pVariableWithInstrumentRange->setBrowseName(UaQualifiedName("VariableWithInstrumentRange", getTypeNamespace()));
    pVariableWithInstrumentRange->setDisplayName(UaLocalizedText("", "VariableWithInstrumentRange"));
    pVariableWithInstrumentRange->setAccessLevel(3);
    pVariableWithInstrumentRange->setUserAccessLevel(3);
    pVariableWithInstrumentRange->setValueRank(-1);
    pVariableWithInstrumentRange->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pVariableWithInstrumentRange->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithInstrumentRange_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pVariableWithInstrumentRange);
    reference.setTargetNode(pEURange);
    pVariableWithInstrumentRange->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pVariableWithInstrumentRange->nodeId());

    // InstrumentRange
    UaBase::Variable *pInstrumentRange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithInstrumentRange_InstrumentRange, getTypeNamespace()));
    pInstrumentRange->setBrowseName(UaQualifiedName("InstrumentRange", 0));
    pInstrumentRange->setDisplayName(UaLocalizedText("", "InstrumentRange"));
    pInstrumentRange->setAccessLevel(1);
    pInstrumentRange->setUserAccessLevel(1);
    pInstrumentRange->setValueRank(-1);
    pInstrumentRange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valInstrumentRange = UaRange(0, 50);
        valInstrumentRange.toVariant(defaultValue);
    }
    pInstrumentRange->setValue(defaultValue);
    pInstrumentRange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pVariableWithInstrumentRange);
    reference.setTargetNode(pInstrumentRange);
    pVariableWithInstrumentRange->addFullReference(reference);
    pInstrumentRange->addFullReference(reference);
    pInstrumentRange->setParentNodeId(pVariableWithInstrumentRange->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pVariableWithInstrumentRange, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pInstrumentRange;
    delete pVariableWithInstrumentRange;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_Arrays(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pAnalogType_Arrays = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays, getTypeNamespace()));
    pAnalogType_Arrays->setBrowseName(UaQualifiedName("AnalogType_Arrays", getTypeNamespace()));
    pAnalogType_Arrays->setDisplayName(UaLocalizedText("", "AnalogType Arrays"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pAnalogType_Arrays, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAnalogType_Arrays;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pDouble->setArrayDimensions(arrayDimensions);
    pDouble->setDataTypeId(OpcUaId_Double);
    {
        UaDoubleArray valDoubleArray;
        defaultValue.setDoubleArray(valDoubleArray);
    }
    pDouble->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Double_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pDouble);
    reference.setTargetNode(pEURange);
    pDouble->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pDouble->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pFloat->setArrayDimensions(arrayDimensions);
    pFloat->setDataTypeId(OpcUaId_Float);
    {
        UaFloatArray valFloatArray;
        defaultValue.setFloatArray(valFloatArray);
    }
    pFloat->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Float_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pFloat);
    reference.setTargetNode(pEURange);
    pFloat->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pFloat->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt16->setArrayDimensions(arrayDimensions);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        UaInt16Array valInt16Array;
        defaultValue.setInt16Array(valInt16Array);
    }
    pInt16->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int16_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt16);
    reference.setTargetNode(pEURange);
    pInt16->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pInt16->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pInt32->setArrayDimensions(arrayDimensions);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        UaInt32Array valInt32Array;
        defaultValue.setInt32Array(valInt32Array);
    }
    pInt32->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int32_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt32);
    reference.setTargetNode(pEURange);
    pInt32->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pInt32->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt16->setArrayDimensions(arrayDimensions);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        UaUInt16Array valUInt16Array;
        defaultValue.setUInt16Array(valUInt16Array);
    }
    pUInt16->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt16_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt16);
    reference.setTargetNode(pEURange);
    pUInt16->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pUInt16->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 10;
    pUInt32->setArrayDimensions(arrayDimensions);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        UaUInt32Array valUInt32Array;
        defaultValue.setUInt32Array(valUInt32Array);
    }
    pUInt32->setValue(defaultValue);

    UaBase::FullReference reference;

    // EURange
    UaBase::Variable *pEURange = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt32_EURange, getTypeNamespace()));
    pEURange->setBrowseName(UaQualifiedName("EURange", 0));
    pEURange->setDisplayName(UaLocalizedText("", "EURange"));
    pEURange->setAccessLevel(1);
    pEURange->setUserAccessLevel(1);
    pEURange->setValueRank(-1);
    pEURange->setDataTypeId(OpcUaId_Range);
    {
        UaRange valEURange = UaRange(0, 50);
        valEURange.toVariant(defaultValue);
    }
    pEURange->setValue(defaultValue);
    pEURange->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt32);
    reference.setTargetNode(pEURange);
    pUInt32->addFullReference(reference);
    pEURange->addFullReference(reference);
    pEURange->setParentNodeId(pUInt32->nodeId());
    OpcUa::AnalogItemType *pVariableType = new OpcUa::AnalogItemType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEURange;
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItem(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDataItem = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getTypeNamespace()));
    pDataItem->setBrowseName(UaQualifiedName("DataItem", getTypeNamespace()));
    pDataItem->setDisplayName(UaLocalizedText("", "DataItem"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pDataItem, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDataItem;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(-1);
    pByte->setDataTypeId(OpcUaId_Byte);
    {
        defaultValue.setByte(0);
    }
    pByte->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemDateTime(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDateTime = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_DateTime, getTypeNamespace()));
    pDateTime->setBrowseName(UaQualifiedName("DateTime", getTypeNamespace()));
    pDateTime->setDisplayName(UaLocalizedText("", "DateTime"));
    pDateTime->setAccessLevel(3);
    pDateTime->setUserAccessLevel(3);
    pDateTime->setValueRank(-1);
    pDateTime->setDataTypeId(OpcUaId_DateTime);
        defaultValue.setDateTime(UaDateTime());
    pDateTime->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pDateTime, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDateTime;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemDouble(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pDouble = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_Double, getTypeNamespace()));
    pDouble->setBrowseName(UaQualifiedName("Double", getTypeNamespace()));
    pDouble->setDisplayName(UaLocalizedText("", "Double"));
    pDouble->setAccessLevel(3);
    pDouble->setUserAccessLevel(3);
    pDouble->setValueRank(-1);
    pDouble->setDataTypeId(OpcUaId_Double);
    {
        defaultValue.setDouble(0);
    }
    pDouble->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pDouble, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDouble;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemFloat(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pFloat = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_Float, getTypeNamespace()));
    pFloat->setBrowseName(UaQualifiedName("Float", getTypeNamespace()));
    pFloat->setDisplayName(UaLocalizedText("", "Float"));
    pFloat->setAccessLevel(3);
    pFloat->setUserAccessLevel(3);
    pFloat->setValueRank(-1);
    pFloat->setDataTypeId(OpcUaId_Float);
    {
        defaultValue.setFloat(0);
    }
    pFloat->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pFloat, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFloat;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(-1);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(0);
    }
    pInt16->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(-1);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        defaultValue.setInt32(0);
    }
    pInt32->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(-1);
    pInt64->setDataTypeId(OpcUaId_Int64);
    {
        defaultValue.setInt64(0);
    }
    pInt64->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(-1);
    pSByte->setDataTypeId(OpcUaId_SByte);
    {
        defaultValue.setSByte(0);
    }
    pSByte->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemString(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pString = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_String, getTypeNamespace()));
    pString->setBrowseName(UaQualifiedName("String", getTypeNamespace()));
    pString->setDisplayName(UaLocalizedText("", "String"));
    pString->setAccessLevel(3);
    pString->setUserAccessLevel(3);
    pString->setValueRank(-1);
    pString->setDataTypeId(OpcUaId_String);
        defaultValue.setString("");
    pString->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pString, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pString;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(-1);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        defaultValue.setUInt16(0);
    }
    pUInt16->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(-1);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pUInt32->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(-1);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
    {
        defaultValue.setUInt64(0);
    }
    pUInt64->setValue(defaultValue);

    OpcUa::DataItemType *pVariableType = new OpcUa::DataItemType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DataItem, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteType(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDiscreteType = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getTypeNamespace()));
    pDiscreteType->setBrowseName(UaQualifiedName("DiscreteType", getTypeNamespace()));
    pDiscreteType->setDisplayName(UaLocalizedText("", "DiscreteType"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pDiscreteType, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDiscreteType;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pMultiStateDiscrete1 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete1, getTypeNamespace()));
    pMultiStateDiscrete1->setBrowseName(UaQualifiedName("MultiStateDiscrete1", getTypeNamespace()));
    pMultiStateDiscrete1->setDisplayName(UaLocalizedText("", "MultiStateDiscrete1"));
    pMultiStateDiscrete1->setAccessLevel(3);
    pMultiStateDiscrete1->setUserAccessLevel(3);
    pMultiStateDiscrete1->setValueRank(-1);
    pMultiStateDiscrete1->setDataTypeId(OpcUaId_UInt16);
    {
        defaultValue.setUInt16(0);
    }
    pMultiStateDiscrete1->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumStrings
    UaBase::Variable *pEnumStrings = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete1_EnumStrings, getTypeNamespace()));
    pEnumStrings->setBrowseName(UaQualifiedName("EnumStrings", 0));
    pEnumStrings->setDisplayName(UaLocalizedText("", "EnumStrings"));
    pEnumStrings->setAccessLevel(1);
    pEnumStrings->setUserAccessLevel(1);
    pEnumStrings->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumStrings->setArrayDimensions(arrayDimensions);
    pEnumStrings->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray value;
        UaString sTemp;
        value.create(5);
        sTemp = "Red";
        sTemp.copyTo(&value[0].Text);
        sTemp = "Green";
        sTemp.copyTo(&value[1].Text);
        sTemp = "Blue";
        sTemp.copyTo(&value[2].Text);
        sTemp = "Purple";
        sTemp.copyTo(&value[3].Text);
        sTemp = "Yellow";
        sTemp.copyTo(&value[4].Text);
        defaultValue.setLocalizedTextArray(value);
    }
    pEnumStrings->setValue(defaultValue);
    pEnumStrings->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pMultiStateDiscrete1);
    reference.setTargetNode(pEnumStrings);
    pMultiStateDiscrete1->addFullReference(reference);
    pEnumStrings->addFullReference(reference);
    pEnumStrings->setParentNodeId(pMultiStateDiscrete1->nodeId());
    OpcUa::MultiStateDiscreteType *pVariableType = new OpcUa::MultiStateDiscreteType(pMultiStateDiscrete1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumStrings;
    delete pMultiStateDiscrete1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pMultiStateDiscrete2 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete2, getTypeNamespace()));
    pMultiStateDiscrete2->setBrowseName(UaQualifiedName("MultiStateDiscrete2", getTypeNamespace()));
    pMultiStateDiscrete2->setDisplayName(UaLocalizedText("", "MultiStateDiscrete2"));
    pMultiStateDiscrete2->setAccessLevel(3);
    pMultiStateDiscrete2->setUserAccessLevel(3);
    pMultiStateDiscrete2->setValueRank(-1);
    pMultiStateDiscrete2->setDataTypeId(OpcUaId_UInt16);
    {
        defaultValue.setUInt16(1);
    }
    pMultiStateDiscrete2->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumStrings
    UaBase::Variable *pEnumStrings = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete2_EnumStrings, getTypeNamespace()));
    pEnumStrings->setBrowseName(UaQualifiedName("EnumStrings", 0));
    pEnumStrings->setDisplayName(UaLocalizedText("", "EnumStrings"));
    pEnumStrings->setAccessLevel(1);
    pEnumStrings->setUserAccessLevel(1);
    pEnumStrings->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumStrings->setArrayDimensions(arrayDimensions);
    pEnumStrings->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray value;
        UaString sTemp;
        value.create(5);
        sTemp = "One";
        sTemp.copyTo(&value[0].Text);
        sTemp = "Two";
        sTemp.copyTo(&value[1].Text);
        sTemp = "Three";
        sTemp.copyTo(&value[2].Text);
        sTemp = "Four";
        sTemp.copyTo(&value[3].Text);
        sTemp = "Five";
        sTemp.copyTo(&value[4].Text);
        defaultValue.setLocalizedTextArray(value);
    }
    pEnumStrings->setValue(defaultValue);
    pEnumStrings->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pMultiStateDiscrete2);
    reference.setTargetNode(pEnumStrings);
    pMultiStateDiscrete2->addFullReference(reference);
    pEnumStrings->addFullReference(reference);
    pEnumStrings->setParentNodeId(pMultiStateDiscrete2->nodeId());
    OpcUa::MultiStateDiscreteType *pVariableType = new OpcUa::MultiStateDiscreteType(pMultiStateDiscrete2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumStrings;
    delete pMultiStateDiscrete2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pMultiStateDiscrete3 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete3, getTypeNamespace()));
    pMultiStateDiscrete3->setBrowseName(UaQualifiedName("MultiStateDiscrete3", getTypeNamespace()));
    pMultiStateDiscrete3->setDisplayName(UaLocalizedText("", "MultiStateDiscrete3"));
    pMultiStateDiscrete3->setAccessLevel(3);
    pMultiStateDiscrete3->setUserAccessLevel(3);
    pMultiStateDiscrete3->setValueRank(-1);
    pMultiStateDiscrete3->setDataTypeId(OpcUaId_UInt16);
    {
        defaultValue.setUInt16(2);
    }
    pMultiStateDiscrete3->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumStrings
    UaBase::Variable *pEnumStrings = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete3_EnumStrings, getTypeNamespace()));
    pEnumStrings->setBrowseName(UaQualifiedName("EnumStrings", 0));
    pEnumStrings->setDisplayName(UaLocalizedText("", "EnumStrings"));
    pEnumStrings->setAccessLevel(1);
    pEnumStrings->setUserAccessLevel(1);
    pEnumStrings->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 3;
    pEnumStrings->setArrayDimensions(arrayDimensions);
    pEnumStrings->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray value;
        UaString sTemp;
        value.create(3);
        sTemp = "TestDiscreteState11";
        sTemp.copyTo(&value[0].Text);
        sTemp = "TestDiscreteState12";
        sTemp.copyTo(&value[1].Text);
        sTemp = "TestDiscreteState13";
        sTemp.copyTo(&value[2].Text);
        defaultValue.setLocalizedTextArray(value);
    }
    pEnumStrings->setValue(defaultValue);
    pEnumStrings->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pMultiStateDiscrete3);
    reference.setTargetNode(pEnumStrings);
    pMultiStateDiscrete3->addFullReference(reference);
    pEnumStrings->addFullReference(reference);
    pEnumStrings->setParentNodeId(pMultiStateDiscrete3->nodeId());
    OpcUa::MultiStateDiscreteType *pVariableType = new OpcUa::MultiStateDiscreteType(pMultiStateDiscrete3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumStrings;
    delete pMultiStateDiscrete3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete4(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pMultiStateDiscrete4 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete4, getTypeNamespace()));
    pMultiStateDiscrete4->setBrowseName(UaQualifiedName("MultiStateDiscrete4", getTypeNamespace()));
    pMultiStateDiscrete4->setDisplayName(UaLocalizedText("", "MultiStateDiscrete4"));
    pMultiStateDiscrete4->setAccessLevel(3);
    pMultiStateDiscrete4->setUserAccessLevel(3);
    pMultiStateDiscrete4->setValueRank(-1);
    pMultiStateDiscrete4->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pMultiStateDiscrete4->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumStrings
    UaBase::Variable *pEnumStrings = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete4_EnumStrings, getTypeNamespace()));
    pEnumStrings->setBrowseName(UaQualifiedName("EnumStrings", 0));
    pEnumStrings->setDisplayName(UaLocalizedText("", "EnumStrings"));
    pEnumStrings->setAccessLevel(1);
    pEnumStrings->setUserAccessLevel(1);
    pEnumStrings->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 3;
    pEnumStrings->setArrayDimensions(arrayDimensions);
    pEnumStrings->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray value;
        UaString sTemp;
        value.create(3);
        sTemp = "TestDiscreteState21";
        sTemp.copyTo(&value[0].Text);
        sTemp = "TestDiscreteState22";
        sTemp.copyTo(&value[1].Text);
        sTemp = "TestDiscreteState23";
        sTemp.copyTo(&value[2].Text);
        defaultValue.setLocalizedTextArray(value);
    }
    pEnumStrings->setValue(defaultValue);
    pEnumStrings->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pMultiStateDiscrete4);
    reference.setTargetNode(pEnumStrings);
    pMultiStateDiscrete4->addFullReference(reference);
    pEnumStrings->addFullReference(reference);
    pEnumStrings->setParentNodeId(pMultiStateDiscrete4->nodeId());
    OpcUa::MultiStateDiscreteType *pVariableType = new OpcUa::MultiStateDiscreteType(pMultiStateDiscrete4, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumStrings;
    delete pMultiStateDiscrete4;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete5(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pMultiStateDiscrete5 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete5, getTypeNamespace()));
    pMultiStateDiscrete5->setBrowseName(UaQualifiedName("MultiStateDiscrete5", getTypeNamespace()));
    pMultiStateDiscrete5->setDisplayName(UaLocalizedText("", "MultiStateDiscrete5"));
    pMultiStateDiscrete5->setAccessLevel(3);
    pMultiStateDiscrete5->setUserAccessLevel(3);
    pMultiStateDiscrete5->setValueRank(-1);
    pMultiStateDiscrete5->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(1);
    }
    pMultiStateDiscrete5->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumStrings
    UaBase::Variable *pEnumStrings = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete5_EnumStrings, getTypeNamespace()));
    pEnumStrings->setBrowseName(UaQualifiedName("EnumStrings", 0));
    pEnumStrings->setDisplayName(UaLocalizedText("", "EnumStrings"));
    pEnumStrings->setAccessLevel(1);
    pEnumStrings->setUserAccessLevel(1);
    pEnumStrings->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 3;
    pEnumStrings->setArrayDimensions(arrayDimensions);
    pEnumStrings->setDataTypeId(OpcUaId_LocalizedText);
    {
        UaLocalizedTextArray value;
        UaString sTemp;
        value.create(3);
        sTemp = "TestDiscreteState31";
        sTemp.copyTo(&value[0].Text);
        sTemp = "TestDiscreteState32";
        sTemp.copyTo(&value[1].Text);
        sTemp = "TestDiscreteState33";
        sTemp.copyTo(&value[2].Text);
        defaultValue.setLocalizedTextArray(value);
    }
    pEnumStrings->setValue(defaultValue);
    pEnumStrings->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pMultiStateDiscrete5);
    reference.setTargetNode(pEnumStrings);
    pMultiStateDiscrete5->addFullReference(reference);
    pEnumStrings->addFullReference(reference);
    pEnumStrings->setParentNodeId(pMultiStateDiscrete5->nodeId());
    OpcUa::MultiStateDiscreteType *pVariableType = new OpcUa::MultiStateDiscreteType(pMultiStateDiscrete5, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumStrings;
    delete pMultiStateDiscrete5;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete1(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pTwoStateDiscrete1 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete1, getTypeNamespace()));
    pTwoStateDiscrete1->setBrowseName(UaQualifiedName("TwoStateDiscrete1", getTypeNamespace()));
    pTwoStateDiscrete1->setDisplayName(UaLocalizedText("", "TwoStateDiscrete1"));
    pTwoStateDiscrete1->setAccessLevel(3);
    pTwoStateDiscrete1->setUserAccessLevel(3);
    pTwoStateDiscrete1->setValueRank(-1);
    pTwoStateDiscrete1->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pTwoStateDiscrete1->setValue(defaultValue);

    UaBase::FullReference reference;

    // FalseState
    UaBase::Variable *pFalseState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete1_FalseState, getTypeNamespace()));
    pFalseState->setBrowseName(UaQualifiedName("FalseState", 0));
    pFalseState->setDisplayName(UaLocalizedText("", "FalseState"));
    pFalseState->setAccessLevel(1);
    pFalseState->setUserAccessLevel(1);
    pFalseState->setValueRank(-1);
    pFalseState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "Stopped"));
    }
    pFalseState->setValue(defaultValue);
    pFalseState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete1);
    reference.setTargetNode(pFalseState);
    pTwoStateDiscrete1->addFullReference(reference);
    pFalseState->addFullReference(reference);
    pFalseState->setParentNodeId(pTwoStateDiscrete1->nodeId());

    // TrueState
    UaBase::Variable *pTrueState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete1_TrueState, getTypeNamespace()));
    pTrueState->setBrowseName(UaQualifiedName("TrueState", 0));
    pTrueState->setDisplayName(UaLocalizedText("", "TrueState"));
    pTrueState->setAccessLevel(1);
    pTrueState->setUserAccessLevel(1);
    pTrueState->setValueRank(-1);
    pTrueState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "Started"));
    }
    pTrueState->setValue(defaultValue);
    pTrueState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete1);
    reference.setTargetNode(pTrueState);
    pTwoStateDiscrete1->addFullReference(reference);
    pTrueState->addFullReference(reference);
    pTrueState->setParentNodeId(pTwoStateDiscrete1->nodeId());
    OpcUa::TwoStateDiscreteType *pVariableType = new OpcUa::TwoStateDiscreteType(pTwoStateDiscrete1, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFalseState;
    delete pTrueState;
    delete pTwoStateDiscrete1;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pTwoStateDiscrete2 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete2, getTypeNamespace()));
    pTwoStateDiscrete2->setBrowseName(UaQualifiedName("TwoStateDiscrete2", getTypeNamespace()));
    pTwoStateDiscrete2->setDisplayName(UaLocalizedText("", "TwoStateDiscrete2"));
    pTwoStateDiscrete2->setAccessLevel(3);
    pTwoStateDiscrete2->setUserAccessLevel(3);
    pTwoStateDiscrete2->setValueRank(-1);
    pTwoStateDiscrete2->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pTwoStateDiscrete2->setValue(defaultValue);

    UaBase::FullReference reference;

    // FalseState
    UaBase::Variable *pFalseState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete2_FalseState, getTypeNamespace()));
    pFalseState->setBrowseName(UaQualifiedName("FalseState", 0));
    pFalseState->setDisplayName(UaLocalizedText("", "FalseState"));
    pFalseState->setAccessLevel(1);
    pFalseState->setUserAccessLevel(1);
    pFalseState->setValueRank(-1);
    pFalseState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "Red"));
    }
    pFalseState->setValue(defaultValue);
    pFalseState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete2);
    reference.setTargetNode(pFalseState);
    pTwoStateDiscrete2->addFullReference(reference);
    pFalseState->addFullReference(reference);
    pFalseState->setParentNodeId(pTwoStateDiscrete2->nodeId());

    // TrueState
    UaBase::Variable *pTrueState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete2_TrueState, getTypeNamespace()));
    pTrueState->setBrowseName(UaQualifiedName("TrueState", 0));
    pTrueState->setDisplayName(UaLocalizedText("", "TrueState"));
    pTrueState->setAccessLevel(1);
    pTrueState->setUserAccessLevel(1);
    pTrueState->setValueRank(-1);
    pTrueState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "Green"));
    }
    pTrueState->setValue(defaultValue);
    pTrueState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete2);
    reference.setTargetNode(pTrueState);
    pTwoStateDiscrete2->addFullReference(reference);
    pTrueState->addFullReference(reference);
    pTrueState->setParentNodeId(pTwoStateDiscrete2->nodeId());
    OpcUa::TwoStateDiscreteType *pVariableType = new OpcUa::TwoStateDiscreteType(pTwoStateDiscrete2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFalseState;
    delete pTrueState;
    delete pTwoStateDiscrete2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete3(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pTwoStateDiscrete3 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete3, getTypeNamespace()));
    pTwoStateDiscrete3->setBrowseName(UaQualifiedName("TwoStateDiscrete3", getTypeNamespace()));
    pTwoStateDiscrete3->setDisplayName(UaLocalizedText("", "TwoStateDiscrete3"));
    pTwoStateDiscrete3->setAccessLevel(3);
    pTwoStateDiscrete3->setUserAccessLevel(3);
    pTwoStateDiscrete3->setValueRank(-1);
    pTwoStateDiscrete3->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pTwoStateDiscrete3->setValue(defaultValue);

    UaBase::FullReference reference;

    // FalseState
    UaBase::Variable *pFalseState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete3_FalseState, getTypeNamespace()));
    pFalseState->setBrowseName(UaQualifiedName("FalseState", 0));
    pFalseState->setDisplayName(UaLocalizedText("", "FalseState"));
    pFalseState->setAccessLevel(1);
    pFalseState->setUserAccessLevel(1);
    pFalseState->setValueRank(-1);
    pFalseState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "TestFalseState1"));
    }
    pFalseState->setValue(defaultValue);
    pFalseState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete3);
    reference.setTargetNode(pFalseState);
    pTwoStateDiscrete3->addFullReference(reference);
    pFalseState->addFullReference(reference);
    pFalseState->setParentNodeId(pTwoStateDiscrete3->nodeId());

    // TrueState
    UaBase::Variable *pTrueState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete3_TrueState, getTypeNamespace()));
    pTrueState->setBrowseName(UaQualifiedName("TrueState", 0));
    pTrueState->setDisplayName(UaLocalizedText("", "TrueState"));
    pTrueState->setAccessLevel(1);
    pTrueState->setUserAccessLevel(1);
    pTrueState->setValueRank(-1);
    pTrueState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "TestTrueState1"));
    }
    pTrueState->setValue(defaultValue);
    pTrueState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete3);
    reference.setTargetNode(pTrueState);
    pTwoStateDiscrete3->addFullReference(reference);
    pTrueState->addFullReference(reference);
    pTrueState->setParentNodeId(pTwoStateDiscrete3->nodeId());
    OpcUa::TwoStateDiscreteType *pVariableType = new OpcUa::TwoStateDiscreteType(pTwoStateDiscrete3, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFalseState;
    delete pTrueState;
    delete pTwoStateDiscrete3;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete4(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pTwoStateDiscrete4 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete4, getTypeNamespace()));
    pTwoStateDiscrete4->setBrowseName(UaQualifiedName("TwoStateDiscrete4", getTypeNamespace()));
    pTwoStateDiscrete4->setDisplayName(UaLocalizedText("", "TwoStateDiscrete4"));
    pTwoStateDiscrete4->setAccessLevel(3);
    pTwoStateDiscrete4->setUserAccessLevel(3);
    pTwoStateDiscrete4->setValueRank(-1);
    pTwoStateDiscrete4->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pTwoStateDiscrete4->setValue(defaultValue);

    UaBase::FullReference reference;

    // FalseState
    UaBase::Variable *pFalseState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete4_FalseState, getTypeNamespace()));
    pFalseState->setBrowseName(UaQualifiedName("FalseState", 0));
    pFalseState->setDisplayName(UaLocalizedText("", "FalseState"));
    pFalseState->setAccessLevel(1);
    pFalseState->setUserAccessLevel(1);
    pFalseState->setValueRank(-1);
    pFalseState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "TestFalseState2"));
    }
    pFalseState->setValue(defaultValue);
    pFalseState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete4);
    reference.setTargetNode(pFalseState);
    pTwoStateDiscrete4->addFullReference(reference);
    pFalseState->addFullReference(reference);
    pFalseState->setParentNodeId(pTwoStateDiscrete4->nodeId());

    // TrueState
    UaBase::Variable *pTrueState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete4_TrueState, getTypeNamespace()));
    pTrueState->setBrowseName(UaQualifiedName("TrueState", 0));
    pTrueState->setDisplayName(UaLocalizedText("", "TrueState"));
    pTrueState->setAccessLevel(1);
    pTrueState->setUserAccessLevel(1);
    pTrueState->setValueRank(-1);
    pTrueState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "TestTrueState2"));
    }
    pTrueState->setValue(defaultValue);
    pTrueState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete4);
    reference.setTargetNode(pTrueState);
    pTwoStateDiscrete4->addFullReference(reference);
    pTrueState->addFullReference(reference);
    pTrueState->setParentNodeId(pTwoStateDiscrete4->nodeId());
    OpcUa::TwoStateDiscreteType *pVariableType = new OpcUa::TwoStateDiscreteType(pTwoStateDiscrete4, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFalseState;
    delete pTrueState;
    delete pTwoStateDiscrete4;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete5(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pTwoStateDiscrete5 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete5, getTypeNamespace()));
    pTwoStateDiscrete5->setBrowseName(UaQualifiedName("TwoStateDiscrete5", getTypeNamespace()));
    pTwoStateDiscrete5->setDisplayName(UaLocalizedText("", "TwoStateDiscrete5"));
    pTwoStateDiscrete5->setAccessLevel(3);
    pTwoStateDiscrete5->setUserAccessLevel(3);
    pTwoStateDiscrete5->setValueRank(-1);
    pTwoStateDiscrete5->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pTwoStateDiscrete5->setValue(defaultValue);

    UaBase::FullReference reference;

    // FalseState
    UaBase::Variable *pFalseState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete5_FalseState, getTypeNamespace()));
    pFalseState->setBrowseName(UaQualifiedName("FalseState", 0));
    pFalseState->setDisplayName(UaLocalizedText("", "FalseState"));
    pFalseState->setAccessLevel(1);
    pFalseState->setUserAccessLevel(1);
    pFalseState->setValueRank(-1);
    pFalseState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "TestFalseState3"));
    }
    pFalseState->setValue(defaultValue);
    pFalseState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete5);
    reference.setTargetNode(pFalseState);
    pTwoStateDiscrete5->addFullReference(reference);
    pFalseState->addFullReference(reference);
    pFalseState->setParentNodeId(pTwoStateDiscrete5->nodeId());

    // TrueState
    UaBase::Variable *pTrueState = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete5_TrueState, getTypeNamespace()));
    pTrueState->setBrowseName(UaQualifiedName("TrueState", 0));
    pTrueState->setDisplayName(UaLocalizedText("", "TrueState"));
    pTrueState->setAccessLevel(1);
    pTrueState->setUserAccessLevel(1);
    pTrueState->setValueRank(-1);
    pTrueState->setDataTypeId(OpcUaId_LocalizedText);
    {
        defaultValue.setLocalizedText(UaLocalizedText("", "TestTrueState3"));
    }
    pTrueState->setValue(defaultValue);
    pTrueState->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pTwoStateDiscrete5);
    reference.setTargetNode(pTrueState);
    pTwoStateDiscrete5->addFullReference(reference);
    pTrueState->addFullReference(reference);
    pTrueState->setParentNodeId(pTwoStateDiscrete5->nodeId());
    OpcUa::TwoStateDiscreteType *pVariableType = new OpcUa::TwoStateDiscreteType(pTwoStateDiscrete5, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_DiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFalseState;
    delete pTrueState;
    delete pTwoStateDiscrete5;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteType(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pMultiStateValueDiscreteType = new UaBase::Object(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getTypeNamespace()));
    pMultiStateValueDiscreteType->setBrowseName(UaQualifiedName("MultiStateValueDiscreteType", getTypeNamespace()));
    pMultiStateValueDiscreteType->setDisplayName(UaLocalizedText("", "MultiStateValueDiscreteType"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pMultiStateValueDiscreteType, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pMultiStateValueDiscreteType;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Byte, getTypeNamespace()));
    pByte->setBrowseName(UaQualifiedName("Byte", getTypeNamespace()));
    pByte->setDisplayName(UaLocalizedText("", "Byte"));
    pByte->setAccessLevel(3);
    pByte->setUserAccessLevel(3);
    pByte->setValueRank(-1);
    pByte->setDataTypeId(OpcUaId_Byte);
    {
        defaultValue.setByte(10);
    }
    pByte->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Byte_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pByte);
    reference.setTargetNode(pEnumValues);
    pByte->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pByte->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Byte_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pByte);
    reference.setTargetNode(pValueAsText);
    pByte->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pByte->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int16, getTypeNamespace()));
    pInt16->setBrowseName(UaQualifiedName("Int16", getTypeNamespace()));
    pInt16->setDisplayName(UaLocalizedText("", "Int16"));
    pInt16->setAccessLevel(3);
    pInt16->setUserAccessLevel(3);
    pInt16->setValueRank(-1);
    pInt16->setDataTypeId(OpcUaId_Int16);
    {
        defaultValue.setInt16(10);
    }
    pInt16->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int16_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt16);
    reference.setTargetNode(pEnumValues);
    pInt16->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pInt16->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int16_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt16);
    reference.setTargetNode(pValueAsText);
    pInt16->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pInt16->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int32, getTypeNamespace()));
    pInt32->setBrowseName(UaQualifiedName("Int32", getTypeNamespace()));
    pInt32->setDisplayName(UaLocalizedText("", "Int32"));
    pInt32->setAccessLevel(3);
    pInt32->setUserAccessLevel(3);
    pInt32->setValueRank(-1);
    pInt32->setDataTypeId(OpcUaId_Int32);
    {
        defaultValue.setInt32(10);
    }
    pInt32->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int32_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt32);
    reference.setTargetNode(pEnumValues);
    pInt32->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pInt32->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int32_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt32);
    reference.setTargetNode(pValueAsText);
    pInt32->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pInt32->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int64, getTypeNamespace()));
    pInt64->setBrowseName(UaQualifiedName("Int64", getTypeNamespace()));
    pInt64->setDisplayName(UaLocalizedText("", "Int64"));
    pInt64->setAccessLevel(3);
    pInt64->setUserAccessLevel(3);
    pInt64->setValueRank(-1);
    pInt64->setDataTypeId(OpcUaId_Int64);
    {
        defaultValue.setInt64(10);
    }
    pInt64->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int64_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt64);
    reference.setTargetNode(pEnumValues);
    pInt64->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pInt64->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int64_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pInt64);
    reference.setTargetNode(pValueAsText);
    pInt64->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pInt64->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeSByte(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSByte = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_SByte, getTypeNamespace()));
    pSByte->setBrowseName(UaQualifiedName("SByte", getTypeNamespace()));
    pSByte->setDisplayName(UaLocalizedText("", "SByte"));
    pSByte->setAccessLevel(3);
    pSByte->setUserAccessLevel(3);
    pSByte->setValueRank(-1);
    pSByte->setDataTypeId(OpcUaId_SByte);
    {
        defaultValue.setSByte(10);
    }
    pSByte->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_SByte_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pSByte);
    reference.setTargetNode(pEnumValues);
    pSByte->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pSByte->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_SByte_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pSByte);
    reference.setTargetNode(pValueAsText);
    pSByte->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pSByte->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pSByte, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pSByte;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt16(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt16 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt16, getTypeNamespace()));
    pUInt16->setBrowseName(UaQualifiedName("UInt16", getTypeNamespace()));
    pUInt16->setDisplayName(UaLocalizedText("", "UInt16"));
    pUInt16->setAccessLevel(3);
    pUInt16->setUserAccessLevel(3);
    pUInt16->setValueRank(-1);
    pUInt16->setDataTypeId(OpcUaId_UInt16);
    {
        defaultValue.setUInt16(10);
    }
    pUInt16->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt16_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt16);
    reference.setTargetNode(pEnumValues);
    pUInt16->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pUInt16->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt16_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt16);
    reference.setTargetNode(pValueAsText);
    pUInt16->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pUInt16->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pUInt16, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pUInt16;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt32(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt32 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt32, getTypeNamespace()));
    pUInt32->setBrowseName(UaQualifiedName("UInt32", getTypeNamespace()));
    pUInt32->setDisplayName(UaLocalizedText("", "UInt32"));
    pUInt32->setAccessLevel(3);
    pUInt32->setUserAccessLevel(3);
    pUInt32->setValueRank(-1);
    pUInt32->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(10);
    }
    pUInt32->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt32_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt32);
    reference.setTargetNode(pEnumValues);
    pUInt32->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pUInt32->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt32_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt32);
    reference.setTargetNode(pValueAsText);
    pUInt32->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pUInt32->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pUInt32, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pUInt32;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt64(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pUInt64 = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt64, getTypeNamespace()));
    pUInt64->setBrowseName(UaQualifiedName("UInt64", getTypeNamespace()));
    pUInt64->setDisplayName(UaLocalizedText("", "UInt64"));
    pUInt64->setAccessLevel(3);
    pUInt64->setUserAccessLevel(3);
    pUInt64->setValueRank(-1);
    pUInt64->setDataTypeId(OpcUaId_UInt64);
    {
        defaultValue.setUInt64(10);
    }
    pUInt64->setValue(defaultValue);

    UaBase::FullReference reference;

    // EnumValues
    UaBase::Variable *pEnumValues = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt64_EnumValues, getTypeNamespace()));
    pEnumValues->setBrowseName(UaQualifiedName("EnumValues", 0));
    pEnumValues->setDisplayName(UaLocalizedText("", "EnumValues"));
    pEnumValues->setAccessLevel(1);
    pEnumValues->setUserAccessLevel(1);
    pEnumValues->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 5;
    pEnumValues->setArrayDimensions(arrayDimensions);
    pEnumValues->setDataTypeId(OpcUaId_EnumValueType);
    {
        UaExtensionObjectArray value;
        value.create(5);
        UaString sTemp;
        OpcUa_EnumValueType *pEnumValue;
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[0],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 10;
        sTemp = "Red";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[1],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 20;
        sTemp = "Yellow";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[2],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 30;
        sTemp = "Green";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[3],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 40;
        sTemp = "Blue";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        OpcUa_EncodeableObject_CreateExtension(&OpcUa_EnumValueType_EncodeableType,
                                               &value[4],
                                               (OpcUa_Void**)&pEnumValue);
        pEnumValue->Value = 50;
        sTemp = "Purple";
        sTemp.copyTo(&pEnumValue->DisplayName.Text);
        value.toVariant(defaultValue);
    }
    pEnumValues->setValue(defaultValue);
    pEnumValues->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt64);
    reference.setTargetNode(pEnumValues);
    pUInt64->addFullReference(reference);
    pEnumValues->addFullReference(reference);
    pEnumValues->setParentNodeId(pUInt64->nodeId());

    // ValueAsText
    UaBase::Variable *pValueAsText = new UaBase::Variable(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt64_ValueAsText, getTypeNamespace()));
    pValueAsText->setBrowseName(UaQualifiedName("ValueAsText", 0));
    pValueAsText->setDisplayName(UaLocalizedText("", "ValueAsText"));
    pValueAsText->setAccessLevel(1);
    pValueAsText->setUserAccessLevel(1);
    pValueAsText->setValueRank(-1);
    pValueAsText->setDataTypeId(OpcUaId_LocalizedText);
    defaultValue.setLocalizedText(UaLocalizedText());
    pValueAsText->setValue(defaultValue);
    pValueAsText->setTypeDefinitionId(OpcUaId_PropertyType);
    reference.setReferenceTypeId(OpcUaId_HasProperty);
    reference.setSourceNode(pUInt64);
    reference.setTargetNode(pValueAsText);
    pUInt64->addFullReference(reference);
    pValueAsText->addFullReference(reference);
    pValueAsText->setParentNodeId(pUInt64->nodeId());
    OpcUa::MultiStateValueDiscreteType *pVariableType = new OpcUa::MultiStateValueDiscreteType(pUInt64, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pEnumValues;
    delete pValueAsText;
    delete pUInt64;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoUnicodeTest(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pUnicodeTest = new UaBase::Object(UaNodeId(DemoId_Demo_UnicodeTest, getTypeNamespace()));
    pUnicodeTest->setBrowseName(UaQualifiedName("UnicodeTest", getTypeNamespace()));
    pUnicodeTest->setDisplayName(UaLocalizedText("", "011_UnicodeTest"));
    pUnicodeTest->setDescription(UaLocalizedText("", "This folder contains objects with different unicode characters."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pUnicodeTest, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pUnicodeTest;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoUnicodeTestDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste = new UaBase::Object(UaNodeId(DemoId_Demo_UnicodeTest_Die_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste, getTypeNamespace()));
    pDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste->setBrowseName(UaQualifiedName("Die_hei\303\237e_Zypernsonne_qu\303\244lte_Max_und_Victoria_ja_b\303\266se_auf_dem_Weg_bis_zur_K\303\274ste", getTypeNamespace()));
    pDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste->setDisplayName(UaLocalizedText("", "Die hei\303\237e Zypernsonne qu\303\244lte Max und Victoria ja b\303\266se auf dem Weg bis zur K\303\274ste"));

    OpcUa::BaseObjectType *pObjectType = new OpcUa::BaseObjectType(pDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_UnicodeTest, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoUnicodeTestFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern = new UaBase::Object(UaNodeId(DemoId_Demo_UnicodeTest_Franz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern, getTypeNamespace()));
    pFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern->setBrowseName(UaQualifiedName("Franz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern", getTypeNamespace()));
    pFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern->setDisplayName(UaLocalizedText("", "Franz jagt im komplett verwahrlosten Taxi quer durch Bayern"));

    OpcUa::BaseObjectType *pObjectType = new OpcUa::BaseObjectType(pFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_UnicodeTest, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoUnicodeTestThe_quick_brown_fox_jumps_over_the_lazy_dog(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pThe_quick_brown_fox_jumps_over_the_lazy_dog = new UaBase::Object(UaNodeId(DemoId_Demo_UnicodeTest_The_quick_brown_fox_jumps_over_the_lazy_dog, getTypeNamespace()));
    pThe_quick_brown_fox_jumps_over_the_lazy_dog->setBrowseName(UaQualifiedName("The_quick_brown_fox_jumps_over_the_lazy_dog", getTypeNamespace()));
    pThe_quick_brown_fox_jumps_over_the_lazy_dog->setDisplayName(UaLocalizedText("", "The quick brown fox jumps over the lazy dog"));

    OpcUa::BaseObjectType *pObjectType = new OpcUa::BaseObjectType(pThe_quick_brown_fox_jumps_over_the_lazy_dog, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_UnicodeTest, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pThe_quick_brown_fox_jumps_over_the_lazy_dog;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoUnicodeTestVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf = new UaBase::Object(UaNodeId(DemoId_Demo_UnicodeTest_Voyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf, getTypeNamespace()));
    pVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf->setBrowseName(UaQualifiedName("Voyez_le_brick_g\303\251ant_que_j\342\200\231examine_pr\303\250s_du_wharf", getTypeNamespace()));
    pVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf->setDisplayName(UaLocalizedText("", "Voyez le brick g\303\251ant que j\342\200\231examine pr\303\250s du wharf"));

    OpcUa::BaseObjectType *pObjectType = new OpcUa::BaseObjectType(pVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_UnicodeTest, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoUnicodeTestNode5016(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pNode5016 = new UaBase::Object(UaNodeId(DemoId_Demo_UnicodeTest_Node5016, getTypeNamespace()));
    pNode5016->setBrowseName(UaQualifiedName("\320\241\321\212\320\265\321\210\321\214_\320\265\321\211\321\221_\321\215\321\202\320\270\321\205_\320\274\321\217\320\263\320\272\320\270\321\205_\321\204\321\200\320\260\320\275\321\206\321\203\320\267\321\201\320\272\320\270\321\205_\320\261\321\203\320\273\320\276\320\272,_\320\264\320\260_\320\262\321\213\320\277\320\265\320\271_\321\207\320\260\321\216", getTypeNamespace()));
    pNode5016->setDisplayName(UaLocalizedText("", "\320\241\321\212\320\265\321\210\321\214 \320\265\321\211\321\221 \321\215\321\202\320\270\321\205 \320\274\321\217\320\263\320\272\320\270\321\205 \321\204\321\200\320\260\320\275\321\206\321\203\320\267\321\201\320\272\320\270\321\205 \320\261\321\203\320\273\320\276\320\272, \320\264\320\260 \320\262\321\213\320\277\320\265\320\271 \321\207\320\260\321\216"));

    OpcUa::BaseObjectType *pObjectType = new OpcUa::BaseObjectType(pNode5016, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_UnicodeTest, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pNode5016;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoIcon(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pIcon = new UaBase::Object(UaNodeId(DemoId_Demo_Icon, getTypeNamespace()));
    pIcon->setBrowseName(UaQualifiedName("Icon", getTypeNamespace()));
    pIcon->setDisplayName(UaLocalizedText("", "012_Icon"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pIcon, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pIcon;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoIconIcon(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pIcon = new UaBase::Variable(UaNodeId(DemoId_Demo_Icon_Icon, getTypeNamespace()));
    pIcon->setBrowseName(UaQualifiedName("Icon", 0));
    pIcon->setDisplayName(UaLocalizedText("", "Icon"));
    pIcon->setAccessLevel(3);
    pIcon->setUserAccessLevel(3);
    pIcon->setValueRank(-1);
    pIcon->setDataTypeId(OpcUaId_ImagePNG);
    {
        UaByteString tmpIcon = UaByteString(4276, (OpcUa_Byte*) "\211\120\116\107\015\012\032\012\000\000\000\015\111\110\104\122\000\000\000\127\000\000\000\127\010\006\000\000\000\161\310\331"
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
        defaultValue.setByteString(tmpIcon, OpcUa_False);
    }
    pIcon->setValue(defaultValue);

    OpcUa::PropertyType *pVariableType = new OpcUa::PropertyType(pIcon, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasProperty;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Icon, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pIcon;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoImages(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pImages = new UaBase::Object(UaNodeId(DemoId_Demo_Images, getTypeNamespace()));
    pImages->setBrowseName(UaQualifiedName("Images", getTypeNamespace()));
    pImages->setDisplayName(UaLocalizedText("", "013_Images"));
    pImages->setDescription(UaLocalizedText("", "This folder contains variables with Image DataTypes"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pImages, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pImages;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoImagesDynamic(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pDynamic = new UaBase::Object(UaNodeId(DemoId_Demo_Images_Dynamic, getTypeNamespace()));
    pDynamic->setBrowseName(UaQualifiedName("Dynamic", getTypeNamespace()));
    pDynamic->setDisplayName(UaLocalizedText("", "Dynamic"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pDynamic, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Images, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pDynamic;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoImagesStatic(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pStatic = new UaBase::Object(UaNodeId(DemoId_Demo_Images_Static, getTypeNamespace()));
    pStatic->setBrowseName(UaQualifiedName("Static", getTypeNamespace()));
    pStatic->setDisplayName(UaLocalizedText("", "Static"));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pStatic, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_Images, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pStatic;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoFiles(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pFiles = new UaBase::Object(UaNodeId(DemoId_Demo_Files, getTypeNamespace()));
    pFiles->setBrowseName(UaQualifiedName("Files", getTypeNamespace()));
    pFiles->setDisplayName(UaLocalizedText("", "014_Files"));

    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pFiles, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pFiles;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoWorkOrder(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Object *pWorkOrder = new UaBase::Object(UaNodeId(DemoId_Demo_WorkOrder, getTypeNamespace()));
    pWorkOrder->setBrowseName(UaQualifiedName("WorkOrder", getTypeNamespace()));
    pWorkOrder->setDisplayName(UaLocalizedText("", "015_WorkOrder"));
    pWorkOrder->setDescription(UaLocalizedText("", "This folder contains instances of WorkOrdervariableType. The Values of the instance root node and its children are consistent."));

    UaBase::FullReference reference;
    OpcUa::FolderType *pObjectType = new OpcUa::FolderType(pWorkOrder, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pObjectType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pWorkOrder;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoWorkOrderWorkOrderVariable(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pWorkOrderVariable = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable, getTypeNamespace()));
    pWorkOrderVariable->setBrowseName(UaQualifiedName("WorkOrderVariable", getTypeNamespace()));
    pWorkOrderVariable->setDisplayName(UaLocalizedText("", "WorkOrderVariable"));
    pWorkOrderVariable->setAccessLevel(3);
    pWorkOrderVariable->setUserAccessLevel(3);
    pWorkOrderVariable->setValueRank(-1);
    pWorkOrderVariable->setDataTypeId(UaNodeId(DemoId_WorkOrderType, getNameSpaceIndex()));
    {
        Demo::WorkOrderType valWorkOrderVariable;
        valWorkOrderVariable.toVariant(defaultValue);
    }
    pWorkOrderVariable->setValue(defaultValue);

    UaBase::FullReference reference;

    // AssetID
    UaBase::Variable *pAssetID = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable_AssetID, getTypeNamespace()));
    pAssetID->setBrowseName(UaQualifiedName("AssetID", getTypeNamespace()));
    pAssetID->setDisplayName(UaLocalizedText("", "AssetID"));
    pAssetID->setAccessLevel(3);
    pAssetID->setUserAccessLevel(3);
    pAssetID->setValueRank(-1);
    pAssetID->setDataTypeId(OpcUaId_String);
    defaultValue.setString("");
    pAssetID->setValue(defaultValue);
    pAssetID->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable);
    reference.setTargetNode(pAssetID);
    pWorkOrderVariable->addFullReference(reference);
    pAssetID->addFullReference(reference);
    pAssetID->setParentNodeId(pWorkOrderVariable->nodeId());

    // ID
    UaBase::Variable *pID = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable_ID, getTypeNamespace()));
    pID->setBrowseName(UaQualifiedName("ID", getTypeNamespace()));
    pID->setDisplayName(UaLocalizedText("", "ID"));
    pID->setAccessLevel(3);
    pID->setUserAccessLevel(3);
    pID->setValueRank(-1);
    pID->setDataTypeId(OpcUaId_Guid);
    defaultValue.setGuid(UaGuid());
    pID->setValue(defaultValue);
    pID->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable);
    reference.setTargetNode(pID);
    pWorkOrderVariable->addFullReference(reference);
    pID->addFullReference(reference);
    pID->setParentNodeId(pWorkOrderVariable->nodeId());

    // StartTime
    UaBase::Variable *pStartTime = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable_StartTime, getTypeNamespace()));
    pStartTime->setBrowseName(UaQualifiedName("StartTime", getTypeNamespace()));
    pStartTime->setDisplayName(UaLocalizedText("", "StartTime"));
    pStartTime->setAccessLevel(3);
    pStartTime->setUserAccessLevel(3);
    pStartTime->setValueRank(-1);
    pStartTime->setDataTypeId(OpcUaId_DateTime);
    defaultValue.setDateTime(UaDateTime());
    pStartTime->setValue(defaultValue);
    pStartTime->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable);
    reference.setTargetNode(pStartTime);
    pWorkOrderVariable->addFullReference(reference);
    pStartTime->addFullReference(reference);
    pStartTime->setParentNodeId(pWorkOrderVariable->nodeId());

    // StatusComments
    UaBase::Variable *pStatusComments = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable_StatusComments, getTypeNamespace()));
    pStatusComments->setBrowseName(UaQualifiedName("StatusComments", getTypeNamespace()));
    pStatusComments->setDisplayName(UaLocalizedText("", "StatusComments"));
    pStatusComments->setAccessLevel(3);
    pStatusComments->setUserAccessLevel(3);
    pStatusComments->setValueRank(1);
    pStatusComments->setDataTypeId(UaNodeId(DemoId_WorkOrderStatusType, getNameSpaceIndex()));
    {
        Demo::WorkOrderStatusTypes valStatusCommentsArray;
        valStatusCommentsArray.toVariant(defaultValue);
    }
    pStatusComments->setValue(defaultValue);
    pStatusComments->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable);
    reference.setTargetNode(pStatusComments);
    pWorkOrderVariable->addFullReference(reference);
    pStatusComments->addFullReference(reference);
    pStatusComments->setParentNodeId(pWorkOrderVariable->nodeId());
    Demo::WorkOrderVariableType *pVariableType = new Demo::WorkOrderVariableType(pWorkOrderVariable, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_WorkOrder, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAssetID;
    delete pID;
    delete pStartTime;
    delete pStatusComments;
    delete pWorkOrderVariable;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoWorkOrderWorkOrderVariable2(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pWorkOrderVariable2 = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable2, getTypeNamespace()));
    pWorkOrderVariable2->setBrowseName(UaQualifiedName("WorkOrderVariable2", getTypeNamespace()));
    pWorkOrderVariable2->setDisplayName(UaLocalizedText("", "WorkOrderVariable2"));
    pWorkOrderVariable2->setAccessLevel(3);
    pWorkOrderVariable2->setUserAccessLevel(3);
    pWorkOrderVariable2->setValueRank(-1);
    pWorkOrderVariable2->setDataTypeId(UaNodeId(DemoId_WorkOrderType, getNameSpaceIndex()));
    {
        Demo::WorkOrderType valWorkOrderVariable2;
        valWorkOrderVariable2.toVariant(defaultValue);
    }
    pWorkOrderVariable2->setValue(defaultValue);

    UaBase::FullReference reference;

    // AssetID
    UaBase::Variable *pAssetID = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable2_AssetID, getTypeNamespace()));
    pAssetID->setBrowseName(UaQualifiedName("AssetID", getTypeNamespace()));
    pAssetID->setDisplayName(UaLocalizedText("", "AssetID"));
    pAssetID->setAccessLevel(3);
    pAssetID->setUserAccessLevel(3);
    pAssetID->setValueRank(-1);
    pAssetID->setDataTypeId(OpcUaId_String);
    defaultValue.setString("");
    pAssetID->setValue(defaultValue);
    pAssetID->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable2);
    reference.setTargetNode(pAssetID);
    pWorkOrderVariable2->addFullReference(reference);
    pAssetID->addFullReference(reference);
    pAssetID->setParentNodeId(pWorkOrderVariable2->nodeId());

    // ID
    UaBase::Variable *pID = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable2_ID, getTypeNamespace()));
    pID->setBrowseName(UaQualifiedName("ID", getTypeNamespace()));
    pID->setDisplayName(UaLocalizedText("", "ID"));
    pID->setAccessLevel(3);
    pID->setUserAccessLevel(3);
    pID->setValueRank(-1);
    pID->setDataTypeId(OpcUaId_Guid);
    defaultValue.setGuid(UaGuid());
    pID->setValue(defaultValue);
    pID->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable2);
    reference.setTargetNode(pID);
    pWorkOrderVariable2->addFullReference(reference);
    pID->addFullReference(reference);
    pID->setParentNodeId(pWorkOrderVariable2->nodeId());

    // StartTime
    UaBase::Variable *pStartTime = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable2_StartTime, getTypeNamespace()));
    pStartTime->setBrowseName(UaQualifiedName("StartTime", getTypeNamespace()));
    pStartTime->setDisplayName(UaLocalizedText("", "StartTime"));
    pStartTime->setAccessLevel(3);
    pStartTime->setUserAccessLevel(3);
    pStartTime->setValueRank(-1);
    pStartTime->setDataTypeId(OpcUaId_DateTime);
    defaultValue.setDateTime(UaDateTime());
    pStartTime->setValue(defaultValue);
    pStartTime->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable2);
    reference.setTargetNode(pStartTime);
    pWorkOrderVariable2->addFullReference(reference);
    pStartTime->addFullReference(reference);
    pStartTime->setParentNodeId(pWorkOrderVariable2->nodeId());

    // StatusComments
    UaBase::Variable *pStatusComments = new UaBase::Variable(UaNodeId(DemoId_Demo_WorkOrder_WorkOrderVariable2_StatusComments, getTypeNamespace()));
    pStatusComments->setBrowseName(UaQualifiedName("StatusComments", getTypeNamespace()));
    pStatusComments->setDisplayName(UaLocalizedText("", "StatusComments"));
    pStatusComments->setAccessLevel(3);
    pStatusComments->setUserAccessLevel(3);
    pStatusComments->setValueRank(1);
    pStatusComments->setDataTypeId(UaNodeId(DemoId_WorkOrderStatusType, getNameSpaceIndex()));
    {
        Demo::WorkOrderStatusTypes valStatusCommentsArray;
        valStatusCommentsArray.toVariant(defaultValue);
    }
    pStatusComments->setValue(defaultValue);
    pStatusComments->setTypeDefinitionId(OpcUaId_BaseDataVariableType);
    reference.setReferenceTypeId(OpcUaId_HasComponent);
    reference.setSourceNode(pWorkOrderVariable2);
    reference.setTargetNode(pStatusComments);
    pWorkOrderVariable2->addFullReference(reference);
    pStatusComments->addFullReference(reference);
    pStatusComments->setParentNodeId(pWorkOrderVariable2->nodeId());
    Demo::WorkOrderVariableType *pVariableType = new Demo::WorkOrderVariableType(pWorkOrderVariable2, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo_WorkOrder, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pAssetID;
    delete pID;
    delete pStartTime;
    delete pStatusComments;
    delete pWorkOrderVariable2;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoSetSimulationSpeed(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_SetSimulationSpeed, getTypeNamespace()), "SetSimulationSpeed", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    UaArgument argument;

    // InputArguments
    UaBase::Variable *pInputArguments = new UaBase::Variable(UaNodeId(DemoId_Demo_SetSimulationSpeed_InputArguments, getTypeNamespace()));
    pInputArguments->setBrowseName(UaQualifiedName("InputArguments", 0));
    pInputArguments->setDisplayName(UaLocalizedText("", "InputArguments"));
    pInputArguments->setAccessLevel(1);
    pInputArguments->setUserAccessLevel(1);
    pInputArguments->setValueRank(1);
    UaUInt32Array arrayDimensions;
    arrayDimensions.create(1);
    arrayDimensions[0] = 1;
    pInputArguments->setArrayDimensions(arrayDimensions);
    pInputArguments->setDataTypeId(OpcUaId_Argument);
    UaArguments valInputArguments;
    valInputArguments.create(1);
    argument.setName("speed");
    argument.setDataType(OpcUaId_UInt32);
    argument.setDescription(UaLocalizedText("", "Simulation interval [ms]."));
    argument.setValueRank(-1);
    argument.copyTo(&valInputArguments[ 0 ]);
    valInputArguments.toVariant(defaultValue);
    pInputArguments->setValue(defaultValue);
    OpcUa::PropertyType *pInputArgumentsProperty = new OpcUa::PropertyType(pInputArguments, (XmlUaNodeFactoryManager*) &factory, this);
    ret = addNodeAndReference(pMethod, pInputArgumentsProperty, OpcUaId_HasProperty);
    UA_ASSERT(ret.isGood());
    delete pInputArguments;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoSimulationActive(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSimulationActive = new UaBase::Variable(UaNodeId(DemoId_Demo_SimulationActive, getTypeNamespace()));
    pSimulationActive->setBrowseName(UaQualifiedName("SimulationActive", getTypeNamespace()));
    pSimulationActive->setDisplayName(UaLocalizedText("", "SimulationActive"));
    pSimulationActive->setAccessLevel(3);
    pSimulationActive->setUserAccessLevel(3);
    pSimulationActive->setValueRank(-1);
    pSimulationActive->setDataTypeId(OpcUaId_Boolean);
    {
        defaultValue.setBool(OpcUa_False);
    }
    pSimulationActive->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSimulationActive, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSimulationActive;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoSimulationSpeed(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    UaBase::Variable *pSimulationSpeed = new UaBase::Variable(UaNodeId(DemoId_Demo_SimulationSpeed, getTypeNamespace()));
    pSimulationSpeed->setBrowseName(UaQualifiedName("SimulationSpeed", getTypeNamespace()));
    pSimulationSpeed->setDisplayName(UaLocalizedText("", "SimulationSpeed"));
    pSimulationSpeed->setAccessLevel(3);
    pSimulationSpeed->setUserAccessLevel(3);
    pSimulationSpeed->setValueRank(-1);
    pSimulationSpeed->setDataTypeId(OpcUaId_UInt32);
    {
        defaultValue.setUInt32(0);
    }
    pSimulationSpeed->setValue(defaultValue);

    OpcUa::BaseDataVariableType *pVariableType = new OpcUa::BaseDataVariableType(pSimulationSpeed, (XmlUaNodeFactoryManager*) &factory, this);
    referenceTypeId = OpcUaId_Organizes;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pVariableType, referenceTypeId);
    UA_ASSERT(ret.isGood());
    delete pSimulationSpeed;
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStartSimulation(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_StartSimulation, getTypeNamespace()), "StartSimulation", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}

UaStatus NodeManagerDemoBase::addObjectsFolderDemoStopSimulation(const XmlUaNodeFactoryManager &factory)
{
    UaStatus ret;
    UaVariant defaultValue;
    UaNodeId referenceTypeId;
    UaNodeId parentEventNodeId;
    OpcUa::BaseMethod *pMethod = new OpcUa::BaseMethod(UaNodeId(DemoId_Demo_StopSimulation, getTypeNamespace()), "StopSimulation", getTypeNamespace());
    referenceTypeId = OpcUaId_HasComponent;
    ret = addNodeAndReference(UaNodeId(DemoId_Demo, getNameSpaceIndex()), pMethod, referenceTypeId);
    UA_ASSERT(ret.isGood());
    OpcUa_ReferenceParameter(factory);
    return ret;
}


UaStatus NodeManagerDemoBase::addAdditionalHierarchicalReferences()
{
    UaStatus ret;
    ret = addUaReference(UaNodeId(DemoId_Demo_Images_Dynamic, getNameSpaceIndex()), UaNodeId(DemoId_Demo_Dynamic_Scalar_ImageGIF, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_AccessRights_Browse_All, getNameSpaceIndex()), UaNodeId(DemoId_Demo_AccessRights_Access_All_All_RW, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_AccessRights_Browse_John, getNameSpaceIndex()), UaNodeId(DemoId_Demo_AccessRights_Access_All_All_RW, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_AccessRights_Browse_Operators, getNameSpaceIndex()), UaNodeId(DemoId_Demo_AccessRights_Access_All_All_RW, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode2, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1_Has3InverseReferences, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode3, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1_Has3InverseReferences, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_CTT_References, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1_Has3InverseReferences, getNameSpaceIndex()), OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_Images_Static, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_Image, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_Images_Static, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageBMP, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_Images_Static, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageGIF, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_Images_Static, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageJPG, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_Images_Static, getNameSpaceIndex()), UaNodeId(DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImagePNG, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());
    ret = addUaReference(UaNodeId(DemoId_Demo_Images_Static, getNameSpaceIndex()), UaNodeId(DemoId_Demo_Icon_Icon, getNameSpaceIndex()), OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    return ret;
}

#if SUPPORT_Method_Server_Facet
MethodHandle* NodeManagerDemoBase::getMethodHandle(
    Session*       pSession,
    OpcUa_NodeId*  pObjectNodeId,
    OpcUa_NodeId*  pMethodNodeId,
    UaStatus&      result) const
{
    if (pObjectNodeId && pMethodNodeId)
    {
        UaNodeId objectId(*pObjectNodeId);
        UaNodeId methodId(*pMethodNodeId);
        if ( (methodId == UaNodeId(DemoId_Demo_History_StartLogging, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_History_StopLogging, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_Method_DoSomethingAfter10s, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_Method_EnumTest, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_Method_Multiply, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_Method_VectorAdd, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_DynamicNodes_CreateDynamicNode, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_DynamicNodes_DeleteDynamicNode, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_CTT_Methods_MethodIO, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_CTT_Methods_MethodNoArgs, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_CTT_NodeClasses_Method, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method1, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method2, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method3, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_SetSimulationSpeed, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_StartSimulation, getNameSpaceIndex()))
          || (methodId == UaNodeId(DemoId_Demo_StopSimulation, getNameSpaceIndex()))
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
UaStatus NodeManagerDemoBase::beginCall(
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

UaStatus NodeManagerDemoBase::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    OpcUa_ReferenceParameter(inputArgumentDiag);
    if (pMethodHandle->getHandleImplementation() == MethodHandle::NODE_ID)
    {
        MethodHandleNodeId* pMethodHandleNodeId = (MethodHandleNodeId*)pMethodHandle;
        if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_History_StartLogging, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_History_StartLogging(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_History_StopLogging, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_History_StopLogging(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_Method_DoSomethingAfter10s, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_Method_DoSomethingAfter10s(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_Method_EnumTest, getNameSpaceIndex()))
        {
                if ( inputArguments.length() < 2 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else if ( inputArguments.length() > 2 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    inputArgumentResults.create(2);

                    Demo::HeaterStatus RetHeaterStatus = Demo::HeaterStatus_Off;
                    Demo::Priority RetPriority = Demo::Priority_Low;
                    UaVariant value;
                    UaStatus tmpRet;

                    Demo::HeaterStatus HeaterStatus;
                    value = inputArguments[ 0 ];
                    HeaterStatus = Demo::HeaterStatus_Off;
                    OpcUa_Int32 intHeaterStatusTmp;
                    tmpRet = value.toInt32(intHeaterStatusTmp);
                    if (tmpRet.isGood())
                    {
                        HeaterStatus = (Demo::HeaterStatus) intHeaterStatusTmp;
                    }
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    Demo::Priority Priority;
                    value = inputArguments[ 1 ];
                    Priority = Demo::Priority_Low;
                    OpcUa_Int32 intPriorityTmp;
                    tmpRet = value.toInt32(intPriorityTmp);
                    if (tmpRet.isGood())
                    {
                        Priority = (Demo::Priority) intPriorityTmp;
                    }
                    inputArgumentResults[ 1 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->Demo_Method_EnumTest(serviceContext, HeaterStatus, Priority, RetHeaterStatus, RetPriority);
                    }
                    if ( ret.isGood() )
                    {
                        outputArguments.create(2);

                        value.setInt32(RetHeaterStatus);
                        outputArguments[ 0 ] = *(OpcUa_Variant*)(const OpcUa_Variant*)value;
                        value.detach();

                        value.setInt32(RetPriority);
                        outputArguments[ 1 ] = *(OpcUa_Variant*)(const OpcUa_Variant*)value;
                        value.detach();
                    }
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_Method_Multiply, getNameSpaceIndex()))
        {
                if ( inputArguments.length() < 2 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else if ( inputArguments.length() > 2 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    inputArgumentResults.create(2);

                    OpcUa_Double result = 0;
                    UaVariant value;
                    UaStatus tmpRet;

                    OpcUa_Double a;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toDouble(a);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    OpcUa_Double b;
                    value = inputArguments[ 1 ];
                    tmpRet = value.toDouble(b);
                    inputArgumentResults[ 1 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->Demo_Method_Multiply(serviceContext, a, b, result);
                    }
                    if ( ret.isGood() )
                    {
                        outputArguments.create(1);

                        value.setDouble(result);
                        outputArguments[ 0 ] = *(OpcUa_Variant*)(const OpcUa_Variant*)value;
                        value.detach();
                    }
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_Method_VectorAdd, getNameSpaceIndex()))
        {
                if ( inputArguments.length() < 2 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else if ( inputArguments.length() > 2 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    inputArgumentResults.create(2);

                    Demo::Vector result;
                    UaVariant value;
                    UaStatus tmpRet;

                    Demo::Vector v1;
                    value = inputArguments[ 0 ];
                    UaExtensionObject extv1;
                    value.toExtensionObject(extv1);
                    tmpRet = v1.setVector(extv1, OpcUa_True);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    Demo::Vector v2;
                    value = inputArguments[ 1 ];
                    UaExtensionObject extv2;
                    value.toExtensionObject(extv2);
                    tmpRet = v2.setVector(extv2, OpcUa_True);
                    inputArgumentResults[ 1 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->Demo_Method_VectorAdd(serviceContext, v1, v2, result);
                    }
                    if ( ret.isGood() )
                    {
                        outputArguments.create(1);

                        result.toVariant(outputArguments[ 0 ]);
                    }
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_DynamicNodes_CreateDynamicNode, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_DynamicNodes_CreateDynamicNode(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_DynamicNodes_DeleteDynamicNode, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_DynamicNodes_DeleteDynamicNode(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_CTT_Methods_MethodIO, getNameSpaceIndex()))
        {
                if ( inputArguments.length() < 2 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else if ( inputArguments.length() > 2 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    inputArgumentResults.create(2);

                    OpcUa_UInt32 Sum = 0;
                    UaVariant value;
                    UaStatus tmpRet;

                    OpcUa_UInt32 Summand1;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toUInt32(Summand1);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    OpcUa_UInt32 Summand2;
                    value = inputArguments[ 1 ];
                    tmpRet = value.toUInt32(Summand2);
                    inputArgumentResults[ 1 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->Demo_CTT_Methods_MethodIO(serviceContext, Summand1, Summand2, Sum);
                    }
                    if ( ret.isGood() )
                    {
                        outputArguments.create(1);

                        value.setUInt32(Sum);
                        outputArguments[ 0 ] = *(OpcUa_Variant*)(const OpcUa_Variant*)value;
                        value.detach();
                    }
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_CTT_Methods_MethodNoArgs, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_CTT_Methods_MethodNoArgs(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_CTT_NodeClasses_Method, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_CTT_NodeClasses_Method(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method1, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_CTT_References_Has3ForwardReferences2_Method1(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method2, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_CTT_References_Has3ForwardReferences2_Method2(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences2_Method3, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_CTT_References_Has3ForwardReferences2_Method3(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_SetSimulationSpeed, getNameSpaceIndex()))
        {
                if ( inputArguments.length() < 1 )
                {
                    ret = OpcUa_BadArgumentsMissing;
                }
                else if ( inputArguments.length() > 1 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    inputArgumentResults.create(1);

                    UaVariant value;
                    UaStatus tmpRet;

                    OpcUa_UInt32 speed;
                    value = inputArguments[ 0 ];
                    tmpRet = value.toUInt32(speed);
                    inputArgumentResults[ 0 ] = tmpRet.statusCode();
                    if (tmpRet.isNotGood()) {ret = tmpRet;}

                    if ( ret.isGood() )
                    {
                        ret = this->Demo_SetSimulationSpeed(serviceContext, speed);
                    }
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_StartSimulation, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_StartSimulation(serviceContext);
                }

        }
        else if (pMethodHandleNodeId->m_methodNodeId == UaNodeId(DemoId_Demo_StopSimulation, getNameSpaceIndex()))
        {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadTooManyArguments;
                }
                else
                {
                    ret = this->Demo_StopSimulation(serviceContext);
                }

        }
        else
        {
            assert(false);
            ret = OpcUa_BadInvalidArgument;
        }
        OpcUa_ReferenceParameter(inputArgumentResults);
    }
    else
    {
        assert(false);
        ret = OpcUa_BadInvalidArgument;
    }
    return ret;
}

#endif // SUPPORT_Method_Server_Facet

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/




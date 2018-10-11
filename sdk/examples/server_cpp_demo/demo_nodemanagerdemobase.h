/******************************************************************************
** demo_nodemanagerdemobase.h
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

#ifndef _DEMO_NODEMANAGERDEMOBASE_H__
#define _DEMO_NODEMANAGERDEMOBASE_H__

#include "uamutex.h"
#include "uabasenodes.h"
#include "nodemanagerbase.h"
#include "methodmanager.h"
#include "uaobjecttypes.h"
#include "demo_identifiers.h"
#include "demo_vector.h"
#include "demo_datatypes.h"

/** Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
*/
namespace Demo {

/** Generated base class for NodeManager for Demo information model.
 *
 *  This abstract class contains the auto generated code for the NodeManagerDemo.
 *  This class is used to create the nodes defined for the namespace http://www.unifiedautomation.com/DemoServer/
 *  and to select the methods for UA Method calls if the UA Methods are not defined at an ObjectType.
 *  Methods that are defined at an ObjectType are selected at the <ObjectTypeName>Base class.
 *
 */
class CPP_DEMO_EXPORT NodeManagerDemoBase : public NodeManagerBase
#if SUPPORT_Method_Server_Facet
    , public MethodManager
#endif
{
    UA_DISABLE_COPY(NodeManagerDemoBase);
    NodeManagerDemoBase();
public:
    /* construction / destruction */
    NodeManagerDemoBase(OpcUa_Boolean firesEvents, OpcUa_Int32 nHashTableSize = 10007);
    virtual ~NodeManagerDemoBase(){};

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
    virtual UaStatus Demo_History_StartLogging(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_History_StopLogging(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_Method_DoSomethingAfter10s(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_Method_EnumTest(const ServiceContext& serviceContext,
        Demo::HeaterStatus HeaterStatus,
        Demo::Priority Priority,
        Demo::HeaterStatus& RetHeaterStatus,
        Demo::Priority& RetPriority) = 0;
    virtual UaStatus Demo_Method_Multiply(const ServiceContext& serviceContext,
        OpcUa_Double a /**<[in] parameter a */,
        OpcUa_Double b /**<[in] parameter b */,
        OpcUa_Double& result /**<[out] result = a * b.*/) = 0;
    virtual UaStatus Demo_Method_VectorAdd(const ServiceContext& serviceContext,
        const Demo::Vector& v1,
        const Demo::Vector& v2,
        Demo::Vector& result) = 0;
    virtual UaStatus Demo_DynamicNodes_CreateDynamicNode(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_DynamicNodes_DeleteDynamicNode(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_CTT_Methods_MethodIO(const ServiceContext& serviceContext,
        OpcUa_UInt32 Summand1,
        OpcUa_UInt32 Summand2,
        OpcUa_UInt32& Sum) = 0;
    virtual UaStatus Demo_CTT_Methods_MethodNoArgs(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_CTT_NodeClasses_Method(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences2_Method1(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences2_Method2(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences2_Method3(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_SetSimulationSpeed(const ServiceContext& serviceContext,
        OpcUa_UInt32 speed /**<[in] Simulation interval [ms]. */) = 0;
    virtual UaStatus Demo_StartSimulation(const ServiceContext& serviceContext) = 0;
    virtual UaStatus Demo_StopSimulation(const ServiceContext& serviceContext) = 0;
    //- Method management -----------------------------------------------------------------
#endif // SUPPORT_Method_Server_Facet
private:
    UaStatus createDataTypeNodes();
    UaStatus createReferenceTypeNodes();
    UaStatus addObjectsFolderDemo(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStatic(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArrays(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysBaseDataType(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysBoolean(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysByteString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysDateTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysDuration(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysExpandedNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysGuid(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysImagePNG(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysInteger(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysLocalizedText(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysNumber(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysNumericRange(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysQualifiedName(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysServerState(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysStatusCode(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysStructure(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysUInteger(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysUtcTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysVector(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysWorkOrder(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticArraysXmlElement(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalar(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarBaseDataType(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarBoolean(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarByteString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarDateTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarDuration(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarExpandedNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarGuid(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarHeaterStatus(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarImagePNG(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarInteger(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarLocalizedText(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarNumber(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarNumericRange(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarPriority(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarQualifiedName(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarServerState(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStatusCode(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStructures(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStructuresOptionalFields(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStructuresOptionSet(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStructuresStructure(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStructuresUnion(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStructuresVector(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarStructuresWorkOrder(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarUInteger(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarUtcTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStaticScalarXmlElement(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamic(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArrays(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysBoolean(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysByteString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysDateTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysExpandedNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysGuid(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysLocalizedText(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysQualifiedName(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysStatusCode(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicArraysXmlElement(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalar(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarBoolean(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarByteString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarDateTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarExpandedNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarGuid(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarImageGIF(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarLocalizedText(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarQualifiedName(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarQuality_DynamicValue(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarQuality_StaticValue(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarStatusCode(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicScalarXmlElement(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoHistory(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoHistoryByteWithHistory(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoHistoryDataLoggerActive(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoHistoryDoubleWithHistory(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoHistoryStartLogging(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoHistoryStopLogging(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoMethod(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoMethodDoSomethingAfter10s(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoMethodEnumTest(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoMethodMultiply(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoMethodVectorAdd(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoEvents(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoEventsMachine(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoEventsSampleEventNotifier(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoEventsTrigger_BaseEvent(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoEventsTrigger_SampleEvent(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRights(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_All(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_AllAll_RO(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_AllAll_RW(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_AllAll_WO(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_John(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_JohnAll_RO_John_RW(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_JohnAll_WO_John_RW(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_JohnJohn_RO(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_JohnJohn_RW(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_JohnJohn_WO(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_Operators(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_OperatorsAll_RO_Operators_RW(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_OperatorsAll_WO_Operators_RW(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_RO(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_RW(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsAccess_OperatorsOperators_WO(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsBrowse_All(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsBrowse_John(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoAccessRightsBrowse_Operators(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoMassfolder_Static(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoMassfolder_Dynamic(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicNodes(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicNodesCreateDynamicNode(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicNodesDeleteDynamicNode(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoDynamicNodesNodeVersion(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoBoilerDemo(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoBoilerDemoBoiler1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTT(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTMethods(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTMethodsMethodIO(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTMethodsMethodNoArgs(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTNodeClasses(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTNodeClassesMethod(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTNodeClassesObject(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTNodeClassesVariable(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPaths(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8folder9(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTPathsfolder1folder2folder3folder4folder5folder6folder7folder8folder9folder10(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferences(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences1ReferencedNode3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences2BaseDataVariable(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Method3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Object(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences2Property(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3InverseReferences(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences3ReferencedNode3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences4(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences4ReferencedNode3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences5(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHas3ForwardReferences5ReferencedNode3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHasInverseAndForwardReferences(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHasInverseAndForwardReferencesReferencedNode(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypes(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTReferencesHasReferencesWithDifferentParentTypesReferencedNode4(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTSecurityAccess(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead_NotCurrentWrite(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentRead_NotUser(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite_NotCurrentRead(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTSecurityAccessAccessLevel_CurrentWrite_NotUser(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStatic(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_Profiles(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArrays(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysBool(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysByteString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysDateTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysDuration(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysGuid(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysLocalizedText(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysQualifiedName(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysVariant(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesArraysXmlElement(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalar(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarBool(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarByteString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarDateTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarDuration(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarEnumeration(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarGuid(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarImage(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageBMP(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageGIF(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarImageJPG(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarImagePNG(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarInteger(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarLocalizedText(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarNodeId(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarNumber(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarQualifiedName(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarUInteger(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarVariant(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticAll_ProfilesScalarXmlElement(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_Profile(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeVariableWithEngineeringUnits(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogTypeVariableWithInstrumentRange(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_Arrays(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileAnalogType_ArraysUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItem(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemDateTime(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemDouble(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemFloat(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemString(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDataItemUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteType(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete4(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeMultiStateDiscrete5(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete1(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete3(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete4(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileDiscreteTypeTwoStateDiscrete5(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteType(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeSByte(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt16(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt32(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoCTTStaticDA_ProfileMultiStateValueDiscreteTypeUInt64(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoUnicodeTest(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoUnicodeTestDie_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoUnicodeTestFranz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoUnicodeTestThe_quick_brown_fox_jumps_over_the_lazy_dog(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoUnicodeTestVoyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoUnicodeTestNode5016(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoIcon(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoIconIcon(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoImages(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoImagesDynamic(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoImagesStatic(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoFiles(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoWorkOrder(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoWorkOrderWorkOrderVariable(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoWorkOrderWorkOrderVariable2(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoSetSimulationSpeed(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoSimulationActive(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoSimulationSpeed(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStartSimulation(const XmlUaNodeFactoryManager &factory);
    UaStatus addObjectsFolderDemoStopSimulation(const XmlUaNodeFactoryManager &factory);
    UaStatus addAdditionalHierarchicalReferences();


protected:
    UaMutex             m_mutex;
    static OpcUa_UInt16 s_namespaceIndex;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __NODEMANAGERDEMOBASE_H__


/******************************************************************************
** demo_nodemanagerdemo.h
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

#ifndef _DEMO_NODEMANAGERDEMO_H__
#define _DEMO_NODEMANAGERDEMO_H__

#include "demo_nodemanagerdemobase.h"

#if SUPPORT_Event_Subscription_Server_Facet
#include "eventmanageruanode.h"
#include "opcua_offnormalalarmtype.h"
#include "opcua_nonexclusivelevelalarmtype.h"
#include "opcua_exclusivelevelalarmtype.h"
#endif // SUPPORT_Event_Subscription_Server_Facet
#include "opcua_foldertype.h"

#include <uagenericoptionsetvalue.h>
#include <uaoptionsetdefinition.h>

#if SUPPORT_Historical_Access
class HistoryManagerCache;
#endif // SUPPORT_Historical_Access

/** Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
*/
namespace Demo {

/** NodeManager for Demo information model.
 *
 *  This class is used to add custom implementation and to implement UA Methods that are
 *  not defined at ObjectTypes.
 *  The base class NodeManagerDemoBase contains the auto generated code for creating the adress space.
 */
class CPP_DEMO_EXPORT NodeManagerDemo :
    public NodeManagerDemoBase,
    public UaThread
{
    UA_DISABLE_COPY(NodeManagerDemo);
    NodeManagerDemo();
public:
    /* construction / destruction */
    NodeManagerDemo(OpcUa_Boolean firesEvents, const UaString& sAppPath, OpcUa_Int32 nHashTableSize = 10007);
    virtual ~NodeManagerDemo();

    //- Interface NodeManagerUaNode -------------------------------------------------------
    virtual UaStatus   afterStartUp();
    virtual UaStatus   beforeShutDown();
    //- Interface NodeManagerUaNode -------------------------------------------------------

#if SUPPORT_Method_Server_Facet
    //- Method management -----------------------------------------------------------------
    virtual UaStatus Demo_History_StartLogging(const ServiceContext& serviceContext);
    virtual UaStatus Demo_History_StopLogging(const ServiceContext& serviceContext);
    virtual UaStatus Demo_Method_DoSomethingAfter10s(const ServiceContext& serviceContext);
    virtual UaStatus Demo_Method_EnumTest(const ServiceContext& serviceContext,
        /*IN*/ Demo::HeaterStatus HeaterStatus,
        /*IN*/ Demo::Priority Priority,
        /*OUT*/ Demo::HeaterStatus& RetHeaterStatus,
        /*OUT*/ Demo::Priority& RetPriority);
    virtual UaStatus Demo_Method_Multiply(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_Double a /**<parameter a */,
        /*IN*/ OpcUa_Double b /**<parameter b */,
        /*OUT*/ OpcUa_Double& result /**<result = a * b.*/);
    virtual UaStatus Demo_Method_VectorAdd(const ServiceContext& serviceContext,
        /*IN*/ const Demo::Vector& v1,
        /*IN*/ const Demo::Vector& v2,
        /*OUT*/ Demo::Vector& result);
    virtual UaStatus Demo_DynamicNodes_CreateDynamicNode(const ServiceContext& serviceContext);
    virtual UaStatus Demo_DynamicNodes_DeleteDynamicNode(const ServiceContext& serviceContext);
    virtual UaStatus Demo_CTT_Methods_MethodIO(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_UInt32 Summand1,
        /*IN*/ OpcUa_UInt32 Summand2,
        /*OUT*/ OpcUa_UInt32& Sum);
    virtual UaStatus Demo_CTT_Methods_MethodNoArgs(const ServiceContext& serviceContext);
    virtual UaStatus Demo_CTT_NodeClasses_Method(const ServiceContext& serviceContext);
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences2_Method1(const ServiceContext& serviceContext);
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences2_Method2(const ServiceContext& serviceContext);
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences2_Method3(const ServiceContext& serviceContext);
    virtual UaStatus Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3(const ServiceContext& serviceContext);
    virtual UaStatus Demo_Images_Dynamic_SetAnimationSpeed(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_UInt32 AnimationSpeed /**<Animation simulation interval [ms]. */);
    virtual UaStatus Demo_SetSimulationSpeed(const ServiceContext& serviceContext,
        /*IN*/ OpcUa_UInt32 speed /**<Simulation interval [ms]. */);
    virtual UaStatus Demo_StartSimulation(const ServiceContext& serviceContext);
    virtual UaStatus Demo_StopSimulation(const ServiceContext& serviceContext);
    virtual UaStatus Demo_MassFolder_AddNodes(const UaNodeId& objectId, OpcUa_UInt32 totalCount);
    //- Method management -----------------------------------------------------------------
#endif // SUPPORT_Method_Server_Facet

    // IOManagerUaNode implementation
#if SUPPORT_Event_Subscription_Server_Facet
    // We overwrite this method only to trigger events or alarm state changes
    virtual void afterSetAttributeValue(
        Session*           pSession,
        UaNode*            pNode,
        OpcUa_Int32        attributeId,
        const UaDataValue& dataValue);
#endif // SUPPORT_Event_Subscription_Server_Facet

    virtual UaStatus readValues(
        const UaVariableArray& arrUaVariables,
        UaDataValueArray&      arrDataValues);

    virtual UaStatus writeValues(
        const UaVariableArray& arrUaVariables,
        const PDataValueArray& arrpDataValues,
        UaStatusCodeArray&     arrStatusCodes);
    // IOManagerUaNode implementation

#if SUPPORT_Event_Subscription_Server_Facet
    //- Event handlers for method calls from UA client need to be implemented by derived class ---
    // Handler for AcknowledgeableConditionType method Acknowledge
    virtual UaStatus OnAcknowledge(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);
    virtual UaStatus OnConfirm(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);
    //--------------------------------------------------------------------------------------------
#endif // SUPPORT_Event_Subscription_Server_Facet

#if SUPPORT_Method_Server_Facet
    //- Method manager handling ----------------
    // We need to handle the methods of the conditions without nodes
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
    //------------------------------------------
#endif // SUPPORT_Method_Server_Facet

    //- Interface UaThread ----------------------------------------------------------------
    void run();
    //- Interface UaThread ----------------------------------------------------------------

private:
    void createTypeNodes();
    void createMassNodes();
    void createFileNodes();
    void createSpecialNodes();
    void createAlarmNodes();
    void setPermissionsForLimitedNodes();
    void initializeScalar();
    void initializeArrays();
    void initializeVariableValue(const UaNodeId& nodeId);
    void initializeMultiState(const UaNodeId& nodeId);
    void initializeWorkOrder(const UaNodeId& nodeId);
    UaVariable* findSimulationVariable(const char *szIdentifier);

private:
    bool                   m_stopThread;
    UaMutex                m_massVarMutex;
    std::list<UaVariable*> m_massVariablesDynamic;
    UaMutexRefCounted*     m_pSharedNodeMutex;
    UaString               m_sAppPath;

#if SUPPORT_Historical_Access
    HistoryManagerCache*                   m_pHistoryManagerCache;
#endif // SUPPORT_Historical_Access
#if SUPPORT_Event_Subscription_Server_Facet
    OpcUa::OffNormalAlarmType*             m_pUaOffNormalAlarm;
    OpcUa::OffNormalAlarmTypeData*         m_pUaOffNormalAlarmData;
    OpcUa::NonExclusiveLevelAlarmType*     m_pUaNonExclusiveLevelAlarm;
    OpcUa::NonExclusiveLevelAlarmTypeData* m_pUaNonExclusiveLevelAlarmData;
    OpcUa::ExclusiveLevelAlarmType*        m_pUaExclusiveLevelAlarm;
    OpcUa::ExclusiveLevelAlarmTypeData*    m_pUaExclusiveLevelAlarmData;
#endif // SUPPORT_Event_Subscription_Server_Facet

    OpcUa_UInt16       m_nodeVersion;
    OpcUa::FolderType* m_pDynamicNode;

    OpcUa_UInt32       m_totalMassNodesStatic;
    OpcUa_UInt32       m_totalMassNodesDynamic;

    OpcUa_UInt16       m_userIdRoot;
    OpcUa_UInt16       m_userIdJoe;
    OpcUa_UInt16       m_userIdJohn;
    OpcUa_UInt16       m_userIdSue;

    OpcUa_UInt16       m_groupIdSecurityAdmin;
    OpcUa_UInt16       m_groupIdConfigAdmin;
    OpcUa_UInt16       m_groupIdUsers;
    OpcUa_UInt16       m_groupIdOperators;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __NODEMANAGERDEMO_H__


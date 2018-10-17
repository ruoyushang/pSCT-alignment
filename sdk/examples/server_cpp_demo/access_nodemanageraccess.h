/******************************************************************************
** access_nodemanageraccess.h
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

#ifndef _ACCESS_NODEMANAGERACCESS_H__
#define _ACCESS_NODEMANAGERACCESS_H__

#include "access_nodemanageraccessbase.h"

/** Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission
*/
namespace Access {

/** NodeManager for Access information model.
 *
 *  This class is used to add custom implementation and to implement UA Methods that are
 *  not defined at ObjectTypes.
 *  The base class NodeManagerAccessBase contains the auto generated code for creating the adress space.
 */
class CPP_ACCESS_EXPORT NodeManagerAccess : public NodeManagerAccessBase
{
    UA_DISABLE_COPY(NodeManagerAccess);
    NodeManagerAccess();
public:
    /* construction / destruction */
    NodeManagerAccess(OpcUa_Boolean firesEvents, OpcUa_Int32 nHashTableSize = 10007);
    virtual ~NodeManagerAccess();

    //- Interface NodeManagerUaNode -------------------------------------------------------
    virtual UaStatus   afterStartUp();
    virtual UaStatus   beforeShutDown();
    //- Interface NodeManagerUaNode -------------------------------------------------------

#if SUPPORT_Event_Subscription_Server_Facet
    //- Event handlers for method calls from UA client need to be implemented by derived class ---
    // Handler for AcknowledgeableConditionType method Acknowledge
    virtual UaStatus OnAcknowledge(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);
    virtual UaStatus OnConfirm(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);
    //--------------------------------------------------------------------------------------------
#endif // SUPPORT_Event_Subscription_Server_Facet

#if SUPPORT_Method_Server_Facet
    //- Method management -----------------------------------------------------------------
    virtual UaStatus Demo_AccessRights_Options_AddAdvancedNodes(const ServiceContext& serviceContext);
    //- Method management -----------------------------------------------------------------
#endif // SUPPORT_Method_Server_Facet
private:
    void createAdditionalReferences();
    void setPermissionsForLimitedNodes();
#if SUPPORT_Method_Server_Facet
    bool m_bAdvancedNodesCreated;
#endif // SUPPORT_Method_Server_Facet

    OpcUa_UInt16       m_userIdRoot;
    OpcUa_UInt16       m_userIdJoe;
    OpcUa_UInt16       m_userIdJohn;
    OpcUa_UInt16       m_userIdSue;

    OpcUa_UInt16       m_groupIdSecurityAdmin;
    OpcUa_UInt16       m_groupIdConfigAdmin;
    OpcUa_UInt16       m_groupIdUsers;
    OpcUa_UInt16       m_groupIdOperators;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/AccessPermission

#endif // #ifndef __NODEMANAGERACCESS_H__


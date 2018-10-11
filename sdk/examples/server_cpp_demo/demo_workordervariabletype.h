/******************************************************************************
** demo_workordervariabletype.h
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

#ifndef __DEMO_WORKORDERVARIABLETYPE_H__
#define __DEMO_WORKORDERVARIABLETYPE_H__

#include "opcua_basedatavariabletype.h"
#include "demo_identifiers.h"
#include "demo_datatypes.h"
#include "demo_workorderstatustype.h"
#include "opcua_basedatavariabletype.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {


/** Implements OPC UA Variables of the type WorkOrderVariableType
 *
 *  **Variable members of the WorkOrderVariableType:**
 *
 *  Browse Name    | DataType            | TypeDefinition       | Modelling Rule | See Also
 *  ---------------|---------------------|----------------------|----------------|-----------------------------------------------
 *  AssetID        | String              | BaseDataVariableType | Mandatory      | \ref getAssetID, \ref setAssetID
 *  ID             | Guid                | BaseDataVariableType | Mandatory      | \ref getID, \ref setID
 *  StartTime      | DateTime            | BaseDataVariableType | Mandatory      | \ref getStartTime, \ref setStartTime
 *  StatusComments | WorkOrderStatusType | BaseDataVariableType | Mandatory      | \ref getStatusComments, \ref setStatusComments
 *
 */
class CPP_DEMO_EXPORT WorkOrderVariableType:
    public OpcUa::BaseDataVariableType
{
    UA_DISABLE_COPY(WorkOrderVariableType);
protected:
    virtual ~WorkOrderVariableType();
public:
    WorkOrderVariableType(
        UaNode*            pParentNode,
        UaVariable*        pInstanceDeclarationVariable,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    WorkOrderVariableType(
        const UaNodeId&    nodeId,
        const UaString&    name,
        OpcUa_UInt16       browseNameNameSpaceIndex,
        const UaVariant&   initialValue,
        OpcUa_Byte         accessLevel,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    WorkOrderVariableType(
        UaBase::Variable*  pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);

    static void createTypes();
    static void clearStaticMembers();

    virtual UaNodeId       typeDefinitionId() const;

    virtual void setAssetID(const UaString& AssetID);
    virtual UaString getAssetID() const;

    virtual void setID(const UaGuid& ID);
    virtual UaGuid getID() const;

    virtual void setStartTime(const UaDateTime& StartTime);
    virtual UaDateTime getStartTime() const;

    virtual void setStatusComments(const Demo::WorkOrderStatusTypes& StatusComments);
    virtual void getStatusComments(Demo::WorkOrderStatusTypes& StatusComments) const;

    virtual OpcUa::BaseDataVariableType* getAssetIDNode();
    virtual OpcUa::BaseDataVariableType* getIDNode();
    virtual OpcUa::BaseDataVariableType* getStartTimeNode();
    virtual OpcUa::BaseDataVariableType* getStatusCommentsNode();

    // NodeAccessInfo management
    virtual void useAccessInfoFromType();
    void useAccessInfoFromInstance(WorkOrderVariableType *pInstance);

protected:
    // Variable nodes
    // Variable AssetID
    static OpcUa::BaseDataVariableType*  s_pAssetID;
    OpcUa::BaseDataVariableType*  m_pAssetID;
    // Variable ID
    static OpcUa::BaseDataVariableType*  s_pID;
    OpcUa::BaseDataVariableType*  m_pID;
    // Variable StartTime
    static OpcUa::BaseDataVariableType*  s_pStartTime;
    OpcUa::BaseDataVariableType*  m_pStartTime;
    // Variable StatusComments
    static OpcUa::BaseDataVariableType*  s_pStatusComments;
    OpcUa::BaseDataVariableType*  m_pStatusComments;


private:
    void initialize(NodeManagerConfig* pNodeConfig);

private:
    static bool s_typeNodesCreated;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __DEMO_WORKORDERVARIABLETYPE_H__


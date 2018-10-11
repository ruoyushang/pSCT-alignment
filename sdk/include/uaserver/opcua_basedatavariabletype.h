/******************************************************************************
** opcua_basedatavariabletype.h
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
** Project: C++ OPC Server SDK information model for namespace http://opcfoundation.org/UA/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#ifndef __OPCUA_BASEDATAVARIABLETYPE_H__
#define __OPCUA_BASEDATAVARIABLETYPE_H__

#include "opcua_basevariabletype.h"
#include "opcua_identifiers.h"

// Namespace for the UA information model http://opcfoundation.org/UA/
namespace OpcUa {


/** Implements OPC UA Variables of the type BaseDataVariableType
 *
 *  This VariableType is used as the type definition whenever there is a
 *  DataVariable having no more concrete type definition available.
 *
 *  This VariableType is the base VariableType for VariableTypes of DataVariables,
 *  and all other VariableTypes of DataVariables shall either directly or
 *  indirectly inherit from it. However, it might not be possible for Servers to
 *  provide all HasSubtype References from this VariableType to its subtypes, and
 *  therefore it is not required to provide this information. The
 *  BaseDataVariableType is a subtype of the BaseVariableType.
 *
 *  There are no References except for HasSubtype References specified for this
 *  VariableType.
 *
 *  See also \ref Doc_OpcUa_BaseDataVariableType for a documentation of the complete Information Model.
 */
class SERVER_CORE_EXPORT BaseDataVariableType:
    public OpcUa::BaseVariableType
{
    UA_DISABLE_COPY(BaseDataVariableType);
protected:
    virtual ~BaseDataVariableType();
public:
    BaseDataVariableType(
        UaNode*            pParentNode,
        UaVariable*        pInstanceDeclarationVariable,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    BaseDataVariableType(
        const UaNodeId&    nodeId,
        const UaString&    name,
        OpcUa_UInt16       browseNameNameSpaceIndex,
        const UaVariant&   initialValue,
        OpcUa_Byte         accessLevel,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    BaseDataVariableType(
        UaBase::Variable*  pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);

    static void createTypes();
    static void clearStaticMembers();

    virtual UaNodeId       typeDefinitionId() const;


    // NodeAccessInfo management
    virtual void useAccessInfoFromType();
    void useAccessInfoFromInstance(BaseDataVariableType *pInstance);

protected:

private:
    void initialize(NodeManagerConfig* pNodeConfig);

private:
    static bool s_typeNodesCreated;
};

} // End namespace for the UA information model http://opcfoundation.org/UA/

#endif // #ifndef __OPCUA_BASEDATAVARIABLETYPE_H__


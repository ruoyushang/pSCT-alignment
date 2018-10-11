/******************************************************************************
** demo_filllevelsensortypebase.h
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

#ifndef __DEMO_FILLLEVELSENSORTYPEBASE_H__
#define __DEMO_FILLLEVELSENSORTYPEBASE_H__

#include "opcua_baseobjecttype.h"
#include "demo_datatypes.h"
#include "basenodes.h"
#include "demo_identifiers.h"
#include "demo_instancefactory_demo.h"
#include "opcua_analogitemtype.h"
#include "uaeuinformation.h"
#include "uarange.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {


/** Generated base class for a FillLevelSensorType.
 *
 *  This class contains the generated base code for the object type FillLevelSensorType
 *  representing an OPC UA ObjectType. This class is used to create the object type and to
 *  create and represent instances of the object type in the server address space.
 *
 *  **Variable members of the FillLevelSensorType:**
 *
 *  Browse Name | DataType | TypeDefinition | Modelling Rule | See Also
 *  ------------|----------|----------------|----------------|-------------------------------------
 *  FillLevel   | Double   | AnalogItemType | Mandatory      | \ref getFillLevel, \ref setFillLevel
 *
 */
class CPP_DEMO_EXPORT FillLevelSensorTypeBase:
    public OpcUa::BaseObjectType
{
    UA_DISABLE_COPY(FillLevelSensorTypeBase);
protected:
    virtual ~FillLevelSensorTypeBase();
public:
    // construction / destruction
    FillLevelSensorTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    FillLevelSensorTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    FillLevelSensorTypeBase(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    virtual UaNodeId       typeDefinitionId() const;


    virtual void setFillLevel(OpcUa_Double FillLevel);
    virtual OpcUa_Double getFillLevel() const;

    virtual OpcUa::AnalogItemType* getFillLevelNode();

    // NodeAccessInfo management
    virtual void useAccessInfoFromType();
    void useAccessInfoFromInstance(FillLevelSensorTypeBase *pInstance);

protected:
    // Variable nodes
    // Variable FillLevel
    static OpcUa::AnalogItemType*  s_pFillLevel;
    OpcUa::AnalogItemType*  m_pFillLevel;



private:
    void initialize();

private:
    static bool s_typeNodesCreated;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __DEMOFILLLEVELSENSORTYPEBASE_H__



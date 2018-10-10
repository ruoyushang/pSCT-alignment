/******************************************************************************
** demo_machinetypebase.h
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

#ifndef __DEMO_MACHINETYPEBASE_H__
#define __DEMO_MACHINETYPEBASE_H__

#include "opcua_baseobjecttype.h"
#include "demo_datatypes.h"
#include "basenodes.h"
#include "demo_identifiers.h"
#include "demo_instancefactory_demo.h"
#include "opcua_analogitemtype.h"
#include "opcua_basedatavariabletype.h"
#include "uarange.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

class TemperatureSensorType;

/** Generated base class for a MachineType.
 *
 *  This class contains the generated base code for the object type MachineType
 *  representing an OPC UA ObjectType. This class is used to create the object type and to
 *  create and represent instances of the object type in the server address space.
 *
 *  **Object members of the MachineType:**
 *
 *  Browse Name       | TypeDefinition        | Modelling Rule | See Also
 *  ------------------|-----------------------|----------------|--------------------------
 *  TemperatureSensor | TemperatureSensorType | Mandatory      | \ref getTemperatureSensor
 *
 *  **Variable members of the MachineType:**
 *
 *  Browse Name  | DataType | TypeDefinition       | Modelling Rule | See Also
 *  -------------|----------|----------------------|----------------|-------------------------------------------
 *  HeaterSwitch | Boolean  | BaseDataVariableType | Mandatory      | \ref getHeaterSwitch, \ref setHeaterSwitch
 *
 */
class CPP_DEMO_EXPORT MachineTypeBase:
    public OpcUa::BaseObjectType
{
    UA_DISABLE_COPY(MachineTypeBase);
protected:
    virtual ~MachineTypeBase();
public:
    // construction / destruction
    MachineTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    MachineTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    MachineTypeBase(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    virtual UaNodeId       typeDefinitionId() const;


    virtual void setHeaterSwitch(OpcUa_Boolean HeaterSwitch);
    virtual OpcUa_Boolean getHeaterSwitch() const;

    virtual OpcUa::BaseDataVariableType* getHeaterSwitchNode();
    virtual Demo::TemperatureSensorType* getTemperatureSensor();

    // NodeAccessInfo management
    virtual void useAccessInfoFromType();
    void useAccessInfoFromInstance(MachineTypeBase *pInstance);

protected:
    // Object nodes
    // Object TemperatureSensor
    static Demo::TemperatureSensorType*  s_pTemperatureSensor;
    Demo::TemperatureSensorType*  m_pTemperatureSensor;

    // Variable nodes
    // Variable HeaterSwitch
    static OpcUa::BaseDataVariableType*  s_pHeaterSwitch;
    OpcUa::BaseDataVariableType*  m_pHeaterSwitch;



private:
    void initialize();

private:
    static bool s_typeNodesCreated;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __DEMOMACHINETYPEBASE_H__



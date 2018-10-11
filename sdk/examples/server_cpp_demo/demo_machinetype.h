/******************************************************************************
** demo_machinetype.h
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

#ifndef __DEMO_MACHINETYPE_H__
#define __DEMO_MACHINETYPE_H__

#include "demo_machinetypebase.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {


/** @brief Class implementing the UaObject interface for the MachineType.
 *
 * OPC UA Objects are used to represent systems, system components, real-world objects and software
 * objects. They have the NodeClass @ref L3UaNodeClassObject. The detailed description of Objects and their attributes
 * can be found in the general description of the @ref L3UaNodeClassObject node class.
 *
 *  **Object members of the MachineType:**
 *
 *  Browse Name       | Modelling Rule | See Also
 *  ------------------|----------------|------------------------------------------------------------------
 *  TemperatureSensor | Mandatory      | \ref MachineTypeBase::getTemperatureSensor "getTemperatureSensor"
 *
 *  **Variable members of the MachineType:**
 *
 *  Browse Name  | Modelling Rule | See Also
 *  -------------|----------------|-----------------------------------------------------------------------------------------------------------------
 *  HeaterSwitch | Mandatory      | \ref MachineTypeBase::getHeaterSwitch "getHeaterSwitch", \ref MachineTypeBase::setHeaterSwitch "setHeaterSwitch"
 *
 */
class CPP_DEMO_EXPORT MachineType:
    public MachineTypeBase
{
    UA_DISABLE_COPY(MachineType);
protected:
    // destruction
    virtual ~MachineType();
public:
    // construction
    MachineType(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    MachineType(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    MachineType(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();


protected:

private:
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __DEMOMACHINETYPE_H__


/******************************************************************************
** demo_boilertype.h
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

#ifndef __DEMO_BOILERTYPE_H__
#define __DEMO_BOILERTYPE_H__

#include "demo_boilertypebase.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {


/** @brief Class implementing the UaObject interface for the BoilerType.
 *
 * OPC UA Objects are used to represent systems, system components, real-world objects and software
 * objects. They have the NodeClass @ref L3UaNodeClassObject. The detailed description of Objects and their attributes
 * can be found in the general description of the @ref L3UaNodeClassObject node class.
 *
 *  **Object members of the BoilerType:**
 *
 *  Browse Name       | Modelling Rule | See Also
 *  ------------------|----------------|-----------------------------------------------------------------
 *  FillLevelSensor   | Mandatory      | \ref BoilerTypeBase::getFillLevelSensor "getFillLevelSensor"
 *  TemperatureSensor | Mandatory      | \ref BoilerTypeBase::getTemperatureSensor "getTemperatureSensor"
 *
 *  **Variable members of the BoilerType:**
 *
 *  Browse Name         | Modelling Rule | See Also
 *  --------------------|----------------|-------------------------------------------------------------------------------------------------------------------------------------------
 *  FillLevelSetPoint   | Mandatory      | \ref BoilerTypeBase::getFillLevelSetPoint "getFillLevelSetPoint", \ref BoilerTypeBase::setFillLevelSetPoint "setFillLevelSetPoint"
 *  HeaterStatus        | Mandatory      | \ref BoilerTypeBase::getHeaterStatus "getHeaterStatus", \ref BoilerTypeBase::setHeaterStatus "setHeaterStatus"
 *  TemperatureSetPoint | Mandatory      | \ref BoilerTypeBase::getTemperatureSetPoint "getTemperatureSetPoint", \ref BoilerTypeBase::setTemperatureSetPoint "setTemperatureSetPoint"
 *
 *  **Method members of the BoilerType:**
 *
 *  Browse Name | Modelling Rule
 *  ------------|---------------
 *  \ref Fill   | Mandatory
 *  \ref Heat   | Mandatory
 *
 */
class CPP_DEMO_EXPORT BoilerType:
    public BoilerTypeBase
{
    UA_DISABLE_COPY(BoilerType);
protected:
    // destruction
    virtual ~BoilerType();
public:
    // construction
    BoilerType(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    BoilerType(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    BoilerType(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    // UaBoilerType method Fill
    virtual UaStatus Fill(
        const ServiceContext& serviceContext /**<General context for the service calls containing
                                                 information like the session object,
                                                 return diagnostic mask and timeout hint.*/,
        /*IN*/ OpcUa_Double SetPoint /**<FillLevel Setpoint [l] */);
    // UaBoilerType method Heat
    virtual UaStatus Heat(
        const ServiceContext& serviceContext /**<General context for the service calls containing
                                                 information like the session object,
                                                 return diagnostic mask and timeout hint.*/,
        /*IN*/ OpcUa_Double SetPoint /**<Temperature Setpoint [°C] */);

protected:

private:
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __DEMOBOILERTYPE_H__


/******************************************************************************
** demo_boilertypebase.h
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

#ifndef __DEMO_BOILERTYPEBASE_H__
#define __DEMO_BOILERTYPEBASE_H__

#include "opcua_baseobjecttype.h"
#include "demo_datatypes.h"
#include "basenodes.h"
#include "demo_identifiers.h"
#include "demo_instancefactory_demo.h"
#include "opcua_analogitemtype.h"
#include "opcua_basedatavariabletype.h"
#include "uaargument.h"
#include "uaeuinformation.h"
#include "uarange.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

class FillLevelSensorType;
class TemperatureSensorType;

/** Generated base class for a BoilerType.
 *
 *  This class contains the generated base code for the object type BoilerType
 *  representing an OPC UA ObjectType. This class is used to create the object type and to
 *  create and represent instances of the object type in the server address space.
 *
 *  **Object members of the BoilerType:**
 *
 *  Browse Name       | TypeDefinition        | Modelling Rule | See Also
 *  ------------------|-----------------------|----------------|--------------------------
 *  FillLevelSensor   | FillLevelSensorType   | Mandatory      | \ref getFillLevelSensor
 *  TemperatureSensor | TemperatureSensorType | Mandatory      | \ref getTemperatureSensor
 *
 *  **Variable members of the BoilerType:**
 *
 *  Browse Name         | DataType     | TypeDefinition       | Modelling Rule | See Also
 *  --------------------|--------------|----------------------|----------------|---------------------------------------------------------
 *  FillLevelSetPoint   | Double       | BaseDataVariableType | Mandatory      | \ref getFillLevelSetPoint, \ref setFillLevelSetPoint
 *  HeaterStatus        | HeaterStatus | BaseDataVariableType | Mandatory      | \ref getHeaterStatus, \ref setHeaterStatus
 *  TemperatureSetPoint | Double       | BaseDataVariableType | Mandatory      | \ref getTemperatureSetPoint, \ref setTemperatureSetPoint
 *
 */
class CPP_DEMO_EXPORT BoilerTypeBase:
    public OpcUa::BaseObjectType
{
    UA_DISABLE_COPY(BoilerTypeBase);
protected:
    virtual ~BoilerTypeBase();
public:
    // construction / destruction
    BoilerTypeBase(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    BoilerTypeBase(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    BoilerTypeBase(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    virtual UaNodeId       typeDefinitionId() const;


    //- Interface MethodManager -----------------------------------------------------------
    /** Call method of an UA object. */
    virtual UaStatus beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments);
    virtual UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);
    //- Interface MethodManager -----------------------------------------------------------

    virtual UaStatus Fill(
        const ServiceContext& serviceContext /**<General context for the service calls containing
                                                 information like the session object,
                                                 return diagnostic mask and timeout hint.*/,
        OpcUa_Double SetPoint /**<[in] FillLevel Setpoint [l] */) = 0;
    virtual UaStatus Heat(
        const ServiceContext& serviceContext /**<General context for the service calls containing
                                                 information like the session object,
                                                 return diagnostic mask and timeout hint.*/,
        OpcUa_Double SetPoint /**<[in] Temperature Setpoint [\302\260C] */) = 0;

    virtual void setFillLevelSetPoint(OpcUa_Double FillLevelSetPoint);
    virtual OpcUa_Double getFillLevelSetPoint() const;

    virtual void setHeaterStatus(Demo::HeaterStatus HeaterStatus);
    virtual Demo::HeaterStatus getHeaterStatus() const;

    virtual void setTemperatureSetPoint(OpcUa_Double TemperatureSetPoint);
    virtual OpcUa_Double getTemperatureSetPoint() const;

    virtual Demo::FillLevelSensorType* getFillLevelSensor();
    virtual OpcUa::BaseDataVariableType* getFillLevelSetPointNode();
    virtual OpcUa::BaseDataVariableType* getHeaterStatusNode();
    virtual Demo::TemperatureSensorType* getTemperatureSensor();
    virtual OpcUa::BaseDataVariableType* getTemperatureSetPointNode();
    virtual OpcUa::BaseMethod* getFill();
    virtual OpcUa::BaseMethod* getHeat();

    // NodeAccessInfo management
    virtual void useAccessInfoFromType();
    void useAccessInfoFromInstance(BoilerTypeBase *pInstance);

protected:
    // Object nodes
    // Object FillLevelSensor
    static Demo::FillLevelSensorType*  s_pFillLevelSensor;
    Demo::FillLevelSensorType*  m_pFillLevelSensor;
    // Object TemperatureSensor
    static Demo::TemperatureSensorType*  s_pTemperatureSensor;
    Demo::TemperatureSensorType*  m_pTemperatureSensor;

    // Variable nodes
    // Variable FillLevelSetPoint
    static OpcUa::BaseDataVariableType*  s_pFillLevelSetPoint;
    OpcUa::BaseDataVariableType*  m_pFillLevelSetPoint;
    // Variable HeaterStatus
    static OpcUa::BaseDataVariableType*  s_pHeaterStatus;
    OpcUa::BaseDataVariableType*  m_pHeaterStatus;
    // Variable TemperatureSetPoint
    static OpcUa::BaseDataVariableType*  s_pTemperatureSetPoint;
    OpcUa::BaseDataVariableType*  m_pTemperatureSetPoint;


    // Method nodes
    static OpcUa::BaseMethod* s_pFillMethod;
    OpcUa::BaseMethod*  m_pFillMethod;
    static OpcUa::BaseMethod* s_pHeatMethod;
    OpcUa::BaseMethod*  m_pHeatMethod;


private:
    void initialize();

private:
    static bool s_typeNodesCreated;
};

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __DEMOBOILERTYPEBASE_H__



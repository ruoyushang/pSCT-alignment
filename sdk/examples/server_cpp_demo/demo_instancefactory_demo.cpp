/******************************************************************************
** demo_instancefactory_demo.cpp
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


#include "demo_instancefactory_demo.h"
#include "demo_identifiers.h"
#include "demo_workordervariabletype.h"
#include "demo_boilertype.h"
#include "demo_filllevelsensortype.h"
#include "demo_machinetype.h"
#include "demo_temperaturesensortype.h"
#include "demo_datatypes.h"
#include "demo_accessrights.h"
#include "demo_structurewithoptionalfields.h"
#include "demo_uniontest.h"
#include "demo_vector.h"
#include "demo_workorderstatustype.h"
#include "demo_workordertype.h"


namespace Demo
{

InstanceFactoryDemo::InstanceFactoryDemo(OpcUa_UInt16 namespaceIndex)
: XmlUaNodeFactoryNamespace(namespaceIndex)
{
}

OpcUa::BaseVariableType* InstanceFactoryDemo::createVariable(
    UaBase::Variable *pVariable,
    XmlUaNodeFactoryManager *pFactory,
    NodeManagerConfig* pNodeConfig,
    UaMutexRefCounted* pSharedMutex)
{
    UA_ASSERT(pVariable->typeDefinitionId().namespaceIndex() == namespaceIndex());
    if (pVariable->typeDefinitionId().namespaceIndex() == namespaceIndex())
    {
        switch (pVariable->typeDefinitionId().identifierNumeric())
        {
        case DemoId_WorkOrderVariableType:
            return new Demo::WorkOrderVariableType(pVariable, pFactory, pNodeConfig, pSharedMutex);
        default:
            UA_ASSERT(false);
            return createGenericVariable(pVariable, pFactory, pNodeConfig, pSharedMutex);
        }
    }
    return NULL;
}

OpcUa::BaseObjectType* InstanceFactoryDemo::createObject(
    UaBase::Object *pObject,
    XmlUaNodeFactoryManager *pFactory,
    NodeManagerConfig* pNodeConfig,
    UaMutexRefCounted* pSharedMutex)
{
    UA_ASSERT(pObject->typeDefinitionId().namespaceIndex() == namespaceIndex());
    if (pObject->typeDefinitionId().namespaceIndex() == namespaceIndex())
    {
        switch (pObject->typeDefinitionId().identifierNumeric())
        {
        case DemoId_BoilerType:
            return new Demo::BoilerType(pObject, pFactory, pNodeConfig, pSharedMutex);
        case DemoId_FillLevelSensorType:
            return new Demo::FillLevelSensorType(pObject, pFactory, pNodeConfig, pSharedMutex);
        case DemoId_MachineType:
            return new Demo::MachineType(pObject, pFactory, pNodeConfig, pSharedMutex);
        case DemoId_TemperatureSensorType:
            return new Demo::TemperatureSensorType(pObject, pFactory, pNodeConfig, pSharedMutex);
        default:
            UA_ASSERT(false);
            return createGenericObject(pObject, pFactory, pNodeConfig, pSharedMutex);
        }
    }
    return NULL;
}

/**Returns default values for all data types provided by the Namespace http://www.unifiedautomation.com/DemoServer/ */
UaVariant InstanceFactoryDemo::defaultValue(const UaNodeId &dataTypeId, OpcUa_Int32 valueRank) const
{
    UA_ASSERT(dataTypeId.namespaceIndex() == namespaceIndex());
    UaVariant value;
    if (dataTypeId.namespaceIndex() == namespaceIndex())
    {
        if (valueRank == -1)
        {
            switch (dataTypeId.identifierNumeric())
            {
            case DemoId_HeaterStatus:
                value.setInt32((OpcUa_Int32)HeaterStatus_Off);
                break;
            case DemoId_Priority:
                value.setInt32((OpcUa_Int32)Priority_Low);
                break;
            case DemoId_AccessRights:
            {
                AccessRights tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_StructureWithOptionalFields:
            {
                StructureWithOptionalFields tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_UnionTest:
            {
                UnionTest tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_Vector:
            {
                Vector tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_WorkOrderStatusType:
            {
                WorkOrderStatusType tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_WorkOrderType:
            {
                WorkOrderType tmp;
                tmp.toVariant(value);
                break;
            }
            default:
                UA_ASSERT(false);
                break;
            }
        }
        else if (valueRank == 1)
        {
            switch (dataTypeId.identifierNumeric())
            {
            case DemoId_HeaterStatus:
                {
                    UaInt32Array int32Array;
                    value.setInt32Array(int32Array, OpcUa_True);
                }
                break;
            case DemoId_Priority:
                {
                    UaInt32Array int32Array;
                    value.setInt32Array(int32Array, OpcUa_True);
                }
                break;
            case DemoId_AccessRights:
            {
                AccessRightss tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_StructureWithOptionalFields:
            {
                StructureWithOptionalFieldss tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_UnionTest:
            {
                UnionTests tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_Vector:
            {
                Vectors tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_WorkOrderStatusType:
            {
                WorkOrderStatusTypes tmp;
                tmp.toVariant(value);
                break;
            }
            case DemoId_WorkOrderType:
            {
                WorkOrderTypes tmp;
                tmp.toVariant(value);
                break;
            }
            default:
                UA_ASSERT(false);
                break;
            }
        }
    }
    return value;
}

};

/******************************************************************************
** opcuaplc_instancefactory_plcopen.h
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
** Project: C++ OPC Server SDK information model for namespace http://PLCopen.org/OpcUa/IEC61131-3/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/


#ifndef __INSTSTANCEFACTORY_PLCopen_
#define __INSTSTANCEFACTORY_PLCopen_
#include <basenodes.h>
#include "opcua_basevariabletype.h"
#include "opcua_baseobjecttype.h"
#include <instancefactory.h>
#include "opcuaplc_identifiers.h"

class XmlUaNodeFactoryManager;

namespace OpcUaPlc
{

class UAMODELS_EXPORT InstanceFactoryPLCopen : public XmlUaNodeFactoryNamespace
{
public:
    InstanceFactoryPLCopen(OpcUa_UInt16 namespaceIndex);

    virtual OpcUa::BaseVariableType* createVariable(
        UaBase::Variable *pVariable,
        XmlUaNodeFactoryManager *pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    virtual OpcUa::BaseObjectType* createObject(
        UaBase::Object *pObject,
        XmlUaNodeFactoryManager *pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    virtual UaVariant defaultValue(const UaNodeId &dataTypeId, OpcUa_Int32 valueRank) const;
};

};
#endif //__INSTSTANCEFACTORY_PLCopen_

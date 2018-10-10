/******************************************************************************
** demo_workordertype.h
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


#ifndef __DEMO_WORKORDERTYPE_H__
#define __DEMO_WORKORDERTYPE_H__

#include <opcua_proxystub.h>

#include "uabase.h"
#include "uaguid.h"
#include "uastring.h"
#include "uadatetime.h"
#include "uaarraytemplates.h"
#include "demo_identifiers.h"
#include "demo_datatypes.h"

class UaExtensionObject;
class UaVariant;
class UaDataValue;

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

class CPP_DEMO_EXPORT WorkOrderTypePrivate;
class CPP_DEMO_EXPORT WorkOrderStatusTypes;

/**
 *  @brief Wrapper class for the UA stack structure Demo_WorkOrderType.
 *
 *  This class encapsulates the native Demo_WorkOrderType structure
 *  and handles memory allocation and cleanup for you.
 *  WorkOrderType uses implicit sharing to avoid needless copying and to boost the performance.
 *  Only if you modify a shared WorkOrderType it creates a copy for that (copy-on-write).
 *  So assigning another WorkOrderType or passing it as parameter needs constant time and is nearly as fast as assigning a pointer.
 */
class CPP_DEMO_EXPORT WorkOrderType
{
    DEMO_DECLARE_PRIVATE(WorkOrderType)
public:
    WorkOrderType();
    WorkOrderType(const WorkOrderType &other);
    WorkOrderType(const Demo_WorkOrderType &other);
    WorkOrderType(
        const UaGuid& ID,
        const UaString& AssetID,
        const UaDateTime& StartTime,
        const Demo::WorkOrderStatusTypes& StatusComments
        );
    WorkOrderType(const UaExtensionObject &extensionObject);
    WorkOrderType(const OpcUa_ExtensionObject &extensionObject);
    WorkOrderType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    WorkOrderType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    ~WorkOrderType();

    void clear();

    bool operator==(const WorkOrderType &other) const;
    bool operator!=(const WorkOrderType &other) const;

    WorkOrderType& operator=(const WorkOrderType &other);

    Demo_WorkOrderType* copy() const;
    void copyTo(Demo_WorkOrderType *pDst) const;

    static Demo_WorkOrderType* clone(const Demo_WorkOrderType& source);
    static void cloneTo(const Demo_WorkOrderType& source, Demo_WorkOrderType& copy);

    void attach(const Demo_WorkOrderType *pValue);
    Demo_WorkOrderType* detach(Demo_WorkOrderType* pDst);

    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    void toExtensionObject(UaExtensionObject &extensionObject) const;
    void toExtensionObject(OpcUa_ExtensionObject &extensionObject) const;
    void toExtensionObject(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    void toExtensionObject(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    OpcUa_StatusCode setWorkOrderType(const UaExtensionObject &extensionObject);
    OpcUa_StatusCode setWorkOrderType(const OpcUa_ExtensionObject &extensionObject);
    OpcUa_StatusCode setWorkOrderType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setWorkOrderType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    void setWorkOrderType(
        const UaGuid& ID,
        const UaString& AssetID,
        const UaDateTime& StartTime,
        const Demo::WorkOrderStatusTypes& StatusComments
        );

    UaGuid getID() const;
    UaString getAssetID() const;
    UaDateTime getStartTime() const;
    void getStatusComments(Demo::WorkOrderStatusTypes& StatusComments) const;

    void setID(const UaGuid& ID);
    void setAssetID(const UaString& AssetID);
    void setStartTime(const UaDateTime& StartTime);
    void setStatusComments(const Demo::WorkOrderStatusTypes& StatusComments);
};

class WorkOrderTypes
{
public:
    WorkOrderTypes();
    WorkOrderTypes(const WorkOrderTypes &other);
    WorkOrderTypes(OpcUa_Int32 length, Demo_WorkOrderType* data);
    virtual ~WorkOrderTypes();

    WorkOrderTypes& operator=(const WorkOrderTypes &other);
    Demo_WorkOrderType& operator[](OpcUa_UInt32 index);
    const Demo_WorkOrderType& operator[](OpcUa_UInt32 index) const;

    void attach(OpcUa_UInt32 length, Demo_WorkOrderType* data);
    void attach(OpcUa_Int32 length, Demo_WorkOrderType* data);
    Demo_WorkOrderType* detach();

    bool operator==(const WorkOrderTypes &other) const;
    bool operator!=(const WorkOrderTypes &other) const;

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const Demo_WorkOrderType* rawData() const {return m_data;}
    inline Demo_WorkOrderType* rawData() {return m_data;}
    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setWorkOrderTypes(const UaVariant &variant);
    OpcUa_StatusCode setWorkOrderTypes(const OpcUa_Variant &variant);
    OpcUa_StatusCode setWorkOrderTypes(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setWorkOrderTypes(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setWorkOrderTypes(OpcUa_Int32 length, Demo_WorkOrderType* data);

    static WorkOrderTypes empty;
private:
    OpcUa_UInt32 m_noOfElements;
    Demo_WorkOrderType* m_data;
};

} // namespace Demo

#endif // __DEMO_WORKORDERTYPE_H__


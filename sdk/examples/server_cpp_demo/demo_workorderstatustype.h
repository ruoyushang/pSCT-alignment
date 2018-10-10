/******************************************************************************
** demo_workorderstatustype.h
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


#ifndef __DEMO_WORKORDERSTATUSTYPE_H__
#define __DEMO_WORKORDERSTATUSTYPE_H__

#include <opcua_proxystub.h>

#include "uabase.h"
#include "ualocalizedtext.h"
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

class CPP_DEMO_EXPORT WorkOrderStatusTypePrivate;

/**
 *  @brief Wrapper class for the UA stack structure Demo_WorkOrderStatusType.
 *
 *  This class encapsulates the native Demo_WorkOrderStatusType structure
 *  and handles memory allocation and cleanup for you.
 *  WorkOrderStatusType uses implicit sharing to avoid needless copying and to boost the performance.
 *  Only if you modify a shared WorkOrderStatusType it creates a copy for that (copy-on-write).
 *  So assigning another WorkOrderStatusType or passing it as parameter needs constant time and is nearly as fast as assigning a pointer.
 */
class CPP_DEMO_EXPORT WorkOrderStatusType
{
    DEMO_DECLARE_PRIVATE(WorkOrderStatusType)
public:
    WorkOrderStatusType();
    WorkOrderStatusType(const WorkOrderStatusType &other);
    WorkOrderStatusType(const Demo_WorkOrderStatusType &other);
    WorkOrderStatusType(
        const UaString& Actor,
        const UaDateTime& Timestamp,
        const UaLocalizedText& Comment
        );
    WorkOrderStatusType(const UaExtensionObject &extensionObject);
    WorkOrderStatusType(const OpcUa_ExtensionObject &extensionObject);
    WorkOrderStatusType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    WorkOrderStatusType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    ~WorkOrderStatusType();

    void clear();

    bool operator==(const WorkOrderStatusType &other) const;
    bool operator!=(const WorkOrderStatusType &other) const;

    WorkOrderStatusType& operator=(const WorkOrderStatusType &other);

    Demo_WorkOrderStatusType* copy() const;
    void copyTo(Demo_WorkOrderStatusType *pDst) const;

    static Demo_WorkOrderStatusType* clone(const Demo_WorkOrderStatusType& source);
    static void cloneTo(const Demo_WorkOrderStatusType& source, Demo_WorkOrderStatusType& copy);

    void attach(const Demo_WorkOrderStatusType *pValue);
    Demo_WorkOrderStatusType* detach(Demo_WorkOrderStatusType* pDst);

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

    OpcUa_StatusCode setWorkOrderStatusType(const UaExtensionObject &extensionObject);
    OpcUa_StatusCode setWorkOrderStatusType(const OpcUa_ExtensionObject &extensionObject);
    OpcUa_StatusCode setWorkOrderStatusType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setWorkOrderStatusType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    void setWorkOrderStatusType(
        const UaString& Actor,
        const UaDateTime& Timestamp,
        const UaLocalizedText& Comment
        );

    UaString getActor() const;
    UaDateTime getTimestamp() const;
    UaLocalizedText getComment() const;

    void setActor(const UaString& Actor);
    void setTimestamp(const UaDateTime& Timestamp);
    void setComment(const UaLocalizedText& Comment);
};

class WorkOrderStatusTypes
{
public:
    WorkOrderStatusTypes();
    WorkOrderStatusTypes(const WorkOrderStatusTypes &other);
    WorkOrderStatusTypes(OpcUa_Int32 length, Demo_WorkOrderStatusType* data);
    virtual ~WorkOrderStatusTypes();

    WorkOrderStatusTypes& operator=(const WorkOrderStatusTypes &other);
    Demo_WorkOrderStatusType& operator[](OpcUa_UInt32 index);
    const Demo_WorkOrderStatusType& operator[](OpcUa_UInt32 index) const;

    void attach(OpcUa_UInt32 length, Demo_WorkOrderStatusType* data);
    void attach(OpcUa_Int32 length, Demo_WorkOrderStatusType* data);
    Demo_WorkOrderStatusType* detach();

    bool operator==(const WorkOrderStatusTypes &other) const;
    bool operator!=(const WorkOrderStatusTypes &other) const;

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const Demo_WorkOrderStatusType* rawData() const {return m_data;}
    inline Demo_WorkOrderStatusType* rawData() {return m_data;}
    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setWorkOrderStatusTypes(const UaVariant &variant);
    OpcUa_StatusCode setWorkOrderStatusTypes(const OpcUa_Variant &variant);
    OpcUa_StatusCode setWorkOrderStatusTypes(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setWorkOrderStatusTypes(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setWorkOrderStatusTypes(OpcUa_Int32 length, Demo_WorkOrderStatusType* data);

    static WorkOrderStatusTypes empty;
private:
    OpcUa_UInt32 m_noOfElements;
    Demo_WorkOrderStatusType* m_data;
};

} // namespace Demo

#endif // __DEMO_WORKORDERSTATUSTYPE_H__


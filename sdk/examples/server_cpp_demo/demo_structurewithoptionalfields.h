/******************************************************************************
** demo_structurewithoptionalfields.h
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


#ifndef __DEMO_STRUCTUREWITHOPTIONALFIELDS_H__
#define __DEMO_STRUCTUREWITHOPTIONALFIELDS_H__

#include <opcua_proxystub.h>

#include "uabase.h"
#include "uastring.h"
#include "uaarraytemplates.h"
#include "demo_identifiers.h"
#include "demo_datatypes.h"

class UaExtensionObject;
class UaVariant;
class UaDataValue;

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

class CPP_DEMO_EXPORT StructureWithOptionalFieldsPrivate;

/**
 *  @brief Wrapper class for the UA stack structure Demo_StructureWithOptionalFields.
 *
 *  This class encapsulates the native Demo_StructureWithOptionalFields structure
 *  and handles memory allocation and cleanup for you.
 *  StructureWithOptionalFields uses implicit sharing to avoid needless copying and to boost the performance.
 *  Only if you modify a shared StructureWithOptionalFields it creates a copy for that (copy-on-write).
 *  So assigning another StructureWithOptionalFields or passing it as parameter needs constant time and is nearly as fast as assigning a pointer.
 */
class CPP_DEMO_EXPORT StructureWithOptionalFields
{
    DEMO_DECLARE_PRIVATE(StructureWithOptionalFields)
public:
    StructureWithOptionalFields();
    StructureWithOptionalFields(const StructureWithOptionalFields &other);
    StructureWithOptionalFields(const Demo_StructureWithOptionalFields &other);
    StructureWithOptionalFields(
        OpcUa_Int32 MandatoryInt32,
        const UaStringArray& MandatoryStringArray
        );
    StructureWithOptionalFields(const UaExtensionObject &extensionObject);
    StructureWithOptionalFields(const OpcUa_ExtensionObject &extensionObject);
    StructureWithOptionalFields(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    StructureWithOptionalFields(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    ~StructureWithOptionalFields();

    void clear();

    bool operator==(const StructureWithOptionalFields &other) const;
    bool operator!=(const StructureWithOptionalFields &other) const;

    StructureWithOptionalFields& operator=(const StructureWithOptionalFields &other);

    Demo_StructureWithOptionalFields* copy() const;
    void copyTo(Demo_StructureWithOptionalFields *pDst) const;

    static Demo_StructureWithOptionalFields* clone(const Demo_StructureWithOptionalFields& source);
    static void cloneTo(const Demo_StructureWithOptionalFields& source, Demo_StructureWithOptionalFields& copy);

    void attach(const Demo_StructureWithOptionalFields *pValue);
    Demo_StructureWithOptionalFields* detach(Demo_StructureWithOptionalFields* pDst);

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

    OpcUa_StatusCode setStructureWithOptionalFields(const UaExtensionObject &extensionObject);
    OpcUa_StatusCode setStructureWithOptionalFields(const OpcUa_ExtensionObject &extensionObject);
    OpcUa_StatusCode setStructureWithOptionalFields(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setStructureWithOptionalFields(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    void setStructureWithOptionalFields(
        OpcUa_Int32 MandatoryInt32,
        const UaStringArray& MandatoryStringArray
        );

    OpcUa_Int32 getMandatoryInt32() const;
    bool isOptionalInt32Set() const;
    OpcUa_Int32 getOptionalInt32(bool *ok = 0) const;
    void getMandatoryStringArray(UaStringArray& MandatoryStringArray) const;
    bool isOptionalStringArraySet() const;
    void getOptionalStringArray(UaStringArray& OptionalStringArray, bool *ok = 0) const;

    void setMandatoryInt32(OpcUa_Int32 MandatoryInt32);
    void setOptionalInt32(OpcUa_Int32 OptionalInt32);
    void unsetOptionalInt32();
    void setMandatoryStringArray(const UaStringArray& MandatoryStringArray);
    void setOptionalStringArray(const UaStringArray& OptionalStringArray);
    void unsetOptionalStringArray();
};

class StructureWithOptionalFieldss
{
public:
    StructureWithOptionalFieldss();
    StructureWithOptionalFieldss(const StructureWithOptionalFieldss &other);
    StructureWithOptionalFieldss(OpcUa_Int32 length, Demo_StructureWithOptionalFields* data);
    virtual ~StructureWithOptionalFieldss();

    StructureWithOptionalFieldss& operator=(const StructureWithOptionalFieldss &other);
    Demo_StructureWithOptionalFields& operator[](OpcUa_UInt32 index);
    const Demo_StructureWithOptionalFields& operator[](OpcUa_UInt32 index) const;

    void attach(OpcUa_UInt32 length, Demo_StructureWithOptionalFields* data);
    void attach(OpcUa_Int32 length, Demo_StructureWithOptionalFields* data);
    Demo_StructureWithOptionalFields* detach();

    bool operator==(const StructureWithOptionalFieldss &other) const;
    bool operator!=(const StructureWithOptionalFieldss &other) const;

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const Demo_StructureWithOptionalFields* rawData() const {return m_data;}
    inline Demo_StructureWithOptionalFields* rawData() {return m_data;}
    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setStructureWithOptionalFieldss(const UaVariant &variant);
    OpcUa_StatusCode setStructureWithOptionalFieldss(const OpcUa_Variant &variant);
    OpcUa_StatusCode setStructureWithOptionalFieldss(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setStructureWithOptionalFieldss(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setStructureWithOptionalFieldss(OpcUa_Int32 length, Demo_StructureWithOptionalFields* data);

    static StructureWithOptionalFieldss empty;
private:
    OpcUa_UInt32 m_noOfElements;
    Demo_StructureWithOptionalFields* m_data;
};

} // namespace Demo

#endif // __DEMO_STRUCTUREWITHOPTIONALFIELDS_H__


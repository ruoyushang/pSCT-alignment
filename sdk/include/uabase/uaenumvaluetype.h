/******************************************************************************
** uaenumvaluetype.h
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
** Project: C++ OPC SDK base module
**
** Portable UaEnumValueType class.
**
******************************************************************************/
#ifndef UAENUMVALUETYPE_H
#define UAENUMVALUETYPE_H

#include <opcua_proxystub.h>

#include "uabase.h"
#include "ualocalizedtext.h"
#include "uaarraytemplates.h"

class UaExtensionObject;
class UaVariant;
class UaDataValue;

class UABASE_EXPORT UaEnumValueTypePrivate;

/** @ingroup CppBaseLibraryClass
 *  @brief Wrapper class for the UA stack structure OpcUa_EnumValueType.
 *
 *  This class encapsulates the native OpcUa_EnumValueType structure
 *  and handles memory allocation and cleanup for you.
 *  UaEnumValueType uses implicit sharing to avoid needless copying and to boost the performance.
 *  Only if you modify a shared EnumValueType it creates a copy for that (copy-on-write).
 *  So assigning another UaEnumValueType or passing it as parameter needs constant time and is nearly as fast as assigning a pointer.
 *
 *  A mapping between a value of an enumerated type and a name and description.
 *
 *  When this type is used in an array representing human readable representations
 *  of an enumeration, each Value shall be unique in that array.
 */
class UABASE_EXPORT UaEnumValueType
{
    UA_DECLARE_PRIVATE(UaEnumValueType)
public:
    UaEnumValueType();
    UaEnumValueType(const UaEnumValueType &other);
    UaEnumValueType(const OpcUa_EnumValueType &other);
    UaEnumValueType(
        OpcUa_Int64 value,
        const UaLocalizedText& displayName,
        const UaLocalizedText& description
        );
    UaEnumValueType(const UaExtensionObject &extensionObject);
    UaEnumValueType(const OpcUa_ExtensionObject &extensionObject);
    UaEnumValueType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    UaEnumValueType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    ~UaEnumValueType();

    void clear();

    bool operator==(const UaEnumValueType &other) const;
    bool operator!=(const UaEnumValueType &other) const;

    UaEnumValueType& operator=(const UaEnumValueType &other);

    OpcUa_EnumValueType* copy() const;
    void copyTo(OpcUa_EnumValueType *pDst) const;

    static OpcUa_EnumValueType* clone(const OpcUa_EnumValueType& source);
    static void cloneTo(const OpcUa_EnumValueType& source, OpcUa_EnumValueType& copy);

    void attach(OpcUa_EnumValueType *pValue);
    OpcUa_EnumValueType* detach(OpcUa_EnumValueType* pDst);

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

    OpcUa_StatusCode setEnumValueType(const UaExtensionObject &extensionObject);
    OpcUa_StatusCode setEnumValueType(const OpcUa_ExtensionObject &extensionObject);
    OpcUa_StatusCode setEnumValueType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setEnumValueType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    void setEnumValueType(
        OpcUa_Int64 value,
        const UaLocalizedText& displayName,
        const UaLocalizedText& description
        );

    OpcUa_Int64 getValue() const;
    UaLocalizedText getDisplayName() const;
    UaLocalizedText getDescription() const;

    void setValue(OpcUa_Int64 value);
    void setDisplayName(const UaLocalizedText& displayName);
    void setDescription(const UaLocalizedText& description);
};

/** @ingroup CppBaseLibraryClass
 *  @brief Array class for the UA stack structure OpcUa_EnumValueType.
 *
 *  This class encapsulates an array of the native OpcUa_EnumValueType structure
 *  and handles memory allocation and cleanup for you.
 *  @see UaEnumValueType for information about the encapsulated structure.
 */
class UABASE_EXPORT UaEnumValueTypes
{
public:
    UaEnumValueTypes();
    UaEnumValueTypes(const UaEnumValueTypes &other);
    UaEnumValueTypes(OpcUa_Int32 length, OpcUa_EnumValueType* data);
    virtual ~UaEnumValueTypes();

    UaEnumValueTypes& operator=(const UaEnumValueTypes &other);
    const OpcUa_EnumValueType& operator[](OpcUa_UInt32 index) const;
    OpcUa_EnumValueType& operator[](OpcUa_UInt32 index);

    bool operator==(const UaEnumValueTypes &other) const;
    bool operator!=(const UaEnumValueTypes &other) const;

    void attach(OpcUa_UInt32 length, OpcUa_EnumValueType* data);
    void attach(OpcUa_Int32 length, OpcUa_EnumValueType* data);
    OpcUa_EnumValueType* detach();

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const OpcUa_EnumValueType* rawData() const {return m_data;}
    inline OpcUa_EnumValueType* rawData() {return m_data;}

    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setEnumValueTypes(const UaVariant &variant);
    OpcUa_StatusCode setEnumValueTypes(const OpcUa_Variant &variant);
    OpcUa_StatusCode setEnumValueTypes(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setEnumValueTypes(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setEnumValueTypes(OpcUa_Int32 length, OpcUa_EnumValueType* data);

private:
    OpcUa_UInt32 m_noOfElements;
    OpcUa_EnumValueType* m_data;
};

#endif // UAENUMVALUETYPE_H


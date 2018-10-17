/******************************************************************************
** demo_accessrights.h
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

#ifndef __DEMO_ACCESSRIGHTS_H__
#define __DEMO_ACCESSRIGHTS_H__

#include <opcua_proxystub.h>
#include <uabase.h>
#include <uaoptionset.h>
#include <opcua_types.h>
#include "demo_datatypes.h"
#include "demo_identifiers.h"

class UaVariant;
class UaDataValue;

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

class CPP_DEMO_EXPORT AccessRights
{
public:
    enum Mask
    {
        Mask_Read = 1,
        Mask_Write = 2,
        Mask_Execute = 4
    };
    enum Bit
    {
        Bit_Read = 0,
        Bit_Write = 1,
        Bit_Execute = 2
    };

    AccessRights(const AccessRights &other);
    AccessRights(const Demo_AccessRights &other);
    AccessRights(Bit bit);
    AccessRights(Mask mask);
    AccessRights();
    AccessRights(const UaOptionSet &mask);
    AccessRights(const UaVariant &variant);
    AccessRights(const OpcUa_Variant &variant);
    AccessRights(const UaDataValue &dataValue);
    AccessRights(const OpcUa_DataValue &dataValue);

    AccessRights operator|(const AccessRights &other) const;
    AccessRights operator|(Bit bit) const;
    AccessRights operator|(Mask mask) const;
    AccessRights& operator|=(const AccessRights &other);
    AccessRights& operator|=(Bit bit);
    AccessRights& operator|=(Mask mask);

    AccessRights operator&(const AccessRights &other) const;
    AccessRights operator&(Bit bit) const;
    AccessRights operator&(Mask mask) const;
    AccessRights& operator&=(const AccessRights &other);
    AccessRights& operator&=(Bit bit);
    AccessRights& operator&=(Mask mask);

    AccessRights operator^(const AccessRights &other) const;
    AccessRights operator^(Bit bit) const;
    AccessRights operator^(Mask mask) const;
    AccessRights& operator^=(const AccessRights &other);
    AccessRights& operator^=(Bit bit);
    AccessRights& operator^=(Mask mask);

    AccessRights operator~() const;

    AccessRights& operator=(const AccessRights &other);

    bool operator==(const AccessRights &other) const;
    bool operator!=(const AccessRights &other) const;

    operator UaOptionSet() const;

    bool hasFlag(Bit bit) const;
    bool hasFlag(Mask mask) const;

    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;

    void setInvalidBit(Bit bit);
    void setInvalidBit(Mask mask);
    void setValidBit(Bit bit, bool value = true);
    void setValidBit(Mask mask, bool value = true);
    void setAllBitsInvalid();

    void apply(const AccessRights& other);

    void copyTo(Demo_AccessRights *pDst) const;

private:
    void applyLength();
    void setValidBits(OpcUa_OptionSet &data) const;

    UaOptionSet m_data;
};

class AccessRightss
{
public:
    AccessRightss();
    AccessRightss(const AccessRightss &other);
    AccessRightss(OpcUa_Int32 length, Demo_AccessRights* data);
    virtual ~AccessRightss();

    AccessRightss& operator=(const AccessRightss &other);
    Demo_AccessRights& operator[](OpcUa_UInt32 index);
    const Demo_AccessRights& operator[](OpcUa_UInt32 index) const;

    void attach(OpcUa_UInt32 length, Demo_AccessRights* data);
    void attach(OpcUa_Int32 length, Demo_AccessRights* data);
    Demo_AccessRights* detach();

    bool operator==(const AccessRightss &other) const;
    bool operator!=(const AccessRightss &other) const;

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const Demo_AccessRights* rawData() const {return m_data;}
    inline Demo_AccessRights* rawData() {return m_data;}

    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setAccessRightss(const UaVariant &variant);
    OpcUa_StatusCode setAccessRightss(const OpcUa_Variant &variant);
    OpcUa_StatusCode setAccessRightss(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setAccessRightss(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setAccessRightss(OpcUa_Int32 length, Demo_AccessRights* data);

    static AccessRightss empty;
private:
    OpcUa_UInt32 m_noOfElements;
    Demo_AccessRights* m_data;
};

} // namespace Demo

#endif // __DEMO_ACCESSRIGHTS_H__

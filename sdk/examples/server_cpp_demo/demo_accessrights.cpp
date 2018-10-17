/******************************************************************************
** demo_accessrights.cpp
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

#include "demo_accessrights.h"
#include <uavariant.h>
#include <uadatavalue.h>
#include <uadatetime.h>

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

static const OpcUa_Int32 g_noOfBytes = 1;
static const OpcUa_Byte g_validBits = 7;

AccessRights::AccessRights(const AccessRights &other)
{
    m_data = other.m_data;
}

AccessRights::AccessRights(const Demo_AccessRights &other)
{
    m_data = (const OpcUa_OptionSet&) other;
}

AccessRights::AccessRights(Bit bit)
{
    OpcUa_OptionSet data;
    data.Value.Length = g_noOfBytes;
    data.Value.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.Value.Data, 0, g_noOfBytes);
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    data.Value.Data[iByteNumber] = mask;

    setValidBits(data);
    m_data.attach(&data);
}

AccessRights::AccessRights(Mask mask)
{
    OpcUa_OptionSet data;
    data.Value.Length = g_noOfBytes;
    data.Value.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.Value.Data, 0, g_noOfBytes);
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    data.Value.Data[iByte] = (OpcUa_Byte) iValue;

    setValidBits(data);
    m_data.attach(&data);
}

AccessRights::AccessRights()
{
    OpcUa_OptionSet data;
    data.Value.Length = g_noOfBytes;
    data.Value.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.Value.Data, 0, g_noOfBytes);
    setValidBits(data);
    m_data.attach(&data);
}

AccessRights::AccessRights(const UaOptionSet &mask)
{
    m_data = mask;
    applyLength();
}

AccessRights::AccessRights(const UaVariant &variant)
{
    UaExtensionObject e;
    OpcUa_StatusCode status = variant.toExtensionObject(e);
    if (OpcUa_IsGood(status))
    {
        const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)e;
        Demo_AccessRights *pAccessRights = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(AccessRights, pExtensionObject);

        if (pAccessRights)
        {
            m_data = UaOptionSet((const OpcUa_OptionSet&) *pAccessRights);
        }
    }
    applyLength();
}

AccessRights::AccessRights(const OpcUa_Variant &variant)
{
    if (variant.Datatype == OpcUaType_ExtensionObject && variant.ArrayType == OpcUa_VariantArrayType_Scalar)
    {
        Demo_AccessRights *pAccessRights = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(AccessRights, variant.Value.ExtensionObject);

        if (pAccessRights)
        {
            m_data = UaOptionSet((const OpcUa_OptionSet&) *pAccessRights);
        }
    }
    applyLength();
}

AccessRights::AccessRights(const UaDataValue &dataValue)
{
    if (dataValue.value()->Datatype == OpcUaType_ExtensionObject && dataValue.value()->ArrayType == OpcUa_VariantArrayType_Scalar)
    {
        Demo_AccessRights *pAccessRights = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(AccessRights, dataValue.value()->Value.ExtensionObject);

        if (pAccessRights)
        {
            m_data = UaOptionSet((const OpcUa_OptionSet&) *pAccessRights);
        }
    }
    applyLength();
}

AccessRights::AccessRights(const OpcUa_DataValue &dataValue)
{
    if (dataValue.Value.Datatype == OpcUaType_ExtensionObject && dataValue.Value.ArrayType == OpcUa_VariantArrayType_Scalar)
    {
        Demo_AccessRights *pAccessRights = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(AccessRights, dataValue.Value.Value.ExtensionObject);

        if (pAccessRights)
        {
            m_data = UaOptionSet((const OpcUa_OptionSet&) *pAccessRights);
        }
    }
    applyLength();
}

AccessRights AccessRights::operator|(const AccessRights &other) const
{
    OpcUa_OptionSet data;
    data.Value.Length = g_noOfBytes;
    data.Value.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.Value.Data, 0, g_noOfBytes);
    for (OpcUa_Int32 i=0; i<g_noOfBytes; i++)
    {
        data.Value.Data[i] = m_data.getValue().data()[i] | other.m_data.getValue().data()[i];
    }
    setValidBits(data);
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights AccessRights::operator|(Bit bit) const
{
    OpcUa_OptionSet data;
    m_data.copyTo(&data);
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    data.Value.Data[iByteNumber] |= mask;
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights AccessRights::operator|(Mask mask) const
{
    OpcUa_OptionSet data;
    m_data.copyTo(&data);
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    data.Value.Data[iByte] |= (OpcUa_Byte) iValue;
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights& AccessRights::operator|=(const AccessRights &other)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    for (OpcUa_Int32 i=0; i<g_noOfBytes; i++)
    {
        data.Value.Data[i] = data.Value.Data[i] | other.m_data.getValue().data()[i];
    }
    m_data.attach(&data);
    return *this;
}

AccessRights& AccessRights::operator|=(Bit bit)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    data.Value.Data[iByteNumber] |= mask;
    m_data.attach(&data);
    return *this;
}

AccessRights& AccessRights::operator|=(Mask mask)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    data.Value.Data[iByte] |= (OpcUa_Byte) iValue;
    m_data.attach(&data);
    return *this;
}

AccessRights AccessRights::operator&(const AccessRights &other) const
{
    OpcUa_OptionSet data;
    data.Value.Length = g_noOfBytes;
    data.Value.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.Value.Data, 0, g_noOfBytes);
    for (OpcUa_Int32 i=0; i<g_noOfBytes; i++)
    {
        data.Value.Data[i] = m_data.getValue().data()[i] & other.m_data.getValue().data()[i];
    }
    setValidBits(data);
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights AccessRights::operator&(Bit bit) const
{
    OpcUa_OptionSet data;
    m_data.copyTo(&data);
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    data.Value.Data[iByteNumber] &= mask;
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights AccessRights::operator&(Mask mask) const
{
    OpcUa_OptionSet data;
    m_data.copyTo(&data);
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    data.Value.Data[iByte] &= (OpcUa_Byte) iValue;
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights& AccessRights::operator&=(const AccessRights &other)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    for (OpcUa_Int32 i=0; i<g_noOfBytes; i++)
    {
        data.Value.Data[i] = data.Value.Data[i] & other.m_data.getValue().data()[i];
    }
    m_data.attach(&data);
    return *this;
}

AccessRights& AccessRights::operator&=(Bit bit)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    data.Value.Data[iByteNumber] &= mask;
    m_data.attach(&data);
    return *this;
}

AccessRights& AccessRights::operator&=(Mask mask)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    data.Value.Data[iByte] &= (OpcUa_Byte) iValue;
    m_data.attach(&data);
    return *this;
}

AccessRights AccessRights::operator^(const AccessRights &other) const
{
    OpcUa_OptionSet data;
    data.Value.Length = g_noOfBytes;
    data.Value.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.Value.Data, 0, g_noOfBytes);
    for (OpcUa_Int32 i=0; i<g_noOfBytes; i++)
    {
        data.Value.Data[i] = m_data.getValue().data()[i] ^ other.m_data.getValue().data()[i];
    }
    setValidBits(data);
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights AccessRights::operator^(Bit bit) const
{
    OpcUa_OptionSet data;
    m_data.copyTo(&data);
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    data.Value.Data[iByteNumber] ^= mask;
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights AccessRights::operator^(Mask mask) const
{
    OpcUa_OptionSet data;
    m_data.copyTo(&data);
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    data.Value.Data[iByte] ^= (OpcUa_Byte) iValue;
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights& AccessRights::operator^=(const AccessRights &other)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    for (OpcUa_Int32 i=0; i<g_noOfBytes; i++)
    {
        data.Value.Data[i] = data.Value.Data[i] ^ other.m_data.getValue().data()[i];
    }
    m_data.attach(&data);
    return *this;
}

AccessRights& AccessRights::operator^=(Bit bit)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    data.Value.Data[iByteNumber] ^= mask;
    m_data.attach(&data);
    return *this;
}

AccessRights& AccessRights::operator^=(Mask mask)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    data.Value.Data[iByte] ^= (OpcUa_Byte) iValue;
    m_data.attach(&data);
    return *this;
}

AccessRights AccessRights::operator~() const
{
    OpcUa_OptionSet data;
    data.Value.Length = g_noOfBytes;
    data.Value.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.Value.Data, 0, g_noOfBytes);
    for (OpcUa_Int32 i=0; i<g_noOfBytes-1; i++)
    {
        data.Value.Data[i] = ~m_data.getValue().data()[i];
    }
    data.Value.Data[g_noOfBytes-1] = ~m_data.getValue().data()[g_noOfBytes-1] | g_validBits;
    setValidBits(data);
    AccessRights ret;
    ret.m_data.attach(&data);
    return ret;
}

AccessRights& AccessRights::operator=(const AccessRights &other)
{
    m_data = other.m_data;
    return *this;
}

bool AccessRights::operator==(const AccessRights &other) const
{
    return m_data == other.m_data;
}

bool AccessRights::operator!=(const AccessRights &other) const
{
    return m_data != other.m_data;
}

AccessRights::operator UaOptionSet() const
{
    return m_data;
}

bool AccessRights::hasFlag(Bit bit) const
{
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    return (mask & m_data.getValue().data()[iByteNumber]) != 0;
}

bool AccessRights::hasFlag(Mask mask) const
{
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    return (mask & m_data.getValue().data()[iByte]) != 0;
}

void AccessRights::toVariant(UaVariant &variant) const
{
    OpcUa_ExtensionObject extensionObject;
    OpcUa_ExtensionObject_Initialize(&extensionObject);
    Demo_AccessRights *pTmp = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_AccessRights_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pTmp)))
    {
        m_data.copyTo((OpcUa_OptionSet*) pTmp);
    }

    UaExtensionObject e;
    e.attach(&extensionObject);
    variant.setExtensionObject(e, OpcUa_True);
}

void AccessRights::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);

    Demo_AccessRights *pTmp = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_AccessRights_EncodeableType,
                                                            variant.Value.ExtensionObject,
                                                            (OpcUa_Void**)&pTmp)))
    {
        m_data.copyTo((OpcUa_OptionSet*) pTmp);
    }
}

void AccessRights::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void AccessRights::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    OpcUa_Variant_Clear(&dataValue.Value);
    toVariant(dataValue.Value);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}


void AccessRights::setInvalidBit(AccessRights::Bit bit)
{
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    mask = ~mask;
    OpcUa_OptionSet data;
    m_data.detach(&data);
    data.ValidBits.Data[iByteNumber] &= mask;
    m_data.attach(&data);
}

void AccessRights::setInvalidBit(AccessRights::Mask mask)
{
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    OpcUa_Byte bValue = (OpcUa_Byte) iValue;
    bValue = ~bValue;
    OpcUa_OptionSet data;
    m_data.detach(&data);
    data.ValidBits.Data[iByte] &= bValue;
    m_data.attach(&data);
}

void AccessRights::setValidBit(AccessRights::Bit bit, bool value)
{
    int iBit = (int) bit;
    int iByteNumber = iBit / 8;
    int iBitInByte = iBit - iByteNumber * 8;
    OpcUa_Byte mask = 1;
    mask <<= iBitInByte;
    OpcUa_OptionSet data;
    m_data.detach(&data);
    data.ValidBits.Data[iByteNumber] |= mask;
    if (value)
    {
        data.Value.Data[iByteNumber] |= mask;
    }
    else
    {
        mask = ~mask;
        data.Value.Data[iByteNumber] &= mask;
    }
    m_data.attach(&data);
}

void AccessRights::setValidBit(AccessRights::Mask mask, bool value)
{
    int iValue = (int) mask;
    int iByte = 0;
    while (iValue > 255)
    {
        iValue /= 256;
        iByte++;
    }
    OpcUa_Byte bValue = (OpcUa_Byte) iValue;
    OpcUa_OptionSet data;
    m_data.detach(&data);
    data.ValidBits.Data[iByte] |= bValue;
    if (value)
    {
        data.Value.Data[iByte] |= bValue;
    }
    else
    {
        bValue = ~bValue;
        data.Value.Data[iByte] &= bValue;
    }
    m_data.attach(&data);
}

void AccessRights::setAllBitsInvalid()
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    OpcUa_MemSet(data.ValidBits.Data, 0, g_noOfBytes);
    m_data.attach(&data);
}

void AccessRights::apply(const AccessRights &other)
{
    OpcUa_OptionSet data;
    m_data.detach(&data);
    for (int i=0; i<g_noOfBytes; i++)
    {
        if (other.m_data.getValidBits().data()[i] == 255
                || (i == g_noOfBytes - 1 && other.m_data.getValidBits().data()[i] == g_validBits))
        {
            data.Value.Data[i] = other.m_data.getValue().data()[i];
        }
        else if (other.m_data.getValidBits().data()[i] > 0)
        {
            OpcUa_Byte newByte = 0;
            OpcUa_Byte ownByte = data.Value.Data[i];
            OpcUa_Byte otherByte = other.m_data.getValue().data()[i];
            OpcUa_Byte validBitsByte = other.m_data.getValidBits().data()[i];
            OpcUa_Byte factor = 1;
            for (int j=0; j<8; j++)
            {
                if ((validBitsByte & factor) != 0)
                {
                    newByte += otherByte & factor;
                }
                else
                {
                    newByte += ownByte & factor;
                }
                factor *= 2;
            }
            data.Value.Data[i] = newByte;
        }
    }
    m_data.attach(&data);
}

void AccessRights::copyTo(Demo_AccessRights *pDst) const
{
    if (pDst == OpcUa_Null) return;
    m_data.copyTo((OpcUa_OptionSet*) pDst);
}

void AccessRights::applyLength()
{
    if (m_data.getValue().length() != g_noOfBytes)
    {
        UaByteString newValue = m_data.getValue();
        newValue.resize(g_noOfBytes);
        m_data.setValue(newValue);
    }
    if (m_data.getValidBits().length() != g_noOfBytes)
    {
        UaByteString newValidBits = m_data.getValidBits();
        newValidBits.resize(g_noOfBytes);
        m_data.setValidBits(newValidBits);
    }
}

void AccessRights::setValidBits(OpcUa_OptionSet &data) const
{
    data.ValidBits.Length = g_noOfBytes;
    data.ValidBits.Data = (OpcUa_Byte*) OpcUa_Alloc(g_noOfBytes * sizeof (OpcUa_Byte));
    OpcUa_MemSet(data.ValidBits.Data, 255, g_noOfBytes);
    data.ValidBits.Data[g_noOfBytes-1] = g_validBits;
}

/******************************************************************************
** Class AccessRightss
******************************************************************************/
AccessRightss AccessRightss::empty;

AccessRightss::AccessRightss()
: m_noOfElements(0),
  m_data((Demo_AccessRights*)OpcUa_Null)
{
}

AccessRightss::AccessRightss(const AccessRightss &other)
: m_noOfElements(0),
  m_data((Demo_AccessRights*)OpcUa_Null)
{
    operator=(other);
}

AccessRightss::AccessRightss(OpcUa_Int32 length, Demo_AccessRights* data)
: m_noOfElements(0),
  m_data((Demo_AccessRights*)OpcUa_Null)
{
    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_AccessRights*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_AccessRights));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            m_data[i] = data[i];
        }
    }
}

AccessRightss::~AccessRightss()
{
    clear();
}

AccessRightss& AccessRightss::operator=(const AccessRightss &other)
{
    if (this == &other) {return *this;}

    clear();
    if (other.m_noOfElements > 0)
    {
        m_noOfElements = other.m_noOfElements;
        m_data = (Demo_AccessRights*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_AccessRights));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_AccessRights_CopyTo(&(other.m_data[i]), &(m_data[i]));
        }
    }
    return *this;
}

/** Returns the Demo_AccessRights at index position index as a modifiable reference.
 *
 *  @param index Must be valid index position in the array.
 */
Demo_AccessRights& AccessRightss::operator[](OpcUa_UInt32 index)
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Returns the Demo_AccessRights at index position index.
 *
 *  @param index Must be valid index position in the array.
 */
const Demo_AccessRights& AccessRightss::operator[](OpcUa_UInt32 index) const
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because AccessRightss is responsible for it now.
 */
void AccessRightss::attach(OpcUa_UInt32 length, Demo_AccessRights* data)
{
    clear();
    m_data = data;
    m_noOfElements = length;
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because AccessRightss is responsible for it now.
 */
void AccessRightss::attach(OpcUa_Int32 length, Demo_AccessRights* data)
{
    clear();
    if (length >= 0)
    {
        m_data = data;
        m_noOfElements = length;
    }
}

bool AccessRightss::operator==(const AccessRightss &other) const
{
    if (m_noOfElements != other.m_noOfElements) {return false;}

    if (m_noOfElements > 0)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            if (Demo_AccessRights_Compare(&(m_data[i]), &(other.m_data[i])) != OPCUA_EQUAL) {return false;};
        }
    }
    return true;
}

bool AccessRightss::operator!=(const AccessRightss &other) const
{
    return !(operator==(other));
}

/** Detaches the raw data from this class.
 *
 *  The caller takes ownership of the raw data and is responsible for deleting it
 *  by calling OpcUa_Free.
 *  @return The raw data.
 */
Demo_AccessRights* AccessRightss::detach()
{
    Demo_AccessRights* pRet = m_data;
    m_noOfElements = 0;
    m_data = (Demo_AccessRights*)OpcUa_Null;
    return pRet;
}

void AccessRightss::create(OpcUa_UInt32 length)
{
    clear();
    if (length > 0)
    {
        m_data = (Demo_AccessRights*)OpcUa_Alloc(length * sizeof(Demo_AccessRights));
        AccessRights tmp;
        for (OpcUa_UInt32 i = 0; i < length; i++)
        {
            tmp.copyTo(&m_data[i]);
        }
        m_noOfElements = length;
    }
}

void AccessRightss::resize(OpcUa_UInt32 length)
{
    if (length < m_noOfElements)
    {
        if (length != 0)
        {
            for (OpcUa_UInt32 i = length; i < m_noOfElements; i++)
            {
                Demo_AccessRights_Clear(&(m_data[i]));
            }
            Demo_AccessRights* newData = (Demo_AccessRights*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_AccessRights));
            if (newData)
            {
                m_data = newData;
                m_noOfElements = length;
            }
        }
        else
        {
            clear();
        }
    }
    else if (length > m_noOfElements)
    {
        Demo_AccessRights* newData = (Demo_AccessRights*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_AccessRights));
        if (newData)
        {
            AccessRights tmp;
            for (OpcUa_UInt32 i = m_noOfElements; i < length; i++)
            {
                tmp.copyTo(&m_data[i]);
            }
            m_data = newData;
            m_noOfElements = length;
        }
    }
}

void AccessRightss::clear()
{
    if (m_data)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_AccessRights_Clear(&(m_data[i]));
        }
        OpcUa_Free(m_data);
        m_data = (Demo_AccessRights*)OpcUa_Null;
    }
    m_noOfElements = 0;
}

void AccessRightss::toVariant(UaVariant &variant) const
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant);
}

void AccessRightss::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);

    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_AccessRights *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_AccessRights_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        Demo_AccessRights_CopyTo(&m_data[i], pValue);
    }
}

void AccessRightss::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant, bDetach);
}

void AccessRightss::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_AccessRights *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_AccessRights_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        if (bDetach != OpcUa_False)
        {
            *pValue = m_data[i];

            OpcUa_Free(m_data);
            m_data = (Demo_AccessRights*)OpcUa_Null;
            m_noOfElements = 0;
        }
        else
        {
            Demo_AccessRights_CopyTo(&m_data[i], pValue);
        }
    }
}

void AccessRightss::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void AccessRightss::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    toVariant(dataValue.Value);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

void AccessRightss::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void AccessRightss::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    toVariant(dataValue.Value, bDetach);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

OpcUa_StatusCode AccessRightss::setAccessRightss(const UaVariant &variant)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setAccessRightss(*pVariant);
}

OpcUa_StatusCode AccessRightss::setAccessRightss(const OpcUa_Variant &variant)
{
    return setAccessRightss((OpcUa_Variant&)variant, OpcUa_False);
}

OpcUa_StatusCode AccessRightss::setAccessRightss(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setAccessRightss(*pVariant, bDetach);
}

OpcUa_StatusCode AccessRightss::setAccessRightss(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    clear();

    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    if (variant.Datatype == OpcUaType_ExtensionObject &&
        variant.ArrayType == OpcUa_VariantArrayType_Array)
    {
        ret = OpcUa_Good;
        m_noOfElements = variant.Value.Array.Length >= 0 ? variant.Value.Array.Length : 0;
        if (m_noOfElements > 0 && variant.Value.Array.Value.ExtensionObjectArray != OpcUa_Null)
        {
            m_data = (Demo_AccessRights*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_AccessRights));

            OpcUa_UInt32 i = 0;
            for (i = 0; i < m_noOfElements; i++)
            {
                Demo_AccessRights_Initialize(&m_data[i]);

                Demo_AccessRights *pValue = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(AccessRights, &variant.Value.Array.Value.ExtensionObjectArray[i]);
                if (pValue)
                {
                    if (bDetach != OpcUa_False)
                    {
                        m_data[i] = *pValue;
                        Demo_AccessRights_Initialize(pValue);
                    }
                    else
                    {
                        Demo_AccessRights_CopyTo(pValue, &m_data[i]);
                    }
                }
                else
                {
                    ret = OpcUa_BadTypeMismatch;
                    break;
                }
            }
            if (OpcUa_IsNotGood(ret))
            {
                for (OpcUa_UInt32 j = 0; j < i; j++)
                {
                    Demo_AccessRights_Clear(&m_data[j]);
                }
                OpcUa_Free(m_data);
                m_data = (Demo_AccessRights*)OpcUa_Null;
                m_noOfElements = 0;
            }
        }
        else
        {
            m_noOfElements = 0;
        }
    }
    return ret;
}

OpcUa_StatusCode AccessRightss::setAccessRightss(OpcUa_Int32 length, Demo_AccessRights* data)
{
    clear();

    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        OpcUa_UInt32 byteCount = m_noOfElements * sizeof(Demo_AccessRights);
        m_data = (Demo_AccessRights*)OpcUa_Alloc(byteCount);
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_AccessRights_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
    return OpcUa_Good;
}

} // namespace Demo


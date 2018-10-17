/******************************************************************************
** demo_vector.h
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


#ifndef __DEMO_VECTOR_H__
#define __DEMO_VECTOR_H__

#include <opcua_proxystub.h>

#include "uabase.h"
#include "uaarraytemplates.h"
#include "demo_identifiers.h"
#include "demo_datatypes.h"

class UaExtensionObject;
class UaVariant;
class UaDataValue;

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

class CPP_DEMO_EXPORT VectorPrivate;

/**
 *  @brief Wrapper class for the UA stack structure Demo_Vector.
 *
 *  This class encapsulates the native Demo_Vector structure
 *  and handles memory allocation and cleanup for you.
 *  Vector uses implicit sharing to avoid needless copying and to boost the performance.
 *  Only if you modify a shared Vector it creates a copy for that (copy-on-write).
 *  So assigning another Vector or passing it as parameter needs constant time and is nearly as fast as assigning a pointer.
 */
class CPP_DEMO_EXPORT Vector
{
    DEMO_DECLARE_PRIVATE(Vector)
public:
    Vector();
    Vector(const Vector &other);
    Vector(const Demo_Vector &other);
    Vector(
        OpcUa_Double X,
        OpcUa_Double Y,
        OpcUa_Double Z
        );
    Vector(const UaExtensionObject &extensionObject);
    Vector(const OpcUa_ExtensionObject &extensionObject);
    Vector(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    Vector(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    ~Vector();

    void clear();

    bool operator==(const Vector &other) const;
    bool operator!=(const Vector &other) const;

    Vector& operator=(const Vector &other);

    Demo_Vector* copy() const;
    void copyTo(Demo_Vector *pDst) const;

    static Demo_Vector* clone(const Demo_Vector& source);
    static void cloneTo(const Demo_Vector& source, Demo_Vector& copy);

    void attach(const Demo_Vector *pValue);
    Demo_Vector* detach(Demo_Vector* pDst);

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

    OpcUa_StatusCode setVector(const UaExtensionObject &extensionObject);
    OpcUa_StatusCode setVector(const OpcUa_ExtensionObject &extensionObject);
    OpcUa_StatusCode setVector(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setVector(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    void setVector(
        OpcUa_Double X,
        OpcUa_Double Y,
        OpcUa_Double Z
        );

    OpcUa_Double getX() const;
    OpcUa_Double getY() const;
    OpcUa_Double getZ() const;

    void setX(OpcUa_Double X);
    void setY(OpcUa_Double Y);
    void setZ(OpcUa_Double Z);
};

class Vectors
{
public:
    Vectors();
    Vectors(const Vectors &other);
    Vectors(OpcUa_Int32 length, Demo_Vector* data);
    virtual ~Vectors();

    Vectors& operator=(const Vectors &other);
    Demo_Vector& operator[](OpcUa_UInt32 index);
    const Demo_Vector& operator[](OpcUa_UInt32 index) const;

    void attach(OpcUa_UInt32 length, Demo_Vector* data);
    void attach(OpcUa_Int32 length, Demo_Vector* data);
    Demo_Vector* detach();

    bool operator==(const Vectors &other) const;
    bool operator!=(const Vectors &other) const;

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const Demo_Vector* rawData() const {return m_data;}
    inline Demo_Vector* rawData() {return m_data;}
    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setVectors(const UaVariant &variant);
    OpcUa_StatusCode setVectors(const OpcUa_Variant &variant);
    OpcUa_StatusCode setVectors(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setVectors(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setVectors(OpcUa_Int32 length, Demo_Vector* data);

    static Vectors empty;
private:
    OpcUa_UInt32 m_noOfElements;
    Demo_Vector* m_data;
};

} // namespace Demo

#endif // __DEMO_VECTOR_H__


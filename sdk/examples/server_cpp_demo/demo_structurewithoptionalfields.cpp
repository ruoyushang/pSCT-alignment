/******************************************************************************
** demo_structurewithoptionalfields.cpp
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

#include "demo_structurewithoptionalfields.h"

#include <opcua_datetime.h>

#include "uavariant.h"
#include "uadatavalue.h"
#include "uaextensionobject.h"
#include "uaatomic.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

/******************************************************************************
** Class StructureWithOptionalFieldsPrivate
******************************************************************************/
class StructureWithOptionalFieldsPrivate : private Demo_StructureWithOptionalFields
{
    StructureWithOptionalFieldsPrivate(const StructureWithOptionalFieldsPrivate &other);
public:
    StructureWithOptionalFieldsPrivate();
    explicit StructureWithOptionalFieldsPrivate(bool bStatic);
    explicit StructureWithOptionalFieldsPrivate(const Demo_StructureWithOptionalFields &other);
    ~StructureWithOptionalFieldsPrivate();

    int addRef();
    int release();
    inline int refCount() { return m_iRefCnt; }

private:
    int m_iRefCnt;
    friend class StructureWithOptionalFields;
};

/* construction */
StructureWithOptionalFieldsPrivate::StructureWithOptionalFieldsPrivate()
 : m_iRefCnt(0)
{
    Demo_StructureWithOptionalFields_Initialize(this);
}

/*  Static initializer.
 *  This is used internaly to initialize the static shared_emtpy variable.
 *  RefCnt is set to 1 so it will never be deleted.
 */
StructureWithOptionalFieldsPrivate::StructureWithOptionalFieldsPrivate(bool)
 : m_iRefCnt(1)
{
    Demo_StructureWithOptionalFields_Initialize(this);
}


/*  construction
 */
StructureWithOptionalFieldsPrivate::StructureWithOptionalFieldsPrivate(const Demo_StructureWithOptionalFields &other) // [in] the other Demo_StructureWithOptionalFields.
 : m_iRefCnt(0)
{
    Demo_StructureWithOptionalFields_CopyTo(&other, this);
}

/* destruction */
StructureWithOptionalFieldsPrivate::~StructureWithOptionalFieldsPrivate()
{
    Demo_StructureWithOptionalFields_Clear(this);
}

/*  Add a reference to a StructureWithOptionalFieldsPrivate
 *  @return 0 if the object was deleted - otherwise a value bigger than 0.
 *  This method is not thread save regarding the return value. The returned counter is only reliable when it is 0.
 *  Other values are not reliable and should not be used for any logic.
 */
int StructureWithOptionalFieldsPrivate::addRef()
{
    ua_atomic_increment(&m_iRefCnt);
    return m_iRefCnt;
}

/*  Release a StructureWithOptionalFieldsPrivate.
 *  @return the reference count.
 */
int StructureWithOptionalFieldsPrivate::release()
{
    int currentCount = ua_atomic_decrement(&m_iRefCnt);
    if (currentCount == 0)
    {
        delete this;
    }
    return currentCount;
}

StructureWithOptionalFieldsPrivate* shared_empty_StructureWithOptionalFields()
{
    static StructureWithOptionalFieldsPrivate shared_static(true);
    return &shared_static;
}


/******************************************************************************
** Class StructureWithOptionalFields
******************************************************************************/

/** Constructs an instance of the class StructureWithOptionalFields with empty default values
*/
StructureWithOptionalFields::StructureWithOptionalFields()
{
    d_ptr = shared_empty_StructureWithOptionalFields();
    d_ptr->addRef();
}

/** Constructs an instance of the class StructureWithOptionalFields with values from another StructureWithOptionalFields object
 *  @param other the other UaStructureWithOptionalFields used to initialize the object
 */
StructureWithOptionalFields::StructureWithOptionalFields(const StructureWithOptionalFields &other)
{
    d_ptr = other.d_ptr;
    d_ptr->addRef();
}

/** Constructs an instance of the class StructureWithOptionalFields with values from another Demo_StructureWithOptionalFields structure
 *  @param other the other Demo_StructureWithOptionalFields used to initialize the object
 */
StructureWithOptionalFields::StructureWithOptionalFields(const Demo_StructureWithOptionalFields &other)
{
    d_ptr = new StructureWithOptionalFieldsPrivate(other);
    d_ptr->addRef();
}

/** Constructs an instance of the class StructureWithOptionalFields with values provided in the constructor
 */
StructureWithOptionalFields::StructureWithOptionalFields(
        OpcUa_Int32 MandatoryInt32,
        const UaStringArray& MandatoryStringArray
        )
{
    d_ptr = new StructureWithOptionalFieldsPrivate();
    d_ptr->addRef();
    setMandatoryInt32(MandatoryInt32);
    setMandatoryStringArray(MandatoryStringArray);
}

/** Constructs an instance of the class StructureWithOptionalFields initialized with value from a UaExtensionObject
 *  @param extensionObject the extension object containing the StructureWithOptionalFields
 */
StructureWithOptionalFields::StructureWithOptionalFields(const UaExtensionObject &extensionObject)
{
    d_ptr = new StructureWithOptionalFieldsPrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(StructureWithOptionalFields, pExtensionObject);

    if (pStructureWithOptionalFields)
    {
        Demo_StructureWithOptionalFields_CopyTo(pStructureWithOptionalFields, d_ptr);
    }
}

/** Constructs an instance of the class StructureWithOptionalFields initialized with value from an OpcUa_ExtensionObject structure
 *  @param extensionObject the extension object containing the StructureWithOptionalFields
 */
StructureWithOptionalFields::StructureWithOptionalFields(const OpcUa_ExtensionObject &extensionObject)
{
    d_ptr = new StructureWithOptionalFieldsPrivate();
    d_ptr->addRef();

    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(StructureWithOptionalFields, &extensionObject);

    if (pStructureWithOptionalFields)
    {
        Demo_StructureWithOptionalFields_CopyTo(pStructureWithOptionalFields, d_ptr);
    }
}

StructureWithOptionalFields::StructureWithOptionalFields(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new StructureWithOptionalFieldsPrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(StructureWithOptionalFields, pExtensionObject);
    if (pStructureWithOptionalFields)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_StructureWithOptionalFields_CopyTo(pStructureWithOptionalFields, d_ptr);
        }
        else
        {
            (*(Demo_StructureWithOptionalFields*)d_ptr) = *pStructureWithOptionalFields;
            Demo_StructureWithOptionalFields_Initialize(pStructureWithOptionalFields);
            extensionObject.clear();
        }
    }
}

StructureWithOptionalFields::StructureWithOptionalFields(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new StructureWithOptionalFieldsPrivate();
    d_ptr->addRef();

    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(StructureWithOptionalFields, &extensionObject);

    if (pStructureWithOptionalFields)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_StructureWithOptionalFields_CopyTo(pStructureWithOptionalFields, d_ptr);
        }
        else
        {
            (*(Demo_StructureWithOptionalFields*)d_ptr) = *pStructureWithOptionalFields;
            Demo_StructureWithOptionalFields_Initialize(pStructureWithOptionalFields);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
}

/** Destroys the StructureWithOptionalFields object
*/
StructureWithOptionalFields::~StructureWithOptionalFields()
{
    DEMO_D(StructureWithOptionalFields);
    d->release();
}

/** Clear the data of the StructureWithOptionalFields.
*/
void StructureWithOptionalFields::clear()
{
    d_ptr->release();
    d_ptr = shared_empty_StructureWithOptionalFields();
    d_ptr->addRef();
}

/** Compare two StructureWithOptionalFields if they are similar.
 *  @param other the StructureWithOptionalFields to compare.
 *  @return true if similar, false if not.
 */
bool StructureWithOptionalFields::operator==(const StructureWithOptionalFields &other) const
{
    return (Demo_StructureWithOptionalFields_Compare(d_ptr, other.d_ptr) == 0);
}

/** Returns true if the other StructureWithOptionalFields is not equal to this.
 *  @see operator==
 *  @param other the StructureWithOptionalFields to compare.
 *  @return true if other is not equal to this.
 */
bool StructureWithOptionalFields::operator!=(const StructureWithOptionalFields &other) const
{
    return !(operator==(other));
}

/** Assignment operator.
 *  @param other the StructureWithOptionalFields to assign.
 *  @return Assignment operator.
 */
StructureWithOptionalFields& StructureWithOptionalFields::operator=(const StructureWithOptionalFields &other)
{
    other.d_ptr->addRef();
    d_ptr->release();
    d_ptr = other.d_ptr;
    return *this;
}

/** Copy StructureWithOptionalFields data to a newly allocated Demo_StructureWithOptionalFields.
 *  @return data to a newly allocated Demo_StructureWithOptionalFields.
 */
Demo_StructureWithOptionalFields* StructureWithOptionalFields::copy() const
{
    return clone(*d_ptr);
}

/** Copy StructureWithOptionalFields data to an existing Demo_StructureWithOptionalFields structure.
 *  @param pDst the destination of this copy operation.
 */
void StructureWithOptionalFields::copyTo(Demo_StructureWithOptionalFields *pDst) const
{
    if (pDst == OpcUa_Null) return;
    cloneTo(*d_ptr, *pDst);
}

/** Copy Demo_StructureWithOptionalFields data to a newly allocated Demo_StructureWithOptionalFields.
 *  @param source Source to clone.
 *  @return new copy.
 */
Demo_StructureWithOptionalFields* StructureWithOptionalFields::clone(const Demo_StructureWithOptionalFields& source)
{
    Demo_StructureWithOptionalFields* pStructureWithOptionalFields = OpcUa_Null;
    Demo_StructureWithOptionalFields_Copy(&source, &pStructureWithOptionalFields);
    return pStructureWithOptionalFields;
}

/** Copy Demo_StructureWithOptionalFields data to an existing Demo_StructureWithOptionalFields structure.
 *  @param source the source of the clone operation.
 *  @param copy the destination of the clone operation.
 */
void StructureWithOptionalFields::cloneTo(const Demo_StructureWithOptionalFields& source, Demo_StructureWithOptionalFields& copy)
{
    Demo_StructureWithOptionalFields_CopyTo(&source, &copy);
}

/** Attaches the data of the parameter pValue.
 * Don't clear the data of pValue afterwards manually, because StructureWithOptionalFields is responsible for it now.
 */
void StructureWithOptionalFields::attach(const Demo_StructureWithOptionalFields *pValue) //!< [in] Parameter to attach to.
{
    if (pValue == OpcUa_Null) return;
    if (d_ptr->refCount() == 1)
    {
        Demo_StructureWithOptionalFields_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new StructureWithOptionalFieldsPrivate();
        d_ptr->addRef();
    }
    // copy data value structure (no deep copy)
    OpcUa_MemCpy(d_ptr, sizeof(Demo_StructureWithOptionalFields), (void*)pValue, sizeof(Demo_StructureWithOptionalFields));
}

/** Detaches the internal StructureWithOptionalFields structure from this class.
 *  This way you take over the control of releasing the StructureWithOptionalFields data.
 *  If more than one references exist, the data is copied to pDst instead of being detached.
 *  @return If the functions succeeds pDst is returned, otherwise 0 is returned.
 */
Demo_StructureWithOptionalFields* StructureWithOptionalFields::detach(Demo_StructureWithOptionalFields* pDst) //!< [out] Pointer to Demo_StructureWithOptionalFields structure that receives the StructureWithOptionalFields data.
{
    if (pDst == OpcUa_Null) return OpcUa_Null;
    if (d_ptr->refCount() == 1)
    {
        OpcUa_MemCpy(pDst, sizeof(Demo_StructureWithOptionalFields), d_ptr, sizeof(Demo_StructureWithOptionalFields));
        // clear internal data without releasing deeper memory
        Demo_StructureWithOptionalFields_Initialize(d_ptr);
    }
    else
    {
        Demo_StructureWithOptionalFields_CopyTo(d_ptr, pDst);
    }
    d_ptr->release();
    d_ptr = shared_empty_StructureWithOptionalFields();
    d_ptr->addRef();
    return pDst;
}

void StructureWithOptionalFields::toVariant(UaVariant &variant) const
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void StructureWithOptionalFields::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject);
}

void StructureWithOptionalFields::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject, bDetach);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void StructureWithOptionalFields::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject, bDetach);
}

void StructureWithOptionalFields::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void StructureWithOptionalFields::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    OpcUa_Variant_Clear(&dataValue.Value);
    dataValue.Value.Datatype = OpcUaType_ExtensionObject;
    dataValue.Value.ArrayType = OpcUa_VariantArrayType_Scalar;
    dataValue.Value.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(dataValue.Value.Value.ExtensionObject);
    toExtensionObject(*dataValue.Value.Value.ExtensionObject);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

void StructureWithOptionalFields::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void StructureWithOptionalFields::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    OpcUa_Variant_Clear(&dataValue.Value);
    dataValue.Value.Datatype = OpcUaType_ExtensionObject;
    dataValue.Value.ArrayType = OpcUa_VariantArrayType_Scalar;
    dataValue.Value.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(dataValue.Value.Value.ExtensionObject);
    toExtensionObject(*dataValue.Value.Value.ExtensionObject, bDetach);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

void StructureWithOptionalFields::toExtensionObject(UaExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject);
}

void StructureWithOptionalFields::toExtensionObject(OpcUa_ExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_StructureWithOptionalFields_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pStructureWithOptionalFields)))
    {
        Demo_StructureWithOptionalFields_CopyTo(d_ptr, pStructureWithOptionalFields);
    }
}

void StructureWithOptionalFields::toExtensionObject(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject, bDetach);
}

void StructureWithOptionalFields::toExtensionObject(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_StructureWithOptionalFields_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pStructureWithOptionalFields)))
    {
        Demo_StructureWithOptionalFields_Initialize(pStructureWithOptionalFields);
        if (bDetach != OpcUa_False && d_ptr->refCount() == 1)
        {
            *pStructureWithOptionalFields = *d_ptr;
            // clear internal data without releasing deeper memory
            Demo_StructureWithOptionalFields_Initialize(d_ptr);
        }
        else
        {
            Demo_StructureWithOptionalFields_CopyTo(d_ptr, pStructureWithOptionalFields);
        }
        if (bDetach != OpcUa_False)
        {
            d_ptr->release();
            d_ptr = shared_empty_StructureWithOptionalFields();
            d_ptr->addRef();
        }
    }
}

OpcUa_StatusCode StructureWithOptionalFields::setStructureWithOptionalFields(const UaExtensionObject &extensionObject)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setStructureWithOptionalFields(*pExtensionObject);
}

OpcUa_StatusCode StructureWithOptionalFields::setStructureWithOptionalFields(const OpcUa_ExtensionObject &extensionObject)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(StructureWithOptionalFields, &extensionObject);

    if (pStructureWithOptionalFields)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_StructureWithOptionalFields_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new StructureWithOptionalFieldsPrivate();
            d_ptr->addRef();
        }

        Demo_StructureWithOptionalFields_CopyTo(pStructureWithOptionalFields, d_ptr);
    }
    return ret;
}

OpcUa_StatusCode StructureWithOptionalFields::setStructureWithOptionalFields(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setStructureWithOptionalFields(*pExtensionObject, bDetach);
}

OpcUa_StatusCode StructureWithOptionalFields::setStructureWithOptionalFields(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_StructureWithOptionalFields *pStructureWithOptionalFields = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(StructureWithOptionalFields, &extensionObject);

    if (pStructureWithOptionalFields != OpcUa_Null)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_StructureWithOptionalFields_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new StructureWithOptionalFieldsPrivate();
            d_ptr->addRef();
        }

        if (bDetach == OpcUa_False)
        {
            Demo_StructureWithOptionalFields_CopyTo(pStructureWithOptionalFields, d_ptr);
        }
        else
        {
            (*(Demo_StructureWithOptionalFields*)d_ptr) = *pStructureWithOptionalFields;
            Demo_StructureWithOptionalFields_Initialize(pStructureWithOptionalFields);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
    return ret;
}

void StructureWithOptionalFields::setStructureWithOptionalFields(
        OpcUa_Int32 MandatoryInt32,
        const UaStringArray& MandatoryStringArray
        )
{
    if (d_ptr->refCount() == 1)
    {
        Demo_StructureWithOptionalFields_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new StructureWithOptionalFieldsPrivate();
        d_ptr->addRef();
    }
    setMandatoryInt32(MandatoryInt32);
    setMandatoryStringArray(MandatoryStringArray);
}

OpcUa_Int32 StructureWithOptionalFields::getMandatoryInt32() const
{
    return d_ptr->MandatoryInt32;
}

bool StructureWithOptionalFields::isOptionalInt32Set() const
{
    return (d_ptr->EncodingMask & Demo_StructureWithOptionalFields_OptionalFields_OptionalInt32) != 0;
}

OpcUa_Int32 StructureWithOptionalFields::getOptionalInt32(bool *ok) const
{
    if (ok)
    {
        *ok = isOptionalInt32Set();
    }
    return d_ptr->OptionalInt32;
}

void StructureWithOptionalFields::getMandatoryStringArray(UaStringArray& MandatoryStringArray) const
{
    MandatoryStringArray.clear();
    if (d_ptr->NoOfMandatoryStringArray > 0)
    {
        MandatoryStringArray.create(d_ptr->NoOfMandatoryStringArray);
        OpcUa_Int32 i = 0;
        for (i = 0; i < d_ptr->NoOfMandatoryStringArray; i++)
        {
            OpcUa_String_CopyTo(&d_ptr->MandatoryStringArray[i], &MandatoryStringArray[i]);
        }
    }
}

bool StructureWithOptionalFields::isOptionalStringArraySet() const
{
    return (d_ptr->EncodingMask & Demo_StructureWithOptionalFields_OptionalFields_OptionalStringArray) != 0;
}

void StructureWithOptionalFields::getOptionalStringArray(UaStringArray& OptionalStringArray, bool *ok) const
{
    if (ok)
    {
        *ok = isOptionalStringArraySet();
    }
    OptionalStringArray.clear();
    if (d_ptr->NoOfOptionalStringArray > 0)
    {
        OptionalStringArray.create(d_ptr->NoOfOptionalStringArray);
        OpcUa_Int32 i = 0;
        for (i = 0; i < d_ptr->NoOfOptionalStringArray; i++)
        {
            OpcUa_String_CopyTo(&d_ptr->OptionalStringArray[i], &OptionalStringArray[i]);
        }
    }
}

void StructureWithOptionalFields::setMandatoryInt32(OpcUa_Int32 MandatoryInt32)
{
    if (d_ptr->refCount() > 1)
    {
        StructureWithOptionalFieldsPrivate *pNew = new StructureWithOptionalFieldsPrivate((const Demo_StructureWithOptionalFields)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    d_ptr->MandatoryInt32 = MandatoryInt32;
}

void StructureWithOptionalFields::unsetOptionalInt32()
{
    if (d_ptr->refCount() > 1)
    {
        StructureWithOptionalFieldsPrivate *pNew = new StructureWithOptionalFieldsPrivate((const Demo_StructureWithOptionalFields)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }
    d_ptr->OptionalInt32 = 0;
    d_ptr->EncodingMask &= (~Demo_StructureWithOptionalFields_OptionalFields_OptionalInt32);
}

void StructureWithOptionalFields::setOptionalInt32(OpcUa_Int32 OptionalInt32)
{
    if (d_ptr->refCount() > 1)
    {
        StructureWithOptionalFieldsPrivate *pNew = new StructureWithOptionalFieldsPrivate((const Demo_StructureWithOptionalFields)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    d_ptr->EncodingMask |= Demo_StructureWithOptionalFields_OptionalFields_OptionalInt32;

    d_ptr->OptionalInt32 = OptionalInt32;
}

void StructureWithOptionalFields::setMandatoryStringArray(const UaStringArray& MandatoryStringArray)
{
    if (d_ptr->refCount() > 1)
    {
        StructureWithOptionalFieldsPrivate *pNew = new StructureWithOptionalFieldsPrivate((const Demo_StructureWithOptionalFields)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }
    if (d_ptr->NoOfMandatoryStringArray > 0)
    {
        OpcUa_Int32 i = 0;
        for (i = 0; i < d_ptr->NoOfMandatoryStringArray; i++)
        {
            OpcUa_String_Clear(&d_ptr->MandatoryStringArray[i]);
        }
        OpcUa_Free(d_ptr->MandatoryStringArray);
    }

    d_ptr->NoOfMandatoryStringArray = MandatoryStringArray.length();
    if (d_ptr->NoOfMandatoryStringArray > 0)
    {
        d_ptr->MandatoryStringArray = (OpcUa_String*)OpcUa_Alloc(d_ptr->NoOfMandatoryStringArray * sizeof(OpcUa_String));
        if (d_ptr->MandatoryStringArray != OpcUa_Null)
        {
            OpcUa_Int32 i = 0;
            for (i = 0; i < d_ptr->NoOfMandatoryStringArray; i++)
            {
                OpcUa_String_Initialize(&d_ptr->MandatoryStringArray[i]);
                OpcUa_String_CopyTo(&MandatoryStringArray[i], &d_ptr->MandatoryStringArray[i]);
            }
        }
        else
        {
            d_ptr->NoOfMandatoryStringArray = 0;
        }
    }
}

void StructureWithOptionalFields::unsetOptionalStringArray()
{
    if (d_ptr->refCount() > 1)
    {
        StructureWithOptionalFieldsPrivate *pNew = new StructureWithOptionalFieldsPrivate((const Demo_StructureWithOptionalFields)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }
    if (d_ptr->NoOfOptionalStringArray > 0)
    {
        OpcUa_Int32 i = 0;
        for (i = 0; i < d_ptr->NoOfOptionalStringArray; i++)
        {
            OpcUa_String_Clear(&d_ptr->OptionalStringArray[i]);
        }
        d_ptr->NoOfOptionalStringArray = 0;
        OpcUa_Free(d_ptr->OptionalStringArray);
        d_ptr->OptionalStringArray = OpcUa_Null;
    }
    d_ptr->EncodingMask &= (~Demo_StructureWithOptionalFields_OptionalFields_OptionalStringArray);
}

void StructureWithOptionalFields::setOptionalStringArray(const UaStringArray& OptionalStringArray)
{
    if (d_ptr->refCount() > 1)
    {
        StructureWithOptionalFieldsPrivate *pNew = new StructureWithOptionalFieldsPrivate((const Demo_StructureWithOptionalFields)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }
    d_ptr->EncodingMask |= Demo_StructureWithOptionalFields_OptionalFields_OptionalStringArray;

    if (d_ptr->NoOfOptionalStringArray > 0)
    {
        OpcUa_Int32 i = 0;
        for (i = 0; i < d_ptr->NoOfOptionalStringArray; i++)
        {
            OpcUa_String_Clear(&d_ptr->OptionalStringArray[i]);
        }
        OpcUa_Free(d_ptr->OptionalStringArray);
    }

    d_ptr->NoOfOptionalStringArray = OptionalStringArray.length();
    if (d_ptr->NoOfOptionalStringArray > 0)
    {
        d_ptr->OptionalStringArray = (OpcUa_String*)OpcUa_Alloc(d_ptr->NoOfOptionalStringArray * sizeof(OpcUa_String));
        if (d_ptr->OptionalStringArray != OpcUa_Null)
        {
            OpcUa_Int32 i = 0;
            for (i = 0; i < d_ptr->NoOfOptionalStringArray; i++)
            {
                OpcUa_String_Initialize(&d_ptr->OptionalStringArray[i]);
                OpcUa_String_CopyTo(&OptionalStringArray[i], &d_ptr->OptionalStringArray[i]);
            }
        }
        else
        {
            d_ptr->NoOfOptionalStringArray = 0;
        }
    }
}


/******************************************************************************
** Class StructureWithOptionalFieldss
******************************************************************************/
StructureWithOptionalFieldss StructureWithOptionalFieldss::empty;

StructureWithOptionalFieldss::StructureWithOptionalFieldss()
: m_noOfElements(0),
  m_data((Demo_StructureWithOptionalFields*)OpcUa_Null)
{
}

StructureWithOptionalFieldss::StructureWithOptionalFieldss(const StructureWithOptionalFieldss &other)
: m_noOfElements(0),
  m_data((Demo_StructureWithOptionalFields*)OpcUa_Null)
{
    operator=(other);
}

StructureWithOptionalFieldss::StructureWithOptionalFieldss(OpcUa_Int32 length, Demo_StructureWithOptionalFields* data)
: m_noOfElements(0),
  m_data((Demo_StructureWithOptionalFields*)OpcUa_Null)
{
    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_StructureWithOptionalFields*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_StructureWithOptionalFields));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_StructureWithOptionalFields_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
}

StructureWithOptionalFieldss::~StructureWithOptionalFieldss()
{
    clear();
}

StructureWithOptionalFieldss& StructureWithOptionalFieldss::operator=(const StructureWithOptionalFieldss &other)
{
    if (this == &other) {return *this;}

    clear();
    if (other.m_noOfElements > 0)
    {
        m_noOfElements = other.m_noOfElements;
        m_data = (Demo_StructureWithOptionalFields*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_StructureWithOptionalFields));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_StructureWithOptionalFields_CopyTo(&(other.m_data[i]), &(m_data[i]));
        }
    }
    return *this;
}

/** Returns the Demo_StructureWithOptionalFields at index position index as a modifiable reference.
 *
 *  index must be valid index position in the array.
 */
Demo_StructureWithOptionalFields& StructureWithOptionalFieldss::operator[](OpcUa_UInt32 index)
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Returns the Demo_StructureWithOptionalFields at index position index.
 *
 *  index must be valid index position in the array.
 */
const Demo_StructureWithOptionalFields& StructureWithOptionalFieldss::operator[](OpcUa_UInt32 index) const
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because StructureWithOptionalFieldss is responsible for it now.
 */
void StructureWithOptionalFieldss::attach(OpcUa_UInt32 length, Demo_StructureWithOptionalFields* data)
{
    clear();
    m_data = data;
    m_noOfElements = length;
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because StructureWithOptionalFieldss is responsible for it now.
 */
void StructureWithOptionalFieldss::attach(OpcUa_Int32 length, Demo_StructureWithOptionalFields* data)
{
    clear();
    if (length >= 0)
    {
        m_data = data;
        m_noOfElements = length;
    }
}

bool StructureWithOptionalFieldss::operator==(const StructureWithOptionalFieldss &other) const
{
    if (m_noOfElements != other.m_noOfElements) {return false;}

    if (m_noOfElements > 0)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            if (Demo_StructureWithOptionalFields_Compare(&(m_data[i]), &(other.m_data[i])) != OPCUA_EQUAL) {return false;};
        }
    }
    return true;
}

bool StructureWithOptionalFieldss::operator!=(const StructureWithOptionalFieldss &other) const
{
    return !(operator==(other));
}

/** Detaches the raw data from this class.
 *
 *  The caller takes ownership of the raw data and is responsible for deleting it
 *  by calling OpcUa_Free.
 *  @return The raw data.
 */
Demo_StructureWithOptionalFields* StructureWithOptionalFieldss::detach()
{
    Demo_StructureWithOptionalFields* pRet = m_data;
    m_noOfElements = 0;
    m_data = (Demo_StructureWithOptionalFields*)OpcUa_Null;
    return pRet;
}

void StructureWithOptionalFieldss::create(OpcUa_UInt32 length)
{
    clear();
    if (length > 0)
    {
        m_data = (Demo_StructureWithOptionalFields*)OpcUa_Alloc(length * sizeof(Demo_StructureWithOptionalFields));
        for (OpcUa_UInt32 i = 0; i < length; i++)
        {
            Demo_StructureWithOptionalFields_Initialize(&(m_data[i]));
        }
        m_noOfElements = length;
    }
}

void StructureWithOptionalFieldss::resize(OpcUa_UInt32 length)
{
    if (length < m_noOfElements)
    {
        if (length != 0)
        {
            for (OpcUa_UInt32 i = length; i < m_noOfElements; i++)
            {
                Demo_StructureWithOptionalFields_Clear(&(m_data[i]));
            }

            Demo_StructureWithOptionalFields* newData = (Demo_StructureWithOptionalFields*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_StructureWithOptionalFields));
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
        Demo_StructureWithOptionalFields* newData = (Demo_StructureWithOptionalFields*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_StructureWithOptionalFields));
        if (newData)
        {
            for (OpcUa_UInt32 i = m_noOfElements; i < length; i++)
            {
                Demo_StructureWithOptionalFields_Initialize(&(newData[i]));
            }
            m_data = newData;
            m_noOfElements = length;
        }
    }
}

void StructureWithOptionalFieldss::clear()
{
    if (m_data)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_StructureWithOptionalFields_Clear(&(m_data[i]));
        }
        OpcUa_Free(m_data);
        m_data = (Demo_StructureWithOptionalFields*)OpcUa_Null;
    }
    m_noOfElements = 0;
}

void StructureWithOptionalFieldss::toVariant(UaVariant &variant) const
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant);
}

void StructureWithOptionalFieldss::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);

    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_StructureWithOptionalFields *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_StructureWithOptionalFields_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        Demo_StructureWithOptionalFields_CopyTo(&m_data[i], pValue);
    }
}

void StructureWithOptionalFieldss::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant, bDetach);
}

void StructureWithOptionalFieldss::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_StructureWithOptionalFields *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_StructureWithOptionalFields_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        if (bDetach != OpcUa_False)
        {
            *pValue = m_data[i];
        }
        else
        {
            Demo_StructureWithOptionalFields_CopyTo(&m_data[i], pValue);
        }
    }
    if (bDetach != OpcUa_False)
    {
        OpcUa_Free(m_data);
        m_data = (Demo_StructureWithOptionalFields*)OpcUa_Null;
        m_noOfElements = 0;
    }
}

void StructureWithOptionalFieldss::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void StructureWithOptionalFieldss::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    toVariant(dataValue.Value);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

void StructureWithOptionalFieldss::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void StructureWithOptionalFieldss::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    toVariant(dataValue.Value, bDetach);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

OpcUa_StatusCode StructureWithOptionalFieldss::setStructureWithOptionalFieldss(const UaVariant &variant)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setStructureWithOptionalFieldss(*pVariant);
}

OpcUa_StatusCode StructureWithOptionalFieldss::setStructureWithOptionalFieldss(const OpcUa_Variant &variant)
{
    return setStructureWithOptionalFieldss((OpcUa_Variant&)variant, OpcUa_False);
}

OpcUa_StatusCode StructureWithOptionalFieldss::setStructureWithOptionalFieldss(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setStructureWithOptionalFieldss(*pVariant, bDetach);
}

OpcUa_StatusCode StructureWithOptionalFieldss::setStructureWithOptionalFieldss(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
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
            m_data = (Demo_StructureWithOptionalFields*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_StructureWithOptionalFields));

            OpcUa_UInt32 i = 0;
            for (i = 0; i < m_noOfElements; i++)
            {
                Demo_StructureWithOptionalFields_Initialize(&m_data[i]);

                Demo_StructureWithOptionalFields *pValue = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(StructureWithOptionalFields, &variant.Value.Array.Value.ExtensionObjectArray[i]);
                if (pValue)
                {
                    if (bDetach != OpcUa_False)
                    {
                        m_data[i] = *pValue;
                        Demo_StructureWithOptionalFields_Initialize(pValue);
                    }
                    else
                    {
                        Demo_StructureWithOptionalFields_CopyTo(pValue, &m_data[i]);
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
                    Demo_StructureWithOptionalFields_Clear(&m_data[j]);
                }
                OpcUa_Free(m_data);
                m_data = (Demo_StructureWithOptionalFields*)OpcUa_Null;
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

OpcUa_StatusCode StructureWithOptionalFieldss::setStructureWithOptionalFieldss(OpcUa_Int32 length, Demo_StructureWithOptionalFields* data)
{
    clear();

    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_StructureWithOptionalFields*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_StructureWithOptionalFields));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_StructureWithOptionalFields_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
    return OpcUa_Good;
}

} // namespace Demo


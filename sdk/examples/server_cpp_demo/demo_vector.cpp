/******************************************************************************
** demo_vector.cpp
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

#include "demo_vector.h"

#include <opcua_datetime.h>

#include "uavariant.h"
#include "uadatavalue.h"
#include "uaextensionobject.h"
#include "uaatomic.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

/******************************************************************************
** Class VectorPrivate
******************************************************************************/
class VectorPrivate : private Demo_Vector
{
    VectorPrivate(const VectorPrivate &other);
public:
    VectorPrivate();
    explicit VectorPrivate(bool bStatic);
    explicit VectorPrivate(const Demo_Vector &other);
    ~VectorPrivate();

    int addRef();
    int release();
    inline int refCount() { return m_iRefCnt; }

private:
    int m_iRefCnt;
    friend class Vector;
};

/* construction */
VectorPrivate::VectorPrivate()
 : m_iRefCnt(0)
{
    Demo_Vector_Initialize(this);
}

/*  Static initializer.
 *  This is used internaly to initialize the static shared_emtpy variable.
 *  RefCnt is set to 1 so it will never be deleted.
 */
VectorPrivate::VectorPrivate(bool)
 : m_iRefCnt(1)
{
    Demo_Vector_Initialize(this);
}


/*  construction
 */
VectorPrivate::VectorPrivate(const Demo_Vector &other) // [in] the other Demo_Vector.
 : m_iRefCnt(0)
{
    Demo_Vector_CopyTo(&other, this);
}

/* destruction */
VectorPrivate::~VectorPrivate()
{
    Demo_Vector_Clear(this);
}

/*  Add a reference to a VectorPrivate
 *  @return 0 if the object was deleted - otherwise a value bigger than 0.
 *  This method is not thread save regarding the return value. The returned counter is only reliable when it is 0.
 *  Other values are not reliable and should not be used for any logic.
 */
int VectorPrivate::addRef()
{
    ua_atomic_increment(&m_iRefCnt);
    return m_iRefCnt;
}

/*  Release a VectorPrivate.
 *  @return the reference count.
 */
int VectorPrivate::release()
{
    int currentCount = ua_atomic_decrement(&m_iRefCnt);
    if (currentCount == 0)
    {
        delete this;
    }
    return currentCount;
}

VectorPrivate* shared_empty_Vector()
{
    static VectorPrivate shared_static(true);
    return &shared_static;
}


/******************************************************************************
** Class Vector
******************************************************************************/

/** Constructs an instance of the class Vector with empty default values
*/
Vector::Vector()
{
    d_ptr = shared_empty_Vector();
    d_ptr->addRef();
}

/** Constructs an instance of the class Vector with values from another Vector object
 *  @param other the other UaVector used to initialize the object
 */
Vector::Vector(const Vector &other)
{
    d_ptr = other.d_ptr;
    d_ptr->addRef();
}

/** Constructs an instance of the class Vector with values from another Demo_Vector structure
 *  @param other the other Demo_Vector used to initialize the object
 */
Vector::Vector(const Demo_Vector &other)
{
    d_ptr = new VectorPrivate(other);
    d_ptr->addRef();
}

/** Constructs an instance of the class Vector with values provided in the constructor
 */
Vector::Vector(
        OpcUa_Double X,
        OpcUa_Double Y,
        OpcUa_Double Z
        )
{
    d_ptr = new VectorPrivate();
    d_ptr->addRef();
    setX(X);
    setY(Y);
    setZ(Z);
}

/** Constructs an instance of the class Vector initialized with value from a UaExtensionObject
 *  @param extensionObject the extension object containing the Vector
 */
Vector::Vector(const UaExtensionObject &extensionObject)
{
    d_ptr = new VectorPrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_Vector *pVector = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(Vector, pExtensionObject);

    if (pVector)
    {
        Demo_Vector_CopyTo(pVector, d_ptr);
    }
}

/** Constructs an instance of the class Vector initialized with value from an OpcUa_ExtensionObject structure
 *  @param extensionObject the extension object containing the Vector
 */
Vector::Vector(const OpcUa_ExtensionObject &extensionObject)
{
    d_ptr = new VectorPrivate();
    d_ptr->addRef();

    Demo_Vector *pVector = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(Vector, &extensionObject);

    if (pVector)
    {
        Demo_Vector_CopyTo(pVector, d_ptr);
    }
}

Vector::Vector(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new VectorPrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_Vector *pVector = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(Vector, pExtensionObject);
    if (pVector)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_Vector_CopyTo(pVector, d_ptr);
        }
        else
        {
            (*(Demo_Vector*)d_ptr) = *pVector;
            Demo_Vector_Initialize(pVector);
            extensionObject.clear();
        }
    }
}

Vector::Vector(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new VectorPrivate();
    d_ptr->addRef();

    Demo_Vector *pVector = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(Vector, &extensionObject);

    if (pVector)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_Vector_CopyTo(pVector, d_ptr);
        }
        else
        {
            (*(Demo_Vector*)d_ptr) = *pVector;
            Demo_Vector_Initialize(pVector);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
}

/** Destroys the Vector object
*/
Vector::~Vector()
{
    DEMO_D(Vector);
    d->release();
}

/** Clear the data of the Vector.
*/
void Vector::clear()
{
    d_ptr->release();
    d_ptr = shared_empty_Vector();
    d_ptr->addRef();
}

/** Compare two Vector if they are similar.
 *  @param other the Vector to compare.
 *  @return true if similar, false if not.
 */
bool Vector::operator==(const Vector &other) const
{
    return (Demo_Vector_Compare(d_ptr, other.d_ptr) == 0);
}

/** Returns true if the other Vector is not equal to this.
 *  @see operator==
 *  @param other the Vector to compare.
 *  @return true if other is not equal to this.
 */
bool Vector::operator!=(const Vector &other) const
{
    return !(operator==(other));
}

/** Assignment operator.
 *  @param other the Vector to assign.
 *  @return Assignment operator.
 */
Vector& Vector::operator=(const Vector &other)
{
    other.d_ptr->addRef();
    d_ptr->release();
    d_ptr = other.d_ptr;
    return *this;
}

/** Copy Vector data to a newly allocated Demo_Vector.
 *  @return data to a newly allocated Demo_Vector.
 */
Demo_Vector* Vector::copy() const
{
    return clone(*d_ptr);
}

/** Copy Vector data to an existing Demo_Vector structure.
 *  @param pDst the destination of this copy operation.
 */
void Vector::copyTo(Demo_Vector *pDst) const
{
    if (pDst == OpcUa_Null) return;
    cloneTo(*d_ptr, *pDst);
}

/** Copy Demo_Vector data to a newly allocated Demo_Vector.
 *  @param source Source to clone.
 *  @return new copy.
 */
Demo_Vector* Vector::clone(const Demo_Vector& source)
{
    Demo_Vector* pVector = OpcUa_Null;
    Demo_Vector_Copy(&source, &pVector);
    return pVector;
}

/** Copy Demo_Vector data to an existing Demo_Vector structure.
 *  @param source the source of the clone operation.
 *  @param copy the destination of the clone operation.
 */
void Vector::cloneTo(const Demo_Vector& source, Demo_Vector& copy)
{
    Demo_Vector_CopyTo(&source, &copy);
}

/** Attaches the data of the parameter pValue.
 * Don't clear the data of pValue afterwards manually, because Vector is responsible for it now.
 */
void Vector::attach(const Demo_Vector *pValue) //!< [in] Parameter to attach to.
{
    if (pValue == OpcUa_Null) return;
    if (d_ptr->refCount() == 1)
    {
        Demo_Vector_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new VectorPrivate();
        d_ptr->addRef();
    }
    // copy data value structure (no deep copy)
    OpcUa_MemCpy(d_ptr, sizeof(Demo_Vector), (void*)pValue, sizeof(Demo_Vector));
}

/** Detaches the internal Vector structure from this class.
 *  This way you take over the control of releasing the Vector data.
 *  If more than one references exist, the data is copied to pDst instead of being detached.
 *  @return If the functions succeeds pDst is returned, otherwise 0 is returned.
 */
Demo_Vector* Vector::detach(Demo_Vector* pDst) //!< [out] Pointer to Demo_Vector structure that receives the Vector data.
{
    if (pDst == OpcUa_Null) return OpcUa_Null;
    if (d_ptr->refCount() == 1)
    {
        OpcUa_MemCpy(pDst, sizeof(Demo_Vector), d_ptr, sizeof(Demo_Vector));
        // clear internal data without releasing deeper memory
        Demo_Vector_Initialize(d_ptr);
    }
    else
    {
        Demo_Vector_CopyTo(d_ptr, pDst);
    }
    d_ptr->release();
    d_ptr = shared_empty_Vector();
    d_ptr->addRef();
    return pDst;
}

void Vector::toVariant(UaVariant &variant) const
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void Vector::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject);
}

void Vector::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject, bDetach);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void Vector::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject, bDetach);
}

void Vector::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void Vector::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
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

void Vector::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void Vector::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
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

void Vector::toExtensionObject(UaExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject);
}

void Vector::toExtensionObject(OpcUa_ExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_Vector *pVector = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_Vector_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pVector)))
    {
        Demo_Vector_CopyTo(d_ptr, pVector);
    }
}

void Vector::toExtensionObject(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject, bDetach);
}

void Vector::toExtensionObject(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_Vector *pVector = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_Vector_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pVector)))
    {
        Demo_Vector_Initialize(pVector);
        if (bDetach != OpcUa_False && d_ptr->refCount() == 1)
        {
            *pVector = *d_ptr;
            // clear internal data without releasing deeper memory
            Demo_Vector_Initialize(d_ptr);
        }
        else
        {
            Demo_Vector_CopyTo(d_ptr, pVector);
        }
        if (bDetach != OpcUa_False)
        {
            d_ptr->release();
            d_ptr = shared_empty_Vector();
            d_ptr->addRef();
        }
    }
}

OpcUa_StatusCode Vector::setVector(const UaExtensionObject &extensionObject)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setVector(*pExtensionObject);
}

OpcUa_StatusCode Vector::setVector(const OpcUa_ExtensionObject &extensionObject)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_Vector *pVector = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(Vector, &extensionObject);

    if (pVector)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_Vector_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new VectorPrivate();
            d_ptr->addRef();
        }

        Demo_Vector_CopyTo(pVector, d_ptr);
    }
    return ret;
}

OpcUa_StatusCode Vector::setVector(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setVector(*pExtensionObject, bDetach);
}

OpcUa_StatusCode Vector::setVector(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_Vector *pVector = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(Vector, &extensionObject);

    if (pVector != OpcUa_Null)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_Vector_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new VectorPrivate();
            d_ptr->addRef();
        }

        if (bDetach == OpcUa_False)
        {
            Demo_Vector_CopyTo(pVector, d_ptr);
        }
        else
        {
            (*(Demo_Vector*)d_ptr) = *pVector;
            Demo_Vector_Initialize(pVector);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
    return ret;
}

void Vector::setVector(
        OpcUa_Double X,
        OpcUa_Double Y,
        OpcUa_Double Z
        )
{
    if (d_ptr->refCount() == 1)
    {
        Demo_Vector_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new VectorPrivate();
        d_ptr->addRef();
    }
    setX(X);
    setY(Y);
    setZ(Z);
}

OpcUa_Double Vector::getX() const
{
    return d_ptr->X;
}

OpcUa_Double Vector::getY() const
{
    return d_ptr->Y;
}

OpcUa_Double Vector::getZ() const
{
    return d_ptr->Z;
}

void Vector::setX(OpcUa_Double X)
{
    if (d_ptr->refCount() > 1)
    {
        VectorPrivate *pNew = new VectorPrivate((const Demo_Vector)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    d_ptr->X = X;
}

void Vector::setY(OpcUa_Double Y)
{
    if (d_ptr->refCount() > 1)
    {
        VectorPrivate *pNew = new VectorPrivate((const Demo_Vector)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    d_ptr->Y = Y;
}

void Vector::setZ(OpcUa_Double Z)
{
    if (d_ptr->refCount() > 1)
    {
        VectorPrivate *pNew = new VectorPrivate((const Demo_Vector)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    d_ptr->Z = Z;
}


/******************************************************************************
** Class Vectors
******************************************************************************/
Vectors Vectors::empty;

Vectors::Vectors()
: m_noOfElements(0),
  m_data((Demo_Vector*)OpcUa_Null)
{
}

Vectors::Vectors(const Vectors &other)
: m_noOfElements(0),
  m_data((Demo_Vector*)OpcUa_Null)
{
    operator=(other);
}

Vectors::Vectors(OpcUa_Int32 length, Demo_Vector* data)
: m_noOfElements(0),
  m_data((Demo_Vector*)OpcUa_Null)
{
    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_Vector*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_Vector));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_Vector_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
}

Vectors::~Vectors()
{
    clear();
}

Vectors& Vectors::operator=(const Vectors &other)
{
    if (this == &other) {return *this;}

    clear();
    if (other.m_noOfElements > 0)
    {
        m_noOfElements = other.m_noOfElements;
        m_data = (Demo_Vector*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_Vector));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_Vector_CopyTo(&(other.m_data[i]), &(m_data[i]));
        }
    }
    return *this;
}

/** Returns the Demo_Vector at index position index as a modifiable reference.
 *
 *  index must be valid index position in the array.
 */
Demo_Vector& Vectors::operator[](OpcUa_UInt32 index)
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Returns the Demo_Vector at index position index.
 *
 *  index must be valid index position in the array.
 */
const Demo_Vector& Vectors::operator[](OpcUa_UInt32 index) const
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because Vectors is responsible for it now.
 */
void Vectors::attach(OpcUa_UInt32 length, Demo_Vector* data)
{
    clear();
    m_data = data;
    m_noOfElements = length;
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because Vectors is responsible for it now.
 */
void Vectors::attach(OpcUa_Int32 length, Demo_Vector* data)
{
    clear();
    if (length >= 0)
    {
        m_data = data;
        m_noOfElements = length;
    }
}

bool Vectors::operator==(const Vectors &other) const
{
    if (m_noOfElements != other.m_noOfElements) {return false;}

    if (m_noOfElements > 0)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            if (Demo_Vector_Compare(&(m_data[i]), &(other.m_data[i])) != OPCUA_EQUAL) {return false;};
        }
    }
    return true;
}

bool Vectors::operator!=(const Vectors &other) const
{
    return !(operator==(other));
}

/** Detaches the raw data from this class.
 *
 *  The caller takes ownership of the raw data and is responsible for deleting it
 *  by calling OpcUa_Free.
 *  @return The raw data.
 */
Demo_Vector* Vectors::detach()
{
    Demo_Vector* pRet = m_data;
    m_noOfElements = 0;
    m_data = (Demo_Vector*)OpcUa_Null;
    return pRet;
}

void Vectors::create(OpcUa_UInt32 length)
{
    clear();
    if (length > 0)
    {
        m_data = (Demo_Vector*)OpcUa_Alloc(length * sizeof(Demo_Vector));
        for (OpcUa_UInt32 i = 0; i < length; i++)
        {
            Demo_Vector_Initialize(&(m_data[i]));
        }
        m_noOfElements = length;
    }
}

void Vectors::resize(OpcUa_UInt32 length)
{
    if (length < m_noOfElements)
    {
        if (length != 0)
        {
            for (OpcUa_UInt32 i = length; i < m_noOfElements; i++)
            {
                Demo_Vector_Clear(&(m_data[i]));
            }

            Demo_Vector* newData = (Demo_Vector*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_Vector));
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
        Demo_Vector* newData = (Demo_Vector*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_Vector));
        if (newData)
        {
            for (OpcUa_UInt32 i = m_noOfElements; i < length; i++)
            {
                Demo_Vector_Initialize(&(newData[i]));
            }
            m_data = newData;
            m_noOfElements = length;
        }
    }
}

void Vectors::clear()
{
    if (m_data)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_Vector_Clear(&(m_data[i]));
        }
        OpcUa_Free(m_data);
        m_data = (Demo_Vector*)OpcUa_Null;
    }
    m_noOfElements = 0;
}

void Vectors::toVariant(UaVariant &variant) const
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant);
}

void Vectors::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);

    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_Vector *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_Vector_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        Demo_Vector_CopyTo(&m_data[i], pValue);
    }
}

void Vectors::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant, bDetach);
}

void Vectors::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_Vector *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_Vector_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        if (bDetach != OpcUa_False)
        {
            *pValue = m_data[i];
        }
        else
        {
            Demo_Vector_CopyTo(&m_data[i], pValue);
        }
    }
    if (bDetach != OpcUa_False)
    {
        OpcUa_Free(m_data);
        m_data = (Demo_Vector*)OpcUa_Null;
        m_noOfElements = 0;
    }
}

void Vectors::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void Vectors::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    toVariant(dataValue.Value);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

void Vectors::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void Vectors::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    toVariant(dataValue.Value, bDetach);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

OpcUa_StatusCode Vectors::setVectors(const UaVariant &variant)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setVectors(*pVariant);
}

OpcUa_StatusCode Vectors::setVectors(const OpcUa_Variant &variant)
{
    return setVectors((OpcUa_Variant&)variant, OpcUa_False);
}

OpcUa_StatusCode Vectors::setVectors(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setVectors(*pVariant, bDetach);
}

OpcUa_StatusCode Vectors::setVectors(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
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
            m_data = (Demo_Vector*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_Vector));

            OpcUa_UInt32 i = 0;
            for (i = 0; i < m_noOfElements; i++)
            {
                Demo_Vector_Initialize(&m_data[i]);

                Demo_Vector *pValue = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(Vector, &variant.Value.Array.Value.ExtensionObjectArray[i]);
                if (pValue)
                {
                    if (bDetach != OpcUa_False)
                    {
                        m_data[i] = *pValue;
                        Demo_Vector_Initialize(pValue);
                    }
                    else
                    {
                        Demo_Vector_CopyTo(pValue, &m_data[i]);
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
                    Demo_Vector_Clear(&m_data[j]);
                }
                OpcUa_Free(m_data);
                m_data = (Demo_Vector*)OpcUa_Null;
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

OpcUa_StatusCode Vectors::setVectors(OpcUa_Int32 length, Demo_Vector* data)
{
    clear();

    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_Vector*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_Vector));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_Vector_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
    return OpcUa_Good;
}

} // namespace Demo


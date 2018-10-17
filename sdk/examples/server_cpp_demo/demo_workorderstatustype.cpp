/******************************************************************************
** demo_workorderstatustype.cpp
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

#include "demo_workorderstatustype.h"

#include <opcua_datetime.h>

#include "uavariant.h"
#include "uadatavalue.h"
#include "uaextensionobject.h"
#include "uaatomic.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

/******************************************************************************
** Class WorkOrderStatusTypePrivate
******************************************************************************/
class WorkOrderStatusTypePrivate : private Demo_WorkOrderStatusType
{
    WorkOrderStatusTypePrivate(const WorkOrderStatusTypePrivate &other);
public:
    WorkOrderStatusTypePrivate();
    explicit WorkOrderStatusTypePrivate(bool bStatic);
    explicit WorkOrderStatusTypePrivate(const Demo_WorkOrderStatusType &other);
    ~WorkOrderStatusTypePrivate();

    int addRef();
    int release();
    inline int refCount() { return m_iRefCnt; }

private:
    int m_iRefCnt;
    friend class WorkOrderStatusType;
};

/* construction */
WorkOrderStatusTypePrivate::WorkOrderStatusTypePrivate()
 : m_iRefCnt(0)
{
    Demo_WorkOrderStatusType_Initialize(this);
}

/*  Static initializer.
 *  This is used internaly to initialize the static shared_emtpy variable.
 *  RefCnt is set to 1 so it will never be deleted.
 */
WorkOrderStatusTypePrivate::WorkOrderStatusTypePrivate(bool)
 : m_iRefCnt(1)
{
    Demo_WorkOrderStatusType_Initialize(this);
}


/*  construction
 */
WorkOrderStatusTypePrivate::WorkOrderStatusTypePrivate(const Demo_WorkOrderStatusType &other) // [in] the other Demo_WorkOrderStatusType.
 : m_iRefCnt(0)
{
    Demo_WorkOrderStatusType_CopyTo(&other, this);
}

/* destruction */
WorkOrderStatusTypePrivate::~WorkOrderStatusTypePrivate()
{
    Demo_WorkOrderStatusType_Clear(this);
}

/*  Add a reference to a WorkOrderStatusTypePrivate
 *  @return 0 if the object was deleted - otherwise a value bigger than 0.
 *  This method is not thread save regarding the return value. The returned counter is only reliable when it is 0.
 *  Other values are not reliable and should not be used for any logic.
 */
int WorkOrderStatusTypePrivate::addRef()
{
    ua_atomic_increment(&m_iRefCnt);
    return m_iRefCnt;
}

/*  Release a WorkOrderStatusTypePrivate.
 *  @return the reference count.
 */
int WorkOrderStatusTypePrivate::release()
{
    int currentCount = ua_atomic_decrement(&m_iRefCnt);
    if (currentCount == 0)
    {
        delete this;
    }
    return currentCount;
}

WorkOrderStatusTypePrivate* shared_empty_WorkOrderStatusType()
{
    static WorkOrderStatusTypePrivate shared_static(true);
    return &shared_static;
}


/******************************************************************************
** Class WorkOrderStatusType
******************************************************************************/

/** Constructs an instance of the class WorkOrderStatusType with empty default values
*/
WorkOrderStatusType::WorkOrderStatusType()
{
    d_ptr = shared_empty_WorkOrderStatusType();
    d_ptr->addRef();
}

/** Constructs an instance of the class WorkOrderStatusType with values from another WorkOrderStatusType object
 *  @param other the other UaWorkOrderStatusType used to initialize the object
 */
WorkOrderStatusType::WorkOrderStatusType(const WorkOrderStatusType &other)
{
    d_ptr = other.d_ptr;
    d_ptr->addRef();
}

/** Constructs an instance of the class WorkOrderStatusType with values from another Demo_WorkOrderStatusType structure
 *  @param other the other Demo_WorkOrderStatusType used to initialize the object
 */
WorkOrderStatusType::WorkOrderStatusType(const Demo_WorkOrderStatusType &other)
{
    d_ptr = new WorkOrderStatusTypePrivate(other);
    d_ptr->addRef();
}

/** Constructs an instance of the class WorkOrderStatusType with values provided in the constructor
 */
WorkOrderStatusType::WorkOrderStatusType(
        const UaString& Actor,
        const UaDateTime& Timestamp,
        const UaLocalizedText& Comment
        )
{
    d_ptr = new WorkOrderStatusTypePrivate();
    d_ptr->addRef();
    setActor(Actor);
    setTimestamp(Timestamp);
    setComment(Comment);
}

/** Constructs an instance of the class WorkOrderStatusType initialized with value from a UaExtensionObject
 *  @param extensionObject the extension object containing the WorkOrderStatusType
 */
WorkOrderStatusType::WorkOrderStatusType(const UaExtensionObject &extensionObject)
{
    d_ptr = new WorkOrderStatusTypePrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_WorkOrderStatusType *pWorkOrderStatusType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderStatusType, pExtensionObject);

    if (pWorkOrderStatusType)
    {
        Demo_WorkOrderStatusType_CopyTo(pWorkOrderStatusType, d_ptr);
    }
}

/** Constructs an instance of the class WorkOrderStatusType initialized with value from an OpcUa_ExtensionObject structure
 *  @param extensionObject the extension object containing the WorkOrderStatusType
 */
WorkOrderStatusType::WorkOrderStatusType(const OpcUa_ExtensionObject &extensionObject)
{
    d_ptr = new WorkOrderStatusTypePrivate();
    d_ptr->addRef();

    Demo_WorkOrderStatusType *pWorkOrderStatusType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderStatusType, &extensionObject);

    if (pWorkOrderStatusType)
    {
        Demo_WorkOrderStatusType_CopyTo(pWorkOrderStatusType, d_ptr);
    }
}

WorkOrderStatusType::WorkOrderStatusType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new WorkOrderStatusTypePrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_WorkOrderStatusType *pWorkOrderStatusType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderStatusType, pExtensionObject);
    if (pWorkOrderStatusType)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_WorkOrderStatusType_CopyTo(pWorkOrderStatusType, d_ptr);
        }
        else
        {
            (*(Demo_WorkOrderStatusType*)d_ptr) = *pWorkOrderStatusType;
            Demo_WorkOrderStatusType_Initialize(pWorkOrderStatusType);
            extensionObject.clear();
        }
    }
}

WorkOrderStatusType::WorkOrderStatusType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new WorkOrderStatusTypePrivate();
    d_ptr->addRef();

    Demo_WorkOrderStatusType *pWorkOrderStatusType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderStatusType, &extensionObject);

    if (pWorkOrderStatusType)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_WorkOrderStatusType_CopyTo(pWorkOrderStatusType, d_ptr);
        }
        else
        {
            (*(Demo_WorkOrderStatusType*)d_ptr) = *pWorkOrderStatusType;
            Demo_WorkOrderStatusType_Initialize(pWorkOrderStatusType);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
}

/** Destroys the WorkOrderStatusType object
*/
WorkOrderStatusType::~WorkOrderStatusType()
{
    DEMO_D(WorkOrderStatusType);
    d->release();
}

/** Clear the data of the WorkOrderStatusType.
*/
void WorkOrderStatusType::clear()
{
    d_ptr->release();
    d_ptr = shared_empty_WorkOrderStatusType();
    d_ptr->addRef();
}

/** Compare two WorkOrderStatusType if they are similar.
 *  @param other the WorkOrderStatusType to compare.
 *  @return true if similar, false if not.
 */
bool WorkOrderStatusType::operator==(const WorkOrderStatusType &other) const
{
    return (Demo_WorkOrderStatusType_Compare(d_ptr, other.d_ptr) == 0);
}

/** Returns true if the other WorkOrderStatusType is not equal to this.
 *  @see operator==
 *  @param other the WorkOrderStatusType to compare.
 *  @return true if other is not equal to this.
 */
bool WorkOrderStatusType::operator!=(const WorkOrderStatusType &other) const
{
    return !(operator==(other));
}

/** Assignment operator.
 *  @param other the WorkOrderStatusType to assign.
 *  @return Assignment operator.
 */
WorkOrderStatusType& WorkOrderStatusType::operator=(const WorkOrderStatusType &other)
{
    other.d_ptr->addRef();
    d_ptr->release();
    d_ptr = other.d_ptr;
    return *this;
}

/** Copy WorkOrderStatusType data to a newly allocated Demo_WorkOrderStatusType.
 *  @return data to a newly allocated Demo_WorkOrderStatusType.
 */
Demo_WorkOrderStatusType* WorkOrderStatusType::copy() const
{
    return clone(*d_ptr);
}

/** Copy WorkOrderStatusType data to an existing Demo_WorkOrderStatusType structure.
 *  @param pDst the destination of this copy operation.
 */
void WorkOrderStatusType::copyTo(Demo_WorkOrderStatusType *pDst) const
{
    if (pDst == OpcUa_Null) return;
    cloneTo(*d_ptr, *pDst);
}

/** Copy Demo_WorkOrderStatusType data to a newly allocated Demo_WorkOrderStatusType.
 *  @param source Source to clone.
 *  @return new copy.
 */
Demo_WorkOrderStatusType* WorkOrderStatusType::clone(const Demo_WorkOrderStatusType& source)
{
    Demo_WorkOrderStatusType* pWorkOrderStatusType = OpcUa_Null;
    Demo_WorkOrderStatusType_Copy(&source, &pWorkOrderStatusType);
    return pWorkOrderStatusType;
}

/** Copy Demo_WorkOrderStatusType data to an existing Demo_WorkOrderStatusType structure.
 *  @param source the source of the clone operation.
 *  @param copy the destination of the clone operation.
 */
void WorkOrderStatusType::cloneTo(const Demo_WorkOrderStatusType& source, Demo_WorkOrderStatusType& copy)
{
    Demo_WorkOrderStatusType_CopyTo(&source, &copy);
}

/** Attaches the data of the parameter pValue.
 * Don't clear the data of pValue afterwards manually, because WorkOrderStatusType is responsible for it now.
 */
void WorkOrderStatusType::attach(const Demo_WorkOrderStatusType *pValue) //!< [in] Parameter to attach to.
{
    if (pValue == OpcUa_Null) return;
    if (d_ptr->refCount() == 1)
    {
        Demo_WorkOrderStatusType_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new WorkOrderStatusTypePrivate();
        d_ptr->addRef();
    }
    // copy data value structure (no deep copy)
    OpcUa_MemCpy(d_ptr, sizeof(Demo_WorkOrderStatusType), (void*)pValue, sizeof(Demo_WorkOrderStatusType));
}

/** Detaches the internal WorkOrderStatusType structure from this class.
 *  This way you take over the control of releasing the WorkOrderStatusType data.
 *  If more than one references exist, the data is copied to pDst instead of being detached.
 *  @return If the functions succeeds pDst is returned, otherwise 0 is returned.
 */
Demo_WorkOrderStatusType* WorkOrderStatusType::detach(Demo_WorkOrderStatusType* pDst) //!< [out] Pointer to Demo_WorkOrderStatusType structure that receives the WorkOrderStatusType data.
{
    if (pDst == OpcUa_Null) return OpcUa_Null;
    if (d_ptr->refCount() == 1)
    {
        OpcUa_MemCpy(pDst, sizeof(Demo_WorkOrderStatusType), d_ptr, sizeof(Demo_WorkOrderStatusType));
        // clear internal data without releasing deeper memory
        Demo_WorkOrderStatusType_Initialize(d_ptr);
    }
    else
    {
        Demo_WorkOrderStatusType_CopyTo(d_ptr, pDst);
    }
    d_ptr->release();
    d_ptr = shared_empty_WorkOrderStatusType();
    d_ptr->addRef();
    return pDst;
}

void WorkOrderStatusType::toVariant(UaVariant &variant) const
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void WorkOrderStatusType::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject);
}

void WorkOrderStatusType::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject, bDetach);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void WorkOrderStatusType::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject, bDetach);
}

void WorkOrderStatusType::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderStatusType::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
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

void WorkOrderStatusType::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderStatusType::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
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

void WorkOrderStatusType::toExtensionObject(UaExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject);
}

void WorkOrderStatusType::toExtensionObject(OpcUa_ExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_WorkOrderStatusType *pWorkOrderStatusType = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderStatusType_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pWorkOrderStatusType)))
    {
        Demo_WorkOrderStatusType_CopyTo(d_ptr, pWorkOrderStatusType);
    }
}

void WorkOrderStatusType::toExtensionObject(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject, bDetach);
}

void WorkOrderStatusType::toExtensionObject(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_WorkOrderStatusType *pWorkOrderStatusType = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderStatusType_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pWorkOrderStatusType)))
    {
        Demo_WorkOrderStatusType_Initialize(pWorkOrderStatusType);
        if (bDetach != OpcUa_False && d_ptr->refCount() == 1)
        {
            *pWorkOrderStatusType = *d_ptr;
            // clear internal data without releasing deeper memory
            Demo_WorkOrderStatusType_Initialize(d_ptr);
        }
        else
        {
            Demo_WorkOrderStatusType_CopyTo(d_ptr, pWorkOrderStatusType);
        }
        if (bDetach != OpcUa_False)
        {
            d_ptr->release();
            d_ptr = shared_empty_WorkOrderStatusType();
            d_ptr->addRef();
        }
    }
}

OpcUa_StatusCode WorkOrderStatusType::setWorkOrderStatusType(const UaExtensionObject &extensionObject)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setWorkOrderStatusType(*pExtensionObject);
}

OpcUa_StatusCode WorkOrderStatusType::setWorkOrderStatusType(const OpcUa_ExtensionObject &extensionObject)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_WorkOrderStatusType *pWorkOrderStatusType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderStatusType, &extensionObject);

    if (pWorkOrderStatusType)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_WorkOrderStatusType_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new WorkOrderStatusTypePrivate();
            d_ptr->addRef();
        }

        Demo_WorkOrderStatusType_CopyTo(pWorkOrderStatusType, d_ptr);
    }
    return ret;
}

OpcUa_StatusCode WorkOrderStatusType::setWorkOrderStatusType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setWorkOrderStatusType(*pExtensionObject, bDetach);
}

OpcUa_StatusCode WorkOrderStatusType::setWorkOrderStatusType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_WorkOrderStatusType *pWorkOrderStatusType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderStatusType, &extensionObject);

    if (pWorkOrderStatusType != OpcUa_Null)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_WorkOrderStatusType_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new WorkOrderStatusTypePrivate();
            d_ptr->addRef();
        }

        if (bDetach == OpcUa_False)
        {
            Demo_WorkOrderStatusType_CopyTo(pWorkOrderStatusType, d_ptr);
        }
        else
        {
            (*(Demo_WorkOrderStatusType*)d_ptr) = *pWorkOrderStatusType;
            Demo_WorkOrderStatusType_Initialize(pWorkOrderStatusType);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
    return ret;
}

void WorkOrderStatusType::setWorkOrderStatusType(
        const UaString& Actor,
        const UaDateTime& Timestamp,
        const UaLocalizedText& Comment
        )
{
    if (d_ptr->refCount() == 1)
    {
        Demo_WorkOrderStatusType_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new WorkOrderStatusTypePrivate();
        d_ptr->addRef();
    }
    setActor(Actor);
    setTimestamp(Timestamp);
    setComment(Comment);
}

UaString WorkOrderStatusType::getActor() const
{
    return d_ptr->Actor;
}

UaDateTime WorkOrderStatusType::getTimestamp() const
{
    return d_ptr->Timestamp;
}

UaLocalizedText WorkOrderStatusType::getComment() const
{
    return d_ptr->Comment;
}

void WorkOrderStatusType::setActor(const UaString& Actor)
{
    if (d_ptr->refCount() > 1)
    {
        WorkOrderStatusTypePrivate *pNew = new WorkOrderStatusTypePrivate((const Demo_WorkOrderStatusType)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    OpcUa_String_Clear(&d_ptr->Actor);
    Actor.copyTo(&d_ptr->Actor);
}

void WorkOrderStatusType::setTimestamp(const UaDateTime& Timestamp)
{
    if (d_ptr->refCount() > 1)
    {
        WorkOrderStatusTypePrivate *pNew = new WorkOrderStatusTypePrivate((const Demo_WorkOrderStatusType)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    d_ptr->Timestamp = Timestamp;
}

void WorkOrderStatusType::setComment(const UaLocalizedText& Comment)
{
    if (d_ptr->refCount() > 1)
    {
        WorkOrderStatusTypePrivate *pNew = new WorkOrderStatusTypePrivate((const Demo_WorkOrderStatusType)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    OpcUa_LocalizedText_Clear(&d_ptr->Comment);
    Comment.copyTo(&d_ptr->Comment);
}


/******************************************************************************
** Class WorkOrderStatusTypes
******************************************************************************/
WorkOrderStatusTypes WorkOrderStatusTypes::empty;

WorkOrderStatusTypes::WorkOrderStatusTypes()
: m_noOfElements(0),
  m_data((Demo_WorkOrderStatusType*)OpcUa_Null)
{
}

WorkOrderStatusTypes::WorkOrderStatusTypes(const WorkOrderStatusTypes &other)
: m_noOfElements(0),
  m_data((Demo_WorkOrderStatusType*)OpcUa_Null)
{
    operator=(other);
}

WorkOrderStatusTypes::WorkOrderStatusTypes(OpcUa_Int32 length, Demo_WorkOrderStatusType* data)
: m_noOfElements(0),
  m_data((Demo_WorkOrderStatusType*)OpcUa_Null)
{
    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_WorkOrderStatusType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderStatusType));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderStatusType_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
}

WorkOrderStatusTypes::~WorkOrderStatusTypes()
{
    clear();
}

WorkOrderStatusTypes& WorkOrderStatusTypes::operator=(const WorkOrderStatusTypes &other)
{
    if (this == &other) {return *this;}

    clear();
    if (other.m_noOfElements > 0)
    {
        m_noOfElements = other.m_noOfElements;
        m_data = (Demo_WorkOrderStatusType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderStatusType));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderStatusType_CopyTo(&(other.m_data[i]), &(m_data[i]));
        }
    }
    return *this;
}

/** Returns the Demo_WorkOrderStatusType at index position index as a modifiable reference.
 *
 *  index must be valid index position in the array.
 */
Demo_WorkOrderStatusType& WorkOrderStatusTypes::operator[](OpcUa_UInt32 index)
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Returns the Demo_WorkOrderStatusType at index position index.
 *
 *  index must be valid index position in the array.
 */
const Demo_WorkOrderStatusType& WorkOrderStatusTypes::operator[](OpcUa_UInt32 index) const
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because WorkOrderStatusTypes is responsible for it now.
 */
void WorkOrderStatusTypes::attach(OpcUa_UInt32 length, Demo_WorkOrderStatusType* data)
{
    clear();
    m_data = data;
    m_noOfElements = length;
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because WorkOrderStatusTypes is responsible for it now.
 */
void WorkOrderStatusTypes::attach(OpcUa_Int32 length, Demo_WorkOrderStatusType* data)
{
    clear();
    if (length >= 0)
    {
        m_data = data;
        m_noOfElements = length;
    }
}

bool WorkOrderStatusTypes::operator==(const WorkOrderStatusTypes &other) const
{
    if (m_noOfElements != other.m_noOfElements) {return false;}

    if (m_noOfElements > 0)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            if (Demo_WorkOrderStatusType_Compare(&(m_data[i]), &(other.m_data[i])) != OPCUA_EQUAL) {return false;};
        }
    }
    return true;
}

bool WorkOrderStatusTypes::operator!=(const WorkOrderStatusTypes &other) const
{
    return !(operator==(other));
}

/** Detaches the raw data from this class.
 *
 *  The caller takes ownership of the raw data and is responsible for deleting it
 *  by calling OpcUa_Free.
 *  @return The raw data.
 */
Demo_WorkOrderStatusType* WorkOrderStatusTypes::detach()
{
    Demo_WorkOrderStatusType* pRet = m_data;
    m_noOfElements = 0;
    m_data = (Demo_WorkOrderStatusType*)OpcUa_Null;
    return pRet;
}

void WorkOrderStatusTypes::create(OpcUa_UInt32 length)
{
    clear();
    if (length > 0)
    {
        m_data = (Demo_WorkOrderStatusType*)OpcUa_Alloc(length * sizeof(Demo_WorkOrderStatusType));
        for (OpcUa_UInt32 i = 0; i < length; i++)
        {
            Demo_WorkOrderStatusType_Initialize(&(m_data[i]));
        }
        m_noOfElements = length;
    }
}

void WorkOrderStatusTypes::resize(OpcUa_UInt32 length)
{
    if (length < m_noOfElements)
    {
        if (length != 0)
        {
            for (OpcUa_UInt32 i = length; i < m_noOfElements; i++)
            {
                Demo_WorkOrderStatusType_Clear(&(m_data[i]));
            }

            Demo_WorkOrderStatusType* newData = (Demo_WorkOrderStatusType*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_WorkOrderStatusType));
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
        Demo_WorkOrderStatusType* newData = (Demo_WorkOrderStatusType*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_WorkOrderStatusType));
        if (newData)
        {
            for (OpcUa_UInt32 i = m_noOfElements; i < length; i++)
            {
                Demo_WorkOrderStatusType_Initialize(&(newData[i]));
            }
            m_data = newData;
            m_noOfElements = length;
        }
    }
}

void WorkOrderStatusTypes::clear()
{
    if (m_data)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderStatusType_Clear(&(m_data[i]));
        }
        OpcUa_Free(m_data);
        m_data = (Demo_WorkOrderStatusType*)OpcUa_Null;
    }
    m_noOfElements = 0;
}

void WorkOrderStatusTypes::toVariant(UaVariant &variant) const
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant);
}

void WorkOrderStatusTypes::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);

    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_WorkOrderStatusType *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderStatusType_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        Demo_WorkOrderStatusType_CopyTo(&m_data[i], pValue);
    }
}

void WorkOrderStatusTypes::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant, bDetach);
}

void WorkOrderStatusTypes::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_WorkOrderStatusType *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderStatusType_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        if (bDetach != OpcUa_False)
        {
            *pValue = m_data[i];
        }
        else
        {
            Demo_WorkOrderStatusType_CopyTo(&m_data[i], pValue);
        }
    }
    if (bDetach != OpcUa_False)
    {
        OpcUa_Free(m_data);
        m_data = (Demo_WorkOrderStatusType*)OpcUa_Null;
        m_noOfElements = 0;
    }
}

void WorkOrderStatusTypes::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderStatusTypes::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    toVariant(dataValue.Value);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

void WorkOrderStatusTypes::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderStatusTypes::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    toVariant(dataValue.Value, bDetach);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

OpcUa_StatusCode WorkOrderStatusTypes::setWorkOrderStatusTypes(const UaVariant &variant)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setWorkOrderStatusTypes(*pVariant);
}

OpcUa_StatusCode WorkOrderStatusTypes::setWorkOrderStatusTypes(const OpcUa_Variant &variant)
{
    return setWorkOrderStatusTypes((OpcUa_Variant&)variant, OpcUa_False);
}

OpcUa_StatusCode WorkOrderStatusTypes::setWorkOrderStatusTypes(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setWorkOrderStatusTypes(*pVariant, bDetach);
}

OpcUa_StatusCode WorkOrderStatusTypes::setWorkOrderStatusTypes(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
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
            m_data = (Demo_WorkOrderStatusType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderStatusType));

            OpcUa_UInt32 i = 0;
            for (i = 0; i < m_noOfElements; i++)
            {
                Demo_WorkOrderStatusType_Initialize(&m_data[i]);

                Demo_WorkOrderStatusType *pValue = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderStatusType, &variant.Value.Array.Value.ExtensionObjectArray[i]);
                if (pValue)
                {
                    if (bDetach != OpcUa_False)
                    {
                        m_data[i] = *pValue;
                        Demo_WorkOrderStatusType_Initialize(pValue);
                    }
                    else
                    {
                        Demo_WorkOrderStatusType_CopyTo(pValue, &m_data[i]);
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
                    Demo_WorkOrderStatusType_Clear(&m_data[j]);
                }
                OpcUa_Free(m_data);
                m_data = (Demo_WorkOrderStatusType*)OpcUa_Null;
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

OpcUa_StatusCode WorkOrderStatusTypes::setWorkOrderStatusTypes(OpcUa_Int32 length, Demo_WorkOrderStatusType* data)
{
    clear();

    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_WorkOrderStatusType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderStatusType));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderStatusType_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
    return OpcUa_Good;
}

} // namespace Demo


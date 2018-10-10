/******************************************************************************
** demo_workordertype.cpp
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

#include "demo_workordertype.h"

#include <opcua_datetime.h>

#include "uavariant.h"
#include "uadatavalue.h"
#include "uaextensionobject.h"
#include "uaatomic.h"
#include "demo_workorderstatustype.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

/******************************************************************************
** Class WorkOrderTypePrivate
******************************************************************************/
class WorkOrderTypePrivate : private Demo_WorkOrderType
{
    WorkOrderTypePrivate(const WorkOrderTypePrivate &other);
public:
    WorkOrderTypePrivate();
    explicit WorkOrderTypePrivate(bool bStatic);
    explicit WorkOrderTypePrivate(const Demo_WorkOrderType &other);
    ~WorkOrderTypePrivate();

    int addRef();
    int release();
    inline int refCount() { return m_iRefCnt; }

private:
    int m_iRefCnt;
    friend class WorkOrderType;
};

/* construction */
WorkOrderTypePrivate::WorkOrderTypePrivate()
 : m_iRefCnt(0)
{
    Demo_WorkOrderType_Initialize(this);
}

/*  Static initializer.
 *  This is used internaly to initialize the static shared_emtpy variable.
 *  RefCnt is set to 1 so it will never be deleted.
 */
WorkOrderTypePrivate::WorkOrderTypePrivate(bool)
 : m_iRefCnt(1)
{
    Demo_WorkOrderType_Initialize(this);
}


/*  construction
 */
WorkOrderTypePrivate::WorkOrderTypePrivate(const Demo_WorkOrderType &other) // [in] the other Demo_WorkOrderType.
 : m_iRefCnt(0)
{
    Demo_WorkOrderType_CopyTo(&other, this);
}

/* destruction */
WorkOrderTypePrivate::~WorkOrderTypePrivate()
{
    Demo_WorkOrderType_Clear(this);
}

/*  Add a reference to a WorkOrderTypePrivate
 *  @return 0 if the object was deleted - otherwise a value bigger than 0.
 *  This method is not thread save regarding the return value. The returned counter is only reliable when it is 0.
 *  Other values are not reliable and should not be used for any logic.
 */
int WorkOrderTypePrivate::addRef()
{
    ua_atomic_increment(&m_iRefCnt);
    return m_iRefCnt;
}

/*  Release a WorkOrderTypePrivate.
 *  @return the reference count.
 */
int WorkOrderTypePrivate::release()
{
    int currentCount = ua_atomic_decrement(&m_iRefCnt);
    if (currentCount == 0)
    {
        delete this;
    }
    return currentCount;
}

WorkOrderTypePrivate* shared_empty_WorkOrderType()
{
    static WorkOrderTypePrivate shared_static(true);
    return &shared_static;
}


/******************************************************************************
** Class WorkOrderType
******************************************************************************/

/** Constructs an instance of the class WorkOrderType with empty default values
*/
WorkOrderType::WorkOrderType()
{
    d_ptr = shared_empty_WorkOrderType();
    d_ptr->addRef();
}

/** Constructs an instance of the class WorkOrderType with values from another WorkOrderType object
 *  @param other the other UaWorkOrderType used to initialize the object
 */
WorkOrderType::WorkOrderType(const WorkOrderType &other)
{
    d_ptr = other.d_ptr;
    d_ptr->addRef();
}

/** Constructs an instance of the class WorkOrderType with values from another Demo_WorkOrderType structure
 *  @param other the other Demo_WorkOrderType used to initialize the object
 */
WorkOrderType::WorkOrderType(const Demo_WorkOrderType &other)
{
    d_ptr = new WorkOrderTypePrivate(other);
    d_ptr->addRef();
}

/** Constructs an instance of the class WorkOrderType with values provided in the constructor
 */
WorkOrderType::WorkOrderType(
        const UaGuid& ID,
        const UaString& AssetID,
        const UaDateTime& StartTime,
        const Demo::WorkOrderStatusTypes& StatusComments
        )
{
    d_ptr = new WorkOrderTypePrivate();
    d_ptr->addRef();
    setID(ID);
    setAssetID(AssetID);
    setStartTime(StartTime);
    setStatusComments(StatusComments);
}

/** Constructs an instance of the class WorkOrderType initialized with value from a UaExtensionObject
 *  @param extensionObject the extension object containing the WorkOrderType
 */
WorkOrderType::WorkOrderType(const UaExtensionObject &extensionObject)
{
    d_ptr = new WorkOrderTypePrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_WorkOrderType *pWorkOrderType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderType, pExtensionObject);

    if (pWorkOrderType)
    {
        Demo_WorkOrderType_CopyTo(pWorkOrderType, d_ptr);
    }
}

/** Constructs an instance of the class WorkOrderType initialized with value from an OpcUa_ExtensionObject structure
 *  @param extensionObject the extension object containing the WorkOrderType
 */
WorkOrderType::WorkOrderType(const OpcUa_ExtensionObject &extensionObject)
{
    d_ptr = new WorkOrderTypePrivate();
    d_ptr->addRef();

    Demo_WorkOrderType *pWorkOrderType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderType, &extensionObject);

    if (pWorkOrderType)
    {
        Demo_WorkOrderType_CopyTo(pWorkOrderType, d_ptr);
    }
}

WorkOrderType::WorkOrderType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new WorkOrderTypePrivate();
    d_ptr->addRef();

    const OpcUa_ExtensionObject *pExtensionObject = (const OpcUa_ExtensionObject*)extensionObject;
    Demo_WorkOrderType *pWorkOrderType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderType, pExtensionObject);
    if (pWorkOrderType)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_WorkOrderType_CopyTo(pWorkOrderType, d_ptr);
        }
        else
        {
            (*(Demo_WorkOrderType*)d_ptr) = *pWorkOrderType;
            Demo_WorkOrderType_Initialize(pWorkOrderType);
            extensionObject.clear();
        }
    }
}

WorkOrderType::WorkOrderType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    d_ptr = new WorkOrderTypePrivate();
    d_ptr->addRef();

    Demo_WorkOrderType *pWorkOrderType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderType, &extensionObject);

    if (pWorkOrderType)
    {
        if (bDetach == OpcUa_False)
        {
            Demo_WorkOrderType_CopyTo(pWorkOrderType, d_ptr);
        }
        else
        {
            (*(Demo_WorkOrderType*)d_ptr) = *pWorkOrderType;
            Demo_WorkOrderType_Initialize(pWorkOrderType);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
}

/** Destroys the WorkOrderType object
*/
WorkOrderType::~WorkOrderType()
{
    DEMO_D(WorkOrderType);
    d->release();
}

/** Clear the data of the WorkOrderType.
*/
void WorkOrderType::clear()
{
    d_ptr->release();
    d_ptr = shared_empty_WorkOrderType();
    d_ptr->addRef();
}

/** Compare two WorkOrderType if they are similar.
 *  @param other the WorkOrderType to compare.
 *  @return true if similar, false if not.
 */
bool WorkOrderType::operator==(const WorkOrderType &other) const
{
    return (Demo_WorkOrderType_Compare(d_ptr, other.d_ptr) == 0);
}

/** Returns true if the other WorkOrderType is not equal to this.
 *  @see operator==
 *  @param other the WorkOrderType to compare.
 *  @return true if other is not equal to this.
 */
bool WorkOrderType::operator!=(const WorkOrderType &other) const
{
    return !(operator==(other));
}

/** Assignment operator.
 *  @param other the WorkOrderType to assign.
 *  @return Assignment operator.
 */
WorkOrderType& WorkOrderType::operator=(const WorkOrderType &other)
{
    other.d_ptr->addRef();
    d_ptr->release();
    d_ptr = other.d_ptr;
    return *this;
}

/** Copy WorkOrderType data to a newly allocated Demo_WorkOrderType.
 *  @return data to a newly allocated Demo_WorkOrderType.
 */
Demo_WorkOrderType* WorkOrderType::copy() const
{
    return clone(*d_ptr);
}

/** Copy WorkOrderType data to an existing Demo_WorkOrderType structure.
 *  @param pDst the destination of this copy operation.
 */
void WorkOrderType::copyTo(Demo_WorkOrderType *pDst) const
{
    if (pDst == OpcUa_Null) return;
    cloneTo(*d_ptr, *pDst);
}

/** Copy Demo_WorkOrderType data to a newly allocated Demo_WorkOrderType.
 *  @param source Source to clone.
 *  @return new copy.
 */
Demo_WorkOrderType* WorkOrderType::clone(const Demo_WorkOrderType& source)
{
    Demo_WorkOrderType* pWorkOrderType = OpcUa_Null;
    Demo_WorkOrderType_Copy(&source, &pWorkOrderType);
    return pWorkOrderType;
}

/** Copy Demo_WorkOrderType data to an existing Demo_WorkOrderType structure.
 *  @param source the source of the clone operation.
 *  @param copy the destination of the clone operation.
 */
void WorkOrderType::cloneTo(const Demo_WorkOrderType& source, Demo_WorkOrderType& copy)
{
    Demo_WorkOrderType_CopyTo(&source, &copy);
}

/** Attaches the data of the parameter pValue.
 * Don't clear the data of pValue afterwards manually, because WorkOrderType is responsible for it now.
 */
void WorkOrderType::attach(const Demo_WorkOrderType *pValue) //!< [in] Parameter to attach to.
{
    if (pValue == OpcUa_Null) return;
    if (d_ptr->refCount() == 1)
    {
        Demo_WorkOrderType_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new WorkOrderTypePrivate();
        d_ptr->addRef();
    }
    // copy data value structure (no deep copy)
    OpcUa_MemCpy(d_ptr, sizeof(Demo_WorkOrderType), (void*)pValue, sizeof(Demo_WorkOrderType));
}

/** Detaches the internal WorkOrderType structure from this class.
 *  This way you take over the control of releasing the WorkOrderType data.
 *  If more than one references exist, the data is copied to pDst instead of being detached.
 *  @return If the functions succeeds pDst is returned, otherwise 0 is returned.
 */
Demo_WorkOrderType* WorkOrderType::detach(Demo_WorkOrderType* pDst) //!< [out] Pointer to Demo_WorkOrderType structure that receives the WorkOrderType data.
{
    if (pDst == OpcUa_Null) return OpcUa_Null;
    if (d_ptr->refCount() == 1)
    {
        OpcUa_MemCpy(pDst, sizeof(Demo_WorkOrderType), d_ptr, sizeof(Demo_WorkOrderType));
        // clear internal data without releasing deeper memory
        Demo_WorkOrderType_Initialize(d_ptr);
    }
    else
    {
        Demo_WorkOrderType_CopyTo(d_ptr, pDst);
    }
    d_ptr->release();
    d_ptr = shared_empty_WorkOrderType();
    d_ptr->addRef();
    return pDst;
}

void WorkOrderType::toVariant(UaVariant &variant) const
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void WorkOrderType::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject);
}

void WorkOrderType::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    UaExtensionObject extensionObject;
    toExtensionObject(extensionObject, bDetach);
    variant.setExtensionObject(extensionObject, OpcUa_True);
}

void WorkOrderType::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Scalar;
    variant.Value.ExtensionObject = (OpcUa_ExtensionObject*)OpcUa_Alloc(sizeof(OpcUa_ExtensionObject));
    OpcUa_ExtensionObject_Initialize(variant.Value.ExtensionObject);
    toExtensionObject(*variant.Value.ExtensionObject, bDetach);
}

void WorkOrderType::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderType::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
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

void WorkOrderType::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderType::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
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

void WorkOrderType::toExtensionObject(UaExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject);
}

void WorkOrderType::toExtensionObject(OpcUa_ExtensionObject &extensionObject) const
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_WorkOrderType *pWorkOrderType = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderType_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pWorkOrderType)))
    {
        Demo_WorkOrderType_CopyTo(d_ptr, pWorkOrderType);
    }
}

void WorkOrderType::toExtensionObject(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    toExtensionObject(*pExtensionObject, bDetach);
}

void WorkOrderType::toExtensionObject(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject_Clear(&extensionObject);
    Demo_WorkOrderType *pWorkOrderType = OpcUa_Null;
    if (OpcUa_IsGood(OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderType_EncodeableType,
                                                            &extensionObject,
                                                            (OpcUa_Void**)&pWorkOrderType)))
    {
        Demo_WorkOrderType_Initialize(pWorkOrderType);
        if (bDetach != OpcUa_False && d_ptr->refCount() == 1)
        {
            *pWorkOrderType = *d_ptr;
            // clear internal data without releasing deeper memory
            Demo_WorkOrderType_Initialize(d_ptr);
        }
        else
        {
            Demo_WorkOrderType_CopyTo(d_ptr, pWorkOrderType);
        }
        if (bDetach != OpcUa_False)
        {
            d_ptr->release();
            d_ptr = shared_empty_WorkOrderType();
            d_ptr->addRef();
        }
    }
}

OpcUa_StatusCode WorkOrderType::setWorkOrderType(const UaExtensionObject &extensionObject)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setWorkOrderType(*pExtensionObject);
}

OpcUa_StatusCode WorkOrderType::setWorkOrderType(const OpcUa_ExtensionObject &extensionObject)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_WorkOrderType *pWorkOrderType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderType, &extensionObject);

    if (pWorkOrderType)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_WorkOrderType_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new WorkOrderTypePrivate();
            d_ptr->addRef();
        }

        Demo_WorkOrderType_CopyTo(pWorkOrderType, d_ptr);
    }
    return ret;
}

OpcUa_StatusCode WorkOrderType::setWorkOrderType(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_ExtensionObject *pExtensionObject = (OpcUa_ExtensionObject*)(const OpcUa_ExtensionObject*)extensionObject;
    return setWorkOrderType(*pExtensionObject, bDetach);
}

OpcUa_StatusCode WorkOrderType::setWorkOrderType(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach)
{
    OpcUa_StatusCode ret = OpcUa_BadTypeMismatch;
    Demo_WorkOrderType *pWorkOrderType = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderType, &extensionObject);

    if (pWorkOrderType != OpcUa_Null)
    {
        ret = OpcUa_Good;

        if (d_ptr->refCount() == 1)
        {
            Demo_WorkOrderType_Clear(d_ptr);
        }
        else
        {
            d_ptr->release();
            d_ptr = new WorkOrderTypePrivate();
            d_ptr->addRef();
        }

        if (bDetach == OpcUa_False)
        {
            Demo_WorkOrderType_CopyTo(pWorkOrderType, d_ptr);
        }
        else
        {
            (*(Demo_WorkOrderType*)d_ptr) = *pWorkOrderType;
            Demo_WorkOrderType_Initialize(pWorkOrderType);
            OpcUa_ExtensionObject_Clear(&extensionObject);
        }
    }
    return ret;
}

void WorkOrderType::setWorkOrderType(
        const UaGuid& ID,
        const UaString& AssetID,
        const UaDateTime& StartTime,
        const Demo::WorkOrderStatusTypes& StatusComments
        )
{
    if (d_ptr->refCount() == 1)
    {
        Demo_WorkOrderType_Clear(d_ptr);
    }
    else
    {
        d_ptr->release();
        d_ptr = new WorkOrderTypePrivate();
        d_ptr->addRef();
    }
    setID(ID);
    setAssetID(AssetID);
    setStartTime(StartTime);
    setStatusComments(StatusComments);
}

UaGuid WorkOrderType::getID() const
{
    return d_ptr->ID;
}

UaString WorkOrderType::getAssetID() const
{
    return d_ptr->AssetID;
}

UaDateTime WorkOrderType::getStartTime() const
{
    return d_ptr->StartTime;
}

void WorkOrderType::getStatusComments(Demo::WorkOrderStatusTypes& StatusComments) const
{
    StatusComments.clear();
    if (d_ptr->NoOfStatusComments > 0)
    {
        StatusComments.create(d_ptr->NoOfStatusComments);
        OpcUa_Int32 i = 0;
        for (i = 0; i < d_ptr->NoOfStatusComments; i++)
        {
            Demo_WorkOrderStatusType_CopyTo(&d_ptr->StatusComments[i], &StatusComments[i]);
        }
    }
}

void WorkOrderType::setID(const UaGuid& ID)
{
    if (d_ptr->refCount() > 1)
    {
        WorkOrderTypePrivate *pNew = new WorkOrderTypePrivate((const Demo_WorkOrderType)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    OpcUa_Guid_Clear(&d_ptr->ID);
    ID.copyTo(&d_ptr->ID);
}

void WorkOrderType::setAssetID(const UaString& AssetID)
{
    if (d_ptr->refCount() > 1)
    {
        WorkOrderTypePrivate *pNew = new WorkOrderTypePrivate((const Demo_WorkOrderType)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    OpcUa_String_Clear(&d_ptr->AssetID);
    AssetID.copyTo(&d_ptr->AssetID);
}

void WorkOrderType::setStartTime(const UaDateTime& StartTime)
{
    if (d_ptr->refCount() > 1)
    {
        WorkOrderTypePrivate *pNew = new WorkOrderTypePrivate((const Demo_WorkOrderType)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }

    d_ptr->StartTime = StartTime;
}

void WorkOrderType::setStatusComments(const Demo::WorkOrderStatusTypes& StatusComments)
{
    if (d_ptr->refCount() > 1)
    {
        WorkOrderTypePrivate *pNew = new WorkOrderTypePrivate((const Demo_WorkOrderType)*d_ptr);
        d_ptr->release();
        d_ptr = pNew;
        d_ptr->addRef();
    }
    if (d_ptr->NoOfStatusComments > 0)
    {
        OpcUa_Int32 i = 0;
        for (i = 0; i < d_ptr->NoOfStatusComments; i++)
        {
            Demo_WorkOrderStatusType_Clear(&d_ptr->StatusComments[i]);
        }
        OpcUa_Free(d_ptr->StatusComments);
    }

    d_ptr->NoOfStatusComments = StatusComments.length();
    if (d_ptr->NoOfStatusComments > 0)
    {
        d_ptr->StatusComments = (Demo_WorkOrderStatusType*)OpcUa_Alloc(d_ptr->NoOfStatusComments * sizeof(Demo_WorkOrderStatusType));
        if (d_ptr->StatusComments != OpcUa_Null)
        {
            OpcUa_Int32 i = 0;
            for (i = 0; i < d_ptr->NoOfStatusComments; i++)
            {
                Demo_WorkOrderStatusType_Initialize(&d_ptr->StatusComments[i]);
                Demo_WorkOrderStatusType_CopyTo(&StatusComments[i], &d_ptr->StatusComments[i]);
            }
        }
        else
        {
            d_ptr->NoOfStatusComments = 0;
        }
    }
}


/******************************************************************************
** Class WorkOrderTypes
******************************************************************************/
WorkOrderTypes WorkOrderTypes::empty;

WorkOrderTypes::WorkOrderTypes()
: m_noOfElements(0),
  m_data((Demo_WorkOrderType*)OpcUa_Null)
{
}

WorkOrderTypes::WorkOrderTypes(const WorkOrderTypes &other)
: m_noOfElements(0),
  m_data((Demo_WorkOrderType*)OpcUa_Null)
{
    operator=(other);
}

WorkOrderTypes::WorkOrderTypes(OpcUa_Int32 length, Demo_WorkOrderType* data)
: m_noOfElements(0),
  m_data((Demo_WorkOrderType*)OpcUa_Null)
{
    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_WorkOrderType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderType));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderType_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
}

WorkOrderTypes::~WorkOrderTypes()
{
    clear();
}

WorkOrderTypes& WorkOrderTypes::operator=(const WorkOrderTypes &other)
{
    if (this == &other) {return *this;}

    clear();
    if (other.m_noOfElements > 0)
    {
        m_noOfElements = other.m_noOfElements;
        m_data = (Demo_WorkOrderType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderType));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderType_CopyTo(&(other.m_data[i]), &(m_data[i]));
        }
    }
    return *this;
}

/** Returns the Demo_WorkOrderType at index position index as a modifiable reference.
 *
 *  index must be valid index position in the array.
 */
Demo_WorkOrderType& WorkOrderTypes::operator[](OpcUa_UInt32 index)
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Returns the Demo_WorkOrderType at index position index.
 *
 *  index must be valid index position in the array.
 */
const Demo_WorkOrderType& WorkOrderTypes::operator[](OpcUa_UInt32 index) const
{
    assert(index<m_noOfElements);
    return m_data[index];
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because WorkOrderTypes is responsible for it now.
 */
void WorkOrderTypes::attach(OpcUa_UInt32 length, Demo_WorkOrderType* data)
{
    clear();
    m_data = data;
    m_noOfElements = length;
}

/** Attaches the raw data.
 *
 *  Clears the array and takes the ownership of data.
 *  Don't clear the data afterwards manually, because WorkOrderTypes is responsible for it now.
 */
void WorkOrderTypes::attach(OpcUa_Int32 length, Demo_WorkOrderType* data)
{
    clear();
    if (length >= 0)
    {
        m_data = data;
        m_noOfElements = length;
    }
}

bool WorkOrderTypes::operator==(const WorkOrderTypes &other) const
{
    if (m_noOfElements != other.m_noOfElements) {return false;}

    if (m_noOfElements > 0)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            if (Demo_WorkOrderType_Compare(&(m_data[i]), &(other.m_data[i])) != OPCUA_EQUAL) {return false;};
        }
    }
    return true;
}

bool WorkOrderTypes::operator!=(const WorkOrderTypes &other) const
{
    return !(operator==(other));
}

/** Detaches the raw data from this class.
 *
 *  The caller takes ownership of the raw data and is responsible for deleting it
 *  by calling OpcUa_Free.
 *  @return The raw data.
 */
Demo_WorkOrderType* WorkOrderTypes::detach()
{
    Demo_WorkOrderType* pRet = m_data;
    m_noOfElements = 0;
    m_data = (Demo_WorkOrderType*)OpcUa_Null;
    return pRet;
}

void WorkOrderTypes::create(OpcUa_UInt32 length)
{
    clear();
    if (length > 0)
    {
        m_data = (Demo_WorkOrderType*)OpcUa_Alloc(length * sizeof(Demo_WorkOrderType));
        for (OpcUa_UInt32 i = 0; i < length; i++)
        {
            Demo_WorkOrderType_Initialize(&(m_data[i]));
        }
        m_noOfElements = length;
    }
}

void WorkOrderTypes::resize(OpcUa_UInt32 length)
{
    if (length < m_noOfElements)
    {
        if (length != 0)
        {
            for (OpcUa_UInt32 i = length; i < m_noOfElements; i++)
            {
                Demo_WorkOrderType_Clear(&(m_data[i]));
            }

            Demo_WorkOrderType* newData = (Demo_WorkOrderType*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_WorkOrderType));
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
        Demo_WorkOrderType* newData = (Demo_WorkOrderType*)OpcUa_ReAlloc(m_data, length * sizeof(Demo_WorkOrderType));
        if (newData)
        {
            for (OpcUa_UInt32 i = m_noOfElements; i < length; i++)
            {
                Demo_WorkOrderType_Initialize(&(newData[i]));
            }
            m_data = newData;
            m_noOfElements = length;
        }
    }
}

void WorkOrderTypes::clear()
{
    if (m_data)
    {
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderType_Clear(&(m_data[i]));
        }
        OpcUa_Free(m_data);
        m_data = (Demo_WorkOrderType*)OpcUa_Null;
    }
    m_noOfElements = 0;
}

void WorkOrderTypes::toVariant(UaVariant &variant) const
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant);
}

void WorkOrderTypes::toVariant(OpcUa_Variant &variant) const
{
    OpcUa_Variant_Clear(&variant);

    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_WorkOrderType *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderType_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        Demo_WorkOrderType_CopyTo(&m_data[i], pValue);
    }
}

void WorkOrderTypes::toVariant(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    toVariant(*pVariant, bDetach);
}

void WorkOrderTypes::toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant_Clear(&variant);
    variant.Datatype = OpcUaType_ExtensionObject;
    variant.ArrayType = OpcUa_VariantArrayType_Array;
    variant.Value.Array.Length = m_noOfElements;
    variant.Value.Array.Value.ExtensionObjectArray = (OpcUa_ExtensionObject*)OpcUa_Alloc(m_noOfElements * sizeof(OpcUa_ExtensionObject));
    for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
    {
        Demo_WorkOrderType *pValue = OpcUa_Null;
        OpcUa_ExtensionObject_Initialize(&variant.Value.Array.Value.ExtensionObjectArray[i]);
        OpcUa_EncodeableObject_CreateExtension(&Demo_WorkOrderType_EncodeableType,
                                               &variant.Value.Array.Value.ExtensionObjectArray[i],
                                               (OpcUa_Void**)&pValue);
        if (bDetach != OpcUa_False)
        {
            *pValue = m_data[i];
        }
        else
        {
            Demo_WorkOrderType_CopyTo(&m_data[i], pValue);
        }
    }
    if (bDetach != OpcUa_False)
    {
        OpcUa_Free(m_data);
        m_data = (Demo_WorkOrderType*)OpcUa_Null;
        m_noOfElements = 0;
    }
}

void WorkOrderTypes::toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    UaVariant variant;
    toVariant(variant);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderTypes::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const
{
    toVariant(dataValue.Value);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

void WorkOrderTypes::toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    UaVariant variant;
    toVariant(variant, bDetach);
    dataValue.setValue(variant, OpcUa_True, updateTimeStamps);
}

void WorkOrderTypes::toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps)
{
    toVariant(dataValue.Value, bDetach);

    if (updateTimeStamps != OpcUa_False)
    {
        dataValue.SourceTimestamp = OpcUa_DateTime_UtcNow();
        dataValue.ServerTimestamp = OpcUa_DateTime_UtcNow();
    }
}

OpcUa_StatusCode WorkOrderTypes::setWorkOrderTypes(const UaVariant &variant)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setWorkOrderTypes(*pVariant);
}

OpcUa_StatusCode WorkOrderTypes::setWorkOrderTypes(const OpcUa_Variant &variant)
{
    return setWorkOrderTypes((OpcUa_Variant&)variant, OpcUa_False);
}

OpcUa_StatusCode WorkOrderTypes::setWorkOrderTypes(UaVariant &variant, OpcUa_Boolean bDetach)
{
    OpcUa_Variant *pVariant = (OpcUa_Variant*)(const OpcUa_Variant*)variant;
    return setWorkOrderTypes(*pVariant, bDetach);
}

OpcUa_StatusCode WorkOrderTypes::setWorkOrderTypes(OpcUa_Variant &variant, OpcUa_Boolean bDetach)
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
            m_data = (Demo_WorkOrderType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderType));

            OpcUa_UInt32 i = 0;
            for (i = 0; i < m_noOfElements; i++)
            {
                Demo_WorkOrderType_Initialize(&m_data[i]);

                Demo_WorkOrderType *pValue = DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(WorkOrderType, &variant.Value.Array.Value.ExtensionObjectArray[i]);
                if (pValue)
                {
                    if (bDetach != OpcUa_False)
                    {
                        m_data[i] = *pValue;
                        Demo_WorkOrderType_Initialize(pValue);
                    }
                    else
                    {
                        Demo_WorkOrderType_CopyTo(pValue, &m_data[i]);
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
                    Demo_WorkOrderType_Clear(&m_data[j]);
                }
                OpcUa_Free(m_data);
                m_data = (Demo_WorkOrderType*)OpcUa_Null;
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

OpcUa_StatusCode WorkOrderTypes::setWorkOrderTypes(OpcUa_Int32 length, Demo_WorkOrderType* data)
{
    clear();

    if (length > 0 && data != OpcUa_Null)
    {
        m_noOfElements = length;
        m_data = (Demo_WorkOrderType*)OpcUa_Alloc(m_noOfElements * sizeof(Demo_WorkOrderType));
        for (OpcUa_UInt32 i = 0; i < m_noOfElements; i++)
        {
            Demo_WorkOrderType_CopyTo(&(data[i]), &(m_data[i]));
        }
    }
    return OpcUa_Good;
}

} // namespace Demo


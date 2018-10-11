/******************************************************************************
** demo_datatypes.h
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

#ifndef __DEMO_DATATYPES_H__
#define __DEMO_DATATYPES_H__

#include <opcua_proxystub.h>
#include <opcua_builtintypes.h>
#include <opcua_exclusions.h>
#include <opcua_types.h>

#include "demo_identifiers.h"

#define DEMO_EXTENSIONOBJECT_GET_ENCODEABLE(xExpectedType, xExtensionObject) \
    (Demo_##xExpectedType*)((((xExtensionObject)->Encoding == OpcUa_ExtensionObjectEncoding_EncodeableObject && \
    (xExtensionObject)->Body.EncodeableObject.Type != OpcUa_Null && \
    (xExtensionObject)->Body.EncodeableObject.Type->TypeId == DemoId_##xExpectedType && \
    OpcUa_StrCmpA((xExtensionObject)->Body.EncodeableObject.Type->NamespaceUri, "http://www.unifiedautomation.com/DemoServer/") == 0 && \
    (xExtensionObject)->Body.EncodeableObject.Object != OpcUa_Null))?((xExtensionObject)->Body.EncodeableObject.Object):OpcUa_Null)


// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

#ifndef OPCUA_EXCLUDE_HeaterStatus
/*============================================================================
 * The HeaterStatus enumeration.
 *===========================================================================*/
enum HeaterStatus
{
    HeaterStatus_Off = 0,
    HeaterStatus_Heating = 1,
    HeaterStatus_Cooling = 2
#if OPCUA_FORCE_INT32_ENUMS
    ,_HeaterStatus_MaxEnumerationValue = OpcUa_Int32_Max
#endif
};

CPP_DEMO_EXPORT inline void HeaterStatus_Clear(HeaterStatus *pValue) {*pValue = HeaterStatus_Off;}

CPP_DEMO_EXPORT inline void HeaterStatus_Initialize(HeaterStatus *pValue) {*pValue = HeaterStatus_Off;}

CPP_DEMO_EXPORT extern struct ::_OpcUa_EnumeratedType HeaterStatus_EnumeratedType;

#endif /*OPCUA_EXCLUDE_HeaterStatus*/
#ifndef OPCUA_EXCLUDE_Priority
/*============================================================================
 * The Priority enumeration.
 *===========================================================================*/
/**
 * This enumerated value defines its possible values in an EnumValues property
 */
enum Priority
{
    Priority_Low = 10,
    Priority_Normal = 40,
    Priority_High = 70,
    Priority_Urgent = 90,
    Priority_Immediate = 100
#if OPCUA_FORCE_INT32_ENUMS
    ,_Priority_MaxEnumerationValue = OpcUa_Int32_Max
#endif
};

CPP_DEMO_EXPORT inline void Priority_Clear(Priority *pValue) {*pValue = Priority_Low;}

CPP_DEMO_EXPORT inline void Priority_Initialize(Priority *pValue) {*pValue = Priority_Low;}

CPP_DEMO_EXPORT extern struct ::_OpcUa_EnumeratedType Priority_EnumeratedType;

#endif /*OPCUA_EXCLUDE_Priority*/
}

#ifndef OPCUA_EXCLUDE_Demo_AccessRights
/*============================================================================
 * The Demo_AccessRights structure.
 *===========================================================================*/
typedef struct _Demo_AccessRights
{
    OpcUa_ByteString Value;/**< Array of bytes representing the bits in the option set.*/
    OpcUa_ByteString ValidBits;/**< Array of bytes with same size as value representing the valid bits in the value parameter.*/
} Demo_AccessRights;

CPP_DEMO_EXPORT OpcUa_Void Demo_AccessRights_Initialize(Demo_AccessRights* pValue);

CPP_DEMO_EXPORT OpcUa_Void Demo_AccessRights_Clear(Demo_AccessRights* pValue);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_AccessRights_GetSize(Demo_AccessRights* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_AccessRights_Encode(Demo_AccessRights* pValue, struct _OpcUa_Encoder* pEncoder);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_AccessRights_Decode(Demo_AccessRights* pValue, struct _OpcUa_Decoder* pDecoder);

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
CPP_DEMO_EXPORT OpcUa_Int        Demo_AccessRights_Compare(const Demo_AccessRights* pValue1, const Demo_AccessRights* pValue2);
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
CPP_DEMO_EXPORT OpcUa_StatusCode Demo_AccessRights_Copy(const Demo_AccessRights* pSource, Demo_AccessRights** ppCopy);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_AccessRights_CopyTo(const Demo_AccessRights* pSource, Demo_AccessRights* pDestination);
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

extern struct _OpcUa_EncodeableType Demo_AccessRights_EncodeableType;
#endif /*OPCUA_EXCLUDE_Demo_AccessRights*/

#ifndef OPCUA_EXCLUDE_Demo_StructureWithOptionalFields
/*============================================================================
 * The Demo_StructureWithOptionalFields structure.
 *===========================================================================*/
enum Demo_StructureWithOptionalFields_OptionalFields
{
    Demo_StructureWithOptionalFields_OptionalFields_OptionalInt32 = 1,
    Demo_StructureWithOptionalFields_OptionalFields_OptionalStringArray = 2,
};

typedef struct _Demo_StructureWithOptionalFields
{
    OpcUa_UInt32 EncodingMask;
    OpcUa_Int32 MandatoryInt32;
    OpcUa_Int32 OptionalInt32;
    OpcUa_Int32 NoOfMandatoryStringArray;
    OpcUa_String* MandatoryStringArray;
    OpcUa_Int32 NoOfOptionalStringArray;
    OpcUa_String* OptionalStringArray;
} Demo_StructureWithOptionalFields;

CPP_DEMO_EXPORT OpcUa_Void Demo_StructureWithOptionalFields_Initialize(Demo_StructureWithOptionalFields* pValue);

CPP_DEMO_EXPORT OpcUa_Void Demo_StructureWithOptionalFields_Clear(Demo_StructureWithOptionalFields* pValue);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_StructureWithOptionalFields_GetSize(Demo_StructureWithOptionalFields* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_StructureWithOptionalFields_Encode(Demo_StructureWithOptionalFields* pValue, struct _OpcUa_Encoder* pEncoder);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_StructureWithOptionalFields_Decode(Demo_StructureWithOptionalFields* pValue, struct _OpcUa_Decoder* pDecoder);

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
CPP_DEMO_EXPORT OpcUa_Int        Demo_StructureWithOptionalFields_Compare(const Demo_StructureWithOptionalFields* pValue1, const Demo_StructureWithOptionalFields* pValue2);
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
CPP_DEMO_EXPORT OpcUa_StatusCode Demo_StructureWithOptionalFields_Copy(const Demo_StructureWithOptionalFields* pSource, Demo_StructureWithOptionalFields** ppCopy);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_StructureWithOptionalFields_CopyTo(const Demo_StructureWithOptionalFields* pSource, Demo_StructureWithOptionalFields* pDestination);
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

extern struct _OpcUa_EncodeableType Demo_StructureWithOptionalFields_EncodeableType;
#endif /*OPCUA_EXCLUDE_Demo_StructureWithOptionalFields*/

#ifndef OPCUA_EXCLUDE_Demo_UnionTest
/*============================================================================
 * The Demo_UnionTest structure.
 *===========================================================================*/
enum Demo_UnionTest_Type
{
    Demo_UnionTest_Null = 0,
    Demo_UnionTest_Int32,
    Demo_UnionTest_String
};

typedef struct _Demo_UnionTest
{
    Demo_UnionTest_Type Type;
    union _Value
    {
        OpcUa_Int32 Int32;
        OpcUa_String String;
    } Value;
} Demo_UnionTest;

CPP_DEMO_EXPORT OpcUa_Void Demo_UnionTest_Initialize(Demo_UnionTest* pValue);

CPP_DEMO_EXPORT OpcUa_Void Demo_UnionTest_Clear(Demo_UnionTest* pValue);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_UnionTest_GetSize(Demo_UnionTest* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_UnionTest_Encode(Demo_UnionTest* pValue, struct _OpcUa_Encoder* pEncoder);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_UnionTest_Decode(Demo_UnionTest* pValue, struct _OpcUa_Decoder* pDecoder);

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
CPP_DEMO_EXPORT OpcUa_Int        Demo_UnionTest_Compare(const Demo_UnionTest* pValue1, const Demo_UnionTest* pValue2);
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
CPP_DEMO_EXPORT OpcUa_StatusCode Demo_UnionTest_Copy(const Demo_UnionTest* pSource, Demo_UnionTest** ppCopy);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_UnionTest_CopyTo(const Demo_UnionTest* pSource, Demo_UnionTest* pDestination);
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

extern struct _OpcUa_EncodeableType Demo_UnionTest_EncodeableType;
#endif /*OPCUA_EXCLUDE_Demo_UnionTest*/

#ifndef OPCUA_EXCLUDE_Demo_Vector
/*============================================================================
 * The Demo_Vector structure.
 *===========================================================================*/
typedef struct _Demo_Vector
{
    OpcUa_Double X;
    OpcUa_Double Y;
    OpcUa_Double Z;
} Demo_Vector;

CPP_DEMO_EXPORT OpcUa_Void Demo_Vector_Initialize(Demo_Vector* pValue);

CPP_DEMO_EXPORT OpcUa_Void Demo_Vector_Clear(Demo_Vector* pValue);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_Vector_GetSize(Demo_Vector* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_Vector_Encode(Demo_Vector* pValue, struct _OpcUa_Encoder* pEncoder);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_Vector_Decode(Demo_Vector* pValue, struct _OpcUa_Decoder* pDecoder);

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
CPP_DEMO_EXPORT OpcUa_Int        Demo_Vector_Compare(const Demo_Vector* pValue1, const Demo_Vector* pValue2);
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
CPP_DEMO_EXPORT OpcUa_StatusCode Demo_Vector_Copy(const Demo_Vector* pSource, Demo_Vector** ppCopy);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_Vector_CopyTo(const Demo_Vector* pSource, Demo_Vector* pDestination);
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

extern struct _OpcUa_EncodeableType Demo_Vector_EncodeableType;
#endif /*OPCUA_EXCLUDE_Demo_Vector*/

#ifndef OPCUA_EXCLUDE_Demo_WorkOrderStatusType
/*============================================================================
 * The Demo_WorkOrderStatusType structure.
 *===========================================================================*/
typedef struct _Demo_WorkOrderStatusType
{
    OpcUa_String Actor;
    OpcUa_DateTime Timestamp;
    OpcUa_LocalizedText Comment;
} Demo_WorkOrderStatusType;

CPP_DEMO_EXPORT OpcUa_Void Demo_WorkOrderStatusType_Initialize(Demo_WorkOrderStatusType* pValue);

CPP_DEMO_EXPORT OpcUa_Void Demo_WorkOrderStatusType_Clear(Demo_WorkOrderStatusType* pValue);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderStatusType_GetSize(Demo_WorkOrderStatusType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderStatusType_Encode(Demo_WorkOrderStatusType* pValue, struct _OpcUa_Encoder* pEncoder);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderStatusType_Decode(Demo_WorkOrderStatusType* pValue, struct _OpcUa_Decoder* pDecoder);

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
CPP_DEMO_EXPORT OpcUa_Int        Demo_WorkOrderStatusType_Compare(const Demo_WorkOrderStatusType* pValue1, const Demo_WorkOrderStatusType* pValue2);
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderStatusType_Copy(const Demo_WorkOrderStatusType* pSource, Demo_WorkOrderStatusType** ppCopy);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderStatusType_CopyTo(const Demo_WorkOrderStatusType* pSource, Demo_WorkOrderStatusType* pDestination);
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

extern struct _OpcUa_EncodeableType Demo_WorkOrderStatusType_EncodeableType;
#endif /*OPCUA_EXCLUDE_Demo_WorkOrderStatusType*/

#ifndef OPCUA_EXCLUDE_Demo_WorkOrderType
/*============================================================================
 * The Demo_WorkOrderType structure.
 *===========================================================================*/
typedef struct _Demo_WorkOrderType
{
    OpcUa_Guid ID;
    OpcUa_String AssetID;
    OpcUa_DateTime StartTime;
    OpcUa_Int32 NoOfStatusComments;
    Demo_WorkOrderStatusType* StatusComments;
} Demo_WorkOrderType;

CPP_DEMO_EXPORT OpcUa_Void Demo_WorkOrderType_Initialize(Demo_WorkOrderType* pValue);

CPP_DEMO_EXPORT OpcUa_Void Demo_WorkOrderType_Clear(Demo_WorkOrderType* pValue);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderType_GetSize(Demo_WorkOrderType* pValue, struct _OpcUa_Encoder* pEncoder, OpcUa_Int32* pSize);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderType_Encode(Demo_WorkOrderType* pValue, struct _OpcUa_Encoder* pEncoder);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderType_Decode(Demo_WorkOrderType* pValue, struct _OpcUa_Decoder* pDecoder);

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
CPP_DEMO_EXPORT OpcUa_Int        Demo_WorkOrderType_Compare(const Demo_WorkOrderType* pValue1, const Demo_WorkOrderType* pValue2);
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderType_Copy(const Demo_WorkOrderType* pSource, Demo_WorkOrderType** ppCopy);

CPP_DEMO_EXPORT OpcUa_StatusCode Demo_WorkOrderType_CopyTo(const Demo_WorkOrderType* pSource, Demo_WorkOrderType* pDestination);
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

extern struct _OpcUa_EncodeableType Demo_WorkOrderType_EncodeableType;
#endif /*OPCUA_EXCLUDE_Demo_WorkOrderType*/


/*============================================================================
 * Table of known types.
 *===========================================================================*/
extern struct _OpcUa_EncodeableType** Demo_KnownEncodeableTypes;

/*============================================================================
 * Register known types.
 *===========================================================================*/
namespace Demo
{
    /** Class used to handle data type registration
    */
    class DataTypes
    {
    public:
        static void registerStructuredTypes();
    private:
        static bool s_dataTypesAdded;
    };
}

#endif // __DEMO_DATATYPES_H__


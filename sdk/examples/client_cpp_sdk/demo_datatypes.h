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
}

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


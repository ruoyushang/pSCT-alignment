/******************************************************************************
** demo_datatypes.cpp
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

#include "demo_datatypes.h"

/* base */
#include <opcua_platformdefs.h>
#include <opcua_statuscodes.h>
#include <opcua_errorhandling.h>

/* core */
#include <opcua_memory.h>
#include <opcua_string.h>
#include <opcua_guid.h>

/* stack */
#include <opcua_encoder.h>
#include <opcua_decoder.h>


/* types */
#include <opcua_identifiers.h>
#include "demo_identifiers.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

#ifndef OPCUA_EXCLUDE_HeaterStatus
/*============================================================================
 * OpcUa_IdType_EnumeratedType
 *===========================================================================*/
static struct ::_OpcUa_EnumeratedValue g_HeaterStatus_EnumeratedValues[] =
{
    {(char*) "HeaterStatus_Off", 0},
    {(char*) "HeaterStatus_Heating", 1},
    {(char*) "HeaterStatus_Cooling", 2},
    { OpcUa_Null, 0 }
};

struct ::_OpcUa_EnumeratedType HeaterStatus_EnumeratedType =
{
    (char*) "HeaterStatus",
    g_HeaterStatus_EnumeratedValues
};
#endif /*OPCUA_EXCLUDE_HeaterStatus*/
}

#ifndef OPCUA_EXCLUDE_Demo_UnionTest
/*============================================================================
 * Demo_UnionTest_Initialize
 *===========================================================================*/
OpcUa_Void Demo_UnionTest_Initialize(Demo_UnionTest* a_pValue)
{
    if (a_pValue != OpcUa_Null)
    {
        a_pValue->Type = Demo_UnionTest_Null;
        OpcUa_MemSet(&a_pValue->Value, 0, sizeof(a_pValue->Value));
    }
}

/*============================================================================
 * Demo_UnionTest_Clear
 *===========================================================================*/
OpcUa_Void Demo_UnionTest_Clear(Demo_UnionTest* a_pValue)
{
    if (a_pValue != OpcUa_Null)
    {
        switch (a_pValue->Type)
        {
        case Demo_UnionTest_Null:
            break;
        case Demo_UnionTest_Int32:
            OpcUa_UnionField_Clear(Int32, Int32);
            break;
        case Demo_UnionTest_String:
            OpcUa_UnionField_Clear(String, String);
            break;
        default:
            /* error */
            break;
        }
    }
}

/*============================================================================
 * Demo_UnionTest_GetSize
 *===========================================================================*/
OpcUa_StatusCode Demo_UnionTest_GetSize(Demo_UnionTest* a_pValue, struct _OpcUa_Encoder* a_pEncoder, OpcUa_Int32* a_pSize)
{
    OpcUa_Int32 iSize = 0;

OpcUa_InitializeStatus(OpcUa_Module_Serializer, (char*) "Demo_UnionTest_GetSize");

    OpcUa_ReturnErrorIfArgumentNull(a_pValue);
    OpcUa_ReturnErrorIfArgumentNull(a_pEncoder);
    OpcUa_ReturnErrorIfArgumentNull(a_pSize);

    *a_pSize = -1;

    if (a_pEncoder->EncoderType == OpcUa_EncoderType_Binary)
    {
        uStatus = a_pEncoder->WriteUInt32(a_pEncoder, "Type", (OpcUa_UInt32*) &a_pValue->Type, &iSize);
        OpcUa_GotoErrorIfBad(uStatus);
    }
    switch (a_pValue->Type)
    {
    case Demo_UnionTest_Null:
        break;
    case Demo_UnionTest_Int32:
        OpcUa_UnionField_GetSize(Int32, Int32);
        break;
    case Demo_UnionTest_String:
        OpcUa_UnionField_GetSize(String, String);
        break;
    default:
        break;
    }

    *a_pSize = iSize;

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    *a_pSize = -1;

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Demo_UnionTest_Encode
 *===========================================================================*/
OpcUa_StatusCode Demo_UnionTest_Encode(Demo_UnionTest* a_pValue, struct _OpcUa_Encoder* a_pEncoder)
{
OpcUa_InitializeStatus(OpcUa_Module_Serializer, (char*) "Demo_UnionTest_Encode");

    OpcUa_ReturnErrorIfArgumentNull(a_pValue);
    OpcUa_ReturnErrorIfArgumentNull(a_pEncoder);

    uStatus = a_pEncoder->WriteUInt32(a_pEncoder, "SwitchField", (OpcUa_UInt32*) &a_pValue->Type, OpcUa_Null);
    OpcUa_GotoErrorIfBad(uStatus);

    switch (a_pValue->Type)
    {
    case Demo_UnionTest_Null:
        break;
    case Demo_UnionTest_Int32:
        OpcUa_UnionField_Write(Int32, Int32);
        break;
    case Demo_UnionTest_String:
        OpcUa_UnionField_Write(String, String);
        break;
    default:
        uStatus = OpcUa_BadEncodingError;
        break;
    }

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    /* nothing to do */

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Demo_UnionTest_Decode
 *===========================================================================*/
OpcUa_StatusCode Demo_UnionTest_Decode(Demo_UnionTest* a_pValue, struct _OpcUa_Decoder* a_pDecoder)
{
    OpcUa_UInt32 type = 0;
OpcUa_InitializeStatus(OpcUa_Module_Serializer, (char*) "Demo_UnionTest_Decode");

    OpcUa_ReturnErrorIfArgumentNull(a_pValue);
    OpcUa_ReturnErrorIfArgumentNull(a_pDecoder);

    Demo_UnionTest_Initialize(a_pValue);

    uStatus = a_pDecoder->ReadUInt32(a_pDecoder, "SwitchField", &type);
    OpcUa_GotoErrorIfBad(uStatus);
    if (type > 2)
    {
        return OpcUa_BadDecodingError;
    }
    a_pValue->Type = (Demo_UnionTest_Type) type;
    switch (a_pValue->Type)
    {
    case Demo_UnionTest_Null:
         break;
    case Demo_UnionTest_Int32:
        OpcUa_UnionField_Read(Int32, Int32);
        break;
    case Demo_UnionTest_String:
        OpcUa_UnionField_Read(String, String);
        break;
    default:
        break;
    }


OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    Demo_UnionTest_Clear(a_pValue);

OpcUa_FinishErrorHandling;
}

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
/*============================================================================
 * Demo_UnionTest_Compare
 *===========================================================================*/
OpcUa_Int Demo_UnionTest_Compare(const Demo_UnionTest* a_pValue1, const Demo_UnionTest* a_pValue2)
{
    if (a_pValue1 == a_pValue2)
    {
        return 0;
    }
    if (a_pValue1 == OpcUa_Null)
    {
        return -1;
    }
    if (a_pValue2 == OpcUa_Null)
    {
        return 1;
    }

    if (a_pValue1->Type != a_pValue2->Type)
    {
        return 1;
    }
    switch (a_pValue1->Type)
    {
    case Demo_UnionTest_Null:
        break;
    case Demo_UnionTest_Int32:
        OpcUa_UnionField_Compare(Int32, Int32);
        break;
    case Demo_UnionTest_String:
        OpcUa_UnionField_Compare(String, String);
        break;
    default:
        break;
    }

    return 0;
}
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
/*============================================================================
 * Demo_UnionTest_Copy
 *===========================================================================*/
OpcUa_StatusCode Demo_UnionTest_Copy(const Demo_UnionTest* a_pSource, Demo_UnionTest** a_ppCopy)
{
OpcUa_InitializeStatus(OpcUa_Module_ProxyStub, (char*) "Demo_UnionTest_Copy");

    OpcUa_ReturnErrorIfArgumentNull(a_pSource);
    OpcUa_ReturnErrorIfArgumentNull(a_ppCopy);

    *a_ppCopy = (Demo_UnionTest*)OpcUa_Alloc(sizeof( Demo_UnionTest ));
    OpcUa_GotoErrorIfAllocFailed((*a_ppCopy));

    uStatus = Demo_UnionTest_CopyTo(a_pSource, *a_ppCopy);
    OpcUa_GotoErrorIfBad(uStatus);

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    if (*a_ppCopy != OpcUa_Null)
    {
        OpcUa_Free(*a_ppCopy);
        *a_ppCopy = OpcUa_Null;
    }

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Demo_UnionTest_CopyTo
 *===========================================================================*/
OpcUa_StatusCode Demo_UnionTest_CopyTo(const Demo_UnionTest* a_pSource, Demo_UnionTest* a_pDestination)
{
OpcUa_InitializeStatus(OpcUa_Module_ProxyStub, (char*) "Demo_UnionTest_CopyTo");

    OpcUa_ReturnErrorIfArgumentNull(a_pSource);
    OpcUa_ReturnErrorIfArgumentNull(a_pDestination);

    Demo_UnionTest_Initialize(a_pDestination);

    a_pDestination->Type = a_pSource->Type;
    switch (a_pSource->Type)
    {
    case Demo_UnionTest_Null:
        break;
    case Demo_UnionTest_Int32:
        OpcUa_UnionField_CopyToScalar(Int32, Int32);
        break;
    case Demo_UnionTest_String:
        OpcUa_UnionField_CopyTo(String, String);
        break;
    default:
        break;
    }

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    Demo_UnionTest_Clear(a_pDestination);

OpcUa_FinishErrorHandling;
}
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

/*============================================================================
 * Demo_UnionTest_EncodeableType
 *===========================================================================*/
struct _OpcUa_EncodeableType Demo_UnionTest_EncodeableType =
{
#if !OPCUA_ENCODEABLE_OBJECT_OMIT_TYPE_NAME
    (char*) "Demo_UnionTest",
#endif /* !OPCUA_ENCODEABLE_OBJECT_OMIT_TYPE_NAME */
    DemoId_UnionTest,
    DemoId_UnionTest_Encoding_DefaultBinary,
    DemoId_UnionTest_Encoding_DefaultXml,
    Demo_NameSpaceUri,
    sizeof(Demo_UnionTest),
    (OpcUa_EncodeableObject_PfnInitialize*)Demo_UnionTest_Initialize,
    (OpcUa_EncodeableObject_PfnClear*)Demo_UnionTest_Clear,
    (OpcUa_EncodeableObject_PfnGetSize*)Demo_UnionTest_GetSize,
    (OpcUa_EncodeableObject_PfnEncode*)Demo_UnionTest_Encode,
    (OpcUa_EncodeableObject_PfnDecode*)Demo_UnionTest_Decode
#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
    ,(OpcUa_EncodeableObject_PfnCompare*)Demo_UnionTest_Compare
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */
#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
    ,(OpcUa_EncodeableObject_PfnCopy*)Demo_UnionTest_Copy,
    (OpcUa_EncodeableObject_PfnCopyTo*)Demo_UnionTest_CopyTo
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */
};
#endif /*OPCUA_EXCLUDE_Demo_UnionTest*/
#ifndef OPCUA_EXCLUDE_Demo_Vector
/*============================================================================
 * Demo_Vector_Initialize
 *===========================================================================*/
OpcUa_Void Demo_Vector_Initialize(Demo_Vector* a_pValue)
{
    if (a_pValue != OpcUa_Null)
    {
        OpcUa_Field_Initialize(Double, X);
        OpcUa_Field_Initialize(Double, Y);
        OpcUa_Field_Initialize(Double, Z);
    }
}

/*============================================================================
 * Demo_Vector_Clear
 *===========================================================================*/
OpcUa_Void Demo_Vector_Clear(Demo_Vector* a_pValue)
{
    if (a_pValue != OpcUa_Null)
    {
        OpcUa_Field_Clear(Double, X);
        OpcUa_Field_Clear(Double, Y);
        OpcUa_Field_Clear(Double, Z);
    }
}

/*============================================================================
 * Demo_Vector_GetSize
 *===========================================================================*/
OpcUa_StatusCode Demo_Vector_GetSize(Demo_Vector* a_pValue, struct _OpcUa_Encoder* a_pEncoder, OpcUa_Int32* a_pSize)
{
    OpcUa_Int32 iSize = 0;

OpcUa_InitializeStatus(OpcUa_Module_Serializer, (char*) "Demo_Vector_GetSize");

    OpcUa_ReturnErrorIfArgumentNull(a_pValue);
    OpcUa_ReturnErrorIfArgumentNull(a_pEncoder);
    OpcUa_ReturnErrorIfArgumentNull(a_pSize);

    *a_pSize = -1;

    OpcUa_Field_GetSize(Double, X);
    OpcUa_Field_GetSize(Double, Y);
    OpcUa_Field_GetSize(Double, Z);

    *a_pSize = iSize;

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    *a_pSize = -1;

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Demo_Vector_Encode
 *===========================================================================*/
OpcUa_StatusCode Demo_Vector_Encode(Demo_Vector* a_pValue, struct _OpcUa_Encoder* a_pEncoder)
{
OpcUa_InitializeStatus(OpcUa_Module_Serializer, (char*) "Demo_Vector_Encode");

    OpcUa_ReturnErrorIfArgumentNull(a_pValue);
    OpcUa_ReturnErrorIfArgumentNull(a_pEncoder);

    OpcUa_Field_Write(Double, X);
    OpcUa_Field_Write(Double, Y);
    OpcUa_Field_Write(Double, Z);

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    /* nothing to do */

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Demo_Vector_Decode
 *===========================================================================*/
OpcUa_StatusCode Demo_Vector_Decode(Demo_Vector* a_pValue, struct _OpcUa_Decoder* a_pDecoder)
{
OpcUa_InitializeStatus(OpcUa_Module_Serializer, (char*) "Demo_Vector_Decode");

    OpcUa_ReturnErrorIfArgumentNull(a_pValue);
    OpcUa_ReturnErrorIfArgumentNull(a_pDecoder);

    Demo_Vector_Initialize(a_pValue);

    OpcUa_Field_Read(Double, X);
    OpcUa_Field_Read(Double, Y);
    OpcUa_Field_Read(Double, Z);

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    Demo_Vector_Clear(a_pValue);

OpcUa_FinishErrorHandling;
}

#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
/*============================================================================
 * Demo_Vector_Compare
 *===========================================================================*/
OpcUa_Int Demo_Vector_Compare(const Demo_Vector* a_pValue1, const Demo_Vector* a_pValue2)
{
    if (a_pValue1 == a_pValue2)
    {
        return 0;
    }
    if (a_pValue1 == OpcUa_Null)
    {
        return -1;
    }
    if (a_pValue2 == OpcUa_Null)
    {
        return 1;
    }

    OpcUa_Field_Compare(Double, X);
    OpcUa_Field_Compare(Double, Y);
    OpcUa_Field_Compare(Double, Z);

    return 0;
}
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */

#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
/*============================================================================
 * Demo_Vector_Copy
 *===========================================================================*/
OpcUa_StatusCode Demo_Vector_Copy(const Demo_Vector* a_pSource, Demo_Vector** a_ppCopy)
{
OpcUa_InitializeStatus(OpcUa_Module_ProxyStub, (char*) "Demo_Vector_Copy");

    OpcUa_ReturnErrorIfArgumentNull(a_pSource);
    OpcUa_ReturnErrorIfArgumentNull(a_ppCopy);

    *a_ppCopy = (Demo_Vector*)OpcUa_Alloc(sizeof( Demo_Vector ));
    OpcUa_GotoErrorIfAllocFailed((*a_ppCopy));

    uStatus = Demo_Vector_CopyTo(a_pSource, *a_ppCopy);
    OpcUa_GotoErrorIfBad(uStatus);

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    if (*a_ppCopy != OpcUa_Null)
    {
        OpcUa_Free(*a_ppCopy);
        *a_ppCopy = OpcUa_Null;
    }

OpcUa_FinishErrorHandling;
}

/*============================================================================
 * Demo_Vector_CopyTo
 *===========================================================================*/
OpcUa_StatusCode Demo_Vector_CopyTo(const Demo_Vector* a_pSource, Demo_Vector* a_pDestination)
{
OpcUa_InitializeStatus(OpcUa_Module_ProxyStub, (char*) "Demo_Vector_CopyTo");

    OpcUa_ReturnErrorIfArgumentNull(a_pSource);
    OpcUa_ReturnErrorIfArgumentNull(a_pDestination);

    Demo_Vector_Initialize(a_pDestination);

    OpcUa_Field_CopyToScalar(Double, X);
    OpcUa_Field_CopyToScalar(Double, Y);
    OpcUa_Field_CopyToScalar(Double, Z);

OpcUa_ReturnStatusCode;
OpcUa_BeginErrorHandling;

    Demo_Vector_Clear(a_pDestination);

OpcUa_FinishErrorHandling;
}
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */

/*============================================================================
 * Demo_Vector_EncodeableType
 *===========================================================================*/
struct _OpcUa_EncodeableType Demo_Vector_EncodeableType =
{
#if !OPCUA_ENCODEABLE_OBJECT_OMIT_TYPE_NAME
    (char*) "Demo_Vector",
#endif /* !OPCUA_ENCODEABLE_OBJECT_OMIT_TYPE_NAME */
    DemoId_Vector,
    DemoId_Vector_Encoding_DefaultBinary,
    DemoId_Vector_Encoding_DefaultXml,
    Demo_NameSpaceUri,
    sizeof(Demo_Vector),
    (OpcUa_EncodeableObject_PfnInitialize*)Demo_Vector_Initialize,
    (OpcUa_EncodeableObject_PfnClear*)Demo_Vector_Clear,
    (OpcUa_EncodeableObject_PfnGetSize*)Demo_Vector_GetSize,
    (OpcUa_EncodeableObject_PfnEncode*)Demo_Vector_Encode,
    (OpcUa_EncodeableObject_PfnDecode*)Demo_Vector_Decode
#if OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED
    ,(OpcUa_EncodeableObject_PfnCompare*)Demo_Vector_Compare
#endif /* OPCUA_ENCODEABLE_OBJECT_COMPARE_SUPPORTED */
#if OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED
    ,(OpcUa_EncodeableObject_PfnCopy*)Demo_Vector_Copy,
    (OpcUa_EncodeableObject_PfnCopyTo*)Demo_Vector_CopyTo
#endif /* OPCUA_ENCODEABLE_OBJECT_COPY_SUPPORTED */
};
#endif /*OPCUA_EXCLUDE_Demo_Vector*/

/*============================================================================
 * Table of known types.
 *===========================================================================*/
static OpcUa_EncodeableType* g_Demo_KnownEncodeableTypes[] =
{
#ifndef OPCUA_EXCLUDE_Demo_UnionTest
    &Demo_UnionTest_EncodeableType,
#endif /*OPCUA_EXCLUDE_Demo_UnionTest*/
#ifndef OPCUA_EXCLUDE_Demo_Vector
    &Demo_Vector_EncodeableType,
#endif /*OPCUA_EXCLUDE_Demo_Vector*/
    OpcUa_Null
};
OpcUa_EncodeableType** Demo_KnownEncodeableTypes = g_Demo_KnownEncodeableTypes;

namespace Demo
{

bool DataTypes::s_dataTypesAdded = false;

/** Static method used to register structure data types with the OPC UA stack
 *
 *  This method can be called multiple times and makes sure the data type is registered only once.
 */
void DataTypes::registerStructuredTypes()
{
    if (!s_dataTypesAdded)
    {
        OpcUa_ProxyStub_AddTypes(Demo_KnownEncodeableTypes);
        s_dataTypesAdded = true;
    }
}

}


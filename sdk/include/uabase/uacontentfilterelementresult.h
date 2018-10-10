/******************************************************************************
** uacontentfilterelementresult.h
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
** Project: C++ OPC SDK base module
**
** Portable UaContentFilterElementResult class.
**
******************************************************************************/
#ifndef UACONTENTFILTERELEMENTRESULT_H
#define UACONTENTFILTERELEMENTRESULT_H

#include <opcua_proxystub.h>

#include "uabase.h"
#include "statuscode.h"
#include "uaarraytemplates.h"

class UaExtensionObject;
class UaVariant;
class UaDataValue;

class UABASE_EXPORT UaContentFilterElementResultPrivate;

/** @ingroup CppBaseLibraryClass
 *  @brief Wrapper class for the UA stack structure OpcUa_ContentFilterElementResult.
 *
 *  This class encapsulates the native OpcUa_ContentFilterElementResult structure
 *  and handles memory allocation and cleanup for you.
 *  UaContentFilterElementResult uses implicit sharing to avoid needless copying and to boost the performance.
 *  Only if you modify a shared ContentFilterElementResult it creates a copy for that (copy-on-write).
 *  So assigning another UaContentFilterElementResult or passing it as parameter needs constant time and is nearly as fast as assigning a pointer.
 */
class UABASE_EXPORT UaContentFilterElementResult
{
    UA_DECLARE_PRIVATE(UaContentFilterElementResult)
public:
    UaContentFilterElementResult();
    UaContentFilterElementResult(const UaContentFilterElementResult &other);
    UaContentFilterElementResult(const OpcUa_ContentFilterElementResult &other);
    UaContentFilterElementResult(
        OpcUa_StatusCode statusCode,
        const UaStatusCodeArray &operandStatusCodes,
        const UaDiagnosticInfos &operandDiagnosticInfos
        );
    UaContentFilterElementResult(const UaExtensionObject &extensionObject);
    UaContentFilterElementResult(const OpcUa_ExtensionObject &extensionObject);
    UaContentFilterElementResult(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    UaContentFilterElementResult(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    ~UaContentFilterElementResult();

    void clear();

    bool operator==(const UaContentFilterElementResult &other) const;
    bool operator!=(const UaContentFilterElementResult &other) const;

    UaContentFilterElementResult& operator=(const UaContentFilterElementResult &other);

    OpcUa_ContentFilterElementResult* copy() const;
    void copyTo(OpcUa_ContentFilterElementResult *pDst) const;

    static OpcUa_ContentFilterElementResult* clone(const OpcUa_ContentFilterElementResult& source);
    static void cloneTo(const OpcUa_ContentFilterElementResult& source, OpcUa_ContentFilterElementResult& copy);

    void attach(OpcUa_ContentFilterElementResult *pValue);
    OpcUa_ContentFilterElementResult* detach(OpcUa_ContentFilterElementResult* pDst);

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

    OpcUa_StatusCode setContentFilterElementResult(const UaExtensionObject &extensionObject);
    OpcUa_StatusCode setContentFilterElementResult(const OpcUa_ExtensionObject &extensionObject);
    OpcUa_StatusCode setContentFilterElementResult(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setContentFilterElementResult(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    void setContentFilterElementResult(
        OpcUa_StatusCode statusCode,
        const UaStatusCodeArray &operandStatusCodes,
        const UaDiagnosticInfos &operandDiagnosticInfos
        );

    OpcUa_StatusCode getStatusCode() const;
    void getOperandStatusCodes(UaStatusCodeArray& operandStatusCodes) const;
    void getOperandDiagnosticInfos(UaDiagnosticInfos& operandDiagnosticInfos) const;

    void setStatusCode(OpcUa_StatusCode statusCode);
    void setOperandStatusCodes(const UaStatusCodeArray &operandStatusCodes);
    void setOperandDiagnosticInfos(const UaDiagnosticInfos &operandDiagnosticInfos);
};

class UABASE_EXPORT UaContentFilterElementResults
{
public:
    UaContentFilterElementResults();
    UaContentFilterElementResults(const UaContentFilterElementResults &other);
    UaContentFilterElementResults(OpcUa_Int32 length, OpcUa_ContentFilterElementResult* data);
    virtual ~UaContentFilterElementResults();

    UaContentFilterElementResults& operator=(const UaContentFilterElementResults &other);
    const OpcUa_ContentFilterElementResult& operator[](OpcUa_UInt32 index) const;
    OpcUa_ContentFilterElementResult& operator[](OpcUa_UInt32 index);

    bool operator==(const UaContentFilterElementResults &other) const;
    bool operator!=(const UaContentFilterElementResults &other) const;

    void attach(OpcUa_UInt32 length, OpcUa_ContentFilterElementResult* data);
    void attach(OpcUa_Int32 length, OpcUa_ContentFilterElementResult* data);
    OpcUa_ContentFilterElementResult* detach();

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const OpcUa_ContentFilterElementResult* rawData() const {return m_data;}
    inline OpcUa_ContentFilterElementResult* rawData() {return m_data;}

    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setContentFilterElementResults(const UaVariant &variant);
    OpcUa_StatusCode setContentFilterElementResults(const OpcUa_Variant &variant);
    OpcUa_StatusCode setContentFilterElementResults(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setContentFilterElementResults(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setContentFilterElementResults(OpcUa_Int32 length, OpcUa_ContentFilterElementResult* data);

private:
    OpcUa_UInt32 m_noOfElements;
    OpcUa_ContentFilterElementResult* m_data;
};

#endif // UACONTENTFILTERELEMENTRESULT_H


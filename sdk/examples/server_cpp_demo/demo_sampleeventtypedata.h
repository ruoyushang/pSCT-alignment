/******************************************************************************
** demo_sampleeventtypedata.h
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

#ifndef __DEMO_SAMPLEEVENTTYPE_H__
#define __DEMO_SAMPLEEVENTTYPE_H__

#include "uaeventdata.h"
#include "demo_datatypes.h"
#include "basenodes.h"
#include "demo_identifiers.h"
#include "demo_instancefactory_demo.h"
#include "opcua_propertytype.h"
#include "demo_datatypes.h"

// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

#if SUPPORT_Event_Subscription_Server_Facet

/** Event data class for the OPC UA event type SampleEventType.
 *
 *  This class provides the functionality necessary to create an event of type SampleEventType, to set the values
 *  of the event fields and to send the event to interested clients through EventManagerBase::fireEvent().
 *
 *  **Event Fields of the SampleEventType:**
 *
 *  Browse Name    | DataType       | TypeDefinition | Modelling Rule | See Also
 *  ---------------|----------------|----------------|----------------|-----------------------------------------------
 *  Boolean        | Boolean        | PropertyType   | Mandatory      | \ref getBoolean, \ref setBoolean
 *  Byte           | Byte           | PropertyType   | Mandatory      | \ref getByte, \ref setByte
 *  ByteString     | ByteString     | PropertyType   | Mandatory      | \ref getByteString, \ref setByteString
 *  DateTime       | DateTime       | PropertyType   | Mandatory      | \ref getDateTime, \ref setDateTime
 *  Double         | Double         | PropertyType   | Mandatory      | \ref getDouble, \ref setDouble
 *  ExpandedNodeId | ExpandedNodeId | PropertyType   | Mandatory      | \ref getExpandedNodeId, \ref setExpandedNodeId
 *  Float          | Float          | PropertyType   | Mandatory      | \ref getFloat, \ref setFloat
 *  Guid           | Guid           | PropertyType   | Mandatory      | \ref getGuid, \ref setGuid
 *  Int16          | Int16          | PropertyType   | Mandatory      | \ref getInt16, \ref setInt16
 *  Int32          | Int32          | PropertyType   | Mandatory      | \ref getInt32, \ref setInt32
 *  Int64          | Int64          | PropertyType   | Mandatory      | \ref getInt64, \ref setInt64
 *  LocalizedText  | LocalizedText  | PropertyType   | Mandatory      | \ref getLocalizedText, \ref setLocalizedText
 *  NodeId         | NodeId         | PropertyType   | Mandatory      | \ref getNodeId, \ref setNodeId
 *  QualifiedName  | QualifiedName  | PropertyType   | Mandatory      | \ref getQualifiedName, \ref setQualifiedName
 *  SByte          | SByte          | PropertyType   | Mandatory      | \ref getSByte, \ref setSByte
 *  StatusCode     | StatusCode     | PropertyType   | Mandatory      | \ref getStatusCode, \ref setStatusCode
 *  String         | String         | PropertyType   | Mandatory      | \ref getString, \ref setString
 *  UInt16         | UInt16         | PropertyType   | Mandatory      | \ref getUInt16, \ref setUInt16
 *  UInt32         | UInt32         | PropertyType   | Mandatory      | \ref getUInt32, \ref setUInt32
 *  UInt64         | UInt64         | PropertyType   | Mandatory      | \ref getUInt64, \ref setUInt64
 *  XmlElement     | XmlElement     | PropertyType   | Mandatory      | \ref getXmlElement, \ref setXmlElement

 */
class CPP_DEMO_EXPORT SampleEventTypeData:
    public BaseEventTypeData
{
    UA_DISABLE_COPY(SampleEventTypeData);
public:
    SampleEventTypeData();
    virtual ~SampleEventTypeData();

    static void createTypes();
    static void clearStaticMembers();
    static void registerEventFields();
    virtual void getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data);
    // Boolean
    void setBoolean(OpcUa_Boolean Boolean);
    void setBooleanStatus(OpcUa_StatusCode status);
    OpcUa_Boolean getBoolean();
    virtual void getBooleanValue(Session* pSession, OpcUa_Variant& value);
    // Byte
    void setByte(OpcUa_Byte Byte);
    void setByteStatus(OpcUa_StatusCode status);
    OpcUa_Byte getByte();
    virtual void getByteValue(Session* pSession, OpcUa_Variant& value);
    // ByteString
    void setByteString(UaByteString& ByteString);
    void setByteStringStatus(OpcUa_StatusCode status);
    UaByteString getByteString();
    virtual void getByteStringValue(Session* pSession, OpcUa_Variant& value);
    // DateTime
    void setDateTime(const UaDateTime& DateTime);
    void setDateTimeStatus(OpcUa_StatusCode status);
    UaDateTime getDateTime();
    virtual void getDateTimeValue(Session* pSession, OpcUa_Variant& value);
    // Double
    void setDouble(OpcUa_Double Double);
    void setDoubleStatus(OpcUa_StatusCode status);
    OpcUa_Double getDouble();
    virtual void getDoubleValue(Session* pSession, OpcUa_Variant& value);
    // ExpandedNodeId
    void setExpandedNodeId(const UaExpandedNodeId& ExpandedNodeId);
    void setExpandedNodeIdStatus(OpcUa_StatusCode status);
    UaExpandedNodeId getExpandedNodeId();
    virtual void getExpandedNodeIdValue(Session* pSession, OpcUa_Variant& value);
    // Float
    void setFloat(OpcUa_Float Float);
    void setFloatStatus(OpcUa_StatusCode status);
    OpcUa_Float getFloat();
    virtual void getFloatValue(Session* pSession, OpcUa_Variant& value);
    // Guid
    void setGuid(const UaGuid& Guid);
    void setGuidStatus(OpcUa_StatusCode status);
    UaGuid getGuid();
    virtual void getGuidValue(Session* pSession, OpcUa_Variant& value);
    // Int16
    void setInt16(OpcUa_Int16 Int16);
    void setInt16Status(OpcUa_StatusCode status);
    OpcUa_Int16 getInt16();
    virtual void getInt16Value(Session* pSession, OpcUa_Variant& value);
    // Int32
    void setInt32(OpcUa_Int32 Int32);
    void setInt32Status(OpcUa_StatusCode status);
    OpcUa_Int32 getInt32();
    virtual void getInt32Value(Session* pSession, OpcUa_Variant& value);
    // Int64
    void setInt64(OpcUa_Int64 Int64);
    void setInt64Status(OpcUa_StatusCode status);
    OpcUa_Int64 getInt64();
    virtual void getInt64Value(Session* pSession, OpcUa_Variant& value);
    // LocalizedText
    void setLocalizedText(const UaLocalizedText& LocalizedText);
    void setLocalizedTextStatus(OpcUa_StatusCode status);
    UaLocalizedText getLocalizedText();
    virtual void getLocalizedTextValue(Session* pSession, OpcUa_Variant& value);
    // NodeId
    void setNodeId(const UaNodeId& NodeId);
    void setNodeIdStatus(OpcUa_StatusCode status);
    UaNodeId getNodeId();
    virtual void getNodeIdValue(Session* pSession, OpcUa_Variant& value);
    // QualifiedName
    void setQualifiedName(const UaQualifiedName& QualifiedName);
    void setQualifiedNameStatus(OpcUa_StatusCode status);
    UaQualifiedName getQualifiedName();
    virtual void getQualifiedNameValue(Session* pSession, OpcUa_Variant& value);
    // SByte
    void setSByte(OpcUa_SByte SByte);
    void setSByteStatus(OpcUa_StatusCode status);
    OpcUa_SByte getSByte();
    virtual void getSByteValue(Session* pSession, OpcUa_Variant& value);
    // StatusCode
    void setStatusCode(OpcUa_StatusCode StatusCode);
    void setStatusCodeStatus(OpcUa_StatusCode status);
    OpcUa_StatusCode getStatusCode();
    virtual void getStatusCodeValue(Session* pSession, OpcUa_Variant& value);
    // String
    void setString(const UaString& String);
    void setStringStatus(OpcUa_StatusCode status);
    UaString getString();
    virtual void getStringValue(Session* pSession, OpcUa_Variant& value);
    // UInt16
    void setUInt16(OpcUa_UInt16 UInt16);
    void setUInt16Status(OpcUa_StatusCode status);
    OpcUa_UInt16 getUInt16();
    virtual void getUInt16Value(Session* pSession, OpcUa_Variant& value);
    // UInt32
    void setUInt32(OpcUa_UInt32 UInt32);
    void setUInt32Status(OpcUa_StatusCode status);
    OpcUa_UInt32 getUInt32();
    virtual void getUInt32Value(Session* pSession, OpcUa_Variant& value);
    // UInt64
    void setUInt64(OpcUa_UInt64 UInt64);
    void setUInt64Status(OpcUa_StatusCode status);
    OpcUa_UInt64 getUInt64();
    virtual void getUInt64Value(Session* pSession, OpcUa_Variant& value);
    // XmlElement
    void setXmlElement(UaByteString& XmlElement);
    void setXmlElementStatus(OpcUa_StatusCode status);
    UaByteString getXmlElement();
    virtual void getXmlElementValue(Session* pSession, OpcUa_Variant& value);

private:
    static bool                                 s_typeNodesCreated;
    static std::map<OpcUa_UInt32, OpcUa_UInt32> s_SampleEventTypeDataFields;
    UaObjectPointerArray<UaVariant>             m_FieldValues;
};

#endif // SUPPORT_Event_Subscription_Server_Facet

} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

#endif // #ifndef __DEMOSAMPLEEVENTTYPE_H__

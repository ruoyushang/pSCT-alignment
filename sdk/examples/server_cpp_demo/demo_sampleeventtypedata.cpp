/******************************************************************************
** demo_sampleeventtypedata.cpp
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

#include "demo_sampleeventtypedata.h"
#include "uagenericnodes.h"
#include "nodemanagerroot.h"
#include "eventmanageruanode.h"
#include "nodemanagerroot.h"
#include "methodhandleuanode.h"
#include "demo_nodemanagerdemo.h"

#include "instancefactory.h"
// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/
namespace Demo {

#if SUPPORT_Event_Subscription_Server_Facet

bool SampleEventTypeData::s_typeNodesCreated = false;
std::map<OpcUa_UInt32, OpcUa_UInt32> SampleEventTypeData::s_SampleEventTypeDataFields;

/** Constructs an instance of the event data class SampleEventTypeData
*/
SampleEventTypeData::SampleEventTypeData()
: BaseEventTypeData()
{
    OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
    m_EventTypeId.setNodeId(DemoId_SampleEventType, nsTypeIdx);

    // Create a unique EventId sent to the client
    UaByteString clientEventId;
    EventManagerUaNode::buildEventId(UaByteString(), clientEventId);
    m_EventId.setByteString(clientEventId, OpcUa_True);

    m_FieldValues.create(21);

    if ( s_typeNodesCreated == false )
    {
        createTypes();
    }
}

/** Destroys the SampleEventTypeData object
*/
SampleEventTypeData::~SampleEventTypeData()
{
}

/** Create the type nodes for the event type SampleEventType
*/
void SampleEventTypeData::createTypes()
{
    if ( s_typeNodesCreated == false )
    {
        s_typeNodesCreated = true;

        registerEventFields();

        UaStatus      addStatus;
        UaVariant     defaultValue;
        NodeManagerRoot* pNodeManagerRoot = NodeManagerRoot::CreateRootNodeManager();
        OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
        OpcUa_Int16 nsSuperTypeIdx = NodeManagerRoot::getTypeNamespace();
        NodeManagerConfig* pTypeNodeConfig = pNodeManagerRoot->getNodeManagerByNamespace(nsTypeIdx)->getNodeManagerConfig();

        OpcUa::GenericObjectType* pObjectType;
        pObjectType = new OpcUa::GenericObjectType(
            UaNodeId(DemoId_SampleEventType, nsTypeIdx),
            UaQualifiedName("SampleEventType", nsTypeIdx),
            UaLocalizedText("", "SampleEventType"),
            UaLocalizedText("", "An event type that contains all datatypes as event fields."),
            OpcUa_False,
            &SampleEventTypeData::clearStaticMembers);
        addStatus = pTypeNodeConfig->addNodeAndReference(UaNodeId(OpcUaId_BaseEventType, nsSuperTypeIdx), pObjectType, OpcUaId_HasSubtype);
        UA_ASSERT(addStatus.isGood());


        // Mandatory variable Boolean
        OpcUa::PropertyType *pBoolean;
        defaultValue.setBool(OpcUa_False);
        pBoolean = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Boolean, nsTypeIdx), "Boolean", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pBoolean->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pBoolean->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pBoolean, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable Byte
        OpcUa::PropertyType *pByte;
        defaultValue.setByte(0);
        pByte = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Byte, nsTypeIdx), "Byte", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pByte->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pByte->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pByte, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable ByteString
        OpcUa::PropertyType *pByteString;
        { UaByteString tmpByteString; defaultValue.setByteString(tmpByteString, OpcUa_False); };
        pByteString = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_ByteString, nsTypeIdx), "ByteString", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pByteString->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pByteString->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pByteString, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable DateTime
        OpcUa::PropertyType *pDateTime;
        defaultValue.setDateTime(UaDateTime());
        pDateTime = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_DateTime, nsTypeIdx), "DateTime", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pDateTime->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pDateTime->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pDateTime, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable Double
        OpcUa::PropertyType *pDouble;
        defaultValue.setDouble(0.0);
        pDouble = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Double, nsTypeIdx), "Double", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pDouble->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pDouble->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pDouble, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable ExpandedNodeId
        OpcUa::PropertyType *pExpandedNodeId;
        defaultValue.setExpandedNodeId(UaExpandedNodeId());
        pExpandedNodeId = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_ExpandedNodeId, nsTypeIdx), "ExpandedNodeId", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pExpandedNodeId->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pExpandedNodeId->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pExpandedNodeId, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable Float
        OpcUa::PropertyType *pFloat;
        defaultValue.setFloat(0.0);
        pFloat = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Float, nsTypeIdx), "Float", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pFloat->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pFloat->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pFloat, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable Guid
        OpcUa::PropertyType *pGuid;
        defaultValue.setGuid(UaGuid());
        pGuid = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Guid, nsTypeIdx), "Guid", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pGuid->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pGuid->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pGuid, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable Int16
        OpcUa::PropertyType *pInt16;
        defaultValue.setInt16(0);
        pInt16 = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Int16, nsTypeIdx), "Int16", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pInt16->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pInt16->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pInt16, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable Int32
        OpcUa::PropertyType *pInt32;
        defaultValue.setInt32(0);
        pInt32 = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Int32, nsTypeIdx), "Int32", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pInt32->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pInt32->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pInt32, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable Int64
        OpcUa::PropertyType *pInt64;
        defaultValue.setInt64(0);
        pInt64 = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_Int64, nsTypeIdx), "Int64", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pInt64->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pInt64->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pInt64, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable LocalizedText
        OpcUa::PropertyType *pLocalizedText;
        defaultValue.setLocalizedText(UaLocalizedText());
        pLocalizedText = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_LocalizedText, nsTypeIdx), "LocalizedText", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pLocalizedText->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pLocalizedText->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pLocalizedText, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable NodeId
        OpcUa::PropertyType *pNodeId;
        defaultValue.setNodeId(UaNodeId());
        pNodeId = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_NodeId, nsTypeIdx), "NodeId", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pNodeId->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pNodeId->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pNodeId, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable QualifiedName
        OpcUa::PropertyType *pQualifiedName;
        defaultValue.setQualifiedName(UaQualifiedName());
        pQualifiedName = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_QualifiedName, nsTypeIdx), "QualifiedName", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pQualifiedName->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pQualifiedName->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pQualifiedName, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable SByte
        OpcUa::PropertyType *pSByte;
        defaultValue.setSByte(0);
        pSByte = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_SByte, nsTypeIdx), "SByte", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pSByte->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pSByte->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pSByte, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable StatusCode
        OpcUa::PropertyType *pStatusCode;
        defaultValue.setStatusCode(0);
        pStatusCode = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_StatusCode, nsTypeIdx), "StatusCode", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pStatusCode->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pStatusCode->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pStatusCode, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable String
        OpcUa::PropertyType *pString;
        defaultValue.setString("");
        pString = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_String, nsTypeIdx), "String", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pString->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pString->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pString, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable UInt16
        OpcUa::PropertyType *pUInt16;
        defaultValue.setUInt16(0);
        pUInt16 = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_UInt16, nsTypeIdx), "UInt16", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pUInt16->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pUInt16->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pUInt16, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable UInt32
        OpcUa::PropertyType *pUInt32;
        defaultValue.setUInt32(0);
        pUInt32 = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_UInt32, nsTypeIdx), "UInt32", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pUInt32->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pUInt32->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pUInt32, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable UInt64
        OpcUa::PropertyType *pUInt64;
        defaultValue.setUInt64(0);
        pUInt64 = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_UInt64, nsTypeIdx), "UInt64", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pUInt64->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pUInt64->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pUInt64, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

        // Mandatory variable XmlElement
        OpcUa::PropertyType *pXmlElement;
        { UaByteString tmpXmlElement; defaultValue.setXmlElement(tmpXmlElement, OpcUa_False); };
        pXmlElement = new OpcUa::PropertyType(UaNodeId(DemoId_SampleEventType_XmlElement, nsTypeIdx), "XmlElement", Demo::NodeManagerDemo::getTypeNamespace(), defaultValue, 3, pTypeNodeConfig);
        pXmlElement->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        pXmlElement->setValueRank(-1);
        addStatus = pTypeNodeConfig->addNodeAndReference(pObjectType, pXmlElement, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());

    }
}

/** Clear the static members of the class
*/
void SampleEventTypeData::clearStaticMembers()
{
    s_typeNodesCreated = false;

    s_SampleEventTypeDataFields.clear();
}

/** Registers all event type fields with the EventManagerBase.
 */
void SampleEventTypeData::registerEventFields()
{
    // Register event type
    OpcUa_Int16 nsSuperTypeIdx = NodeManagerRoot::getTypeNamespace();
    OpcUa_Int16 nsTypeIdx = NodeManagerDemo::getTypeNamespace();
    EventManagerBase::registerEventType(UaNodeId(OpcUaId_BaseEventType, nsSuperTypeIdx), UaNodeId(DemoId_SampleEventType, nsTypeIdx));
    // Register event fields
    s_SampleEventTypeDataFields.clear();
    /* Boolean*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Boolean", nsTypeIdx).toFullString())] = 1;
    /* Byte*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Byte", nsTypeIdx).toFullString())] = 2;
    /* ByteString*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("ByteString", nsTypeIdx).toFullString())] = 3;
    /* DateTime*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("DateTime", nsTypeIdx).toFullString())] = 4;
    /* Double*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Double", nsTypeIdx).toFullString())] = 5;
    /* ExpandedNodeId*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("ExpandedNodeId", nsTypeIdx).toFullString())] = 6;
    /* Float*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Float", nsTypeIdx).toFullString())] = 7;
    /* Guid*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Guid", nsTypeIdx).toFullString())] = 8;
    /* Int16*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Int16", nsTypeIdx).toFullString())] = 9;
    /* Int32*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Int32", nsTypeIdx).toFullString())] = 10;
    /* Int64*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("Int64", nsTypeIdx).toFullString())] = 11;
    /* LocalizedText*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("LocalizedText", nsTypeIdx).toFullString())] = 12;
    /* NodeId*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("NodeId", nsTypeIdx).toFullString())] = 13;
    /* QualifiedName*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("QualifiedName", nsTypeIdx).toFullString())] = 14;
    /* SByte*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("SByte", nsTypeIdx).toFullString())] = 15;
    /* StatusCode*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("StatusCode", nsTypeIdx).toFullString())] = 16;
    /* String*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("String", nsTypeIdx).toFullString())] = 17;
    /* UInt16*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("UInt16", nsTypeIdx).toFullString())] = 18;
    /* UInt32*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("UInt32", nsTypeIdx).toFullString())] = 19;
    /* UInt64*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("UInt64", nsTypeIdx).toFullString())] = 20;
    /* XmlElement*/
    s_SampleEventTypeDataFields[EventManagerBase::registerEventField(UaQualifiedName("XmlElement", nsTypeIdx).toFullString())] = 21;
}

/** Get the field value for the passed index.
 *  @param index The index of the selected field.
 *  @param pSession The session context for the method call.
 *  @param data The data for the selected field.
 */
void SampleEventTypeData::getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data)
{
    // Try to find the field index
    std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator it;
    it = SampleEventTypeData::s_SampleEventTypeDataFields.find(index);

    if ( it == SampleEventTypeData::s_SampleEventTypeDataFields.end()  )
    {
        BaseEventTypeData::getFieldData(index, pSession, data);
        return;
    }

    switch (it->second)
    {
        case 1: // Boolean
        {
            getBooleanValue(pSession, data);
            break;
        }
        case 2: // Byte
        {
            getByteValue(pSession, data);
            break;
        }
        case 3: // ByteString
        {
            getByteStringValue(pSession, data);
            break;
        }
        case 4: // DateTime
        {
            getDateTimeValue(pSession, data);
            break;
        }
        case 5: // Double
        {
            getDoubleValue(pSession, data);
            break;
        }
        case 6: // ExpandedNodeId
        {
            getExpandedNodeIdValue(pSession, data);
            break;
        }
        case 7: // Float
        {
            getFloatValue(pSession, data);
            break;
        }
        case 8: // Guid
        {
            getGuidValue(pSession, data);
            break;
        }
        case 9: // Int16
        {
            getInt16Value(pSession, data);
            break;
        }
        case 10: // Int32
        {
            getInt32Value(pSession, data);
            break;
        }
        case 11: // Int64
        {
            getInt64Value(pSession, data);
            break;
        }
        case 12: // LocalizedText
        {
            getLocalizedTextValue(pSession, data);
            break;
        }
        case 13: // NodeId
        {
            getNodeIdValue(pSession, data);
            break;
        }
        case 14: // QualifiedName
        {
            getQualifiedNameValue(pSession, data);
            break;
        }
        case 15: // SByte
        {
            getSByteValue(pSession, data);
            break;
        }
        case 16: // StatusCode
        {
            getStatusCodeValue(pSession, data);
            break;
        }
        case 17: // String
        {
            getStringValue(pSession, data);
            break;
        }
        case 18: // UInt16
        {
            getUInt16Value(pSession, data);
            break;
        }
        case 19: // UInt32
        {
            getUInt32Value(pSession, data);
            break;
        }
        case 20: // UInt64
        {
            getUInt64Value(pSession, data);
            break;
        }
        case 21: // XmlElement
        {
            getXmlElementValue(pSession, data);
            break;
        }
        default:
        {
            OpcUa_Variant_Clear(&data);
        }
    }
}

/** Set the event field value Boolean
 */
void SampleEventTypeData::setBoolean(OpcUa_Boolean Boolean)
{
    if ( m_FieldValues[ 0 ] == NULL )
    {
        m_FieldValues[ 0 ] = new UaVariant;
    }

    m_FieldValues[ 0 ]->setBool(Boolean);
}
/** Set a status code for the event field value Boolean
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setBooleanStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 0 ] == NULL )
    {
        m_FieldValues[ 0 ] = new UaVariant;
    }

    m_FieldValues[ 0 ]->setStatusCode(status);
}

/** Get the event field value Boolean
*/
OpcUa_Boolean SampleEventTypeData::getBoolean()
{
    if ( m_FieldValues[ 0 ] == NULL )
    {
        return OpcUa_False;
    }
    else
    {
        OpcUa_Boolean ret = OpcUa_False;
        m_FieldValues[ 0 ]->toBool(ret);
        return ret;
    }
}
/** Get the event field value Boolean for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getBooleanValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 0 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 0 ]->copyTo(&value);
    }
}
/** Set the event field value Byte
 */
void SampleEventTypeData::setByte(OpcUa_Byte Byte)
{
    if ( m_FieldValues[ 1 ] == NULL )
    {
        m_FieldValues[ 1 ] = new UaVariant;
    }

    m_FieldValues[ 1 ]->setByte(Byte);
}
/** Set a status code for the event field value Byte
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setByteStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 1 ] == NULL )
    {
        m_FieldValues[ 1 ] = new UaVariant;
    }

    m_FieldValues[ 1 ]->setStatusCode(status);
}

/** Get the event field value Byte
*/
OpcUa_Byte SampleEventTypeData::getByte()
{
    if ( m_FieldValues[ 1 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_Byte ret = 0;
        m_FieldValues[ 1 ]->toByte(ret);
        return ret;
    }
}
/** Get the event field value Byte for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getByteValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 1 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 1 ]->copyTo(&value);
    }
}
/** Set the event field value ByteString
 */
void SampleEventTypeData::setByteString(UaByteString& ByteString)
{
    if ( m_FieldValues[ 2 ] == NULL )
    {
        m_FieldValues[ 2 ] = new UaVariant;
    }

    m_FieldValues[ 2 ]->setByteString(ByteString, OpcUa_False);
}
/** Set a status code for the event field value ByteString
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setByteStringStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 2 ] == NULL )
    {
        m_FieldValues[ 2 ] = new UaVariant;
    }

    m_FieldValues[ 2 ]->setStatusCode(status);
}

/** Get the event field value ByteString
*/
UaByteString SampleEventTypeData::getByteString()
{
    if ( m_FieldValues[ 2 ] == NULL )
    {
        return UaByteString();
    }
    else
    {
        UaByteString ret;
        m_FieldValues[ 2 ]->toByteString(ret);
        return ret;
    }
}
/** Get the event field value ByteString for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getByteStringValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 2 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 2 ]->copyTo(&value);
    }
}
/** Set the event field value DateTime
 */
void SampleEventTypeData::setDateTime(const UaDateTime& DateTime)
{
    if ( m_FieldValues[ 3 ] == NULL )
    {
        m_FieldValues[ 3 ] = new UaVariant;
    }

    m_FieldValues[ 3 ]->setDateTime(DateTime);
}
/** Set a status code for the event field value DateTime
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setDateTimeStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 3 ] == NULL )
    {
        m_FieldValues[ 3 ] = new UaVariant;
    }

    m_FieldValues[ 3 ]->setStatusCode(status);
}

/** Get the event field value DateTime
*/
UaDateTime SampleEventTypeData::getDateTime()
{
    if ( m_FieldValues[ 3 ] == NULL )
    {
        return UaDateTime();
    }
    else
    {
        UaDateTime ret = UaDateTime();
        m_FieldValues[ 3 ]->toDateTime(ret);
        return ret;
    }
}
/** Get the event field value DateTime for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getDateTimeValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 3 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 3 ]->copyTo(&value);
    }
}
/** Set the event field value Double
 */
void SampleEventTypeData::setDouble(OpcUa_Double Double)
{
    if ( m_FieldValues[ 4 ] == NULL )
    {
        m_FieldValues[ 4 ] = new UaVariant;
    }

    m_FieldValues[ 4 ]->setDouble(Double);
}
/** Set a status code for the event field value Double
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setDoubleStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 4 ] == NULL )
    {
        m_FieldValues[ 4 ] = new UaVariant;
    }

    m_FieldValues[ 4 ]->setStatusCode(status);
}

/** Get the event field value Double
*/
OpcUa_Double SampleEventTypeData::getDouble()
{
    if ( m_FieldValues[ 4 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_Double ret = 0;
        m_FieldValues[ 4 ]->toDouble(ret);
        return ret;
    }
}
/** Get the event field value Double for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getDoubleValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 4 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 4 ]->copyTo(&value);
    }
}
/** Set the event field value ExpandedNodeId
 */
void SampleEventTypeData::setExpandedNodeId(const UaExpandedNodeId& ExpandedNodeId)
{
    if ( m_FieldValues[ 5 ] == NULL )
    {
        m_FieldValues[ 5 ] = new UaVariant;
    }

    m_FieldValues[ 5 ]->setExpandedNodeId(ExpandedNodeId);
}
/** Set a status code for the event field value ExpandedNodeId
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setExpandedNodeIdStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 5 ] == NULL )
    {
        m_FieldValues[ 5 ] = new UaVariant;
    }

    m_FieldValues[ 5 ]->setStatusCode(status);
}

/** Get the event field value ExpandedNodeId
*/
UaExpandedNodeId SampleEventTypeData::getExpandedNodeId()
{
    if ( m_FieldValues[ 5 ] == NULL )
    {
        return UaExpandedNodeId();
    }
    else
    {
        UaExpandedNodeId ret;
        m_FieldValues[ 5 ]->toExpandedNodeId(ret);
        return ret;
    }
}
/** Get the event field value ExpandedNodeId for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getExpandedNodeIdValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 5 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 5 ]->copyTo(&value);
    }
}
/** Set the event field value Float
 */
void SampleEventTypeData::setFloat(OpcUa_Float Float)
{
    if ( m_FieldValues[ 6 ] == NULL )
    {
        m_FieldValues[ 6 ] = new UaVariant;
    }

    m_FieldValues[ 6 ]->setFloat(Float);
}
/** Set a status code for the event field value Float
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setFloatStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 6 ] == NULL )
    {
        m_FieldValues[ 6 ] = new UaVariant;
    }

    m_FieldValues[ 6 ]->setStatusCode(status);
}

/** Get the event field value Float
*/
OpcUa_Float SampleEventTypeData::getFloat()
{
    if ( m_FieldValues[ 6 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_Float ret = 0;
        m_FieldValues[ 6 ]->toFloat(ret);
        return ret;
    }
}
/** Get the event field value Float for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getFloatValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 6 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 6 ]->copyTo(&value);
    }
}
/** Set the event field value Guid
 */
void SampleEventTypeData::setGuid(const UaGuid& Guid)
{
    if ( m_FieldValues[ 7 ] == NULL )
    {
        m_FieldValues[ 7 ] = new UaVariant;
    }

    m_FieldValues[ 7 ]->setGuid(Guid);
}
/** Set a status code for the event field value Guid
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setGuidStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 7 ] == NULL )
    {
        m_FieldValues[ 7 ] = new UaVariant;
    }

    m_FieldValues[ 7 ]->setStatusCode(status);
}

/** Get the event field value Guid
*/
UaGuid SampleEventTypeData::getGuid()
{
    if ( m_FieldValues[ 7 ] == NULL )
    {
        return UaGuid();
    }
    else
    {
        UaGuid ret;
        m_FieldValues[ 7 ]->toGuid(ret);
        return ret;
    }
}
/** Get the event field value Guid for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getGuidValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 7 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 7 ]->copyTo(&value);
    }
}
/** Set the event field value Int16
 */
void SampleEventTypeData::setInt16(OpcUa_Int16 Int16)
{
    if ( m_FieldValues[ 8 ] == NULL )
    {
        m_FieldValues[ 8 ] = new UaVariant;
    }

    m_FieldValues[ 8 ]->setInt16(Int16);
}
/** Set a status code for the event field value Int16
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setInt16Status(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 8 ] == NULL )
    {
        m_FieldValues[ 8 ] = new UaVariant;
    }

    m_FieldValues[ 8 ]->setStatusCode(status);
}

/** Get the event field value Int16
*/
OpcUa_Int16 SampleEventTypeData::getInt16()
{
    if ( m_FieldValues[ 8 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_Int16 ret = 0;
        m_FieldValues[ 8 ]->toInt16(ret);
        return ret;
    }
}
/** Get the event field value Int16 for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getInt16Value(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 8 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 8 ]->copyTo(&value);
    }
}
/** Set the event field value Int32
 */
void SampleEventTypeData::setInt32(OpcUa_Int32 Int32)
{
    if ( m_FieldValues[ 9 ] == NULL )
    {
        m_FieldValues[ 9 ] = new UaVariant;
    }

    m_FieldValues[ 9 ]->setInt32(Int32);
}
/** Set a status code for the event field value Int32
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setInt32Status(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 9 ] == NULL )
    {
        m_FieldValues[ 9 ] = new UaVariant;
    }

    m_FieldValues[ 9 ]->setStatusCode(status);
}

/** Get the event field value Int32
*/
OpcUa_Int32 SampleEventTypeData::getInt32()
{
    if ( m_FieldValues[ 9 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_Int32 ret = 0;
        m_FieldValues[ 9 ]->toInt32(ret);
        return ret;
    }
}
/** Get the event field value Int32 for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getInt32Value(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 9 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 9 ]->copyTo(&value);
    }
}
/** Set the event field value Int64
 */
void SampleEventTypeData::setInt64(OpcUa_Int64 Int64)
{
    if ( m_FieldValues[ 10 ] == NULL )
    {
        m_FieldValues[ 10 ] = new UaVariant;
    }

    m_FieldValues[ 10 ]->setInt64(Int64);
}
/** Set a status code for the event field value Int64
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setInt64Status(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 10 ] == NULL )
    {
        m_FieldValues[ 10 ] = new UaVariant;
    }

    m_FieldValues[ 10 ]->setStatusCode(status);
}

/** Get the event field value Int64
*/
OpcUa_Int64 SampleEventTypeData::getInt64()
{
    if ( m_FieldValues[ 10 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_Int64 ret = 0;
        m_FieldValues[ 10 ]->toInt64(ret);
        return ret;
    }
}
/** Get the event field value Int64 for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getInt64Value(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 10 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 10 ]->copyTo(&value);
    }
}
/** Set the event field value LocalizedText
 */
void SampleEventTypeData::setLocalizedText(const UaLocalizedText& LocalizedText)
{
    if ( m_FieldValues[ 11 ] == NULL )
    {
        m_FieldValues[ 11 ] = new UaVariant;
    }

    m_FieldValues[ 11 ]->setLocalizedText(LocalizedText);
}
/** Set a status code for the event field value LocalizedText
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setLocalizedTextStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 11 ] == NULL )
    {
        m_FieldValues[ 11 ] = new UaVariant;
    }

    m_FieldValues[ 11 ]->setStatusCode(status);
}

/** Get the event field value LocalizedText
*/
UaLocalizedText SampleEventTypeData::getLocalizedText()
{
    if ( m_FieldValues[ 11 ] == NULL )
    {
        return UaLocalizedText();
    }
    else
    {
        UaLocalizedText ret;
        m_FieldValues[ 11 ]->toLocalizedText(ret);
        return ret;
    }
}
/** Get the event field value LocalizedText for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getLocalizedTextValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 11 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 11 ]->copyTo(&value);
    }
}
/** Set the event field value NodeId
 */
void SampleEventTypeData::setNodeId(const UaNodeId& NodeId)
{
    if ( m_FieldValues[ 12 ] == NULL )
    {
        m_FieldValues[ 12 ] = new UaVariant;
    }

    m_FieldValues[ 12 ]->setNodeId(NodeId);
}
/** Set a status code for the event field value NodeId
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setNodeIdStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 12 ] == NULL )
    {
        m_FieldValues[ 12 ] = new UaVariant;
    }

    m_FieldValues[ 12 ]->setStatusCode(status);
}

/** Get the event field value NodeId
*/
UaNodeId SampleEventTypeData::getNodeId()
{
    if ( m_FieldValues[ 12 ] == NULL )
    {
        return UaNodeId();
    }
    else
    {
        UaNodeId ret;
        m_FieldValues[ 12 ]->toNodeId(ret);
        return ret;
    }
}
/** Get the event field value NodeId for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getNodeIdValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 12 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 12 ]->copyTo(&value);
    }
}
/** Set the event field value QualifiedName
 */
void SampleEventTypeData::setQualifiedName(const UaQualifiedName& QualifiedName)
{
    if ( m_FieldValues[ 13 ] == NULL )
    {
        m_FieldValues[ 13 ] = new UaVariant;
    }

    m_FieldValues[ 13 ]->setQualifiedName(QualifiedName);
}
/** Set a status code for the event field value QualifiedName
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setQualifiedNameStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 13 ] == NULL )
    {
        m_FieldValues[ 13 ] = new UaVariant;
    }

    m_FieldValues[ 13 ]->setStatusCode(status);
}

/** Get the event field value QualifiedName
*/
UaQualifiedName SampleEventTypeData::getQualifiedName()
{
    if ( m_FieldValues[ 13 ] == NULL )
    {
        return UaQualifiedName();
    }
    else
    {
        UaQualifiedName ret;
        m_FieldValues[ 13 ]->toQualifiedName(ret);
        return ret;
    }
}
/** Get the event field value QualifiedName for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getQualifiedNameValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 13 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 13 ]->copyTo(&value);
    }
}
/** Set the event field value SByte
 */
void SampleEventTypeData::setSByte(OpcUa_SByte SByte)
{
    if ( m_FieldValues[ 14 ] == NULL )
    {
        m_FieldValues[ 14 ] = new UaVariant;
    }

    m_FieldValues[ 14 ]->setSByte(SByte);
}
/** Set a status code for the event field value SByte
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setSByteStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 14 ] == NULL )
    {
        m_FieldValues[ 14 ] = new UaVariant;
    }

    m_FieldValues[ 14 ]->setStatusCode(status);
}

/** Get the event field value SByte
*/
OpcUa_SByte SampleEventTypeData::getSByte()
{
    if ( m_FieldValues[ 14 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_SByte ret = 0;
        m_FieldValues[ 14 ]->toSByte(ret);
        return ret;
    }
}
/** Get the event field value SByte for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getSByteValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 14 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 14 ]->copyTo(&value);
    }
}
/** Set the event field value StatusCode
 */
void SampleEventTypeData::setStatusCode(OpcUa_StatusCode StatusCode)
{
    if ( m_FieldValues[ 15 ] == NULL )
    {
        m_FieldValues[ 15 ] = new UaVariant;
    }

    m_FieldValues[ 15 ]->setStatusCode(StatusCode);
}
/** Set a status code for the event field value StatusCode
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setStatusCodeStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 15 ] == NULL )
    {
        m_FieldValues[ 15 ] = new UaVariant;
    }

    m_FieldValues[ 15 ]->setStatusCode(status);
}

/** Get the event field value StatusCode
*/
OpcUa_StatusCode SampleEventTypeData::getStatusCode()
{
    if ( m_FieldValues[ 15 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_StatusCode ret = 0;
        m_FieldValues[ 15 ]->toStatusCode(ret);
        return ret;
    }
}
/** Get the event field value StatusCode for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getStatusCodeValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 15 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 15 ]->copyTo(&value);
    }
}
/** Set the event field value String
 */
void SampleEventTypeData::setString(const UaString& String)
{
    if ( m_FieldValues[ 16 ] == NULL )
    {
        m_FieldValues[ 16 ] = new UaVariant;
    }

    m_FieldValues[ 16 ]->setString(String);
}
/** Set a status code for the event field value String
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setStringStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 16 ] == NULL )
    {
        m_FieldValues[ 16 ] = new UaVariant;
    }

    m_FieldValues[ 16 ]->setStatusCode(status);
}

/** Get the event field value String
*/
UaString SampleEventTypeData::getString()
{
    if ( m_FieldValues[ 16 ] == NULL )
    {
        return "";
    }
    else
    {
        return m_FieldValues[ 16 ]->toString();
    }
}
/** Get the event field value String for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getStringValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 16 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 16 ]->copyTo(&value);
    }
}
/** Set the event field value UInt16
 */
void SampleEventTypeData::setUInt16(OpcUa_UInt16 UInt16)
{
    if ( m_FieldValues[ 17 ] == NULL )
    {
        m_FieldValues[ 17 ] = new UaVariant;
    }

    m_FieldValues[ 17 ]->setUInt16(UInt16);
}
/** Set a status code for the event field value UInt16
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setUInt16Status(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 17 ] == NULL )
    {
        m_FieldValues[ 17 ] = new UaVariant;
    }

    m_FieldValues[ 17 ]->setStatusCode(status);
}

/** Get the event field value UInt16
*/
OpcUa_UInt16 SampleEventTypeData::getUInt16()
{
    if ( m_FieldValues[ 17 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_UInt16 ret = 0;
        m_FieldValues[ 17 ]->toUInt16(ret);
        return ret;
    }
}
/** Get the event field value UInt16 for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getUInt16Value(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 17 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 17 ]->copyTo(&value);
    }
}
/** Set the event field value UInt32
 */
void SampleEventTypeData::setUInt32(OpcUa_UInt32 UInt32)
{
    if ( m_FieldValues[ 18 ] == NULL )
    {
        m_FieldValues[ 18 ] = new UaVariant;
    }

    m_FieldValues[ 18 ]->setUInt32(UInt32);
}
/** Set a status code for the event field value UInt32
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setUInt32Status(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 18 ] == NULL )
    {
        m_FieldValues[ 18 ] = new UaVariant;
    }

    m_FieldValues[ 18 ]->setStatusCode(status);
}

/** Get the event field value UInt32
*/
OpcUa_UInt32 SampleEventTypeData::getUInt32()
{
    if ( m_FieldValues[ 18 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_UInt32 ret = 0;
        m_FieldValues[ 18 ]->toUInt32(ret);
        return ret;
    }
}
/** Get the event field value UInt32 for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getUInt32Value(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 18 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 18 ]->copyTo(&value);
    }
}
/** Set the event field value UInt64
 */
void SampleEventTypeData::setUInt64(OpcUa_UInt64 UInt64)
{
    if ( m_FieldValues[ 19 ] == NULL )
    {
        m_FieldValues[ 19 ] = new UaVariant;
    }

    m_FieldValues[ 19 ]->setUInt64(UInt64);
}
/** Set a status code for the event field value UInt64
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setUInt64Status(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 19 ] == NULL )
    {
        m_FieldValues[ 19 ] = new UaVariant;
    }

    m_FieldValues[ 19 ]->setStatusCode(status);
}

/** Get the event field value UInt64
*/
OpcUa_UInt64 SampleEventTypeData::getUInt64()
{
    if ( m_FieldValues[ 19 ] == NULL )
    {
        return 0;
    }
    else
    {
        OpcUa_UInt64 ret = 0;
        m_FieldValues[ 19 ]->toUInt64(ret);
        return ret;
    }
}
/** Get the event field value UInt64 for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getUInt64Value(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 19 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 19 ]->copyTo(&value);
    }
}
/** Set the event field value XmlElement
 */
void SampleEventTypeData::setXmlElement(UaByteString& XmlElement)
{
    if ( m_FieldValues[ 20 ] == NULL )
    {
        m_FieldValues[ 20 ] = new UaVariant;
    }

    m_FieldValues[ 20 ]->setXmlElement(XmlElement, OpcUa_False);
}
/** Set a status code for the event field value XmlElement
 *
 * If an event field is not available, a StatusCode must be provided
 * instead of the value to indicate the error situation.
 */
void SampleEventTypeData::setXmlElementStatus(OpcUa_StatusCode status)
{
    if ( m_FieldValues[ 20 ] == NULL )
    {
        m_FieldValues[ 20 ] = new UaVariant;
    }

    m_FieldValues[ 20 ]->setStatusCode(status);
}

/** Get the event field value XmlElement
*/
UaByteString SampleEventTypeData::getXmlElement()
{
    if ( m_FieldValues[ 20 ] == NULL )
    {
        return UaByteString();
    }
    else
    {
        UaByteString ret;
        m_FieldValues[ 20 ]->toXmlElement(ret);
        return ret;
    }
}
/** Get the event field value XmlElement for delivering the event data.
 *
 * This method can be overwritten in a derived class to return a client session specific value like a localized text matching the
 * LocaleId requested by the client
 */
void SampleEventTypeData::getXmlElementValue(Session* pSession, OpcUa_Variant& value)
{
    OpcUa_ReferenceParameter(pSession);

    if ( m_FieldValues[ 20 ] == NULL )
    {
        OpcUa_Variant_Clear(&value);
    }
    else
    {
        m_FieldValues[ 20 ]->copyTo(&value);
    }
}

#endif // SUPPORT_Event_Subscription_Server_Facet



} // End namespace for the UA information model http://www.unifiedautomation.com/DemoServer/


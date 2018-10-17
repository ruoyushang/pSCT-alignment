/******************************************************************************
** Copyright (C) 2006-2015 Unified Automation GmbH. All Rights Reserved.
** Web: http://www.unifiedautomation.com
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Project: Sample OPC server
**
** Description: Event Data classes.
**
******************************************************************************/
#include "baeventdata.h"
#include "buildingautomationtypeids.h"
#include "eventmanageruanode.h"

/* ----------------------------------------------------------------------------
    Begin Class    ControllerEventTypeData
    constructors / destructors
-----------------------------------------------------------------------------*/
std::map<OpcUa_UInt32, OpcUa_UInt32> ControllerEventTypeData::s_ControllerEventTypeDataFields;
ControllerEventTypeData::ControllerEventTypeData(OpcUa_Int16 nsIdx)
{
    m_nsIdx = nsIdx;
    m_EventTypeId.setNodeId(Ba_ControllerEventType, m_nsIdx);
}
ControllerEventTypeData::~ControllerEventTypeData()
{
}
/* Registers all event type fields with the EventManagerUaNode.
 */
void ControllerEventTypeData::registerEventFields()
{
    // Register event type
    EventManagerUaNode::registerEventType(OpcUaId_BaseEventType, m_EventTypeId);
    // Register event fields
    s_ControllerEventTypeDataFields.clear();
    s_ControllerEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("Temperature", m_nsIdx).toFullString())] = 1;
    s_ControllerEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("State", m_nsIdx).toFullString())] = 2;
}

/* Get the field value for the passed index. 
 *  @param index The index of the selected field.
 *  @param data The data for the selected field.
 */
void ControllerEventTypeData::getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data)
{
    // Try to find the field index
    std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator it;
    it = s_ControllerEventTypeDataFields.find(index);

    if ( it == s_ControllerEventTypeDataFields.end()  )
    {
        BaseEventTypeData::getFieldData(index, pSession, data);
        return;
    }
    switch(it->second)
    {
        case 1:
        {
            m_Temperature.copyTo(&data);
            break;
        }
        case 2:
        {
            m_State.copyTo(&data);
            break;
        }
        default:
        {
            OpcUa_Variant_Initialize(&data);
        }
    }
}
/* ----------------------------------------------------------------------------
    End Class    ControllerEventTypeData
-----------------------------------------------------------------------------*/



#include "mpeseventdata.hpp"
#include "passervertypeids.hpp"
#include "uaserver/eventmanageruanode.h"

/* ----------------------------------------------------------------------------
    Begin Class    MPESEventTypeData
    constructors / destructors
-----------------------------------------------------------------------------*/
std::map<OpcUa_UInt32, OpcUa_UInt32> MPESEventTypeData::s_MPESEventTypeDataFields;

MPESEventTypeData::MPESEventTypeData(OpcUa_Int16 nsIdx)
{
    m_nsIdx = nsIdx;
    m_EventTypeId.setNodeId(PAS_MPESEventType, m_nsIdx);
}
MPESEventTypeData::~MPESEventTypeData()
{
}

/* Registers all event type fields with the EventManagerUaNode.
 */
void MPESEventTypeData::registerEventFields()
{
    // Register event type
    EventManagerUaNode::registerEventType(OpcUaId_BaseEventType, m_EventTypeId);
    // Register event fields
    s_MPESEventTypeDataFields.clear();
    s_MPESEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("State", m_nsIdx).toFullString())] = 1;
    s_MPESEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("xCentroid", m_nsIdx).toFullString())] = 2;
    s_MPESEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("yCentroid", m_nsIdx).toFullString())] = 3;
    s_MPESEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("xCentroidSpotWidth", m_nsIdx).toFullString())] = 4;
    s_MPESEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("yCentroidSpotWidth", m_nsIdx).toFullString())] = 5;
    s_MPESEventTypeDataFields[EventManagerUaNode::registerEventField(UaQualifiedName("CleanedIntensity", m_nsIdx).toFullString())] = 6;
}

/* Get the field value for the passed index. 
 *  @param index The index of the selected field.
 *  @param data The data for the selected field.
 */
void MPESEventTypeData::getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data)
{
    // Try to find the field index
    std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator it;
    it = s_MPESEventTypeDataFields.find(index);

    if ( it == s_MPESEventTypeDataFields.end()  )
    {
        BaseEventTypeData::getFieldData(index, pSession, data);
        return;
    }
    switch(it->second)
    {
        case 1:
        {
            m_State.copyTo(&data);
            break;
        }
        case 2:
        {
            m_xCentroidAvg.copyTo(&data);
            break;
        }
        case 3:
        {
            m_yCentroidAvg.copyTo(&data);
            break;
        }
        case 4:
        {
            m_xCentroidSpotWidth.copyTo(&data);
            break;
        }
        case 5:
        {
            m_yCentroidSpotWidth.copyTo(&data);
            break;
        }
        case 6:
        {
            m_CleanedIntensity.copyTo(&data);
            break;
        }

        default:
        {
            OpcUa_Variant_Initialize(&data);
        }
    }
}
/* ----------------------------------------------------------------------------
    End Class    MPESEventTypeData
-----------------------------------------------------------------------------*/

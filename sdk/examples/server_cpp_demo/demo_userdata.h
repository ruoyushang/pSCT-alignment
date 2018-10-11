/******************************************************************************
** demo_userdata.h
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

#ifndef __DEMO_USERDATA_H__
#define __DEMO_USERDATA_H__

#include "userdatabase.h"
#include "referencecounter.h"

#define SharedDataWorkOrder 1


// Namespace for the UA information model http://www.unifiedautomation.com/DemoServer/

namespace Demo {

// UserData implementation for DemoServer
class CPP_DEMO_EXPORT UserDataDemo : public UserDataBase
{
    UA_DISABLE_COPY(UserDataDemo);
public:
    enum UserDataDemoType
    {
        UserDataDemoType_Shared,
        UserDataDemoType_Event,
        UserDataDemoType_Alarm
    };
    UserDataDemo(){}
    virtual ~UserDataDemo(){}

    // UserDataDemoType
    virtual UserDataDemoType getUserDataDemoType() const = 0;
protected:
private:
};
// A reference counted class to use with UserDataDemoShared. Subclass this class to hold your specific UserData.
class CPP_DEMO_EXPORT DemoSharedData : public ReferenceCounter
{
    UA_DISABLE_COPY(DemoSharedData);
protected:
    virtual ~DemoSharedData(){}
public:
    DemoSharedData(){}
    virtual int sharedDataId(){return 0;}

    // Value
    virtual void setValue(const UaVariant& value)
    {
        m_value = value;
    }
    virtual UaVariant getValue() const
    {
        return m_value;
    }

    // SourceTimestamp
    virtual void setSourceTimestamp(const UaDateTime& sourceTimestamp)
    {
        m_sourceTimestamp = sourceTimestamp;
    }
    virtual UaDateTime getSourceTimestamp() const
    {
        return m_sourceTimestamp;
    }

protected:
    UaVariant m_value;
    UaDateTime m_sourceTimestamp;
};

// UserDataDemo implementation working with a shared data class DemoSharedData
class CPP_DEMO_EXPORT UserDataDemoShared : public UserDataDemo
{
    UA_DISABLE_COPY(UserDataDemoShared);
public:
    UserDataDemoShared()
    {
        m_pSharedData = NULL;
    }
    UserDataDemoShared(DemoSharedData* pSharedData)
    {
        if (pSharedData)
        {
            pSharedData->addReference();
        }
        m_pSharedData = pSharedData;
    }
    virtual ~UserDataDemoShared()
    {
        if (m_pSharedData)
        {
            m_pSharedData->releaseReference();
            m_pSharedData = NULL;
        }
    }

    // get and set shared data
    virtual void setSharedData(DemoSharedData* pSharedData)
    {
        if (m_pSharedData)
        {
            m_pSharedData->releaseReference();
        }
        if (pSharedData)
        {
            pSharedData->addReference();
        }
        m_pSharedData = pSharedData;
    }
    virtual DemoSharedData* pSharedData() const
    {
        return m_pSharedData;
    }

    virtual int sharedDataId() const
    {
        if (m_pSharedData)
        {
            return m_pSharedData->sharedDataId();
        }
        return 0;
    }

    // UserDataDemoType
    virtual UserDataDemoType getUserDataDemoType() const
    {
        return UserDataDemo::UserDataDemoType_Shared;
    }

protected:
    DemoSharedData* m_pSharedData;
};

// UserDataDemoShared implementation working for WorkOrder Sample Data
class CPP_DEMO_EXPORT UserDataWorkOrder : public UserDataDemoShared
{
    UA_DISABLE_COPY(UserDataWorkOrder);
public:
    enum Field
    {
        CompleteStructure,
        AssetID,
        ID,
        StartTime,
        StatusComments
    };
    UserDataWorkOrder()
    {
        m_Field = CompleteStructure;
    }
    UserDataWorkOrder(Field field)
    {
        m_Field = field;
    }
    UserDataWorkOrder(Field field, DemoSharedData* pSharedData)
        : UserDataDemoShared(pSharedData)
    {
        m_Field = field;
    }
    virtual ~UserDataWorkOrder(){}

    virtual int sharedDataId() const{return SharedDataWorkOrder;}

    // get and set field enum
    Field getField() const{return m_Field;}
    void setField(Field field){m_Field = field;}

protected:
    Field m_Field;
};

#if SUPPORT_Event_Subscription_Server_Facet
// UserDataDemo implementation for handling events
class CPP_DEMO_EXPORT UserDataDemoEvent : public UserDataDemo
{
    UA_DISABLE_COPY(UserDataDemoEvent);
public:
    enum EventType
    {
        EventType_Base,
        EventType_Sample
    };
    UserDataDemoEvent(EventType eventType)
    {
        m_eventType = eventType;
    }
    virtual ~UserDataDemoEvent(){}

    // UserDataDemoType
    virtual UserDataDemoType getUserDataDemoType() const
    {
        return UserDataDemo::UserDataDemoType_Event;
    }

    virtual void fireEvent(UaNode* pNode, EventManagerBase* pEventManager)
    {
        if (pNode && pEventManager)
        {
            // fire base event
            if (m_eventType == EventType_Base)
            {
                BaseEventTypeData eventData;
                // Fill all default event fields
                eventData.m_SourceNode.setNodeId(pNode->nodeId());
                eventData.m_SourceName.setString(pNode->browseName().toString());
                eventData.setMessage(UaLocalizedText("en", "Base Event was triggererd."));
                eventData.setSeverity(500);
                // Set timestamps and unique EventId
                eventData.prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Fire the event
                pEventManager->fireEvent(&eventData);
            }
            // fire sample event
            else
            {
                SampleEventTypeData eventData;
                // Handle SampleEventTypeData specific fields
                eventData.setBoolean(OpcUa_True);
                eventData.setByte(123);
                UaByteString bsTmp = UaByteString::fromHex("0123ABCD");
                eventData.setByteString(bsTmp);
                eventData.setDateTime(UaDateTime::now());
                eventData.setDouble(123.567);
                eventData.setExpandedNodeId(UaExpandedNodeId(UaNodeId("String Identifier", 42), "", 23));
                eventData.setFloat(23.76f);
                eventData.setGuid(UaGuid::create());
                eventData.setInt16(-321);
                eventData.setInt32(65432);
                eventData.setInt64(-876543);
                eventData.setLocalizedText(UaLocalizedText("de", "Das ist deutsch"));
                eventData.setNodeId(UaNodeId("String Identifier", 42));
                eventData.setQualifiedName(UaQualifiedName("Name", 42));
                eventData.setSByte(123);
                eventData.setStatusCode(OpcUa_BadNotImplemented);
                eventData.setString("Hello world");
                eventData.setUInt16(32);
                eventData.setUInt32(546);
                eventData.setUInt64(8877);
                UaString sTemp = UaString("<a>Hello world</a>");
                bsTmp = UaByteString(sTemp.size(), (OpcUa_Byte*)sTemp.toUtf8());
                eventData.setXmlElement(bsTmp);
                // Fill all default event fields
                eventData.m_SourceNode.setNodeId(pNode->nodeId());
                eventData.m_SourceName.setString(pNode->browseName().toString());
                eventData.setMessage(UaLocalizedText("en", "Sample Event was triggererd."));
                eventData.setSeverity(500);
                // Set timestamps and unique EventId
                eventData.prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Fire the event
                pEventManager->fireEvent(&eventData);
            }
        }
    }

protected:
    EventType m_eventType;
};

// UserDataDemo implementation for handling alarms
class CPP_DEMO_EXPORT UserDataDemoAlarm : public UserDataDemo
{
    UA_DISABLE_COPY(UserDataDemoAlarm);
public:
    enum AlarmType
    {
        AlarmType_OffNormal,
        AlarmType_NonExclusiveLevel,
        AlarmType_ExclusiveLevel
    };

    UserDataDemoAlarm(){}
    virtual ~UserDataDemoAlarm(){}

    // AlarmType
    virtual AlarmType getAlarmType() const = 0;

    // UserDataDemoType
    virtual UserDataDemoType getUserDataDemoType() const
    {
        return UserDataDemo::UserDataDemoType_Alarm;
    }
protected:
};

// UserDataDemoAlarm implementation for handling OffNormalAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmOffNormal : public UserDataDemoAlarm
{
    UA_DISABLE_COPY(UserDataDemoAlarmOffNormal);
public:
    UserDataDemoAlarmOffNormal(){}
    virtual ~UserDataDemoAlarmOffNormal(){}

    virtual void setActiveState(OpcUa_Boolean bActive, EventManagerBase* pEventManager) = 0;

    // AlarmType
    virtual AlarmType getAlarmType() const
    {
        return AlarmType_OffNormal;
    }

    // UserDataDemoType
    virtual UserDataDemoType getUserDataDemoType() const
    {
        return UserDataDemo::UserDataDemoType_Alarm;
    }
protected:
};

// UserDataDemoAlarmOffNormal implementation for handling OffNormalAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmOffNormalNode : public UserDataDemoAlarmOffNormal
{
    UA_DISABLE_COPY(UserDataDemoAlarmOffNormalNode);
public:
    UserDataDemoAlarmOffNormalNode(OpcUa::OffNormalAlarmType* pOffNormalAlarm)
    {
        m_pOffNormalAlarm = pOffNormalAlarm;
        if (m_pOffNormalAlarm)
        {
            m_pOffNormalAlarm->addReference();
        }
    }
    virtual ~UserDataDemoAlarmOffNormalNode()
    {
        if (m_pOffNormalAlarm)
        {
            m_pOffNormalAlarm->releaseReference();
            m_pOffNormalAlarm = NULL;
        }
    }

    virtual void setActiveState(OpcUa_Boolean bActive, EventManagerBase* pEventManager)
    {
        OpcUa_ReferenceParameter(pEventManager);
        if (m_pOffNormalAlarm && bActive != m_pOffNormalAlarm->getActiveStateBool())
        {
            m_pOffNormalAlarm->setActiveState(bActive);
            m_pOffNormalAlarm->setRetain(bActive);
            if (OpcUa_False == bActive)
            {
                m_pOffNormalAlarm->setMessage(UaLocalizedText("en", "Off normal alarm inactive"));
            }
            else
            {
                m_pOffNormalAlarm->setMessage(UaLocalizedText("en", "Off normal alarm active"));
            }
            m_pOffNormalAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        }
    }

    // UserDataDemoType
    virtual UserDataDemoType getUserDataDemoType() const
    {
        return UserDataDemo::UserDataDemoType_Alarm;
    }
protected:
    OpcUa::OffNormalAlarmType* m_pOffNormalAlarm;
};

// UserDataDemoAlarmOffNormal implementation for handling OffNormalAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmOffNormalNoNode : public UserDataDemoAlarmOffNormal
{
    UA_DISABLE_COPY(UserDataDemoAlarmOffNormalNoNode);
public:
    UserDataDemoAlarmOffNormalNoNode(OpcUa::OffNormalAlarmTypeData* pOffNormalAlarmData)
    {
        m_pOffNormalAlarmData = pOffNormalAlarmData;
        if (m_pOffNormalAlarmData)
        {
            m_pOffNormalAlarmData->addReference();
        }
    }
    virtual ~UserDataDemoAlarmOffNormalNoNode()
    {
        if (m_pOffNormalAlarmData)
        {
            m_pOffNormalAlarmData->releaseReference();
            m_pOffNormalAlarmData = NULL;
        }
    }

    virtual void setActiveState(OpcUa_Boolean bActive, EventManagerBase* pEventManager)
    {
        if (m_pOffNormalAlarmData && bActive != m_pOffNormalAlarmData->getActiveState())
        {
            // Lock the event data for change since a refresh may access the data in parallel
            m_pOffNormalAlarmData->lockEventData();
            m_pOffNormalAlarmData->setActiveState(bActive);
            m_pOffNormalAlarmData->setActiveState_TransitionTime(UaDateTime::now());
            m_pOffNormalAlarmData->setRetain(bActive);
            if (OpcUa_False == bActive)
            {
                m_pOffNormalAlarmData->setMessage(UaLocalizedText("en", "No nodes off normal alarm inactive"));
            }
            else
            {
                m_pOffNormalAlarmData->setMessage(UaLocalizedText("en", "No nodes off normal alarm active"));
            }
            m_pOffNormalAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            // Unlock event data before fire event
            m_pOffNormalAlarmData->unlockEventData();
            pEventManager->fireEvent(m_pOffNormalAlarmData);
        }
    }

    // UserDataDemoType
    virtual UserDataDemoType getUserDataDemoType() const
    {
        return UserDataDemo::UserDataDemoType_Alarm;
    }
protected:
    OpcUa::OffNormalAlarmTypeData* m_pOffNormalAlarmData;
};

// UserDataDemoAlarm implementation for handling LevelAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmLevel : public UserDataDemoAlarm
{
    UA_DISABLE_COPY(UserDataDemoAlarmLevel);
public:
    UserDataDemoAlarmLevel(){}
    virtual ~UserDataDemoAlarmLevel(){}
    virtual void setLevel(OpcUa_Double level, EventManagerBase* pEventManager) = 0;
};

// UserDataDemoAlarmLevel implementation for handling NonExclusiveLevelAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmLevelNonEx : public UserDataDemoAlarmLevel
{
    UA_DISABLE_COPY(UserDataDemoAlarmLevelNonEx);
public:
    UserDataDemoAlarmLevelNonEx(){}
    virtual ~UserDataDemoAlarmLevelNonEx(){}
    virtual void setLevel(OpcUa_Double level, EventManagerBase* pEventManager) = 0;

    // AlarmType
    virtual AlarmType getAlarmType() const
    {
        return AlarmType_NonExclusiveLevel;
    }
};

// UserDataDemoAlarmLevel implementation for handling NonExclusiveLevelAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmLevelNonExNode : public UserDataDemoAlarmLevelNonEx
{
    UA_DISABLE_COPY(UserDataDemoAlarmLevelNonExNode);
public:
    UserDataDemoAlarmLevelNonExNode(OpcUa::NonExclusiveLevelAlarmType* pNonExclusiveLevelAlarm)
    {
        m_pNonExclusiveLevelAlarm = pNonExclusiveLevelAlarm;
        if (m_pNonExclusiveLevelAlarm)
        {
            m_pNonExclusiveLevelAlarm->addReference();
        }
    }
    virtual ~UserDataDemoAlarmLevelNonExNode()
    {
        if (m_pNonExclusiveLevelAlarm)
        {
            m_pNonExclusiveLevelAlarm->releaseReference();
            m_pNonExclusiveLevelAlarm = NULL;
        }
    }
    virtual void setLevel(OpcUa_Double level, EventManagerBase* pEventManager)
    {
        OpcUa_ReferenceParameter(pEventManager);
        if (m_pNonExclusiveLevelAlarm)
        {
            if (level <= m_pNonExclusiveLevelAlarm->getLowLowLimit() && m_pNonExclusiveLevelAlarm->getLowLowStateBool() == OpcUa_False)
            {
                m_pNonExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in LowLow"));
                m_pNonExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLowLowState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLowState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLastSeverity(m_pNonExclusiveLevelAlarm->getSeverity());
                m_pNonExclusiveLevelAlarm->setSeverity(800);
                m_pNonExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (level <= m_pNonExclusiveLevelAlarm->getLowLimit() && m_pNonExclusiveLevelAlarm->getLowStateBool() == OpcUa_False)
            {
                m_pNonExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in Low"));
                m_pNonExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setLowState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLastSeverity(m_pNonExclusiveLevelAlarm->getSeverity());
                m_pNonExclusiveLevelAlarm->setSeverity(400);
                m_pNonExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (level >= m_pNonExclusiveLevelAlarm->getHighHighLimit() && m_pNonExclusiveLevelAlarm->getHighHighStateBool() == OpcUa_False)
            {
                m_pNonExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in HighHigh"));
                m_pNonExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setHighState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setHighHighState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLastSeverity(m_pNonExclusiveLevelAlarm->getSeverity());
                m_pNonExclusiveLevelAlarm->setSeverity(800);
                m_pNonExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (level >= m_pNonExclusiveLevelAlarm->getHighLimit() && m_pNonExclusiveLevelAlarm->getHighHighStateBool() == OpcUa_False)
            {
                m_pNonExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in High"));
                m_pNonExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setHighState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setLastSeverity(m_pNonExclusiveLevelAlarm->getSeverity());
                m_pNonExclusiveLevelAlarm->setSeverity(400);
                m_pNonExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (m_pNonExclusiveLevelAlarm->getActiveStateBool() != OpcUa_False)
            {
                m_pNonExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Non exclusive level alarm inactive"));
                m_pNonExclusiveLevelAlarm->setActiveState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setAckedState(OpcUa_True);
                m_pNonExclusiveLevelAlarm->setRetain(OpcUa_False);
                m_pNonExclusiveLevelAlarm->setLastSeverity(m_pNonExclusiveLevelAlarm->getSeverity());
                m_pNonExclusiveLevelAlarm->setSeverity(100);
                m_pNonExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
        }
    }
protected:
    OpcUa::NonExclusiveLevelAlarmType* m_pNonExclusiveLevelAlarm;
private:
    UserDataDemoAlarmLevelNonExNode();
};

// UserDataDemoAlarmLevel implementation for handling NonExclusiveLevelAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmLevelNonExNoNode : public UserDataDemoAlarmLevelNonEx
{
    UA_DISABLE_COPY(UserDataDemoAlarmLevelNonExNoNode);
public:
    UserDataDemoAlarmLevelNonExNoNode(OpcUa::NonExclusiveLevelAlarmTypeData* pNonExclusiveLevelAlarmData)
    {
        m_pNonExclusiveLevelAlarmData = pNonExclusiveLevelAlarmData;
        if (m_pNonExclusiveLevelAlarmData)
        {
            m_pNonExclusiveLevelAlarmData->addReference();
        }
    }
    virtual ~UserDataDemoAlarmLevelNonExNoNode()
    {
        if (m_pNonExclusiveLevelAlarmData)
        {
            m_pNonExclusiveLevelAlarmData->releaseReference();
            m_pNonExclusiveLevelAlarmData = NULL;
        }
    }
    virtual void setLevel(OpcUa_Double level, EventManagerBase* pEventManager)
    {
        if (m_pNonExclusiveLevelAlarmData)
        {
            if (level <= m_pNonExclusiveLevelAlarmData->getLowLowLimit() && m_pNonExclusiveLevelAlarmData->getLowLowState() == OpcUa_False)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pNonExclusiveLevelAlarmData->lockEventData();
                m_pNonExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in LowLow"));
                m_pNonExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setActiveState_TransitionTime(UaDateTime::now());
                m_pNonExclusiveLevelAlarmData->setLowLowState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLowState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLastSeverity(m_pNonExclusiveLevelAlarmData->getSeverity());
                m_pNonExclusiveLevelAlarmData->setSeverity(800);
                m_pNonExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pNonExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pNonExclusiveLevelAlarmData);
            }
            else if (level <= m_pNonExclusiveLevelAlarmData->getLowLimit() && m_pNonExclusiveLevelAlarmData->getLowState() == OpcUa_False)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pNonExclusiveLevelAlarmData->lockEventData();
                m_pNonExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in Low"));
                m_pNonExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setLowState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLastSeverity(m_pNonExclusiveLevelAlarmData->getSeverity());
                m_pNonExclusiveLevelAlarmData->setSeverity(400);
                m_pNonExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pNonExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pNonExclusiveLevelAlarmData);
            }
            else if (level <= m_pNonExclusiveLevelAlarmData->getHighHighLimit() && m_pNonExclusiveLevelAlarmData->getHighHighState() == OpcUa_False)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pNonExclusiveLevelAlarmData->lockEventData();
                m_pNonExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in HighHigh"));
                m_pNonExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setHighState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setHighHighState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLastSeverity(m_pNonExclusiveLevelAlarmData->getSeverity());
                m_pNonExclusiveLevelAlarmData->setSeverity(800);
                m_pNonExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pNonExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pNonExclusiveLevelAlarmData);
            }
            else if (level <= m_pNonExclusiveLevelAlarmData->getHighLimit() && m_pNonExclusiveLevelAlarmData->getHighState() == OpcUa_False)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pNonExclusiveLevelAlarmData->lockEventData();
                m_pNonExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in High"));
                m_pNonExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setHighState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setLastSeverity(m_pNonExclusiveLevelAlarmData->getSeverity());
                m_pNonExclusiveLevelAlarmData->setSeverity(400);
                m_pNonExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pNonExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pNonExclusiveLevelAlarmData);
            }
            else if (m_pNonExclusiveLevelAlarmData->getActiveState() != OpcUa_False)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pNonExclusiveLevelAlarmData->lockEventData();
                m_pNonExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm inactive"));
                m_pNonExclusiveLevelAlarmData->setActiveState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setLowLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setLowState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setHighHighState(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setAckedState(OpcUa_True);
                m_pNonExclusiveLevelAlarmData->setRetain(OpcUa_False);
                m_pNonExclusiveLevelAlarmData->setLastSeverity(m_pNonExclusiveLevelAlarmData->getSeverity());
                m_pNonExclusiveLevelAlarmData->setSeverity(100);
                m_pNonExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pNonExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pNonExclusiveLevelAlarmData);
            }
        }
    }
protected:
    OpcUa::NonExclusiveLevelAlarmTypeData* m_pNonExclusiveLevelAlarmData;
private:
    UserDataDemoAlarmLevelNonExNoNode();
};

// UserDataDemoAlarmLevel implementation for handling ExclusiveLevelAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmLevelExclusive : public UserDataDemoAlarmLevel
{
    UA_DISABLE_COPY(UserDataDemoAlarmLevelExclusive);
public:
    UserDataDemoAlarmLevelExclusive(){}
    virtual ~UserDataDemoAlarmLevelExclusive(){}
    virtual void setLevel(OpcUa_Double level, EventManagerBase* pEventManager) = 0;

    // AlarmType
    virtual AlarmType getAlarmType() const
    {
        return AlarmType_ExclusiveLevel;
    }
};

// UserDataDemoAlarmLevel implementation for handling NonExclusiveLevelAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmLevelExclusiveNode : public UserDataDemoAlarmLevelExclusive
{
    UA_DISABLE_COPY(UserDataDemoAlarmLevelExclusiveNode);
public:
    UserDataDemoAlarmLevelExclusiveNode(OpcUa::ExclusiveLevelAlarmType* pExclusiveLevelAlarm)
    {
        m_pExclusiveLevelAlarm = pExclusiveLevelAlarm;
        if (m_pExclusiveLevelAlarm)
        {
            m_pExclusiveLevelAlarm->addReference();
        }
    }
    virtual ~UserDataDemoAlarmLevelExclusiveNode()
    {
        if (m_pExclusiveLevelAlarm)
        {
            m_pExclusiveLevelAlarm->releaseReference();
            m_pExclusiveLevelAlarm = NULL;
        }
    }
    virtual void setLevel(OpcUa_Double level, EventManagerBase* pEventManager)
    {
        OpcUa_ReferenceParameter(pEventManager);
        if (m_pExclusiveLevelAlarm)
        {
            if (level <= m_pExclusiveLevelAlarm->getLowLowLimit() && m_pExclusiveLevelAlarm->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::LowLow)
            {
                m_pExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Exclusive level alarm active in LowLow"));
                m_pExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarm->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::LowLow);
                m_pExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarm->setLastSeverity(m_pExclusiveLevelAlarm->getSeverity());
                m_pExclusiveLevelAlarm->setSeverity(800);
                m_pExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (level <= m_pExclusiveLevelAlarm->getLowLimit() && m_pExclusiveLevelAlarm->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::Low)
            {
                m_pExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Exclusive level alarm active in Low"));
                m_pExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarm->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Low);
                m_pExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarm->setLastSeverity(m_pExclusiveLevelAlarm->getSeverity());
                m_pExclusiveLevelAlarm->setSeverity(400);
                m_pExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (level >= m_pExclusiveLevelAlarm->getHighHighLimit() && m_pExclusiveLevelAlarm->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::HighHigh)
            {
                m_pExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Exclusive level alarm active in HighHigh"));
                m_pExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarm->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::HighHigh);
                m_pExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarm->setLastSeverity(m_pExclusiveLevelAlarm->getSeverity());
                m_pExclusiveLevelAlarm->setSeverity(800);
                m_pExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (level >= m_pExclusiveLevelAlarm->getHighLimit() && m_pExclusiveLevelAlarm->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::High)
            {
                m_pExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Exclusive level alarm active in High"));
                m_pExclusiveLevelAlarm->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarm->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::High);
                m_pExclusiveLevelAlarm->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarm->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarm->setLastSeverity(m_pExclusiveLevelAlarm->getSeverity());
                m_pExclusiveLevelAlarm->setSeverity(400);
                m_pExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
            else if (m_pExclusiveLevelAlarm->getActiveStateBool() != OpcUa_False)
            {
                m_pExclusiveLevelAlarm->setMessage(UaLocalizedText("en", "Exclusive level alarm inactive"));
                m_pExclusiveLevelAlarm->setActiveState(OpcUa_False);
                m_pExclusiveLevelAlarm->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Disabled);
                m_pExclusiveLevelAlarm->setAckedState(OpcUa_True);
                m_pExclusiveLevelAlarm->setRetain(OpcUa_False);
                m_pExclusiveLevelAlarm->setLastSeverity(m_pExclusiveLevelAlarm->getSeverity());
                m_pExclusiveLevelAlarm->setSeverity(400);
                m_pExclusiveLevelAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
        }
    }
protected:
    OpcUa::ExclusiveLevelAlarmType* m_pExclusiveLevelAlarm;
private:
    UserDataDemoAlarmLevelExclusiveNode();
};

// UserDataDemoAlarmLevel implementation for handling NonExclusiveLevelAlarms
class CPP_DEMO_EXPORT UserDataDemoAlarmLevelExclusiveNoNode : public UserDataDemoAlarmLevelExclusive
{
    UA_DISABLE_COPY(UserDataDemoAlarmLevelExclusiveNoNode);
public:
    UserDataDemoAlarmLevelExclusiveNoNode(OpcUa::ExclusiveLevelAlarmTypeData* pExclusiveLevelAlarmData)
    {
        m_pExclusiveLevelAlarmData = pExclusiveLevelAlarmData;
        if (m_pExclusiveLevelAlarmData)
        {
            m_pExclusiveLevelAlarmData->addReference();
        }
    }
    virtual ~UserDataDemoAlarmLevelExclusiveNoNode()
    {
        if (m_pExclusiveLevelAlarmData)
        {
            m_pExclusiveLevelAlarmData->releaseReference();
            m_pExclusiveLevelAlarmData = NULL;
        }
    }
    virtual void setLevel(OpcUa_Double level, EventManagerBase* pEventManager)
    {
        if (m_pExclusiveLevelAlarmData)
        {
            if (level <= m_pExclusiveLevelAlarmData->getLowLowLimit() && m_pExclusiveLevelAlarmData->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::LowLow)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pExclusiveLevelAlarmData->lockEventData();
                m_pExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Exclusive level alarm active in LowLow"));
                m_pExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarmData->setActiveState_TransitionTime(UaDateTime::now());
                m_pExclusiveLevelAlarmData->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::LowLow);
                m_pExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarmData->setLastSeverity(m_pExclusiveLevelAlarmData->getSeverity());
                m_pExclusiveLevelAlarmData->setSeverity(800);
                m_pExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pExclusiveLevelAlarmData);
            }
            else if (level <= m_pExclusiveLevelAlarmData->getLowLimit() && m_pExclusiveLevelAlarmData->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::Low)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pExclusiveLevelAlarmData->lockEventData();
                m_pExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in Low"));
                m_pExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarmData->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Low);
                m_pExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarmData->setLastSeverity(m_pExclusiveLevelAlarmData->getSeverity());
                m_pExclusiveLevelAlarmData->setSeverity(400);
                m_pExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pExclusiveLevelAlarmData);
            }
            else if (level >= m_pExclusiveLevelAlarmData->getHighHighLimit() && m_pExclusiveLevelAlarmData->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::HighHigh)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pExclusiveLevelAlarmData->lockEventData();
                m_pExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in HighHigh"));
                m_pExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarmData->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::HighHigh);
                m_pExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarmData->setLastSeverity(m_pExclusiveLevelAlarmData->getSeverity());
                m_pExclusiveLevelAlarmData->setSeverity(800);
                m_pExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pExclusiveLevelAlarmData);
            }
            else if (level >= m_pExclusiveLevelAlarmData->getHighLimit() && m_pExclusiveLevelAlarmData->getExclusiveState() != OpcUa::ExclusiveLimitStateMachineType::High)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pExclusiveLevelAlarmData->lockEventData();
                m_pExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Non exclusive level alarm active in High"));
                m_pExclusiveLevelAlarmData->setActiveState(OpcUa_True);
                m_pExclusiveLevelAlarmData->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::High);
                m_pExclusiveLevelAlarmData->setAckedState(OpcUa_False);
                m_pExclusiveLevelAlarmData->setRetain(OpcUa_True);
                m_pExclusiveLevelAlarmData->setLastSeverity(m_pExclusiveLevelAlarmData->getSeverity());
                m_pExclusiveLevelAlarmData->setSeverity(400);
                m_pExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pExclusiveLevelAlarmData);
            }
            else if (m_pExclusiveLevelAlarmData->getActiveState() != OpcUa_False)
            {
                // Lock the event data for change since a refresh may access the data in parallel
                m_pExclusiveLevelAlarmData->lockEventData();
                m_pExclusiveLevelAlarmData->setMessage(UaLocalizedText("en", "Exclusive level alarm inactive"));
                m_pExclusiveLevelAlarmData->setActiveState(OpcUa_False);
                m_pExclusiveLevelAlarmData->setExclusiveState(OpcUa::ExclusiveLimitStateMachineType::Disabled);
                m_pExclusiveLevelAlarmData->setAckedState(OpcUa_True);
                m_pExclusiveLevelAlarmData->setRetain(OpcUa_False);
                m_pExclusiveLevelAlarmData->setLastSeverity(m_pExclusiveLevelAlarmData->getSeverity());
                m_pExclusiveLevelAlarmData->setSeverity(400);
                m_pExclusiveLevelAlarmData->prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
                // Unlock event data before fire event
                m_pExclusiveLevelAlarmData->unlockEventData();
                pEventManager->fireEvent(m_pExclusiveLevelAlarmData);
            }
        }
    }
protected:
    OpcUa::ExclusiveLevelAlarmTypeData* m_pExclusiveLevelAlarmData;
private:
    UserDataDemoAlarmLevelExclusiveNoNode();
};
#endif // SUPPORT_Event_Subscription_Server_Facet

} // namespace Demo

#endif // #ifndef __DEMO_USERDATA_H__


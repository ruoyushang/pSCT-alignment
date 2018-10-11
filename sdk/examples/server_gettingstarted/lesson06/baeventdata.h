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
#ifndef BAEVENTDATA_H
#define BAEVENTDATA_H

#include "uaeventdata.h"

/** ControllerEventTypeData Class.
*/
class ControllerEventTypeData: public BaseEventTypeData
{
    UA_DISABLE_COPY(ControllerEventTypeData);
public:
    ControllerEventTypeData(OpcUa_Int16 nsIdx);
    virtual ~ControllerEventTypeData();

    /** Registers all event type fields with the EventManagerUaNode. */
    void registerEventFields();
    /** Get the field value for the passed index. */
    virtual void getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data);

    /** Field 1 - Temperature */
    UaVariant m_Temperature;
    /** Field 2 - State */
    UaVariant m_State;

    OpcUa_Int16 m_nsIdx;

private:
    static std::map<OpcUa_UInt32, OpcUa_UInt32> s_ControllerEventTypeDataFields;
};

#endif // BAEVENTDATA_H

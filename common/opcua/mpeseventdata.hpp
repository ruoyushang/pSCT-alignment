#ifndef COMMON_OPCUA_MPESEVENTDATA_HPP
#define COMMON_OPCUA_MPESEVENTDATA_HPP

#include "uaserver/uaeventdata.h"

/** MPESEventTypeData Class.
*/
class MPESEventTypeData: public BaseEventTypeData
{
    UA_DISABLE_COPY(MPESEventTypeData);
public:
    explicit MPESEventTypeData(OpcUa_Int16 nsIdx);

    /** Registers all event type fields with the EventManagerUaNode. */
    void registerEventFields();
    /** Get the field value for the passed index. */
    void getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data) override;
    /** set the field value for the passed index. **/
    // virtual void setFieldData(OpcUa_UInt32 index, OpcUa_Variant& data);
    
    /** Field 1 - State **/
    UaVariant m_State;
    /** Field 2 - xCentroidAvg **/
    UaVariant m_xCentroidAvg;
    /** Field 3 - yCentroidAvg **/
    UaVariant m_yCentroidAvg;
    /** Field 4 - xSpotSize **/
    UaVariant m_xCentroidSpotWidth;
    /** Field 5 - ySpotSize **/
    UaVariant m_yCentroidSpotWidth;
    /** Field 6 - Cleaned Intensity **/
    UaVariant m_CleanedIntensity;
   
    OpcUa_Int16 m_nsIdx;

private:
    static std::map<OpcUa_UInt32, OpcUa_UInt32> s_MPESEventTypeDataFields;
};

#endif //COMMON_OPCUA_MPESEVENTDATA_HPP
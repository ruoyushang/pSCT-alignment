#ifndef BAUSERDATA_H
#define BAUSERDATA_H

#include "userdatabase.h"

class BaUserData : public UserDataBase
{
    UA_DISABLE_COPY(BaUserData);
protected:
    BaUserData()
    : m_isState(StateType_Invalid),
      m_deviceAddress(0),
      m_variableOffset(0),
      m_updateTimestamp(OpcUa_False)
    {}
public:
    enum StateType
    {
        StateType_Invalid,         // The user data is invalid
        StateType_IsStateVariable, // The user data is for a state variable
        StateType_IsDataVariable   // The user data is for a data variable
    };

    BaUserData(
        OpcUa_Boolean   isState,
        OpcUa_UInt32    deviceAddress,
        OpcUa_UInt32    variableOffset)
    : m_isState(StateType_Invalid),
      m_deviceAddress(deviceAddress),
      m_variableOffset(variableOffset),
      m_updateTimestamp(OpcUa_False)
    {
        if ( isState == OpcUa_False )
        {
            m_isState = StateType_IsDataVariable;
        }
        else
        {
            m_isState = StateType_IsStateVariable;
        }
    }
    virtual ~BaUserData(){}

    /** Indicates if this is a state variable. */
    inline StateType isState() const { return m_isState; }
    /** Returns the device address. */
    inline OpcUa_UInt32 deviceAddress() const { return m_deviceAddress; }
    /** Rerturns the variable offset in the device. */
    inline OpcUa_UInt32 variableOffset() const { return m_variableOffset; }
    /** Update timestamp flag after write. */
    inline OpcUa_UInt32 updateTimestamp() const { return m_updateTimestamp; }
    inline void setUpdateTimestamp(OpcUa_Boolean updateTimestamp) { m_updateTimestamp = updateTimestamp; }

protected:
    StateType     m_isState;
    OpcUa_UInt32  m_deviceAddress;
    OpcUa_UInt32  m_variableOffset;
    OpcUa_Boolean m_updateTimestamp;
};

#endif // BAUSERDATA_H

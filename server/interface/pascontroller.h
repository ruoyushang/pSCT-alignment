/******************************************************************************
** Project: MPES server
**
** Description: implementation of MPES.
******************************************************************************/
#ifndef __PASCONTROLLER_H__
#define __PASCONTROLLER_H__

#include "uabase.h"
#include "uastring.h"
#include "statuscode.h"
#include "pascommunicationinterface.h"
#include "uamutex.h"
#include <chrono>

class Platform;

/*===========================================================================*/
/* SCTController class, general class from which other SCT devices inherit.  */
/*===========================================================================*/
class PasController
{
    UA_DISABLE_COPY(PasController);
public:
    /* construction / destruction */
    PasController(int ID, Platform *platform = nullptr, int updateInterval = 0) : m_ID(ID), m_pPlatform(platform), m_kUpdateInterval_ms(updateInterval) {}
    PasController(int ID, int updateInterval) : PasController(ID, nullptr, updateInterval) {}
    virtual bool Initialize() {return true;} // only MPES overload this for now
    int getId() { return m_ID; }
    virtual ~PasController(){}

    /* Get Controller status and data */
    virtual UaStatusCode getState(PASState& state);
    virtual UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value) = 0;

    /* set Controller status and data */
    virtual UaStatusCode setState(PASState state);
    virtual UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value) = 0;
    virtual UaStatusCode Operate(OpcUa_UInt32 offset, const UaVariantArray& args = UaVariantArray()) = 0;

protected:
    UaMutex m_mutex;
    int m_ID;
    PASState m_state;
    Platform *m_pPlatform;

    typedef std::chrono::system_clock TIME;
    const int m_kUpdateInterval_ms;
    bool _expired() const {return (std::chrono::duration_cast<std::chrono::milliseconds>(TIME::now() - m_lastUpdateTime).count() > m_kUpdateInterval_ms); }
    std::chrono::time_point<TIME> m_lastUpdateTime;
};

/*===========================================================================*/
/* Edge sensor class, part of the SCT family of devices.                     */
/*===========================================================================*/
class PasMPES : public PasController
{
    UA_DISABLE_COPY(PasMPES);
public:
    /* construction / destruction */
    PasMPES(int ID, Platform *platform);
    ~PasMPES();

    /* Initialize the device -- test if it's actually working */
    bool Initialize();

    /* Get Controller status and data */
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    /* set Controller status and data */
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray& args = UaVariantArray());

private:
    PASState m_state = PASState::PAS_Off;
    bool m_updated = false;
    
    OpcUa_Int32 read();
};

/*===========================================================================*/
/* Actuator class, part of the SCT family of devices.                        */
/*===========================================================================*/
class PasACT : public PasController
{
    UA_DISABLE_COPY(PasACT);
public:
    // construction / destruction 
    PasACT(int ID, Platform *platform);
    virtual ~PasACT();

    // Get Controller status and data 
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data 
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray& args = UaVariantArray());

private:
    PASState m_state = PASState::PAS_Off;
    OpcUa_Float m_DeltaL;
    OpcUa_Float m_inLength;

    UaStatusCode moveDelta();
};

/*===========================================================================*/
/* Panel class, part of the SCT family of devices.                        */
/*===========================================================================*/
class PasPanel : public PasController
{
    UA_DISABLE_COPY(PasPanel);
public:
    // construction / destruction
    PasPanel(int ID, Platform *platform);
    virtual ~PasPanel();

    // Get Controller status and data
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray& args = UaVariantArray());

    void addActuator(PasACT * const pActuator) { m_pActuators.push_back(pActuator); };

private:
    PASState m_state = PASState::PAS_Off;

    // vector of Actuators on this panel
    std::vector<PasACT *> m_pActuators;
};


// only include PSD if not compiling on AMD64
#ifndef _AMD64

class GASPSD;
/******************************************************/
/* Position Sensitive Device                          */
/******************************************************/
class PasPSD : public PasController
{
    UA_DISABLE_COPY(PasPSD);
public:
    /* construction / destruction */
    PasPSD(int ID);
    ~PasPSD();

    /* Get Controller status and data */
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    /* set Controller status and data */
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray& args = UaVariantArray());

private:
    PASState m_state = PASState::PAS_Off;
    GASPSD *m_pPSD;

    const int kUpdateInterval_ms = 500;
    UaStatus read();
};

#endif

#endif // #ifndef __PASCONTROLLER_H__

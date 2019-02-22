/******************************************************************************
** Project: MPES server
**
** Description: implementation of MPES.
******************************************************************************/
#ifndef _AMD64
    #include "psdclass.h"
#endif

#include "pascontroller.h"
#include "passervertypeids.h"
#include "platform.hpp"
#include <iostream>
#include <array>

using namespace std;
UaStatusCode PasController::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

UaStatusCode PasController::setState(PASState state)
{
    if ( state == PASState::PAS_Error )
    {
        return OpcUa_BadInvalidArgument;
    }
    if ( state == m_state )
    {
        return OpcUa_BadInvalidState;
    }
    UaMutexLocker lock(&m_mutex);

    m_state = state;
    return OpcUa_Good;
}

/* =================================================================== *
 *  Specific implementations from here
 * =================================================================== */

/* ----------------------------------------------------------------------------
    Begin Class    PasPanel
    we only need to implement the abstract methods.
-----------------------------------------------------------------------------*/
// no data for now, so just let these be
PasPanel::PasPanel(int ID, Platform *platform) : PasController(ID, platform)
{
    if (m_state != PASState::PAS_On)
        m_state = PASState::PAS_On;
} 

PasPanel::~PasPanel()
{
    m_pPlatform = nullptr;
    m_state = PASState::PAS_Off;
}

UaStatusCode PasPanel::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    // update internal state
    switch ( m_pPlatform->getState() ) {
        case PlatformState::On :
            m_state = PASState::PAS_On;
            break;
        case PlatformState::Off :
            m_state = PASState::PAS_Off;
            break;
        case PlatformState::Busy :
            m_state = PASState::PAS_Busy;
            break;
        case PlatformState::Error :
            m_state = PASState::PAS_Error;
            break;
        default :
            return OpcUa_BadInvalidState;
    }

    state = m_state;
    return OpcUa_Good;
}

UaStatusCode PasPanel::setState(PASState state)
{
    if ( state == PASState::PAS_Error || state == PASState::PAS_Busy) {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);

    m_state = state;
    switch ( m_state ) {
        case PASState::PAS_On :
            m_pPlatform->setState(PlatformState::On);
            break;
        case PASState::PAS_Off :
            m_pPlatform->setState(PlatformState::Off);
            break;
        default :
            return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

UaStatusCode PasPanel::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    if (offset == PAS_PanelType_ExtTemperature)
        value.setFloat(m_pPlatform->ReadInternalTemperature());
    else if (offset == PAS_PanelType_IntTemperature)
        value.setFloat(m_pPlatform->ReadExternalTemperature());
    else
        return OpcUa_BadInvalidArgument;

    return OpcUa_Good;
}

UaStatusCode PasPanel::setData(OpcUa_UInt32 offset, UaVariant value)
{
    return OpcUa_BadNotImplemented;
}

UaStatusCode PasPanel::Operate(OpcUa_UInt32 offset, const UaVariantArray& args)
{
    UaStatusCode status;

    // update the current state
    PASState dummy;
    getState(dummy);

    if (m_state == PASState::PAS_Error) {
        cout << "PasPnael::Operate(): Panel in error state! "
            << "Check what's wrong, fix it, and try again." << endl;
        return OpcUa_BadInvalidState;
    }

    if (offset == PAS_PanelType_StepAll) { // step all!
        if (m_state == PASState::PAS_Off)
            setState(PASState::PAS_On);
        else if (m_state == PASState::PAS_Busy) {
            cout << "PasPanel::Operate(): Busy at the moment. "
                << "Wait for the current operation to finish and try again." << endl;
            return OpcUa_Good;
        }

        // set the state immadiately
        m_state = PASState::PAS_Busy;

        array<float, 6> deltaLengths;

        UaVariant vTemp;
        for (int i = 0; i < 6; i++)
        {
            m_pActuators.at(i)->getData(PAS_ACTType_Steps, vTemp);
            vTemp.toFloat(deltaLengths[i]);
        }
        deltaLengths = m_pPlatform->MoveDeltaLengths(deltaLengths);
        // update missed lengths
        for (int i = 0; i < 6; i++)
        {
            vTemp.setFloat(deltaLengths[i]);
            m_pActuators.at(i)->setData(PAS_ACTType_Steps, vTemp);
        }

        status = OpcUa_Good;
    }
    else if (offset == PAS_PanelType_MoveTo_Acts) { //move to!
        if (m_state == PASState::PAS_Off)
            setState(PASState::PAS_On);
        else if (m_state == PASState::PAS_Busy) {
            cout << "PasPanel::Operate(): Busy at the moment. "
                << "Wait for the current operation to finish and try again." << endl;
            return OpcUa_Good;
        }

        // set the state immeadiately
        m_state = PASState::PAS_Busy;

        array<float, 6> lengths;
        UaVariant vTemp;
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->getData(PAS_ACTType_inLength_mm, vTemp);
            vTemp.toFloat(lengths[i]);
        }
        lengths = m_pPlatform->MoveToLengths(lengths);

        status = OpcUa_Good;
    }
    else if (offset == PAS_PanelType_Stop) {
        cout << "PasPanel::Operate(): Attempting to gracefully stop the motion." << endl;
        status = setState(PASState::PAS_Off);
    }
    else
        status = OpcUa_BadInvalidArgument;

    // update the state again
    getState(dummy);

    return status;
}


/* ----------------------------------------------------------------------------
    Begin Class    PasMPES
    constructors / destructors
-----------------------------------------------------------------------------*/
PasMPES::PasMPES(int ID, Platform *platform) : PasController(ID, platform)
{
    if (m_state != PASState::PAS_On)
        m_state = PASState::PAS_On;
}

PasMPES::~PasMPES()
{
    m_pPlatform = nullptr;
    m_state = PASState::PAS_Off;
}

bool PasMPES::Initialize()
{
    return (m_pPlatform->getMPESAt(m_ID)->setExposure() > 0);
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::getData(
    OpcUa_UInt32 offset,
    UaVariant& value)
{
    UaMutexLocker lock(&m_mutex);
    int dataoffset = offset - PAS_MPESType_xCentroidAvg;
    if ( (dataoffset >= 7) || (dataoffset < 0))
        return OpcUa_BadInvalidArgument;

    if (!m_updated)
        value.setFloat(0.);
    else
        value.setFloat(*((float *)&(m_pPlatform->getMPESAt(m_ID)->getPosition()) + dataoffset));

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::setData(
    OpcUa_UInt32 offset,
    UaVariant value)
{
    int dataoffset = offset - PAS_MPESType_xCentroidAvg;
    if ( (dataoffset == 5) || (dataoffset == 6))
    {
        value.toFloat(*((float *)&(m_pPlatform->getMPESAt(m_ID)->getPosition()) + dataoffset));
        return OpcUa_Good;
    }

    return OpcUa_BadNotWritable;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::Operate(OpcUa_UInt32 offset, const UaVariantArray& args)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;
    
    switch ( offset )
    {
        case PAS_MPESType_Read:
            status = read();
            break;
        case PAS_MPESType_SetExposure:
            m_pPlatform->getMPESAt(m_ID)->setExposure();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       read
    Description  Read Controller data.
-----------------------------------------------------------------------------*/
OpcUa_Int32 PasMPES::read()
{
    UaMutexLocker lock(&m_mutex);

    if ( m_state == PASState::PAS_On )
    {
        printf("\nReading MPES %d\n", m_ID);
        m_pPlatform->ReadMPES(m_ID);
        m_updated = true;
        return 0;
    }
    else
        m_updated = false;

    return -1;
}
/* ----------------------------------------------------------------------------
    End Class    PasMPES
==============================================================================*/

/* ===========================================================================
    Begin Class    PasACT
    constructors / destructors
-----------------------------------------------------------------------------*/

PasACT::PasACT(int ID, Platform *platform) : PasController(ID, platform)
{
    m_state = PASState::PAS_On;
    // initialize m_inLength and m_DeltaL
    m_inLength = m_pPlatform->getActuatorAt(m_ID)->MeasureLength();
    m_DeltaL = 0.;
}

PasACT::~PasACT()
{
    m_pPlatform = nullptr;
    m_state = PASState::PAS_Off;
}

/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::setState(PASState state)
{
    // do not lock the object -- might want to access it during stepping!
    // UaMutexLocker lock(&m_mutex);

    return OpcUa_BadNotImplemented;
    if ( state == PASState::PAS_Error )
    {
        return OpcUa_BadInvalidArgument;
    }
    if ( state == m_state )
    {
        return OpcUa_BadInvalidState;
    }
    m_state = state;
    /*
    if ( m_state == PASState::PAS_Off )
        m_pPlatform->getActuatorAt(m_ID)->SetStatus(Actuator::StatusModes::);
    else if ( m_state == PASState::PAS_On )
        m_pPlatform->getActuatorAt(m_ID)->SetStatus(1);
    */

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::getData(
    OpcUa_UInt32 offset,
    UaVariant& value)
{
    int dataoffset = offset - PAS_ACTType_Steps;
    if ( (dataoffset >= 3) || (dataoffset < 0) )
    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);

    switch ( dataoffset )
    {
    case 0:
        value.setFloat(m_DeltaL);
        break;
    case 1:
        {
            value.setFloat(m_pPlatform->getActuatorAt(m_ID)->MeasureLength());
            break;
        }
    case 2:
        value.setFloat(m_inLength);
        break;
    }

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::setData(
    OpcUa_UInt32 offset,
    UaVariant value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;

    if (offset == PAS_ACTType_Steps) {
        value.toFloat(m_DeltaL);
        status = OpcUa_Good;
    }
    else if (offset == PAS_ACTType_inLength_mm) {
        value.toFloat(m_inLength);
        status = OpcUa_Good;
    }
    else
        status = OpcUa_BadNotWritable;

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       Operate
    Description  run a method on the actuator
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::Operate(OpcUa_UInt32 offset, const UaVariantArray& args)
{
    // do not lock the object -- might want to access it during stepping!
    // UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;
    
    if ( offset >= 1 )
        return OpcUa_BadInvalidArgument;

    switch ( offset )
    {
        case 0:
            status = moveDelta();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       move
    Description  Step the actuator
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::moveDelta()
{
    // do not lock the object -- might want to access it during stepping!
    // UaMutexLocker lock(&m_mutex);

    if ( !(m_state == PASState::PAS_On) )
        return OpcUa_BadNothingToDo;

    array<float, 6> deltaL = {0., 0., 0., 0., 0., 0.};
    deltaL[m_ID] = m_DeltaL;
    cout << "moving actuator " << m_ID << " by " << m_DeltaL << " mm" << endl;
    deltaL = m_pPlatform->MoveDeltaLengths(deltaL);
    m_DeltaL = deltaL[m_ID];

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    END Class    PasACT
    constructors / destructors
==============================================================================*/

// do not include PSD if compiled on AMD 64
#ifndef _AMD64

PasPSD::PasPSD(int ID) : PasController(ID, 500)
{
    if (m_state != PASState::PAS_On)
        m_state = PASState::PAS_On;
    m_pPSD = new GASPSD();
    m_pPSD->setPort();
    m_pPSD->Initialize();
    m_mutex.unlock();
} 

PasPSD::~PasPSD()
{
    if (m_pPSD) {
        delete m_pPSD;
        m_pPSD = nullptr;
    }
    m_state = PASState::PAS_Off;
}

UaStatusCode PasPSD::getData(
    OpcUa_UInt32 offset,
    UaVariant& value)
{
    //UaMutexLocker lock(&m_mutex);
    UaStatus status = OpcUa_Good;

    int dataoffset = offset - PAS_PSDType_x1;
    if ( (dataoffset >= 9) || (dataoffset < 0))
        return OpcUa_BadInvalidArgument;

    if (_expired()) // if cached value expired, update it
        status = read();
    
    value.setDouble(*(m_pPSD->getOutput() + dataoffset));

    return status;
}

UaStatusCode PasPSD::setData(
    OpcUa_UInt32 offset,
    UaVariant value)
{
    return OpcUa_BadNotWritable;
}

UaStatusCode PasPSD::Operate(OpcUa_UInt32 offset, const UaVariantArray& args)
{
    //UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;
    
    if ( offset >= 1 )
        return OpcUa_BadInvalidArgument;

    switch ( offset )
    {
        case 0:
            status = read();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

UaStatus PasPSD::read()
{
    //UaMutexLocker lock(&m_mutex);

    if ( m_state == PASState::PAS_On )
    {
        printf("\nReading PSD %d\n", m_ID);
        m_pPSD->Update();
        m_lastUpdateTime = TIME::now();
        return OpcUa_Good;
    }
    else
        return OpcUa_BadOutOfService;

    return OpcUa_BadUnexpectedError;
}

#endif

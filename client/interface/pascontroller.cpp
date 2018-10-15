/******************************************************************************
** Project: MPES server
**
** Description: implementation of MPES.
******************************************************************************/
#include "pascontroller.h"
#include "ccdclass.h"
#include "clienthelper.h"
#include "components.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstdlib> // system
#include <unistd.h> // usleep

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"

using namespace Eigen;
using namespace std;

int PasController::kUpdateInterval = 0;
// 1 s update interval for panels
int PasPanel::kUpdateInterval = 1000;
// 500 ms update interval for PSDs
int PasPSD::kUpdateInterval = 500;

// implement PasCompositeController::addChild()
void PasCompositeController::addChild(OpcUa_UInt32 deviceType, PasController *const pController)
{
    cout << " --- " << m_ID.name << "::addChild(): "
        << m_ID.name << ": adding " << pController->getId() << endl;

    auto id = pController->getId();
    auto pos = pController->getId().position;
    // don't add the same device multiple times
    try {
        // m_ChildrenPositionMap.at(deviceType).at(pos);
        m_ChildrenIdentityMap.at(deviceType).at(id);
        cout << "\tAlready added. Moving on..." << endl;

        return;
    }
    catch (out_of_range) {
        // only add if this is a possible child
        if (m_ChildrenTypes.count(deviceType)) {
            m_pChildren[deviceType].push_back(pController);
            m_ChildrenIdentityMap[deviceType][id] = m_pChildren.at(deviceType).size() - 1;
            // this doesn't work for edges, since they don't have an assigned position
            m_ChildrenPositionMap[deviceType][pos] = m_pChildren.at(deviceType).size() - 1;
        }
    }

    return;
}

/* ----------------------------------------------------------------------------
    Begin Class    PasMPES
    constructors / destructors
-----------------------------------------------------------------------------*/
PasMPES::PasMPES(Identity identity, Client *pClient) : PasController(identity, pClient),
    m_updated(false)
{
    m_state = PASState::PAS_On;

    // get the nominal aligned readings and response matrices from DB
    /* BEGIN DATABASE HACK */
    //string db_ip="10.0.50.114";
    //string db_port="3406";
    //string db_user="CTAreadonly";
    //string db_password="readCTAdb";
    //string db_name="CTAonline";
    //string db_address = "tcp://" + db_ip + ":" + db_port;
    //Ruo
    std::string db_ip="remus.ucsc.edu";
    std::string db_port="3406";
    std::string db_user="CTAreadonly";
    std::string db_password="readCTAdb";
    std::string db_name="CTAonline";
    std::string db_address = "tcp://" + db_ip + ":" + db_port;

    cout << "Initializing MPES " << m_ID.serialNumber << endl;
    try {
        sql::Driver *sql_driver = get_driver_instance();
        sql::Connection *sql_conn = sql_driver->connect(db_address, db_user, db_password);
        sql_conn->setSchema(db_name);
        sql::Statement *sql_stmt = sql_conn->createStatement();
        sql::ResultSet *sql_results;

        string query("");
        // obtain topological data
        query = "SELECT w_panel, l_panel FROM Opt_MPESMapping WHERE serial_number=" + to_string(m_ID.serialNumber);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();
        while (sql_results->next()) {
            m_PanelSideMap[sql_results->getInt(1)] = 'w';
            m_PanelSideMap[sql_results->getInt(2)] = 'l';
        }

        // build query configuration -- obtain coord, nominal reading and response matrix elements
        query.clear();
        const char panelType[2] = {'w', 'l'};
        for (const auto& panel : panelType)
            for (int act = 1; act <= 6; act++)
                query += ", " + string(1, panel) + "_response_actuator" + to_string(act);
        query = "SELECT coord, nominal_reading" + query + " FROM Opt_MPESConfigurationAndCalibration WHERE serial_number=" + to_string(m_ID.serialNumber);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();

        while (sql_results->next()) {
            char coord = sql_results->getString(1)[0];
            m_AlignedReadings(int(coord - 'x')) = sql_results->getDouble(2);
            for (int panel = 0; panel < 2; panel++) {
                for (int act = 1; act <= 6; act++)
                    m_ResponseMatMap[panelType[panel]](int(coord-'x'), act-1) =
                        sql_results->getDouble(2 + panel*6 + act);
            }
        }

        // print out the loaded values
        cout << "\t Aligned readings:\n" << m_AlignedReadings << endl;
        for (const auto& matrixPair : m_ResponseMatMap)
            cout << "\t " << matrixPair.first << "-side response matrix:\n"
                << matrixPair.second << endl;

        // pass the aligned readings on to the server
        UaVariant value;
        value.setDouble(m_AlignedReadings(0));
        setData(PAS_MPESType_xCentroidNominal, value);
        value.setDouble(m_AlignedReadings(1));
        setData(PAS_MPESType_yCentroidNominal, value);

        // close the connection!
        sql_conn->close();
        delete sql_conn;
        sql_driver->threadEnd();
    }
    catch (sql::SQLException &e) {
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;

        // set state to error
        m_state = PASState::PAS_Error;
    }
    /* END DATABASE HACK */
}

PasMPES::~PasMPES()
{
    m_pClient = nullptr;
    m_state = PASState::PAS_Off;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::setState(PASState state)
{
    UaStatus status;

    if ( state == m_state )
    {
        return OpcUa_BadInvalidState;
    }
    UaMutexLocker lock(&m_mutex);

    m_state = state;
    if ( state == PASState::PAS_Off )
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Stop"));
    else if ( state == PASState::PAS_On )
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Start"));
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    UaStatus status;
    UaMutexLocker lock(&m_mutex);

    int dataoffset = offset - PAS_MPESType_xCentroidAvg;
    if ( (dataoffset >= 7) || (dataoffset < 0))
        return OpcUa_BadInvalidArgument;

    if (!m_updated)
        status = read();

    // cast struct to double through reinterpret_cast!
    value.setDouble(*(reinterpret_cast<OpcUa_Double *>(&data) + dataoffset));

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::setData(OpcUa_UInt32 offset, UaVariant value)
{
    UaStatus status;

    int dataoffset = offset - PAS_MPESType_xCentroidAvg;
    if ( (dataoffset > 6) | (dataoffset < 5))
        return OpcUa_BadNotWritable;

    string varstowrite[2] {".xCentroidNominal", ".yCentroidNominal"};
    vector<string> vec_curwrite {m_ID.eAddress + varstowrite[dataoffset - 5]};

    // set local variable
    value.toDouble(*(reinterpret_cast<OpcUa_Double *>(&data) + dataoffset));
    // and write it to the server
    status = m_pClient->write(vec_curwrite, &value);

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatusCode PasMPES::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;

    if ( offset >= 1 )
        return OpcUa_BadInvalidArgument;

    return read();
}
/* ----------------------------------------------------------------------------
    Class        PasMPES
    Method       read
    Description  Read Controller data.
-----------------------------------------------------------------------------*/
UaStatus PasMPES::read()
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    cout << "calling read() on " << m_ID << endl;
    if ( m_state == PASState::PAS_On )
    {
        // read the values on the server first
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Read"));
        if (!status.isGood()) return status;
        // get the updated values from the server

        // get all the updated values from the server
        std::vector<std::string> varstoread {
            "xCentroidAvg",
            "yCentroidAvg",
            "xCentroidSD",
            "yCentroidSD",
            "CleanedIntensity",
            "xCentroidNominal",
            "yCentroidNominal"};
        std::transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
                    [this](std::string& str){ return m_ID.eAddress + "." + str;});
        UaVariant valstoread[7];

        status = m_pClient->read(varstoread, &valstoread[0]);
        if (status.isGood()) m_updated = true;

        for (unsigned i = 0; i < varstoread.size(); i++)
            valstoread[i].toDouble(*(reinterpret_cast<OpcUa_Double *>(&data) + i));
    }
    else
        m_updated = false;


    return status;
}

char PasMPES::getPanelSide(unsigned panelpos)
{
    char panelside;
    try {
        panelside = m_PanelSideMap.at(panelpos);
    }
    catch (out_of_range) {
        panelside = 0;
    }

    return panelside;
}

/* ----------------------------------------------------------------------------
    End Class    PasMPES
==============================================================================*/

/* ===========================================================================
    Begin Class    PasACT
    constructors / destructors
-----------------------------------------------------------------------------*/

PasACT::PasACT(Identity identity, Client *pClient) : PasController(identity, pClient),
    m_DeltaL {0.}
{
    m_state = PASState::PAS_On;
}

PasACT::~PasACT()
{
    m_pClient = nullptr;
    m_state = PASState::PAS_Off;
}

/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::getState(PASState& state)
{
    state = m_state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::setState(PASState state)
{
    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if ( state == m_state )
    {
        return OpcUa_BadInvalidState;
    }
    m_state = state;
    if ( state == PASState::PAS_Off )
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Stop"));
    else if ( state == PASState::PAS_On )
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Start"));
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    int dataoffset = offset - PAS_ACTType_Steps;
    if ( (dataoffset >= 3) || (dataoffset < 0) )

    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    string varstoread[3] {"Steps", "curLength_mm", "inLength_mm"};

    vector<string> vec_curread {m_ID.eAddress + "." + varstoread[dataoffset]};
    status = m_pClient->read(vec_curread, &value);

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasACT
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasACT::setData(OpcUa_UInt32 offset, UaVariant value)
{
    UaStatusCode  status;

    if (offset == PAS_ACTType_Steps) {
        vector<string> vec_curwrite {m_ID.eAddress + ".Steps"};
        value.toFloat(m_DeltaL);
        status = m_pClient->write(vec_curwrite, &value);
    }
    else if (offset == PAS_ACTType_inLength_mm) {
        vector<string> vec_curwrite {m_ID.eAddress + ".inLength_mm"};
        status = m_pClient->write(vec_curwrite, &value);
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
UaStatusCode PasACT::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaStatusCode  status;

    if ( offset >= 1 )
        return OpcUa_BadInvalidArgument;

    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);
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
    Method       step
    Description  Step the actuator
-----------------------------------------------------------------------------*/
UaStatus PasACT::moveDelta()
{
    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);
    if ( m_state == PASState::PAS_On )
    {
        printf("stepping actuator %d by %5.3f mm\n", m_ID.serialNumber, m_DeltaL);
        return m_pClient->callMethodAsync(m_ID.eAddress, UaString("Step"));
    }

    return OpcUa_BadInvalidState;
}
/* ----------------------------------------------------------------------------
    END Class    PasACT
==============================================================================*/

/* ===========================================================================
    Begin Class    PasPanel
    constructors / destructors
-----------------------------------------------------------------------------*/

// update interval set to 500ms -- sample every half a second
PasPanel::PasPanel(Identity identity, Client *pClient) :
    PasCompositeController(identity, pClient, kUpdateInterval),
    m_inCoordsUpdated(false)
{
    m_ID.name = string("Panel_") + to_string(m_ID.position);
    m_state = PASState::PAS_On;
    m_SP.SetPanelType(StewartPlatform::PanelType::OPT);

    // define possible children types
    m_ChildrenTypes = {PAS_ACTType, PAS_MPESType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratiot
    m_lastUpdateTime = TIME::now() - chrono::duration<int, ratio<1, 1000>>(m_UpdateInterval_ms);
}

PasPanel::~PasPanel()
{
    m_pClient = nullptr;

    for (auto& devVector : m_pChildren)
       for (auto& dev : devVector.second)
           dev = nullptr;

    m_state = PASState::PAS_Off;
}

unsigned PasPanel::getActuatorCount()
{
    try {
        return m_pChildren.at(PAS_ACTType).size();
    }
    catch (out_of_range) {
        return 0;
    }
}

UaStatusCode PasPanel::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    UaVariant val;
    unsigned intState;

    vector<string> vec_curread {"ns=2;s=Panel_0.State"};
    status = m_pClient->read(vec_curread, &val);
    if (!status.isGood())
        return status;

    val.toUInt32(intState);
    switch ( intState ) {
        case static_cast<unsigned>(PASState::PAS_On) :
            m_state = PASState::PAS_On;
            break;
        case static_cast<unsigned>(PASState::PAS_Off) :
            m_state = PASState::PAS_Off;
            break;
        case static_cast<unsigned>(PASState::PAS_Busy) :
            m_state = PASState::PAS_Busy;
            break;
        case static_cast<unsigned>(PASState::PAS_Error) :
            m_state = PASState::PAS_Error;
            break;
        default:
            return OpcUa_BadInvalidState;
    }

    state = m_state;

    return status;

}


UaStatusCode PasPanel::setState(PASState state)
{
    if ( state == PASState::PAS_Error )
    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);
    m_state = state;
    return OpcUa_Good;
}

UaStatusCode PasPanel::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status = OpcUa_Good;

    if (getActuatorCount() == 0) {
        cout << m_ID << "::getData() : no actuators, data undefined." << endl;
        return OpcUa_Good;
    }

    if (offset >= PAS_PanelType_curCoords_x && offset <= PAS_PanelType_curCoords_zRot) {
        // update current coordinates
        __updateCoords();
        int dataoffset = offset - PAS_PanelType_curCoords_x;
        value.setDouble(m_curCoords[dataoffset]);
    }
    else if (offset >= PAS_PanelType_inCoords_x && offset <= PAS_PanelType_inCoords_zRot) {
        // update coordinates on initial boot
        if (!m_inCoordsUpdated) {
            __updateCoords();
            for (int i = 0; i < 6; i++)
                m_inCoords[i] = m_curCoords[i];
            m_inCoordsUpdated = true;
        }

        int dataoffset = offset - PAS_PanelType_inCoords_x;
        value.setDouble(m_inCoords[dataoffset]);
    }
    else if (offset == PAS_PanelType_IntTemperature)
        status = m_pClient->read({"ns=2;s=Panel_0.InternalTemperature"}, &value);
    else if (offset == PAS_PanelType_ExtTemperature)
        status = m_pClient->read({"ns=2;s=Panel_0.ExternalTemperature"}, &value);

    else
        status = OpcUa_BadInvalidArgument;

    return status;
}

UaStatusCode PasPanel::setData(OpcUa_UInt32 offset, UaVariant value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status = OpcUa_Good;

    // update coordinates on initial boot
    if (!m_inCoordsUpdated) {
        __updateCoords();
        for (int i = 0; i < 6; i++)
            m_inCoords[i] = m_curCoords[i];
        m_inCoordsUpdated = true;
    }

    if (offset >= PAS_PanelType_inCoords_x && offset <= PAS_PanelType_inCoords_zRot) {
        int dataoffset = offset - PAS_PanelType_inCoords_x;
        value.toDouble(m_inCoords[dataoffset]);
    }
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}

// move actuators to the preset length or panel to the preset coords
UaStatusCode PasPanel::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;

    if (getActuatorCount() == 0) {
        cout << m_ID << "::Operate() : no actuators, nothing to be done." << endl;
        return OpcUa_Good;
    }

    // update the state
    PASState dummy;
    getState(dummy);

    if (offset != PAS_PanelType_Stop) {
        if (m_state == PASState::PAS_Error)
            cout << m_ID << "::Operate() : Current state is 'Error'! This won't do anything." << endl;
        if (m_state == PASState::PAS_Busy)
            cout << m_ID << "::Operate() : Current state is 'Busy'! This won't do anything." << endl;
    }

    auto& actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto& pACT = m_pChildren.at(PAS_ACTType);


    /************************************************
     * move actuators to the preset lengths         *
     * **********************************************/
    if (offset == PAS_PanelType_MoveTo_Acts) {
        status =  __moveTo();
    }


    /************************************************
     * move the panel to the preset coordinates     *
     * **********************************************/
    else if (offset == PAS_PanelType_MoveTo_Coords) {
        // find actuator lengths needed
        m_SP.ComputeActsFromPanel(m_inCoords);
        // set actuator lengths
        UaVariant val;
        for (const auto& act : actuatorPositionMap) {
            val.setDouble(m_SP.GetActLengths()[act.first - 1]);
            pACT.at(act.second)->setData(PAS_ACTType_inLength_mm, val);
        }
        status =  __moveTo();
        PASState state;
        getState(state);
        cout << "Current State is " << static_cast<unsigned>(m_state) << " and it is equivalent to PASState::PAS_Busy : " << (state == PASState::PAS_Busy) << endl;
    }


    /************************************************
     * move all actuators by the specified deltas   *
     * **********************************************/
    else if (offset == PAS_PanelType_StepAll || offset == PAS_PanelType_StepAll_move) {
        cout << endl << m_ID << ":" << endl;
        if (args.length() != pACT.size())
            return OpcUa_BadInvalidArgument;

        double actLengths[6];
        UaVariant val;
        __updateCoords();
        cout << "\tCurrent panel coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (int i = 0; i < 6; i++)
            cout << m_curCoords[i] << " ";
        cout << endl << endl;

        for (const auto& act : actuatorPositionMap) {
            cout << "\t\tThis will change the length of Actuator "
                << pACT.at(act.second)->getId().serialNumber
                << " at position " << act.first << " by " << args[act.first - 1].Value.Float
                << " mm" << endl;

            // current actuator lengths were updated in __updateCoords();
            actLengths[act.first - 1] = m_ActuatorLengths(act.first - 1) + args[act.first - 1].Value.Float;
            val.setFloat(actLengths[act.first - 1]);
            status = pACT.at(act.second)->setData(PAS_ACTType_inLength_mm, val);
            if (!status.isGood()) return status;

        }
        m_SP.ComputeStewart(actLengths);
        cout << "\n\tThe new panel coordinates (x, y ,z xRot, yRot, zRot) will be:\n\t\t";
        for (int i = 0; i < 6; i++) {
            cout << m_SP.GetPanelCoords()[i] << " ";
            m_inCoords[i] = m_SP.GetPanelCoords()[i];
        }
        cout << endl;
        cout << "Input actuator lengths and input panel coords updated accordingly. "
            << "Call " << m_ID.name << ".MoveToActs or .MoveToCoords to move this panel." << endl;

        if (offset == PAS_PanelType_StepAll_move) { // actually move the panel
            status = m_pClient->callMethod(string("ns=2;s=Panel_0"), UaString("MoveTo"));
        }
    }

    /************************************************
     * read current position and actuator lengths   *
     * **********************************************/
    else if (offset == PAS_PanelType_Read) {
        UaVariant val;
        __updateCoords();
        cout << endl << m_ID << " :" << endl;
        cout << "\tCurrent coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (int i = 0; i < 6; i++)
            cout << m_curCoords[i] << " ";
        cout << endl << endl;

        cout << "\tCurrent actuator lengths:\n";
        // map is ordered by the first index, in our case, the position
        for (const auto& i : actuatorPositionMap) {
            cout << "\t\t(" << pACT.at(i.second)->getId().serialNumber << ", " << i.first << ") : "
                << m_ActuatorLengths(i.first - 1) << " mm" << endl;
        }

        status = OpcUa_Good;
    }


    /************************************************
     * stop the motion in progress                  *
     * **********************************************/
    else if (offset == PAS_PanelType_Stop) {
        cout << m_ID << "::Operate() : Attempting to gracefully stop the motion." << endl;
        status = m_pClient->callMethod(string("ns=2;s=Panel_0"), UaString("Stop"));
    }
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}


// actual execution method to move the panel
UaStatus PasPanel::__moveTo()
{
    UaVariant val;
    float in_length = 0.;
    UaStatus status;

    // this should have already been caught, but let's do it nevertheless
    // nothing to be done, but all good
    if (getActuatorCount() == 0)
        return OpcUa_Good;

    auto& actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto& pACT = m_pChildren.at(PAS_ACTType);

    // map is ordered by the first index, in our case, the position
    for (const auto& i : actuatorPositionMap) {
        pACT.at(i.second)->getData(PAS_ACTType_inLength_mm, val);
        val.toFloat(in_length);
        cout << "Will move actuator " << pACT.at(i.second)->getId().serialNumber
            << " at position " << m_ID.position
            << "." << i.first << " to " << in_length << " mm" << endl;
    }

    status = m_pClient->callMethodAsync(string("ns=2;s=Panel_0"), UaString("MoveTo"));

    return status;
}

void PasPanel::__updateCoords(bool printout)
{
    // do nothing if values haven't expired
    if (!__expired())
        return;

    UaVariant val;
    m_ActuatorLengths.resize(getActuatorCount());

    auto& actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto& pACT = m_pChildren.at(PAS_ACTType);

    for (const auto& i : actuatorPositionMap)
    {
        pACT.at(i.second)->getData(PAS_ACTType_curLength_mm, val);
        val.toDouble(m_ActuatorLengths(i.first - 1));

        if (printout) {
            cout << "Actual length of actuator " << pACT.at(i.second)->getId().serialNumber
                << " at position " << i.first << " is " << m_ActuatorLengths(i.first - 1) << " mm" << endl;
        }
    }
    // update current coordinates
    m_SP.ComputeStewart(m_ActuatorLengths.data());
    // panel coords
    for (int i = 0; i < 6; i++)
        m_curCoords[i] = m_SP.GetPanelCoords()[i];

    // pad coords -- each column corresponds to a pad
    for (int pad = 0; pad < 3; pad++)
        // populate panel frame pad coordinates
        for (int coord = 0; coord < 3; coord++)
            m_PadCoords.col(pad)(coord) = m_SP.GetPadCoords(pad)[coord];

    m_lastUpdateTime = TIME::now();
}


// helper function that updates actuator steps
// unsafe -- expects args to be of the right size
void PasPanel::getActuatorSteps(UaVariantArray &args) const
{
    auto& actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto& pACT = m_pChildren.at(PAS_ACTType);

    // set args to missed steps:
    UaVariant vtmp;
    for (const auto& act : actuatorPositionMap) {
        pACT.at(act.second)->getData(PAS_ACTType_Steps, vtmp);
        vtmp.copyTo(&args[act.first - 1]);
    }
}

/* ----------------------------------------------------------------------------
    END Class    PasPanel
==============================================================================*/

/* ===========================================================================
    Begin Class    PasEdge
    constructors / destructors
-----------------------------------------------------------------------------*/

PasEdge::PasEdge(Identity identity) : PasCompositeController(identity, nullptr),
    m_DeltaL {0.5}, m_responseMatUpdated(false), m_alignmentUpdated(false), m_isAligned(false)
{
    m_ID.name = string("Edge_") + m_ID.eAddress;
    m_state = PASState::PAS_On;
    // defin possible children types
    m_ChildrenTypes = {PAS_MPESType, PAS_PanelType};

    m_SystematicOffsets = VectorXd(6);
    m_SystematicOffsets.setZero();
}

PasEdge::~PasEdge()
{
    m_pClient = nullptr;

    for (auto& devVector : m_pChildren)
       for (auto& dev : devVector.second)
           dev = nullptr;

    m_state = PASState::PAS_Off;
}

/* ----------------------------------------------------------------------------
    Class        PasEdge
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasEdge::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasEdge
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasEdge::setState(PASState state)
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
/* ----------------------------------------------------------------------------
    Class        PasEdge
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasEdge::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status = OpcUa_Good;

    if (offset == PAS_EdgeType_StepSize)
        value.setFloat(m_DeltaL);
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasEdge
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasEdge::setData(OpcUa_UInt32 offset, UaVariant value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status = OpcUa_Good;

    if (offset == PAS_EdgeType_StepSize)
        value.toFloat(m_DeltaL);
    else if (offset >= PAS_MirrorType_sysOffsets_x && offset <= PAS_MirrorType_sysOffsets_zRot) {
        int dataoffset = offset - PAS_MirrorType_sysOffsets_x;
        value.toDouble(m_SystematicOffsets(dataoffset));
    }
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasEdge
    Method       Operate
    Description  run a method on the actuator
-----------------------------------------------------------------------------*/
UaStatusCode PasEdge::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;

    for(auto elem :m_pChildren)
    {
           std::cout << "Ruo, child of an edge: " << elem.first << "\n";
           for (auto elem2nd :elem.second)
           {
                    std::cout << " " << elem2nd->getId() << "\n";
           }
    }

    unsigned numPanels;
    try {
        numPanels = m_pChildren.at(PAS_PanelType).size();
        m_pChildren.at(PAS_MPESType).size();
    }
    catch (out_of_range) {
        // this should never happen -- and edge should never exist without panels
        cout << m_ID << "::findMatrix() : no panels or sensors, nothing to do. "
            << "This should never happen, by the way. Just saying." << endl;
        return OpcUa_BadInvalidState;
    }

    switch ( offset )
    {
        case PAS_EdgeType_FindMatrix:
            status = findMatrix();
            break;
        case PAS_EdgeType_Align:
            {
                // input args should have been validated by this point.

                // this is the logic:
                //      if edge has 2 panels, things are unambiguous -- move the
                //      panel specified by the panel_move value (arg[0]) keeping
                //      the panel_fix (arg[1]) panel fixed.
                //
                //      if the edge has 3 panels, things become a bit more ambiguous,
                //      but there are still only two possibilities:
                //      keep the P1 panel fixed while moving the P2 panels;
                //      or move the P1 panel while keeping the two P2 panels fixed.
                //      so in this case, we're looking for which of the two args is
                //      the P1 panel. If neither is a P1 panel, the input is bad.

                OpcUa_UInt32 panel;
                bool moveit;

                if (numPanels == 2) {
                    panel = args[0].Value.UInt32;
                    moveit = true;
                }
                else {
                    // panel ring is the decimal value -- 1 for inner; 2 for outer:
                    //              panelRing = (panel_pos / 10) % 10;
                    if ( ((panel = args[0].Value.UInt32) / 10 ) % 10 == 1)
                        // the panel_move panel is P1
                        moveit = true;
                    else if ( ((panel = args[1].Value.UInt32) / 10 ) % 10 == 1)
                        // the panel_fix panel is P1
                        moveit = false;
                    else {
                        status = OpcUa_BadInvalidArgument;
                        break;
                    }
                }

                status = align(panel, moveit);
                break;
            }
        case PAS_EdgeType_Read:
            // update current and target readings and print them out
            cout << "\n" << m_ID.name << " :" << endl;;
            getCurrentReadings();
            getAlignedReadings();
            cout << "\nCurrent MPES readings:\n" << m_CurrentReadings << endl;
            cout << "\nTarget MPES readings:\n" << m_AlignedReadings << endl << endl;
            status = OpcUa_Good;
            break;
        case PAS_EdgeType_Move:
            {
                // move assigned panels
                for (auto panel2move : m_PanelsToMove) {
                    auto panel = m_pChildren.at(PAS_PanelType).at(panel2move);
                    panel->Operate(PAS_PanelType_MoveTo_Acts);
                }
                // motion done, don't risk repeating it
                m_PanelsToMove.clear();
                break;
            }
        case PAS_EdgeType_Stop:
            {
                // stop motion of all panels
                for (const auto& panel : m_pChildren.at(PAS_PanelType))
                    panel->Operate(PAS_PanelType_Stop);
                break;
            }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        PasEdge
    Method       findMatrix
    Description  Find the alignment matrix for this edge
-----------------------------------------------------------------------------*/
UaStatus PasEdge::findMatrix()
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    // safety of this has already been checked by the caller
    unsigned numPanels = m_pChildren.at(PAS_PanelType).size();

    for (unsigned i = 0; i < numPanels; i++) {
        status = __findSingleMatrix(i);
        if (!status.isGood())
            return status;
    }

    return status;
}

// helper method for the above -- actually moving the panel and measuring the matrix
UaStatus PasEdge::__findSingleMatrix(unsigned panelidx)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    double responseMatrix[6][6];;
    // convenience variable;
    // no need to check with a try/catch block anymore as this has already been done
    // by the caller
    auto& pMPES = m_pChildren.at(PAS_MPESType);
    PasPanel *pCurPanel = static_cast<PasPanel *>(m_pChildren.at(PAS_PanelType).at(panelidx));
    unsigned nACTs = pCurPanel->getActuatorCount();

    OpcUa_Double vector0[6]; // maximum possible size
    OpcUa_Double vector1[6]; // maximum possible size

    UaVariant minusdeltaL, deltaL, zeroDelta;
    deltaL.setFloat(m_DeltaL);
    minusdeltaL.setFloat(-1*m_DeltaL);
    zeroDelta.setFloat(0.);

    UaVariantArray deltas;
    deltas.create(nACTs);

    UaVariant vtmp;
    float missedDelta;

    for (unsigned j = 0; j < nACTs; j++) {
        // extra safety -- initialize array to zero every time
        for (unsigned k = 0; k < nACTs; k++)
            zeroDelta.copyTo(&deltas[k]);

        missedDelta = 0.;
        for (unsigned i = 0; i < pMPES.size(); i++) {
            status = pMPES.at(i)->Operate();
            if (!status.isGood()) return status;
            pMPES.at(i)->getData(PAS_MPESType_xCentroidAvg, vtmp);
            vtmp.toDouble(vector0[2*i]);
            pMPES.at(i)->getData(PAS_MPESType_yCentroidAvg, vtmp);
            vtmp.toDouble(vector0[2*i + 1]);
        }
        printf("attempting to move actuator %d by %5.3f mm\n", j, m_DeltaL);
        deltaL.copyTo(&deltas[j]);
        status = pCurPanel->Operate(PAS_PanelType_StepAll_move, deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        PASState curState = PASState::PAS_Busy;
        while (curState == PASState::PAS_Busy) {
            usleep(50*1000); // microseconds
            pCurPanel->getState(curState);
        }

        // update missed steps
        pCurPanel->getActuatorSteps(deltas);
        missedDelta = deltas[j].Value.Float;

        printf("actuator %d missed target by %5.3f mm\n", j, missedDelta);

        for (unsigned i = 0; i < pMPES.size(); i++) {
            status = pMPES.at(i)->Operate();
            if (!status.isGood()) return status;
            pMPES.at(i)->getData(PAS_MPESType_xCentroidAvg, vtmp);
            vtmp.toDouble(vector1[2*i]);
            pMPES.at(i)->getData(PAS_MPESType_yCentroidAvg, vtmp);
            vtmp.toDouble(vector1[2*i + 1]);
        }
        // move the same actuator back
        minusdeltaL.copyTo(&deltas[j]);
        status = pCurPanel->Operate(PAS_PanelType_StepAll_move, deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        curState = PASState::PAS_Busy;
        while (curState == PASState::PAS_Busy) {
            usleep(50*1000); // microseconds
            pCurPanel->getState(curState);
        }

        for (unsigned i = 0; i < 2*pMPES.size(); i++)
        {
            responseMatrix[i][j]=(vector1[i]-vector0[i])/(m_DeltaL - missedDelta);
            printf("[%d,%d]= [%6.4lf,%6.4lf,%6.4lf]\n", i+1, j+1,
                    vector1[i], vector0[i], responseMatrix[i][j]);
        }
    }

    string outfilename = "/home/ctauser/PanelAlignmentData/ResponseMatrix.txt";
    ofstream output(outfilename, ios_base::app);
    stringstream outputstr;

    for (unsigned i = 0; i < pMPES.size(); i++) {
        for (auto const& coord : {'x', 'y'}) {
            // explicit cast here
            char panelside = static_cast<PasMPES *>(pMPES.at(i))->getPanelSide(pCurPanel->getId().position);

            // prepare command for DB
            outputstr.str(string());
            outputstr << "addResponseMatrix2db "
                << pMPES.at(i)->getId().serialNumber << " " << coord << " " << panelside;
            outputstr << " \""; // begin quoted list of values
            // -------------------- //

            output << pMPES.at(i)->getId().serialNumber << " " << coord << " " << panelside;
            for (unsigned j = 0; j < nACTs; j++) {
                output << " " << responseMatrix[2*i + int(coord - 'x')][j];
                outputstr << " " << responseMatrix[2*i + int(coord - 'x')][j];
            }
            output << endl;

            outputstr << "\""; //end quoted list of values
            system(outputstr.str().c_str());
        }
    }
    output.close();

    return status;
}

/* ----------------------------------------------------------------------------
    Class        PasEdge
    Method       align
    Description  Align this edge to the nominal position using the found matrix
-----------------------------------------------------------------------------*/
UaStatus PasEdge::align(unsigned panel_pos, bool moveit)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    cout << "\nAligning " << m_ID.name << ": ";
    if (moveit)
        cout << "Will align panel " << panel_pos << endl;
    else
        cout << "Will keep fixed panel " << panel_pos << endl;

    status = __alignSinglePanel(panel_pos, moveit);


/*
    unsigned numPanels;
    try {
        numPanels = m_pChildren.at(PAS_PanelType).size();
        m_pChildren.at(PAS_MPESType).size();
    }
    catch (out_of_range) {
        // this should never happen -- and edge should never exist without panels
        cout << m_ID << "::align() : no panels or sensors, nothing to do. "
            << "This should never happen, by the way. Just saying." << endl;
        return OpcUa_BadInvalidState;
    }

    // FOR NOW: move only the panel with the lower position value
    // (this will move P1/S1 in the case of the P1-P2/S1-S2 edge)
    for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        cout << "Will align panel " << panelPair.first << endl;
        status = __alignSinglePanel(panelPair.second);
        // return after the first panel
        //if (!status.isGood())
            return status;
    }
*/
    return status;
}

UaStatus PasEdge::__alignSinglePanel(unsigned panelpos, bool moveit)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    MatrixXd A; // response matrix
    MatrixXd C; // constraint matrix
    MatrixXd B; // complete matrix we want to solve for
    auto current_read = getCurrentReadings();
    auto aligned_read = getAlignedReadings();
    VectorXd X; // solutions vector
    VectorXd Y; // sensor misalignment vector, we want to fit to this

    if (moveit) {
        A = getResponseMatrix(panelpos);
        C.setZero();

        B = A;
        Y = aligned_read - current_read;
    }
    else { // this panel kept fixed -- we have three panels
        // get the response matrix, which is [A1 | A2]
        vector<MatrixXd> responseMats;
        for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType))
            if (panelPair.first != panelpos)
                responseMats.push_back(getResponseMatrix(panelPair.first)); // get response from the two other panels
        auto totalRows = max_element(responseMats.begin(), responseMats.end(),
                [](MatrixXd a, MatrixXd b) { return a.rows() < b.rows(); })->rows();
        auto totalCols = accumulate(responseMats.begin(), responseMats.end(), 0,
                [](int curval, MatrixXd a) {return curval + a.cols();});

        A = MatrixXd(totalRows, totalCols);
        A.setZero();

        int curcol = 0;
        for (unsigned i = 0; i < responseMats.size(); i++) {
            auto curMat = responseMats.at(i);
            A.block(0, curcol, curMat.rows(), curMat.cols()) = curMat;
            curcol += curMat.cols();
        }

        // get the constraint matrix, which is [U1 | U2]: we need U1*L1 + U2*L2 = <OVERLAP MISALIGNMENT>;
        // so we construct the constraint matrix as [U1 | U2] and the vector to solve for as [L1; L2]
        //
        // get the overlapping sensors between the other two panels
        int twopanels[2]; // hold positions of the other two panels
        int i = 0;
        for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType))
            if (panelPair.first != panelpos)
                twopanels[i++] = panelPair.first;

        vector<PasMPES *> overlapMPES;
        for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
            if (panelPair.first == panelpos)
                continue;

            auto pMPES = static_cast<PasPanel *>(m_pChildren.at(PAS_PanelType).at(panelPair.second))->getChildren(PAS_MPESType);
            for (const auto& mpes : pMPES)
                if (static_cast<PasMPES *>(mpes)->getPanelSide(twopanels[0])
                        && static_cast<PasMPES *>(mpes)->getPanelSide(twopanels[1]))
                    overlapMPES.push_back(static_cast<PasMPES *>(mpes));
        }
        auto blockRows = overlapMPES.front()->getResponseMatrix().rows();
        auto blockCols = overlapMPES.front()->getResponseMatrix().cols();

        // get misalignment of overlap sensors
        VectorXd overlap_current(overlapMPES.size()*2);
        VectorXd overlap_aligned(overlapMPES.size()*2);
        UaVariant vtmp;
        for (int i = 0; i < overlapMPES.size(); i++) {
            overlapMPES.at(i)->Operate();

            overlapMPES.at(i)->getData(PAS_MPESType_xCentroidAvg, vtmp);
            vtmp.toDouble(overlap_current(i*2));
            overlapMPES.at(i)->getData(PAS_MPESType_yCentroidAvg, vtmp);
            vtmp.toDouble(overlap_current(i*2 + 1));

            overlap_aligned.segment(i*2, 2) = overlapMPES.at(i)->getAlignedReadings();
        }

        C = MatrixXd(overlapMPES.size()*blockRows, 2*blockCols);
        C.setZero();

        for (int j = 0; j < 2; j++) {
            int i = 0;
            for (const auto& mpes : overlapMPES) {
                auto panelside = mpes->getPanelSide(twopanels[j]);
                C.block(blockRows*i, blockCols*j, blockRows, blockCols) = mpes->getResponseMatrix(panelside);
                i++;
            }
        }

        // B = |A|
        //     |C|
        // Y = |y1|, where y1,2 = aligned - current
        //     |y2|
        B = MatrixXd(A.rows() + C.rows(), A.cols());
        B.block(0, 0, A.rows(), A.cols()) = A;
        B.block(A.rows(), 0, C.rows(), A.cols()) = sqrt(2.)*C; // increase the weight of the constraint due to the degenaracy along the P1-P2 edge
        Y = VectorXd(B.rows());
        Y.head(A.rows()) = aligned_read - current_read;
        Y.tail(C.rows()) = sqrt(2.)*(overlap_aligned - overlap_current);

        aligned_read.conservativeResize(aligned_read.size() + overlap_aligned.size());
        aligned_read.tail(overlap_aligned.size()) = overlap_aligned;
        current_read.conservativeResize(current_read.size() + overlap_current.size());
        current_read.tail(overlap_current.size()) = overlap_current;
    }

    try {
        X = B.jacobiSvd(ComputeThinU | ComputeThinV).solve(Y);
    }
    catch (...) {
        cout << "+++ WARNING! +++ Failed to perform Singular Value Decomposition. "
            << "Check your sensor readings! Discarding this result!" << endl;
    }

    cout << "\nCurrent MPES readings:\n" << current_read << endl;
    cout << "\nTarget MPES readings:\n" << aligned_read << endl;
    cout << "\nActuator response matrix for this edge:\n" << A << endl;
    if (!C.isZero(0))
        cout << "\nConstraint matrix for this edge:\n" << C << endl;

    // in the case of moving this panel (3 sensors and 6 actuators),
    // we want to solve delS = A * delL for delL.
    // we get a least squares solution: A^T * delS = (A^T * A) delL
    // delL = (A^T * A) \ A^T * delS
    // This is done through singular value decomposition for numerical stability.
    //
    // When this panel is fixed and we move the other two (4 sensors and 12 actuators),
    // we want to solve the constrained least squares problem
    // min(|A * delL - delS|) given that C*delL = 0 for delL.
    // we get the following least squares solution:
    // delL = (A^T * A) \ A^T * delS - (A^T * A) \ C^T * [C(A^T * A) \ C^T]\C * (A^T * A) \ A^T * delS
    // We do this partly through brute force and partly through SVD.

    cout << "\nLEAST SQUARES SOLUTION:\n" << X << endl;

    // make sure we don't move what we're not supposed to
    m_PanelsToMove.clear();

    /* MOVE ACTUATORS */
    PasPanel *pCurPanel;
    int j = 0;
    for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        if ( (panelPair.first == panelpos) == moveit ) { // clever but not clear...
            pCurPanel = static_cast<PasPanel *>(m_pChildren.at(PAS_PanelType).at(panelPair.second));
            auto nACT = pCurPanel->getActuatorCount();
            // WORK AROUND TO HANDLE SYSTEMATIC OFFSETS DURING ALIGNMENT ONLY
            // update the current coordinates of the panel
            pCurPanel->__updateCoords();
            // get the current actuator lenghts
            VectorXd curLengths = pCurPanel->m_ActuatorLengths;
            // get the new coordinates -- add deltas to actuator lengths and recompute
            curLengths += X.segment(j, 6);
            pCurPanel->m_SP.ComputeStewart(curLengths.data());
            VectorXd curCoords(6);
            for (unsigned i = 0; i < 6; i++)
                curCoords(i) = pCurPanel->m_SP.GetPanelCoords()[i];
            // subtract offsets
            curCoords -= m_SystematicOffsets;
            // recompute actuator lengths
            pCurPanel->m_SP.ComputeActsFromPanel(curCoords.data());
            VectorXd newLengths(6);
            for (unsigned i = 0; i < 6; i++)
                newLengths(i) = pCurPanel->m_SP.GetActLengths()[i];
            // finally, update the deltas
            X.segment(j, 6) = newLengths - pCurPanel->m_ActuatorLengths;
            // print out to make sure
            cout << "After correction for systematic offsets, will move actuators of "
                << pCurPanel->getId().name << " by\n" << X.segment(j, 6) << endl;

            UaVariantArray deltas;
            deltas.create(nACT);
            for (unsigned i = 0; i < nACT; i++)
                deltas[i].Value.Float = X(j++); // X has dimension of 6*nPanelsToMove !

            status = pCurPanel->Operate(PAS_PanelType_StepAll, deltas);
            if (!status.isGood()) return status;
            m_PanelsToMove.push_back(panelPair.second);
        }
    }

    // arbitrary
    if (X.array().abs().maxCoeff() >= 0.05)
        m_isAligned = false;
    else
        m_isAligned = true;

    cout << "\nYou can also call " << m_ID.name << ".Move to move the panel(s).\n" << endl;

    /* LOG ALIGNMENT */
/*
    ofstream output("convergence.log", ios_base::app);

    cout << "Old Positions: (";
    output << "Old Positions: (";
    for (unsigned i = 0; i < nMPES; i++ )
    {
        cout << current_read[i*2] << ", " << current_read[i*2 + 1] << "; ";
        output << current_read[i*2] << ", " << current_read[i*2 + 1] << "; ";
    }
    cout << ")." << endl;
    output << ")." << endl;

    cout << "Target Positions: (";
    output << "Target Positions: (";
    for (unsigned i = 0; i < nMPES; i++ )
    {
        cout << aligned_read[i*2] << ", " << aligned_read[i*2 + 1] << "; ";
        output << aligned_read[i*2] << ", " << aligned_read[i*2 + 1] << "; ";
    }
    cout << ")." << endl;
    output << ")." << endl;

    for (unsigned i = 0; i < nMPES; i++)
    {
        status = pMPES.at(i)->Operate();
        if (!status.isGood()) return status;
        pMPES.at(i)->getData(PAS_MPESType_xCentroidAvg, vtmp);
        vtmp.toDouble(current_read[i*2]);
        pMPES.at(i)->getData(PAS_MPESType_yCentroidAvg, vtmp);
        vtmp.toDouble(current_read[i*2 + 1]);
    }

    cout << "New Positions: (";
    output << "New Positions: (";
    for (unsigned i = 0; i < nMPES; i++ )
    {
        cout << current_read[i*2] << ", " << current_read[i*2 + 1] << "; ";
        output << current_read[i*2] << ", " << current_read[i*2 + 1] << "; ";
    }
    cout << ")." << endl;
    output << ")." << endl;

    output.close();
*/

    return status;
}

const MatrixXd& PasEdge::getResponseMatrix(unsigned panelpos)
{
    if (!m_responseMatUpdated)
        __updateResponse();

    // no need to try-catch -- panelpos here is always valid
    return m_ResponseMatMap.at(panelpos);
}

void PasEdge::__updateResponse()
{
    auto& pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned nACT = static_cast<PasMPES *>(pMPES.front())->getResponseMatrix().cols();

    for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        unsigned panel = panelPair.first;

        m_ResponseMatMap[panel] = MatrixXd(maxMPES*2, nACT);
        m_ResponseMatMap.at(panel).setZero();
        for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
            auto panelside = static_cast<PasMPES *>(pMPES.at(nMPES))->getPanelSide(panel);
            // if this is nonzero, add it to the edge response matrix
            if (panelside) {
                const auto& curresponse = static_cast<PasMPES *>(pMPES.at(nMPES))->getResponseMatrix(panelside);
                m_ResponseMatMap.at(panel).block(2*nMPES, 0, curresponse.rows(), curresponse.cols()) = curresponse;
            }
        }
    }

    m_responseMatUpdated = true;
}


const VectorXd& PasEdge::getAlignedReadings()
{
    if (!m_alignmentUpdated)
        __updateAlignmentData();

    return m_AlignedReadings;
}

void PasEdge::__updateAlignmentData()
{
    auto& pMPES = m_pChildren.at(PAS_MPESType);
    unsigned nMPES = pMPES.size();

    m_AlignedReadings = VectorXd(2*nMPES);
    for (unsigned mpes = 0; mpes < nMPES; mpes++) {
        auto mpes_response = static_cast<PasMPES *>(pMPES.at(mpes))->getAlignedReadings();
        m_AlignedReadings.segment(2*mpes, 2) = mpes_response;
    }

    m_alignmentUpdated = true;

}


const Eigen::VectorXd& PasEdge::getCurrentReadings()
{
    // edge should have at least one sensor by definition -- otherwise it wouldn't be created.
    // so this is safe.
    auto& pMPES = m_pChildren.at(PAS_MPESType);
    unsigned nMPES = pMPES.size();

    m_CurrentReadings = VectorXd(2*nMPES);

    UaVariant vtmp;
    for (unsigned i = 0; i < nMPES; i++) {
        pMPES.at(i)->Operate();

        pMPES.at(i)->getData(PAS_MPESType_xCentroidAvg, vtmp);
        vtmp.toDouble(m_CurrentReadings(i*2));
        pMPES.at(i)->getData(PAS_MPESType_yCentroidAvg, vtmp);
        vtmp.toDouble(m_CurrentReadings(i*2 + 1));
    }

    return m_CurrentReadings;
}

/* ----------------------------------------------------------------------------
    END Class    PasEdge
==============================================================================*/

/* ----------------------------------------------------------------------------
    Begin Class    PasCCD
    constructors / destructors
-----------------------------------------------------------------------------*/
PasCCD::PasCCD(Identity identity) : PasController(identity, nullptr)
{
    m_pCCD = new GASCCD();
    m_pCCD->setConfig(m_ID.eAddress);
    m_pCCD->Initialize();

    m_ID.serialNumber = m_pCCD->getSerial();
    m_ID.eAddress = m_pCCD->getAddress();
    m_ID.name = m_pCCD->getName();
    cout << "assigned the name " << m_ID.name << " to this CCD" << endl;

    if (m_state != PASState::PAS_On)
        m_state = PASState::PAS_On;
}

PasCCD::~PasCCD()
{
    m_state = PASState::PAS_Off;
}
/* ----------------------------------------------------------------------------
    Class        PasCCD
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasCCD::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasCCD
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode PasCCD::setState(PASState state)
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
/* ----------------------------------------------------------------------------
    Class        PasCCD
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasCCD::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    UaMutexLocker lock(&m_mutex);
    int dataoffset = offset - PAS_CCDType_xFromLED;
    if ( dataoffset >= 6 )
        return OpcUa_BadInvalidArgument;

    if (!m_updated)
        value.setDouble(0.);
    else
        value.setDouble(*(static_cast<const double *>(m_pCCD->getOutput() + dataoffset)));

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasCCD
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode PasCCD::setData(OpcUa_UInt32 offset, UaVariant value)
{
    int dataoffset = offset - PAS_CCDType_xFromLED;
    OpcUa_Double val;
    value.toDouble(val);
    if ( (dataoffset > 5) && (dataoffset < 12))
    {
        m_pCCD->setNominalValues(dataoffset - 6, val);
        return OpcUa_Good;
    }

    return OpcUa_BadNotWritable;
}
/* ----------------------------------------------------------------------------
    Class        PasCCD
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatusCode PasCCD::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaMutexLocker lock(&m_mutex);
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
/* ----------------------------------------------------------------------------
    Class        PasCCD
    Method       read
    Description  Read Controller data.
-----------------------------------------------------------------------------*/
UaStatus PasCCD::read()
{
    UaMutexLocker lock(&m_mutex);

    if ( m_state == PASState::PAS_On )
    {
        printf("\nReading CCD %s\n", m_ID.name.c_str());
        m_pCCD->Update();
        m_updated = true;
        return OpcUa_Good;
    }
    else
        m_updated = false;

    return OpcUa_Bad;
}
/* ----------------------------------------------------------------------------
    End Class    PasCCD
==============================================================================*/

/* ===========================================================================
    Begin Class    PasPSD
    constructors / destructors
-----------------------------------------------------------------------------*/

PasPSD::PasPSD(Identity identity, Client *pClient) :
    PasController(identity, pClient, kUpdateInterval)
{
    m_state = PASState::PAS_On;
    cout << "PasPSD: update interval set to " << m_UpdateInterval_ms << " ms" << endl;

    m_lastUpdateTime = TIME::now() - chrono::duration<int, ratio<1, 1000>>(m_UpdateInterval_ms);
}

PasPSD::~PasPSD()
{
    m_pClient = nullptr;
    m_state = PASState::PAS_Off;
}

UaStatusCode PasPSD::getState(PASState& state)
{
    state = m_state;
    return OpcUa_Good;
}

UaStatusCode PasPSD::setState(PASState state)
{
    UaMutexLocker lock(&m_mutex);

    return OpcUa_BadNotWritable;
}

UaStatusCode PasPSD::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    int dataoffset = offset - PAS_PSDType_x1;
    if ( dataoffset >= 9 )
    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (__expired()) // if cached values expired, update them
        status = read();

    value.setDouble(*(reinterpret_cast<OpcUa_Double *>(&data) + dataoffset));

    return status;
}

UaStatusCode PasPSD::setData(OpcUa_UInt32 offset, UaVariant value)
{
    return OpcUa_BadNotWritable;
}

UaStatusCode PasPSD::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaStatusCode  status;

    if ( offset >= 1 )
        return OpcUa_BadInvalidArgument;

    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);
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

/* ----------------------------------------------------------------------------
    END Class    PasPSD
==============================================================================*/

UaStatus PasPSD::read()
{
    UaStatus status;

    vector<string> varstoread {"x1", "y1", "x2", "y2", "dx1", "dy1", "dx2", "dy2", "Temperature"};
    UaVariant valstoread[9];

    transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
                [this](string& str){ return m_ID.eAddress + "." + str;});

    status = m_pClient->read(varstoread, &valstoread[0]);
    if (!status.isGood())
        return status;

    m_lastUpdateTime = TIME::now();
    for (unsigned i = 0; i < varstoread.size(); i++)
        valstoread[i].toDouble(*(reinterpret_cast<OpcUa_Double *>(&data) + i));

    return OpcUa_Good;
}

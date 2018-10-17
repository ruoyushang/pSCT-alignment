#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "airconditionercontrollerobject.h"
#include "furnacecontrollerobject.h"
#include "opcua_analogitemtype.h"
#include "bacommunicationinterface.h"
#include "baeventdata.h"
#include "opcua_foldertype.h"
#include "uaenumdefinition.h"
#if SUPPORT_Event_Subscription_Server_Facet
#include "opcua_offnormalalarmtype.h"
#endif // SUPPORT_Event_Subscription_Server_Facet
#if SUPPORT_Historical_Access
#include "historymanagercache.h"
#include "opcua_historicaldataconfigurationtype.h"
#endif // SUPPORT_Historical_Access
#include "nodemanagerroot.h"
#include "methodhandleuanode.h"

class BAFolder:
    public OpcUa::FolderType
{
    UA_DISABLE_COPY(BAFolder);
protected:
    virtual ~BAFolder(){}
public:
    // construction / destruction
    BAFolder(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NmBuildingAutomation* pNM);

    //- Interface MethodManager -----------------------------------------------------------
    virtual UaStatus beginCall(MethodManagerCallback*, const ServiceContext&, OpcUa_UInt32, MethodHandle*, const UaVariantArray&);
    virtual UaStatus call(const ServiceContext&, MethodHandle*, const UaVariantArray&, UaVariantArray&, UaStatusCodeArray&, UaDiagnosticInfos&);
    //- Interface MethodManager -----------------------------------------------------------

private:
    NmBuildingAutomation* m_pNM;
};

BAFolder::BAFolder(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NmBuildingAutomation* pNM)
: OpcUa::FolderType(nodeId, name, browseNameNameSpaceIndex, pNM),
  m_pNM(pNM)
{
}

UaStatus BAFolder::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    UaStatus            ret;

    OpcUa::MethodCallJob* pCallJob = new OpcUa::MethodCallJob;
    pCallJob->initialize(this, pCallback, serviceContext, callbackHandle, pMethodHandle, inputArguments);
    ret = NodeManagerRoot::CreateRootNodeManager()->pServerManager()->getThreadPool()->addJob(pCallJob);
    if ( ret.isBad() )
    {
        delete pCallJob;
    }

    return ret;
}

UaStatus BAFolder::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    MethodHandleUaNode* pMethodHandleUaNode = (MethodHandleUaNode*)pMethodHandle;
    UaMethod*           pMethod             = NULL;

    if (pMethodHandleUaNode)
    {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if (pMethod)
        {
            if ( pMethod->nodeId() == UaNodeId("StartLogging", m_pNM->getNameSpaceIndex()) )
            {
                m_pNM->startLogging();
            }
            else if ( pMethod->nodeId() == UaNodeId("StopLogging", m_pNM->getNameSpaceIndex()) )
            {
                m_pNM->stopLogging();
            }
            else
            {
                return FolderType::call(serviceContext, pMethodHandle, inputArguments, outputArguments, inputArgumentResults, inputArgumentDiag);
            }
        }
        else
        {
            assert(false);
            ret = OpcUa_BadInvalidArgument;
        }
    }
    else
    {
        assert(false);
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}

NmBuildingAutomation::NmBuildingAutomation()
: NodeManagerBase("urn:UnifiedAutomation:CppDemoServer:BuildingAutomation", OpcUa_True),
  // SamplingOnRequestExample change begin
  // Added: Initialization of new members
  m_stopThread(false),
  m_changedMonitoredVariables(false)
  // SamplingOnRequestExample change end
{
    m_pCommIf = new BaCommunicationInterface;

#if SUPPORT_Historical_Access
    // Create the HistoryManager responsible for this NodeManager
    m_pHistoryManagerCache = new HistoryManagerCache;
    // Register the HistoryManager with the NodeManager base class
    setHistoryManager(m_pHistoryManagerCache);
#endif // SUPPORT_Historical_Access
}

NmBuildingAutomation::~NmBuildingAutomation()
{
#if SUPPORT_Historical_Access
    setHistoryManager(NULL);
    delete m_pHistoryManagerCache;
#endif // SUPPORT_Historical_Access
    delete m_pCommIf;
}

UaStatus NmBuildingAutomation::afterStartUp()
{
    UaStatus ret;
    BAFolder                       *pFolder = NULL;
    AirConditionerControllerObject *pAirConditioner = NULL;
    FurnaceControllerObject        *pFurnace = NULL;
    UaString       sControllerName;
    OpcUa_UInt32   count = m_pCommIf->getCountControllers();
    OpcUa_UInt32   i;
    OpcUa_UInt32   controllerAddress;
    BaCommunicationInterface::ControllerType controllerType;

    createTypeNodes();

#if SUPPORT_Historical_Access
    // We support historical data access
    m_pHistoryManagerCache->pHistoryServerCapabilities()->setAccessHistoryDataCapability(OpcUa_True);
#endif // SUPPORT_Historical_Access

    /**************************************************************
     Create a folder for the controller objects and add the folder to the ObjectsFolder
    ***************************************************************/
    pFolder = new BAFolder(UaNodeId("BuildingAutomation", getNameSpaceIndex()) , "BuildingAutomation", getNameSpaceIndex(), this);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pFolder, OpcUaId_Organizes);

#if SUPPORT_Method_Server_Facet
    UaMethodGeneric* pMethod = NULL;
    // Add Method "StartLogging"
    pMethod = new UaMethodGeneric("StartLogging", UaNodeId("StartLogging", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(pFolder, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());

    // Add Method "StopLogging"
    pMethod = new UaMethodGeneric("StopLogging", UaNodeId("StopLogging", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(pFolder, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());
#endif // SUPPORT_Method_Server_Facet

    //Configuration Variable
    UaVariant defaultConfiguration;
    OpcUa::BaseDataVariableType *pConfigurationsVariable =
        new OpcUa::BaseDataVariableType(
            UaNodeId("ControllerConfigurations", getNameSpaceIndex()),
            "ControllerConfigurations",
            getNameSpaceIndex(),
            defaultConfiguration,
            OpcUa_AccessLevels_CurrentRead,
            this,
            NULL);
    pConfigurationsVariable->setDataType(UaNodeId(Ba_Configurations, getNameSpaceIndex()));
    pConfigurationsVariable->setValueRank(OpcUa_ValueRanks_Scalar);
    pConfigurationsVariable->setValueHandling(UaVariable_Value_Cache);
    addNodeAndReference(pFolder, pConfigurationsVariable, OpcUaId_Organizes);


#if SUPPORT_Event_Subscription_Server_Facet
    /**************************************************************
     Create alarm area folders for the controller objects and add them to the Server object
    ***************************************************************/
    OpcUa::FolderType* pAreaAirConditioner = new OpcUa::FolderType(
        UaNodeId("AreaAirConditioner", getNameSpaceIndex()),
        "AreaAirConditioner",
        getNameSpaceIndex(),
        this);
    // Set EventNotifier attribute with SubscribeToEvents, HistoryRead and HistoryWrite
    pAreaAirConditioner->setEventNotifier(OpcUa_EventNotifiers_SubscribeToEvents | OpcUa_EventNotifiers_HistoryRead | OpcUa_EventNotifiers_HistoryWrite);
    addNodeAndReference(OpcUaId_Server, pAreaAirConditioner, OpcUaId_HasNotifier);
    // Register event notifier tree
    registerEventNotifier(OpcUaId_Server, pAreaAirConditioner->nodeId());

#if SUPPORT_Historical_Access
    UaEventFilter               eventFilter;
    UaSimpleAttributeOperand    selectElement;

    // request all fields defined for the BaseEventType
    selectElement.setBrowsePathElement(0, UaQualifiedName("EventId", 0), 1);
    eventFilter.setSelectClauseElement(0, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("EventType", 0), 1);
    eventFilter.setSelectClauseElement(1, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("LocalTime", 0), 1);
    eventFilter.setSelectClauseElement(2, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
    eventFilter.setSelectClauseElement(3, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("ReceiveTime", 0), 1);
    eventFilter.setSelectClauseElement(4, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Severity", 0), 1);
    eventFilter.setSelectClauseElement(5, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
    eventFilter.setSelectClauseElement(6, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("SourceNode", 0), 1);
    eventFilter.setSelectClauseElement(7, selectElement, 9);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
    eventFilter.setSelectClauseElement(8, selectElement, 9);
    // we define no where clause
    // add area to historizing
    m_pHistoryManagerCache->addEventNotifierToHistorize(pAreaAirConditioner, eventFilter);
#endif // SUPPORT_Historical_Access

    OpcUa::FolderType* pAreaFurnace = new OpcUa::FolderType(
        UaNodeId("AreaFurnace", getNameSpaceIndex()),
        "AreaFurnace",
        getNameSpaceIndex(),
        this);
    // Set EventNotifier attribute with SubscribeToEvents, HistoryRead and HistoryWrite
    pAreaFurnace->setEventNotifier(OpcUa_EventNotifiers_SubscribeToEvents | OpcUa_EventNotifiers_HistoryRead | OpcUa_EventNotifiers_HistoryWrite);
    addNodeAndReference(OpcUaId_Server, pAreaFurnace, OpcUaId_HasNotifier);
    // Register event notifier tree
    registerEventNotifier(OpcUaId_Server, pAreaFurnace->nodeId());
#if SUPPORT_Historical_Access
    // add area to historizing
    m_pHistoryManagerCache->addEventNotifierToHistorize(pAreaFurnace, eventFilter);
#endif // SUPPORT_Historical_Access
#endif // SUPPORT_Event_Subscription_Server_Facet

    /**************************************************************
     Create two views for the two different controller types
    ***************************************************************/
    UaGenericView* pViewAirConditioner = new UaGenericView(
        UaNodeId(Ba_AirConditionerView, getNameSpaceIndex()),
        UaQualifiedName("AirConditionerView", getNameSpaceIndex()),
        UaLocalizedText("", "AirConditionerView"),
        UaLocalizedText("", "View containing the AirConditioner objects"),
        OpcUa_True,
        0);
    addNodeAndReference(OpcUaId_ViewsFolder, pViewAirConditioner, OpcUaId_Organizes);

    UaGenericView* pViewFurnace = new UaGenericView(
        UaNodeId(Ba_FurnaceView, getNameSpaceIndex()),
        UaQualifiedName("FurnaceView", getNameSpaceIndex()),
        UaLocalizedText("", "FurnaceView"),
        UaLocalizedText("", "View containing the Furnace objects"),
        OpcUa_True,
        0);
    addNodeAndReference(OpcUaId_ViewsFolder, pViewFurnace, OpcUaId_Organizes);

    /**************************************************************
     * Create the Controller Object Instances
     **************************************************************/
    for ( i=0; i<count; i++ )
    {
        ret = m_pCommIf->getControllerConfig(
            i,
            controllerType,
            sControllerName,
            controllerAddress);

        if ( controllerType == BaCommunicationInterface::AIR_CONDITIONER )
        {
            pAirConditioner = new AirConditionerControllerObject(
                sControllerName,
                UaNodeId(sControllerName, getNameSpaceIndex()),
                m_defaultLocaleId,
                this,
                controllerAddress,
                m_pCommIf);
            ret = addNodeAndReference(pFolder, pAirConditioner, OpcUaId_Organizes);
            UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
            // Add HasNotifier reference from alarm area to controller object
            ret = addUaReference(pAreaAirConditioner, pAirConditioner, OpcUaId_HasNotifier);
            UA_ASSERT(ret.isGood());
            // Register event notifier tree
            registerEventNotifier(pAreaAirConditioner->nodeId(), pAirConditioner->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
            addUaReference(pViewAirConditioner, pAirConditioner, OpcUaId_Organizes);
#if SUPPORT_Historical_Access
            // Register Temperature node for historizing
            m_pHistoryManagerCache->addVariableToHistorize(pAirConditioner->getTargetNodeByBrowseName(UaQualifiedName("Temperature", getNameSpaceIndex())));
#endif // SUPPORT_Historical_Access
        }
        else
        {
            pFurnace = new FurnaceControllerObject(
                sControllerName,
                UaNodeId(sControllerName, getNameSpaceIndex()),
                m_defaultLocaleId,
                this,
                controllerAddress,
                m_pCommIf);
            ret = addNodeAndReference(pFolder, pFurnace, OpcUaId_Organizes);
            UA_ASSERT(ret.isGood());
#if SUPPORT_Event_Subscription_Server_Facet
            // Add HasNotifier reference from alarm area to controller object
            ret = addUaReference(pAreaFurnace, pFurnace, OpcUaId_HasNotifier);
            UA_ASSERT(ret.isGood());
            // Register event notifier tree
            registerEventNotifier(pAreaFurnace->nodeId(), pFurnace->nodeId());
#endif // SUPPORT_Event_Subscription_Server_Facet
            addUaReference(pViewFurnace, pFurnace, OpcUaId_Organizes);
#if SUPPORT_Historical_Access
            // Register Temperature node for historizing
            m_pHistoryManagerCache->addVariableToHistorize(pFurnace->getTargetNodeByBrowseName(UaQualifiedName("Temperature", getNameSpaceIndex())));
#endif // SUPPORT_Historical_Access
        }
    }

    // Set started flag - this is necessary to be able to subscribe for data changes in HistoryManagerBuildingAutomation->startUp
    m_isStarted = OpcUa_True;

#if SUPPORT_Historical_Access
    m_pHistoryManagerCache->startUp(m_pServerManager);
#endif // SUPPORT_Historical_Access

    // SamplingOnRequestExample change begin
    // Added: Start worker thread
    start();
    // SamplingOnRequestExample change end

    return ret;
}

UaStatus NmBuildingAutomation::beforeShutDown()
{
    UaStatus ret;

#if SUPPORT_Historical_Access
    m_pHistoryManagerCache->stopLogging();
    m_pHistoryManagerCache->shutDown();
#endif // SUPPORT_Historical_Access

    // SamplingOnRequestExample change begin
    // Added: Stop worker thread
    m_stopThread = true;
    // Wait for thread completion
    wait();
    // SamplingOnRequestExample change end

    return ret;
}

void NmBuildingAutomation::startLogging()
{
#if SUPPORT_Historical_Access
    m_pHistoryManagerCache->startLogging();
#endif // SUPPORT_Historical_Access
}

void NmBuildingAutomation::stopLogging()
{
#if SUPPORT_Historical_Access
    m_pHistoryManagerCache->stopLogging();
#endif // SUPPORT_Historical_Access
}

// SamplingOnRequestExample change begin
/** Overwrite of base class function to get informed by NodeManagerBase about a change in monitoring
 */
void NmBuildingAutomation::variableCacheMonitoringChanged(UaVariableCache* pVariable, TransactionType transactionType)
{
    // Just make sure only handle the right variables
    if ( pVariable->valueHandling() != (UaVariable_Value_CacheIsSource | UaVariable_Value_CacheIsUpdatedOnRequest) )
    {
        return;
    }

    // Get fastest requested sampling interval requested by a client
    // Can be used to change polling rate to device if fastest rate changed
    OpcUa_Double fastedRequestedRate = pVariable->getMinSamplingInterval();
    OpcUa_ReferenceParameter(fastedRequestedRate);
    // This is not used in this example

    if ( (transactionType == IOManager::TransactionMonitorBegin) && (pVariable->signalCount() == 1) )
    {
        // The first monitored item was created for variable (pVariable)
        // Lock access to variable list
        UaMutexLocker lock(&m_mutexMonitoredVariables);

        // Add to map and set changed flag
        m_mapMonitoredVariables[pVariable] = pVariable;
        m_changedMonitoredVariables = true;
        // Increment reference counter for the entry in the map
        pVariable->addReference();
    }
    else if ( (transactionType == IOManager::TransactionMonitorStop) && (pVariable->signalCount() == 0) )
    {
        // The last monitored item was removed for variable (pVariable)
        // Lock access to variable list
        UaMutexLocker lock(&m_mutexMonitoredVariables);

        // Add to map and set changed flag
        std::map<UaVariableCache*, UaVariableCache*>::iterator it = m_mapMonitoredVariables.find(pVariable);
        if ( it != m_mapMonitoredVariables.end() )
        {
            m_mapMonitoredVariables.erase(it);
            m_changedMonitoredVariables = true;
            // Decrement reference counter since we removed the entry from the map
            pVariable->releaseReference();
        }
    }
}

/** Main method of worker thread for internal sampling
 */
void NmBuildingAutomation::run()
{
    UaStatus     ret;
    OpcUa_UInt32 i;
    OpcUa_UInt32 count;
    OpcUa_DateTime nullTime;
    OpcUa_DateTime_Initialize(&nullTime);

    std::map<UaVariableCache*, UaVariableCache*>::iterator it;

    while ( m_stopThread == false )
    {
        // Lock access to variable list
        UaMutexLocker lock(&m_mutexMonitoredVariables);
        // Check if the list was changed
        if ( m_changedMonitoredVariables )
        {
            // Update list for sampling
            // First release reference for all variables in array
            count = m_arrayMonitoredVariables.length();
            for ( i=0; i<count; i++ )
            {
                // Check if the variable is still used
                it = m_mapMonitoredVariables.find((UaVariableCache*)m_arrayMonitoredVariables[i]);
                if ( it == m_mapMonitoredVariables.end() )
                {
                    // Change value of variable to bad status BadWaitingForInitialData - it is not longer used
                    // This makes sure we do not deliver an old value when the monitoring is activated later for this variable
                    UaDataValue badStatusValue;
                    badStatusValue.setStatusCode(OpcUa_BadWaitingForInitialData);
                    m_arrayMonitoredVariables[i]->setValue(NULL, badStatusValue, OpcUa_False);
                }
                // Decrement reference counter for the variable - we removed it from the list
                m_arrayMonitoredVariables[i]->releaseReference();
            }
            // And clear old array
            m_arrayMonitoredVariables.clear();

            // Create the new array and increment reference counter for added variables
            it = m_mapMonitoredVariables.begin();
            m_arrayMonitoredVariables.create((OpcUa_UInt32) m_mapMonitoredVariables.size());
            count = m_arrayMonitoredVariables.length();
            for ( i=0; i<count; i++ )
            {
                m_arrayMonitoredVariables[i] = it->first;
                // Increment reference counter - it was added to the list
                m_arrayMonitoredVariables[i]->addReference();
                it++;
            }

            // Reset the change flag
            m_changedMonitoredVariables = false;
        }
        lock.unlock();

        // Check if we have anything to sample
        if ( m_arrayMonitoredVariables.length() > 0 )
        {
            UaDataValueArray results;
            count = m_arrayMonitoredVariables.length();

            // Call readValues to update variable values
            ret = readValues(m_arrayMonitoredVariables, results);
            if ( ret.isGood() )
            {
                // Update values
                for ( i=0; i<count; i++ )
                {
                    // The Read sets a new source timestamp
                    // -> we should only update source timestamp after a write
                    // -> or after value change (detected and set by UaVariable::setValue()
                    BaUserData* pUserData = (BaUserData*)m_arrayMonitoredVariables[i]->getUserData();
                    if ( pUserData && pUserData->updateTimestamp() != OpcUa_False )
                    {
                        // Reset update flag and keep source timestamp from read
                        pUserData->setUpdateTimestamp(OpcUa_False);
                    }
                    else
                    {
                        // Reset source timestamp to do the check for change in UaVariable::setValue()
                        results[i].setSourceTimestamp(nullTime);
                    }
                    m_arrayMonitoredVariables[i]->setValue(NULL, results[i], OpcUa_True);
                }
            }
            else
            {
                // Set bad status for all variables
                UaDataValue badStatusValue;
                badStatusValue.setStatusCode(ret.statusCode());
                for ( i=0; i<count; i++ )
                {
                    m_arrayMonitoredVariables[i]->setValue(NULL, badStatusValue, OpcUa_True);
                }
            }
        }

        UaThread::msleep(500);
    }
}
// SamplingOnRequestExample change end

UaStatus NmBuildingAutomation::readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues)
{
    UaStatus ret;
    OpcUa_UInt32 i;
    OpcUa_UInt32 count = arrUaVariables.length();
    UaDateTime timeStamp = UaDateTime::now();

    // Create result array
    arrDataValues.create(count);

    for (i=0; i<count; i++)
    {
        // Set timestamps
        arrDataValues[i].setSourceTimestamp(timeStamp);
        arrDataValues[i].setServerTimestamp(timeStamp);

        // Cast UaVariable to BaControllerVariable
        UaVariable* pVariable = arrUaVariables[i];
        if (pVariable)
        {
            BaUserData* pUserData = (BaUserData*)pVariable->getUserData();

            if ( pUserData )
            {
                UaVariant    vTemp;
                UaStatusCode status;

                if ( pUserData->isState() == BaUserData::StateType_IsDataVariable )
                {
                    // Read of a data variable
                    OpcUa_Double value;

                    // Get the data for the controller from the communication interface
                    status = m_pCommIf->getControllerData(
                        pUserData->deviceAddress(),
                        pUserData->variableOffset(),
                        value);
                    if ( status.isGood() )
                    {
                        // Set value
                        vTemp.setDouble(value);
                        arrDataValues[i].setValue(vTemp, OpcUa_True, OpcUa_False);
                    }
                    else
                    {
                        // Set Error
                        arrDataValues[i].setStatusCode(status.statusCode());
                    }
                }
                else if ( pUserData->isState() == BaUserData::StateType_IsStateVariable )
                {
                    // Read of a state variable
                    // We need to get the state of the controller
                    BaCommunicationInterface::ControllerState state;

                    // Get the data for the controller from the communication interface
                    status = m_pCommIf->getControllerState(
                        pUserData->deviceAddress(),
                        state);
                    if ( status.isGood() )
                    {
                        // Set value
                        vTemp.setInt32(state);
                        arrDataValues[i].setValue(vTemp, OpcUa_True, OpcUa_False);
                    }
                    else
                    {
                        // Set Error
                        arrDataValues[i].setStatusCode(status.statusCode());
                    }
                }
                else
                {
                    arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
                }
            }
            else
            {
                if (pVariable->nodeId() == UaNodeId("ControllerConfigurations", getNameSpaceIndex()))
                {
                    OpcUa_UInt16 nsIdx = getNameSpaceIndex();
                    UaGenericStructureArray airConditionerConfigurations(structureDefinition(UaNodeId(Ba_AirConditionerControllerConfiguration, nsIdx)));
                    UaGenericStructureArray humidityConfigurations(structureDefinition(UaNodeId(Ba_ControllerConfiguration, nsIdx)));
                    UaString       sControllerName;
                    OpcUa_UInt32   controllerCount = m_pCommIf->getCountControllers();
                    OpcUa_UInt32   controllerAddress;
                    BaCommunicationInterface::ControllerType controllerType;

                    OpcUa_UInt32 noOfAirConditioners = 0;
                    OpcUa_UInt32 noOfFurnaces = 0;
                    for ( OpcUa_UInt32 j=0; j<controllerCount; j++ )
                    {
                        ret = m_pCommIf->getControllerConfig(
                            j,
                            controllerType,
                            sControllerName,
                            controllerAddress);

                        if ( controllerType == BaCommunicationInterface::AIR_CONDITIONER )
                        {
                            noOfAirConditioners++;
                        }
                        else
                        {
                            noOfFurnaces++;
                        }
                    }
                    airConditionerConfigurations.create(noOfAirConditioners);
                    humidityConfigurations.create(noOfFurnaces);

                    OpcUa_UInt32 currentAirConditioner = 0;
                    OpcUa_UInt32 currentFurnace = 0;
                    UaVariant value;
                    for ( OpcUa_UInt32 j=0; j<controllerCount; j++ )
                    {
                        ret = m_pCommIf->getControllerConfig(
                            j,
                            controllerType,
                            sControllerName,
                            controllerAddress);

                        if ( controllerType == BaCommunicationInterface::AIR_CONDITIONER )
                        {
                            UaGenericStructureValue configuration(structureDefinition(UaNodeId(Ba_AirConditionerControllerConfiguration, getNameSpaceIndex())));
                            value = sControllerName;
                            configuration.setField("Name", value, OpcUa_True);
                            OpcUa_Double temperatureSetpoint;
                            m_pCommIf->getControllerData(controllerAddress, 1, temperatureSetpoint);
                            value = UaVariant(temperatureSetpoint);
                            configuration.setField("TemperatureSetpoint", value, OpcUa_True);
                            OpcUa_Double humiditySetpoint;
                            m_pCommIf->getControllerData(controllerAddress, 4, humiditySetpoint);
                            value = UaVariant(humiditySetpoint);
                            configuration.setField("HumiditySetpoint", value, OpcUa_True);
                            value = controllerAddress;
                            configuration.setField("DeviceAddress", value, OpcUa_True);
                            airConditionerConfigurations[currentAirConditioner++] = configuration;
                        }
                        else
                        {
                            UaGenericStructureValue configuration(structureDefinition(UaNodeId(Ba_ControllerConfiguration, getNameSpaceIndex())));
                            value = sControllerName;
                            configuration.setField("Name", value, OpcUa_True);
                            OpcUa_Double temperatureSetpoint;
                            m_pCommIf->getControllerData(controllerAddress, 1, temperatureSetpoint);
                            value = UaVariant(temperatureSetpoint);
                            configuration.setField("TemperatureSetpoint", value, OpcUa_True);
                            value = controllerAddress;
                            configuration.setField("DeviceAddress", value, OpcUa_True);
                            humidityConfigurations[currentFurnace++] = configuration;
                        }
                    }
                    UaGenericStructureValue configurations(structureDefinition(UaNodeId(Ba_Configurations, getNameSpaceIndex())));
                    configurations.setField("AirConditionerControllers", airConditionerConfigurations);
                    configurations.setField("FurnaceControllers", humidityConfigurations);
                    configurations.toDataValue(arrDataValues[i], OpcUa_True);
                }
                else
                {
                    arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
                }
            }
        }
        else
        {
            arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
        }
    }

    return ret;
}

UaStatus NmBuildingAutomation::writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes)
{
    UaStatus ret;
    OpcUa_UInt32 i;
    OpcUa_UInt32 count = arrUaVariables.length();

    // Create result array
    arrStatusCodes.create(count);

    for ( i=0; i<count; i++ )
    {
        // Cast UaVariable to BaControllerVariable
        UaVariable* pVariable = arrUaVariables[i];
        if ( pVariable )
        {
            BaUserData* pUserData = (BaUserData*)pVariable->getUserData();

            if ( pUserData )
            {
                if ( pUserData->isState() == BaUserData::StateType_IsDataVariable )
                {
                    UaVariant    vTemp(arrpDataValues[i]->Value);
                    UaStatusCode status;
                    OpcUa_Double value;

                    status = vTemp.toDouble(value);
                    if ( status.isGood() )
                    {
                        // Get the data for the controller from the communication interface
                        status = m_pCommIf->setControllerData(
                            pUserData->deviceAddress(),
                            pUserData->variableOffset(),
                            value);
                    }
                    arrStatusCodes[i] = status.statusCode();
                    if ( status.isGood() )
                    {
                        pUserData->setUpdateTimestamp(OpcUa_True);
                    }
                }
                else
                {
                    // State variable can not be written
                    arrStatusCodes[i] = OpcUa_BadNotWritable;
                }
            }
            else
            {
                arrStatusCodes[i] = OpcUa_BadInternalError;
            }
        }
        else
        {
            arrStatusCodes[i] = OpcUa_BadInternalError;
        }
    }

    return ret;
}

#if SUPPORT_Event_Subscription_Server_Facet
UaStatus NmBuildingAutomation::OnAcknowledge(
    const ServiceContext&  serviceContext, //!< [in] The client service context including the session object used for the call
    OpcUa::AcknowledgeableConditionType* pCondition, //!< [in] The affected acknowledgeable condition object
    const UaByteString&    EventId,        //!< [in] The EventId of the event the acknowledge is related to
    const UaLocalizedText& Comment)        //!< [in] The comment provied by the client
{
    OpcUa_ReferenceParameter(EventId);

    // Check if the condition is unacknowledged
    if ( pCondition->getAckedStateBool() != OpcUa_False )
    {
        return OpcUa_BadInvalidState;
    }

    // Chance condition to acknowledged
    pCondition->setAckedState(OpcUa_True);
    pCondition->setComment(Comment);
    pCondition->setClientUserId(serviceContext.pSession()->getClientUserId());
    pCondition->setMessage(UaLocalizedText("en", "Condition state acknowledged by UA client"));

    OpcUa::AlarmConditionType* pAlarmCondition = (OpcUa::AlarmConditionType*)pCondition;
    if ( pAlarmCondition->getActiveStateBool() == OpcUa_False )
    {
        pCondition->setRetain(OpcUa_False);
    }

    // Trigger state change event
    pCondition->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());

    return OpcUa_Good;
}
#endif // SUPPORT_Event_Subscription_Server_Facet

UaStatus NmBuildingAutomation::createTypeNodes()
{
    UaStatus ret;
    UaStatus addStatus;

    UaVariant                    defaultValue;
    UaObjectTypeSimple*          pControllerType = NULL;
    UaObjectTypeSimple*          pAirConditionerControllerType = NULL;
    UaObjectTypeSimple*          pFurnaceControllerType = NULL;
    OpcUa::BaseDataVariableType* pDataVariable;
    OpcUa::DataItemType*         pDataItem;
    OpcUa::AnalogItemType*       pAnalogItem;
    // Method helpers
#if SUPPORT_Method_Server_Facet
    OpcUa::BaseMethod*           pMethod = NULL;
    UaPropertyMethodArgument*    pPropertyArg = NULL;
    UaUInt32Array                nullarray;
#endif // SUPPORT_Method_Server_Facet
    // Event helpers
#if SUPPORT_Event_Subscription_Server_Facet
    UaObjectTypeSimple*          pControllerEventType = NULL;
    UaPropertyCache*             pProperty = NULL;
#endif // SUPPORT_Method_Server_Facet

    /**************************************************************
     * Create DataType nodes
     **************************************************************/
    addTypeDictionary(UaNodeId("BA", getNameSpaceIndex()), "BA");

    // Controller State Enumeration
    UaEnumDefinition controllerState;
    controllerState.setName("ControllerState");
    controllerState.setDataTypeId(UaNodeId(Ba_ControllerState, getNameSpaceIndex()));
    UaEnumValue controllerValue;
    controllerValue.setName("Off");
    controllerValue.setValue(0);
    controllerState.addChild(controllerValue);
    controllerValue.setName("On");
    controllerValue.setValue(1);
    controllerState.addChild(controllerValue);
    controllerValue.setName("Error");
    controllerValue.setValue(2);
    controllerState.addChild(controllerValue);

    addEnumeratedType(controllerState);

    // Controller configuration DataTypes
    UaStructureDefinition controllerConfiguration;
    controllerConfiguration.setName("ControllerConfiguration");
    controllerConfiguration.setDataTypeId(UaNodeId(Ba_ControllerConfiguration, getNameSpaceIndex()));
    controllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultBinary, getNameSpaceIndex()));
    controllerConfiguration.setXmlEncodingId(UaNodeId(Ba_ControllerConfiguration_DefaultXml, getNameSpaceIndex()));

    UaStructureField controllerConfigurationField;
    controllerConfigurationField.setName("Name");
    controllerConfigurationField.setDataTypeId(OpcUaId_String);
    controllerConfigurationField.setArrayType(UaStructureField::ArrayType_Scalar);
    controllerConfiguration.addChild(controllerConfigurationField);
    controllerConfigurationField.setName("DeviceAddress");
    controllerConfigurationField.setDataTypeId(OpcUaId_UInt32);
    controllerConfiguration.addChild(controllerConfigurationField);
    controllerConfigurationField.setName("TemperatureSetpoint");
    controllerConfigurationField.setDataTypeId(OpcUaId_Double);
    controllerConfiguration.addChild(controllerConfigurationField);

    addStructuredType(controllerConfiguration);

    UaStructureDefinition airConditionerControllerConfiguration
        = controllerConfiguration.createSubtype();
    airConditionerControllerConfiguration.setName("AirConditionerControllerConfiguration");
    airConditionerControllerConfiguration.setDataTypeId(UaNodeId(Ba_AirConditionerControllerConfiguration, getNameSpaceIndex()));
    airConditionerControllerConfiguration.setBinaryEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultBinary, getNameSpaceIndex()));
    airConditionerControllerConfiguration.setXmlEncodingId(UaNodeId(Ba_AirConditionerControllerConfiguration_DefaultXml, getNameSpaceIndex()));

    UaStructureField airConditionerControllerConfigurationField;
    airConditionerControllerConfigurationField.setName("HumiditySetpoint");
    airConditionerControllerConfigurationField.setDataTypeId(OpcUaId_Double);
    airConditionerControllerConfigurationField.setArrayType(UaStructureField::ArrayType_Scalar);
    airConditionerControllerConfiguration.addChild(airConditionerControllerConfigurationField);

    addStructuredType(airConditionerControllerConfiguration);

    UaStructureDefinition baConfigurartions;
    baConfigurartions.setName("BAConfigurations");
    baConfigurartions.setDataTypeId(UaNodeId(Ba_Configurations, getNameSpaceIndex()));
    baConfigurartions.setBinaryEncodingId(UaNodeId(Ba_Configurations_DefaultBinary, getNameSpaceIndex()));
    baConfigurartions.setXmlEncodingId(UaNodeId(Ba_Configurations_DefaultXml, getNameSpaceIndex()));

    UaStructureField baConfigurartionsField;
    baConfigurartionsField.setName("AirConditionerControllers");
    baConfigurartionsField.setStructureDefinition(airConditionerControllerConfiguration);
    baConfigurartionsField.setArrayType(UaStructureField::ArrayType_Array);
    baConfigurartions.addChild(baConfigurartionsField);
    baConfigurartionsField.setName("FurnaceControllers");
    baConfigurartionsField.setStructureDefinition(controllerConfiguration);
    baConfigurartions.addChild(baConfigurartionsField);

    addStructuredType(baConfigurartions);

    /**************************************************************
     * Create the Controller Type
     **************************************************************/
    // Add ObjectType "ControllerType"
    pControllerType = new UaObjectTypeSimple(
        "ControllerType",    // Used as string in browse name and display name
        UaNodeId(Ba_ControllerType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pControllerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Controller Type Instance declaration
     ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    defaultValue.setInt32(0);
    pDataVariable = new OpcUa::BaseDataVariableType(
        UaNodeId(Ba_ControllerType_State, getNameSpaceIndex()), // NodeId of the Variable
        "State",                // Name of the Variable
        getNameSpaceIndex(),    // Namespace index of the browse name (same like NodeId)
        defaultValue,           // Initial value
        Ua_AccessLevel_CurrentRead, // Access level
        this);                  // Node manager for this variable
    pDataVariable->setDataType(UaNodeId(Ba_ControllerState, getNameSpaceIndex()));
    pDataVariable->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pDataVariable, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

#if SUPPORT_Event_Subscription_Server_Facet
    // Add state alarm condition "StateCondition" as compontent to the object type
    OpcUa::OffNormalAlarmType* pOffNormalAlarm = new OpcUa::OffNormalAlarmType(
        UaNodeId(Ba_ControllerType_StateCondition, getNameSpaceIndex()), // NodeId of the node
        "StateCondition",      // Name of the node used for browse name and display name
        getNameSpaceIndex(),   // Namespace index of the browse name
        this,                  // Node manager responsible for this node
        pControllerType->nodeId(),             // NodeId of the source node
        pControllerType->browseName().name()); // Name of the source node
    pOffNormalAlarm->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pOffNormalAlarm, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
#endif // SUPPORT_Event_Subscription_Server_Facet

    // Add Variable "Temperature" as AnalogItem
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_ControllerType_Temperature, getNameSpaceIndex()),
        "Temperature",
        getNameSpaceIndex(),
        defaultValue,
#if SUPPORT_Historical_Access
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_HistoryRead,
#else // SUPPORT_Historical_Access
        Ua_AccessLevel_CurrentRead,
#endif // SUPPORT_Historical_Access
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    UaRange tempRange(0, 100);
    pAnalogItem->setEURange(tempRange);
    // Create unit from OPC UA defined engineering units degrees fahrenheit
    UaEUInformation tempEUInformation(UaEUInformation::EngineeringUnit_degree_Fahrenheit);
    pAnalogItem->setEngineeringUnits(tempEUInformation);
#if SUPPORT_Historical_Access
    // Set Historizing flag to indicate that the server collects history for the variable
    pAnalogItem->setHistorizing(OpcUa_True);

    // Create HA Configuration node with all children
    OpcUa::HistoricalDataConfigurationType* pHAConfig = NULL;
    pHAConfig = new OpcUa::HistoricalDataConfigurationType(
        UaNodeId(Ba_ControllerType_Temperature_HA_Configuration, getNameSpaceIndex()),
        "HA Configuration",
        0,
        this);
    // Set configuration options or use default values
    pHAConfig->setStepped(OpcUa_False);
    // Add HA Configuration node
    addStatus = addNodeAndReference(pAnalogItem, pHAConfig, OpcUaId_HasHistoricalConfiguration);
    UA_ASSERT(addStatus.isGood());
#endif // SUPPORT_Historical_Access

    // Add Variable "TemperatureSetPoint" as AnalogItem
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_ControllerType_TemperatureSetPoint, getNameSpaceIndex()),
        "TemperatureSetPoint",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    pAnalogItem->setEngineeringUnits(tempEUInformation);

    // Add Variable "PowerConsumption"
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(Ba_ControllerType_PowerConsumption, getNameSpaceIndex()),
        "PowerConsumption",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

#if SUPPORT_Method_Server_Facet
    // Add Method "Start"
    pMethod = new OpcUa::BaseMethod(UaNodeId(Ba_ControllerType_Start, getNameSpaceIndex()), "Start", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(Ba_ControllerType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pControllerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
#endif // SUPPORT_Method_Server_Facet


    /**************************************************************
     * Create the AirConditionerController Type
     **************************************************************/
    pAirConditionerControllerType = new UaObjectTypeSimple(
        "AirConditionerControllerType", // Used as string in browse name and display name
        UaNodeId(Ba_AirConditionerControllerType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,
        OpcUa_False);
    // Add Object Type node to address space and create reference to Controler type
    addStatus = addNodeAndReference(pControllerType, pAirConditionerControllerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the AirConditionerController Type Instance declaration
     ***************************************************************/
    // Add Variable "Humidity"
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_AirConditionerControllerType_Humidity, getNameSpaceIndex()),
        "Humidity",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pAirConditionerControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    // Create unit from OPC UA defined engineering units percent
    UaEUInformation tempEUPercent(UaEUInformation::EngineeringUnit_percent);
    pAnalogItem->setEngineeringUnits(tempEUPercent);

    // Add Variable "HumiditySetpoint"
    defaultValue.setDouble(0);
    pAnalogItem = new OpcUa::AnalogItemType(
        UaNodeId(Ba_AirConditionerControllerType_HumiditySetpoint, getNameSpaceIndex()),
        "HumiditySetpoint",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
        this);
    pAnalogItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pAirConditionerControllerType, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    pAnalogItem->setEngineeringUnits(tempEUPercent);

#if SUPPORT_Method_Server_Facet
    // Add Method "StartWithSetpoint"
    pMethod = new OpcUa::BaseMethod(
        UaNodeId(Ba_AirConditionerControllerType_StartWithSetpoint, getNameSpaceIndex()),
        "StartWithSetpoint",
        getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pAirConditionerControllerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(Ba_AirConditionerControllerType_StartWithSetpoint_In, getNameSpaceIndex()), // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        2,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetPoint
    pPropertyArg->setArgument(
        0,                       // Index of the argument
        "TemperatureSetPoint",   // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                      // Array rank of the argument
        nullarray,               // Array dimensions of the argument
        UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Argument HumiditySetpoint
    pPropertyArg->setArgument(
        1,                       // Index of the argument
        "HumiditySetpoint",      // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                      // Array rank of the argument
        nullarray,               // Array dimensions of the argument
        UaLocalizedText("en", "Controller set point for humidity")); // Description
    // Add property to method
    addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());
#endif // SUPPORT_Method_Server_Facet

    /**************************************************************
     * Create the FurnaceController Type
     **************************************************************/
    pFurnaceControllerType = new UaObjectTypeSimple(
        "FurnaceControllerType", // Used as string in browse name and display name
        UaNodeId(Ba_FurnaceControllerType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,
        OpcUa_False);
    // Add Object Type node to address space and create reference to Controler type
    addStatus = addNodeAndReference(pControllerType, pFurnaceControllerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the FurnaceController Type Instance declaration
     **************************************************************/
    // Add Variable "GasFlow"
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(Ba_FurnaceControllerType_GasFlow, getNameSpaceIndex()),
        "GasFlow",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pFurnaceControllerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

#if SUPPORT_Method_Server_Facet
    // Add Method "StartWithSetpoint"
    pMethod = new OpcUa::BaseMethod(
        UaNodeId(Ba_FurnaceControllerType_StartWithSetpoint, getNameSpaceIndex()),
        "StartWithSetpoint",
        getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pFurnaceControllerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(Ba_FurnaceControllerType_StartWithSetpoint_In, getNameSpaceIndex()), // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        1,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetPoint
    pPropertyArg->setArgument(
        0,                       // Index of the argument
        "TemperatureSetPoint",   // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        -1,                      // Array rank of the argument
        nullarray,               // Array dimensions of the argument
        UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Add property to method
    addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());
#endif // SUPPORT_Method_Server_Facet

#if SUPPORT_Event_Subscription_Server_Facet
    /**************************************************************
     * Create the ControllerEventType and its event field properties
     **************************************************************/
    pControllerEventType = new UaObjectTypeSimple(
        "ControllerEventType", // Used as string in browse name and display name
        UaNodeId(Ba_ControllerEventType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,
        OpcUa_True);
    // Add Event Type node to address space as subtype of BaseEventType
    addStatus = addNodeAndReference(OpcUaId_BaseEventType, pControllerEventType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    // Temperature Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "Temperature",
        UaNodeId(Ba_ControllerEventType_Temperature, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pControllerEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // State Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "State",
        UaNodeId(Ba_ControllerEventType_State, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pControllerEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // Create Reference "GeneratesEvent" between Controller Type and ControllerEventType
    addStatus = addUaReference(
        UaNodeId(Ba_ControllerType, getNameSpaceIndex()),
        UaNodeId(Ba_ControllerEventType, getNameSpaceIndex()),
        OpcUaId_GeneratesEvent);
    UA_ASSERT(addStatus.isGood());

    // Register the event data class with the BaseEventType to allow selection of custom event fields
    ControllerEventTypeData eventTypeData(getNameSpaceIndex());
    eventTypeData.registerEventFields();
#endif // SUPPORT_Event_Subscription_Server_Facet

    return ret;
}

UaVariable* NmBuildingAutomation::getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier)
{
    // Try to find the instance declaration node with the numeric identifier
    // and the namespace index of this node manager
    UaNode* pNode = findNode(UaNodeId(numericIdentifier, getNameSpaceIndex()));
    if ( (pNode != NULL) && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        // Return the node if valid and a variable
        return (UaVariable*)pNode;
    }
    else
    {
        return NULL;
    }
}

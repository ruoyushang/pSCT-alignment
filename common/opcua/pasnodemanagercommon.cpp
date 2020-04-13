#include "pasnodemanagercommon.hpp"
#include "passervertypeids.hpp"
#include "pasobject.hpp"
#include "pascominterfacecommon.hpp"
#include "mpeseventdata.hpp"
#include "uaserver/opcua_offnormalalarmtype.h"
#include "uaserver/opcua_analogitemtype.h"
#include "uavariant.h"
#include "uaserver/opcua_foldertype.h"

PasNodeManagerCommon::PasNodeManagerCommon()
: NodeManagerBase("urn:UnifiedAutomation:CppServer:P2PAS", OpcUa_True),
    // SamplingOnRequestExample change begin
    // Added: Initialization of new members
    m_stopThread(false),
    m_changedMonitoredVariables(false)
    // SamplingOnRequestExample change end
{
}

UaStatus PasNodeManagerCommon::readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues)
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

        // Cast UaVariable to PasUserData
        UaVariable* pVariable = arrUaVariables[i];
        if (pVariable)
        {
            auto pUserData = (PasUserData *) pVariable->getUserData();

            if ( pUserData )
            {
                UaVariant    vTemp;

                if ( pUserData->isState() == OpcUa_False )
                {
                    // Read of a data variable
                    // Get the data for the sensor from the communication interface
                    ret = m_pCommIf->getDeviceData(pUserData->DeviceType(), pUserData->DeviceId(),
                                                   pUserData->variableOffset(), vTemp);
                    if (ret.isGood())
                    {
                        // Set value
                        arrDataValues[i].setValue(vTemp, OpcUa_True, OpcUa_False);
                    }
                    else
                    {
                        // Set Error
                        arrDataValues[i].setStatusCode(ret.statusCode());
                    }
                }
                else
                {
                    // Read of a state variable
                    // We need to get the state of the sensor
                    Device::DeviceState state;

                    // Get the data for the sensor from the communication interface
                    ret = m_pCommIf->getDeviceState(pUserData->DeviceType(), pUserData->DeviceId(),
                                                    state);
                    if (ret.isGood())
                    {
                        // Set value
                        vTemp.setUInt32(static_cast<unsigned>(state));
                        arrDataValues[i].setValue(vTemp, OpcUa_True, OpcUa_False);
                    }
                    else
                    {
                        // Set Error
                        arrDataValues[i].setStatusCode(ret.statusCode());
                    }
                }
            }
            else
            {
                arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
            }
        }
        else
        {
            arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
        }
    }

    return ret;
}

UaStatus PasNodeManagerCommon::writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes)
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
            auto pUserData = (PasUserData *) pVariable->getUserData();

            if ( pUserData )
            {
                if ( pUserData->isState() == OpcUa_False )
                {
                    UaVariant vTemp(arrpDataValues[i]->Value);

                    if (ret.isGood())
                    {
                        // Get the data for the controller from the communication interface
                        ret = m_pCommIf->setDeviceData(pUserData->DeviceType(), pUserData->DeviceId(),
                                                       pUserData->variableOffset(), vTemp);
                    }
                    arrStatusCodes[i] = ret.statusCode();
                }
                else
                {
                    UaVariant vTemp(arrpDataValues[i]->Value);
                    UaStatus status;

                    if ( status.isGood() )
                    {
                        unsigned stateInt;
                        vTemp.toUInt32(stateInt);
                        // Get the data for the controller from the communication interface
                        status = m_pCommIf->setDeviceState(pUserData->DeviceType(), pUserData->DeviceId(),
                                                           static_cast<Device::DeviceState>(stateInt));
                    }
                    arrStatusCodes[i] = status.statusCode();
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

UaStatus PasNodeManagerCommon::OnAcknowledge(
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

    // Change condition to acknowledged
    pCondition->setAckedState(OpcUa_True);
    pCondition->setComment(Comment);
    pCondition->setClientUserId(serviceContext.pSession()->getClientUserId());
    pCondition->setMessage(UaLocalizedText("en", "Condition state acknowledged by UA client"));

    auto pAlarmCondition = (OpcUa::AlarmConditionType *) pCondition;
    if ( pAlarmCondition->getActiveStateBool() == OpcUa_False )
    {
        pCondition->setRetain(OpcUa_False);
    }

    // Trigger state change event
    pCondition->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());

    return OpcUa_Good;
}

UaStatus PasNodeManagerCommon::createTypeNodes()
{
    UaStatus ret;
    UaStatus addStatus;

    UaVariant                    defaultValue;
    UaObjectTypeSimple *pMPESType = nullptr;
    UaObjectTypeSimple *pACTType = nullptr;
    UaObjectTypeSimple *pPSDType = nullptr;
    OpcUa::DataItemType*         pDataItem;
    // Method helpers
    OpcUa::BaseMethod *pMethod = nullptr;
    UaPropertyMethodArgument *pPropertyArg = nullptr;
    // Event helpers
    UaObjectTypeSimple *pMPESEventType = nullptr;
    UaPropertyCache *pProperty = nullptr;

    UaUInt32Array nullarray;

    /**************************************************************
     * Create the MPES Type
     **************************************************************/
    // Add ObjectType "MPESType"
    pMPESType = new UaObjectTypeSimple(
        "MPESType",    // Used as string in browse name and display name
        UaNodeId(PAS_MPESType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pMPESType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the MPES Type Instance declaration
     ***************************************************************/
    // Register all variables
    for (auto v : MPESObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : MPESObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : MPESObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pMPESType, pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Add arguments
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId((std::to_string(m.first) + "_" + m.second.first + "_args").c_str(),
                     getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            m.second.second.size(),                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        for (size_t i = 0; i < m.second.second.size(); i++) {
            pPropertyArg->setArgument(
                (OpcUa_UInt32) i,                       // Index of the argument
                std::get<0>(m.second.second[i]).c_str(),   // Name of the argument
                std::get<1>(m.second.second[i]),// Data type of the argument
                -1,                      // Array rank of the argument
                nullarray,               // Array dimensions of the argument
                UaLocalizedText("en", (std::get<2>(m.second.second[i])).c_str())); // Description
        }
        addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }

    /**************************************************************
     * Create the MPESEventType and its event field properties
     **************************************************************/
    pMPESEventType = new UaObjectTypeSimple(
        "MPESEventType", // Used as string in browse name and display name
        UaNodeId(PAS_MPESEventType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,
        OpcUa_True);
    // Add Event Type node to address space as subtype of BaseEventType
    addStatus = addNodeAndReference(OpcUaId_BaseEventType, pMPESEventType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());


    // State Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "State",
        UaNodeId(PAS_MPESEventType_State, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pMPESEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // xCentroidAvg Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "xCentroidAvg",
        UaNodeId(PAS_MPESEventType_xCentroidAvg, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pMPESEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // yCentroidAvg Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "yCentroidAvg",
        UaNodeId(PAS_MPESEventType_yCentroidAvg, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pMPESEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // xCentroidSpotWidth Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "xCentroidSpotWidth",
        UaNodeId(PAS_MPESEventType_xCentroidSpotWidth, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pMPESEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // yCentroidSpotWidth Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "yCentroidSpotWidth",
        UaNodeId(PAS_MPESEventType_yCentroidSpotWidth, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pMPESEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // Cleaned Intensity Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "CleanedIntensity",
        UaNodeId(PAS_MPESEventType_CleanedIntensity, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pMPESEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // Create Reference "GeneratesEvent" between MPESType and MPESEventType
    addStatus = addUaReference(
        UaNodeId(PAS_MPESType, getNameSpaceIndex()),
        UaNodeId(PAS_MPESEventType, getNameSpaceIndex()),
        OpcUaId_GeneratesEvent);
    UA_ASSERT(addStatus.isGood());

    // Register the event data class with the BaseEventType to allow selection of custom event fields
    MPESEventTypeData eventTypeData(getNameSpaceIndex());
    eventTypeData.registerEventFields();

    /**************************************************************
     * Create the ACT Type
     **************************************************************/
    // Add ObjectType "ACTType"
    pACTType = new UaObjectTypeSimple(
        "ACTType",    // Used as string in browse name and display name
        UaNodeId(PAS_ACTType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pACTType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the ACT Type Instance declaration
     ***************************************************************/

    // Register all variables
    for (auto v : ACTObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pACTType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : ACTObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pACTType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : ACTObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pACTType, pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Add arguments
        pPropertyArg = new UaPropertyMethodArgument(
                UaNodeId((std::to_string(m.first) + "_" + m.second.first + "_args").c_str(),
                         getNameSpaceIndex()), // NodeId of the property
                Ua_AccessLevel_CurrentRead,             // Access level of the property
                m.second.second.size(),                                      // Number of arguments
                UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        for (size_t i = 0; i < m.second.second.size(); i++) {
            pPropertyArg->setArgument(
                    (OpcUa_UInt32) i,                       // Index of the argument
                    std::get<0>(m.second.second[i]).c_str(),   // Name of the argument
                    std::get<1>(m.second.second[i]),// Data type of the argument
                    -1,                      // Array rank of the argument
                    nullarray,               // Array dimensions of the argument
                    UaLocalizedText("en", (std::get<2>(m.second.second[i])).c_str())); // Description
        }
        addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }

    /**************************************************************
     * Create the PSD Type
     **************************************************************/
    // Add ObjectType "PSDType"
    pPSDType = new UaObjectTypeSimple(
            "PSDType",    // Used as string in browse name and display name
            UaNodeId(PAS_PSDType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pPSDType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the ACT Type Instance declaration
     ***************************************************************/

    // Register all variables
    for (auto v : PSDObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : PSDObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : PSDObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pPSDType, pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Add arguments
        pPropertyArg = new UaPropertyMethodArgument(
                UaNodeId((std::to_string(m.first) + "_" + m.second.first + "_args").c_str(),
                         getNameSpaceIndex()), // NodeId of the property
                Ua_AccessLevel_CurrentRead,             // Access level of the property
                m.second.second.size(),                                      // Number of arguments
                UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        for (size_t i = 0; i < m.second.second.size(); i++) {
            pPropertyArg->setArgument(
                    (OpcUa_UInt32) i,                       // Index of the argument
                    std::get<0>(m.second.second[i]).c_str(),   // Name of the argument
                    std::get<1>(m.second.second[i]),// Data type of the argument
                    -1,                      // Array rank of the argument
                    nullarray,               // Array dimensions of the argument
                    UaLocalizedText("en", (std::get<2>(m.second.second[i])).c_str())); // Description
        }
        addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }

    return ret;
}

UaVariable* PasNodeManagerCommon::getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier)
{
    // Try to find the instance declaration node with the numeric identifier
    // and the namespace index of this node manager
    UaNode* pNode = findNode(UaNodeId(numericIdentifier, getNameSpaceIndex()));
    if ((pNode != nullptr) && (pNode->nodeClass() == OpcUa_NodeClass_Variable))
    {
        // Return the node if valid and a variable
        return (UaVariable*)pNode;
    }
    else
    {
        return nullptr;
    }
}

// SamplingOnRequestExample change begin
/** Overwrite of base class function to get informed by NodeManagerBase about a change in monitoring
 */
void PasNodeManagerCommon::variableCacheMonitoringChanged(UaVariableCache* pVariable, TransactionType transactionType)
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
void PasNodeManagerCommon::run()
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
                    PasUserData* pUserData = (PasUserData*)m_arrayMonitoredVariables[i]->getUserData();
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
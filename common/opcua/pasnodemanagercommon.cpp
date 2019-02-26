#include "pasnodemanagercommon.h"
#include "passervertypeids.h"
#include "pasobject.h"
#include "pascominterfacecommon.h"
#include "mpeseventdata.h"
#include "uaserver/opcua_offnormalalarmtype.h"
#include "uaserver/opcua_analogitemtype.h"
#include "uavariant.h"

PasNodeManagerCommon::PasNodeManagerCommon()
: NodeManagerBase("urn:UnifiedAutomation:CppServer:P2PAS", OpcUa_True)
{
}


PasNodeManagerCommon::~PasNodeManagerCommon()
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
            PasUserData* pUserData = (PasUserData*)pVariable->getUserData();

            if ( pUserData )
            {
                UaVariant    vTemp;
                UaStatusCode status;

                if ( pUserData->isState() == OpcUa_False )
                {
                    // Read of a data variable
                    // Get the data for the sensor from the communication interface
                    status = m_pCommIf->getDeviceData(pUserData->DeviceType(), pUserData->DeviceId(),
                            pUserData->variableOffset(), vTemp);
                    if ( status.isGood() )
                    {
                        // Set value
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
                    // Read of a state variable
                    // We need to get the state of the sensor
                    PASState state;

                    // Get the data for the sensor from the communication interface
                    status = m_pCommIf->getDeviceState(pUserData->DeviceType(), pUserData->DeviceId(),
                            state);
                    if ( status.isGood() )
                    {
                        // Set value
                        vTemp.setUInt32(static_cast<unsigned>(state));
                        arrDataValues[i].setValue(vTemp, OpcUa_True, OpcUa_False);
                    }
                    else
                    {
                        // Set Error
                        arrDataValues[i].setStatusCode(status.statusCode());
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
            PasUserData* pUserData = (PasUserData*)pVariable->getUserData();

            if ( pUserData )
            {
                if ( pUserData->isState() == OpcUa_False )
                {
                    UaVariant vTemp(arrpDataValues[i]->Value);
                    UaStatusCode status;

                    if ( status.isGood() )
                    {
                        // Get the data for the controller from the communication interface
                        status = m_pCommIf->setDeviceData(pUserData->DeviceType(), pUserData->DeviceId(),
                                pUserData->variableOffset(), vTemp);
                    }
                    arrStatusCodes[i] = status.statusCode();
                }
                else
                {
                    UaVariant vTemp(arrpDataValues[i]->Value);
                    UaStatusCode status;

                    if ( status.isGood() )
                    {
                        unsigned stateInt;
                        vTemp.toUInt32(stateInt);
                        // Get the data for the controller from the communication interface
                        status = m_pCommIf->setDeviceState(pUserData->DeviceType(), pUserData->DeviceId(), static_cast<PASState>(stateInt));
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

    OpcUa::AlarmConditionType* pAlarmCondition = (OpcUa::AlarmConditionType*)pCondition;
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
    UaObjectTypeSimple*          pMPESType = NULL;
    UaObjectTypeSimple*          pACTType = NULL;
    UaObjectTypeSimple*          pPSDType = NULL;
    OpcUa::DataItemType*         pDataItem;
    // Method helpers
    OpcUa::BaseMethod*           pMethod = NULL;
    // Event helpers
    UaObjectTypeSimple*          pMPESEventType = NULL;
    UaPropertyCache*             pProperty = NULL;

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
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_State, getNameSpaceIndex()), // NodeId of the Variable
        "State",                // Name of the Variable
        getNameSpaceIndex(),    // Namespace index of the browse name (same like NodeId)
        defaultValue,           // Initial value
        Ua_AccessLevel_CurrentRead, // Access level
        this);                  // Node manager for this variable
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add state alarm condition "StateCondition" as component to the object type
    OpcUa::OffNormalAlarmType* pOffNormalAlarm = new OpcUa::OffNormalAlarmType(
        UaNodeId(PAS_MPESType_StateCondition, getNameSpaceIndex()), // NodeId of the node
        "StateCondition",      // Name of the node used for browse name and display name
        getNameSpaceIndex(),   // Namespace index of the browse name
        this,                  // Node manager responsible for this node
        pMPESType->nodeId(),             // NodeId of the source node
        pMPESType->browseName().name()); // Name of the source node
    pOffNormalAlarm->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pOffNormalAlarm, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "xCentroidAvg" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_xCentroidAvg, getNameSpaceIndex()),
        "xCentroidAvg",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "yCentroidAvg" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_yCentroidAvg, getNameSpaceIndex()),
        "yCentroidAvg",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "xCentroidSD" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_xCentroidSD, getNameSpaceIndex()),
        "xCentroidSD",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "yCentroidSD" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_yCentroidSD, getNameSpaceIndex()),
        "yCentroidSD",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "CleanedIntensity" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_CleanedIntensity, getNameSpaceIndex()),
        "CleanedIntensity",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    // Add Variable "xCentroidNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_xCentroidNominal, getNameSpaceIndex()),
        "xCentroidNominal",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "yCentroidNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MPESType_yCentroidNominal, getNameSpaceIndex()),
        "yCentroidNominal",
        getNameSpaceIndex(),
        defaultValue,
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    // Add Method "Start"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MPESType_Start, getNameSpaceIndex()), "Start", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MPESType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MPESType_Read, getNameSpaceIndex()), "Read", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "setExposure"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MPESType_SetExposure, getNameSpaceIndex()), "SetExposure", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMPESType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


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

    // xCentroidSD Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "xCentroidSD",
        UaNodeId(PAS_MPESEventType_xCentroidSD, getNameSpaceIndex()),
        defaultValue,
        Ua_AccessLevel_CurrentRead,
        m_defaultLocaleId);
    addStatus = addNodeAndReference(pMPESEventType, pProperty, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    // yCentroidSD Event field property
    defaultValue.setDouble(0);
    pProperty = new UaPropertyCache(
        "yCentroidSD",
        UaNodeId(PAS_MPESEventType_yCentroidSD, getNameSpaceIndex()),
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
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_ACTType_State, getNameSpaceIndex()), // NodeId of the Variable
        "State",                // Name of the Variable
        getNameSpaceIndex(),    // Namespace index of the browse name (same like NodeId)
        defaultValue,           // Initial value
        Ua_AccessLevel_CurrentRead, // Access level
        this);                  // Node manager for this variable
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pACTType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "Steps" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_ACTType_Steps, getNameSpaceIndex()),
            "Steps", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pACTType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "curLength_mm" as DataItem
    defaultValue.setFloat(0);
    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_ACTType_curLength_mm, getNameSpaceIndex()), "curLength_mm",
            getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pACTType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Variable "curLength_mm" as DataItem
    defaultValue.setFloat(0);
    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_ACTType_inLength_mm, getNameSpaceIndex()), "inLength_mm",
            getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pACTType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    // Add Method "Start"
    pMethod = new OpcUa::BaseMethod(
            UaNodeId(PAS_ACTType_Start, getNameSpaceIndex()), "Start", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pACTType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(
            UaNodeId(PAS_ACTType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pACTType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Step"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_ACTType_Step, getNameSpaceIndex()), "Step", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pACTType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the PSD Type and instance decalaration
     **************************************************************/
    pPSDType = new UaObjectTypeSimple("PSDType", UaNodeId(PAS_PSDType, getNameSpaceIndex()),
            m_defaultLocaleId, OpcUa_True);
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pPSDType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    // add the variables to the type
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_State, getNameSpaceIndex()), "State", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_x1, getNameSpaceIndex()), "x1", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_y1, getNameSpaceIndex()), "y1", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_x2, getNameSpaceIndex()), "x2", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_y2, getNameSpaceIndex()), "y2", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_dx1, getNameSpaceIndex()), "dx1", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_dy1, getNameSpaceIndex()), "dy1", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_dx2, getNameSpaceIndex()), "dx2", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_dy2, getNameSpaceIndex()), "dy2", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(
            UaNodeId(PAS_PSDType_Temp, getNameSpaceIndex()), "Temperature", getNameSpaceIndex(),
            defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPSDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    return ret;
}

UaVariable* PasNodeManagerCommon::getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier)
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

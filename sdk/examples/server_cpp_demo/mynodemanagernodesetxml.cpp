#include "mynodemanagernodesetxml.h"
#include "bacommunicationinterface.h"
#include "bauserdata.h"
#include "methodhandleuanode.h"
#if SUPPORT_Event_Subscription_Server_Facet
#include "opcua_offnormalalarmtype.h"
#endif // SUPPORT_Event_Subscription_Server_Facet
#include "opcua_multistatediscretetype.h"
#include "opcua_multistatevaluediscretetype.h"

class BaControllerUserData : public UserDataBase
{
    UA_DISABLE_COPY(BaControllerUserData);
protected:
    BaControllerUserData()
    : m_deviceAddress(0)
    {}
public:
    BaControllerUserData(
        OpcUa_UInt32    deviceAddress,
        const UaNodeId& nidStartMethod,
        const UaNodeId& nidStopMethod)
    : m_deviceAddress(deviceAddress),
#if SUPPORT_Event_Subscription_Server_Facet
      m_pStateCondition(NULL),
#endif // SUPPORT_Event_Subscription_Server_Facet
      m_nidStartMethod(nidStartMethod),
      m_nidStopMethod(nidStopMethod)
    {}
    virtual ~BaControllerUserData()
    {
#if SUPPORT_Event_Subscription_Server_Facet
        if ( m_pStateCondition )
        {
            m_pStateCondition->releaseReference();
            m_pStateCondition = NULL;
        }
#endif // SUPPORT_Event_Subscription_Server_Facet
    }

    /** Returns the device address. */
    inline OpcUa_UInt32 deviceAddress() const { return m_deviceAddress; }
    inline UaNodeId nidStartMethod() const { return m_nidStartMethod; }
    inline UaNodeId nidStopMethod() const { return m_nidStopMethod; }

#if SUPPORT_Event_Subscription_Server_Facet
    inline OpcUa::OffNormalAlarmType* pStateCondition() const { return m_pStateCondition; }
    inline void setStateCondition(OpcUa::OffNormalAlarmType* pStateCondition) {m_pStateCondition = pStateCondition;}
#endif // SUPPORT_Event_Subscription_Server_Facet

protected:
    OpcUa_UInt32  m_deviceAddress;
#if SUPPORT_Event_Subscription_Server_Facet
    OpcUa::OffNormalAlarmType* m_pStateCondition;
#endif // SUPPORT_Event_Subscription_Server_Facet
    UaNodeId      m_nidStartMethod;
    UaNodeId      m_nidStopMethod;
};


class CttUserData : public UserDataVariableGetValue
{
    UA_DISABLE_COPY(CttUserData);
protected:
    CttUserData()
    : m_pVariable(NULL)
    {}
public:
    CttUserData(UaVariable* pVariable)
    : m_pVariable(pVariable)
    {
    }
    virtual ~CttUserData()
    {
    }

    virtual OpcUa_StatusCode getValue(UaVariant& value) const
    {
        UaDataValue dataValue(m_pVariable->value(NULL));
        value = *dataValue.value();
        return dataValue.statusCode();
    }
    virtual void getDataValue(UaDataValue& dataValue) const
    {
        dataValue = m_pVariable->value(NULL);
    }
    virtual OpcUa_StatusCode setDataValue(const UaDataValue& dataValue)
    {
        return m_pVariable->setValue(NULL, dataValue, OpcUa_True).statusCode();
    }

protected:
    UaVariable* m_pVariable;
};


MyNodeManagerNodeSetXml::MyNodeManagerNodeSetXml(const UaString& sNamespaceUri)
: NodeManagerNodeSetXml(sNamespaceUri)
{
    m_pCommIf = new BaCommunicationInterface;
}

MyNodeManagerNodeSetXml::~MyNodeManagerNodeSetXml()
{
    delete m_pCommIf;
}

void MyNodeManagerNodeSetXml::variableCreated(UaVariable* pNewNode, UaBase::Variable *pVariable)
{
    OpcUa_ReferenceParameter(pVariable);
    if ( pNewNode->getUserData() != NULL )
    {
        // Change value handling to get read and write calls to the node manager
        pNewNode->setValueHandling(UaVariable_Value_Cache);
    }
    else
    {
        UaUniString usNodeId(pNewNode->nodeId().toString().toUtf16());
        if ( usNodeId.startsWith("Demo.CTT.AllProfiles") || usNodeId.startsWith("Demo.CTT.DAProfile") )
        {
            // Change value handling to get read and write calls to the node manager
            pNewNode->setValueHandling(UaVariable_Value_Cache);
            // Set special user data
            CttUserData* pCttUserData = new CttUserData(pNewNode);
            pNewNode->setUserData(pCttUserData);
            // set initial value
            initializeVariableValue(pNewNode, pCttUserData);
        }
    }
}

void MyNodeManagerNodeSetXml::objectCreated(UaObject* pNewNode, UaBase::Object *pObject)
{
    OpcUa_ReferenceParameter(pObject);
    if ( (pNewNode->typeDefinitionId() == UaNodeId(1002/*AirConditionerControllerTypeXml*/, getNameSpaceIndex())) ||
         (pNewNode->typeDefinitionId() == UaNodeId(1003/*FurnaceControllerTypeXml*/, getNameSpaceIndex())) )
    {
        UaString sNodeId = pNewNode->nodeId().toString();
        UaString sChildNode = UaString("%1.State").arg(sNodeId);
        UaNode* pNode = findNode(UaNodeId(sChildNode, getNameSpaceIndex()));
        if ( pNode )
        {
            BaUserData* pVarUserData = (BaUserData*)pNode->getUserData();
            if ( pVarUserData )
            {
                sChildNode = UaString("%1.Start").arg(sNodeId);
                UaNodeId nidStartMethod(sChildNode, getNameSpaceIndex());
                sChildNode = UaString("%1.Stop").arg(sNodeId);
                UaNodeId nidStopMethod(sChildNode, getNameSpaceIndex());

                BaControllerUserData* pCtrlUserData = new BaControllerUserData(pVarUserData->deviceAddress(), nidStartMethod, nidStopMethod);
                pNewNode->setUserData(pCtrlUserData);

#if SUPPORT_Event_Subscription_Server_Facet
                // Check if we have the condition
                sChildNode = UaString("%1.StateCondition").arg(sNodeId);
                pNode = findNode(UaNodeId(sChildNode, getNameSpaceIndex()));
                if ( pNode )
                {
                    if ( pNode->typeDefinitionId() == OpcUaId_OffNormalAlarmType )
                    {
                        OpcUa::OffNormalAlarmType* pStateCondition = (OpcUa::OffNormalAlarmType*)pNode;

                        // Initialize condition
                        pStateCondition->setSourceNode(pNewNode->nodeId());
                        pStateCondition->setSourceName(pNewNode->browseName().toString());
                        pStateCondition->setConditionName("StateCondition");
                        pStateCondition->setAckedState(OpcUa_True);
                        pStateCondition->setEnabledState(OpcUa_True);

                        // Store condition node after incrementing reference count
                        pStateCondition->addReference();
                        pCtrlUserData->setStateCondition(pStateCondition);
                    }
                }
#endif // SUPPORT_Event_Subscription_Server_Facet
            }
        }
    }
}

UaStatus MyNodeManagerNodeSetXml::readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues)
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
        if (pVariable && pVariable->getUserData())
        {
            if ( pVariable->getUserData()->getUserDataType() == UserDataBase::UserDataType_Custom )
            {
                BaUserData* pUserData = (BaUserData*)pVariable->getUserData();

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
                        vTemp.setUInt32(state);
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
                CttUserData* pCttUserData = (CttUserData*)pVariable->getUserData();
                pCttUserData->getDataValue(arrDataValues[i]);
                arrDataValues[i].setServerTimestamp(timeStamp);
            }
        }
        else
        {
            arrDataValues[i].setStatusCode(OpcUa_BadInternalError);
        }
    }

    return ret;
}

UaStatus MyNodeManagerNodeSetXml::writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes)
{
    UaStatus ret;
    OpcUa_UInt32 i;
    OpcUa_UInt32 count = arrUaVariables.length();

    // Create result array
    arrStatusCodes.create(count);

    for ( i=0; i<count; i++ )
    {
        UaVariable* pVariable = arrUaVariables[i];

        if (pVariable && pVariable->getUserData())
        {
            // check valid values for MultiStateDiscreteType
            if (pVariable->typeDefinitionId() == UaNodeId(OpcUaId_MultiStateDiscreteType))
            {
                OpcUa::MultiStateDiscreteType* pMultiStateVar = (OpcUa::MultiStateDiscreteType*)pVariable;
                UaLocalizedTextArray enumStrings;
                pMultiStateVar->getEnumStrings(enumStrings);
                UaVariant varTmp(arrpDataValues[i]->Value);

                // check for correct datatype
                if (pVariable->dataType() == varTmp.dataType())
                {
                    OpcUa_UInt64 newValue;
                    if (OpcUa_IsGood(varTmp.toUInt64(newValue)))
                    {
                        // Check for valid range
                        if (newValue < enumStrings.length())
                        {
                            UaDataValue dataValue(*arrpDataValues[i]);
                            dataValue.setSourceTimestamp(UaDateTime::now());
                            pVariable->setValue(NULL, dataValue);
                        }
                        else
                        {
                            arrStatusCodes[i] = OpcUa_BadOutOfRange;
                        }
                    }
                    else
                    {
                        arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                    }
                }
                else
                {
                    arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                }
            }
            // check valid values for MultiStateValueDiscreteType
            else if (pVariable->typeDefinitionId() == UaNodeId(OpcUaId_MultiStateValueDiscreteType))
            {
                OpcUa::MultiStateValueDiscreteType* pMultiStateVar = (OpcUa::MultiStateValueDiscreteType*)pVariable;
                UaEnumValueTypes enumValues;
                pMultiStateVar->getEnumValues(enumValues);
                UaVariant varTmp(arrpDataValues[i]->Value);

                // check for correct datatype
                if (pVariable->dataType() == varTmp.dataType())
                {
                    OpcUa_Int64 newValue;
                    if (OpcUa_IsGood(varTmp.toInt64(newValue)))
                    {
                        // Check for valid value
                        bool bFound = false;
                        for (OpcUa_UInt32 j=0; j<enumValues.length(); j++)
                        {
                            if (newValue == enumValues[j].Value)
                            {
                                bFound = true;
                                break;
                            }
                        }
                        if (bFound)
                        {
                            UaDataValue dataValue(*arrpDataValues[i]);
                            dataValue.setSourceTimestamp(UaDateTime::now());
                            pVariable->setValue(NULL, dataValue);
                        }
                        else
                        {
                            arrStatusCodes[i] = OpcUa_BadOutOfRange;
                        }
                    }
                    else
                    {
                        arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                    }
                }
                else
                {
                    arrStatusCodes[i] = OpcUa_BadTypeMismatch;
                }
            }

            if (OpcUa_IsBad(arrStatusCodes[i]))
            {
                continue;
            }

            if ( pVariable->getUserData()->getUserDataType() == UserDataBase::UserDataType_Custom )
            {
                BaUserData* pUserData = (BaUserData*)pVariable->getUserData();

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
                }
                else
                {
                    // State variable can not be written
                    arrStatusCodes[i] = OpcUa_BadNotWritable;
                }
            }
            else
            {
                CttUserData* pCttUserData = (CttUserData*)pVariable->getUserData();
                UaDataValue dataValue(*arrpDataValues[i]);
                arrStatusCodes[i] = pCttUserData->setDataValue(dataValue);
            }
        }
        else
        {
            arrStatusCodes[i] = OpcUa_BadInternalError;
        }
    }

    return ret;
}

#if SUPPORT_Method_Server_Facet

MethodHandle* MyNodeManagerNodeSetXml::getMethodHandle(
    Session*       pSession,
    OpcUa_NodeId*  pObjectNodeId,
    OpcUa_NodeId*  pMethodNodeId,
    UaStatus&      result) const
{
    UaMutexLocker lock((UaMutex*)&m_mutexNodes);

    UaNode* pObjectNode = findNode(pObjectNodeId);
    UaNode* pMethodNode = findNode(pMethodNodeId);
    if ( pObjectNode &&
         (pObjectNode->getUserData() != NULL) &&
         pMethodNode )
    {
        MethodHandleUaNode* pMethodHandle = new MethodHandleUaNode;
        pMethodHandle->m_pMethodManager = (MethodManager*)this;
        ((MethodHandleUaNode*)pMethodHandle)->setUaNodes((UaObject*)pObjectNode, (UaMethod*)pMethodNode);
        return pMethodHandle;
    }
    else
    {
        MethodHandle* pMethodHandle = NodeManagerUaNode::getMethodHandle(pSession, pObjectNodeId, pMethodNodeId, result);
        return pMethodHandle;
    }
}

UaStatus MyNodeManagerNodeSetXml::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus            ret;
    UaStatusCodeArray   inputArgumentResults;
    UaDiagnosticInfos   inputArgumentDiag;
    UaVariantArray      outputArguments;
    MethodHandleUaNode* pMethodHandleUaNode = NULL;
    BaControllerUserData* pUserData = NULL;

    if ( pMethodHandle && pMethodHandle->getHandleImplementation() == MethodHandle::UA_NODE )
    {
        pMethodHandleUaNode = (MethodHandleUaNode*)pMethodHandle;
        pUserData = (BaControllerUserData*)pMethodHandleUaNode->pUaObject()->getUserData();
        if ( pUserData )
        {
            if ( pUserData->nidStartMethod() == pMethodHandleUaNode->pUaMethod()->nodeId() )
            {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadInvalidArgument;
                }
                else
                {
                    // Start controller
                    ret = m_pCommIf->setControllerState(
                        pUserData->deviceAddress(),
                        BaCommunicationInterface::Ba_ControllerState_On );
                }
            }
            else if ( pUserData->nidStopMethod() == pMethodHandleUaNode->pUaMethod()->nodeId() )
            {
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadInvalidArgument;
                }
                else
                {
                    // Stop controller
                    ret = m_pCommIf->setControllerState(
                        pUserData->deviceAddress(),
                        BaCommunicationInterface::Ba_ControllerState_Off );
                }
            }
            else
            {
                ret = OpcUa_BadInvalidArgument;
            }
        }
        else
        {
            ret = OpcUa_BadInvalidArgument;
        }
    }
    else
    {
        ret = OpcUa_BadInvalidArgument;
    }

    if (ret.isGood())
    {
        pCallback->finishCall(
            callbackHandle,
            inputArgumentResults,
            inputArgumentDiag,
            outputArguments,
            ret);
    }

#if SUPPORT_Event_Subscription_Server_Facet
    // Create Controller Events if state change succeeded
    if ( ret.isGood() )
    {
        // State
        BaCommunicationInterface::ControllerState state;
        m_pCommIf->getControllerState(pUserData->deviceAddress(), state);

        char messageText[200];
        if ( state == BaCommunicationInterface::Ba_ControllerState_Off )
        {
            snprintf( messageText, 200, "State of %s changed to OFF", pMethodHandleUaNode->pUaObject()->nodeId().toString().toUtf8());
        }
        else
        {
            snprintf( messageText, 200, "State of %s changed to ON", pMethodHandleUaNode->pUaObject()->nodeId().toString().toUtf8());
        }

        // Change state of alarm condition
        if ( state == BaCommunicationInterface::Ba_ControllerState_Off )
        {
            pUserData->pStateCondition()->setAckedState(OpcUa_False);
            pUserData->pStateCondition()->setActiveState(OpcUa_True);
            pUserData->pStateCondition()->setRetain(OpcUa_True);
        }
        else
        {
            pUserData->pStateCondition()->setAckedState(OpcUa_True);
            pUserData->pStateCondition()->setActiveState(OpcUa_False);
            pUserData->pStateCondition()->setRetain(OpcUa_False);
        }
        pUserData->pStateCondition()->setMessage(UaLocalizedText("en", messageText));
        pUserData->pStateCondition()->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
    }
#endif // SUPPORT_Event_Subscription_Server_Facet

    return ret;
}
#endif // SUPPORT_Method_Server_Facet

void MyNodeManagerNodeSetXml::initializeVariableValue(UaVariable* pNewNode, CttUserData* pCttUserData)
{
    OpcUa_Int32 i,j;
    OpcUa_Int32 count = 10;
    UaDataValue dataValue;
    UaVariant uvValue;
    UaNodeId dataType = pNewNode->dataType();

    // unsupported datatype
    if (dataType.namespaceIndex() != 0 || dataType.identifierType() != OpcUa_IdentifierType_Numeric)
    {
        return;
    }

    // no user data
    if (pCttUserData == NULL)
    {
        return;
    }

    // scalar
    if (pNewNode->valueRank() == OpcUa_ValueRanks_Scalar)
    {
        switch (dataType.identifierNumeric())
        {
        case OpcUaId_ByteString:
            {
                UaByteString bsTmp;
                UaByteArray valueByteStringHelper;
                valueByteStringHelper.resize(count);

                for ( i=0; i<count; i++ )
                {
                    valueByteStringHelper[i] = (char)(i * 0x10);
                }
                bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
                uvValue.setByteString(bsTmp, OpcUa_True);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_DateTime:
            uvValue.setDateTime(UaDateTime::now());
            dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
            pCttUserData->setDataValue(dataValue);
            break;
        case OpcUaId_Guid:
            uvValue.setGuid(UaGuid::create());
            dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
            pCttUserData->setDataValue(dataValue);
            break;
        case OpcUaId_String:
            uvValue.setString("Hello world");
            dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
            pCttUserData->setDataValue(dataValue);
            break;
        case OpcUaId_XmlElement:
            {
                UaByteString bsTmp;
                UaByteArray valueByteStringHelper;
                UaString sTmp("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                valueByteStringHelper = sTmp.toLocal8Bit();
                bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
                uvValue.setXmlElement(bsTmp, OpcUa_True);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        default:
            break;
        }
    }
    // array
    else if (pNewNode->valueRank() == OpcUa_ValueRanks_OneDimension)
    {
        switch (dataType.identifierNumeric())
        {
        case OpcUaId_Boolean:
            {
                UaBooleanArray tempArray;
                tempArray.create(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = OpcUa_True;
                }
                uvValue.setBoolArray(tempArray, OpcUa_True);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_Byte:
            {
                UaByteArray tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[(unsigned int)i] = (char)i;
                }
                uvValue.setByteArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_ByteString:
            {
                UaByteStringArray tempArray;
                tempArray.resize(count);
                UaByteString bsTmp;
                UaByteArray valueByteStringHelper;
                for ( i=0; i<count; i++ )
                {
                    OpcUa_ByteString_Initialize(&tempArray[i]);
                    valueByteStringHelper.resize(8);
                    for ( j=0; j<valueByteStringHelper.size(); j++ )
                    {
                        valueByteStringHelper[j] = (char)( (i+j) * 0x10);
                    }
                    bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());
                    bsTmp.copyTo(&tempArray[i]);
                }
                uvValue.setByteStringArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_DateTime:
            {
                UaDateTimeArray tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = UaDateTime::now();
                }
                uvValue.setDateTimeArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_Double:
            {
                UaDoubleArray tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = i*0.2+10;
                }
                uvValue.setDoubleArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_Float:
            {
                UaFloatArray tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = (OpcUa_Float)(i*0.1+20);
                }
                uvValue.setFloatArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_Guid:
            {
                UaGuidArray tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    UaGuid::create().copyTo(&tempArray[i]);
                }
                uvValue.setGuidArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_Int16:
            {
                UaInt16Array tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = (OpcUa_Int16)i-5;
                }
                uvValue.setInt16Array(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_Int32:
            {
                UaInt32Array tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = i-5;
                }
                uvValue.setInt32Array(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_Int64:
            {
                UaInt64Array tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = i-5;
                }
                uvValue.setInt64Array(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_SByte:
            {
                UaSByteArray tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = (OpcUa_SByte)i-5;
                }
                uvValue.setSByteArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_String:
            {
                UaStringArray tempArray;
                tempArray.resize(count);
                UaString sTemp;
                for ( i=0; i<count; i++ )
                {
                    OpcUa_String_Initialize(&tempArray[i]);
                    sTemp = UaString("Hello_%1")
                        .arg((int)i);
                    sTemp.copyTo(&tempArray[i]);
                }
                uvValue.setStringArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_UInt16:
            {
                UaUInt16Array tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = (OpcUa_UInt16)i;
                }
                uvValue.setUInt16Array(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_UInt32:
            {
                UaUInt32Array tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = i;
                }
                uvValue.setUInt32Array(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_UInt64:
            {
                UaUInt64Array tempArray;
                tempArray.resize(count);
                for ( i=0; i<count; i++ )
                {
                    tempArray[i] = i;
                }
                uvValue.setUInt64Array(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        case OpcUaId_XmlElement:
            {
                UaByteStringArray tempArray;
                tempArray.resize(count);
                UaString sTmp("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                UaByteString bsTmp;
                UaByteArray valueByteStringHelper = sTmp.toLocal8Bit();
                bsTmp.setByteString(valueByteStringHelper.size(), (OpcUa_Byte*)valueByteStringHelper.data());

                for ( i=0; i<count; i++ )
                {
                    OpcUa_ByteString_Initialize(&tempArray[i]);
                    bsTmp.copyTo(&tempArray[i]);
                }
                uvValue.setXmlElementArray(tempArray);
                dataValue.setValue(uvValue, OpcUa_True, OpcUa_True);
                pCttUserData->setDataValue(dataValue);
            }
            break;
        default:
            break;
        }
    }
}


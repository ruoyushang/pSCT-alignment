#include "ccdobject.hpp"
#include "pasnodemanager.hpp"
#include "passervertypeids.hpp"
#include "pascommunicationinterface.hpp"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

const std::map <OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> CCDObject::VARIABLES = {
        {PAS_CCDType_State,        std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_xFromLED,     std::make_tuple("x", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_yFromLED,     std::make_tuple("y", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_zFromLED,     std::make_tuple("z", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_psiFromLED,   std::make_tuple("psi", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_thetaFromLED, std::make_tuple("theta", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_phiFromLED,   std::make_tuple("phi", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_xNominal,     std::make_tuple("xNominal", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_yNominal,     std::make_tuple("yNominal", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_zNominal,     std::make_tuple("zNominal", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_psiNominal,   std::make_tuple("psiNominal", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_thetaNominal, std::make_tuple("thetaNominal", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_CCDType_phiNominal,   std::make_tuple("phiNominal", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
};

const std::map <OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> CCDObject::ERRORS = {
};

const std::map <OpcUa_UInt32, std::pair<std::string, std::vector < std::tuple < std::string, UaNodeId, std::string>>>>
CCDObject::METHODS = {
        {PAS_CCDType_Start,   {"Start", {}}},
        {PAS_CCDType_ReadAll, {"Read",  {}}},
        {PAS_CCDType_Stop,    {"Stop",  {}}}
};

UaNodeId CCDObject::typeDefinitionId() const {
    UaNodeId ret(PAS_CCDType, browseName().namespaceIndex());
    return ret;
}

UaStatus CCDObject::call(
        const ServiceContext &serviceContext,
        MethodHandle *pMethodHandle,
        const UaVariantArray &inputArguments,
        UaVariantArray &outputArguments,
        UaStatusCodeArray &inputArgumentResults,
        UaDiagnosticInfos &inputArgumentDiag) {
    UaStatus ret;
    MethodHandleUaNode *pMethodHandleUaNode = static_cast<MethodHandleUaNode *>(pMethodHandle);
    UaMethod *pMethod = NULL;

    if (pMethodHandleUaNode) {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if (pMethod) {
            // Check if we have the start method
            if (pMethod->nodeId() == m_pMethodStart->nodeId()) {
                // Number of input arguments must be 0
                if (inputArguments.length() > 0)
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_CCDType, m_Identity,
                                                    PASState::On);
            }
                // Check if we have the stop method
            else if (pMethod->nodeId() == m_pMethodStop->nodeId()) {
                // Number of input arguments must be 0
                // Number of input arguments must be 0
                if (inputArguments.length() > 0)
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_CCDType, m_Identity,
                                                    PASState::Off);
            }
                // Check if we have the read method
            else if (pMethod->nodeId() == m_pMethodRead->nodeId()) {
                // Number of input arguments must be 0
                if (inputArguments.length() > 0)
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_CCDType, m_Identity);
            }
        } else {
            assert(false);
            ret = OpcUa_BadInvalidArgument;
        }
    } else {
        assert(false);
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}
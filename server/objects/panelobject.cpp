#include "server/objects/panelobject.hpp"

#include <map>
#include <string>
#include <tuple>

#include "uabase/uavariant.h"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.h"
#include "common/opcua/components.h"

#include "server/pasnodemanager.hpp"
#include "server/pascommunicationinterface.hpp"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> PanelObject::VARIABLES = {
    {PAS_PanelType_State, std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
    {PAS_PanelType_ExtTemperature, std::make_tuple("ExternalTemperature", UaVariant(0.0), OpcUa_False, Ua_AccessLevel_CurrentRead)},
    {PAS_PanelType_IntTemperature, std::make_tuple("InternalTemperature", UaVariant(0.0), OpcUa_False, Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> PanelObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, int>> PanelObject::METHODS = {
    {PAS_PanelType_StepAll, {"StepAll", 0}},
    {PAS_PanelType_MoveTo_Acts, {"MoveToActs", 0}},
    {PAS_PanelType_Stop, {"Stop", 0}}
};

std::map<UaNodeId, std::pair<UaMethodGeneric *, OpcUa_UInt32>> m_MethodMap;

/// @details Adds all child variable nodes and child method nodes. nitializes a reference-counting shared mutex for thread locking.
PanelObject::PanelObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManager *pNodeManager,
    Identity identity,
    PasCommunicationInterface *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
{
    // Use a mutex shared across all variables of this object
    // This will ensure thread-locking is applied across all variables sharing the same mutex
    m_pSharedMutex = new UaMutexRefCounted;

    UaStatus addStatus;

    // Add all child variable nodes
    for (auto it = VARIABLES.begin(); it != VARIABLES.end(); it++) {
        addVariable(pNodeManager, PAS_PanelType, it->first, std::get<2>(it->second));
    }

    // Add all child method nodes
    UaString sName;
    UaString sNodeId;
    OpcUa_Int16 nsIdx = pNodeManager->getNameSpaceIndex();
    for (auto it = METHODS.begin(); it != METHODS.end(); it++)
    {
      sName = UaString(it->second.first.c_str());
      sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
      m_MethodMap[UaNodeId(sNodeId, nsIdx)] = std::make_pair(new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId), it->first);
      addStatus = pNodeManager->addNodeAndReference(this, m_MethodMap[UaNodeId(sNodeId, nsIdx)].first, OpcUaId_HasComponent);
      UA_ASSERT(addStatus.isGood());
    }
}

/// @details Releases reference in shared mutex.
PanelObject::~PanelObject()
{
    if ( m_pSharedMutex )
    {
        m_pSharedMutex->releaseReference(); // Release our local reference
        m_pSharedMutex = NULL;
    }
}

UaNodeId PanelObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_PanelType, browseName().namespaceIndex());
    return ret;
}

/// @details Executes the requested method by calling the communication interface to retrieve the desired device controller.
UaStatus PanelObject::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    MethodHandleUaNode* pMethodHandleUaNode = static_cast<MethodHandleUaNode*>(pMethodHandle);
    UaMethod*           pMethod             = NULL;

    int numArgs;
    OpcUa_UInt32 methodTypeID;

    if(pMethodHandleUaNode)
    {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if(m_MethodMap.find(pMethod->nodeId()) != m_MethodMap.end())
        {
            methodTypeID = m_MethodMap[pMethod->nodeId()].second;
            numArgs = METHODS.at(methodTypeID).second;

            if ( inputArguments.length() != numArgs )
                ret = OpcUa_BadInvalidArgument;
            else
                ret = m_pCommIf->OperateDevice(PAS_PanelType, m_Identity, methodTypeID);
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

    return ret;
}

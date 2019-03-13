#include "server/opcua_objects/panelobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.h"
#include "common/opcua/components.h"

#include "server/pasnodemanager.hpp"
#include "server/pascommunicationinterface.hpp"



/// @details
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
    for (auto it = variables.begin(); it != variables.end(); it++) {
        addStatus = addVariable(pNodeManager, PAS_PanelType, std::get<0>(it->second), std::get<2>(it->second));
        UA_ASSERT(addStatus.isGood());
    }

    // Add all method variable nodes
    UaString sName;
    UaString sNodeId;
    OpcUa_Int16 nsIdx = pNodeManager->getNameSpaceIndex();
    for (auto it = methods.begin(); it != methods.end(); it++)
    {
      sName = it->second.first;
      sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
      m_MethodMap[UaNodeId(sNodeId, nsIdx)] = std::make_pair(new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId), it->first);
      addStatus = pNodeManager->addNodeAndReference(this, it->first, OpcUaId_HasComponent);
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

/// @details
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
            numArgs = PanelObject::methods[methodTypeID].second;

            if ( inputArguments.length() != numArgs; )
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

#include "client/pasobjectfactory.hpp"

#include "common/alignment/device.hpp"

#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasnodemanagercommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "client/objects/opttableobject.hpp"
#include "client/objects/mirrorobject.hpp"
#include "client/objects/panelobject.hpp"
#include "client/objects/edgeobject.hpp"
#include "client/objects/ccdobject.hpp"
#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"


PasObject* PasObjectFactory::Create(
    unsigned deviceType,
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManager *pNodeManager,
    const Device::Identity &identity,
    PasCommunicationInterface *pCommIf)
{
    switch (deviceType)
    {
        case PAS_MirrorType:
            return new MirrorObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf);
        case PAS_ACTType:
            return new ACTObject(name, newNodeId, defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(pNodeManager), identity, dynamic_cast<PasComInterfaceCommon *>(pCommIf));
        case PAS_MPESType:
            return new MPESObject(name, newNodeId, defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(pNodeManager), identity, dynamic_cast<PasComInterfaceCommon *>(pCommIf));
        case PAS_PanelType:
            return new PanelObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf);
        case PAS_EdgeType:
            return new EdgeObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf);
        case PAS_PSDType:
            return new PSDObject(name, newNodeId, defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(pNodeManager), identity, dynamic_cast<PasComInterfaceCommon *>(pCommIf));
        case PAS_CCDType:
            return new CCDObject(name, newNodeId, defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(pNodeManager), identity, dynamic_cast<PasComInterfaceCommon *>(pCommIf));
        default:
            return nullptr;
    }
}

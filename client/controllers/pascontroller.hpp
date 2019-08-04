#ifndef __PASCONTROLLER_H__
#define __PASCONTROLLER_H__

#include <chrono>

#include <map>
#include <set>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/alignment/device.hpp"
#include "common/opcua/pascontrollercommon.hpp"
#include "common/opcua/passervertypeids.hpp"
#include "common/simulatestewart/stewartplatform.hpp"

#include "client/pascommunicationinterface.hpp"


class Client;

class PasController : public PasControllerCommon
{
    UA_DISABLE_COPY(PasController);
public:
    /* construction / destruction */
    PasController(Device::Identity identity, Client *pClient, int updateInterval = 0)
        : PasControllerCommon(std::move(identity),
                              updateInterval),
          m_pClient(pClient) {}

protected:
    Client *m_pClient;
};



// helper class for composite devices like panel and edge
class PasCompositeController : public PasController
{
    public:
    PasCompositeController(Device::Identity identity, Client *pClient, int updateInterval = 0) :
        PasController(std::move(identity), pClient, updateInterval) {}

    ~PasCompositeController() override = default;

    // yes, this indeed needs to be the specified type -- a const pointer to a
    // (non-const) PasController. The constness guarantees that the panel doesn't change the
    // pointer -- but other methods we use require that the underlying object itself is not
    // a const.
    // declaring these as virtual, but they should be the same for everything that
    // inherts from here
    virtual void addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController);

    virtual const std::vector<std::shared_ptr<PasController>> &getChildren(unsigned type) const {
        return m_pChildren.at(type);
    };

    virtual const std::shared_ptr<PasController> &getChildAtPosition(unsigned type, int position) const {
        return m_ChildrenPositionMap.at(type).at(position);
    };

    protected:
        // stores the possbile types of children
        std::set<unsigned> m_ChildrenTypes;
        // deviceType -> vector of Children devices
        std::map<unsigned, std::vector<std::shared_ptr<PasController>>> m_pChildren;

    std::map<unsigned, std::map<Device::Identity, std::shared_ptr<PasController>>> m_ChildrenIdentityMap;
        // deviceType -> {position -> index in m_pChildren.at(deviceType) }
        // m_ChildrenPositionMap.at(deviceType) is ordered by the first index, position.
        // This makes it perfect for cycling through positions.
        std::map<unsigned, std::map<int, std::shared_ptr<PasController>>> m_ChildrenPositionMap;
        // almost a duplicate of the above, but not quite.
        // This guarantees that sensors along the 4-sensor edge are not discarded if their
        // positions are the same; this also provides a map for edges within a mirror
        std::map<unsigned, std::map<int, std::shared_ptr<PasController>>> m_ChildrenSerialMap;
    std::map<unsigned, std::map<std::string, std::shared_ptr<PasController>>> m_ChildrenEaddressMap;
};

#endif // #ifndef __PASCONTROLLER_H__

#ifndef __PASCONTROLLER_H__
#define __PASCONTROLLER_H__

#include "client/pascommunicationinterface.hpp"
#include "passervertypeids.hpp"
#include "stewartplatform.h"
#include "components.hpp"

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include <vector>
#include <set>
#include <map>
#include <chrono>
#include <iostream>

class Client;


class PasController
{
    UA_DISABLE_COPY(PasController);
public:
    // update interval in ms
    static int kUpdateInterval;

    /* construction / destruction */
    PasController(Identity identity, Client *pClient, int updateInterval = 0) :
            m_state(PASState::On),
            m_ID(std::move(identity)),
            m_pClient(pClient),
            m_UpdateInterval_ms(updateInterval) {};

    ~PasController() = default;

    const Identity& getId() const {return m_ID;}

    /* Get Controller status and data */
    virtual UaStatus getState(PASState &state) = 0;

    virtual UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) = 0;

    /* set Controller status and data */
    virtual UaStatus setState(PASState state) = 0;

    virtual UaStatus setData(OpcUa_UInt32 offset, UaVariant value) = 0;

    virtual UaStatus operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray()) = 0;

    virtual bool Initialize() { return true; }

protected:
    UaMutex m_mutex;
    PASState m_state;

    Identity m_ID;
    Client *m_pClient;

    // update interval in milliseconds
    const int m_UpdateInterval_ms;

    // be able to check if data has expired
    typedef std::chrono::system_clock TIME;
    bool __expired() const {return (std::chrono::duration_cast<std::chrono::milliseconds>(TIME::now() - m_lastUpdateTime).count() > m_UpdateInterval_ms); }
    std::chrono::time_point<TIME> m_lastUpdateTime;
};

// helper class for composite devices like panel and edge
class PasCompositeController : public PasController
{
    public:
        PasCompositeController(Identity identity, Client *pClient, int updateInterval = 0) :
                PasController(std::move(identity), pClient, updateInterval) {};

    ~PasCompositeController() = default;

        // yes, this indeed needs to be the specified type -- a const pointer to a
        // (non-const) PasController. The constness guarantees that the panel doesn't change the
        // pointer -- but other methods we use require that the underlying object itself is not
        // a const.
        // declaring these as virtual, but they should be the same for everything that
        // inherts from here
        virtual void addChild(OpcUa_UInt32 deviceType, PasController *pController);
        virtual const std::vector<PasController *>& getChildren(unsigned type) const { return m_pChildren.at(type); };

    protected:
        // stores the possbile types of children
        std::set<unsigned> m_ChildrenTypes;
        // deviceType -> vector of Children devices
        std::map< unsigned, std::vector<PasController *> > m_pChildren;
        // deviceType -> {position -> index in m_pChildren.at(deviceType) }
        // m_ChildrenPositionMap.at(deviceType) is oredered by the first index, position.
        // This makes it perfect for cycling through positions.
        std::map< unsigned, std::map<unsigned, unsigned> > m_ChildrenPositionMap;
        // almost a duplicate of the above, but not quite.
        // This guarantees that sensors along the 4-sensor edge are not discarded if their
        // positions are the same; this also provides a map for edges within a mirror
        std::map< unsigned, std::map<Identity, unsigned> > m_ChildrenIdentityMap;
};

#endif // #ifndef __PASCONTROLLER_H__

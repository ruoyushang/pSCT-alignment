#include "client/controllers/pascontroller.hpp"

#include <algorithm>
#include <cstdlib> // system
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h> // usleep
#include <utility> // make_pair

#include "common/alignment/device.hpp"
#include "common/globalalignment/ccdclass.h"
#include "common/opcua/pasobject.hpp"
#include "common/utilities/DBConfig.hpp"

#include "client/clienthelper.hpp"

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"

#include <Eigen/Dense>

// implement PasCompositeController::addChild()
void PasCompositeController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController)
{
    const auto &id = pController->getId();
    auto pos = pController->getId().position;
    // don't add the same device multiple times
    try {
        m_ChildrenPositionMap.at(deviceType).at(pos);
        m_ChildrenIdentityMap.at(deviceType).at(id);
        return;
    }
    catch (std::out_of_range &e) {
        // only add if this is a possible child
        if (m_ChildrenTypes.count(deviceType)) {
            std::cout << " --- " << m_ID.name << "::addChild(): "
                      << m_ID.name << ": adding " << pController->getId() << std::endl;
            m_pChildren[deviceType].push_back(std::shared_ptr<PasController>(pController));
            m_ChildrenSerialMap[deviceType][id.serialNumber] = m_pChildren.at(deviceType).size() - 1;
            m_ChildrenEAddressMap[deviceType][id.eAddress] = m_pChildren.at(deviceType).size() - 1;
            // this doesn't work for edges, since they don't have an assigned position
            m_ChildrenPositionMap[deviceType][pos] = m_pChildren.at(deviceType).size() - 1;
        }
    }
}

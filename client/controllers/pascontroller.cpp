#include "client/controllers/pascontroller.hpp"

#include <algorithm>
#include <cstdlib> // system
#include <fstream>

#include <sstream>
#include <string>
#include <unistd.h> // usleep
#include <utility> // make_pair

#include "common/alignment/device.hpp"
#include "common/globalalignment/ccdclass.h"
#include "common/opcua/pasobject.hpp"
#include "common/utilities/DBConfig.hpp"

#include "client/clienthelper.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"

#include <Eigen/Dense>

// implement PasCompositeController::addChild()
void PasCompositeController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController)
{
    const auto &id = pController->getIdentity();
    // don't add the same device multiple times
    try {
        m_ChildrenPositionMap.at(deviceType).at(id.position);
        m_ChildrenSerialMap.at(deviceType).at(id.serialNumber);
        m_ChildrenEaddressMap.at(deviceType).at(id.eAddress);
        return;
    }
    catch (std::out_of_range &e) {
        // only add if this is a possible child
        if (m_ChildrenTypes.count(deviceType)) {
            spdlog::info("{}: PasCompositeController::addChild(): added child with ID {}.", m_Identity,
                         pController->getIdentity());
            m_pChildren[deviceType].push_back(std::shared_ptr<PasController>(pController));
            m_ChildrenIdentityMap[deviceType][id] = pController;
            m_ChildrenSerialMap[deviceType][id.serialNumber] = pController;
            m_ChildrenEaddressMap[deviceType][id.eAddress] = pController;
            m_ChildrenPositionMap[deviceType][id.position] = pController;
        } else{
            spdlog::debug("{}: PasCompositeController::addChild(): Did not add child with ID {}.", m_Identity,
                         pController->getIdentity());
        }
    }
}

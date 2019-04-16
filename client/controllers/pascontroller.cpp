#include "pascontroller.h"
#include "ccdclass.h"
#include "clienthelper.h"
#include "components.h"
#include "common/opcua/pasobject.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstdlib> // system
#include <unistd.h> // usleep
#include <utility> // make_pair

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"
#include "DBConfig.hpp"

#include <Eigen/Dense>

int PasController::kUpdateInterval = 0;

// implement PasCompositeController::addChild()
void PasCompositeController::addChild(OpcUa_UInt32 deviceType, PasController *const pController)
{
    std::cout << " --- " << m_ID.name << "::addChild(): "
              << m_ID.name << ": adding " << pController->getId() << std::endl;

    const auto &id = pController->getId();
    auto pos = pController->getId().position;
    // don't add the same device multiple times
    try {
        // m_ChildrenPositionMap.at(deviceType).at(pos);
        m_ChildrenIdentityMap.at(deviceType).at(id);
        cout << "\tAlready added. Moving on..." << endl;

        return;
    }
    catch (std::out_of_range &e) {
        // only add if this is a possible child
        if (m_ChildrenTypes.count(deviceType)) {
            m_pChildren[deviceType].push_back(pController);
            m_ChildrenIdentityMap[deviceType][id] = m_pChildren.at(deviceType).size() - 1;
            // this doesn't work for edges, since they don't have an assigned position
            m_ChildrenPositionMap[deviceType][pos] = m_pChildren.at(deviceType).size() - 1;
        }
    }
}
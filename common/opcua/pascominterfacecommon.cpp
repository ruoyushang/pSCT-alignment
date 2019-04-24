/// @details Returns -1 on invalid device type ID.
std::size_t PasComInterfaceCommon::getDeviceCount(OpcUa_UInt32 deviceType) {
    try {
        return m_pControllers.at(deviceType).size();
    }
    catch (std::out_of_range &e) {
        return -1;
    }
}

std::vector <Identity> PasComInterfaceCommon::getValidDeviceIdentities(OpcUa_UInt32 deviceType) {
    std::vector <Identity> validIdentities;
    std::map <Identity, std::shared_ptr<PasController>> devices = m_pControllers.at(deviceType);

    for (auto &it : devices) {
        validIdentities.push_back(it.first);
    }

    return validIdentities;
}
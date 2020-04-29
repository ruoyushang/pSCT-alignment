//
// Created by Deivid Ribeiro on 4/27/20.
//

#ifndef ALIGNMENT_OPTTABLECONTROLLER_H
#define ALIGNMENT_OPTTABLECONTROLLER_H

#include "client/controllers/pascontroller.hpp"

class OptTableController : public PasCompositeController {
    UA_DISABLE_COPY(OptTableController);
public:
    explicit OptTableController(Device::Identity identity, Client *pClient);

    // initialize and precompute everything
    bool initialize() override;

    // same as before -- get/set status/data and Operate:
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

    // own implementation
    void addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController) override;

protected:
    //

private:
    std::string m_Mode;
    std::set<unsigned> m_selectedPanels;
    std::set<unsigned> m_selectedPSD;
    OpcUa_Boolean m_tracking;

};

#endif //ALIGNMENT_OPTTABLECONTROLLER_H

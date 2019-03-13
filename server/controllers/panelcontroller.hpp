class PanelController : public PasController
{
    UA_DISABLE_COPY(PanelController);
public:
    // construction / destruction
    PanelController(int ID, std::shared_ptr<Platform> pPlatform);
    virtual ~PanelController();

    // Get Controller status and data
    UaStatus getState(PASState& state);
    UaStatus getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data
    UaStatus updateState();
    UaStatus setState(PASState state);
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatus Operate(OpcUa_UInt32 offset, const UaVariantArray& args);

    void addActuator(std::shared_ptr<ActController> pActuator) { m_pActuators.push_back(pActuator); };

private:
    PASState m_state = PASState::PAS_Off;

    // vector of Actuators on this panel
    std::vector<std::shared_ptr<ActController>> m_pActuators;
};

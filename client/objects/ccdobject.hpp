#ifndef CLIENT_CCDOBJECT_HPP
#define CLIENT_CCDOBJECT_HPP

class CCDObject : public PasObject {
    UA_DISABLE_COPY(CCDObject);
public:
    CCDObject(
            const UaString &name,
            const UaNodeId &newNodeId,
            const UaString &defaultLocaleId,
            PasNodeManagerCommon *pNodeManager,
            Identity identity,
            PasComInterfaceCommon *pCommIf) : PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity,
                                                        pCommIf) { initialize(); }

    // implement the synchronous call
    UaStatus call(
            const ServiceContext &serviceContext,
            MethodHandle *pMethodHandle,
            const UaVariantArray &inputArguments,
            UaVariantArray &outputArguments,
            UaStatusCodeArray &inputArgumentResults,
            UaDiagnosticInfos &inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

    const std::map <OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() override { return CCDObject::VARIABLES; }

    const std::map <OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() override { return CCDObject::ERRORS; }

    const std::map <OpcUa_UInt32, std::pair<std::string,
            std::vector < std::tuple < std::string, UaNodeId, std::string>>>>

    getMethodDefs() override { return CCDObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map <OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map <OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map <OpcUa_UInt32, std::pair<std::string,
            std::vector < std::tuple < std::string, UaNodeId, std::string>>>>
    METHODS;
};

#endif //CLIENT_CCDOBJECT_HPP

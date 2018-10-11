#ifndef MYNODEMANAGERNODESETXML_H
#define MYNODEMANAGERNODESETXML_H

#include "nodemanagernodesetxml.h"
#include "methodmanager.h"

class BaCommunicationInterface;
class CttUserData;

class MyNodeManagerNodeSetXml:
#if SUPPORT_Method_Server_Facet
    public MethodManager,
#endif // SUPPORT_Method_Server_Facet
    public NodeManagerNodeSetXml
{
    UA_DISABLE_COPY(MyNodeManagerNodeSetXml);
public:
    /** Construction */
    MyNodeManagerNodeSetXml(const UaString& sNamespaceUri);
    /** Destruction */
    virtual ~MyNodeManagerNodeSetXml();

    // NodeManagerNodeSetXml overwrites
    virtual void variableCreated(UaVariable* pNewNode, UaBase::Variable *pVariable);
    virtual void objectCreated(UaObject* pNewNode, UaBase::Object *pObject);

    // IOManagerUaNode overwrites
    virtual UaStatus readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues);
    virtual UaStatus writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes);

#if SUPPORT_Method_Server_Facet
    // NodeManager / NodeManagerUaNode overwrite
    virtual MethodHandle* getMethodHandle(
        Session* pSession,
        OpcUa_NodeId* pObjectNodeId,
        OpcUa_NodeId* pMethodNodeId,
        UaStatus& result) const;

    // MethodManager implementation
    virtual UaStatus beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments);
#endif // SUPPORT_Method_Server_Facet

private:
    void initializeVariableValue(UaVariable* pNewNode, CttUserData* pCttUserData);

private:
    BaCommunicationInterface* m_pCommIf;
};

#endif // MYNODEMANAGERNODESETXML_H

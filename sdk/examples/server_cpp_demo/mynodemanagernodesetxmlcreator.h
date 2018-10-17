#ifndef MYNODEMANAGERNODESETXMLCREATOR_H
#define MYNODEMANAGERNODESETXMLCREATOR_H

#include "nodemanagernodesetxml.h"
#include "bauserdata.h"

// Factory to instantiate NodeManager implementations derived from NodeManagerNodeSetXml
// Creation of NodeManagers is based on the namespace URI
class MyNodeManagerNodeSetXmlCreator: public NodeManagerNodeSetXmlCreator
{
    UA_DISABLE_COPY(MyNodeManagerNodeSetXmlCreator);
public:
    MyNodeManagerNodeSetXmlCreator();
    virtual ~MyNodeManagerNodeSetXmlCreator();

    // Creates an instance of NodeManagerNodeSetXml or a class derived from NodeManagerNodeSetXml
    virtual NodeManagerNodeSetXml* createNodeManager(const UaString& sNamespaceUri);
};



// User data class that initializes the user data for Building Automation controllers
// from extensions in the UANodesSet XML file
class MyBaUserData : public BaUserData
{
    UA_DISABLE_COPY(MyBaUserData);
public:
    MyBaUserData(const UaByteString &bsValue);
    virtual ~MyBaUserData(){}
};



// Specialized UaBase::Variable class that can load its user data from an XML extension in UANodeSet
class MyVariable : public UaBase::Variable
{
public:
    MyVariable(const UaNodeId &nodeId);
    virtual MyBaUserData* userData() const;
};



// Speczialized factory for BaseNodes creating Variable classes with UserData
class MyBaseNodeFactory : public UaBase::BaseNodeFactory
{
public:
    virtual MyVariable* createVariable(const UaNodeId &nodeId) const;
};


#endif // MYNODEMANAGERNODESETXMLCREATOR_H

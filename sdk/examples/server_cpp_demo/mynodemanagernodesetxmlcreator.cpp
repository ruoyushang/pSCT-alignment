#include "mynodemanagernodesetxmlcreator.h"
#include "mynodemanagernodesetxml.h"

MyNodeManagerNodeSetXmlCreator::MyNodeManagerNodeSetXmlCreator()
{
}

MyNodeManagerNodeSetXmlCreator::~MyNodeManagerNodeSetXmlCreator()
{
}

NodeManagerNodeSetXml* MyNodeManagerNodeSetXmlCreator::createNodeManager(const UaString& sNamespaceUri)
{
    if ( sNamespaceUri == "urn:UnifiedAutomation:CppDemoServer:UANodeSetXmlImport" )
    {
        return new MyNodeManagerNodeSetXml(sNamespaceUri);
    }
    else
    {
        return NodeManagerNodeSetXmlCreator::createNodeManager(sNamespaceUri);
    }
}


// Extract user data from XML extension
MyBaUserData::MyBaUserData(const UaByteString &bsValue)
{
    UaXmlDocument xmlDocument;
    UaXmlValue xmlValue;
    UaVariant  uvValue;
    UaString   sValue;
    UaStatus   processingStatus;

    // Get XML element from the extension byte string
    xmlDocument.loadXmlText((char*) bsValue.data(), bsValue.length(), NULL);

    // Try to extract DeviceAddress
    if (xmlDocument.getChild("DeviceAddress") == 0)
    {
        xmlDocument.getContent(xmlValue);
        uvValue.setString(xmlValue.pValue());
        if ( OpcUa_IsNotGood(uvValue.toUInt32(m_deviceAddress)) )
        {
            processingStatus = OpcUa_Bad;
        }
        xmlDocument.getParentNode();
    }
    else
    {
        processingStatus = OpcUa_Bad;
    }

    // Try to extract VariableOffset
    if (xmlDocument.getChild("VariableOffset") == 0)
    {
        xmlDocument.getContent(xmlValue);
        uvValue.setString(xmlValue.pValue());
        if ( OpcUa_IsNotGood(uvValue.toUInt32(m_variableOffset)) )
        {
            processingStatus = OpcUa_Bad;
        }
        xmlDocument.getParentNode();
    }
    else
    {
        processingStatus = OpcUa_Bad;
    }

    // Change state to a valid value only if the complete user data can be processed
    if ( processingStatus.isGood() )
    {
        if (xmlDocument.getChild("IsState") == 0)
        {
            xmlDocument.getContent(xmlValue);
            sValue = xmlValue.pValue();
            if ( sValue == "true" )
            {
                m_isState = BaUserData::StateType_IsStateVariable;
            }
            else
            {
                m_isState = BaUserData::StateType_IsDataVariable;
            }
            xmlDocument.getParentNode();
        }
    }
}


MyVariable::MyVariable(const UaNodeId &nodeId)
: UaBase::Variable(nodeId)
{
}

MyBaUserData* MyVariable::userData() const
{
    const UaBase::Extension *pExtension = extension("BAVariableAddressInformation");
    if (pExtension)
    {
        return new MyBaUserData(pExtension->value());
    }
    return NULL;
}


MyVariable* MyBaseNodeFactory::createVariable(const UaNodeId &nodeId) const
{
    return new MyVariable(nodeId);
}

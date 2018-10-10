#include "configuration.h"
#include "uasettings.h"

Configuration::Configuration()
{
}

Configuration::~Configuration()
{
}

UaString Configuration::getServerUrl() const
{
    return m_serverUrl;
}

UaString Configuration::getDiscoveryUrl() const
{
    return m_discoveryUrl;
}

UaString Configuration::getApplicationName() const
{
    return m_applicationName;
}

OpcUa_Boolean Configuration::getAutomaticReconnect() const
{
    return m_bAutomaticReconnect;
}

OpcUa_Boolean Configuration::getRetryInitialConnect() const
{
    return m_bRetryInitialConnect;
}

UaNodeIdArray Configuration::getNodesToRead() const
{
    return m_nodesToRead;
}

UaNodeIdArray Configuration::getNodesToWrite() const
{
    return m_nodesToWrite;
}

UaVariantArray Configuration::getWriteValues() const
{
    return m_writeValues;
}

UaStatus Configuration::loadConfiguration(const UaString& sConfigurationFile)
{
    UaStatus result;
    UaVariant value;
    UaString sTempKey;
    OpcUa_UInt32 i = 0;
    OpcUa_UInt32 size = 0;
    OpcUa_Byte byte;
    UaSettings* pSettings = NULL;
    pSettings = new UaSettings(sConfigurationFile.toUtf16());

    pSettings->beginGroup("UaSampleConfig");

    // Application Name
    value = pSettings->value("ApplicationName", UaString());
    m_applicationName = value.toString();

    // Server URLs
    value = pSettings->value("DiscoveryURL", UaString("opc.tcp://localhost:48010"));
    m_discoveryUrl = value.toString();
    value = pSettings->value("ServerUrl", UaString("opc.tcp://localhost:48010"));
    m_serverUrl = value.toString();

    // Reconnection settings
    value = pSettings->value("AutomaticReconnect", UaVariant((OpcUa_Boolean)OpcUa_True));
    value.toBool(m_bAutomaticReconnect);
    value = pSettings->value("RetryInitialConnect", UaVariant((OpcUa_Boolean)OpcUa_False));
    value.toBool(m_bRetryInitialConnect);

    // Read NamespaceArray
    m_namespaceArray.clear();
    pSettings->beginGroup("NSArray");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_namespaceArray.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("NameSpaceUri0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_namespaceArray[i]);
    }
    pSettings->endGroup();  // NSArray

    // Read NodeIds to use for reading
    m_nodesToRead.clear();
    pSettings->beginGroup("NodesToRead");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_nodesToRead.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("Variable0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString(value.toString()).copyTo(&m_nodesToRead[i]);
    }
    pSettings->endGroup();  // NodesToRead

    // Read NodeIds, DataTypes and Values to use for writing
    m_nodesToWrite.clear();
    pSettings->beginGroup("NodesToWrite");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    // NodeIds
    m_nodesToWrite.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("Variable0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString(value.toString()).copyTo(&m_nodesToWrite[i]);
    }
    // DataTypes
    UaByteArray writeDataTypes;
    writeDataTypes.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("DataType0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toByte(byte);
        writeDataTypes[(int)i] = byte;
    }
    // Values
    m_writeValues.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("Value0%1").arg((int)i);
        value = pSettings->value(sTempKey.toUtf16());
        // convert the value to the correct type
        OpcUa_BuiltInType type = (OpcUa_BuiltInType)(char)writeDataTypes[(int)i];
        if (OpcUa_IsGood(value.changeType(type, OpcUa_False)))
        {
            value.copyTo(&m_writeValues[i]);
        }
        else
        {
            printf("Cannot convert variant value: %s\n", value.toString().toUtf8());
        }
    }
    pSettings->endGroup();  // NodesToWrite

    pSettings->endGroup(); // UaClientConfig

    delete pSettings;
    pSettings = NULL;

    return result;
}

UaStatus Configuration::updateNamespaceIndexes(const UaStringArray& namespaceArray)
{
    UaStatus result;
    OpcUa_UInt32 i, j;
    OpcUa_UInt32 size;

    // create mapping table
    size = m_namespaceArray.length();
    UaInt16Array mappingTable;
    mappingTable.resize(size);

    // fill mapping table
    for (i = 0; i < m_namespaceArray.length(); i++)
    {
        mappingTable[i] = (OpcUa_UInt16)i;
        // find namespace uri
        for (j = 0; j < namespaceArray.length(); j++)
        {
            UaString string1(m_namespaceArray[i]);
            UaString string2(namespaceArray[j]);
            if (string1 == string2)
            {
                mappingTable[i] = (OpcUa_UInt16)j;
                break;
            }
        }
    }

    // replace namespace index in NodeIds
    // NodesToRead
    for (i = 0; i < m_nodesToRead.length(); i++)
    {
        m_nodesToRead[i].NamespaceIndex = mappingTable[m_nodesToRead[i].NamespaceIndex];
    }
    // NodeToWrite
    for (i = 0; i < m_nodesToWrite.length(); i++)
    {
        m_nodesToWrite[i].NamespaceIndex = mappingTable[m_nodesToWrite[i].NamespaceIndex];
    }

    // update namespace array
    m_namespaceArray = namespaceArray;
    return result;
}

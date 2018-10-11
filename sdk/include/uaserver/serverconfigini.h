/******************************************************************************
** serverconfigini.h
**
** Copyright (c) 2006-2015 Unified Automation GmbH All rights reserved.
**
** Software License Agreement ("SLA") Version 2.5
**
** Unless explicitly acquired and licensed from Licensor under another
** license, the contents of this file are subject to the Software License
** Agreement ("SLA") Version 2.5, or subsequent versions
** as allowed by the SLA, and You may not copy or use this file in either
** source code or executable form, except in compliance with the terms and
** conditions of the SLA.
**
** All software distributed under the SLA is provided strictly on an
** "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
** AND LICENSOR HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
** LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
** PURPOSE, QUIET ENJOYMENT, OR NON-INFRINGEMENT. See the SLA for specific
** language governing rights and limitations under the SLA.
**
** The complete license agreement can be found here:
** http://unifiedautomation.com/License/SLA/2.5/
**
** Project: C++ OPC Server SDK sample code
**
** Description: Configuration management class for the OPC Server.
**
******************************************************************************/
#ifndef SERVERCONFIGINI_H
#define SERVERCONFIGINI_H

#include "serverconfigdata.h"
#include "uasettings.h"

/** This class represent a sercer configuration loaded from an INI configuration file.
 */
class SERVER_CORE_EXPORT ServerConfigIni: public ServerConfigData
{
    UA_DISABLE_COPY(ServerConfigIni);
public:
    /* construction. */
    ServerConfigIni(const UaString& sIniFilePath, const UaString& sApplicationPath, const UaString& sConfigPath = "", const UaString& sTracePath = "");
    /* destruction */
    virtual ~ServerConfigIni();

    /*  Load the configuration from the config file.
     *  First method called after creation of ServerConfig. Must create all NodeManagers
     *  before method startUp is called.
     *  @return               Error code.
     */
    UaStatus loadConfiguration();

    /* Save the configuration to the config file. */
    UaStatus saveConfiguration();

    inline UaString sIniFileName() const {return m_sIniFileName;}

private:
    UaString              m_sIniFileName;
};

/** This class represent an UaEndpoint configured by the INI configuration file.
 */
class SERVER_CORE_EXPORT UaEndpointIni: public UaEndpoint
{
public:
    /** construction. */
    UaEndpointIni(){};
    /** destruction */
    virtual ~UaEndpointIni(){};

    UaStatus setIniConfig(
        int             index,
        UaSettings*     pSettings,
        const UaString& sApplicationPath,
        const UaString& sConfigPath,
        ServerConfigIni* pServerConfig);
};

#endif // SERVERCONFIGINI_H

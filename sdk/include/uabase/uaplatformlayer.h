/******************************************************************************
** uaplatformlayer.h
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
** Project: C++ OPC SDK base module
**
** UA Stack Platform layer handling class.
**
******************************************************************************/
#ifndef UAPLATFORMLAYER_H
#define UAPLATFORMLAYER_H

#include "uaplatformdefs.h"
#include "opcua_proxystub.h"

/** @ingroup CppBaseLibraryClass
 *  @brief Platform layer handling class.
 */
class UABASE_EXPORT UaPlatformLayer
{
public:

    static int init();
    static int reInitConfig();
    static void cleanup();

    static void changeTraceSettings(
        OpcUa_Boolean bTraceEnabled,
        OpcUa_UInt32  uTraceLevel);

    static void changeSerializerSettings(
        OpcUa_Int32     iMaxAlloc,
        OpcUa_Int32     iMaxStringLength,
        OpcUa_Int32     iMaxByteStringLength,
        OpcUa_Int32     iMaxArrayLength,
        OpcUa_Int32     iMaxMessageSize);

    static void changeThreadPoolSettings(
        OpcUa_Boolean   bEnabled,
        OpcUa_Int32     iMinThreads,
        OpcUa_Int32     iMaxThreads,
        OpcUa_Int32     iMaxJobs,
        OpcUa_Boolean   bBlockOnAdd,
        OpcUa_UInt32    nTimeout);

    static void changeTransportSettings(
        OpcUa_Boolean bTcpListener_ClientThreadsEnabled,
        OpcUa_Int32   iTcpListener_DefaultChunkSize,
        OpcUa_Int32   iTcpConnection_DefaultChunkSize,
        OpcUa_Int32   iTcpTransport_MaxMessageLength,
        OpcUa_Int32   iTcpTransport_MaxChunkCount);

    static void changeSecureConnectionSettings(
        OpcUa_Int32   iSecureConnection_MinRevisedSecureChannelLifetime,
        OpcUa_Int32   iSecureConnection_MaxRevisedSecureChannelLifetime);

    /** Get the current maxMessageSize setting for the serializer. */
    static OpcUa_UInt32 nMaxMessageSize() {return s_nMaxMessageSize;}

    /** Get the current maxArrayLength setting for the serializer. */
    static OpcUa_UInt32 nMaxArrayLength() {return s_nMaxArrayLength;}

    /** Get the current maxByteStringLength setting for the serializer. */
    static OpcUa_UInt32 nMaxByteStringLength() {return s_nMaxByteStringLength;}

    /** Get the current maxStringLength setting for the serializer. */
    static OpcUa_UInt32 nMaxStringLength() {return s_nMaxStringLength;}

private:
    static void configInit();

    static OpcUa_UInt32                 s_nMaxMessageSize;
    static OpcUa_UInt32                 s_nMaxArrayLength;
    static OpcUa_UInt32                 s_nMaxByteStringLength;
    static OpcUa_UInt32                 s_nMaxStringLength;
    static OpcUa_ProxyStubConfiguration s_ProxyStubConfiguration;
    static OpcUa_Boolean                s_isConfigInit;
    static OpcUa_UInt32                 s_initCounter;
};

#endif /* UAPLATFORMLAYER_H */



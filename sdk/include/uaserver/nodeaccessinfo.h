/******************************************************************************
** nodeaccessinfo.h
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
** Project: C++ OPC Server SDK core module
**
** Description: OPC server core module access info for user authorisation
**
******************************************************************************/
#ifndef NODEACCESSINFO_H
#define NODEACCESSINFO_H

#include "opcuatypes.h"

/** @brief Base class for user context of a Session.

 The user context gives information about the authenticated user that activated the session.
 */
class SERVER_CORE_EXPORT NodeAccessInfo : public ReferenceCounter
{
public:
    enum PermissionType
    {
        UA_ATTRREADABLE     = 0x0001,       // read all non value attributes
        UA_READABLE         = 0x0002,       // read value attribute
        UA_WRITABLE         = 0x0004,       // write value attribute
        UA_BROWSEABLE       = 0x0008,       // browse node
        UA_HISTORYREADABLE  = 0x0010,       // read history
        UA_HISTORYINSERT    = 0x0020,       // insert into history
        UA_HISTORYMODIFY    = 0x0040,       // modify existing history
        UA_HISTORYDELETE    = 0x0080,       // delete from history
        UA_EVENTREADABLE    = 0x0100,       // receive events
        UA_EXECUTABLE       = UA_WRITABLE,  // execute method call
        UA_ATTRWRITABLE     = 0x0200,       // write non value attributes dependent on write mask
        UA_PERMISSION_ALL   = 0x03FF
    };
    static const OpcUa_UInt16 UA_NUM_PERMISSION_BITS = 10;
    enum AccessMask
    {
        // access for others
        UA_OTHER_ATTRREADABLE    = UA_ATTRREADABLE,
        UA_OTHER_READABLE        = UA_READABLE,
        UA_OTHER_WRITABLE        = UA_WRITABLE,
        UA_OTHER_BROWSEABLE      = UA_BROWSEABLE,
        UA_OTHER_HISTORYREADABLE = UA_HISTORYREADABLE,
        UA_OTHER_HISTORYINSERT   = UA_HISTORYINSERT,
        UA_OTHER_HISTORYMODIFY   = UA_HISTORYMODIFY,
        UA_OTHER_HISTORYDELETE   = UA_HISTORYDELETE,
        UA_OTHER_EVENTREADABLE   = UA_EVENTREADABLE,
        UA_OTHER_EXECUTABLE      = UA_EXECUTABLE,
        UA_OTHER_ATTRWRITABLE    = UA_ATTRWRITABLE,
        UA_OTHER_OBSERVATION     = UA_OTHER_ATTRREADABLE | UA_OTHER_READABLE | UA_OTHER_BROWSEABLE | UA_OTHER_HISTORYREADABLE | UA_OTHER_EVENTREADABLE,
        UA_OTHER_OPERATION       = UA_OTHER_OBSERVATION | UA_OTHER_WRITABLE | UA_OTHER_EXECUTABLE,
        UA_OTHER_ALL             = UA_PERMISSION_ALL,

        // access for group members
        UA_GROUP_ATTRREADABLE    = UA_ATTRREADABLE      << UA_NUM_PERMISSION_BITS,
        UA_GROUP_READABLE        = UA_READABLE          << UA_NUM_PERMISSION_BITS,
        UA_GROUP_WRITABLE        = UA_WRITABLE          << UA_NUM_PERMISSION_BITS,
        UA_GROUP_BROWSEABLE      = UA_BROWSEABLE        << UA_NUM_PERMISSION_BITS,
        UA_GROUP_HISTORYREADABLE = UA_HISTORYREADABLE   << UA_NUM_PERMISSION_BITS,
        UA_GROUP_HISTORYINSERT   = UA_HISTORYINSERT     << UA_NUM_PERMISSION_BITS,
        UA_GROUP_HISTORYMODIFY   = UA_HISTORYMODIFY     << UA_NUM_PERMISSION_BITS,
        UA_GROUP_HISTORYDELETE   = UA_HISTORYDELETE     << UA_NUM_PERMISSION_BITS,
        UA_GROUP_EVENTREADABLE   = UA_EVENTREADABLE     << UA_NUM_PERMISSION_BITS,
        UA_GROUP_EXECUTABLE      = UA_EXECUTABLE        << UA_NUM_PERMISSION_BITS,
        UA_GROUP_ATTRWRITABLE    = UA_ATTRWRITABLE      << UA_NUM_PERMISSION_BITS,
        UA_GROUP_OBSERVATION     = UA_GROUP_ATTRREADABLE | UA_GROUP_READABLE | UA_GROUP_BROWSEABLE | UA_GROUP_HISTORYREADABLE | UA_GROUP_EVENTREADABLE,
        UA_GROUP_OPERATION       = UA_GROUP_OBSERVATION | UA_GROUP_WRITABLE | UA_GROUP_EXECUTABLE,
        UA_GROUP_ALL             = UA_PERMISSION_ALL    << UA_NUM_PERMISSION_BITS,

        // access for owner
        UA_OWNER_ATTRREADABLE    = UA_GROUP_ATTRREADABLE    << UA_NUM_PERMISSION_BITS,
        UA_OWNER_READABLE        = UA_GROUP_READABLE        << UA_NUM_PERMISSION_BITS,
        UA_OWNER_WRITABLE        = UA_GROUP_WRITABLE        << UA_NUM_PERMISSION_BITS,
        UA_OWNER_BROWSEABLE      = UA_GROUP_BROWSEABLE      << UA_NUM_PERMISSION_BITS,
        UA_OWNER_HISTORYREADABLE = UA_GROUP_HISTORYREADABLE << UA_NUM_PERMISSION_BITS,
        UA_OWNER_HISTORYINSERT   = UA_GROUP_HISTORYINSERT   << UA_NUM_PERMISSION_BITS,
        UA_OWNER_HISTORYMODIFY   = UA_GROUP_HISTORYMODIFY   << UA_NUM_PERMISSION_BITS,
        UA_OWNER_HISTORYDELETE   = UA_GROUP_HISTORYDELETE   << UA_NUM_PERMISSION_BITS,
        UA_OWNER_EVENTREADABLE   = UA_GROUP_EVENTREADABLE   << UA_NUM_PERMISSION_BITS,
        UA_OWNER_EXECUTABLE      = UA_GROUP_EXECUTABLE      << UA_NUM_PERMISSION_BITS,
        UA_OWNER_ATTRWRITABLE    = UA_GROUP_ATTRWRITABLE    << UA_NUM_PERMISSION_BITS,
        UA_OWNER_OBSERVATION     = UA_OWNER_ATTRREADABLE | UA_OWNER_READABLE | UA_OWNER_BROWSEABLE | UA_OWNER_HISTORYREADABLE | UA_OWNER_EVENTREADABLE,
        UA_OWNER_OPERATION       = UA_OWNER_OBSERVATION | UA_OWNER_WRITABLE | UA_OWNER_EXECUTABLE,
        UA_OWNER_ALL             = UA_GROUP_ALL             << UA_NUM_PERMISSION_BITS,

        // additional flags
        UA_ALL_SIGNING_REQUIRED     = (0x1 << (3*UA_NUM_PERMISSION_BITS)),
        UA_ALL_ENCRYPTION_REQUIRED  = (0x2 << (3*UA_NUM_PERMISSION_BITS))
    };

    NodeAccessInfo(OpcUa_UInt32 mode, OpcUa_UInt16 ownerId, OpcUa_UInt16 groupId)
        : m_mode(mode), m_ownerId(ownerId), m_groupId(groupId)
    {}
    NodeAccessInfo(const NodeAccessInfo* pOther)
        : m_mode(pOther->m_mode), m_ownerId(pOther->m_ownerId), m_groupId(pOther->m_groupId)
    {}

    virtual bool isDefaultAttributeReadable() const
    {
        return (m_mode & UA_ATTRREADABLE) != 0;
    }
    virtual bool isDefaultReadable() const
    {
        return (m_mode & UA_READABLE) != 0;
    }
    virtual bool isDefaultWritable() const
    {
        return (m_mode & UA_WRITABLE) != 0;
    }
    virtual bool isDefaultBrowseable() const
    {
        return (m_mode & UA_BROWSEABLE) != 0;
    }
    virtual bool isDefaultHistoryReadable() const
    {
        return (m_mode & UA_HISTORYREADABLE) != 0;
    }
    virtual bool isDefaultHistoryInsertable() const
    {
        return (m_mode & UA_HISTORYINSERT) != 0;
    }
    virtual bool isDefaultHistoryModifiable() const
    {
        return (m_mode & UA_HISTORYMODIFY) != 0;
    }
    virtual bool isDefaultHistoryDeletable() const
    {
        return (m_mode & UA_HISTORYDELETE) != 0;
    }
    virtual bool isDefaultEventReadable() const
    {
        return (m_mode & UA_EVENTREADABLE) != 0;
    }
    virtual bool isDefaultExecutable() const
    {
        return (m_mode & UA_EXECUTABLE) != 0;
    }
    virtual bool isDefaultAttributeWritable() const
    {
        return (m_mode & UA_ATTRWRITABLE) != 0;
    }

    virtual bool checkSecurityMode(OpcUa_MessageSecurityMode securityMode) const
    {
        if (m_mode & UA_ALL_ENCRYPTION_REQUIRED)
        {
            return securityMode == OpcUa_MessageSecurityMode_SignAndEncrypt ? OpcUa_True : OpcUa_False;
        }
        else if (m_mode & UA_ALL_SIGNING_REQUIRED)
        {
            if (securityMode == OpcUa_MessageSecurityMode_Sign || securityMode == OpcUa_MessageSecurityMode_SignAndEncrypt)
            {
                return true;
            }
            return false;
        }

        return OpcUa_True;
    }
    OpcUa_UInt32 m_mode;
    OpcUa_UInt16 m_ownerId;
    OpcUa_UInt16 m_groupId;

protected:
    virtual ~NodeAccessInfo(){};

private:
    NodeAccessInfo(const NodeAccessInfo&);
    NodeAccessInfo& operator=(const NodeAccessInfo&);
};

#endif // SESSIONUSERCONTEXT_H
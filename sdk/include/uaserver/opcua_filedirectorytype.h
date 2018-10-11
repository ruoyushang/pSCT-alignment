/******************************************************************************
** opcua_filedirectorytype.h
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
** Project: C++ OPC Server SDK information model for namespace http://opcfoundation.org/UA/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#ifndef __OPCUA_FILEDIRECTORYTYPE_H__
#define __OPCUA_FILEDIRECTORYTYPE_H__

#include "opcua_filedirectorytypebase.h"

// Namespace for the UA information model http://opcfoundation.org/UA/
namespace OpcUa {


/** @brief Class implementing the UaObject interface for the FileDirectoryType.
 *
 * OPC UA Objects are used to represent systems, system components, real-world objects and software
 * objects. They have the NodeClass @ref L3UaNodeClassObject. The detailed description of Objects and their attributes
 * can be found in the general description of the @ref L3UaNodeClassObject node class.
 *
 *  **Object members of the FileDirectoryType:**
 *
 *  Browse Name           | Modelling Rule      | See Also
 *  ----------------------|---------------------|---------
 *  \<FileDirectoryName\> | OptionalPlaceholder | &nbsp;
 *  \<FileName\>          | OptionalPlaceholder | &nbsp;
 *
 *  **Method members of the FileDirectoryType:**
 *
 *  Browse Name          | Modelling Rule
 *  ---------------------|---------------
 *  \ref CreateDirectory | Mandatory
 *  \ref CreateFile      | Mandatory
 *  \ref Delete          | Mandatory
 *  \ref MoveOrCopy      | Mandatory
 *
 *  Defines a type for the representation of file directories.
 *
 *  It is expected that OPC UA Servers will create vendor specific subtypes of the
 *  \ref OpcUa::FileDirectoryType "FileDirectoryType" with additional
 *  functionalities like methods for creating symbolic links or setting access
 *  permissions. OPC UA Clients providing specialized file transfer user interfaces
 *  should be prepared to expose such additional methods to the user.
 *
 *  Instances of the ObjectType contain a list of \ref OpcUa::FileDirectoryType
 *  "FileDirectoryType" Objects representing the subdirectories of the file
 *  directory represented by the instance of this ObjectType.
 *
 *  Instances of the ObjectType contain a list of \ref OpcUa::FileType "FileType"
 *  Objects representing the files in the file directory represented by the
 *  instance of this ObjectType.
 *
 *  See also \ref Doc_OpcUa_FileDirectoryType for a documentation of the complete Information Model.
 */
class SERVER_CORE_EXPORT FileDirectoryType:
    public FileDirectoryTypeBase
{
    UA_DISABLE_COPY(FileDirectoryType);
protected:
    // destruction
    virtual ~FileDirectoryType();
public:
    // construction
    FileDirectoryType(const UaNodeId& nodeId, UaObject* pInstanceDeclarationObject, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    FileDirectoryType(const UaNodeId& nodeId, const UaString& name, OpcUa_UInt16 browseNameNameSpaceIndex, NodeManagerConfig* pNodeConfig, UaMutexRefCounted* pSharedMutex = NULL);
    FileDirectoryType(
        UaBase::Object*    pBaseNode,
        XmlUaNodeFactoryManager*   pFactory,
        NodeManagerConfig* pNodeConfig,
        UaMutexRefCounted* pSharedMutex = NULL);
    static void createTypes();
    static void clearStaticMembers();

    // UaFileDirectoryType method CreateDirectory
    virtual UaStatus CreateDirectory(
        const ServiceContext& serviceContext,
        const UaString& DirectoryName,
        UaNodeId& DirectoryNodeId);
    // UaFileDirectoryType method CreateFile
    virtual UaStatus CreateFile(
        const ServiceContext& serviceContext,
        const UaString& FileName,
        OpcUa_Boolean RequestFileOpen,
        UaNodeId& FileNodeId,
        OpcUa_UInt32& FileHandle);
    // UaFileDirectoryType method Delete
    virtual UaStatus Delete(
        const ServiceContext& serviceContext,
        const UaNodeId& ObjectToDelete);
    // UaFileDirectoryType method MoveOrCopy
    virtual UaStatus MoveOrCopy(
        const ServiceContext& serviceContext,
        const UaNodeId& ObjectToMoveOrCopy,
        const UaNodeId& TargetDirectory,
        OpcUa_Boolean CreateCopy,
        const UaString& NewName,
        UaNodeId& NewNodeId);

protected:

private:
};

} // End namespace for the UA information model http://opcfoundation.org/UA/

#endif // #ifndef __OPCUAFILEDIRECTORYTYPE_H__


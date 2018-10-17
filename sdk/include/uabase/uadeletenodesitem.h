/******************************************************************************
** uadeletenodesitem.h
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
** Portable UaDeleteNodesItem class.
**
******************************************************************************/
#ifndef UADELETENODESITEM_H
#define UADELETENODESITEM_H

#include <opcua_proxystub.h>

#include "uabase.h"
#include "uanodeid.h"
#include "uaarraytemplates.h"

class UaExtensionObject;
class UaVariant;
class UaDataValue;

class UABASE_EXPORT UaDeleteNodesItemPrivate;

/** @ingroup CppBaseLibraryClass
 *  @brief Wrapper class for the UA stack structure OpcUa_DeleteNodesItem.
 *
 *  This class encapsulates the native OpcUa_DeleteNodesItem structure
 *  and handles memory allocation and cleanup for you.
 *  UaDeleteNodesItem uses implicit sharing to avoid needless copying and to boost the performance.
 *  Only if you modify a shared DeleteNodesItem it creates a copy for that (copy-on-write).
 *  So assigning another UaDeleteNodesItem or passing it as parameter needs constant time and is nearly as fast as assigning a pointer.
 *
 *  A structure that is defined as the type of the nodesToDelete parameter of the
 *  DeleteNodes Service.
 */
class UABASE_EXPORT UaDeleteNodesItem
{
    UA_DECLARE_PRIVATE(UaDeleteNodesItem)
public:
    UaDeleteNodesItem();
    UaDeleteNodesItem(const UaDeleteNodesItem &other);
    UaDeleteNodesItem(const OpcUa_DeleteNodesItem &other);
    UaDeleteNodesItem(
        const UaNodeId& nodeId,
        OpcUa_Boolean deleteTargetReferences
        );
    UaDeleteNodesItem(const UaExtensionObject &extensionObject);
    UaDeleteNodesItem(const OpcUa_ExtensionObject &extensionObject);
    UaDeleteNodesItem(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    UaDeleteNodesItem(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    ~UaDeleteNodesItem();

    void clear();

    bool operator==(const UaDeleteNodesItem &other) const;
    bool operator!=(const UaDeleteNodesItem &other) const;

    UaDeleteNodesItem& operator=(const UaDeleteNodesItem &other);

    OpcUa_DeleteNodesItem* copy() const;
    void copyTo(OpcUa_DeleteNodesItem *pDst) const;

    static OpcUa_DeleteNodesItem* clone(const OpcUa_DeleteNodesItem& source);
    static void cloneTo(const OpcUa_DeleteNodesItem& source, OpcUa_DeleteNodesItem& copy);

    void attach(OpcUa_DeleteNodesItem *pValue);
    OpcUa_DeleteNodesItem* detach(OpcUa_DeleteNodesItem* pDst);

    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    void toExtensionObject(UaExtensionObject &extensionObject) const;
    void toExtensionObject(OpcUa_ExtensionObject &extensionObject) const;
    void toExtensionObject(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    void toExtensionObject(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    OpcUa_StatusCode setDeleteNodesItem(const UaExtensionObject &extensionObject);
    OpcUa_StatusCode setDeleteNodesItem(const OpcUa_ExtensionObject &extensionObject);
    OpcUa_StatusCode setDeleteNodesItem(UaExtensionObject &extensionObject, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setDeleteNodesItem(OpcUa_ExtensionObject &extensionObject, OpcUa_Boolean bDetach);

    void setDeleteNodesItem(
        const UaNodeId& nodeId,
        OpcUa_Boolean deleteTargetReferences
        );

    UaNodeId getNodeId() const;
    OpcUa_Boolean getDeleteTargetReferences() const;

    void setNodeId(const UaNodeId& nodeId);
    void setDeleteTargetReferences(OpcUa_Boolean deleteTargetReferences);
};

/** @ingroup CppBaseLibraryClass
 *  @brief Array class for the UA stack structure OpcUa_DeleteNodesItem.
 *
 *  This class encapsulates an array of the native OpcUa_DeleteNodesItem structure
 *  and handles memory allocation and cleanup for you.
 *  @see UaDeleteNodesItem for information about the encapsulated structure.
 */
class UABASE_EXPORT UaDeleteNodesItems
{
public:
    UaDeleteNodesItems();
    UaDeleteNodesItems(const UaDeleteNodesItems &other);
    UaDeleteNodesItems(OpcUa_Int32 length, OpcUa_DeleteNodesItem* data);
    virtual ~UaDeleteNodesItems();

    UaDeleteNodesItems& operator=(const UaDeleteNodesItems &other);
    const OpcUa_DeleteNodesItem& operator[](OpcUa_UInt32 index) const;
    OpcUa_DeleteNodesItem& operator[](OpcUa_UInt32 index);

    bool operator==(const UaDeleteNodesItems &other) const;
    bool operator!=(const UaDeleteNodesItems &other) const;

    void attach(OpcUa_UInt32 length, OpcUa_DeleteNodesItem* data);
    void attach(OpcUa_Int32 length, OpcUa_DeleteNodesItem* data);
    OpcUa_DeleteNodesItem* detach();

    void create(OpcUa_UInt32 length);
    void resize(OpcUa_UInt32 length);
    void clear();

    inline OpcUa_UInt32 length() const {return m_noOfElements;}
    inline const OpcUa_DeleteNodesItem* rawData() const {return m_data;}
    inline OpcUa_DeleteNodesItem* rawData() {return m_data;}

    void toVariant(UaVariant &variant) const;
    void toVariant(OpcUa_Variant &variant) const;
    void toVariant(UaVariant &variant, OpcUa_Boolean bDetach);
    void toVariant(OpcUa_Variant &variant, OpcUa_Boolean bDetach);

    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean updateTimeStamps) const;
    void toDataValue(UaDataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);
    void toDataValue(OpcUa_DataValue &dataValue, OpcUa_Boolean bDetach, OpcUa_Boolean updateTimeStamps);

    OpcUa_StatusCode setDeleteNodesItems(const UaVariant &variant);
    OpcUa_StatusCode setDeleteNodesItems(const OpcUa_Variant &variant);
    OpcUa_StatusCode setDeleteNodesItems(UaVariant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setDeleteNodesItems(OpcUa_Variant &variant, OpcUa_Boolean bDetach);
    OpcUa_StatusCode setDeleteNodesItems(OpcUa_Int32 length, OpcUa_DeleteNodesItem* data);

private:
    OpcUa_UInt32 m_noOfElements;
    OpcUa_DeleteNodesItem* m_data;
};

#endif // UADELETENODESITEM_H


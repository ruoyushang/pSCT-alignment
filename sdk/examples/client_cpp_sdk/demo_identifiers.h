/******************************************************************************
** demo_identifiers.h
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
** Project: C++ OPC Server SDK information model for namespace http://www.unifiedautomation.com/DemoServer/
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#ifndef __DEMO_IDENTIFIERS_H__
#define __DEMO_IDENTIFIERS_H__

#define Demo_NameSpaceUri "http://www.unifiedautomation.com/DemoServer/"


/*============================================================================
* DataType Identifiers
 *===========================================================================*/
#define DemoId_UnionTest 3006
#define DemoId_UnionTest_Encoding_DefaultBinary 5003
#define DemoId_UnionTest_Encoding_DefaultXml 5004
#define DemoId_Vector 3002
#define DemoId_Vector_Encoding_DefaultBinary 5054
#define DemoId_Vector_Encoding_DefaultXml 5055
#define DemoId_BinarySchema 6181
#define DemoId_BinarySchema_UnionTest 6034
#define DemoId_BinarySchema_Vector 6185
#define DemoId_BinarySchema_NamespaceUri 6182
#define DemoId_XmlSchema 6183
#define DemoId_XmlSchema_UnionTest 6035
#define DemoId_XmlSchema_Vector 6186
#define DemoId_XmlSchema_NamespaceUri 6184

#ifdef _WIN32
#ifdef _CPP_DEMO_BUILD_DLL
# define CPP_DEMO_EXPORT __declspec(dllexport)
#elif defined _CPP_DEMO_USE_DLL
# define CPP_DEMO_EXPORT __declspec(dllimport)
#else
# define CPP_DEMO_EXPORT
#endif
#else
# define CPP_DEMO_EXPORT
#endif

///////////////////////////////////////////////
// Private Class Hiding Macros
#define DEMO_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr); } \
    friend class Class##Private; \
    Class##Private *d_ptr;

#define DEMO_DECLARE_PUBLIC(Class) \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define DEMO_D(Class) Class##Private * const d = d_func()
#define DEMO_CD(Class) const Class##Private * const d = d_func()
#define DEMO_Q(Class) Class * const q = q_func()

#endif // __DEMO_IDENTIFIERS_H__


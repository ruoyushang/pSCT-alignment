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

#include "version_coremodule.h"
#if (PROD_MAJOR != 1 || (PROD_MAJOR == 1 && PROD_MINOR < 5) || (PROD_MAJOR == 1 && PROD_MINOR == 5 && PROD_MINOR2 < 0))
#if ! (PROD_MAJOR == 1 && PROD_MINOR == 3 && PROD_MINOR2 == 0 && CPP_SDK_BUILD == 193)
#error The code is generated for SDK version 1.5.0
#endif
#endif

/*============================================================================
* ObjectType Identifiers
 *===========================================================================*/
#define DemoId_SampleEventType 1005
#define DemoId_BoilerType 1003
#define DemoId_FillLevelSensorType 1002
#define DemoId_MachineType 1004
#define DemoId_TemperatureSensorType 1001

/*============================================================================
* VariableType Identifiers
 *===========================================================================*/
#define DemoId_WorkOrderVariableType 2001

/*============================================================================
* DataType Identifiers
 *===========================================================================*/
#define DemoId_HeaterStatus 3001
#define DemoId_HeaterStatus_EnumStrings 6028
#define DemoId_Priority 3005
#define DemoId_Priority_EnumValues 6033
#define DemoId_AccessRights 3008
#define DemoId_AccessRights_OptionSetValues 6039
#define DemoId_AccessRights_Encoding_DefaultBinary 5007
#define DemoId_AccessRights_Encoding_DefaultXml 5008
#define DemoId_StructureWithOptionalFields 3007
#define DemoId_StructureWithOptionalFields_Encoding_DefaultBinary 5005
#define DemoId_StructureWithOptionalFields_Encoding_DefaultXml 5006
#define DemoId_UnionTest 3006
#define DemoId_UnionTest_Encoding_DefaultBinary 5003
#define DemoId_UnionTest_Encoding_DefaultXml 5004
#define DemoId_Vector 3002
#define DemoId_Vector_Encoding_DefaultBinary 5054
#define DemoId_Vector_Encoding_DefaultXml 5055
#define DemoId_WorkOrderStatusType 3004
#define DemoId_WorkOrderStatusType_Encoding_DefaultBinary 5011
#define DemoId_WorkOrderStatusType_Encoding_DefaultXml 5012
#define DemoId_WorkOrderType 3003
#define DemoId_WorkOrderType_Encoding_DefaultBinary 5013
#define DemoId_WorkOrderType_Encoding_DefaultXml 5014
#define DemoId_BinarySchema 6181
#define DemoId_BinarySchema_AccessRights 6040
#define DemoId_BinarySchema_StructureWithOptionalFields 6037
#define DemoId_BinarySchema_UnionTest 6034
#define DemoId_BinarySchema_Vector 6185
#define DemoId_BinarySchema_WorkOrderStatusType 6200
#define DemoId_BinarySchema_WorkOrderType 6202
#define DemoId_BinarySchema_NamespaceUri 6182
#define DemoId_XmlSchema 6183
#define DemoId_XmlSchema_AccessRights 6041
#define DemoId_XmlSchema_StructureWithOptionalFields 6038
#define DemoId_XmlSchema_UnionTest 6035
#define DemoId_XmlSchema_Vector 6186
#define DemoId_XmlSchema_WorkOrderStatusType 6201
#define DemoId_XmlSchema_WorkOrderType 6203
#define DemoId_XmlSchema_NamespaceUri 6184

/*============================================================================
* ReferenceType Identifiers
 *===========================================================================*/

/*============================================================================
* Object Identifiers
 *===========================================================================*/
#define DemoId_BoilerType_FillLevelSensor 5002
#define DemoId_BoilerType_TemperatureSensor 5001
#define DemoId_MachineType_TemperatureSensor 5065
#define DemoId_Demo "Demo"
#define DemoId_Demo_Static "Demo.Static"
#define DemoId_Demo_Static_Arrays "Demo.Static.Arrays"
#define DemoId_Demo_Static_Scalar "Demo.Static.Scalar"
#define DemoId_Demo_Static_Scalar_Structures "Demo.Static.Scalar.Structures"
#define DemoId_Demo_Dynamic "Demo.Dynamic"
#define DemoId_Demo_Dynamic_Arrays "Demo.Dynamic.Arrays"
#define DemoId_Demo_Dynamic_Scalar "Demo.Dynamic.Scalar"
#define DemoId_Demo_History "Demo.History"
#define DemoId_Demo_Method "Demo.Method"
#define DemoId_Demo_Events "Demo.Events"
#define DemoId_Server_Machine 5066
#define DemoId_Server_Machine_TemperatureSensor 5067
#define DemoId_Server_SampleEventNotifier 5068
#define DemoId_Demo_AccessRights "Demo.AccessRights"
#define DemoId_Demo_AccessRights_Access_All "Demo.AccessRights.Access_All"
#define DemoId_Demo_AccessRights_Access_John "Demo.AccessRights.Access_John"
#define DemoId_Demo_AccessRights_Access_Operators "Demo.AccessRights.Access_Operators"
#define DemoId_Demo_AccessRights_Browse_All "Demo.AccessRights.Browse_All"
#define DemoId_Demo_AccessRights_Browse_John "Demo.AccessRights.Browse_John"
#define DemoId_Demo_AccessRights_Browse_Operators "Demo.AccessRights.Browse_Operators"
#define DemoId_Demo_Massfolder_Static "Demo.Massfolder_Static"
#define DemoId_Demo_Massfolder_Dynamic "Demo.Massfolder_Dynamic"
#define DemoId_Demo_DynamicNodes "Demo.DynamicNodes"
#define DemoId_Demo_BoilerDemo "Demo.BoilerDemo"
#define DemoId_Demo_BoilerDemo_Boiler1 "Demo.BoilerDemo.Boiler1"
#define DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor "Demo.BoilerDemo.Boiler1.FillLevelSensor"
#define DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor "Demo.BoilerDemo.Boiler1.TemperatureSensor"
#define DemoId_Demo_CTT "Demo.CTT"
#define DemoId_Demo_CTT_Methods "Demo.CTT.Methods"
#define DemoId_Demo_CTT_NodeClasses "Demo.CTT.NodeClasses"
#define DemoId_Demo_CTT_NodeClasses_Object "Demo.CTT.NodeClasses.Object"
#define DemoId_Demo_CTT_Paths "Demo.CTT.Paths"
#define DemoId_Demo_CTT_Paths_folder1 "Demo.CTT.Paths.folder1"
#define DemoId_Demo_CTT_Paths_folder1_folder2 "Demo.CTT.Paths.folder1.folder2"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3 "Demo.CTT.Paths.folder1.folder2.folder3"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4 "Demo.CTT.Paths.folder1.folder2.folder3.folder4"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5 "Demo.CTT.Paths.folder1.folder2.folder3.folder4.folder5"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6 "Demo.CTT.Paths.folder1.folder2.folder3.folder4.folder5.folder6"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7 "Demo.CTT.Paths.folder1.folder2.folder3.folder4.folder5.folder6.folder7"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8 "Demo.CTT.Paths.folder1.folder2.folder3.folder4.folder5.folder6.folder7.folder8"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8_folder9 "Demo.CTT.Paths.folder1.folder2.folder3.folder4.folder5.folder6.folder7.folder8.folder9"
#define DemoId_Demo_CTT_Paths_folder1_folder2_folder3_folder4_folder5_folder6_folder7_folder8_folder9_folder10 "Demo.CTT.Paths.folder1.folder2.folder3.folder4.folder5.folder6.folder7.folder8.folder9.folder10"
#define DemoId_Demo_CTT_References "Demo.CTT.References"
#define DemoId_Demo_CTT_References_Has3ForwardReferences1 "Demo.CTT.References.Has3ForwardReferences1"
#define DemoId_Demo_CTT_References_Has3ForwardReferences2 "Demo.CTT.References.Has3ForwardReferences2"
#define DemoId_Demo_CTT_References_Has3ForwardReferences2_Object 5061
#define DemoId_Demo_CTT_References_Has3ForwardReferences3 "Demo.CTT.References.Has3ForwardReferences3"
#define DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1 5056
#define DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode1_Has3InverseReferences "Demo.CTT.References.Has3InverseReferences"
#define DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode2 5057
#define DemoId_Demo_CTT_References_Has3ForwardReferences3_ReferencedNode3 5058
#define DemoId_Demo_CTT_References_Has3ForwardReferences4 "Demo.CTT.References.Has3ForwardReferences4"
#define DemoId_Demo_CTT_References_Has3ForwardReferences5 "Demo.CTT.References.Has3ForwardReferences5"
#define DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode1 5059
#define DemoId_Demo_CTT_References_HasInverseAndForwardReferences "Demo.CTT.References.HasInverseAndForwardReferences"
#define DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes "Demo.CTT.References.HasReferencesWithDifferentParentTypes"
#define DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode4 5060
#define DemoId_Demo_CTT_SecurityAccess "Demo.CTT.SecurityAccess"
#define DemoId_Demo_CTT_Static "Demo.CTT.Static"
#define DemoId_Demo_CTT_Static_All_Profiles "Demo.CTT.Static.AllProfiles"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays "Demo.CTT.AllProfiles.Arrays"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar "Demo.CTT.AllProfiles.Scalar"
#define DemoId_Demo_CTT_Static_DA_Profile "Demo.CTT.Static.DAProfile"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType "Demo.CTT.DAProfile.AnalogType"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays "Demo.CTT.DAProfile.AnalogTypeArrays"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem "Demo.CTT.DAProfile.DataItem"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType "Demo.CTT.DAProfile.DiscreteType"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType "Demo.CTT.DAProfile.MultiStateValueDiscreteType"
#define DemoId_Demo_UnicodeTest "Demo.UnicodeTest"
#define DemoId_Demo_UnicodeTest_Die_heisse_Zypernsonne_quaelte_Max_und_Victoria_ja_boese_auf_dem_Weg_bis_zur_Kueste 5017
#define DemoId_Demo_UnicodeTest_Franz_jagt_im_komplett_verwahrlosten_Taxi_quer_durch_Bayern 5018
#define DemoId_Demo_UnicodeTest_The_quick_brown_fox_jumps_over_the_lazy_dog 5019
#define DemoId_Demo_UnicodeTest_Voyez_le_brick_g_ant_que_j_examine_pr_s_du_wharf 5020
#define DemoId_Demo_UnicodeTest_Node5016 5016
#define DemoId_Demo_Icon "Demo.Icon"
#define DemoId_Demo_Images "Demo.Images"
#define DemoId_Demo_Images_Dynamic "Demo.Images.Dynamic"
#define DemoId_Demo_Images_Static "Demo.Images.Static"
#define DemoId_Demo_Files "Demo.Files"
#define DemoId_Demo_WorkOrder "Demo.WorkOrder"

/*============================================================================
* Variable Identifiers
 *===========================================================================*/
#define DemoId_SampleEventType_Boolean 6258
#define DemoId_SampleEventType_Byte 6260
#define DemoId_SampleEventType_ByteString 6272
#define DemoId_SampleEventType_DateTime 6270
#define DemoId_SampleEventType_Double 6268
#define DemoId_SampleEventType_ExpandedNodeId 6275
#define DemoId_SampleEventType_Float 6267
#define DemoId_SampleEventType_Guid 6271
#define DemoId_SampleEventType_Int16 6261
#define DemoId_SampleEventType_Int32 6263
#define DemoId_SampleEventType_Int64 6265
#define DemoId_SampleEventType_LocalizedText 6278
#define DemoId_SampleEventType_NodeId 6274
#define DemoId_SampleEventType_QualifiedName 6277
#define DemoId_SampleEventType_SByte 6259
#define DemoId_SampleEventType_StatusCode 6276
#define DemoId_SampleEventType_String 6269
#define DemoId_SampleEventType_UInt16 6262
#define DemoId_SampleEventType_UInt32 6264
#define DemoId_SampleEventType_UInt64 6266
#define DemoId_SampleEventType_XmlElement 6273
#define DemoId_BoilerType_Fill_InputArguments 6029
#define DemoId_BoilerType_FillLevelSensor_FillLevel 6019
#define DemoId_BoilerType_FillLevelSensor_FillLevel_Definition 6020
#define DemoId_BoilerType_FillLevelSensor_FillLevel_EngineeringUnits 6021
#define DemoId_BoilerType_FillLevelSensor_FillLevel_EURange 6022
#define DemoId_BoilerType_FillLevelSensor_FillLevel_InstrumentRange 6023
#define DemoId_BoilerType_FillLevelSensor_FillLevel_ValuePrecision 6024
#define DemoId_BoilerType_FillLevelSetPoint 6025
#define DemoId_BoilerType_Heat_InputArguments 6030
#define DemoId_BoilerType_HeaterStatus 6027
#define DemoId_BoilerType_TemperatureSensor_Temperature 6013
#define DemoId_BoilerType_TemperatureSensor_Temperature_Definition 6014
#define DemoId_BoilerType_TemperatureSensor_Temperature_EngineeringUnits 6015
#define DemoId_BoilerType_TemperatureSensor_Temperature_EURange 6016
#define DemoId_BoilerType_TemperatureSensor_Temperature_InstrumentRange 6017
#define DemoId_BoilerType_TemperatureSensor_Temperature_ValuePrecision 6018
#define DemoId_BoilerType_TemperatureSetPoint 6026
#define DemoId_FillLevelSensorType_FillLevel 6007
#define DemoId_FillLevelSensorType_FillLevel_Definition 6011
#define DemoId_FillLevelSensorType_FillLevel_EngineeringUnits 6008
#define DemoId_FillLevelSensorType_FillLevel_EURange 6009
#define DemoId_FillLevelSensorType_FillLevel_InstrumentRange 6010
#define DemoId_FillLevelSensorType_FillLevel_ValuePrecision 6012
#define DemoId_MachineType_HeaterSwitch 6256
#define DemoId_MachineType_TemperatureSensor_Temperature 6250
#define DemoId_MachineType_TemperatureSensor_Temperature_EURange 6251
#define DemoId_TemperatureSensorType_Temperature 6001
#define DemoId_TemperatureSensorType_Temperature_Definition 6005
#define DemoId_TemperatureSensorType_Temperature_EngineeringUnits 6002
#define DemoId_TemperatureSensorType_Temperature_EURange 6003
#define DemoId_TemperatureSensorType_Temperature_InstrumentRange 6004
#define DemoId_TemperatureSensorType_Temperature_ValuePrecision 6006
#define DemoId_WorkOrderVariableType_AssetID 6043
#define DemoId_WorkOrderVariableType_ID 6042
#define DemoId_WorkOrderVariableType_StartTime 6044
#define DemoId_WorkOrderVariableType_StatusComments 6045
#define DemoId_Demo_Static_Arrays_BaseDataType "Demo.Static.Arrays.BaseDataType"
#define DemoId_Demo_Static_Arrays_Boolean "Demo.Static.Arrays.Boolean"
#define DemoId_Demo_Static_Arrays_Byte "Demo.Static.Arrays.Byte"
#define DemoId_Demo_Static_Arrays_ByteString "Demo.Static.Arrays.ByteString"
#define DemoId_Demo_Static_Arrays_DateTime "Demo.Static.Arrays.DateTime"
#define DemoId_Demo_Static_Arrays_Double "Demo.Static.Arrays.Double"
#define DemoId_Demo_Static_Arrays_Duration "Demo.Static.Arrays.Duration"
#define DemoId_Demo_Static_Arrays_ExpandedNodeId "Demo.Static.Arrays.ExpandedNodeId"
#define DemoId_Demo_Static_Arrays_Float "Demo.Static.Arrays.Float"
#define DemoId_Demo_Static_Arrays_Guid "Demo.Static.Arrays.Guid"
#define DemoId_Demo_Static_Arrays_ImagePNG "Demo.Static.Arrays.ImagePNG"
#define DemoId_Demo_Static_Arrays_Int16 "Demo.Static.Arrays.Int16"
#define DemoId_Demo_Static_Arrays_Int32 "Demo.Static.Arrays.Int32"
#define DemoId_Demo_Static_Arrays_Int64 "Demo.Static.Arrays.Int64"
#define DemoId_Demo_Static_Arrays_Integer "Demo.Static.Arrays.Integer"
#define DemoId_Demo_Static_Arrays_LocalizedText "Demo.Static.Arrays.LocalizedText"
#define DemoId_Demo_Static_Arrays_NodeId "Demo.Static.Arrays.NodeId"
#define DemoId_Demo_Static_Arrays_Number "Demo.Static.Arrays.Number"
#define DemoId_Demo_Static_Arrays_NumericRange "Demo.Static.Arrays.NumericRange"
#define DemoId_Demo_Static_Arrays_QualifiedName "Demo.Static.Arrays.QualifiedName"
#define DemoId_Demo_Static_Arrays_SByte "Demo.Static.Arrays.SByte"
#define DemoId_Demo_Static_Arrays_ServerState "Demo.Static.Arrays.ServerState"
#define DemoId_Demo_Static_Arrays_StatusCode "Demo.Static.Arrays.StatusCode"
#define DemoId_Demo_Static_Arrays_String "Demo.Static.Arrays.String"
#define DemoId_Demo_Static_Arrays_Structure "Demo.Static.Arrays.Structure"
#define DemoId_Demo_Static_Arrays_UInt16 "Demo.Static.Arrays.UInt16"
#define DemoId_Demo_Static_Arrays_UInt32 "Demo.Static.Arrays.UInt32"
#define DemoId_Demo_Static_Arrays_UInt64 "Demo.Static.Arrays.UInt64"
#define DemoId_Demo_Static_Arrays_UInteger "Demo.Static.Arrays.UInteger"
#define DemoId_Demo_Static_Arrays_UtcTime "Demo.Static.Arrays.UtcTime"
#define DemoId_Demo_Static_Arrays_Vector "Demo.Static.Arrays.Vector"
#define DemoId_Demo_Static_Arrays_WorkOrder "Demo.Static.Arrays.WorkOrder"
#define DemoId_Demo_Static_Arrays_XmlElement "Demo.Static.Arrays.XmlElement"
#define DemoId_Demo_Static_Scalar_BaseDataType "Demo.Static.Scalar.BaseDataType"
#define DemoId_Demo_Static_Scalar_Boolean "Demo.Static.Scalar.Boolean"
#define DemoId_Demo_Static_Scalar_Byte "Demo.Static.Scalar.Byte"
#define DemoId_Demo_Static_Scalar_ByteString "Demo.Static.Scalar.ByteString"
#define DemoId_Demo_Static_Scalar_DateTime "Demo.Static.Scalar.DateTime"
#define DemoId_Demo_Static_Scalar_Double "Demo.Static.Scalar.Double"
#define DemoId_Demo_Static_Scalar_Duration "Demo.Static.Scalar.Duration"
#define DemoId_Demo_Static_Scalar_ExpandedNodeId "Demo.Static.Scalar.ExpandedNodeId"
#define DemoId_Demo_Static_Scalar_Float "Demo.Static.Scalar.Float"
#define DemoId_Demo_Static_Scalar_Guid "Demo.Static.Scalar.Guid"
#define DemoId_Demo_Static_Scalar_HeaterStatus "Demo.Static.Scalar.HeaterStatus"
#define DemoId_Demo_Static_Scalar_ImagePNG "Demo.Static.Scalar.ImagePNG"
#define DemoId_Demo_Static_Scalar_Int16 "Demo.Static.Scalar.Int16"
#define DemoId_Demo_Static_Scalar_Int32 "Demo.Static.Scalar.Int32"
#define DemoId_Demo_Static_Scalar_Int64 "Demo.Static.Scalar.Int64"
#define DemoId_Demo_Static_Scalar_Integer "Demo.Static.Scalar.Integer"
#define DemoId_Demo_Static_Scalar_LocalizedText "Demo.Static.Scalar.LocalizedText"
#define DemoId_Demo_Static_Scalar_NodeId "Demo.Static.Scalar.NodeId"
#define DemoId_Demo_Static_Scalar_Number "Demo.Static.Scalar.Number"
#define DemoId_Demo_Static_Scalar_NumericRange "Demo.Static.Scalar.NumericRange"
#define DemoId_Demo_Static_Scalar_Priority "Demo.Static.Scalar.Priority"
#define DemoId_Demo_Static_Scalar_QualifiedName "Demo.Static.Scalar.QualifiedName"
#define DemoId_Demo_Static_Scalar_SByte "Demo.Static.Scalar.SByte"
#define DemoId_Demo_Static_Scalar_ServerState "Demo.Static.Scalar.ServerState"
#define DemoId_Demo_Static_Scalar_StatusCode "Demo.Static.Scalar.StatusCode"
#define DemoId_Demo_Static_Scalar_String "Demo.Static.Scalar.String"
#define DemoId_Demo_Static_Scalar_Structures_OptionalFields "Demo.Static.Scalar.OptionalFields"
#define DemoId_Demo_Static_Scalar_Structures_OptionSet "Demo.Static.Scalar.OptionSet"
#define DemoId_Demo_Static_Scalar_Structures_Structure "Demo.Static.Scalar.Structure"
#define DemoId_Demo_Static_Scalar_Structures_Union "Demo.Static.Scalar.Union"
#define DemoId_Demo_Static_Scalar_Structures_Vector "Demo.Static.Scalar.Vector"
#define DemoId_Demo_Static_Scalar_Structures_WorkOrder "Demo.Static.Scalar.WorkOrder"
#define DemoId_Demo_Static_Scalar_UInt16 "Demo.Static.Scalar.UInt16"
#define DemoId_Demo_Static_Scalar_UInt32 "Demo.Static.Scalar.UInt32"
#define DemoId_Demo_Static_Scalar_UInt64 "Demo.Static.Scalar.UInt64"
#define DemoId_Demo_Static_Scalar_UInteger "Demo.Static.Scalar.UInteger"
#define DemoId_Demo_Static_Scalar_UtcTime "Demo.Static.Scalar.UtcTime"
#define DemoId_Demo_Static_Scalar_XmlElement "Demo.Static.Scalar.XmlElement"
#define DemoId_Demo_Dynamic_Arrays_Boolean "Demo.Dynamic.Arrays.Boolean"
#define DemoId_Demo_Dynamic_Arrays_Byte "Demo.Dynamic.Arrays.Byte"
#define DemoId_Demo_Dynamic_Arrays_ByteString "Demo.Dynamic.Arrays.ByteString"
#define DemoId_Demo_Dynamic_Arrays_DateTime "Demo.Dynamic.Arrays.DateTime"
#define DemoId_Demo_Dynamic_Arrays_Double "Demo.Dynamic.Arrays.Double"
#define DemoId_Demo_Dynamic_Arrays_ExpandedNodeId "Demo.Dynamic.Arrays.ExpandedNodeId"
#define DemoId_Demo_Dynamic_Arrays_Float "Demo.Dynamic.Arrays.Float"
#define DemoId_Demo_Dynamic_Arrays_Guid "Demo.Dynamic.Arrays.Guid"
#define DemoId_Demo_Dynamic_Arrays_Int16 "Demo.Dynamic.Arrays.Int16"
#define DemoId_Demo_Dynamic_Arrays_Int32 "Demo.Dynamic.Arrays.Int32"
#define DemoId_Demo_Dynamic_Arrays_Int64 "Demo.Dynamic.Arrays.Int64"
#define DemoId_Demo_Dynamic_Arrays_LocalizedText "Demo.Dynamic.Arrays.LocalizedText"
#define DemoId_Demo_Dynamic_Arrays_NodeId "Demo.Dynamic.Arrays.NodeId"
#define DemoId_Demo_Dynamic_Arrays_QualifiedName "Demo.Dynamic.Arrays.QualifiedName"
#define DemoId_Demo_Dynamic_Arrays_SByte "Demo.Dynamic.Arrays.SByte"
#define DemoId_Demo_Dynamic_Arrays_StatusCode "Demo.Dynamic.Arrays.StatusCode"
#define DemoId_Demo_Dynamic_Arrays_String "Demo.Dynamic.Arrays.String"
#define DemoId_Demo_Dynamic_Arrays_UInt16 "Demo.Dynamic.Arrays.UInt16"
#define DemoId_Demo_Dynamic_Arrays_UInt32 "Demo.Dynamic.Arrays.UInt32"
#define DemoId_Demo_Dynamic_Arrays_UInt64 "Demo.Dynamic.Arrays.UInt64"
#define DemoId_Demo_Dynamic_Arrays_XmlElement "Demo.Dynamic.Arrays.XmlElement"
#define DemoId_Demo_Dynamic_Scalar_Boolean "Demo.Dynamic.Scalar.Boolean"
#define DemoId_Demo_Dynamic_Scalar_Byte "Demo.Dynamic.Scalar.Byte"
#define DemoId_Demo_Dynamic_Scalar_ByteString "Demo.Dynamic.Scalar.ByteString"
#define DemoId_Demo_Dynamic_Scalar_DateTime "Demo.Dynamic.Scalar.DateTime"
#define DemoId_Demo_Dynamic_Scalar_Double "Demo.Dynamic.Scalar.Double"
#define DemoId_Demo_Dynamic_Scalar_ExpandedNodeId "Demo.Dynamic.Scalar.ExpandedNodeId"
#define DemoId_Demo_Dynamic_Scalar_Float "Demo.Dynamic.Scalar.Float"
#define DemoId_Demo_Dynamic_Scalar_Guid "Demo.Dynamic.Scalar.Guid"
#define DemoId_Demo_Dynamic_Scalar_ImageGIF "Demo.Dynamic.Scalar.ImageGIF"
#define DemoId_Demo_Dynamic_Scalar_Int16 "Demo.Dynamic.Scalar.Int16"
#define DemoId_Demo_Dynamic_Scalar_Int32 "Demo.Dynamic.Scalar.Int32"
#define DemoId_Demo_Dynamic_Scalar_Int64 "Demo.Dynamic.Scalar.Int64"
#define DemoId_Demo_Dynamic_Scalar_LocalizedText "Demo.Dynamic.Scalar.LocalizedText"
#define DemoId_Demo_Dynamic_Scalar_NodeId "Demo.Dynamic.Scalar.NodeId"
#define DemoId_Demo_Dynamic_Scalar_QualifiedName "Demo.Dynamic.Scalar.QualifiedName"
#define DemoId_Demo_Dynamic_Scalar_Quality_DynamicValue "Demo.Dynamic.Scalar.Quality_DynamicValue"
#define DemoId_Demo_Dynamic_Scalar_Quality_StaticValue "Demo.Dynamic.Scalar.Quality_StaticValue"
#define DemoId_Demo_Dynamic_Scalar_SByte "Demo.Dynamic.Scalar.SByte"
#define DemoId_Demo_Dynamic_Scalar_StatusCode "Demo.Dynamic.Scalar.StatusCode"
#define DemoId_Demo_Dynamic_Scalar_String "Demo.Dynamic.Scalar.String"
#define DemoId_Demo_Dynamic_Scalar_UInt16 "Demo.Dynamic.Scalar.UInt16"
#define DemoId_Demo_Dynamic_Scalar_UInt32 "Demo.Dynamic.Scalar.UInt32"
#define DemoId_Demo_Dynamic_Scalar_UInt64 "Demo.Dynamic.Scalar.UInt64"
#define DemoId_Demo_Dynamic_Scalar_XmlElement "Demo.Dynamic.Scalar.XmlElement"
#define DemoId_Demo_History_ByteWithHistory "Demo.History.ByteWithHistory"
#define DemoId_Demo_History_DataLoggerActive "Demo.History.DataLoggerActive"
#define DemoId_Demo_History_DoubleWithHistory "Demo.History.DoubleWithHistory"
#define DemoId_Demo_Method_EnumTest_InputArguments "Demo.Method.EnumTest.InputArguments"
#define DemoId_Demo_Method_EnumTest_OutputArguments "Demo.Method.EnumTest.OutputArguments"
#define DemoId_Demo_Method_Multiply_InputArguments 6176
#define DemoId_Demo_Method_Multiply_OutputArguments 6177
#define DemoId_Demo_Method_VectorAdd_InputArguments 6187
#define DemoId_Demo_Method_VectorAdd_OutputArguments 6188
#define DemoId_Server_Machine_HeaterSwitch 6257
#define DemoId_Server_Machine_TemperatureSensor_Temperature 6252
#define DemoId_Server_Machine_TemperatureSensor_Temperature_EURange 6253
#define DemoId_Demo_Events_Trigger_BaseEvent 6254
#define DemoId_Demo_Events_Trigger_SampleEvent 6255
#define DemoId_Demo_AccessRights_Access_All_All_RO "Demo.AccessRights.Access_All.All_RO"
#define DemoId_Demo_AccessRights_Access_All_All_RW "Demo.AccessRights.Access_All.All_RW"
#define DemoId_Demo_AccessRights_Access_All_All_WO "Demo.AccessRights.Access_All.All_WO"
#define DemoId_Demo_AccessRights_Access_John_All_RO_John_RW "Demo.AccessRights.Access_John.All_RO_John_RW"
#define DemoId_Demo_AccessRights_Access_John_All_WO_John_RW "Demo.AccessRights.Access_John.All_WO_John_RW"
#define DemoId_Demo_AccessRights_Access_John_John_RO "Demo.AccessRights.Access_John.John_RO"
#define DemoId_Demo_AccessRights_Access_John_John_RW "Demo.AccessRights.Access_John.John_RW"
#define DemoId_Demo_AccessRights_Access_John_John_WO "Demo.AccessRights.Access_John.John_WO"
#define DemoId_Demo_AccessRights_Access_Operators_All_RO_Operators_RW "Demo.AccessRights.Access_Operators.All_RO_Operators_RW"
#define DemoId_Demo_AccessRights_Access_Operators_All_WO_Operators_RW 6338
#define DemoId_Demo_AccessRights_Access_Operators_Operators_RO "Demo.AccessRights.Access_Operators.Operators_RO"
#define DemoId_Demo_AccessRights_Access_Operators_Operators_RW "Demo.AccessRights.Access_Operators.Operators_RW"
#define DemoId_Demo_AccessRights_Access_Operators_Operators_WO "Demo.AccessRights.Access_Operators.Operators_WO"
#define DemoId_Demo_DynamicNodes_NodeVersion 6031
#define DemoId_Demo_BoilerDemo_Boiler1_Fill_InputArguments 6032
#define DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor_FillLevel "Demo.BoilerDemo.Boiler1.FillLevelSensor.FillLevel"
#define DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor_FillLevel_EngineeringUnits "Demo.BoilerDemo.Boiler1.FillLevelSensor.FillLevel.EngineeringUnits"
#define DemoId_Demo_BoilerDemo_Boiler1_FillLevelSensor_FillLevel_EURange "Demo.BoilerDemo.Boiler1.FillLevelSensor.FillLevel.EURange"
#define DemoId_Demo_BoilerDemo_Boiler1_FillLevelSetPoint "Demo.BoilerDemo.Boiler1.FillLevelSetPoint"
#define DemoId_Demo_BoilerDemo_Boiler1_Heat_InputArguments 6036
#define DemoId_Demo_BoilerDemo_Boiler1_HeaterStatus "Demo.BoilerDemo.Boiler1.HeaterStatus"
#define DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor_Temperature "Demo.BoilerDemo.Boiler1.TemperatureSensor.Temperature"
#define DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor_Temperature_EngineeringUnits "Demo.BoilerDemo.Boiler1.TemperatureSensor.Temperature.EngineeringUnits"
#define DemoId_Demo_BoilerDemo_Boiler1_TemperatureSensor_Temperature_EURange "Demo.BoilerDemo.Boiler1.TemperatureSensor.Temperature.EURange"
#define DemoId_Demo_BoilerDemo_Boiler1_TemperatureSetPoint "Demo.BoilerDemo.Boiler1.TemperatureSetPoint"
#define DemoId_Demo_CTT_Methods_MethodIO_InputArguments "Demo.CTT.Methods.MethodIO.InputArguments"
#define DemoId_Demo_CTT_Methods_MethodIO_OutputArguments "Demo.CTT.Methods.MethodIO.OutputArguments"
#define DemoId_Demo_CTT_NodeClasses_Variable "Demo.CTT.NodeClasses.Variable"
#define DemoId_Demo_CTT_References_Has3ForwardReferences1_ReferencedNode1 6223
#define DemoId_Demo_CTT_References_Has3ForwardReferences1_ReferencedNode2 6224
#define DemoId_Demo_CTT_References_Has3ForwardReferences1_ReferencedNode3 6225
#define DemoId_Demo_CTT_References_Has3ForwardReferences2_BaseDataVariable 6234
#define DemoId_Demo_CTT_References_Has3ForwardReferences2_Property 6235
#define DemoId_Demo_CTT_References_Has3ForwardReferences4_ReferencedNode1 6226
#define DemoId_Demo_CTT_References_Has3ForwardReferences4_ReferencedNode2 6227
#define DemoId_Demo_CTT_References_Has3ForwardReferences4_ReferencedNode3 6228
#define DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode2 6229
#define DemoId_Demo_CTT_References_HasInverseAndForwardReferences_ReferencedNode 6230
#define DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode1 6231
#define DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode2 6232
#define DemoId_Demo_CTT_References_HasReferencesWithDifferentParentTypes_ReferencedNode3 6233
#define DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead "Demo.CTT.SecurityAccess.AccessLevel_CurrentRead"
#define DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead_NotCurrentWrite "Demo.CTT.SecurityAccess.AccessLevel_CurrentRead_NotCurrentWwrite"
#define DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentRead_NotUser "Demo.CTT.SecurityAccess.AccessLevel_CurrentRead_NotUser"
#define DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite "Demo.CTT.SecurityAccess.AccessLevel_CurrentWrite"
#define DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite_NotCurrentRead "Demo.CTT.SecurityAccess.AccessLevel_CurrentWrite_NotCurrentRead"
#define DemoId_Demo_CTT_SecurityAccess_AccessLevel_CurrentWrite_NotUser "Demo.CTT.SecurityAccess.AccessLevel_CurrentWrite_NotUser"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Bool "Demo.CTT.AllProfiles.Arrays.Bool"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Byte "Demo.CTT.AllProfiles.Arrays.Byte"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_ByteString "Demo.CTT.AllProfiles.Arrays.ByteString"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_DateTime "Demo.CTT.AllProfiles.Arrays.DateTime"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Double "Demo.CTT.AllProfiles.Arrays.Double"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Duration "Demo.CTT.AllProfiles.Arrays.Duration"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Float "Demo.CTT.AllProfiles.Arrays.Float"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Guid "Demo.CTT.AllProfiles.Arrays.Guid"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int16 "Demo.CTT.AllProfiles.Arrays.Int16"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int32 "Demo.CTT.AllProfiles.Arrays.Int32"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Int64 "Demo.CTT.AllProfiles.Arrays.Int64"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_LocalizedText "Demo.CTT.AllProfiles.Arrays.LocalizedText"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_QualifiedName "Demo.CTT.AllProfiles.Arrays.QualifiedName"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_SByte "Demo.CTT.AllProfiles.Arrays.SByte"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_String "Demo.CTT.AllProfiles.Arrays.String"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt16 "Demo.CTT.AllProfiles.Arrays.UInt16"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt32 "Demo.CTT.AllProfiles.Arrays.UInt32"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_UInt64 "Demo.CTT.AllProfiles.Arrays.UInt64"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_Variant "Demo.CTT.AllProfiles.Arrays.Variant"
#define DemoId_Demo_CTT_Static_All_Profiles_Arrays_XmlElement "Demo.CTT.AllProfiles.Arrays.XmlElement"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Bool "Demo.CTT.AllProfiles.Scalar.Bool"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Byte "Demo.CTT.AllProfiles.Scalar.Byte"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_ByteString "Demo.CTT.AllProfiles.Scalar.ByteString"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_DateTime "Demo.CTT.AllProfiles.Scalar.DateTime"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Double "Demo.CTT.AllProfiles.Scalar.Double"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Duration "Demo.CTT.AllProfiles.Scalar.Duration"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Enumeration "Demo.CTT.AllProfiles.Scalar.Enumeration"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Float "Demo.CTT.AllProfiles.Scalar.Float"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Guid "Demo.CTT.AllProfiles.Scalar.Guid"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Image "Demo.CTT.AllProfiles.Scalar.Image"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageBMP "Demo.CTT.AllProfiles.Scalar.ImageBMP"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageGIF "Demo.CTT.AllProfiles.Scalar.ImageGIF"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImageJPG "Demo.CTT.AllProfiles.Scalar.ImageJPG"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_ImagePNG "Demo.CTT.AllProfiles.Scalar.ImagePNG"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Int16 "Demo.CTT.AllProfiles.Scalar.Int16"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Int32 "Demo.CTT.AllProfiles.Scalar.Int32"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Int64 "Demo.CTT.AllProfiles.Scalar.Int64"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Integer "Demo.CTT.AllProfiles.Scalar.Integer"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_LocalizedText "Demo.CTT.AllProfiles.Scalar.LocalizedText"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_NodeId "Demo.CTT.AllProfiles.Scalar.NodeId"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Number "Demo.CTT.AllProfiles.Scalar.Number"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_QualifiedName "Demo.CTT.AllProfiles.Scalar.QualifiedName"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_SByte "Demo.CTT.AllProfiles.Scalar.SByte"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_String "Demo.CTT.AllProfiles.Scalar.String"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInt16 "Demo.CTT.AllProfiles.Scalar.UInt16"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInt32 "Demo.CTT.AllProfiles.Scalar.UInt32"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInt64 "Demo.CTT.AllProfiles.Scalar.UInt64"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_UInteger "Demo.CTT.AllProfiles.Scalar.UInteger"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_Variant "Demo.CTT.AllProfiles.Scalar.Variant"
#define DemoId_Demo_CTT_Static_All_Profiles_Scalar_XmlElement "Demo.CTT.AllProfiles.Scalar.XmlElement"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Byte "Demo.CTT.DAProfile.AnalogType.Byte"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Byte_EURange 6102
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Double "Demo.CTT.DAProfile.AnalogType.Double"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Double_EURange 6086
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Float "Demo.CTT.DAProfile.AnalogType.Float"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Float_EURange 6088
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int16 "Demo.CTT.DAProfile.AnalogType.Int16"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int16_EURange 6090
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int32 "Demo.CTT.DAProfile.AnalogType.Int32"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int32_EURange 6094
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int64 "Demo.CTT.DAProfile.AnalogType.Int64"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Int64_EURange 6098
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_SByte "Demo.CTT.DAProfile.AnalogType.SByte"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_SByte_EURange 6104
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt16 "Demo.CTT.DAProfile.AnalogType.UInt16"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt16_EURange 6092
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt32 "Demo.CTT.DAProfile.AnalogType.UInt32"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt32_EURange 6096
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt64 "Demo.CTT.DAProfile.AnalogType.UInt64"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_UInt64_EURange 6100
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithEngineeringUnits "Demo.CTT.DAProfile.AnalogType.VariableWithEngineeringUnits"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithEngineeringUnits_EngineeringUnits 6109
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithEngineeringUnits_EURange 6106
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithInstrumentRange "Demo.CTT.DAProfile.AnalogType.VariableWithInstrumentRange"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithInstrumentRange_EURange 6108
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_VariableWithInstrumentRange_InstrumentRange 6110
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Double "Demo.CTT.DAProfile.AnalogTypeArrays.Double"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Double_EURange 6122
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Float "Demo.CTT.DAProfile.AnalogTypeArrays.Float"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Float_EURange 6120
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int16 "Demo.CTT.DAProfile.AnalogTypeArrays.Int16"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int16_EURange 6112
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int32 "Demo.CTT.DAProfile.AnalogTypeArrays.Int32"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_Int32_EURange 6114
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt16 "Demo.CTT.DAProfile.AnalogTypeArrays.UInt16"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt16_EURange 6116
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt32 "Demo.CTT.DAProfile.AnalogTypeArrays.UInt32"
#define DemoId_Demo_CTT_Static_DA_Profile_AnalogType_Arrays_UInt32_EURange 6118
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_Byte "Demo.CTT.DAProfile.DataItem.Byte"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_DateTime "Demo.CTT.DAProfile.DataItem.DateTime"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_Double "Demo.CTT.DAProfile.DataItem.Double"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_Float "Demo.CTT.DAProfile.DataItem.Float"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_Int16 "Demo.CTT.DAProfile.DataItem.Int16"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_Int32 "Demo.CTT.DAProfile.DataItem.Int32"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_Int64 "Demo.CTT.DAProfile.DataItem.Int64"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_SByte "Demo.CTT.DAProfile.DataItem.SByte"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_String "Demo.CTT.DAProfile.DataItem.String"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_UInt16 "Demo.CTT.DAProfile.DataItem.UInt16"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_UInt32 "Demo.CTT.DAProfile.DataItem.UInt32"
#define DemoId_Demo_CTT_Static_DA_Profile_DataItem_UInt64 "Demo.CTT.DAProfile.DataItem.UInt64"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete1 "Demo.CTT.DAProfile.DiscreteType.MultiStateDiscrete1"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete1_EnumStrings 6124
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete2 "Demo.CTT.DAProfile.DiscreteType.MultiStateDiscrete2"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete2_EnumStrings 6126
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete3 "Demo.CTT.DAProfile.DiscreteType.MultiStateDiscrete3"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete3_EnumStrings 6128
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete4 "Demo.CTT.DAProfile.DiscreteType.MultiStateDiscrete4"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete4_EnumStrings 6130
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete5 "Demo.CTT.DAProfile.DiscreteType.MultiStateDiscrete5"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_MultiStateDiscrete5_EnumStrings 6132
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete1 "Demo.CTT.DAProfile.DiscreteType.TwoStateDiscrete1"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete1_FalseState 6134
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete1_TrueState 6135
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete2 "Demo.CTT.DAProfile.DiscreteType.TwoStateDiscrete2"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete2_FalseState 6137
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete2_TrueState 6138
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete3 "Demo.CTT.DAProfile.DiscreteType.TwoStateDiscrete3"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete3_FalseState 6140
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete3_TrueState 6141
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete4 "Demo.CTT.DAProfile.DiscreteType.TwoStateDiscrete4"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete4_FalseState 6143
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete4_TrueState 6144
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete5 "Demo.CTT.DAProfile.DiscreteType.TwoStateDiscrete5"
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete5_FalseState 6146
#define DemoId_Demo_CTT_Static_DA_Profile_DiscreteType_TwoStateDiscrete5_TrueState 6147
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Byte "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Byte"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Byte_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Byte.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Byte_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Byte.ValueAsText"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int16 "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int16"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int16_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int16.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int16_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int16.ValueAsText"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int32 "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int32"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int32_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int32.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int32_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int32.ValueAsText"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int64 "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int64"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int64_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int64.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_Int64_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.Int64.ValueAsText"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_SByte "Demo.CTT.DAProfile.MultiStateValueDiscreteType.SByte"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_SByte_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.SByte.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_SByte_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.SByte.ValueAsText"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt16 "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt16"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt16_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt16.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt16_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt16.ValueAsText"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt32 "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt32"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt32_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt32.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt32_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt32.ValueAsText"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt64 "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt64"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt64_EnumValues "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt64.EnumValues"
#define DemoId_Demo_CTT_Static_DA_Profile_MultiStateValueDiscreteType_UInt64_ValueAsText "Demo.CTT.DAProfile.MultiStateValueDiscreteType.UInt64.ValueAsText"
#define DemoId_Demo_Icon_Icon "Demo.Icon.Icon"
#define DemoId_Demo_WorkOrder_WorkOrderVariable "Demo.WorkOrder.WorkOrderVariable"
#define DemoId_Demo_WorkOrder_WorkOrderVariable_AssetID "Demo.WorkOrder.WorkOrderVariable.AssetID"
#define DemoId_Demo_WorkOrder_WorkOrderVariable_ID "Demo.WorkOrder.WorkOrderVariable.ID"
#define DemoId_Demo_WorkOrder_WorkOrderVariable_StartTime "Demo.WorkOrder.WorkOrderVariable.StartTime"
#define DemoId_Demo_WorkOrder_WorkOrderVariable_StatusComments "Demo.WorkOrder.WorkOrderVariable.StatusComments"
#define DemoId_Demo_WorkOrder_WorkOrderVariable2 "Demo.WorkOrder.WorkOrderVariable2"
#define DemoId_Demo_WorkOrder_WorkOrderVariable2_AssetID "Demo.WorkOrder.WorkOrderVariable2.AssetID"
#define DemoId_Demo_WorkOrder_WorkOrderVariable2_ID "Demo.WorkOrder.WorkOrderVariable2.ID"
#define DemoId_Demo_WorkOrder_WorkOrderVariable2_StartTime "Demo.WorkOrder.WorkOrderVariable2.StartTime"
#define DemoId_Demo_WorkOrder_WorkOrderVariable2_StatusComments "Demo.WorkOrder.WorkOrderVariable2.StatusComments"
#define DemoId_Demo_SetSimulationSpeed_InputArguments 6249
#define DemoId_Demo_SimulationActive "Demo.SimulationActive"
#define DemoId_Demo_SimulationSpeed "Demo.SimulationSpeed"

/*============================================================================
* Method Identifiers
 *===========================================================================*/
#define DemoId_BoilerType_Fill 7002
#define DemoId_BoilerType_Heat 7001
#define DemoId_Demo_History_StartLogging "Demo.History.StartLogging"
#define DemoId_Demo_History_StopLogging "Demo.History.StopLogging"
#define DemoId_Demo_Method_DoSomethingAfter10s "Demo.Method.DoSomethingAfter10s"
#define DemoId_Demo_Method_EnumTest "Demo.Method.EnumTest"
#define DemoId_Demo_Method_Multiply "Demo.Method.Multiply"
#define DemoId_Demo_Method_VectorAdd "Demo.Method.VectorAdd"
#define DemoId_Demo_DynamicNodes_CreateDynamicNode "Demo.DynamicNodes.CreateDynamicNode"
#define DemoId_Demo_DynamicNodes_DeleteDynamicNode "Demo.DynamicNodes.DeleteDynamicNode"
#define DemoId_Demo_BoilerDemo_Boiler1_Fill "Demo.BoilerDemo.Boiler1.Fill"
#define DemoId_Demo_BoilerDemo_Boiler1_Heat "Demo.BoilerDemo.Boiler1.Heat"
#define DemoId_Demo_CTT_Methods_MethodIO "Demo.CTT.Methods.MethodIO"
#define DemoId_Demo_CTT_Methods_MethodNoArgs "Demo.Method.MethodNoArgs"
#define DemoId_Demo_CTT_NodeClasses_Method "Demo.CTT.NodeClasses.Method"
#define DemoId_Demo_CTT_References_Has3ForwardReferences2_Method1 7010
#define DemoId_Demo_CTT_References_Has3ForwardReferences2_Method2 7011
#define DemoId_Demo_CTT_References_Has3ForwardReferences2_Method3 7012
#define DemoId_Demo_CTT_References_Has3ForwardReferences5_ReferencedNode3 7013
#define DemoId_Demo_SetSimulationSpeed "Demo.SetSimulationSpeed"
#define DemoId_Demo_StartSimulation "Demo.StartSimulation"
#define DemoId_Demo_StopSimulation "Demo.StopSimulation"


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


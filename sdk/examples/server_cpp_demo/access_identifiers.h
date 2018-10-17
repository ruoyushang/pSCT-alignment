/******************************************************************************
** access_identifiers.h
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
** Project: C++ OPC Server SDK information model for namespace http://www.unifiedautomation.com/DemoServer/AccessPermission
**
** Description: OPC Unified Architecture Software Development Kit.
**
******************************************************************************/

#ifndef __ACCESS_IDENTIFIERS_H__
#define __ACCESS_IDENTIFIERS_H__

#define Access_NameSpaceUri "http://www.unifiedautomation.com/DemoServer/AccessPermission"

#include "version_coremodule.h"
#if (PROD_MAJOR != 1 || (PROD_MAJOR == 1 && PROD_MINOR < 5) || (PROD_MAJOR == 1 && PROD_MINOR == 5 && PROD_MINOR2 < 0))
#if ! (PROD_MAJOR == 1 && PROD_MINOR == 3 && PROD_MINOR2 == 0 && CPP_SDK_BUILD == 193)
#error The code is generated for SDK version 1.5.0
#endif
#endif

/*============================================================================
* ObjectType Identifiers
 *===========================================================================*/
#define AccessId_AccessPermissionObjectType 1006

/*============================================================================
* VariableType Identifiers
 *===========================================================================*/

/*============================================================================
* DataType Identifiers
 *===========================================================================*/

/*============================================================================
* ReferenceType Identifiers
 *===========================================================================*/

/*============================================================================
* Object Identifiers
 *===========================================================================*/
#define AccessId_AccessPermissionObjectType_LimitAlarm_All 5001
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_LimitState 5002
#define AccessId_AccessPermissionObjectType_LimitAlarm_John 5003
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_LimitState 5004
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators 5005
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LimitState 5006
#define AccessId_Server_Object_All "Demo.AccessRights.Access_All.Object_All"
#define AccessId_Server_Object_All_LimitAlarm_All "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All"
#define AccessId_Server_Object_All_LimitAlarm_All_LimitState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.LimitState"
#define AccessId_Server_Object_All_LimitAlarm_John "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John"
#define AccessId_Server_Object_All_LimitAlarm_John_LimitState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.LimitState"
#define AccessId_Server_Object_All_LimitAlarm_Operators "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators"
#define AccessId_Server_Object_All_LimitAlarm_Operators_LimitState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.LimitState"
#define AccessId_Server_Object_John "Demo.AccessRights.Access_All.Object_John"
#define AccessId_Server_Object_John_LimitAlarm_All "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All"
#define AccessId_Server_Object_John_LimitAlarm_All_LimitState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.LimitState"
#define AccessId_Server_Object_John_LimitAlarm_John "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John"
#define AccessId_Server_Object_John_LimitAlarm_John_LimitState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.LimitState"
#define AccessId_Server_Object_John_LimitAlarm_Operators "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators"
#define AccessId_Server_Object_John_LimitAlarm_Operators_LimitState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.LimitState"
#define AccessId_Server_Object_Operators "Demo.AccessRights.Access_All.Object_Operators"
#define AccessId_Server_Object_Operators_LimitAlarm_All "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All"
#define AccessId_Server_Object_Operators_LimitAlarm_All_LimitState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.LimitState"
#define AccessId_Server_Object_Operators_LimitAlarm_John "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John"
#define AccessId_Server_Object_Operators_LimitAlarm_John_LimitState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.LimitState"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_LimitState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.LimitState"
#define AccessId_Demo_AccessRights_Options "Demo.AccessRights.Options"

/*============================================================================
* Variable Identifiers
 *===========================================================================*/
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_AckedState 6004
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_AckedState_Id 6005
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Acknowledge_InputArguments 6006
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_ActiveState 6007
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_ActiveState_Id 6008
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_AddComment_InputArguments 6009
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_BranchId 6010
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_ClientUserId 6011
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Comment 6012
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Comment_SourceTimestamp 6013
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_ConditionClassId 6014
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_ConditionClassName 6015
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_ConditionName 6016
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_EnabledState 6017
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_EnabledState_Id 6018
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_EventId 6019
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_EventType 6020
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_InputNode 6021
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_LastSeverity 6022
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_LastSeverity_SourceTimestamp 6023
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_LimitState_CurrentState 6024
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_LimitState_CurrentState_Id 6025
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_LocalTime 6026
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Message 6027
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Quality 6028
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Quality_SourceTimestamp 6029
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_ReceiveTime 6030
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Retain 6031
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Severity 6032
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_SourceName 6033
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_SourceNode 6035
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_SuppressedOrShelved 6036
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Time 6037
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_AckedState 6038
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_AckedState_Id 6039
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Acknowledge_InputArguments 6040
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_ActiveState 6041
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_ActiveState_Id 6042
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_AddComment_InputArguments 6043
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_BranchId 6044
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_ClientUserId 6045
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Comment 6046
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Comment_SourceTimestamp 6047
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_ConditionClassId 6048
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_ConditionClassName 6050
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_ConditionName 6053
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_EnabledState 6054
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_EnabledState_Id 6055
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_EventId 6056
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_EventType 6057
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_InputNode 6058
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_LastSeverity 6059
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_LastSeverity_SourceTimestamp 6060
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_LimitState_CurrentState 6061
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_LimitState_CurrentState_Id 6062
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_LocalTime 6063
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Message 6064
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Quality 6065
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Quality_SourceTimestamp 6066
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_ReceiveTime 6067
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Retain 6068
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Severity 6069
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_SourceName 6070
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_SourceNode 6071
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_SuppressedOrShelved 6072
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Time 6073
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AckedState 6074
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AckedState_Id 6075
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Acknowledge_InputArguments 6076
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ActiveState 6077
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ActiveState_Id 6078
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AddComment_InputArguments 6079
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_BranchId 6080
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ClientUserId 6081
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Comment 6082
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Comment_SourceTimestamp 6083
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ConditionClassId 6084
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ConditionClassName 6085
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ConditionName 6086
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EnabledState 6087
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EnabledState_Id 6088
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EventId 6089
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_EventType 6090
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_InputNode 6091
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LastSeverity 6092
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LastSeverity_SourceTimestamp 6093
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LimitState_CurrentState 6094
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LimitState_CurrentState_Id 6095
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_LocalTime 6096
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Message 6097
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Quality 6098
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Quality_SourceTimestamp 6099
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_ReceiveTime 6100
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Retain 6101
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Severity 6102
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_SourceName 6103
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_SourceNode 6104
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_SuppressedOrShelved 6105
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Time 6106
#define AccessId_AccessPermissionObjectType_SetValue_All_InputArguments 6107
#define AccessId_AccessPermissionObjectType_SetValue_John_InputArguments 6108
#define AccessId_AccessPermissionObjectType_SetValue_Operators_InputArguments 6109
#define AccessId_AccessPermissionObjectType_Value 6110
#define AccessId_Demo_AccessRights_Access_All_All_RequireEncryption 6052
#define AccessId_Demo_AccessRights_Access_All_All_RequireSigning 6051
#define AccessId_Server_Object_All_LimitAlarm_All_AckedState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.AckedState"
#define AccessId_Server_Object_All_LimitAlarm_All_AckedState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.AckedState.Id"
#define AccessId_Server_Object_All_LimitAlarm_All_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Acknowledge.InputArguments"
#define AccessId_Server_Object_All_LimitAlarm_All_ActiveState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.ActiveState"
#define AccessId_Server_Object_All_LimitAlarm_All_ActiveState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.ActiveState.Id"
#define AccessId_Server_Object_All_LimitAlarm_All_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.AddComment.InputArguments"
#define AccessId_Server_Object_All_LimitAlarm_All_BranchId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.BranchId"
#define AccessId_Server_Object_All_LimitAlarm_All_ClientUserId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.ClientUserId"
#define AccessId_Server_Object_All_LimitAlarm_All_Comment "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Comment"
#define AccessId_Server_Object_All_LimitAlarm_All_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Comment.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_All_ConditionClassId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.ConditionClassId"
#define AccessId_Server_Object_All_LimitAlarm_All_ConditionClassName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.ConditionClassName"
#define AccessId_Server_Object_All_LimitAlarm_All_ConditionName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.ConditionName"
#define AccessId_Server_Object_All_LimitAlarm_All_EnabledState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.EnabledState"
#define AccessId_Server_Object_All_LimitAlarm_All_EnabledState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.EnabledState.Id"
#define AccessId_Server_Object_All_LimitAlarm_All_EventId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.EventId"
#define AccessId_Server_Object_All_LimitAlarm_All_EventType "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.EventType"
#define AccessId_Server_Object_All_LimitAlarm_All_InputNode "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.InputNode"
#define AccessId_Server_Object_All_LimitAlarm_All_LastSeverity "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.LastSeverity"
#define AccessId_Server_Object_All_LimitAlarm_All_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_All_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.LimitState.CurrentState"
#define AccessId_Server_Object_All_LimitAlarm_All_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.LimitState.CurrentState.Id"
#define AccessId_Server_Object_All_LimitAlarm_All_LocalTime "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.LocalTime"
#define AccessId_Server_Object_All_LimitAlarm_All_Message "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Message"
#define AccessId_Server_Object_All_LimitAlarm_All_Quality "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Quality"
#define AccessId_Server_Object_All_LimitAlarm_All_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Quality.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_All_ReceiveTime "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.ReceiveTime"
#define AccessId_Server_Object_All_LimitAlarm_All_Retain "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Retain"
#define AccessId_Server_Object_All_LimitAlarm_All_Severity "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Severity"
#define AccessId_Server_Object_All_LimitAlarm_All_SourceName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.SourceName"
#define AccessId_Server_Object_All_LimitAlarm_All_SourceNode "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.SourceNode"
#define AccessId_Server_Object_All_LimitAlarm_All_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.SuppressedOrShelved"
#define AccessId_Server_Object_All_LimitAlarm_All_Time "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Time"
#define AccessId_Server_Object_All_LimitAlarm_John_AckedState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.AckedState"
#define AccessId_Server_Object_All_LimitAlarm_John_AckedState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.AckedState.Id"
#define AccessId_Server_Object_All_LimitAlarm_John_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Acknowledge.InputArguments"
#define AccessId_Server_Object_All_LimitAlarm_John_ActiveState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.ActiveState"
#define AccessId_Server_Object_All_LimitAlarm_John_ActiveState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.ActiveState.Id"
#define AccessId_Server_Object_All_LimitAlarm_John_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.AddComment.InputArguments"
#define AccessId_Server_Object_All_LimitAlarm_John_BranchId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.BranchId"
#define AccessId_Server_Object_All_LimitAlarm_John_ClientUserId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.ClientUserId"
#define AccessId_Server_Object_All_LimitAlarm_John_Comment "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Comment"
#define AccessId_Server_Object_All_LimitAlarm_John_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Comment.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_John_ConditionClassId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.ConditionClassId"
#define AccessId_Server_Object_All_LimitAlarm_John_ConditionClassName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.ConditionClassName"
#define AccessId_Server_Object_All_LimitAlarm_John_ConditionName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.ConditionName"
#define AccessId_Server_Object_All_LimitAlarm_John_EnabledState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.EnabledState"
#define AccessId_Server_Object_All_LimitAlarm_John_EnabledState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.EnabledState.Id"
#define AccessId_Server_Object_All_LimitAlarm_John_EventId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.EventId"
#define AccessId_Server_Object_All_LimitAlarm_John_EventType "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.EventType"
#define AccessId_Server_Object_All_LimitAlarm_John_InputNode "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.InputNode"
#define AccessId_Server_Object_All_LimitAlarm_John_LastSeverity "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.LastSeverity"
#define AccessId_Server_Object_All_LimitAlarm_John_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_John_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.LimitState.CurrentState"
#define AccessId_Server_Object_All_LimitAlarm_John_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.LimitState.CurrentState.Id"
#define AccessId_Server_Object_All_LimitAlarm_John_LocalTime "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.LocalTime"
#define AccessId_Server_Object_All_LimitAlarm_John_Message "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Message"
#define AccessId_Server_Object_All_LimitAlarm_John_Quality "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Quality"
#define AccessId_Server_Object_All_LimitAlarm_John_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Quality.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_John_ReceiveTime "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.ReceiveTime"
#define AccessId_Server_Object_All_LimitAlarm_John_Retain "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Retain"
#define AccessId_Server_Object_All_LimitAlarm_John_Severity "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Severity"
#define AccessId_Server_Object_All_LimitAlarm_John_SourceName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.SourceName"
#define AccessId_Server_Object_All_LimitAlarm_John_SourceNode "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.SourceNode"
#define AccessId_Server_Object_All_LimitAlarm_John_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.SuppressedOrShelved"
#define AccessId_Server_Object_All_LimitAlarm_John_Time "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Time"
#define AccessId_Server_Object_All_LimitAlarm_Operators_AckedState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.AckedState"
#define AccessId_Server_Object_All_LimitAlarm_Operators_AckedState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.AckedState.Id"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Acknowledge.InputArguments"
#define AccessId_Server_Object_All_LimitAlarm_Operators_ActiveState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.ActiveState"
#define AccessId_Server_Object_All_LimitAlarm_Operators_ActiveState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.ActiveState.Id"
#define AccessId_Server_Object_All_LimitAlarm_Operators_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.AddComment.InputArguments"
#define AccessId_Server_Object_All_LimitAlarm_Operators_BranchId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.BranchId"
#define AccessId_Server_Object_All_LimitAlarm_Operators_ClientUserId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.ClientUserId"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Comment "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Comment"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Comment.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_Operators_ConditionClassId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.ConditionClassId"
#define AccessId_Server_Object_All_LimitAlarm_Operators_ConditionClassName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.ConditionClassName"
#define AccessId_Server_Object_All_LimitAlarm_Operators_ConditionName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.ConditionName"
#define AccessId_Server_Object_All_LimitAlarm_Operators_EnabledState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.EnabledState"
#define AccessId_Server_Object_All_LimitAlarm_Operators_EnabledState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.EnabledState.Id"
#define AccessId_Server_Object_All_LimitAlarm_Operators_EventId "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.EventId"
#define AccessId_Server_Object_All_LimitAlarm_Operators_EventType "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.EventType"
#define AccessId_Server_Object_All_LimitAlarm_Operators_InputNode "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.InputNode"
#define AccessId_Server_Object_All_LimitAlarm_Operators_LastSeverity "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.LastSeverity"
#define AccessId_Server_Object_All_LimitAlarm_Operators_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_Operators_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.LimitState.CurrentState"
#define AccessId_Server_Object_All_LimitAlarm_Operators_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.LimitState.CurrentState.Id"
#define AccessId_Server_Object_All_LimitAlarm_Operators_LocalTime "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.LocalTime"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Message "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Message"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Quality "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Quality"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Quality.SourceTimestamp"
#define AccessId_Server_Object_All_LimitAlarm_Operators_ReceiveTime "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.ReceiveTime"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Retain "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Retain"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Severity "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Severity"
#define AccessId_Server_Object_All_LimitAlarm_Operators_SourceName "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.SourceName"
#define AccessId_Server_Object_All_LimitAlarm_Operators_SourceNode "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.SourceNode"
#define AccessId_Server_Object_All_LimitAlarm_Operators_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.SuppressedOrShelved"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Time "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Time"
#define AccessId_Server_Object_All_SetValue_All_InputArguments "Demo.AccessRights.Access_All.Object_All.SetValue_All.InputArguments"
#define AccessId_Server_Object_All_SetValue_John_InputArguments "Demo.AccessRights.Access_All.Object_All.SetValue_John.InputArguments"
#define AccessId_Server_Object_All_SetValue_Operators_InputArguments "Demo.AccessRights.Access_All.Object_All.SetValue_Operators.InputArguments"
#define AccessId_Server_Object_All_Value "Demo.AccessRights.Access_All.Object_All.Value"
#define AccessId_Demo_AccessRights_Browse_All_John_ReadAttributes "Demo.AccessRights.Access_John.John_ReadAttributes"
#define AccessId_Server_Object_John_LimitAlarm_All_AckedState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.AckedState"
#define AccessId_Server_Object_John_LimitAlarm_All_AckedState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.AckedState.Id"
#define AccessId_Server_Object_John_LimitAlarm_All_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Acknowledge.InputArguments"
#define AccessId_Server_Object_John_LimitAlarm_All_ActiveState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.ActiveState"
#define AccessId_Server_Object_John_LimitAlarm_All_ActiveState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.ActiveState.Id"
#define AccessId_Server_Object_John_LimitAlarm_All_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.AddComment.InputArguments"
#define AccessId_Server_Object_John_LimitAlarm_All_BranchId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.BranchId"
#define AccessId_Server_Object_John_LimitAlarm_All_ClientUserId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.ClientUserId"
#define AccessId_Server_Object_John_LimitAlarm_All_Comment "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Comment"
#define AccessId_Server_Object_John_LimitAlarm_All_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Comment.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_All_ConditionClassId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.ConditionClassId"
#define AccessId_Server_Object_John_LimitAlarm_All_ConditionClassName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.ConditionClassName"
#define AccessId_Server_Object_John_LimitAlarm_All_ConditionName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.ConditionName"
#define AccessId_Server_Object_John_LimitAlarm_All_EnabledState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.EnabledState"
#define AccessId_Server_Object_John_LimitAlarm_All_EnabledState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.EnabledState.Id"
#define AccessId_Server_Object_John_LimitAlarm_All_EventId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.EventId"
#define AccessId_Server_Object_John_LimitAlarm_All_EventType "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.EventType"
#define AccessId_Server_Object_John_LimitAlarm_All_InputNode "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.InputNode"
#define AccessId_Server_Object_John_LimitAlarm_All_LastSeverity "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.LastSeverity"
#define AccessId_Server_Object_John_LimitAlarm_All_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_All_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.LimitState.CurrentState"
#define AccessId_Server_Object_John_LimitAlarm_All_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.LimitState.CurrentState.Id"
#define AccessId_Server_Object_John_LimitAlarm_All_LocalTime "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.LocalTime"
#define AccessId_Server_Object_John_LimitAlarm_All_Message "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Message"
#define AccessId_Server_Object_John_LimitAlarm_All_Quality "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Quality"
#define AccessId_Server_Object_John_LimitAlarm_All_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Quality.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_All_ReceiveTime "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.ReceiveTime"
#define AccessId_Server_Object_John_LimitAlarm_All_Retain "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Retain"
#define AccessId_Server_Object_John_LimitAlarm_All_Severity "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Severity"
#define AccessId_Server_Object_John_LimitAlarm_All_SourceName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.SourceName"
#define AccessId_Server_Object_John_LimitAlarm_All_SourceNode "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.SourceNode"
#define AccessId_Server_Object_John_LimitAlarm_All_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.SuppressedOrShelved"
#define AccessId_Server_Object_John_LimitAlarm_All_Time "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Time"
#define AccessId_Server_Object_John_LimitAlarm_John_AckedState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.AckedState"
#define AccessId_Server_Object_John_LimitAlarm_John_AckedState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.AckedState.Id"
#define AccessId_Server_Object_John_LimitAlarm_John_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Acknowledge.InputArguments"
#define AccessId_Server_Object_John_LimitAlarm_John_ActiveState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.ActiveState"
#define AccessId_Server_Object_John_LimitAlarm_John_ActiveState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.ActiveState.Id"
#define AccessId_Server_Object_John_LimitAlarm_John_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.AddComment.InputArguments"
#define AccessId_Server_Object_John_LimitAlarm_John_BranchId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.BranchId"
#define AccessId_Server_Object_John_LimitAlarm_John_ClientUserId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.ClientUserId"
#define AccessId_Server_Object_John_LimitAlarm_John_Comment "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Comment"
#define AccessId_Server_Object_John_LimitAlarm_John_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Comment.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_John_ConditionClassId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.ConditionClassId"
#define AccessId_Server_Object_John_LimitAlarm_John_ConditionClassName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.ConditionClassName"
#define AccessId_Server_Object_John_LimitAlarm_John_ConditionName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.ConditionName"
#define AccessId_Server_Object_John_LimitAlarm_John_EnabledState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.EnabledState"
#define AccessId_Server_Object_John_LimitAlarm_John_EnabledState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.EnabledState.Id"
#define AccessId_Server_Object_John_LimitAlarm_John_EventId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.EventId"
#define AccessId_Server_Object_John_LimitAlarm_John_EventType "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.EventType"
#define AccessId_Server_Object_John_LimitAlarm_John_InputNode "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.InputNode"
#define AccessId_Server_Object_John_LimitAlarm_John_LastSeverity "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.LastSeverity"
#define AccessId_Server_Object_John_LimitAlarm_John_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_John_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.LimitState.CurrentState"
#define AccessId_Server_Object_John_LimitAlarm_John_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.LimitState.CurrentState.Id"
#define AccessId_Server_Object_John_LimitAlarm_John_LocalTime "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.LocalTime"
#define AccessId_Server_Object_John_LimitAlarm_John_Message "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Message"
#define AccessId_Server_Object_John_LimitAlarm_John_Quality "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Quality"
#define AccessId_Server_Object_John_LimitAlarm_John_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Quality.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_John_ReceiveTime "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.ReceiveTime"
#define AccessId_Server_Object_John_LimitAlarm_John_Retain "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Retain"
#define AccessId_Server_Object_John_LimitAlarm_John_Severity "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Severity"
#define AccessId_Server_Object_John_LimitAlarm_John_SourceName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.SourceName"
#define AccessId_Server_Object_John_LimitAlarm_John_SourceNode "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.SourceNode"
#define AccessId_Server_Object_John_LimitAlarm_John_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.SuppressedOrShelved"
#define AccessId_Server_Object_John_LimitAlarm_John_Time "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Time"
#define AccessId_Server_Object_John_LimitAlarm_Operators_AckedState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.AckedState"
#define AccessId_Server_Object_John_LimitAlarm_Operators_AckedState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.AckedState.Id"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Acknowledge.InputArguments"
#define AccessId_Server_Object_John_LimitAlarm_Operators_ActiveState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.ActiveState"
#define AccessId_Server_Object_John_LimitAlarm_Operators_ActiveState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.ActiveState.Id"
#define AccessId_Server_Object_John_LimitAlarm_Operators_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.AddComment.InputArguments"
#define AccessId_Server_Object_John_LimitAlarm_Operators_BranchId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.BranchId"
#define AccessId_Server_Object_John_LimitAlarm_Operators_ClientUserId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.ClientUserId"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Comment "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Comment"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Comment.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_Operators_ConditionClassId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.ConditionClassId"
#define AccessId_Server_Object_John_LimitAlarm_Operators_ConditionClassName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.ConditionClassName"
#define AccessId_Server_Object_John_LimitAlarm_Operators_ConditionName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.ConditionName"
#define AccessId_Server_Object_John_LimitAlarm_Operators_EnabledState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.EnabledState"
#define AccessId_Server_Object_John_LimitAlarm_Operators_EnabledState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.EnabledState.Id"
#define AccessId_Server_Object_John_LimitAlarm_Operators_EventId "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.EventId"
#define AccessId_Server_Object_John_LimitAlarm_Operators_EventType "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.EventType"
#define AccessId_Server_Object_John_LimitAlarm_Operators_InputNode "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.InputNode"
#define AccessId_Server_Object_John_LimitAlarm_Operators_LastSeverity "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.LastSeverity"
#define AccessId_Server_Object_John_LimitAlarm_Operators_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_Operators_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.LimitState.CurrentState"
#define AccessId_Server_Object_John_LimitAlarm_Operators_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.LimitState.CurrentState.Id"
#define AccessId_Server_Object_John_LimitAlarm_Operators_LocalTime "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.LocalTime"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Message "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Message"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Quality "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Quality"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Quality.SourceTimestamp"
#define AccessId_Server_Object_John_LimitAlarm_Operators_ReceiveTime "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.ReceiveTime"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Retain "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Retain"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Severity "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Severity"
#define AccessId_Server_Object_John_LimitAlarm_Operators_SourceName "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.SourceName"
#define AccessId_Server_Object_John_LimitAlarm_Operators_SourceNode "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.SourceNode"
#define AccessId_Server_Object_John_LimitAlarm_Operators_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.SuppressedOrShelved"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Time "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Time"
#define AccessId_Server_Object_John_SetValue_All_InputArguments "Demo.AccessRights.Access_All.Object_John.SetValue_All.InputArguments"
#define AccessId_Server_Object_John_SetValue_John_InputArguments "Demo.AccessRights.Access_All.Object_John.SetValue_John.InputArguments"
#define AccessId_Server_Object_John_SetValue_Operators_InputArguments "Demo.AccessRights.Access_All.Object_John.SetValue_Operators.InputArguments"
#define AccessId_Server_Object_John_Value "Demo.AccessRights.Access_All.Object_John.Value"
#define AccessId_Server_Object_Operators_LimitAlarm_All_AckedState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.AckedState"
#define AccessId_Server_Object_Operators_LimitAlarm_All_AckedState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.AckedState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Acknowledge.InputArguments"
#define AccessId_Server_Object_Operators_LimitAlarm_All_ActiveState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.ActiveState"
#define AccessId_Server_Object_Operators_LimitAlarm_All_ActiveState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.ActiveState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_All_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.AddComment.InputArguments"
#define AccessId_Server_Object_Operators_LimitAlarm_All_BranchId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.BranchId"
#define AccessId_Server_Object_Operators_LimitAlarm_All_ClientUserId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.ClientUserId"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Comment "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Comment"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Comment.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_All_ConditionClassId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.ConditionClassId"
#define AccessId_Server_Object_Operators_LimitAlarm_All_ConditionClassName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.ConditionClassName"
#define AccessId_Server_Object_Operators_LimitAlarm_All_ConditionName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.ConditionName"
#define AccessId_Server_Object_Operators_LimitAlarm_All_EnabledState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.EnabledState"
#define AccessId_Server_Object_Operators_LimitAlarm_All_EnabledState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.EnabledState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_All_EventId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.EventId"
#define AccessId_Server_Object_Operators_LimitAlarm_All_EventType "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.EventType"
#define AccessId_Server_Object_Operators_LimitAlarm_All_InputNode "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.InputNode"
#define AccessId_Server_Object_Operators_LimitAlarm_All_LastSeverity "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.LastSeverity"
#define AccessId_Server_Object_Operators_LimitAlarm_All_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_All_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.LimitState.CurrentState"
#define AccessId_Server_Object_Operators_LimitAlarm_All_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.LimitState.CurrentState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_All_LocalTime "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.LocalTime"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Message "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Message"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Quality "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Quality"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Quality.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_All_ReceiveTime "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.ReceiveTime"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Retain "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Retain"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Severity "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Severity"
#define AccessId_Server_Object_Operators_LimitAlarm_All_SourceName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.SourceName"
#define AccessId_Server_Object_Operators_LimitAlarm_All_SourceNode "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.SourceNode"
#define AccessId_Server_Object_Operators_LimitAlarm_All_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.SuppressedOrShelved"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Time "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Time"
#define AccessId_Server_Object_Operators_LimitAlarm_John_AckedState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.AckedState"
#define AccessId_Server_Object_Operators_LimitAlarm_John_AckedState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.AckedState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Acknowledge.InputArguments"
#define AccessId_Server_Object_Operators_LimitAlarm_John_ActiveState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.ActiveState"
#define AccessId_Server_Object_Operators_LimitAlarm_John_ActiveState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.ActiveState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_John_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.AddComment.InputArguments"
#define AccessId_Server_Object_Operators_LimitAlarm_John_BranchId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.BranchId"
#define AccessId_Server_Object_Operators_LimitAlarm_John_ClientUserId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.ClientUserId"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Comment "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Comment"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Comment.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_John_ConditionClassId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.ConditionClassId"
#define AccessId_Server_Object_Operators_LimitAlarm_John_ConditionClassName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.ConditionClassName"
#define AccessId_Server_Object_Operators_LimitAlarm_John_ConditionName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.ConditionName"
#define AccessId_Server_Object_Operators_LimitAlarm_John_EnabledState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.EnabledState"
#define AccessId_Server_Object_Operators_LimitAlarm_John_EnabledState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.EnabledState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_John_EventId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.EventId"
#define AccessId_Server_Object_Operators_LimitAlarm_John_EventType "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.EventType"
#define AccessId_Server_Object_Operators_LimitAlarm_John_InputNode "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.InputNode"
#define AccessId_Server_Object_Operators_LimitAlarm_John_LastSeverity "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.LastSeverity"
#define AccessId_Server_Object_Operators_LimitAlarm_John_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_John_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.LimitState.CurrentState"
#define AccessId_Server_Object_Operators_LimitAlarm_John_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.LimitState.CurrentState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_John_LocalTime "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.LocalTime"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Message "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Message"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Quality "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Quality"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Quality.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_John_ReceiveTime "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.ReceiveTime"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Retain "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Retain"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Severity "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Severity"
#define AccessId_Server_Object_Operators_LimitAlarm_John_SourceName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.SourceName"
#define AccessId_Server_Object_Operators_LimitAlarm_John_SourceNode "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.SourceNode"
#define AccessId_Server_Object_Operators_LimitAlarm_John_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.SuppressedOrShelved"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Time "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Time"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_AckedState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.AckedState"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_AckedState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.AckedState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Acknowledge_InputArguments "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Acknowledge.InputArguments"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_ActiveState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.ActiveState"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_ActiveState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.ActiveState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_AddComment_InputArguments "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.AddComment.InputArguments"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_BranchId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.BranchId"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_ClientUserId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.ClientUserId"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Comment "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Comment"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Comment_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Comment.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_ConditionClassId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.ConditionClassId"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_ConditionClassName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.ConditionClassName"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_ConditionName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.ConditionName"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_EnabledState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.EnabledState"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_EnabledState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.EnabledState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_EventId "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.EventId"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_EventType "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.EventType"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_InputNode "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.InputNode"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_LastSeverity "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.LastSeverity"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_LastSeverity_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.LastSeverity.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_LimitState_CurrentState "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.LimitState.CurrentState"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_LimitState_CurrentState_Id "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.LimitState.CurrentState.Id"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_LocalTime "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.LocalTime"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Message "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Message"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Quality "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Quality"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Quality_SourceTimestamp "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Quality.SourceTimestamp"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_ReceiveTime "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.ReceiveTime"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Retain "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Retain"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Severity "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Severity"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_SourceName "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.SourceName"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_SourceNode "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.SourceNode"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_SuppressedOrShelved "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.SuppressedOrShelved"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Time "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Time"
#define AccessId_Server_Object_Operators_SetValue_All_InputArguments "Demo.AccessRights.Access_All.Object_Operators.SetValue_All.InputArguments"
#define AccessId_Server_Object_Operators_SetValue_John_InputArguments "Demo.AccessRights.Access_All.Object_Operators.SetValue_John.InputArguments"
#define AccessId_Server_Object_Operators_SetValue_Operators_InputArguments "Demo.AccessRights.Access_All.Object_Operators.SetValue_Operators.InputArguments"
#define AccessId_Server_Object_Operators_Value "Demo.AccessRights.Access_All.Object_Operators.Value"
#define AccessId_Demo_AccessRights_Browse_All_Operators_ReadAttributes 6355

/*============================================================================
* Method Identifiers
 *===========================================================================*/
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Acknowledge 7001
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_AddComment 7002
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Disable 7003
#define AccessId_AccessPermissionObjectType_LimitAlarm_All_Enable 7004
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Acknowledge 7005
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_AddComment 7006
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Disable 7007
#define AccessId_AccessPermissionObjectType_LimitAlarm_John_Enable 7008
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Acknowledge 7009
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_AddComment 7010
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Disable 7011
#define AccessId_AccessPermissionObjectType_LimitAlarm_Operators_Enable 7012
#define AccessId_AccessPermissionObjectType_SetValue_All 7013
#define AccessId_AccessPermissionObjectType_SetValue_John 7014
#define AccessId_AccessPermissionObjectType_SetValue_Operators 7015
#define AccessId_Server_Object_All_LimitAlarm_All_Acknowledge "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Acknowledge"
#define AccessId_Server_Object_All_LimitAlarm_All_AddComment "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.AddComment"
#define AccessId_Server_Object_All_LimitAlarm_All_Disable "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Disable"
#define AccessId_Server_Object_All_LimitAlarm_All_Enable "Demo.AccessRights.Access_All.Object_All.LimitAlarm_All.Enable"
#define AccessId_Server_Object_All_LimitAlarm_John_Acknowledge "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Acknowledge"
#define AccessId_Server_Object_All_LimitAlarm_John_AddComment "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.AddComment"
#define AccessId_Server_Object_All_LimitAlarm_John_Disable "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Disable"
#define AccessId_Server_Object_All_LimitAlarm_John_Enable "Demo.AccessRights.Access_All.Object_All.LimitAlarm_John.Enable"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Acknowledge "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Acknowledge"
#define AccessId_Server_Object_All_LimitAlarm_Operators_AddComment "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.AddComment"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Disable "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Disable"
#define AccessId_Server_Object_All_LimitAlarm_Operators_Enable "Demo.AccessRights.Access_All.Object_All.LimitAlarm_Operators.Enable"
#define AccessId_Server_Object_All_SetValue_All "Demo.AccessRights.Access_All.Object_All.SetValue_All"
#define AccessId_Server_Object_All_SetValue_John "Demo.AccessRights.Access_All.Object_All.SetValue_John"
#define AccessId_Server_Object_All_SetValue_Operators "Demo.AccessRights.Access_All.Object_All.SetValue_Operators"
#define AccessId_Server_Object_John_LimitAlarm_All_Acknowledge "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Acknowledge"
#define AccessId_Server_Object_John_LimitAlarm_All_AddComment "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.AddComment"
#define AccessId_Server_Object_John_LimitAlarm_All_Disable "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Disable"
#define AccessId_Server_Object_John_LimitAlarm_All_Enable "Demo.AccessRights.Access_All.Object_John.LimitAlarm_All.Enable"
#define AccessId_Server_Object_John_LimitAlarm_John_Acknowledge "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Acknowledge"
#define AccessId_Server_Object_John_LimitAlarm_John_AddComment "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.AddComment"
#define AccessId_Server_Object_John_LimitAlarm_John_Disable "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Disable"
#define AccessId_Server_Object_John_LimitAlarm_John_Enable "Demo.AccessRights.Access_All.Object_John.LimitAlarm_John.Enable"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Acknowledge "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Acknowledge"
#define AccessId_Server_Object_John_LimitAlarm_Operators_AddComment "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.AddComment"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Disable "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Disable"
#define AccessId_Server_Object_John_LimitAlarm_Operators_Enable "Demo.AccessRights.Access_All.Object_John.LimitAlarm_Operators.Enable"
#define AccessId_Server_Object_John_SetValue_All "Demo.AccessRights.Access_All.Object_John.SetValue_All"
#define AccessId_Server_Object_John_SetValue_John "Demo.AccessRights.Access_All.Object_John.SetValue_John"
#define AccessId_Server_Object_John_SetValue_Operators "Demo.AccessRights.Access_All.Object_John.SetValue_Operators"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Acknowledge "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Acknowledge"
#define AccessId_Server_Object_Operators_LimitAlarm_All_AddComment "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.AddComment"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Disable "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Disable"
#define AccessId_Server_Object_Operators_LimitAlarm_All_Enable "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_All.Enable"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Acknowledge "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Acknowledge"
#define AccessId_Server_Object_Operators_LimitAlarm_John_AddComment "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.AddComment"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Disable "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Disable"
#define AccessId_Server_Object_Operators_LimitAlarm_John_Enable "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_John.Enable"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Acknowledge "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Acknowledge"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_AddComment "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.AddComment"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Disable "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Disable"
#define AccessId_Server_Object_Operators_LimitAlarm_Operators_Enable "Demo.AccessRights.Access_All.Object_Operators.LimitAlarm_Operators.Enable"
#define AccessId_Server_Object_Operators_SetValue_All "Demo.AccessRights.Access_All.Object_Operators.SetValue_All"
#define AccessId_Server_Object_Operators_SetValue_John "Demo.AccessRights.Access_All.Object_Operators.SetValue_John"
#define AccessId_Server_Object_Operators_SetValue_Operators "Demo.AccessRights.Access_All.Object_Operators.SetValue_Operators"
#define AccessId_Demo_AccessRights_Options_AddAdvancedNodes "Demo.AccessRights.AddAdvancedNodes"


#ifdef _WIN32
#ifdef _CPP_ACCESS_BUILD_DLL
# define CPP_ACCESS_EXPORT __declspec(dllexport)
#elif defined _CPP_ACCESS_USE_DLL
# define CPP_ACCESS_EXPORT __declspec(dllimport)
#else
# define CPP_ACCESS_EXPORT
#endif
#else
# define CPP_ACCESS_EXPORT
#endif

///////////////////////////////////////////////
// Private Class Hiding Macros
#define ACCESS_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr); } \
    inline const Class##Private* d_func() const { return reinterpret_cast<const Class##Private *>(d_ptr); } \
    friend class Class##Private; \
    Class##Private *d_ptr;

#define ACCESS_DECLARE_PUBLIC(Class) \
    inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
    friend class Class;

#define ACCESS_D(Class) Class##Private * const d = d_func()
#define ACCESS_CD(Class) const Class##Private * const d = d_func()
#define ACCESS_Q(Class) Class * const q = q_func()

#endif // __ACCESS_IDENTIFIERS_H__


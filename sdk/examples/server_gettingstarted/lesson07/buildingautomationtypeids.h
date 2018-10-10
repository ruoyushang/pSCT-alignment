/******************************************************************************
** Copyright (C) 2006-2015 Unified Automation GmbH. All Rights Reserved.
** Web: http://www.unifiedautomation.com
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Project: Sample OPC server
**
** Description: Numeric Id definitions for building automation type nodes.
******************************************************************************/

/************************************************************
 Controller Type and its instance declaration
*************************************************************/
// Controller Type
#define Ba_ControllerType                                1000
// Instance declaration
#define Ba_ControllerType_State                          1001
#define Ba_ControllerType_Temperature                    1002
#define Ba_ControllerType_TemperatureSetPoint            1003
#define Ba_ControllerType_PowerConsumption               1004
#define Ba_ControllerType_Start                          1006
#define Ba_ControllerType_Stop                           1007
#define Ba_ControllerType_Temperature_EURange            1008
#define Ba_ControllerType_Temperature_EngineeringUnits   1009
#define Ba_ControllerType_TemperatureSetPoint_EURange    1010
#define Ba_ControllerType_TemperatureSetPoint_EngineeringUnits 1011
#define Ba_ControllerType_StateCondition                 1012
#define Ba_ControllerType_Temperature_HA_Configuration   1013
/************************************************************/

/************************************************************
 AirConditioner Controller Type and its instance declaration
*************************************************************/
// AirConditioner Controller Type
#define Ba_AirConditionerControllerType                  2000
// Instance declaration
#define Ba_AirConditionerControllerType_State            2001
#define Ba_AirConditionerControllerType_Humidity         2002
#define Ba_AirConditionerControllerType_HumiditySetpoint 2003
#define Ba_AirConditionerControllerType_StartWithSetpoint 2004
#define Ba_AirConditionerControllerType_StartWithSetpoint_In 2005
/************************************************************/

/************************************************************
 Furnace Controller Type and its instance declaration
*************************************************************/
// Furnace Controller Type
#define Ba_FurnaceControllerType                         3000
// Instance declaration
#define Ba_FurnaceControllerType_State                   3001
#define Ba_FurnaceControllerType_GasFlow                 3002
#define Ba_FurnaceControllerType_StartWithSetpoint       3003
#define Ba_FurnaceControllerType_StartWithSetpoint_In    3004
/************************************************************/

/************************************************************
 ControllerEventType and its event field properties
*************************************************************/
// ControllerEventType
#define Ba_ControllerEventType                           4000
// Event field properties
#define Ba_ControllerEventType_Temperature               4001
#define Ba_ControllerEventType_State                     4002
/************************************************************/

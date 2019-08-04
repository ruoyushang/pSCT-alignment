#ifndef __PASSERVERTYPEIDS_H__
#define __PASSERVERTYPEIDS_H__

// Numeric Id definitions for PAS Object types



/************************************************************
 Controller Type and its instance declaration
*************************************************************/
//----------------------------------------------------------//
// POSITIONER
#define GLOB_PositionerType                            0
// Instance declaration
#define GLOB_PositionerType_Initialize                 1
#define GLOB_PositionerType_Move                       2
#define GLOB_PositionerType_Stop                       3
#define GLOB_PositionerType_SetEnergy                  4
#define GLOB_PositionerType_isMoving                  11
#define GLOB_PositionerType_curAz                     12
#define GLOB_PositionerType_curEl                     13
#define GLOB_PositionerType_inAz                      14
#define GLOB_PositionerType_inEl                      15
#define GLOB_PositionerType_EnergyLevel               16
//----------------------------------------------------------//
// Mirror Type
#define PAS_MirrorType                               100
// Instance declaration
#define PAS_MirrorType_State                         101
#define PAS_MirrorType_x                             102
#define PAS_MirrorType_y                             103
#define PAS_MirrorType_z                             104
#define PAS_MirrorType_xRot                          105
#define PAS_MirrorType_yRot                          106
#define PAS_MirrorType_zRot                          107
#define PAS_MirrorType_SafetyRadius                  108
#define PAS_MirrorType_SelectedMPES                  109
#define PAS_MirrorType_SelectedPanels                110
#define PAS_MirrorType_SelectedEdges                 111
#define PAS_MirrorType_sysOffsetsMPES_x1             120
#define PAS_MirrorType_sysOffsetsMPES_y1             121
#define PAS_MirrorType_sysOffsetsMPES_x2             122
#define PAS_MirrorType_sysOffsetsMPES_y2             123
#define PAS_MirrorType_sysOffsetsMPES_x3             124
#define PAS_MirrorType_sysOffsetsMPES_y3             125
#define PAS_MirrorType_Position                      126
#define PAS_MirrorType_ErrorState                    127
#define PAS_MirrorType_ReadPosition                  130
#define PAS_MirrorType_SavePosition                  131
#define PAS_MirrorType_LoadPosition                  132
#define PAS_MirrorType_ReadSensors                   133
#define PAS_MirrorType_ReadSensorsParallel           134
#define PAS_MirrorType_MoveToCoords                  135
#define PAS_MirrorType_MoveDeltaCoords               136
#define PAS_MirrorType_AlignSequentialRecursive      137
#define PAS_MirrorType_AlignSector                   138
#define PAS_MirrorType_AlignRing                     139
#define PAS_MirrorType_SelectAll                     140
#define PAS_MirrorType_TestActuators                 141
#define PAS_MirrorType_TestSensors                   142
#define PAS_MirrorType_CheckStatus                   143
#define PAS_MirrorType_Stop                          144

//----------------------------------------------------------//
// Edge Type
#define PAS_EdgeType                                1000
// Instance declaration
#define PAS_EdgeType_State                          1001
#define PAS_EdgeType_Position                       1002
#define PAS_EdgeType_ErrorState                     1003
#define PAS_EdgeType_FindMatrix                     1011
#define PAS_EdgeType_Align                          1012
#define PAS_EdgeType_Read                           1013
#define PAS_EdgeType_Stop                           1014
//----------------------------------------------------------//
// MPES Type
#define PAS_MPESType                                1100
// Instance declaration
#define PAS_MPESType_State                          1101
#define PAS_MPESType_xCentroidAvg                   1102
#define PAS_MPESType_yCentroidAvg                   1103
#define PAS_MPESType_xCentroidSpotWidth             1104
#define PAS_MPESType_yCentroidSpotWidth             1105
#define PAS_MPESType_CleanedIntensity               1106
#define PAS_MPESType_xCentroidNominal               1107
#define PAS_MPESType_yCentroidNominal               1108
#define PAS_MPESType_Position                       1109
#define PAS_MPESType_Serial                         1110
#define PAS_MPESType_ErrorState                     1111
#define PAS_MPESType_Exposure                       1112
#define PAS_MPESType_Timestamp                      1113
#define PAS_MPESType_RawTimestamp                   1114
#define PAS_MPESType_TurnOn                         1151
#define PAS_MPESType_TurnOff                        1152
#define PAS_MPESType_Read                           1153
#define PAS_MPESType_SetExposure                    1154
#define PAS_MPESType_ClearError                     1155
#define PAS_MPESType_ClearAllErrors                 1156
#define PAS_MPESType_StateCondition                 1161
// Error variable declarations
#define PAS_MPESType_Error0                          1191
#define PAS_MPESType_Error1                          1192
#define PAS_MPESType_Error2                          1193
#define PAS_MPESType_Error3                          1194
#define PAS_MPESType_Error4                          1195
#define PAS_MPESType_Error5                          1196
#define PAS_MPESType_Error6                          1197
#define PAS_MPESType_Error7                          1198
//----------------------------------------------------------//
//
// Panel Type
#define PAS_PanelType                               2000
// Instance declaration
#define PAS_PanelType_State                         2001
#define PAS_PanelType_x                             2002
#define PAS_PanelType_y                             2003
#define PAS_PanelType_z                             2004
#define PAS_PanelType_xRot                          2005
#define PAS_PanelType_yRot                          2006
#define PAS_PanelType_zRot                          2007
#define PAS_PanelType_IntTemperature                2008
#define PAS_PanelType_ExtTemperature                2009
#define PAS_PanelType_SafetyRadius                  2010
#define PAS_PanelType_Position                      2011
#define PAS_PanelType_Serial                        2012
#define PAS_PanelType_ErrorState                    2013
#define PAS_PanelType_MoveDeltaLengths              2020
#define PAS_PanelType_MoveToLengths                 2021
#define PAS_PanelType_MoveDeltaCoords               2022
#define PAS_PanelType_MoveToCoords                  2023
#define PAS_PanelType_ReadPosition                  2024
#define PAS_PanelType_Stop                          2025
#define PAS_PanelType_FindHome                      2026
#define PAS_PanelType_ClearError                    2027
#define PAS_PanelType_ClearAllErrors                2028
#define PAS_PanelType_ClearActuatorErrors           2029
#define PAS_PanelType_ClearPlatformErrors           2030
#define PAS_PanelType_TurnOn                        2031
#define PAS_PanelType_TurnOff                       2032

// Error variable declarations
#define PAS_PanelType_Error0                          2800
#define PAS_PanelType_Error1                          2801
#define PAS_PanelType_Error2                          2802
#define PAS_PanelType_Error3                          2803
#define PAS_PanelType_Error4                          2804
#define PAS_PanelType_Error5                          2805
#define PAS_PanelType_Error6                          2806
#define PAS_PanelType_Error7                          2807
#define PAS_PanelType_Error8                          2808
#define PAS_PanelType_Error9                          2809
#define PAS_PanelType_Error10                         2810
#define PAS_PanelType_Error11                         2811
#define PAS_PanelType_Error12                         2812
#define PAS_PanelType_Error13                         2813
#define PAS_PanelType_Error14                         2814
//----------------------------------------------------------//
// Actuator Type
#define PAS_ACTType                                 2100
// Instance declaration
#define PAS_ACTType_State                           2101
#define PAS_ACTType_CurrentLength                   2102
#define PAS_ACTType_TargetLength                    2103
#define PAS_ACTType_DeltaLength                     2104
#define PAS_ACTType_Position                        2105
#define PAS_ACTType_Serial                          2106
#define PAS_ACTType_ErrorState                      2107
#define PAS_ACTType_TurnOn                          2111
#define PAS_ACTType_TurnOff                         2112
#define PAS_ACTType_MoveDeltaLength                 2113
#define PAS_ACTType_MoveToLength                    2114
#define PAS_ACTType_ForceRecover                    2115
#define PAS_ACTType_ClearError                      2116
#define PAS_ACTType_ClearAllErrors                  2117
#define PAS_ACTType_Stop                            2118
// Error variable declarations
#define PAS_ACTType_Error0                          2900
#define PAS_ACTType_Error1                          2901
#define PAS_ACTType_Error2                          2902
#define PAS_ACTType_Error3                          2903
#define PAS_ACTType_Error4                          2904
#define PAS_ACTType_Error5                          2905
#define PAS_ACTType_Error6                          2906
#define PAS_ACTType_Error7                          2907
#define PAS_ACTType_Error8                          2908
#define PAS_ACTType_Error9                          2909
#define PAS_ACTType_Error10                         2910
#define PAS_ACTType_Error11                         2911
#define PAS_ACTType_Error12                         2912
#define PAS_ACTType_Error13                         2913
//----------------------------------------------------------//
//
// Optical Table Type
#define PAS_OptTableType                            3000
// Instance declaration
#define PAS_OptTableType_State                      3001
#define PAS_OptTableType_FindMatrix                 3011
#define PAS_OptTableType_Align                      3012
#define PAS_OptTableType_StepAll                    3013
//----------------------------------------------------------//
//
// CCD Type
#define PAS_CCDType                                 3100
// Instance declaration
#define PAS_CCDType_State                           3101
#define PAS_CCDType_xFromLED                        3102
#define PAS_CCDType_yFromLED                        3103
#define PAS_CCDType_zFromLED                        3104
#define PAS_CCDType_psiFromLED                      3105
#define PAS_CCDType_thetaFromLED                    3106
#define PAS_CCDType_phiFromLED                      3107
#define PAS_CCDType_xNominal                        3108
#define PAS_CCDType_yNominal                        3109
#define PAS_CCDType_zNominal                        3110
#define PAS_CCDType_psiNominal                      3111
#define PAS_CCDType_thetaNominal                    3112
#define PAS_CCDType_phiNominal                      3113
#define PAS_CCDType_ErrorState                      3101
#define PAS_CCDType_Start                           3121
#define PAS_CCDType_Stop                            3122
#define PAS_CCDType_Read                            3123
//----------------------------------------------------------//
// PSD Type
#define PAS_PSDType                                 3200
// Instance declaration
#define PAS_PSDType_State                           3201
#define PAS_PSDType_x1                              3202
#define PAS_PSDType_y1                              3203
#define PAS_PSDType_x2                              3204
#define PAS_PSDType_y2                              3205
#define PAS_PSDType_dx1                             3206
#define PAS_PSDType_dy1                             3207
#define PAS_PSDType_dx2                             3208
#define PAS_PSDType_dy2                             3209
#define PAS_PSDType_Temp                            3210
#define PAS_PSDType_ErrorState                      3211
#define PAS_PSDType_Read                            3220
//----------------------------------------------------------//

/************************************************************
 MPESEventType and its event field properties
*************************************************************/
// MPESEventType
#define PAS_MPESEventType                           10000
// Event field properties
#define PAS_MPESEventType_State                     10001
#define PAS_MPESEventType_xCentroidAvg              10002
#define PAS_MPESEventType_yCentroidAvg              10003
#define PAS_MPESEventType_xCentroidSD               10004
#define PAS_MPESEventType_yCentroidSD               10005
#define PAS_MPESEventType_CleanedIntensity          10006
/************************************************************/

#endif // #ifndef __PASSERVERTYPEIDS_H__

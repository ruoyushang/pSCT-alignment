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
#define GLOB_PositionerType_Init                       1
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
#define PAS_MirrorType_ReadPos                       111
#define PAS_MirrorType_MoveTo_Coords                 112
#define PAS_MirrorType_ReadAlign                     113
#define PAS_MirrorType_Align                         114
#define PAS_MirrorType_AlignSector                   115
#define PAS_MirrorType_MoveSector                    116
#define PAS_MirrorType_GlobalAlign                   117
#define PAS_MirrorType_Stop                          120
#define PAS_MirrorType_curCoords_x                   121
#define PAS_MirrorType_curCoords_y                   122
#define PAS_MirrorType_curCoords_z                   123
#define PAS_MirrorType_curCoords_xRot                124
#define PAS_MirrorType_curCoords_yRot                125
#define PAS_MirrorType_curCoords_zRot                126
#define PAS_MirrorType_inCoords_x                    131
#define PAS_MirrorType_inCoords_y                    132
#define PAS_MirrorType_inCoords_z                    133
#define PAS_MirrorType_inCoords_xRot                 134
#define PAS_MirrorType_inCoords_yRot                 135
#define PAS_MirrorType_inCoords_zRot                 136
#define PAS_MirrorType_sysOffsetsMPES_x1             137
#define PAS_MirrorType_sysOffsetsMPES_y1             138
#define PAS_MirrorType_sysOffsetsMPES_x2             139
#define PAS_MirrorType_sysOffsetsMPES_y2             140
#define PAS_MirrorType_sysOffsetsMPES_x3             141
#define PAS_MirrorType_sysOffsetsMPES_y3             142
#define PAS_MirrorType_selectedPanels                151
#define PAS_MirrorType_selectedEdges                 152
#define PAS_MirrorType_selectedMPES                  153
#define PAS_MirrorType_Align_InEdge                  161
#define PAS_MirrorType_Align_InPanel                 162
#define PAS_MirrorType_AlignFrac                     163
//----------------------------------------------------------//
// Edge Type
#define PAS_EdgeType                                1000
// Instance declaration
#define PAS_EdgeType_State                          1001
#define PAS_EdgeType_StepSize                       1002
#define PAS_EdgeType_AlignFrac                      1003
#define PAS_EdgeType_FindMatrix                     1011
#define PAS_EdgeType_Align                          1012
#define PAS_EdgeType_Move                           1013
#define PAS_EdgeType_Read                           1014
#define PAS_EdgeType_Stop                           1020
#define PAS_EdgeType_Align_InPanel                  1021
// MPES Type
#define PAS_MPESType                                1100
// Instance declaration
#define PAS_MPESType_State                          1101
#define PAS_MPESType_xCentroidAvg                   1102
#define PAS_MPESType_yCentroidAvg                   1103
#define PAS_MPESType_xCentroidSD                    1104
#define PAS_MPESType_yCentroidSD                    1105
#define PAS_MPESType_CleanedIntensity               1106
#define PAS_MPESType_xCentroidNominal               1107
#define PAS_MPESType_yCentroidNominal               1108
#define PAS_MPESType_Start                          1111
#define PAS_MPESType_Stop                           1112
#define PAS_MPESType_Read                           1113
#define PAS_MPESType_SetExposure                    1114
#define PAS_MPESType_StateCondition                 1121
//----------------------------------------------------------//
//
// Panel Type
#define PAS_PanelType                               2000
// Instance declaration
#define PAS_PanelType_State                         2001
#define PAS_PanelType_StepAll                       2011
#define PAS_PanelType_StepAll_move                  2019
#define PAS_PanelType_MoveTo_Acts                   2012
#define PAS_PanelType_MoveTo_Coords                 2013
#define PAS_PanelType_Read                          2014
#define PAS_PanelType_Stop                          2020
#define PAS_PanelType_curCoords_x                   2021
#define PAS_PanelType_curCoords_y                   2022
#define PAS_PanelType_curCoords_z                   2023
#define PAS_PanelType_curCoords_xRot                2024
#define PAS_PanelType_curCoords_yRot                2025
#define PAS_PanelType_curCoords_zRot                2026
#define PAS_PanelType_inCoords_x                    2031
#define PAS_PanelType_inCoords_y                    2032
#define PAS_PanelType_inCoords_z                    2033
#define PAS_PanelType_inCoords_xRot                 2034
#define PAS_PanelType_inCoords_yRot                 2035
#define PAS_PanelType_inCoords_zRot                 2036
#define PAS_PanelType_IntTemperature                2041
#define PAS_PanelType_ExtTemperature                2042
//----------------------------------------------------------//
// Actuator Type
#define PAS_ACTType                                 2100
// Instance declaration
#define PAS_ACTType_State                           2101
#define PAS_ACTType_Steps                           2102
#define PAS_ACTType_curLength_mm                    2103
#define PAS_ACTType_inLength_mm                     2104
#define PAS_ACTType_Start                           2111
#define PAS_ACTType_Stop                            2112
#define PAS_ACTType_Step                            2113
//----------------------------------------------------------//
//
// Optical Table Type
#define PAS_OptTableType                            3000
// Instance declaration
#define PAS_OptTableType_State                      3001
#define PAS_OptTableType_StepSize                   3002
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

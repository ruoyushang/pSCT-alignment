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
#define PAS_MirrorType_MoveToCoords                  112
#define PAS_MirrorType_ReadAlign                     113
#define PAS_MirrorType_Align                         114
#define PAS_MirrorType_AlignSector                   115
#define PAS_MirrorType_SimulateAlignSector           116
#define PAS_MirrorType_GlobalAlign                   117
#define PAS_MirrorType_Stop                          120
#define PAS_MirrorType_curCoords_x                   121
#define PAS_MirrorType_curCoords_y                   122
#define PAS_MirrorType_curCoords_z                   123
#define PAS_MirrorType_curCoords_xRot                124
#define PAS_MirrorType_curCoords_yRot                125
#define PAS_MirrorType_curCoords_zRot                126
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
#define PAS_PanelType_x                             2002
#define PAS_PanelType_y                             2003
#define PAS_PanelType_z                             2004
#define PAS_PanelType_xRot                          2005
#define PAS_PanelType_yRot                          2006
#define PAS_PanelType_zRot                          2007
#define PAS_PanelType_IntTemperature                2008
#define PAS_PanelType_ExtTemperature                2009
#define PAS_PanelType_MoveDeltaLengths              2010
#define PAS_PanelType_MoveToLengths                 2011
#define PAS_PanelType_MoveToCoords                  2012
#define PAS_PanelType_ReadAll                       2013
#define PAS_PanelType_Stop                          2014
//----------------------------------------------------------//
// Actuator Type
#define PAS_ACTType                                 2100
// Instance declaration
#define PAS_ACTType_State                           2101
#define PAS_ACTType_CurrentLength                   2102
#define PAS_ACTType_TargetLength                    2103
#define PAS_ACTType_DeltaLength                     2104
#define PAS_ACTType_Start                           2111
#define PAS_ACTType_Stop                            2112
#define PAS_ACTType_MoveDeltaLength                 2113
#define PAS_ACTType_MoveToLength                    2114
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

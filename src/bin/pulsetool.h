/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
/* define generic way of addressing each object */
#define BASE_FRAME		0
#define CREATE_FRAME		1
#define SIMULATE_FRAME		2
#define FIRST_FRAME			BASE_FRAME
#define LAST_FRAME			SIMULATE_FRAME

#define LG_WIN			3	/* must be first of WIN's */
#define PLOT_WIN		4
#define SM_WIN_1		5
#define SM_WIN_2		6
#define SM_WIN_3		7
#define SM_WIN_4		8
#define SM_WIN_5		9
#define SM_WIN_6		10
#define FIRST_WINDOW		LG_WIN
#define NUM_WINDOWS		SM_WIN_6 - FIRST_WINDOW + 1

#define MAIN_BUTTON_PANEL		11
#define MAIN_PARAMETER_PANEL		12
#define CREATE_PANEL			13
#define CREATE_BUTTON_PANEL		14
#define SIMULATE_PANEL			15
#define SIMULATE_BUTTON_PANEL		16

#define MAIN_FILE_BUTTON		17
#define MAIN_HORIZ_BUTTON		18
#define MAIN_EXPAND_BUTTON		19
#define MAIN_SIMULATE_BUTTON		20
#define MAIN_CREATE_BUTTON		21
#define MAIN_ZERO_FILL_BUTTON		22
#define MAIN_PRINT_DONE_BUTTON		23
#define MAIN_SAVE_DONE_BUTTON		24
#define MAIN_PRINT_BUTTON		25
#define MAIN_SAVE_BUTTON		26
#define MAIN_HELP_BUTTON		27
#define MAIN_QUIT_BUTTON		28

#define CREATE_DONE_BUTTON		29
#define CREATE_PREVIEW_BUTTON		30
#define CREATE_EXECUTE_BUTTON		31
#define SIMULATE_DONE_BUTTON		32
#define SIMULATE_GO_BUTTON		33
#define SIMULATE_STEP_BUTTON		34
#define SIMULATE_DELAY_BUTTON		35
#define SIMULATE_3D_BUTTON		36
#define FIRST_BUTTON			MAIN_FILE_BUTTON
#define LAST_BUTTON			SIMULATE_3D_BUTTON

#define FIRST_CYCLE			37
#define MAIN_DISPLAY_CYCLE		37
#define MAIN_GRID_CYCLE			38
#define CREATE_STEPS_CYCLE		39
#define SIMULATE_FORMAT_CYCLE		40
#define SIMULATE_INIT_CYCLE		41
#define LAST_CYCLE			41

#define FIRST_TEXTFIELD			42
#define MAIN_PRINT_PANEL		42
#define MAIN_SAVE_PANEL			43
#define CREATE_NAME_PANEL		44
#define CREATE_ATTRIBUTE_PANEL_0	45
#define CREATE_ATTRIBUTE_PANEL_1	46
#define CREATE_ATTRIBUTE_PANEL_2	47
#define CREATE_ATTRIBUTE_PANEL_3	48
#define CREATE_ATTRIBUTE_PANEL_4	49
#define SIMULATE_TIME_PANEL		50
#define SIMULATE_DELAY_PANEL		51
#define SIMULATE_PARAMETER_PANEL_0	52
#define SIMULATE_PARAMETER_PANEL_1	53
#define SIMULATE_PARAMETER_PANEL_2	54
#define SIMULATE_PARAMETER_PANEL_3	55
#define SIMULATE_PARAMETER_PANEL_4	56
#define SIMULATE_PARAMETER_PANEL_5	57
#define SIMULATE_PARAMETER_PANEL_6	58
#define SIMULATE_PARAMETER_PANEL_7	59
#define SIMULATE_PARAMETER_PANEL_8	60
#define SIMULATE_PARAMETER_PANEL_9	61
#define PARAM_DIRECTORY_NAME		62
#define PARAM_FILE_NAME			63
#define PARAM_PULSE_LENGTH		64
#define PARAM_STEPS			65
#define PARAM_FOURIER_SIZE		66
#define PARAM_POWER_FACTOR		67
#define PARAM_VERTICAL_SCALE		68
#define PARAM_VERTICAL_REF		69
#define PARAM_INTEGRAL			70
#define CREATE_MENU			71
#define LAST_TEXTFIELD			71

#define CREATE_COMMENT_PANEL_1		72
#define CREATE_COMMENT_PANEL_2		73
#define NUM_OBJECTS			74

#define RED                1
#define WHITE              2
#define CYAN               8
#define ORANGE             9
#define YELLOW             10
#define PURPLE             11
#define AQUA               12
#define GREEN              13
#define BLUE               14

typedef unsigned long 	win_val;

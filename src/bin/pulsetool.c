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

/************************************************************************
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>
#include "pulsetool.h"

#ifndef AIX
#define TRUE (0 == 0)
#define FALSE !TRUE
#endif

#define MAXPATHL	   257

/* BEGIN PLOT DEFINITIONS */
#define PS		0
#define PS_PPMM		 11.81
#define PLOT_WIDTH	 250
#define PLOT_HEIGHT	 150
#define PLOT_XOFFSET	-11
#define PLOT_YOFFSET	-10

typedef struct {
	FILE *file;
	char print_dir[MAXPATHL];
	float ppmm;
	int width, height, x_offset, y_offset;
	int which;
	int (*draw_line)();
	int (*draw_segments)();
	int (*draw_point)();
	int (*draw_points)();
	int (*draw_string)();
	} ppar_struct;

static ppar_struct *plot_par;
int plot_flag = FALSE;
/* END PLOT DEFINITIONS */

#define COMMENTCHAR        '#'
#define SM_WIDTH           174        /* width of each small canvas */
#define SM_HEIGHT          110        /* height of each small canvas */
#define LG_WIDTH           1045       /* width of the large canvas */
#define LG_HEIGHT          550        /* height of the large canvas */
#define CREATE_WIDTH	   470
#define CREATE_HEIGHT	   230
#define SIMULATE_WIDTH	   475
#define SIMULATE_HEIGHT	   210
#define LEFT_MARGIN        150        /* left margin between large and plot canvas */
#define TOP_MARGIN         50         /* top margin between large and plot canvas */
#define BOTTOM_MARGIN      70         /* bottom margin between large and plot canvas */
#define WIDTH              3*LG_WIDTH/4        /* width of plot canvas */
#define HEIGHT             (LG_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN)
#define AMP                0
#define PHASE              1
#define FREQ               2
#define REAL               3
#define IMAG               4
#define FFT                5
#define FT_RE              6
#define FT_IM              7
#define INOVA_MAX_PULSE_STEPS    8192
#define MAX_PULSE_STEPS    (8192*32)
#define MAX_FFT_STEPS      (8192*32)
#define MAX_AMP            1023.0
#define GO                 0 
#define STEP               1 
#define SIM_DELAY          2 
#define PREVIEW            0 
#define EXECUTE            1 

#define BUTTON_GAP	2
#define BUTTON_YOFFSET	4
#define TEXT_GAP	8*BUTTON_GAP

#define NUM_SM		   6
#ifdef sparc
#define BLINK_TIME         10
#else
#define BLINK_TIME         1
#endif

int			char_width=0, char_height=0, char_ascent=0, char_descent=0;
extern int		row(),
			column(),
			color_display;
void                    start_pulsechild(),
                        frame_message(),
                        send_to_pulsechild(),

                        repaint_small_canvases(),
			repaint_large_canvas(),
			repaint_plot_canvas(),
			grid_cycle_proc(),
                        update_cursor_labels(),
			draw_line(),
			draw_point(),
			draw_points(),
			draw_segments(),
			draw_string(),
			draw_inverse_string(),
			clear_area(),

                        read_pattern_file(),     /* Calculation routines */

                        quit_proc(),             /* Notification procedures */
                        save_proc(),    
                        print_proc(),    
                        help_proc(),    
                        files_proc(),
    			do_files_proc(),
			base_menu_action_proc(),
                        select_large_state(),
			simulate_3d_proc(),
                        display_cycle_proc(),
                        horiz_notify_proc(),
                        expand_notify_proc(),
                        param_notify_proc(),
                        simulate_proc(),
                        create_proc(),
                        create_event_proc(),
                        create_menu_proc(),
                        create_cycle_proc(),
                        large_canvas_event_handler(),
                        plot_canvas_event_handler();

char	                pattern_name[80],
                        directory_name[256],
                        pulselength_array[20] = "5.000",
                        sim_param_array[12][16] = {"0.000", "0.000", "1.000", "2.000",
                            "-5.000", "5.000", "100", "0.0", "0", "16"},
                        steps_array[20],
			simulate_time_array[16] = "0",
			simulate_delay_array[16] = "0",
                        fourier_size_array[20],
                        power_factor_array[20],
                        integral_array[20],
                        vertical_scale_array[20] = "1.000",
                        vertical_ref_array[20] = "0.000",
                        units_list[2][8] = {"sec", "Hz"};

static char             plot_title[13][32] = {"AMPLITUDE", "PHASE", "FREQUENCY",
                          "REAL", "IMAGINARY", "FT (Amplitude)", "FT (Real)",
                          "FT (Imaginary)", "X component", "Y component",
                          "Z component", "XY component", "Phase"};

static char             xaxis_label[13][32] =
                            {"Time", "Time", "Time", "Time", "Time",
                             "Frequency", "Frequency", "Frequency", "Frequency",
                             "Frequency", "Frequency", "Frequency", "Frequency"};

#define			NUM_CREATE_MENU_STRINGS  8
char			*create_menu_strings[NUM_CREATE_MENU_STRINGS] =
				{"Square", "Sinc", "Gaussian", "Hermite 90",
				 "Hermite 180", "Sech/Tanh 180", "Tan 180",
				 "Sin/Cos 90"};

static char             simulate_frame_label[32] = "Bloch Simulator",
                        create_frame_label[32] = "Pulse Generation Utility";

struct comment          {
			    int     index;
                            char    str[100];
                        }  comments_array[100];

static double           pulse[5][MAX_PULSE_STEPS],
                        fourier[3][MAX_FFT_STEPS],
                        *pattern[] = {pulse[0], pulse[1], pulse[2], pulse[3], pulse[4],
                            fourier[0], fourier[1], fourier[2]},

                        bloch_array[5][MAX_PULSE_STEPS],
                        min_max[13][2] = {
                            {0.0, MAX_AMP}, {0.0, 0.0}, {0.0, 0.0}, {-MAX_AMP, MAX_AMP},
                            {-MAX_AMP, MAX_AMP}, {0.0, 1.0}, {-1.0, 1.0}, {-1.0, 1.0},
                            {-1.0, 1.0}, {-1.0, 1.0}, {-1.0, 1.0}, {0.0, 1.0},
                            {-180.0, 180.0} },
                        pulselength=0.005,
                        power_factor,
			integral[2],
                        pi,
                        Mx=0.0,
                        My=0.0,
                        Mz=1.0,
                        phase=0.0,
                        B1_max=2000.0,
                        b1_start=0.0,
                        b1_end=2500.0,
                        frequency=0.0,
                        freq_start=(-5000.0),
                        freq_end=5000.0,
                        sim_start=(-5000.0),
                        sim_end=5000.0,
                        vscale=1.0,
                        vref=0.0,
                        xaxis_begin_val[2],
                        xaxis_end_val[2];

static int              nsteps=0,
			fft_steps=0,
			max_steps=0,
			sim_steps=100,
			comment_counter,
			simulate_flag=0,
			clear_flag=1,
			fft_flag=0,
			BW_flag=0,
			flag_3d=0,
                        fft_index,
			pulse_index,
			index_inc=16,
                        sim_is_current=0,
                        menu_val,
                        lg_state=0,
                        plot_color=CYAN,
                        left_flag[2]={0,0},
                        right_flag[2]={0,0},
                        horiz_flag[2]={0,0},
                        left_pos[2],
                        right_pos[2],
                        delta_pos[2],
                        horiz_pos[2];

int	                tochild,
                        fromchild,
                        childpid,
                        interrupt;

int			lg_width, lg_height,
			sm_width, sm_height,
			pl_width, pl_height;

char			*font_name;

win_val			get_panel_value();
static void init_small_canvases();
static void init_main_button_panel();
static void init_large_canvas();
static void init_parameter_panel();
static void init_create_window();
static void init_simulate_window();
static void parse_pulsetool_cmd_line(int argc, char *argv[]);
static void pulse_gen(int mode);
static void zero_fill_notify_proc();
static void draw_cursors(int cursor_status, int cursor_position);
static void clear_canvas(int mode);
static void plot_large_canvas(int num);
static void plot_small_canvas(int num);
static void bloch_simulate(int mode);
static void find_min_max();
static void calc_real_imag_freq();
static void fft_calc();
static void polar_display(int button, int down, int up, int drag,
                          int x, int y, int source_identifier);
static void project_3d_point(double trig_values[], double x3d, double y3d, double z3d,
                             double *x2d, double *y2d, double *intensity);

/************************************************************************
*  Global Variable Definitions:
*************************************************************************
*  pattern:         8 columned array to hold pulse (5 columns) and
		    fourier (3 columns) data.  This division is made to
		    save memory, since zero-filling can result in a larger
		    required FFT space.
*  bloch_array:     5 columned array to hold results from a Bloch simulation.
*  min_max:         array of minimum and maximum values of each attribute
		    of pulse, FFT, and simulation data.

*  pulselength:     length of pulse, in seconds.
*  power_factor:    mean square amplitude of the pulse, normalized to one.
*  integral:        sum of [0] amp*sin(phase) or [1] amp*cos(phase) over
		    each pulse step.

*  Mx, My, Mz:      magnetization components for Bloch simulation.
*  phase:           constant phase shift added to each step in a simulation.
*  B1_max:          used in Bloch simulation when "sweeping" frequency.
   freq_start:     
   freq_end: 
*  frequency:       used in Bloch simulation when "sweeping" B1.
   b1_start:        
   b1_end:
*  sim_start:       used to designate start and end values of a Bloch
   sim_end:         simulation for plotting purposes.

*  xaxis_begin_val: start and end values for the large plot canvas.
   xaxis_end_val:   [0] for pulse or simulation; [1] for FFT plots.
*  vscale:          multiplier for the vertical scale in the large plot canvas.
*  vref:            vertical offset value for the large plot canvas.

*  nsteps:          number of steps in the pulse.
*  fft_steps:       number of points in the fourier transform (fourier number).
*  sim_steps:       number of points calculated for a Bloch simulation.
*  comment_counter: counter for comment lines read in with a pulse template file.

*  menu_val:        menu selection for pulse creation.
*  lg_state:        index to one of the 13 data sets which can be displayed
		    in the large graphics window.
*  plot_color:      color selection for the plot in the large canvas.

*  save_flag:       set to 1 if save button is selected, to 0 when done.
*  clear_flag:      1 clears main canvas before re-draw, 0 does not.
*  BW_flag:         set to 1 if "BW" is an argument to main(), resulting in
		    black and white operation on a color monitor.
*  fft_flag:        1 if an FFT data set is currently displayed in the large canvas.
*  fft_index:       base 2 log of fourier number; increments at zero-fill.
*  simulate_flag:   1 if simulation data is currently displayed, otherwise 0.
*  sim_is_current:  1 if simulation is consistent with all current parameters.
*  pulse_index:     current index into the pulse template, ranging from 0 to
                    nsteps.  Used only during Bloch simulation.
*  index_inc:       number of pulse steps to advance from current pulse_index
		    during simulation when the "Steps" button is selected.

*  left_flag        flag for active left cursor; [0] pulse & simulation, [1] FFT.
*  right_flag       flag for active right cursor; [0] pulse & simulation, [1] FFT.
*  horiz_flag       flag for active horiz cursor; [0] pulse & simulation, [1] FFT.

*  left_pos         pixel position of left cursor in plot_canvas.  [0] pulse
		    and simulation, [1] FFT.
*  right_pos        pixel position of right cursor in plot_canvas.
*  delta_pos        difference between left and right cursors, in pixels.
*  horiz_pos        pixel position of horizontal cursor in plot_canvas.
************************************************************************/

/*************************************************************************
*  main:  starts the window system, initializes all of the panels and
*  canvases, exec's the program pulsechild and establishes communications
*  to it.  Pulsechild handles all the funcions of files and directories,
*  and contains the information displayed when the "Help" button is
*  selected.
*************************************************************************/
main(argc, argv)
int  argc;
char *argv[];
{
    int w, h;

    if (access("/vnmr/psg/psgmain.cpp", 0) == 0)          /* VnmrS */
       max_steps = MAX_PULSE_STEPS;
    else
       max_steps = INOVA_MAX_PULSE_STEPS;
    init_window(argc,argv);
    pi = acos(-1.0);
    screen_dims(&w,&h);
    if (w < LG_WIDTH)
    {
      lg_width = 0.8*LG_WIDTH;
      lg_height = 0.8*LG_HEIGHT;
      sm_width = 0.8*SM_WIDTH;
      sm_height = 0.8*SM_HEIGHT;
      pl_width = 0.7*WIDTH;
      pl_height = 0.7*HEIGHT;
    }
    else
    {
      lg_width = LG_WIDTH;
      lg_height = LG_HEIGHT;
      sm_width = SM_WIDTH;
      sm_height = SM_HEIGHT;
      pl_width = WIDTH;
      pl_height = HEIGHT;
    }

    setup_font(&font_name);
    setup_colormap();
    create_frame(BASE_FRAME,NULL,"Pulsetool",20,40,1200,900);
    setup_icon();
    start_pulsechild(argc, argv);
#ifdef SOLARIS
    getcwd(directory_name, sizeof( directory_name ));
#else
    getwd(directory_name);
#endif
    init_small_canvases();
    init_main_button_panel();
    init_large_canvas();
    init_parameter_panel();
    init_simulate_window();
    init_create_window();
    plot_par = (ppar_struct *)malloc(sizeof(ppar_struct));
#ifdef SOLARIS
    getcwd(plot_par->print_dir, sizeof( plot_par->print_dir ));
#else
    getwd(plot_par->print_dir);
#endif
    parse_pulsetool_cmd_line(argc, argv);
    window_start_loop(BASE_FRAME);

    exit(0);
}

/****************************************************************************
*	INIT PROCS
****************************************************************************/

/*****************************************************************
*  Initialize the small canvases along the top of the window.
*****************************************************************/
static void init_small_canvases()
{
    create_canvas(SM_WIN_1,BASE_FRAME,0,0,sm_width,sm_height,
			repaint_small_canvases,select_large_state,TRUE);
    create_canvas(SM_WIN_2,BASE_FRAME,0,0,sm_width,sm_height,
			repaint_small_canvases,select_large_state,TRUE);
    set_win_right_of(SM_WIN_2,SM_WIN_1);
    create_canvas(SM_WIN_3,BASE_FRAME,0,0,sm_width,sm_height,
			repaint_small_canvases,select_large_state,TRUE);
    set_win_right_of(SM_WIN_3,SM_WIN_2);
    create_canvas(SM_WIN_4,BASE_FRAME,0,0,sm_width,sm_height,
			repaint_small_canvases,select_large_state,TRUE);
    set_win_right_of(SM_WIN_4,SM_WIN_3);
    create_canvas(SM_WIN_5,BASE_FRAME,0,0,sm_width,sm_height,
			repaint_small_canvases,select_large_state,TRUE);
    set_win_right_of(SM_WIN_5,SM_WIN_4);
    create_canvas(SM_WIN_6,BASE_FRAME,0,0,sm_width,sm_height,
			repaint_small_canvases,select_large_state,TRUE);
    set_win_right_of(SM_WIN_6,SM_WIN_5);
}

/*****************************************************************
*  Initialize the main control panel between small and large
*  canvases.  This panel holds the Files, Thresh/Scale,
*  Full/Expand, Simulate, Create, Zero fill, Help, and Quit
*  buttons, and the Grid On/Off and Display Pulse/Simulation
*  cycles.
*****************************************************************/
static void init_main_button_panel()
{
    char *strs[8];
    int tot_width=0, width1, width2;
    int object_width();

    create_panel(MAIN_BUTTON_PANEL,BASE_FRAME,0,0,lg_width,lg_height);
    set_win_below(MAIN_BUTTON_PANEL,SM_WIN_1);

    create_button(MAIN_FILE_BUTTON,MAIN_BUTTON_PANEL,BUTTON_GAP,BUTTON_YOFFSET,
			"Files",files_proc);
    tot_width = object_width(MAIN_FILE_BUTTON) + 2*BUTTON_GAP;
    create_button(MAIN_HORIZ_BUTTON,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET,
			"Thresh",horiz_notify_proc);
    tot_width += object_width(MAIN_HORIZ_BUTTON) + BUTTON_GAP;
    create_button(MAIN_EXPAND_BUTTON,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET,
			"Expand",expand_notify_proc);
    tot_width += object_width(MAIN_EXPAND_BUTTON) + BUTTON_GAP;
    set_panel_label(MAIN_EXPAND_BUTTON,"Full");
    create_button(MAIN_SIMULATE_BUTTON,MAIN_BUTTON_PANEL,tot_width,
			BUTTON_YOFFSET,"Simulation",simulate_proc);
    tot_width += object_width(MAIN_SIMULATE_BUTTON) + BUTTON_GAP;
    create_button(MAIN_CREATE_BUTTON,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET,
			"Create",NULL);
    attach_menu_to_button(MAIN_CREATE_BUTTON,CREATE_MENU,
				NUM_CREATE_MENU_STRINGS, create_menu_strings,
				create_menu_proc);
    tot_width += object_width(MAIN_CREATE_BUTTON) + BUTTON_GAP;
    create_button(MAIN_ZERO_FILL_BUTTON,MAIN_BUTTON_PANEL,tot_width,
			BUTTON_YOFFSET,"Zero Fill",zero_fill_notify_proc);
    tot_width += object_width(MAIN_ZERO_FILL_BUTTON) + BUTTON_GAP;
    hide_object(MAIN_ZERO_FILL_BUTTON);
    strs[0] = "Off";
    strs[1] = "On";

    create_cycle(MAIN_GRID_CYCLE,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET-3,
			"Grid:",2,strs,0,grid_cycle_proc);
    width1 = object_width(MAIN_GRID_CYCLE) + BUTTON_GAP;
    strs[0] = "Simulation";
    strs[1] = "Pulse";
    create_cycle(MAIN_DISPLAY_CYCLE,MAIN_BUTTON_PANEL,tot_width+width1,
			BUTTON_YOFFSET-3,"Display:",2,strs,0,
			display_cycle_proc);
    width1 += object_width(MAIN_DISPLAY_CYCLE) + BUTTON_GAP;
    create_panel_text(MAIN_PRINT_PANEL,MAIN_BUTTON_PANEL,tot_width,
			BUTTON_YOFFSET,"File:","PLT_pulse_amp.ps",18,print_proc);
    width2 = object_width(MAIN_PRINT_PANEL) + BUTTON_GAP;
    hide_object(MAIN_PRINT_PANEL);
    create_panel_text(MAIN_SAVE_PANEL,MAIN_BUTTON_PANEL,tot_width,
			BUTTON_YOFFSET,"File:","pulsedata",18,save_proc);
    hide_object(MAIN_SAVE_PANEL);
    create_button(MAIN_PRINT_DONE_BUTTON,MAIN_BUTTON_PANEL,tot_width+width2,
			BUTTON_YOFFSET,"Done",print_proc);
    hide_object(MAIN_PRINT_DONE_BUTTON);
    create_button(MAIN_SAVE_DONE_BUTTON,MAIN_BUTTON_PANEL,tot_width+width2,
			BUTTON_YOFFSET,"Done",save_proc);
    width2 += object_width(MAIN_SAVE_DONE_BUTTON) + BUTTON_GAP;
    hide_object(MAIN_SAVE_DONE_BUTTON);
    tot_width += ((width2 > width1) ? width2 : width1);
    create_button(MAIN_PRINT_BUTTON,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET,
			"Print",print_proc);
    tot_width += object_width(MAIN_PRINT_BUTTON) + BUTTON_GAP;
    create_button(MAIN_SAVE_BUTTON,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET,
			"Save",save_proc);
    tot_width += object_width(MAIN_SAVE_BUTTON) + BUTTON_GAP;
    create_button(MAIN_HELP_BUTTON,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET,
			"Help",help_proc);
    tot_width += object_width(MAIN_HELP_BUTTON) + BUTTON_GAP;
    create_button(MAIN_QUIT_BUTTON,MAIN_BUTTON_PANEL,tot_width,BUTTON_YOFFSET,
			"Quit",quit_proc);
    tot_width += object_width(MAIN_QUIT_BUTTON) + BUTTON_GAP;
    if (tot_width > lg_width)
      set_width(MAIN_BUTTON_PANEL,tot_width);
/*    set_height(MAIN_BUTTON_PANEL,row(MAIN_BUTTON_PANEL,3));*/
    fit_height(MAIN_BUTTON_PANEL);
}

/*****************************************************************
*  Initialize the two canvases which comprise the main graphics
*  window.  lg_canvas encompasses the entire main graphics area,
*  and is used for the plot title, axes, and labels.  plot_canvas
*  overlays the central portion of the main graphics area, and
*  is used to plot the selected data.
*****************************************************************/
static void init_large_canvas()
{
    create_canvas(LG_WIN,BASE_FRAME,0,0,lg_width,lg_height,
			repaint_large_canvas,large_canvas_event_handler,TRUE);
    set_win_below(LG_WIN,MAIN_BUTTON_PANEL);

    create_canvas(PLOT_WIN,LG_WIN,LEFT_MARGIN+1,TOP_MARGIN+1,
			pl_width+1,pl_height+1,
			repaint_plot_canvas,plot_canvas_event_handler,FALSE);
}


/*****************************************************************
*  Initialize the parameter panel at the bottom of the window.
*  This panel is home to the directory, file name, pulse length,
*  number of pulse steps, current fourier number, power factor,
*  pulse integral, vertical scale and vertical reference.
*****************************************************************/
static void init_parameter_panel()
{
    int tot_width, width1, width2;

    create_panel(MAIN_PARAMETER_PANEL,BASE_FRAME,0,0,lg_width,100);
    set_win_below(MAIN_PARAMETER_PANEL,LG_WIN);
    create_panel_text(PARAM_DIRECTORY_NAME,MAIN_PARAMETER_PANEL,BUTTON_GAP,
			row(MAIN_PARAMETER_PANEL,0),
			"Directory:",directory_name,30,files_proc);
    width1 = object_width(PARAM_DIRECTORY_NAME);
    create_panel_text(PARAM_FILE_NAME,MAIN_PARAMETER_PANEL,BUTTON_GAP,
			row(MAIN_PARAMETER_PANEL,4),
			"Pulse Name:",pattern_name,30,files_proc);
    width2 = object_width(PARAM_FILE_NAME);
    tot_width = ((width2 > width1) ? width2 : width1);
    create_panel_text(PARAM_PULSE_LENGTH,MAIN_PARAMETER_PANEL,BUTTON_GAP,
			row(MAIN_PARAMETER_PANEL,8),
			"pw(msec):",pulselength_array,20,param_notify_proc);
    width2 = object_width(PARAM_PULSE_LENGTH);
    tot_width = ((width2 > tot_width) ? width2 : tot_width) + TEXT_GAP +
							BUTTON_GAP;
    create_panel_text(PARAM_STEPS,MAIN_PARAMETER_PANEL,tot_width,
			row(MAIN_PARAMETER_PANEL,0),
			"No. Steps:",steps_array,8,param_notify_proc);
    width1 = object_width(PARAM_STEPS);
    create_panel_text(PARAM_FOURIER_SIZE,MAIN_PARAMETER_PANEL,tot_width,
			row(MAIN_PARAMETER_PANEL,4),
			"Fourier size:",fourier_size_array,8,param_notify_proc);
    width2 = object_width(PARAM_FOURIER_SIZE);
    width1 = ((width2 > width1) ? width2 : width1);
    create_panel_text(PARAM_POWER_FACTOR,MAIN_PARAMETER_PANEL,tot_width,
			row(MAIN_PARAMETER_PANEL,8),
			"Power factor:",power_factor_array,8,param_notify_proc);
    width2 = object_width(PARAM_POWER_FACTOR);
    tot_width += ((width2 > width1) ? width2 : width1) + TEXT_GAP;
    create_panel_text(PARAM_VERTICAL_SCALE,MAIN_PARAMETER_PANEL,tot_width,
			row(MAIN_PARAMETER_PANEL,0),
			"Vertical scale:",vertical_scale_array,20,
			param_notify_proc);
    width1 = object_width(PARAM_VERTICAL_SCALE);
    create_panel_text(PARAM_VERTICAL_REF,MAIN_PARAMETER_PANEL,tot_width,
			row(MAIN_PARAMETER_PANEL,4),
			"Vertical reference:",vertical_ref_array,20,
			param_notify_proc);
    width2 = object_width(PARAM_VERTICAL_REF);
    width1 = ((width2 > width1) ? width2 : width1);
    create_panel_text(PARAM_INTEGRAL,MAIN_PARAMETER_PANEL,tot_width,
			row(MAIN_PARAMETER_PANEL,8),
			"Integral:",integral_array,20,
			param_notify_proc);
    width2 = object_width(PARAM_INTEGRAL);
    width1 = ((width2 > width1) ? width2 : width1);
    tot_width += width1 + BUTTON_GAP;
    if (tot_width > lg_width)
      set_width(MAIN_PARAMETER_PANEL,tot_width);

    fit_height(MAIN_PARAMETER_PANEL);
}


/*****************************************************************
*  Initialize the pulse creation window.  
*****************************************************************/
static void init_create_window()
{
    char *strs[12];
    int row_inc=4, i;
    int width, tot_width;

    i=0;
    create_frame(CREATE_FRAME,BASE_FRAME,create_frame_label,
			50,210,CREATE_WIDTH,CREATE_HEIGHT);
    hide_object(CREATE_FRAME);
    create_panel(CREATE_PANEL,CREATE_FRAME,0,0,CREATE_WIDTH,CREATE_HEIGHT-30);
    create_panel_message(CREATE_COMMENT_PANEL_1,CREATE_PANEL,
			column(CREATE_PANEL,3),
			row(CREATE_PANEL,(i++)*row_inc),
			"Hi");
    create_panel_message(CREATE_COMMENT_PANEL_2,CREATE_PANEL,
			column(CREATE_PANEL,3),
			row(CREATE_PANEL,(i++)*row_inc),
			"Does this work?");
    create_panel_text(CREATE_NAME_PANEL,CREATE_PANEL,
			column(CREATE_PANEL,3),
			row(CREATE_PANEL,(i++)*row_inc),
			"File Name:","",40,NULL);
    strs[0] = "1"; strs[1] = "2";
    strs[2] = "4"; strs[3] = "8";
    strs[4] = "16"; strs[5] = "32";
    strs[6] = "64"; strs[7] = "128";
    strs[8] = "256"; strs[9] = "512";
    strs[10] = "1024"; strs[11] = "2048";
    create_cycle(CREATE_STEPS_CYCLE,CREATE_PANEL,
			column(CREATE_PANEL,6),
			row(CREATE_PANEL,(i++)*row_inc),
			"Number of Steps:",12,strs,8,create_cycle_proc);
    create_panel_text(CREATE_ATTRIBUTE_PANEL_0,CREATE_PANEL,
			column(CREATE_PANEL,6),
			row(CREATE_PANEL,(i++)*row_inc),
			"Attribute:","",32,NULL);
    create_panel_text(CREATE_ATTRIBUTE_PANEL_1,CREATE_PANEL,
			column(CREATE_PANEL,6),
			row(CREATE_PANEL,(i++)*row_inc),
			"Attribute:","",32,NULL);
    create_panel_text(CREATE_ATTRIBUTE_PANEL_2,CREATE_PANEL,
			column(CREATE_PANEL,6),
			row(CREATE_PANEL,(i++)*row_inc),
			"Attribute:","",32,NULL);
    create_panel_text(CREATE_ATTRIBUTE_PANEL_3,CREATE_PANEL,
			column(CREATE_PANEL,6),
			row(CREATE_PANEL,(i++)*row_inc),
			"Attribute:","",32,NULL);
    create_panel_text(CREATE_ATTRIBUTE_PANEL_4,CREATE_PANEL,
			column(CREATE_PANEL,6),
			row(CREATE_PANEL,(i++)*row_inc),
			"Attribute:","",32,NULL);
    set_width(CREATE_PANEL,column(CREATE_PANEL,65));
    set_height(CREATE_PANEL,row(CREATE_PANEL,i*row_inc));
    create_panel(CREATE_BUTTON_PANEL,CREATE_FRAME,0,0,CREATE_WIDTH,30);
    set_win_below(CREATE_BUTTON_PANEL,CREATE_PANEL);
    tot_width = BUTTON_GAP;
    create_button(CREATE_DONE_BUTTON,CREATE_BUTTON_PANEL,tot_width,
			row(CREATE_BUTTON_PANEL,0),
			"Done",create_proc);
    tot_width += object_width(CREATE_DONE_BUTTON) + BUTTON_GAP;
    create_button(CREATE_PREVIEW_BUTTON,CREATE_BUTTON_PANEL,tot_width,
			row(CREATE_BUTTON_PANEL,0),
			"Preview",create_proc);
    tot_width += object_width(CREATE_PREVIEW_BUTTON) + BUTTON_GAP;
    create_button(CREATE_EXECUTE_BUTTON,CREATE_BUTTON_PANEL,tot_width,
			row(CREATE_BUTTON_PANEL,0),
			"Execute",create_proc);
    set_width(CREATE_BUTTON_PANEL,column(CREATE_BUTTON_PANEL,65));
    set_height(CREATE_BUTTON_PANEL,row(CREATE_BUTTON_PANEL,4));
    fit_frame(CREATE_FRAME);
}


/*****************************************************************
*  Initialize the Bloch simulation window.
*****************************************************************/
static void init_simulate_window()
{
    char *strs[3];
    static char   parameter_label_string[10][32] = {"Mx:", "My:", "Mz:",
		      "B1max (KHz):", "Start Freq (KHz):", "Stop Freq (KHz):",
		      "Steps:", "Phase:", "Index:", "Step Inc:"};
    int row_inc=4, i;
    int tot_width, width1, width2;

    i=0;
    create_frame(SIMULATE_FRAME,BASE_FRAME,"Bloch Simulator",
			550,180,SIMULATE_WIDTH,SIMULATE_HEIGHT);
    hide_object(SIMULATE_FRAME);
    create_panel(SIMULATE_PANEL,SIMULATE_FRAME,0,0,SIMULATE_WIDTH,
			SIMULATE_HEIGHT-30);
    strs[0] = "Yes";  strs[1] = "No";
    create_cycle(SIMULATE_INIT_CYCLE,SIMULATE_PANEL, BUTTON_GAP,
			row(SIMULATE_PANEL,(i++)*row_inc),
			"Initialize:",2,strs,0,NULL);
    width1 = object_width(SIMULATE_INIT_CYCLE);
    create_panel_text(SIMULATE_PARAMETER_PANEL_0,SIMULATE_PANEL,BUTTON_GAP,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[0],
			sim_param_array[0],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_0);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_PARAMETER_PANEL_1,SIMULATE_PANEL,BUTTON_GAP,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[1],
			sim_param_array[1],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_1);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_PARAMETER_PANEL_2,SIMULATE_PANEL,BUTTON_GAP,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[2],
			sim_param_array[2],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_2);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_PARAMETER_PANEL_3,SIMULATE_PANEL,BUTTON_GAP,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[3],
			sim_param_array[3],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_3);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_PARAMETER_PANEL_4,SIMULATE_PANEL,BUTTON_GAP,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[4],
			sim_param_array[4],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_4);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_PARAMETER_PANEL_5,SIMULATE_PANEL,BUTTON_GAP,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[5],
			sim_param_array[5],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_5);
    width1 = (width2 > width1) ? width2 : width1;
    tot_width = width1 + BUTTON_GAP + TEXT_GAP;
    i = 0;
    create_panel_text(SIMULATE_PARAMETER_PANEL_6,SIMULATE_PANEL,tot_width,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[6],
			sim_param_array[6],
			10,NULL);
    width1 = object_width(SIMULATE_PARAMETER_PANEL_6);
    create_panel_text(SIMULATE_PARAMETER_PANEL_7,SIMULATE_PANEL,tot_width,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[7],
			sim_param_array[7],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_7);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_PARAMETER_PANEL_8,SIMULATE_PANEL,tot_width,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[8],
			sim_param_array[8],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_8);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_PARAMETER_PANEL_9,SIMULATE_PANEL,tot_width,
			row(SIMULATE_PANEL,(i++)*row_inc),
			parameter_label_string[9],
			sim_param_array[9],
			10,NULL);
    width2 = object_width(SIMULATE_PARAMETER_PANEL_9);
    width1 = (width2 > width1) ? width2 : width1;

    create_panel_text(SIMULATE_TIME_PANEL,SIMULATE_PANEL,tot_width,
			row(SIMULATE_PANEL,(i++)*row_inc),
			"Time:", simulate_time_array,10,NULL);
    width2 = object_width(SIMULATE_TIME_PANEL);
    width1 = (width2 > width1) ? width2 : width1;
    create_panel_text(SIMULATE_DELAY_PANEL,SIMULATE_PANEL,tot_width,
			row(SIMULATE_PANEL,(i++)*row_inc),
			"Delay (msec):", simulate_delay_array,10,NULL);
    width2 = object_width(SIMULATE_DELAY_PANEL);
    width1 = (width2 > width1) ? width2 : width1;
    tot_width += width1 + BUTTON_GAP;
    set_width(SIMULATE_PANEL,tot_width);
    set_height(SIMULATE_PANEL,row(SIMULATE_PANEL,7*row_inc));
    create_panel(SIMULATE_BUTTON_PANEL,SIMULATE_FRAME,0,0,tot_width,
			row(SIMULATE_PANEL,4));
    set_win_below(SIMULATE_BUTTON_PANEL,SIMULATE_PANEL);
    tot_width = BUTTON_GAP;
    create_button(SIMULATE_DONE_BUTTON,SIMULATE_BUTTON_PANEL,tot_width,
			row(SIMULATE_BUTTON_PANEL,0),
			"Done",simulate_proc);
    tot_width += object_width(SIMULATE_DONE_BUTTON) + BUTTON_GAP;
    create_button(SIMULATE_GO_BUTTON,SIMULATE_BUTTON_PANEL,tot_width,
			row(SIMULATE_BUTTON_PANEL,0),
			"Go",simulate_proc);
    tot_width += object_width(SIMULATE_GO_BUTTON) + BUTTON_GAP;
    create_button(SIMULATE_STEP_BUTTON,SIMULATE_BUTTON_PANEL,tot_width,
			row(SIMULATE_BUTTON_PANEL,0),
			"Step",simulate_proc);
    tot_width += object_width(SIMULATE_STEP_BUTTON) + BUTTON_GAP;
    create_button(SIMULATE_DELAY_BUTTON,SIMULATE_BUTTON_PANEL,tot_width,
			row(SIMULATE_BUTTON_PANEL,0),
			"Delay",simulate_proc);
    tot_width += object_width(SIMULATE_DELAY_BUTTON) + BUTTON_GAP;
    create_button(SIMULATE_3D_BUTTON,SIMULATE_BUTTON_PANEL,tot_width,
			row(SIMULATE_BUTTON_PANEL,0),
			"3D",simulate_3d_proc);
    tot_width += object_width(SIMULATE_3D_BUTTON) + BUTTON_GAP;
    strs[0] = "Freq"; strs[1] = "B1"; strs[2] = "Time";
    create_cycle(SIMULATE_FORMAT_CYCLE,SIMULATE_BUTTON_PANEL,tot_width,
			row(SIMULATE_BUTTON_PANEL,0)-4,
			"Sweep:",3,strs,0,simulate_proc);

    set_height(SIMULATE_BUTTON_PANEL,row(SIMULATE_BUTTON_PANEL,4));

    fit_frame(SIMULATE_FRAME);
}

static void parse_pulsetool_cmd_line(int argc, char *argv[])
{
    for (argv++, argc--; argc && *argv; argv++, argc--){
	if (strcmp(*argv, "-shape") == 0){ /* File name */
	    argv++; argc--;
	    if (argc && *argv){
		strncpy(pattern_name, *argv, sizeof(pattern_name));
		pattern_name[sizeof(pattern_name)-1] = '\0';
		set_panel_value(PARAM_FILE_NAME, pattern_name);
	    }
	}
    }
}

set_print_save_filenames()
{
    char name[MAXPATHL];

	/* reset print and save filenames, if necessary */
	strcpy(name,(char *)get_panel_value(MAIN_PRINT_PANEL));
	if (name[0] != '/')  {
	  strcpy(name,"PLT_");
	  if (flag_3d)  {
	    strcat(name,"sim_3d");
	    }
	  else  {
	    switch (lg_state)  {
		case 0 : strcat(name,"pulse_amp");  break;
		case 1 : strcat(name,"pulse_phase");  break;
		case 2 : strcat(name,"pulse_freq");  break;
		case 3 : strcat(name,"pulse_real");  break;
		case 4 : strcat(name,"pulse_imag");  break;
		case 5 : strcat(name,"ft_amp");  break;
		case 6 : strcat(name,"ft_real");  break;
		case 7 : strcat(name,"ft_imag");  break;
		case 8 : strcat(name,"sim_mx");  break;
		case 9 : strcat(name,"sim_my");  break;
		case 10 : strcat(name,"sim_mz");  break;
		case 11 : strcat(name,"sim_mxy");  break;
		case 12 : strcat(name,"sim_phase");  break;
		}
	    }
	  strcat(name,".ps");
	  set_panel_value(MAIN_PRINT_PANEL,name);
	  }
	strcpy(name,(char *)get_panel_value(MAIN_SAVE_PANEL));
	if (name[0] != '/')  {
	  strcpy(name,"SAVE_");
	  if (flag_3d)  {
	    strcat(name,"sim_3d");
	    }
	  else  {
	    switch (lg_state)  {
		case 0 : strcat(name,"pulse_amp");  break;
		case 1 : strcat(name,"pulse_phase");  break;
		case 2 : strcat(name,"pulse_freq");  break;
		case 3 : strcat(name,"pulse_real");  break;
		case 4 : strcat(name,"pulse_imag");  break;
		case 5 : strcat(name,"ft_amp");  break;
		case 6 : strcat(name,"ft_real");  break;
		case 7 : strcat(name,"ft_imag");  break;
		case 8 : strcat(name,"sim_mx");  break;
		case 9 : strcat(name,"sim_my");  break;
		case 10 : strcat(name,"sim_mz");  break;
		case 11 : strcat(name,"sim_mxy");  break;
		case 12 : strcat(name,"sim_phase");  break;
		}
	    }
	  strcat(name,".data");
	  set_panel_value(MAIN_SAVE_PANEL,name);
	  }
}

/*****************************************************************
*  Read the pattern data into the pattern array.  If steps is
*  passed as zero, then we are reading from the external file
*  found in the string pattern_name.  If steps is a non-zero
*  value, then we are processing data already loaded into the
*  "pattern" array by pulse_gen().
******************************************************************/
void read_pattern_file(steps)
int   steps;
{
    int     i,k;
    double  amplitude, phase, time_count;
    FILE    *infile_ptr, *fopen();

    nsteps = steps;
    power_factor = 0.0;
    integral[0] = 0.0;
    integral[1] = 0.0;
    /****
    * If nsteps is non-zero, we already have the data in pattern, so
    * we just need to calculate the power_factor and integral.
    ****/
    if (nsteps) {
        for (i=0; i<nsteps; i++) {
            power_factor += pattern[0][i]*pattern[0][i];
	    integral[0] += pattern[0][i]*sin(pi/180.0*pattern[1][i]);
	    integral[1] += pattern[0][i]*cos(pi/180.0*pattern[1][i]);
        }
    }
    /****
    * Otherwise, read in the pulse, determining nsteps, power_factor
    * and integral on the fly.
    ****/
    else {
        infile_ptr = fopen(pattern_name, "r");
        if (infile_ptr == NULL ) {
	    strcat(pattern_name, ".RF");
	    infile_ptr = fopen(pattern_name, "r");
	    if (infile_ptr == NULL ) {
		frame_message("File read error:  File not found.",
			      PARAM_FILE_NAME);
		clear_canvas(0);
		return;
	    }
        }
        comment_counter = 0;
	/****
	* Read in each line until EOF is reached.
	****/
        while ((fgets(comments_array[comment_counter].str, 99, infile_ptr)) != NULL) {
	    /****
	    * If the line starts with COMMENTCHAR, save it in comments_array,
	    * keep track of its position as the current value of nsteps,
	    * increment comment_counter, and go on to the next line.
	    ****/
            if (comments_array[comment_counter].str[0] == COMMENTCHAR) {
                comments_array[comment_counter].index = nsteps;
                ++comment_counter;
            }
	    /****
	    * Otherwise, attempt to read pulse data from the line.
	    ****/
            else {
		/****
		* If successful, put the data from this line in the pattern
		* array, update the values of power_factor and integral,
		* increment nsteps.
		****/
                if (sscanf(comments_array[comment_counter].str, "%lf %lf %lf",
                  &phase, &amplitude, &time_count) == 3) {
                    for (i=0; i<time_count; i++) {
                        pattern[0][nsteps] = amplitude;
                        pattern[1][nsteps] = phase;
                        power_factor += amplitude*amplitude;
	                integral[0] += amplitude*sin(pi/180.0*phase);
	                integral[1] += amplitude*cos(pi/180.0*phase);
                        ++nsteps;
                    }
                }
		/****
		* Otherwise, check to see if the entire line is blank.  If
		* not, we have a problem with the format of the data.  If this
		* is the case, inform the user and end the attempt to read data.
		****/
                else {
                    for (i=0; i<strlen(comments_array[comment_counter].str); i++) {
                        if (!isspace(comments_array[comment_counter].str[i])) {
                            frame_message(
                                "Read error:  Incorrect pattern format.", PARAM_FILE_NAME);
                            fclose(infile_ptr);
			    start_graphics(LG_WIN);
			    set_frame_label(BASE_FRAME,simulate_flag);
			    flush_graphics(LG_WIN);
                            clear_canvas(0);
                            return;
                        }
                    }
                }
            }
	    /****
	    * If the maximum number of steps is reached, stop.
	    ****/
            if (nsteps > max_steps) {
                frame_message("Read error:  Maximum number of steps exceeded.",
                    PARAM_FILE_NAME);
                fclose(infile_ptr);
		start_graphics(LG_WIN);
		set_frame_label(BASE_FRAME,simulate_flag);
		flush_graphics(LG_WIN);
                clear_canvas(0);
                return;
            }
        }
        fclose(infile_ptr);
    }

    /****
    * Reset all of the display variables.
    ****/
    lg_state = 0;
    fft_index = ceil(log((double)nsteps)/log(2.0));
    pulse_index = 0;
    fft_flag = 0;
    simulate_flag = 0;
    flag_3d = 0;
    sim_is_current = 0;
    left_flag[0] = 0;
    left_flag[1] = 0;
    right_flag[0] = 0;
    right_flag[1] = 0;
    horiz_flag[0] = 0;
    horiz_flag[1] = 0;
    delta_pos[0] = 0;
    delta_pos[1] = 0;
    horiz_pos[0] = -1;
    horiz_pos[1] = -1;
    vscale = 1.0;
    vref = 0.0;
    set_print_save_filenames();
    sprintf(vertical_scale_array, "%-.3f", vscale);
    set_panel_value(PARAM_VERTICAL_SCALE,vertical_scale_array);
    sprintf(vertical_ref_array, "%-.3f", vref);
    set_panel_value(PARAM_VERTICAL_REF,vertical_ref_array);

    sprintf(steps_array, "%-d", nsteps);
    set_panel_value(PARAM_STEPS,steps_array);

    /****
    * Normalize and display the power_factor and integral.
    ****/
    power_factor /= MAX_AMP*MAX_AMP*(double)nsteps;
    integral[0] /= MAX_AMP*(double)nsteps;
    integral[1] /= MAX_AMP*(double)nsteps;
    sprintf(power_factor_array, "%-.4f", power_factor);
    set_panel_value(PARAM_POWER_FACTOR,power_factor_array);
    sprintf(integral_array, "%-.4f, %-.4f", integral[0], integral[1]);
    set_panel_value(PARAM_INTEGRAL,integral_array);

    /****
    * Just in case the template file contained no information.
    ****/
    if (!nsteps) {
        frame_message("Pulse template error:  Time count sum is zero.", PARAM_FILE_NAME);
        start_graphics(LG_WIN);
	set_frame_label(BASE_FRAME,simulate_flag);
        flush_graphics(LG_WIN);
        clear_canvas(0);
        return;
    }
    /****
    * index_inc should not be greater than the number of steps.
    ****/
    if (index_inc > nsteps) {
        index_inc = nsteps;
        sprintf(sim_param_array[9], "%-d", index_inc);
        set_panel_value(SIMULATE_PARAMETER_PANEL_9,sim_param_array[9]);
    }
    /****
    * Set the start and end values of the plot; calculate real, imaginary
    * frequency, and FFT; find minima and maxima; plot the data in small
    * and large canvases.
    ****/
    xaxis_begin_val[0] = 0.0;
    xaxis_begin_val[1] = -(double)nsteps/(2.0*pulselength);
    xaxis_end_val[0] = pulselength;
    xaxis_end_val[1] = -xaxis_begin_val[1];
    calc_real_imag_freq();
    fft_calc();
    find_min_max();
    for (k=0;k<NUM_SM;k++)
      plot_small_canvas(k);
    plot_large_canvas(0);
    plot_large_canvas(1);
    set_panel_value(MAIN_DISPLAY_CYCLE, 1);
    hide_object(MAIN_ZERO_FILL_BUTTON);
    set_panel_label(MAIN_HORIZ_BUTTON,"Thresh");
    start_graphics(LG_WIN);
    set_frame_label(BASE_FRAME,simulate_flag);
    flush_graphics(LG_WIN);
}


/*****************************************************************
*  Save data in large canvas to an external file.       
*****************************************************************/
void do_save_proc(item)
int item;
{
    int     i;
    double  ft_freq;
    char    name[MAXPATHL], name0[MAXPATHL];
    FILE    *out_ptr, *fopen();

    static int  save_flag=0;

    if (nsteps == 0) {
	frame_message("Nothing to save.", MAIN_SAVE_BUTTON);
	return;
    }

    if (item == MAIN_SAVE_BUTTON  &&  !save_flag) {
	
	hide_object(MAIN_GRID_CYCLE);
	hide_object(MAIN_DISPLAY_CYCLE);
	hide_object(MAIN_PRINT_BUTTON);
	show_object(MAIN_SAVE_PANEL);
	show_object(MAIN_SAVE_DONE_BUTTON);
	save_flag = 1;
    }
    else if (item == MAIN_SAVE_BUTTON  && save_flag) {
	strcpy(name0, (char *)get_panel_value(MAIN_SAVE_PANEL));
	if (name0[0] != '/')  {
	  strcpy(name,plot_par->print_dir);
	  strcat(name,"/");
	  strcat(name,name0);
	  }
	else
	  strcpy(name,name0);
	if ((out_ptr = fopen (name, "w")) == NULL) {
	    frame_message("Error:  Unable to open file.", MAIN_SAVE_BUTTON);
	    return;
	}
	switch (lg_state) {
	    case 0:  case 1:  case 2:  case 3:  case 4:
		for (i=0; i<nsteps; i++) {
		    fprintf(out_ptr,"%12.6f      %12.6f\n",
		      pulselength*(double)i/(double)nsteps, pattern[lg_state][i]);
		}
		break;
    
	    case 5:  case 6:  case 7:
		ft_freq = (double)nsteps/(2.0*pulselength);
		for (i=0; i<=fft_steps; i++) {
		    fprintf(out_ptr,"%12.6f      %12.6f\n",
		      ft_freq*(-1.0 + 2.0*(double)i/(double)fft_steps),
		      pattern[lg_state][i]);
		}
		break;

	    case 8:  case 9:  case 10:  case 11:  case 12:
		for (i=0; i<sim_steps; i++) {
		    fprintf(out_ptr,"%12.6f      %12.6f\n",
		      sim_start + (sim_end - sim_start)*(double)i/(double)(sim_steps-1),
		      bloch_array[lg_state-8][i]);
		}
		break;
	}
	fclose(out_ptr);
    }
    else if (item == MAIN_SAVE_DONE_BUTTON) {
	hide_object(MAIN_SAVE_PANEL);
	hide_object(MAIN_SAVE_DONE_BUTTON);
	show_object(MAIN_GRID_CYCLE);
	show_object(MAIN_DISPLAY_CYCLE);
	show_object(MAIN_PRINT_BUTTON);
	save_flag = 0;
    }
}


/*****************************************************************
*  Print data in large canvas to an external file.       
*****************************************************************/
void do_print_proc(item)
int item;
{
    int     i;
    double  ft_freq;
    char    name[MAXPATHL],name0[MAXPATHL];
    FILE    *out_ptr, *fopen();

    static int  print_flag=FALSE;

    if (nsteps == 0) {
	frame_message("Nothing to print.", MAIN_PRINT_BUTTON);
	return;
    }

    if (item == MAIN_PRINT_BUTTON  &&  !print_flag) {
	hide_object(MAIN_GRID_CYCLE);
	hide_object(MAIN_DISPLAY_CYCLE);
	hide_object(MAIN_SAVE_BUTTON);
	show_object(MAIN_PRINT_PANEL);
	show_object(MAIN_PRINT_DONE_BUTTON);
	print_flag = TRUE;
    }
    else if (item == MAIN_PRINT_BUTTON  && print_flag) {
	strcpy(name0, (char *)get_panel_value(MAIN_PRINT_PANEL));
	if (name0[0] != '/')  {
	  strcpy(name,plot_par->print_dir);
	  strcat(name,"/");
	  strcat(name,name0);
	  }
	else
	  strcpy(name,name0);
	if ((plot_par->file = fopen (name, "w")) == NULL) {
	    frame_message("Error:  Unable to open file.", MAIN_PRINT_BUTTON);
	    return;
	}
	plot_flag = TRUE;
	init_plot_par(plot_par,PS);
	start_plot(plot_par);
	if (flag_3d)
	  polar_display(0,0,0,0,0,0,0);
	else  {
	  plot_large_canvas(0);
	  plot_large_canvas(1);
	}
	end_plot(plot_par);
	plot_flag = FALSE;
	fclose(plot_par->file);
    }
    else if (item == MAIN_PRINT_DONE_BUTTON) {
	hide_object(MAIN_PRINT_PANEL);
	hide_object(MAIN_PRINT_DONE_BUTTON);
	show_object(MAIN_GRID_CYCLE);
	show_object(MAIN_DISPLAY_CYCLE);
	show_object(MAIN_SAVE_BUTTON);
	print_flag = 0;
    }
}

/*****************************************************************
*  Called when the "Thresh"/"Scale" button is selected.
*****************************************************************/
void do_horiz_notify_proc()
{
    /****
    * If the horizontal cursor is inactive, turn it on and change
    * the label on the button to "Scale".
    ****/
    if (!horiz_flag[fft_flag]) {
	set_panel_label(MAIN_HORIZ_BUTTON,"Scale");
	if (horiz_pos[fft_flag] > 0)  {
            draw_cursors(3, horiz_pos[fft_flag]);
	    }
	else  {
            draw_cursors(3, pl_height/3);
	    }
    }
    /****
    * Otherwise, turn off the horizontal cursor and change the label back.
    ****/
    else {
        draw_cursors(-3, 0);
	set_panel_label(MAIN_HORIZ_BUTTON,"Thresh");
    }
}


/*****************************************************************
*  Called when the Expand/Full button is selected.
*****************************************************************/
void do_expand_notify_proc()
{
    double   left_val, right_val, points, ft_freq;
    int      start_point, end_point;

    /****
    * If the pattern consists of only a single point, don't bother
    * with expansion or full display.
    ****/
    if (nsteps < 2)
	return;

    /****
    * Clear the screen before a "Full" or "Expand" operation.
    ****/
    clear_flag = 1;

    /****
    * If both left and right cursors are active, must be an expansion:
    ****/
    if (left_flag[fft_flag] && right_flag[fft_flag]) {
	left_val = xaxis_begin_val[fft_flag] + (double)(left_pos[fft_flag])
	    *(xaxis_end_val[fft_flag] - xaxis_begin_val[fft_flag])/(double)pl_width;
	right_val = xaxis_begin_val[fft_flag] + (double)(right_pos[fft_flag])
	    *(xaxis_end_val[fft_flag] - xaxis_begin_val[fft_flag])/(double)pl_width;

	switch (lg_state) {
	  /****
	  * Amp, phase, frequency, real, and imaginary cases.
	  ****/
	  case 0:  case 1:  case 2:  case 3:  case 4:
	      points = (double)(nsteps - 1);
	      start_point = rint(floor(left_val/pulselength*points));
	      end_point = rint(ceil(right_val/pulselength*points));
	      xaxis_begin_val[0] = (double)start_point/points*pulselength;
	      xaxis_end_val[0] = (double)end_point/points*pulselength;
	      break;

	  /****
	  * Amp, real, and imaginary FFT cases.
	  ****/
	  case 5:  case 6:  case 7:
	      points = (double)(fft_steps);
	      ft_freq = (double)nsteps/(2.0*pulselength);
	      start_point = (int)floor(points*(left_val+ft_freq)/(2.0*ft_freq));
	      end_point = (int)(ceil(points*(right_val+ft_freq)/(2.0*ft_freq)));
	      xaxis_begin_val[1] = (double)start_point/points*2.0*ft_freq - ft_freq;
	      xaxis_end_val[1] = (double)end_point/points*2.0*ft_freq - ft_freq;
	      break;

	  /****
	  * Simulation cases.
	  ****/
	  case 8:  case 9:  case 10:  case 11:  case 12:
	      points = (double)(sim_steps - 1);
	      start_point = floor(points*(left_val - sim_start)/(sim_end - sim_start));
	      end_point = (ceil(points*(right_val - sim_start)/(sim_end - sim_start)));
	      xaxis_begin_val[0] = (double)start_point/points
		  *(sim_end - sim_start) + sim_start;
	      xaxis_end_val[0] = (double)end_point/points
		  *(sim_end - sim_start) + sim_start;
	      break;
	}
    }
    /****
    * Otherwise, set the upper and lower limits for full display.
    ****/
    else {
	switch (fft_flag) {
	    case  0:
		if (simulate_flag) {
		    xaxis_begin_val[0] = sim_start;
		    xaxis_end_val[0] = sim_end;
		}
		else {
		    xaxis_begin_val[0] = 0.0;
		    xaxis_end_val[0] = pulselength;
		}
		break;  
	    case  1:
		ft_freq = (double)nsteps/(2.0*pulselength);
		xaxis_begin_val[1] = -ft_freq;
		xaxis_end_val[1] = ft_freq;
		break;  
	}
    }
    /****
    * Turn off the cursors, and display the data.
    ****/
    left_flag[fft_flag] = 0;
    right_flag[fft_flag] = 0;
    delta_pos[fft_flag] = 0;
    plot_large_canvas(0);
    plot_large_canvas(1);
    
    set_panel_label(MAIN_EXPAND_BUTTON,"Full");
}


/*****************************************************************
*  This routine is called when the mouse button is released
*  after selecting the "Create" button.  The menu item selected
*  is used to determine what set of pulse parameters to display
*  in the create window, which is subsequently activated.
*  
*****************************************************************/
void do_create_menu_proc(str)
char *str;
{
    int    i, count, length;
    static char  *comment[][2] = {
	{"Hard square pulse.",
	 ""},
	{"Sinc:  sin(x)/x amplitude modulation.",
	 "Reference:  R.J. Sutherland, J. Phys EB, 11, 217, 1978."},
	{"Gaussian:  e(-A*x**2) amplitude modulation.",
	 "Reference:  C. Bauer, J. Magn. Reson., 58, 442, 1984."},
	{"Hermite 90:  (1-A*x**2)*e(-x**2) amplitude modulation.",
	 "Reference:  W.S. Warren, J. Chem. Phys. 81, 1984"},
	{"Hermite 180:  (1-A*x**2)*e(-x**2) amplitude modulation.",
	 "Reference:  W.S. Warren, J. Chem. Phys. 81, 1984."},
	{"Hsech 180:  Adiabatic inversion, sech/tanh modulation.",
	 "Reference:  M.S. Silver, J. Magn. Reson., 59, 347, 1984."},
	{"Tan 180:  Adiabatic inversion, constant/tan modulation.",
	 "Reference:  C.J. Hardy, J. Magn. Reson., 66, 470, 1986."},
	{"Sincos 90:  Adiabatic 90, sin/cos modulation.",
	 "Reference:  M.R. Bendall, J. Magn. Reson., 67, 376, 1986."},
    };

    static char  *attribute_label[][6] = {
	{"0"},
	{"1", "Number of side lobes: "},
	{"1", "Amplitude threshold: "},
	{"1", "A: "},
	{"1", "A: "},
	{"3", "Pulse length (msec): ", "Amplitude threshold: ", "Bandwidth (kHz): "},
	{"3", "Pulse length (msec): ", "B1 minimum (kHz): ", "Tangent cutoff: "},
	{"2", "Pulse length (msec): ", "Frequency sweep maximum (kHz): "},
	{"4", "Pulse length (msec): ", "Frequency sweep maximum (kHz): ",
	 "Tip angle (deg): ", "Tangent cutoff: "}
    };

    static char  *attribute_value[][6] = {
	{""},
	{"2"},
	{"0.01"},
	{"0.667"},
	{"0.956"},
	{"5.0", "0.01", "4.0"},
	{"5.0", "1.0", "50.0"},
	{"5.0", "2.0"},
	{"5.0", "50.0", "90.0", "10.0"}
    };


    i=0;
    while ((strcmp(str,create_menu_strings[i]) != 0) &&
		(i < NUM_CREATE_MENU_STRINGS))
      {i++;}
    menu_val = i;

	if (menu_val == NUM_CREATE_MENU_STRINGS) {
	    return;
	}
	/****
	* Otherwise, update the two comment lines to reflect the
	* selected pulse type.
	****/
 	set_panel_label(CREATE_COMMENT_PANEL_1,comment[menu_val][0]);
 	set_panel_label(CREATE_COMMENT_PANEL_2,comment[menu_val][1]);

	/****
	* The first field in attribute_label gives the number of 
	* parameters for the pulse type.
	****/
	sscanf(attribute_label[menu_val][0], "%d", &count);
	/****
	* Now update each create_attribute_item with the new parameters;
	* de-activate any lines which are not needed to display all
	* the parameters.
	****/
	i = 0;
	if (i < count)  {
 	  set_panel_label(CREATE_ATTRIBUTE_PANEL_0,attribute_label[menu_val][i+1]);
	  set_panel_value(CREATE_ATTRIBUTE_PANEL_0,
			attribute_value[menu_val][i]);
	  show_object(CREATE_ATTRIBUTE_PANEL_0);
	  }
	else {
	  hide_object(CREATE_ATTRIBUTE_PANEL_0);
	  }
	i++;
	if (i < count)  {
 	  set_panel_label(CREATE_ATTRIBUTE_PANEL_1,attribute_label[menu_val][i+1]);
	  set_panel_value(CREATE_ATTRIBUTE_PANEL_1,
			attribute_value[menu_val][i]);
	  show_object(CREATE_ATTRIBUTE_PANEL_1);
	  }
	else {
	  hide_object(CREATE_ATTRIBUTE_PANEL_1);
	  }
	i++;
	if (i < count)  {
 	  set_panel_label(CREATE_ATTRIBUTE_PANEL_2,attribute_label[menu_val][i+1]);
	  set_panel_value(CREATE_ATTRIBUTE_PANEL_2,
			attribute_value[menu_val][i]);
	  show_object(CREATE_ATTRIBUTE_PANEL_2);
	  }
	else {
	  hide_object(CREATE_ATTRIBUTE_PANEL_2);
	  }
	i++;
	if (i < count)  {
 	  set_panel_label(CREATE_ATTRIBUTE_PANEL_3,attribute_label[menu_val][i+1]);
	  set_panel_value(CREATE_ATTRIBUTE_PANEL_3,
			attribute_value[menu_val][i]);
	  show_object(CREATE_ATTRIBUTE_PANEL_3);
	  }
	else {
	  hide_object(CREATE_ATTRIBUTE_PANEL_3);
	  }
	i++;
	if (i < count)  {
 	  set_panel_label(CREATE_ATTRIBUTE_PANEL_4,attribute_label[menu_val][i+1]);
	  set_panel_value(CREATE_ATTRIBUTE_PANEL_4,
			attribute_value[menu_val][i]);
	  show_object(CREATE_ATTRIBUTE_PANEL_4);
	  }
	else {
	  hide_object(CREATE_ATTRIBUTE_PANEL_4);
	  }
	/****
	* A square pulse needs only 1 step;  all other pulse default
	* to 256 steps.
	* A square pulse needs to be > 1 or it doesn't get drawn and can't
	* be used for simulation - (MER 9-4-92)
	****/
	if (menu_val == 0)
	  set_panel_value(CREATE_STEPS_CYCLE,8);
	else
	  set_panel_value(CREATE_STEPS_CYCLE,8);

	/****
	* Fit the window to the length of the comment lines.
	****/
	show_object(CREATE_FRAME);
}

/*****************************************************************************
* Routine for the Done, Preview, or
* Execute buttons of the create panel.
******************************************************************************/
void do_create_proc(item)
int    item;
{
    if (item == CREATE_DONE_BUTTON) {
	hide_object(CREATE_FRAME);
    }
    else if (item == CREATE_PREVIEW_BUTTON) {
	pulse_gen(PREVIEW);
    }
    else if (item == CREATE_EXECUTE_BUTTON) {
	pulse_gen(EXECUTE);
    }
}


/*********************************************************************
*  This routine is called by create proc, in either the PREVIEW or
*  EXECUTE mode.  The first creates the selected pulse, using the
*  attribute values read from the create window, and loads it
*  directly into the pattern array.  The second first creates the
*  pulse, then writes the data to an external file, defined in the
*  create window.
*********************************************************************/
static void pulse_gen(int mode)
{
    int     i = (int) get_panel_value(CREATE_STEPS_CYCLE);
    int     j, steps, return_val, confirm_val;
    char    name[64];
    double  *amp, *phase;
    double  A, var1, var2, var3, lobes, length, coshx, mu, tip_angle;
    double  theta_max, alpha, thresh, bw, time, b1min, fmax, cutoff; 
    double  constant;
    FILE    *out_ptr, *fopen();

    struct stat  statbuf;

    steps = (int)pow(2.0, (double)i);
    /****
    * Create the temporary amp and phase arrays to hold the new pulse.
    ****/
    amp = (double *)malloc(steps*sizeof(double));
    phase = (double *)malloc(steps*sizeof(double));

    /****
    * Calculate the selected pulse.
    ****/
    switch (menu_val+1) {
	case  1:  /* Square */
	    for (i=0; i<steps; i++) {
		amp[i] = MAX_AMP;
		phase[i] =  0.0;
	    }
	    break;

	case  2:  /* Sinc */
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_0),"%lf",&lobes);
	    for (i=(-steps/2), j=0; i<steps/2; i++, j++) {
		var1 = (double)i + 0.5;
		var2 = var1*2.0*(lobes+1.0)*pi/(double)(steps-1);
		var3 = sin(var2)/var2;
		amp[j] = MAX_AMP*fabs(var3);
		phase[j] = (var3 >= 0.0) ? 0.0 : 180.0;
	    }
	    break;

	case  3:  /* Gaussian */
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_0),"%lf",&A);
	    A = -(log(A))/((double)((steps-1)*(steps-1))/4.0);
	    for (i=0; i<steps; i++) {
		var1 = (double)(i - steps/2) + 0.5;
		var1 *= var1;
		amp[i] = MAX_AMP*exp(-A*var1);
		phase[i] = 0.0;
	    }
	    break;

	case  4:  /* Hermite 90 */
	case  5:  /* Hermite 180 */
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_0),"%lf",&A);
	    var1 = 0.0;
	    var2 = 0.1;
	    while (var1 >= -0.01) {
		var3 = (double)(steps*steps/4)/(var2*var2);
		var1 = (1.0 - A*var3)*exp(-var3);
		var2 += 0.1;
	    }
	    for (i=(-steps/2), j=0; i<steps/2; i++, j++) {
		var1 = ((double)i + 0.5)/var2;
		var3 = MAX_AMP*(1.0 - A*var1*var1)*exp(-var1*var1);
		phase[j] = (var3 >= 0.0) ? 0.0 : 180.0;
		amp[j] = fabs(var3);
	    }
	    break;

	case  6:  /* Sech/Tanh 180 */
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_0),"%lf",&length);
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_1),"%lf",&thresh);
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_2),"%lf",&bw);
	    length /= 1000.0;
	    bw *= 2.0*pi*1000.0;
	    var1 = (log(1.0/thresh + sqrt(1.0/(thresh*thresh) - 1.0)))/(length/2.0);
	    mu = bw/(2.0*var1);
	    for (i=0; i<steps; i++) {
		time = -length/2.0 + (double)i*length/(double)(steps - 1);
		coshx = (exp(var1*time) + exp(-var1*time))/2.0;
		amp[i] = MAX_AMP/coshx;
		phase[i] = 180.0/pi*mu*log(coshx);
	    }
	    break;

	case  7:  /* Tan swept 180 */
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_0),"%lf",&length);
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_1),"%lf",&b1min);
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_2),"%lf",&cutoff);
	    b1min *= 2.0*pi*1000.0;
	    length /= 1000.0;
	    alpha = 2.0/(length)*atan(cutoff);
	    for (i=0; i<steps; i++) {
		time = -length/2.0 + (double)i*length/(double)(steps - 1);
		amp[i] = MAX_AMP;
		phase[i] = -180.0/pi*b1min/alpha*log(cos(alpha*time));
	    }
	    break;

	case  8:  /* Sin/Cos 90 */
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_0),"%lf",&length);
	    sscanf((char *)get_panel_value(CREATE_ATTRIBUTE_PANEL_1),"%lf",&fmax);
	    length /= 1000.0;
	    fmax *= 1000.0;
	    constant = 4.0*fmax*length*180.0/pi;
	    for (i=0; i<steps; i++) {
		time = (double)i*length/(double)(steps - 1);
		amp[i] = MAX_AMP*sin(pi/2.0*time/length);
		phase[i] = 180.0/pi*4.0*fmax*length*sin(pi/2.0*time/length) + constant;
	    }
	    break;

	default:
	    break;
   }

    if (mode == PREVIEW) {
	for (i=0; i<steps; i++) {
	    pattern[0][i] = amp[i];
	    pattern[1][i] = phase[i];
	}
	strcpy(pattern_name, "");
	set_panel_value(PARAM_FILE_NAME,pattern_name);
	read_pattern_file(steps);
    }
    else if (mode == EXECUTE) {
	strcpy(name, (char *)get_panel_value(CREATE_NAME_PANEL));
	if (!strcmp(name, "")) {
	    frame_message("Please enter a file name.", CREATE_EXECUTE_BUTTON);
	    return;
	}
	return_val = stat(name, &statbuf);
	/****
	* If the file already exists, pop up confirm window.
	****/
	if (return_val==0) {
	    confirm_val = notice_yn(CREATE_FRAME,"Overwrite existing file?");
	    if (!confirm_val) {
		free(amp);
		free(phase);
		return;
	    }
	}
	/****
	* If the file is new, or confirm_val of 0 is returned,
	* open and write to the file. 
	****/
	if (out_ptr = fopen (name, "w")) {
	    for (i=0; i<steps; i++) {
		fprintf (out_ptr,"%8.2f        %6.2f        %4.1f\n",
		    phase[i], amp[i], 1.0);
	    }
	    fclose(out_ptr);
	}
	else {
	    frame_message("Unable to write to file.", CREATE_EXECUTE_BUTTON);
	}
    }
    free(amp);
    free(phase);
}


/*****************************************************************
*  This routine is called when the "Zero Fill" button is
*  selected.  It increments the fft_index counter, calculates
*  a new FFT with a fourier size increased by two, and updates
*  the plot.
*****************************************************************/
static void zero_fill_notify_proc()
{
    if (++fft_index > 12) {
      --fft_index;
      frame_message("Advisory:  Maximum fft size reached.", MAIN_ZERO_FILL_BUTTON);
      return;
    }
    fft_calc();
    find_min_max();
    set_frame_label(BASE_FRAME,simulate_flag);
    plot_large_canvas(1);
}


/*****************************************************************
*  This routine handles mouse events in the plot canvas to
*  activate and update the vertical and horizontal cursors.
*****************************************************************/
void
do_plot_canvas_event_handler(button,down,up,drag, ctrl, x, y)
int button,down, up, drag, ctrl, x, y;
{
    int     position, y_position, point, ev;
    double  ft_freq, x_val, y_val, data_val;

    if (flag_3d) {
	polar_display(button,down,up, drag, x, y, 1);
    }

    else if (up) {
	/* do nothing */
      }

    /****
    * Middle mouse button: if the horizontal cursor is not active, adjust
    * the vertical scale (<control> key up) or the vertical position
    * (<control> key down).
    ****/
    else if ((button == 2) && (down || drag) && !horiz_flag[fft_flag])  {
	position = x;
	y_position = pl_height - y;
	x_val = xaxis_begin_val[fft_flag] + (double)position/(double)pl_width
	    *(xaxis_end_val[fft_flag] - xaxis_begin_val[fft_flag]);
	y_val = (min_max[lg_state][0] + (double)y_position/(double)pl_height
	    * (min_max[lg_state][1] - min_max[lg_state][0]))/vscale - vref;
	if (fft_flag) {
	    ft_freq = (double)nsteps/(2.0*pulselength);
	    point = rint((x_val + ft_freq)/(2.0*ft_freq)*(double)fft_steps);
	}
	else {
	    if (simulate_flag)
		point = rint((double)(sim_steps - 1)*(x_val - sim_start)
		    /(sim_end - sim_start));
	    else
		point = rint(x_val/pulselength*(double)(nsteps-1));
	}

	if (simulate_flag  &&  !fft_flag)
	    data_val = bloch_array[lg_state-8][point];
	else
	    data_val = pattern[lg_state][point];

	if (ctrl) {
	    vref += y_val - data_val;
	    sprintf(vertical_ref_array, "%-.3f", vref);
	    set_panel_value(PARAM_VERTICAL_REF,vertical_ref_array);
	    plot_large_canvas(1);
	}
	else {
	    if (data_val == 0.0)
		return;
	    if (y_val/data_val > 0.0) {
		vscale *= y_val/data_val;
		vref /= (y_val/data_val);
		sprintf(vertical_scale_array, "%-.3f", vscale);
		set_panel_value(PARAM_VERTICAL_SCALE,vertical_scale_array);
		sprintf(vertical_ref_array, "%-.3f", vref);
		set_panel_value(PARAM_VERTICAL_REF,vertical_ref_array);
		plot_large_canvas(1);
	    }
	    else
		return;
	}
    }

    /****
    * Middle mouse button: if the horizontal cursor is active, update
    * its position on the screen.
    ****/
    else if ((button == 2) && (down || drag) && horiz_flag[fft_flag])  {
	y_position = y;
	if (y_position >= 0  &&  y_position <= (double)pl_height) {
	    draw_cursors(3, y_position);
	}
    }

    /****
    * Left mouse button: update the position of the left cursor.
    ****/
    else if ((button == 1) && (down || drag) && !ctrl)  {
	position = x;
	if (position >=0 &&  position <= pl_width - delta_pos[fft_flag]) {
	    draw_cursors(1, position);
	}
    }

    /****
    * Left mouse button: if the left cursor is active, turn it off.
    ****/
    else if ((button == 1) && ctrl && (down || drag) && left_flag[fft_flag])  {
	draw_cursors(-1, 0);
    }

    /****
    * Right mouse button: update the position of the left cursor.
    ****/
    else if ((button == 3) && !ctrl && (down || drag) && left_flag[fft_flag])  {
	position = x;
	if (position > left_pos[fft_flag] && position <= pl_width) {
	    draw_cursors(2, position);
	}
    }

    /****
    * Right mouse button: if the right cursor is active, turn it off.
    ****/
    else if ((button == 3) && ctrl && (down || drag) && right_flag[fft_flag])  {
	draw_cursors(-2, 0);
    }
}


/*****************************************************************
*  This routine draws, updates, or erases left, right, and
*  horizontal cursors.
*****************************************************************/
static void draw_cursors(int cursor_status, int cursor_position)
{
    int   l_pos, r_pos, del_pos, h_pos;

    l_pos = left_pos[fft_flag];
    r_pos = right_pos[fft_flag];
    del_pos = delta_pos[fft_flag];
    h_pos = horiz_pos[fft_flag];

    if (!nsteps) return;
    if (!plot_flag)
      xor_mode(PLOT_WIN);
    switch (cursor_status) {
      case -3:
	/****
	* Called from horiz_notify_proc;
	* turns off horizontal cursor.
	****/
	  draw_line(PLOT_WIN,0,h_pos,pl_width,h_pos,YELLOW);
	  horiz_flag[fft_flag] = 0;
	  break;

      case -2:
	/****
	* Called from plot_canvas_event_handler by <CTRL>MS_RIGHT;
	* turns off right cursor.
	****/
	  draw_line(PLOT_WIN,r_pos,0,r_pos,pl_height,ORANGE);
	  right_flag[fft_flag] = 0;
	  delta_pos[fft_flag] = 0;
	  set_panel_label(MAIN_EXPAND_BUTTON, "Full");
	  break;

      case -1:
	/****
	* Called from plot_canvas_event_handler by <CTRL>MS_LEFT;
	* turns off left cursor, and right cursor if it exists.
	****/
	  draw_line(PLOT_WIN,l_pos,0,l_pos,pl_height,ORANGE);
	  if (right_flag[fft_flag])
	    draw_line(PLOT_WIN,r_pos,0,r_pos,pl_height,ORANGE);
	  delta_pos[fft_flag] = 0;
	  left_flag[fft_flag] = 0;
	  right_flag[fft_flag] = 0;
	  set_panel_label(MAIN_EXPAND_BUTTON, "Full");
	  break;

      case  0:
	/****
	* Called from plot_large_canvas to re-draw the cursor(s) if
	* they are turned on.
	****/
	  if (left_flag[fft_flag])  {
	    draw_line(PLOT_WIN,l_pos,0,l_pos,pl_height,ORANGE);
	    }
	  if (right_flag[fft_flag]) {
	    draw_line(PLOT_WIN,r_pos,0,r_pos,pl_height,ORANGE);
	    set_panel_label(MAIN_EXPAND_BUTTON, "Expand");
	  }
	  else {
	    set_panel_label(MAIN_EXPAND_BUTTON, "Full");
	  }
	  if (horiz_flag[fft_flag]) {
	    draw_line(PLOT_WIN,0,h_pos,pl_width,h_pos,YELLOW);
	  }
	  break;

      case  1:
	/****
	* Called from plot_canvas_event_handler with MS_LEFT to
	* initiate or update the left cursor.
	****/
	  if (left_flag[fft_flag])  {
	    draw_line(PLOT_WIN,l_pos,0,l_pos,pl_height,ORANGE);
	    }
	  draw_line(PLOT_WIN,cursor_position,0,cursor_position,pl_height,ORANGE);
	  left_pos[fft_flag] = cursor_position;
	  if (right_flag[fft_flag]) {
	      draw_line(PLOT_WIN,r_pos,0,r_pos,pl_height,ORANGE);
	      right_pos[fft_flag] = cursor_position + del_pos;
	      draw_line(PLOT_WIN,cursor_position+del_pos,0,
			cursor_position+del_pos,pl_height,ORANGE);
	  }
	  if (!left_flag[fft_flag]) {
	      left_flag[fft_flag] = 1;
	  }
	  break;

      case  2:
	/****
	* Called from plot_canvas_event_handler with MS_RIGHT to
	* initiate or update the right cursor.
	****/
	  if (right_flag[fft_flag])  {
	    draw_line(PLOT_WIN,r_pos,0,r_pos,pl_height,ORANGE);
	    }
	  right_pos[fft_flag] = cursor_position;
	  delta_pos[fft_flag] = cursor_position - l_pos;
	  draw_line(PLOT_WIN,cursor_position,0,cursor_position,pl_height,ORANGE);
	  if (!right_flag[fft_flag]) {
	    right_flag[fft_flag] = 1;
	    set_panel_label(MAIN_EXPAND_BUTTON, "Expand");
	  }
	  break;
      case  3:
	/****
	* Called from horiz_notify_proc, and plot_canvas_event_handler
	* to initiate or update the horizontal cursor.
	****/
	  if (horiz_flag[fft_flag])  {
	    draw_line(PLOT_WIN,0,h_pos,pl_width,h_pos,YELLOW);
	    }
	  horiz_flag[fft_flag] = 1;
	  horiz_pos[fft_flag] = cursor_position;
	  draw_line(PLOT_WIN,0,cursor_position,pl_width,cursor_position,YELLOW);
	  break;
    }  
    update_cursor_labels();
}


/*****************************************************************
*  This is a utility routine which takes a numerical input value,
*  and returns a string which contains the original value scaled
*  to the appropriate power of ten.  The appropriate prefix
*  (i.e., M for Mega, k for kilo, m for milli, u for micro) is
*  concatenated after the numerical part of the string.
*****************************************************************/
char *scale_and_add_units(value)
double  value;
{
    static char   label[32];
    int    unit_size;

    unit_size = (value == 0.0) ? 0 : (int)log10(fabs(value));
    if (unit_size >= 6) {
	sprintf(label, "%-.2f", value/1.0e6);
	strcat(label, " M");
    }
    else if (unit_size >= 3) {
	sprintf(label, "%-.2f", value/1000.0);
	strcat(label, " k");
    }
    else if (unit_size >= 0) {
	sprintf(label, "%-.2f", value);
	strcat(label, " ");
    }
    else if (unit_size < 0  &&  unit_size > -3) {
	sprintf(label, "%-.2f", value*1000.0);
	strcat(label, " m");
    }
    else if (unit_size <= -3) {
	sprintf(label, "%-.2f", value*1.0e6);
	strcat(label, " u");
    }
    return(label);
}



/*****************************************************************
*  Print the values represented by the cursors at the bottom
*  of the large canvas.
*****************************************************************/
void update_cursor_labels()
{
    int     ycoord, l_pos, r_pos, del_pos, h_pos, precision, sweep_var, units_index;
    double  left_val, right_val, delta_val, horiz_val,
	    min_val, max_val, xbegin, xend;
    char    label1[16], label2[16], label3[16];

    l_pos = left_pos[fft_flag];
    r_pos = right_pos[fft_flag];
    del_pos = delta_pos[fft_flag];
    h_pos = horiz_pos[fft_flag];
    xbegin = xaxis_begin_val[fft_flag];
    xend = xaxis_end_val[fft_flag];
    ycoord = lg_height - 8;

    sweep_var = (int)get_panel_value(SIMULATE_FORMAT_CYCLE);
    if (lg_state <= 4  ||  (lg_state >=8 && sweep_var == 2))
	units_index = 0;
    else
	units_index = 1;

    if (!plot_flag)  {
      normal_mode(LG_WIN);
      start_graphics(LG_WIN);
      }
    if (left_flag[fft_flag]) {
	left_val = xbegin + (double)(l_pos)*(xend - xbegin)/(double)pl_width;
	strcpy(label1, scale_and_add_units(left_val)); 
	strcat(label1, units_list[units_index]);
	if (!plot_flag)
	  clear_area(LG_WIN,6,ycoord-char_ascent-1,8*24,char_height+2);
	draw_string(LG_WIN,6,ycoord,"left:",YELLOW);
	if (!plot_flag)
	  draw_inverse_string(LG_WIN,48,ycoord,label1,YELLOW);
	else
	  draw_string(LG_WIN,58,ycoord,label1,YELLOW);
    }
    else {
	if (!plot_flag)
	  clear_area(LG_WIN,6,ycoord-char_ascent-1,8*24,char_height+2);
    }
    if (right_flag[fft_flag]) {
	right_val = xbegin + (double)(r_pos)*(xend - xbegin)/(double)pl_width;
	strcpy(label2, scale_and_add_units(right_val)); 
	strcat(label2, units_list[units_index]);
	if (!plot_flag)  {
	  clear_area(LG_WIN,158,ycoord-char_ascent-1,8*24,char_height+2);
	  clear_area(LG_WIN,318,ycoord-char_ascent-1,8*24,char_height+2);
	  }
	draw_string(LG_WIN,158,ycoord,"right:",YELLOW);
	if (!plot_flag)
	  draw_inverse_string(LG_WIN,208,ycoord,label2,YELLOW);
	else
	  draw_string(LG_WIN,218,ycoord,label2,YELLOW);
	delta_val = del_pos/(double)pl_width*(xend - xbegin);
	strcpy(label3, scale_and_add_units(delta_val)); 
	strcat(label3, units_list[units_index]);
	draw_string(LG_WIN,318,ycoord,"delta:",YELLOW);
	if (!plot_flag)
	  draw_inverse_string(LG_WIN,368,ycoord,label3,YELLOW);
	else
	  draw_string(LG_WIN,378,ycoord,label3,YELLOW);
    }
    else {
	if (!plot_flag)  {
	  clear_area(LG_WIN,158,ycoord-char_ascent-1,8*24,char_height+2);
	  clear_area(LG_WIN,318,ycoord-char_ascent-1,8*24,char_height+2);
	  }
    }
    if (horiz_flag[fft_flag]) {
	min_val = min_max[lg_state][0]/vscale - vref;
	max_val = min_max[lg_state][1]/vscale - vref;
	horiz_val = max_val - (double)h_pos/(double)pl_height*(max_val - min_val);
	if (fabs(max_val - min_val) == 0.0)
	    precision = 3;
	else
	    precision = 3 - log(fabs(max_val - min_val));
	precision = precision <= 0 ? 1 : precision;
	precision = precision > 6 ? 6 : precision;
	sprintf(label1, "%-.*f", precision, horiz_val);
	if (!plot_flag)
	  clear_area(LG_WIN,910,ycoord-char_ascent-1,8*24,char_height+2);
	draw_string(LG_WIN,910,ycoord,"horiz:",YELLOW);
	if (!plot_flag)
	  draw_inverse_string(LG_WIN,960,ycoord,label1,YELLOW);
	else
	  draw_string(LG_WIN,970,ycoord,label1,YELLOW);
    }
    else {
	if (!plot_flag)
	  clear_area(LG_WIN,910,ycoord-char_ascent-1,8*24,char_height+2);
    }
  if (!plot_flag)
    flush_graphics(LG_WIN);
}

/*****************************************************************
*  This routine is called when the "Directory" and "Pulse Name"
*  text items are modified, and by the "Files" button.
*****************************************************************/
void do_files_proc(item)
int   item;
{
    char         string[512], old_directory_name[256];

    if (item == PARAM_DIRECTORY_NAME) {
        strcpy(old_directory_name, directory_name);
        strcpy(directory_name, (char *)get_panel_value(PARAM_DIRECTORY_NAME));
        if (chdir(directory_name) == -1) {
            frame_message("Error:  Unable to change directory.", PARAM_DIRECTORY_NAME);
	    set_panel_value(PARAM_DIRECTORY_NAME, old_directory_name);
            return;
        }
        strcpy(string, "dir");
        strcat(string, directory_name);
        send_to_pulsechild(string);
    }
    else if (item == PARAM_FILE_NAME) {
        strcpy(pattern_name, (char *)get_panel_value(PARAM_FILE_NAME));
	if (*pattern_name){
	    read_pattern_file(0);
	}
        return;
    }
    else if (item == MAIN_FILE_BUTTON) {
        send_to_pulsechild("show");
        return;
    }
}


/*****************************************************************
*  Clear any or all of the canvases, determined by "mode":
*  mode=0 clears all canvases (small, large, plot);
*  mode=1 clears only the small canvases;
*  mode=2 clears both the large and plot canvases.
*  mode=3 clears only the large canvas (axes, labels, etc.);
*  mode=4 clears only the plot canvas;
*  mode=5 clears only the plot canvas, regardless of clear_flag;
*****************************************************************/
static void clear_canvas(int mode)
{
    int   i;

    if (mode == 0  ||  mode == 1) {
	clear_window(SM_WIN_1);
	clear_window(SM_WIN_2);
	clear_window(SM_WIN_3);
	clear_window(SM_WIN_4);
	clear_window(SM_WIN_5);
	clear_window(SM_WIN_6);
    }
    if (mode == 0  ||  mode == 2  ||  mode == 3) {
	  clear_window(LG_WIN);
    }
    /****
    * clear_flag must be set to clear the plot_canvas.
    ****/
    if ((mode == 0  ||  mode == 2  ||  mode == 4)  &&  clear_flag) {
	  clear_window(PLOT_WIN);
    }
    if (mode == 5) {
	  clear_window(PLOT_WIN);
    }
}


/*****************************************************************
*  This routine is called by the "Display" cycle in the main
*  button panel to toggle between display of pulse data and 
*  simulation results.
*****************************************************************/
void do_display_cycle_proc()
{
    int    display_state, k;

    display_state = (int) get_panel_value(MAIN_DISPLAY_CYCLE);
    /****
    * If display_state is 1, display the pulse data, resetting
    * all the display parameters first.
    ****/
    if (display_state) {
        lg_state = 0;
        fft_flag = 0;
        simulate_flag = 0;
        vscale = 1.0;
        vref = 0.0;
        xaxis_begin_val[0] = 0.0;
        xaxis_end_val[0] = pulselength;
        set_print_save_filenames();
	for (k=0;k<NUM_SM;k++)
          plot_small_canvas(k);
        plot_large_canvas(0);
        plot_large_canvas(1);
        set_frame_label(BASE_FRAME, simulate_flag);
    }
    /****
    * Otherwise, if the simulation is current display the simulation results.
    ****/
    else {
        if (sim_is_current) {
            lg_state = 8;
            fft_flag = 0;
            simulate_flag = 1;
            vscale = 1.0;
            vref = 0.0;
            xaxis_begin_val[0] = sim_start;
            xaxis_end_val[0] = sim_end;
            set_print_save_filenames();
	    for (k=0;k<NUM_SM;k++)
              plot_small_canvas(k);
            plot_large_canvas(0);
            plot_large_canvas(1);
            set_frame_label(BASE_FRAME, simulate_flag);
        }
        else {
            frame_message ("Error:  Simulation data is not current.", MAIN_DISPLAY_CYCLE);
            
	set_panel_value(MAIN_DISPLAY_CYCLE, 1);
        }
    }
}

void do_grid_cycle_proc()
{
    plot_large_canvas(0);
    plot_large_canvas(1);
}


/*****************************************************************
*  This routine handles the buttons associated with the Bloch
*  simulation window.
*****************************************************************/
void do_simulate_proc(item)
int    item;
{
    int     i, sweep_var, x_pos, y_pos, k;
    double  value;
    char    string[10][64];
    char    tmpstr[64];

    sweep_var = (int)get_panel_value(SIMULATE_FORMAT_CYCLE);
    
    hide_object(SIMULATE_3D_BUTTON);

    /****
    * Activate the simulate window.
    ****/
    if (item == MAIN_SIMULATE_BUTTON) {
	show_object(SIMULATE_FRAME);
    }

    /****
    * Close the simulate window.
    ****/
    else if (item == SIMULATE_DONE_BUTTON) {
	hide_object(SIMULATE_FRAME);
    }

    /****
    * Cycle the simulation sweep variable through frequency, B1, and time.
    ****/
    else if (item == SIMULATE_FORMAT_CYCLE) {
	switch (sweep_var) {
	  case  0:
	set_panel_label(SIMULATE_PARAMETER_PANEL_3, "B1max (KHz):");
	set_panel_label(SIMULATE_PARAMETER_PANEL_4, "Start Freq (KHz):");
	set_panel_label(SIMULATE_PARAMETER_PANEL_5, "Stop Freq (KHz):");
	show_object(SIMULATE_PARAMETER_PANEL_5);
	show_object(SIMULATE_PARAMETER_PANEL_6);
	     sprintf(sim_param_array[3], "%-.3f", B1_max/1000.0);
	     sprintf(sim_param_array[4], "%-.3f", freq_start/1000.0);
	   sprintf(sim_param_array[5], "%-.3f", freq_end/1000.0);
	      break;
	  case  1:
	set_panel_label(SIMULATE_PARAMETER_PANEL_3, "Frequency (KHz):");
	set_panel_label(SIMULATE_PARAMETER_PANEL_4, "B1 start (KHz):");
	set_panel_label(SIMULATE_PARAMETER_PANEL_5, "B1 stop (KHz):");
	show_object(SIMULATE_PARAMETER_PANEL_5);
	show_object(SIMULATE_PARAMETER_PANEL_6);
	      sprintf(sim_param_array[3], "%-.3f", frequency/1000.0);
	      sprintf(sim_param_array[4], "%-.3f", b1_start/1000.0);
	      sprintf(sim_param_array[5], "%-.3f", b1_end/1000.0);
	      break;
	  case  2:
	set_panel_label(SIMULATE_PARAMETER_PANEL_3, "B1max (KHz):");
	set_panel_label(SIMULATE_PARAMETER_PANEL_4, "Frequency (KHz):");
	hide_object(SIMULATE_PARAMETER_PANEL_5);
	hide_object(SIMULATE_PARAMETER_PANEL_6);
	      sprintf(sim_param_array[3], "%-.3f", B1_max/1000.0);
	      sprintf(sim_param_array[4], "%-.3f", frequency/1000.0);
	      break;
	}
	set_panel_value(SIMULATE_PARAMETER_PANEL_3,sim_param_array[3]);
	set_panel_value(SIMULATE_PARAMETER_PANEL_4,sim_param_array[4]);
	set_panel_value(SIMULATE_PARAMETER_PANEL_5,sim_param_array[5]);
	flag_3d = 0;
	pulse_index = 0;
	sim_is_current = 0;
	return;
    }

    /****
    * If the "Go" button is selected, or if the "Step" button is selected
    * with the pulse Index at zero.
    ****/
    else if (item == SIMULATE_GO_BUTTON || (item == SIMULATE_STEP_BUTTON && !pulse_index)) {
	/****
	* Make sure there is at least one step in the pulse.
	****/
	if (!nsteps) {
	    frame_message("Pulse template error:  steps = 0.", item);
	    return;
	}
	/****
	* Read the simulation parameters; check for validity and out-of-range.
	****/
	strcpy(string[0], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_0));
	if (!strcmp(string[0], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[1], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_1));
	if (!strcmp(string[1], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[2], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_2));
	if (!strcmp(string[2], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[3], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_3));
	if (!strcmp(string[3], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[4], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_4));
	if (!strcmp(string[4], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[5], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_5));
	if (!strcmp(string[5], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[6], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_6));
	if (!strcmp(string[6], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[7], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_7));
	if (!strcmp(string[7], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[8], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_8));
	if (!strcmp(string[8], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	strcpy(string[9], (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_9));
	if (!strcmp(string[9], "")) {
		frame_message("Error:  Undefined parameter value.", item);
		return;
	    }
	for (i=0; i<10; i++) {
	    sscanf(string[i], "%lf", &value);
	    switch (i) {
		case  0:
		    Mx = value;
		    break;
		case  1:
		    My = value;
		    break;
		case  2:
		    Mz = value;
		  /****
		  * Check the magnitude of the magnetization.
		  ****/
		    if (Mx*Mx + My*My + Mz*Mz  >  1.0) {
			frame_message("Error:  Magnetization greater than one.",
				   item);
			return;
		    }
		    break;
		case  3:
		    if (sweep_var == 0 || sweep_var == 2)
			B1_max = 1000.0*value;
		    else
			frequency = 1000.0*value;
		    break;
		case  4:
		    freq_start = (sweep_var == 0) ? 1000.0*value : freq_start;
		    b1_start = (sweep_var == 1) ? 1000.0*value : b1_start;
		    frequency = (sweep_var == 2) ? 1000.0*value : frequency;
		    break;
		case  5:
		    freq_end = (sweep_var == 0) ? 1000.0*value : freq_end;
		    b1_end = (sweep_var == 1) ? 1000.0*value : b1_end;
		    if (sweep_var == 1) {
			if (b1_start < 0.0  ||  b1_end < 0.0  ||  b1_start == b1_end) {
			    frame_message("Error:  Improper B1 value.", item);
			    return;
			}
		    }
		    else {
			if (B1_max < 0.0) {
			    frame_message("Error:  Improper B1 value.", item);
			    return;
			}
			if (freq_start == freq_end) {
			    frame_message("Error:  Improper frequency range.",
				item);
			    return;
			}
		    }
		    break;
		case  6:
		    sim_steps = (int)value;
		    if (sim_steps < 3  ||  sim_steps > max_steps) {
                        if (max_steps == INOVA_MAX_PULSE_STEPS)
			   frame_message("Error:  3 <= Steps <= 8192.", item);
                        else
			   frame_message("Error:  3 <= Steps <= 256K.", item);
			return;
		    }
		    if (sweep_var == 2) {
			sim_steps = nsteps;
		    }
		    break;
		case  7:
		    phase = value;
		    break;
		case  8:
		    if (value) {
/*			frame_message("Error:  Index may not be set externally.",
			    item);*/
			set_panel_value(SIMULATE_PARAMETER_PANEL_8,
					 sim_param_array[8]);
		    }
		    break;
		case  9:
		    index_inc = (int)value;
		    if (index_inc < 1) {
			frame_message("Error:  Minimum increment = 1",item);
			return;
		    }
		    else if (index_inc + pulse_index > nsteps) {
			frame_message("Error:  Maximum increment exceeded.",
			    item);
			return;
		    }
		    break;
		default:
		    break;
	    }
	}
	simulate_flag = 1;
	/****
	* If this routine is called with the "Step" button:
	* (not functional when sweeping time).
	****/
	if (item == SIMULATE_STEP_BUTTON) {
	    if (sweep_var == 2) {
		frame_message("Sorry, cannot step \"time\".", item);
		simulate_flag = 0;
		return;
	    }
	    bloch_simulate(STEP);
	}
	/****
	* Otherwise, it was called with the "Go" button, so pop-up the "Cancel"
	* button from pulsechild, do the simulation, then hide the "Cancel"
	* pop-up.
	****/
	else {
	    if (sweep_var < 2) {

		x_pos = 850;
		y_pos = 180;
		sprintf(tmpstr, "%s %d %d", "show_cancel", x_pos, y_pos);
		send_to_pulsechild(tmpstr);
	    }
	    else
	    show_object(SIMULATE_3D_BUTTON);
	    pulse_index = 0;
	    bloch_simulate(GO);
	    send_to_pulsechild("hide_cancel");
	}
	/****
	* If the "Cancel" button is hit during a simulation, "interrupt" will
	* have been set to 1 at this point, so reset everything.
	****/
	if (interrupt) {
	    sim_is_current = 0;
	    pulse_index = 0;
	    simulate_flag = 0;
	    interrupt = 0;
	    frame_message("Simulation cancelled.", item);
	    sprintf(sim_param_array[8], "%-d", pulse_index);

	    set_panel_value(SIMULATE_PARAMETER_PANEL_8, sim_param_array[8]);
	    return;
	}
	/****
	* Set the start and end limits of the independent variable for plotting.
	****/
	if (sweep_var == 0) {
	    sim_start = freq_start;
	    sim_end = freq_end;
	    for (i=8; i<=12; i++)
		strcpy(xaxis_label[i], "Frequency");
	}
	else if (sweep_var == 1) {
	    sim_start = b1_start;
	    sim_end = b1_end;
	    for (i=8; i<=12; i++)
		strcpy(xaxis_label[i], "B1");
	}
	else if (sweep_var == 2) {
	    sim_start = 0.0;
	    sim_end = pulselength;
	    for (i=8; i<=12; i++)
		strcpy(xaxis_label[i], "Time");
	}
	/****
	* Reset plot parameters, and plot to small and large canvases.
	****/
	xaxis_begin_val[0] = sim_start;
	xaxis_end_val[0] = sim_end;
	vscale = 1.0;
	vref = 0.0;
	lg_state = (lg_state < 8) ? 8 : lg_state;
	fft_flag = 0;
	sim_is_current = 1;
        set_print_save_filenames();
	for (k=0;k<NUM_SM;k++)
	  plot_small_canvas(k);
	if (flag_3d)
	    polar_display(0,0,0,0,0,0,0);
	else  {
	    plot_large_canvas(0);
	    plot_large_canvas(1);
	    }
        set_panel_value(MAIN_DISPLAY_CYCLE, 0);
	sprintf(vertical_scale_array, "%-.3f", vscale);
	set_panel_value(PARAM_VERTICAL_SCALE, vertical_scale_array);
	sprintf(vertical_ref_array, "%-.3f", vref);
	set_panel_value(PARAM_VERTICAL_REF, vertical_ref_array);
	set_frame_label(BASE_FRAME, simulate_flag);
    }
    /****
    * If the "Step" button started all this, but "pulse_index" is non-zero,
    * then we are in the middle of a simulation, and should just continue
    * with the next "increment" steps.
    ****/
    else if (item == SIMULATE_STEP_BUTTON) {
	if ((int)get_panel_value(MAIN_DISPLAY_CYCLE)) {
	    frame_message("Current simulation must be displayed.", item);
	    return;
	}

	strcpy(tmpstr, (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_9));
	sscanf(tmpstr, "%d", &index_inc);
	if (index_inc < 1) {
	    frame_message("Error:  Minimum increment = 1", item);
	    return;
	}
	else if (index_inc + pulse_index > nsteps) {
	    frame_message("Error:  Maximum increment exceeded.", item);
	    return;
	}

	strcpy(tmpstr, (char *)get_panel_value(SIMULATE_PARAMETER_PANEL_7));
	if (!strcmp(tmpstr, "")) {
	    frame_message("Error:  Undefined parameter value.", item);
	    return;
	}
	sscanf(tmpstr, "%lf", &phase);
	bloch_simulate(STEP);
	for (k=0;k<NUM_SM;k++)
	  plot_small_canvas(k);
	plot_large_canvas(0);
	plot_large_canvas(1);
    }

    /****
    * If the "Delay" button was selected, then read the delay time, and
    * perform a one step simulation with this delay, without incrementing
    * the pulse index.
    ****/
    else if (item == SIMULATE_DELAY_BUTTON) {
	if ((int)get_panel_value(MAIN_DISPLAY_CYCLE)) {
	    frame_message("Current simulation must be displayed.", item);
	    return;
	}
	if (sweep_var > 1) {
	    frame_message("Delay not allowed with Time sweep.", item);
	    return;
	}
	bloch_simulate(SIM_DELAY);
	for (k=0;k<NUM_SM;k++)
	  plot_small_canvas(k);
	plot_large_canvas(0);
	plot_large_canvas(1);
    }
}


/*****************************************************************
*  This routine is called any time one of the parameter fields
*  in the bottom panel is changed.  It checks the validity of the
*  new value or string, and takes appropriate action in each case.
*****************************************************************/
void do_param_notify_proc(item)
int   item;
{
    double  new_val;
    char    string[32];

    strcpy(string, (char *)get_panel_value(item));
    if (item == PARAM_PULSE_LENGTH) {
	/****
	* pulselength cannot be "undefined" or zero.
	****/
	if (!strcmp(string, "")) {
	    frame_message("Error:  Pulse width is not defined.", item);
	    set_panel_value(PARAM_PULSE_LENGTH,pulselength_array);
	    return;
	}
	sscanf(string, "%lf", &new_val);
	if (new_val == 0.0) {
	    frame_message("Error:  Pulse width set to zero.", item);
	    set_panel_value(PARAM_PULSE_LENGTH,pulselength_array);
	    return;
	}
	/****
	* If the pulselength is ok, recalculate the FFT and min/max values.
	****/
	sprintf(pulselength_array, "%-.3f", new_val);
	set_panel_value(PARAM_PULSE_LENGTH,pulselength_array);
	pulselength = new_val/1000.0;
	xaxis_begin_val[1] = -(double)nsteps/(2.0*pulselength);
	xaxis_end_val[1] = -xaxis_begin_val[1];
	sim_is_current = 0;
	pulse_index = 0;

	calc_real_imag_freq();
	fft_calc();
	find_min_max();
	if (simulate_flag) {
	    simulate_flag = 0;
	    frame_message("Advisory:  Simulation is not correct for this pulse length.", item);
	}
	else {
	    set_frame_label(BASE_FRAME, simulate_flag);
	    xaxis_begin_val[0] = 0.0;
	    xaxis_end_val[0] = pulselength;
	    plot_large_canvas(0);
	    plot_large_canvas(1);
	}
    }
    else if (item == PARAM_VERTICAL_SCALE) {
	if (!strcmp(string, "")) {
	    frame_message("Error:  Vertical scale is not defined.", item);
	    set_panel_value(PARAM_VERTICAL_SCALE,vertical_scale_array);
	    return;
	}
	sscanf(string, "%lf", &new_val);
	if (new_val == 0.0) {
	    frame_message("Error:  Vertical scale set to zero.", item);
	    set_panel_value(PARAM_VERTICAL_SCALE, vertical_scale_array);
	    return;
	}
	sprintf(vertical_scale_array, "%-.3f", new_val);
	set_panel_value(PARAM_VERTICAL_SCALE, vertical_scale_array);
	vscale = new_val;
	plot_large_canvas(0);
	plot_large_canvas(1);
    }
    else if (item == PARAM_VERTICAL_REF) {
	if (!strcmp(string, "")) {
	    frame_message("Error:  Vertical reference is not defined.", item);
	    set_panel_value(PARAM_VERTICAL_REF,vertical_ref_array);
	    return;
	}
	sscanf(string, "%lf", &new_val);
	sprintf(vertical_ref_array, "%-.3f", new_val);
	set_panel_value(PARAM_VERTICAL_REF, vertical_ref_array);
	vref = new_val;
	plot_large_canvas(0);
	plot_large_canvas(1);
    }
    else if (item == PARAM_STEPS) {
	frame_message("Error:  Steps may not be set externally.", item);
	set_panel_value(PARAM_STEPS,steps_array);
    }
    else if (item == PARAM_FOURIER_SIZE) {
	frame_message("Error:  Fourier Size may not be set externally.", item);
	set_panel_value(PARAM_FOURIER_SIZE,fourier_size_array);
    }
    else if (item == PARAM_POWER_FACTOR) {
	frame_message("Error:  Power Factor may not be set externally.", item);
	set_panel_value(PARAM_POWER_FACTOR,power_factor_array);
    }
    else if (item == PARAM_INTEGRAL) {
	frame_message("Error:  Integral may not be set externally.", item);
	set_panel_value(PARAM_INTEGRAL,integral_array);
    }
}
 

void do_repaint_large_canvas()
{
    if (flag_3d)  {
	start_graphics(LG_WIN);
        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN-1,
            LEFT_MARGIN+40, TOP_MARGIN-1, BLUE);
        draw_line(LG_WIN, LEFT_MARGIN+pl_width-40, TOP_MARGIN-1,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN-1, BLUE);

        draw_line(LG_WIN, LEFT_MARGIN+pl_width+2, TOP_MARGIN-1,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN+40, BLUE);
        draw_line(LG_WIN,LEFT_MARGIN+pl_width+2,TOP_MARGIN+pl_height-40,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN+pl_height+2, BLUE);

        draw_line(LG_WIN,LEFT_MARGIN+pl_width-40,TOP_MARGIN+pl_height+2,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN+pl_height+2, BLUE);
        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN+pl_height+2,
            LEFT_MARGIN+40, TOP_MARGIN+pl_height+2, BLUE);

        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN+pl_height-40,
            LEFT_MARGIN-1, TOP_MARGIN+pl_height+2, BLUE);
        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN-1,
            LEFT_MARGIN-1, TOP_MARGIN+40, BLUE);
	flush_graphics(LG_WIN);
	}
    else
      plot_large_canvas(0);
}


void do_repaint_plot_canvas()
{
    unsigned long event;
    static int firsttime = TRUE;

    if (firsttime){
	firsttime = FALSE;
	do_files_proc(PARAM_FILE_NAME);
    }
    if (flag_3d)
      polar_display(0,0,0,0,0,0,0);
    else
      plot_large_canvas(1);
}


/*****************************************************************
*  Plot one of the six small canvases in the large canvas,
*  determined by the global variable lg_state.
******************************************************************/
static void plot_large_canvas(int num)
{
    int      i, x_new, x_old, y_new, y_old, precision, sweep_var, units_index,
             start_point, end_point, xpos, ypos, grid_flag, temp, counter;

    double   data_width, ft_freq, min_val, max_val;
    
    char     label1[20], label2[20], label3[20],
             label4[20], label5[20], label6[20];
    int data[2*MAX_PULSE_STEPS];

    if (!nsteps)
        return;

    min_val = min_max[lg_state][0]/vscale - vref;
    max_val = min_max[lg_state][1]/vscale - vref;

    switch (lg_state) {
	/****
	* Find the start and end points for the plot.
	****/
        case 0:  case 1:  case 2:  case 3:  case 4:
            start_point = floor((double)(nsteps-1)*xaxis_begin_val[0]/pulselength);
            end_point = ceil((double)(nsteps-1)*xaxis_end_val[0]/pulselength);
	    start_point = start_point < 0 ? 0 : start_point;
	    end_point = end_point >= nsteps ? nsteps-1 : end_point;
            data_width = (double)(end_point - start_point);
            break;

        case 5:  case 6:  case 7:
            ft_freq = (double)nsteps/(2.0*pulselength);
            start_point = floor((double)fft_steps
                *(xaxis_begin_val[1] + ft_freq)/(2.0*ft_freq));
            end_point = ceil((double)fft_steps
                *(xaxis_end_val[1] + ft_freq)/(2.0*ft_freq));
	    start_point = start_point < 0 ? 0 : start_point;
	    end_point = end_point > fft_steps ? fft_steps : end_point;
            data_width = (double)(end_point - start_point);
            break;

        case 8:  case 9:  case 10:  case 11:  case 12:
            start_point = floor((double)(sim_steps - 1)
                *(xaxis_begin_val[0] - sim_start)/(sim_end - sim_start));
            end_point = ceil((double)(sim_steps - 1)
                *(xaxis_end_val[0] - sim_start)/(sim_end - sim_start));
	    start_point = start_point < 0 ? 0 : start_point;
	    end_point = end_point >= sim_steps ? sim_steps-1 : end_point;
            data_width = (double)(end_point - start_point);
            break;
    }

    /*****
    *  clear the canvas.
    *****/
  if (num == 0)  {
    if (!plot_flag)  {
      normal_mode(LG_WIN);
      start_graphics(LG_WIN);
      clear_canvas(3);
      }

    /*****
    * Draw box around plot area.
    *****/

    draw_line(LG_WIN,LEFT_MARGIN-1, TOP_MARGIN-1,
			LEFT_MARGIN+pl_width+2,TOP_MARGIN-1,GREEN);
    draw_line(LG_WIN,LEFT_MARGIN+pl_width+2, TOP_MARGIN-1,
			LEFT_MARGIN+pl_width+2,TOP_MARGIN+pl_height+2,GREEN);
    draw_line(LG_WIN,LEFT_MARGIN+pl_width+2, TOP_MARGIN+pl_height+2,
			LEFT_MARGIN-1,TOP_MARGIN+pl_height+2,GREEN);
    draw_line(LG_WIN,LEFT_MARGIN-1, TOP_MARGIN+pl_height+2,
			LEFT_MARGIN-1,TOP_MARGIN-1,GREEN);
    

    /*****
    *  Draw "tik" marks at bottom and left of box;
    *****/
    grid_flag = (int)get_panel_value(MAIN_GRID_CYCLE);
    for (i=0; i<=10; i++) {
        xpos = LEFT_MARGIN + i*pl_width/10;
        draw_line(LG_WIN,xpos, TOP_MARGIN+pl_height+2,
				xpos, TOP_MARGIN+pl_height+2+7,GREEN);
    }

    for (i=0; i<=10; i++) {
        ypos = TOP_MARGIN + i*pl_height/10;
        draw_line(LG_WIN,LEFT_MARGIN-1-7, ypos,
				LEFT_MARGIN-1, ypos,GREEN);
    }

    /*****
    * Calculate the x positions for the title and x-axis label.
    *****/
    xpos = LEFT_MARGIN + pl_width/2 - (9*strlen(plot_title[lg_state]))/2;
    draw_string(LG_WIN,xpos,TOP_MARGIN/2,plot_title[lg_state],GREEN);

    xpos = LEFT_MARGIN + pl_width/2 - (9*strlen(xaxis_label[lg_state]))/2;
    draw_string(LG_WIN,xpos,TOP_MARGIN+pl_height+55,xaxis_label[lg_state],GREEN);

    /*****
    *  Write values to the first, last, and middle tik marks of each axis.
    *****/
    temp = (max_val == min_val) ? 0 : (int)log10(fabs(max_val - min_val));
    if (temp <= 0)
        precision = abs(temp) + 2;
    else
        precision = 1;

    sweep_var = (int)get_panel_value(SIMULATE_FORMAT_CYCLE);
    if (lg_state <= 4  ||  (lg_state >=8 && sweep_var == 2))
	units_index = 0;
    else
	units_index = 1;

    sprintf(label1, "%.*f", precision, min_val);
    sprintf(label2, "%.*f", precision, (min_val+max_val)/2.0);
    sprintf(label3, "%.*f", precision, max_val);
    strcpy(label4, scale_and_add_units(xaxis_begin_val[fft_flag])); 
    strcat(label4, units_list[units_index]);
    strcpy(label5, scale_and_add_units(
        (xaxis_begin_val[fft_flag]+xaxis_end_val[fft_flag])/2.0)); 
    strcat(label5, units_list[units_index]);
    strcpy(label6, scale_and_add_units(xaxis_end_val[fft_flag])); 
    strcat(label6, units_list[units_index]);

    xpos = LEFT_MARGIN - 9*strlen(label1) - 18;
    ypos = TOP_MARGIN + pl_height + 5;
    draw_string(LG_WIN,xpos,ypos,label1,GREEN);
    xpos = LEFT_MARGIN - 9*strlen(label2) - 18;
    ypos = TOP_MARGIN + pl_height/2 + 5;
    draw_string(LG_WIN,xpos,ypos,label2,GREEN);
    xpos = LEFT_MARGIN - 9*strlen(label3) - 18;
    ypos = TOP_MARGIN + 1 + 5;
    draw_string(LG_WIN,xpos,ypos,label3,GREEN);

    ypos = TOP_MARGIN + pl_height + 28;
    xpos = LEFT_MARGIN - 5;
    draw_string(LG_WIN,xpos,ypos,label4,GREEN);
    xpos = LEFT_MARGIN + pl_width/2 - (9*strlen(label5))/2;
    draw_string(LG_WIN,xpos,ypos,label5,GREEN);
    xpos = LEFT_MARGIN + pl_width - (9*strlen(label6))*2/3;
    draw_string(LG_WIN,xpos,ypos,label6,GREEN);
    update_cursor_labels();
    if (!plot_flag)  {
      flush_graphics(LG_WIN);
      }
    }
  else if (num == 1)  {  /* redraw plot canvas */
    if (!plot_flag)  {
      normal_mode(PLOT_WIN);
      start_graphics(PLOT_WIN);
      if (clear_flag)
        clear_canvas(5);
      }
    /*****
    *  Draw dotted grid lines if "grid_flag" is set to 1.
    *****/
    grid_flag = (int)get_panel_value(MAIN_GRID_CYCLE);
  if (!plot_flag)
    for (i=0; i<=10; i++) {
        xpos = LEFT_MARGIN + i*pl_width/10;
        if (grid_flag  &&  (i>0 && i<10)){
            ypos = 2;
	    counter = 0;
            while (ypos <= pl_height+1) {
		data[2*counter] = xpos-LEFT_MARGIN;
		data[2*counter+1] = ypos;
		counter++;
                ypos += 3;
            }
	    if (counter != 0)
	      draw_points(PLOT_WIN,data,counter,RED);
        }
    }

  if (!plot_flag)
    for (i=0; i<=10; i++) {
        ypos = TOP_MARGIN + i*pl_height/10;
        if (grid_flag  &&  (i>0 && i<10)){
            xpos = 1;
	    counter = 0;
            while (xpos <= pl_width+1) {
		data[2*counter] = xpos;
		data[2*counter+1] = ypos -TOP_MARGIN;
		counter++;
                xpos += 3;
            }
	    if (counter != 0)
	      draw_points(PLOT_WIN,data,counter,RED);
        }
    }
    /*****
    * Set the starting x and y coordinate values for the plot.
    * successive values are determined inside the loop below.
    *****/
    x_old = 0;
    if (max_val == min_val)
        y_old = pl_height/2;
    else
        if (lg_state >= 8)
            y_old = pl_height - rint((bloch_array[lg_state-8][start_point] - min_val)
                /(max_val - min_val)*(double)pl_height);
        else
            y_old = pl_height - rint((pattern[lg_state][start_point] - min_val)
                /(max_val - min_val)*(double)pl_height);

    /*****
    * Complete the remainder of the plot.
    *****/
    counter = 0;
    for (i=start_point+1; i<=end_point; i++) {
        x_new = (double)(i-start_point)*(double)pl_width/data_width;

        if (max_val == min_val)
            y_new = pl_height/2;
        else
            if (lg_state >= 8)
                y_new = pl_height - rint((bloch_array[lg_state-8][i] - min_val)
                    /(max_val - min_val)*(double)pl_height);
            else
                y_new = pl_height - rint((pattern[lg_state][i] - min_val)
                    /(max_val - min_val)*(double)pl_height);
	y_new = abs(y_new) - 5*pl_height > 0 ?
	    (int)copysign(5.0*(double)pl_height, (double)y_new) : y_new;
	data[2*counter] = x_old;
	data[2*counter+1] = y_old;
	counter++;
        y_old = y_new;
        x_old = x_new;
        }
      data[2*counter] = x_old;
      data[2*counter+1] = y_old;
      counter++;
      if (counter > 1) 
        draw_segments(PLOT_WIN,data,counter,plot_color);

    /*****
    * If cursors are turned on, redraw them.
    *****/
    if (clear_flag)
        draw_cursors(0, 0);
    else
        update_cursor_labels();
    if (!plot_flag)  {
      flush_graphics(PLOT_WIN);
      }
    }

}


/*********************************************************************
*  Repaint small canvases 
*********************************************************************/
void do_repaint_small_canvases(which)
int which;
{
    plot_small_canvas(which-SM_WIN_1);
}


/*********************************************************************
*  Plot each of the six small canvases.
*********************************************************************/
static void plot_small_canvas(int num)
{
    int      i, margin=3, x_old, x_new, y_new, y_old[6], counter;
    double   width, height;
    int data[2*MAX_PULSE_STEPS];

    if (!nsteps) {
        return;
    }

    normal_mode(num+SM_WIN_1);
    width = (double)(sm_width - 2*margin);
    height = (double)(sm_height - 2*margin);

    x_old = margin;

    /*****
    * Initialize the first point y-value in each of the small canvases.
    * If simulate_flag is set, plot the data in bloch_array, otherwise
    * plot the data in pattern.
    *****/
        if (min_max[num][1] - min_max[num][0] == 0.0)
            y_old[num] = sm_height/2;
        else
            y_old[num] = sm_height-margin-rint((pattern[num][0]-min_max[num][0])
                /(min_max[num][1] - min_max[num][0])*height);
    if (simulate_flag && num < 5) {
            y_old[num] = sm_height - margin - rint((bloch_array[num][0] - min_max[num+8][0])
                /(min_max[num+8][1] - min_max[num+8][0])*height);
    }

    start_graphics(SM_WIN_1+num);
    clear_window(SM_WIN_1+num);

    /*****
    * Plot the first five small canvases: amplitude, phase, frequency, real,
    * and imaginary.  If simulate_flag, the first five canvases contain Mx,
    * My, Mz, Mxy, Phase. The sixth shows the FFT in both cases.
    *****/
  if (num != 5)  {
    if (simulate_flag) {
	counter = 0;
        for (i=1; i<sim_steps; i++) {
            x_new = margin + (double)i*width/(double)(sim_steps - 1);
                y_new = sm_height - margin - rint((bloch_array[num][i] - min_max[num+8][0])
                    /(min_max[num+8][1] - min_max[num+8][0])*height);
	    data[2*counter] = x_old;
	    data[2*counter+1] = y_old[num];
	    counter++;
            y_old[num] = y_new;
            x_old = x_new;
        }
        data[2*counter] = x_old;
        data[2*counter+1] = y_old[num];
        counter++;
	if (counter != 0) 
	  draw_segments(SM_WIN_1+num,data,counter,CYAN+num);
    }
    else {
	counter = 0;
        for (i=1; i<nsteps && nsteps>1; i++) {
            x_new = margin + (double)i*width/(double)(nsteps - 1);
                if (min_max[num][1] - min_max[num][0] == 0.0)
                    y_new = sm_height/2;
                else
                    y_new = sm_height - margin - rint((pattern[num][i]-min_max[num][0])
                        /(min_max[num][1] - min_max[num][0])*height);
		data[2*counter] = x_old;
		data[2*counter+1] = y_old[num];
		counter++;
                y_old[num] = y_new;
                x_old = x_new;
        }
        data[2*counter] = x_old;
        data[2*counter+1] = y_old[num];
        counter++;
	if (counter != 0) 
	  draw_segments(SM_WIN_1+num,data,counter,CYAN+num);
    }
    }

    /*****
    *  Plot the sixth small canvas: fft.  This plot is done separately
    *  due to possible difference between "nsteps" and "fft_steps".
    *****/
  if (num == FFT)  {
    counter = 0;
    for (i=1; i<=fft_steps; i++) {
        x_new = margin + (double)i*width/(double)fft_steps;
        if (min_max[FFT][1] - min_max[FFT][0] == 0.0)
            y_new = sm_height/2;
        else
            y_new = sm_height
                - (margin + (pattern[FFT][i]-min_max[FFT][0])
                /(min_max[FFT][1] - min_max[FFT][0])*height);
	data[2*counter] = x_old;
	data[2*counter+1] = y_old[num];
	counter++;
        y_old[FFT] = y_new;
        x_old = x_new;
    }
    data[2*counter] = x_old;
    data[2*counter+1] = y_old[num];
    counter++;
    if (counter != 0) 
	  draw_segments(SM_WIN_1+num,data,counter,CYAN+num);
    }
  flush_graphics(SM_WIN_1+num);
}


/*****************************************************************
*  This routine is called when the left mouse button is clicked
*  in any of the six small canvases, displaying that selected in
*  the large center canvas.  When the "FFT" small canvas is
*  selected, and is already displayed, the display will cycle
*  through amp, real, and imaginary representations of the FT.
*  The determination of which small canvas has been selected, and
*  the resulting value to be assigned to lg_state, is somewhat
*  contorted because of the three possible sets of data:  pulse,
*  fourier transform, and simulation.  The values of lg_state for
*  each of the possibilities are:
*
*     pulse:        0  amplitude           3  real
*                   1  phase               4  imaginary
*                   2  frequency
*
*     FFT:          5  amplitude
*                   6  real
*                   7  imaginary
*
*     simulation:   8  Mx                  11  Mxy
*                   9  My                  12  phase
*                  10  Mz
*
******************************************************************/
void do_select_large_state(which, button)
int which, button;
{
    int     i;
    char name[MAXPATHL];

    flag_3d = 0;
    if (button == 2)
      clear_flag = 0;
    else
      clear_flag = 1;

	/****
	* If the small FFT canvas is selected and one of the FFT attributes
	* is already active, cycle to the next FFT attribute.
	****/
        if (lg_state >= FFT  &&  lg_state <= FT_IM  && which == SM_WIN_6) {
            lg_state = 5 + ((lg_state - 4) % 3);
        }
	/****
	* Otherwise, lg_state is just the index of the selected small
	* canvas; +8 if a simulation is currently displayed.
	****/
        else {
            lg_state = which-SM_WIN_1;
            if (lg_state <= 4  &&  simulate_flag) {
                if (!sim_is_current) {
                    frame_message("Error:  Simulation data is not current.",which);
                    return;
                }
                lg_state += 8;
            }
        }
        if (lg_state >= FFT  &&  lg_state <= FT_IM) {
            fft_flag = 1;
	    show_object(MAIN_ZERO_FILL_BUTTON);
        }
        else {
            fft_flag = 0;
	    hide_object(MAIN_ZERO_FILL_BUTTON);
        }
	/****
	* Reset the color of the plot to CYAN; if the clear_flag is
	* not set, set the plot color to match the appropriate small canvas.
	****/
        plot_color = CYAN;
        if (!clear_flag) {
	  plot_color = CYAN + which - SM_WIN_1;
        }
	/****
	* Always reset the scale and reference, then plot the new selection.
	****/
        vscale = 1.0;
        vref = 0.0;
        sprintf(vertical_scale_array, "%-.3f", vscale);
	set_panel_value(PARAM_VERTICAL_SCALE,vertical_scale_array);
        sprintf(vertical_ref_array, "%-.3f", vref);
	set_panel_value(PARAM_VERTICAL_REF,vertical_ref_array);
	set_print_save_filenames();
	plot_large_canvas(0);
        plot_large_canvas(1);
}


/*********************************************************************
* Simulate the response to a pulse based on the bloch equations.
* T1 and T2 are assumed to be inifinite, in which case the bloch
* equations reduce to a 3x3 rotation matrix.  An outer loop increments
* the index to the pulse template, and an inner loop cycles through
* "sim_steps" number of points in the B1 or frequency range.
*********************************************************************/
static void bloch_simulate(int mode)
{
    char     string[32];
    double   b1, b1max, b1mult, beta, theta, increment;
    double   sb, cb, mag, x_1_minus_ct, y_1_minus_ct, one_min_ct;
    double   dt, freq;
    double   mx, my, mz;
    double   *bloch_ptr[3];
    register double   x, y, z, st, ct;
    register int      i, j, steps, sweep_var;

    steps = (mode) ? index_inc : nsteps;

    if (!mode)  {
        set_frame_label(SIMULATE_FRAME,"Bloch simulation in progress.");
	}
    
    sweep_var = (int)get_panel_value(SIMULATE_FORMAT_CYCLE);
    /****
    * Setup to sweep frequency:
    ****/
    if (sweep_var == 0) {
        increment = (freq_end - freq_start)/(double)(sim_steps - 1);
        b1max = B1_max;
        freq = freq_start;
    }
    /****
    * Setup to sweep B1:
    ****/
    else if (sweep_var == 1) {
        increment = (b1_end - b1_start)/(double)(sim_steps - 1);
        freq = frequency;
        b1max = b1_start;
    }
    /****
    * Setup to sweep time:
    ****/
    else if (sweep_var == 2) {
        freq = frequency;
        b1max = B1_max;
    }

    dt = 2.0*pi*pulselength/(double)nsteps;

    if (sweep_var == 2) {

        mx = Mx;
        my = My;
        mz = Mz;

        /*****
        * Step through the pulse template.
        *****/
        for(i=0; i<steps; i++) {

            beta = (phase + pattern[PHASE][i])*pi/180.0;
            sb = sin(beta);
            cb = cos(beta);

            b1 = b1max*pattern[AMP][i]/MAX_AMP;
            mag = sqrt(freq*freq + b1*b1);
            if (mag > 0.0) {
                theta = mag*dt;
                st = sin(theta);
                ct = cos(theta);
		one_min_ct = 1.0 - ct;

                x = b1*cb/mag;
                y = b1*sb/mag;
                z = freq/mag;

                x_1_minus_ct = x*one_min_ct;
                y_1_minus_ct = y*one_min_ct;

                /*****
                * Multiply the magnetization vector by the rotation matrix
                *****/
                bloch_array[0][i] = mx*(x_1_minus_ct*x + ct)
				  + my*(x_1_minus_ct*y + st*z)
		                  + mz*(x_1_minus_ct*z - st*y);

                bloch_array[1][i] = mx*(x_1_minus_ct*y - st*z)
				  + my*(y_1_minus_ct*y + ct)
		                  + mz*(y_1_minus_ct*z + st*x);

                bloch_array[2][i] = mx*(x_1_minus_ct*z + st*y)
				  + my*(y_1_minus_ct*z - st*x)
		                  + mz*(one_min_ct*z*z + ct);
            }
            /*****
            * If mag == 0, the pulse has no effect at this step.
            *****/
            else {
                bloch_array[0][i] = mx;
                bloch_array[1][i] = my;
                bloch_array[2][i] = mz;
            }
            mx = bloch_array[0][i];
            my = bloch_array[1][i];
            mz = bloch_array[2][i];
        }
    }

    else {
        /*****
        * If pulse_index is zero, and simulate_init_cycle is "No", then
	* initialize the magnetization vector at each frequency or b1 value.
        *****/
        bloch_ptr[0] = bloch_array[0];
        bloch_ptr[1] = bloch_array[1];
        bloch_ptr[2] = bloch_array[2];

        if (!pulse_index  &&  !(int)get_panel_value(SIMULATE_INIT_CYCLE))  {
            for (j=0; j<sim_steps; j++) {
                *bloch_ptr[0]++ = Mx;
                *bloch_ptr[1]++ = My;
                *bloch_ptr[2]++ = Mz;
            }
        }

        if (mode == SIM_DELAY) {
            strcpy(string, (char *)get_panel_value(SIMULATE_DELAY_PANEL));
            if (!strcmp(string, "")) {
                frame_message("Error:  Undefined parameter value.", SIMULATE_DELAY_PANEL);
                set_frame_label(SIMULATE_FRAME, simulate_frame_label);
	        return;
            }
            sscanf(string, "%lf", &dt);
	    dt = 2.0*pi*dt/1000.0;
	    steps = 1;
	}

        /*****
        * Step through the pulse template.
        *****/
        for(i=0; i<steps && pulse_index<nsteps; i++, pulse_index++) {
            if (interrupt)  {
                set_frame_label(SIMULATE_FRAME, simulate_frame_label);
                return;
		}

            bloch_ptr[0] = bloch_array[0];
            bloch_ptr[1] = bloch_array[1];
            bloch_ptr[2] = bloch_array[2];

            beta = (phase + pattern[PHASE][pulse_index])*pi/180.0;
            sb = sin(beta);
            cb = cos(beta);

            b1mult = pattern[AMP][pulse_index]/MAX_AMP;

            if (sweep_var == 0)
                freq = freq_start;
            else if (sweep_var == 1)
                b1max = b1_start;

            if (!mode  &&  !(i % 50)) {
                sprintf(sim_param_array[8], "%d", pulse_index);
		set_panel_value(SIMULATE_PARAMETER_PANEL_8,sim_param_array[8]);
            }

	    if (mode == SIM_DELAY) {
		b1mult = 0.0;
		pulse_index--;
	    }

            /*****
            * Step through either the B1, or frequency range.
            *****/
            for (j=0; j<sim_steps; j++) {

                mx = *bloch_ptr[0];
                my = *bloch_ptr[1];
                mz = *bloch_ptr[2];

                b1 = b1mult*b1max;
                mag = sqrt(freq*freq + b1*b1);
                if (mag > 0.0) {
                    theta = mag*dt;
                    st = sin(theta);
                    ct = cos(theta);
		    one_min_ct = 1.0 - ct;

                    x = b1*cb/mag;
                    y = b1*sb/mag;
                    z = freq/mag;

                    x_1_minus_ct = x*one_min_ct;
                    y_1_minus_ct = y*one_min_ct;

                    /*****
                    * Multiply the magnetization vector by the rotation matrix
                    *****/
                    *bloch_ptr[0]++ = mx*(x_1_minus_ct*x + ct)
				    + my*(x_1_minus_ct*y + st*z)
		                    + mz*(x_1_minus_ct*z - st*y);
    
                    *bloch_ptr[1]++ = mx*(x_1_minus_ct*y - st*z)
				    + my*(y_1_minus_ct*y + ct)
		                    + mz*(y_1_minus_ct*z + st*x);

                    *bloch_ptr[2]++ = mx*(x_1_minus_ct*z + st*y)
				    + my*(y_1_minus_ct*z - st*x)
		                    + mz*(one_min_ct*z*z + ct);
                }
                /*****
                * If mag == 0, the pulse has no effect at this step.
                *****/
                else {
                    bloch_ptr[0]++;
                    bloch_ptr[1]++;
                    bloch_ptr[2]++;
                }

                if (sweep_var == 0)
                    freq += increment;
                else if (sweep_var == 1)
                    b1max += increment;
            } 
        }
    }

    /*****
    * Fill in the amplitude and phase elements of bloch_array.
    *****/
    for (j=0; j<sim_steps; j++) {
        bloch_array[3][j] = sqrt(bloch_array[0][j]*bloch_array[0][j]
            + bloch_array[1][j]*bloch_array[1][j]);

        if (bloch_array[1][j] == 0.0)
            bloch_array[4][j] = 0.0;
        else
            bloch_array[4][j] =
                180.0/pi*atan2(bloch_array[0][j],bloch_array[1][j]);
    }
    /*****
    * If the end of the pulse template has been reached:
    *****/
    if (pulse_index == nsteps)
        pulse_index = 0;

    sprintf(sim_param_array[8], "%d", pulse_index);
    set_panel_value(SIMULATE_PARAMETER_PANEL_8, sim_param_array[8]);

    if (!mode)
        set_frame_label(SIMULATE_FRAME, simulate_frame_label);
}


/*********************************************************************
*  Find the minima and maxima in amplitude, phase, 
*  real, imaginary, frequency, and fft data.
*********************************************************************/
static void find_min_max()
{
    int      i, j;
    double   ft_max=0.0;

    for (i=1; i<3; i++) {
        min_max[i][0] = 1e8;
        min_max[i][1] = -1e8;
    }
    /*****
    * Phase and frequency require a search over "nsteps"
    *****/
    for (i=0; i<nsteps; i++) {
        for (j=1; j<3; j++) {
            min_max[j][0] = (pattern[j][i] < min_max[j][0]) ? 
                pattern[j][i] : min_max[j][0];
            min_max[j][1] = (pattern[j][i] > min_max[j][1]) ? 
                pattern[j][i] : min_max[j][1];
        }
    }
    /*****
    * Fourier transfrom requires a search over "fft_steps".
    *****/
    for (i=0; i<fft_steps; i++) {
        ft_max = (pattern[FFT][i] > ft_max) ? pattern[FFT][i] : ft_max;
    }
    /*****
    * Normalize FT amplitude, real, and imaginary to 1.
    *****/
    for (i=0; i<=fft_steps; i++) {
        pattern[FFT][i] /= ft_max;
        pattern[FT_RE][i] /= ft_max;
        pattern[FT_IM][i] /= ft_max;
    }
}


/*********************************************************************
*  Fill in the pattern array with real, imaginary, and frequency.
*********************************************************************/
static void calc_real_imag_freq()
{
    int     i;
    double  freq_old, freq_new;

    pattern[REAL][0] = pattern[AMP][0]*cos(pi/180.0*pattern[PHASE][0]);
    pattern[IMAG][0] = pattern[AMP][0]*sin(pi/180.0*pattern[PHASE][0]);
    freq_old = pattern[PHASE][1] - pattern[PHASE][0];

    for (i=1; i<nsteps; i++) {
        pattern[REAL][i] = pattern[AMP][i]*cos(pi/180.0*pattern[PHASE][i]);
        pattern[IMAG][i] = pattern[AMP][i]*sin(pi/180.0*pattern[PHASE][i]);

        freq_new = pattern[PHASE][i] - pattern[PHASE][i-1];
        if (fabs(freq_new - freq_old) > 30.0
          &&  fabs(freq_new) - fabs(freq_old) > 20.0) {
            pattern[FREQ][i-1] = freq_old/pulselength*(double)nsteps/360.0;
        }
        else {
            pattern[FREQ][i-1] = freq_new/pulselength*(double)nsteps/360.0;
            freq_old = freq_new;
        }
    }

    /*****
    *  Frequency at each step requires two values, giving (nsteps - 1) 
    *  frequency points.  The last point is set to the value of the
    *  second-to-last point.
    *****/
    if (nsteps > 1)
        pattern[FREQ][nsteps-1] = pattern[FREQ][nsteps-2];
}


/*********************************************************************
*  Calculate the fourier transform of the pattern.
*********************************************************************/
static void fft_calc()
{
    int      i, j, mmax, m, istep, n;
    double   wr, wi, wpr, wpi, wtemp, tempr, tempi, theta;
    double  *fft_array;

    fft_array = (double *)malloc((2 + 2*(int)pow(2.0,(double)fft_index))*sizeof(double));
    if (fft_array == 0) {
        frame_message("Malloc failure: Unable to complete FFT.", MAIN_SIMULATE_BUTTON);
        return;
    }

    /*****
    * Transfer real and imaginary pulse data to the "fft_array" array,
    * which is used for the discrete transform, and holds the results.
    * This is base 2 FFT, so that "fft_steps" may be greater than "nsteps".
    *****/

    fft_steps = (int)(pow(2.0,(double)fft_index));
    for (i=0; i<nsteps; i++) {
        fft_array[2*i+1] = pattern[REAL][i];
        fft_array[2*i+2] = pattern[IMAG][i];
    }
    for (i=(2*nsteps+1); i<(2*fft_steps+2); i++) {
        fft_array[i] = 0.0;
    }
    n = 2*fft_steps;
    j = 1;

    for (i=1; i<=n; i+=2) {
        if (j > i) {
            tempr = fft_array[j];
            tempi = fft_array[j+1];
            fft_array[j] = fft_array[i];
            fft_array[j+1] = fft_array[i+1];
            fft_array[i] = tempr;
            fft_array[i+1] = tempi;
        }
        m = n/2;
        while (m >= 2  &&  j > m) {
            j = j - m;
            m = m/2;
        }
        j = j + m;
    }
    mmax = 2;
    while (n > mmax) {
        istep = 2*mmax;
        theta = 2.0*pi/(double)mmax;
        wpr = -2.0*sin(theta/2.0)*sin(theta/2.0);
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m=1; m<=mmax; m+=2) {
            for (i=m; i<=n; i+=istep) {
                j = i + mmax;
                tempr = wr*fft_array[j] - wi*fft_array[j+1];
                tempi = wr*fft_array[j+1] + wi*fft_array[j];
                fft_array[j] = fft_array[i] - tempr;
                fft_array[j+1] = fft_array[i+1] - tempi;
                fft_array[i] = fft_array[i] + tempr;
                fft_array[i+1] = fft_array[i+1] + tempi;
            }
            wtemp = wr;
            wr = wr*wpr - wi*wpi + wr;
            wi = wi*wpr + wtemp*wpi + wi;
        }
        mmax = istep;
    }
    for (i=0; i<fft_steps; i++) {
        pattern[FT_RE][i] = fft_array[(2*i+fft_steps)%(2*fft_steps)+1];
        pattern[FT_IM][i] = fft_array[(2*i+fft_steps)%(2*fft_steps)+2];
        pattern[FFT][i] = sqrt(pattern[FT_RE][i]*pattern[FT_RE][i]
            + pattern[FT_IM][i]*pattern[FT_IM][i]);
    }
    pattern[FT_RE][fft_steps] = pattern[FT_RE][0];
    pattern[FT_IM][fft_steps] = pattern[FT_IM][0];
    pattern[FFT][fft_steps] = pattern[FFT][0];

    sprintf(fourier_size_array, "%-d", fft_steps);
    set_panel_value(PARAM_FOURIER_SIZE, fourier_size_array);
    free(fft_array);
}


/*****************************************************************
******************************************************************/
void do_simulate_3d_proc()
{
        flag_3d = 1;
	clear_canvas(2);
	clear_canvas(5);
	set_print_save_filenames();

        /*****
        * Draw corners around plot area.
        *****/
	normal_mode(LG_WIN);
        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN-1,
            LEFT_MARGIN+40, TOP_MARGIN-1, BLUE);
        draw_line(LG_WIN, LEFT_MARGIN+pl_width-40, TOP_MARGIN-1,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN-1, BLUE);

        draw_line(LG_WIN, LEFT_MARGIN+pl_width+2, TOP_MARGIN-1,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN+40, BLUE);
        draw_line(LG_WIN,LEFT_MARGIN+pl_width+2,TOP_MARGIN+pl_height-40,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN+pl_height+2, BLUE);

        draw_line(LG_WIN,LEFT_MARGIN+pl_width-40,TOP_MARGIN+pl_height+2,
            LEFT_MARGIN+pl_width+2, TOP_MARGIN+pl_height+2, BLUE);
        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN+pl_height+2,
            LEFT_MARGIN+40, TOP_MARGIN+pl_height+2, BLUE);

        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN+pl_height-40,
            LEFT_MARGIN-1, TOP_MARGIN+pl_height+2, BLUE);
        draw_line(LG_WIN, LEFT_MARGIN-1, TOP_MARGIN-1,
            LEFT_MARGIN-1, TOP_MARGIN+40, BLUE);

	polar_display(0,0,0,0,0,0,0);
}


/*****************************************************************
******************************************************************/
static void polar_display(int button, int down, int up, int drag, int x, int y, int source_identifier)
{
    char           label[32], string[32];
    int            i, j, k, x1, x2, y1, y2, time;

    static int     phi=40, theta=20, x_old, y_old, ready_flag=0;

    double         intensity, radius, x3d, y3d, z3d, xprime, yprime, zprime,
	           x2d, y2d, trig_values[4];

    if (source_identifier == 1) {

	if (drag)  {
	      if ((x_old == x  &&  y_old == y) || !ready_flag)
	          return;

              if (button == 1) {
	          theta = theta + /*y_old - y*/y-y_old;
	          theta = (theta > 180) ? 180 : theta;
	          theta = (theta < -180) ? -180 : theta;
	          phi = phi + x - x_old;
	          phi = (phi > 360) ? phi - 720 : phi;
	          phi = (phi < -360) ? phi + 720 : phi;
                  x_old = x;
                  y_old = y;
	      }
              else if (button == 2) {
	          phi = phi + x - x_old;
	          phi = (phi > 360) ? phi - 720 : phi;
	          phi = (phi < -360) ? phi + 720 : phi;
                  x_old = x;
	      }
              else if (button == 3) {
	          theta = theta + /*y_old - y*/y-y_old;
	          theta = (theta > 180) ? 180 : theta;
	          theta = (theta < -180) ? -180 : theta;
                  y_old = y;
	      }

	    }
	else if (button > 0)  {
		if (down)  {
		  x_old = x;
		  y_old = y;
		  ready_flag = 1;
		  }
                else  {
		  ready_flag = 0;
		  }
	    }
        }

    radius = 0.95*(double)pl_height/2.0;
    trig_values[0] = sin((double)phi/2.0*pi/180.0);
    trig_values[1] = cos((double)phi/2.0*pi/180.0);
    trig_values[2] = sin((double)theta/2.0*pi/180.0);
    trig_values[3] = cos((double)theta/2.0*pi/180.0);

  if (!plot_flag)  {
    start_graphics(PLOT_WIN);
    clear_canvas(4);

    normal_mode(PLOT_WIN);
    draw_string(PLOT_WIN,30,pl_height-50,"Theta:",YELLOW);
    sprintf(label, "%-.1f", (double)theta/2.0);
    if (!plot_flag)
      draw_inverse_string(PLOT_WIN,88,pl_height-50,label,YELLOW);
    else
      draw_string(PLOT_WIN,88,pl_height-50,label,YELLOW);
    draw_string(PLOT_WIN,48,pl_height-20,"Phi:",YELLOW);
    sprintf(label, "%-.1f", (double)phi/2.0);
    if (!plot_flag)
      draw_inverse_string(PLOT_WIN,88,pl_height-20,label, YELLOW);
    else
      draw_string(PLOT_WIN,88,pl_height-20,label, YELLOW);
    }

    /****
    * Draw the three coordinate axes.
    ****/
    project_3d_point(trig_values, -radius, 0.0, 0.0, &x2d, &y2d, &intensity);
    x1 = pl_width/2 + (int)x2d;
    y1 = pl_height/2 - (int)y2d;
    project_3d_point(trig_values, radius, 0.0, 0.0, &x2d, &y2d, &intensity);
    x2 = pl_width/2 + (int)x2d;
    y2 = pl_height/2 - (int)y2d;
    draw_line(PLOT_WIN,x1,y1,x2,y2,RED);

    project_3d_point(trig_values, 0.0, -radius, 0.0, &x2d, &y2d, &intensity);
    x1 = pl_width/2 + (int)x2d;
    y1 = pl_height/2 - (int)y2d;
    project_3d_point(trig_values, 0.0, radius, 0.0, &x2d, &y2d, &intensity);
    x2 = pl_width/2 + (int)x2d;
    y2 = pl_height/2 - (int)y2d;
    draw_line(PLOT_WIN,x1,y1,x2,y2,RED);

    project_3d_point(trig_values, 0.0, 0.0, -radius, &x2d, &y2d, &intensity);
    x1 = pl_width/2 + (int)x2d;
    y1 = pl_height/2 - (int)y2d;
    project_3d_point(trig_values, 0.0, 0.0, radius, &x2d, &y2d, &intensity);
    x2 = pl_width/2 + (int)x2d;
    y2 = pl_height/2 - (int)y2d;
    draw_line(PLOT_WIN,x1,y1,x2,y2,ORANGE);

    /****
    * Draw the three coordinate axes.
    ****/
    if (!source_identifier || (button > 0 && up)) {
        for (i=0; i<180; i++) {
            x3d = radius*cos((double)i*2.0*pi/180.0);
            y3d = 0.0;
            z3d = radius*sin((double)i*2.0*pi/180.0);
            project_3d_point(trig_values, x3d, y3d, z3d, &x2d, &y2d, &intensity);
            x1 = pl_width/2 + (int)x2d;
            y1 = pl_height/2 - (int)y2d;
	    draw_point(PLOT_WIN,x1,y1,ORANGE);

            x3d = 0.0;
            y3d = radius*cos((double)i*2.0*pi/180.0);
            z3d = radius*sin((double)i*2.0*pi/180.0);
            project_3d_point(trig_values, x3d, y3d, z3d, &x2d, &y2d, &intensity);
            x1 = pl_width/2 + (int)x2d;
            y1 = pl_height/2 - (int)y2d;
	    draw_point(PLOT_WIN,x1,y1,ORANGE);

            x3d = radius*cos((double)i*2.0*pi/180.0);
            y3d = radius*sin((double)i*2.0*pi/180.0);
            z3d = 0.0;
            project_3d_point(trig_values, x3d, y3d, z3d, &x2d, &y2d, &intensity);
            x1 = pl_width/2 + (int)x2d;
            y1 = pl_height/2 - (int)y2d;
	    draw_point(PLOT_WIN,x1,y1,RED);
        }

	/****
	* Insert a time delay between vectors to give the appearance of 
	* the magnetization progressing in time.  If the time parameter
	* is zero, batching is left on until the data has been plotted,
	* otherwise, each vector is displayed as it is drawn to achieve
	* the affect.
	****/
	strcpy(string, (char *)get_panel_value(SIMULATE_TIME_PANEL));
	if (string[0] == '\0') {
	    time = 0;
	    set_panel_value(SIMULATE_TIME_PANEL,"0");
	}
        else
            sscanf(string, "%d", &time);

        project_3d_point(trig_values, Mx, My, Mz, &x2d, &y2d, &intensity);
        x1 = pl_width/2 + (int)(radius*x2d);
        y1 = pl_height/2 - (int)(radius*y2d);

        for (i=0; i<nsteps; i++) {
	    project_3d_point(trig_values, bloch_array[0][i], bloch_array[1][i],
	        bloch_array[2][i], &x2d, &y2d, &intensity);
            x2 = pl_width/2 + (int)(radius*x2d);
            y2 = pl_height/2 - (int)(radius*y2d);
	    intensity = 23.0 + 7.0*intensity;
	    draw_line(PLOT_WIN,x1,y1,x2,y2,(int)intensity);
	    x1 = x2;
	    y1 = y2;

	    if (time > 0)  {
	      flush_graphics(PLOT_WIN);
	      start_graphics(PLOT_WIN);
	      }
	    /****
	    * "time" is used here as the multiplier for delay between
	    * successive vectors.
	    ****/
	    for (j=0; j<time*1e2; j++) ;
        }
    }
  if (!plot_flag)
    flush_graphics(PLOT_WIN);
}


/*****************************************************************
******************************************************************/
static void project_3d_point(double trig_values[], double x3d, double y3d, double z3d,
                             double *x2d, double *y2d, double *intensity)
{
    double   xprime, yprime, zprime;

    xprime = x3d*trig_values[1] + y3d*trig_values[0];
    yprime = y3d*trig_values[1] - x3d*trig_values[0];
    zprime = z3d;

    *x2d = xprime;
    *y2d = zprime*trig_values[3] - yprime*trig_values[2];
    *intensity = zprime*trig_values[2] + yprime*trig_values[3];
}

void draw_line(which, x1, y1, x2, y2, color)
int which, x1, y1, x2, y2, color;
{
    void win_draw_line();

    if (!plot_flag)  {
      win_draw_line(which, x1, y1, x2, y2, color);
     }
    else  {
      if (which == PLOT_WIN)
	plot_par->draw_line(plot_par, x1+LEFT_MARGIN, y1+TOP_MARGIN,
			x2+LEFT_MARGIN, y2+TOP_MARGIN);
      else
	plot_par->draw_line(plot_par, x1, y1, x2, y2);
      }
}

void draw_segments(which, data, num, color)
int which;
int *data;
int num, color;
{
    int i;

    if (!plot_flag)
      win_draw_segments(which, data, num, color);
    else  {
      if (which == PLOT_WIN)
	for (i=0;i<num;i++)  {
	  data[2*i] += LEFT_MARGIN;
	  data[2*i+1] += TOP_MARGIN;
	  }
      plot_par->draw_segments(plot_par, data, num);
      }
}

void draw_point(which, x1, y1, color)
int which, x1, y1, color;
{
    if (!plot_flag)
      win_draw_point(which, x1, y1, color);
    else  {
      if (which == PLOT_WIN)
        plot_par->draw_point(plot_par, x1+LEFT_MARGIN, y1+TOP_MARGIN);
      else
        plot_par->draw_point(plot_par, x1, y1);
      }
}

void draw_points(which, data, num, color)
int which;
int *data;
int num, color;
{
    int i;

    if (!plot_flag)
      win_draw_points(which, data, num, color);
    else  {
      if (which == PLOT_WIN)
	for (i=0;i<num;i++)  {
	  data[2*i] += LEFT_MARGIN;
	  data[2*i+1] += TOP_MARGIN;
	  }
      plot_par->draw_points(plot_par, data, num);
      }
}

void draw_string(which, x, y, str, color)
int which, x, y;
char *str;
int color;
{
    if (!plot_flag)
      win_draw_string(which, x, y, str, color);
    else  {
      if (which == PLOT_WIN)
	plot_par->draw_string(plot_par, x+LEFT_MARGIN, y+TOP_MARGIN, str);
      else
	plot_par->draw_string(plot_par, x, y, str);
      }
}

init_plot_par(plot_par,which)
ppar_struct *plot_par;
int which;
{
    int ps_draw_line(), ps_draw_segments(), ps_draw_point(), ps_draw_points(),
	ps_draw_string();
    plot_par->which = which;
    switch (which)  {
      case PS :
      default :
		plot_par->ppmm = PS_PPMM;
		plot_par->draw_line = ps_draw_line;
		plot_par->draw_segments = ps_draw_segments;
		plot_par->draw_point = ps_draw_point;
		plot_par->draw_points = ps_draw_points;
		plot_par->draw_string = ps_draw_string;
		break;
      }
    plot_par->width = PLOT_WIDTH*plot_par->ppmm;
    plot_par->height = PLOT_HEIGHT*plot_par->ppmm;
    plot_par->x_offset = PLOT_XOFFSET*plot_par->ppmm;
    plot_par->y_offset = PLOT_YOFFSET*plot_par->ppmm;
}

start_plot(plot_par)
ppar_struct *plot_par;
{
 switch (plot_par->which)  {
   case PS :
   default :
     /* first say hello  and move to lower left corner */
     fprintf(plot_par->file,"%%!\n%% PostScript File\n");
     fprintf(plot_par->file,"gsave\n");
     /* these defines shorten the plot file substantially */
     fprintf(plot_par->file,"/mv { moveto } def\n");
     fprintf(plot_par->file,"/ls { lineto stroke } def\n");
     fprintf(plot_par->file, "/bx { exch dup 0 rlineto\n");
     fprintf(plot_par->file, "exch 0 exch rlineto\n");
     fprintf(plot_par->file, "neg 0 rlineto closepath fill } def\n");
     fprintf(plot_par->file,
       "/Courier-Bold findfont %d scalefont setfont\n",(int)(3.88*plot_par->ppmm+0.5));
     /* Courier is mono-spaced - necessary for Vnmr */
     /* 540 72 translate 90 rotate imports correctly */
     fprintf(plot_par->file,"540 72 translate\n 90 rotate\n");
     fprintf(plot_par->file,"%6.4f %6.4f scale\n",2.8346/plot_par->ppmm,2.8346/plot_par->ppmm);
     fprintf(plot_par->file,"0 setlinewidth\n");
     break;
   }
}

ps_draw_line(plot_par,x1,y1,x2,y2)
ppar_struct *plot_par;
int x1,y1,x2,y2;
{ 
  x1 = (x1*plot_par->width)/lg_width+plot_par->x_offset;
  x2 = (x2*plot_par->width)/lg_width+plot_par->x_offset;
  y1 = plot_par->height-((y1*plot_par->height)/lg_height+plot_par->y_offset);
  y2 = plot_par->height-((y2*plot_par->height)/lg_height+plot_par->y_offset);
  fprintf(plot_par->file,"%d %d mv\n%d %d ls\n",x1,y1,x2,y2);
}

ps_draw_segments(plot_par,data,num)
ppar_struct *plot_par;
int *data, num;
{ 
    int i;

    for (i=0;i<num-1;i++)  {
      ps_draw_line(plot_par,*(data+2*i),*(data+2*i+1),
		*(data+2*(i+1)),*(data+2*(i+1)+1));
      }
}

ps_draw_point(plot_par,x1,y1)
ppar_struct *plot_par;
int x1,y1;
{ 
  x1 = (x1*plot_par->width)/lg_width+plot_par->x_offset;
  y1 = plot_par->height-((y1*plot_par->height)/lg_height+plot_par->y_offset);
  fprintf(plot_par->file,"%d %d mv %d %d bx\n",x1,y1,2,2);
}

ps_draw_points(plot_par,data,num)
ppar_struct *plot_par;
int *data, num;
{ 
    int i;

    for (i=0;i<num;i++)  {
      ps_draw_point(plot_par,*(data+2*i),*(data+2*i+1));
      }
}

ps_draw_string(plot_par,x,y,str)
ppar_struct *plot_par;
int x,y;
char *str;
{
   x = (x*plot_par->width)/lg_width+plot_par->x_offset;
   y = plot_par->height-((y*plot_par->height)/lg_height+plot_par->y_offset);
   fprintf(plot_par->file,"%d %d mv\n(%s) show\n",x,y,str);
}

end_plot(plot_par)
ppar_struct *plot_par;
{
 switch (plot_par->which)  {
   case PS :
   default :
     fprintf(plot_par->file,"%% plotpage \nshowpage grestore\n");
     break;
   }
}

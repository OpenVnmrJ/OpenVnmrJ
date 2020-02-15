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
 This is the child program of pulsetool.c, and controls everything
 having to do with directory listings, files, and editing of files.
 The on-line help manual is also contained in and controlled by this
 program.
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/signal.h>

#include "pulsechild.h"

#define MAX_ENTRIES        800
#define MAX_ENTRY_LENGTH   100
#define CHAR_GAP 4

#define MAX_WIDTH        800

#define FILES_WIN_ROWS	15   /* number of text rows in the file browser win */
#define FILES_WIN_COLS	81   /* number of columns in the file browser win */

#define BUTTON_GAP	5

typedef struct {
	int		canvas_width,	/* canvas width in pixels */
	    		canvas_height,	/* canvas height in pixels */
	    		no_rows,	/* num. rows of filenames on canvas */
	    		no_cols,	/* num. columns of filenames of canvas*/
	    		col_width,	/* width of each column in pixels */
	    		row_height,	/* height of each row in pixels */
			sel_col,	/*column index of highlighted filename*/
			sel_row,	/* row index of highlighted filename */
	    		scroll_start,	/* index of row of filenames which is
					   written on top row of canvas */
	    		scroll_end,	/* index of row of filenames which is
					   written on bottom row of canvas */
	    		char_width,	/* average width of character in font */
	    		char_height,	/* average width of character in font */
			num_entries;    /* number of filenames in directory */
	char		directory_name[256],
                        directory_list[MAX_ENTRIES][MAX_ENTRY_LENGTH];
	} files_struct;

files_struct *files;

int			char_height=0,
			char_width=0,
			char_ascent=0,
			char_descent=0;

extern void		file_control(),
                        quit_proc(),
                        help_proc(),
			directory_event_proc(),
    			draw_inverse_string(),
    			draw_string(),
                        display_directory_list(),
			base_frame_event_proc();

void do_display_directory_list();
static void screen_message(char *string, int which);
static void init_help_window();
static void init_cancel_window();
static void cancel_notify_proc();
static void init_files_window();
static void get_directory_entries();
static void reset_files_win();

static char		pattern_file_name[80] = "sinc",
			*help_text[5][150] = {

/* Text for the help utility;  maximum number of characters = 76 */
{"OVERVIEW\n",
 "\n",
 "  Pulsetool is a utility designed to display and examine shaped r.f. pulses.\n",
 "  The standard pulse template file format is: \"phase  amplitude  time-count\",\n",
 "  where phase is in degrees, amplitude is a value between 0 and 1023, and\n",
 "  time-count is an integer which describes the relative time duration of the\n",
 "  step.  The general features of this program are:\n",
 "\n",
 "  The amplitude and phase are displayed in the small windows at the top of\n",
 "  the display, along with the effective frequency of the pulse, the\n",
 "  quadrature components of the pulse, and its Fourier transform.  The\n",
 "  contents of any of the smaller windows may be selected for display in the\n",
 "  large graphics window in the center of the screen.\n",
 "\n",
 "  Between the small windows at the top of the display and the large central\n",
 "  graphics window is the \"control panel\", home to a number of buttons which\n",
 "  perform various operations, or activate the routines described below.\n", 
 "\n",
 "  Below the main graphics window is a panel which contains miscellaneous\n",
 "  information about the current pulse and display status.  The \"Directory\",\n",
 "  \"File name\", \"Pulse length\", \"Vertical scale\" and \"Vertical reference\"\n",
 "  fields display current information which may be altered by the user.  The\n",
 "  \"Steps\", \"Fourier size\", \"Power factor\" and \"Integral\" fields are advisory\n",
 "  only, and may not be entered or changed by the user.  \"Power factor\" is\n",
 "  calculated when a pulse is loaded, and is the mean square amplitude of the\n",
 "  pulse.  A square pulse has a power factor of 1.  The \"Integral\" of the\n",
 "  pulse is an attempt to calculate the tip-angle per unit time and B1 field\n",
 "  strength.  This number is strictly valid only for pulses which are\n",
 "  modulated in amplitude only, and may be used to determine the B1 field\n",
 "  required to obtain, for example, a 90 degree tip with a Sinc pulse.  To do\n",
 "  this, just divide the desired tip angle (in revolutions) by the product of\n",
 "  the integral value and the pulse length (in msec).  The result will be the\n",
 "  required B1 field strength, in kHz.\n",
 "\n",
 "  The directory system may be viewed, and pulse files selected for loading\n",
 "  through the use of the \"Files\" button.\n",
 "\n",
 "  Simulation of the actual reponse to a pulse, based on Bloch equation\n",
 "  calculations, is available by selecting the \"Simulation\" button.\n",
 "\n",
 "  A number of \"standard\" pulses may be created, with attributes tailored\n",
 "  through the \"Create\" utility.\n",
 "\n",
 "  The data currently displayed in the main graphics window may be saved with\n",
 "  the \"Save\" button or written out to a file for printing on a PostScript\n",
 "  printer with the \"Print\" button.  The default filename for the Save or\n",
 "  Print operation will be displayed in the main button panel and may be\n",
 "  changed by typing over it.  When the \"Save\" or \"Print\" button is pushed a\n",
 "  second time, the specified file is created in the directory from which the\n",
 "  pulsetool program was started.  If an absolute pathname (starting with a \"/\")\n",
 "  precedes the filename, then the Save or Print operation writes to this\n",
 "  file instead.  Absolute pathnames are not updated automatically to reflect\n",
 "  changes in the contents of the main plot window.\n",
 "END"},

{"DIRECTORY AND FILE OPERATIONS\n",
 "\n",
 "File selection:\n",
 "\n",
 "  Both the working directory, and the pulse template file name may be\n",
 "  specified by direct entry into the \"Directory\" and \"Pulse name\" fields\n",
 "  found in the panel at the bottom of the display (use the <delete> key\n",
 "  to erase characters, if necessary, and type in the desired name, followed\n",
 "  by a <return> to indicate completion).  When the \"Pulse name\" field is\n",
 "  selected, a <return> will cause the named file to be loaded and displayed.\n",
 "\n",
 "  Alternatively, the \"Files\" button causes a pop-up window to be displayed,\n",
 "  listing the contents of the current directory.  A trailing \"/\" following\n",
 "  a member of the list indicates a sub-directory, and an \"*\" an executable\n",
 "  file.  The \"Load\", \"Chdir\", and \"Edit\" buttons operate on an item\n",
 "  selected from this listing with the left mouse button:\n",
 "\n",
 "  The \"Load\" button causes the selected file to be read, and displayed in\n",
 "  the graphics windows.  If the file does not correspond to the proper\n",
 "  format for pulse template files, an error message will be displayed.\n",
 "  Comment lines beginning with the character \"#\" are ignored.  Descriptive\n",
 "  information about the pulse is displayed in the bottom panel:  the name of\n",
 "  the file; the number of steps in the pulse; the Fourier size required to\n",
 "  do the FFT of the pulse; and a \"power factor\" calculated for the pulse,\n",
 "  based on the mean square amplitude of the pulse.\n",
 "\n",
 "  The \"Chdir\" button changes to and then lists the selected directory.\n",
 "\n",
/* "  The \"Edit\" button allows the selected file to be edited in a pop-up window\n",
 "  using the vi editor.  The \"View Contents\" button which appears along with\n",
 "  the editing window loads a copy of the edit contents at the time as if it\n",
 "  were a standard pulse template file.  The original file is unaffected.\n",
 "  (The editor may exhibit sporadic behavior at times, due to a sunview bug\n",
 "  in 3.5 SunOS.  This should be corrected with the 4.1 operating system.)\n",*/
 "\n",
 "  The \"Parent\" button changes to and then lists the parent of the current\n",
 "  directory.\n",
 "\n",
 "  The \"Save\" button located in the main control panel can be used to save\n",
 "  data currently displayed in the main graphics window to a file.  When this\n",
 "  button is selected, a second button labeled \"Done\" will appear, along with\n",
 "  a type-in field which holds the name of the file which will be created.\n",
 "  First enter an appropriate name, then select the \"Save\" button once again\n",
 "  to write the file.  Once you have entered the \"Save\" mode, you may repeat\n",
 "  this as many times as you like:  display a different attribute in the main\n",
 "  window, enter a new file name, and select \"Save\".  To exit from this mode,\n",
 "  select \"Done\".\n",
 "END"},

{"DISPLAY FEATURES\n",
 "\n",
 "Attribute selection:\n",
 "\n",
 "  The six small graphics windows at the top of the tool will initially\n",
 "  display the six different attributes of the current pulse:  amplitude,\n",
 "  phase, effective off-resonance frequency, real and imaginary quadrature\n",
 "  components, and Fourier transform.\n",
 "\n",
 "  Any of these six windows may be displayed in the large graphics window\n",
 "  by clicking in the appropriate small window with either the left or middle\n",
 "  mouse buttons.  Using the left mouse button causes the large window to be\n",
 "  cleared before drawing, and sets the clear mode to \"on.\"  The middle mouse\n",
 "  button turns off the clear mode and displays the selected attribute,\n",
 "  overlaying any current display in the large graphics window.\n",
 "\n",
 "  Repeated selection of the small Fourier transform window will result in\n",
 "  the large window cycling through the magnitude of the Fourier transform,\n",
 "  the real component, and the imaginary component.\n",
 "\n",
 "Scale and Reference:\n",
 "\n",
 "  The vertical scale may be adjusted either by clicking the middle button\n",
 "  inside the boundary of the large graphics window, or by manually entering\n",
 "  a value in the \"Vertical scale\" field of the bottom panel, ending with\n",
 "  <return>.  Using the middle mouse button causes the scale to be adjusted\n",
 "  interactively so that the active curve passes through the mouse arrow.\n",
 "  Note that no rescaling will occur if the y-value specified with the middle\n",
 "  button does not have the same sign as the actual attribute value at that\n",
 "  point on the x-axis.  A negative value may, however, be entered as a\n",
 "  vertical scale if so desired.\n",
 "\n",
 "  The vertical reference controls the vertical position of the active curve\n",
 "  on the large graphics window, representing the offset from zero measured\n",
 "  in y-axis units.  A positive value moves the curve up, and a negative\n",
 "  value moves it down.  Like the vertical scale, the vertical reference may\n",
 "  be adjusted in one of two ways:  1) a value may be entered manually into\n",
 "  the \"Vertical reference\" field in the bottom panel, or 2) the middle mouse\n",
 "  button can be used interactively anywhere in the large graphics window,\n",
 "  while simultaneously depressing the <Control> key.  In the second case,\n",
 "  the vertical reference will be set so that the curve passes through the\n",
 "  mouse arrow.\n",
 "\n",
 "  The vertical scale and reference are reset whenever an attribute is\n",
 "  selected from any of the small graphics windows.  Use this to extricate\n",
 "  yourself when things get out of hand by re-selecting the current small\n",
 "  window with the left mouse button.\n",
 "\n",
 "Cursors:\n",
 "\n",
 "  Interactive left, right, and horizontal cursors are available, and display\n",
 "  a readout of position at the bottom of the large window when active.  The\n",
 "  left cursor is activated by clicking the left mouse button inside the\n",
 "  large window.  When the left cursor is present, the right cursor may be\n",
 "  activated by clicking the right mouse button anywhere to the right of the\n",
 "  left cursor.  At this point, the right mouse button controls the position\n",
 "  of the right cursor independently, while the left mouse button will move\n",
 "  both cursors in tandem.  When both cursors are active, the control panel\n",
 "  button normally marked \"Full\" will read \"Expand\", and may be used to\n",
 "  display an expanded view of the region selected between the two cursors.\n",
 "  (Note that the clear mode will always be set to \"on\" after an \"Expand\" or\n",
 "  \"Full\" operation.)  The left and right cursors are turned off by clicking\n",
 "  the appropriate mouse button in the large window while simultaneously\n",
 "  depressing the <Control> key.\n",
 "\n",
 "  The horizontal cursor is activated with the \"Thresh\" button located\n",
 "  in the control panel.  When this cursor is active, it is controlled\n",
 "  interactively with the middle mouse button.  The interactive scale and\n",
 "  reference functions normally controlled with the middle mouse are not\n",
 "  available when the horizontal cursor is present.  Select the \"Scale\"\n",
 "  button in the control panel to turn off the horizontal cursor and\n",
 "  reactivate the scale and reference functions (vertical scale and reference\n",
 "  may be adjusted even with the horizontal cursor active by direct entry in\n",
 "  the appropriate fields in the bottom panel).\n",
 "END"},

{"SIMULATION\n",
 "\n",
 "Overview:\n",
 "\n",
 "  This routine simulates the effects of an r.f. pulse by use of the\n",
 "  classical model of nuclear spin evolution described by the Bloch\n",
 "  equations.  T1 and T2 relaxation effects are ignored, in which case the\n",
 "  evolution of a magnetization vector in the presence of an applied r.f.\n",
 "  magnetic field may be evaluated by multiplication with a 3 by 3 rotation\n",
 "  matrix.  The simulation consists simply of repeated multiplication by\n",
 "  such a matrix, whose elements are determined at each step by the values\n",
 "  of amplitude and phase found in the pulse template file, and by user\n",
 "  input values of initial magnetization, B1 field strength, pulse length,\n",
 "  and resonance offset.  The simulation is performed over one of three\n",
 "  possible independent variables:  resonance offset, B1 field strength, or\n",
 "  time, and is determined by the \"Sweep\" cycle in the small button panel.\n",  
 "\n",
 "Parameters:\n",
 "\n",
 "  Select the \"Simulation\" button in the control panel to activate the\n",
 "  Bloch Simulation sub-window.  This window consists of a panel containing\n",
 "  all required parameters (the pulse length is taken from the value in the\n",
 "  bottom panel of the main window), and a small button panel at the bottom\n",
 "  of the window.  To change the value of any of the parameters, select it\n",
 "  with the left mouse button, then delete the appropriate characters and\n",
 "  enter the desired value from the keyboard.  Parameters are updated\n",
 "  each time the \"Go\" button is selected, or when the \"Steps\" button is\n",
 "  selected with \"Index\" equal to zero.\n",
 "\n",
 "  The first three parameters in the left hand column describe the starting\n",
 "  values for the magnetization components Mx, My, and Mz, whose vector sum\n",
 "  must be less than or equal to one.  The next three fields change to\n",
 "  reflect the state of the \"Sweep\" cycle, which may toggled between \"B1\",\n",
 "  \"Freq\" and \"Time\".  When \"Freq\" is selected, the first of these fields\n",
 "  will read \"B1max\", the value of B1 at the maximum pulse amplitude.  The\n",
 "  second and third fields determine the lower and upper off-resonance\n",
 "  frequency boundaries.  When the \"Sweep\" cycle is set to \"B1\", these\n",
 "  three fields are reversed so that the first determines a constant\n",
 "  off-resonance frequency, and the remaining two determine the lower and\n",
 "  upper boundaries of the maximum B1 amplitude.  Selecting \"Time\" will\n",
 "  yield a display of the magnetization as a function of progression\n",
 "  through the pulse, at the frequency and B1 field strength specified by\n",
 "  the parameter values displayed.  In this last case, the number of steps\n",
 "  in the simulation is taken from the number of points in the pulse\n",
 "  template, and may not be altered externally.  To get finer resolution in\n",
 "  the simulation, use a pulse template with a greater number of steps.\n",
 "\n",
 "  The \"Initialize\" cycle determines if the magnetization is re-initialized\n",
 "  to the values of Mx, My, and Mz, or if the simulation uses the values at\n",
 "  each point which were the result of the previous simulation.  In this way,\n",
 "  the effect of a series of pulses may be evaluated by loading the pulse and\n",
 "  performing the simulation with \"Initialize\" set to \"Yes\", then loading the\n",
 "  next pulse, setting \"Initialize\" to \"No\", and selecting \"Go\".  Any number\n",
 "  of pulses may be concatenated in this fashion.  This feature works only\n",
 "  for \"Frequency\" and \"B1\" sweep, but not \"Time\".\n",
 "\n",
 "  When \"Time\" sweep is selected, the results may also be displayed in the\n",
 "  form of a projected 3-dimensional coordinate system, showing the path\n",
 "  of the magnetization over the course of the pulse.  This display is\n",
 "  obtained by selecting the \"3D\" button after first selecting the \"Go\"\n",
 "  button.  When the 3D display is active, the left mouse button will\n",
 "  control the viewing angle from withing the canvas region delineated by\n",
 "  the blue corner markers.  This viewing angle is described by the two\n",
 "  parameters \"phi\" (the amount of rotation about the Z-axis) and \"theta\"\n",
 "  (the declination relative to the XY-plane).  A \"family\" of trajectories\n",
 "  may be displayed by first selecting any of the small canvases with the\n",
 "  middle mouse button, then selecting the \"3D\" button.  Changing either\n",
 "  the B1 field strength, or the resonance offset followed by the \"Go\"\n",
 "  button will result in display of the result without clearing the\n",
 "  display.  To reactivate the automatic clearing feature, select any of\n",
 "  the small canvases with the left mouse button.  To see the 3D display\n",
 "  drawn in real time, enter a non-zero integer value in the \"Time\" field.\n",
 "  The appropriate value will depend on the number of steps in the pulse, and\n",
 "  the type of computer you have.  Try something like 100 for a Sparc 1.\n",
 "\n",
 "  The last parameter in this column determines the number of points at which\n",
 "  the simulation will be performed along the y-axis.  A larger number will\n",
 "  give more detail in the result, but will require proportionally more time\n",
 "  to complete.\n",
 "\n",
/* "  The \"Droop\" parameter is useful for exploring the effects of non-ideal\n",
 "  amplifier behavior.  This parameter determines the percentage decrease in\n",
 "  amplitude at the end of a pulse, and is modeled as a linear decrease\n",
 "  over the entire pulse.  A 10 percent droop, e.g., means that B1max drops\n",
 "  linearly from 100% at t=0 to 90% at the end of the pulse.  The maximum\n",
 "  allowed value is 100%.\n",
 "\n",*/
 "  The \"Index\" parameter is a counter which updates the status of the\n",
 "  simulation, and cannot be set externally.  The value displayed is the\n",
 "  number of steps in the pulse template which have been completed.\n",
 "\n",
 "  The \"Step Inc\" parameter is used by the \"Step\" button (see below) to\n",
 "  control the number of intermediate steps to be calculated.\n",
 "\n",
 "Performing a simulation:\n",
 "\n",
 "  When you have adjusted all of the parameters to your liking, you will\n",
 "  probably want to select the \"Go\" button.  This will do the simulation\n",
 "  calculations, and then display the results in the first five small\n",
 "  graphics windows, replacing (but not destroying!) the pulse information\n",
 "  which was displayed there.  The Fourier transform information remains\n",
 "  unaffected, so that comparisons may be made between this and the exact\n",
 "  simulation results.\n",
 "\n",
 "  All of the display functions which are described elsewhere are active as\n",
 "  well with the simulation data.  Additionally, the original pulse data is\n",
 "  still present in the background, and may be swapped into view with the\n",
 "  \"Display\" cycle found in the main control panel.\n",
 "\n",
 "  The \"Step\" button offers the ability to view the course of the\n",
 "  magnetization at intermediate stages through the pulse.  When this\n",
 "  function is selected,  the next \"Steps Inc\" steps of the pulse will be\n",
 "  simulated, starting at the current value of \"Index\".  The intermediate\n",
 "  result will then be displayed in the normal fashion.\n",
 "\n",
 "  During a \"Go\" simulation, a small panel containing a \"Cancel\" button\n",
 "  will pop into view.  Use this to kill the simulation if necessary (there\n",
 "  may be some delay between hitting the button and the end of the process;\n",
 "  it won't do any good to \"Cancel\" more than once).\n",
 "END"},

{"CREATE\n",
 "\n",
 "Overview:\n",
 "\n",
 "  The pulse creation routine currently offers the following pulse types:\n",
 "\n",
 "     Square                Sinc                 Gaussian\n",
 "     Hermite 90            Hermite 180          Hyperbolic secant inversion\n",
 "     Tan swept inversion   Sin/cos 90\n",
 "\n",
 "  A file containing the pulse template for any of these pulses, suitable for\n",
 "  use with the S.I.S. spectrometer, may be created from scratch with this\n",
 "  utility.  Alternatively, pulses may be created for examination only, using\n",
 "  the display capabilities of Pulsetool.  Each pulse is generated with user\n",
 "  definable parameters appropriate for the pulse in question.\n",
 "\n",
 "Creating a pulse:\n",
 "\n",
 "  When the \"Create\" button is selected, a menu of pulse types will appear.\n",
 "  Hold the right mouse button down in the \"Create\" button, select one of the\n",
 "  pulses in the resulting menu, and release the mouse button.  If you\n",
 "  decide you don't like any of the possibilities, just move the mouse arrow\n",
 "  out of the menu area, and release the button.  When a pulse type is\n",
 "  selected, a small window will appear with a brief description of the\n",
 "  characteristics of the pulse, and a set of changeable attributes whose\n",
 "  values you may alter if so desired.  The number of steps in the pulse is\n",
 "  limited to powers of 2, and may be set by clicking the left mouse button,\n",
 "  or by holding the right mouse button down and selecting the desired value\n",
 "  from the resulting menu.  All other attributes, which will vary depending\n",
 "  on the pulse type, may be altered from their default values by first\n",
 "  selecting the appropriate field with the left mouse button, deleting with\n",
 "  the <Delete> key, and typing in the desired value (a <Return> is not\n",
 "  required).\n",
 "\n",
 "  At this point, you may select one of the three buttons at the bottom of\n",
 "  the window:  \"Done\", \"Preview\", or \"Execute\":\n",
 "\n",
 "  Preview:  uses the attributes as they appear on the screen to create a\n",
 "  pulse which is loaded internally into Pulsetool.  All Pulsetool features\n",
 "  may then be used to examine and evaluate the new pulse.  Any previous\n",
 "  pulse information is deleted.\n",
 "\n",
 "  Execute:  uses the attributes as they appear on the screen to create a\n",
 "  pulse, which is written to a standard Unix file.  The name of the file is\n",
 "  taken from the \"File name\" field in the Create window, and written into\n",
 "  the current directory, listed in the \"Directory\" field in the bottom\n",
 "  panel.  If a file of the same name already exists, you will be asked to\n",
 "  confirm your request.  If, for any reason, the program is unable to write\n",
 "  to the named file, a flashing message will appear.  This is generally\n",
 "  symptomatic of not having write permission in the current directory.\n",
 "\n",
 "  At present, there is no convenient way for a user to add new pulse types\n",
 "  to those listed above.  We are, however, amenable to suggestions for those\n",
 "  pulse types which should be included in the future!\n",
 "END"} };


int              files_flag=0;

/*************************************************************************
*  main():  
*************************************************************************/
main(argc, argv)
int  argc;
char *argv[];
{
    static int   i;
    static char *font_name;
    void send_to_parent();

    files = (files_struct *)malloc(sizeof(files_struct));
    init_window(argc,argv);
    setup_font(&font_name);
    setup_colormap();

    create_frame_with_event_proc(BASE_FRAME,NULL,"",40,200,1,1,base_frame_event_proc);
    hide_object(BASE_FRAME);

    init_files_window();
    init_help_window();
    init_cancel_window();
    send_to_parent("childok");
    window_start_loop(BASE_FRAME);
    exit(0);
}

/*****************************************************************
*  Send "string" to the parent pulsetool, via the pipe at fd 1.
*****************************************************************/
void send_to_parent(char *string)
{
    int  return_val;
    char str[512];

    strcpy(str,string);
    strcat(str, "\0");
    return_val = write(1, str, strlen(str));
}

/*****************************************************************
*	Display notice in notice box.
*****************************************************************/
static void screen_message(char *string, int which)
{
    
    notice_prompt_ok(which, string);
}

/*****************************************************************
*  Initialize the help window.  This is a frame, a panel to 
*  display the text, and a button panel at the bottom.  The
*  message items which hold the text are created in help_proc.
*****************************************************************/
static void init_help_window()
{
    extern int row(), column();
    char *strs[5];
    int tot_width;
 
    create_frame(HELP_FRAME,BASE_FRAME,"Help",150,45,50,50);
    hide_object(HELP_FRAME);

/*
 *  The following statement and the TEXTSW_FILE argument in the call to
 *  xv_create cause the text window to use a disk file as the backing
 *  storage instead of memory.  This prevents the "insertion failed;
 *  memory full"  message from appearing if large files are displayed
 *  in the text window.
 */

    system("(umask 0; cat /dev/null > /tmp/pthelp)\n");
    create_sw(HELP_SW,HELP_FRAME,23,81,"/tmp/pthelp");
    create_panel(HELP_BUTTON_PANEL,HELP_FRAME,0,0,
			column(HELP_FRAME,81),row(HELP_FRAME,4));
    set_win_below(HELP_BUTTON_PANEL,HELP_SW);
    create_button(HELP_DONE_BUTTON,HELP_BUTTON_PANEL,BUTTON_GAP,
			row(HELP_BUTTON_PANEL,0)+3,
			"Done",help_proc);
    strs[0] = "Overview"; strs[1] = "Files";
    strs[2] = "Display"; strs[3] = "Simulation";
    strs[4] = "Create";
    create_choice_button(HELP_CHOICE_BUTTONS,HELP_BUTTON_PANEL,
			2*BUTTON_GAP+object_width(HELP_DONE_BUTTON),
			row(HELP_BUTTON_PANEL,0)-1,
			"",5,strs,help_proc);

    fit_frame(HELP_FRAME);
}


/*****************************************************************
*  This routine displays a window containing help information
*  about all of the Pulsetool capabilities.  It is initially
*  called from pipe_reader, and then by either the "Done" button
*  in the help window, or by one of the help choices in the 
*  window.  Each time a new help selection is made, the old 
*  message items used to display the previous text are destroyed,
*  and new ones created for the new text.  Each block of text
*  must end with the line "END".
*****************************************************************/
void do_help_proc(obj, choice)
int obj, choice;
{
    int   i,j;

    if (obj == HELP_DONE_BUTTON) {
        hide_object(HELP_FRAME);
    }
    else if (obj == HELP_CHOICE_BUTTONS) {
	/****
	* Now create just enough message items to hold the text
	* for the new help choice.
	****/
 	i = 0;
        while (strcmp(help_text[choice][i], "END")) { i++; }
	show_text(HELP_SW,i,help_text[choice]);
	show_object(HELP_FRAME);
        }
}

/*****************************************************************
* Initialize the window containing the "CANCEL" button, used
* when simulating a pulse to stop the simulation.  
*****************************************************************/
static void init_cancel_window()
{
    create_frame(CANCEL_FRAME,BASE_FRAME,"",850,180,80,40);
    hide_frame_label(CANCEL_FRAME);
    hide_object(CANCEL_FRAME);
    create_panel(CANCEL_BUTTON_PANEL,CANCEL_FRAME,0,0,80,40);
    create_button(CANCEL_BUTTON,CANCEL_BUTTON_PANEL,
			column(CANCEL_BUTTON_PANEL,0),
			row(CANCEL_BUTTON_PANEL,0)+4,"Cancel",
			cancel_notify_proc);
    fit_frame(CANCEL_FRAME);
}


/*****************************************************************
*  When a pulse simulation is in progress, a "Cancel" button is
*  displayed, and controlled by pulsechild.  This routine sends
*  signal to the parent to stop the simulation when the Cancel
*  button is pressed.
*****************************************************************/
static void cancel_notify_proc()
{
    kill(getppid(), SIGUSR1);
    hide_object(CANCEL_FRAME);
}

/*****************************************************************
*  This routine initializes the window which is used to display
*  the contents of the current directory.  A control panel
*  is also created below the main panel, and is the home of
*  the buttons which allow changing directories, loading a
*  pulse template file, and editing or viewing a selected file.
*****************************************************************/
static void init_files_window()
{
    int tot_width;

    files->char_width = char_width;
    files->char_height = char_height;
    files->canvas_width = FILES_WIN_COLS*files->char_width;
    if (files->canvas_width > MAX_WIDTH)
      files->canvas_width = MAX_WIDTH;
    files->canvas_height = (FILES_WIN_ROWS+2)*(files->char_height+CHAR_GAP)+
				CHAR_GAP;
    files->num_entries = 0;
    create_frame(FILES_FRAME,BASE_FRAME,"Directory Assistance",60,40,
			files->canvas_width,files->canvas_height);
/* create files window and canvas */
    create_canvas(FILES_CANVAS,FILES_FRAME,0,0,files->canvas_width,
				files->canvas_height,display_directory_list,
				directory_event_proc);
 
/* initialize files canvas variables */
    files->scroll_start = files->scroll_end = 0;
    files->sel_col = files->sel_row = 0;
    
/* create files button panel and buttons below files canvas */
    create_panel(FILES_BUTTON_PANEL,FILES_FRAME,0,files->canvas_height,
				files->canvas_width,30);
    set_win_below(FILES_BUTTON_PANEL,FILES_CANVAS);
    tot_width = BUTTON_GAP;
    create_button(FILES_DONE_BUTTON,FILES_BUTTON_PANEL,tot_width,
			row(FILES_BUTTON_PANEL,0)-1,"Done",file_control);
    tot_width += object_width(FILES_DONE_BUTTON) + BUTTON_GAP;
    create_button(FILES_LOAD_BUTTON,FILES_BUTTON_PANEL,tot_width,
			row(FILES_BUTTON_PANEL,0)-1,"Load",file_control);
    tot_width += object_width(FILES_LOAD_BUTTON) + BUTTON_GAP;
    create_button(FILES_CHDIR_BUTTON,FILES_BUTTON_PANEL,tot_width,
			row(FILES_BUTTON_PANEL,0)-1,"Chdir",file_control);
    tot_width += object_width(FILES_CHDIR_BUTTON) + BUTTON_GAP;
    create_button(FILES_PARENT_BUTTON,FILES_BUTTON_PANEL,tot_width,
			row(FILES_BUTTON_PANEL,0)-1,"Parent",file_control);
    tot_width += object_width(FILES_PARENT_BUTTON) + BUTTON_GAP;
    fit_frame(FILES_FRAME);
} 

/*****************************************************************
*  This routine is called by the "Done", "Chdir", "Parent",
*  and "Load" buttons, and also when the "Directory" and
*  "Pulse Name" text items are modified in the parent pulsetool.
*****************************************************************/
void do_file_control(obj)
int   obj;
{
    int          counter=0;
    char         new_name[64], string[512];

    if (obj == FILES_DONE_BUTTON  &&  files_flag) {
        files_flag = 0;
	hide_object(FILES_FRAME);
    }
    else if (obj == FILES_PARENT_BUTTON) { 
        if (chdir("..") == -1) {
	    screen_message("Error:  Unable to change directory.",FILES_PARENT_BUTTON);
	    return;
	}
	get_directory_entries();
	reset_files_win();
        do_display_directory_list();
	strcpy(string, "dir");
	strcat(string, files->directory_name);
	send_to_parent(string);
    }
    else if (obj == FILES_CHDIR_BUTTON) {
	strcpy(new_name, files->directory_list[files->sel_row*files->no_cols+
				files->sel_col]);
	if (new_name[strlen(new_name)-1] != '/') {
	    screen_message("Error:  Not a directory.",FILES_CHDIR_BUTTON);
	    return;
        }
        if (chdir(new_name) == -1) {
	    screen_message("Error:  Unable to change directory.",FILES_CHDIR_BUTTON);
	    return;
	}
	get_directory_entries();
	reset_files_win();
        display_directory_list();
	strcpy(string, "dir");
	strcat(string, files->directory_name);
	send_to_parent(string);
    }
    else if (obj == FILES_LOAD_BUTTON) {
	strcpy(new_name, files->directory_list[files->sel_row*files->no_cols+
				files->sel_col]);
        if (new_name[strlen(new_name)-1] == '/') {
	    screen_message("Error:  Cannot load a directory.",
			   FILES_LOAD_BUTTON);
            return;
        }
	if (new_name[strlen(new_name)-1] == '*') {
	    new_name[strlen(new_name)-1] = '\0';
	}
	strcpy(pattern_file_name, new_name);
	strcpy(string, "load");
	strcat(string, new_name);
	send_to_parent(string);
    }
}

/**********************************************************************
*	process messages from pulsetool
**********************************************************************/
int do_pipe_reader(buf)
char *buf;
{
            if (!strcmp("show", buf)) {
                if (!files_flag) {
                    files_flag = 1;
                    get_directory_entries();
                    reset_files_win();
                    do_display_directory_list();
                }
                show_object(FILES_FRAME);
            }
            else if (!strcmp("quit", buf)) {
                quit_proc();
            }
            else if (!strncmp("dir", buf, 3)) {
                strcpy(files->directory_name, &buf[3]);
                if (chdir(files->directory_name) == -1) {
                    return(1);
                }
                get_directory_entries();
                reset_files_win();
                do_display_directory_list();
            }
            else if (!strncmp("show_cancel", buf, 11)) {
                show_object(CANCEL_FRAME);
            }
            else if (!strcmp("hide_cancel", buf)) {
                hide_object(CANCEL_FRAME);
            }
            else if (!strcmp("help", buf)) {
                do_help_proc(HELP_CHOICE_BUTTONS,
			(int)get_panel_value(HELP_CHOICE_BUTTONS));
            }
            else if (!strcmp("close", buf)) {
                do_file_control(FILES_DONE_BUTTON);
                do_help_proc(HELP_DONE_BUTTON,
			(int)get_panel_value(HELP_CHOICE_BUTTONS));
	    }
}

/************************************************************************
*  Display the directory listing in the "files_panel" popup.
*  Each time this routine is called, "entry_counter" filenames
*  are written row by row to a canvas.  If all rows don't fit
*  on one screen, allows paging forward or back to see all filenames.
*************************************************************************/
void do_display_directory_list()
{
    int     entry_counter=0, s_len, win_cols,
	    i, j, counter=0, max_name_width=0;
    char *top_string = "Click HERE for previous elements";
    char *bottom_string = "Click HERE for more elements";
    int get_string_width();
    
    clear_window(FILES_CANVAS);

    /* count number of filenames */
    while (strcmp(files->directory_list[entry_counter], "")) {
	max_name_width = (get_string_width(files->directory_list[entry_counter])
				> max_name_width)
	    ? get_string_width(files->directory_list[entry_counter]) :
				max_name_width;
        entry_counter++;
    }

    /* set number of columns and rows on the canvas based on the size of 
       the font being used and on the length of the longest filename. */
    files->col_width = 2*files->char_width + max_name_width;
    files->row_height = files->char_height+CHAR_GAP;
    win_cols = files->canvas_width/files->col_width;
    files->no_cols = (entry_counter) > win_cols   ? 
		win_cols : entry_counter;
    if (files->no_cols <= 0)
      files->no_cols = 1;
    files->no_rows = (entry_counter%win_cols)  ?
		entry_counter/win_cols + 1 :
		entry_counter/win_cols;
    if (files->no_rows <= 0)
      files->no_rows = 1;
    files->scroll_end = files->scroll_start + FILES_WIN_ROWS-1;

    /* write bar on top of canvas to "scroll" back a page of filenames,
       if necessary */
    if (files->scroll_start > 0) 
      draw_inverse_string(FILES_CANVAS,files->canvas_width/2-
				(get_string_width(top_string)/2),
				files->row_height,top_string,0);

    for (j=0; j<files->no_rows; j++) {
        for (i=0; i<files->no_cols; i++) {
	    if (j*files->no_cols + i + 1 > entry_counter)
		break;
	    if ((j >= files->scroll_start) && (j <= files->scroll_end))  {
	      if ((files->sel_row == j) && (files->sel_col == i)) {
		draw_inverse_string(FILES_CANVAS,
			CHAR_GAP+(i*files->col_width),
			files->row_height*(j-files->scroll_start+2),
			files->directory_list[counter],0);
		}
	      else  {
		draw_string(FILES_CANVAS,
			CHAR_GAP+(i*files->col_width),
			files->row_height*(j-files->scroll_start+2),
			files->directory_list[counter],0);
	        }
	      }
	    counter++;
        }
    }

    /* write bar on bottom of canvas to "scroll" forward a page of filenames,
       if necessary */
    if (files->scroll_end < files->no_rows) 
      draw_inverse_string(FILES_CANVAS,files->canvas_width/2-
				(get_string_width(top_string)/2),
		(FILES_WIN_ROWS+2)*files->row_height, bottom_string,0);

    /* make sure server actually displays everything on the window */
    flush_graphics();
}

/****************************************************************************
*  Process events in the file browser window.  Only events we need to consider
*  are configuration events and mouse down events.  
*  Call display_directory_list() to redraw the window.
*****************************************************************************/

void do_directory_event_proc(button_down, x, y, resize)
int button_down, x, y, resize;
{
    int row, col, counter;

    if (resize)  {
	files->canvas_width = get_canvas_width(FILES_CANVAS);
	files->canvas_height = get_canvas_height(FILES_CANVAS);
    	do_display_directory_list();
	}
    else if (button_down)  {
	  row = y/files->row_height;
	  col = x/files->col_width;
	  if (row < 0)  {
	    row = 0;
	    }
	  if (col < 0)  {
	    col = 0;
	    }
	  if (row == 0)  {  /* click on top bar to page back */
	    if (files->scroll_start > 0)  {
	      files->scroll_start -= (files->scroll_end -
					files->scroll_start);
	      if (files->scroll_start < 0) files->scroll_start = 0;
    	      do_display_directory_list();
	      }
	    else   return;
	    }
	  else if (row >= FILES_WIN_ROWS+1)  {  /* click on bottom
						bar to page forward */
	    if (files->scroll_end < files->no_rows)  {
	      files->scroll_start += (files->scroll_end -
					files->scroll_start);
	      if (files->scroll_start >= files->no_rows)
		files->scroll_start = files->no_rows - 1;
    	      do_display_directory_list();
	      }
	    else   return;
	    }
	  else if ((row + files->scroll_start > files->no_rows) ||
		   (col >= files->no_cols)) {
		/* click in blank part of canvas - do nothing */
	    return;
	    }
	  else if (col + 1 + (row-1+files->scroll_start)*files->no_cols >
			files->num_entries) {
		/* click in blank part of canvas - do nothing */
	    return;
	    }
	  else  { /* click on a filename - redraw with it highlighted */
	    if ((files->sel_row >= files->scroll_start) &&
		(files->sel_row <= files->scroll_end))  {
	        counter = files->sel_row * files->no_cols + files->sel_col;
                draw_string(FILES_CANVAS,
		      CHAR_GAP+(files->sel_col*files->col_width),
		      files->row_height*(files->sel_row-files->scroll_start+2),
		      files->directory_list[counter], 0);
		}
	    files->sel_row = row - 1 + files->scroll_start;
	    files->sel_col = col;
	    counter = files->sel_row * files->no_cols + files->sel_col;
            draw_inverse_string(FILES_CANVAS,
		CHAR_GAP+(files->sel_col*files->col_width),
		files->row_height*(files->sel_row-files->scroll_start+2),
		files->directory_list[counter],0);
	    }
	  }
    flush_graphics();
}

/****************************************************************************
*   Reset variables for files canvas after change of directory
*****************************************************************************/

static void reset_files_win()
{
	files->sel_col=0;
	files->sel_row=0;
	files->scroll_start = 0;
}

/*****************************************************************
*  Get the names of all entries in the current directory, and 
*  put them in the string array "directory_list".  Entries which
*  are directories are appended with "/", and executable entry
*  names are appended with "*".  The list of names is then
*  sorted alphabetically using "qsort".  The last entry in the
*  list is set to "".  It might be possible to replace some of
*  this routine with "scandir".
*****************************************************************/
static void get_directory_entries()
{
    int      entry_counter=0;
    DIR      *dir_ptr;
    struct dirent   *entry_ptr;
    struct stat     file_info;

    getcwd(files->directory_name, sizeof( files->directory_name ));
    dir_ptr = opendir(files->directory_name);

    while ((entry_ptr = readdir(dir_ptr)) != NULL && entry_counter < MAX_ENTRIES-1) {
	if (!strcmp(entry_ptr->d_name,".") || !strcmp(entry_ptr->d_name,".."))
	    continue;

	if (stat(entry_ptr->d_name, &file_info) == -1) {
           if (errno != ENOENT)
	      screen_message("Directory error:  Can't locate a file listed in directory.",
                              FILES_PARENT_BUTTON);
	    continue;
	}
	if ((file_info.st_mode & S_IFMT) == S_IFDIR)
	    strcat(entry_ptr->d_name, "/");    /* If the entry is a directory */
	else if ((file_info.st_mode & 0111))
	    strcat(entry_ptr->d_name, "*");    /* If the entry is an executable file */

	strcpy (files->directory_list[entry_counter], entry_ptr->d_name);
	entry_counter++;
    }
    if (entry_counter == (MAX_ENTRIES - 1)) {
	screen_message("Directory error:  Maximum number of files exceeded.",FILES_CHDIR_BUTTON);
    }
    files->num_entries = entry_counter;
    closedir(dir_ptr);

    qsort(files->directory_list, entry_counter, 
		sizeof(files->directory_list[0]), strcmp);
    strcpy(files->directory_list[entry_counter], "");
    if (!entry_counter)
	screen_message("Advisory:  Empty directory.",FILES_CHDIR_BUTTON);
}

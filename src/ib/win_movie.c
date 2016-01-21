/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid(){
    return "@(#)win_movie.c 18.1 03/21/08 (c)1991-92 SISCO";
}

//#define DMALLOC	// Uncomment to run with debug_malloc

/************************************************************************
*									*
*  Doug Landau								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Window routines related to the movie function                  	*
*									*
*************************************************************************/
#include "stderr.h"
#include <math.h>
#include <stream.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <sys/time.h>

#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "initstart.h"
#include "convert.h"
#include "process.h"
#include "interrupt.h"
#include "filelist_id.h"
#include "ipgwin.h"
#include "framelist.h"
#ifdef LINUX
#include <strstream>
#else
#include <strstream.h>
#endif
#include "movie.h"

#define DEFAULT_VERT_GAP  30
#define MOVIE_TICK 2


extern void win_print_msg(char *, ...);

Movie_frame *movie_head = 0;
Movie_frame *display_frame ;
Gframe *gframe = NULL ;
extern Gframe *targetframe;
extern int auto_skip_frame;

// Class used to create movie controller
class Win_movie
{

 private:
      Frame frame;	// Parent
      Frame popup;	// Popup frame (subframe)
      static Panel panel;	// panel
      static Panel_item start_stop_item;
      static Panel_item frame_slider;
      static Panel_item delay_item;
      static Panel_item movie_speedometer;
      static Panel_item low_frame_item;
      static Panel_item high_frame_item;
#ifdef SOLARIS
      static struct timespec last_frame_time;
#else
      static struct timeval last_frame_time;
#endif
      static double actual_frame_delay;
      static double movie_lag;
      static void done_proc(Frame);
      static void movie_load(void);
      static void start_stop(Panel_item, int, Event *);
      static void start_playback();
      static void stop_playback();
      static void pause_playback();
      static void get_delay();
      static void movie_tick(int tick);
      static Panel_setting low_frame_set(Panel_item, int, Event*);
      static Panel_setting high_frame_set(Panel_item, int, Event*);
      static void mode_change(Panel_item, int, Event*);
      static void slider_slid(Panel_item, Event*);
      static void show_frame();
      static void update_controls(int nframes);
      static long delay;             // delay between frames (microseconds)
      static int frame_number, number_of_frames;
      static int low_frame, high_frame ;

      static double get_stuff(Movie_frame*);
      static void get_the_frame();
      
   public:
      Win_movie(void);
      ~Win_movie(void);
      void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
      static double get_name(char*, char*, int);
};

// Initialize static class members
Panel Win_movie::panel = 0;
Panel_item Win_movie::start_stop_item = 0;
Panel_item Win_movie::frame_slider = 0;
Panel_item Win_movie::delay_item = 0;
Panel_item Win_movie::movie_speedometer = 0;
Panel_item Win_movie::low_frame_item = 0;
Panel_item Win_movie::high_frame_item = 0;
double Win_movie::actual_frame_delay = 0;
double Win_movie::movie_lag = 0;
long Win_movie::delay = 0;	// delay between frames (microseconds)
int Win_movie::frame_number = 0;
int Win_movie::number_of_frames = 0;
int Win_movie::low_frame = 0;
int Win_movie::high_frame = 0;
#ifdef SOLARIS
struct timespec Win_movie::last_frame_time = {0,0};
#else
struct timeval Win_movie::last_frame_time = {0,0};
#endif

static Win_movie *winmovie=NULL;

enum play_state_type { NOT_PLAYING, PLAYING } ;
enum play_direction_type { FORWARD, BACKWARD } ;
enum playback_mode_type { AUTOMATIC, MANUAL } ;
play_state_type play_state = NOT_PLAYING ;
play_direction_type play_direction = FORWARD ;
playback_mode_type playback_mode = AUTOMATIC ;



/************************************************************************
*                                                                       *
*  Show the window.							*
*									*/
void
winpro_movie_show(void)
{
   if (winmovie == NULL)
      winmovie = new Win_movie;
   else
      winmovie->show_window();
}

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_movie::Win_movie(void)
{
   int xitempos;	// current panel item position
   int yitempos;	// current panel item position
   Panel_item item;	// Panel item
   int xpos, ypos;      // window position
   int name_width;      // width of filename (in pixel)
   int num_names;       // number of filenames showed at one time
   char initname[128];	// init file

   low_frame = 0 ;
   high_frame = 0 ;

   Win_movie::get_the_frame();

   (void)init_get_win_filename(initname);
   // Get the position of the control panel
   if (init_get_val(initname, "WINPRO_MOVIE", "dd", &xpos, &ypos) == NOT_OK)
   {
      xpos = 400;
      ypos = 20;
   }

   frame = xv_create(NULL, FRAME, NULL);

   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
	FRAME_LABEL,	"Movie Control",
	FRAME_DONE_PROC,	&Win_movie::done_proc,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

   xitempos = 150;
   yitempos = 10;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Load Frames ...",
		PANEL_NOTIFY_PROC,	&Win_movie::movie_load,
		NULL);

   xitempos = 20;
   yitempos += (int)(DEFAULT_VERT_GAP * 1.17);
   start_stop_item = xv_create(panel, PANEL_CHOICE,
		    XV_X, xitempos,
		    XV_Y, yitempos,
		    PANEL_CHOICE_STRINGS, "REVERSE", "STOP", "FORWARD", NULL,
		    PANEL_NOTIFY_PROC, (&Win_movie::start_stop),
		    PANEL_VALUE, 1,
		    XV_SHOW, FALSE,
		    NULL);

   xitempos = 10;
   yitempos += (int)(DEFAULT_VERT_GAP * 1.17);
   delay_item = xv_create(panel, PANEL_TEXT,
	     XV_X,		xitempos,
	     XV_Y,		yitempos,
	     PANEL_LABEL_STRING,  "Frames/sec",
	     PANEL_NOTIFY_PROC, NULL,
	     PANEL_VALUE, "10",
	     PANEL_VALUE_DISPLAY_LENGTH, 5,
	     NULL);

   //xitempos = 60;
   xitempos = 10;
   yitempos += DEFAULT_VERT_GAP;
   movie_speedometer = xv_create(panel, PANEL_GAUGE,
				 XV_X, xitempos,
				 XV_Y, yitempos,
				 PANEL_LABEL_STRING, "Speed (%)  ",
				 PANEL_MIN_VALUE, 0,
				 PANEL_MAX_VALUE, 100,
				 PANEL_TICKS, 3,
				 PANEL_GAUGE_WIDTH, 100,
				 PANEL_VALUE, 0,
				 NULL);

/*
   yitempos += DEFAULT_VERT_GAP;
   xitempos += 20;
   item = xv_create(panel, PANEL_MESSAGE,
		    XV_X, xitempos,
		    XV_Y, yitempos,
		    PANEL_LABEL_STRING, "Movie speed (%)",
		    NULL);
*/

   xitempos = 10;
   yitempos += (int)(DEFAULT_VERT_GAP * 1.17);
   item = xv_create(panel, PANEL_MESSAGE,
		    XV_X, xitempos,
		    XV_Y, yitempos,
		    PANEL_LABEL_STRING, "Display frames:",
		    NULL);

   xitempos += 100;
   low_frame_item = xv_create(panel, /*PANEL_TEXT,*/ PANEL_NUMERIC_TEXT,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_LABEL_STRING,  "First",
	     //PANEL_NOTIFY_LEVEL,  PANEL_ALL,
	     PANEL_NOTIFY_PROC, Win_movie::low_frame_set,
	     PANEL_VALUE_DISPLAY_LENGTH, 4,
	     PANEL_MIN_VALUE, 1,
	     PANEL_MAX_VALUE, 1,
	     PANEL_VALUE, 1,
	     NULL);

   xitempos += 2;
   yitempos += DEFAULT_VERT_GAP;
   high_frame_item = xv_create(panel, /*PANEL_TEXT,*/ PANEL_NUMERIC_TEXT,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_LABEL_STRING,  "Last",
	     //PANEL_NOTIFY_LEVEL,  PANEL_ALL,
	     PANEL_NOTIFY_PROC, Win_movie::high_frame_set,
	     PANEL_VALUE_DISPLAY_LENGTH, 4,
	     PANEL_MIN_VALUE, 1,
	     PANEL_MAX_VALUE, 1,
	     PANEL_VALUE, 1,
	     NULL);

   xitempos += -25;
   yitempos += DEFAULT_VERT_GAP;
   frame_slider = xv_create(panel, PANEL_NUMERIC_TEXT, //PANEL_SLIDER,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_LABEL_STRING, "Current",
	     PANEL_MIN_VALUE, 1,
	     PANEL_MAX_VALUE, 1,
	     //PANEL_SLIDER_WIDTH, 150,
	     //PANEL_TICKS, 2,
	     PANEL_VALUE_DISPLAY_LENGTH, 3,
	     PANEL_NOTIFY_PROC, &Win_movie::slider_slid,
	     XV_SHOW, FALSE,
	     NULL);
   
   window_fit(panel);
   window_fit(popup);
   window_fit(frame);

   xv_set(popup, XV_SHOW, TRUE, NULL);

   (void)init_get_win_filename(initname);

   // Get the initialized file-browser window position and its size
   if (init_get_val(initname, "FILE_BROWSER", "dddd", &xpos, &ypos,
       &name_width, &num_names) == NOT_OK)
   {   
      // Default
      xpos = ypos = 0;
      name_width = 0;
      num_names = 0;
   }  

   // Create frame-browser
   if (framelist_win_create(frame, xpos, ypos, name_width,
       num_names)  == NOT_OK)
   {
      STDERR("window_create_frame_browser:framelist_win_create");
      exit(1);
   }

   framelist_notify_func ((long)&Win_movie::get_name);
}


/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_movie::~Win_movie(void)
{
   framelist_win_destroy () ;
   xv_destroy_safe(frame);
}


/************************************************************************
*                                                                       *
*  Selects the frame in which to display                                *
*                                                                       */
void
Win_movie::get_the_frame(void)
{
   int nth = 0;
   int maxframes = 400 ;    // this should be sufficient 

   do
     gframe=Frame_select::get_selected_frame(++nth) ;
   while ((gframe==NULL) && (nth<maxframes));

   if (nth >= maxframes)
   {
     Frame_routine::FindNextFreeFrame() ;
     gframe = targetframe ;
   }
}

/************************************************************************
*                                                                       *
*  Notifier proceedure for start/stop buttons
*
*/
void
Win_movie::start_stop(Panel_item, int value, Event *)
{
    switch (value){
      case 0:
	play_direction = BACKWARD;
	start_playback();
	break;

      case 1:
	stop_playback();
	break;

      case 2:
	play_direction = FORWARD;
	start_playback();
	break;
    }
}

/************************************************************************
*                                                                       *
*  Movie Playback procedure                   				*
*									*/
void
Win_movie::start_playback(void)
{
   // do nothing if currently playing or if there is no movie
   if (play_state == NOT_PLAYING && movie_head){
       Win_movie::get_delay();
       if (delay <= 0){
	   delay = 10000;	// 10 milliseconds (a short time)
	   xv_set(delay_item, PANEL_VALUE, "", NULL);
       }
       play_state = PLAYING ;
       Win_movie::get_the_frame();

       // Start at the last used movie frame, if possible
       display_frame = movie_head;
       if (frame_number >= number_of_frames){
	   frame_number = 0;
       }
       for (int i=0; i<frame_number; i++){
	   display_frame = display_frame->nextframe;
       }

       // Register the movie callback function
       ipgwin_register_user_func(MOVIE_TICK,
				 (long) (delay / 1000000),
				 (long) (delay % 1000000),
				 (void(*)(int)) &Win_movie::movie_tick);

       // Init our timer
#ifdef SOLARIS
       clock_gettime(CLOCK_REALTIME, &last_frame_time);
#else /* (not) SOLARIS */
       gettimeofday(&last_frame_time, NULL);
#endif /* (not) SOLARIS */
       actual_frame_delay = 1.0e-6 * delay;	// Optimistic initial estimate
       movie_lag = 0.0;

       // Hide the frame selection slider, since we won't update it
       xv_set(frame_slider, XV_SHOW, FALSE, NULL);
   }
}



/************************************************************************
*                                                                       *
*  get_delay                                                            *
*									*/
void 
Win_movie::get_delay(void)
{
   char delay_text[80] ;
   float delay_float ;

   strcpy (delay_text, (char *)xv_get(delay_item, PANEL_VALUE));
   if ( sscanf ( delay_text, "%f", &delay_float ) > 0 ){ 
     delay = (int)(1000000 / delay_float);      // Convert to delay in usec
   }else{
     delay = 0;
   }
}


/************************************************************************
*                                                                       *
*  get_stuff                                       			*
*									*/
double 
Win_movie::get_stuff(Movie_frame *current_frame)
{
   char *pathpart = current_frame->filename;
   int place = strlen(pathpart) - 2;	// Leave off the trailing "/", if any
   while ( (place) && (pathpart[place] != '/') ) // Get past the "file" name
     place--;
   char *filepart = &pathpart[place+1];
   pathpart[place] = NULL ; 

   int old_auto_skip_frame = auto_skip_frame;
   auto_skip_frame = FALSE;
   int err = Frame_routine::load_data (pathpart, filepart);
   auto_skip_frame = old_auto_skip_frame;
   if (err == NOT_OK){
       return (-1.0);
   }

   Win_movie::get_the_frame();
   Frame_data::display_data(gframe,
			    gframe->imginfo->datastx,
			    gframe->imginfo->datasty,
			    gframe->imginfo->datawd,
			    gframe->imginfo->dataht,
			    gframe->imginfo->vs,
			    FALSE);
   attach_imginfo(current_frame->img, targetframe->imginfo);
   return targetframe->imginfo->vs;
}


/************************************************************************
*                                                                       *
*  low_frame_set                                                        *
*									*/
Panel_setting 
Win_movie::low_frame_set(Panel_item item, int, Event *)
{
    int n = (int)xv_get(item, PANEL_VALUE);
    low_frame = n - 1;
    xv_set(high_frame_item, PANEL_MIN_VALUE, n, NULL);
    return (PANEL_NONE) ;
}

/************************************************************************
*                                                                       *
*  high_frame_set                                                        *
*									*/
Panel_setting
Win_movie::high_frame_set(Panel_item item, int, Event *)
{
    int n = (int)xv_get(item, PANEL_VALUE);
    high_frame = n - 1;
    xv_set(low_frame_item, PANEL_MAX_VALUE, n, NULL);
    return (PANEL_NONE) ;
}


/************************************************************************
 *                                                                       *
 *  slider_slid                                                          *
 *									*/
void
Win_movie::slider_slid(Panel_item, Event *)
{
    frame_number = (int)xv_get(frame_slider, PANEL_VALUE) - 1;
    Win_movie::show_frame() ;
}

/************************************************************************
 *                                                                       *
 *  get_name                                                              *
 *									*/
double
Win_movie::get_name(char *s1, char *s2, int where)
{
    Movie_frame *insert_point, *list_point, *newframe ;
    int zip ;
    double the_vs = 0.0;

#ifdef DMALLOC
    cerr << "v1.1 ";
    malloc_verify();
#endif

    if ((strcmp(s1,"delete")==0) && (strcmp(s2,"delete")==0)){
	if ((where<number_of_frames)&&(number_of_frames>0)){
	    insert_point = movie_head ;
	    for (zip = 0 ; zip < where ; zip++ ){
		insert_point = insert_point->nextframe ;
	    }
	    insert_point->prevframe->nextframe = insert_point->nextframe ; 
	    insert_point->nextframe->prevframe = insert_point->prevframe ; 
	    if (where==0){
		movie_head = insert_point->nextframe ;
	    }
	    delete insert_point ;
	    if (number_of_frames==1){       /* if there was only one, */
		movie_head = NULL ;          /* there are none now. */
	    }
	}
    }else{
	char *filename = new char[strlen(s1) + strlen(s2) + 2];
	strcpy(filename, s1);
	strcat(filename, "/");
	strcat(filename, s2);
    
	if (movie_head == NULL){
	    movie_head = new Movie_frame(filename);
	    delete [] filename;
	    if ((the_vs = get_stuff (movie_head)) == -1.0){
		delete movie_head ;
		movie_head = NULL;
		return (-1.0);
	    }else{
		movie_head->nextframe = movie_head ;
		movie_head->prevframe = movie_head ;
	    }
	}else{
	    insert_point = movie_head ;
	    for (zip = 0 ; zip < where ; zip++ ){
		insert_point = insert_point->nextframe ;
	    }
	    newframe = new Movie_frame(filename);
	    delete [] filename;
	    if ((the_vs = get_stuff (newframe)) == -1.0){  
		delete newframe ;
		newframe = NULL; 
		return (-1.0);
	    }else{ 
		newframe->nextframe = insert_point ;
		newframe->prevframe = insert_point->prevframe;
		insert_point->prevframe->nextframe = newframe ;
		insert_point->prevframe = newframe ;
	    }
	    
	    if (where==0){
		movie_head = newframe;
	    }
	}
    }

#ifdef DMALLOC
    cerr << "v1.2 ";
    malloc_verify();
#endif

    number_of_frames = 0 ;
    list_point = movie_head ;
    if (movie_head != NULL){
	do{
	    list_point = list_point->nextframe ;
	    number_of_frames++ ;
	} while (list_point != movie_head);
    }
    
    frame_number = high_frame = number_of_frames - 1 ;

    // If we have more than 1 frame, show frame slider and start/stop buttons

    int slider_show = number_of_frames > 1 ? TRUE : FALSE;
    xv_set(frame_slider,
	   PANEL_MAX_VALUE, number_of_frames,
	   PANEL_VALUE, number_of_frames,
	   XV_SHOW, slider_show,
	   NULL);
    xv_set(start_stop_item, XV_SHOW, slider_show, NULL);

    if (number_of_frames){
	xv_set(high_frame_item,
	       PANEL_VALUE, number_of_frames,
	       PANEL_MAX_VALUE, number_of_frames,
	       NULL);
	xv_set(low_frame_item, PANEL_MAX_VALUE, number_of_frames, NULL);
	int t = (int)xv_get(low_frame_item, PANEL_VALUE);
	if (t <= 0){
	    xv_set(low_frame_item, PANEL_VALUE, 1, NULL);
	}else if (t > number_of_frames){
	    xv_set(low_frame_item, PANEL_VALUE, number_of_frames, NULL);
	}
    }else{
	xv_set(high_frame_item, PANEL_VALUE, 1, NULL);
	xv_set(low_frame_item, PANEL_VALUE, 1, NULL);
    }

#ifdef DMALLOC
    cerr << "v1.3 ";
    malloc_verify();
#endif
    
    return the_vs;
}


/************************************************************************
 *                                                                       *
 *  Stop Movie Playback procedure                   			*
 *									*/
void
Win_movie::stop_playback(void)
{
   if (play_state == PLAYING){       // do nothing if NOT currently playing
       ipgwin_register_user_func(MOVIE_TICK, 1, 0, (void(*)(int)) NULL);
       play_state = NOT_PLAYING;
       if (number_of_frames > 1){
	   xv_set(frame_slider,
		  PANEL_VALUE, frame_number + 1,
		  XV_SHOW, TRUE,
		  NULL);
       }
       framelist_select (frame_number) ;
       xv_set(movie_speedometer, PANEL_VALUE, 0, NULL);
   }
}


/************************************************************************
*                                                                       *
*  Movie Tick Callback                             			*
*									*/
void 
Win_movie::movie_tick(int)
{
    static int tick_count = 0;
    
    if (display_frame && number_of_frames>0){

	// Validate the frame limits (may change while movie running)
	if (high_frame > number_of_frames - 1
	    || low_frame < 0
	    || high_frame < low_frame)
	{
	    if (high_frame > number_of_frames - 1 || high_frame < 0){
		high_frame = number_of_frames - 1;
	    }
	    if (low_frame > number_of_frames - 1 || low_frame < 0){
		low_frame = 0;
	    }
	    if (high_frame < low_frame){
		int t = low_frame;
		low_frame = high_frame;
		high_frame = t;
	    }
	    xv_set(high_frame_item, PANEL_VALUE, high_frame + 1, NULL);
	    xv_set(low_frame_item, PANEL_VALUE, low_frame + 1, NULL);
	}

	if (play_direction == FORWARD){
	    do{
		display_frame = display_frame->nextframe ;  
		frame_number++ ;
		if (frame_number >= number_of_frames)
		frame_number = 0;
	    }while ((frame_number > high_frame) || (frame_number < low_frame));
	}else{
	    do{
		display_frame = display_frame->prevframe ;
		frame_number-- ;
		if (frame_number < 0)
		frame_number = number_of_frames - 1 ;
	    }while ((frame_number > high_frame) || (frame_number < low_frame));
	} 
	
	detach_imginfo(gframe->imginfo);
	attach_imginfo(gframe->imginfo, display_frame->img);
	Frame_data::display_data(gframe,
				 gframe->imginfo->datastx,
				 gframe->imginfo->datasty,
				 gframe->imginfo->datawd,
				 gframe->imginfo->dataht,
				 gframe->imginfo->vs,
				 FALSE);
    }

    // Update the movie timing data
#ifdef SOLARIS
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    double sec_per_frame = (now.tv_sec - last_frame_time.tv_sec
			    + 1.0e-9 * (now.tv_nsec - last_frame_time.tv_nsec));
#else /* (not) SOLARIS */
    struct timeval now;
    gettimeofday(&now, NULL);
    double sec_per_frame = (now.tv_sec - last_frame_time.tv_sec
			    + 1.0e-6 * (now.tv_usec - last_frame_time.tv_usec));
#endif /* (not) SOLARIS */
    actual_frame_delay = 0.9 * actual_frame_delay + 0.1 * sec_per_frame;

    // Update speedometer every so often
    if (now.tv_sec - last_frame_time.tv_sec){
	xv_set(movie_speedometer,
	       PANEL_VALUE, (int)((delay * 1.0e-6 / actual_frame_delay)
				  * 100.0 + 1.5),	/* 1.5 is empirical */
	       NULL);
    }
    
    // Check if we are hopelessly behind and need to restart clock
    movie_lag += sec_per_frame - 1.0e-6 * delay;
    if (movie_lag > 1.0e-6 * delay){
	// Reregister the movie callback function
	ipgwin_register_user_func(MOVIE_TICK,
				  (long) (delay / 1000000),
				  (long) (delay % 1000000),
				  (void(*)(int)) &Win_movie::movie_tick);
	// Re-init the slowness detector
	movie_lag = 0.0;
    }

    last_frame_time = now;
}

/************************************************************************
*                                                                       *
*  Frame_slider callback                           			*
*									*/
void 
Win_movie::show_frame()
{
  int frame_count ;

  if ((movie_head) && (frame_number <= number_of_frames))
  {
    frame_count = 0 ;
    display_frame = movie_head ;
    while (frame_count < frame_number)
    {
      display_frame = display_frame->nextframe ;
      frame_count++ ;
    }

    detach_imginfo(gframe->imginfo);
    attach_imginfo(gframe->imginfo, display_frame->img);

    Frame_data::display_data(gframe,
			     gframe->imginfo->datastx,
			     gframe->imginfo->datasty,
			     gframe->imginfo->datawd,
			     gframe->imginfo->dataht,
			     gframe->imginfo->vs,
			     FALSE);
    framelist_select (frame_number) ;
  } 
}

/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  [STATIC]								*
*									*/
void
Win_movie::done_proc(Frame subframe)
{
  xv_set(subframe, XV_SHOW, FALSE, NULL);
  win_print_msg("Movie: Exit");
}


/************************************************************************
*                                                                       *
*  Win_movie::movie load	                                        *
*   						                        *
*									*/
void
Win_movie::movie_load(void)
{
    filelist_win_show (FILELIST_MOVIE_ID, FILELIST_LOAD_ALL, NULL,
                       "Movie Frame Loader" ) ;

    framelist_win_show ((char *) NULL);
}

/************************************************************************
*                                                                       *
*  in_a_movie(Imginfo *img)
*	If the given image ("img") is in a movie, returns the movie
*	head. Else returns 0.
*
*/
Movie_frame *
in_a_movie(Imginfo *img)
{
    if (img == 0 || movie_head == 0){
	return 0;
    }

    Movie_frame *first = movie_head->nextframe;
    Movie_frame *mframe = first;
    do{
	if (mframe->img == img){
	    return movie_head;
	}
	mframe=mframe->nextframe;
    } while (mframe != first);

    return 0;
}

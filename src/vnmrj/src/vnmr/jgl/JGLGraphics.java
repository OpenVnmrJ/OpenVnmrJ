/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import java.awt.Dimension;
import java.awt.Color;
import java.awt.Cursor;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import java.awt.print.PageFormat;
import java.awt.print.Paper;
import java.awt.print.Printable;
import java.awt.print.PrinterException;
import java.awt.print.PrinterJob;
import java.util.*;
import java.util.concurrent.Semaphore;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.nio.*;
import java.text.DecimalFormat;
import java.io.File;
import java.io.IOException;

import com.jogamp.opengl.util.awt.AWTGLReadBufferUtil;
import com.jogamp.opengl.util.awt.TextRenderer;
import com.jogamp.common.nio.Buffers;

import javax.imageio.ImageIO;
import javax.media.opengl.*;
import javax.media.opengl.awt.GLJPanel;
import javax.media.opengl.glu.GLU;
import javax.print.PrintService;
import javax.swing.BoxLayout;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.BevelBorder;
import javax.swing.filechooser.FileFilter;

import jogamp.opengl.GLContextImpl;
import jogamp.opengl.GLDrawableFactoryImpl;
import vnmr.util.*;

/**
 * @author deans
 *
 * New features and things to fix
 *  TODO: OGL Add xy cursors to 2D projection 
 *      - drawn in model view (not screen axis)
 *      - show xy position text on screen (in model coords) 
 *  TODO: OGL In 2D projection map color based on distance from xy selection point
 *      - should be useful at identifying cross peaks
 *  TODO: OGL Add Selection for 1D->2D (show info on selected point)
 *  TODO: OGL Add Ruler tool (select start - select end) show distance
 *  TODO: OGL Draw Scales
 *  TODO: OGL Show labels for PP (calc in vnmrbg)
 *  TODO: OGL Show integration curves (calc in vnmrbg)
 *  TODO: OGL Add max/min height bands for contour lines
 *  TODO: OGL Add support for false lighting in 3D view
 *      - Use density gradient as normal vector
 * FIXME: OGL fix pivot problem in 1D phase algorithm
 * FIXME: OGL fix shader failure when switching window in and out
 * FIXME: OGL write a better 2D shader for ARBProgram model
 * FIXME: OGL fix row clipping problem in 2D projection
 */
public class JGLGraphics  extends GLJPanel implements GLEventListener, 
    JGLDef, PropertyChangeListener, Printable,
    MouseListener, MouseMotionListener,MouseWheelListener,JGLComListenerIF
    {	
    static final int DFLT_CURSOR = 0;
    static final int CURSOR1 = 1;
    static final int CURSOR2 = 2;
    static final int PHASECURSOR = 3;
    static final int MOVE_CURSOR = 4;
    static final int COMPRESS_CURSOR = 5;
    static final int EXPAND_CURSOR = 6;
    static final int COLOR_CURSOR = 7;
    static final int BIAS_CURSOR = 8;
    static final int SELECT_CURSOR = 9;
    static final int ROTATE_CURSOR = 10;
    static final int XPARANCY_CURSOR = 11;

    static final int POINT3D= 4;

    static final int PREV = 0;
    static final int NEXT = 1;
    static final int DECREASE = 3;
    static final int INCREASE = 4;

    static final int LEFT=1;      // left button
    static final int RIGHT=2;     // right button
    static final int MIDDLE=3;    // middle button

    static final int P0 = 1;
    static final int P1 = 2;
    
    static double dflt_rotx=90;
    static double dflt_roty=0;
    static double dflt_intensity=1.0;
    static double dflt_contrast=1.0;
    static double dflt_threshold=0.1;
    static double dflt_contours=1.0;
    static double delx_max=0.05;
    static double delx_dflt=0.2;
    static double delx_step=0.5;
    static double intensity_step=0.1;
    static double bias_step=0.01;
    static double contrast_step=0.05;   
    
    static boolean range_check=true;

    private int prevMouseX, prevMouseY;
    private int mouseX, mouseY;
    private boolean dragphase=false;
    private boolean ymouse=false;
    private double rotx = dflt_rotx, roty = dflt_roty, rotz = 0.0;
    private double tilt = 0.0, twist = 0.0;
    private double xoffset = 0.0, yoffset = 0.2, zoffset = 0.0;
    private double xcenter=0.0;
    private double ascale=1,xscale=1.0,yscale=1.0,zscale=1.0;
    private double hscale=1.0,wscale=1.0;
    private double aspect=1.0;
    private int width=0;
    private int height=0;
    private static GLCapabilities caps;
    private int render_time=0;
    private int repeat_time=0;
    private long repeat_start=0;
    private double reset_threshold=dflt_threshold;
    
    private long data_read_time=0;
    private long data_xfer_time=0;
    private long tex_build_start=0;
    private long tex_build_end=0;
    private long xfer_start=0;
    
    private static Font fpsFont = new Font("SansSerif", Font.BOLD, 12);
    private static DecimalFormat intformat = new DecimalFormat("###");
    private static DecimalFormat fltformat1 = new DecimalFormat("0.000");
    private static DecimalFormat fltformat2 = new DecimalFormat("0.0");
    private static DecimalFormat fltformat3 = new DecimalFormat("0.####E0");  
    
    private boolean show_info=true;
    private float line_width=1.0f;
    private float point_size=3.0f;

    // JGLdata properties
    JGLDataMgr data=new JGLDataMgr();

    private double dmin=0.0,dmax=1.0;
    private boolean havedata=false;
    private boolean havedatascale=false;
    private static double dflt_spectrum_cursor1=0.05;
    private static double dflt_spectrum_cursor2=1-dflt_spectrum_cursor1;
    private static double dflt_fid_cursor1=0.0;
    private static double dflt_fid_cursor2=0.5;
    private double cursor1=dflt_spectrum_cursor1;
    private double cursor2=dflt_spectrum_cursor2;
    private LinkedList<ZoomState> zoom_state=new LinkedList<ZoomState>();
    private boolean show_cursors=true;

    private double phasecursor1=0.5;
    private float[] ctrcols = null;
    private float[] abscols = null;
    private float[] phscols = null;
    private float[] stdcols = null;
    private float[] grays = null;  
    private int button=0;
    private boolean native_ogl=true;
    private boolean dragging=false;
    private boolean enddrag=false;
    private boolean repeating=false;
    private boolean phase_mode=false;
    private boolean phasing=false;
    private boolean rotating=false;
    private boolean mousemode=false;
    private boolean animating=false;
    private boolean data_selected=false;
    private boolean ctrlkey=false;
    private boolean shiftkey=false;

    private int data_flags=0;
    private int which_cursor=0;
    private int which_tool=0;
    
    private int cursor_type=DFLT_CURSOR;

    private double delx=delx_dflt,dely=1.0;
    private double intensity=1.0;
    private double contrast=1.0;
    private double bias=1.0;
    private double threshold=dflt_threshold;
    private double contours=dflt_contours;
    private double limit=1;
    private double transparency=1.0;
    private double alphascale=1.0;
    private double slab_width=1.0;
    private double reset_width=1.0;
    private double sfactor=1;
    private double sampling_factor=1.0;
    private double rotation_step=1;
    private double dflt_rotation_step=1;

    private int step=1;
    private boolean mousecursor=false;
    private GLRendererIF renderer=null;
    private int step_direction=NEXT;
    private boolean view_invalid=true;
    private boolean ydrag=false;
    private boolean mouse_pressed=false;
    private boolean mouse_clicked=false;
    private boolean mouse_wheel=false;
    private boolean mouse_event=false;

    private boolean cursor_set_mode=false;    
    private boolean selection_mode=false;    

    private int show=0;
    private int status=0;
    private int prefs=0;
    private int laststatus=0;
    private int projection=-1;
    private int sliceplane=X;
    private boolean initialized=false;

    String gl_vendor=null;
    String gl_renderer=null;
    String gl_version=null;       
    Point3D minCoord = new Point3D(0.0,-0.5,0.0);
    Point3D maxCoord = new Point3D(1.0,0.5,1.0);   
    Point3D selpt=new Point3D();
    Point3D delpt=new Point3D();
    Point3D mouse=new Point3D();
    Point3D oldmouse=new Point3D();
    Point3D selproj=new Point3D();
    Point3D seldata=new Point3D();
    double selvalue=0;
    Point3D dproj=new Point3D();
    Point3D initms=new Point3D();
    Point3D delms=new Point3D();
    Point3D svect=new Point3D(0.5,0,0);
    Point3D mvect=new Point3D();
    Point3D vproj=new Point3D();
    Point3D vpnt=new Point3D(0.5,0.5,0.5,0.5);
    Point3D vrot=new Point3D();   
    Point3D rstsvect=svect;
    Point3D rstvpnt=vpnt;   
    Point3D rstvrot=vrot; 
    double svscale=1.0;
    int mslice=0; 
    Volume volume=new Volume(minCoord,maxCoord);  
    JGLView view=new JGLView(volume);
    boolean draw_axis=true;
    static final int BUFSIZE=1024;
    private TextRenderer text3D=null;
    private GL2 gl=null;
    private int psuedo_slices=500;
    private int real_slices=0;
    private int max_slices=0;
    private int max_step=8;
    int reset_mode=RESETALL;
    float slice_xparancy=0.2f;
    float plane_xparancy=0.1f;
    JGLComMgr comMgr;
    private int shader=NONE;
    String axislabel[]=new String[6];
    String dflt_axislabels="+X +Y +Z -X -Y -Z";
    float label_scale=0.75f;
    String version="1.0";
    boolean show_version=false;  
    int render_mode=GL2.GL_RENDER;
    boolean svselect=false;
    boolean newsliceplane=false;
    boolean stepflag=false;
    Point3D xaxis=new Point3D();
    Point3D yaxis=new Point3D();
    Point3D zaxis=new Point3D();
    Point3D span=new Point3D();
    private Semaphore timersem = new Semaphore(1, true);
    private int repaint_ticks=0;
    private int timer_ticks=0;
    private int draw_ticks=0;
    private int skip_ticks=0;
    private long draw_ticks_sum=0;
    private int draw_ticks_ave=0;
    private static final long init_delay=500;
    private static final long repeat_delay=50;
    private java.util.Timer timer=null;
    private java.util.TimerTask animator=null;
    private boolean timer_reset=false;
       
    private static final int MAXDCNTS=4;
    int dcounts[] = new int[MAXDCNTS];
    int dcount=0;
    JGLGraphics graphics=null;
    private boolean imagecontext=false;
    private BufferedImage image=null;
    int image_width=1024,image_height=1024;
    int print_width=1024,print_height=1024;
    double image_scale=2;
    double print_scale=2;
	private String savepath=null;

    
    static {
        caps = new GLCapabilities(GLProfile.get("GL2"));
        caps.setAlphaBits(8);
        //caps.setHardwareAccelerated(true);
        caps.setSampleBuffers(true);
        caps.setNumSamples(4);  // !! WARNING do not set >4 !!
        caps.setDoubleBuffered(true);
        caps.setPBuffer(true);
        //caps.setOnscreen(false);
        //GLProfile.initSingleton();
     }

    //##############  public methods follow ###########################

    public JGLGraphics(JGLComMgr mgr) {
        super(caps, null, null);
        //caps.setDoubleBuffered(true);
        comMgr=mgr;
        native_ogl=false;
        setAxisLabels(dflt_axislabels);
        String option = System.getProperty("glmode");
        if (option != null && option.equals("native")){
            CGLJNI cgljni=new CGLJNI();
            if(!cgljni.libraryLoaded())
                System.out.println("Warning could not load cgl library: running jogl version");
            else{
                native_ogl=true;
                renderer=cgljni;
            }
        }
        if(native_ogl==false){
            renderer=new JGLRenderer();
        }
        addMouseListener(this);
        addMouseMotionListener(this);
        addMouseWheelListener(this);
        addGLEventListener(this);
        graphics=this;
        getColors();
        DisplayOptions.addChangeListener(this);
        //setAutoSwapBufferMode(false);
    } 
    /**
     * free bound resources.
     */
    public void destroy(){
        if(renderer!=null)
            renderer.destroy();
        renderer=null;
    }
    
    /**
     * Set axis labels.
     * @param lstr
     * String format (e.g.)  "+X +Y +Z -X -Y -Z"
     */
    public void setAxisLabels(String lstr){
        StringTokenizer tok=new StringTokenizer(lstr," ,\n");
        int i=0;
        while(tok.hasMoreElements()){
            String label=tok.nextToken();
            axislabel[i++]=label;
        }
    }
    /**
     * @return true if using native opengl renderer
     */
    public boolean nativeGl(){
        return native_ogl;
    }

    /**
     * Show or Hide 3D graphics window.
     * @param value
     */
    public void setShowing(boolean value){
        if(!value){
            comMgr.comCmd(G3DRUN, 0);
            //stopTimer();
        }
        comMgr.comCmd(G3DSHOWING, value?1:0);
    }
    /* (non-Javadoc)
     * @see java.awt.Component#repaint()
     */
    public void repaint(){
        if(!pacingDisplay()){
            super.repaint();
            if(DebugOutput.isSetFor("glrepaint"))
                Messages.postDebug("JGLGraphics.repaint()");
        }
     }
     

    public void repaintDisplay(){
        super.repaint();
        repaint_ticks++;
        if(DebugOutput.isSetFor("glrepaint"))
            Messages.postDebug("JGLGraphics.repaintDisplay()");        
     }

    private boolean sliceTool(){
        if(which_tool>=SEL_SPLANE && which_tool<=SEL_ZPLANE)
            return true;
        return false;
    }
    private boolean axisTool(){
        if(which_tool>=SEL_XAXIS && which_tool<=SEL_ZAXIS)
            return true;
        return false;
    }
    private boolean pacingDisplay(){
        if((status & PACEDISPLAY)==0)
            return false;
        if(data.dim<2) 
            return false;
        if(projection<=TWOD)
            return false;
        if((status & ANIMATEMODE)==ANIMATEROT)
            return false;
        if(animating) 
            return true;
        if(dragging && (sliceTool()|| axisTool()))
            return true;       
        return false;
    }

    private void resetPaceParams(){
        draw_ticks=0;
        timer_ticks=0;
        skip_ticks=0;
        draw_ticks_sum=0;
        draw_ticks_ave=0;
        repaint_ticks=0;
        for(int i=0;i<MAXDCNTS;i++)
            dcounts[i]=0;
        dcount=0;
        timer_reset=true;
    }

    // timer functions
    // private class cmndData
    class timerTask extends java.util.TimerTask {
        public void run() {
            if (animating) {
                mouseEventHandler();
            } else if (dragging && which_tool > 0) {
                if (sliceTool())
                    comMgr.updateVpnt(true);
                else if (axisTool())
                    comMgr.updateVrot(true);
                
            } else if(repeating && !dragging)
                mouseEventHandler();

            if(stepflag && data.dim > 1 && projection > TWOD){
                setVPntFromSlice(true);
                stepflag=false;
            }

            if (pacingDisplay()) {
                timer_ticks++;
                int counts = getTimerSem();
                if (counts == 1) {
                    int index = dcount % MAXDCNTS;
                    dcount++;
                    int n = dcount > MAXDCNTS ? MAXDCNTS : dcount;
                    // When the 3d window stops being shown because of a workspace selection change
                    // the timer continues to run, (because it no longer is part of the GUI thread),
                    // but the window redraw events stop. When the window becomes visible again,
                    // at the first semTake the display takes a very long time to draw (because the
                    // GUI thread thinks that the long delay time was due to normal vnmrbg or java
                    // processing activity).
                    // So far I've not been able to find a listener that reports when the window
                    // redraw thread has been suspended in this situation. As a workaround, the 
                    // problem is minimized by rejecting draw time events that are presumed to be
                    // larger than what could be expected from the normal processing overhead. 
                    if(draw_ticks_ave>0 && !timer_reset && draw_ticks>4*draw_ticks_ave){
                        if (DebugOutput.isSetFor("gltimer"))
                            Messages.postDebug("JGLGraphics.timer TIMER DRAW TIME INVALID");
                        draw_ticks=draw_ticks_ave;
                        timer_ticks=2;
                    }
                    dcounts[index] = draw_ticks;

                    draw_ticks_sum = 0;
                    for (int i = 0; i < n; i++)
                        draw_ticks_sum += dcounts[i];
                    draw_ticks_ave = (int) (draw_ticks_sum / n);
                    int margin = (int) (draw_ticks_ave * 1.0);
                    margin = margin < 1 ? 1 : margin;
                    skip_ticks = draw_ticks_ave;
                    timer_ticks -= 1;
                    repaint_ticks = 0;
                    timer_reset=false;
                } else if (timer_ticks >= skip_ticks) {
                    int n = dcount > MAXDCNTS ? MAXDCNTS : dcount;
                    if (DebugOutput.isSetFor("gltimer"))
                        Messages.postDebug("JGLGraphics.timer TIMER"
                                + " ave:" + draw_ticks_ave + " n:" + n
                                + " skip:" + skip_ticks + " timer:"
                                + timer_ticks + " repaint:" + repaint_ticks);
                    if (repaint_ticks == 0){
                        
                        repaintDisplay();
                    }
                    timer_ticks = 0;
                }
            } else
                repaint();
        }
    }
    
    private synchronized void startTimer(){
        if (DebugOutput.isSetFor("gltimer"))
            Messages.postDebug("JGLGraphics.startTimer()");        

        if(timer==null){
            resetPaceParams();
            timer = new Timer();
            animator=new timerTask();
            clrTimerSem();
            setTimerSem();
            repeat_start=System.nanoTime();
            timer.scheduleAtFixedRate(animator, init_delay,repeat_delay);
        }
    }
    private synchronized void stopTimer(){
        if (DebugOutput.isSetFor("gltimer"))
            Messages.postDebug("JGLGraphics.stopTimer()");        
        if(timer!=null){
             timer.cancel();
             timer.purge();
             animator=null;
             resetPaceParams();
       }
        timer=null;
    }
     private synchronized void setTimerSem(){
        timersem.release();
    }
    private synchronized void clrTimerSem(){
        timersem.drainPermits();
    
    }
    private synchronized int getTimerSem(){
         return timersem.drainPermits();
    }
    private synchronized void setTimerDelay(long d){
        if (DebugOutput.isSetFor("gltimer"))
            Messages.postDebug("JGLGraphics.setTimerDelay "+d);
    }

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        if(isShowing()){   
            getColors();
            //System.out.println("propertyChange");
            repaint();
        }
    }

    private void setButtons(MouseEvent e){
        if((e.getModifiers() & e.BUTTON1_MASK) != 0)
            button=LEFT;
        else if((e.getModifiers() & e.BUTTON3_MASK) != 0)
            button=RIGHT;
        else if((e.getModifiers() & e.BUTTON2_MASK) != 0)
            button=MIDDLE;
        
        ctrlkey=e.isControlDown();
        shiftkey=e.isShiftDown();
    }
     
    // Methods required for the implementation of MouseListener

    public void mouseEntered(MouseEvent e) {}
    public void mouseExited(MouseEvent e) {}
    public void mousePressed(MouseEvent e) {
        prevMouseX = mouseX= e.getX();
        prevMouseY = mouseY= e.getY();
        setButtons(e);
        int x = prevMouseX;
        int y = prevMouseY;

		cursor_set_mode = false;
		selection_mode = false;


		if (ctrlkey && show_cursors && projection<TWOD) {
			cursor_set_mode = true;
		}
		else if (shiftkey && projection<=TWOD && button==LEFT)
			selection_mode=true;
        
        float dy=1.0f-((float)y)/height;
        hscale = 0.5 - dy ;
        wscale  = (width / 2.0f - x) / width;
        
        ymouse=Math.abs(hscale)>Math.abs(wscale);
        which_cursor=0;
        repeating=false;

        if(!shiftkey && !ctrlkey && button==LEFT){
            if(projection <TWOD || phase_mode){
                which_cursor=selectCursor(x);
                if(which_cursor==PHASECURSOR)
                	phasing=true;
                else
                	phasing=false;
            }
            if(ctrlkey && (projection>=OBLIQUE))
            	setCursor(ROTATE_CURSOR);
           // else
            //    mouse_pressed=true;
        }
        mouse_pressed=true;
        if(!animating && !selection_mode && !cursor_set_mode && which_cursor==0 && which_tool==0 && button!=LEFT){
            repeating=true;
            repaint();        
        }
        mouse.x=mouseX;
        mouse.y=height-mouseY;
        mouse.z=0;
        
        initms=new Point3D(mouse);
        oldmouse=new Point3D(mouse);
                
        dragging=false;
        enddrag=false;
        data_flags=0;
        draw_ticks=0;
        //if(projection<SLICES)
            startTimer();       
    }

    public void setCursor(int type) {
    	if(cursor_type != type){
    		switch(type){
    		case DFLT_CURSOR: 
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:DEFAULT"); 
    			setCursor(Cursor.getDefaultCursor()); 
    			break;
    		case MOVE_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:MOVE DATA"); 
    			setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
    			break;
    		case CURSOR1:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:CURSOR SELECT"); 
    			setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
    			break;
    		case CURSOR2:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:CURSOR SELECT"); 
    			setCursor(Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR));
    			break;
    		case EXPAND_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:EXPAND"); 
    			setCursor(Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR));
    	        break;
    		case COMPRESS_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:EXPAND"); 
    			setCursor(Cursor.getPredefinedCursor(Cursor.S_RESIZE_CURSOR));
    	        break;
    		case XPARANCY_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:TRANSPARANCY"); 
    			setCursor(VCursor.getCursor("xparancy_cursor"));
    			break;
    		case COLOR_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:INTENSITY & CONTRAST"); 
    			setCursor(VCursor.getCursor("intensity"));
    			break;
    		case BIAS_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:COLOR BIAS"); 
    			setCursor(VCursor.getCursor("intensity2")); 
    			break;
    		case SELECT_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:DATA SELECT");    	        
    			setCursor(Cursor.getPredefinedCursor(Cursor.CROSSHAIR_CURSOR));
    			break;
    		case ROTATE_CURSOR:
    	        if (DebugOutput.isSetFor("glcursors"))
    	            Messages.postDebug("setCursor:ROTATE"); 
    			//setCursor(Cursor.getPredefinedCursor(Cursor.NW_RESIZE_CURSOR));

    			setCursor(VCursor.getCursor("recycleXbg"));
    			break;
    		}
    		cursor_type=type;
    	}
    }

    // Methods required for the implementation of MouseMotionListener
    
    public void mouseDragged(MouseEvent e) {
        double x = e.getX();
        double y = e.getY();
        double dy = (y - prevMouseY)/height;
        double dx = (x - prevMouseX)/width;
        double ax=Math.abs(dx);
        double ay=Math.abs(dy);
        double dc,cmin,cmax;
        
        mouse.x=x;
        mouse.y=height-y;
        
        if(shiftkey)
        	return;
        ctrlkey=e.isControlDown();
        
        prevMouseX = (int)x;
        prevMouseY = (int)y;

        if(which_cursor > 0) {
            double s = screenToValue(x / width);
            switch(which_cursor) {
            case CURSOR1:
            	cursor2+=(s-cursor1);
            	cursor1=s;
            	dc=cursor2-cursor1;
            	cmin=xoffset;
            	cmax=xscale+xoffset;
            	if(cursor1<cmin){
            		cursor1=cmin;
            		cursor2=cursor1+dc;
            	}
            	if(cursor2>cmax){
            		cursor2=cmax;
            		cursor1=cursor2-dc;
            	}
                break;
            case CURSOR2:
                cursor2 = s;
                if(cursor2<cursor1)
                	cursor2=cursor1;
                cmax=xscale+xoffset;
                if(cursor2>cmax)
                	cursor2=cmax;
                break;
            case PHASECURSOR:
                if(!dragging) {
                    if(ay > ax)
                        dragphase = true;
                    else
                        dragphase = false;
                }
                if(dragphase) {
                    double pstep = 100 * dy / xscale;
                    data.lp += pstep;
                    data.rp -= phasecursor1 * pstep;
                } else
                    phasecursor1 = s;
                break;
            }
            dragging=true;
            repaint();
            return;
        } 
        
        if(!dragging){
            ydrag=(ay>2*ax)?true:false;
            dragging = true;
            delms=mouse.sub(initms);
            setTimerDelay(repeat_delay);
            repaint();
            return;
        }
        if(ydrag && ax>0 && ay> 0 && ax>2*ay){
            ydrag=false;
        }
        else if(!ydrag && ax>0 && ay> 0 && ay>2*ax){
            ydrag=true;
        }

        if(which_tool!=0){
            repaint();
            return;
        }
            
        if(button == LEFT && (!phase_mode || which_cursor == 0)) {
            switch(projection){
            default:
                xoffset -= dx * xscale;
                xoffset = rangeCheck(xoffset);               
                yoffset -= dy * yscale;
                reset_mode=RESETOFFSET;
                setCursor(MOVE_CURSOR);
                break;
            case OBLIQUE:
                if(ydrag)
                	yoffset -= dy * zscale;
                else
	                xoffset -= dx * xscale;               	
                reset_mode=RESETOFFSET;
                setCursor(MOVE_CURSOR);
                break;
            case TWOD:
        		setCursor(DFLT_CURSOR);
	            xoffset -= dx * zscale;
	            xoffset=rangeCheck(xoffset);
	            zoffset -= dy * zscale;            	
                reset_mode=RESETOFFSET;
                setCursor(MOVE_CURSOR);
                break;
            case SLICES:
            case THREED:
                if(ydrag)
                    rotx -= 360*dy;
                else
                    roty -= 360*dx;
                roty=roty%360.0;
                if(roty<0)
                    roty=360+roty;
                rotx=rotx%360.0;
                if(rotx<0)
                    rotx=360+rotx;
                reset_mode=RESETROTATION;
                setCursor(ROTATE_CURSOR);

                break;
            }
        } else if(button == MIDDLE && !ctrlkey) {
            switch(status & MOUSEMODE){
            case TRANSPARENCY:
                if(!mousecursor)
                    setCursor(XPARANCY_CURSOR);
                if(ay>ax){
                    double istep = 1 + 5*ay;
                    if(threshold<=0)
                        threshold=0.01;
                    else if(dy < 0)
                        threshold *= istep;
                    else
                        threshold /= istep;
                    comMgr.sendVFltToVnmr(G3DTHRESHOLD,threshold,true);
                }
                else{
                    double istep = 1.2;
                    if(limit<=0)
                        limit=0.01;
                    if(dx < 0)
                        limit *= istep;
                    else
                        limit /= istep;
                    limit=limit>1?1:limit;
                    limit=limit<threshold?threshold:limit;
                    comMgr.sendVFltToVnmr(G3DLIMIT,limit,true);
                }
                reset_mode=RESETXPARENCY;
                break;
            case RESOLUTION:
                //if(!mousecursor)
                //    setCursor(VCursor.getCursor("rescursor"));
                if(ay>ax){
                    double istep = 1 + 2*ay;
                    if(dy < 0)
                        data.ymax *= istep;
                    else
                        data.ymax /= istep;
                    comMgr.sendVFltToVnmr(G3DDSCALE,data.ymax,true);
                }
                else if(data.dim>1){
                    int max=(int)(max_step/sfactor);
                    step= (int)(x*max/width+1);
                    if(step>max)
                       step=max;
                    if(step<1)
                       step=1;
                    setAlphaScale();
                }
                reset_mode=RESETRESOLUTION;
                break;
            default:
            case INTENSITY:
                if(!mousecursor)
                    setCursor(COLOR_CURSOR);
                
                if(ax>2*ay){
                    double cstep = 1 + 2 * ax;
                    if(dx > 0)
                        contrast *= cstep;
                    else
                        contrast /= cstep;
                    comMgr.sendVFltToVnmr(G3DCONTRAST,contrast,true);
                }
                else{
                    double istep=1+10*ay*intensity_step; 
                    if(dy <0)
                        intensity *=istep;
                    else
                        intensity /=istep;       
                    comMgr.sendVFltToVnmr(G3DINTENSITY,intensity,true);
                }
                reset_mode=RESETINTENSITY;
                break;
            }
            mousemode=true;
            mousecursor = true;
        }
        else if(button == MIDDLE && ctrlkey) {
            switch(status & MOUSEMODE){
            default:
            case INTENSITY:
         		setCursor(BIAS_CURSOR);
        		if(dy > 0)
        			bias += bias_step;
        		else
        			bias-=bias_step;
        		comMgr.sendVFltToVnmr(G3DINTENSITY,bias,true);
                reset_mode=RESETINTENSITY;
            	break;
            }
            mousemode=true;
            mousecursor = true;
        	
        }
        else if(button == RIGHT && !ctrlkey) {
        	 switch(projection){
             case OBLIQUE:
                 if(ydrag){
 	                 dely+=dy;
 	                 dely=dely<-1?-1:dely;
 	                 dely=dely>1?1:dely;    
                 }
                 else{
                     double pstep=dx*delx_step;
                     double pmax=1-delx_max;
                     delx+=pstep;
                     delx=delx>pmax?pmax:delx;
                 }
                 reset_mode=RESETROTATION;
          		 setCursor(ROTATE_CURSOR);
                 break;
             case TWOD:
          		 setCursor(ROTATE_CURSOR);
         		 if(ydrag)
         			 tilt -= 45*dy;
                 else
                     twist -= 45*dx;
                 twist=twist%360.0;
                 if(twist<0)
                 	twist=360+twist;
                 reset_mode=RESETROTATION;
                 break;        	 
        	 }
        }
        repaint();
    }

	public void mouseClicked(MouseEvent e) {
		mouse_clicked = true;
        setButtons(e);
		if (selection_mode)
			repaint();
	}

    public void mouseReleased(MouseEvent e) {
        double x = e.getX();
        double y = e.getY();
        mouse.x=x;
        mouse.y=height-y;
        ctrlkey=e.isControlDown();
        shiftkey=e.isShiftDown();
        setCursor(DFLT_CURSOR);
        if(!animating)
            stopTimer();
        
		//if (!shiftkey && show_single_trace && !dragging && !repeating ){
		if (!selection_mode && projection <TWOD && !dragging && !repeating ){
			if(cursor_set_mode){
				if (button == LEFT)
					cursor1 = screenToValue(x / width);
				else if (button == RIGHT)
					cursor2 = screenToValue(x / width);
				else if (button == MIDDLE && phase_mode)
					phasecursor1 = screenToValue(x / width);
			}
			else if(button == LEFT && show_cursors && which_cursor == 0){
				double s = screenToValue(((double) prevMouseX) / width);
				if (s > cursor1 && s < cursor2)
					zoomIn();
				else
					zoomOut();
			}
		}
 		if(!selection_mode && !phasing && !cursor_set_mode);
			mouseEventHandler();         
        //mouse_pressed=false;
        cursor_set_mode=false;
        which_cursor=0;
        which_tool=0;
        button=0;
        svselect=false;
        if(dragging)
        	enddrag=true;
        dragging=false;
        mousemode=false;
        repeating=false;
        mousecursor=false;
        draw_ticks=0;
        ctrlkey=false;
        rotating=false;
        phasing=false;
        if(!animating)
            setTimerDelay(init_delay);
   }
      
    public void mouseMoved(MouseEvent e) {
    	// check for mouse pass over cursors
    	if(!dragging && which_cursor==0 && projection <=TWOD){
    		int cursor=0;
    		if(show_cursors && projection <TWOD)
    			cursor=selectCursor(e.getX());
    		if(cursor>0)
    			setCursor(cursor);   		
    		else if(e.isShiftDown())
    			setCursor(SELECT_CURSOR);
    		else
    			setCursor(DFLT_CURSOR);
    	}
    }

    //  Methods required for the implementation of MouseWheelListener 
    
	public void mouseWheelMoved(MouseWheelEvent e) {
		int ticks = e.getWheelRotation();
		double pstep = 0;
		double istep;
		switch (projection) {
		case SLICES:
		case THREED:
			istep = 1 + 0.01 * Math.abs(ticks);
			// if(threshold<=0)
			// threshold=0.01;
			if (ticks > 0)
				threshold *= istep;
			else
				threshold /= istep;
			comMgr.sendVFltToVnmr(G3DTHRESHOLD, threshold, true);
			break;
		case TWOD:
			istep = 0.1 * Math.abs(ticks);
			if (ticks > 0)
				contours += istep;
			else
				contours -= istep;
			if (contours < 0)
				contours = 0.0;
			// if(contours>1)
			// contours=1.0;
			reset_mode = RESETCONTOURS;
			comMgr.sendVFltToVnmr(G3DCWIDTH, contours, true);
			break;
		case ONETRACE:
		case OBLIQUE:
			if(ticks>=0)
			    setCursor(COMPRESS_CURSOR);
			else
			    setCursor(EXPAND_CURSOR);
			pstep = 1.0f + 0.5 * Math.abs(hscale);
			pstep = (ticks <= 0) ? pstep : 1.0 / pstep;
			ascale *= pstep;
			break;
		}
		mouse_wheel=true;
		repaint();
	}

    /**
     * Display opengl information in startup console window
     *   use vnmrj -Dglinfo
     */
    public static void showGLInfo(){
        GL2 gl=GLU.getCurrentGL().getGL2();
        if(gl==null)
            return;
        String vendor=gl.glGetString (GL2.GL_VENDOR);
        String renderer=gl.glGetString (GL2.GL_RENDERER);
        String version=gl.glGetString(GL2.GL_VERSION);
        System.out.println("GL_VERSION: "+version);
        System.out.println("Graphics card and driver information:" );
        System.out.println("Vendor: "+vendor+" Renderer: "+renderer);

        String gstr;
        IntBuffer intbuffer=IntBuffer.allocate(1);            
       
        if (GLUtil.isExtensionSupported ("GL_ARB_multitexture")) {
            gl.glGetIntegerv( GL2.GL_MAX_TEXTURE_UNITS,intbuffer);
            System.out.println("Max texture units: "+intbuffer.get(0));
        }
 
        gstr=gl.glGetString(GL2.GL_SHADING_LANGUAGE_VERSION_ARB);
        System.out.println("Shading Language information:");

        System.out.println(" SHADING LANGUAGE VERSION: "+gstr);
        if (GLUtil.isExtensionSupported ("GL_ARB_shader_objects"))
            System.out.println(" Shader Objects: supported");
        else
            System.out.println(" Shader Objects: NOT supported");
        
        if (GLUtil.isExtensionSupported ("GL_EXT_Cg_shader"))
            System.out.println(" CG Shading language: supported");
        else
            System.out.println(" CG Shading language: NOT supported");
        if (GLUtil.isExtensionSupported ("GL_ARB_shading_language_100")){
            System.out.println(" GLSL Shading language: supported");
            GLSLProgram glsl=new GLSLProgram();
            glsl.showGLInfo();
            glsl.destroy();
        }
        else
            System.out.println(" GLSL Shading language: NOT supported");
    }

    /* (non-Javadoc)
     * @see java.awt.Component#setBounds(int, int, int, int)
     * - for some reason y toggles between 0 and  hgap in FlowLayout(=5) which
     *   causes ogl window to shift up and down in the viewport.
     * - workaround is to always force window origin to be 0,0
     */
    public void setBounds(int x, int y, int w, int h){
    	super.setBounds(0, 0, w, h);
    }
    // Methods required for the implementation of GLEventListener

   /** Initialize openGL environment.
    *  This function is called once per GLEventListener instantiation.
    */
    public void init(GLAutoDrawable drawable){  
        gl = drawable.getGL().getGL2();
        //System.out.println("init:"+gl);

        GLUtil.init();
        if(DebugOutput.isSetFor("glinfo"))
            showGLInfo();
        shader=GLUtil.getShaderType();
        renderer.init(shader);
        gl.glLineWidth(line_width);
        gl.glPointSize(point_size);
        gl_vendor=gl.glGetString (GL2.GL_VENDOR);
        gl_renderer=gl.glGetString (GL2.GL_RENDERER);
        gl_version=gl.glGetString(GL2.GL_VERSION);
        gl.glCullFace(GL2.GL_BACK);
        gl.glFrontFace(GL2.GL_CCW);            // Clock wise wound is front       
        view.init();
    }
   
   /** Set drawing dimensions.
    *  This function is called when the display window is first created
    *  and after each window resize event.
    */
    public void reshape(GLAutoDrawable drawable, int x, int y, int w, int h){       
        boolean newsize=(w!=width || h!=height || x!=0 || y!=0);
       // if(!newsize && initialized){
        //    System.err.println("resize called with unchanged dimensions");
            //return;
        //}
        gl = drawable.getGL().getGL2();
        width=w;
        height=h;
       // System.out.println("w:"+width+" h:"+height);
        aspect = ((double)height)/width;
        view_invalid=true;
        renderer.resize(width, height);
        view.setResized();
        init3DText();
        // old bug: for some reason trace colors seems to get set to two-color
        //          mode while resizing unless setArrayColors is called        
        // new bug: calling setArrayColors here causes 1D shader errors
        //setArrayColors(show & SHOWPALETTE);
    }
    
    private void init3DText(){
        text3D=new TextRenderer(new Font("SansSarif", Font.PLAIN, 72),true,true);
    }
 
    private void setSliceVector() {
		xaxis = volume.XPlane();
		yaxis = volume.YPlane();
		zaxis = volume.ZPlane();

		switch (sliceplane) {
		case X:
			if((status & SLICELOCK)>0 ||newsliceplane)
				svect = xaxis;
			break;
		case Y:
			if((status & SLICELOCK)>0 ||newsliceplane)
				svect = yaxis;
			break;
		case Z:
			if((status & SLICELOCK)>0 || newsliceplane)
				svect = zaxis;
			break;
		}
	}

   /** Draw the graphics content.
     *  This function is called in response to a "repaint" event
     */
    public void display(GLAutoDrawable drawable) {
        
    	gl = drawable.getGL().getGL2();
    	
    	initialized=true;
    	
        //if(pacingDisplay()){
            long endTime = System.nanoTime();
            repeat_time = (int)millisecs(endTime - repeat_start);
            repeat_start=endTime;
            draw_ticks=(int)(repeat_time/repeat_delay);
            if (pacingDisplay() && DebugOutput.isSetFor("gltimer"))
                Messages.postDebug("JGLGraphics.display DRAW["+repeat_time+"] ticks:"+draw_ticks);
        //}

        if(!havedata)
            clearDisplay();
        else if(which_cursor == 0 || dragphase) {
            long startTime = System.nanoTime();
            setLocalView(); 

            if(!imagecontext && !animating && (mouse_clicked || mouse_pressed) && button==LEFT && !mouse_wheel)
            //if(mouse_pressed && button==LEFT && !mouse_wheel)
                 which_tool = selectTool();
            mouse_pressed=false;
            mouse_wheel=false;
            //mouse_clicked=false;
            if(which_tool != SET_SVECT)
                setSliceVector();
            
            setLocalView();
            
            if(newsliceplane && projection >TWOD){
                initSlice();
            }
            if(which_tool != 0)
                controlTool(which_tool);
            clearDisplay();

            if(projection >TWOD){
            	if(((prefs & V3DDATA) == 0) && ((prefs & V3DTOOLS) > 0)){
                    draw3DTools(false);
                    draw3DTools(true);            		
            	}
            	else if(((prefs & V3DDATA) > 0) && ((prefs & V3DTOOLS) > 0)){
                    draw3DTools(false);
                    render();
                    draw3DTools(true);
            	}
	            else
	            	render();
            }
            else
            	render();
            view_invalid = false;
            stepflag=false;
            if(mouse_event || selection_mode || which_tool>0)
            	mouse_clicked=false;

            if(dragging)
            	oldmouse=new Point3D(mouse);
            if(enddrag){
            	enddrag=false;
            	which_tool=0;
            }
            endTime = System.nanoTime();
            render_time = (int)millisecs(endTime - startTime);
            if(pacingDisplay())
                setTimerSem();
          }
        if(DebugOutput.isSetFor("glrepaint"))
            Messages.postDebug("JGLGraphics.display(drawable)");
         //drawable.swapBuffers();
    }

    /**
     * Respond to a display change event. <This function is currently unused>
     */
    public void displayChanged(GLAutoDrawable drawable, boolean modeChanged, boolean deviceChanged){ 
        //System.out.println("displayChanged");
        //drawable.swapBuffers();
    }

    //  Methods required for the implementation of JGLComListenerIF
 
    public void setStringValue(int id, String value) {
        switch(id){
        case G3DAXIS:
            setAxisLabels(value);
            break;
        case G3DVERSION:
            version=value;
            break;
        }
    }

    /* (non-Javadoc)
     * @see vnmr.jgl.JGLComListenerIF#setPointValue(int, vnmr.jgl.Point3D)
     */
    public void setPointValue(int id, Point3D value) {
        switch(id){
        case G3DPNT:
            vpnt=new Point3D(value);
            rstvpnt=vpnt;
        	if(comMgr.connected)
        		reset_mode=RESETVPNT;
            //if((status & SLICELOCK)>0){
                //if(value.w>0)
               //     newsliceplane=true;
            //}
            setSliceFromVPnt();
            break;
        case G3DROT:
        	if(comMgr.connected)
        		reset_mode=RESETVROT;
            vrot=value;
            break;
        }
        view.vrot=vrot;
    }

    /* (non-Javadoc)
     * @see vnmr.jgl.JGLComListenerIF#setFloatValue(int, float)
     */
    public void setFloatValue(int type, int index, float value) {
        switch(type){
        case G3DGF:
        	switch(index){
            case G3DSXP:
                slice_xparancy=value;
                break;
            case G3DPXP:
                plane_xparancy=value;
                break;
        	}
        	break;
        case G3DF:
        	switch(index){
            case G3DCBIAS:
            	if(comMgr.connected)
            		reset_mode=RESETINTENSITY;
                bias=value;
                break;
            case G3DINTENSITY:
            	if(comMgr.connected)
            		reset_mode=RESETINTENSITY;
                intensity=value;
                break;
            case G3DCONTRAST:
            	if(comMgr.connected)
            		reset_mode=RESETINTENSITY;
                contrast=value;
                break;
            case G3DTHRESHOLD:
            	if(comMgr.connected)
            		reset_mode=RESETXPARENCY;
                threshold=value;
                break;
            case G3DCWIDTH:
            	if(comMgr.connected)
            		reset_mode=RESETCONTOURS;
                contours=value;
                break;
            case G3DLIMIT:
            	if(comMgr.connected)
            		reset_mode=RESETXPARENCY;
                limit=value;
                break;
            case G3DXPARENCY:
            	if(comMgr.connected)
            		reset_mode=RESETXPARENCY;
                transparency=value;
                break;
            case G3DDSCALE:
            	if(comMgr.connected){
            		reset_mode=RESETRESOLUTION;
            		data.ymax=dmax=value;
            	}
                break;
            case G3DZSCALE:
            	if(comMgr.connected)
            		reset_mode=RESETZOOM;
                zscale=value;
                break;
            case G3DSWIDTH:
                slab_width=value;
            	if(comMgr.connected)
            		reset_mode=RESETWIDTH;
                break;
        	}
        	break;
        }
        if(comMgr.connected)
        repaint();
    }

    /* (non-Javadoc)
     * @see vnmr.jgl.JGLComListenerIF#setIntValue(int, int)
     */
    public void setIntValue(int type, int index, int value) {
        switch(index){
        case G3DMAXS:
            if(projection==THREED && psuedo_slices!=value)
                data.slice=data.slice*value/psuedo_slices;
            
            psuedo_slices=value;
            setAlphaScale();
            break;
        case G3DMINS:
            max_step=value>0?value:1;
            break;
        }
        if(comMgr.connected)
        repaint();
     }

    /* (non-Javadoc)
     * @see vnmr.jgl.JGLComListenerIF#setComCmd(int, int)
     */
    public void setComCmd(int code, int value) {
        int rvalue=0;
        int last=0;
        switch(code) {
        case G3DRUN:
        	break;
        case G3DIMAGE:
        	System.out.println("setComCmd G3DIMAGE:"+value);
        	if(value>1)
        		printImage(value);
        	else
            	createImage();
        	break;
       case G3DSHOWREV:
            show_version=true;
            repaint();
            break;
        case G3DREPAINT:
            repaint();
            break;
        case G3DRESET:
            if(value==-1)
                rvalue=RESETALL;
            else if(value==0)
                rvalue=reset_mode;
            else
                rvalue=value;
            if(DebugOutput.isSetFor("glcom"))
                Messages.postDebug("JGLGraphics.setComCmd G3DRESET: "+value+":"+rvalue);
            reset(rvalue);
            repaint();
            break;
        case G3DINIT:
            if(DebugOutput.isSetFor("glcom"))
                Messages.postDebug("JGLGraphics.setComCmd G3DINIT");
            break;
        case G3DSTEP:
            if(DebugOutput.isSetFor("glcom"))
                Messages.postDebug("JGLGraphics.setComCmd G3DSTEP:"+value);
            switch(value){
            case NEXT:
            case PREV:
                step(value);
                if(step_direction!=value)
                    comMgr.setStatusField(STATINDEX, value,2);
                step_direction=value;
                break;
            case INCREASE:
            	if(animateSlice()&&data.dim>1){
            		if(step<(int)(max_step/sfactor))
            			step++;           		
            	}
            	if(animateRotation()){
            		rotation_step*=1.5;
            	}
                break;
            case DECREASE:
            	if(animateSlice()&&data.dim>1){
            		if(step>1)
            			step--;           		
            	}
            	if(animateRotation()){
            		rotation_step/=1.5;
            	}
                break;
            }
        
            repaint();
            break;
        case G3DPREFS:  // UI control
            if(DebugOutput.isSetFor("glcom"))
                Messages.postDebug("JGLGraphics.setComCmd G3DPREFS:0x"+Integer.toHexString(value).toUpperCase());
            if((value & VTEXT)==0)
                show_info=false;
            else
                show_info=true;
            if((value & VCURSORS)==0)
            	show_cursors=false;
            else
            	show_cursors=true;

        	prefs=value;
        	break;

        case G3DSTATUS: // animation control
            if(DebugOutput.isSetFor("glcom"))
                Messages.postDebug("JGLGraphics.setComCmd G3DSTATUS:0x"+Integer.toHexString(value).toUpperCase());

            if((value&STATPHASING)>0 && data.complex)
                phase_mode = true;
            else
                phase_mode = false;
            if((value & SLICELOCK) != (status & SLICELOCK)){
                newsliceplane=true;
                if((value & SLICELOCK)>0){
                    switch(sliceplane){
                    case X:
                        vpnt.w=vpnt.x;
                        break;
                    case Y:
                        vpnt.w=vpnt.y;
                        break;
                    case Z:
                        vpnt.w=vpnt.z;
                        break;
                    }
                }
            }
            laststatus=status;
            if(status !=value)
                data_flags=0;
            if((value & SHOWORTHO) != (status & SHOWORTHO))
                resetPaceParams();
            if((value & PACEDISPLAY) != (status &PACEDISPLAY))
                resetPaceParams();
            if((value & ANIMATEMODE) != (status & ANIMATEMODE))
                resetPaceParams();
           
            status=value;
            
            if((value & STATRUNNING)>0 && !animating){
	            //if(data.dim==2 || (data.dim==1 && (projection==ONETRACE))){
	                setTimerDelay(repeat_delay);
	                Messages.postDebug("JGLGraphics.setComCmd animation started");
	                startTimer();
	           // }
	            animating = true;
                return;
            }
            else if ((value & STATRUNNING)==0 && animating){
            	animating = false;
                draw_ticks=0;
                stopTimer();
                setTimerDelay(init_delay);
                Messages.postDebug("JGLGraphics.setComCmd animation stopped");
                comMgr.sendVPntToVnmr(vpnt, false);
                return;
            }
            reset_mode=RESETSTATUS;
            break;
        case G3DSHOW: //display control
            if(DebugOutput.isSetFor("glcom"))
                Messages.postDebug("JGLGraphics.setComCmd G3DSHOW:0x"+Integer.toHexString(value).toUpperCase());
            last=projection;
            if(last==THREED && (value & SHOWPTYPE)!=SHOW3D){
            	// when switching from 3D to other projection modes the slice index
            	// needs to be rescaled.
            	double real_slice=((double)data.slice*real_slices)/max_slices;
            	data.slice=(int)real_slice;           	
            }
            switch(value & SHOWPTYPE){
            default:
            case SHOW1D:
                projection=ONETRACE;
                if(last>0 && last!=projection){
         		    data.trace=0;
         		    yoffset=(data.type==FID)?0.5:0.2;
                }
                break;
            case SHOW1DSP:
                projection=OBLIQUE;
                if(last!=projection){
                	yoffset=0.0;
                	zoffset=0.0;
                	xoffset=xcenter=0;
                }
                break;
             case SHOW2D:
                projection=TWOD;
                break;
             case SHOW2DSP:
                projection=SLICES;
	            if(last!=projection)
	                newsliceplane=true;
                break;
             case SHOW3D:
                projection=THREED;
	            if(last!=projection)
	                newsliceplane=true;
                 break;
            }
            if(last!=projection)
                resetPaceParams();

            last=sliceplane;
            if(data.dim<2)
            	sliceplane=Z;
            else
            switch(value & SHOWAXIS){
            case SHOWY:
                sliceplane=Y;
                break;
            case SHOWX:
                sliceplane=X;
                break;
            case SHOWZ:
                sliceplane=Z;
                break;
            }
            if(last!=sliceplane)
                newsliceplane=true;
            int maxs=maxSlice();
            data.slice=data.slice>maxs?maxs:data.slice;
            data.slice=data.slice<0?0:data.slice;

            if((show & SHADERMODE) != (value & SHADERMODE))
                resetPaceParams();

            if((show & SHOWPALETTE) != (value &SHOWPALETTE))
                setArrayColors(value & SHOWPALETTE);
                        
            reset_mode=RESETSHOW;

            show=value;
            repaint();
           break;
        }
    }

    /* (non-Javadoc)
     * @see vnmr.jgl.JGLComListenerIF#setData(int, vnmr.jgl.JGLData)
     */
    public void setData(int dflags, JGLData cdata){      
        data_flags=dflags;
        if(DebugOutput.isSetFor("gldata"))
            Messages.postDebug("JGLGraphics.setData flags:"+dflags); 
        if((dflags & JGLData.XFER)>0){
            //data.slice=0;
            havedata=false;
            switch(dflags & JGLData.XFERMODE){
            case JGLData.XFERERROR:
                repaint();
                return;
            case JGLData.XFERREAD:
                tex_build_end=0;
                xfer_start=System.nanoTime();
                repaint();
                return;
            case JGLData.XFERBGN:
                data_read_time = System.nanoTime() - xfer_start;
                repaint();
                return;
            case JGLData.XFEREND:
            default:
                data_xfer_time = System.nanoTime()-xfer_start-data_read_time;
                break;
            }           
        }
        //data=new JGLData(cdata);
        data=new JGLDataMgr(cdata);
        if(!havedatascale || ((status & AUTOSCALE)!=0)){
	        dmin=data.ymin;
	        dmax=data.ymax;
	        havedatascale=true;
        }
        else{
	        data.ymin=dmin;
	        data.ymax=dmax;       	
        }
        span=new Point3D(data.sx,data.sy,data.sz);
        reset_threshold=data.mean-0.25*data.stdev;
        if(reset_threshold<0)
        	reset_threshold=0;
        
        if(DebugOutput.isSetFor("gldata")){
            String read=intformat.format(millisecs(data_read_time));
            String xfer=intformat.format(millisecs(data_xfer_time));
        	Messages.postDebug("JGLGraphics.setData read:"+read+" xfer:"+xfer+" ms");
        }
        //System.out.println("mean="+data.mean+" stddev="+data.stdev+" threshold="+reset_threshold);
        
        if(data.dim==2){
        	setSliceFromVPnt();
        }
        comMgr.sendVPntToVnmr(vpnt,false);
            
        view.setDataPars(data.np, data.traces, data.slices, data.data_type);       
        renderer.setDataPars(data.np, data.traces, data.slices, data.data_type);
        data.setDataPars(data.np, data.traces, data.slices, data.data_type);
        if(data.mmapped)
            renderer.setDataMap(data.volmapfile);
        else
            renderer.setDataMap("");

        havedata=true;
     }
    
    private void paintInfoLine(Graphics g){
        float A[] = getStdColor(TEXTCOLOR);
        Color c = new Color(A[0], A[1], A[2]);
        g.setColor(c);
        g.setFont(fpsFont);
        
        String str;
        String rstr="";
        if(DebugOutput.isSetFor("gltime")){
            rstr="";
            rstr+=" CYCLE:"+ intformat.format(1000.0/repeat_time);
            rstr+=" DRAW:"+ intformat.format(1000.0/render_time);
            rstr+=" fps";
        }
        if((data_flags & JGLData.XFER)>0){
            switch(data_flags & JGLData.XFERMODE){
            case JGLData.XFERBGN:
                g.drawString(" XFER: STARTING UPLOAD", width/2-100,height/2);
                break;
            case JGLData.XFERREAD:
                g.drawString(" XFER: READING", width/2-100,height/2);
                break;
            case JGLData.XFEREND:
                if(data.dim==2){
                    g.drawString("INITIALIZING 3D", width/2-100,height/2);
                    data_flags=JGLData.XFERCLR|JGLData.XFER;
                    tex_build_start = System.nanoTime();
                    repaint();
                    break;
                }
            case JGLData.XFERCLR:
                float read_time=millisecs(data_read_time)/1000.0f;
                float xfer_time=millisecs(data_xfer_time)/1000.0f;
                str="" 
                    + "  NP:" + data.np 
                    + "  NT:"+ intformat.format(data.traces) 
                    + "  NS:"+ intformat.format(data.slices) 
                    + "  PTS:"+ intformat.format(data.np*data.slices*data.traces/1024) +" k"
                    + "  MAP:" + data.mmapped
                    + "  READ:" + fltformat2.format(read_time) 
                    + "  XFER:" + fltformat2.format(xfer_time)
                    ;
                if(data.dim==2){
                    if(tex_build_end==0)
                        tex_build_end=System.nanoTime();
                    float tex_time=millisecs(tex_build_end-tex_build_start)/1000.0f;
                    str+="  3D:" + fltformat2.format(tex_time);
                }
                g.drawString(str, 0, 10);
                //data_flags=0;
                break;
            case JGLData.XFERERROR:
                g.drawString(" XFER: ERROR READING DATA", width/2-100,height/2);
                data_flags=0;
                break;
            }
        }
        else if(!havedata || show_version){
           if(gl_version !=null){
                g.drawString("G3D Version: " + version, 0, 10); 
                g.drawString("OpenGL Version: " + gl_version, 0, 25); 
                g.drawString("Shader: "+GLUtil.getShaderString(shader), 0, 40);
                g.drawString("Renderer: "+(native_ogl?"Native":"JOGL"), 0, 55);
                show_version=false;
            }
        }
        else if(phase_mode) {
            g.drawString("" 
                    + "  RP:" + fltformat2.format(data.rp % 360) 
                    + "  LP:" + fltformat2.format(data.lp) 
                    + rstr
                    , 0, 10);                
        }
        else if(data_selected){
        	if(seldata.z>0)
            g.drawString("" 
                    + "  TRC:" + intformat.format(seldata.y) 
                    + " IMAG:" + intformat.format(seldata.x) 
                    + " VALUE:"+ fltformat3.format(selvalue)
                    + rstr
                    , 0, 10); 
        	else
                g.drawString("" 
                        + "  TRC:" + intformat.format(seldata.y) 
                        + " REAL:" + intformat.format(seldata.x) 
                        + " VALUE:"+fltformat3.format(selvalue)
                        + rstr
                        , 0, 10);         		
        }
        else if(rotating) {
            g.drawString("" 
                    + "  H:" + fltformat2.format(vrot.x) 
                    + "  A:" + fltformat2.format(vrot.y) 
                    + "  B:" + fltformat2.format(vrot.z) 
                    + rstr
                    , 0, 10);                
        }
        else if(mousemode) {
            switch(status & MOUSEMODE){
            case TRANSPARENCY:
                g.drawString("" 
                    + "  T:"+ fltformat1.format(threshold) 
                    + "  L:"+ fltformat1.format(limit) 
                     + rstr
                    ,0, 10);
                break;
            case RESOLUTION:
                g.drawString("" 
                    + "  M:"+ fltformat1.format(1.0f/data.ymax) 
                    + "  S:"+ fltformat1.format(sampling_factor) 
                    + rstr
                    ,0, 10);
                break;
            case INTENSITY:
                g.drawString("" 
                     + "  I:"+ fltformat1.format(intensity) 
                     + "  C:"+ fltformat1.format(contrast) 
                     + "  B:"+ fltformat1.format(bias) 
                     + rstr
                    ,0, 10);
                break;
            }
        }
        else  {
        	double dslice=(data.slices>1)?(double)100*data.slice/(max_slices-1):0;
            switch(projection){
            case ONETRACE:
                if(data.slices>1)
                    str="  SP "+ fltformat2.format(dslice)+"  TRC "+ intformat.format(data.trace);
                else
                    str=data.traces>0?"  TRC "+ intformat.format(data.trace):"";
                g.drawString("" 
                        + "  YS:"+ fltformat3.format(ascale) 
                        + "  XS:"+ fltformat3.format(xscale) 
                        + "  X:"+ fltformat3.format(xoffset) 
                        + str 
                        + rstr      
                         ,0, 10);
                break;
            case OBLIQUE:
                str=data.slices>1?"  SP:"+ fltformat2.format(dslice)+" %":"";
                g.drawString("" 
                        + "  YS:"+ fltformat3.format(ascale) 
                        + "  XS:"+ fltformat3.format(xscale) 
                        + "  X:"+ fltformat3.format(xoffset) 
                        + rstr     
                        + str 
                         ,0, 10);
                break;
            case TWOD:
                str=data.slices>1?"  SP:"+ fltformat2.format(dslice)+" %":"";
                g.drawString("" 
                        + "  SC:"+ fltformat3.format(yscale) 
                        + "  X:"+ fltformat3.format(xoffset) 
                        + "  Y:"+ fltformat3.format(zoffset) 
                        + "  RX:"+ fltformat2.format(tilt) 
                        + "  RY:"+ fltformat2.format(twist) 
                        + str 
                        + rstr      
                        ,0, 10);
                break;
            case SLICES:
            case THREED:
                str=data.slices>1?"  SP:"+ fltformat2.format(dslice)+" %":"";
                g.drawString("" 
                        + "  SC:"+ fltformat3.format(zscale) 
                        + "  RX:"+ fltformat2.format(tilt) 
                        + "  RY:"+ fltformat2.format(twist) 
                        + str 
                        + rstr      
                        ,0, 10);
                break;
            }
        }    	
    }

    /** Adorn window surface using 2d graphics.
     *  Called after OpenGL has drawn the primary content of the window
     *  - used to display text3D and image overlays, 1D cursors etc.
     */
    public void paintComponent(Graphics g) {
        super.paintComponent(g);
        paint1DCursers(g);
        if(show_info)
        	paintInfoLine(g);
    }

    //##############  private methods follow ###########################
    
    private void zoomIn(){
    	zoom_state.push(new ZoomState(xoffset,xscale,cursor1,cursor2));
		double x1=cursor1;
		double w1=Math.abs(cursor2-cursor1);
		xoffset=x1;
		xscale=w1;
		//if(data.type==FID)
	    if(cursor1==0)
			cursor1=x1;
		else
			cursor1=x1+dflt_spectrum_cursor1*w1;
	    if(cursor2<1.0)
	    	cursor2=x1+dflt_spectrum_cursor2*w1;
		repaint();
    }

    private void zoomOut(){
    	if(!zoom_state.isEmpty()){
    		ZoomState zs=zoom_state.pop();
	    	xoffset=zs.xoffset;
	    	xscale=zs.xscale;
	    	cursor1=zs.cursor1;
	    	cursor2=zs.cursor2;
			repaint();
    	}
    }
    
    // graphics2D functions

    private void paintCursor(Graphics g,int col, double x){
        double pos=valueToScreen(x);
        if(pos<0|| pos>1)
            return; 
        float A[]=getStdColor(col);
        int h=getHeight();
        int w=getWidth();
        int xpos=(int)(pos*w);
        if(x==1.0)
        	xpos--; // keeps right cursor on screen
        Color c=new Color(A[0],A[1],A[2]);

        g.setColor(c);
        g.drawLine(xpos, h, xpos, 0);
     }

    private void paint1DCursers(Graphics g){
        if(phase_mode){
            paintCursor(g,PHSCURSORCOLOR,phasecursor1);
        }
        if(projection <TWOD && show_cursors){
            paintCursor(g,CURS0R1COLOR,cursor1);
            paintCursor(g,CURS0R2COLOR,cursor2);           
        }
    }

    private void getColors(){
        int i,j,k,index;
        
         // get std colors
        
        stdcols=new float[NUMSTDCOLS*4];
        setStdColor(BGCOLOR,"g3dBackground");
        setStdColor(REALCOLOR,"graphics9");
        setStdColor(IMAGCOLOR,"graphics10");
        
        setStdColor(PHSREALCOLOR,"graphics12");
        setStdColor(CURS0R1COLOR,"green");
        setStdColor(CURS0R2COLOR,"red");
        setStdColor(PHSCURSORCOLOR,"orange");

        setStdColor(IMGBGCOLOR,"g3dMin");
        setStdColor(IMGFGCOLOR,"g3dMax");
        setStdColor(GRIDCOLOR,"g3dGrid");
        
        setStdColor(XAXISCOLOR,"graphics57");
        setStdColor(YAXISCOLOR,"graphics58");
        setStdColor(ZAXISCOLOR,"graphics59");
        setStdColor(EYECOLOR,"graphics60");
        setStdColor(TEXTCOLOR,"g3dText");
        setStdColor(BOXCOLOR,"g3dBox");
        
        renderer.setColorArray(STDCOLS, stdcols);
        
        index=273;
        ctrcols=new float[NUMCTRCOLS*4];
        for(i=0,j=0;i<NUMCTRCOLS;i++,index++,j++){
            String cstr="graphics"+index;
            float A[]=getColor(cstr);
            for(k=0;k<4;k++){
                ctrcols[4*j+k]=A[k];
            }
            //ctrcols[4*j+3]=0;
        }
        int base=(NUMCTRCOLS-1)*2; // center color
        //ctrcols[base+3]=0;  // test to make base color transparent
        // get absolute value colors array
        
        index=20;
        abscols=new float[NUMABSCOLS*4];
        for(i=0,j=0;i<NUMABSCOLS;i++,index++){
            String cstr="graphics"+index;
            float A[]=getColor(cstr);
            abscols[j++]=A[0];
            abscols[j++]=A[1];        
            abscols[j++]=A[2];        
            abscols[j++]=A[3];        
       }

        // get phase colors array
        
        index=36;
        phscols=new float[NUMPHSCOLS*4];
        for(i=0,j=0;i<NUMPHSCOLS/2;i++,index++,j++){
            String cstr="graphics"+index;
            float A[]=getColor(cstr);
            for(k=0;k<4;k++){
                phscols[4*j+k]=A[k];
            }
        }
        index=44;
        for(i=0;i<NUMPHSCOLS/2;i++,index++,j++){
            String cstr="graphics"+index;
            float A[]=getColor(cstr);
            for(k=0;k<4;k++){
                phscols[4*j+k]=A[k];
            }
        }
        grays=new float[8];
        float[] A=getStdColor(IMGBGCOLOR);
        j=0;
        for(k=0;k<4;k++,j++){
            grays[j]=A[k];
        }
        A=getStdColor(IMGFGCOLOR);
        for(k=0;k<4;k++,j++){
            grays[j]=A[k];
        }
        setArrayColors(show & SHOWPALETTE);
    }
    private void setArrayColors(int value){
        switch(value) {
        default:
        case SHOWABSCOLS:
            renderer.setColorArray(ABSCOLS, abscols);
            break;
        case SHOWPHSCOLS:
            renderer.setColorArray(PHSCOLS, phscols);
            break;
        case SHOWCTRCOLS:
            renderer.setColorArray(CTRCOLS, ctrcols);
            break;
        case SHOWGRAYS:
            renderer.setColorArray(GRAYCOLS, grays);
            break;
        }
    }

    // utility functions

    private float[] getColor(String color){
        Color c = DisplayOptions.getColor(color);
        return getColor(c);
    }

    private float[] getColor(Color c){
         return getColor(c,1.0f);
    }
    private float[] getColor(Color c, float a){
        float A[]=new float[4];
        A[0]=c.getRed()/255.0f;
        A[1]=c.getGreen()/255.0f;
        A[2]=c.getBlue()/255.0f;
        A[3]=a;
        return A;
    }

    private float[] getStdColor(int i){
        float A[]=new float[4];
        int indx=i*4;
        A[0]=stdcols[indx];
        A[1]=stdcols[indx+1];
        A[2]=stdcols[indx+2];
        A[3]=stdcols[indx+3];
        return A;
    }

    private void setStdColor(int i,float[] A){
        int indx=i*4;
        stdcols[indx]=A[0];
        stdcols[indx+1]=A[1];
        stdcols[indx+2]=A[2];
        stdcols[indx+3]=A[3];
    }

    private void setStdColor(int i, String s){
        float A[]=getColor(s);
        int indx=i*4;
        stdcols[indx]=A[0];
        stdcols[indx+1]=A[1];
        stdcols[indx+2]=A[2];
        stdcols[indx+3]=A[3];
     }

    private void setStdColor(int i, String s, float a){
        float A[]=getColor(s);
        int indx=i*4;
        stdcols[indx]=A[0];
        stdcols[indx+1]=A[1];
        stdcols[indx+2]=A[2];
        stdcols[indx+3]=a;
     }
    private  int setBits(int w, int m, int b) {
        int r=w&(~m);
        return r|b;
    }
    private int setBit(int w, int b) {
        return w|b;
    }
    private  int clrBit(int w, int b) {
        return w&(~b);
    }

    private float microsecs(long t) {
        return (float)(t/1000);
    }

    private float millisecs(long t) {
        return (float)(t/1000000);
    }
    private int selectCursor(double x){
        double tol=0.01*xscale;
        double r=screenToValue(x/width);
        if(phase_mode && Math.abs(r-phasecursor1)<tol){
            return PHASECURSOR;
        }
        if(show_cursors){
	        if(Math.abs(r-cursor1)<tol)
	            return CURSOR1;
	        if(Math.abs(r-cursor2)<tol)
	            return CURSOR2;
        }
        return 0;
    }
    
    private double screenToValue(double x){
        double r=(x-xcenter)*xscale+xoffset;
        return r;
    }

    private double valueToScreen(double r){
        double x=(r-xoffset)/xscale+xcenter;
        return x;
    }

    private double rangeCheck(double x){
        double r=x;
        if(projection>=TWOD || !range_check)
            return x;
        else if(xscale>1)
            xscale=1;
        double ww=xscale;
        double left=x-xcenter*ww;
        double right=x+(1-xcenter)*ww;
        if(left<0)
            r=xcenter*ww;
        else if(right>1)
            r=1-(1-xcenter)*ww;       
        return r;
    }

    private void resetVPnt(){
        vpnt=new Point3D(0.5,0.5,0.5,0.5);
        newsliceplane=true;
        if(comMgr.connected)
            comMgr.sendVPntToVnmr(vpnt,false);
    }
    private void reset(int mode){
        if((mode & RESETOFFSET)!=0){
            xcenter=0;//(data.type==FID)?0.0:0.5;
            xoffset=xcenter;
            zoffset=0.0;
            if(projection==ONETRACE)
            	yoffset=(data.type==FID)?0.5:0.2;
            else
            	yoffset=0;
            if(data.type==FID){
            	cursor1=dflt_fid_cursor1;
            	cursor2=dflt_fid_cursor2;
            }
            else{
                cursor1=dflt_spectrum_cursor1;
                cursor2=dflt_spectrum_cursor2;           	
            }
            view_invalid=true;
        }
        if((mode & RESETROTATION)!=0){
            delx=delx_dflt;
            dely=1.0;
            tilt=0;
            twist=0;
            rotx=dflt_rotx;
            roty=dflt_roty;
            rotz=0;
            view_invalid=true;
            rotation_step=dflt_rotation_step;
        }
        if((mode & RESETZOOM)!=0){
            xscale=1.0;
            ascale=0.5;
            yscale=1.0;
            zscale=1.0;
            if(data.type==FID){
            	cursor1=dflt_fid_cursor1;
            	cursor2=dflt_fid_cursor2;
            }
            else{
                cursor1=dflt_spectrum_cursor1;
                cursor2=dflt_spectrum_cursor2;           	
            }
            zoom_state.clear();
            comMgr.sendVFltToVnmr(G3DZSCALE,1.0,false);
        }
        if((mode & RESETWIDTH)!=0){
            slab_width=reset_width;
            comMgr.sendVFltToVnmr(G3DSWIDTH,slab_width,false);
            if(comMgr.connected)
                repaint();
        }
        if((mode & RESETSLICE)!=0){
            resetVPnt();
            setSliceFromVPnt();
            step=1;
            if(comMgr.connected)
                repaint();
        }
        if((mode&RESETVPNT)!=0){
            resetVPnt();
            if(comMgr.connected)
                repaint();
        }
        if((mode&RESETVROT)!=0){
         	vrot=new Point3D(0.0,0.0,0.0,0.0);
            view.vrot=vrot;
            if(comMgr.connected){
            	comMgr.sendVRotToVnmr(vrot,false);
                repaint();
            }
        }
        if((mode & RESETINTENSITY)!=0){           
            if((show & SHADERMODE)==MIP)
                intensity=dflt_intensity;
            else
                intensity=2*dflt_intensity;
            contrast=dflt_contrast;
            bias=0;
 	        comMgr.sendVFltToVnmr(G3DINTENSITY,intensity,false);
	        comMgr.sendVFltToVnmr(G3DCONTRAST,contrast,false);
	        comMgr.sendVFltToVnmr(G3DCBIAS,bias,false);
       }
        if((mode & RESETCONTOURS)!=0){           
            contours=dflt_contours;
            comMgr.sendVFltToVnmr(G3DCWIDTH,contours,false);
        }
        if((mode & RESETRESOLUTION)!=0){
            data.ymax=dmax;
            data.ymin=dmin;
            step=1;
            setAlphaScale();            
        }
        if((mode & RESETXPARENCY)!=0){
            threshold=reset_threshold;
            transparency=1;
            limit=1;
            comMgr.sendVFltToVnmr(G3DTHRESHOLD,threshold,false);
            comMgr.sendVFltToVnmr(G3DLIMIT,limit,false);
         }
        if((mode & RESETSVECT)!=0){
            svect=rstsvect;
            return;
        }
        if((mode & RESETSTATUS)!=0){
            comMgr.setStatusValue(laststatus);
            repaint();
            return;
        }
        reset_mode=RESETALL;
    }
    
    private void mouseEventHandler(){
        double pstep=0;
        if(animating){
            step(step_direction);
            if(!mouse_clicked)
            	return;
            mouse_event=true;
        }
        if(selection_mode || dragging || cursor_set_mode){
        	repaint();
        	return;        	
        }
        else if(button==LEFT) {  
            pstep=wscale*0.05;
            switch(projection){
            case THREED:
            case SLICES:
                break;
            case TWOD:
                xoffset -= pstep* zscale/aspect;
                reset_mode=RESETOFFSET;
                break;
            default:
                xoffset -= pstep* xscale;
                xoffset=rangeCheck(xoffset);
                reset_mode=RESETOFFSET;
                break;
            }
        }
        else if(button==RIGHT) {
            pstep=1.0f+0.5*Math.abs(wscale);
            pstep=(wscale>=0)?pstep:1.0f/pstep;
            switch(projection){
            case THREED:
            case SLICES:
                if(repeating){
                    pstep=1.0f+0.1*Math.abs(wscale);
                    pstep=(wscale>=0)?pstep:1.0f/pstep;
                }
                zscale *= pstep;
                comMgr.sendVFltToVnmr(G3DZSCALE,zscale,true);
                break;
            case TWOD:
            	if(ymouse)
            		ascale -= 0.5 * hscale;
            	else
            		yscale *= pstep;
                break;
            case OBLIQUE:
            case ONETRACE:
            	if(ymouse){
					pstep = 1.0f + 0.5 * Math.abs(hscale);
					pstep = (hscale <= 0) ? pstep : 1.0 / pstep;
					ascale *= pstep;
            	}
            	else{
	               xscale *= pstep;                
	               if(xscale > 1.0 && projection==ONETRACE)
	                    xscale = 1.0;
            	}
                break;
            default:
                xscale *= pstep;                
                if(xscale > 1.0)
                    xscale = 1.0;
                break;
            }
            reset_mode=RESETZOOM;
        }    
        else if (button == MIDDLE) {
        	if (ctrlkey) {
        		if (hscale < 0)
        			bias += bias_step;
        		else
        			bias-=bias_step;
				comMgr.sendVFltToVnmr(G3DCBIAS, bias, true);
				reset_mode = RESETINTENSITY;
        	}
        	else {
				double istep = 1 + intensity_step;
				if (hscale < 0)
					intensity *= istep;
				else
					intensity /= istep;
				comMgr.sendVFltToVnmr(G3DINTENSITY, intensity, true);
				reset_mode = RESETINTENSITY;
			}
		}
        if(!animating)
        	repeating=true;
        //button=0;
        repaint();
    }

    private void setAlphaScale(){
        int max=0;
        switch(sliceplane){
        case Y:
            max=data.traces;
            break;
        case X:
            max=data.np;
            break;
        case Z:
            max=data.slices;
            break;
        }
        if(projection==THREED){
            if(max>psuedo_slices || (show & SHADERMODE)==MIP)
                sfactor=1;
            else
                sfactor=((double)max)/(psuedo_slices);
            alphascale=sfactor*step;
            sampling_factor=1/alphascale;
            alphascale=alphascale>1?1:alphascale;
        }
        else{
            sfactor=1;
            alphascale=1;
            sampling_factor=1.0/step;
        }
    }

    private void initSlice(){
        setAlphaScale();
        setSliceFromVPnt();
        if((status & AUTOFRONTFACE)>0){
	        boolean backfacing=!view.sliceIsFrontFacing();
	        boolean reverse=(status & STATREVERSE)>0;
	
	        if(backfacing && !reverse)
	            comMgr.setStatusField(STATINDEX,4,4);
	        else if(!backfacing && reverse)
	            comMgr.setStatusField(STATINDEX,0,4);
        }
        reset_mode=RESETVPNT;
        newsliceplane=false;
    }

    private boolean setVPntFromSlice(boolean wait) {
		double max = maxSlice();
		if (data.dim > 1 && projection > TWOD) {
			double s = (sliceplane == Y) ? (data.slice + step - 0.5) / max
					: (data.slice + step) / max;
			vpnt.w=s;
			if ((status & SLICELOCK) > 0) {
				switch (sliceplane) {
				case Y:
					vpnt.y=s;
					break;
				case X:
					vpnt.x=s;
					break;
				case Z:
					vpnt.z=s;
					break;
				}
			}
            return comMgr.sendVPntToVnmr(vpnt, wait);
		}
		return false;
	}
   
    private void setSliceFromVPnt() {
        int max = maxSlice();
        if (data.dim > 1 && projection > TWOD) {
            if ((status & SLICELOCK) > 0)
                switch (sliceplane) {
                case Y:
                    data.slice = (int) (max * vpnt.y + 0.5) - step;
                    break;
                case X:
                    data.slice = (int) (max * vpnt.x) - step;
                    break;
                case Z:
                    data.slice = (int) (max * vpnt.z) - step;
                    break;
                }
            else
                data.slice = (int) (max * vpnt.w) - step;
        }
        data.slice = data.slice < 0 ? 0 : data.slice;
        data.slice = data.slice > max ? max : data.slice;
    }

     private int maxSlice(){
        max_slices=1;
        if(data.dim>1){
	        switch(sliceplane){
	        case Y:
	        	real_slices=data.traces;
	            break;
	        case X:
	        	real_slices=data.np;
	            break;
	        case Z:
	        	real_slices=data.slices;
	            break;
	        }
	        max_slices=real_slices;
	        if((projection==THREED) && ((show & SHOWLTYPE)==SHOWPOLYGONS))
	        	max_slices=psuedo_slices;
        }
        return max_slices-step;
    }

     private int numSlices(){
    	 if(data.dim>1){
	         int max=maxSlice()+step;
	         if((prefs & V3DSWIDTH)==0)
	             return max;
	         return (int)(max*slab_width);
    	 }
    	 else
    		 return 1;
     }

     private double widthPlane(){
         int max=maxSlice();
         double w;
         if((status & STATREVERSE)==0){
             w=data.slice+numSlices();
             w=w>max?max:w;
         }
         else{
             w=data.slice-numSlices();
             w=w<0?0:w;
         }            
         return w/max;
     }

    private int maxTrace(){
        int max=0;
        if(data.dim>1){
	        switch(sliceplane){
	        case Y:
	            max=data.slices-step;
	            break;
	        case X:
	            max=data.slices-step;
	            break;
	        default:
	        case Z:
	            max=data.slices-step;
	            break;
	        }
        }
        else
        	max=data.traces-step;
        return max;
    }
    
    private boolean animateSlice(){
    	if((status & ANIMATESLC)!=0)
    		return true;
    	return false;
    }

    private boolean animateRotation(){
    	if((status & ANIMATEROT)!=0)
    		return true;
    	return false;
    }
    private void step(int dir) {
        int maxs;
        int maxt;
        if (animateSlice()) {
            maxs = maxSlice();
            maxt = maxTrace();
            switch (dir) {
            case NEXT:
                switch (projection) {
                case ONETRACE:
                	if(data.dim>0){
	                    data.trace++;
	                    data.trace = data.trace > maxt ? 0 : data.trace;
                	}
                    break;
                case SLICES:
                default:
                	if(data.dim>1){
	                    data.slice += step;
	                    data.slice = data.slice > maxs ? 0 : data.slice;
	                    stepflag = true;
	                    reset_mode |= RESETSLICE;
                	}
                    break;
                }
                break;
            case PREV:
                switch (projection) {
                case ONETRACE:
                	if(data.dim>0){
	                    data.trace--;
	                    data.trace = data.trace < 0 ? maxt : data.trace;
                	}
                    break;
                default:
                case SLICES:
                	if(data.dim>1){
	                    data.slice -= step;
	                    data.slice = data.slice < 0 ? maxs : data.slice;
	                    stepflag = true;
	                    reset_mode |= RESETSLICE;
                	}
                    break;
                }
            }
        }
        if (animateRotation() && projection >=OBLIQUE) {
        	switch(projection){
        	case OBLIQUE:
        		
        		int sign=(dir==NEXT)?1:-1;
                double pstep=sign*0.01;
                double pmax=1-delx_max;
                delx+=pstep*rotation_step;
                if(delx>pmax){
                	step_direction=PREV;
                	delx=pmax;
                }
                else if(delx<-pmax){
                	delx=-pmax;
                	step_direction=NEXT;
                }
         		break;
        	case TWOD:
                switch (dir) {
                case NEXT:
                	twist += rotation_step;
                    break;
                case PREV:
                	twist -= rotation_step;
                    break;
                }
                twist=twist%360.0;
                if(twist<0)
                	twist=360+twist;
                break;
        	default:
                switch (dir) {
                case NEXT:
                	roty += rotation_step;
                    break;
                case PREV:
                	roty -= rotation_step;
                    break;
                }
                roty=roty%360.0;
                if(roty<0)
                	roty=360+roty;        			
        	}
            reset_mode|=RESETROTATION;
        }
    }
 
    // openGl functions
    
    private void drawSelectionTools(){
        if(projection==THREED && (status & SLICELOCK)==0)
            drawSliceVector();
        drawPlanes();
        drawPlaneVectors();
        
    }
    private int selectTool(){
     	if(projection>TWOD && ((prefs & V3DTOOLS) == 0))
    		return 0;
    	if(projection==ONETRACE && cursor_set_mode)
    		return 0;
    	if(projection<=TWOD && !selection_mode)
    		return 0;
        gl=GLU.getCurrentGL().getGL2();
        //IntBuffer selectBuffer = BufferUtil.newIntBuffer(BUFSIZE);
        IntBuffer selectBuffer = Buffers.newDirectIntBuffer(BUFSIZE);

        gl.glSelectBuffer(BUFSIZE, selectBuffer);
        
        render_mode=GL2.GL_SELECT;
        gl.glRenderMode(GL2.GL_SELECT);

        view.pushProjectionMatrix(); 
        view.pickMatrix(mouseX,mouseY,5);

        gl.glInitNames();
        gl.glPushName(0);
 
        gl.glMatrixMode(GL2.GL_MODELVIEW);
        gl.glPushAttrib(GL2.GL_ALL_ATTRIB_BITS);
        gl.glDisable(GL2.GL_TEXTURE_1D);
        gl.glDisable(GL2.GL_TEXTURE_2D);
        gl.glDisable(GL2.GL_TEXTURE_3D);
        gl.glDisable(GL2.GL_BLEND);

        if(projection>TWOD)
        	drawSelectionTools();
        else{
        	gl.glPointSize(10);
        	render();
            gl.glPointSize(point_size);
        }
        
        gl.glPopAttrib();
        
        int hits = gl.glRenderMode(GL2.GL_RENDER);
        int[] buffer = new int[BUFSIZE];
        selectBuffer.get(buffer);
        
        render_mode=GL2.GL_RENDER;
        gl.glPopName();
        gl.glMatrixMode(GL2.GL_PROJECTION);
        view.popMatrix();
        gl.glMatrixMode(GL2.GL_MODELVIEW);
        data_selected=false;
        if(hits>=1){ 
            // select hit with closest z
            int min=buffer[1];
            int sel=0;
            for(int i=1;i<hits;i++){
                if(buffer[i*4+1]<min){
                    min=buffer[i*4+1];
                    sel=i*4;
                }
            }       
            int id=buffer[sel+3];
            int z1=buffer[sel+1];
            int z2=buffer[sel+2];
            double max=4294967295.0; // 2^32-1
            double z=0.5*(double)(z1+z2)/max+0.5;  // ave z value
            selpt=view.pickPoint(mouseX,mouseY,z); // select point (model)
            selproj=view.project(selpt); // select point (screen)
            if(selection_mode){
            	int dpt=id/2;
        		int trace=dpt/data.np;
        		int selpnt=dpt-trace*data.np;
        		int isImag=(id&0x1);
        	    seldata=new Point3D(selpnt,trace,isImag);
            	selvalue=data.get2DValue(selpnt,trace,isImag);
            	data_selected=true;
            	return 0;          	           	
            }

            mslice=data.slice; // current data.slice on selection          
            svselect=false;
            switch(id){
            case SEL_AXIS:
                {
                    boolean locked=(status & AXISLOCK)>0?true:false;
                    if(locked)
                        comMgr.setStatusField(LOCKSINDEX, 0, 2);
                    else
                        comMgr.setStatusField(LOCKSINDEX, 2, 2);
                 }
                 break;
            case SEL_WPLANE: 
                reset_width=slab_width;
                if((status & SLICELOCK)==0)
                    delpt=selpt.add(view.sliceVector());    
                else{
                    setSliceVector();
                    delpt=selpt.add(svect);
                }                   
                dproj=view.project(delpt); // projection of data.slice vector
                reset_mode=RESETWIDTH;
                break;
            case SEL_SPLANE:
                delpt=selpt.add(view.sliceVector());                    
                dproj=view.project(delpt); // projection of data.slice vector
                reset_mode=RESETSVECT;
                break;
            case SEL_XPLANE:
                setSliceVector();
                rstvpnt=new Point3D(vpnt);
                delpt=selpt.add(xaxis);                    
                dproj=view.project(delpt); // projection of vector
                if(((status & SLICELOCK)>0)&&((status & AUTOSELECT)>0)){
                    newsliceplane=true;
                    comMgr.setShowField(AXISINDEX, 2, 0);
                }
                reset_mode=RESETVPNT;
                break;
            case SEL_YPLANE:
                setSliceVector();
                rstvpnt=new Point3D(vpnt);
                delpt=selpt.add(yaxis);                    
                dproj=view.project(delpt); // projection of vector
                if(((status & SLICELOCK)>0)&&((status & AUTOSELECT)>0)){
                    newsliceplane=true;
                    comMgr.setShowField(AXISINDEX, 1, 0);
                }
                reset_mode=RESETVPNT;
                break;
            case SEL_ZPLANE:
                setSliceVector();
                rstvpnt=new Point3D(vpnt);
                delpt=selpt.add(zaxis);                    
                dproj=view.project(delpt); // projection of vector
                if(((status & SLICELOCK)>0)&&((status & AUTOSELECT)>0)){
                    newsliceplane=true;
                    comMgr.setShowField(AXISINDEX, 0, 0);
                }
                reset_mode=RESETVPNT;
                break;
            case SET_SVECT:
                rstsvect=new Point3D(svect);
                rstvrot=new Point3D(vrot);
                vproj=view.project(svect);
                comMgr.setStatusField(LOCKSINDEX, 0,1);
                svselect=true;
                reset_mode=RESETSVECT;
                break;
            case SEL_XAXIS:
            case SEL_YAXIS:
            case SEL_ZAXIS:
                rstvrot=new Point3D(vrot);
                reset_mode=RESETVROT;
                rotating=true;
                break;
            }
            return id;
        }
        return 0;        
    }
    /**
     * Control on-screen opengl SELECT objects 
     * @param id selected tool identifier (glLoadName)
     */
    private void controlTool(int id){
        if(!dragging && !enddrag)
            return;
        Point3D delm=mouse.sub(oldmouse); // delta mouse pt
       // if(delm.length()==0)
      //      return;
        boolean wait=enddrag?false:true;

        delm=mouse.sub(initms); // delta mouse pt
        switch(id){
        case SET_SVECT:
        {
            Point3D mv=vproj.add(delm);   // new svect projection (add delta x,y)
            mv=view.unproject(mv);        // convert to model frame
            mv=mv.add(volume.center());
            svect=volume.minPoint(mv);  // closest vertex along sv-center line
            svect=svect.sub(volume.center());
            break;
        }
        case SEL_XAXIS:
        case SEL_YAXIS:
        case SEL_ZAXIS:       
        {
            double rmax=180.0;
            double dx=rmax*delm.x/width;
            double dy=rmax*delm.y/height;
            boolean vdrag=Math.abs(delms.y)>Math.abs(delms.x);
            boolean locked=(status & VROTLOCK)>0;
            switch(id){
            case SEL_XAXIS:
                if(!locked)
                    vrot=new Point3D(rstvrot.x+dx,rstvrot.y+dy,rstvrot.z);
                else if(vdrag)
                    vrot=new Point3D(rstvrot.x,rstvrot.y+dy,rstvrot.z);
                else
                    vrot=new Point3D(rstvrot.x+dx,rstvrot.y,rstvrot.z);
                break;
            case SEL_YAXIS:
                if(!locked)
                    vrot=new Point3D(rstvrot.x,rstvrot.y+dx,rstvrot.z+dy);
                else if(vdrag)
                    vrot=new Point3D(rstvrot.x,rstvrot.y,rstvrot.z+dy);
                else
                    vrot=new Point3D(rstvrot.x,rstvrot.y+dx,rstvrot.z);
                break;
            case SEL_ZAXIS:
                if(!locked)
                    vrot=new Point3D(rstvrot.x+dx,rstvrot.y,rstvrot.z+dy);
                else if(vdrag)
                    vrot=new Point3D(rstvrot.x,rstvrot.y,rstvrot.z+dy);
                else
                    vrot=new Point3D(rstvrot.x+dx,rstvrot.y,rstvrot.z);
                break;
            }
            setSliceVector();
            vrot.x=vrot.x % 360.0;
            if(vrot.x<0)
                vrot.x+=360.0;
            vrot.y=vrot.y % 360.0;
            if(vrot.y<0)
                vrot.y+=360.0;
            vrot.z=vrot.z % 360.0;
            if(vrot.z<0)
                vrot.z+=360.0;
            view.vrot=vrot;
            comMgr.sendVRotToVnmr(vrot,wait);
            break;
        }
        case SEL_XPLANE:
        case SEL_YPLANE:
        case SEL_ZPLANE:
        case SEL_SPLANE:
        case SEL_WPLANE:
        {
            Point3D mp=new Point3D(mouse.x,mouse.y);     // current mouse pt (screen)
            Point3D sp=new Point3D(selproj.x,selproj.y); // select pt (screen)
            Point3D dp=new Point3D(dproj.x,dproj.y,0);   // proj vector pt (screen)
            Point3D sv=delpt.sub(selpt); // data.slice vector (model)                    
            Point3D ds=dp.sub(sp); // vector-select (screen)
            Point3D dm=mp.sub(sp); 
            double ml=dm.length();
            double dl=ds.length();
            double vl=sv.length();
            double s=0,delta;

            if(((int)ml)==0) // mouse is at selection point
                return;
            if(dl/vl/width<0.05) // data.slice vector is || screen, use xy motion
                delta=(ydrag?dm.y/height:dm.x/width);
            else // mouse tracks data.slice plane screen motion
                delta=vl*dm.dot(ds)/dl/dl; // project mouse-select onto data.slice vector          
            switch(id){
            case SEL_WPLANE:
                if((status & STATREVERSE)>0)
                    s=reset_width-delta;
                else
                    s=reset_width+delta;
                s=s<0?0:s;
                s=s>1?1:s;
                slab_width=s;
                comMgr.sendVFltToVnmr(G3DSWIDTH,s,wait);
                return;
            case SEL_SPLANE:
                int max=maxSlice();
                s=max*delta;
                data.slice=(int)(s+mslice);               
                if(data.slice>max)
                    data.slice=max;
                if(data.slice<0)
                    data.slice=0;
                reset_mode=RESETSLICE;
                s=(float)data.slice/max;
                vpnt.w=s;
                break;
            case SEL_XPLANE:
                s=delta+rstvpnt.x;
                s=s<0?0:s;
                s=s>1?1:s;
                vpnt.x=s;
                if((status & SLICELOCK) > 0 && sliceplane==X)
                    vpnt.w=s;
                setSliceFromVPnt();
                break;
            case SEL_YPLANE:
                s=delta+rstvpnt.y;
                s=s<0?0:s;
                s=s>1?1:s;
                vpnt.y=s;
                if((status & SLICELOCK) > 0 && sliceplane==Y)
                    vpnt.w=s;
                setSliceFromVPnt();
                break;
            case SEL_ZPLANE:
                s=delta+rstvpnt.z;
                s=s<0?0:s;
                s=s>1?1:s;
                vpnt.z=s;
                if((status & SLICELOCK) > 0 && sliceplane==Z)
                    vpnt.w=s;
                setSliceFromVPnt();
                break;
            }
            comMgr.sendVPntToVnmr(vpnt, wait);
        }
        break;
        } 
    }

    private void setLocalView(){
        if(view_invalid){
            view.setReset();
        }
        int options= projection|sliceplane;
        if((status & AXISLOCK)>0)
            options |= FIXAXIS;

        view.setDataScale(data.ymin,data.ymax);
        
        view.setSlice(data.slice,maxSlice(),numSlices());
        view.setSliceVector(svect.x,svect.y,svect.z);
        view.setScale(ascale,xscale,yscale/aspect,zscale/aspect);
        view.setSpan(span.x,span.y,span.z);
        view.setRotation2D(tilt,twist);
        view.setRotation3D(rotx,roty,rotz);
        view.setOffset(xoffset,yoffset,zoffset);
        view.setSlant(delx,dely); 

        view.setOptions(options);
        view.setView();
    }
    
    /** Clear the display to background color
     */
    public void clearDisplay(){
       float A[]=getStdColor(BGCOLOR);
       gl=GLU.getCurrentGL().getGL2();
       gl.glClearColor(A[0], A[1], A[2], 0.0f);
       gl.glClear(GL2.GL_COLOR_BUFFER_BIT | GL2.GL_DEPTH_BUFFER_BIT);           
       gl.glClear(GL2.GL_DEPTH_BUFFER_BIT);           

    }
   /** Draw data content using rendererIF.
    */
   private void render() {
       int rflags=view_invalid?INVALIDVIEW:0;
       if((status & STATREVERSE)>0)
           rflags |=FROMFRNT;
       if((status & AXISLOCK)>0)
           rflags |=LOCKAXIS;
       
       renderer.setDataPtr(data.vertexData);
       renderer.setDataScale(data.ymin,data.ymax);
       renderer.setOffset(xoffset,yoffset,zoffset);
       renderer.setScale(ascale,xscale,yscale/aspect,zscale/aspect);
       renderer.setTrace(data.trace,maxTrace(),maxTrace()); 
       renderer.setSlice(data.slice,maxSlice(),numSlices()); 
       renderer.setIntensity(intensity);
       renderer.setBias(bias);
       renderer.setContrast(contrast);
       renderer.setThreshold(threshold);
       renderer.setContours(contours);
    	   
       renderer.setLimit(limit);
       renderer.setTransparency(transparency*transparency);
       renderer.setAlphaScale(alphascale);
       renderer.setOptions(0,show);
       renderer.setPhase(data.rp,data.lp);
       renderer.setSpan(span.x,span.y,span.z);
       renderer.setObjectRotation(vrot.x,vrot.y,vrot.z);
       renderer.setRotation2D(tilt,twist);
       renderer.setRotation3D(rotx,roty,rotz);
       renderer.setStep((int)step); 
       renderer.setSlant(delx,dely);
       renderer.setSliceVector(svect.x,svect.y,svect.z,svect.w);

       renderer.render(rflags);
       data.setAbsValue(show);
       if(data_selected)
    	   drawSpnt();
       
       renderer.releaseDataPtr(data.vertexData);
       
    }

   /** Draw 3D controls.
    */
    private void draw3DTools(boolean finish){
        gl=GLU.getCurrentGL().getGL2();
        gl.glPushAttrib(GL2.GL_ALL_ATTRIB_BITS);
        gl.glDisable(GL2.GL_TEXTURE_1D);
        gl.glDisable(GL2.GL_TEXTURE_2D);
        gl.glDisable(GL2.GL_TEXTURE_3D);
        gl.glEnable(GL2.GL_BLEND);
        gl.glEnable(GL2.GL_POINT_SMOOTH);
        gl.glEnable(GL2.GL_LINE_SMOOTH);
        gl.glEnable(GL2.GL_MULTISAMPLE);
        if(!finish){  // pre render: draw volume box & axis 
        	drawBoxLabels(finish);
            drawPlaneVectors();
            drawPlanes();
            drawRotationAxis();
            drawDataBox();
        }
        else {        // post render: draw data.slice plane & labels
            drawVpnt();
            drawBoxLabels(finish);
        }
        gl.glDisable(GL2.GL_LINE_SMOOTH);
        gl.glDisable(GL2.GL_MULTISAMPLE);
        gl.glPopAttrib();
     }

   /** Draw axis.
    */
    private void drawRotationAxis(){
        if((prefs & V3DROTAXIS)==0)
            return;
        if(render_mode==GL2.GL_SELECT)
            gl.glLoadName(SEL_AXIS);  
        short pattern=0x0f0f;
        gl.glLineStipple(3, pattern);
        gl.glEnable(GL2.GL_LINE_STIPPLE);
        gl.glColor3f(0, 1, 0);
        gl.glBegin(GL2.GL_LINES);
        if((status & AXISLOCK)>0)
            gl.glColor3f(1, 0, 0); 
        else
            gl.glColor3f(0, 1, 0); 
        Point3D p0,p1;
        p0=volume.center();
        p1=view.yaxis;
        p1=p1.add(p0);
        gl.glVertex3d(p0.x,p0.y,p0.z);
        gl.glVertex3d(p1.x,p1.y,p1.z);                
        p1=view.xaxis;
        p1=p1.add(p0);
        if((status & AXISLOCK)>0)
            gl.glColor3f(1, 0, 0);
        else
            gl.glColor3f(0, 1, 0); 
        gl.glVertex3d(p0.x,p0.y,p0.z);
        gl.glVertex3d(p1.x,p1.y,p1.z);    
        gl.glEnd();
        gl.glDisable(GL2.GL_LINE_STIPPLE);
    }
 
    private void drawBoxLabels(boolean finish) {
		if ((prefs & VLABELS) == 0)
			return;
		float scale = label_scale / 1000;
		Point3D pc = volume.center();
		Point3D p1 = volume.faceCenter(MINUS_Z);

		gl.glDisable(GL2.GL_DEPTH_TEST);
		gl = GLU.getCurrentGL().getGL2();
		gl.glMatrixMode(GL2.GL_MODELVIEW);
		gl.glPushMatrix();
		gl.glLoadIdentity();
		gl.glTranslated(pc.x, pc.y, pc.z);

		boolean plus_front = view.faceIsFrontFacing(PLUS_Z);
		p1 = volume.faceCenter(MINUS_Z).sub(pc);
		// label Z faces
		if(finish){ // draw front faces
			if(plus_front){
				gl.glRotated(180, 0, 1, 0);
				drawText(axislabel[MINUS_Z], 0, 0, p1.z, scale, ZAXISCOLOR);
			}
			else
			    drawText(axislabel[PLUS_Z], 0, 0, p1.z, scale, ZAXISCOLOR);
		}
		else{       // draw back faces
			if(plus_front){
				gl.glRotated(180, 0, 1, 0);
				drawText(axislabel[PLUS_Z], 0, 0, -p1.z, scale, ZAXISCOLOR);
			}
			else
				drawText(axislabel[MINUS_Z], 0, 0, -p1.z, scale, ZAXISCOLOR);			
		}

		gl.glLoadIdentity();
		gl.glTranslated(pc.x, pc.y, pc.z);

		// label Y faces
		plus_front = view.faceIsFrontFacing(PLUS_Y);
		p1 = volume.faceCenter(PLUS_Y).sub(pc);
		if(finish){ // draw front faces
			if(plus_front){
				gl.glRotated(90, 1, 0, 0);
				drawText(axislabel[PLUS_Y], 0, 0, -p1.y, scale, YAXISCOLOR);
			}
			else{
				gl.glRotated(270, 1, 0, 0);
			    drawText(axislabel[MINUS_Y], 0, 0, -p1.y, scale, YAXISCOLOR);
			}
		}
		else{       // draw back faces
			if(plus_front){
				gl.glRotated(90, 1, 0, 0);
				drawText(axislabel[MINUS_Y], 0, 0, p1.y, scale, YAXISCOLOR);
			}
			else{
				gl.glRotated(270, 1, 0, 0);
			    drawText(axislabel[PLUS_Y], 0, 0, p1.y, scale, YAXISCOLOR);
			}
		}
		
		// label X faces
		gl.glLoadIdentity();
		gl.glTranslated(pc.x, pc.y, pc.z);

		plus_front = view.faceIsFrontFacing(PLUS_X);
		p1 = volume.faceCenter(PLUS_X).sub(pc);
		if(finish){ // draw front faces
			if(plus_front){
				gl.glRotated(270, 0, 1, 0);
				drawText(axislabel[PLUS_X], 0, 0, -p1.x, scale, XAXISCOLOR);
			}
			else{
				gl.glRotated(90, 0, 1, 0);
			    drawText(axislabel[MINUS_X], 0, 0, -p1.x, scale, XAXISCOLOR);
			}
		}
		else{       // draw back faces
			if(plus_front){
				gl.glRotated(270, 0, 1, 0);
				drawText(axislabel[MINUS_X], 0, 0, p1.x, scale, XAXISCOLOR);
			}
			else{
				gl.glRotated(90, 0, 1, 0);
			    drawText(axislabel[PLUS_X], 0, 0, p1.x, scale, XAXISCOLOR);
			}
		}
		gl.glPopMatrix();
		gl.glEnable(GL2.GL_DEPTH_TEST);
	}

     private void drawDataBox(){
         if((prefs & V3DBOX)==0)
             return;
        int i;
        Point3D[] pts;
        gl.glLineWidth(2.0f);
        float A[]=getStdColor(BOXCOLOR);
        gl.glColor3f(A[0],A[1],A[2]);

        //      front face
        pts=volume.getFace(MINUS_Z);
        gl.glBegin(GL2.GL_LINE_LOOP);        
        for(i=0;i<4;i++){
            gl.glVertex3d(pts[i].x,pts[i].y,pts[i].z);            
        }
        gl.glEnd();

        // back face
        pts=volume.getFace(PLUS_Z);
        gl.glBegin(GL2.GL_LINE_LOOP);
        for(i=0;i<4;i++)
            gl.glVertex3d(pts[i].x,pts[i].y,pts[i].z);
        gl.glEnd();
        
        // top face
        pts=volume.getFace(PLUS_Y);
        gl.glBegin(GL2.GL_LINE_LOOP);
        for(i=0;i<4;i++)
            gl.glVertex3d(pts[i].x,pts[i].y,pts[i].z);
        gl.glEnd();

        // bottom face
        pts=volume.getFace(MINUS_Y);
        gl.glBegin(GL2.GL_LINE_LOOP);
        for(i=0;i<4;i++)
            gl.glVertex3d(pts[i].x,pts[i].y,pts[i].z);
        gl.glEnd();

    }
     /** Draw data.slice vector.
      */
      private void drawVector(Point3D p, int c) {
		
		view.pushRotationMatrix();
		
		p = view.matrixMultiply(p);
		view.popMatrix();

		Point3D p1 = volume.center().sub(p).scale(0.999);
		Point3D p0 = p.add(volume.center()).scale(0.999);

		float A[] = getStdColor(c);
		gl.glColor3f(A[0], A[1], A[2]);
		gl.glBegin(GL2.GL_LINES);
		gl.glVertex3d(p0.x, p0.y, p0.z);
		gl.glVertex3d(p1.x, p1.y, p1.z);
		gl.glEnd();
		gl.glPointSize(point_size);
		if (p == svect) {
			A = getStdColor(EYECOLOR);
			gl.glColor3f(A[0], A[1], A[2]);
		}

		gl.glPointSize(8);
		gl.glBegin(GL2.GL_POINTS);
		gl.glVertex3d(p0.x, p0.y, p0.z);

		gl.glVertex3d(p1.x, p1.y, p1.z);
		gl.glEnd();

	}
      /** Draw data.slice vector.
       */
       private void drawPlaneVectors(){
           if(projection !=THREED)
               return;
           if((prefs & V3DROT)>0){              
               drawXAxis();
               drawYAxis();
               drawZAxis();
           }
           if((status & SLICELOCK)==0)
               drawSliceVector();
       }
       
    /** Draw data.slice vector.
     */
     private void drawSliceVector(){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SET_SVECT);
         
         drawVector(svect,EYECOLOR);
     }
     private void drawPlanes(){
         gl.glEnable(GL2.GL_DEPTH_TEST);
         drawPlanes(false);
         if(render_mode==GL2.GL_SELECT)
             return;
         gl.glDisable(GL2.GL_DEPTH_TEST);
         drawPlanes(true);
         gl.glEnable(GL2.GL_DEPTH_TEST);
     }
     private void drawVpnt(){
         if((prefs & V3DPNT)==0 && (prefs & V3DROT)==0)
             return;
         view.pushRotationMatrix();
         Point3D p=new Point3D(vpnt.x,vpnt.y,vpnt.z);
         p.y-=0.5;
         p=p.mul(volume.scale);
         p=p.sub(volume.center());
         p=view.matrixMultiply(p);
         
         p=p.add(volume.center());

         view.popMatrix();
         gl.glColor3f(1, 0, 0);
         gl.glPointSize(6);
         gl.glBegin(GL2.GL_POINTS);
 
         gl.glVertex3d(p.x,p.y,p.z);
         gl.glEnd();
     }

     private void drawSpnt(){
         gl.glColor3f(0, 1, 0);
         gl.glPointSize(6);
         gl.glBegin(GL2.GL_POINTS);
 
         renderer.render2DPoint((int)seldata.x, (int)seldata.y, (int)seldata.z);
         gl.glEnd();
         gl.glPointSize(point_size);
     }

     private void drawPlanes(boolean fill){
         gl.glMatrixMode(GL2.GL_MODELVIEW);
         if((status & SLICELOCK)==0)
             drawSPlane(fill);
         if((prefs & V3DTOOLS)>0)
             drawVPlanes(fill);
     }

     private void drawVPlanes(boolean fill){
         //if((status & VPNT)==0)
         if((prefs & V3DPNT)==0)
             return;
         drawXPlane(fill);
         drawYPlane(fill);
         drawZPlane(fill);        
     }
     private void drawWPlane(boolean fill, int color, Point3D p){
         double s=widthPlane();
         if(s==0 || s>=1 || (prefs & V3DSWIDTH)==0)
             return;
         
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_WPLANE);
         else{
             short pattern=0x0f0f;
             gl.glLineStipple(3, pattern);
             gl.glEnable(GL2.GL_LINE_STIPPLE);
         }
         drawPlane(fill,color,p,s,true);
         gl.glDisable(GL2.GL_LINE_STIPPLE);
     }
     private void drawSPlane(boolean fill){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_SPLANE); 
         double max=maxSlice();
         double s=((double)data.slice)/max;
         Point3D p=view.sliceVector();
         drawPlane(fill,EYECOLOR,p,s,true);
         drawWPlane(fill,EYECOLOR,p);
     }
     private void drawXPlane(boolean fill){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_XPLANE);
         Point3D p=volume.XPlane();
         boolean selected=(status & SLICELOCK)>0 && (sliceplane==X);
         drawPlane(fill,XAXISCOLOR,p,vpnt.x,selected);
         if(selected)
             drawWPlane(fill, XAXISCOLOR, p);
     }
     private void drawYPlane(boolean fill){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_YPLANE);  
         Point3D p=volume.YPlane();
         boolean selected=(status & SLICELOCK)>0 && (sliceplane==Y);
         drawPlane(fill,YAXISCOLOR,p,vpnt.y,selected);
         if(selected)
             drawWPlane(fill, YAXISCOLOR, p);
     }
     private void drawZPlane(boolean fill){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_ZPLANE);
         Point3D p=volume.ZPlane();
         boolean selected=(status & SLICELOCK)>0 && (sliceplane==Z);
         drawPlane(fill,ZAXISCOLOR,p,vpnt.z,selected);
         if(selected)
             drawWPlane(fill, ZAXISCOLOR, p);
     }
     private void drawXAxis(){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_XAXIS);  
         drawVector(xaxis,XAXISCOLOR);
     }
     private void drawYAxis(){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_YAXIS);  
         drawVector(yaxis,YAXISCOLOR);
     }
     private void drawZAxis(){
         if(render_mode==GL2.GL_SELECT)
             gl.glLoadName(SEL_ZAXIS);  
         drawVector(zaxis,ZAXISCOLOR);
     }
     
    /**
     * draw a plane for an orthogonal axis using the cross product method
     * @param fill
     * @param c
     * @param n
     * @param s
     * @param is_sliceplane
     */
    private void drawOrthoPlane(boolean fill, int c, Point3D n, double s, boolean is_sliceplane){
         int i;
         gl.glLineWidth(2.0f);  
         Point3D[] spts=new Point3D[4];
         
         // find a vector (t)  on the plane perpendicular to n
         // using the relationship n.dot(t) = 0
         Point3D t=new Point3D(-n.y,n.x,0); // one possibility
         if(n.y==0)
        	 t=new Point3D(-n.z,0,n.x); // another (must have at least 2 non-zero components)
         
         Point3D b=n.cross(t);
         view.pushRotationMatrix();
         n=view.matrixMultiply(n);
         t=view.matrixMultiply(t);
         b=view.matrixMultiply(b);

         view.popMatrix();
                
         float A[]=getStdColor(c);
         float a=is_sliceplane?slice_xparancy:plane_xparancy;
         
         gl.glColor3f(A[0],A[1],A[2]);
         s=s>0.999?0.999:s;
         spts[0]=t.add(b);
         spts[1]=b.sub(t);
         spts[2]=t.minus().sub(b);
         spts[3]=t.sub(b);

         for(i=0;i<spts.length;i++)
             spts[i]=spts[i].add(volume.center());

         gl.glBegin(GL2.GL_LINE_LOOP);
         for(i=0;i<spts.length;i++)
             gl.glVertex3d(spts[i].x,spts[i].y,spts[i].z);
        	        	 
         gl.glEnd();
         if(fill){
             gl.glColor4f(A[0],A[1],A[2],a);       
             gl.glBegin(GL2.GL_TRIANGLE_FAN);
             for(i=0;i<spts.length;i++)
                 gl.glVertex3d(spts[i].x,spts[i].y,spts[i].z);
             gl.glEnd();
         }
     }
 
    /**
     * draw a plane for an orthogonal axis using the plane equation method
     * @param fill
     * @param c
     * @param p
     * @param s
     * @param is_sliceplane
     * 
     * note: data scaling and cross product (or plane) operations are NOT
     *       associative. The orthogonal vectors need to be determined first (with a
     *       zero-based origin). These vectors are then rotated and scaled
     */
    private void drawPlane(boolean fill, int c, Point3D p,double s, boolean is_sliceplane){
         int i;
         gl.glLineWidth(2.0f);  
         Point3D[] spts;
         
         Point3D pc=volume.center();
         
         if((status & CLIPVPLANES)>0){ // clip rotated slice planes to volume box 
	         view.pushRotationMatrix();
	         Point3D pt=view.matrixMultiply(p);
	         view.popMatrix();
	
	         Point3D min=pc.sub(pt);
	         Point3D max=pt.add(pc);
	                  
	         Point3D plane=min.plane(max, s);
	        
	         spts=volume.boundsPts(plane);
         }
         else{
	         Point3D min=pc.sub(p);
	         Point3D max=p.add(pc);
	                  
	         Point3D plane=min.plane(max, s);
	         spts=volume.boundsPts(plane);
	         view.pushRotationMatrix();
	         for(i=0;i<spts.length;i++){
	        	 spts[i]=spts[i].sub(pc);
	        	 spts[i]=view.matrixMultiply(spts[i]);
	        	 spts[i]=spts[i].add(pc);
	         }
	         view.popMatrix();
         }

         float A[]=getStdColor(c);
         float a=is_sliceplane?slice_xparancy:plane_xparancy;
         
         gl.glColor3f(A[0],A[1],A[2]);
         s=s>0.999?0.999:s;
          
         gl.glBegin(GL2.GL_LINE_LOOP);
         for(i=0;i<spts.length;i++)
             gl.glVertex3d(spts[i].x,spts[i].y,spts[i].z);
         gl.glEnd();
         if(fill){
             gl.glColor4f(A[0],A[1],A[2],a);       
             gl.glBegin(GL2.GL_TRIANGLE_FAN);
             for(i=0;i<spts.length;i++)
                 gl.glVertex3d(spts[i].x,spts[i].y,spts[i].z);
             gl.glEnd();
         }
    }

    private void drawText(String text, Point3D p, double scale, int c) {
        float A[]=getStdColor(c);
        text3D.begin3DRendering();
        text3D.setColor(A[0], A[1], A[2], 1.0f);

        //text3D.setColor(c);
        text3D.draw3D(text, (float)(p.x), (float)(p.y), (float)(p.z),
                (float)scale);
        text3D.end3DRendering();
    }
    private void drawText(String text, double x, double y, double z, double scale, int c) {
    	drawText(text,new Point3D(x,y,z),scale,c);
    }

    private class ZoomState {
    	public double cursor1;
    	public double cursor2;
    	public double xoffset;
    	public double xscale;
    	ZoomState(double x1, double xs,double c1, double c2){
    		xoffset=x1;
    		xscale=xs;
    		cursor1=c1;
    		cursor2=c2;
    	}
    }

    @Override
    public void dispose(GLAutoDrawable arg0) {
        // TODO Auto-generated method stub       
    }

    void createImage() {
        if(!initialized) {
            System.out.println("OpenGL Window not initialized (exiting)");
  		  return;
        }

		JFrame frame= new JFrame();
		frame.setTitle("Save Image");		
		JFileChooser fc= new JFileChooser();			
		fc.setFileSelectionMode(JFileChooser.FILES_ONLY);
		fc.setDialogTitle("Save Image");
		fc.setAcceptAllFileFilterUsed(false);
		fc.addChoosableFileFilter(new PNGFilter());
		fc.addChoosableFileFilter(new JPGFilter());
		fc.addChoosableFileFilter(new GIFFilter());		
		
		fc.setAccessory(new ImageOptions());

		if(savepath!=null)
			fc.setSelectedFile(new File(savepath));
		int returnVal= fc.showSaveDialog(frame);			
		if(returnVal==JFileChooser.APPROVE_OPTION){
			File file= fc.getSelectedFile();
			if(file==null)
				return;
			savepath=file.getPath();
			String ext=getExtension(savepath);
			FileFilter filter=fc.getFileFilter();
			String fext=filter.getDescription();
			if(ext==null){
				savepath=savepath+"."+fext;
			}
			else if(!fext.equals(ext)){
				File dir=fc.getCurrentDirectory();
				String f=file.getName();
				f=f.substring(0, f.length()-4)+"."+fext;
				//fc.setSelectedFile(new File(f));
				savepath=dir.getPath()+"/"+f;
			}
			System.out.println(savepath);
	    	createImage(savepath,(int)(image_width),(int)(image_height));
		}
    }
	public class ImageOptions extends JPanel implements ActionListener{		
		private static final long serialVersionUID = 1L;
		JTextField width_entry;
		JTextField height_entry;
		JComboBox scale_menu;
        String[] scaleChoices = {"1.0","2.0","3.0","4.0"};


	    public ImageOptions() {
	        setPreferredSize(new Dimension(100, 50));
	        setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
	        scale_menu=new JComboBox(scaleChoices);
	        //scale_menu.setEditable(true);
	        scale_menu.setActionCommand("scale");
	        scale_menu.getEditor().setItem(Double.toString(image_scale));
	        scale_menu.setSelectedIndex((int)(image_scale-1));
	        
	        image_width = (int)(width*image_scale);
	        image_height = (int)(height*image_scale);
	        
	        width_entry=new JTextField(Integer.toString(image_width));
	        width_entry.setEditable(true);
	        width_entry.setActionCommand("width");

	        height_entry=new JTextField(Integer.toString(image_height));
	        height_entry.setEditable(true);
	        height_entry.setActionCommand("height");

	        setBorder(new BevelBorder(BevelBorder.RAISED));
	        add(new JLabel("Width"));
	        add(width_entry);
	        add(new JLabel("Height"));
	        add(height_entry);
	        add(new JLabel("Scale"));
	        add(scale_menu);

	        width_entry.addActionListener(this);
	        height_entry.addActionListener(this);
	        scale_menu.addActionListener(this);

	    }
	    // Methods required for the implementation of ActionListener

	    /** handle action events. */
	    public void actionPerformed(ActionEvent e ) {
	        String cmd = e.getActionCommand();
	        if(cmd.equals("width")){
	        	String w=(String)width_entry.getText();
	        	try{
	        		image_width=Integer.parseInt(w);
	        	}
	        	catch (NumberFormatException err){
	        		System.out.println("Invalid integer string:"+w);
	        	}
	        }
	        if(cmd.equals("height")){
	        	String h=(String)height_entry.getText();
	        	try{
	        		image_height=Integer.parseInt(h);
	        	}
	        	catch (NumberFormatException err){
	        		System.out.println("Invalid integer string:"+h);
	        	}
	        }
	        if(cmd.equals("scale")){
	        	int index=scale_menu.getSelectedIndex();
	        	try{
	        		image_scale=index+1;
	        		image_width=(int)(width*image_scale);
	        		image_height=(int)(height*image_scale);
	        		width_entry.setText(Integer.toString(image_width));
	        		height_entry.setText(Integer.toString(image_height));
	        	}
	        	catch (NumberFormatException err){
	        		System.out.println("Invalid real string");
	        	}
	        }
	    }
	}

	void createImage(String file,int width,int height) {
		// Create GLDrawable, GLContext and make GLContext current for off-screen rendering
		imagecontext=true;
		
		GLProfile glProfile = GLProfile.getDefault(GLProfile.getDefaultDevice());
		GLCapabilities imagecaps = new GLCapabilities(glProfile);
		imagecaps = imagecaps.copyFrom(caps);
		imagecaps.setOnscreen(false); // single-buffered
		imagecaps.setDoubleBuffered(false);
		imagecaps.setPBuffer(false);  //
		GLDrawableFactoryImpl factory = GLDrawableFactoryImpl.getFactoryImpl(glProfile);
		
		GLOffscreenAutoDrawable glad = factory.createOffscreenAutoDrawable(null, imagecaps, null, width, height, null);
		glad.setRealized(true);
		glad.addGLEventListener(this);  // let this class do the drawing

		GLContext offscreenContext = (GLContextImpl) glad.getContext();

		offscreenContext.makeCurrent();
		
		glad.display(); // render off-screen
		
		// read back pixels into a BufferedImage
		
		AWTGLReadBufferUtil awtGLReadBufferUtil = new AWTGLReadBufferUtil(glad.getGLProfile(), false);
        image = awtGLReadBufferUtil.readPixelsToBufferedImage(glad.getGL(), 0, 0, width, height, true);
			
        if(file !=null){ // optional: save the image as a .png file
        	File imageFile = new File(file);
        	String ext=getExtension(imageFile);
        	try {
				ImageIO.write(image, ext, imageFile);
				System.out.println("Image saved at:"+file+" "+width+"x"+height);
			} catch (IOException e) {
				System.out.println("Error: failed to create image file:"+file);
				image=null;
			} 
        }
        // clear out off-screen resources
        awtGLReadBufferUtil.dispose(glad.getGL());
		glad.destroy();
		
		// Create a new GLContext and make current 
		// - will cause init and resize to get called with on-screen caps and geometry
		GLContext normalContext = (GLContextImpl) createContext(null);
		setContext(normalContext, true); // true causes the previous context to be destroyed		
		setRealized(true);
		imagecontext=false;
	}
	
	/**
	 * Create an off screen image for printing
	 */
	void printImage(int mode){
        if(!initialized) {
            System.out.println("OpenGL Window not initialized (exiting)");
  		  return;
        }
        PrinterJob pj = PrinterJob.getPrinterJob();
        PrintService printer = pj.getPrintService();
        if(printer == null) {
          System.out.println("No Printer Available (exiting)");
		  return;
        }

       //if (pj.printDialog()) {
            PageFormat pf =pj.defaultPage();
            Paper paper = pf.getPaper();
            pf.setPaper(paper);            
            PageFormat pageFormat = pj.validatePage(pf);
            pageFormat.setOrientation(PageFormat.LANDSCAPE);
            int page_width = (int)pageFormat.getImageableWidth();
            int page_height = (int)pageFormat.getImageableHeight();
            
            double page_aspect=(double)page_width/page_height;
            double view_aspect=(double)width/height;
                        
            if(page_aspect>view_aspect){
            	pageFormat.setOrientation(PageFormat.PORTRAIT);
            	print_height=page_width;
            	print_width=(int)(print_height*view_aspect);
            }
            else{
            	print_width=page_width;
                print_height=(int)(print_width/view_aspect);
            }
            if(DebugOutput.isSetFor("glprint")){
	            System.out.println("page w:"+page_width+" h:"+page_height+" a:"+page_aspect);
	            System.out.println("view w:"+width+" h:"+height+" a:"+view_aspect);
	            System.out.println("print w:"+print_width+" h:"+print_height+" a:"+(double)print_width/print_height);
            }
           
            createImage("/tmp/tmpimage.png",(int)(print_width*print_scale),(int)(print_height*print_scale));
            if(mode==3){  // test mode: create image but don't print it
            	return;
            }
            pj.setPrintable(this, pageFormat);
            try {
                pj.print();
            } catch (PrinterException ex) {
                ex.printStackTrace();
            }
       // }
		System.out.println("Printing ...");
	}
	
	/* Implements "Printable"
	 * @see java.awt.print.Printable#print(java.awt.Graphics, java.awt.print.PageFormat, int)
	 */
	@Override
	public int print(Graphics graphics, PageFormat pageFormat, int pageIndex)
			throws PrinterException {
        int result = NO_SUCH_PAGE;
        if (pageIndex < 1) {
            Graphics2D g2d = (Graphics2D) graphics;
           
            g2d.translate((int) pageFormat.getImageableX(),
                            (int) pageFormat.getImageableY());
            createImage(null,(int)(print_width*print_scale),(int)(print_height*print_scale));
            if(image !=null){
                g2d.setRenderingHint(RenderingHints.KEY_INTERPOLATION,
                        RenderingHints.VALUE_INTERPOLATION_BILINEAR);
            	g2d.drawImage(image, 0, 0,(int)(print_width),(int)(print_height), null);
            }
            result = PAGE_EXISTS;
        }
        return result;
	}
    public static String getExtension(String s) {
        String ext = null;
        int i = s.lastIndexOf('.');
 
        if (i > 0 &&  i < s.length() - 1) {
            ext = s.substring(i+1).toLowerCase();
        }
        return ext;
    }
   public static String getExtension(File f) {
        return getExtension(f.getName());
   }
   
   boolean isSupportedImageType(String s){
	   if(s==null)
		   return false;
       if (s.equals("png")||s.equals("jpg")||s.equals("gif")){
           return true;
       } else {
           return false;
       }
	   
   }
	public class PNGFilter extends FileFilter {			 
	    public boolean accept(File f) {
	        if (f.isDirectory()) {
	            return true;
	        }		 
	        return isSupportedImageType(getExtension(f));
	    }		 
	    public String getDescription() {
	        return "png";
	    }
	}
	public class JPGFilter extends FileFilter {			 
	    public boolean accept(File f) {
	        if (f.isDirectory()) {
	            return true;
	        }		 
	        return isSupportedImageType(getExtension(f));
	    }		 
	    public String getDescription() {
	        return "jpg";
	    }
	}
	public class GIFFilter extends FileFilter {			 
	    public boolean accept(File f) {
	        if (f.isDirectory()) {
	            return true;
	        }		 
	        return isSupportedImageType(getExtension(f));
	    }		 
	    public String getDescription() {
	        return "gif";
	    }
	}
}

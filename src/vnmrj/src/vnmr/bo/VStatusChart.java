/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.awt.geom.Point2D;
import java.awt.print.PageFormat;
import java.awt.print.Printable;
import java.awt.print.PrinterException;
import java.io.*;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.Timer;
import java.beans.*;

import vnmr.ui.*;
import vnmr.util.*;

/**
 * A two-line button that shows Infostat data.
 */
public class VStatusChart extends JPanel
    implements DropTargetListener, VEditIF, VObjDef, VObjIF, StatusListenerIF,
               VStatusIF, PropertyChangeListener, VStatusChartIF
{
    private String  type = null;
    private String  fg = "black";
    private String  isEnabled = "yes";
    private Color   fgColor = null;
    private boolean autoscale = true;
    private boolean time_of_day = true;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private ButtonIF vnmrIf;
    private MouseAdapter ml;
    private String  fontName = "Dialog";
    private String  fontStyle = "Bold";
    private String  fontSize = "12";
    private String  statcol = "fg";
    private double  set_value = 0;
    private int     data_max = -1;
    private int     data_cnt = 0;
    private int     set_cnt = 0;
    private int     status_index = -1;
    private double  range_max = -100000;
    private double  range_min = 100000;
    private boolean show_max = false;
    private boolean show_status = false;
    private boolean logging = false;

    private String  title = null;
    private boolean show = true;
    protected String  statkey = null;
    protected String  statpar = null;
    protected String  statset = null;
    protected int     state = UNSET;
    protected String  valstr = "0";
    protected String  setval = null;
    protected String precision = null;
    protected String vnmrVar = null;
    protected String setValue = null;
    private boolean showset = false;
    private JLabel  jtitle = null;
    private JLabel  jvalue = null;
    private JPanel  jlabels = null;
    private int numdigits=2;
    private Dimension chart;
    private int     init_width = 160;
    private int     init_height = 140;
    private final static Font bold = DisplayOptions.getFont("Dialog",Font.BOLD,12);
    private final static Font italics = DisplayOptions.getFont("Dialog",Font.ITALIC,12);
    private final static Font plain = DisplayOptions.getFont("Dialog",Font.PLAIN,12);
    private final static Font small = DisplayOptions.getFont("Dialog",Font.PLAIN,11);
    private SPlot  plotpanel=null;
    private long start=0,last=0;
    boolean testmode=false;
    boolean timemode=false;
    boolean tmr_running=false;
    boolean show_target=false;
    
    private double testval=0.0;
    private double status_value=0;
    private static Calendar cal=Calendar.getInstance();

    private SMenu   menu=null;

    private Dimension defDim;
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);

    private static int ONID=0;
    private static int OFFID=1;
    private static int INTERACTIVEID=2;
    private static int NPID=3;
    private static String[] stat_names = {"On","Off","Interactive","NotPresent"};
    private Color  stat_colors[]={
    		Global.ONCOLOR,
            Global.OFFCOLOR,
            Global.INTERACTIVECOLOR,
            Global.NPCOLOR};
    private Font  stat_fonts[]= {plain, plain, italics, plain};

    private Color statcolor=Color.BLACK;
    private Font statfont=plain;
    
    private static String[] graph_names = {
    	"Background",
    	"Grid",
    	"Tick",
    	"Foreground",
    	"Foreground2",
    	"Foreground3",
    	"Text"
    	};
    
    private static int BG=0;
    private static int GRID=1;
    private static int TICK=2;
    private static int LINES=3;
    private static int POINTS=4;
    private static int TARGET=5;
    private static int LABELS=6;

    private Color  graph_colors[]={
    		Color.LIGHT_GRAY,
    		Color.WHITE,
    		Color.BLACK,
    		Color.BLUE,
    		Color.BLACK,
    		Color.RED,
    		Color.GREEN,
    		Color.BLACK
    		};
    private static Font graph_font=DisplayOptions.getFont("Dialog",Font.BOLD,12);
    private FileWriter writer=null;
    protected SimpleDateFormat _dateFormat = null;
    protected String logfile=null;
    protected int repeat_delay=5000;
    private java.util.Timer timer=null;
    private java.util.TimerTask tester=null;
    
    //  protected static final ResourceBundle labels
    //                      = ResourceBundle.getBundle("vnmr.properties.Status");

    /** constructor (for LayoutBuilder) */
    public VStatusChart(SessionShare sshare, ButtonIF vif, String typ) {
        if(Global.STATUS_TO_FG==false)
            statcol=Util.getLabel("mlBg","bg");
        else
            statcol=Util.getLabel("mlFg","fg");
        setLayout(new BorderLayout());
        chart=new Dimension(200,100);
        jlabels=new JPanel();
        jlabels.setLayout(new BorderLayout());
        jlabels.setBorder(BorderFactory.createLoweredBevelBorder());
        jtitle=new JLabel("title");
        jvalue=new JLabel("value");
        jlabels.add(jtitle, BorderLayout.WEST);
        jlabels.add(jvalue, BorderLayout.EAST);
 
        add(jlabels, BorderLayout.SOUTH);
        setBorder(BorderFactory.createEtchedBorder());
        type = typ;
        vnmrIf = vif;
        setTitleColor(fgColor);
        getFonts();
        changeFont();
        setValueString("     ");
        defDim = new Dimension(init_width, init_height);
        setPreferredSize(defDim);
        setSize(defDim);
        
        //createPlotPanel();
        
       // menu=new SMenu();
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if((modifier & InputEvent.BUTTON3_MASK) !=0)
                    menuAction(evt);
                else if ((modifier & InputEvent.BUTTON1_MASK) != 0 && clicks >= 2)
                    ParamEditUtil.setEditObj((VObjIF) evt.getSource());
            }
        };
        addMouseListener(ml);
       // plotpanel.addMouseListener(ml);
        DisplayOptions.addChangeListener(this);
        //init_data();
    }

    /** show popup menu. */
    private void menuAction(MouseEvent  e) {
    	Point p=e.getPoint();
    	if(menu !=null)
        menu.show(this,(int)p.getX(),(int)p.getY());
    }

    private void getFonts(){
        for(int i=0;i<stat_names.length;i++){
            String name=DisplayOptions.getOption(DisplayOptions.COLOR,stat_names[i]);
            if(name !=null){
                stat_colors[i]=DisplayOptions.getColor(name);
                stat_fonts[i]=DisplayOptions.getFont(stat_names[i]);
            }
        }
        graph_font = DisplayOptions.getFont("GraphText", "GraphText", "GraphText");
    }

    private void getColors(){
        for(int i=0;i<graph_names.length;i++){
            String name="Graph"+graph_names[i];
            if(DisplayOptions.hasColor(name)){
                graph_colors[i]=DisplayOptions.getColor(name);
            }
        }
     }

    private void setColors(){
        if(plotpanel !=null){
        	plotpanel.setPlotBackground(graph_colors[BG]);
        	plotpanel.setGridColor(graph_colors[GRID]);
        	plotpanel.setTickColor(graph_colors[TICK]);
            plotpanel.setMarkColor(graph_colors[LINES], 0);
            plotpanel.setMarkColor(graph_colors[POINTS], 1);
            plotpanel.setMarkColor(graph_colors[TARGET], 2);
        }
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        if(DisplayOptions.isUpdateUIEvent(evt))
            SwingUtilities.updateComponentTreeUI(this);

        getFonts();
        getColors();
        
        setState(state);
        setColors();
        repaint();
    }

    public void updateValue(Vector params)
        {
            String param;
            if (vnmrVar == null)
                return;

            StringTokenizer objTokenizer = new StringTokenizer(vnmrVar);
            while (objTokenizer.hasMoreTokens())
            {
                param = objTokenizer.nextToken();
                if (params.contains(param))
                    updateValue();
            }
        }

    public void updateValue()
        {
            if (vnmrIf == null)
                return;
            if (setValue != null)
                vnmrIf.asyncQueryParam(this, setValue, precision);
        }

    /** Update state from Infostat. */
    public void updateStatus(String msg) {
        if (msg == null)
            return;
        boolean bRepaint = false;
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals(statkey)) {
                parm = tok.nextToken();
                state = getState(parm);
                setState(state);
                bRepaint = true;
            }
            else if (parm.equals(statpar)) {
                parm = tok.nextToken();
                if (!parm.equals("-")) {
                    valstr = parm;
                    setStatusValue(Double.parseDouble(valstr));
                    if (tok.hasMoreElements())
                        valstr += " " + tok.nextToken();
                    setState(state);
                    bRepaint = true;
                }
            }
            else if (show_target && parm.equals(statset)){
                parm = tok.nextToken();
                if (parm !=null && !parm.equals("-")) {
                    setSetValue(set_value);
                    setval = parm;
                    set_value=Double.parseDouble(setval);
                    setSetValue(Double.parseDouble(setval));
                    if (tok.hasMoreElements())
                        setval += " " + tok.nextToken();
                    setState(state);
                    bRepaint = true;
                }
            }
        }
        if (bRepaint)
            repaint();
    }

    public void setValue(ParamIF pf)
        {
            String strValue;
            if (pf != null)
            {
                strValue = pf.value;
                setValue(strValue);
            }

        }

    public void setValue(String value)
        {
            try
            {
                setStatusValue(Double.parseDouble(value));
                setState(state);
                repaint();
            }
            catch (Exception e)
            {
                Messages.logError(e.toString());
            }
        }

    /** Get state option. */
    protected int getState(String s)
        {
            for(int i=0;i<states.length;i++)
                if(s.equals(states[i]))
                    return i+1;
            return UNSET;
        }

    /** Get state display string. */
    protected String getStateString(int s)
        {
            switch (s) {
            default:
            case OFF:
                return Util.getLabel("sOFF","Off");
            case REGULATED:
                return Util.getLabel("sREGULATED","Regulated");
            case NOTREG:
                return Util.getLabel("sNOTREG","Not Regulated");
            case NOTPRESENT:
                return Util.getLabel("sNOTPRESENT","Not present");
            }
        }

    /** set state option. */
    protected void setState(int s) {
    	
    	String vstr=Fmt.fg(numdigits, status_value);

        setStatusFont(plain);
        setValueColor(null);
        setShow(show);
        switch (s) {
        default:
        case OFF:
            setStatusColor(stat_colors[OFFID]);
            setStatusFont(stat_fonts[OFFID]);
            setValueString(vstr);
            showset=false;
            break;
        case REGULATED:
            setStatusColor(stat_colors[ONID]);
            setStatusFont(stat_fonts[ONID]);
            setValueString(vstr);
            showset=true;
            break;
        case NOTREG:
            setStatusColor(stat_colors[INTERACTIVEID]);
            setStatusFont(stat_fonts[INTERACTIVEID]);
            setValueString(vstr);
            showset=true;
            break;
        case NOTPRESENT:
            setStatusColor(stat_colors[NPID]);
            setStatusFont(stat_fonts[NPID]);
            setValueString("    ");
            showset=false;
            break;
        }
        setToolTip();
    }

    /** set tooltip text. */
    protected void setToolTip() {
        String s=getStateString(state);
        if(showset && setval != null)
            s+=" ["+setval+"]";
        setToolTipText(s);
    }

    /** re-layout content. */
    public void validate(){
        Dimension win = getSize();
        Insets insets = getInsets();
        win.height -= jlabels.getHeight() + insets.top + insets.bottom;
        chart.width=win.width - insets.left - insets.right;
        chart.height=win.height;
        if(plotpanel !=null){       
    	    plotpanel.setSize(chart);
    	    plotpanel.setPreferredSize(chart);
        }
               
        super.validate();
    }

    /** set value. */
    protected void setSetValue(double d, long time){
        if(plotpanel ==null)
            return;
        set_value=d;

        set_cnt++;
        range_max=range_max<set_value?set_value:range_max;
        range_min=set_value<range_min?set_value:range_min;

//        if(data_cnt==2){
            plotpanel.setXAxis(true);
            if(range_max>range_min)
                plotpanel.setYAxis(true);
            plotpanel.unsetXRange();
            if(range_max>range_min)
                plotpanel.unsetYRange();
//        }

        if(data_cnt>0){
            last=time;
            plotpanel.addPoint(0, time, status_value, true);
            plotpanel.addPoint(1, time, status_value, false);            
            plotpanel.addPoint(2, time, set_value, true);
            if(writer!=null)
                logData(time);
        }
    }
    /** set value. */
    protected void setSetValue(double d){
        setSetValue(d,System.currentTimeMillis());
    }

    /** set value. */
    protected void setStatusValue(double d, long time){
        if(plotpanel ==null)
            return;
        status_value=d;
        data_cnt++;
        if(show_target && set_cnt==0){
            return;
        }

        if(data_cnt==0)
            start=time;
        last=time;
        range_max=range_max<status_value?status_value:range_max;
        range_min=status_value<range_min?status_value:range_min;

        if(data_cnt==2){
            plotpanel.setXAxis(true);
            if(range_max>range_min)
                plotpanel.setYAxis(true);
            plotpanel.unsetXRange();
            if(range_max>range_min)
                plotpanel.unsetYRange();
        }
        plotpanel.addPoint(0, time, status_value, true);
        plotpanel.addPoint(1, time, status_value, false);
        if(show_target)
            plotpanel.addPoint(2, time, set_value, true);
        if(writer!=null)
            logData(time);
    }
    /** set value. */
    protected void setStatusValue(double d){
        setStatusValue(d,System.currentTimeMillis());
    }

    private void init_data(){
        range_max=-100000;
        range_min=100000;
        data_cnt=0;
        //set_cnt=0;
        if(plotpanel !=null)
            plotpanel.clear(false);
        start=last=System.currentTimeMillis();
    }

    /** show/hide widget. */
    protected void setShow(boolean flag) {
        setVisible(flag);
    }

    /** Set status color. */
    protected void setStatusColor(Color c) {
    	statcolor=c;
    }

    protected void setTitleString(String s){
        jtitle.setText(s);
    }

    protected void setTitleFont(Font f){
        jtitle.setFont(f);
    }

    protected void setTitleColor(Color c){
        jtitle.setForeground(c);
    }

    protected void setValueString(String s){
        jvalue.setText(s);
    }

    protected void setStatusFont(Font f){
    	statfont=new Font(f.getName(),f.getStyle(),16);
        jvalue.setFont(f);
    }

    protected void setValueColor(Color c){
        jvalue.setForeground(c);
    }

    protected void setChartBGColor(Color c){
        if(plotpanel !=null)
            plotpanel.setPlotBackground(c);
    }

    protected void setChartFGColor(Color c){
        if(plotpanel !=null)
            plotpanel.setForeground(c);
    }

    // VObjIF interface

    /** set an attribute. */
    public void setAttribute(int attr, String c){
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case TITLE:
            title = c;
            setTitleString(title);
            repaint();
            break;
        case NUMDIGIT:
            if (c != null)
            {
               precision = c;
               numdigits=Integer.valueOf(c);
            }
            break;
        case STATCOL:
            statcol = c;
            repaint();
            break;
        case STATKEY:
            if (c != null)
            {
                for (int i = 0; i < status_key.length; i++) {
                    if (c.equals(status_key[i]) && i != status_index) {
                        statkey = c;
                        setval = null;
                        valstr = "0";
                        statpar = status_val[i];
                        statset = status_set[i];
                        if(statset.length()>0)
                            show_target=true;
                        status_index = i;
                        createPlotPanel();
                        init_data();
                    }
                }
                if (isEditing && status_index >= 0) {
                    updateStatus(ExpPanel.getStatusValue(statkey));
                    updateStatus(ExpPanel.getStatusValue(statpar));
                    if(show_target)
                        updateStatus(ExpPanel.getStatusValue(statset));
                    repaint();
                }
            }
            break;
        case ENABLED:
            isEnabled=c;
            if(c.equals("yes"))
                setEnabled(true);
            else
                setEnabled(false);
            repaint();
            break;
        case VARIABLE:
            vnmrVar = c;
            if ( (c != null) && (statpar == null) )
            {
               statkey = c;
               setval = null;
               valstr = "0";
               statpar = c;
               statset = "";
               show_target=false;
               createPlotPanel();
               init_data();
            }
            break;
        case SETVAL:
            setValue = c;
            break;
        case INCR1:
            if (c != null)
               repeat_delay=Integer.parseInt(c);
            break;
        case VALUES:
            if (c != null)
               data_max=Integer.parseInt(c);
            //if(data_max>0){
	        //    plotpanel.setPointsPersistence(data_max);
	        //    repaint();
           // }
            break;
        }
    }

    /** get an attribute. */
    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case TITLE:
            return title;
        case NUMDIGIT:
            return precision;
        case STATKEY:
            return statkey;
        case STATCOL:
            return statcol;
        case FGCOLOR:
            return fg;
        case ENABLED:
            return isEnabled;
        case VALUES:
            return Integer.toString(data_max);
        case VARIABLE:
            return vnmrVar;
        case SETVAL:
            return setValue;
        case INCR1:
            return Integer.toString(repeat_delay);
        default:
            return null;
        }
    }

    public void setEditStatus(boolean s) {isEditing = s;}

    public void setEditMode(boolean s)   {
        inEditMode = s;
        if (s) {
            defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
        }
    }

    public void addDefChoice(String s) {}

    public void addDefValue(String s) {}

    public void setDefLabel(String s) {}

    public void setDefColor(String s) {fg=s;}

    public void setShowValue(ParamIF p) {}

    public void changeFocus(boolean s) {}

    public ButtonIF getVnmrIF() {return vnmrIf;}

    public void setVnmrIF(ButtonIF vif) {vnmrIf=vif;}

    public void refresh() { setState(state);repaint();}

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
        stopTimer();
    }

    public void changeFont() {
        Font font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setTitleFont(font);
    }

    // DropTargetListener interface
    public void setDefLoc(int x, int y) {
        defLoc.x = x;
        defLoc.y = y;
    }

    public Point getDefLoc() {
        tmpLoc.x = defLoc.x;
        tmpLoc.y = defLoc.y;
        return tmpLoc;
    }

    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void reshape(int x, int y, int w, int h) {
        if (inEditMode) {
            defLoc.x = x;
            defLoc.y = y;
            defDim.width = w;
            defDim.height = h;
        }
        curDim.width = w;
        curDim.height = h;
        curLoc.x = x;
        curLoc.y = y;
        super.reshape(x, y, w, h);
    }

    public Point getLocation() {
        if (inEditMode) {
            tmpLoc.x = defLoc.x;
            tmpLoc.y = defLoc.y;
        }
        else {
            tmpLoc.x = curLoc.x;
            tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }

    public void dragEnter(DropTargetDragEvent e) { }

    public void dragExit(DropTargetEvent e) {}

    public void dragOver(DropTargetDragEvent e) {}

    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop


    // VEditIF interface

    private final static String[] true_false =
    {Util.getLabel("mlFalse"), Util.getLabel("mlTrue")};

    public Object[][] getAttributes()  { return attributes; }

    private final static Object[][] attributes = {
        {new Integer(TITLE),     Util.getLabel("lcTITLE")},
        {new Integer(STATCOL),   Util.getLabel(STATCOL),"menu",status_color},
        {new Integer(STATKEY),   Util.getLabel(STATKEY)},
        {new Integer(VALUES),    Util.getLabel("lcVALUES")},
        {new Integer(SHOWMAX),   Util.getLabel("lcSHOWMAX"), "menu", true_false},
    };
   

    private void createPlotPanel(){
        menu=new SMenu();

    	//set up History plots
        plotpanel= new SPlot();
        plotpanel.setBackground(null);
        plotpanel.setGrid(true);
        plotpanel.setGridColor(Color.WHITE);
        plotpanel.setPlotBackground(Color.LIGHT_GRAY);
        plotpanel.setXAxis(true);
        plotpanel.setYAxis(true);
		plotpanel.setFillButton(true);
		plotpanel.setTailButton(true);
		plotpanel.setFormatButton(true);
		plotpanel.setPrintButton(true);
		
		getColors();
		setColors();

   		plotpanel.setMarksStyle("none",0);
   		plotpanel.setMarksStyle("dots",1);
        if(show_target)
            plotpanel.setMarksStyle("none",2);

   		plotpanel.setConnected(true,0);
   		plotpanel.setConnected(false,1);
        if(show_target)
            plotpanel.setConnected(true,2);
		plotpanel.setXTimeOfDay(true);
		//plotpanel.setXLabel("Time");
		
		plotpanel.setOpaque(true);
		plotpanel.setBorder(BorderFactory.createEtchedBorder());
		
		plotpanel.setYRange(0, 100);
		plotpanel.addLegendButton(0, 0, "","Lines");
		plotpanel.addLegendButton(1, 1, "","Points");
        if(show_target)
            plotpanel.addLegendButton(2, 2, "","Set Value");

		plotpanel.setXAxis(false);
		plotpanel.setYAxis(false);

		add(plotpanel, BorderLayout.NORTH);
        plotpanel.addMouseListener(ml);
        if(data_max>0)
            plotpanel.setPointsPersistence(data_max);
		plotpanel.setVisible(true);  		     
    }
  
    private String getSysLogPath(){
        return FileUtil.sysdir()+FileUtil.separator+"status"+FileUtil.separator+"logs"+FileUtil.separator+
                statkey+".xml";
    }
    private boolean haveSysLog(){
        String path=getSysLogPath();
        File fd = new File(path);
        if (fd.exists() && fd.canRead()) 
            return true;
        return false;
    }
    private void loadSysLog(){
        String path=getSysLogPath();
        File fd = new File(path);
        if (fd.exists() && fd.canRead()) {
            try {
                SAXParserFactory spf = SAXParserFactory.newInstance();
                spf.setValidating(false); // set to true if we get DOCTYPE
                spf.setNamespaceAware(false); // set to true with referencing
                SAXParser parser = spf.newSAXParser();
                init_data();
                parser.parse(fd, new StatLogParser());
            } catch (Exception e) {
                System.out.println("Error parsing status log file");
            }
        }

    }
    private boolean setLogFile(){
		JFrame saveLog= new JFrame();
		saveLog.setTitle("Set Log file location");
	
		JFileChooser fc= new JFileChooser();
		
		fc.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
		if(logfile!=null)
			fc.setSelectedFile(new File(logfile));
		int returnVal= fc.showDialog(saveLog, "Save File Path");
		
		if(returnVal!=JFileChooser.APPROVE_OPTION)
			return false;
		File file= fc.getSelectedFile();
		logfile=file.getPath();
		return true;
    }
    private void startLog(){
    	try{
    	writer=new FileWriter(logfile, false);	//open file and clear data
		String line= new String("DATE    TIME   VALUE\n");
		writer.write(line);
    	}
    	catch (IOException e){    		
    	}
    }
    private void endLog(){
    	if(writer==null)
    		return;
    	try{
		writer.flush();
		writer.close();
		writer=null;
    	}
    	catch (IOException e){    		
    	}
    }
    
    private void logData(long tm){
    	String vstr=Fmt.fg(numdigits, status_value);
        if (_dateFormat == null)
            _dateFormat = new SimpleDateFormat();
    	String time_str="MM/dd HH:mm:ss";
        _dateFormat.applyPattern(time_str);

    	String dstr =_dateFormat.format(new Date(tm));
    	long elapsed_time=tm-start; // seconds
 
        String run_str="HHH:mm:ss";
            
        _dateFormat.applyPattern(run_str);
        Date date=new Date(elapsed_time-cal.get(Calendar.ZONE_OFFSET));
        String tstr = _dateFormat.format(date);
        String sstr=Fmt.fg(numdigits, set_value);        
        String str = dstr+"  "+tstr+"  "+vstr+" "+sstr;
   		//System.out.println(str);  	

    	try{
    		writer.write(str+"\n");
    		writer.flush();
        	}
        	catch (IOException e){    		
        	}
    }
    
    private synchronized void startTimer(){
        if(timer==null){
            timer = new Timer();
            tester=new testTask();

            timer.scheduleAtFixedRate(tester, repeat_delay,repeat_delay);
        }
    }
    private synchronized void stopTimer(){        
        if(timer!=null){
             timer.cancel();
             timer.purge();
             tester=null;
        }
        timer=null;
    }
    
    // internal classes
    
    public  class SPlot extends Plot {
    	public double xrange=10*1000;
        /* (non-Javadoc)
         * Draw plot with optional text overlays
         * @see vnmr.bo.PlotBox#paintComponent(java.awt.Graphics)
         */
        public void paintComponent(Graphics g) {
        	super.paintComponent(g);
            Insets insets=new Insets(20,40,40,40);
            int width = getWidth();
            int height = getHeight();
            FontMetrics metrics = g.getFontMetrics(graph_font);
            if(show_max && data_cnt>0){
                g.setColor(graph_colors[LABELS]);
                g.setFont(graph_font);
                String vstr=Fmt.fg(numdigits, range_max);
                g.drawString("YMAX:"+vstr, insets.left, insets.top + metrics.getAscent());
                vstr=Fmt.fg(numdigits, range_min);
                g.drawString("YMIN:"+vstr, insets.left,height - insets.bottom);
            }
            if(show_status){
                g.setColor(statcolor);
                g.setFont(statfont);
                metrics = g.getFontMetrics(statfont);
                String s=getStateString(state);
            	g.drawString(s,width/2+insets.left-metrics.stringWidth(s), insets.top + metrics.getAscent());
            }
            if(logging){
            	g.setColor(Color.RED);
            	g.fillOval(width-insets.right-10, insets.top, 10, 10);
            }

        }
       
        /* (non-Javadoc)
         * modified base class function so that printing area is scaled to fill the page 
         * (standard or landscape)
         * @see vnmr.bo.PlotBox#print(java.awt.Graphics, java.awt.print.PageFormat, int)
         */
        public synchronized int print(Graphics graphics, PageFormat format,
                int index) throws PrinterException {
            if (graphics == null) return Printable.NO_SUCH_PAGE;
            // We only print on one page.
            if (index >= 1) {
                return Printable.NO_SUCH_PAGE;
            }
            graphics.translate((int)format.getImageableX(),
                    (int)format.getImageableY());
            Rectangle rect=new Rectangle(0,0,(int)format.getImageableWidth()
            		 ,(int)format.getImageableHeight());
            _drawPlot(graphics, rect); // modified function that passes in page dimensions
            return Printable.PAGE_EXISTS;
        }

        protected String _formatTime(double dtime, double step_ms) {
            if (_dateFormat == null)
                _dateFormat = new SimpleDateFormat();
            long ltm=(long)dtime;
        	String time_str="";
        	String str ="";
        	if(time_of_day){
	            if (step_ms < 10 * 60 *1000) { // Step < 10 minutes
	            	time_str="H:mm:ss";
	                //_dateFormat.applyPattern("H:mm:ss");
	            }
	            else { // Step < 1 hour
	            	time_str="MM/dd H:mm";
	                // _dateFormat.applyPattern("MM/dd H:mm");
	            }
	            _dateFormat.applyPattern(time_str);
	            str = _dateFormat.format(new Date((long)dtime));
        	}
        	else{
            	long elapsed_time=ltm-start; // seconds
	            if (elapsed_time < 24 * 60 * 60 * 1000) { // Step < 10 minutes
	            	time_str="HH:mm:ss";
	            }
	            else { 
	            	time_str="dd HH:mm:ss";
	            }
	            _dateFormat.applyPattern(time_str);
	            Date date=new Date(elapsed_time-cal.get(Calendar.ZONE_OFFSET));	            str = _dateFormat.format(date);
       		
        	}
            return str;
      }

      public synchronized void setFormatButton(boolean visible){
    	  super.setFormatButton(visible);
    	  if(_formatButton!=null)
    		  _formatButton.setToolTipText("Show Elapsed Time");
      }
      protected void fillButtonClick() {
          setWalking(false);
    	  super.fillButtonClick();
      }
      
      public boolean zoomin(){
    	  boolean zi=super.zoomin();
    	  if(zi){
        	  double[] x=getFullXRange();
        	  double dx=x[1]-x[0];
        	  xrange=dx;    		  
    	  }
      	  return zi;
      }

      protected void formatButtonClick() {
    	  time_of_day=time_of_day?false:true;
    	  if(time_of_day)
    		  _formatButton.setToolTipText("Show Elapsed Time");
    	  else
    		  _formatButton.setToolTipText("Show Clock Time");
      }

      protected void tailButtonClick() {
          setWalking(true);
          double[] x=getFullXRange();
          double dx = x[1]-x[0];
          double xMax = getMaxX();
          if(xrange<dx )
        	  dx=xrange;
          setXRange(xMax - dx, xMax);
    	  if(autoscale)
    		  unsetYRange();
          repaint();
      }
    }

    class testTask extends java.util.TimerTask {
        public void run() {
            if(testmode){
            	testval+=5.0;
            	String msg=statpar+" ";
            	double d=20.0*Math.sin(testval*Math.PI/180.0);
            	
            	updateStatus(msg+d);
            }
            else if(timemode && valstr.length()>0){
                updateStatus(statpar+" " +valstr);
                
            }
        }
    }
	class SMenu extends JPopupMenu {
	    ActionListener listener;
		JMenuItem AutoMenuItem;
		JMenuItem ClearMenuItem;
		JMenuItem MinMaxMenuItem;
		JMenuItem StatusMenuItem=null;
		JMenuItem LogMenuItem=null;
		JMenuItem EventMenuItem=null;
		JMenuItem SysLogMenuItem=null;
        JMenuItem TestMenuItem=null;

		public SMenu() {
			String string;
			// string = Util.getLabel("mlShowMarkers","Show/Hide Markers");
			AutoMenuItem = add(Util.getLabel("vscLockScale","Lock Scale"));
			listener = new ActionListener() {
				public void actionPerformed(ActionEvent evt) {
					autoscale = autoscale ? false : true;
					if (autoscale) {
					    AutoMenuItem.setText(Util.getLabel("vscLockScale","Lock Scale"));
				        if(plotpanel !=null)
				            plotpanel.unsetYRange();
					} else if(plotpanel !=null){
						double[] range = plotpanel.getFullYRange();
						if (range != null)
							plotpanel.setYRange(range[0], range[1]);
						AutoMenuItem.setText(Util.getLabel("vscAutoScale","Autoscale"));
					}
					refresh();
				}
			};
			AutoMenuItem.addActionListener(listener);
			
			// 1 Clear data 
			string = Util.getLabel("mlClearHistory", "Clear history");
			ClearMenuItem = add(string);
			listener = new ActionListener() {
				public void actionPerformed(ActionEvent evt) {
					init_data();
					refresh();
				}
			};
			ClearMenuItem.addActionListener(listener);

			// 2 Show/Hide markers

			string = Util.getLabel("vscShowMinMax","Show Min/Max");
			MinMaxMenuItem=add(string);
            listener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    show_max=show_max?false:true;
					if (!show_max) {
					    MinMaxMenuItem.setText(Util.getLabel("vscShowMinMax","Show Min/Max"));
					} else {
					    MinMaxMenuItem.setText(Util.getLabel("vscHideMinMax","Hide Min/Max"));
					}
                    refresh();
                }
            };
            MinMaxMenuItem.addActionListener(listener);
            
			// 3 Show/Hide stats

            string = Util.getLabel("vscShowStatus","Show Status");
            StatusMenuItem=add(string);
            listener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    show_status=show_status?false:true;
					if (!show_status) {
					    StatusMenuItem.setText(Util.getLabel("vscShowStatus","Show Status"));
					} else {
					    StatusMenuItem.setText(Util.getLabel("vscHideStatus","Hide Status"));
					}
                    refresh();
                }
            };
            StatusMenuItem.addActionListener(listener);

			// 4 log data
			string = Util.getLabel("vscStartLog","Start Logging");
			LogMenuItem = add(string);

			listener = new ActionListener() {
				public void actionPerformed(ActionEvent evt) {
					if (logging) {
					    LogMenuItem.setText(Util.getLabel("vscStartLog","Start Logging"));
						endLog();
						logging=false;
						
					} else {
						if(setLogFile()){
						    LogMenuItem.setText(Util.getLabel("vscStopLog","Stop Logging"));
							startLog();
							logging=true;
						}
					}
					// pop-up save dialog
					refresh();
				}
			};
			LogMenuItem.addActionListener(listener);

			// 5 log type
            string = Util.getLabel("vscLogTime","Log Time");
            EventMenuItem = add(string);

            listener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    if (!timemode) {
                        startTimer();
                        EventMenuItem.setText(Util.getLabel("vscLogEvents","Log Events"));
                        timemode = true;
                    } else {
                        EventMenuItem.setText(Util.getLabel("vscLogTime","Log Time"));
                        stopTimer();
                        timemode = false;
                    }
                    refresh();
                }
            };
            EventMenuItem.addActionListener(listener);

            string = Util.getLabel("vscLoadSystemLog","Load System Log");
            SysLogMenuItem = add(string);

            listener = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    loadSysLog();
                    refresh();
                }
            };
            if(haveSysLog())
                SysLogMenuItem.addActionListener(listener);
            else
                SysLogMenuItem.setEnabled(false);
            		
	        String option = System.getProperty("status_test");
	        if (option != null){
    			string = "Start Test";
    			TestMenuItem = add(string);
    
    			listener = new ActionListener() {
    				public void actionPerformed(ActionEvent evt) {
    					if (!testmode) {
    						startTimer();
    						TestMenuItem.setText("Stop Test");
    						init_data();
    						testmode = true;
    					} else {
    					    TestMenuItem.setText("Start Test");
    						stopTimer();
    						testmode = false;
    					}					
    					refresh();
    				}
    			};
    			TestMenuItem.addActionListener(listener);
	        }
		}
	} // class SMenu
    /**
     * Sax parser for statlog.xml
     */
    public class StatLogParser extends DefaultHandler {
        public void error(SAXParseException spe) {
            System.out.println("Error at line " + spe.getLineNumber()
                    + ", column " + spe.getColumnNumber());
        }

        public void startElement(String uri, String localName, String qName,
                Attributes attr) {
            if (qName.equals(statpar)|| qName.equals(statset)) {
                String val = attr.getValue("value").trim();
                String time = attr.getValue("time").trim();
                long secs=Long.parseLong(time)*1000;
                StringTokenizer tok=new StringTokenizer(val);
                if(tok.hasMoreTokens())
                    val=tok.nextToken();
                double dval=Double.parseDouble(val);
                if (qName.equals(statpar)){
                    valstr = val;
                    setStatusValue(dval,secs);
                }
                else{
                    if(data_cnt>0)
                    setSetValue(set_value,secs);
                    setval = val;
                    setSetValue(dval,secs);
                }                    
            }
        }
    }
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
} // class VStatusChart


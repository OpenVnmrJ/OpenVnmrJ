/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

// TODO: - move special case (gtk+, motif) tests to VNMRFrame
//       - save LAF state as vnmrj,system or other(e.g. nimbus)
//       - Suppress entry borders only for system laf
package vnmr.util;

import javax.swing.*;
import javax.swing.border.*;
import javax.swing.plaf.metal.*;
import javax.swing.plaf.ColorUIResource;
import javax.swing.plaf.basic.*;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.Toolkit;
import java.text.Collator;
import java.util.*;
import java.beans.*;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.templates.*;

public class DisplayOptions extends ModelessDialog
    implements ActionListener, VObjDef, ExpListenerIF
{
    private static final long serialVersionUID = 1L;
    public final static String UI = "Interface";
    public final static String GRAPHICS = "Graphics";
    public final static String PLOT = "Plot";

    public final static String DEFAULT_THEME = "Default";

    public static String  interface_perfile="USER/PERSISTENCE/Interface"; // persistence file styles
    public static String  graphics_perfile="USER/PERSISTENCE/Graphics"; // persistence file styles
    public static String  plot_perfile="USER/PERSISTENCE/Plot"; // persistence file styles
    public static String  plot="USER/PERSISTENCE/Plot"; // persistence file styles
    public static String  prefsfile="USER/PERSISTENCE/DisplayPrefs"; // persistence file prefs
    public static String  colorsfile="DEFAULT";// colors file
    public static String  plotterfile="";// plotter colors file
    public static String  colorsdir="USER/TEMPLATES/color/";// colors directory
    public static String  USRTHEMES="USER/TEMPLATES/themes";        // user themes
    public static String  SYSTHEMES="SYSTEM/TEMPLATES/themes";      // system themes
    private final static Font bold = new Font("Dialog", Font.BOLD, 14);
    private final static Font italics = new Font("Dialog", Font.ITALIC, 14);
    private final static Font plain = new Font("Dialog", Font.PLAIN, 14);

    public final static int SYSTEM=0;
    public final static int SELSYS=1;
    public final static int VNMR=2;
    public final static int ALL=3;  
    public final static int PLOTTER=4;

    public final static int USER=4;
    public final static int DFLT=5;
    
    public final static int VNMR_MAX_COLOR = 255;

    public final static String FONT   = "Font";
    public final static String STYLE  = "Style";
    public final static String SIZE   = "Size";
    public final static String COLOR  = "Color";
    public final static String THICKNESS  = "Thickness";
    public final static String VJBGCOLOR  = "VJBackgroundColor";

    // define hard coded vnmrbg colors:
    public final static int BLACK = 0;
    public final static int RED = 1;
    public final static int YELLOW = 2;
    public final static int GREEN = 3;
    public final static int CYAN = 4;
    public final static int BLUE = 5;
    public final static int MAGENTA = 6;
    public final static int WHITE = 7;
    public final static int ORANGE = 56;
    public final static int PINK = 63;
    public final static int GRAY = 64;
    
    public static String  LAF=null;
    public static String  oldLAF=null;
    public static String  newLAF=null;

    private ButtonIF          vnmrIF;
    private JPanel            panel=null;
    private JPanel            optionsPan=null;
    private JPanel            optionsContentPan=null;
    private JPanel            folderPan=null;
    private VTabbedPane       m_interfaceTabs=null;
    private VTabbedPane       m_graphicsTabs=null;
    private VTabbedPane       m_plotTabs=null;
    private VLabel            plotterInfo = null;
    private static String     m_tabsType=GRAPHICS;

    private static HashArrayList      vars = new HashArrayList();
    private static HashArrayList      xvars = new HashArrayList();
   
    private static ArrayList<String>  types=new ArrayList<String>();
    private static ArrayList  symbols=null;
    private static HashMap  hmFonts = new HashMap();
    private static HashMap  hmColors = new HashMap();
    private static HashMap  uiColors = new HashMap();
    private static Map<String,DisplayStyle> m_styleMap
            = new HashMap<String,DisplayStyle>();

    private static PropertyChangeSupport  typesmgr=new PropertyChangeSupport(new PropertySupportObject());
    private static String[] libs = new String[4];
    private static boolean built=false;
    private static boolean m_outputHex=true;
    private static boolean m_enableAA=true;

    private static String m_interface_item="Default";
    private static int m_interface_type=SYSTEM;
    private static String m_interface_tab="Labels";
    
    private static String m_graphics_item="Default";
    private static int m_graphics_type=SYSTEM;
    private static String m_graphics_tab="Display";
    
    private static String m_plot_item="Default";
    private static String m_plot_tab="Display";
    private static int m_plot_type=SYSTEM;

    private ArrayList<Theme> m_themes=new ArrayList<Theme>();
    private JCheckBox showHexcheck;
    private JCheckBox enableAAcheck;
    
    private JRadioButton showUIBtn;
    private JRadioButton showGraphicsBtn;
    private JRadioButton showPlotBtn;
    private JButton saveThemeBtn;
    private JButton deleteThemeBtn;
    protected static JComboBox theme_menu=null;
    protected static JComboBox laf_menu=null;
    protected JLabel laf_label=null;

    private static boolean m_repaint=false;
    private static boolean m_changemode=false;
    private static boolean m_newlaf=false;
    private static boolean bAdminUser = false;

    private static boolean m_vars_initialized=false;
    public static boolean m_system_theme=false;
    public static boolean bCustomUiColor = false;
    public static boolean bVnmrjUiColor = true;
    
    private javax.swing.Timer timer=new javax.swing.Timer(500,this);
    private static Color bgc;

    private static OceanTheme oceanTheme = new OceanTheme();
    
    private static UIDefaults defaults=UIManager.getDefaults();
    //private static UIDefaults defaults=UIManager.getLookAndFeelDefaults();
    ShutdownHook shutdownHook = new ShutdownHook();
    private static Runnable uiUpdater;

    public DisplayOptions(SessionShare ss, ButtonIF ex) {
        super(Util.getLabel("dlTital","Styles and Themes"));
        Runtime.getRuntime().addShutdownHook(shutdownHook);
        Font font=getFont("Dialog","plain","12");
        vnmrIF=ex;

        folderPan=new JPanel();
        folderPan.setLayout(new TemplateLayout());
        optionsPan=new JPanel();
        optionsPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, true,false));
        optionsPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));

        // construct color value option section

        optionsContentPan = new JPanel();
        
        showHexcheck = new JCheckBox(Util.getLabel("dlHex"));
        showHexcheck.addActionListener(this);
        showHexcheck.setActionCommand("ShowHex");
        showHexcheck.setFocusPainted(false);
        showHexcheck.setOpaque(false);
        showHexcheck.setFont(font);
        //showHexcheck.setPreferredSize(new Dimension(60, 25));
        showHexcheck.setSelected(m_outputHex);
        
        enableAAcheck = new JCheckBox("AA");
        enableAAcheck.addActionListener(this);
        enableAAcheck.setActionCommand("EnableAA");
        enableAAcheck.setFocusPainted(false);
        enableAAcheck.setOpaque(false);
        enableAAcheck.setFont(font);
        enableAAcheck.setSelected(m_enableAA);
        
        ButtonGroup typeGroup = new ButtonGroup();
        showUIBtn = new JRadioButton(Util.getLabel("UI"));
        showUIBtn.setFocusPainted(false);
        showUIBtn.setFont(font);
        showUIBtn.setOpaque(false);
        showUIBtn.setSelected(false);
        showUIBtn.addActionListener(this);
        showUIBtn.setActionCommand("ShowUI");
        typeGroup.add(showUIBtn);
        
        showGraphicsBtn = new JRadioButton(Util.getLabel("Display"));
        showGraphicsBtn.setFocusPainted(false);
        showGraphicsBtn.setFont(font);
        showGraphicsBtn.setOpaque(false);
        showGraphicsBtn.setSelected(false);
        showGraphicsBtn.addActionListener(this);
        showGraphicsBtn.setActionCommand("ShowDisp");
        typeGroup.add(showGraphicsBtn);        
        
        showPlotBtn = new JRadioButton(Util.getLabel("Plot"));
        showPlotBtn.setFocusPainted(false);        
        showPlotBtn.setFont(font);
        showPlotBtn.setOpaque(false);
        showPlotBtn.setSelected(false);
        showPlotBtn.addActionListener(this);
        showPlotBtn.setActionCommand("ShowPlot");
        typeGroup.add(showPlotBtn);
  
        optionsContentPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT,5, 0, true, false));

        optionsContentPan.add(showUIBtn);
        optionsContentPan.add(showGraphicsBtn);
        optionsContentPan.add(showPlotBtn);

        // construct theme section

        theme_menu=new JComboBox();
        theme_menu.setActionCommand("theme_menu");
        theme_menu.addActionListener(this);
        theme_menu.setEditable(true);
        theme_menu.setOpaque(false);
        theme_menu.setRenderer(new ThemesListRenderer());
        theme_menu.setPreferredSize(new Dimension(100, 25));
        optionsContentPan.add(theme_menu);

        saveThemeBtn=new JButton(Util.getLabel("blSave"));
        saveThemeBtn.setFont(font);
        saveThemeBtn.addActionListener(this);
        saveThemeBtn.setActionCommand("SaveTheme");
        optionsContentPan.add(saveThemeBtn);
        deleteThemeBtn=new JButton(Util.getLabel("blDelete"));
        deleteThemeBtn.setActionCommand("DeleteTheme");
        deleteThemeBtn.addActionListener(this);
        deleteThemeBtn.setFont(font);
        optionsContentPan.add(deleteThemeBtn);
        optionsContentPan.add(showHexcheck);
        optionsContentPan.add(enableAAcheck);

        optionsPan.add(optionsContentPan);

        laf_label = new JLabel(Util.getLabel("LAF"));
        laf_label.setFont(font);

        optionsContentPan.add(laf_label);
        laf_menu=new JComboBox();
        laf_menu.setActionCommand("laf_menu");
        laf_menu.addActionListener(this);
        laf_menu.setEditable(false);
        laf_menu.setRenderer(new LafListRenderer());
        laf_menu.setEnabled(true);
        optionsContentPan.add(laf_menu);

        panel=new JPanel();
        panel.add(optionsPan);
        panel.add(folderPan);
        panel.setLayout(new panelLayout());

        Container contentPane = getContentPane();

        contentPane.add(panel, BorderLayout.CENTER);

        setLocationRelativeTo(getOwner());
        setTitle(Util.getLabel("dlTital","Styles and Themes"));
//        setHelpEnabled(false);
        setUndoEnabled(false);

        setResizable(false);
        setActionListeners();

        getThemes();
        getLAFs();

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent we) {
                getSelectedTab();
                writePersistence();
                writeColors(colorsfile,ALL);
                getPlotterName();
                if(plotterfile.length()>0)
                	writeColors(plotterfile,PLOTTER);
                setVisible(false);
                dispose();
            }
        });
        
        uiUpdater = new Runnable() {
           public void run() {
                  updateUIColor();
           }
        };
        timer.setActionCommand("timer");
        timer.start();
    }
 
    public static void updateLookAndFeel() {
         bgc = null;
         if(LAF.equals(oldLAF)){
             setDebug("updateLookAndFeel called for old LAF:"+LAF);
         }
         else
         setDebug("updateLookAndFeel "+LAF);
         VLookAndFeelFactory.setDefaultTheme(LAF);
         VLookAndFeelFactory.setCustomTheme(LAF);
         VLookAndFeelFactory.setLookAndFeel(LAF);
         setUIColors();
         VLookAndFeelFactory.updateUI();
         SwingUtilities.invokeLater(uiUpdater);
         oldLAF=LAF;
    }

    /** set initial colors from persistence file or "Default" theme
     *  This function is called in VNMRFrame before any GUI is instantiated
     *  to set the colors for UI components on construction.
     */
    public static void initTheme(boolean bAdmin) {
		String laf = System.getProperty("lookandfeel");

                bAdminUser = bAdmin;
		if (laf == null || laf.length() < 1)
			laf = VLookAndFeelFactory.VNMRJ;
		readPrefs();
		if (LAF == null)
			LAF = laf;
		
               setDebug("initTheme");

		types = new ArrayList<String>();
		types.add(DisplayOptions.FONT);
		types.add(DisplayOptions.STYLE);
		types.add(DisplayOptions.SIZE);
		types.add(DisplayOptions.COLOR);
		types.add(DisplayOptions.THICKNESS);

		readPersistence();
                if (!bAdminUser)
		    writePersistence();
		writeColors(colorsfile, ALL);
		if (plotterfile.length() > 0)
			writeColors(plotterfile, PLOTTER);	
		VLookAndFeelFactory.setDefaultTheme(LAF);
		VLookAndFeelFactory.setLookAndFeel(LAF);
		
		//DisplayOptions disp=Util.getDisplayOptions();
		//disp.build();
		
                setUIColors();
       
        // don't updateUI now, it will cause Exception
		//  VLookAndFeelFactory.updateUI();
    }

    public static void initTheme() {
        initTheme(false);
    }

    public static boolean isAAEnabled(){
        return m_enableAA;
    }
    public static Color getVJColor(String symbol) {
        symbol=symbol.replace("Color", "");
        symbol=symbol+"Color";
        DVar var=(DVar)vars.get(symbol);
        if(var==null)
            return null;
        Color c=VnmrRgb.getColorByName(var.value);
        if(var.value.equals("black") || !c.equals(Color.black)) {
             return c;
        }
        return null;
    }

    public static ColorUIResource getUIColor(String symbol) {
         Color c = getVJColor(symbol);
         if (c == null)
              return null;
         return (new ColorUIResource(c));
    }

    public static void enableCustomUIColor(String str) {
         if (str == null)
            return;
         StringTokenizer tok = new StringTokenizer(str, " ,\n");
         if (!tok.hasMoreTokens())
            return;
         bCustomUiColor = false;
         bVnmrjUiColor = false;
         while (tok.hasMoreTokens()) {
             String data = tok.nextToken().trim();
             if (data.equalsIgnoreCase("on") || data.equalsIgnoreCase("yes")) {
                 bCustomUiColor = true;
                 bVnmrjUiColor = true;
             }
             else if (data.equalsIgnoreCase("vnmrj"))
                 bVnmrjUiColor = true;
         }
    }

    public static boolean isCustomUIColorEnabled() {
         return bCustomUiColor;
    }

    public static void setDefaultUIColors() {
        UIDefaults defs = UIManager.getDefaults();
        Color c = defs.getColor("Panel.background");
        if (c != null)
            Util.setBgColor(c);
        c = defs.getColor("List.background");
        if (c != null)
            Util.setListBgColor(c);
        c = defs.getColor("List.selectionBackground");
        if (c != null)
            Util.setListSelectBg(c);
        c = defs.getColor("List.selectionForeground");
        if (c != null)
            Util.setListSelectFg(c);
       	c = defs.getColor("MenuBar.background");
        if (c != null) {
            Util.setMenuBarBg(c);
            Util.setToolBarBg(c);
        }
       	c = defs.getColor("ToolBar.background");
        if (c != null)
            Util.setToolBarBg(c);
        c = defs.getColor("Separator.background");
        if (c != null)
            Util.setSeparatorBg(c);
        c = defs.getColor("Table.gridColor");
        if (c != null)
            Util.setGridColor(c);
        c = defs.getColor("TextField.background");
        if (c != null)
            Util.setInputBg(c);
        c = defs.getColor("TextField.foreground");
        if (c != null)
            Util.setInputFg(c);
    }

    /** Set UI colors using VnmrJTheme class.
     *  This function is called whenever a "VJ.." color variable is modified.
     *  Causes rebuild of entire user interface.
     */

    public static void setUIColors() {
        boolean vnmrj_theme = LAF.equals(VLookAndFeelFactory.VNMRJ);

        setDefaultUIColors();
        Color c=getVJColor("VJGridColor");
        if(c!=null)
            Util.setGridColor(c);

        if (vnmrj_theme) {
            if (!bVnmrjUiColor) {
                MetalLookAndFeel.setCurrentTheme(oceanTheme);
                return;
            }
        }
        else if (!bCustomUiColor) {
            return;
        }

        
        // LookAndFeel laf=UIManager.getLookAndFeel();
        m_system_theme = !vnmrj_theme;
        UIDefaults defs=m_system_theme?defaults:UIManager.getDefaults();
        setDebug("setUIColors");
        Color newbgc;

        newbgc=getVJColor("VJBackground");
        if (newbgc == null)
            newbgc=defs.getColor("Panel.background");

        // For some reason the background color of some components isn't set correctly ..
        // unless it is changed slightly. 
        // note: JComponent.setBackground doesn't call repaint unless bg color is "different"
        if(newbgc!=null){
            // newbgc=newbgc.darker().brighter();
            Util.setBgColor(newbgc);
            Util.setListBgColor(newbgc);
        }
        c = getVJColor("VJSelectedBtnColor");
        if (c == null)
            c = defs.getColor("List.selectionBackground");
        Util.setListSelectBg(c);
        // Util.setActiveBg(c);

        c = getVJColor("VJSelectedTextColor");
        if (c == null)
            c = defs.getColor("List.selectionForeground");
        Util.setListSelectFg(c);

        // Util.setActiveFg(c);

        c = getVJColor("VJTextFGColor");
        if (c != null)
            Util.setInputFg(c);
        
        // c = getVJColor("VJInactiveBGColor");
        // if (c != null)
        //     Util.setInactiveBg(c);

       	c=defs.getColor("MenuBar.background");
        if (vnmrj_theme) {
             // if (c instanceof ColorUIResource)
             //    c= getUIColor("VJMenubarBG");
             // else
                c=getVJColor("VJMenubarBG");
        }
        if(c!=null){
            Util.setMenuBarBg(c);
            Util.setMenuBg(c.brighter());
        }

       	c = newbgc;
        if (vnmrj_theme) {
             if ((c != null) && (newbgc instanceof ColorUIResource))
                 c = getUIColor("VJToolbarBG");
             else
                 c = getVJColor("VJToolbarBG");
        }
        if(c!=null)
            Util.setToolBarBg(c.darker().brighter());

        c = getVJColor("VJSeparator");
        if (c == null)
            c=defs.getColor("Separator.background");
        if(c!=null)
            Util.setSeparatorBg(c);

        c=getVJColor("VJGridColor");
        if(c!=null){
            VnmrJTheme.setGridColor(c);
        }

        c=getVJColor("VJHighlightColor");
        if(c!=null)
            Util.setControlHighlight(c);

        c= getVJColor("VJEntryBGColor");
        if(c!=null)
            Util.setInputBg(c);
        
        // Note: "Themes" are only supported in Metal LAF
        
        if(m_system_theme)
        	return;
        
        VnmrJTheme.setDefaultBG(Util.getBgColor());
        VnmrJTheme.setMenuBarBGColor(Util.getMenuBarBg());
        VnmrJTheme.setMenuBGColor(Util.getMenuBg());
        VnmrJTheme.setSeparator(Util.getSeparatorBg());
        c=getVJColor("VJShadowColor");
        if(c!=null){
            VnmrJTheme.setShadowColor(c);
            c=c.darker();
            VnmrJTheme.setDarkShadowColor(c);
        }
        VnmrJTheme.setGridColor(Util.getGridColor());
        c=getVJColor("VJCtrlHighlightColor");
        if(c!=null){
            VnmrJTheme.setControlLightHighlight(c);
        }

        c=getVJColor("VJTabPaneBGColor");
        if(c!=null){
            VnmrJTheme.setTabPaneBG(c);
            c=Util.darken(c,0.2);
            VnmrJTheme.setTabPaneTabs(c);
        }

        // VnmrJTheme.setSelectedTextColor(getVJColor("VJSelectedTextColor"));
        VnmrJTheme.setSelectedTextColor(getVJColor("VJSelectedBtnColor"));
        VnmrJTheme.setSelectedBtnColor(getVJColor("VJSelectedBtnColor"));
        VnmrJTheme.setSelectBoxColor(getVJColor("VJSelectBoxColor"));
        VnmrJTheme.setFocusedColor(getVJColor("VJFocusedColor"));
        VnmrJTheme.setInactiveFG(getVJColor("VJInactiveFGColor"));
        VnmrJTheme.setInactiveBG(getVJColor("VJInactiveBGColor"));
        VnmrJTheme.setDisabledFG(getVJColor("VJDisabledFGColor"));
        VnmrJTheme.setDisabledBG(getVJColor("VJDisabledBGColor"));

        VnmrJTheme.setEntryBG(getVJColor("VJEntryBGColor"));

        VnmrJTheme.setGradient1(getVJColor("VJGradient1Color"));
        VnmrJTheme.setGradient2(getVJColor("VJGradient2Color"));

        // defaults=UIManager.getDefaults();
        // defaults.putDefaults(VnmrJTheme.getDefaults());
        MetalLookAndFeel.setCurrentTheme(new VnmrJTheme());
        VLookAndFeelFactory.setLookAndFeel(LAF);
    }

    /** show LAF attributes
    * @param match  show only symbols containing match String (if not null)
    */
    static public void showUIKeys(String match){
    	//UIDefaults defs=UIManager.getDefaults();
	     Enumeration e = defaults.keys();  
	     while( e.hasMoreElements() )  {  
	    	 String s=e.nextElement().toString();
	    	 if(match==null || match.isEmpty() || s.contains(match))
	    		 System.out.println( s );  
	    	 e.nextElement();
	     }  
   	
    }
    // ExpListenerIF interface

    public static void updatePnewValues(Vector<String> params) {
    	int n=params.size()/2;
        for(int k = 0; k < n; k++) {
            if(params.elementAt(k).equals("plotter")) {
            	try{
	            	String newplotter=(String)params.elementAt(k+n);
	            	if(!newplotter.equals(plotterfile)){
	            		plotterfile=newplotter;
	            		setDebug("new plotter file:"+plotterfile);
	            		//writeColors(plotterfile,PLOTTER);
	            	}
            	}
            	catch(Exception e){
            		System.out.println("DisplayOptions::Error parsing params vector:"+params);
            	}
            }           
        }  	
    }
    
    public void setViewPort(int id) {
        ButtonIF vif = Util.getViewArea().getExp(id);
        if (vif == null)
            return;
        if(m_interfaceTabs !=null)
            m_interfaceTabs.setVnmrIF(vif);
        if(m_graphicsTabs !=null)
            m_graphicsTabs.setVnmrIF(vif);
        if(m_plotTabs !=null)
            m_plotTabs.setVnmrIF(vif);
    }
    /** called from ExpPanel (pnew) */
    public void updateValue(Vector<String> params) {
        if(folderPan != null) {
            for(int i = 0; i < folderPan.getComponentCount(); i++) {
                Component comp = folderPan.getComponent(i);
                if(comp instanceof ExpListenerIF)
                    ((ExpListenerIF)comp).updateValue(params);
            }
        }
    }

    /** called from ExperimentIF for first time initialization */
    public void updateValue(VTabbedPane folder) {
        folder.updateValue();
    }

    /** called from ExperimentIF for first time initialization */
    public void updateValue() {
        if(folderPan != null) {
            for(int i = 0; i < getComponentCount(); i++) {
                Component comp = folderPan.getComponent(i);
                if(comp instanceof ExpListenerIF)
                    ((ExpListenerIF)comp).updateValue();
            }
        }
    }

    public static boolean sameColor(String c1, String c2){
        if(c1.equals(c2))
            return true;

        Color color1=VnmrRgb.getColorByName(c1);
        Color color2=VnmrRgb.getColorByName(c2);
        if(color1==null && color2!=null)
            return false;
        if(color1!=null && color2==null)
            return false;
        if(color1==null && color2==null)
            return true;
        if(color1.getRGB()==color2.getRGB())
            return true;
        return false;
    }

    private static void sendRepaint(){
        setDebug("repaint");
        Util.sendToAllVnmr("repaint('all')");
    }

    private static void sendSetColor(String cmd){
        setDebug(cmd);
        Util.sendToAllVnmr(cmd);
    }

    public static void sendNewBgColor(){
        DVar var=(DVar)vars.get(VJBGCOLOR);
        setDebug("new vj color "+var.value);
        typesmgr.firePropertyChange("DisplayOptions "+VJBGCOLOR,"All","Update");
    }

    public static void sendUpdateAll(){
        setDebug("update all");
        typesmgr.firePropertyChange("DisplayOptions  All","All","Update");
    }

    private static void setDebug(String s){
        if(DebugOutput.isSetFor("display"))
            Messages.postDebug("DisplayOptions : "+s);
    }

    /**
     * send setcolor() for all colors to all viewports
     */
    public static void initColors(){
        setDebug("Setting DPS colors");
        int nLength = vars.size();
        for(int i=0;i<nLength;i++){
            String key=(String)vars.getKey(i);
            DVar var=(DVar)vars.get(key);
            String symbol=var.symbol;
            if(var.type.equals("Color")){
                testDpsColor(symbol,var.value);
           }
        }
    }

    public static void initSpecColors(){
        setDebug("Setting Spec colors");
        int nLength = vars.size();
        for(int i=0;i<nLength;i++){

            String key=(String)vars.getKey(i);
            DVar var=(DVar)vars.get(key);
            String symbol=var.symbol;
            if(symbol.equals("graphics11Color"))
               Util.sendToVnmr("setSpecColor('init',11,'" + var.value + "')");
            if(symbol.equals("graphics42Color"))
               Util.sendToVnmr("setSpecColor('init',42,'" + var.value + "')");
            if(symbol.equals("graphics44Color"))
               Util.sendToVnmr("setSpecColor('init',44,'" + var.value + "')");
        }
        Util.sendToVnmr("setSpecColor");
    }

    private void loadTheme(){
        int i=theme_menu.getSelectedIndex();
        if(i<0)
        	return;
        Theme theme=m_themes.get(i);
        String path=null;
        m_repaint=false;
        if(m_tabsType.equals(UI)){
	        m_interface_item=theme.name;
	        m_interface_type=theme.type;
        }
        else if(m_tabsType.equals(PLOT)){
            m_plot_item=theme.name;
            m_plot_type=theme.type;                     
        }
        else{
            m_graphics_item=theme.name;
            m_graphics_type=theme.type;         
        }

        setDebug("loading theme: "+theme.name);
        if(theme.type==USER)
            path = FileUtil.openPath(USRTHEMES+"/"+m_tabsType+"/"+theme.name);
        else if(theme.type==SYSTEM){
            path=getSystemThemeDir();
            path = FileUtil.openPath(path+"/"+m_tabsType+"/"+theme.name);
        }
        if(path !=null){
            readStyles(path);
            if(m_repaint)
            	sendRepaint();
            if(m_tabsType.equals(UI)){
                if(m_newlaf)
                	laf_menu.setSelectedItem(newLAF);
                setLAFParam();
                //m_interfaceTabs.updateValue();
                updateLookAndFeel();
            }
            else{
                sendUpdateAll();
            }
        }
        // clear current selection so that reselection of same menu 
        // item causes a reload of current theme 
        m_changemode=true;
        theme_menu.setSelectedIndex(-1); 
        theme_menu.setSelectedItem(theme.name+"");
        m_changemode=false;

    }

    private void saveTheme(){
        String s=theme_menu.getSelectedItem().toString();
        String path = FileUtil.savePath(USRTHEMES+"/"+m_tabsType+"/"+s);
        writeStyles(path);
        if(m_tabsType.equals(UI)){
	        m_interface_item=s;
	        m_interface_type=USER;
        }
        else if(m_tabsType.equals(PLOT)){
	        m_plot_item=s;
	        m_plot_type=USER;        	
        }
        else{
            m_graphics_item=s;
            m_graphics_type=USER;            
        }
        getThemes();
    }

    private void deleteTheme(){
        String s=theme_menu.getSelectedItem().toString();
        String path = FileUtil.openPath(USRTHEMES+"/"+m_tabsType+"/"+s);
        File file=new File(path);
        if(file !=null){
            setDebug("delete theme "+s);
            file.delete();
            if(m_tabsType.equals(UI)){
	            m_interface_item="Default";
	            m_interface_type=DFLT;
            }
            else if(m_tabsType.equals(PLOT)){
            	m_plot_item="Default";
            	m_plot_type=DFLT;            	
            }
            else{
                m_graphics_item="Default";
                m_graphics_type=DFLT;                               
            }
            getThemes();
        }
    }

    private void getSelectedTab() {
        String tab=null;
        Component comp;
        if(m_tabsType.equals(UI))
        	comp= m_interfaceTabs.getSelectedComponent();
        else if(m_tabsType.equals(GRAPHICS))
        	comp= m_graphicsTabs.getSelectedComponent();
        else
            comp= m_plotTabs.getSelectedComponent();
        	
        if(comp instanceof VGroup) {
            String label = ((VGroup)comp).getAttribute(LABEL);
            if(label != null)
                tab=label;
        }
        if(tab==null || !built)
        	return;
        if(m_tabsType.equals(UI))
            m_interface_tab=tab;
        else if(m_tabsType.equals(PLOT))
            m_plot_tab=tab;
        else
            m_graphics_tab=tab;
    }

    public void showTab(String name) {
        int i = 0;
        VTabbedPane tabs;
        if(m_tabsType.equals(UI))
        	tabs=m_interfaceTabs;
        else if(m_tabsType.equals(PLOT))
            tabs=m_plotTabs;
        else
        	tabs=m_graphicsTabs;
        
        setDebug("showTab "+m_tabsType+" "+name);
       	
        if(name != null)
            i = tabs.indexOfTab(name);
        else
        	tabs.setSelectedIndex(-1);
        if(i >= 0)
        	tabs.setSelectedIndex(i);
    }

    public void setVisible(boolean t, String strhelpfile){
        if (strhelpfile != null) {
            m_strHelpFile = strhelpfile;
        }
        setVisible(t);
    }

    public void setVisible(boolean t){
        if(t && !built){
        	build();
            if(m_outputHex)
                showHexcheck.setSelected(true);
            else
            	showHexcheck.setSelected(false);
        }
        super.setVisible(t);
        if (t) {
            if (plotterInfo == null) {
                plotterInfo = new VLabel(null, vnmrIF, null);
                plotterInfo.setAttribute(SETVAL, "$VALUE=plotter");
            }
            vnmrIF = (ButtonIF) Util.getActiveView();
            plotterInfo.setVnmrIF(vnmrIF);
            plotterInfo.updateValue();
        }
    }

    public static void addChangeListener(PropertyChangeListener l){
        if (typesmgr != null)
            typesmgr.addPropertyChangeListener(l);
    }

    public static void removeChangeListener(PropertyChangeListener l){
        if (typesmgr != null)
            typesmgr.removePropertyChangeListener(l);
    }

    /** return true if property change event is for a global UI change" */
    public static boolean isUpdateUIEvent(PropertyChangeEvent evt){
        String strProperty = evt.getPropertyName();
        if (strProperty == null)
            return false;
        String attr =  strProperty.toLowerCase();
        if (attr.indexOf("vjbackground") >= 0)
            return true;
        if (attr.indexOf("all") >= 0)
            return true;
        return false;
   }

    public static boolean isUpdateAll(PropertyChangeEvent evt){
        String strProperty = evt.getPropertyName();
        if (strProperty == null)
            return false;
        String attr =  strProperty.toLowerCase();
        if (attr.indexOf("all") >= 0)
            return true;
        return false;
   }


    private void setThemeComp(Component comp, int type) {
        switch(type){
        default:
        case DFLT:
            comp.setFont(plain);
            break;
        case SYSTEM:
            comp.setFont(italics);
            break;
        case USER:
            comp.setFont(bold);
            break;
        }
    }

    /** get list of theme names */
    private Theme getTheme(String name, int type) {
        Theme theme;
        for(int i=0;i<m_themes.size();i++){
            theme=m_themes.get(i);
            if(theme.name.equals(name)&&theme.type==type)
                return theme;
        }
        return null;
    }
    private static String getSystemThemeDir(){
        ArrayList apptypes=FileUtil.getAppTypes();
        String systhemes=SYSTHEMES;
        if(apptypes.size()>0){
            String apptype = (String)apptypes.get(0);
            String dir;
            if(apptype.equals(Global.WALKUP)) {
                // For walkup type, the directory is now /vnmr
                dir = "";
            }
            else {
                // for imaging and lc, it is the same as the apptype
                dir = apptype;
            }
            String path="SYSTEM/"+dir+"/templates/themes";
            path=FileUtil.openPath(path);
            if(path !=null)
                systhemes=path;
        }
        return FileUtil.openPath(systhemes);
    }

    private void getLAFs(){
        String[] lists = VLookAndFeelFactory.getLAFs();
        if (lists == null || lists.length < 1) {
            return;
        }
        m_changemode = true;
        laf_menu.removeAllItems();
        int index = 0;
        for (int i = 0; i < lists.length; i++) {
            if (lists[i] != null) {
                if (LAF.equals(lists[i]))
                    index = i;
                laf_menu.addItem(lists[i]);
            }
        }

        laf_menu.setSelectedIndex(index);
        m_changemode=false;
    }

    /**
     * return symbol for LAF name
     * @param s
     * @return
     */
    public static String getTokenForLAF(String s){
    	String laf=s.toLowerCase();
    	
    	if(laf.contains("motif")) // motif/cde 
    		laf="motif";	    
    	if(laf.contains("metal")) // varian default
    		laf= VLookAndFeelFactory.VNMRJ;
    	if(laf.contains("gtk+")) // system laf on Linux 
    		laf= VLookAndFeelFactory.SYSTEM;
     	
    	return laf;
    }
    /**
     * get LAF name from symbol
     * @param s
     * @return
     */
    public static String getLAFForToken(String s){
    	String laf=s.toLowerCase();
    	
    	if(laf.equals("motif")) // this one doesn't play nice with colors 
    		laf="motif";	    
    	if(laf.equals(VLookAndFeelFactory.VNMRJ)) // varian default
    		laf="metal";
    	if(laf.equals(VLookAndFeelFactory.SYSTEM)) // system laf on Linux 
    		laf="gtk+";
    	return laf;
    }
    public static void setLAFParam(){
        String str = "exists('LAF','parameter','global'):$e if $e<0.5 then create('LAF','string','global') endif LAF='"+LAF+"'";
        //String str = "LAF='"+LAF+"'";
        DisplayOptions d=Util.getDisplayOptions();
        setDebug("setLAF:"+str);
        d.vnmrIF.sendToVnmr(str);
    }
    private static void setLAF(){
        setUIColors();
        updateLookAndFeel();
    }

    /** get list of theme names */
    private void getThemes() {
        m_changemode=true;
        String syspath = getSystemThemeDir();
        String usrpath = FileUtil.openPath(USRTHEMES);
        String[] flist;
        File dir;
        int indx=0;
        Theme theme;
        m_themes.clear();
        theme_menu.removeAllItems();
        
        syspath+="/";
        syspath+=m_tabsType;

        usrpath+="/";
        usrpath+=m_tabsType;

        if(syspath!=null){
            dir=new File(syspath);
            if(dir!=null){
                flist=dir.list(null);
                if(flist !=null){  // could be null if have permissions problem
                    for(int i=0;i<flist.length;i++){
                        String s=flist[i];
                        theme=new Theme(s,SYSTEM,indx++);
                        m_themes.add(theme);
                        theme_menu.addItem(theme);
                    }
                }
            }
        }
        if(getTheme("Default", SYSTEM)==null){
            theme=new Theme("Default",DFLT,indx++);
            m_themes.add(theme);
            theme_menu.addItem(theme);
        }
        if(usrpath!=null){    //
            dir=new File(usrpath);
            if(dir!=null){
                flist=dir.list(null);
                if(flist !=null){  // could be null if have permissions problem
                    for(int i=0;i<flist.length;i++){
                        String s=flist[i];
                        theme=new Theme(s,USER,indx++);
                        m_themes.add(theme);
                        theme_menu.addItem(theme);
                    }
                }
            }
        }
        if(m_tabsType.equals(UI))
        	theme=getTheme(m_interface_item,m_interface_type);
        else if(m_tabsType.equals(PLOT))
            theme=getTheme(m_plot_item,m_plot_type);
        else
        	theme=getTheme(m_graphics_item,m_graphics_type);
        if(theme==null)
            theme=m_themes.get(0);
        if(m_tabsType.equals(UI)){
	        m_interface_item=theme.name;
	        m_interface_type=theme.type;
        }
        else if(m_tabsType.equals(PLOT)){
	        m_plot_item=theme.name;
	        m_plot_type=theme.type;       	
        }
        else{
            m_graphics_item=theme.name;
            m_graphics_type=theme.type;         
        }
        if(m_themes.size()>1)
            theme_menu.setSelectedItem(theme);
        else{
            theme_menu.setSelectedIndex(-1);
            theme_menu.setSelectedItem(theme+"");   
        }
        setThemeComp(theme_menu,theme.type);
        setThemeButtons(theme.indx);

        m_changemode=false;
    }

    private void setThemeButtons(int indx){
        if(indx<0 || indx>=m_themes.size()){
            saveThemeBtn.setEnabled(true);
            deleteThemeBtn.setEnabled(false);
            return;
        }
        Theme theme=m_themes.get(indx);
        if(theme.type==DFLT||theme.type==SYSTEM){
            saveThemeBtn.setEnabled(true);
            deleteThemeBtn.setEnabled(false);
        }
        else{
            saveThemeBtn.setEnabled(true);
            deleteThemeBtn.setEnabled(true);
        }
    }

    /** return menu list of show catagories */
    public static String[] getShowTypes() {
        libs[SYSTEM] = Util.getLabel("dlSystem","System");
        libs[SELSYS] = Util.getLabel("dlSelected","Selected");
        libs[VNMR] = Util.getLabel("dlVnmr","Vnmr");
        libs[ALL] = Util.getLabel("dlAll","All");
        return libs;
    }

    public static boolean testVJColor(String symbol, String value) {
        if(!symbol.contains("Color") || !symbol.startsWith("VJ"))
            return false;
        DVar var=(DVar)vars.get(symbol);
        if(var==null)
            return true;
        Color newcol=VnmrRgb.getColorByName(value);
        Color oldcol=VnmrRgb.getColorByName(var.value);
        if(newcol.getRGB()==oldcol.getRGB())
            return false;
        return true;
     }

    public static boolean testDpsColor(String symbol, String value) {
        if(!symbol.contains("Color") || !symbol.contains("Dps"))
            return false;
        setDebug("set"+symbol+" "+value);
        // String sVal = "dpsvj('-vj', 'color', '"+symbol+"', '"+value+"')\n";

        // Util.sendToVnmr(sVal);

        return true;
    }

    public static boolean testSysColor(String cname, String value) {
        if(!cname.contains("Color"))
            return false;
        String keys = "graphics ps pcl hpgl pen newgraphics";
        String name = cname.replace("Color", "");
        String key = null;
        StringTokenizer tok = new StringTokenizer(keys, " ", true);
        String cmd = null;
        int id = -1;
        int red, grn, blu;

        while(tok.hasMoreTokens()) {
            String t = tok.nextToken();
            if(name.startsWith(t)) {
                key = t;
                break;
            }
        }
        if(key == null)
            return false;
        try {
            String sid = name.replace(key, "");
            id = Integer.decode(sid);
        } catch(NumberFormatException e) {
            return false;
        }
        if(id < 0 || id > VNMR_MAX_COLOR)
            return false;
        if(key.equals("graphics") || key.equals("ps") || key.equals("newgraphics")) {
            Color vcolor = VnmrRgb.getColorByName(value);
            red = vcolor.getRed();
            grn = vcolor.getGreen();
            blu = vcolor.getBlue();
            cmd = "setcolor('" + key + "','" + id + "','" + red + "','" + grn
                    + "','" + blu + "')";
            sendSetColor(cmd);
            if(id == 11 || id == 42 || id == 44) {
               Util.sendToVnmr("setSpecColor('init'," + id + ",'" + value + "')");
            }

            if(key.equals("graphics") || key.equals("newgraphics"))
                return true;
            else
                return false;
        } else {
            String r = "red green blue cyan magenta yellow orange white black";
            tok = new StringTokenizer(r, " ", true);
            while(tok.hasMoreTokens()) {
                String t = tok.nextToken();
                if(t.equals(value)) {
                    cmd = "setcolor('" + key + "','" + id + "','" + value
                            + "')";
                    sendSetColor(cmd);
                    return false;
                }
            }
        }
        return false;
    }

    /** read preferences file in ~/vnmrsys/persistence */
    public static void readPrefs() {
        BufferedReader in;
        String line;
        String symbol;
        String value;
        StringTokenizer tok;
        String path = FileUtil.openPath(prefsfile);
        if(path==null)
            return;
        try {
        	setDebug("readPrefs:"+path);
            in = new BufferedReader(new FileReader(path));
            while ((line = in.readLine()) != null) {
                tok = new StringTokenizer(line, " \t");
                if (!tok.hasMoreTokens())
                    continue;
                symbol=tok.nextToken();
                if (!tok.hasMoreTokens())
                    continue;
                value=tok.nextToken();
                if(symbol.equals("format")){
                    if(value.equals("rgb"))
                        m_outputHex=false;
                    else
                        m_outputHex=true;
                }
                if(symbol.equals("antialias")){
                    if(value.equals("true"))
                        m_enableAA=true;
                    else
                        m_enableAA=false;
                }
                // NB: always get LAF from theme or persistence file
                else if(symbol.equals("laf")){
                    LAF=value;
                    setDebug("readPrefs LAF="+LAF);
                }
                else if(symbol.equals("interface_theme")){
                    m_interface_item=value;
                    m_interface_type=USER;
                    if (tok.hasMoreTokens()){
                        value = tok.nextToken();
                        if(value.equals("SYSTEM"))
                            m_interface_type=SYSTEM;
                        else if(value.equals("DFLT"))
                            m_interface_type=DFLT;
                   }
                }
                else if(symbol.equals("graphics_theme")){
                    m_graphics_item=value;
                    m_graphics_type=USER;
                    if (tok.hasMoreTokens()){
                        value = tok.nextToken();
                        if(value.equals("SYSTEM"))
                            m_graphics_type=SYSTEM;
                        else if(value.equals("DFLT"))
                            m_graphics_type=DFLT;
                   }
                }
                else if(symbol.equals("plot_theme")){
                    m_plot_item=value;
                    m_plot_type=USER;
                    if (tok.hasMoreTokens()){
                        value = tok.nextToken();
                        if(value.equals("SYSTEM"))
                            m_plot_type=SYSTEM;
                        else if(value.equals("DFLT"))
                            m_plot_type=DFLT;
                   }
                }
                else if (symbol.equals("tabstype")){
                    while(tok.hasMoreTokens())
                        value += (" "+tok.nextToken());
                    m_tabsType=value;
                }
                else if (symbol.equals("graphics_tab")){
                    while(tok.hasMoreTokens())
                        value += (" "+tok.nextToken());
                    m_graphics_tab=value;
                }
                else if (symbol.equals("plot_tab")){
                    while(tok.hasMoreTokens())
                        value += (" "+tok.nextToken());
                    m_plot_tab=value;
                }
                else if (symbol.equals("interface_tab")){
                    while(tok.hasMoreTokens())
                        value += (" "+tok.nextToken());
                    m_interface_tab=value;
                }
            }
        	setDebug("Prefs: TabsGroup="+m_tabsType+" GraphicsTab="+m_graphics_tab+" UITab="+m_interface_tab+" PlotTab="+m_plot_tab);

            in.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }

    /** read persistence file in ~/vnmrsys/persistence */
    protected static void readStyles(String path) {
        BufferedReader in;
        String line;
        String symbol;
        String type;
        String value;
        StringTokenizer tok;
        DVar var;
        m_newlaf=false;
        if(path==null)
            return;
        try {
        	setDebug("readStyles:"+path);
            in = new BufferedReader(new FileReader(path));
            while ((line = in.readLine()) != null) {
                tok = new StringTokenizer(line, " \t");
                if (!tok.hasMoreTokens())
                    continue;
                symbol = tok.nextToken();
                if (!tok.hasMoreTokens())
                    continue;
                if(m_tabsType.equals(UI) && symbol.equals("LAF")){
                	value=tok.nextToken();
                	if(LAF !=null && !value.equals(LAF)){
                    	newLAF=value;
                    	m_newlaf=true;
                	}
                	continue;
                }

                type=optionType(symbol);
                if(type==null)
                    continue;
                if (!tok.hasMoreTokens())
                    continue;
                value = tok.nextToken();
                var=null;
                var=(DVar)vars.get(symbol);
                if(var!=null && m_vars_initialized){
                    if(var.type.equals(COLOR)){
                        value=formatColorString(value);
                        if(!sameColor(value,var.value)){
                            if(testSysColor(symbol,value))
                                m_repaint=true;
                            else if(testDpsColor(symbol,value))
                                m_repaint=true;
                            // else if(testVJColor(symbol,value))
                            //     m_lafchanged=true;
                        }
                    }
                    var.value=value;                    
                }
                if(var==null){
                	setDebug("Adding "+m_tabsType+" Variable: "+symbol+" "+value);
                    var=new DVar(symbol,type,m_tabsType,value);
                    vars.put(symbol,var);
                }
            }
            in.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }

    /** write persistence file in ~/vnmrsys/persistence */
    public static void writePersistence() {
        setDebug("writePersistence");
        String oldtabs=m_tabsType;
        writePrefs();
        m_tabsType=UI;
        String path = FileUtil.savePath(getPersistenceFile(UI));
        writeStyles(path);
        
        m_tabsType=GRAPHICS;
        path = FileUtil.savePath(getPersistenceFile(GRAPHICS));
        writeStyles(path);

        m_tabsType=PLOT;
        path = FileUtil.savePath(getPersistenceFile(PLOT));
        writeStyles(path);
        
        writeFonts();
        m_tabsType=oldtabs;

    }
    
    /** read persistence file in ~/vnmrsys/persistence */
    public static void readPersistence() {
        String oldtabs=m_tabsType;
        setDebug("readPersistence");
        readPrefs();
        m_tabsType=UI; 
        String path = FileUtil.openPath(getPersistenceFile(UI));
        if(path ==null || bAdminUser){
            path=getSystemThemeDir();
            path = FileUtil.openPath(path+"/"+m_tabsType+"/Default");
            LAF= VLookAndFeelFactory.VNMRJ;
        }
        if(path!=null)
        	readStyles(path);
        else
            Messages.postError("DisplayOptions: could not find "+UI+" theme file on startup");

        m_tabsType=GRAPHICS;
        path = FileUtil.openPath(getPersistenceFile(GRAPHICS));
        if(path ==null || bAdminUser){
            path=getSystemThemeDir();
            path = FileUtil.openPath(path+"/"+m_tabsType+"/Default");
        }
        if(path!=null)
        	readStyles(path);
        else
            Messages.postError("DisplayOptions: could not find "+GRAPHICS+" theme file on startup");

        m_tabsType=PLOT;
        path = FileUtil.openPath(getPersistenceFile(PLOT));
        if(path ==null){
            path=getSystemThemeDir();
            path = FileUtil.openPath(path+"/"+m_tabsType+"/Default");
        }
        if(path!=null)
            readStyles(path);
        else
            Messages.postError("DisplayOptions: could not find "+PLOT+" theme file on startup");

        m_tabsType=oldtabs;
        m_vars_initialized=true;

        if(FileUtil.openPath(colorsdir+colorsfile)==null)
            writeColors(colorsfile,ALL);
    }

    /** write prefs file in ~/vnmrsys/persistence */
    protected static void writePrefs() {
        String path = FileUtil.savePath(prefsfile);
        if(path==null){
            Messages.postError(" DisplayOptions: error writing preferences file");
            return;
        }
        PrintWriter os;
        try {
            os = new PrintWriter(new FileWriter(path));
            if(m_outputHex)
                os.println("format hex");
            else
                os.println("format rgb");
            if(m_enableAA)
                os.println("antialias true");
            else
                os.println("antialias false");
           setDebug("writePrefs LAF="+LAF);
            os.println("laf "+LAF);
            if(m_interface_type==SYSTEM)
                os.println("interface_theme "+m_interface_item+" SYSTEM");
            else if(m_interface_type==DFLT)
                os.println("interface_theme "+m_interface_item+" DFLT");
            else
                os.println("interface_theme "+m_interface_item+" USER");
            if(m_graphics_type==SYSTEM)
                os.println("graphics_theme "+m_graphics_item+" SYSTEM");
            else if(m_graphics_type==DFLT)
                os.println("graphics_theme "+m_graphics_item+" DFLT");
            else
                os.println("graphics_theme "+m_graphics_item+" USER");
            if(m_plot_type==SYSTEM)
                os.println("plot_theme "+m_plot_item+" SYSTEM");
            else if(m_plot_type==DFLT)
                os.println("plot_theme "+m_plot_item+" DFLT");
            else
                os.println("plot_theme "+m_plot_item+" USER");
            os.println("tabstype "+m_tabsType);
            os.println("graphics_tab "+m_graphics_tab);
            os.println("interface_tab "+m_interface_tab);
            os.println("plot_tab "+m_plot_tab);
            os.close();
       }
        catch(IOException er) {
            Messages.postError("DisplayOptions: could not create preferences file");
        }
    }

    /** write persistence file in ~/vnmrsys/persistence */
    protected static void writeStyles(String path) {
        //String path = FileUtil.savePath(perfile);
        if(path==null){
            Messages.postError("DisplayOptions: theme path is null");
            return;
        }

        // Use this to print out styles in alphabetical order
        Collator alphaOrder = Collator.getInstance(); // For default locale
        SortedSet<String> sortedValues = new TreeSet<String>(alphaOrder);
        int nLength = vars.size();
        for (int i=0; i < nLength; i++) {
            String key = (String)vars.getKey(i);
            DVar var = (DVar)vars.get(key);
            if(var.tabs.equals(m_tabsType))
            	sortedValues.add(var.toString());
        }
        PrintWriter os = null;
        try {
            os = new PrintWriter(new FileWriter(path));
            if(m_tabsType.equals(UI) && LAF!=null)
            	os.println("LAF "+LAF);
            for (String value : sortedValues) {
                os.println(value);
            }
        } catch(IOException er) {
            Messages.postError("DisplayOptions: can't write persistence file");
        } finally {
            try {
                os.close();
            } catch (Exception e) {}
        }
    }

    /** write DEFAULT file in ~/vnmrsys/templates/colors */
    protected static void writeColors(String file,int type) {
        String path = FileUtil.savePath(colorsdir+file);
        if(path==null){
            Messages.postError(" DisplayOptions: error writing colors file");
            return;
        }
        PrintWriter os;
        try {
            os = new PrintWriter(new FileWriter(path));
            setDebug("writing colors file: "+path);
            String keys = "graphics ps pcl hpgl pen";
            int nLength = vars.size();
            for(int i=0;i<nLength;i++){
                DVar var=(DVar)vars.get((String)vars.getKey(i));
                String symbol=var.symbol;
                String value=var.value;
                if(var.type.equals("Color")){

                    String name = symbol.replace("Color", "");
                    String key = null;
                    StringTokenizer tok = new StringTokenizer(keys, " ", true);
                    String cmd = null;
                    int id = -1;
                    int red, grn, blu;

                    while(tok.hasMoreTokens()) {
                        String t = tok.nextToken();
                        if(name.startsWith(t)) {
                            key = t;
                            break;
                        }
                    }
                    if(key == null)
                        continue;
                    try {
                        String sid = name.replace(key, "");
                        id = Integer.decode(sid);
                    } catch(NumberFormatException e) {
                        continue;
                    }
                    if(type==PLOTTER && (key.equals("graphics") || key.equals("newgraphics")))
                    	continue;
                    if(key.equals("graphics") || key.equals("ps") || key.equals("newgraphics")) {
                        Color vcolor = VnmrRgb.getColorByName(value);
                        red = vcolor.getRed();
                        grn = vcolor.getGreen();
                        blu = vcolor.getBlue();
                        cmd = "setcolor('" + key + "','" + id + "','" + red + "','" + grn
                                + "','" + blu + "')";
                     } else {
                        String r = "red green blue cyan magenta yellow orange white black";
                        tok = new StringTokenizer(r, " ", true);
                        while(tok.hasMoreTokens()) {
                            String t = tok.nextToken();
                            if(t.equals(value)) {
                                cmd = "setcolor('" + key + "','" + id + "','" + value + "')";
                                break;
                            }
                        }
                    }
                    if(cmd !=null){
                        os.println(cmd);
                    }
                }
            }
            os.close();
       }
       catch(IOException er) {
            Messages.postError("DisplayOptions: could not create colors file");
       }
       //Util.sendToVnmr("savecolors");
    }

    private static JLabel refLabel = null;
    private static GraphicsFont defaultFont = null;

    protected static void writeFont(PrintWriter os, Font fnt, String name) {
        FontMetrics fm = refLabel.getFontMetrics(fnt);
        int w = fm.charWidth('M');
        int a = fm.getAscent();
        int d = fm.getDescent();
        int h = a + d;
        if (h < 5)
            h = fnt.getSize();
        os.println(name+"  "+fnt.getName()+" "+w+" "+h+" "+a+" "+d);
    }

    public static void writeFonts(String path) {
        if (path == null || vars == null)
            return;
        int nLength = vars.size();
        PrintWriter os = null;
        if (nLength < 1)
            return;
        if (refLabel == null)
            refLabel = new JLabel();
        try {
            os = new PrintWriter(new FileWriter(path));
            int r = Toolkit.getDefaultToolkit().getScreenResolution();
            if (defaultFont == null)
                 defaultFont = new GraphicsFont();
            else
                 defaultFont.update();
            writeFont(os, defaultFont.getFont(), "Default");
            for (int i = 0; i < nLength; i++) {
                 String key = (String)vars.getKey(i);
                 DVar var = (DVar)vars.get(key);
                 if (var.tabs.equals(GRAPHICS)) {
                     if (var.type.equals(FONT)) {
                          String name;
                          int k = var.symbol.indexOf(FONT);
                          if (k > 0)
                              name = var.symbol.substring(0, k);
                          else
                              name = var.symbol;
                          Font fnt = getFont(name);
                          writeFont(os, fnt, name);
                     }
                 }
            }
            if (r < 20)
                r = 90;
            os.println("ScreenDpi Screen "+r+" "+r+" "+r+" "+r);
        } catch(IOException er) {
            Messages.postError("DisplayOptions: can't write persistence file");
        }
        catch(Exception ex) {
        }
         finally {
            try {
                os.close();
            } catch (Exception e) {}
        }
    }

    public static void writeFonts() {
        String path = FileUtil.savePath(getPersistenceFile(".Fontlist"));
        writeFonts(path);
    }

    /** identify variable container objects  */
    private void  getVariables(JComponent parant) {
        int nSize = parant.getComponentCount();
        for (int i=0; i<nSize; i++) {
            Component comp = parant.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
                String symbol=obj.getAttribute(KEYSTR);
                if(symbol!=null){
                    String val=obj.getAttribute(KEYVAL);
                    xvars.put(symbol,val);
                    DVar var=(DVar)vars.get(symbol);
                    if(var != null){
                        var.obj=obj;
                        var.obj.setAttribute(KEYVAL,var.value);
                    }
                    else{
                        String type=optionType(symbol);
                        if(type !=null){
                            setDebug("Adding Style Variable: "+symbol+" "+val);
                            var=new DVar(symbol,type,m_tabsType,val);
                            var.obj=obj;
                            vars.put(symbol,var);
                        }
                    }
                }
                if(obj instanceof VGroupIF)
                    getVariables((JComponent)obj);
            }
        }
    }

    /** remove "orphaned" display variables (not in xml file) */
    private void  pruneVariables() {
        Enumeration elems=vars.keys();
        while(elems.hasMoreElements()){
            String symbol=(String)elems.nextElement();
            if(!xvars.containsKey(symbol)){
                setDebug("deleting "+symbol);
                vars.remove(symbol);
            }
        }
    }

    public void setVnmrIf(ButtonIF e) {
        vnmrIF = e;
    }

    private static String getXmlFile(String type){
    	return "INTERFACE/"+type+".xml";
    }
    private static String getPersistenceFile(String type){
    	return "USER/PERSISTENCE/"+type;
    }
    /** build xml folderPan   */
    // note: folderPan cannot be built in constructor since xml widgits may be
    //       PropertyChange listeners. (Util.getDisplayOptions not valid yet)
    //
    public void build() {
        if(built)
        	return;        
        try {
        	setDebug("build");
            folderPan.removeAll();
            String oldtabs=m_tabsType;
            setDebug("building interface tabs"); 

            m_tabsType=UI;
        	String path=FileUtil.openPath(getXmlFile(UI));
        	LayoutBuilder.build(folderPan,vnmrIF,path);
            Component comp=folderPan.getComponent(0);
            m_interfaceTabs=(VTabbedPane)comp;
            getVariables(m_interfaceTabs);
            
            setDebug("building graphics tabs"); 
            m_tabsType=GRAPHICS;
            folderPan.removeAll();
            path=FileUtil.openPath(getXmlFile(GRAPHICS));
            LayoutBuilder.build(folderPan,vnmrIF,path);
            comp=folderPan.getComponent(0);
            m_graphicsTabs=(VTabbedPane)comp;
            getVariables(m_graphicsTabs);

            setDebug("building plot tabs"); 
            m_tabsType=PLOT;
            folderPan.removeAll();
            path=FileUtil.openPath(getXmlFile(PLOT));
            LayoutBuilder.build(folderPan,vnmrIF,path);
            comp=folderPan.getComponent(0);
            m_plotTabs=(VTabbedPane)comp;
            getVariables(m_plotTabs);

            //pruneVariables();
   
            //setLAFParam();

            ExpPanel.addExpListener(this); // register as a vnmrbg listener
            //ExpPanel.addExpListener(m_interfaceTabs);
            //ExpPanel.addExpListener(m_graphicsTabs);
            //ExpPanel.addExpListener(m_plotTabs);

            updateValue(m_interfaceTabs);
            updateValue(m_graphicsTabs);
            updateValue(m_plotTabs);
            
            pack();

            m_tabsType=oldtabs;
            if(m_tabsType.equals(UI))
            	showUIBtn.setSelected(true);
            else if(m_tabsType.equals(GRAPHICS))
            	showGraphicsBtn.setSelected(true);
            else
                showPlotBtn.setSelected(true);
            
            changeTabType(m_tabsType);
            
            setLAFParam();
          	
            built=true;
        }
        catch(Exception e) {
            Messages.writeStackTrace(e);
        }
     }
    // public static functions
    public static boolean outputHex(){
        return m_outputHex;
    }
    public static String HexToRGBString(String str) {
        String rstr=str;
        if (str.startsWith("0x") || str.startsWith("0X")) {
            try {
                String s = str.substring(2);
                int icolor = Integer.parseInt(s, 16);
                int red=(icolor>>16)&0xFF;
                int grn=(icolor>>8)&0xFF;
                int blu=icolor&0xFF;
                rstr=""+red+","+grn+","+blu;
            } catch (NumberFormatException e ) {
            }
        }
        return rstr;
    }

    public static String RGBToHexString(String str) {
        String rstr=str;
        if(str.contains(",")) {
            try {
                int red=0;
                int grn=0;
                int blu=0;
                String symbol;
                StringTokenizer tok= new StringTokenizer(str, ",");
                if (tok.hasMoreTokens()){
                    symbol = tok.nextToken();
                    red=(Integer.parseInt(symbol, 10))&0xFF;
                    if (tok.hasMoreTokens()){
                        symbol = tok.nextToken();
                        grn=(Integer.parseInt(symbol, 10))&0xFF;
                        if (tok.hasMoreTokens()){
                            symbol = tok.nextToken();
                            blu=(Integer.parseInt(symbol, 10))&0xFF;
                        }
                   }
                }
                int icolor = red*0x10000+grn*0x100+blu;
                String s = Integer.toString(icolor, 16);
                s=s.toUpperCase();
                rstr="0x"+s;
            } catch (NumberFormatException e ) {
            }
        }
        return rstr;
    }
    public static String formatColorString(String str) {
        if(m_outputHex)
            return RGBToHexString(str);
        else
            return HexToRGBString(str);
    }

    public static String colorToRGBString(Color color) {
        String scolor = ""+color.getRed()+","+color.getGreen()+","+color.getBlue();
        return scolor;
    }

    public static String colorToHexString(Color color) {
       int icolor = (color.getRed() * 0x10000
                     + color.getGreen() * 0x100
                     + color.getBlue());
       String scolor = Integer.toString(icolor, 16);
       scolor=scolor.toUpperCase();
       return "0x"+scolor;
   }

    /** get type (e.g."Style" from symbol "SmallTitleStyle") */
    public static String optionType(String symbol){
        int nLength = types.size();
        for(int i=0;i<nLength;i++){
            String str=(String)types.get(i);
            int indx=symbol.indexOf(str);
            if(indx>=0)
                return str;
        }
        return null;
    }

    /** get type (e.g."SmallTitle" from symbol "SmallTitleStyle") */
    public static String optionFamily(String symbol){
        int nLength = types.size();
        for(int i=0;i<nLength;i++){
            String str=(String)types.get(i);
            int indx=symbol.indexOf(str);
            if(indx>=0)
                return symbol.substring(0,indx);
        }
        return null;
    }

    /** set option value.
        called from ExpPanel on vnmrjcmd "display options"
        xml attr e.g. vc="vnmrjcmd('display options','NormalTitleStyle $VALUE')"
    */
    public static void setOption(String s){
        StringTokenizer tok = new StringTokenizer(s, " \t=");

        String symbol=tok.nextToken();
        if(!tok.hasMoreTokens())
            return;
        String val=tok.nextToken();
        DVar var=(DVar)vars.get(symbol);

        if(var ==null)
            return;
        if(var.type==COLOR && sameColor(var.value,val))
            return;

        setDebug("changed var "+var.symbol+"="+val);
        boolean bUpdateUI = false;

        if(var.obj !=null)
            var.obj.setAttribute(KEYVAL,val);
        if(var.type.equals("Color")){
            if(testSysColor(symbol,val))
                sendRepaint();
            else if(testDpsColor(symbol,val))
                sendRepaint();
            else if(testVJColor(symbol,val)){
                var.value=val;
                setUIColors();
                bUpdateUI = true;
                /****
                if (!LAF.equals(VLookAndFeelFactory.VNMRJ))
                    bUpdateUI = true;
                else
                    sendNewBgColor();
                ****/
            }
       }
        var.value=val;
        m_styleMap.clear();
       if (bUpdateUI)
           updateLookAndFeel();
       else
    	   setDebug("firePropertyChange");
           typesmgr.firePropertyChange("DisplayOptions "+var.symbol,var.symbol,var.value);
    }

    private void updateVars(){
        setDebug("changed color format");
        int nLength = vars.size();
        for(int i=0;i<nLength;i++){
            String key=(String)vars.getKey(i);
            DVar var=(DVar)vars.get(key);
            String oldval=var.value;
            String newval=formatColorString(oldval);
            if(!newval.equals(oldval)){
                var.value=newval;
                var=(DVar)vars.get(key);
            }
        }
        sendUpdateAll();
     }

    /** return list of all vnmr variables  */
    public static ArrayList getSymbols(){
        if(symbols !=null)
            return symbols;
        Hashtable hash=new Hashtable();
        symbols=new ArrayList();
        int nLength = vars.size();
        for(int i=0;i<nLength;i++){
            String key=(String)vars.getKey(i);
            DVar var=(DVar)vars.get(key);
            String name=var.family;
        	if(GRAPHICS.equals(var.tabs))
        		continue;
            if(hash.containsKey(name))
                continue;
            hash.put(name,name);
            symbols.add(name);
        }
        return symbols;
    }

    /** return list of system variables for type keyword  */
    public static ArrayList getTypes(String attr){
    	Util.getDisplayOptions();
        ArrayList list=new ArrayList();
        int nLength = vars.size();
        for(int i=0;i<nLength;i++){
            String key=(String)vars.getKey(i);
            DVar var=(DVar)vars.get(key);
            if(var.type.equals(attr)){
            	if(GRAPHICS.equals(var.tabs))
            		continue;
                list.add(var.family);
            }
        }
        return list;
    }

    /** return Option string for keyword attr (or null)  */
    public static String getOption(String s){
        if(s==null)
            return null;
        DVar var=(DVar)vars.get(s);
        if(var !=null){
            return var.value;
        }
        return null;
    }

    /** return Option string for keyword attr (or null)  */
    public static String getOption(String attr, String s){
        if(s==null)
            return null;
        DVar var=(DVar)vars.get(s+attr);
        if(var !=null){
            return var.value;
        }
        return null;
    }

    /** return true if c is a DisplayOptions variable */
    public static boolean isOption(String attr, String c){
        String s=getOption(attr,c);
        return s==null?false:true;
    }

    /** return a VJ Font */
    public static Font getFont(String fontName, String fontStyle, String fontSize){
        String[] aStrfont = getfont(fontName, fontStyle, fontSize);
        String name = aStrfont[0];
        int style = Integer.parseInt(aStrfont[1]);
        int size = Integer.parseInt(aStrfont[2]);
        return getFont(name, style, size);
    }
    /** return a VJ Font */
    public static Font getFont(String fontName, int fontStyle, int fontSize) {
        String strFont = new StringBuffer().append(fontName).append(fontStyle)
                .append(fontSize).toString();
        Font font = (Font)hmFonts.get(strFont);
        if(font == null) {
            font = new Font(fontName, fontStyle, fontSize);
            hmFonts.put(strFont, font);
        }
        return font;
    }

    protected static String[] getfont(String strName, String strStyle, String strSize)
    {
        String[] aStrfont = {"", "", ""};
        String s;
        int style=Font.PLAIN;
        int size=12;
        String name="Dialog";
        aStrfont[0] = name;
        if(strStyle ==null)
        	strStyle="PlainText";
        s=getOption(STYLE,strStyle);
        if(s==null)
        	strStyle="PlainText";
        s=getOption(STYLE,strStyle);
        if(s != null)
            strName=strSize=strStyle;
        else
            s=strStyle;
        	
        style=Util.fontStyle(s);
        aStrfont[1] = s;
        
        if(strName != null){
            s=getOption(FONT,strName);
            if(s==null){
              try {   
                Font f=Font.getFont(strName);
                if(f==null)
                    s=strName;
              }
              catch (Exception er) {
                 s = "Dialog";
              }
            }
            name=s;
            aStrfont[0] = s;
        }
        if(strSize != null){
            s=getOption(SIZE,strSize);
            if(s==null)
                s = strSize;
            aStrfont[2] = s;
            try {
                size = Integer.parseInt(s);
            }
            catch (NumberFormatException fe) {}
        }
        aStrfont[0] = name;
        aStrfont[1] = String.valueOf(style);
        aStrfont[2] = String.valueOf(size);
        return aStrfont;
    }

    // get line thickness by string strName+"Thickness", e.g., AxisThickness,
    // where strName="Axis"
    public static double getLineThickness(String strName)
    {
        double thick = 1.0;
        String s = getOption(THICKNESS, strName);
        if (s == null) {
            return thick;
        }
        try {
            thick = Double.parseDouble(s);
        } catch (NumberFormatException nfe) {
            Messages.postDebug("getLineThickness: bad thickness string: \"" + s + "\"");
        }
        return thick;
    }

    // screen resolution is pixels per inch. 
    public static int getLineThicknessPix(String strName) 
    {
        double thick = getLineThickness(strName);
        int n = 0;

        boolean points = false; // thickness is in pixels, not points.
        if(points) n = Toolkit.getDefaultToolkit().getScreenResolution();

        if(n <= 0) n = (int)Math.floor(0.5 + thick);
        else n = (int)Math.floor(0.5 + thick*n/72); //thick is in points (72 points per inch) 
        if(n<1) return 1;
        else return n;
    }

    public static int getLineThicknessPix(String strName1, String strName2, double ratio) 
    {
        double thick;
        double minthick = getLineThicknessPix(strName1);
        double maxthick = getLineThicknessPix(strName2);
        if(minthick > maxthick) {
	   thick = minthick;
	   minthick = maxthick;
	   maxthick = thick; 
        }
        thick = minthick + ratio*(maxthick - minthick); 
        int n = (int)Math.floor(0.5 + thick);
        if(n<1) return 1;
	else if(n>maxthick) return (int)maxthick;
        else return n;
    }

    public static Font getFont(String strName, String strStyle)
    {
        String[] aStrfont = getfont(strName, strStyle, "");
        String name = aStrfont[0];
        int style = Integer.parseInt(aStrfont[1]);
        int size = Integer.parseInt(aStrfont[2]);
        Font font = new Font(name, style, size);
        return font;
    }

    /** return a VJ Font */
    public static Font getFont(String name){
        return getFont(name,name,name);
    }

    public static DisplayStyle getCachedStyle(String key) {
        return m_styleMap.get(key);
    }

    public static void putCachedStyle(String key, DisplayStyle style) {
        m_styleMap.put(key, style);
    }

    public static int getFontStyle(String strStyle) {
        String s = getOption(STYLE, strStyle);
        if(s == null) {
            s = strStyle;
        }
        int style = Util.fontStyle(s);
        return style;
    }

    public static float getFontSize(String strSize) {
        float size = 12;
        String s = getOption(SIZE, strSize);
        if (s == null) {
            s = strSize;
        }
        try {
            size = Float.parseFloat(s);
        } catch (NumberFormatException nfe) {
            Messages.postDebug("getFontSize: bad size string: \"" + s + "\"");
        }
        return size;
    }

    public static String getFontName(String name) {
        String s = getOption(FONT, name);
        if (s == null) {
            Font f = Font.getFont(name);
            if(f == null) {
                s = name;
            }
        }
        name = s;
        return name;
    }

    /** return a VJ Color */
    public static Color getColor(String c){
        String s=getOption(DisplayOptions.COLOR,c);
        if(s!=null)
            c=s;
	Color color = (Color)hmColors.get(c);
        if (color == null)
        {
            color = VnmrRgb.getColorByName(c);
            hmColors.put(c, color);
        }

        return color;
    }

    // get color defined by string graphicsNNN or newgraphicsNNN, such as graphics12,
    // where NN=12 is colorID
    public static Color getColor(int colorID){
        // hard coded vnmrbg colors that are not defined in DisplayOptions:
        switch (colorID) {
          case BLACK:
              return Color.BLACK;
          case RED:
              return Color.RED;
          case YELLOW:
              return Color.YELLOW;
          case GREEN:
              return Color.GREEN;
          case CYAN:
              return Color.CYAN;
          case BLUE:
              return Color.BLUE;
          case MAGENTA:
              return Color.MAGENTA;
          case WHITE:
              return Color.WHITE;
          case ORANGE:
              return Color.ORANGE;
          case PINK:
              return Color.PINK;
          case GRAY:
              return Color.GRAY;
        }
        String c;
/*
        if(colorID > 256) c = "newgraphics"+colorID;
        else 
*/
        c = "graphics"+colorID;
        return getColor(c);
    }

    /** return a VJ Color */
    public static boolean hasColor(String c){
        String s=getOption(DisplayOptions.COLOR,c);
        if(s==null)
            return false;
        return true;
    }

    public static DisplayStyle getDisplayStyle(String dispStyle) {
        Font font = getFont(dispStyle);
        Color color = getColor(dispStyle);
        DisplayStyle rtn = new DisplayStyle(font, color);
        return rtn;
    }

    /** set action listeners  */
    private void  setActionListeners(){
        historyButton.setActionCommand("history");
        historyButton.addActionListener(this);
        undoButton.setActionCommand("undo");
        undoButton.addActionListener(this);
        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("abandon");
        abandonButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

        setCloseEnabled(true);
        setAbandonEnabled(true);
    }

    private void setTabs(VTabbedPane tabs){
    	folderPan.removeAll();
    	folderPan.add(tabs);
        updateValue(tabs);
    	pack();
    	tabs.invalidate();
    	folderPan.invalidate();
    	folderPan.repaint();
    }

    public static void setTheme(String type,String s){
        setDebug("setTheme:"+type+" "+s);
        
        if(type.equalsIgnoreCase(GRAPHICS)){
            DisplayOptions d=Util.getDisplayOptions();
            m_graphics_item=s;
            d.getSelectedTab();
            d.showGraphicsBtn.setSelected(true);
            d.changeTabType(GRAPHICS);
            d.loadTheme();
            sendUpdateAll();
        }
        else if(type.equalsIgnoreCase(UI)){
            DisplayOptions d=Util.getDisplayOptions();
            m_interface_item=s;
            d.getSelectedTab();
            d.showUIBtn.setSelected(true);
            d.changeTabType(UI);
            d.loadTheme();
        }
        else if(type.equalsIgnoreCase(PLOT)){
            DisplayOptions d=Util.getDisplayOptions();
            m_plot_item=s;
            d.getSelectedTab();
            d.showPlotBtn.setSelected(true);
            d.changeTabType(PLOT);
            d.loadTheme();
        }

    }
    public void changeTabType(String type){
    	setDebug("changeTabType:"+type);
    	m_tabsType=type;
    	if(type.equals(UI)){
    		setTabs(m_interfaceTabs);
    		laf_label.setVisible(true);
    		laf_menu.setVisible(true);
    		enableAAcheck.setVisible(false);
    	}
    	else if(type.equals(GRAPHICS)){
    		laf_menu.setVisible(false);
            laf_label.setVisible(false);
            enableAAcheck.setVisible(true);
    		setTabs(m_graphicsTabs);
    	}
        else if(type.equals(PLOT)){
            laf_menu.setVisible(false);
            laf_label.setVisible(false);
            enableAAcheck.setVisible(true);
            setTabs(m_plotTabs);
        }
    	getThemes();
    	if(type.equals(UI))
    		showTab(m_interface_tab);
    	else if(type.equals(PLOT))
    	    showTab(m_plot_tab);
    	else
    		showTab(m_graphics_tab);
    }

    private void getPlotterName() {
        if (plotterInfo == null)
           return;
        String str = plotterInfo.getText();
        if (str != null && str.length() > 0)
            plotterfile = str;
    }

    /** handle button events. */
    public void actionPerformed(ActionEvent e ) {
        String cmd = e.getActionCommand();
        if(cmd.equals("close"))  {
            getSelectedTab();
            writePersistence();
            writeColors(colorsfile,ALL);
            getPlotterName();
            if(plotterfile.length()>0)
            	writeColors(plotterfile,PLOTTER);
            setVisible(false);
            dispose();
            return;
        }
        else if(cmd.equals("abandon"))  {
            readPersistence();
            getLAFs();
            setLAF();
             //sendRepaint();
            setVisible(false);
            dispose();
            sendRepaint();
            return;
        }
        else if (cmd.equals("help")) {
            displayHelp();
            return;
        }
        else if (cmd.equals("SaveTheme")){
            saveTheme();
            return;
        }
        else if (cmd.equals("DeleteTheme")){
            deleteTheme();
            return;
        }
        else if (cmd.equals("ReloadTheme")){
            m_changemode=true;
            loadTheme();
            m_changemode=false;
            return;
        }
        else if (cmd.equals("EnableAA")){
            m_enableAA=enableAAcheck.isSelected();
            sendRepaint();
            return;
        }
        else if (cmd.equals("ShowHex")){
        	m_outputHex=showHexcheck.isSelected();
            updateVars();
            return;
        }
        else if (cmd.equals("ShowUI")){
        	getSelectedTab();
        	changeTabType(UI);
            return;
        }
        else if (cmd.equals("ShowDisp")){
        	getSelectedTab();
        	changeTabType(GRAPHICS);
            return;
        }        
        else if (cmd.equals("ShowPlot")){
            getSelectedTab();
            changeTabType(PLOT);
            return;
        }        
        else if (cmd.equals("theme_menu")){
            if(!m_changemode){
                //System.out.println("theme_menu event");
                JComboBox cb = (JComboBox)e.getSource();
                int index = cb.getSelectedIndex();
                Theme theme=(Theme)theme_menu.getItemAt(index);
                if(theme !=null)
                    setThemeComp(theme_menu,theme.type);
                else
                    setThemeComp(theme_menu,USER);
                setThemeButtons(index);
                loadTheme();
            }
            return;
        }
        else if (cmd.equals("laf_menu")){
            if(!m_changemode){
                JComboBox cb = (JComboBox)e.getSource();
                int index = cb.getSelectedIndex();
                String laf=(String)laf_menu.getItemAt(index);
                if(laf.equalsIgnoreCase("system"))
                	setThemeComp(laf_menu,SYSTEM);
                else
                	setThemeComp(laf_menu,DFLT);
                	
                LAF=laf;
                setLAF();
            }
            return;
        }        
        else if (cmd.equals("timer")){
        	//defaults=UIManager.getDefaults();
          	defaults=UIManager.getLookAndFeelDefaults();
        	Color newbgc=defaults.getColor("Panel.background");
        	if(!newbgc.equals(bgc)){
        		setDebug("new system laf newbg="+newbgc+" oldbg="+bgc);
	        	bgc=newbgc;
	        	setUIColors();
	            sendNewBgColor();
	            bgc=newbgc;
        	}
            timer.stop();
            return;
         }
        else{
            setDebug("UNTRAPPED ACTION: "+cmd);
        }
    }

	public static void updateUIColor() {
		defaults = UIManager.getLookAndFeelDefaults();
		Color newbgc = defaults.getColor("Panel.background");
		if (bgc == null || !newbgc.equals(bgc)) {
			bgc = newbgc;
			setUIColors();
			// sendNewBgColor();
			sendUpdateAll();
		}
	}

    class ThemesListRenderer extends  BasicComboBoxRenderer {
		public Component getListCellRendererComponent(JList list, Object value,
				int index, boolean isSelected, boolean cellHasFocus) {
			super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
			int indx = (index >= 0 && index < m_themes.size()) ? index : 0;
			Theme theme = m_themes.get(indx);
			setThemeComp(this, theme.type);
			setText(value.toString());
			return this;
		}
	}

    class LafListRenderer extends  BasicComboBoxRenderer {
 		public Component getListCellRendererComponent(JList list, Object value,
 				int index, boolean isSelected, boolean cellHasFocus) {
 			super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
 			String label=value.toString();
 			if(label.equalsIgnoreCase("system"))
 				setThemeComp(this, SYSTEM);
 			else
 				setThemeComp(this, DFLT);
 			setText(value.toString());
 			return this;
 		}
 	}

    class panelLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int   w = 0;
            int   h = 0;
            int   k;
            int   n = target.getComponentCount();
            for ( k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height+2;
            }
            return new Dimension(w, h);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim0 = optionsPan.getPreferredSize();
                Dimension dim1 = folderPan.getPreferredSize();
                int ww = dim1.width;
                optionsPan.setBounds(2, 2, ww-4, dim0.height);
                optionsContentPan.setBounds(4, 4, ww-6, dim0.height-4);
                folderPan.setBounds(2, dim0.height+2, ww-4, dim1.height);

             }
        }
    } // class panelLayout

    // inner class DVar

    public static class DVar {
        protected String symbol;
        protected String family;
        protected String type;
        protected String tabs;
        protected String value;
        protected VObjIF obj;

        public DVar(String s, String t,String c,String v){
            family=optionFamily(s);
            symbol=s;
            type=t;
            tabs=c;
            value=v;
            obj=null;
            if(DebugOutput.isSetFor("vars") )
                setDebug("new var: "+properties());

        }
        public String toString(){
            return symbol+" "+value;
        }
        public String properties(){
            return new String(tabs+": "+symbol+" "+value);
        }
    }

    // inner class Theme

    class Theme {
        protected String name;
        protected int type;
        protected int indx;

        public Theme(String s, int t, int i){
            name=s;
            type=t;
            indx=i;
         }
        public String toString(){
            return name;
        }
    }

    public static class DisplayStyle {
        private Font font;
        private Color fontColor;

        public DisplayStyle(Font font, Color fgcolor) {
            this.font = font;
            this.fontColor = fgcolor;
        }

        /**
         * @return the font
         */
        public Font getFont() {
            return font;
        }

        /**
         * @param font the font to set
         */
        public void setFont(Font font) {
            this.font = font;
        }

        /**
         * @return the fontColor
         */
        public Color getFontColor() {
            return fontColor;
        }

        /**
         * @param fontColor the fontColor to set
         */
        public void setFontColor(Color fontColor) {
            this.fontColor = fontColor;
        }

        public DisplayStyle deriveStyle(Font newFont) {
            return new DisplayStyle(newFont, fontColor);
        }

        public DisplayStyle deriveStyle(Color newColor) {
            return new DisplayStyle(font, newColor);
        }

        public String toString() {
            Color c = getFontColor();
            return new StringBuffer("DisplayStyle,name=")
                            .append(font.getFontName())
                            .append(",style=").append(font.getStyle())
                            .append(",size=").append(font.getSize2D())
                            .append(",color=").append(c)
                            .toString();
        }
    }
    class ShutdownHook extends Thread {
        public void run() {
            //System.out.println("Display options Shutting down");
            writePersistence();
        }
    }
    static class PropertySupportObject {
        public PropertySupportObject(){

        }
    }
}

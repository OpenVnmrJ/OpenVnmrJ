/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.lang.reflect.*;
import javax.swing.*;

import org.w3c.dom.*;
import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.DefaultHandler;
import javax.xml.parsers.SAXParserFactory;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.util.*;

/**
 * builder class for VObjIF based swing panels
 * @author      Dean Sindorf
 */
public class LayoutBuilder extends PanelTemplate implements VObjDef, Types
{
    static HashArrayList vattrs = new HashArrayList();
    static boolean  initialized = false;
    static Hashtable shuffler_tokens = new Hashtable();
    static Hashtable vobjs = new Hashtable();
    static Class[] argtypes = new Class[3];
    static boolean firstItem=true;
    static boolean isPanel=false;
    boolean expand_tabs=false;
    static VLabelResource m_resource = null;
    static boolean xml_locale=false;

    /** use SAX parser if true, DOM parser if false. */
    static public boolean useSAX=true;
    /** time XML build if true. */
    /** minimum dimension extent of a component. */
    static public int mindim=4;

    Object[]        vargs = new Object[3];
    JComponent      last_group=null;
    ButtonIF        VnmrIf;
    double          TMstart;
    double          TMend;
    String          TMname;
    JComponent      firstChild;
    private AppIF apIf = null;
    private SessionShare sshare = null;


   /** <pre>Constructor.
     *  The constructor for this class carries out a set of
     *  of static object initialization operations that are used to
     *  configure the builder. VObjIF developers can add or modify
     *  these operations to change or extend the builder's behavior.
     *
     * I. Initialize a static "attributes" list that ties xml
     *    attribute string names to VObjDef integer values.
     *
     *  Add an entry for each token VObjDef value attribute pair
     *
     *  <i>example entry</i>
     *     addAttribute("vc", new Integer(CMD));
     *
     *   <B>The following attribute String-Integer pairs are defined</B>
     *   -------------------------------------------------
     *   attr key       VObjDef int
     *   -------------------------------------------------
     *   "actionfile"   ACTION_FILE
     *   "arrow"        ARROW
     *   "arrowcolor"   ARROW_COLOR
     *   "bg"           BGCOLOR
     *   "border"       BORDER
     *   "chval"        CHVAL
     *   "color1"       COLOR1
     *   "color2"       COLOR2
     *   "color3"       COLOR3
     *   "color4"       COLOR4
     *   "color5"       COLOR5
     *   "color6"       COLOR6
     *   "color7"       COLOR7
     *   "color8"       COLOR8
     *   "color9"       COLOR9
     *   "color10"      COLOR10
     *   "color11"      COLOR11
     *   "color12"      COLOR12
     *   "color13"      COLOR13
     *   "color14"      COLOR14
     *   "color15"      COLOR15
     *   "count"        COUNT
     *   "decor1"       DECOR1
     *   "decor2"       DECOR2
     *   "decor3"       DECOR3
     *   "digital"      DIGITAL
     *   "digits"       NUMDIGIT
     *   "display"      DISPLAY
     *   "drag"         DRAG
     *   "editable"     EDITABLE
     *   "elastic"      ELASTIC
     *   "enable"       ENABLE
     *   "expanded"     EXPANDED
     *   "fg"           FGCOLOR
     *   "file"         PANEL_FILE
     *   "font"         FONT_NAME
     *   "graphbgcolor" GRAPHBGCOL
     *   "graphbgcolor2" GRAPHBGCOL2
     *   "graphfgcolor" GRAPHFGCOL
     *   "graphfgcolor2" GRAPHFGCOL2
     *   "graphfgcolor3" GRAPHFGCOL3
     *   "gridcolor",   GRIDCOLOR
     *   "icon"         ICON
     *   "incr1"        INCR1
     *   "incr2"        INCR2
     *   "justify"      JUSTIFY
     *   "key"          KEYSTR
     *   "keyval"       KEYVAL
     *   "logxaxis",    LOGXAXIS
     *   "logyaxis",    LOGYAXIS
     *   "max"          MAX
     *   "maxmark"      SHOWMAX
     *   "min"          MIN
     *   "minmark"      SHOWMIN
     *   "name"         PANEL_NAME
     *   "orientation"  ORIENTATION
     *   "panel"        PANEL_TAB
     *   "param"        PANEL_PARAM
     *   "point"        FONT_SIZE
     *   "pointy"       POINTY
     *   "radiobutton"  RADIOBUTTON
     *   "range",       RANGE
     *   "reference"    REFERENCE
     *   "rocker"       ROCKER
     *   "set"          SETVAL
     *   "set2"         SETVAL2
     *   "show"         SHOW
     *   "showgrid",    SHOWGRID
     *   "side"         SIDE
     *   "size1"        SIZE1
     *   "size2"        SIZE2
     *   "statcol"      STATCOL
     *   "statkey"      STATKEY
     *   "statpar"      STATPAR
     *   "statset"      STATSET
     *   "statshow"     STATSHOW
     *   "statval"      STATVAL
     *   "style"        FONT_STYLE
     *   "tab"          TAB
     *   "tickcolor",   TICKCOLOR
     *   "type"         PANEL_TYPE
     *   "useref"       USEREF
     *   "value"        LABEL
     *   "values"       VALUES
     *   "vc"           CMD
     *   "vc2"          CMD2
     *   "visible"      VISIBLE
     *   "vq"           VARIABLE
     *   "vq2"          VAR2
     *   "wrap"   	    WRAP
     *   "xaxisshow"    XAXISSHOW
     *   "xlabelshow"   XLABELSHOW
     *   "yaxisshow"    YAXISSHOW
     *   "ylabelshow"   YLABELSHOW
     *
     * II. Initialize a static "objects" list that ties xml
     *    object names to VObjIF class names.
     *
     *    Add an entry for each VObjIF xml object
     *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *  WARNING: these entry constructors must be included in the Dash-O dop
     *           file as Trigger Methods
     *     e.g.  in the dop file 'VnmrJ.dop'
     *     -TriggerMethods:
     *  vnmr.bo.VStyleChooser:<init>:vnmr.ui.SessionShare,vnmr.util.ButtonIF,java.lang.String
     *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *
     *  <i>example entry</i>
     *     addVObject("group", "vnmr.bo.VGroup");
     *
     *   <B>The following object String-String pairs are defined</B>
     *   -------------------------------------------------
     *   object key     VObjIF class name          extended class
     *   -------------------------------------------------
     *   "group"        "vnmr.bo.VGroup"           JPanel
     *   "label"        "vnmr.bo.VLabel"           JLabel
     *   "entry"        "vnmr.bo.VEntry"           JTextField
     *   "menu"         "vnmr.bo.VMenu"            JComboBox
     *   "radio"        "vnmr.bo.VRadio"           JRadioButton
     *   "tab"          "vnmr.bo.VTab"             JLabel
     *   "check"        "vnmr.bo.VCheck"           JCheckBox
     *   "button"       "vnmr.bo.VButton"          JButton
     *   "text"         "vnmr.bo.VText"            JScrollPane
     *   "scroll"       "vnmr.bo.VScroll"          JComponent
     *   "toggle"       "vnmr.bo.VToggle"          JToggleButton
     *   "magicbutton"  "vnmr.bo.VMagicButton"     JToggleButton
     *   "dial"         "vnmr.bo.VClockDial"       ClockDial
     *   "shimbutton"   "vnmr.bo.VUpDownButton"    UpDownButton
     *   "slider"       "vnmr.bo.VSlider"          ComboSlider
     *   "textmessage"  "vnmr.bo.VTextMsg"         JTextField
     *   "mlabel"       "vnmr.bo.VMenuLabel"       JLabel
     *   "panel"        "vnmr.bo.VPanelTab"        JLayeredPane
     *   "rfmonbutton"  "vnmr.bo.VRFMonButton"     VStatusButton
     *   "shimset"      "vnmr.bo.VShimSet"         VGroup
     *   "textfile"     "vnmr.bo.VTextFileWin"     VText
     *   "statusbutton" "vnmr.bo.VStatusButton"    TwoLineButton
     *   "hwstatus"     "vnmr.bo.VHardwareStatus"  JComponent
     *   "expmeter"     "vnmr.bo.VExperimentMeter" JProgressBar
     *   "trashcan"     "vnmr.ui.TrashCan"         VLabel
     *   "probebutton"  "vnmr.ui.ProbeButton"      VButton
     *   "thermometer"  "vnmr.bo.VThermometer"     JProgressBar
     *   "chart"        "vnmr.bo.VStatusChart"     JPanel
     *   "folder"       "vnmr.bo.VTabbedPane"      JTabbedPane
     *   "tabpage"      "vnmr.bo.VTabPage"         VGroup
     *   "colorchooser" "vnmr.bo.VColorChooser"    JPanel
     *   "fontchooser"  "vnmr.bo.VFontChooser"     VMenu
     *   "vsctl"        "vnmr.bo.VVsControl"       VObj
     *   "toolbarbutton" "vnmr.bo.VToolBarButton"  VButton
     *   "plot"          "vnmr.bo.VPlot"           VPlot
     *   "lcplot"        "vnmr.lc.VLcPlot"         VLcPlot
     *   "msstatus"      "vnmr.bo.VMsStatusButton" VStatusButton
     *
     *  notes: the method uses java class reflection.
     *         class names must be full path names starting at vnmr.
     *
     * III. assign the shuffler statement for save() and writeToFile() operations
     *
     *   <i>methods</i>
     *
     *   1. static void setShufflerString(String key, String value)
     *      set a key="value" substring in the Shuffler statement string.
     *      if value==null the substring will be removed from the Shuffler
     *      statement string.
     *
     *   2. static void clrShufflerString()
     *      reset shuffler string to "".
     *
     *   3. static String getShufflerString()
     *      return entire shuffler statement string.
     *
     *   4. static String getShufflerString(String key)
     *      return string value of key
     *
     *   note: Since Shuffler string operations modify static variables, changes will
     *         remain in effect until subsequent calls are made.
     *
     *  <i>example (save a composite element)</i>
     *     setShufflerString(PTYPE,PROCESSING);
     *     setShufflerString(ETYPE,COMPOSITE);
     *
     *  <i>example (save a basic element)</i>
     *     setShufflerString(PTYPE,BASIC);
     *     setShufflerString(ETYPE,ELEMENT);
     *
     *  <i>example (save a panel)</i>
     *     setShufflerString(PTYPE,ACQUISITION);
     *     setShufflerString(ETYPE,PANEL);
     *     setShufflerString(NAME,"single pulse");
     *
     *   see {@link vnmr.templates.Types Types} for a list of defined Shuffler tokens
     *
     *   <B>initially assigned shuffler token pairs</B>
     *   -------------------------------------------------
     *   PTYPE  "type"      ACQUISITION "acquistion"
     *   ETYPE  "element"   PANEL       "panel"
     * </pre>
     */
    public LayoutBuilder(JComponent pnl, ButtonIF vif)  {
        super(pnl);

        VnmrIf=vif;

        if(initialized)  // do it only once
            return;

        argtypes[0]=vnmr.ui.SessionShare.class;
        argtypes[1]=vnmr.util.ButtonIF.class;
        argtypes[2]=java.lang.String.class;

        // registered attributes
        // developers: add a line here for each xml attribute

        addAttribute("font",        Integer.valueOf(FONT_NAME));
        addAttribute("style",       Integer.valueOf(FONT_STYLE));
        addAttribute("point",       Integer.valueOf(FONT_SIZE));
        addAttribute("fg",          Integer.valueOf(FGCOLOR));
        addAttribute("label",       Integer.valueOf(LABEL));
        addAttribute("value",       Integer.valueOf(LABEL));
        addAttribute("vq",          Integer.valueOf(VARIABLE));
        addAttribute("vc",          Integer.valueOf(CMD));
        addAttribute("vc2",         Integer.valueOf(CMD2));
        addAttribute("set",         Integer.valueOf(SETVAL));
        addAttribute("show",        Integer.valueOf(SHOW));
        addAttribute("digital",     Integer.valueOf(DIGITAL));
        addAttribute("min",         Integer.valueOf(MIN));
        addAttribute("max",         Integer.valueOf(MAX));
        addAttribute("minmark",     Integer.valueOf(SHOWMIN));
        addAttribute("maxmark",     Integer.valueOf(SHOWMAX));
        addAttribute("incr1",       Integer.valueOf(INCR1));
        addAttribute("incr2",       Integer.valueOf(INCR2));
        addAttribute("chval",       Integer.valueOf(CHVAL));
        addAttribute("values",      Integer.valueOf(VALUES));
        addAttribute("panel",       Integer.valueOf(PANEL_TAB));
        addAttribute("name",        Integer.valueOf(PANEL_NAME));
        addAttribute("file",        Integer.valueOf(PANEL_FILE));
        addAttribute("param",       Integer.valueOf(PANEL_PARAM));
        addAttribute("type",        Integer.valueOf(PANEL_TYPE));
        addAttribute("actionfile",  Integer.valueOf(ACTION_FILE));
        addAttribute("bg",          Integer.valueOf(BGCOLOR));
        addAttribute("editable",    Integer.valueOf(EDITABLE));
        addAttribute("border",      Integer.valueOf(BORDER));
        addAttribute("side",        Integer.valueOf(SIDE));
        addAttribute("justify",     Integer.valueOf(JUSTIFY));
        addAttribute("tab",         Integer.valueOf(TAB));
        addAttribute("savekids",    Integer.valueOf(SAVEKIDS));
        addAttribute("vq2",         Integer.valueOf(VAR2));
        addAttribute("set2",        Integer.valueOf(SETVAL2));
        addAttribute("seperator",   Integer.valueOf(SEPERATOR));
        addAttribute("digits",      Integer.valueOf(NUMDIGIT));
        addAttribute("title",       Integer.valueOf(TITLE));
        addAttribute("statkey",     Integer.valueOf(STATKEY));
        addAttribute("statpar",     Integer.valueOf(STATPAR));
        addAttribute("statset",     Integer.valueOf(STATSET));
        addAttribute("statcol",     Integer.valueOf(STATCOL));
        addAttribute("statval",     Integer.valueOf(STATVAL));
        addAttribute("statshow",    Integer.valueOf(STATSHOW));
        addAttribute("enabled",     Integer.valueOf(ENABLED));
        addAttribute("pointy",      Integer.valueOf(POINTY));
        addAttribute("rocker",      Integer.valueOf(ROCKER));
        addAttribute("arrow",       Integer.valueOf(ARROW));
        addAttribute("arrowcolor",  Integer.valueOf(ARROW_COLOR));
        addAttribute("orientation", Integer.valueOf(ORIENTATION));
        addAttribute("checkenabled", Integer.valueOf(CHECKENABLED));
        addAttribute("checkvalue",  Integer.valueOf(CHECKVALUE));
        addAttribute("checkcmd",    Integer.valueOf(CHECKCMD));
        addAttribute("checkcmd2",   Integer.valueOf(CHECKCMD2));
        addAttribute("entryvalue",  Integer.valueOf(ENTRYVALUE));
        addAttribute("entrycmd",    Integer.valueOf(ENTRYCMD));
        addAttribute("entrysize",   Integer.valueOf(ENTRYSIZE));
        addAttribute("unitsenabled", Integer.valueOf(UNITSENABLED));
        addAttribute("unitssize",   Integer.valueOf(UNITSSIZE));
        addAttribute("unitscmd",    Integer.valueOf(UNITSCMD));
        addAttribute("unitslabel",  Integer.valueOf(UNITSLABEL));
        addAttribute("unitsvalue",  Integer.valueOf(UNITSVALUE));
        addAttribute("count",       Integer.valueOf(COUNT));
        addAttribute("color1",      Integer.valueOf(COLOR1));
        addAttribute("color2",      Integer.valueOf(COLOR2));
        addAttribute("color3",      Integer.valueOf(COLOR3));
        addAttribute("color4",      Integer.valueOf(COLOR4));
        addAttribute("color5",      Integer.valueOf(COLOR5));
        addAttribute("color6",      Integer.valueOf(COLOR6));
        addAttribute("color7",      Integer.valueOf(COLOR7));
        addAttribute("color8",      Integer.valueOf(COLOR8));
        addAttribute("color9",      Integer.valueOf(COLOR9));
        addAttribute("color10",     Integer.valueOf(COLOR10));
        addAttribute("color11",     Integer.valueOf(COLOR11));
        addAttribute("color12",     Integer.valueOf(COLOR12));
        addAttribute("color13",     Integer.valueOf(COLOR13));
        addAttribute("color14",     Integer.valueOf(COLOR14));
        addAttribute("color15",     Integer.valueOf(COLOR15));
        addAttribute("decor1",      Integer.valueOf(DECOR1));
        addAttribute("decor2",      Integer.valueOf(DECOR2));
        addAttribute("decor3",      Integer.valueOf(DECOR3));
        addAttribute("key",         Integer.valueOf(KEYSTR));
        addAttribute("keyval",      Integer.valueOf(KEYVAL));
        addAttribute("icon",        Integer.valueOf(ICON));
        addAttribute("enable",      Integer.valueOf(ENABLE));
        addAttribute("visible",     Integer.valueOf(VISIBLE));
        addAttribute("elastic",     Integer.valueOf(ELASTIC));
        addAttribute("radiobutton", Integer.valueOf(RADIOBUTTON));
        addAttribute("display",     Integer.valueOf(DISPLAY));
        addAttribute("reference",   Integer.valueOf(REFERENCE));
        addAttribute("useref",      Integer.valueOf(USEREF));
        addAttribute("hotkey",      Integer.valueOf(HOTKEY));
        addAttribute("jointpts",    Integer.valueOf(JOINPTS));
        addAttribute("fillhistgm",  Integer.valueOf(FILLHISTGM));
        addAttribute("pointsize",   Integer.valueOf(POINTSIZE));
        addAttribute("disable",     Integer.valueOf(DISABLE));
        addAttribute("lcvq",        Integer.valueOf(LCVARIABLE));
        addAttribute("lcset",       Integer.valueOf(LCSETVAL));
        addAttribute("lccmd",       Integer.valueOf(LCCMD));
        addAttribute("lccmd2",      Integer.valueOf(LCCMD2));
        addAttribute("lccmd3",      Integer.valueOf(LCCMD3));
        addAttribute("lcshow",      Integer.valueOf(LCSHOW));
        addAttribute("lccolor",     Integer.valueOf(LCCOLOR));
        addAttribute("rcvq",        Integer.valueOf(RCVARIABLE));
        addAttribute("rcset",       Integer.valueOf(RCSETVAL));
        addAttribute("rccmd",       Integer.valueOf(RCCMD));
        addAttribute("rccmd2",      Integer.valueOf(RCCMD2));
        addAttribute("rccmd3",      Integer.valueOf(RCCMD3));
        addAttribute("rcshow",      Integer.valueOf(RCSHOW));
        addAttribute("rccolor",     Integer.valueOf(RCCOLOR));
        addAttribute("graphbgcolor",Integer.valueOf(GRAPHBGCOL));
        addAttribute("graphbgcolor2",Integer.valueOf(GRAPHBGCOL2));
        addAttribute("graphfgcolor",Integer.valueOf(GRAPHFGCOL));
        addAttribute("graphfgcolor2",Integer.valueOf(GRAPHFGCOL2));
        addAttribute("graphfgcolor3",Integer.valueOf(GRAPHFGCOL3));
        addAttribute("xaxisshow",   Integer.valueOf(XAXISSHOW));
        addAttribute("yaxisshow",   Integer.valueOf(YAXISSHOW));
        addAttribute("xlabelshow",  Integer.valueOf(XLABELSHOW));
        addAttribute("ylabelshow",  Integer.valueOf(YLABELSHOW));
        addAttribute("logxaxis",    Integer.valueOf(LOGXAXIS));
        addAttribute("logyaxis",    Integer.valueOf(LOGYAXIS));
        addAttribute("showgrid",    Integer.valueOf(SHOWGRID));
        addAttribute("range",       Integer.valueOf(RANGE));
        addAttribute("gridcolor",   Integer.valueOf(GRIDCOLOR));
        addAttribute("tickcolor",   Integer.valueOf(TICKCOLOR));
        addAttribute("subtype",     Integer.valueOf(SUBTYPE));
        addAttribute("tabled",      Integer.valueOf(TABLED));
        addAttribute("keyword",     Integer.valueOf(KEYWORD));
        addAttribute("path1",       Integer.valueOf(PATH1));
        addAttribute("path2",       Integer.valueOf(PATH2));
        addAttribute("wrap",        Integer.valueOf(WRAP));
        addAttribute("expanded",    Integer.valueOf(EXPANDED));
        addAttribute("drag",        Integer.valueOf(DRAG));
        addAttribute("size1",       Integer.valueOf(SIZE1));
        addAttribute("size2",       Integer.valueOf(SIZE2));
        addAttribute("units",       Integer.valueOf(UNITS));
        addAttribute("layout",      Integer.valueOf(LAYOUT));
        addAttribute("row",         Integer.valueOf(ROW));
        addAttribute("column",      Integer.valueOf(COLUMN));
        addAttribute("prefix",      Integer.valueOf(PREFIX));

        //Additional Attributes for DefaultToolBar.xml

        addAttribute("ImageOrLabel", Integer.valueOf(IMAGEORLABEL));
        addAttribute("setVC",      Integer.valueOf(SET_VC));
        addAttribute("toolTip",    Integer.valueOf(TOOL_TIP));
        addAttribute("tooltip",    Integer.valueOf(TOOLTIP));
        addAttribute("dockat",    Integer.valueOf(DOCKAT));
        addAttribute("halignment",  Integer.valueOf(HALIGN));
        addAttribute("valignment",  Integer.valueOf(VALIGN));

        addAttribute("locx",  Integer.valueOf(LOCX));
        addAttribute("locy",  Integer.valueOf(LOCY));
        addAttribute("rows",  Integer.valueOf(ROWS));
        addAttribute("columns",  Integer.valueOf(COLUMNS));
        addAttribute("idnumber",  Integer.valueOf(OBJID));
        addAttribute("checkmark",  Integer.valueOf(CHECKMARK));
        addAttribute("checkobject",  Integer.valueOf(CHECKOBJ));
        addAttribute("checkObject",  Integer.valueOf(CHECKOBJ));
        addAttribute("showObject",  Integer.valueOf(SHOWOBJ));
        addAttribute("actionCmd",  Integer.valueOf(ACTIONCMD));
        addAttribute("trackViewport",  Integer.valueOf(TRACKVIEWPORT));
        addAttribute("helplink",  Integer.valueOf(HELPLINK));

        addAttribute("grid-column",  Integer.valueOf(GRID_COLUMN));
        addAttribute("grid-row",  Integer.valueOf(GRID_ROW));
        addAttribute("grid-column-span",  Integer.valueOf(GRID_XSPAN));
        addAttribute("grid-row-span",  Integer.valueOf(GRID_YSPAN));
        addAttribute("grid-row-align",  Integer.valueOf(GRID_ROWALIGN));
        addAttribute("stretch",  Integer.valueOf(STRETCH));
        addAttribute("squish",  Integer.valueOf(SQUISH));
        addAttribute("modalclose",  Integer.valueOf(MODALCLOSE));

        // registered objects
        // developers: add a line here for each xml object
        //
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // WARNING: these entry constructors must be included in the
        // Dash-O dop file as Trigger Methods
        // e.g.  in the dop file 'VnmrJ.dop'
        // -TriggerMethods:
        // vnmr.bo.VStyleChooser:<init>:vnmr.ui.SessionShare,vnmr.util.ButtonIF,
        //                                          java.lang.String
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //

        addVObject("group",         vnmr.bo.VGroup.class);
        addVObject("label",         vnmr.bo.VLabel.class);
        addVObject("entry",         vnmr.bo.VEntry.class);
        addVObject("menu",          vnmr.bo.VMenu.class);
        addVObject("popup",         vnmr.bo.VPopup.class);
        addVObject("radio",         vnmr.bo.VRadio.class);
        addVObject("tab",           vnmr.bo.VTab.class);
        addVObject("check",         vnmr.bo.VCheck.class);
        addVObject("button",        vnmr.bo.VButton.class);
        addVObject("text",          vnmr.bo.VText.class);
        addVObject("scroll",        vnmr.bo.VScroll.class);
        addVObject("toggle",        vnmr.bo.VToggle.class);
        addVObject("magicbutton",   vnmr.bo.VMagicButton.class);
        addVObject("dial",          vnmr.bo.VClockDial.class);
        addVObject("shimbutton",    vnmr.bo.VUpDownButton.class);
        addVObject("slider",        vnmr.bo.VSlider.class);
        addVObject("textmessage",   vnmr.bo.VTextMsg.class);
        addVObject("mlabel",        vnmr.bo.VMenuLabel.class);
        addVObject("panel",         vnmr.bo.VPanelTab.class);
        addVObject("parameter",     vnmr.bo.VParameter.class);
        addVObject("comboboxbutton", vnmr.bo.VComboBoxButton.class);
        addVObject("spinner",       vnmr.bo.VSpinner.class);
        addVObject("shimset",       vnmr.bo.VShimSet.class);
        addVObject("textfile",      vnmr.bo.VTextFileWin.class);
        addVObject("statusbutton",  vnmr.bo.VStatusButton.class);
        addVObject("lcstatusbutton",vnmr.bo.VLcStatusButton.class);
        addVObject("hwstatus",      vnmr.bo.VHardwareStatus.class);
        addVObject("expmeter",      vnmr.bo.VExperimentMeter.class);
        addVObject("trashcan",      vnmr.ui.TrashCan.class);
        addVObject("probebutton",   vnmr.ui.ProbeButton.class);
        addVObject("thermometer",   vnmr.bo.VThermometer.class);
        addVObject("chart",         vnmr.bo.VStatusChart.class);
        addVObject("folder",        vnmr.bo.VTabbedPane.class);
        addVObject("tabpage",       vnmr.bo.VTabPage.class);
        addVObject("colorchooser",  vnmr.bo.VColorChooser.class);
        addVObject("fontchooser",   vnmr.bo.VFontChooser.class);
        addVObject("stylechooser",  vnmr.bo.VStyleChooser.class);
        addVObject("messagebox",    vnmr.bo.VMessageBox.class);
        addVObject("filemenu",      vnmr.bo.VFileMenu.class);
        addVObject("filetable",     vnmr.bo.VFileTable.class);
        addVObject("filebrowser",   vnmr.bo.VFileBrowser.class);
        addVObject("combofiletable",vnmr.bo.VComboFileTable.class);
        addVObject("audit",         vnmr.bo.VAudit.class);
        addVObject("vsctl",         vnmr.bo.VVsControl.class);
        addVObject("submenu",       vnmr.bo.VSubMenu.class);
        addVObject("subfilemenu",   vnmr.bo.VSubFileMenu.class);
        addVObject("mchoice",       vnmr.bo.VSubMenuItem.class);
        addVObject("checkboxchoice",vnmr.bo.VCheckBoxMenuItem.class);
        addVObject("toolBarButton", vnmr.bo.VToolBarButton.class);
        addVObject("centry",        vnmr.bo.VCaretEntry.class);
        addVObject("selmenu",       vnmr.bo.VSelMenu.class);
        addVObject("selfilemenu",   vnmr.bo.VSelFileMenu.class);
        addVObject("plot",          vnmr.bo.VPlot.class);
        // CORBA has been remove after java 1.8
        if ( System.getProperty("java.version").startsWith("1.") )
        {
           addVObject("lcplot",        vnmr.lc.VLcPlot.class);
           addVObject("msplot",        vnmr.lc.VMsPlot.class);
           addVObject("pdaplot",       vnmr.lc.VPdaPlot.class);
           addVObject("pdaimage",      vnmr.lc.VPdaImage.class);
           addVObject("lcTableItem",   vnmr.lc.VLcTableItem.class);
        }
        addVObject("undobutton",    vnmr.bo.VUndoButton.class);
        addVObject("redobutton",    vnmr.bo.VRedoButton.class);
        addVObject("rfmonbutton",   vnmr.bo.VRFMonButton.class);
        addVObject("msstatus",      vnmr.bo.VMsStatusButton.class);
        addVObject("capturebutton", vnmr.bo.VCaptureButton.class);
        addVObject("structuredfile", vnmr.bo.VTextFilePane.class);
        addVObject("splitpane",     vnmr.bo.VSplitPane.class);
        addVObject("cascadingMenu", vnmr.bo.VCascadingMenu.class);
        addVObject("experimentSubmenu", vnmr.bo.VExperimentSubMenu.class);

        // SpinCAD & Jpsg related
        addVObject("paramcreate",   vnmr.bo.VParamCreateEntry.class);
        addVObject("makepanelstag", vnmr.bo.VMakePanelsTag.class);

        // Wanda Interface classes
        addVObject("wchoice",       vnmr.admin.vobj.WSubMenuItem.class);
        addVObject("wbutton",       vnmr.admin.vobj.WButton.class);
        addVObject("wcheck",        vnmr.admin.vobj.WCheck.class);
        addVObject("wentry",        vnmr.admin.vobj.WEntry.class);
        addVObject("wlabel",        vnmr.admin.vobj.WLabel.class);
        addVObject("wmenu",         vnmr.admin.vobj.WMenu.class);
        addVObject("wradio",        vnmr.admin.vobj.WRadio.class);
        addVObject("wtoggle",       vnmr.admin.vobj.WToggle.class);
        addVObject("wgroup",        vnmr.admin.vobj.WGroup.class);
        addVObject("wcolorchooser", vnmr.admin.vobj.WColorChooser.class);

        addVObject("textline",      vnmr.bo.VLabel.class);
        addVObject("annotatebox",   vnmr.bo.VAnnotateBox.class);
        addVObject("annotatetable", vnmr.bo.VAnnotateTable.class);
        addVObject("traycolorchooser", vnmr.bo.VTrayColor.class);
        addVObject("colormap", vnmr.bo.VColorMapPanel.class);
        addVObject("imagelist", vnmr.bo.VColorImageSelector.class);
        addVObject("grid", vnmr.bo.VGrid.class);

        // initialze shuffler string tokens

        setShufflerString(PTYPE,ACQUISITION);
        setShufflerString(ETYPE,PANEL);

       initialized=true;
    }

    //----------------------------------------------------------------
    /** force a stack trace. */
    //----------------------------------------------------------------
    public static void stackTrace() {
        Vector bb=null;
        try{
             bb.add("");
        }
        catch (NullPointerException e){
             e.printStackTrace();
        }
    }

    //----------------------------------------------------------------
    /** Create sax parser. */
    //----------------------------------------------------------------
    private XMLReader makeSaxParser() {
        try {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            factory.setNamespaceAware(false);
            return factory.newSAXParser().getXMLReader();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e,"Could not make XML parser");
        }
        return null;
    }
    //----------------------------------------------------------------
    /** Register an object String. */
    //----------------------------------------------------------------
    private void addVObject(String s, Class vs) {
        try{
            Constructor vc= vs.getConstructor(argtypes);
            vobjs.put(s,vc);
        }
        catch (Exception e){
        /* The possible exceptions are:
         *     newInstance():
         *     InstantiationException
         *     IllegalAccessException
         *     IllegalArgumentException
         *     InvocationTargetException
         */
            Messages.postError(new StringBuffer().append(s).append(" ").append(s).
                               append(" ").append(e.toString()).toString());
        }
    }

    //----------------------------------------------------------------
    /** Factory method used to instantiate VObjIF objects. */
    //----------------------------------------------------------------
    protected VObjIF getVObj(String type) {
        if (sshare == null) {
            if (apIf == null) {
                 apIf = Util.getAppIF();
            }
            if (apIf != null) {
                 sshare = apIf.getSessionShare();
            }
        }
        // vargs[0] = Util.getAppIF().getSessionShare();
        vargs[0] = sshare;
        vargs[1] = VnmrIf;
        vargs[2] = type;

        Constructor vc=(Constructor)vobjs.get(type);
        if(vc!=null){
            try {
                Object obj=vc.newInstance(vargs);
                if(obj instanceof VObjIF)
                    return (VObjIF)obj;
            } catch (Exception e) {
           /* The possible exceptions are:
            *     newInstance():
            *     InstantiationException
            *     IllegalAccessException
            *     IllegalArgumentException
            *     InvocationTargetException
            */
                Messages.postError(new StringBuffer().append(type).append(" ").
                                     append(e.toString()).toString());
                Messages.writeStackTrace(e);
            }
        }
        return null;
    }

    //----------------------------------------------------------------
    /** Build the swing panel. */
    //----------------------------------------------------------------
    public void build(){
        if(testdoc(true))
            return;
        last_group=panel;
        build(rootElement());
    }

    //----------------------------------------------------------------
    /** Recursive call to build JComponents in Swing panel. */
    //----------------------------------------------------------------
    protected void build(VElement elem){
        Enumeration     elems=elem.children();
        JComponent      new_group=null;
        JComponent      jobj=null;

        if(elem instanceof GElement)
            jobj=((GElement)elem).jcomp();

        if(jobj!=null && (jobj instanceof VGroupIF || jobj instanceof VMenu)){
            new_group=last_group;
            last_group=jobj;
        }

        while(elems.hasMoreElements()){
            build((VElement)elems.nextElement());
        }

        if(jobj!=null){
            if(new_group != null)
                last_group=new_group;
            if(last_group !=null)
                last_group.add(jobj);
        }
    }

    //----------------------------------------------------------------
    /** build panel using SAX parser. */
    //----------------------------------------------------------------
    public JComponent SAXparse(JComponent pnl,String fn)
        throws Exception
    {
        SAXparser sb=new SAXparser(this);
        return sb.parse(pnl,fn);
    }

    //----------------------------------------------------------------
    /** build item using SAX parser. */
    //----------------------------------------------------------------
    public JComponent SAXparse(JComponent pnl, String fn, int x, int y)
        throws Exception
    {
        SAXparser sb=new SAXparser(this);
        return sb.parse(pnl,fn,x,y);
    }

    //++++++++++++++ static builder methods +++++++++++++++++++++++++++++

    public static void dumpObject(JComponent comp,int indent){
        int i;
        if(!(comp instanceof JComponent)){
            System.out.println(new StringBuffer().append("unknown component type: ").
                                 append(comp.toString()).toString());
        }
        if (comp instanceof VObjIF) {
            VObjIF obj = (VObjIF) comp;
            StringBuffer sbData=new StringBuffer();
            for (i = 0; i < indent; i++)
                sbData.append("  ");
            Point loc=comp.getLocation();
            System.out.println(sbData.append(obj.getAttribute(TYPE)).
                                 append("[").append(loc.x).append(",").append(loc.y).
                                 append("]").append(obj.getAttribute(LABEL)).
                                 append(")").toString());
        }
        int nSize = comp.getComponentCount();
        for (i = 0; i < nSize; i++) {
            dumpObject((JComponent)comp.getComponent(i),indent+1);
        }
    }

    //----------------------------------------------------------------
    /**<pre>Build a JComponent panel by parsing an XML file.
     *  - used to build ParamLayout type panels
     *  - This method creates a temporary LayoutBuilder stack object
     *  - if static public boolean useSAX == true
     *       the panel is build in one pass using a SAX parser
     *  - if static public boolean useSAX == false
     *       a DOM parser is used to generate an XMLDocument and element tree.
     *       The tree is then traversed to build a corresponding VObjIF JComponent
     *       tree which is added as a child node to the JComponent pnl argument.
     *       When the function returns the local LayoutBuilder XMLDocument
     *       goes out of scope and will be reaped by the Java garbage collector.
     *       The XML element tree is destroyed but the JComponent tree survives.
     * </pre>
     */
    //----------------------------------------------------------------
    static public synchronized JComponent build(JComponent pnl, ButtonIF vif, String fn)
        throws Exception
    {
        return build(pnl,vif,fn,false);
    }

    static public synchronized JComponent build(JComponent pnl, ButtonIF vif, String fn, boolean exp)
        throws Exception
    {
        if (fn == null || fn.length() < 1)
            return pnl;

	m_resource = getLabelResource(fn);
        LayoutBuilder builder=new LayoutBuilder(pnl,vif);
        builder.expand_tabs=exp;
        builder.start(fn);
        if(useSAX)
            builder.SAXparse(pnl,fn);
        else
            builder.open(fn);
        builder.finish();
        return pnl;
    }

    //----------------------------------------------------------------
    /** <pre>Build and attach a JComponent sub-tree.
     *  - used to create JComponent sub-trees from XML files
     *    Applications include drag and drop of component XML file references
     *    from the shuffler window to the ParamLayout panel in edit mode.
     *  - This method creates a VObjIF JComponent tree from an XML file as
     *    in the above static build method.
     *    The JComponent subtree is positioned at x and y with respect to the
     *    grp object's location.
     * </pre>
     */
    //----------------------------------------------------------------
    static public synchronized JComponent build(JComponent grp, ButtonIF vif,
                String fn, int x, int y)
    throws Exception
    {
        if (fn == null || fn.length() < 1)
            return null;
	m_resource = getLabelResource(fn);
        LayoutBuilder builder=new LayoutBuilder(grp,vif);
        builder.start(fn);
        JComponent child=null;
        if(useSAX)
            child=builder.SAXparse(grp,fn,x,y);
        else{
            builder.open(fn);
            VElement template=builder.rootElement();
            Enumeration elems=template.children();
            while(elems.hasMoreElements()){
                VElement elem=(VElement)elems.nextElement();
                if(elem instanceof VObjElement){
                    child=((VObjElement)elem).jcomp();
                    child.setLocation(child.getX()+x,child.getY()+y);
                    ((VObjIF)elem).setDefLoc(child.getX()+x,child.getY()+y);
                    grp.add(child);
                }
            }
        }
        builder.finish();
        return child;
    }

    public static VLabelResource getLabelResource(String fn) {
	xml_locale = true;
	if(fn == null) return null;
	VLabelResource resource = null;

	String language = FileUtil.getLanguage();
	// to minimize over head for "en", return here.
	// if(language.equals("en")) return null;
	if(fn.endsWith("."+language+".xml")) return null;
	if(fn.endsWith("."+language+"_"+FileUtil.getCountry()+".xml")) return null;

	xml_locale = false;
	String path = fn;
	if(fn.endsWith(".xml")) path = fn.substring(0, fn.lastIndexOf(".xml")); 

	resource = new VLabelResource(path);
	if(resource.getLabelResource() == null) resource = null;
	return resource;
    }

    private String getLabelString(String str) {
	if(xml_locale) return str;
	else if(m_resource == null) return str = Util.getLabelString(str);
	else return m_resource.getString(str);
    }

    private String getTooltipString(String str) {
	if(xml_locale) return str;
	else if(m_resource == null) return str = Util.getTooltipString(str);
	else return m_resource.getString(str);
    }

    //----------------------------------------------------------------
    /** parse an XML file (start function)  */
    //----------------------------------------------------------------
    private void start(String f) {
        if (Util.iswindows())
            f = UtilB.unixPathToWindows(f);
        File file=new File( f );
        if (!file.exists()) {
            if(DebugOutput.isSetFor("traceXML"))
                Messages.postError(new StringBuffer().append("building : " ).append( f).append(" does not exist").toString());
            return;
        }
        TMname=file.getName();
        if(DebugOutput.isSetFor("traceXML"))
            Messages.postDebug(new StringBuffer().append("building : " ).append( f).toString());

        if(DebugOutput.isSetFor("timeXML"))
            TMstart=System.currentTimeMillis();
    }

   //----------------------------------------------------------------
    /** parse an XML file (finish function)  */
    //----------------------------------------------------------------
    private void finish() {
        if(DebugOutput.isSetFor("timeXML")){
            TMend=System.currentTimeMillis();
            double dt1=TMend-TMstart;
            Messages.postDebug(new StringBuffer().append("XML build time: ").
                               append(dt1).append("\t(").append(TMname).append(")").toString());

        }
    }

    //----------------------------------------------------------------
    /** <pre>archive a JComponent tree.
     *  - used to save a JComponent tree in XML format.
     *    Applications include saving edited Parameter panels.
     *  - Output is generated only by JComponent nodes that implement the
     *    VObjIF interface.
     * </pre>
     */
    //----------------------------------------------------------------
    static public void writeToFile(JComponent pnl, String fn)
    {
        isPanel = false;
        writeLayout(pnl,fn);
    }

    static public void writeToFile(JComponent pnl, String fn, boolean panelFlag)
    {
        isPanel = panelFlag;
        writeLayout(pnl,fn);
    }

    //----------------------------------------------------------------
    /** Return an Enumeration of attribute keys. */
    //----------------------------------------------------------------
    public static Enumeration keys() {
        return vattrs.keys();
    }

    //----------------------------------------------------------------
    /** Register an attribute String. */
    //----------------------------------------------------------------
    public static void addAttribute(String s, Object o) {
        vattrs.put(s,o);
    }

    //----------------------------------------------------------------
    /** get an attribute Integer value from a key. */
    //----------------------------------------------------------------
    public static int getAttribute(String key) {
        Integer val=(Integer)vattrs.get(key);
        if(val == null)
            return 0;
        else
            return val.intValue();
    }

    //----------------------------------------------------------------
    /** Determine if a given group wants its children saved
     *  when it is written out to an XML file.
     *  Returns true unless the group returns the SAVEKIDS
     *  attribute and it is set to "false". */
    //----------------------------------------------------------------
    public static boolean getSaveKidsAttr(VObjIF obj) {
        String rtn = obj.getAttribute(SAVEKIDS);
        if (rtn == null || !rtn.equals("false"))
        return true;
        else
        return false;
    }

    //----------------------------------------------------------------
    /** Initialize the shuffler token list. */
    //----------------------------------------------------------------
    public static void clrShufflerString() {
        shuffler_tokens.clear();
    }

    //----------------------------------------------------------------
    /** set shuffler token list from a vector. */
    //----------------------------------------------------------------
    public static void setShufflerString(Vector v) {
        clrShufflerString();
        Enumeration e=v.elements();
        while(e.hasMoreElements()){
            String name=(String)e.nextElement();
            String value=(String)e.nextElement();
            setShufflerString(name,value);
        }
    }

    //----------------------------------------------------------------
    /** Add a token pair to the shuffler token list. */
    //----------------------------------------------------------------
    public static void setShufflerString(String s, String v) {
        if(v==null && shuffler_tokens.containsKey(s))
            shuffler_tokens.remove(s);
        else
            shuffler_tokens.put(s,v);
    }

    //----------------------------------------------------------------
    /** Return shuffler attribute string. */
    //----------------------------------------------------------------
    public static String getShufflerString()    {
        String str="";

        Enumeration keys=shuffler_tokens.keys();
        while(keys.hasMoreElements()){
            String key=(String)keys.nextElement();
            String val=(String)shuffler_tokens.get(key);
            str+=key+"=\""+val+"\" ";
        }
        return str;
    }

    //----------------------------------------------------------------
    /** Return shuffler attribute value. */
    //----------------------------------------------------------------
    public static String getShufflerString(String s)    {
        return (String)shuffler_tokens.get(s);
    }

    //+++++++++++++ parser methods +++++++++++++++++++++++++++++++++++

    //----------------------------------------------------------------
    /** Set default class bindings for xml keys.
     * <pre>
     *  builder classes extend this method to specify element classes
     *  <B>this (extended) method sets the following bindings</B>
     *   -------------------------------------------------
     *   key        classname
     *   -------------------------------------------------
     *   *Element   vnmr.templates.VObjElement
     * </pre>
     */
    //----------------------------------------------------------------
    protected void setDefaultKeys(){
        super.setDefaultKeys();
        setKey("*Element",  vnmr.templates.VObjElement.class);
    }

    //----------------------------------------------------------------
    /** Set shuffler attributes in root element (DOM based method). */
    //----------------------------------------------------------------
    public void setShufflerElement()    {
        if(noDocument())
            return;
        Element root=doc.getDocumentElement();
        Enumeration keys=shuffler_tokens.keys();
        removeAttributes(root);
        while(keys.hasMoreElements()){
            String key=(String)keys.nextElement();
            String val=(String)shuffler_tokens.get(key);
            root.setAttribute(key,val);
        }
    }

    //----------------------------------------------------------------
    /** save overide (DOM based method). */
    //----------------------------------------------------------------
    public void save(String name)   {
        setShufflerElement();
        super.save(name);
    }

    //----------------------------------------------------------------
    /** Make a new VObj element node. */
    //----------------------------------------------------------------
    public VObjElement newVElement(String s)    {
        VObjElement elem=(VObjElement)newElement(s);
        elem.build();
        return elem;
    }

    //----------------------------------------------------------------
    /** Get element corresponding to VObjIF. */
    //----------------------------------------------------------------
    public VObjElement getElement(JComponent e){
        ElementTree  tree=getTree();
        VElement     elem=tree.rootElement();

        while(elem !=null){
            if(elem instanceof VObjElement){
                if(((VObjElement)elem).vcomp()==e)
                    return (VObjElement)elem;
            }
            elem=tree.nextElement();
        }
        return null;
    }

    //++++++++++++++ private methods ++++++++++++++++++++++++++++++++++

    public static boolean useReference(VObjIF obj) {
        String att=obj.getAttribute(USEREF);
        if(att !=null && att.equals("yes"))
            return true;
        return false;
    }

    public static boolean isReference(VObjIF obj) {
        if(obj instanceof VGroup){
            String att=obj.getAttribute(REFERENCE);
            if(att !=null && att.length()>0){
                return true;
            }
        }
        return false;
    }

    public static boolean isTabGroup(VObjIF obj) {
        if(obj instanceof VGroup){
             String att=obj.getAttribute(TAB);
             if(att !=null && att.equals("yes"))
                 return true;
        }
        return false;
    }

    public static boolean isExpanded(VObjIF obj){
        if(obj instanceof VGroup){
             String att=obj.getAttribute(EXPANDED);
             if(att !=null && att.equals("yes"))
                 return true;
        }
        return false;
    }

    public static boolean writeExpandedRef(VObjIF obj,int lvl){
        if(!isReference(obj))
            return true;

        // verify that reference exists

        String file=obj.getAttribute(REFERENCE);
        String path=null;
        String fname=new StringBuffer().append(file).append(".xml").toString();

        String dir=null;
        if(lvl==1){
            ParamPanel layout=ParamEditUtil.getEditPanel();
            if(layout !=null){
                dir=layout.getLayoutName();
                if(FileUtil.findReference(dir,fname))
                    return useReference(obj) ? false:true;
            }
        }
        if(path==null)
            path=FileUtil.openPath(new StringBuffer().append("PANELITEMS/").
                                   append(fname).toString());
        if(path==null){
            obj.setAttribute(USEREF,"no");
            return true;
        }
        return useReference(obj) ? false:true;
    }
    //----------------------------------------------------------------
    /** save JComponent tree as an XML file (static builder call). */
    //----------------------------------------------------------------
    private static void writeLayout(JComponent pan, String fname) {
        PrintWriter os;
        if (fname == null || fname.length() <= 0)
            return;

        if (Util.iswindows())
            fname = UtilB.unixPathToWindows(fname);
        
        File f = new File(fname);
        if (!f.exists()) {
            String dir = f.getParent();
            File p = new File(dir);
            p.mkdirs();
        }
        os = null;
        try {
            os = new PrintWriter(
            new OutputStreamWriter(
            new FileOutputStream(fname), "UTF-8"));
        }
        catch(IOException er) { }
        if (os == null) {
            Messages.postError(new StringBuffer().append("error writing: ").
                                append(fname).toString());
            return;
        }
        if (isPanel)
            firstItem= false;
        else
            firstItem=true;
        if(DebugOutput.isSetFor("traceXML"))
            Messages.postDebug(new StringBuffer().append("writing: ").append(fname).toString());

        os.println("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
        os.println("");
        os.println(new StringBuffer().append("<template ").append(getShufflerString()).
                   append(">").toString());

        if (pan instanceof ParamPanel) {
            int num = pan.getComponentCount();
            for (int i = 0; i < num; i++) {
                Component comp = pan.getComponent(i);
                if (comp instanceof VObjIF) {
                    VObjIF grp = (VObjIF) comp;
                    if ((grp instanceof VGroupSave) && getSaveKidsAttr(grp))
                        groupWrite(os, grp, 1);
                    else
                        compWrite(os, grp, 1);
                }
            }
        }
        else if (pan instanceof VObjIF) {
            VObjIF obj = (VObjIF) pan;
            if ((obj instanceof VGroupSave) && getSaveKidsAttr(obj))
                groupWrite(os, obj, 1);
            else
                compWrite(os, obj, 1);
        }
        else {
            int num = pan.getComponentCount();
            for (int i = 0; i < num; i++) {
                Component comp = pan.getComponent(i);
                if (comp instanceof VObjIF) {
                    VObjIF obj = (VObjIF) comp;
                    if ((obj instanceof VGroupSave) && getSaveKidsAttr(obj))
                        groupWrite(os, obj, 1);
                    else
                        compWrite(os, obj, 1);
                }
            }
        }

        os.println("</template>");
        os.close();
        VObjElement.save_position=true;
    }

    private static void newLine(PrintWriter fd,  int level) {
        fd.println("");
        for (int k = 0; k < level * 2; k++)
            fd.print(" ");
        fd.print("  ");
    }

    public static void attrsWrite(PrintWriter fd, VObjIF obj, int level) {
        int k;
        Rectangle rect;
        String d;
        int acnt=0;
        JComponent comp = (JComponent) obj;
        boolean expand=false;

        rect = comp.getBounds();
        if(rect.width > 0 && rect.height > 0){
            if(!firstItem && (rect.x>=0 || rect.y>=0)){
                fd.print(new StringBuffer().append("loc=\"").append(rect.x).
                           append(" ").append(rect.y).append("\"").toString());
                acnt++;
            }
            if(acnt>0)
                fd.print(" ");
            fd.print(new StringBuffer().append("size=\"").append(rect.width).
                       append(" ").append(rect.height).append("\"").toString());
            acnt++;
        }

        firstItem=false;

        expand=writeExpandedRef(obj,level);
        if(isReference(obj)){
            if(expand)
                obj.setAttribute(EXPANDED,"yes");
            else
                obj.setAttribute(EXPANDED,"no");
        }
        int nLength = vattrs.size();
        for(int i=0;i<nLength;i++){
            String key=(String)vattrs.getKey(i);
            int val=getAttribute(key);
            char  ch;
            if (key.equals("value"))  // use "label" attribute to set LABEL
                 continue;
            d=obj.getAttribute(val);
            if (d != null && d.length()!=0){

                // save only "style" attribute for DisplayOptions style (FONT_STYLE)

                if (val==FONT_SIZE || val==FONT_NAME || val==FGCOLOR)
                    continue;
                newLine(fd, level);

/*
                if(acnt>0 && acnt<3)
                    fd.print(" ");
*/
                fd.print(new StringBuffer().append(key).append("=\"").toString());

                // replace those xml predefined characters
                int nSize = d.length();
                for (k = 0; k < nSize; k++) {
                   ch = d.charAt(k);
                   switch (ch) {
                     case '<':
                      fd.print("&lt;");
                      break;
                     case '>':
                      fd.print("&gt;");
                      break;
                     case '&':
                      fd.print("&amp;");
                      break;
                     case '"':
                      fd.print("&quot;");
                      break;
/*
                     case '\'':
                      fd.print("&apos;");
                      break;
*/
                     default:
                      fd.print(ch);
                      break;
                   }
                }
                fd.print("\"");
/*
                fd.print(key+"=\""+d+"\"");
*/
/*
                acnt++;
                if(acnt>2)
                    newLine(fd, level);
*/
            }
        }
        newLine(fd, level);
    }

    private static void compWrite(PrintWriter fd, VObjIF obj, int level) {
        int k;
        String d;
        if (obj == null)
            return;
        JComponent c = (JComponent) obj;
        Rectangle r = c.getBounds();
        if (r.x < 0 || r.y < 0) {
                return;
        }
        if (r.width == 2 && r.height == 2) {
                return;
        }
        for (k = 0; k < level * 2; k++)
           fd.print(" ");
        d = obj.getAttribute(TYPE);
        fd.print(new StringBuffer().append("<").append(d).append(" ").toString());
        attrsWrite(fd, obj, level);
        fd.println("/>");
    }

    private static void groupWrite(PrintWriter fd, VObjIF obj, int level) {
        int k;
        JComponent p = (JComponent) obj;
        Rectangle r = p.getBounds();
        if (r.x < 0 || r.y < 0)
                return;
        if (r.width == 2 && r.height == 2)
                return;
        for (k = 0; k < level * 2; k++)
           fd.print(" ");
        fd.print(new StringBuffer().append("<").append(obj.getAttribute(TYPE)).
                   append(" ").toString());
        attrsWrite(fd, obj, level);
        fd.println(">");

        boolean expand=writeExpandedRef(obj,level);
        if(expand){
            if (obj instanceof VMenu) {
                ArrayList list = ((VMenu) obj).getCompList();
                if (list != null) {
                    for (k = 0; k < list.size(); k++) {
                        Component c = (Component) list.get(k);
                        if (c != null && (c instanceof VObjIF)) {
                             VObjIF vobj = (VObjIF) c;
                             compWrite(fd, vobj, level+1);
                        }
                    }
                }
            }
            else {
               k = p.getComponentCount();
               for (int i = 0; i < k; i++) {
                   Component comp = p.getComponent(i);
                   if (comp instanceof VObjIF) {
                       VObjIF child = (VObjIF) comp;
                       if ((child instanceof VGroupSave) && getSaveKidsAttr(child))
                           groupWrite(fd, child, level+1);
                       else
                           compWrite(fd, child, level+1);
                  }
              }
            }
        }
        for (k = 0; k < level * 2; k++)
            fd.print(" ");
        fd.println(new StringBuffer().append("</").append(obj.getAttribute(TYPE)).
                   append(">").toString());

    }

    //+++++++++++++ local classes +++++++++++++++++++++++++++++++++++

    public class SAXparser extends DefaultHandler implements VObjDef
    {
        private  Container curParent;
        private  Container preParent;
        private  LayoutBuilder lb=null;
        private  XMLReader parser;
        private  boolean badObj;

        public SAXparser(LayoutBuilder l){
            lb=l;
            parser=makeSaxParser();
            parser.setContentHandler( SAXparser.this );
        }

        //----------------------------------------------------------------
        /** parse an XML file (panel) */
        //----------------------------------------------------------------
        public synchronized JComponent parse(JComponent p, String f)
            throws Exception
        {
            return parse(p, f, null);
        }

        public synchronized JComponent parse(JComponent p, String f,
                                             JComponent comp) throws Exception
        {
            if(f==null) {
                Messages.postError("LayoutBuilder.parse file=null");
                return p;
            }
            if (p == null)
                p = new JPanel();
                parseXML(p, f, comp);
                return p;
        }

        //----------------------------------------------------------------
        /** parse an XML file (component) */
        //----------------------------------------------------------------
        public synchronized JComponent parse(JComponent parent, String f, int x, int y)
            throws Exception
        {
            if(f==null) {
                Messages.postError("LayoutBuilder.parse() file=null");
                return null;
            }
            if (parent == null) {
                Messages.postError("LayoutBuilder.parse() parent=null");
                return null;
            }
            parseXML(parent, f);
            return firstChild;
        }

        //----------------------------------------------------------------
        /** parse an XML file (start function)  */
        //----------------------------------------------------------------
        private void parseXML(JComponent p, String f)
            throws Exception
        {
            parseXML(p, f, null);
        }

        private void parseXML(JComponent p, String f, JComponent comp) throws Exception
        {
            firstChild= comp;
            curParent = (Container) p;
            preParent = (Container) p;
            InputSource     input;
            if (Util.iswindows())
                f = UtilB.unixPathToWindows(f);
            InputStream stream = null;
            try {
                stream = new BufferedInputStream(new FileInputStream(f));
            }
            catch (FileNotFoundException er) {
                return;
            }
            
            stream=FileUtil.skipUtf8Bom(stream);
            input = new InputSource(stream);
            input.setSystemId(new File(f).toURI().toString());
            //input = Resolver.createInputSource( new File( f ));
            parser.parse(input);
            try {
                // parser.parse seems to close the input stream but ...
                stream.close(); // Make sure stream is closed
            } catch (IOException ioe) {
            }
        }

        private boolean readExpandedRef(Attributes attrs){
            String attr;
            attr=attrs.getValue("reference");
            if(attr == null || attr.length()==0)
                return false;
            attr=attrs.getValue("expanded");
            if(attr == null || attr.length()==0)
                return false;
            if(attr != null && attr.equals("yes"))
                return false;
            attr=attrs.getValue("useref");
            if(attr == null)
                return false;
            if(attr.equals("no"))
                return true;
            attr=attrs.getValue("label");
            if(attr == null || attr.length()==0)
                return true;
            if(expand_tabs)
                return true;
            attr=attrs.getValue("tab");
            if(attr == null || attr.length()==0 || attr.equals("no"))
                return true;
            return false;
        }

        //===========================================================
        //          SAX DocumentHandler methods
        //===========================================================
        public void startElement( String uri, String localName,
                                  String name, Attributes attrs )
        {
        StringTokenizer st;
            boolean  font;
            VObjIF vobj = lb.getVObj(name);
            if (vobj == null)
                return;
            boolean expand=false;
            boolean bFg;
            String file=null;
            String path=null;
            badObj = false;

            if(vobj instanceof VGroup)
                expand=readExpandedRef(attrs);
            if(expand){
                file=attrs.getValue("reference");
                file=FileUtil.fileName(file); // strip off extensions

                path=FileUtil.openPath(new StringBuffer().append("PANELITEMS/").
                                       append(file).append(".xml").toString());

                if(DebugOutput.isSetFor("traceXML"))
                    Messages.postDebug(new StringBuffer().append("LayoutBuilder expanding ").
                                       append(file).toString());
                if (path == null) {
                   if(DebugOutput.isSetFor("traceXML"))
                        Messages.postError(new StringBuffer().append("error expanding reference ").append(file).toString());
                   return;
                }
                try {
                    SAXparser sp=new SAXparser(lb);
                    //JComponent cp=sp.parse(null,path);
                    JComponent cp = sp.parse(null, path, firstChild);
                    cp=(JComponent)cp.getComponent(0);
                    VObjIF robj=(VObjIF)cp;
                    robj.setModalMode(true);
                    robj.setAttribute(REFERENCE,vobj.getAttribute(REFERENCE));
                    robj.setAttribute(USEREF,vobj.getAttribute(USEREF));
                    if(vobj.getAttribute(EXPANDED) !=null){
                        robj.setAttribute(TAB,vobj.getAttribute(TAB));
                        robj.setAttribute(VARIABLE,vobj.getAttribute(VARIABLE));
                        robj.setAttribute(SHOW,vobj.getAttribute(SHOW));
			String s = getLabelString(vobj.getAttribute(LABEL));
                        robj.setAttribute(LABEL,s);
                        //robj.setAttribute(LABEL,vobj.getAttribute(LABEL));
                        robj.setAttribute(CMD,vobj.getAttribute(CMD));
                        robj.setAttribute(CMD2,vobj.getAttribute(CMD2));
                    }
                    robj.setAttribute(EXPANDED,"yes");
                    robj.setModalMode(false);
                    vobj.destroy();
                    vobj=robj;
                }
                catch (Exception e){
                   Messages.postError(new StringBuffer().append("error expanding reference ").
                                      append(file).toString());
                }
            }
            JComponent c = (JComponent) vobj;
            if(firstChild==null)
                firstChild=c;
            font = false;
            bFg = false;
            if( attrs != null ) {
                int nSize = attrs.getLength();
                for( int i = 0; i < nSize; i++ )  {
                    String s = attrs.getQName(i);
                    String s2 = attrs.getValue(i);
                    if (s!=null && s.equals("size")) {
                        st = new StringTokenizer(s2," ");
                        if(st.countTokens() == 2){
                            int w=Integer.parseInt(st.nextToken());
                            int h=Integer.parseInt(st.nextToken());
                            if (w < 0 || h < 0) {
                                badObj = true;
                                w = 2;
                                h = 2;
                            }
                            else {
                                if (w<mindim)
                                w=mindim;
                                if(h<mindim)
                                    h=mindim;
                            }
                            c.setSize(w, h);
                            c.setPreferredSize(new Dimension(w,h));
                        }
                        continue;
                    }
                    if (s.equals("loc")) {
                        st = new StringTokenizer(s2," ");
                        if(st.countTokens() == 2){
                            int x=Integer.parseInt(st.nextToken());
                            int y=Integer.parseInt(st.nextToken());
                            if(x < 0 || y < 0) {
                                x = -1; y = -1;
                                badObj = true;
                            }
                            c.setLocation(x, y);
                            vobj.setDefLoc(x, y);
                        }
                        continue;
                    }
                    int k = lb.getAttribute(s);
                    if(s2 != null) {
                        if (k == FONT_NAME || k==FONT_SIZE || k == FONT_STYLE)
                            font = true;
                        if(name.equals("panel") && k == PANEL_NAME) {
                            s2 = getLabelString(s2);
                        }
                        if(k == LABEL || k == TITLE) {
                            s2 = getLabelString(s2);
                        }
                        else if(k == TOOL_TIP || k == TOOLTIP) {
                            s2 = getTooltipString(s2);
                        }
                        else if(k == HELPLINK) {
                            s2 = getLabelString(s2);
                        }
                        if (k == FGCOLOR || k == BGCOLOR) {
                            if (s2 != null) {
                                s2 = s2.trim();
                                if (s2.length() < 1)
                                   s2 = null;
                                if (s2 != null) {
                                    vobj.setAttribute(k, s2);
                                    if (k == FGCOLOR)
                                        bFg = true;
                                }
                            }
                        }
                        else {
                            vobj.setAttribute(k, s2);
                        }
                        if(k==FONT_STYLE && !bFg) {
                            if (s2 != null) {
                               if (s2.equals("Plain"))
                                   s2 = "PlainText";
                               vobj.setAttribute(FGCOLOR, s2);
                            }
                        }
                    }
                } // for
            }

/*
            Point loc=c.getLocation();
            c.setLocation(loc.x,loc.y);
*/
            if(vobj instanceof VGroup){
                String show=vobj.getAttribute(SHOW);
                if(show !=null)
                    ((JComponent)vobj).setVisible(false);
            }
            if (font)
                vobj.changeFont();
            curParent.add(c);
            if (vobj instanceof VMenuitemIF) {
                VMenuitemIF v = (VMenuitemIF) vobj;
                v.setVParent(curParent);
            }
            if (vobj instanceof Container) {
                preParent = curParent;
                curParent = (Container) vobj;
            }
            if (badObj) {
                c.setVisible(false);
                vobj.setAttribute(SHOW, null);
            }
        }

        public void endElement( String uri, String localName, String name ) throws SAXException {
            if (curParent instanceof VMenuitemIF) {
                VMenuitemIF v = (VMenuitemIF) curParent;
                curParent = v.getVParent();
                return;
            }
            curParent = curParent.getParent();
            if (curParent == null)
                curParent = preParent;
            }
        } // SAXparser
} // Layoutbuilder






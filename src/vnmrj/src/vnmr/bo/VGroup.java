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
import java.util.*;
import java.util.List;
import java.beans.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.io.*;

import javax.swing.*;

import vnmr.util.*;
import vnmr.ui.*;

import javax.swing.border.*;


public class VGroup extends JPanel
  implements VObjIF, VEditIF, VObjDef, VGroupIF, ExpListenerIF,
   DropTargetListener, StatusListenerIF, PropertyChangeListener {
    protected static String m_no=Util.getLabel("mlNo");
    protected static String m_yes=Util.getLabel("mlYes");
    protected static String m_true=Util.getLabel("mlTrue");
    protected static String m_false=Util.getLabel("mlFalse");
    protected static String m_titled="Titled";
    protected static String m_untitled="Untitled";
    protected static String m_minor="Minor";
    protected static String m_major="Major";

    public String reference = null;
    public String type = null;
    public String title = null;
    public String fg = null;
    public String vnmrVar = null;
    public String vnmrCmd = null;
    public String vnmrCmd2 = null;
    public String showVal = null;
    protected String strSubtype = m_untitled;
    protected String strPanels = null;
    protected String strPanel = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public Color  fgColor = Color.blue;
    public Font   font = null;
    public Font   curFont = null;
    public int    layerIndex = -1;
    private String borderType = null;
    private String xmlBorderType ="";
    private String grpBorderType = "Etched";
    private String titlePosition = null;
    private String titleJustification = null;
    private String tab = m_no;
    private String useRef=m_no;
    private String objName = null;
    protected String showCmd=null;
    protected String hideCmd=null;
    protected String m_strEnable = m_no;
    protected String m_viewport = null;
    protected String m_helplink = null;
    protected String m_squish = null;

    private boolean isEditing = false;
    protected boolean inEditMode = false;
    private boolean inModalMode = false;
    private boolean isFocused = false;
    protected boolean bNeedUpdate = true;
    protected boolean bKeepSize = false;
    protected boolean bTitle = false;
    // protected boolean bFirstShow = true;
    protected boolean bShowTab = false;
    protected int    members = 0;
    protected int m_nPanels = 0;
    private ButtonGroup toggleGrp = null;
    private ButtonGroup radioGrp = null;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    private RightsList rightsList=null;
    protected ButtonIF vnmrIf;
    protected SessionShare sshare;
    private List<VToggle> toggleList = null;
    private List<VRadio> radioList = null;
    private String expanded = null;
    public boolean showing = false;
    private double xRatio = 1.0;
    private double xRatio2 = 1.0;
    private double yRatio = 1.0;
    protected Dimension defDim = new Dimension(0,0);
    protected Dimension prefDim = new Dimension(0,0);
    protected Dimension actualDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Dimension tmpDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private boolean newMember = true;
    private boolean bOneRowOnly = false;
    private boolean bMargin = false;
    private List<Component> compList = null;
    private int ptLen = 0;
    private int nWidth = 0;
    private int nHeight = 0;
    private int fontH1 = 0;  // origin font height
    private int fontH2 = 0;  // current font height
    private int showIntVal = 1;
    private int gridCols = 1;
    private int gridRows = 1;
    private int gridHalign = -1;
    private Border grpBorder = null;
    private Insets margin = new Insets(4, 4, 3, 3);
    private VGridLayout gridLayout;
    private VTabbedPane folder=null;
 
    public static final String m_strGroupBorder = "INTERFACE/groupborder";
    public static final String GROUPBORDER = "border";
    public static final String GROUPSIDE = "side";
    public static final String GROUPJUSTIFY = "justify";
    public static final String[] subtypes = {
        Util.getLabel("mlMajor"), 
        Util.getLabel("mlMinor"), 
        Util.getLabel("mlTitled"), 
        Util.getLabel("mlUntitled"), 
     };
    private final static String[] bdrTypes = { 
    	Util.getLabel("mlNone"),
        Util.getLabel("mlEtched"), 
        Util.getLabel("mlRaisedBevel"),
        Util.getLabel("mlLoweredBevel") 
    };

    /**
     * Name of local parameter tracked through the ActionCommand.
     * Accessible through set/getAttribute(PANEL_PARAM).
     */
    private String m_parameter = null;

    /**
     * ActionCommand used to handle value changes locally; not through Vnmr.
     * Accessible through set/getAttribute(ACTIONCMD).
     */
    private String m_actionCmd = null;

    /**
     * Java listeners that get sent the ActionCommand on value changes.
     */
    private Set<ActionListener> m_actionListenerList
        = new TreeSet<ActionListener>();

    /** Used to check for visibility if m_actionCmd is set. */
    protected String m_showValues = null;

    /** Command to execute as soon as it is loaded. */
    protected String m_loadCmd = null;


    public VGroup(SessionShare sShare, ButtonIF vif, String typ) {
        super();
        this.type = typ;
        this.vnmrIf = vif;
        this.tab = m_no;
        sshare = sShare;
        DisplayOptions.addChangeListener(this);

        setOpaque(false);   // overlapping groups allowed
        setLayout(new VGrpLayout());
        setBackground(null);
        mlEditor = new MouseAdapter() {
	        public void mouseEntered(MouseEvent me) {
	        	if(inEditMode)
	        		ParamEditUtil.testNewPanel(VGroup.this);
	        }
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                ParamEditUtil.clickInPanel(evt);
                if((modifier & InputEvent.BUTTON3_MASK) !=0){
                    ParamEditUtil.menuAction(evt);
                }
                else if ((modifier & (1 << 4)) != 0) {
                    if (inEditMode && clicks >= 1) {
                        if (m_nPanels >= 1 && strPanel != null && !strPanel.equals(""))
                            showPanel(evt.getPoint());
                        else{
                            ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                        }
                    }
                }
            }
        };
        mlNonEditor = new CSHMouseAdapter(); 
        
        // Start with the non editor listener.  If we change to Editor mode
        // it will be changed.
        
/*  There is a problem with the panels caused by adding the CSH
mouse listener.It only seems to be a problem when there is a "group"
which is partially covering another "group". The top group has some
empty space and the bottom group has an item which is visible through
the empty space.  If you click over the item, the top group catches
the click in the CSH listener.  The click never gets to the item.
When there is no CSH listener, somehow java lets the click go down
to the next level and the items gets the click.  I have found no way to
reissue the mouse event so that the lower level can catch it.
I also tried using Robot.class but it does not treat Btn1,2,3 the same
as the rest of Java.  I can make it work for right handed people, but
then it fails for left handed people.

For now, stop catching the right click for CSH in VGroup.  This means that
right clicking in the empty space of a panel, will not bring up help.  You
must click on some type of item.

If this line or some replacement is used again, remember that it also needs
to be done in setEditMode()
*/
        addMouseListener(mlNonEditor);

        addComponentListener(new ComponentListener() {
            public void componentResized(ComponentEvent e) {}

            public void componentMoved(ComponentEvent e) {}

            public void componentShown(ComponentEvent e) {
                 updateChildValue();
                // if (bFirstShow) {
                //    updateChildren(true);
                //    bFirstShow = false;
                // }
            }

            public void componentHidden(ComponentEvent e) {}

        });

/***
    addHierarchyListener(new HierarchyListener() {
        public void hierarchyChanged(HierarchyEvent e) {
        if (e.getChangeFlags() == HierarchyEvent.SHOWING_CHANGED)
            updateChildren(true);
        }
    });
**/
        new DropTarget(this, this);
    }

    public void setMargin(Insets m) {
        margin = m;
        revalidate();
    }

    public void enableChildren(boolean enable) {
        int nSize = getComponentCount();
        String s = enable ? m_true : m_false;
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                if (comp instanceof VGroup)
                    ((VGroup) comp).enableChildren(enable);
                else {
                    // String s = enable ? m_true : m_false;
                    // if the panel is to be enabled, then enable all the
                    // children,
                    // if the panel is being disabled, and if the global enable
                    // is true then don't disable the children
                    // use getAttribute as the subclasses of VGroup might be
                    // override getAttribute.
                    // String strpanel = getAttribute(ENABLE);
                    if (enable || (!enable && m_strEnable.equalsIgnoreCase(m_no))) {
                        ((VObjIF) comp).setAttribute(ENABLED, s);
                        // ((VObjIF) comp).updateValue();
                    } else if (m_strEnable.equalsIgnoreCase(m_yes)) {
                        ((VObjIF) comp).setAttribute(ENABLED, m_true);
                    }
                }
            }
        }
    }
    public void setFolder(VTabbedPane f){
        folder=f;
    }
    public boolean isTab() {
        return tab.equals(m_yes);
    }

    public boolean isFolderTab() {
        //Container cont = (Container) getParent();
        //if(cont !=null && (cont instanceof VTabbedPane))
        if(folder!=null)
            return true;
        return false;
    }

    public boolean isTabGroup() {
        if(tab.equals(m_yes) && getAttribute(LABEL)!=null)
            return true;
        return false;
    }

	public boolean isLayeredGroup() {
		if (strPanel != null && m_nPanels == 0)
			return true;
		return false;
	}

    public int getLayerGroups() {
        return m_nPanels;
    }

    private ParamLayout getParamLayout() {
        Container cont = (Container) getParent();
        while (cont != null) {
            if (cont instanceof ParamLayout)
                break;
            cont = cont.getParent();
        }
        return (ParamLayout) cont;
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        if (fg != null) {
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        changeFont();
    }

    // StatusListenerIF interface

    public void updateStatus(String msg) {
        int nSize = getComponentCount();
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof StatusListenerIF)
                ((StatusListenerIF) comp).updateStatus(msg);
        }
    }

    // ExpListenerIF interface

    private void updateChildValue() {
        if (!bNeedUpdate)
            return;
        bNeedUpdate = false;
        int n = getComponentCount();

        for (int i=0; i < n; i++) {
            Component comp = getComponent(i);
            // if (comp != null && !(comp instanceof VGroup))
            if (comp != null && (comp instanceof VObjIF))
                ((VObjIF)comp).updateValue();
        }
    }

    public void updateChildren(boolean visibleOnly) {
        if (!bNeedUpdate)
            return;
        if (!isVisible() && visibleOnly) {
            // bNeedUpdate = true;
            return;
        }
        
        updateChildValue();
    }

    /** called from ExpPanel (pnew) */
    public void updateValue(Vector params) {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
        if (inEditMode)
           return;

        if (showVal != null && hasVariable(this, params) != null) {
            bNeedUpdate = true;
            vnmrIf.asyncQueryShow(this, showVal);
            return;
        }
        if (!isShowing()) {
            // bNeedUpdate = true;
            if (isVisible())
               bNeedUpdate = true;
            return;
        }
        int i;
        Component comp;
        int n = getComponentCount();
        for (i = 0; i < n; i++) {
           comp = getComponent(i);
           if (comp instanceof VGroup) {
                  ((VGroup)comp).updateValue(params);
           }
        }
        for (i = 0; i < n; i++) {
            comp = getComponent(i);
            if (!(comp instanceof VObjIF))
                continue;
            if (comp instanceof VGroup)
                continue;
            VObjIF obj = (VObjIF) comp;
            String vname;
            String varstring;
            if (obj instanceof ExpListenerIF) {
                    ((ExpListenerIF) obj).updateValue(params);
            }
            else if ((varstring = obj.getAttribute(VARIABLE)) != null
                     && (vname = hasVariable(varstring, params)) != null)
            {
            	/*
                 String wantExpr = "$VALUE=" + vname;
                 int k = params.indexOf(vname);
                 String setval = obj.getAttribute(SETVAL);
                 String shoval = obj.getAttribute(SHOW);
                 int pnum = params.size() / 2;
                 String val;
                 if (varstring.trim().equals(vname)
                        && k >= 0
                        && k < pnum
                        && (shoval == null || shoval.trim().length() == 0)
                        && equalsIgnoreSpaces(setval, wantExpr)
                        && !(val = (String) params.elementAt(k + pnum))
                        .equals("\033")) {
                        obj.setValue(new ParamIF("", "", val));
                 } else {
                 */
                        obj.updateValue();
                 //}
            }
        }
    }

    /**
     * Checks if str1 is the same as str2, ignoring any extra spaces in str1.
     */
    /*************
    private boolean equalsIgnoreSpaces(String str1, String str2) {
        if (str1 == null)
            return false;
        int i, j;
        int nSize = str1.length();
        for (i=j=0; i<nSize; ++i) {
            char c;
            if ((c=str1.charAt(i)) != ' ') { // Skip spaces
                if(j>=str2.length())
                    return false;
                if (c != str2.charAt(j))
                    return false;
                ++j;
            }
        }
        return true;
    }
    *************/

    public void setVisible(boolean b) {
        boolean oldVis = isVisible();
        super.setVisible(b);
        Component comp = getParent();
        if (comp == null)
            return;
         if (oldVis != b ) {
           Container cnt = (Container) comp;
           LayoutManager l = null;
           while (cnt != null) {
               if (!(cnt instanceof VGroup)) {
                 l = cnt.getLayout();
                 if (l != null && (l instanceof VRubberPanLayout))
                    break;
                 l = null;
               }
               cnt = cnt.getParent();
           }
           if (l != null && (l instanceof VRubberPanLayout)) {
                   ((VRubberPanLayout)l).resetPreferredSize();
           }
        }
    }

    public void setvisible(boolean bShow) {
        super.setVisible(bShow);
        revalidate();
    }

    public void group_show() {
        if(showCmd !=null && showCmd.length()>0)
            vnmrIf.sendVnmrCmd(this, showCmd);
        bShowTab = true;
    }

    public void group_hide(){
        if(isShowing() && hideCmd !=null && hideCmd.length()>0)
            vnmrIf.sendVnmrCmd(this, hideCmd);
        bShowTab = false;
    }


    public void update(Vector params){
        updateValue(params);
    }

    /**
     * Returns the first variable in the VARIABLE list of the
     * VObjIF "obj" that matches one of the strings in the Vector
     * "params".
     * Returns null if there is no match.
     */
    private String hasVariable(VObjIF obj, Vector params){
        String  vars=obj.getAttribute(VARIABLE);
        if (vars == null)
            return null;
        StringTokenizer tok=new StringTokenizer(vars, " \t,\n");
        int nLength = params.size();
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken().trim();
            for (int k = 0; k < nLength; k++){
                if (var.equals(params.elementAt(k)))
                    return var;
            }
        }
        return null;
    }

    /**
     * Returns the first variable in the given "vars" string
     * that matches one of the names in the Vector "params".
     * Returns null if there is no match.
     */
    private String hasVariable(String vars, Vector params){
        if (vars == null)
            return null;
        StringTokenizer tok=new StringTokenizer(vars, " \t,\n");
        int nLength = params.size();
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken().trim();
            for (int k = 0; k < nLength; k++){
                if (var.equals(params.elementAt(k)))
                    return var;
            }
        }
        return null;
    }

    /** called from ExperimentIF for first time initialization  */
    public void updateValue() {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
        if (inEditMode)
            return;
        
        bNeedUpdate = true;
        if (showVal != null) {
            if (!isShowing()) {
               if (!bShowTab)
                  setVisible(false);
            }
            vnmrIf.asyncQueryShow(this, showVal);
            return;
        }
        if (!isVisible())
            return;

       /*************
        int i, k;
        int nSize = getComponentCount();
        k = 0;
        for (i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VGroup) {
                  ((VObjIF)comp).updateValue();
            }
            else
                 k++;
        }
        if (k > 0)
            bNeedUpdate = true;
        ***************/

        updateChildValue();       
     }

    /** call "destructors" of VObjIF children */
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF)
                ((VObjIF)comp).destroy();
        }
    }

    public String getName() {
        if(title!=null)
            return title;
        return "untitled";
    }

    public void setDefColor(String c) {
        fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public boolean getEditMode() {
        return inEditMode;
    }

    public void setEditMode(boolean s){
        boolean oldMode = inEditMode;
        inEditMode = s;
        int gcnt=0;
        int k = getComponentCount();

        for (int i = 0; i < k; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setEditMode(s);
                if (comp instanceof VGroup){
                    VGroup grp=(VGroup)comp;
                    if (m_nPanels >= 1){
                        if (grp.isLayeredGroup()){
                        	gcnt++;
                        	if(gcnt==layerIndex)
                        		grp.setVisible(true);
                        	else
                        		grp.setVisible(!inEditMode);
                        }
                    }
                    else if(inEditMode)  // supports old-style layered groups
                        grp.setVisible(true);
                 }
            }
        }
        
        if(!isFolderTab() && !isTabGroup()&& inEditMode && m_nPanels==0){
            setVisible(true);
        }
        if (s) {
            // Be sure both are cleared out
            removeMouseListener(mlNonEditor);
            removeMouseListener(mlEditor);
            addMouseListener(mlEditor);
        }
        else {
            // Be sure both are cleared out
            removeMouseListener(mlEditor);
            removeMouseListener(mlNonEditor);
            addMouseListener(mlNonEditor);
        }
        
        if(oldMode && oldMode!=s)
            bNeedUpdate=true;
        if (s) {
            if (defDim.width <= 0)
                defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            nWidth = defDim.width;
            nHeight = defDim.height;
            xRatio = 1.0;
            xRatio2 = 1.0;
            yRatio = 1.0;
            if (font == null) {
                font = getFont();
                fontH1 = font.getSize();
                fontH2 = fontH1;
            }
            curFont = font;
            if (title != null) {
                if (bTitle)
                    ((TitledBorder) grpBorder).setTitleFont(font);
            }
        } else {
            getActualSize();
            if (oldMode) {
                if (bMargin)
                    xRatio = 0.1;
                revalidate();
            }
        }
    }

    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case FGCOLOR:
            return fg;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case BORDER:
             return borderType;
        case SIDE:
            return titlePosition;
        case JUSTIFY:
            return titleJustification;
        case LABEL:
            return title;
        case TAB:
            return tab;
        case SHOW:
            return showVal;
        case ENABLE:
            return m_strEnable;
        case VARIABLE:
            return vnmrVar;
        case SUBTYPE:
            return strSubtype;
        case DIGITAL:
            return strPanels;
        case COUNT:
            return strPanel;
        case REFERENCE:
            return reference;
        case USEREF:
            return useRef;
        case CMD:
            return showCmd;
        case CMD2:
            return hideCmd;
        case EXPANDED:
            return expanded;
        case ACTIONCMD:
            return m_actionCmd;
        case PANEL_PARAM:
            return m_parameter;
        case VALUES:
            return m_showValues;
        case ENTRYCMD:          // Do this command on loading (now)
            return m_loadCmd;
        case TRACKVIEWPORT:
            return m_viewport;
        case PANEL_NAME:
            return objName;
        case HELPLINK:
        	return m_helplink;
        case ROWS:  // rows
            return Integer.toString(gridRows);
        case COLUMNS:  // columns
            return Integer.toString(gridCols);
        case SQUISH:
            return m_squish;
        default:
            return null;
        }
    }

    private int parseInt(String str, int min) {
        int value = min;

        if (str == null || str.length() < 1)
           return value;
        try {
              value = Integer.parseInt(str);
        } catch (Exception e) { }
        if (value < min)
           value = min;
        return value;
    }

    public void setAttribute(int attr, String c) {
        String strValue = c;
        if (c != null)
            c = c.trim();
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case FGCOLOR:
            fg = c;
            fgColor = DisplayOptions.getColor(fg);
            setForeground(fgColor);
            repaint();
            break;
        case PANEL_NAME:
            objName = c;
            break;
        case FONT_NAME:
            fontName = c;
            break;
        case FONT_STYLE:
            fontStyle = c;
            break;
        case FONT_SIZE:
            fontSize = c;
            break;
        case BORDER:   
        	xmlBorderType=c;
        	borderType=getBorderType(c);
            setBorder();
            break;
        case LABEL:
            String old_title = title;
            title = strValue;
            if (!inModalMode && inEditMode && strValue != null
                && !strValue.equals(old_title)) {
                ParamLayout p = getParamLayout();
                if (p != null) {
                    p.relabelTab(this, isTab());
                }
            }
            setBorder();
            break;
        case SIDE:
            titlePosition = c;
            setBorder();
            break;
        case JUSTIFY:
            titleJustification = c;
            setBorder();
            break;
        case TAB:
            String oldtab = tab;
            if (c != null && (c.equalsIgnoreCase(m_true) 
                              || c.equalsIgnoreCase(m_yes) 
                              || c.equals(m_yes))) 
            {
                tab = m_yes;
            } else 
                tab = m_no;

            ParamLayout p = getParamLayout();
            if (!inModalMode && inEditMode && title != null && p != null
                && !tab.equals(oldtab)) {
                if (tab.equals(m_no)) {
                    p.relabelTab(this, false);
                    // p.rebuildTabs();
                } else {
                    p.relabelTab(this, true);
                }
            }
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case SUBTYPE:
            strSubtype = getSubType(c);
            setGrouptype();
            break;
        case DIGITAL:
            strPanels = c;
            setPanels();
            break;
        case COUNT:
            strPanel = c;
            layerIndex = -1;
            if (c != null && c.length() > 0) {
               try {
                  layerIndex = Integer.parseInt(c);
               } catch (Exception e) { }
            }
            break;
        case SHOW:
            showVal = c;
            if (c != null && c.length() > 0) {
                if (!inEditMode)
                    setVisible(false);
            }
            else {
                if (!inEditMode)
                   setVisible(true);
            }
            break;
        case ENABLE:
            m_strEnable = c;
            break;
        case REFERENCE:
            reference = c;
            break;
        case USEREF:
            useRef = c;
            break;
        case CMD:
            showCmd = c;
            break;
        case CMD2:
            hideCmd = c;
            break;
        case EXPANDED:
            expanded = c;
            break;
        case ACTIONCMD:
            m_actionCmd = c;
            break;
        case PANEL_PARAM:
            m_parameter = c;
            break;
        case VALUES:
            m_showValues = c;
            break;
        case ENTRYCMD:          // Do this command on loading (now)
            m_loadCmd = c;
            if (c != null && c.length() > 0) {
                vnmrIf.sendVnmrCmd(this, c);
            }
            break;
        case TRACKVIEWPORT:
            m_viewport = c;
            break;
        case HELPLINK:
        	m_helplink = c;
            break;
        case LAYOUT:
            if (gridLayout == null) {
                gridLayout = new VGridLayout();
                gridLayout.setRows(gridRows);
                gridLayout.setColumns(gridCols);
                gridLayout.setHorizontalAlignment(gridHalign);
                setLayout(gridLayout);
            }
            break;
        case ROWS: // rows
            if (c != null) {
               gridRows = parseInt(c, 1);
               if (gridLayout != null)
                   gridLayout.setRows(gridRows);
            }
            break;
        case COLUMNS: // columns
            if (c != null) {
               gridCols = parseInt(c, 1);
               if (gridLayout != null)
                   gridLayout.setColumns(gridCols);
            }
            break;
        case GRID_ROWALIGN: // grid-row-align
            gridHalign = -1;
            if (c != null) {
               if (c.equalsIgnoreCase("left"))
                   gridHalign = SwingConstants.LEFT;
               else if (c.equalsIgnoreCase("right"))
                   gridHalign = SwingConstants.RIGHT;
               else if (c.equalsIgnoreCase("center"))
                   gridHalign = SwingConstants.CENTER;
            }
            if (gridLayout != null)
                gridLayout.setHorizontalAlignment(gridHalign);
            break;
        case SQUISH:
            m_squish = c;
            break;
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
        int nSize = getComponentCount();
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setVnmrIF(vif);
            }
        }
    }

    public void setValue(ParamIF pf) { }

    protected void setPanels() {
        if (strPanels == null || strPanels.equals(""))
            m_nPanels = 0;
        else {
            try {
                m_nPanels = Integer.parseInt(strPanels);
            } catch (Exception e) {
                m_nPanels = 0;
            }
        }
    }

    public void showPanel() {
        if (m_nPanels <= 0)
            return;

        int nPanel = 0;
        try {
            nPanel = Integer.parseInt(strPanel);
        } catch (Exception e) {
        }

        if (nPanel > m_nPanels)
            return;
        if (nPanel >= 1)
            showPanel(nPanel);
    }

    protected void showPanel(int nPanel) {
        int nSize = getComponentCount();
        VGroup group = null;
        // if the panel is already created, then show the panel.
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            boolean bShow = false;
            if (comp instanceof VGroup) {
                String strPanel2 = ((VGroup) comp).getAttribute(COUNT);
                if (strPanel.equals(strPanel2)) {
                    bShow = true;
                    group = (VGroup) comp;
                }
            }
            comp.setVisible(bShow);
        }

        if (group != null) {
            ParamEditUtil.setEditObj(group);
            group.isEditing = true;
            return;
        }
        // create a new panel, and set the panel number.
        group = new VGroup(sshare, vnmrIf, type);
        group.setAttribute(COUNT, strPanel);
        group.setAttribute(SUBTYPE, subtypes[2]);
        add(group);
        group.isEditing = true;
        Dimension size = getSize();
        int gap=ParamEditUtil.getSnapGap();
        size.width = size.width - gap;
        size.height = size.height - gap;
        group.setSize(size.width, size.height);
        group.setPreferredSize(size);
        group.setLocation(0, 0);
        ParamEditUtil.setEditObj(group);
    }

    protected void showPanel(Point point) {
    	int gap=ParamEditUtil.getSnapGap();
        Dimension size = getSize();
        if (point.x <= size.width - gap && point.y <= size.height - gap)
            showPanel();
        else
            ParamEditUtil.setEditObj(this);
    }

    protected void setGrouptype() {
        setGroupborder();
        setBorder();
    }

    private String getBorderType(String c){
    	String btype = c.trim().toLowerCase();
		if(c.equals(Util.getLabel("mlNone"))||btype.equals("none"))
			c="None";
		else if(c.equals(Util.getLabel("mlEtched"))||btype.startsWith("etch"))
			c="Etched";
		else if(c.equals(Util.getLabel("mlRaisedBevel"))||btype.contains("raise"))
			c="RaisedBevel";
		else if(c.equals(Util.getLabel("mlLoweredBevel"))||btype.contains("low"))
			c="LoweredBevel";
		else c = "None";
		return c;
    }
    private String getSubType(String c){
		if(c.equals(Util.getLabel("mlMajor"))||c.equalsIgnoreCase(m_major))
			c=m_major;
		else if(c.equals(Util.getLabel("mlMinor"))||c.equalsIgnoreCase(m_minor))
			c=m_minor;
		else if(c.equalsIgnoreCase("Basic") && xmlBorderType.equalsIgnoreCase("etched"))
			c=m_titled;
		else if(c.equals(Util.getLabel("mlTitled"))||c.equalsIgnoreCase(m_titled))
			c=m_titled;
		else
			c=m_untitled;
		return c;
    }
    protected void setGroupborder() {
        if (strSubtype.isEmpty() || (!strSubtype.equals(m_minor) && !strSubtype.equals(m_major)))
            return;

        String strPath = FileUtil.openPath(m_strGroupBorder);
        if (strPath == null)
            return;

        try {
            BufferedReader reader = new BufferedReader(new FileReader(strPath));
            String strLine;
            while ((strLine = reader.readLine()) != null) {
                StringTokenizer strTok = new StringTokenizer(strLine);
                if (strTok.hasMoreTokens()) {
                    String strkey = strTok.nextToken();
                    if (strTok.hasMoreTokens()) {
                        String strValue = strTok.nextToken();
                        if (strkey.equalsIgnoreCase(GROUPBORDER))
                        	grpBorderType = getBorderType(strValue);
                        else if (strkey.equalsIgnoreCase(GROUPSIDE))
                            titlePosition = strValue;
                        else if (strkey.equalsIgnoreCase(GROUPJUSTIFY))
                            titleJustification = strValue;
                    }
                }
            }
            reader.close();
        } catch (Exception e) {
            // e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    public void showGroup(boolean vis) {
        setVisible(vis);
        int nSize = getComponentCount();
        for (int i=0; i<nSize; i++) {
            Component comp = getComponent(i);
            if(comp instanceof VGroup)
                ((VGroup)comp).showGroup(vis);
            else
                comp.setVisible(vis);
        }
    }

    public int getShowValue() {
        return showIntVal;
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null && !inEditMode) {
            String s = pf.value.trim();
            boolean oldshow = showing;
            try {
                showIntVal = Integer.parseInt(s);
            } catch (Exception e) {
            }
            showing = showIntVal > 0 ? true : false;
            if(isFolderTab()){
                folder.setTabVisible(this, showing);
            }
            else if (isTabGroup()) {
                ParamLayout p = getParamLayout();
                if (p != null) {
                    if (oldshow != showing)
                        p.setChangedTab(title, showing);
                    if (p.isSelectedTabGroup(this))
                        updateChildValue();
                } else {
                    if (showing) {
                        if (!isVisible())
                            bNeedUpdate = true;
                        updateChildValue();
                    }
                    setVisible(showing);
                }
            } else {
                if (showing) {
                    if (!isVisible())
                        bNeedUpdate = true;
                    updateChildValue();
                }
                setVisible(showing);
            }
        }
    }

    public String toString () {
        return title;
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        //repaint();
    }

    public void setOneRowLayout(boolean s) {
        bOneRowOnly = s;
        int nSize = getComponentCount();
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VGroup) {
                ((VGroup) comp).setOneRowLayout(s);
            }
        }
    }

    public void paint(Graphics g) {
        super.paint(g);
        if (!inEditMode) {
           return;
        }
        Dimension psize = getSize();
        if (isEditing) {
            if (isFocused)
                g.setColor(Color.yellow);
            else
                g.setColor(Color.green);
        } else
            g.setColor(Color.blue);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height - 1, psize.width - 1, psize.height - 1);
        g.drawLine(psize.width - 1, 0, psize.width - 1, psize.height - 1);
    }

    public Component add(Component comp) {
        return add(comp, -1);
    }

    public Component add(Component comp, int index) {
        int k, num;
        newMember = true;
        if (comp instanceof VToggle
            && ((VObjIF)comp).getAttribute(RADIOBUTTON).equals("yes"))
        {
            VToggle toggleComp = (VToggle)comp;
            if (toggleList == null)
                toggleList = new ArrayList<VToggle>();
            toggleList.add(toggleComp);
            num = toggleList.size();
            if (num >= 2) {
                if (toggleGrp == null)
                    toggleGrp = new ButtonGroup();
                if (num == 2) {
                    for (k = 0; k < 2; k++)
                        toggleGrp.add(toggleList.get(k));
                } else
                    toggleGrp.add(toggleComp);
            }
        } else if (comp instanceof VRadio) {
            VRadio radioComp = (VRadio)comp;
            if (radioList == null)
                radioList = new ArrayList<VRadio>();
            radioList.add(radioComp);
            num = radioList.size();
            if (num >= 2) {
                if (radioGrp == null)
                    radioGrp = new ButtonGroup();
                if (num == 2) {
                    for (k = 0; k < 2; k++)
                        radioGrp.add(radioList.get(k));
                } else
                    radioGrp.add(radioComp);
            }
        }
        return super.add(comp, index);
    }

    public void remove(Component comp) {
        newMember = true;
        if (comp instanceof VToggle
            && ((VToggle)comp).getAttribute(RADIOBUTTON).equals(m_true)) {
            if (toggleList != null) {
                toggleList.remove(comp);
                if (toggleGrp != null) {
                    toggleGrp.remove((VToggle)comp);
                    if (toggleList.size() == 1)
                        toggleGrp.remove(toggleList.get(0));
                }
            }
        } else if (comp instanceof VRadio) {
            if (radioList != null) {
                radioList.remove(comp);
                if (radioGrp != null) {
                    radioGrp.remove((VRadio)comp);
                    if (radioList.size() == 1)
                        radioGrp.remove(radioList.get(0));
                }
            }
        }
        super.remove(comp);
    }

    public void refresh() {}
    public void setDefLabel(String s) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void changeFont() {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        curFont = font;
        fontH1 = font.getSize();
        fontH2 = fontH1;
        setBorder();
        validate();
        repaint();
    }

    public void setDefaultLayout(){
        setLayout(new VGrpLayout());
    }
    private class VGrpLayout implements LayoutManager {
        private Dimension psize;

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            int nmembers = target.getComponentCount();
            int w = 0;
            int h = 0;
            Point pt;
            for (int i = 0; i < nmembers; i++) {
                Component comp = target.getComponent(i);
                if (comp != null) {
                    psize = comp.getPreferredSize();
                    pt = comp.getLocation();
                    if (pt.x + psize.width > w)
                        w = pt.x + psize.width;
                    if (pt.y + psize.height > h)
                        h = pt.y + psize.height;
                }
            }
            prefDim.width = w + 2;
            prefDim.height = h + 2;
            return prefDim;
        } // preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        } // minimumLayoutSize()

        private void floatLayout(Container target) {
            synchronized (target.getTreeLock()) {
                int x = 0;
                Point pt;
                int nmembers = compList.size();
                for (int i = 0; i < nmembers; i++) {
                    Component comp = compList.get(i);
                    if ((comp != null) && comp.isVisible()) {
                        psize = comp.getSize();
                        if (psize.width <= 1 || psize.height <= 1) {
                            psize = comp.getPreferredSize();
                            if (psize.width > 1 && psize.height > 1)
                                comp.setSize(psize.width, psize.height);
                        }
                        // comp.setBounds(x, 0, psize.width, psize.height);
                        if (comp instanceof JButton)
                            ((JButton)comp).setMargin(new Insets(0, 0, 0, 0));
                        if (xRatio != 1.0) {
                            comp.setLocation(x, 0);
                            x += psize.width;
                        } else {
                            pt = comp.getLocation();
                            if (pt.x < x)
                                pt.x = x;
                            comp.setLocation(pt.x, 0);
                            x = pt.x + psize.width;
                        }
                    }
                }
            }
        }
        
        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Point loc;
                int count = target.getComponentCount();
                if (bOneRowOnly) {
                    if (compList == null || count != members)
                        sortOrder(target);
                    members = count;
                    floatLayout(target);
                    return;
                }
                members = 0;
                int width = target.getWidth();
                int height = target.getHeight();
                int gap=ParamEditUtil.getSnapGap();
                for (int i = 0; i < count; i++) {
                    Component comp = target.getComponent(i);
                    //if ((comp != null)&& comp.isVisible()) {
                        if (inEditMode)
                            psize = comp.getPreferredSize();
                        else
                            psize = comp.getSize();
                        if (m_nPanels >= 1 && comp instanceof VGroup) {
                            if (((VGroup) comp).getAttribute(COUNT) != null) {
                                psize.width = width - gap;
                                psize.height = height - gap;
                            }
                        }
                        loc = comp.getLocation();
                        comp.setBounds(loc.x, loc.y, psize.width, psize.height);
                        members++;
                    //}
                }
            }
        }
    } // VGrpLayout

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this,inEditMode);
    } // drop

    public Object[][] getAttributes() {
        boolean bLayered = false;
        Container container = getParent();
        if (container instanceof VGroup
            && ((VGroup) container).getLayerGroups() >= 1)
            bLayered = true;

        String[] aStrSubtype = subtypes;
        attributes[6][3] = aStrSubtype;
        attributes2[6][3] = aStrSubtype;

        if (bLayered)
            return attributes2;
        
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            rightsList = vif.getRightsList();
        if(rightsList != null && rightsList.isApproved("mayedithelpfield"))
            return attributesH;
        
        return attributes;
    }

    private final static String[] yes_no = {m_yes,m_no};

    private final static Object[][] attributes = {
        { new Integer(LABEL), Util.getLabel(LABEL) },
        { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
        { new Integer(SHOW), Util.getLabel("vgSHOW") },
        { new Integer(CMD), Util.getLabel("vgCMD") },
        { new Integer(CMD2), Util.getLabel("vgCMD2") },
        { new Integer(BORDER), Util.getLabel("vgBORDER"), "radio",bdrTypes },
        { new Integer(SUBTYPE), Util.getLabel("vgSUBTYPE"), "radio",subtypes },
        { new Integer(DIGITAL), Util.getLabel("vgDIGITAL"), m_true },
        { new Integer(COUNT), Util.getLabel("vgCOUNT"), m_true },
        { new Integer(BGCOLOR), Util.getLabel(BGCOLOR), "color" },
        { new Integer(TAB), Util.getLabel("vgTAB"), "radio", yes_no },
        { new Integer(ENABLE), Util.getLabel("vgENABLE"), "radio", yes_no }, 
        };
    
    private final static Object[][] attributesH = {
        { new Integer(LABEL), Util.getLabel(LABEL) },
        { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
        { new Integer(SHOW), Util.getLabel("vgSHOW") },
        { new Integer(CMD), Util.getLabel("vgCMD") },
        { new Integer(CMD2), Util.getLabel("vgCMD2") },
        { new Integer(BORDER), Util.getLabel("vgBORDER"), "radio",bdrTypes },
        { new Integer(SUBTYPE), Util.getLabel("vgSUBTYPE"), "radio",subtypes },
        { new Integer(DIGITAL), Util.getLabel("vgDIGITAL"), m_true },
        { new Integer(COUNT), Util.getLabel("vgCOUNT"), m_true },
        { new Integer(TAB), Util.getLabel("vgTAB"), "radio", yes_no },
        { new Integer(ENABLE), Util.getLabel("vgENABLE"), "radio", yes_no }, 
        { new Integer(HELPLINK), Util.getLabel("blHelp")}, 
        };

    
    private final static Object[][] attributes2 = {
        { new Integer(LABEL), Util.getLabel(LABEL) },
        { new Integer(VARIABLE), Util.getLabel(VARIABLE) },
        { new Integer(SHOW), Util.getLabel("vgSHOW") },
        { new Integer(CMD), Util.getLabel("vgCMD") },
        { new Integer(CMD2), Util.getLabel("vgCMD2") },
        { new Integer(BORDER), Util.getLabel("vgBORDER"), "radio",bdrTypes },
        { new Integer(SUBTYPE), Util.getLabel("vgSUBTYPE"), "radio",subtypes },
        };

    private void setBorder() {
    	 if (borderType == null || borderType.isEmpty())
    		 return;
        Color titleColor = fgColor == null ? getForeground() : fgColor;
        if (curFont == null) {
            font = getFont();
            curFont = font;
            fontH1 = font.getSize();
        }
       	        
        String btype=borderType;
        String btitle="";
        if (strSubtype.equals(m_minor)){
        	btype="Empty";
        	btitle=title;
        }
        else if(strSubtype.equals(m_major)){
            btype=grpBorderType;
         	btitle=title;
        }
        else if(strSubtype.equals(m_titled))
        	btitle=title;
        grpBorder = BorderDeli.createBorder(btype, btitle, titlePosition,
                                            titleJustification, titleColor,
                                            curFont);
        if (grpBorder != null) {
            if (grpBorder instanceof TitledBorder)
                bTitle = true;
            bKeepSize = true;
            setBorder(grpBorder);
        } else
            bKeepSize = false;
    }
    public void setModalMode(boolean s) {inModalMode=s;}
    public void sendVnmrCmd() {}


    /**
     * Sets the action command to the given string.
     * @param actionCommand The new action command.
     */
    public void setActionCommand(String actionCommand) {
        m_actionCmd = actionCommand;
    }

    /**
     * Add an action listener to the list of listeners to be notified
     * of actions.
     */
    public void addActionListener(ActionListener listener) {
        m_actionListenerList.add(listener);
    }

    public Dimension getActualSize() {
        int w = 0;
        int w2 = 0;
        int h = 0;
        Dimension dim;
        Point pt;
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
            Component comp = getComponent(i);
            if ((comp != null) && comp.isVisible()) {
                if (comp instanceof VObjIF)
                    pt = ((VObjIF) comp).getDefLoc();
                else
                    pt = comp.getLocation();
                if (comp instanceof VGroup)
                    dim = ((VGroup) comp).getActualSize();
                else
                    dim = comp.getPreferredSize();
                w2 += dim.width;
                if ((pt.x + dim.width) > w)
                    w = pt.x + dim.width;
                if ((pt.y + dim.height) > h)
                    h = pt.y + dim.height;
            }
        }
        if (bOneRowOnly) {
            if (w2 > w)
                w = w2;
        }
        if (bMargin) {
            w = w + margin.left + margin.right;
            h = h + margin.top + margin.bottom;
        }
        actualDim.width = w;
        actualDim.height = h;
        if (!inEditMode) {
            if (defDim.width <= 0)
                defDim = getPreferredSize();
            if (!isPreferredSizeSet()) {
                setPreferredSize(defDim);
            }
            if (bKeepSize || isOpaque()) {
                actualDim.width = defDim.width;
                actualDim.height = defDim.height;
            } else {
                if ((defDim.width < w) || (defDim.height < h)) {
                    if (w > defDim.width)
                        defDim.width = w;
                    if (h > defDim.height)
                        defDim.height = h;
                    setPreferredSize(defDim);
                }
            }
        }
        return actualDim;
    }

    public Dimension getRealSize() {
        int w = 0;
        int h = 0;
        Dimension dim;
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
            Component comp = getComponent(i);
            if ((comp != null) && comp.isVisible()) {
                if (comp instanceof VGroup)
                    dim = ((VGroup) comp).getRealSize();
                else
                    dim = comp.getPreferredSize();
                w += dim.width;
                if (dim.height > h)
                    h = dim.height;
            }
        }
        if (bOneRowOnly) {
            if (w > actualDim.width)
                actualDim.width = w + 1;
        }
        tmpDim.width = w + 1;
        tmpDim.height = h + 1;

        return tmpDim;
    }

    public double getXRatio() {
        return xRatio2;
    }

    public double getYRatio() {
        return yRatio;
    }

    public void setSizeRatio(double rx, double ry) {
        double d;
        boolean bGrpOnly = false;
        boolean bSetMargin = false;
        if (inEditMode) {
            rx = 1.0;
            ry = 1.0;
        } else {
            if (bMargin && margin != null)
                bSetMargin = true;
        }
        if ((rx == xRatio) && (ry == yRatio)) {
            // if (!bSetMargin)
            bGrpOnly = true;
        }
        xRatio = rx;
        yRatio = ry;
        if (defDim.width <= 0) {
            defDim = getPreferredSize();
        }
        if (newMember || (actualDim.width < 1)) {
            newMember = false;
            getActualSize();
        }
        if ((rx != 1.0) || (ry != 1.0)) {
            if (rx > 1.0) {
                xRatio = rx - 1.0;
                curLoc.x = (int) ((double) defLoc.x * xRatio);
                d = (double) (defLoc.x + actualDim.width) * xRatio;
                curDim.width = (int) d - curLoc.x;
            } else {
                curLoc.x = defLoc.x;
                curDim.width = defDim.width;
            }
            if (ry > 1.0) {
                yRatio = ry - 1.0;
                curLoc.y = (int) ((double) defLoc.y * yRatio);
                d = (double) (defLoc.y + actualDim.height) * yRatio;
                curDim.height = (int) d - curLoc.y;
            } else {
                curLoc.y = defLoc.y;
                curDim.height = defDim.height;
            }
        } else {
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
        }

        double w, nx, ny;
        int mx = margin.left;
        int my = margin.top;
        int min = 1;
        nx = rx;
        ny = ry;
        if (bSetMargin) {
            if (bKeepSize)
                min = 2;
            w = curDim.width - mx - margin.right - min * 2;
            if (w > 6) {
                w = mx + margin.right + min * 2;
                if (rx != 1.0)
                    nx = rx - w / (double) actualDim.width;
                else
                    nx = 2.0 - w / (double) defDim.width;
                if (nx >= 2.0)
                    nx = 1.99;
                mx = (int) (mx * xRatio) + min;
            } else {
                mx = 2;
                nx = rx - 0.1;
            }
            w = curDim.height - my - margin.bottom - min * 2;
            if (w > 6) {
                w = my + margin.bottom + min * 2;
                if (ry != 1.0)
                    ny = ry - w / (double) actualDim.height;
                else
                    ny = 2.0 - w / (double) defDim.height;
                if (ny >= 2.0)
                    ny = 1.99;
                my = (int) (my * yRatio) + min;
            } else {
                my = 2;
                ny = ry - 0.1;
            }
        }
        int nSize = getComponentCount();
        for (int i = 0; i < nSize; i++) {
            Component comp = getComponent(i);
            if (comp != null && (comp instanceof VObjIF)) {
                if (bGrpOnly) {
                    if (!(comp instanceof VGroup))
                        continue;
                }
                ((VObjIF) comp).setSizeRatio(nx, ny);
                if (bSetMargin) {
                    Point pt = comp.getLocation();
                    comp.setLocation(pt.x + mx, pt.y + my);
                }
            }
        }

        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
        xRatio = rx;
        xRatio2 = rx;
        yRatio = ry;
    }

    public void resizeAll()
    {
        double oldx = xRatio;
        double oldy = yRatio;
        xRatio = 0.0;
        setSizeRatio(oldx, oldy);
    }

    public void setDefLoc(int x, int y) {
        defLoc.x = x;
        defLoc.y = y;
        super.setLocation(x, y);
    }

    public Point getDefLoc() {
        tmpLoc.x = defLoc.x;
        tmpLoc.y = defLoc.y;
        return tmpLoc;
    }

    private void adjustFont() {
        FontMetrics fm2;
        int rWidth, nw, nh;

        if (title == null)
            return;
        if (curDim.width <= 10)
            return;

        if (font == null || fontH1 <= 0) {
            font = getFont();
            fontH1 = font.getSize();
            fontH2 = fontH1;
        }
        nw = nWidth;
        nWidth = curDim.width;
        nHeight = curDim.height;
        nh = nHeight / 2;

        if (nWidth >= nw && nw > 0) {
            if (fontH2 >= fontH1) {
                if (nh >= fontH2)
                    return;
            }
        }

        nw = nWidth - 5;
        // curFont = font.deriveFont(s);
        // curFont = DisplayOptions.getFont(font.getName(), font.getStyle(),
        // fontH2);
        String strfont = font.getName();
        int nstyle = font.getStyle();
        curFont = DisplayOptions.getFont(strfont, nstyle, fontH2);
        fm2 = getFontMetrics(curFont);
        rWidth = fm2.stringWidth(title);
        if (rWidth < nw) {
            fontH2 = fontH1;
            curFont = DisplayOptions.getFont(strfont, nstyle, fontH2);
            fm2 = getFontMetrics(curFont);
            rWidth = fm2.stringWidth(title);
        }

        while (fontH2 >= 9) {
            if (rWidth < nw && fontH2 <= nh)
                break;
            fontH2--;
            // curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, fontH2);
            fm2 = getFontMetrics(curFont);
            rWidth = fm2.stringWidth(title);
        }
        ((TitledBorder) grpBorder).setTitleFont(curFont);
    }

    public void setBounds(int x, int y, int w, int h) {
        if (inEditMode) {
            defLoc.x = x;
            defLoc.y = y;
            defDim.width = w;
            defDim.height = h;
        }
        curLoc.x = x;
        curLoc.y = y;
        curDim.width = w;
        curDim.height = h;
        if ((nWidth != w) || (nHeight != h)) {
            if (!inEditMode && bTitle) {
                adjustFont();
            }
        }
        super.setBounds(x, y, w, h);
    }

    public Point getLocation() {
        if (inEditMode) {
            tmpLoc.x = defLoc.x;
            tmpLoc.y = defLoc.y;
        } else {
            tmpLoc.x = curLoc.x;
            tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }

    private void sortOrder(Container target) {
        int count = target.getComponentCount();
        int x = 0;
        int k;
        Component c1, c2;
        if ((compList == null) || (compList.size() < count))
            compList = new ArrayList<Component>(count);
        if (ptLen < count) {
            ptLen = count + 2;
        }
        compList.clear();
        for (int i = 0; i < count; i++) {
            c1 = target.getComponent(i);
            if (c1 == null)
                continue;
            x = c1.getLocation().x;
            if (i == 0) {
                compList.add(c1);
            } else {
                k = i / 2;
                while (k > 0) {
                    c2 = compList.get(k);
                    if (c2.getLocation().x > x)
                        k = k / 2;
                    else
                        break;
                }
                while (k < i) {
                    c2 = (Component) compList.get(k);
                    if (c2.getLocation().x > x) {
                        compList.add(k, c1);
                        break;
                    }
                    k++;
                }
                if (k == i) {
                    //room not found
                    compList.add(c1);
                }
            }
        }
    }
    
    /* CSHMouseAdapter
     * 
     * Mouse Listener to put up Context Sensitive Help (CSH) Menu and
     * respond to selection of that menu.  The panel's .xml file must have
     * "helplink" set to the keyword/topic for Robohelp.  It must be a 
     * topic listed in the .properties file for this help manual.
     * If helplink is not set, it will open the main manual.
     */
    private class CSHMouseAdapter extends MouseAdapter  {
        public CSHMouseAdapter() {
            super();
        }
        
        public void mousePressed(MouseEvent evt) {
            if (evt.getButton() != MouseEvent.BUTTON3)
                 VMouseDispatch.dispatch(evt); 
        }
        
        public void mouseReleased(MouseEvent evt) {
            if (evt.getButton() != MouseEvent.BUTTON3)
                 VMouseDispatch.dispatch(evt); 
        }

        public void mouseEntered(MouseEvent evt) {
                 // VMouseDispatch.dispatch(evt); 
        }
        
        public void mouseExited(MouseEvent evt) {
                 // VMouseDispatch.dispatch(evt); 
        }
        
        
        public void mouseClicked(MouseEvent evt) {
            int btn = evt.getButton();

            if (btn != MouseEvent.BUTTON3) {
                 VMouseDispatch.dispatch(evt); 
                 return;
            }

            if(btn == MouseEvent.BUTTON3) {
                // Find out if there is any help for this item. If not, bail out
                String helpstr=m_helplink;
                // If helpstr is not set, see if there is a higher
                // level VGroup that has a helplink set.  If so, use it.
                // Try up to 3 levels of group above this.
                if(helpstr==null){
                    Container group = getParent();
                        helpstr = CSH_Util.getHelpFromGroupParent(group, title, LABEL, HELPLINK);
                }

                // If no help available, don't put up the menu.
                if(!CSH_Util.haveTopic(helpstr))
                    return;
                
                // Create the menu and show it
                JPopupMenu helpMenu = new JPopupMenu();
                String helpLabel = Util.getLabel("CSHMenu");
                JMenuItem helpMenuItem = new JMenuItem(helpLabel);
                helpMenuItem.setActionCommand("help");
                helpMenu.add(helpMenuItem);
                    
                ActionListener alMenuItem = new ActionListener()
                    {
                        public void actionPerformed(ActionEvent e)
                        {  
                            // Get the helplink string for this object
                            String helpstr=m_helplink;
                                          
                            // If helpstr is not set, see if there is a higher
                            // level VGroup that has a helplink set.  If so, use it.
                            // Try up to 3 levels of group above this.
                            if(helpstr==null){
                                  Container group = getParent();
                                  helpstr = CSH_Util.getHelpFromGroupParent(group, title, LABEL, HELPLINK);    
                            }

                            // Get the ID and display the help content
                            CSH_Util.displayCSHelp(helpstr);
                        }
                    };
                helpMenuItem.addActionListener(alMenuItem);
                    
                Point pt = evt.getPoint();
                helpMenu.show(VGroup.this, (int)pt.getX(), (int)pt.getY());

            }
             
        }
    }  /* End CSHMouseAdapter class */

}


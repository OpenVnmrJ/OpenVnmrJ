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
import java.awt.dnd.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import java.beans.*;
import javax.swing.*;
import javax.swing.plaf.LabelUI;
import vnmr.util.*;
import vnmr.ui.*;

public class VLabel extends JLabel
  implements VEditIF, VObjIF, VObjDef, DropTargetListener,
   PropertyChangeListener, VAnnotationIF
 {
    public String type = null;
    public String value = null;
    public String showText = null;
    public String fg = null;
    public String bg = null;
    public String iconName=null;
    public String vnmrVar = null;
    public String showVal = null;
    public String setVal = null;
    public String iconSetVal = null;
    public String labelSetVal = null;
    public String tipStr = null;
    protected String strSubtype = null;
    protected boolean m_bParameter = false;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String dispSize = null;
    public String stretchStr = null;
    public Color  fgColor = Color.black;
    public Color  bgColor, orgBg;
    public Font   font = null;
    public Font   curFont = null;
    public Font   prtFont = null;
    public int    curWidth = 0;
    public int    curHeight = 0;
    public int    elasticType = 2; // variable
    public int    orgElasticType = 2;
    public int    fontHeight = 12;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean bPrintMode = false;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    private RightsList rightsList=null;
    protected String m_helplink = null;
    private ButtonIF vnmrIf;
    protected String keyStr = null;
    private int isActive = 1;

    private boolean isFocused = false;
    private boolean bTitleBar = false;

    private FontMetrics origFm = null;
    private FontMetrics curFm = null;
    private float fontRatio = 1.0f;
    private int labelW = 0;
    private int labelH = 0;
    private int nWidth = 0;
    private int nHeight = 0;
    private int titleW = 0;
    private int prefW = 0;
    private int orgFontH = 0;
    public  int iconW = 0;
    public  int iconH = 0;
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    public  ImageIcon imgIcon = null;
    public  VButtonIcon vIcon = null;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Dimension tmpDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private Point m_toolTipLocation = null;
    private Rectangle annRect = new Rectangle(0, 0, 20, 20); // rect of annotation

    /* The following variables are prepared for Annotation */
    private VDropHandlerIF dropHandler = null;
    private VobjEditorIF editor = null;
    private boolean bAnnotation = false;
    private boolean textLine = false;
    private boolean bVertical = false;
    private boolean bHtmlType = false;
    private String precision = null;
    private String orientVal = null;
    private String hJustyVal = null;
    private String vJustyVal = "Center";
    private String objName = null;
    private int cols = 2;
    private int rows = 2;
    private int annX = 0;
    private int annY = 0;
    private int idNum = 0;
    private int gridNum = 10;
    private int orientInt = SwingConstants.HORIZONTAL;
    private int stretchDir = VStretchConstants.NONE;
    private float  objRx = 0;
    private float  objRy = 0;
    private float  objRw = 0;
    private float  objRh = 0;
    private float  gridX = 20;
    private float  gridY = 20;
    private float  printRw = 1;
    private float  printRh = 1;

    /** The string array for displaying the left, right, center justification in the menu.  */
    private final static String[] m_arrStrTtlJust = {"Left","Center","Right"};
    private final static String[] m_orientStr = {"Horizontal", "Vertical"};
    private final static String[] m_verticalJuststr = {"Top", "Center", "Bottom"};
    private final static String[] m_sizeStr = {"Fit Only", "Variable", "Fixed",
		"Fixed and Fit"};

    /** The integer array that corresponds to the constant value for the left, right, and center justification. */
    private final static int[] m_hAlignments = {SwingConstants.LEFT, SwingConstants.CENTER, SwingConstants.RIGHT};
    private final static int[] m_vAlignments = {SwingConstants.TOP, SwingConstants.CENTER, SwingConstants.BOTTOM};
    private final static int[] m_orientations = {SwingConstants.HORIZONTAL, SwingConstants.VERTICAL};

    /** The array of the attributes that are displayed in the edit template.*/
    private final static Object[][] m_attributes = {
	{new Integer(LABEL), "Label of item:"},
	{new Integer(ICON), 	"Icon of item:"},
	{new Integer(VARIABLE),	"Vnmr variables:    "},
	{new Integer(SHOW),	"Enable condition:"},
	{new Integer(JUSTIFY), "Label justification:", "menu", m_arrStrTtlJust},
    };
    
    private final static Object[][] m_attributes_H = {
        {new Integer(LABEL), "Label of item:"},
        {new Integer(ICON),     "Icon of item:"},
        {new Integer(VARIABLE), "Vnmr variables:    "},
        {new Integer(SHOW), "Enable condition:"},
        {new Integer(JUSTIFY), "Label justification:", "menu", m_arrStrTtlJust},
        {new Integer(HELPLINK), Util.getLabel("blHelp")}
        };

    private final static Object[][] m_attributes_2 = {
	{new Integer(LABEL), "Label of item:"},
	{new Integer(ICON), 	"Icon of item:"},
	{new Integer(SHOW),	"Show condition:"},
	{new Integer(HALIGN), "Horizontal alignment:", "menu", m_arrStrTtlJust},
	{new Integer(VALIGN), "Vertical alignment:", "menu", m_verticalJuststr},
        {new Integer(ORIENTATION), "Orientation:", "menu", m_orientStr},
        {new Integer(ELASTIC), "Font Size:", "menu", m_sizeStr}
    };

    private final static Object[][] m_attributes_3 = {
        {new Integer(SETVAL),   "Imaging parameter to display:"},
        {new Integer(NUMDIGIT), "Number of digits:"},
	{new Integer(SHOW),	"Show condition:"},
	{new Integer(HALIGN), "Horizontal alignment:", "menu", m_arrStrTtlJust},
	{new Integer(VALIGN), "Vertical alignment:", "menu", m_verticalJuststr},
        {new Integer(ORIENTATION), "Orientation:", "menu", m_orientStr},
        {new Integer(ELASTIC), "Font Size:", "menu", m_sizeStr}
    };

    public VLabel(SessionShare sshare, ButtonIF vif, String typ) {
        super("", null, SwingConstants.LEFT);
	this.type = typ;
	this.vnmrIf = vif;
        if (typ != null && typ.equals("textline")) {
            textLine = true;
        }
	setOpaque(false);
	orgBg = getBackground();
	bgColor = Util.getBgColor();
	setBackground(bgColor);
        setIconTextGap(6);
	mlEditor = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        informEditor(evt);
                    }
                }
             }
            };
	
        mlNonEditor = new CSHMouseAdapter();
    
        // Start with the non editor listener.  If we change to Editor mode
        // it will be changed.
        addMouseListener(mlNonEditor);

	new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);
    }

    public void setEditor(VobjEditorIF obj) {
        editor = obj;
    }

    private void informEditor(MouseEvent e) {
        VObjIF obj = (VObjIF)e.getSource();
        if (m_bParameter)
        {
            Component comp = ((Component)obj).getParent();
            if (comp instanceof VParameter)
                obj = (VObjIF)comp;
        }
        if (editor != null) {
             editor.setEditObj(obj);
        }
        else
             ParamEditUtil.setEditObj(obj);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        if (evt != null) {
            if (bg != null)
                bgColor=DisplayOptions.getColor(bg);
            else
                bgColor = Util.getBgColor();
            setBackground(bgColor);
        }
        if (fg!=null)
            fgColor=DisplayOptions.getColor(fg);
        // NB: This code doesn't work for new LAF themes (bg color not accessible)
        /*
        if (fgColor != null) {
            int fv = fgColor.getRed();
            int bv = bgColor.getRed();
            int d1 = Math.abs(fv - bv);
            if (d1 < 60) {
                int d2 = Math.abs(fgColor.getGreen() - bgColor.getGreen());
                int d3 = Math.abs(bgColor.getBlue() - bgColor.getBlue());
                fv = 0;
                if (d1 > 30)
                    fv++;
                if (d2 > 30) {
                    fv++;
                    if (d2 > 60)
                        fv++;
                }
                if (d3 > 30) {
                    fv++;
                    if (d3 > 60)
                        fv++;
                }
                if (fv < 2) {
                    fv = fgColor.getRed();
                    bv = fgColor.getGreen();
                    if (fv > 120 && bv > 120)
                        fgColor = Color.black;
                    else
                        fgColor = Color.white;
                }
            }
        }
        */
        setForeground(fgColor);
        
        changeFont();
     }

    public String getType() {
        return type;
    }

    public String getLabel() {
        return getText();
    }

    public void setDefLabel(String s) {
    	this.value = s;
    	showText = s;
        setText(s);
        labelW = 0;
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setRowCol(int r, int c) {
        rows = r;
        cols = c;
    }

    public Point getRowCol() {
        return new Point(rows, cols);
    }

    public void setXPosition(int x, int y) {
        annX = x;
        annY = y;
    }


    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setDropHandler(VDropHandlerIF c) {
        dropHandler = c;
    }

    public void setEditMode(boolean s) {
        if (bg == null)
            setOpaque(s);
        if (s) {
           // Be sure both are cleared out
           removeMouseListener(mlNonEditor);
           removeMouseListener(mlEditor);
           addMouseListener(mlEditor);
           if (font == null)
               getDefaultFont();
           setFont(font);
           // curFont = font;
           fontRatio = 1.0f;
           if (defDim.width <= 0)
               defDim = getPreferredSize();
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
           xRatio = 1.0;
           yRatio = 1.0;
           curHeight = labelH;
           curWidth = labelW;
           if (!isVisible())
               setVisible(true);
        }
        else {
           // Be sure both are cleared out
           removeMouseListener(mlEditor);
           removeMouseListener(mlNonEditor);
           addMouseListener(mlNonEditor);
        }
        inEditMode = s;
    }

    private void getDefaultFont() {
        font = DisplayOptions.getFont(fontName,fontStyle,fontSize);
        origFm = getFontMetrics(font);
        orgFontH = font.getSize();
    }

    public void changeFont() {
        getDefaultFont();
        setFont(font);
        fontRatio = 1.0f;
        curFont = font;
        curFm = origFm;
        calSize();
        fontHeight = orgFontH;
        if (!inEditMode || bAnnotation) {
             if ((curDim.width > 0) && (labelW > curDim.width)) {
                 adjustFont(curDim.width, curDim.height);
             }
        }
        repaint();
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    public String getAttribute(int attr) {
        switch (attr) {
          case TYPE:
                     return type;
          case LABEL:
                     if (bAnnotation && iconName != null)
                        return null;
                     if (labelSetVal != null)
                        return labelSetVal;
                     return value;
          case ICON:
                     if (iconSetVal != null)
    		         return iconSetVal;
    		     return iconName;
          case FGCOLOR:
                     return fg;
          case BGCOLOR:
                     return bg;
          case SHOW:
                     return showVal;
          case FONT_NAME:
                     return fontName;
          case FONT_STYLE:
                     return fontStyle;
          case FONT_SIZE:
                     return fontSize;
          case VARIABLE:
                     return vnmrVar;
          case KEYSTR:
                     return keyStr;
          case KEYVAL:
                     return keyStr==null ? null: getText();
          case SETVAL:
                     return setVal;
          case SUBTYPE:
                     return strSubtype;
          case NUMDIGIT:
                     return precision;
          case JUSTIFY:
                    return hJustyVal;
          case HALIGN:
                     if (bAnnotation)
                        return hJustyVal;
                     else
                         return null;
          case VALIGN:
                     if (bAnnotation)
                        return vJustyVal;
                     else
                         return null;
         case ORIENTATION:
                     if (bAnnotation)
                        return orientVal;
                     else
                         return null;
          case ROW:
                     if (bAnnotation)
                         return Integer.toString(rows);
                     else
                         return null;
          case COLUMN:
                     if (bAnnotation)
                         return Integer.toString(cols);
                     else
                         return null;
          case LOCX:
                     if (bAnnotation)
                         return Integer.toString(annX);
                     else
                         return null;
          case LOCY:
                     if (bAnnotation)
                         return Integer.toString(annY);
                     else
                         return null;
          case UNITS:
                     if (bAnnotation)
                         return Integer.toString(gridNum);
                     else
                         return null;
          case ELASTIC:
                     if (bAnnotation)
                         return dispSize;
                     else
                         return null;
          case TOOL_TIP:
                     return tipStr;
          case OBJID:
                     if (idNum > 0)
                         return Integer.toString(idNum);
                     else
                         return null;
          case PANEL_NAME:
                     return objName;
          case HELPLINK:
              return m_helplink;
          case STRETCH:
              return stretchStr;
          default:
                     return null;
        }
    }

    private boolean loadIcon(String c) {
        if (c != null) {
            c = c.trim();
            if (c.length() <= 0)
               c = null;
        } 
        iconName = c;
        labelW = 0;
        iconW = 0;
        imgIcon = null;
        vIcon = null;
        if (c == null) {
            setIcon(null);
            setText(showText);
            return false;
        }
        // ImageIcon imgIcon = Util.getImageIcon(iconName+".gif");
        imgIcon = Util.getGeneralIcon(iconName);
        if(imgIcon != null) {
            vIcon = new VButtonIcon(imgIcon.getImage());
            vIcon.setStretch(stretchDir);
        }
        if (stretchDir == VStretchConstants.NONE)
            setIcon(imgIcon);
        else
            setIcon(vIcon);
        if (imgIcon != null) {
            iconW = imgIcon.getIconWidth();
            iconH = imgIcon.getIconHeight();
            if (bAnnotation)
                setText(null);
            return true;
        }
        return false;
    }

    public void setAttribute(int attr, String c) {
        int k;

        if (c != null) {
            c = c.trim();
            if (c.length() <= 0)
                c = null;
        }

        switch (attr) {
          case TYPE:
                     type = c;
                     break;
          case LABEL:
                     if (c != null) {
                        if (c.indexOf("$VALUE") >= 0) {
                            if (c.indexOf("=") >= 0) {
                                labelSetVal = c;
                                return;
                            }
                        }
                     }
                     value = c;
          case KEYVAL:
                     bHtmlType = false;
    	             showText = c;
                     if (bAnnotation) {
                        if (iconName == null)
                           setText(c);
                     }
                     else
                        setText(c);
                     labelW = 0;
                     if (c != null && c.length() > 6) {
                        if (c.indexOf("<html>") >= 0)
                           bHtmlType = true;
                     }
                     break;
          case ICON:
                    iconSetVal = null;
                    if (c != null) {
                        if (c.indexOf("$VALUE") >= 0) {
                            if (c.indexOf("=") >= 0) {
                                iconSetVal = c;
                                return;
                            }
                        }
                    }
                    loadIcon(c);
                    break;
          case FGCOLOR:
                     fg = c;
                    /***
                     fgColor=DisplayOptions.getColor(fg);
                     setForeground(fgColor);
                    *****/
                     propertyChange(null);
                     repaint();
                     break;
          case BGCOLOR:
                    bg = c;
                    if (c != null) {
                        bgColor=DisplayOptions.getColor(c);
                        setOpaque(true);
                    }
                    else {
                        bgColor = Util.getBgColor();
                        setOpaque(inEditMode);
                    }
                    setBackground(bgColor);
                    repaint();
                    break;
          case SHOW:
                     showVal = c;
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
          case VARIABLE:
                     vnmrVar = c;
                     break;
          case KEYSTR:
      		         keyStr=c;
                     break;
/*
          case KEYVAL:
    	             showText = c;
                     setText(c);
		     labelW = 0;
                     break;
*/
          case SETVAL:
                    setVal = c;
                     break;
          case SUBTYPE:
              strSubtype = c;
              if (strSubtype != null && strSubtype.equals("parameter"))
                  m_bParameter = true;
              else
                  m_bParameter = false;
              break;
          case NUMDIGIT:
                     precision = c;
                     break;
          case JUSTIFY:
          case HALIGN:
                    hJustyVal = c;
                    setTextPosition(false, c);
                    break;
          case VALIGN:
                    vJustyVal = c;
                    setTextPosition(true, c);
                    break;
          case ANNOTATION:
                    bAnnotation = false;
                    if (c != null) {
                       if (c.equals("yes")) {
                           bAnnotation = true;
                           DisplayOptions.removeChangeListener(this);
                           if(value == null && iconName == null ) {
                              if (textLine)
                                 value = "textmessage";
                              else
                                 value = type;
    	                      showText = value;
                           }
                       }
                    }
                    break;
         case ORIENTATION:
                    orientVal = c;
                    if (c != null) {
                        for (k = 0; k < m_orientStr.length; k++) {
                            if (m_orientStr[k].equals(c)) {
                                 if (orientInt != m_orientations[k]) {
                                      orientInt = m_orientations[k];
                                      calSize();
                                      if (!inEditMode || bAnnotation)
                                          adjustFont(curDim.width, curDim.height);
                                      repaint();
                                 }
                                 return;
                             }
                        }
                    }
                    break;
          case ROW:
                    if (c != null)
                        rows =  Integer.parseInt(c);
                    if (rows <= 0)
                        rows = 1;
                    break;
          case COLUMN:
                    if (c != null)
                        cols =  Integer.parseInt(c);
                    if (cols <= 0)
                        cols = 1;
                    break;
          case UNITS:
                    if (c != null)
                        gridNum =  Integer.parseInt(c);
                    break;
          case LOCX:
                    if (c != null)
                        annX =  Integer.parseInt(c);
                    if (annX < 0)
                        annX = 0;
                    break;
          case LOCY:
                    if (c != null)
                        annY =  Integer.parseInt(c);
                    if (annY < 0)
                        annY = 0;
                    break;
          case ELASTIC:
                    dispSize = c;
                    elasticType = 2;
                    if (c.equals(m_sizeStr[0]))
                        elasticType = 1;
                    else if (c.equals(m_sizeStr[1]))
                        elasticType = 2;
                    else if (c.equals(m_sizeStr[2]))
                        elasticType = 3;
                    else if (c.equals(m_sizeStr[3]))
                    elasticType = 4;
                    orgElasticType = elasticType;
                    break;
          case TOOL_TIP:
          case TOOLTIP:
                    tipStr = c;
                    if (c != null && c.length() > 0)
                        setToolTipText(Util.getLabel(c));
                    else
                        setToolTipText(null);
                    break;
          case OBJID:
                    if (c != null)
                        idNum =  Integer.parseInt(c);
                    if (idNum < 0)
                        idNum = 0;
                    break;
          case PANEL_NAME:
                    objName = c;
                    break;
          case HELPLINK:
                    m_helplink = c;
                    break;
          case STRETCH:
                    stretchStr = c;
                    stretchDir = VStretchConstants.NONE;
                    if (c != null) {
                        if (c.equalsIgnoreCase("horizontal"))
                            stretchDir = VStretchConstants.HORIZONTAL;
                        else if (c.equalsIgnoreCase("vertical"))
                            stretchDir = VStretchConstants.VERTICAL;
                        else if (c.equalsIgnoreCase("both"))
                             stretchDir = VStretchConstants.BOTH;
                        else if (c.equalsIgnoreCase("even"))
                             stretchDir = VStretchConstants.EVEN;
                    }
                    if (stretchDir != VStretchConstants.NONE && vIcon != null) {
                        vIcon.setStretch(stretchDir);
                        setIcon(vIcon);
                    }
                    else
                        setIcon(imgIcon);
                    break;
        }
    }

    /**
     *  Sets the text position of the title specified by the string as "left", "right" or "center".
     */
    private void setTextPosition(boolean vertical, String c)
    {
        int i;

        if (vertical) {
            for (i = 0; i < m_verticalJuststr.length; i++) {
               if (m_verticalJuststr[i].equals(c)) {
                  setVerticalAlignment(m_vAlignments[i]);
                  break;
               }
            }
            return;
        }
        for (i = 0; i < m_arrStrTtlJust.length; i++)
        {
            if (m_arrStrTtlJust[i].equals(c))
            {
                setHorizontalAlignment(m_hAlignments[i]);
                break;
            }
        }
    }

    /**
     *  Returns the attributes array object.
     */
    public Object[][] getAttributes()
    {
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            rightsList = vif.getRightsList();
      
        if (textLine)
            return m_attributes_3;
        else if (bAnnotation)
            return m_attributes_2;
        else if(rightsList != null && rightsList.isApproved("mayedithelpfield"))
            return m_attributes_H;
        
        return m_attributes;
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setTitlebarObj(boolean s) {
        bTitleBar = s;
        if (s) {
           prefW = getPreferredSize().width;
           if (prefW < 1)
              prefW = 100;
        }
    }

    public String getText() {
        if (!bAnnotation)
            return showText;
    	if (iconName != null)
            return null;
        else
            return showText;
/*
        if (showText == null) {
    	    if (iconName == null)
                return "label";
            else
                return showText;
        }
        else
            return showText;
*/
    }

    public int getLabelWidth() {
        return titleW;
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            if (setVal == null && iconSetVal != null) {
               if (loadIcon(pf.value))
                   return;
            }
 
            value = pf.value;
            if (precision != null) {
                value = Util.setPrecision(value, precision);
            }
    	    showText = value;
            /*  call repaint directly to avoid revalidate call */
            /*  revalidate may generate parent's layout events */
            if (bTitleBar) {
               if (origFm != null) {
                    int w = titleW;
                    titleW = origFm.stringWidth(value) + 8;
                    if (titleW > prefW || w > prefW) {
                        revalidate();
                    }
               }
               repaint();
            }
            else
               setText(value);
            labelW = 0;
        }
    }

    private void queryValue() {
        if (setVal != null) {
             if (textLine && precision != null)
                  vnmrIf.asyncQueryParam(this, setVal, precision);
             else
                  vnmrIf.asyncQueryParam(this, setVal);
             return;
        }
        if (iconSetVal != null) {
             vnmrIf.asyncQueryParam(this, iconSetVal);
             return;
        }
        if (labelSetVal != null) {
             vnmrIf.asyncQueryParam(this, labelSetVal);
             return;
        }
    }

    public void setShowValue(ParamIF pf) {
        if (pf == null || pf.value == null)
            return;
        String  s = pf.value.trim();
        isActive = Integer.parseInt(s);
        if (isActive > 0) {
             setOpaque(inEditMode);
             if (bg != null) {
                  setOpaque(true);
             }
             if (bgColor != null) {
                  setBackground(bgColor);
             }
        }
        else {
             setOpaque(true);
             if (isActive == 0)
                 setBackground(Global.IDLECOLOR);
             else
                 setBackground(Global.NPCOLOR);

        }
        queryValue();
    }

    public void updateValue() {
        if (vnmrIf == null)
            return;
        if (showVal != null) {
            vnmrIf.asyncQueryShow(this, showVal);
            return;
        }
        queryValue();
    }

    public void setId(int n) {
        idNum = n;
    }

    public int getId() {
        return idNum;
    }


    private void vpaint(Graphics g) {
        int x = 2;
        int x2 = 2;
        int w = 2;
        int y = 2;
        int ha = getHorizontalAlignment();
        int va = getVerticalAlignment();
        int k;
        Dimension  size = getSize();
        if (inEditMode && isOpaque()) {
            g.setColor(bgColor);
            g.fillRect(0, 0, size.width, size.height);
        }
        Icon icon = (isEnabled()) ? getIcon() : getDisabledIcon();
        if (va == SwingConstants.CENTER)
            y = (size.height - curHeight) / 2;
        else if (va == SwingConstants.BOTTOM)
            y = size.height - curHeight;
        if (y < 0)
            y = 0;
        if (icon != null) {
            if (ha == SwingConstants.CENTER)
                x = (size.width - icon.getIconWidth()) / 2;
            else if (ha == SwingConstants.RIGHT)
                x = size.width - icon.getIconWidth() - 2;
            icon.paintIcon(this, g, x, y);
            y = y + icon.getIconHeight() + 2;
        }

        if (value == null)
            return;
        g.setFont(getFont());
        g.setColor(fgColor);
        x = 2;
        if (ha == SwingConstants.CENTER)
            x = (size.width - curWidth) / 2;
        else if (ha == SwingConstants.RIGHT)
            x = size.width - curWidth - 2;
        if (x < 0)
            x = 0;
        y = y + curFm.getAscent();
        for (k = 0; k < value.length(); k++) {
            String str = value.substring(k, k+1);
            w = curFm.charWidth(value.charAt(k));
            x2 = x + (curWidth - w) / 2;
            g.drawString(str, x2, y);
            y += fontHeight;
        }
    }

    public void paint(Graphics g) {
        if (orientInt == SwingConstants.HORIZONTAL) {
            super.paint(g);
        }
        else {
            vpaint(g);
        }
        if (!isEditing)
            return;
        Dimension  psize = getSize();

        if (isFocused)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    public void draw(Graphics2D g, int offX, int offY) {
        int x = curLoc.x + offX;
        int y = curLoc.y + offY;
        if (!isVisible())
            return;
        if (curDim.width < 5 || curDim.height < 7)
            return;
        setFont(curFont);
        g.translate(x, y);
        if (imgIcon != null) {
            if (curDim.width < iconW || curDim.height < iconH || bPrintMode) {
                int w = iconW;
                int h = iconH;
                int vx = 0;
                int vy = 0;
                int ha = getHorizontalAlignment();
                int va = getVerticalAlignment();
                Image img = imgIcon.getImage();		 
                if (bPrintMode) {
                   float rw = (float) curDim.width / (float) w;
                   float rh = (float) curDim.height / (float) h;
                   if (rw > rh)
                         rw = rh;
                   w = (int) ((float)iconW * rw);
                   h = (int) ((float)iconH * rw);
                }    
                else {
                   if (w > curDim.width)  w = curDim.width;
                   if (h > curDim.height)  h = curDim.height;
                }
                if (va == SwingConstants.CENTER)
                    vy = (curDim.height - h) / 2;
                else if (va == SwingConstants.BOTTOM)
                    vy = curDim.height - h;
                if (vy < 0)   vy = 0;
                if (ha == SwingConstants.CENTER)
                    vx = (curDim.width - w) / 2;
                else if (ha == SwingConstants.RIGHT)
                    vx = curDim.width - w;
                if (vx < 0)   vx = 0;
                    g.drawImage(img, vx, vy, w, h, null);
                g.translate(-x, -y);
                return;
            }
        }
        if (orientInt == SwingConstants.HORIZONTAL) {
            super.paint(g);
        }
        else {
            vpaint(g);
        }
        g.translate(-x, -y);
    }

    public void refresh() {
    	System.out.println(getAttribute(TYPE)+" refresh");
    }

    public void destroy() {
        imgIcon = null;
        vIcon = null;
        DisplayOptions.removeChangeListener(this);
    }

    public void setPrintMode(boolean s) {
        if (!s && bPrintMode) {
            calSize();
            curFont = font;
            setFont(font);
        }
        bPrintMode = s;
        if (!s) {
            elasticType = orgElasticType;
            setForeground(fgColor);
        }
        else {
            elasticType = 2;
            if (prtFont != null)
                setFont(prtFont);
        }
    }

    public void setPrintColor(boolean s) {
        if (!s)
            setForeground(Color.black);
        else
            setForeground(fgColor);
 
    }

    public void setPrintRatio(float rw, float rh) {
        printRw = rw;
        printRh = rh;
        Rectangle r = getBounds();
        curLoc.x = (int) ((float) r.x * rw);
        curLoc.y = (int) ((float) r.y * rh);
        curDim.width = (int) ((float) r.width * rw);
        curDim.height = (int) ((float) r.height * rh);
        int h = 10;
        if (font != null)
           h = font.getSize();
        if (rw > rh)
           h = (int) ((float) h * rh);
        else
           h = (int) ((float) h * rw);
        changeFontSize(h);
    }

    public void objPrintf(PrintWriter f, int gap) {
    }

    public int getGridNum() {
        return gridNum;
    }

    public void adjustBounds(int row, int col, float xgap, float ygap) {
         int x = (int) ((float) annX * xgap);
         int y = (int) ((float) annY * ygap);
         int w = (int) ((float) cols * xgap);
         int h = (int) ((float) rows * ygap);
         gridNum = row;
         gridX = xgap;
         gridY = ygap;
         setBounds(x, y, w, h);
    }

    public void setGridInfo(int row, int col, float xgap, float ygap) {
        gridNum = row;
        gridX = xgap;
        gridY = ygap;
    }

    public void adjustBounds() {
         adjustBounds(gridNum, gridNum, gridX, gridY);
    }

    public void readjustBounds(int r, int c, float xgap, float ygap) {
        annX = (int) ((float) annRect.x / xgap+ 0.5f);
        annY = (int) ((float) annRect.y / ygap+ 0.5f);
        cols = (int) ((float) annRect.width / xgap+ 0.5f);
        rows = (int) ((float) annRect.height / ygap+ 0.5f);
        if (annX >= c)
            annX = c - 1;
        if (annY >= r)
            annY = r - 1;
        if ((annX + cols) > c)
            cols = c - annX;
        if ((annY + rows) > r)
            rows = r - annY;

        if (cols <= 0)
            cols = 1;
        if (rows <= 0)
            rows = 1;
        gridNum = r;
        gridX = xgap;
        gridY = ygap;
    }

    public void readjustBounds() {
    }

    public void setBoundsRatio(boolean all) {
        Rectangle r = getBounds();
        Rectangle r2 = getParent().getBounds();
        objRx = ((float) r.x / (float) r2.width);
        objRy = ((float) r.y / (float) r2.height);
        objRw = ((float) r.width / (float) r2.width);
        objRh = ((float) r.height / (float) r2.height);
        annX = (int) ((float) r.x / gridX + 0.5f);
        annY = (int) ((float) r.y / gridY + 0.5f);
        cols = (int) ((float) r.width / gridX + 0.5f);
        rows = (int) ((float) r.height / gridY + 0.5f);
        annRect.x = r.x;
        annRect.y = r.y;
        annRect.width = r.width;
        annRect.height = r.height;
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
         if (dropHandler != null) {
            dropHandler.processDrop(e, this, inEditMode);
         }
         else
            VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public void setSizeRatio(double x, double y) {
        xRatio =  x;
        yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
        if (defDim.width <= 0)
           defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void setDefLoc(int x, int y) {
         defLoc.x = x;
         defLoc.y = y;
    }

    public Point getDefLoc() {
         tmpLoc.x = defLoc.x;
         tmpLoc.y = defLoc.y;
         return tmpLoc;
    }

    public void calSize() {
        if (font == null) {
            getDefaultFont();
            setFont(font);
            curFont = font;
        }
        curFm = origFm;
        labelH = orgFontH;
        labelW = 2;
        if (value != null) {
            if (orientInt == SwingConstants.HORIZONTAL) {
                labelW += origFm.stringWidth(value);
            }
            else {
                labelW = origFm.charWidth('W');
                labelH = orgFontH * value.length() + 4;
            }
        }
        if (getIcon() != null) {
            if (orientInt == SwingConstants.HORIZONTAL) {
                labelW += getIcon().getIconWidth();
                labelW += getIconTextGap();
            }
            else {
                labelH += getIcon().getIconHeight();
                labelH += getIconTextGap();
            }
        }
        curWidth = labelW;
        curHeight = labelH;
        if (curFont == null) {
           curFont = font;
           fontHeight = orgFontH;
        }
    }


    public void reshape(int x, int y, int w, int h) {
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
        if (!inEditMode || bAnnotation) {
           if (vIcon != null) {
               vIcon.setSize(w, h);
               iconW = vIcon.getIconWidth();
               iconH = vIcon.getIconHeight();
           }
           if (labelW <= 0) {
                calSize();
           }
           if (value != null) {
                boolean bAdjust = false;
                if (w != nWidth) {
                    if ((w > nWidth) && (nWidth < labelW))
                        bAdjust = true;
                    if (bPrintMode)
                        bAdjust = true;
                    else if (w < labelW)
                        bAdjust = true;
                }
                if (!bAdjust && (h != nHeight)) {
                    if ((h > nHeight) && (nHeight < labelH))
                         bAdjust = true;
                    else if (h < labelH)
                         bAdjust = true;
                }
                if (!bAdjust) {
                    if (!curFont.equals(getFont()))
                        bAdjust = true;
                }
                if (bAdjust)
                   adjustFont(w, h);
           }
/*
           if (orientInt == SwingConstants.HORIZONTAL) {
               if ((w != nWidth) || (h < nHeight))
                   adjustFont(w, h);
	       }
           else {
               if ((h != nHeight) || (w < nWidth))
                   adjustFont(w, h);
           }
*/
        }
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

    public void adjustHtmlFont(int w, int h) {
        if (orientInt != SwingConstants.HORIZONTAL)
            return;
        LabelUI ui = getUI();
        if (ui == null)
            return;

        setFont(font);
        Dimension dim = ui.getPreferredSize(this);
        if (w >= dim.width && h >= dim.height)
            return;
        String strfont = font.getName();
        int nstyle = font.getStyle();
        float s = (float) orgFontH - 0.5f;
        curFont = font;
        while (s > 7.0f) {
            s = s - 0.5f;
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
            setFont(curFont);
            dim = ui.getPreferredSize(this);
            if (w >= dim.width && h >= dim.height)
                break;
        }
        curFm = getFontMetrics(curFont);
        fontHeight = curFont.getSize();
        curWidth = dim.width;
        curHeight = dim.height;
        setFont(curFont);
        if (bPrintMode)
             prtFont = curFont;
        if (elasticType == 1) { // fit only
            if ((w < curWidth) || (h < curHeight)) {
                if (isVisible())
                    setVisible(false);
                return;
            }
        }
        if (!isVisible())
            setVisible(true);
    }


    public void adjustFont(int w, int h) {
        if (bTitleBar)
            return;
        if (labelW <= 0)
           calSize();
        if (elasticType > 2) { // fixed size
            nWidth = w;
            nHeight = h;
            if (elasticType == 4) {  // fit only
                if ((w - labelW < 0) || (h - labelH < 0)) {
                    if (isVisible())
                        setVisible(false);
                    return;
                }
            }
            if (!isVisible())
                setVisible(true);
            return;
        }
        if (w < 5 || h < 7 || labelW < 2)
           return;
        nWidth = w;
        nHeight = h;
        if (bHtmlType) {
            adjustHtmlFont(w, h);
            return;
        }

        float s = 1.0f;
        boolean bFreeSize = false;

        if (bPrintMode || stretchDir != VStretchConstants.NONE)
            bFreeSize = true;
        Icon icon = getIcon();
        if (value == null || (value.length() <= 0)) // icon only
           return;
        if (orientInt == SwingConstants.HORIZONTAL) {
            if (w < labelW)
                s = (float) w / (float) labelW;
            else
                s = (float) h / (float) labelH;
        }
        else {
            s = (float) h / (float) labelH;
        }
        if (s > 1.0f && !bFreeSize)
            s = 1.0f;
        if (s == fontRatio)
            return;
        fontRatio = s;
        if (!bFreeSize && fontRatio >= 1.0f) {
            curFont = font;
            curWidth = labelW;
            curHeight = labelH;
            fontHeight = orgFontH;
            curFm = origFm;
            setFont(font);
            return;
        }

        s = (float) orgFontH * fontRatio;
        if (s < 8)
            s = 8;
        String strfont = font.getName();
        Insets insets = getInsets(null);
        int nstyle = font.getStyle();
        int marginW = insets.left+ insets.right + 2;
        int marginH = insets.top+ insets.bottom + 2;
        while (s > 7.0f) {
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
            curFm = getFontMetrics(curFont);
            fontHeight = curFont.getSize();
            curHeight = fontHeight;
            curWidth = fontHeight;
            if (orientInt == SwingConstants.HORIZONTAL) {
                curWidth = curFm.stringWidth(value) + marginW;
                if (icon != null) {
                    curWidth += icon.getIconWidth();
                    curWidth += getIconTextGap();
                }
            }
            else {
                curHeight = value.length() * fontHeight + marginH;
                if (icon != null)
                    curHeight += icon.getIconHeight();
                curWidth = curFm.charWidth('W');
            }
            if ((curWidth < w) && (curHeight < h))
                break;
            s = s - 0.5f;
        }
        setFont(curFont);
        if (bPrintMode)
             prtFont = curFont;
        if (elasticType == 1) { // fit only
            if ((w < curWidth) || (h < curHeight)) {
                if (isVisible())
                    setVisible(false);
                return;
            }
        }
        if (!isVisible())
            setVisible(true);
    }

    public void changeShape(int x, int y, int w, int h) {
        curLoc.x = x;
        curLoc.y = y;
        curDim.width = w;
        curDim.height = h;
        // if (!bPrintMode)
           super.reshape(x, y, w, h);
    }

    public int getWidth() {
       /*
        if (bPrintMode) {
            int w = curDim.width;
           return w;
        }
       */
        return super.getWidth();
    }

    public int getHeight() {
       /*
        if (bPrintMode) {
           int h =  curDim.height;
           return h;
        }
       */
        return super.getHeight();
    }

    public void changeFontSize(int s) {

        if (elasticType > 2)
           return;
        if (value == null || (value.length() <= 0))
           return;
        String strfont = font.getName();
        int nstyle = font.getStyle();
        while (true) {
            curFont = DisplayOptions.getFont(strfont, nstyle, s);
            curFm = getFontMetrics(curFont);
            fontHeight = curFont.getSize();
            curHeight = fontHeight;
            curWidth = fontHeight;
            if (fontHeight < curDim.height)
               break;
            if (s < 10)
               break;
            s = s - 1;
        }
        Icon icon = getIcon();
        if (orientInt == SwingConstants.HORIZONTAL) {
            curWidth = curFm.stringWidth(value) + 2;
            if (icon != null)
                    curWidth += icon.getIconWidth();
        }
        else {
            curHeight = value.length() * fontHeight + 4;
            curWidth = curFm.charWidth('W');
            if (icon != null) { 
                curHeight += icon.getIconHeight();
                curWidth += icon.getIconWidth();
            }
        }
        setFont(curFont);
    }

     /**
      * Sets a fixed tool tip location (w.r.t. this component).
      * @param pt The tool tip location, relative to the upper-left
      * corner of this label.  Use "null" to have it calculate a location.
      */
     public void setToolTipLocation(Point pt) {
         m_toolTipLocation = pt;
     }

     public Point getToolTipLocation(MouseEvent event) {
         if (m_toolTipLocation != null) {
             return m_toolTipLocation;
         } else {
             return super.getToolTipLocation(event);
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

         public void mouseClicked(MouseEvent evt) {
             int btn = evt.getButton();
             if(btn == MouseEvent.BUTTON3) {
                // Find out if there is any help for this item. If not, bail out
                String helpstr=m_helplink;
                
                // If value is null, try tool tip
                String label2use;
                if(value == null || value.length() == 0) {
                    label2use = getAttribute(TOOLTIP);
                }
                else
                    label2use = value;
             
                // If helpstr is not set, see if there is a higher
                // level VGroup that has a helplink set.  If so, use it.
                // Try up to 3 levels of group above this.
                if(helpstr==null){
                    Container group = getParent();
                    helpstr = CSH_Util.getHelpFromGroupParent(group, label2use, LABEL, HELPLINK);
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

                             // If value is null, try tool tip
                             String label2use;
                             if(value == null || value.length() == 0) {
                                 label2use = getAttribute(TOOLTIP);
                             }
                             else
                                 label2use = value;

                             // If helpstr is not set, see if there is a higher
                             // level VGroup that has a helplink set.  If so, use it.
                             // Try up to 3 levels of group above this.
                             if(helpstr==null){
                                 Container group = getParent();
                                 helpstr = CSH_Util.getHelpFromGroupParent(group, label2use, LABEL, HELPLINK);
                                 
                             }

                             // Get the ID and display the help content
                             CSH_Util.displayCSHelp(helpstr);
                         }
                     };
                 helpMenuItem.addActionListener(alMenuItem);
                     
                 Point pt = evt.getPoint();
                 helpMenu.show(VLabel.this, (int)pt.getX(), (int)pt.getY());

             }
              
         }
     }  /* End CSHMouseAdapter class */

}



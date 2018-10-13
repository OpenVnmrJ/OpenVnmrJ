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
import vnmr.util.*;
import vnmr.ui.*;

public class VTextMsg extends JTextField
implements VObjIF, VEditIF, StatusListenerIF, VObjDef,
           DropTargetListener,PropertyChangeListener
{
    public String type = null;
    public String chval = null;
    public String value = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String showVal = null;
    public String setVal = null;
    protected String strSubtype = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String precision = null;
    public Color  fgColor = Color.black;
    public Color  bgColor, orgBg;
    public Font   font = null;
    private String statusParam = null;
    protected boolean m_bParameter = false;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private MouseAdapter mlEditor;
    private MouseAdapter mlNonEditor;
    private RightsList rightsList=null;
    protected String m_helplink = null;
    private ButtonIF vnmrIf;
    private boolean isValid = false;
    private boolean isActive = true;
    private VTextMsg m_objTxtMsg;

    private int nHeight = 0;
    private int nWidth = 0;
    private int rHeight = 90;
    private int fontH = 0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private Point m_toolTipLocation = null;
    protected String m_viewport = null;
    private String m_alignment = null;


    public VTextMsg(SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        m_objTxtMsg = this;
        setOpaque(false);
        orgBg = getBackground();
        bgColor = Util.getBgColor();
        setBackground(bgColor);
        setBorder(BorderFactory.createEmptyBorder());
        setEditable(false);
        setFocusable(false);
        mlEditor = new MouseAdapter() {
              public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        if (!m_bParameter)
                            ParamEditUtil.setEditObj((VObjIF) evt.getSource());
                        else
                        {
                            Component comp = ((Component)evt.getSource()).getParent();
                            if (comp instanceof VParameter)
                                ParamEditUtil.setEditObj((VObjIF)comp);
                        }
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

        /**
        addFocusListener(new FocusAdapter()
        {
            public void focusGained(FocusEvent e)
            {
                if (!inEditMode)
                    m_objTxtMsg.transferFocus();
            }
        });
        **/
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
           if (fv < 2) {
               fv = fgColor.getRed();
               bv = fgColor.getGreen();
               if (fv > 120 && bv > 120)
                   fgColor = Color.black;
               else
                   fgColor = Color.white;
            }
        }
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
        setText(Util.getLabelString(s));
    }

    public void setDefColor(String c) {
        this.fg = c;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if (bg == null)
           setOpaque(s);
        if (s) {
           // Be sure both are cleared out
           removeMouseListener(mlNonEditor);
           removeMouseListener(mlEditor);
           addMouseListener(mlEditor);
           if (font != null) {
                setFont(font);
                fontH = font.getSize();
                rHeight = fontH;
           }
           defDim = getPreferredSize();
           curLoc.x = defLoc.x;
           curLoc.y = defLoc.y;
           curDim.width = defDim.width;
           curDim.height = defDim.height;
        }
        else {
            // Be sure both are cleared out
            removeMouseListener(mlEditor);
            removeMouseListener(mlNonEditor);
            addMouseListener(mlNonEditor);
        }
        inEditMode = s;
        setFocusable(s);
    }

    public void changeFont() {
            font = DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
        fontH = font.getSize();
        rHeight = fontH;
        if (!inEditMode) {
             if ((curDim.height > 0) && (rHeight > curDim.height)) {
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
        case CHVAL:
            return chval;
        case LABEL:
        case VALUE:
            return value;
        case FGCOLOR:
            return fg;
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
        case SETVAL:
            return setVal;
        case SUBTYPE:
            return strSubtype;
        case NUMDIGIT:
            return precision;
        case STATPAR:
            return statusParam;
        case TRACKVIEWPORT:
            return m_viewport;
        case HALIGN:
            return m_alignment ;
        case HELPLINK:
            return m_helplink;
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        if (c != null)
            c = c.trim();
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case CHVAL:
            chval=c;
            break;
        case LABEL:
            // NB: Don't set a default value; absence of a value has meaning
            //value=c;
            //setText(c);
            break;
        case FGCOLOR:
            fg = c;
            /**
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            **/
            propertyChange(null);
            repaint();
            break;
        case BGCOLOR:
            bg = c;
            if (c != null) {
                bgColor = VnmrRgb.getColorByName(c);
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
            /*updateValue();*/
            break;
        case SETVAL:
            setVal = c;
            /*updateValue();*/
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
        case STATPAR:
            statusParam = c;
            if (c != null)
                updateStatus(ExpPanel.getStatusValue(statusParam));
            break;
        case TRACKVIEWPORT:
            m_viewport = c;
            break;
        case HALIGN:
            m_alignment = c;
            setHorizontalAlignment(c);
            break;
        case HELPLINK:
            m_helplink = c;
            break;
        }
    }

    private void setHorizontalAlignment(String c) {
        if ("Right".equalsIgnoreCase(c)) {
            setHorizontalAlignment(JTextField.RIGHT);
        } else if ("Center".equalsIgnoreCase(c)) {
            setHorizontalAlignment(JTextField.CENTER);
        } else {
            setHorizontalAlignment(JTextField.LEFT);
        }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            value = pf.value;
            if (precision != null) {
                // Set precision, in case we're getting this direct from pnew
                value = Util.setPrecision(value, precision);
            }
            setText(Util.getLabelString(value));
        }
    }

    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
                int showval= Integer.parseInt(s);
                isValid =showval>=0?true:false;
                isActive =showval>0?true:false;
           }
    }

    public void updateStatus(String msg) {
        if (msg == null || msg.length() == 0 || msg.equals("null") ||
            statusParam == null) {
            return;
        }
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals(statusParam)) {
                value = tok.nextToken("").trim();
                if (value.equals("-")) value = "";
                setText(Util.getLabelString(value));
            }
        }
    }

    public void updateValue() {
        if (m_viewport != null && m_viewport.toLowerCase().startsWith("y")) {
            vnmrIf = Util.getActiveView();
        }
        if (vnmrIf == null)
            return;
        if (showVal != null) {
                vnmrIf.asyncQueryShow(this, showVal);
        }
        else
                isValid=true;
        if (setVal != null && isValid) {
            String expr = FileUtil.expandPaths(setVal);
            vnmrIf.asyncQueryParam(this, expr, precision);
        }
    }


    public void paint(Graphics g) {
        super.paint(g);
        if (!inEditMode)
            return;
        Dimension  psize = getSize();
        if (isEditing) {
            if (isFocused)
                g.setColor(Color.yellow);
            else
                g.setColor(Color.green);
        }
        else
            g.setColor(Color.darkGray);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

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


    public void refresh() {
    }
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
                VObjDropHandler.processDrop(e, this, inEditMode);
    }

    public Object[][] getAttributes()
    {
        VnmrjIF vif = Util.getVjIF();
        if (vif != null)
            rightsList = vif.getRightsList();
        
        if(rightsList != null && rightsList.isApproved("mayedithelpfield"))
            return attributes_H;
        else
            return attributes;
    }

    private final static Object[][] attributes = {
        {new Integer(STATPAR),  "Status parameter to display:"},
        {new Integer(VARIABLE), "Vnmr variables:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(SETVAL),   "Vnmr expression to display:"},
        {new Integer(NUMDIGIT), "Number of digits:"},
    };
    
    private final static Object[][] attributes_H = {
        {new Integer(STATPAR),  "Status parameter to display:"},
        {new Integer(VARIABLE), "Vnmr variables:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(SETVAL),   "Vnmr expression to display:"},
        {new Integer(NUMDIGIT), "Number of digits:"},
        {new Integer(HELPLINK), Util.getLabel("blHelp")}
    };
    
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
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
        if (!inEditMode) {
           if ((w != nWidth) || (h != nHeight) || (h < rHeight)) {
              adjustFont(w, h);
           }
        }
        super.reshape(x, y, w, h);
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

    public void adjustFont(int w, int h) {
        if (h <= 0)
           return;
        int oldH = rHeight;
        if (font == null) {
            font = getFont();
            fontH = font.getSize();
            rHeight = fontH;
        }
        nHeight = h;
        nWidth = w;
        if (fontH > h)
            rHeight = h - 1;
        else
            rHeight = fontH;
        if ((rHeight < 10) && (fontH > 10))
            rHeight = 10;
        if (oldH != rHeight) {
            //Font  curFont = font.deriveFont((float) rHeight);
            Font curFont = DisplayOptions.getFont(font.getName(),font.getStyle(),rHeight);
            setFont(curFont);
        }
        if (rHeight > h)
            rHeight = h;
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
                if(helpstr==null){
                    Container group = getParent();
                    if(group instanceof VGroup)
                        helpstr=((VGroup)group).getAttribute(HELPLINK);
                    if(helpstr==null){
                        Container group2 = group.getParent();
                        if(group2 instanceof VGroup)
                            helpstr=((VGroup)group2).getAttribute(HELPLINK);
                        if(helpstr==null){
                            Container group3 = group2.getParent();
                            if(group3 instanceof VGroup)
                                helpstr=((VGroup)group3).getAttribute(HELPLINK);
                        }
                    }
                }
                // If no help is found, don't put up the menu, just abort
                if(helpstr == null) {
                    return;
                }
                
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
                                  if(group instanceof VGroup)
                                         helpstr=((VGroup)group).getAttribute(HELPLINK);
                                  if(helpstr==null){
                                      Container group2 = group.getParent();
                                      if(group2 instanceof VGroup)
                                             helpstr=((VGroup)group2).getAttribute(HELPLINK);
                                      if(helpstr==null){
                                          Container group3 = group2.getParent();
                                          if(group3 instanceof VGroup)
                                                 helpstr=((VGroup)group3).getAttribute(HELPLINK);
                                      }
                                  }
                            }
                            // Get the ID and display the help content
                            CSH_Util.displayCSHelp(helpstr);
                        }

                    };
                helpMenuItem.addActionListener(alMenuItem);
                    
                Point pt = evt.getPoint();
                helpMenu.show(VTextMsg.this, (int)pt.getX(), (int)pt.getY());

            }
             
        }
    }  /* End CSHMouseAdapter class */

}

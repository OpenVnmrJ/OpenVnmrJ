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
import java.beans.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import java.io.*;
import javax.swing.*;
import javax.swing.border.*;

import vnmr.util.*;
import vnmr.ui.*;

public class VAnnotateBox extends JPanel
  implements VObjIF, VEditIF, VObjDef, SwingConstants,
   DropTargetListener, PropertyChangeListener, VAnnotationIF
{
    public String type = null;
    public String fg = null;
    public String bg = null;
    public String vnmrVar = null;
    public String showVal = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String dockStr = "None";
    public String title = null;
    public String orientStr = "P";
    public String dispSize = "Variable";
    public Color  fgColor = Color.blue;
    public Color  bgColor=null;
    public Font   font = null;
    public Font   curFont = null;
    public boolean bOrientObj = false;

    public  boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    public  boolean showing = false;
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension prefDim = new Dimension(0,0);
    private Dimension actualDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Dimension tmpDim = new Dimension(0,0);
    private MouseAdapter ml;
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private Rectangle annRect = new Rectangle(0, 0, 20, 20); // rect of annotation
    private int width = 0;
    private int height = 0;
    private int nWidth = 0;
    private int nHeight = 0;
    private int fontH = 0;
    private int idNum = 0;
    public  int rows = 2;
    public  int cols = 2;
    public  int annX = 0;
    public  int annY = 0;
    public  int prtX = 0;
    public  int prtY = 0;
    public  int prtW = 0;
    public  int prtH = 0;
    public  int gridNum = 10;
    public  int dockAt = 0;
    public  int orientId = 0;
    private float fontRatio = 1;
    public  float  objRx = 0;
    public  float  objRy = 0;
    public  float  objRw = 0;
    public  float  objRh = 0;
    public  float  gridXGap = 20;
    public  float  gridYGap = 20;
    protected ButtonIF vnmrIf;
    public  VobjEditorIF editor = null;
    public  VDropHandlerIF dropHandler = null;

    public  int grpId;
    public  int layerNum = 1;
    public  int curGrp = 1;
    public  int elasticType = 2;
    public  int orgElasticType = 2;
    public  boolean bGrpRoot;
    public  boolean bPrintMode = false;
    public  boolean bColorPrint = true;
    private JLabel orientLabel = null;
    public  VAnnotateBox grpRoot = null;
    private VAnnotateBox grpVec[] = null;

    private final static String[] layoutStrs = {"Float","Fixed"};
    private final static String[] dockChoice = {"Upper Center", "Lower Center",
           "Middle Right", "Middle Left", "Upper Right", "Upper Left", 
           "Lower Right", "Lower Left", "None"};

    private final static String[] dockOldChoice = {"North", "South",
           "East", "West", "North_East", "North_West",
           "South_East", "South_West", "None"};
    
    private final static int[] dockVal = {NORTH, SOUTH, EAST, WEST,
           NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST, 0};

    private final static String[] m_sizeStr = {"Fit Only", "Variable", "Fixed",
                "Fixed and Fit"};

    private final static Object[][] attributes2 = {
       {new Integer(ELASTIC), "Font Size:", "menu", m_sizeStr}
    };

    public VAnnotateBox(SessionShare sshare, ButtonIF vif, String typ) {
        super();
        this.type = typ;
        this.vnmrIf = vif;
        objRw = 0.1f;
        objRh = 0.1f;
        this.grpId = 0;
        this.bGrpRoot = false;
        setOpaque(false);   // overlapping groups allowed
        setLayout(new vAnnBoxLayout());
        setBackground(Util.getBgColor());
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2) {
                        informEditor();
                        // informEditor(evt);
                    }
                }
            }
        };
        new DropTarget(this, this);
        // DisplayOptions.addChangeListener(this);
    }

    public void setEditor(VobjEditorIF obj) {
        editor = obj;
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
                ((VAnnotationIF)comp).setEditor(obj);
            }
        }
    }

    private void informEditor() {
        if (editor != null) {
             editor.setEditObj(this);
             // editor.setEditObj((VObjIF) e.getSource());
        }
    }

    public void propertyChange(PropertyChangeEvent e)
    {
        if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            if (orientLabel != null)
               orientLabel.setForeground(fgColor);
        }
        if(bg!=null && bg.length()>0 && !bg.equals("transparent") && !bg.equals("default")){
            bgColor=DisplayOptions.getColor(bg);
            setBackground(bgColor);
        }
        else
            setBackground(null);
        changeFont();
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof PropertyChangeListener)) {
              ((PropertyChangeListener) comp).propertyChange(e);
           }
        }
    }


    public int getDockInfo() {
        return dockAt;
    }

    public int getGridNum() {
        return gridNum;
    }

    public void updateValue() {
        if (showVal != null)
            vnmrIf.asyncQueryShow(this, showVal);
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF)
                ((VObjIF)comp).updateValue();
        }
        setBorder();
    }

    public boolean isOrientObj() {
        return bOrientObj;
    }

    public void setOrientAlignment() {
        if (orientLabel == null)
           return;
        orientLabel.setHorizontalAlignment(SwingConstants.CENTER);
        orientLabel.setVerticalAlignment(SwingConstants.CENTER);
        if (dockAt == 0)
           return;
        if (dockAt == WEST) {
            orientLabel.setHorizontalAlignment(SwingConstants.LEFT);
            orientId = 1;
        }
        if (dockAt == EAST) {
            orientLabel.setHorizontalAlignment(SwingConstants.RIGHT);
            orientId = 2;
        }
        if (dockAt == NORTH) {
            orientLabel.setVerticalAlignment(SwingConstants.TOP);
            orientId = 3;
        }
        if (dockAt == SOUTH) {
            orientLabel.setVerticalAlignment(SwingConstants.BOTTOM);
            orientId = 4;
        }
    }

    public void setOrientObj(boolean b) {
        bOrientObj = b;
        if (b) {
           if (orientLabel == null) {
               orientLabel = new JLabel(orientStr);
               orientLabel.setForeground(fgColor);
               setOrientAlignment();
               add(orientLabel);
           }
           else if (orientLabel != null) {
               orientLabel.setText(orientStr);
               if (!orientLabel.isVisible())
                  orientLabel.setVisible(true);
           }
        }
        else {
           if (orientLabel != null)
               orientLabel.setVisible(false);
        }
    }

    private void drawOrientObj(Graphics2D g) {
        FontMetrics fm = getFontMetrics(curFont);
        int w = fm.stringWidth(orientStr);
        int h = curFont.getSize();
        int asc = fm.getAscent();
        int x = 1;
        int y = (prtH - h) / 2 + asc;
        if (dockAt == EAST) {
            x = prtW - w;
        }
        else if (dockAt == SOUTH) {
            x = (prtW - w) / 2;
            y = prtH - h + asc;
        }
        else if (dockAt == NORTH) {
            x = (prtW - w) / 2;
            y = h;
        }
        g.setFont(curFont);
        if (bColorPrint)
            g.setColor(fgColor);
        else
            g.setColor(Color.black);
        g.drawString(orientStr, x, y);
    }

    /** call "destructors" of VObjIF children  */
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF)
                ((VObjIF)comp).destroy();
        }
    }

    public void setDefColor(String c) {
        fg = c;
    }

    public void setDropHandler(VDropHandlerIF c) {
        dropHandler = c;
        int k = getComponentCount();
        for (int i = 0; i < k; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
                ((VAnnotationIF) comp).setDropHandler(c);
            }
        }
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
        annY = x;
    }


    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public boolean getEditMode() {
        return inEditMode;
    }

    public void showNextGrp(boolean forward) {
        if (!bGrpRoot) {
            if (grpRoot != null)
               grpRoot.showNextGrp(forward);
            return;
        }
        if (grpVec == null)
            return;
        int n = -1;
        int k;
        for (k = 0; k < grpVec.length; k++) {
            if (grpVec[k] != null && grpVec[k].isVisible()) {
               n = k + 1;
               grpVec[k].setVisible(false);
            }
        }
        if (n < 0)  n = 0;
        if (n >= layerNum)  n = layerNum;
        if (forward)
            n++;
        else
            n--;
        if (n <= 0)  n = layerNum;
        if (n > layerNum)  n = 1;
        showGrp(n, true);
    }

    public void resetMembers() {
    }

    public void showGrp(int n, boolean toSet) {
        if (n <= 0)
           n = 1;
        if (n > layerNum)
           return;
        if (grpVec == null || grpVec.length < n)
           return;
        boolean bNew = false;
        VAnnotateBox grp = grpVec[n - 1];
        if (grp == null) {
            grp = new VAnnotateBox(null, vnmrIf, type);
            grp.fgColor = fgColor;
            grp.grpId = n;
            grp.grpRoot = this;
            grp.editor = editor;
            grp.dropHandler = dropHandler;
            grpVec[n-1] = grp;
            bNew = true;
        } 
        grp.dockStr = dockStr;
        grp.rows = rows;
        grp.cols = cols;
        grp.gridNum = gridNum;
        grp.annX = 0;
        grp.annY = 0;
        grp.layerNum = layerNum;
        grp.gridXGap = gridXGap;
        grp.gridYGap = gridYGap;
        grp.dockAt = dockAt;
        grp.objRx = objRx;
        grp.objRy = objRy;
        grp.objRw = objRw;
        grp.objRh = objRh;
        grp.setDock();
        if (bNew) {
           add(grp);
           grp.setEditMode(inEditMode);
        }
        if (inEditMode) {
           for (int k = 0; k < grpVec.length; k++) {
              if (grpVec[k] != null) {
                  grpVec[k].setVisible(false);
              }
           }
           grp.setVisible(true);
        }
        if (toSet) {
           if (editor != null)
             editor.setEditObj((VObjIF) grp);
        }
    }

    private void setGrpCapacity(int num) {
        int k;

        if (grpVec == null || grpVec.length < num) {
            VAnnotateBox newVec[] = new VAnnotateBox[num];
            for (k = 0; k < num; k++)
               newVec[k] = null;
            if (grpVec != null) {
               for (k = 0; k < grpVec.length; k++) {
                  newVec[k] = grpVec[k];
               }
            }
            grpVec = newVec;
        }
    }

   /* this is only called by GroupRoot */
    public void setGrpLayer(VAnnotateBox grp, int n) {
        int k;
        setGrpCapacity(n);
        layerNum = n;
        for (k = n; k < grpVec.length; k++) {
            if (grpVec[k] != null) {
                remove(grpVec[k]);
            }
            grpVec[k] = null;
        }
        for (k = 0; k < grpVec.length; k++) {
            if (grpVec[k] != null)
                grpVec[k].layerNum = layerNum;
        }
        if (grp.grpId > n)
            showGrp(1, true);
    }

    public void setGrpLayer(int n) {
        if (layerNum == n)
            return;
        if (!inEditMode) {
            layerNum = n;
            return;
        }
        if (!isEditing) {
            return;
        }
        if (n > 99)
            n = 99;
        if (grpRoot != null) {
            grpRoot.setGrpLayer(this, n);
            return;
        }
        grpId = 1;
        grpRoot = new VAnnotateBox(null, vnmrIf, type);
        grpRoot.showVal = null;
        grpRoot.dockStr = dockStr;
        grpRoot.rows = rows;
        grpRoot.cols = cols;
        grpRoot.gridNum = gridNum;
        grpRoot.annX = annX;
        grpRoot.annY = annY;
        grpRoot.editor = editor;
        grpRoot.fgColor = fgColor;
        grpRoot.gridXGap = gridXGap;
        grpRoot.gridYGap = gridYGap;
        grpRoot.dockAt = dockAt;
        grpRoot.objRx = objRx;
        grpRoot.objRy = objRy;
        grpRoot.objRw = objRw;
        grpRoot.objRh = objRh;
        grpRoot.dropHandler = dropHandler;

        grpRoot.bGrpRoot = true;
        grpRoot.layerNum = n;
        Rectangle r = getBounds();
        grpRoot.setBounds(r);
        Container parent = getParent();
        parent.remove(this);
        r.x = 0;
        r.y = 0;
        // setBounds(r);
        parent.add(grpRoot);
        grpRoot.add(this);
        grpRoot.setEditMode(inEditMode);
        grpRoot.validate();
    }

    public void addGroup(VAnnotateBox obj) {
        if (obj.grpId < 1)
           return;
        int n;
        boolean bSpace = false;
        setGrpCapacity(layerNum);
        obj.grpRoot = this;
        bGrpRoot = true;
        for (n = 0; n < layerNum; n++) {
           if (obj == grpVec[n]) 
              return;
        }
        for (n = 0; n < layerNum; n++) {
           if (grpVec[n] == null) {
               bSpace = true;
               break;
           }
        }
        if (!bSpace) {
            layerNum++;
            setGrpCapacity(layerNum);
        }
        for (n = 0; n < layerNum; n++) {
            if (grpVec[n] == null) {
               grpVec[n] = obj;
               obj.grpId = n + 1;
               break;
            }
        }
    }

    public void removeGroup(VAnnotateBox obj) {
        if (grpVec == null)
           return;
        for (int n = 0; n < grpVec.length; n++) {
           if (obj == grpVec[n])
               grpVec[n] = null;
        }
    }

    public Component add(Component comp, int index) {
        if (comp instanceof VAnnotateBox) {
            addGroup((VAnnotateBox)comp);
        }
        return super.add(comp, index);
    }
    
    public Component add(Component comp) {
        return add(comp, -1);
    }

    public void remove(Component comp) {
        super.remove(comp);
        if (comp instanceof VAnnotateBox)
           removeGroup((VAnnotateBox) comp);
    }
    
    public void setGrpId(int n) {
        if (!inEditMode) {
            grpId = n;
            return;
        }
        if (n < 1 || n > layerNum) {
            if (grpId < 1)
               editor.setAttr(COUNT, Integer.toString(1));
            else
               editor.setAttr(COUNT, Integer.toString(grpId));
            return;
        }
        if (n == grpId)
            return;
        if (!isEditing || !isVisible())
            return;
        if (grpRoot != null) {
            grpRoot.showGrp(n, true);
        }
    }

    public void setEditMode(boolean s)
    {
        boolean oldMode = inEditMode;
        inEditMode = s;
        int k = getComponentCount();
        if (!bOrientObj) {
            for (int i = 0; i < k; i++) {
                Component comp = getComponent(i);
                if (comp instanceof VObjIF) {
                    VObjIF vobj = (VObjIF) comp;
                    vobj.setEditMode(s);
                    ((JComponent)comp).setOpaque(false);
                }
            }
        }
        if (s)
            addMouseListener(ml);
        else
            removeMouseListener(ml);
        if (s) {
            setOpaque(false);
            if (defDim.width <= 0)
                defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            nWidth = defDim.width;
            nHeight = defDim.height;
            xRatio = 1.0;
            yRatio = 1.0;
            if (font == null) {
                font = getFont();
                fontH = font.getSize();
            }
            curFont = font;
            if (bGrpRoot) {
               if (grpVec != null) {
                  for (k = 0; k < grpVec.length; k++) {
                      VAnnotateBox grp = grpVec[k];
                      if (grp != null) {
                          grp.grpId = k + 1;
                      }
                  }
               }
               showGrp(1, false);
            }
        } else {
            setOpaque(false);
            // getActualSize();
            if (oldMode) {
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
        case BGCOLOR:
             return bg;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case LABEL:
            if (bOrientObj)
               return null;
            else
               return title;
        case SHOW:
            return showVal;
        case VARIABLE:
            return vnmrVar;
        case DOCKAT:
            return dockStr;
        case ROW:
            return Integer.toString(rows);
        case COLUMN:
            return Integer.toString(cols);
        case UNITS:
            return Integer.toString(gridNum);
        case LOCX:
            return Integer.toString(annX);
        case LOCY:
            return Integer.toString(annY);
        case DIGITAL:
            return Integer.toString(layerNum);
        case COUNT:
            if (bGrpRoot)
                return Integer.toString(0);
            else {
               if (grpId <= 0)
                  return Integer.toString(1);
               else
                  return Integer.toString(grpId);
            }
        case ORIENTATION:
            if (bOrientObj)
               return orientStr;
            else
               return null;
        case ELASTIC:
            if (bOrientObj)
               return dispSize;
            else
               return null;
        case OBJID:
            if (idNum >0)
               return Integer.toString(idNum);
            else
               return null;
        default:
            return null;
        }
    }

    private int getInt(String s) {
        int n = 0;

        if (s != null) {
           try {
               n = Integer.parseInt(s);
           }
           catch (NumberFormatException e) {
               n = 0;
           }
        }
        return n;
    }

    public void setAttribute(int attr, String c) {
        if (c != null)
             c = c.trim();
        int k;

        switch (attr) {
        case TYPE:
            type = c;
            break;
        case FGCOLOR:
            fg = c;
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
            if (orientLabel != null)
               orientLabel.setForeground(fgColor);
            setBorder();
            repaint();
            break;
        case BGCOLOR:
            bg=c;
            if (c==null || c.length()==0
                || c.equals("default") || c.equals("transparent"))
            {
                bgColor=null;
                setOpaque(false);
            } else {
                bgColor = DisplayOptions.getColor(c);
/*
                setOpaque(true);
*/
            }
            setBackground(bgColor);
            repaint();
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
            setBorder();
            break;
        case VARIABLE:
            vnmrVar = c;
            break;
        case SHOW:
            showVal = c;
            break;
        case ANNOTATION:
            k = getComponentCount();
            for (int i = 0; i < k; i++) {
                Component comp = getComponent(i);
                if (comp instanceof VObjIF) {
                    ((VObjIF) comp).setAttribute(ANNOTATION, c);
                }
            }
            break;
        case DOCKAT:
            dockStr = c;
            setDock();
            if (isEditing) {
               if (grpRoot != null)
                   grpRoot.setAttribute(attr, c);
               if (!bGrpRoot) {
                   if (editor != null)
                     editor.setEditObj((VObjIF) this);
               }
            }
            break;
        case ROW:
            rows = getInt(c);
            if (rows <= 0)
               rows = 1;
            break;
        case COLUMN:
            if (c != null)
               cols = getInt(c);
            if (cols <= 0)
               cols = 1;
            break;
        case UNITS:
            if (c != null)
               gridNum = getInt(c);
            if (gridNum <= 0)
               gridNum = 40;
            break;
        case LOCX:
            annX = getInt(c);
            if (annX < 0)
               annX = 0;
            break;
        case LOCY:
            annY = getInt(c);
            if (annY < 0)
               annY = 0;
            break;
        case DIGITAL:
            if (c != null && c.length() > 0 ) {
               k = getInt(c);
               if (k <= 0)
                  k = 1;
               setGrpLayer(k);
            }
            break;
        case COUNT:
            if (c != null && c.length() > 0 ) {
               k = getInt(c);
               if (k < 0)
                  k = 0;
               setGrpId(k);
            }
            break;
        case ORIENTATION:
            orientStr = c;
            if (orientStr != null)
               setOrientObj(true);
            else
               setOrientObj(false);
            break;
        case ELASTIC:
            dispSize = c;
	    elasticType = 2;
	    if (c != null) {
                if (c.equals(m_sizeStr[0]))
		    elasticType = 1;
                else if (c.equals(m_sizeStr[2]))
		    elasticType = 3;
                else if (c.equals(m_sizeStr[3]))
		    elasticType = 4;
	    }
	    orgElasticType = elasticType;
            break;
        case OBJID:
            idNum = getInt(c);
            if (idNum < 0)
               idNum = 0;
            break;
       }
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        vnmrIf = vif;
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setVnmrIF(vif);
            }
        }
    }

    public void setValue(ParamIF pf) { }

    public void setShowValue(ParamIF pf) {
       if (pf != null && pf.value != null && !inEditMode) {
            String  s = pf.value.trim();
            boolean oldshow=showing;
            showing =Integer.parseInt(s)>0?true:false;
            setVisible(showing);
        }
    }


    public void changeFocus(boolean s) {
        isFocused = s;
    }

    public void setId(int n) {
        idNum = n;
    }

    public int getId() {
        return idNum;
    }


    public void paint(Graphics g) {
        super.paint(g);
/*
        if (!inEditMode)
            return;
*/
        Dimension  psize = getSize();
        if (isEditing) {
            if (isFocused)
                g.setColor(Color.yellow);
            else
                g.setColor(Color.green);
        }
        else
            g.setColor(Color.blue);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    public void draw(Graphics2D g, int offX, int offY) {
        if (!isVisible())
            return;
        int x = curLoc.x + offX;
        int y = curLoc.y + offY;
       /*
        if (bPrintMode) {
            x = offX + prtX;
            y = offY + prtY;
        }
        */
        if (bOrientObj) {
            if (orientLabel != null && orientLabel.isVisible()) {
                g.translate(x, y);
                /***
                if (bPrintMode) {
                    drawOrientObj(g);
                }
                else
                ***/
                    orientLabel.paint(g);
                g.translate(-x, -y);
            }
            return;
        }
        int k = getComponentCount();
        for (int i = 0; i < k; i++) {
             Component comp = getComponent(i);
             if (comp instanceof VAnnotationIF)
                   ((VAnnotationIF)comp).draw(g, x, y);
             else {
                   g.translate(x, y);
                   comp.paint(g);
                   g.translate(-x, -y);
             }
        }
    }

    public void setPrintMode(boolean b) {
        int k = getComponentCount();
        if (b)
            elasticType = 2;
        else {
            bColorPrint = true;
            elasticType = orgElasticType;
            if (orientLabel != null)
               orientLabel.setForeground(fgColor);
        }
        for (int i = 0; i < k; i++) {
             Component comp = getComponent(i);
             if (comp instanceof VAnnotationIF)
                 ((VAnnotationIF)comp).setPrintMode(b);
        }
        /*
        if (!b && bPrintMode) {
            if (bOrientObj) {
               if (orientLabel != null && orientLabel.isVisible())
                  orientLabel.setFont(curFont);
            }
        }
        */
        bPrintMode = b;
    }

    public void setPrintColor(boolean b) {
        bColorPrint = b;
        for (int i = 0; i < getComponentCount(); i++) {
             Component comp = getComponent(i);
             if (comp instanceof VAnnotationIF)
                 ((VAnnotationIF)comp).setPrintColor(b);
        }
        if (orientLabel != null) {
             if (b)
               orientLabel.setForeground(fgColor);
             else
               orientLabel.setForeground(Color.black);
        }
    }

    public void setPrintRatio(float rw, float rh) {
        prtX = (int) ((float)curLoc.x * rw);
        prtY = (int) ((float)curLoc.y * rh);
        prtW = (int) ((float)curDim.width * rw);
        prtH = (int) ((float)curDim.height * rh);
        int k = getComponentCount();
        for (int i = 0; i < k; i++) {
             Component comp = getComponent(i);
             if (comp instanceof VAnnotationIF)
                 ((VAnnotationIF)comp).setPrintRatio(rw, rh);
        }
        if (!bPrintMode || !bOrientObj)
             return;
        if (orientLabel != null && orientLabel.isVisible()) {
             float s;
             if (rw > rh)
                 s = rh;
             else
                 s = rw;
             curFont = orientLabel.getFont();
             s = ((float) curFont.getSize() * s);
             curFont = curFont.deriveFont(s);
        }
    }

    public void objPrintf(PrintWriter f, int gap) {
    }

    public void adjustRowCol(int r, int c) {
        int x = 0;
        int y = 0;
        int cg;
        boolean oddGridNum = false;
        if ((gridNum / 2) != ((gridNum + 1) / 2))
           oddGridNum = true;
        
        switch (dockAt) {
          case NORTH:
                     cg = c / 2;
                     if (oddGridNum) {
                        x = cg - cols / 2;
                        cols = (cols / 2) * 2 + 1;
                     }
                     else {
                        cols = ((cols + 1) / 2) * 2;
                        x = cg - cols / 2;
                     }
                     break;
          case NORTH_WEST:
                     break;
          case NORTH_EAST:
                     x = gridNum - cols;
                     break;
          case EAST:
                     x = gridNum - cols;
          case WEST:
                     cg = r / 2;
                     if (oddGridNum) {
                        y = cg - rows / 2;
                        rows = (rows / 2) * 2 + 1;
                     }
                     else {
                        rows = ((rows + 1) / 2) * 2;
                        y = cg - rows / 2;
                     }
                     break;
          case SOUTH:
                     cg = c / 2;
                     if (oddGridNum) {
                        x = cg - cols / 2;
                        cols = (cols / 2) * 2 + 1;
                     }
                     else {
                        cols = ((cols + 1) / 2) * 2;
                        x = cg - cols / 2;
                     }
                     y = gridNum - rows;
                     break;
          case SOUTH_WEST:
                     y = gridNum - rows;
                     break;
          case SOUTH_EAST:
                     y = gridNum - rows;
                     x = gridNum - cols;
                     break;
          default:
                     x = annX;
                     y = annY;
                     break;
        }
        annX = x;
        annY = y;
    }

    public void adjustBounds(int r, int c, float xgap, float ygap) {
        int x, y;
        int w, h;
        gridNum = r;
        gridXGap = xgap;
        gridYGap = ygap;
        if (grpId > 0 && grpRoot != null) {
            annX = 0;
            annY = 0;
            rows = r;
            cols = c;
        }
        else {
            if (dockAt != 0)
                adjustRowCol(r, c);
            x = (int) ((float) annX * xgap);
            y = (int) ((float) annY * ygap);
            w = (int) ((float) cols * xgap);
            h = (int) ((float) rows * ygap);
            setBounds(x, y, w, h);
        }
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
               ((VAnnotationIF)comp).adjustBounds(rows, cols, gridXGap, gridYGap);
            }
        }
    }

    public void setGridInfo(int row, int col, float xgap, float ygap) {
        gridNum = row;
        gridXGap = xgap;
        gridYGap = ygap;
    }

    public void adjustBounds() {
        adjustBounds(gridNum, gridNum, gridXGap, gridYGap);
    }

    public void readjustBounds(int r, int c, float xgap, float ygap) {
        gridNum = r;
        gridXGap = xgap;
        gridYGap = ygap;
        annX = (int) ((float) annRect.x / xgap + 0.5f);
        annY = (int) ((float) annRect.y / ygap + 0.5f);
        cols = (int) ((float) annRect.width / xgap + 0.5f);
        rows = (int) ((float) annRect.height / ygap + 0.5f);
 
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
        if (dockAt != 0)
            adjustRowCol(r, c);
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
               ((VAnnotationIF)comp).readjustBounds(rows, cols, xgap, ygap);
            }
        }
    }

    public void readjustBounds() {
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
               ((VAnnotationIF)comp).readjustBounds(rows, cols, gridXGap, gridYGap);
               ((VAnnotationIF)comp).adjustBounds();
            }
        }
    }

    public void setBoundsRatio(boolean all) {
        Dimension dim = getParent().getSize();
        Rectangle r = getBounds();
        objRx = ((float) r.x / (float) dim.width);
        objRy = ((float) r.y / (float) dim.height);
        objRw = ((float) r.width / (float) dim.width);
        objRh = ((float) r.height / (float) dim.height);

        annX = (int) ((float) r.x / gridXGap + 0.5f);
        annY = (int) ((float) r.y / gridYGap + 0.5f);
        cols = (int) ((float) r.width / gridXGap + 0.5f);
        rows = (int) ((float) r.height / gridYGap + 0.5f);
        annRect.x = r.x;
        annRect.y = r.y;
        annRect.width = r.width;
        annRect.height = r.height;

        if (all) {
            for (int i = 0; i < getComponentCount(); i++) {
                Component comp = getComponent(i);
                if (comp instanceof VAnnotationIF) {
                   ((VAnnotationIF)comp).setBoundsRatio(all);
                }
            }
        }
        else if (bGrpRoot) {
            if (grpVec != null) {
               for (int k = 0; k < grpVec.length; k++) {
                   if (grpVec[k] != null) 
                       grpVec[k].setBoundsRatio(all);
               }
            }
        }
    }

    public void setDock() {
         int old = dockAt;
         dockAt = 0;
         if (dockStr != null) {
             for (int k = 0; k < dockChoice.length; k++) {
                 if (dockStr.equals(dockChoice[k])) {
                     dockAt = dockVal[k];
                     break;
                 }
             }
             if (dockAt == 0) {
                for (int k = 0; k < dockOldChoice.length; k++) {
                    if (dockStr.equals(dockOldChoice[k])) {
                        dockAt = dockVal[k];
                        dockStr = dockChoice[k];
                        break;
                    }
                }
             }
         }
         if (dockAt != old && isShowing()) {
             adjustBounds();
         }
         if (bGrpRoot && grpVec != null) {
             for (int k = 0; k < grpVec.length; k++) {
                 if (grpVec[k] != null) {
                       grpVec[k].dockStr = dockStr;
                       grpVec[k].dockAt = dockAt;
                       grpVec[k].annX = 0;
                       grpVec[k].annY = 0;
                       grpVec[k].cols = cols;
                       grpVec[k].rows = rows;
                 }
             }
         }
         setOrientAlignment();
    }

    public void refresh() {}
    public void setDefLabel(String s) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void changeFont()
    {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        if (orientLabel != null)
           orientLabel.setFont(font);
        curFont = font;
        fontH = font.getSize();
        setBorder();
        validate();
        repaint();
    }

    class vAnnBoxLayout implements LayoutManager {
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
                if  (comp != null) {
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
        } //preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0);
        } //minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
                Dimension size = target.getSize();
                int count = target.getComponentCount();
                if (bOrientObj && orientLabel != null)
                     orientLabel.setBounds(0, 0, size.width, size.height);
                if (grpVec != null) {
                    for (int k = 0; k < grpVec.length; k++) {
                        if (grpVec[k] != null) 
                            grpVec[k].setBounds(0, 0, size.width, size.height);
                    }
                }
                for (int i = 0; i < count; i++) {
                    Component comp = target.getComponent(i);
                    if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                        ((VAnnotationIF)comp).adjustBounds(rows, cols,gridXGap,gridYGap);
                    }
                }
            }
        }
    } // vAnnBoxLayout

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        if (dropHandler != null)
            dropHandler.processDrop(e, this, inEditMode);
        else
            VObjDropHandler.processDrop(e, this,inEditMode);
    } // drop

    private final static String[] bdrTypes =
       {"None", "Etched", "RaisedBevel", "LoweredBevel"};
    private final static String[] ttlPosn =
       {"Top","AboveTop","BelowTop","Bottom","AboveBottom","BelowBottom"};
    private final static String[] ttlJust = {"Left","Right","Center"};
    private final static Object[][] attributes = {
       {new Integer(DIGITAL),      "Number of Layers:"},
       {new Integer(COUNT),        "Edit Layer:"},
       {new Integer(SHOW),         "Show condition:"},
       {new Integer(DOCKAT),      "Dock at:", "menu", dockChoice}
    };


    public Object[][] getAttributes() {
        if (bOrientObj)
            return attributes2;
        else
            return attributes;
    }

    private void setBorder()
    { }

    public void setModalMode(boolean s) { }
    public void sendVnmrCmd() {}

    public Dimension getActualSize()
    {
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
        actualDim.width = w + 1;
        actualDim.height = h + 1;
        if (!inEditMode) {
            if (!isPreferredSizeSet()) {
                defDim = getPreferredSize();
                setPreferredSize(defDim);
            }
            if ((defDim.width < w) || (defDim.height < h)) {
                if (defDim.width > 0) {
                    if (defDim.width < w)
                        defDim.width = w + 2;
                    if (defDim.height < h)
                        defDim.height = h + 2;
                    setPreferredSize(defDim);
                }
            }
            if (isOpaque()) {
                if (defDim.width <= 0)
                    defDim = getPreferredSize();
                if (actualDim.width < defDim.width)
                    actualDim.width = defDim.width;
                if (actualDim.height < defDim.height)
                    actualDim.height = defDim.height;
            }
        }
        return actualDim;
    }

    public Dimension getRealSize()
    {
        int w = 0;
        int h = 0;
        Dimension dim;
        Point pt;
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
        tmpDim.width = w + 1;
        tmpDim.height = h + 1;

        return tmpDim;
    }

    public double getXRatio()
    {
        return xRatio;
    }

    public double getYRatio()
    {
        return yRatio;
    }

    public void setSizeRatio(double rx, double ry)
    {
    }

    public void resizeAll()
    {
        double oldx = xRatio;
        double oldy = yRatio;
        xRatio = 0.0;
        setSizeRatio(oldx, oldy);
        /*
         * int count = getComponentCount(); for (int i = 0; i < count; i++) {
         * Component comp = getComponent(i); if (comp instanceof VObjIF) {
         * ((VObjIF) comp).setSizeRatio(xRatio, yRatio); } }
         */
    }

    public void setDefLoc(int x, int y) {
        defLoc.x = x;
        defLoc.y = y;
        super.setLocation(x, y);
    }

    public Point getDefLoc()
    {
        tmpLoc.x = defLoc.x;
        tmpLoc.y = defLoc.y;
        return tmpLoc;
    }

    private void adjustFont()
    {
        if (curDim.width <= 0)
            return;
        if (font == null)
            changeFont();
        String str = title;
        if (bOrientObj)
            str = orientStr;
        if (str == null)
            return;
        float s = 1.0f;
        
        if (bPrintMode) {
             if (fontH > 0)
                 s = (float) curDim.height / (float) fontH;
        }
        nWidth = curDim.width;
        nHeight = curDim.height;
        if (fontH <= 0)
            fontH = 12;
        if (bOrientObj) {
            if (orientLabel == null)
                return;
            if (elasticType > 2) { // fixed font
                if (elasticType == 4) { // fit only
                    if (nHeight < fontH) {
                        if (orientLabel.isVisible())
                            orientLabel.setVisible(false);
                        return;
                    }
                }
                if (!orientLabel.isVisible())
                    orientLabel.setVisible(true);
                return;
            }
        }
        if (bPrintMode) {
            if (s > 1.5f)
                s = s * 0.8f;
            s = (float) fontH * s; 
        }
        else {
           if (fontH < nHeight)
               s = (float) fontH;
           else
               s = (float) nHeight;
        }
        if ((s < 10) && (fontH >= 10))
           s = 10;
        curFont = font.deriveFont(s);
        int w = nWidth;
        int h = 0;
        while (s >= 7) {
            FontMetrics fm2 = getFontMetrics(curFont);
            w = fm2.stringWidth(str);
            h = curFont.getSize();
            if (w < nWidth && h < nHeight)
                break;
            if (s < 8)
                break;
            s--;
            curFont = curFont.deriveFont(s);
        }
        if (bOrientObj && orientLabel != null) {
            orientLabel.setFont(curFont);
            if (elasticType == 1) { // fit only
                if (h >= nHeight) {
                    orientLabel.setVisible(false);
                    return;
                }
            }		
            if (!orientLabel.isVisible())
               orientLabel.setVisible(true);
        }
    }

    public void reshape(int x, int y, int w, int h)
    {
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
            if (!inEditMode || bOrientObj) {
                 adjustFont();
            }
            if (bOrientObj && !inEditMode) {
                if (elasticType == 3) {
                    if (h <= fontH)
                        h = fontH + 2;
                    if (w < fontH)
                        w = fontH;
                }
            }
        }
        super.reshape(x, y, w, h);
    }

    public Point getLocation()
    {
        if (inEditMode) {
            tmpLoc.x = defLoc.x;
            tmpLoc.y = defLoc.y;
        } else {
            tmpLoc.x = curLoc.x;
            tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }
}


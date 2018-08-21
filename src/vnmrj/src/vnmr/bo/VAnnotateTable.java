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

public class VAnnotateTable extends JPanel
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
    public Color  fgColor = Color.blue;
    public Color  bgColor=null;
    public Font   font = null;
    public Font   curFont = null;

    public  boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean bPrintMode = false;
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    private Dimension defDim = new Dimension(0,0);
    private Dimension prefDim = new Dimension(0,0);
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
    private int fontH = 10;
    public  int rows = 2;
    public  int cols = 2;
    public  int numRows = 2;
    public  int numCols = 2;
    public  int annX = 0;
    public  int annY = 0;
    public  int gridNum = 10;
    public  int dockAt = 0;
    public  int prtX = 0;
    public  int prtY = 0;
    public  float  gridXGap = 20;
    public  float  gridYGap = 20;
    public  float  printRw = 1;
    public  float  printRh = 1;
    public  int  rowGap = 10;
    public  int  colGap = 10;
    public  int  idNum = 0;
    public  int  tableWidth = 20;
    public  int  tableHeight = 10;
    private int[] rws = null;
    private int[] rhs = null;
    private int[] xws = null;
    private int[] xhs = null;
    private int[] prtW = null;
    private int[] prtH = null;
    protected ButtonIF vnmrIf;
    public  VobjEditorIF editor = null;
    public  VDropHandlerIF dropHandler = null;

    private VAnnotationIF vArray[][] = null;
    private VAnnotationIF nArray[][] = null;

    private final static String[] dockChoice = {"Upper Center", "Lower Center",
           "Middle Right", "Middle Left", "Upper Right", "Upper Left", 
           "Lower Right", "Lower Left", "None"};

    private final static String[] dockOldChoice = {"North", "South",
           "East", "West", "North_East", "North_West",
           "South_East", "South_West", "None"};
    
    private final static int[] dockVal = {NORTH, SOUTH, EAST, WEST,
           NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST, 0};

    public VAnnotateTable(SessionShare sshare, ButtonIF vif, String typ) {
        super();
        this.type = typ;
        this.vnmrIf = vif;
        setOpaque(false);   // overlapping groups allowed
        setLayout(new vAnnTableLayout());
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
        vArray = new VAnnotationIF[numRows][numCols];
        for (int r = 0; r < numRows; r++) {
	    for (int c = 0; c < numCols; c++)
               vArray[r][c] = null;
        }
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

    public void setId(int n) {
        idNum = n;
    }

    public int getId() {
        return idNum;
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

    /** call "destructors" of VObjIF children  */
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF)
                ((VObjIF)comp).destroy();
        }
        vArray = null;
        nArray = null;
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

    public void resetMembers() {
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

    public Component add(Component comp, int index) {
        if (!(comp instanceof VLabel)) {
            return null;
        }
        return super.add(comp, index);
    }
    
    public Component add(Component comp) {
        if (!(comp instanceof VLabel))
            return null;
        VObjIF vobj = (VObjIF) comp;
        int r = 0;
        int c = 0;
        vobj.setAttribute(ANNOTATION, "yes");
        String a = vobj.getAttribute(LOCX);
        if (a != null)
	    c =  getInt(a);
        a = vobj.getAttribute(LOCY);
        if (a != null)
	    r =  getInt(a);
        VAnnotationIF obj = (VAnnotationIF) comp;
        if (numRows <= r)
            changeRows(r+1);
        if (numCols <= c)
            changeCols(c+1);
        if (vArray[r][c] != null) {
           //  if (vArray[r][c] != obj)
                remove((Component) vArray[r][c]);
        }
        vArray[r][c] = obj;
        obj.setRowCol(1, 1);
        obj.setXPosition(c, r);
        obj.setGridInfo(numRows, numCols, (float)colGap, (float)rowGap);
        obj.adjustBounds();
        add(comp, -1);
        validate();
        return comp;
    }

    public void remove(Component comp) {
        super.remove(comp);
        if (!(comp instanceof VLabel))
	    return;
        VObjIF vobj = (VObjIF) comp;
        for (int r = 0; r < numRows; r++) {
            for (int c = 0; c < numCols; c++) {
                if (vArray[r][c] == comp) {
                   vArray[r][c] = null;
                   return;
                }
            }
        }
    }
    

    public void setEditMode(boolean s)
    {
        boolean oldMode = inEditMode;
        inEditMode = s;
        int k = getComponentCount();
        for (int i = 0; i < k; i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF vobj = (VObjIF) comp;
                vobj.setEditMode(s);
                ((JComponent)comp).setOpaque(false);
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
            }
            curFont = font;
        } else {
            setOpaque(false);
            if (oldMode) {
                revalidate();
            }
        }
    }

    private void changeRows(int newRows) {
        if (newRows == numRows)
	    return;
        if (vArray == null)
	    return;
        int r, c;
        if (newRows < numRows) {
            for (r = newRows; r < numRows; r++) {
               for (c = 0; c < numCols; c++) {
                   if (vArray[r][c] != null) {
                       Component comp = (Component) vArray[r][c];
                       remove(comp);
                       vArray[r][c] = null;
                   }
               }
            }
            numRows = newRows;
            return;
        }
        nArray = new VAnnotationIF[newRows][numCols];
        for (r = 0; r < numRows; r++) {
            for (c = 0; c < numCols; c++)
                nArray[r][c] = vArray[r][c];
        }
        for (r = numRows; r < newRows; r++) {
            for (c = 0; c < numCols; c++)
                nArray[r][c] = null;
        }
        numRows = newRows;
        vArray = nArray;
        if (isShowing())
            revalidate();
    }

    private void changeCols(int newCols) {
        if (newCols == numCols)
	    return;
        if (vArray == null)
            return;
        int r, c;
        if (newCols < numCols) {
            for (c = newCols; c < numCols; c++) {
               for (r = 0; r < numRows; r++) {
                   if (vArray[r][c] != null) {
                       Component comp = (Component) vArray[r][c];
                       remove(comp);
                       vArray[r][c] = null;
                   }
               }
            }
            numCols = newCols;
            return;
        }
        nArray = new VAnnotationIF[numRows][newCols];
        for (c = 0; c < numCols; c++) {
            for (r = 0; r < numRows; r++)
                nArray[r][c] = vArray[r][c];
        }
        for (c = numCols; c < newCols; c++) {
            for (r = 0; r < numRows; r++)
                nArray[r][c] = null;
        }
        numCols = newCols;
        vArray = nArray;
        if (isShowing())
            revalidate();
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
        case ROWS:
             return Integer.toString(numRows);
        case COLUMNS:
             return Integer.toString(numCols);
        case OBJID:
             if (idNum > 0)
                 return Integer.toString(idNum);
             else 
                 return null;
        default:
            return null;
        }
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
               if (editor != null)
                     editor.setEditObj((VObjIF) this);
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
        case ROWS:
            k = 1; 
            if (c != null && c.length() > 0 )
               k = getInt(c);
            if (k < 1)
               k = 1;
            changeRows(k);
            repaint();
            break;
        case COLUMNS:
            k = 1; 
            if (c != null && c.length() > 0 )
               k = getInt(c);
            if (k < 1)
               k = 1;
            changeCols(k);
            repaint();
            break;
        case OBJID:
            if (c != null)
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
            boolean showing;
            showing =Integer.parseInt(s)>0?true:false;
            setVisible(showing);
        }
    }


    public void changeFocus(boolean s) {
        isFocused = s;
    }

    public boolean hasObjAt(int x, int y) {
        int r, c;
        r = y / rowGap;
        c = x / colGap;
        if (r < 0) r = 0;
        if (c < 0) c = 0;
        if (r >= numRows || c >= numCols)
            return true;
        if (vArray[r][c] != null)
            return true;
        return false;
    }

    public boolean addDropObj(Component comp, int x, int y) {
        if (!(comp instanceof VLabel))
             return false;
        VAnnotationIF obj = (VAnnotationIF) comp;
        int r, c;
        r = y / rowGap;
        c = x / colGap;
        if (r < 0) r = 0;
        if (c < 0) c = 0;
        if (r >= numRows || c >= numCols)
            return false;
        obj.setRowCol(1, 1); // set the size of obj
        obj.setXPosition(c, r);
        obj.setGridInfo(numRows, numCols, (float)colGap, (float)rowGap);
        add(comp);
        return true;
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
            g.setColor(Color.blue);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
        int x = colGap;
        int y = rowGap;
        int k;
        for (k = 1; k < numRows; k++) {
            g.drawLine(0, y, psize.width-1, y);
            y += rowGap;
        }
        for (k = 1; k < numCols; k++) {
            g.drawLine(x, 0, x, psize.height-1);
            x += colGap;
        }
    }

    public void draw(Graphics2D g, int offX, int offY) {
        if (!isVisible())
            return;
        int x = offX + curLoc.x;	
        int y = offY + curLoc.y;	
        /**
        if (bPrintMode) {
            x = offX + prtX;  
            y = offY + prtY;  
        }
        **/
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
                ((VAnnotationIF)comp).draw(g, x, y);
            }
        }
    }


    public void setPrintMode(boolean s) {
        bPrintMode = s;
        if (!s) {
            printRw = 1;
            printRh = 1;
        }
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
                ((VAnnotationIF)comp).setPrintMode(s);
            }
        }
    }

    public void setPrintColor(boolean s) {
        for (int i = 0; i < getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VAnnotationIF) {
                ((VAnnotationIF)comp).setPrintColor(s);
            }
        }
    }

    public void setPrintRatio(float rw, float rh) {
        if (printRw != rw || printRh != rh) {
            printRw = rw;
            printRh = rh;
            printLayoutTable();
        }
    }

    public void objPrintf(PrintWriter f, int gap) {
    }

    public void adjustRowCol(int r, int c) {
        int x = 0;
        int y = 0;
        int cg;
        boolean oddGridNum = false;
        
        // if (bPrintMode)
        //     return;
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
        if (dockAt != 0)
            adjustRowCol(r, c);
       
        x = (int) ((float) annX * xgap);
        y = (int) ((float) annY * ygap);
        w = (int) ((float) cols * xgap);
        h = (int) ((float) rows * ygap);
        setBounds(x, y, w, h);
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
    }

    public void readjustBounds() {
    }

    public void setBoundsRatio(boolean all) {
        Dimension dim = getParent().getSize();
        Rectangle r = getBounds();

        annX = (int) ((float) r.x / gridXGap + 0.5f);
        annY = (int) ((float) r.y / gridYGap + 0.5f);
        cols = (int) ((float) r.width / gridXGap + 0.5f);
        rows = (int) ((float) r.height / gridYGap + 0.5f);
        annRect.x = r.x;
        annRect.y = r.y;
        annRect.width = r.width;
        annRect.height = r.height;
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
    }

    private void printLayoutTable() {
        if (rws == null || rhs == null)
            return;
        int r, c, dw, dh, x, x0, y, y0;
        int fh = 0;
        VLabel vobj;
        JComponent comp;
        x0 = 0;
        y0 = 0;
        prtX = (int) ((float)curLoc.x * printRw);
        prtY = (int) ((float)curLoc.y * printRh);
        for (r = 0; r < numRows; r++) {
            comp = (JComponent) vArray[r][0];
            if (comp != null && (comp instanceof VLabel)) {
               vobj = (VLabel) comp;
               Rectangle r1 = comp.getBounds();
               x0 = (int) ((float)r1.x * printRw);
               if (fh == 0)
                   fh = vobj.getFont().getSize();
               break;
            }
        }
        for (c = 0; c < numCols; c++) {
            comp = (JComponent) vArray[0][c];
            if (comp != null && (comp instanceof VLabel)) {
               vobj = (VLabel) comp;
               Rectangle r2 = comp.getBounds();
               y0 = (int) ((float)r2.y * printRh);
               if (fh == 0)
                   fh = vobj.getFont().getSize();
               break;
            }
        }
        y = y0;
        if (prtW == null) {
             prtW = new int[numCols];
             prtH = new int[numRows];
        }
        for (r = 0; r < numRows; r++)
            prtH[r] = (int) ((float)rhs[r] * printRh);
        for (c = 0; c < numCols; c++)
            prtW[c] = (int) ((float)rws[c] * printRw);

        for (r = 0; r < numRows; r++) {
            x = x0;
            for (c = 0; c < numCols; c++) {
                 comp = (JComponent) vArray[r][c];
                 if (comp != null && (comp instanceof VLabel)) {
                      vobj = (VLabel) comp;
                      vobj.changeShape(x, y, prtW[c], prtH[r]);
                 }
                 x += prtW[c];
            }
            y += prtH[r];
        }
        if (printRw > printRh)
            fh = (int) (float) (fh * printRw);
        else
            fh = (int) (float) (fh * printRh);
        dw = 6;
        dh = 6;
        while (dw > 1 || dh > 1) {
            dw = 0;
            dh = 0;
            fh -= 1;
            if (fh < 7)
                break;
            for (r = 0; r < numRows; r++) {
                for (c = 0; c < numCols; c++) {
                    comp = (JComponent) vArray[r][c];
                    if (comp != null && (comp instanceof VLabel)) {
                        vobj = (VLabel) comp;
                        vobj.changeFontSize(fh);
                        dw = vobj.curWidth - prtW[c];
                        dh = vobj.curHeight - prtH[r];
                     }
                     if (dw > 0 || dh > 0)
                         break;
                 }
                 if (dw > 0 || dh > 0)
                     break;
             }
         }
    }

    private void layoutPrtTable() {
        if (rws == null || rhs == null)
            return;
        Dimension size = getSize();
        float fw = (float) size.width / (float) tableWidth;
        float fh = (float) size.height / (float) tableHeight;
        int x, y, w, h, r, c;
        int x0, y0;
        VLabel vobj;
        JComponent comp;

        w = 2;
        h = 2; 
        for (r = 0; r < numRows; r++) {
            x = (int) ((float) rhs[r] * fh);
            h += x;
        }
        for (c = 0; c < numCols; c++) {
            x = (int) ((float) rws[c] * fw);
            w += x;
        }
        y0 = 0;
        x0 = 0;
        if (dockAt > 0) {
            switch (dockAt) {
              case NORTH:
                          x0 = (size.width - w) / 2;
                          break;
              case NORTH_EAST:
                          x0 = size.width - w;
                          break;
              case EAST:
                          x0 = size.width - w;
                          y0 = (size.height - h) / 2;
                          break;
              case WEST:
                          y0 = (size.height - h) / 2;
                          break;
              case SOUTH:
                          x0 = (size.width - w) / 2;
                          y0 = size.height - h;
                          break;
              case SOUTH_WEST:
                          y0 = size.height - h;
                          break;
              case SOUTH_EAST:
                          x0 = size.width - w;
                          y0 = size.height - h;
                          break;
            }
            if (x0 < 0)
                x0 = 0;
            if (y0 < 0)
                y0 = 0;
        }
        y = y0;
        for (r = 0; r < numRows; r++) {
            x = x0;
            h = (int) ((float) rhs[r] * fh);
            for (c = 0; c < numCols; c++) {
                 comp = (JComponent) vArray[r][c];
                 w = (int) ((float) rws[c] * fw);
                 if (comp != null && (comp instanceof VLabel)) {
                      vobj = (VLabel) comp;
                      // vobj.setBounds(x, y, w, h);
                      vobj.adjustFont(w, h);
                 }
                 x += w;
            }
            y += h;
        }
    }

    public void layoutTable() {
        Dimension size = getSize();
        int x, y, x0, y0, mh;
        int r, c, tw, th, dw, dh;
        int fh;
        JComponent comp;
        VLabel vobj;
        boolean adjustable = false;
 
        dw = size.width / numCols;
        dh = size.height / numRows;
        if (dw < 6 || dh < 7) {
            doLayout();
            return;
        }
        /***
        if (bPrintMode) {
            layoutPrtTable();
            return;
        }
        ***/
        tableWidth = size.width;
        tableHeight = size.height;
        if (rws == null) {
             rws = new int[numCols];
             xws = new int[numCols];
             rhs = new int[numRows];
             xhs = new int[numRows];
        }
        for (c = 0; c < numCols; c++) {
            rws[c] = 0;
            xws[c] = 0;
        }
        for (c = 0; c < numRows; c++) {
            rhs[c] = 0;
            xhs[c] = 0;
        }
        fh = 6;
        for (r = 0; r < numRows; r++) {
            for (c = 0; c < numCols; c++) {
                 comp = (JComponent) vArray[r][c];
                 // if (comp != null && (comp instanceof VLabel)) {
                 if (comp != null) {  // table contains VLabel only
                     vobj = (VLabel) comp;
                     if (bPrintMode) // print resolution may be higher than screeen's
                         vobj.adjustFont(dw, dh);
                     else
                         vobj.calSize();
                     if (vobj.curWidth > rws[c])
                         rws[c] = vobj.curWidth;
                     if (vobj.curHeight > rhs[r])
                         rhs[r] = vobj.curHeight;
                     if (vobj.font != null) {
                         vobj.setFont(vobj.font);
                         vobj.fontHeight = vobj.font.getSize();
                     }
                     if (vobj.elasticType <= 2) { // font size is not fixed
                         if (vobj.fontHeight > fh)
                             fh = vobj.fontHeight;
                     }
                     else {
                         if (vobj.curWidth > xws[c])
                             xws[c] = vobj.curWidth;
                         if (vobj.curHeight > xhs[r])
                             xhs[r] = vobj.curHeight;
                     }
                 }
            }
        }
        fontH = fh;
        tw = 0;
        for (c = 0; c < numCols; c++) {
            tw = tw + rws[c];
            if (rws[c] > xws[c])
               adjustable = true;
        }
        th = 0;
        mh = 1;
        for (c = 0; c < numRows; c++) {
            th = th + rhs[c];
            if (rhs[c] > xhs[c]) {
               adjustable = true;
               mh++;
            }
        }
        dw = tw - size.width;
        dh = th - size.height;
        if (dh > 0 && fh > 8) {
            x = dh / mh;
            if (x < 1)   x = 1;
            if (x > 4)   x = 4;
            fh = fh - x + 1;
            if (fh < 8)
                fh = 8;
        }
        while (adjustable && (dw > 4 || dh > 4)) {
            fh -= 1; 
            if (fh < 7)
                break;
            for (c = 0; c < numCols; c++)
                rws[c] = 0;
            for (r = 0; r < numRows; r++)
                rhs[r] = 0;
            for (r = 0; r < numRows; r++) {
                for (c = 0; c < numCols; c++) {
                    comp = (JComponent) vArray[r][c];
                    if (comp != null && (comp instanceof VLabel)) {
                        vobj = (VLabel) comp;
                        if (vobj.curWidth > xws[c] || vobj.curHeight > xhs[r]) {
                             if (vobj.fontHeight > fh)
                                 vobj.changeFontSize(fh);
                        }
                        if (vobj.curWidth > rws[c])
                            rws[c] = vobj.curWidth;
                        if (vobj.curHeight > rhs[r])
                            rhs[r] = vobj.curHeight;
                     }
                }
            }
            fontH = fh;
            adjustable = false;
            tw = 0;
            for (c = 0; c < numCols; c++) {
                tw += rws[c];
                if (rws[c] > xws[c])
                    adjustable = true;
            }
            th = 0;
            for (r = 0; r < numRows; r++) {
                th += rhs[r];
                if (rhs[r] > xhs[r])
                    adjustable = true;
            }
            dw = tw - size.width;
            dh = th - size.height;
        }
        x0 = 0;
        y0 = 1;
        if (!inEditMode && dockAt > 0) {
            switch (dockAt) {
                case NORTH:
                        x0 = (size.width - tw) / 2;
                        break;
                case NORTH_EAST:
                        x0 = size.width - tw - 2;
                        break;
                case EAST:
                        x0 = size.width - tw - 2;
                        y0 = (size.height - th) / 2;
                        break;
                case WEST:
                        y0 = (size.height - th) / 2;
                        break;
                case SOUTH:
                        x0 = (size.width - tw) / 2;
                        y0 = size.height - th;
                        break;
                case SOUTH_WEST:
                        y0 = size.height - th;
                        break;
                case SOUTH_EAST:
                        x0 = size.width - tw - 2;
                        y0 = size.height - th;
                        break;
            }
            if (x0 < 0)   x0 = 0;
            if (y0 < 1)   y0 = 1;
        }
        y = y0;
        for (r = 0; r < numRows; r++) {
            x = x0;
            for (c = 0; c < numCols; c++) {
                 comp = (JComponent) vArray[r][c];
                 if (comp != null && (comp instanceof VLabel)) {
                      vobj = (VLabel) comp;
                      vobj.changeShape(x, y, rws[c], rhs[r]);
                 }
                 x += rws[c];
            }
            y += rhs[r];
        }
    }

    public void refresh() {}
    public void setDefLabel(String s) {}
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}

    public void changeFont()
    {
        font = DisplayOptions.getFont(fontName, fontStyle, fontSize);
        setFont(font);
        curFont = font;
        setBorder();
        validate();
        repaint();
    }

    class vAnnTableLayout implements LayoutManager {
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
                // int count = target.getComponentCount();
                for (int r = 0; r < numRows; r++) {
                    for (int c = 0; c < numCols; c++) {
                       if (vArray[r][c] != null) {
                          VAnnotationIF comp = (VAnnotationIF) vArray[r][c];
                          comp.adjustBounds(numRows, numCols, (float)colGap, (float)rowGap);
                       }
                    }
                }
            }
        }
    } // vAnnTableLayout

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

    private final static Object[][] attributes = {
       {new Integer(ROWS),      "Number of Rows:"},
       {new Integer(COLUMNS),   "Number of Cols:"},
       {new Integer(SHOW),      "Show condition:"},
       {new Integer(DOCKAT),    "Dock at:", "menu", dockChoice}
    };

    public Object[][] getAttributes() {
        return attributes;
    }

    private void setBorder()
    { }

    public void setModalMode(boolean s) { }
    public void sendVnmrCmd() {}

    public double getXRatio()
    {
        return xRatio;
    }

    public double getYRatio()
    {
        return yRatio;
    }

    public void setSizeRatio(double x, double y)
    {
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
        rowGap = h / numRows;
        colGap = w / numCols;
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


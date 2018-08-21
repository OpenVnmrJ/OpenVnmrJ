/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import javax.swing.text.*;
import javax.swing.border.*;
import java.io.*;
import java.awt.*;
import java.beans.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import java.util.*;

import vnmr.ui.*;
import vnmr.templates.*;
import vnmr.bo.*;

public class VAnnotatePanel extends JPanel implements VObjDef,
     PropertyChangeListener, DropTargetListener, SwingConstants, VDropHandlerIF 
{
    static final int XNORTH = 1;
    static final int XSOUTH = 2;
    static final int XEAST = 4;
    static final int XWEST = 8;
    static final int XVERT = 16;
    static final int XHOR = 32;
    static final int GRIDMIN = 12;

    private VObjIF editVObj = null;
    private dummyCheck orientCk = null;
    private Component editObj = null;
    private VDummy dummyObj;
    // private VOrient[] orientObjs;
    private VAnnotateBox[] orientObjs;
    private AnnotationEditor annEditor;
    private VCheck xcheck = null; 
    private Graphics gr = null;
    private VobjEditorIF editor = null;
    private boolean isDraging = false;
    private boolean isResizing = false;
    private boolean isCopying = false;
    private boolean noOp = true;
    private boolean newGeom = true;
    private boolean moveOnly = false;
    private MouseAdapter ml;
    private MouseMotionAdapter mvl;
    private FocusListener fl;
    private KeyAdapter kl;
    private ComponentAdapter cl;
    private String templateName = null;
    private Color gridColor = Color.gray;
    private int YGAP = 8;
    private int XGAP = 8;
    private int gridNum = 40;
    private int gridRef = 40;
    private int dockAt;
    private int cursor;
    private int panW;
    private int panH;
    private int objH;
    private int objW;
    private int objX;
    private int objY;
    private int mX;
    private int mY;
    private int mW;
    private int mH;
    private int dX = 0;
    private int dY = 0;
    private int refX = 0;
    private int refY = 0;
    private int resizeDir = 0;
    private int expandDir = 0;
    private int fontH = 0;
    private int pointX = 0;
    private int pointY = 0;
    private int objId = 0;
    private float gridX = 0;
    private float gridY = 0;
    private Font dummyFont;
    private Color dummyColor;
    private final static String[] orientNames = {"L", "R", "A", "P"};
    private final static String[] orientDocks = {"West", "East", "North", "South"};
    
    
    public VAnnotatePanel(AnnotationEditor pan) {
         // setBackground(Util.getBgColor());
        //  setBackground(Color.lightGray);
         this.annEditor = pan;
         this.editor = (VobjEditorIF) pan;
         setLayout(new annotateLayout());
         setOpaque(false);
        
         addComponentListener(new ComponentAdapter() {
            public void componentResized(ComponentEvent evt) {
                 resizeProc();
            }
         });

         ml = new MouseAdapter() {
            public void mousePressed(MouseEvent evt) {
                 mPress(evt);
            }

            public void mouseReleased(MouseEvent evt) {
                 mRelease(evt);
            }

            public void mouseExited(MouseEvent evt) {
            }

            public void mouseClicked(MouseEvent evt) {
                 int clicks = evt.getClickCount();
                 if (clicks >= 2) {
                     int modifier = evt.getModifiers();
                          selectNextVobj(evt);
/*
                     if ((modifier & (1 << 4)) != 0) {
                          selectNextVobj(evt);
                     }
*/
                 }
                 requestFocus();
            }

         };

         mvl = new MouseMotionAdapter() {
             public void mouseDragged(MouseEvent evt) {
                  mDrag(evt);
             }

             public void mouseMoved(MouseEvent evt) {
                  mMove(evt);
             }
         };

         kl = new KeyAdapter() {
             public void keyTyped(KeyEvent e) {
                 e.consume();
             }
             public void keyPressed(KeyEvent e) {
                 e.consume();
             }
             public void keyReleased(KeyEvent e) {
                 e.consume();
                 int k = e.getKeyCode();
                 int m = e.getModifiers();
                 if (m == 0)
                      keyMove(k);
                 else if (m == Event.CTRL_MASK)
                      keyResize(k);
             }
         };

         fl = new FocusAdapter() {
             public void focusGained(FocusEvent e) {
                   setFocusFlag(true);
             }
             public void focusLost(FocusEvent e) {
                   setFocusFlag(false);
             }
         };

         cl = new ComponentAdapter() {
             public void componentResized(ComponentEvent e) {
                    setDummyObjGeom();
             }

             public void componentMoved(ComponentEvent e) {
                    setDummyObjGeom();
             }
         };

         addKeyListener(kl);
         addFocusListener(fl);
         dummyObj = new VDummy();
         dummyObj.addMouseListener(ml);
         dummyObj.addMouseMotionListener(mvl);
         dummyObj.setDropHandler(this);

         orientCk = new dummyCheck(null, null, "check", this);
         dummyFont = orientCk.getFont();
         fontH = dummyFont.getSize() + 4;
         dummyColor = orientCk.getForeground();

         orientObjs = new VAnnotateBox[4];
/*
         for (int i = 0; i < 4; i++) {
            orientObjs[i] = new VOrient("L", this);
            orientObjs[i].setVisible(false);
            orientObjs[i].setFont(dummyFont);
            orientObjs[i].setForeground(dummyColor);
         }
         orientObjs[1].setText("R");
         orientObjs[2].setText("A");
         orientObjs[3].setText("P");
*/

         new DropTarget(this, this);
         DisplayOptions.addChangeListener(this);
    }


    public void propertyChange(PropertyChangeEvent e) {
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof PropertyChangeListener)) {
              ((PropertyChangeListener) comp).propertyChange(e);
           }
        }
        gridColor=DisplayOptions.getColor("GraphGrid");
        repaint();
    }

    public void show_orientation(boolean b) {
	int i;

        VAnnotateBox newObj;
        if (b) {
            for (i = 0; i < 4; i++) {
                if (orientObjs[i] == null) {
                    newObj = new VAnnotateBox(null, null, "annotatebox");
                    orientObjs[i] = newObj;
                    if (xcheck != null) {
                        newObj.bg = xcheck.bg;
                        newObj.fontName = xcheck.fontName;
                        newObj.fontStyle = xcheck.fontStyle;
                        newObj.fontSize = xcheck.fontSize;
                        newObj.setAttribute(FGCOLOR, xcheck.fg);
                        newObj.changeFont(); 
                    }
                    newObj.setAttribute(ORIENTATION, orientNames[i]);
                    newObj.setGridInfo(gridNum, gridNum, gridX, gridY);
                    if (gridNum > 20)
                       newObj.setRowCol(2, 2);
                    else
                       newObj.setRowCol(1, 1);
                    newObj.setAttribute(DOCKAT, orientDocks[i]);
                    newObj.setDropHandler(this);
                    newObj.setEditor(editor);
                    ((VObjIF) newObj).setEditMode(true);
                }
            }
        }
        for (i = 0; i < 4; i++) {
            if (orientObjs[i] != null) {
                orientObjs[i].setVisible(b);
                if (orientObjs[i].isEditing)
                   editor.setEditObj(null);
            }
        }
        if (!b) {
            return;
        }
        int count = getComponentCount();
        for (i = 0; i < count; i++) {
            Component comp = getComponent(i);
            if  ((comp != null) && (comp instanceof VAnnotateBox)) {
                if (((VAnnotateBox) comp).isOrientObj())
                    return;
            }
        }
        for (i = 0; i < 4; i++)
            add(orientObjs[i], 0);
      
        validate();
        repaint();
    }

    private void setOrientEdit() {
       //  editor.setEditObj(orientCk);
    }

    private void setOrientSelected(boolean b) {
/*
        for (int i = 0; i < 4; i++) {
            orientObjs[i].bSelected = b;
        }
*/
    }

    private void setOrientObjs() {
        if (orientCk.font == null)
            return;
        
        dummyFont = orientCk.font;
        fontH = dummyFont.getSize() + 4;
        dummyColor = orientCk.getForeground();
/*
        for (int i = 0; i < 4; i++) {
            orientObjs[i].setFont(dummyFont);
            orientObjs[i].setForeground(dummyColor);
        }
*/
    }

    public void deleteAll() {
         removeAll();
         if (annEditor.isOriented()) {
             show_orientation(true);
         }
    }

    public void deleteObj(JComponent obj) {
         if  (obj instanceof VAnnotateBox) {
             VAnnotateBox box = (VAnnotateBox) obj;
             if (box.isOrientObj()) // orientation obj
                 return; 
         }
         if (obj instanceof VOrient)
              return; 
         if (obj instanceof VCheck)
              return; 
         obj.getParent().remove(obj);
    }

    public void loadTemplate(String f) {
         if (f == null)
            return;
         templateName = f;
         removeAll();
         try {
               LayoutBuilder.build(this, null, f);
         }
         catch (Exception e) { e.printStackTrace(); }
         setObjEditor();
         setEditMode(true);
         setDropHandler();
         getGridNum();
         newGeom = true;
         boolean bOriented = false; 
         boolean bRemove = false; 
         int count = getComponentCount();
         int i;
         xcheck = null; 
         for (i = 0; i < 4; i++)
             orientObjs[i] = null;
         for (i = 0; i < count; i++) {
             Component comp = getComponent(i);
             if  (comp != null) {
                if  (comp instanceof VCheck) {
                    xcheck = (VCheck) comp;
                }
                if  (comp instanceof VAnnotateBox) {
                    VAnnotateBox vObj = (VAnnotateBox) comp;
                    if (vObj.isOrientObj()) {
	                bOriented = true;
                        if (vObj.dockAt == WEST)
                            orientObjs[0] = vObj;               
                        else if (vObj.dockAt == EAST)
                            orientObjs[1] = vObj;               
                        else if (vObj.dockAt == NORTH)
                            orientObjs[2] = vObj;               
                        else
                            orientObjs[3] = vObj;               
                    }
                }
             }
         }
         if (xcheck != null) {
             String v = xcheck.getAttribute(LABEL);
             orientCk.fg = xcheck.fg;
             orientCk.bg = xcheck.bg;
             orientCk.fontName = xcheck.fontName;
             orientCk.fontStyle = xcheck.fontStyle;
             orientCk.fontSize = xcheck.fontSize;
             orientCk.fgColor = xcheck.fgColor;
             orientCk.font = xcheck.font;
             orientCk.propertyChange(null);

             // setOrientObjs(); 
             if (v != null && v.equals("1")) {
                  bOriented = true;
             }
             remove(xcheck);
         }
         for (i = 0; i < 4; i++) {
             if (orientObjs[i] != null)
                 remove(orientObjs[i]);
         }
         annEditor.setOrientation(bOriented);
         show_orientation(bOriented);
         editor.setEditObj(null);
         validate();
         repaint();
    }

    private void assignId(Container obj) {
		int i;
        int count = obj.getComponentCount();

        for (i = 0; i < count; i++) {
             Component comp = obj.getComponent(i);
             if  ((comp != null) && (comp instanceof VAnnotationIF)) {
				((VAnnotationIF) comp).setId(objId);
			 	objId++;
			    if (comp instanceof Container)
				    assignId((Container) comp);
             }
		}
    }

    private void assignObjId() {
		int i;
        int count = getComponentCount();

		objId = 1;
        for (i = 0; i < count; i++) {
             Component comp = getComponent(i);
             if  ((comp != null) && (comp instanceof VAnnotationIF)) {
				((VAnnotationIF) comp).setId(objId);
			 	objId++;
			    if (comp instanceof Container)
				    assignId((Container) comp);
             }
		}
    }

    private void xwrite2(PrintWriter fout, String s1, String s2, int g) {
         for (int k = 0; k < g; k++)
           fout.print("  ");
         fout.println(s1+"=\""+s2+"\"");
    }

    private void xwrite2(PrintWriter fout, String s1, int v, int g) {
         for (int k = 0; k < g; k++)
           fout.print("  ");
         String s2 = Integer.toString(v);
         fout.println(s1+"=\""+s2+"\"");
    }

    private void xwrite(PrintWriter fout, JComponent xobj, int gap) {
         VObjIF vobj = (VObjIF) xobj;
         Font font = null;
        String v = vobj.getAttribute(FONT_STYLE);
        if (v != null) {
            font = DisplayOptions.getFont(null, v, null);
        }
        if (font == null)
            font = xobj.getFont();
         xwrite2(fout, "psfont", font.getPSName(), gap);
         xwrite2(fout, "fontName", font.getName(), gap);
         xwrite2(fout, "fontSize", font.getSize(), gap);
         if (font.isBold())
            xwrite2(fout, "fontStyle", "bold", gap);
         if (font.isItalic())
            xwrite2(fout, "fontStyle", "italic", gap);
         Color fg = xobj.getForeground();
         xwrite2(fout, "color", fg.getRGB(), gap);
    }

    private void xwriteObj(PrintWriter fout, JComponent obj, int gap) {
         VObjIF vobj = (VObjIF) obj;
         int k, i, count;
         VAnnotateBox box;

         if (obj instanceof VAnnotateBox) {
            box = (VAnnotateBox) obj;
            if (box.isOrientObj()) {
               if (!annEditor.isOriented())
                  return;
            }
         }
         for (k = 0; k < gap; k++)
           fout.print("  ");
         String type = vobj.getAttribute(TYPE);
         fout.println("<"+type+">");
         LayoutBuilder.attrsWrite(fout, vobj, gap); 
         xwrite(fout, obj, gap+1); 
         if (obj instanceof VAnnotateBox) {
            box = (VAnnotateBox) obj;
            if (box.isOrientObj()) {
                xwrite2(fout,  "orientid", box.orientId, gap+1); 
            }
            count = obj.getComponentCount();
            for (i = 0; i < count; i++) {
                Component comp = obj.getComponent(i);
                if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                     xwriteObj(fout, (JComponent)comp, gap+1);
                }
            }
         }
         if (obj instanceof VAnnotateTable) {
            count = obj.getComponentCount();
            for (i = 0; i < count; i++) {
                Component comp = obj.getComponent(i);
                if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                     xwriteObj(fout, (JComponent)comp, gap+1);
                }
            }
         }
         if (obj instanceof JLabel) {
            Icon icon = ((JLabel)obj).getIcon();
            if (icon != null) {
                k = icon.getIconWidth();	
                xwrite2(fout,  "iconWidth", k, gap+1); 
                k = icon.getIconHeight();	
                xwrite2(fout,  "iconHeight", k, gap+1); 
            }
         }
         for (k = 0; k < gap; k++)
           fout.print("  ");
         fout.println("</"+type+">");
    }

    public void saveTemplate_txt(String f) {
         if (f == null || f.length() <= 0)
            return;
         PrintWriter os = null;
         int gap = 1;

         try {
            os = new PrintWriter( new OutputStreamWriter(
                   new FileOutputStream(f), "UTF-8"));
         }
         catch(IOException er) { }
         if (os == null) {
            Messages.postError("error writing: "+f);
            return;
         }
         xwrite2(os, "file", f, 0);
         xwrite2(os, "grid", gridNum, 1);
         if (annEditor.isOriented())
             xwrite2(os, "show_orientation", "yes", 1);
         else
             xwrite2(os, "show_orientation", "no", 1);
		 assignObjId();
         int count = getComponentCount();
         for (int i = 0; i < count; i++) {
             Component comp = getComponent(i);
             if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                 xwriteObj(os, (JComponent) comp, gap);
             }
         }
/*
         orientCk.bOutput = true;
         if (annEditor.isOriented())
             orientCk.setAttribute(LABEL, "1");
         else
             orientCk.setAttribute(LABEL, "0");
         xwriteObj(os, (JComponent) orientCk, gap);
         orientCk.bOutput = false;
*/

         os.close();
    }

    public void saveTemplate(String f) {
         if (f == null || f.length() <= 0)
            return;
         PrintWriter os = null;
         int gap = 1;

         try {
            os = new PrintWriter( new OutputStreamWriter(
                   new FileOutputStream(f), "UTF-8"));
         }
         catch(IOException er) { }
         if (os == null) {
            Messages.postError("error writing: "+f);
            return;
         }
         os.println("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
         os.println("<template>");
		 assignObjId();
         int count = getComponentCount();
         for (int i = 0; i < count; i++) {
             Component comp = getComponent(i);
             if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                 writeObj(os, (JComponent) comp, gap);
             }
         }
/*
         orientCk.bOutput = true;
         if (annEditor.isOriented())
             orientCk.setAttribute(LABEL, "1");
         else
             orientCk.setAttribute(LABEL, "0");
         writeObj(os, (JComponent) orientCk, gap);
         orientCk.bOutput = false;
*/

         os.println("</template>");
         os.close();
    }

    private void writeAttrs(PrintWriter fout, VObjIF obj, int gap) {
         LayoutBuilder.attrsWrite(fout, obj, gap); 
    }

    private void writeObj(PrintWriter fout, JComponent obj, int gap) {
         VObjIF vobj = (VObjIF) obj;
         int k;

         if (obj instanceof VAnnotateBox) {
            VAnnotateBox box = (VAnnotateBox) obj;
            if (box.isOrientObj()) {
               if (!annEditor.isOriented())
                  return;
            }
         }
         for (k = 0; k < gap * 2; k++)
           fout.print(" ");
         String type = vobj.getAttribute(TYPE);
         fout.print("<"+type+" ");
         writeAttrs(fout, vobj, gap);
         if ((obj instanceof VAnnotateBox) || (obj instanceof VAnnotateTable)) {
            fout.println(">");
            int count = obj.getComponentCount();
            for (int i = 0; i < count; i++) {
                Component comp = obj.getComponent(i);
                if  ((comp != null) && (comp instanceof VAnnotationIF))
                    writeObj(fout, (JComponent) comp, gap+1);
            }
            for (k = 0; k < gap * 2; k++)
                fout.print(" ");
            fout.println("</"+type+">");
         }
         else
            fout.println("/>");
    }

    private void saveObj(JComponent comp, String f) {
         if (f == null || f.length() <= 0)
            return;
         PrintWriter os = null;
         try {
            os = new PrintWriter( new OutputStreamWriter(
                   new FileOutputStream(f), "UTF-8"));
         }
         catch(IOException er) { }
         if (os == null) {
            Messages.postError("error writing: "+f);
            return;
         }
         os.println("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>");
         writeObj(os, comp, 1);

         os.close();
    }

    private void resizeProc() {
         if (gr != null)
              gr.dispose();
         gr = getGraphics();
         panW = getWidth();
         panH = getHeight();
         gridX = ((float) panW) / ((float) gridNum);
         gridY = ((float) panH) / ((float) gridNum);
         newGeom = true;
    }

    private void setFocusFlag(boolean s) {
         dummyObj.changeFocus(s);
    }

    public void setDropHandler() {
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              ((VAnnotationIF) comp).setDropHandler(this);
           }
        }
    }

    public int getGridNum() {
        int count = getComponentCount();
        gridNum = GRIDMIN;
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              int k = ((VAnnotationIF) comp).getGridNum();
              if (k > gridNum)
                  gridNum = k;
           }
        }
        gridRef = gridNum;
        return gridNum;
    }


    public void setEditMode(boolean s) {
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VObjIF)) {
              ((VObjIF) comp).setEditMode(s);
              ((VObjIF) comp).setAttribute(ANNOTATION, "yes");
           }
        }
    }

    public void setEditor(VobjEditorIF e) {
        editor = e;
    }

    public void setObjEditor() {
        if (editor == null)
             return;
        int count = getComponentCount();
        for (int i = 0; i < count; i++) {
           Component comp = getComponent(i);
           if  ((comp != null) && (comp instanceof VAnnotationIF)) {
              ((VAnnotationIF) comp).setEditor(editor);
           }
        }
    }

    public void setDummyObjGeom() {
         Rectangle  rect = editObj.getBounds();
         objW = rect.width;
         objH = rect.height;
         objX = rect.x;
         objY = rect.y;
         Container pp = editObj.getParent();
         while (pp != null) {
                if (pp == this)
                    break;
                rect = pp.getBounds();
                objX = objX + rect.x;
                objY = objY + rect.y;
                pp = pp.getParent();
         }
         dummyObj.setBounds(objX, objY, objW, objH);
         XGAP = 3;
         if (objW > 16)
              XGAP = 6;
         if (objW > 20)
              XGAP = 8;
         YGAP = 3;
         if (objH > 16)
              YGAP = 6;
         if (objH > 20)
              YGAP = 8;
    }

    public void setObjDockAttr() {
        expandDir = 0;
        dockAt = 0;
        if (editVObj == null)
            return;
        if (editVObj instanceof VAnnotateBox)
           dockAt = ((VAnnotateBox) editVObj).getDockInfo();
        else if (editVObj instanceof VAnnotateTable )
           dockAt = ((VAnnotateTable) editVObj).getDockInfo();
        switch (dockAt) {
           case NORTH:
                expandDir = XEAST | XWEST | XSOUTH | XHOR;
                break;
           case NORTH_EAST:
                expandDir = XWEST | XSOUTH;
                break;
           case NORTH_WEST:
                expandDir = XEAST | XSOUTH;
                break;
           case WEST:
                expandDir = XEAST | XNORTH | XSOUTH | XVERT;
                break;
           case EAST:
                expandDir = XWEST | XNORTH | XSOUTH | XVERT;
                break;
           case SOUTH:
                expandDir = XEAST | XWEST | XNORTH | XHOR;
                break;
           case SOUTH_EAST:
                expandDir = XWEST | XNORTH;
                break;
           case SOUTH_WEST:
                expandDir = XEAST | XNORTH;
                break;
        }
        setDummyObjGeom();
    }

    public void setEditObj(VObjIF obj) {
        if (obj != null) {
            if (obj == editVObj) {
               setObjDockAttr();
               return;
            }
        }
        if (editObj != null) {
            editVObj.setEditStatus(false);
            editObj.removeComponentListener(cl);
        }
        dockAt = 0;
        editVObj = obj;
        editObj = null;
        remove(dummyObj);
        // setOrientSelected(false);
        if (editVObj != null) {
            editObj = (Component) editVObj;
            editVObj.setEditStatus(true);
            setObjDockAttr();
            requestFocus();
            cursor = Cursor.DEFAULT_CURSOR;
            dummyObj.setCursor(Cursor.getPredefinedCursor(cursor));
            if (!(obj instanceof VCheck)) {
               Container p = editObj.getParent();
               if (p != null && (p instanceof VAnnotateTable))
                  moveOnly = true;
               else
                  moveOnly = false;
               add(dummyObj, 0); 
               editObj.addComponentListener(cl);
            }
/*
            else
               setOrientSelected(true);
*/
        }
        repaint();
    }

    private void setRefPoints(int dir) {
        resizeDir = 0;
        switch (dir) {
            case Cursor.N_RESIZE_CURSOR:
                 resizeDir = XNORTH;
                 break;
            case Cursor.S_RESIZE_CURSOR:
                 resizeDir = XSOUTH;
                 break;
            case Cursor.W_RESIZE_CURSOR:
                 resizeDir = XWEST;
                 break;
            case Cursor.E_RESIZE_CURSOR:
                 resizeDir = XEAST;
                 break;
            case Cursor.NE_RESIZE_CURSOR:
                 resizeDir = XNORTH | XEAST;
                 break;
            case Cursor.SE_RESIZE_CURSOR:
                 resizeDir = XSOUTH | XEAST;
                 break;
            case Cursor.NW_RESIZE_CURSOR:
                 resizeDir = XNORTH | XWEST;
                 break;
            case Cursor.SW_RESIZE_CURSOR:
                 resizeDir = XSOUTH | XWEST;
                 break;
        }
        int minY = 5;
        int minX = 5;
        boolean oddGridNum = false;
        if ((gridNum / 2) != ((gridNum + 1) / 2))
           oddGridNum = true;

        if (gridNum >= GRIDMIN) {
            minY = (int) gridY;
            minX = (int) gridX;
        }
        if ((resizeDir & XNORTH) != 0) {
            refY = objH - minY;
            if ((expandDir & XVERT) != 0) {
                if (oddGridNum)
                  refY = objH / 2 - minY / 2;
                else
                  refY = objH / 2 - minY;
            }
        }
        else if ((resizeDir & XSOUTH) != 0) {
            refY = minY;
            if ((expandDir & XVERT) != 0) {
                if (oddGridNum)
                   refY = objH / 2 + minY / 2;
                else
                   refY = objH / 2 + minY;
            }
        }
        if ((resizeDir & XEAST) != 0) {
            refX = minX;
            if ((expandDir & XHOR) != 0) {
                if (oddGridNum)
                   refX = objW / 2 + minX / 2;
                else
                   refX = objW / 2 + minX;
            }
        }
        else if ((resizeDir & XWEST) != 0) {
            refX = objW - minX;
            if ((expandDir & XHOR) != 0) {
                if (oddGridNum)
                   refX = objW / 2 - minX / 2;
                else
                   refX = objW / 2 - minX;
            }
        }
        if (refY < 0)
            refY = 0;
        if (refX < 0)
            refX = 0;
    }


    private void keyMove (int key) {
        if (moveOnly)
           return;
        if (dockAt != 0)
            return;
        if (editObj == null)
            return;
        isCopying = false;
        isResizing = false;
        isDraging = true;
        mW = objW;
        mH = objH;
        mX = objX; 
        mY = objY; 
        int dx = 1;
        int dy = 1;
        if (gridNum >= GRIDMIN) {
            dx = (int)gridX;
            dy = (int)gridY;
        }
        if (key == KeyEvent.VK_LEFT)
            mX = mX - dx;
        else if (key == KeyEvent.VK_RIGHT)
            mX = mX + dx;
        else if (key == KeyEvent.VK_UP)
            mY = mY - dy;
        else if (key == KeyEvent.VK_DOWN)
            mY = mY + dy;
        else
            return;
        if (mX < 0) 
            mX = 0;
        if (mX + mW > panW)
            mX = panW - objW;

        if (mY < 0) 
            mY = 0;
        if (mY + mH > panH) 
            mY = panH - mH;
        putObj(mX, mY, 0,0);
        isDraging = false;
    }

    private void keyResize (int key) {
        if (editObj == null || moveOnly)
            return;
        isCopying = false;
        isDraging = false;
        isResizing = true;
        mW = objW;
        mH = objH;
        mX = objX; 
        mY = objY; 
        int dw = 1;
        int dh = 1;
        int dir = 0;
        int dx = objX + objW;
        int dy = objY + objH;
        if (gridNum >= GRIDMIN) {
            dw = (int)gridX;
            dh = (int)gridY;
        }
        if (key == KeyEvent.VK_LEFT) {
            if (dockAt == EAST || dockAt == NORTH_EAST || dockAt == SOUTH_EAST) {
               dir = Cursor.W_RESIZE_CURSOR;
               dx = objX - dw;
            }
            else {
               dir = Cursor.E_RESIZE_CURSOR;
               dx = dx - dw;
            }
        }
        else if (key == KeyEvent.VK_RIGHT) {
           if (dockAt == EAST || dockAt == NORTH_EAST || dockAt == SOUTH_EAST) {
               dir = Cursor.W_RESIZE_CURSOR;
               dx = objX + dw;
            }
            else {
               dir = Cursor.E_RESIZE_CURSOR;
               dx = dx + dw;
            }
        }
        else if (key == KeyEvent.VK_UP) {
           if (dockAt == SOUTH || dockAt == SOUTH_EAST || dockAt == SOUTH_WEST) {
               dir = Cursor.N_RESIZE_CURSOR;
               dy = objY - dh;
            }
            else {
               dir = Cursor.S_RESIZE_CURSOR;
               dy = dy - dh;
            }
        }
        else if (key == KeyEvent.VK_DOWN) {
           if (dockAt == SOUTH || dockAt == SOUTH_EAST || dockAt == SOUTH_WEST) {
               dir = Cursor.N_RESIZE_CURSOR;
               dy = objY + dh;
            }
            else {
               dir = Cursor.S_RESIZE_CURSOR;
               dy = dy + dh;
            }
        }
        else
            return;
        setRefPoints(dir);
        if (dx < 0) 
            dx = 0;
        if (dy < 0) 
            dy = 0;
        putObj(dx, dy, 0,0);
        isResizing = false;
    }

    private void mPress (MouseEvent e) {
        InputEvent ine = (InputEvent) e;
        if (moveOnly && cursor != Cursor.DEFAULT_CURSOR)
           return;
        
        isCopying = false;
        isResizing = false;
        isDraging = false;
        noOp = false;
        resizeDir = 0;

        dX = e.getX();
        dY = e.getY();
        mX = -1;
        if (ine.isControlDown()) { // control  key pressed
            if (editObj instanceof VAnnotateBox)
               isCopying = false;
            else
               isCopying = true;
        }
        if (cursor == Cursor.DEFAULT_CURSOR) {
            if (dockAt == 0 || isCopying) {
                isDraging = true;
                mW = objW;
                mH = objH;
            }
            else {
                noOp = true;
                return;
            }
            if (isCopying) {
                cursor = 20; // any number other than DEFAULT_CURSOR
                dummyObj.setCursor(DragSource.DefaultCopyDrop);
	    }
        }
        else {
            isResizing = true;
            setRefPoints(cursor);
        }
        gr.setXORMode(Util.getBgColor());
        gr.setColor(Color.red);
    }

    private void copyVobj (int x, int y) {
        File fd;
        JComponent newComp = null;

        try {
            fd = File.createTempFile("edit", ".xml");
        }
        catch (IOException e) {
            Messages.writeStackTrace(e);
            return;
        }
        String f = fd.getAbsolutePath();
/*
        LayoutBuilder.writeToFile((JComponent)editVObj, f);
*/
        saveObj((JComponent)editVObj, f);
        if (!fd.exists()) {
            return;
        }

        try {
            newComp = LayoutBuilder.build(this, null, f, mX, mY);
        }
        catch(Exception be) {
            Messages.writeStackTrace(be);
        }
        fd.delete();
        if ((newComp == null) || !(newComp instanceof VObjIF))
            return;
        VObjIF oldVobj = editVObj;
        VAnnotationIF nObj = (VAnnotationIF) newComp;
        editVObj = (VObjIF) newComp;
        editObj = newComp;
        nObj.setGridInfo(gridNum, gridNum, gridX, gridY);
        nObj.setDropHandler(this);
        nObj.setEditor(editor);
        editVObj.setAttribute(ANNOTATION, "yes");
        editVObj.setEditMode(true);
        editVObj.setAttribute(DOCKAT, "None");
        putObj(mX, mY, x, y);
        editVObj = oldVobj;
        editObj = (Component) oldVobj;
        editor.setEditObj((VObjIF) newComp);
    }

    private void mRelease (MouseEvent e) {
        if (noOp || mX < 0)
           return;
        if (moveOnly && isResizing)
           return;
        gr.drawRect(mX, mY, mW, mH);
        int x = e.getX();
        int y = e.getY();
        if (isResizing) {
           putObj(x+objX, y+objY, 0,0);
           isResizing = false;
        }
        if (isDraging) {
           if (isCopying) {
              copyVobj(x+objX, y+objY);
           }
           else
              putObj(mX, mY, x+objX, y+objY);
           isDraging = false;
           isCopying = false;
        }
        requestFocus();
    }

    private void mDrag (MouseEvent e) {
        if (noOp)
            return;
        if (isResizing && moveOnly)
            return;
        if (mX >= 0)
            gr.drawRect(mX, mY, mW, mH);
        mX = objX + e.getX(); 
        mY = objY + e.getY(); 
        if (mX < 0 || mY < 0) {  // out of boundary 
            mX = -1;
            return;
        }
        if (mX > panW || mY > panH) {  // out of boundary 
            mX = -1;
            return;
        }
        if (isResizing) {
           mResize(e);
           return;
        }
        if (isDraging) {
           mX = mX - dX; 
           mY = mY - dY; 
           if (mX < 0)  mX = 0;
           if (mX + objW > panW)  mX = panW - objW;
           if (mY < 0)  mY = 0;
           if (mY + objH > panH)  mY = panH - objH;
           gr.drawRect(mX, mY, mW, mH);
           return;
        }

    }

    private void mMove (MouseEvent e) {
        JComponent comp = (JComponent)e.getSource();
        int mx = e.getX();
        int my = e.getY();
        int md = e.getModifiers();

        if ((md & InputEvent.BUTTON1_MASK) != 0) {
            mDrag(e);
            return;
        }
        if (moveOnly)
           return;
        int newCursor = Cursor.DEFAULT_CURSOR;
        if (mx < XGAP) {
           if (dockAt == WEST || dockAt == NORTH_WEST || dockAt == SOUTH_WEST) {
                if (my < YGAP) {
                    if (dockAt == WEST || dockAt == SOUTH_WEST)
                        newCursor = Cursor.N_RESIZE_CURSOR;
                }
                else if (my > objH - YGAP) {
                    if (dockAt == WEST || dockAt == NORTH_WEST)
                        newCursor = Cursor.S_RESIZE_CURSOR;
                }
           }
           else {
                if (my < YGAP) {
                    if (dockAt != NORTH && dockAt != NORTH_EAST)
                        newCursor = Cursor.NW_RESIZE_CURSOR;
                }
                else if (my > objH - YGAP) {
                    if (dockAt != SOUTH && dockAt != SOUTH_EAST)
                        newCursor = Cursor.SW_RESIZE_CURSOR;
                }
                else
                    newCursor = Cursor.W_RESIZE_CURSOR;
           }
        }
        else if (mx > objW - XGAP) {
           if (dockAt == EAST || dockAt == NORTH_EAST || dockAt == SOUTH_EAST) {
                if (my < YGAP) {
                    if (dockAt == EAST || dockAt == SOUTH_EAST)
                        newCursor = Cursor.N_RESIZE_CURSOR;
                }
                else if (my > objH - YGAP) {
                    if (dockAt == EAST || dockAt == NORTH_EAST)
                        newCursor = Cursor.S_RESIZE_CURSOR;
                }
           }
           else {
                if (my < YGAP) {
                    if (dockAt != NORTH && dockAt != NORTH_WEST)
                        newCursor = Cursor.NE_RESIZE_CURSOR;
                }
                else if (my > objH - YGAP) {
                    if (dockAt != SOUTH && dockAt != SOUTH_WEST)
                        newCursor = Cursor.SE_RESIZE_CURSOR;
                }
                else
                    newCursor = Cursor.E_RESIZE_CURSOR;
           }
        }
        else if (my < YGAP) {
           if (dockAt == NORTH || dockAt == NORTH_WEST || dockAt == NORTH_EAST)
                newCursor = Cursor.DEFAULT_CURSOR;
           else
                newCursor = Cursor.N_RESIZE_CURSOR;
        }
        else if (my > objH - YGAP) {
           if (dockAt == SOUTH || dockAt == SOUTH_WEST || dockAt == SOUTH_EAST)
                newCursor = Cursor.DEFAULT_CURSOR;
           else
                newCursor = Cursor.S_RESIZE_CURSOR;
        }
        if (newCursor != cursor) {
           cursor = newCursor;
           dummyObj.setCursor(Cursor.getPredefinedCursor(cursor));

           // setObjCursor(Cursor.getPredefinedCursor(cursor));
        }
    }

    private void adjustGeom(int x, int y) {
        int dw, dh;
        mY = objY;
        mX = objX;
        mW = objW;
        mH = objH;
        if ((resizeDir & XNORTH) != 0) {
            if (y > refY)
               y = refY;
            if (y+objY < 0)
               y = -objY;
            dh = 0 - y;
            mY = objY - dh;
            mH = objH + dh;
            if ((expandDir & XVERT) != 0)
               mH += dh;
        }
        else if ((resizeDir & XSOUTH) != 0) {
            if (y < refY)
               y = refY;
            if (y+objY > panH)
               y = panH - objY;
            dh = y - objH;
            mH = objH + dh;
            if ((expandDir & XVERT) != 0) {
               mY = objY - dh;
               mH += dh;
            }
        }
        if ((resizeDir & XEAST) != 0) {
            if (x < refX)
               x = refX;
            if (x+objX > panW)
               x = panW - objX;
            dw = x - objW;
            mW = objW + dw;
            if ((expandDir & XHOR) != 0) {
               mW += dw;
               mX = objX - dw;
            }
        }
        else if ((resizeDir & XWEST) != 0){
            if (x > refX)
               x = refX;
            if (x+objX < 0)
               x = -objX;
            dw = 0 - x;
            mW = objW + dw;
            mX = objX - dw;
            if ((expandDir & XHOR) != 0) {
               mW += dw;
            }
        }
    }

    private void adjustSize() {
        if (gridNum < GRIDMIN)
            return;
        float  f1, f2;
        int  v;
        f1 = gridX / 2;
        f2 = ((float) mW) + f1;
        v = (int)(f2 / gridX);
        if (v < 1)
            v = 1;
        mW = (int) (((float) v) * gridX);
        f1 = gridY / 2;
        f2 = ((float) mH) + f1;
        v = (int)(f2 / gridY);
        if (v < 1)
            v = 1;
        mH = (int) (((float) v) * gridY);
        if (mX + mW > panW)
            mW = panW - mX; 
        if (mY + mH > panH)
            mH = panH - mY; 
    }

    private void mResize (MouseEvent e) {
        if (moveOnly)
           return;
        adjustGeom(e.getX(), e.getY());
        gr.drawRect(mX, mY, mW, mH);
    }

    private void putObj (int x, int y, int px, int py) {
        if (x < 0 || x > panW)
            return;
        if (y < 0 || y > panH)
            return;
        if (mW < 3 || mH < 3)
            return;
        pointX = px;
        pointY = py;
        VObjIF orgVObj = editVObj;
        Component orgObj = editObj;
        boolean bGrp = false;
        if (editObj instanceof VAnnotateBox) {
            VAnnotateBox box = (VAnnotateBox) editObj;
            if (box.grpId > 0 && box.grpRoot != null) {
                 editVObj = (VObjIF) box.grpRoot;
                 editObj = (Component) editVObj;
                 bGrp = true;
            }
        }
        float  f1, f2;
        int  v;
        if (gridNum >= GRIDMIN) {
            f1 = gridX / 2;
            f2 = ((float) x) + f1;
            v = (int)(f2 / gridX);
            x = (int) (((float) v) * gridX);
            f1 = gridY / 2;
            f2 = ((float) y) + f1;
            v = (int)(f2 / gridY);
            y = (int) (((float) v) * gridY);
        }
        if (isResizing) {
            x = x - objX;
            y = y - objY;
            adjustGeom(x, y);
        }
        else {
            mX = x;
            mY = y;
            adjustSize();
        }
        changeParent();
        
        if (editObj instanceof VAnnotateBox) {
            if (!bGrp)
              getNewChild();
        }
        Container p = editObj.getParent();
        if (p != this) 
            ((JComponent)editObj).setOpaque(false);
        else {
            if (!(editObj instanceof VAnnotateBox))
                ((JComponent)editObj).setOpaque(true);
        }
        editObj.validate();
        ((VAnnotationIF) editObj).setBoundsRatio(false);
        if (editObj instanceof VAnnotateBox) {
            ((VAnnotationIF) editObj).readjustBounds();
        }
        if (bGrp) {
            ((VAnnotateBox)editVObj).resetMembers();
            editVObj = orgVObj;
            editObj = orgObj;
            setDummyObjGeom();
        }
    }

    private void resizeObj (MouseEvent e) {
        int x = e.getX() + objX;
        int y = e.getY() + objY;
        putObj(x, y, 0, 0);
    }

    private JComponent deeperParent(JComponent p) {
        int nmembers = p.getComponentCount();
        JComponent newp = null;
        for (int i = 0; i < nmembers; i++) {
            Component comp = p.getComponent(i);
            if (comp instanceof JPanel) {
                JComponent nobj = (JComponent) comp;
                if (!nobj.equals(editObj)) {
                        newp = deeperParent(nobj);
                        if (newp != null) {
                           p = newp;
                           break;
                        }
                }
            }
        }
        if(!p.isShowing())
            return null;
        if (p instanceof VAnnotateTable) {
            if (!(editObj instanceof VLabel))
               return null;
        }
            
        Rectangle rect = p.getBounds();
        Point pt = p.getLocationOnScreen();
        Point pt2 = getLocationOnScreen();
        pt.x = pt.x - pt2.x;
        pt.y = pt.y - pt2.y;
        if (mX < pt.x)
            return null;
        if (mY < pt.y)
            return null;
        if (mY + mH > pt.y + rect.height)
            return null;
        if (mX + mW > pt.x + rect.width)
            return null;
        if (p instanceof VAnnotateTable) {
            int nx = mX - pt.x; 
            int ny = mY - pt.y; 
            if (pointX > 0) {
                nx = pointX - pt.x;
                ny = pointY - pt.y;
           }
           VAnnotateTable table = (VAnnotateTable) p; 
           if (table.hasObjAt(nx, ny))
              return null;
        }
        return p;
    }

    private void changeParent() {
        if (dockAt != 0) {
            editObj.setBounds(mX, mY, mW, mH);
            return;
        }
        Container p1 = editObj.getParent();
        int ncount = getComponentCount();
        JComponent np = this;
        for (int i = 0; i < ncount; i++) {
           Component comp = getComponent(i);
           if (comp instanceof JPanel) {
               JComponent p = (JComponent) comp;
               if (!p.equals(editObj)) {
                    JComponent p2 = deeperParent(p);
                    if (p2 != null) {
                           np = p2;
                           break;
                    }
               }
           }
        }
        if (np instanceof VAnnotateTable) {
           if (!(editObj instanceof VLabel))
               return;
        }
        Point pt = np.getLocationOnScreen();
        Point pt2 = getLocationOnScreen();
        pt.x = pt.x - pt2.x;
        pt.y = pt.y - pt2.y;
        mX = mX - pt.x;
        mY = mY - pt.y;

        if (np != p1)
           p1.remove(editObj);
        if (np instanceof VAnnotateTable) {
           if (np == p1)
                np.remove(editObj);
           if (pointX > 0) {
		mX = pointX - pt.x;
		mY = pointY - pt.y;
           }
           ((VAnnotateTable) np).addDropObj(editObj, mX, mY);
           moveOnly = true;
           setEditObj(editVObj);
           return;
        }
        if (np != p1) {
           // p1.remove(editObj);
           if (editObj instanceof VAnnotateBox)
               np.add(editObj);
           else {
               if (np == this)
                  np.add(editObj, 1);
               else
                  np.add(editObj);
           }
           p1 = np;
        }
        moveOnly = false;
        editObj.setBounds(mX, mY, mW, mH);
        ((VAnnotationIF)editObj).readjustBounds(gridNum, gridNum, gridX, gridY);
    }

    private void getNewChild() {
        Rectangle  rect = editObj.getBounds();
        Point pt1 = editObj.getLocationOnScreen();
        Container pp = editObj.getParent();
        Point pt2;
        Rectangle rect2;
        JComponent obj;
        Component comp;
        int     px1 = pt1.x;
        int     py1 = pt1.y;
        int     px2 = px1 + rect.width;
        int     py2 = py1 + rect.height;
        int     nmembers = getComponentCount();
        int     i = nmembers - 1;
        while (i >= 0) {
           comp = getComponent(i);
           i--;
           if (!comp.isShowing())
               continue;
           if ((comp == editObj) || (comp == pp)) {
               continue;
           }
           if ((comp instanceof VAnnotationIF)) {
               if (comp instanceof VAnnotateBox) {
                  int doc = ((VAnnotateBox) comp).getDockInfo();
                  if (doc != 0)
                       continue;
               } 
               obj = (JComponent) comp;
               rect2 = obj.getBounds();
               pt2 = obj.getLocationOnScreen();
               if (pt2.x >= px1 && (pt2.x + rect2.width <= px2)) {
                    if (pt2.y >= py1 && (pt2.y + rect2.height <= py2)) {
                       remove(comp);
                       ((JComponent)editObj).add(obj);
                       obj.setBounds(pt2.x - px1, pt2.y - py1,
                                 rect2.width, rect2.height);
                       ((VAnnotationIF) comp).setBoundsRatio(false);
                       obj.setOpaque(false);
                    }
               }
           }
        }
    }

    private Component getNextObj(Container p, int x, int y) {
        int ncount = p.getComponentCount();
        Component ncomp = null;
        for (int i = 0; i < ncount; i++) {
            Component comp = p.getComponent(i);
            if ((comp != null) && comp.isVisible()) {
               if (comp.contains(x - comp.getX(), y - comp.getY())) {
                  if (comp instanceof VObjIF) {
                        if (comp != editObj)
                            ncomp = comp;
                  }
                  if (comp instanceof Container) {
                    Container child = (Container) comp;
                    Component deeper = getNextObj(child, x - child.getX(), y - child.getY());
                    if (deeper != null) {
                        ncomp = deeper;
                    }
                  }
                }
            }
            if (ncomp != null)
                break;
        }
        return ncomp;
    }

    private boolean selectGrpVobj(MouseEvent e) {
        int x = e.getX();
        int y = e.getY();
        boolean res = false;
        VObjIF newVobj = null;
        Container cont = (Container) editVObj;
        int ncount = cont.getComponentCount();
        for (int i = 0; i < ncount; i++) {
            Component comp = cont.getComponent(i);
            if (!(comp instanceof VObjIF))
                continue;
            if ((comp != null) && comp.isVisible()) {
                if (comp.contains(x - comp.getX(), y - comp.getY())) {
                    if (comp != editVObj)
                        newVobj = (VObjIF) comp;
                    if (comp instanceof Container) {
                        Container child = (Container) comp;
                        Component deeper = getNextObj(child, x - child.getX(), y - child.getY());
                        if (deeper != null) 
                            newVobj = (VObjIF) deeper;
                    }
                }
            }
            if (newVobj != null)
                break;
        }
        if (newVobj != null && newVobj != editVObj) {
            res = true;
            editor.setEditObj(newVobj);
        }
        else {
            if (cursor != Cursor.DEFAULT_CURSOR) {
               int oldCursor = cursor;
               int md = e.getModifiers();
               boolean forward = true;
               if ((md & InputEvent.BUTTON3_MASK) != 0)
                   forward = false;
               VAnnotateBox box = (VAnnotateBox) editVObj;
               box.showNextGrp(forward);
               cursor = oldCursor;
               dummyObj.setCursor(Cursor.getPredefinedCursor(cursor));
               res = true;
            }
        }
        return res;
    }


    private void selectNextVobj(MouseEvent e) {
        if (editor == null)
            return;
        if (editVObj instanceof VAnnotateBox) {
            VAnnotateBox box = (VAnnotateBox) editVObj;
            if (box.grpId > 0) { // it is multilayer group
                // box.showNextGrp();
                if (selectGrpVobj(e))
                    return;
            }
        }
        Component newComp = null;
        Point pt = dummyObj.getLocation();
        int x = e.getX() + pt.x;
        int y = e.getY() + pt.y;

        int ncount = getComponentCount();
        for (int i = 0; i < ncount; i++) {
            Component comp = getComponent(i);
            if (comp == dummyObj)
                continue;
            if (!(comp instanceof VObjIF) && !(comp instanceof VOrient))
                continue;
            if ((comp != null) && comp.isVisible()) {
                if (comp.contains(x - comp.getX(), y - comp.getY())) {
                    if (comp != editVObj)
                        newComp = comp;
                    if (comp instanceof Container) {
                        Container child = (Container) comp;
                        Component deeper = getNextObj(child, x - child.getX(), y - child.getY());
                        if (deeper != null) 
                            newComp = deeper;
                    }
                }
            }
            if (newComp != null)
                break;
        }
        if (newComp == null)
            return;
        if (newComp instanceof VOrient) {
            setOrientEdit();
            return;
        }
        if (newComp != null && newComp != editVObj)
            editor.setEditObj((VObjIF) newComp);
    }

    private void  drawGrid(Graphics g) {
        Dimension size = getSize();
        // g.setColor(getBackground().darker());
        g.setColor(DisplayOptions.getColor("VJGrid"));
        int c;
        int x = 1;
        int y = 0;
        int x2 = size.width - 1;
        int y2 = size.height - 1;
        float f1 = 0;
        gridY = ((float) size.height) / ((float) gridNum);
        if (gridY >= 3) {
            for (c = 0; c < gridNum; c++) {
                f1 += gridY;
                y = (int) f1;
                g.drawLine(x, y, x2, y);
            }
        }
        gridX = ((float) size.width) / ((float) gridNum);
        if (gridX >= 3) {
            y = 1;
            f1 = 0;
            for (c = 0; c < gridNum; c++) {
                f1 += gridX;
                x = (int) f1;
                g.drawLine(x, y, x, y2);
            }
        }
    }

    public void  paint(Graphics g) {
        if (!isShowing())
            return;
        if (gridRef >= GRIDMIN)
            drawGrid(g);
        super.paint(g);
        // dummyObj.paint(g);
    }

    public void setGridNum(int k) {
        if (gridRef != k) {
            gridRef = k;
            if (k >= GRIDMIN) {
                // gridNum = ((k + 1) / 2) * 2;
                gridNum = k;
                gridX = ((float) panW) / ((float) gridNum);
                gridY = ((float) panH) / ((float) gridNum);
                int count = getComponentCount();
                for (int i = 0; i < count; i++) {
                   Component comp = getComponent(i);
                   if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                      ((VAnnotationIF)comp).readjustBounds(gridNum, gridNum, gridX, gridY);
                   }
                }
                revalidate();
                repaint();
            }
        }
    }

    public void processDrop(DropTargetDropEvent e, JComponent c, boolean b) {
        Point pt, pt2, pt3;
        JComponent comp;
        VAnnotationIF newObj = null;
        int   x = 0;
        int   y = 0;
        int   w = 40;
        int   h = 30;

        pt3 = e.getLocation();
        try {
            Transferable tr = e.getTransferable();
            if (!tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)) {
                // e.rejectDrop();
                return;
            }
            Object obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
            if (!(obj instanceof AnnotateObj)) {
                // e.rejectDrop();
                return;
            }
            e.acceptDrop(DnDConstants.ACTION_COPY);
            String name = ((AnnotateObj)obj).getName();
            pt = c.getLocationOnScreen();
            pt2 = getLocationOnScreen();
            x = pt.x - pt2.x + pt3.x - 10;
            y = pt.y - pt2.y + pt3.y - 10;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (name.equals("box")) {
                 newObj = new VAnnotateBox(null,null, "annotatebox");
                 h = 50;
                 w = 50;
            }
            else if (name.equals("label")) {
                 newObj = new VLabel(null,null, "label");
            }
            else if (name.equals("text")) {
                 w = 50;
                 newObj = new VLabel(null,null, "textline");
            }
            else if (name.equals("table")) {
                 newObj = new VAnnotateTable(null,null, "annotatetable");
                 h = 60;
                 w = 60;
            }
        } // try
        catch (IOException io) {}
        catch (UnsupportedFlavorException ufe) { }
        if (newObj == null)
            return;
        mX = x;
        mY = y;
        mW = w;
        mH = h;
        Component newComp = (Component) newObj;
        newObj.setGridInfo(gridNum, gridNum, gridX, gridY);
        newComp.setBounds(x, y, mW, mH);
        newObj.setDropHandler(this);
        newObj.setEditor(editor);
        ((VObjIF) newObj).setAttribute(ANNOTATION, "yes");
        ((VObjIF) newObj).setEditMode(true);
        VAnnotateTable table = null;
        if (newObj instanceof VLabel) {
           if (c == dummyObj && (editObj instanceof VAnnotateTable))
               table = (VAnnotateTable) editObj;
           else if (c instanceof VAnnotateTable)
               table = (VAnnotateTable) c;
           
        }
        if (table != null) {
           // table.addDropObj(newComp, pt3.x - 10, pt3.y - 10);
           if (!table.hasObjAt( pt3.x - 10, pt3.y - 10))
              table.addDropObj(newComp, pt3.x - 10, pt3.y - 10);
           else
              table = null;
        }
        if (table == null)
            add(newComp, 0);
        editor.setEditObj((VObjIF) newObj);
        ((VObjIF) newObj).setEditMode(true);
        if (table == null)
           putObj(x, y, 0, 0);
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
         processDrop(e, this, true);
    } // drop

    class annotateLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            return new Dimension(0, 0);
        }


        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(100, 100);
        }

        public void layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
               Dimension dim = target.getSize();
               gridX = ((float) dim.width) / ((float) gridNum);
               gridY = ((float) dim.height) / ((float) gridNum);
               int count = target.getComponentCount();
               int i;
               for (i = 0; i < count; i++) {
                    Component comp = target.getComponent(i);
                    if  ((comp != null) && (comp instanceof VAnnotationIF)) {
                        ((VAnnotationIF)comp).adjustBounds(gridNum, gridNum, gridX, gridY);
                    }
               }
               if (newGeom) {
                    newGeom = false;
                    for (i = 0; i < count; i++) {
                        Component comp = target.getComponent(i);
                        if  (comp instanceof VOrient) {
                            continue;
                        }
                        if  (comp instanceof VAnnotationIF) {
                            ((VAnnotationIF)comp).setBoundsRatio(true);
                        }
                    }
               }
/*
               if (orientObjs[0].isVisible()) {
                    int x = 0;
                    int y = (dim.height - fontH) / 2;
                    orientObjs[0].setBounds(x, y, fontH, fontH);
                    x = dim.width - fontH - 1;
                    orientObjs[1].setBounds(x, y, fontH, fontH);
                    x = (dim.width - fontH) / 2;
                    orientObjs[2].setBounds(x, 0, fontH, fontH);
                    y = dim.height - fontH - 1;
                    orientObjs[3].setBounds(x, y, fontH, fontH);
               }
*/

            } // synchronized
        } // layoutContainer
    }

    private class VDummy extends JComponent implements DropTargetListener
    {
        private boolean bFocus = true;
        private VDropHandlerIF dropHandler = null;
        
        public VDummy() {
            setOpaque(false);
            new DropTarget(this, this);
        }

        public void changeFocus(boolean b) {
            if (bFocus != b) {
                bFocus = b;
                repaint();
            }
        }

        public void setDropHandler(VDropHandlerIF d) {
            dropHandler = d;
        }

        public void paint(Graphics g) {
            Dimension  psize = getSize();
            if (bFocus)
                g.setColor(Color.yellow);
            else
                g.setColor(Color.green);
            g.drawLine(0, 0, psize.width, 0);
            g.drawLine(0, 0, 0, psize.height);
            g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
            g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
        }

        public void dragEnter(DropTargetDragEvent e) { }
        public void dragExit(DropTargetEvent e) {}
        public void dragOver(DropTargetDragEvent e) {}
        public void dropActionChanged (DropTargetDragEvent e) {}

        public void drop(DropTargetDropEvent e) {
             if (dropHandler != null)
                dropHandler.processDrop(e, this, true);
        } // drop
    }

    private class VOrient extends JLabel
    {
        public String label;
        public boolean bSelected = false;
        VAnnotatePanel control;

        public VOrient(String l, VAnnotatePanel ctl) {
            this.label = l;
            this.control = ctl;
            setHorizontalAlignment(SwingConstants.CENTER);
            setText(l);
            addMouseListener(new MouseAdapter() {
               public void mouseClicked(MouseEvent evt) {
                   int clicks = evt.getClickCount();
                   int modifier = evt.getModifiers();
                   if ((modifier & (1 << 4)) != 0) {
                      if (clicks >= 2)
                          informControl();
                   }
                }

            });
        }

        public void informControl() {
            setOrientEdit();
        }

        
        public void paint(Graphics g) {
            super.paint(g);
	    Dimension  psize = getSize();
	    if (bSelected)
                g.setColor(Color.yellow);
            else
                g.setColor(Color.blue);
            g.drawLine(0, 0, psize.width, 0);
            g.drawLine(0, 0, 0, psize.height);
            g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
            g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
        }

    }

    private class dummyCheck extends VCheck
    {
        public int height;
        public boolean bOutput = false;
        VAnnotatePanel control;

        public dummyCheck(SessionShare sshare,
                        ButtonIF vif, String typ, VAnnotatePanel ctl) {
           super(sshare, vif, typ);
           this.control = ctl;
        } 

        public Object[][] getAttributes()
        {
            if (!bOutput)
                return null;
            else
                return super.getAttributes();
        }

        public void changeFont() {
           super.changeFont();
           setOrientObjs();
        }
    }

}

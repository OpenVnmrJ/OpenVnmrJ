/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import java.util.*;
import java.lang.*;
import java.io.*;
import java.awt.datatransfer.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.beans.*;
import javax.swing.*;
import javax.swing.event.*;

import vnmr.ui.*;
import vnmr.util.*;

public class VColorImageSelector extends JPanel 
    implements VEditIF, VObjIF, VObjDef, VColorImageListenerIF, PropertyChangeListener
{
    public String fgStr = null;
    public String bgStr = null;
    public String fontName = null;
    public String fontStyle = null;
    public String fontSize = null;
    public String showVal = null;
    public Color  fgColor = Color.black;
    public Color  bgColor, orgBg;
    public Font   font = null;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean bLoading = false;
    private boolean bVobjType = false;

    private JList  imgList;
    private DefaultListModel  listModel;
    private DataFlavor localObjectFlavor;
    private VColorLookupPanel lookupTable;
    private VColorMapSelector mapSelector;
    private int selectedId, num_list;
    private JScrollPane scrollpane;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private VobjEditorIF editor = null;
    private double xRatio = 1.0;
    private double yRatio = 1.0;
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    public static int order = 0;
    private int idNum = 0;
    
    private final static Object[][] m_attributes = {
        {new Integer(SHOW),     "Show condition:"},
    };

    public VColorImageSelector(SessionShare sshare, ButtonIF vif, String typ) {
        this.bVobjType = true;
        buildUi();
        order++;
        this.idNum = order;
    }

    public VColorImageSelector() {
        // this(null, null, null);
        buildUi();
    }

    private void buildUi() {
        bgColor = Util.getBgColor();
        setBackground(bgColor);

        setLayout(new BorderLayout());
        listModel = new DefaultListModel();
        imgList = new JList(listModel);
       // imgList.setDragEnabled(true);
        imgList.setDropMode(DropMode.INSERT);
        imgList.setTransferHandler(new ListTransferHandler(imgList)); 
        imgList.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        num_list = 0;

        if (bVobjType) {
            scrollpane = new JScrollPane(imgList,
               JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
               JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
            add(scrollpane, BorderLayout.CENTER);
            JViewport vp = scrollpane.getViewport();
            vp.setBackground(bgColor);
            scrollpane.setBorder(BorderFactory.createEmptyBorder());

        }
        else
            add(imgList, BorderLayout.CENTER);

        imgList.addListSelectionListener(new ListSelectionListener() {
            public void valueChanged(ListSelectionEvent e) {
                listChange();
            }
        });
        imgList.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                listClicked();
            }
        });

        ml = new MouseAdapter() {
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

        DisplayOptions.addChangeListener(this);
        ImageColorMapEditor.addImageInfoListener(this);
    }

    public void propertyChange(PropertyChangeEvent evt){
        if (evt != null) {
           if (bgStr != null)
                bgColor=DisplayOptions.getColor(bgStr);
            else
                bgColor = Util.getBgColor();
            setBackground(bgColor);
            if (scrollpane != null) {
                JViewport vp = scrollpane.getViewport();
                vp.setBackground(bgColor);
            }
        }
        if (fgStr != null) {
            fgColor = DisplayOptions.getColor(fgStr);
            setForeground(fgColor);
            imgList.setForeground(fgColor);
        }
    }

    public void setEditor(VobjEditorIF obj) {
        editor = obj;
    }

    private void informEditor(MouseEvent e) {
        if (editor != null)
             editor.setEditObj(this);
        else
             ParamEditUtil.setEditObj(this);
    }


    public void setShowValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            int isActive = Integer.parseInt(s);

            if (isActive > 0 )
               setVisible(true);
            else
               setVisible(false);
        }
    }

    public Object[][] getAttributes()
    {
        return m_attributes;
    }

    public String getAttribute(int attr) {
        switch (attr) {
           case FGCOLOR:
                     return fgStr;
           case BGCOLOR:
                     return bgStr;
           case FONT_NAME:
                     return fontName;
           case FONT_STYLE:
                     return fontStyle;
           case FONT_SIZE:
                     return fontSize;
           case SHOW:
                     return showVal;
           default:
                     return null;
        }
    }

    public void setAttribute(int attr, String c) {
        int k;

        switch (attr) {
           case FGCOLOR:
                     fgStr = c;
                     break;
           case BGCOLOR:
                     bgStr = c;
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
           case SHOW:
                     showVal = c;
                     break;
        }
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        if (bgStr == null)
            setOpaque(s);
        if (s) {
            addMouseListener(ml);
            setOpaque(true);
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            xRatio = 1.0;
            yRatio = 1.0;

            setVisible(true);
        }
        else {
           removeMouseListener(ml);
        }
        inEditMode = s;
    }

    public void paint(Graphics g) {
        super.paint(g);
        if (!isEditing)
            return;
        Dimension  psize = getSize();
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

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
         return defLoc;
    }

    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {}
    public void refresh() {}
    public void changeFont() {}
    public void updateValue() {}
    public void setValue(ParamIF p) {}
    public void changeFocus(boolean s) {}
    public ButtonIF getVnmrIF() { return null; }
    public void setVnmrIF(ButtonIF vif) {}
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}


    private void showColorMap(VColorImageInfo obj) {
        if (obj == null)
            return;
        int mapId = obj.getMapId();
        if (mapId < 0)
            return;
        VColorModelPool pool = VColorModelPool.getInstance();
        VColorModel cm = pool.getColorModel(mapId);
        if (cm == null)
            return;
        if (mapSelector != null)
            mapSelector.setSelected(cm.getName());
        if (lookupTable != null)
            lookupTable.setColorModel(cm, obj.getTransparency());
    }

    public void addImageInfo(int id, int order, int mapId,
                   int transparency, String imgName) {
        bLoading = true;
        if (id <= 0 || order <= 0) {
            clearImageInfo();
            bLoading = false;
            return;
        }  
        int rows = listModel.getSize();
        VColorImageInfo obj = null;
        num_list++;
        int index = order - 1;
        if (index < rows) {
            obj = (VColorImageInfo)listModel.getElementAt(index);
            if (obj != null) {
                obj.setId(id);
                obj.setMapId(mapId);
                obj.setText(imgName);
                obj.setTransparency(transparency);
            }
        }
        if (obj == null) {
            obj = new VColorImageInfo(id,mapId,transparency,imgName);
            if (index >= rows)
                listModel.setSize(index + 1);
            listModel.setElementAt(obj, index);
        }
        obj.setOrder(order);
        bLoading = false;
        repaint();
    }

    public void selectImageInfo(int id) {
        if (id <= 0)
            return;
        bLoading = true;
        int rows = listModel.getSize();
        int index = -1;
        VColorImageInfo obj = null;
        
        for (int i = 0; i < rows; i++) {
            obj = (VColorImageInfo)listModel.getElementAt(i);
            if (obj != null) {
                 if (id == obj.getId()) {
                     index = i;
                     break;
                 }
            }
        }
        if (index >= 0) {
            showColorMap(obj);
            imgList.setSelectedIndex(index);
        }
        bLoading = false;
    }

    public void setLookupPanel(VColorLookupPanel p) {
        lookupTable = p;
    }

    public void setMapSelector(VColorMapSelector p) {
        mapSelector = p;
    }

    public void addImageInfo(int i, String name) {
        int row = listModel.getSize();
        VColorImageInfo obj = new VColorImageInfo(i, name);
        int rows = listModel.getSize();
        bLoading = true;
        if (i > rows) {
            listModel.setSize(i);     
        }
        listModel.insertElementAt(obj, i);
        bLoading = false;
    }
    
    public String getImage() {
        return null;
    }

    public void clear() {
        bLoading = true;
        selectedId = -1;
        int rows = listModel.getSize();
        if (scrollpane != null || rows > 3) {
            listModel.removeAllElements();
            bLoading = false;
            return;
        }
        for (int i = 0; i < rows; i++) {
            VColorImageInfo obj = (VColorImageInfo)listModel.getElementAt(i);
            if (obj != null)
                obj.clear();
        }
        bLoading = false;
    }

    public void clearImageInfo() {
         clear();
         num_list = 0;
    }

    private void listClicked()
    {
        VColorImageInfo obj = (VColorImageInfo)imgList.getSelectedValue();
        if (obj == null)
            return;
        int mapId = obj.getMapId();
        if (mapId < 0)
            return;
        showColorMap(obj);
    }

    public void listChange()
    {
        VColorImageInfo obj = (VColorImageInfo)imgList.getSelectedValue();
        if (obj == null)
            return;
        int mapId = obj.getMapId();
        if (mapId < 0)
            return;
        showColorMap(obj);
        if (bLoading) {
            selectedId = obj.getId();
            return; 
        }
        if (obj.getId() == selectedId)
            return;
        selectedId = obj.getId();
        ExpPanel exp = Util.getActiveView();
        if (exp == null)
            return;
        String mess = new StringBuffer().append("jFunc(").append(VnmrKey.SELECTIMG)
            .append(", ").append(obj.getId()).append(",").append(obj.getOrder()).append(",").append(mapId).append(")\n").toString();

        exp.sendToVnmr(mess);
        ImageColorMapEditor.selectColorInfo(this, selectedId);
    }

    private void changeOrder(Object obj, int order) {
        if (!(obj instanceof VColorImageInfo))
           return;
        VColorImageInfo vobj = (VColorImageInfo) obj;
        vobj.setOrder(order+1);
        ExpPanel exp = Util.getActiveView();
        if (exp == null)
            return;
        String mess = new StringBuffer().append("jFunc(").append(VnmrKey.IMGORDER)
            .append(", ").append(vobj.getId()).append(", ").append(order+1).append(")\n").toString();
        exp.sendToVnmr(mess);
    }

    private class ListTransferHandler extends TransferHandler {
       private JList ximgList = null;

       public ListTransferHandler(JList list) {
          this.ximgList = list;
       }

       @Override
       protected Transferable createTransferable(JComponent c) {
          return new ImageListTransferable(c);
       }

       @Override
       public boolean canImport(TransferHandler.TransferSupport info) {
          boolean b = info.getComponent() == ximgList && info.isDrop() && info.isDataFlavorSupported(localObjectFlavor);
          ximgList.setCursor(b ? DragSource.DefaultMoveDrop : DragSource.DefaultMoveNoDrop);
          return b;
       }

       @Override
       public int getSourceActions(JComponent c) {
          return TransferHandler.MOVE;
       }

       @Override
       public boolean importData(TransferHandler.TransferSupport info) {
          JList target = (JList) info.getComponent();
          JList.DropLocation dl = (JList.DropLocation) info.getDropLocation();
          int index = dl.getIndex();
          int max = ximgList.getModel().getSize();
          int k = ximgList.getSelectedIndex();
          target.setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
          if (max < 2 || num_list < 2)
              return true;
          if (index < 0 || index > max)
              return true;
          if (k >= max)
              k = max - 1;
          if (index >= max)
              index = max - 1;
          if (index >= num_list)
              index = num_list - 1;
          if (k == index)
              return true;
          VColorImageInfo vobj = (VColorImageInfo)listModel.getElementAt(k);
          if (vobj.getMapId() < 0)
              return true;
          
          Object obj = listModel.remove(k);
          if (obj == null)
              return true;
          if (!(obj instanceof VColorImageInfo))
              return true;
          listModel.insertElementAt(obj, index);
          changeOrder(obj, index);
          return true;
       }

       @Override
       protected void exportDone(JComponent c, Transferable t, int act) {
          if (act == TransferHandler.MOVE) {
             ximgList.setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
          }
       }
    }  // end of class ListTransferHandler

    private class ImageListTransferable implements Transferable {
        JComponent componentToPass;

        public ImageListTransferable(JComponent comp) {
            // Save the component, which is actually the JTree
            componentToPass = comp;
        }

        public Object getTransferData(DataFlavor flavor)
                throws UnsupportedFlavorException, IOException {
            return componentToPass;
        }
        public DataFlavor[] getTransferDataFlavors() {
            // Make a list of length 1 containing our dataFlavor
            DataFlavor flavorList[] = new DataFlavor[1];
            flavorList[0] = localObjectFlavor;
            return flavorList;
        }
        public boolean isDataFlavorSupported(DataFlavor flavor) {
            if(flavor.equals(localObjectFlavor))
                return true;
            else
                return false;
        }
    }
}


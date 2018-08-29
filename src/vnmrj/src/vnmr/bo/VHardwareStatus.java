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
import java.io.*;
import javax.swing.*;
import javax.swing.border.*;
import java.util.*;
import java.beans.*;
import vnmr.util.*;
import vnmr.ui.*;

/**
 * hardware status indicators
 */
public class VHardwareStatus extends JComponent
    implements DropTargetListener, VObjDef, VObjIF,StatusListenerIF,VStatusIF,
    PropertyChangeListener
{
    protected int state = INACTIVE;
    protected String statcol="fg";
    private JLabel label=null;
    private int width=14;
    private Font font;
    private BorderLayout layout;

    private String type = null;
    private String fg = "black";
    private Color  fgColor = Color.black;
    private Color  bgColor= null;

    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private ButtonIF vnmrIf;
    private MouseAdapter ml;

    private String fontName = "Dialog";
    private String fontStyle = "Bold";
    private String fontSize = "12";

    private Dimension defDim = new Dimension(0, 0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);

    private static int ONCOLOR=0;
    private static int OFFCOLOR=1;
    private static int INTERACTIVECOLOR=2;
    private static int READYCOLOR=3;
    private static String[] names = {"On","Off","Interactive","Ready"};

    private Color  colors[]={Global.ONCOLOR,
                             Global.OFFCOLOR,
                             Global.INTERACTIVECOLOR,
                             Global.READYCOLOR};

    /** constructor (for LayoutBuilder) */
    public  VHardwareStatus(SessionShare sshare, ButtonIF vif, String typ) {
        type = typ;
        vnmrIf = vif;
        setBorder(BorderFactory.createBevelBorder(BevelBorder.LOWERED));
        layout=new BorderLayout();
        setLayout(layout);
        label = new JLabel("Loading");
        label.setBorder(BorderFactory.createEmptyBorder());
        //font=new Font("Dialog",Font.BOLD,12);
        font = DisplayOptions.getFont("Dialog", Font.BOLD, 12);
        label.setFont(font);

        add(label, BorderLayout.CENTER);
        // setPreferredSize(new Dimension(100,30));
        setLabel("Loading");
        if(Global.STATUS_TO_FG==false)
            statcol="bg";
        changeFont();
        setPreferredSize(new Dimension(65, 20));
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & (1 << 4)) != 0)
                if (clicks >= 2)
                ParamEditUtil.setEditObj((VObjIF) evt.getSource());
            }
        };
       DisplayOptions.addChangeListener(this);
       getStatusColors();
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        if(fg!=null){
            fgColor=DisplayOptions.getColor(fg);
            setForeground(fgColor);
        }
        getStatusColors();
        setStatusColors(state);
        changeFont();
        repaint();
    }

    private void getStatusColors(){
        for(int i=0;i<names.length;i++){
            String name=DisplayOptions.getOption(DisplayOptions.COLOR,names[i]);
            if(name !=null)
                colors[i]=DisplayOptions.getColor(name);
        }
    }

    private void setStatusColors(int choice){
        switch(choice){
        default:
        case READY:
            setColor(colors[READYCOLOR]);
            break;
        case ACQUIRING:
            setColor(colors[ONCOLOR]);
            break;
        case INACTIVE:
            setColor(colors[OFFCOLOR]);
            break;
        case REGULATING:
            setColor(colors[ONCOLOR]);
            break;
        case INTERACTIVE:
            setColor(colors[INTERACTIVECOLOR]);
            break;
        }
    }

    /** Update state from Infostat. */
    public void updateStatus(String msg) {
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals("status")) {
                setState(tok.nextToken());
            }
        }
    }
    /** Get state option. */
    protected int getState(String s){
        if(s.equals("Inactive"))
            return INACTIVE;
        else if(s.equals("SpinReg"))
            return REGULATING;
        else if(s.equals("VTReg"))
            return REGULATING;
        else if(s.equals("Interactive"))
            return INTERACTIVE;
        else if(s.equals("Acquiring"))
            return ACQUIRING;
        else
            return READY;
   }

    /** Get state display string. */
    protected String getStateString(int s) {
        switch (s) {
        default:
        case READY:
            return Util.getLabel("sREADY");
        case INACTIVE:
            return Util.getLabel("sINACTIVE");
        case INTERACTIVE:
            return Util.getLabel("sINTERACTIVE");
        case REGULATING:
            return Util.getLabel("sREGULATING");
        case ACQUIRING:
            return Util.getLabel("sACQUIRING");
        }
    }

    private void setLabel(String status){
        int length=status.length();
        if(length<width){
            StringBuffer sbLabel=new StringBuffer();
            int pad=(width-length)/2;
            int i;
            for(i=0;i<pad;i++)
                sbLabel.append(" ");
            sbLabel.append(status);

            for(i=0;i<pad;i++)
                sbLabel.append(" ");

            label.setText(sbLabel.toString());
        }
        else
            label.setText(status);
        label.invalidate();
    }

    /** set state option. */
    public void setState(String parm){
        state=getState(parm);
        setColor(Color.black,Global.BGCOLOR);
        setLabel(getStateString(state));
        setStatusColors(state);
        setToolTipText(parm);
        invalidate();
    }

    /** Set status colors. */
    protected void setColor(Color f, Color b){
        if (statcol.equals(status_color[0])){
            label.setOpaque(false);
            label.setForeground(f);
        }
        else{
            label.setOpaque(true);
            label.setBackground(b);
        }
    }

    /** Set status colors. */
    protected void setColor(Color c){
        if (statcol.equals(status_color[0])){
            label.setOpaque(false);
            label.setForeground(c);
        }
        else{
            label.setOpaque(true);
            label.setBackground(c);
        }
    }

    // VObjIF interface

    /** set an attribute. */
    public void setAttribute(int attr, String c) {
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case STATCOL:
            statcol = c;
            repaint();
            break;
        case FONT_NAME:
            fontName = c;
            changeFont();
            repaint();
            break;
        case FONT_STYLE:
            fontStyle = c;
            changeFont();
            repaint();
            break;
        case FONT_SIZE:
            fontSize = c;
            changeFont();
            repaint();
            break;
        }
    }

    /** get an attribute. */
    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case STATCOL:
            return statcol;
        default:
            return null;
        }
    }
    public void setEditStatus(boolean s) {isEditing = s;}
    public void setEditMode(boolean s) {
        if (s) {
            addMouseListener(ml);
            defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
        } else
            removeMouseListener(ml);
        inEditMode = s;
    }
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {fg=s;}
    public void refresh() {}
    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }
    public void changeFont() {
        Font font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
    }
    public void updateValue() {}
    public void setValue(ParamIF p) {}
    public void setShowValue(ParamIF p) {}
    public void changeFocus(boolean s) {isFocused = s;}
    public ButtonIF getVnmrIF() {return vnmrIf;}
    public void setVnmrIF(ButtonIF vif) {vnmrIf=vif;}

   // DropTargetListener interface

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

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

} // class VHardwareStatus

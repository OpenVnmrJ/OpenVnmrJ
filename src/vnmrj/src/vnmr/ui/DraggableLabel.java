/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.beans.*;
import java.util.*;
import java.io.Serializable;
import java.awt.datatransfer.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;

import  vnmr.bo.ShufflerItem;
import  vnmr.bo.VObjDef;
import  vnmr.ui.shuf.*;
import  vnmr.util.*;

public class DraggableLabel extends JLabel implements   DragSourceListener,
                                                        DragGestureListener,
							PropertyChangeListener,
							VObjDef {
    private static Border b = BorderFactory.createEtchedBorder();
    DragSource dragSource;
    LocalRefSelection lrs;
    String labelString;
    String fileName,name,application,author,source;
    String fontName, fontSize, fontStyle;
    String fg, bg, type;
    String fgColorTx, bgColorTx;
    String bykeywords;
    HashArrayList subList=null;
    private Color fgColor, bgColor;
    String info;

    public DraggableLabel (String s, String font) {
        super(s, CENTER);
        labelString=s;
        dragSource = new DragSource();
        dragSource.createDefaultDragGestureRecognizer(
                this, DnDConstants.ACTION_COPY, this);
        setBorder(b);
        fontName = font;
        fg =fontSize = fontStyle = fontName;
        updateProperty();
        DisplayOptions.addChangeListener(this);
        
        addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent evt) {

                 setActive(true);
            }

            public void mouseReleased(MouseEvent evt) {

                 setActive(false);
            } 

            public void mouseEntered(MouseEvent evt) {
            } 

         });
    }
    
    public DraggableLabel(String s) {
        this(s, "PlainText");
        /****
        super(s, CENTER);
        labelString=s;
        dragSource = new DragSource();
        dragSource.createDefaultDragGestureRecognizer(
				this, DnDConstants.ACTION_COPY, this);
        setBorder(b);
        // If not specified as an arg, default it
        fontName = new String("PlainText");
        fg =fontSize = fontStyle = fontName;
        DisplayOptions.addChangeListener(this);
        ****/
    }

    private void setActive(boolean s) {
        if (s) {
            setForeground(Util.getSelectFg());
            setBackground(Util.getSelectBg());
        }
        else {
            setForeground(fgColor);
            setBackground(bgColor);
        }
    }

    private String  appTab;
    public void setAppTab(String s) { appTab = s; }
    public String getAppTab() { return appTab; }

    /* DragGestureListener */
    public void dragGestureRecognized (DragGestureEvent e) {
        // StringSelection text = new StringSelection (getText() );
        ShufflerItem si = getShufflerItem();
        
        // If null, it was not meant to be dragged, such as an ExpSel menu
        if(si == null)
            return;
        lrs = new LocalRefSelection(si);
	    dragSource.startDrag ( e, DragSource.DefaultCopyDrop, lrs, this);
    }

    public ShufflerItem getShufflerItem()
    {
        // If no fileName, don't try to get a shufflerItem
        if(fileName == null)
            return null;
        
        // The fileName we have is for the local mount.  We need
        // the dhost and dpath for the ShufflerItem
        Vector mp = MountPaths.getPathAndHost(fileName);
        String dhost = (String) mp.get(Shuf.HOST);
        String dpath = (String) mp.get(Shuf.PATH);

        
        ShufflerItem si = new ShufflerItem();
        si.setValue("attrname0","name");
        si.setValue("attrname1","application");
        si.setValue("attrname2","author");
        si.setValue("value0",name);
        if(application != null)
            si.setValue("value1",application);
        if(author != null)
            si.setValue("value2",author);
        si.setValue("filename",name);
        si.setValue("fullpath",dpath);
        si.setValue("source",source);
        si.setValue("objType",Shuf.DB_PROTOCOL);
        si.setValue("hostName",dhost);
        si.setValue("hostFullpath",dhost+":"+dpath);
        if(author != null)
            si.setValue("owner",author);
        return si;
    }

    /* DragSourceListener */
    public void dragDropEnd (DragSourceDropEvent e) {
        setActive(false);
    }

    public void dragEnter (DragSourceDragEvent e) {
    }

    public void dragExit (DragSourceEvent e) {
    }

    public void dragOver (DragSourceDragEvent e) {
    }

    public void dropActionChanged (DragSourceDragEvent e) {
    }

    public void updateProperty() {
        // Use foreground color if set
        if(fgColorTx != null && !fgColorTx.equals("")) {
            fgColor=DisplayOptions.getColor(fgColorTx);
            setForeground(fgColor);
        }
        else {
            // default
            fgColor=DisplayOptions.getColor("PlainText");
            setForeground(fgColor);
        }
        // Use background color if set
        if(bgColorTx != null && !bgColorTx.equals("")) {
            bgColor=DisplayOptions.getColor(bgColorTx);
            setBackground(bgColor);
        }
        else {
            // default
            bgColor = Util.getBgColor();
            setBackground(bgColor);
        }

        Font font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
    }

    /* PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        updateProperty();
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setBykeywords(String byKeywords) {
        bykeywords = byKeywords;
    }
    public void setApplication(String appl) {
        application = appl;
    }

    public void setAuthor(String author) {
        this.author = author;
    }

    public void setFileName(String path) {
        fileName = path;
    }

    public void setSource(String src) {
        source = src;
    }

    public void setSubList(HashArrayList list) {
        subList = list;
    }

    public HashArrayList getSubList() {
        return subList;
    }

    public String getFullpath() {
        return fileName;
    }
    public void setInfo(String info) {
        this.info = info;
    }
    public String getInfo() {
        return info;
    }
}


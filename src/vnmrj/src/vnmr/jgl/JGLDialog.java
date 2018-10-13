/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import vnmr.ui.ExpPanel;
import vnmr.ui.SessionShare;
import vnmr.util.*;
import vnmr.bo.*;

import java.io.*;
import java.util.Hashtable;

import javax.swing.*;

import java.awt.*;
import java.awt.event.*;
import java.beans.*;

import javax.swing.JPanel;

public class JGLDialog extends JFrame 
  implements PropertyChangeListener,  ActionListener, VObjDef, JGLDef {
    private static final long serialVersionUID = 1L;
    public JGLGraphics graphics=null;
    private Container contentPane;
    private boolean showing=false;
    private ExpPanel exppanel;
    static int show_state=0;
    int width=500;
    int height=450;
    public int glSize=50;
    contentLayout layout=new contentLayout();
    private JGLComMgr comMgr=null;
    private boolean layout_changed=true;
    private boolean joined=false;
    private SessionShare sshare;
    private JPanel glw=null;
    private boolean initialized=false;
    
    Dimension gdim=new Dimension(width,height);  //  default graphics window size    

    public JGLDialog(JGLComMgr mgr,ExpPanel exp,SessionShare ss) {
        comMgr=mgr;
        exppanel=exp;
        sshare=ss;
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
               saveStatus();
               setVisible(false);
               showing=false;
            }
        });
        contentPane = getContentPane();
        
        DisplayOptions.addChangeListener(this);

        graphics=new JGLGraphics(comMgr);
        graphics.setPreferredSize(gdim);
        glw=exppanel.getOglPanel();
        
        comMgr.setDropTarget(graphics);
        comMgr.addListener(graphics);
 
        if(graphics.nativeGl())
            setTitle("OGL Graphics <native> ");
        else
            setTitle("OGL Graphics <java> ");
     
        contentPane.add(graphics);       
        contentPane.setLayout(layout);
        
        setSize(width, height);
        setLocation( 500, 400 );
        setResizable(true);
        this.setAlwaysOnTop(true);
        initStatus();
        setVisible(false);
    }

    /**
     * save window properties in persistence file
     * - Currently not supported
     */
    public void  saveStatus() {
    }
    /**
     * restore window properties from persistence
     * - Currently not supported
     */
    public void  initStatus() {
    }

    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        //if(DisplayOptions.isUpdateUIEvent(evt)){           
            Color bg = DisplayOptions.getColor("graphics18");
            glw.setBackground(bg);
        //}
    }

    public JGLGraphics getJGLGraphics(){
        return graphics;
    }

    public void destroy(){
        graphics.destroy();
    }
    /**
     * show or hide graphics window
     * @param value
     */
    public void setShowWindow(String value) {
        if (DebugOutput.isSetFor("gldialog"))
            Messages.postDebug("JGLDialog.setShowWindow:"+value);
        if(value.equals("true")){
        	if(joined)
        		glw.setVisible(true);
        	else
        		setVisible(true);
        	showing=true;
        }
        else if(value.equals("false")){
        	if(joined)
        		glw.setVisible(false);
        	else
        		setVisible(false);
        	showing=false;        	
        }
        else{ // toggle
	    	if(joined){
	    		if(showing){
	    			glw.setVisible(false);
	    			showing=false;
	    		}
	    		else{
	    			glw.setVisible(true);
	    			showing=true;   			
	    		}
	    	}
	    	else if(isShowing()){
	    		setVisible(false);
	    		showing=false;
	    	}
	    	else{
	    		setVisible(true);
    			showing=true;   			
	    	}
        }
 	    comMgr.comCmd(JGLComMgr.G3DSHOWING, showing?1:0);
    	if(!initialized&&showing){
    		comMgr.sendDataRequest();
    		initialized=true;
    	}
   }
    public void setSplitRatio(int r) {
    	glSize=r;
    }
    public int getSplitRatio() {
    	return glSize;
    }

    /**
     * Join or detach graphics window and viewport
     * @param value
     */
   public void setJoinViewport(String value) {
        if (DebugOutput.isSetFor("gldialog"))
            Messages.postDebug("JGLDialog.setJoinViewport:"+value);
        boolean join=false;
	    if (value.equals("full")){
	    	glSize=100;
	    	join=true;
	    }
	    else if(value.equals("split")){
	    	glSize=50;
	    	join=true;	    	
	    }
	    else
	    	join=false;
    	if(join && !joined){
    		super.setVisible(false);
    		contentPane.remove(graphics);
    		glw.add(graphics);
    		glw.setVisible(true);
     	}
    	else if(!join && joined){
    		glw.setVisible(false);
    		glw.remove(graphics);
    		add(graphics);
 			layout_changed=true;
			invalidate();
    		super.setVisible(true);
     	}
    	joined=join;
    	showing=true;
    	if(!initialized)
    		comMgr.sendDataRequest();
    	comMgr.comCmd(JGLComMgr.G3DSHOWING, 1);
    	initialized=true;
    	saveStatus();
    }

    /**
     * called as a result of a resize event in a viewport-joined panel
     */
    public void setChangedPanel() {
    	Rectangle bounds=glw.getBounds();
    	graphics.setBounds(bounds);
    	Dimension dim=new Dimension(bounds.width, bounds.height);
    	graphics.setPreferredSize(dim);
    	graphics.reshape(0, 0, bounds.width, bounds.height);
        if (DebugOutput.isSetFor("gldialog"))
            Messages.postDebug("JGLDialog.setChangedPanel:"+ bounds);
    }

    public void graphicsCmd(DataInputStream ins, int code){
        comMgr.graphicsCmd(ins,code);
     }
    public boolean comCmd(String value) {
        if(comMgr.comCmd(value))
            return true;
        return false;
    }
    public void setVisible(boolean b) {
	    showing=b;
	    graphics.setShowing(b);
	    super.setVisible(b);
    }
    
     /** handle button events. */
    public void actionPerformed(ActionEvent e ) {
        String cmd = e.getActionCommand();
        if(cmd.equals("close"))  {
        	saveStatus();
            setVisible(false);
            dispose();
        }
    }
    class contentLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
             return new Dimension(width, height);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(100, 100); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                if (DebugOutput.isSetFor("gldialog"))
                    Messages.postDebug("JGLDialog.layoutContainer:"+width+" "+height);
                int w=width;
                int h=height;
                width=contentPane.getWidth();
                height=contentPane.getHeight();
                boolean newsize=(w!=width || h!=height);
                if(!newsize && !layout_changed) 
                    return;
                graphics.setBounds(1, 1, width, height);

                gdim=new Dimension(width, height);
                graphics.setPreferredSize(gdim);
                layout_changed=false;
            }
        }
    }
}

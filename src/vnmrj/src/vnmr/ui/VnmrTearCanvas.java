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
// import java.awt.peer.*;
import com.sun.java.swing.plaf.motif.*;
import java.awt.datatransfer.*;
import sun.awt.*;
import java.lang.reflect.*;

import java.awt.dnd.*;
import java.awt.event.*;
import java.beans.*;
import java.io.*;
import java.util.*;
import java.net.*;
import javax.swing.*;
import javax.swing.plaf.*;
import javax.swing.border.*;
import java.awt.image.BufferedImage;
import java.awt.image.IndexColorModel;

import vnmr.bo.*;
import vnmr.util.*;


/**
 * The container for experiment windows. A key responsibility is to
 * receive drop events.
 */

public class VnmrTearCanvas extends JFrame
	 	implements  VGaphDef, VnmrKey
{
    /** session share */
    private SessionShare sshare;
    private BufferedImage bimg;
    private BufferedImage newImg;
    private boolean isBacking = false;
    private CanvasIF  canvasIf;
    private ButtonIF  expIf;
    private DrawWindow  dWindow;
    private Container  container;
    private boolean  xGraph = false; // native graphics

    /**
     * constructor
     * @param sshare session share
     */
    public VnmrTearCanvas(CanvasIF ap, ButtonIF sp) {
	this.canvasIf = ap;
	this.expIf = sp;

	setTitle("TearOff Window");
	String backUp = System.getProperty("backingStore");
	xGraph = Util.isNativeGraphics();
	if (!xGraph) {
	    isBacking = true;
	    if ((backUp != null) && (backUp.equals("no")))
	        isBacking = false;
	}
	container = getContentPane();
	dWindow = new DrawWindow();
	dWindow.setPreferredSize(canvasIf.getWinSize());
	container.add(dWindow);
	canvasIf.registerTearOff(this);
	pack();
    } // VnmrTearCanvas()


    public void  clearWindow() {
	if (bimgc != null)
		bimgc.clearRect(0, 0, winSize.width, winSize.height);
	repaint();
    } 

    public void processWindowEvent(WindowEvent e) {
	int id = e.getID();
	switch (id) {
	  case WindowEvent.WINDOW_CLOSING:
		expIf.tearOffOpen(false);
		break;
	  case WindowEvent.WINDOW_OPENED:
		sendWindowId();
		expIf.tearOffOpen(true);
		break;
	  case WindowEvent.WINDOW_ICONIFIED:
		expIf.tearOffOpen(false);
		break;
	  case WindowEvent.WINDOW_DEICONIFIED:
		sendWindowId();
		expIf.tearOffOpen(true);
		break;
	  case WindowEvent.WINDOW_CLOSED:
		expIf.tearOffOpen(false);
		break;
	  default:
		break;
	}
	super.processWindowEvent(e);
    }

    public class DrawWindow extends JComponent {
	public DrawWindow() {
	   setBackground(Color.black);
	   addComponentListener(new ComponentAdapter() {
	    	public void componentResized(ComponentEvent evt) {
		   winSize = getSize();
		   mygc = getGraphics();
		   mygc.setClip(0, 0, winSize.width, winSize.height);
	           if (isBacking) {
			if ((bimgW < winSize.width) ||
				(bimgH < winSize.height)) {
			   newImg = (BufferedImage) createImage(winSize.width, winSize.height);
			   bimgc = newImg.createGraphics();
			   bimgc.setBackground(Color.black);
			   bimgc.clearRect(0, 0, winSize.width, winSize.height);
			   if (bimg != null) {
		    		bimgc.drawImage(bimg, 0, 0, bimgW, bimgH, null);
		    		bimg = null;
			   }
			   bimg = newImg;
		    	   bimgW = winSize.width;
		    	   bimgH = winSize.height;
		    	   newImg = null;
			}
	          } // if isBacking
	          canvasIf.setTearGC(mygc, bimgc);
		  if (xGraph)
			expIf.sendToVnmr("jFunc("+PAINT2+")\n");
	        }

	    	public void componentShown(ComponentEvent evt) {
		  if (xGraph)
			expIf.sendToVnmr("jFunc("+PAINT2+")\n");
	        }
	   });


	   addMouseListener(new MouseAdapter() {
         	public void mousePressed(MouseEvent ev) {
		   if (!xGraph)
			expIf.childMouseProc(ev, false, false);
		}
         	public void mouseReleased(MouseEvent ev) {
		   if (!xGraph)
			expIf.childMouseProc(ev, true, false);
		}
	   });
	   addMouseMotionListener(new MouseMotionAdapter() {
                public void mouseDragged(MouseEvent ev) {
		   if (!xGraph)
			expIf.childMouseProc(ev, false, true);
		}
	   });
	} // DrawWindow constructor

        public void  paint(Graphics g) {
	    if (xGraph) {
		ComponentEvent e = new ComponentEvent(this,
                                ComponentEvent.COMPONENT_SHOWN);
                Toolkit.getDefaultToolkit().getSystemEventQueue().postEvent(e);
		return;
	    }
	    if (bimgc != null) {
		g.drawImage(bimg, 0, 0, this);
	    }
	    else {
		super.paint(g);
	    }
	}
    } // Class DrawWindow


    private void sendWindowId() {
	Point pt1 = getLocationOnScreen();
        Point pt2 = dWindow.getLocationOnScreen();
        int dx = pt2.x - pt1.x + 1;
        int dy = pt2.y - pt1.y;
	winSize = dWindow.getSize();
        String str = "jFunc("+VSIZE2+","+winSize.width+","+winSize.height+", "+dx+", "+dy+")\n";
        expIf.sendToVnmr(str);

	if (xid > 0)
            return;
/**
        ComponentPeer peer = getPeer();
        if (peer == null)
            return;
        DrawingSurface surf = (DrawingSurface) peer;
        DrawingSurfaceInfo info = surf.getDrawingSurfaceInfo();
        Class d_class = info.getClass();
        Method getHandle = null;
        try {
                getHandle = d_class.getMethod("getDrawable", null);
        }
        catch (NoSuchMethodException e) { }
        catch (SecurityException e) { }
        if (getHandle == null)
            return;
        info.lock();

        Integer handle = null;
        try {
                handle = (Integer) getHandle.invoke(info, null);
        }
        catch (InvocationTargetException e) {}
        catch (IllegalAccessException e) {}
        info.unlock();
        if (handle == null) {
                return;
        }
	xid = handle.intValue();
**/
	xid = VNMRFrame.getVNMRFrame().getXwinId(this);
	str = "jFunc("+WINID2+", '"+xid+"', '1')\n";
        expIf.sendToVnmr(str);
    }

    private Graphics mygc;
    private Graphics imgc;
    private Graphics2D bimgc;
    private Dimension winSize;
    private int winId = 0;
    private boolean xorMode = false;
    private int colorSize = 120;

    private int bimgW = 0;
    private int bimgH = 0;
    private int xid = 0;
    private int textColorIndex = 0;
    Color textColor;
    Color graphColor;
    IndexColorModel cmIndex;
    private byte[] redByte;
    private byte[] grnByte;
    private byte[] bluByte;


} // class VnmrTearCanvas


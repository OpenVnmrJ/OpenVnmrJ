/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.UnsupportedFlavorException;
import java.io.DataInputStream;
import java.io.File;
import java.io.IOException;
import java.text.DecimalFormat;
import java.awt.dnd.*;
import java.util.*;
import java.awt.Component;
import java.util.concurrent.Semaphore;

import vnmr.bo.ShufflerItem;
import vnmr.bo.VObjAdapter;
import vnmr.bo.VObjIF;
import vnmr.ui.ExpPanel;
import vnmr.ui.LocalRefSelection;
import vnmr.ui.SessionShare;
import vnmr.util.DebugOutput;
import vnmr.util.Messages;
import vnmr.util.ParamIF;
import vnmr.util.QuotedStringTokenizer;
import vnmr.ui.ExpListenerIF;

import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.ByteOrder;

public class JGLComMgr extends JGLData implements JGLDef, DropTargetListener,
        JGLComListenerIF, ExpListenerIF {
    private int show = 0;
    private int status = 0;
    private int prefs = 0;
    private DecimalFormat fltformat1 = new DecimalFormat("0.000");
    public ExpPanel exppanel;
    public boolean connected = false;
    public boolean initialized = false;
    public boolean batch = false;
    public boolean showing = false;
    private static Hashtable vjcmnds = null;
    private final Semaphore vnmrsem = new Semaphore(1, true);
    static final int MMAPTEST = 30;
    static final int MMAPPED = 0x00000040;

    static final int XFRAME = 1;
    static final int YFRAME = 2;
    static final int ZFRAME = 3;
    static final int SFRAME = 4;

    private PnewObj pntObj = new PnewObj("g3dpnt");
    private PnewObj rotObj = new PnewObj("g3drot");
    private PnewObj fltObj = new PnewObj("g3df");
    private PnewObj statObj = new PnewObj("g3ds");
    private PnewObj prefsObj = new PnewObj("g3dp");
    private PnewObj showObliqueObj = new PnewObj("showObliquePlanesPanel",false);
    Point3D vpnt = new Point3D(0.5, 0.5, 0.5);
    Point3D vrot = new Point3D();

    public int viewport;
    public String version=null;

    int oldframe = 0;

    private ArrayList<JGLComListenerIF> listenerList = new ArrayList<JGLComListenerIF>();

    public JGLComMgr(ExpPanel ex) {
        exppanel = ex;
        viewport = ex.getViewId() + 1;
        if (vjcmnds == null) {
            vjcmnds = new Hashtable<String, cmndData>();
            vjcmnds.put("image", new cmndData(CMND, G3DIMAGE));
            vjcmnds.put("reset", new cmndData(CMND, G3DRESET));
            vjcmnds.put("connect", new cmndData(CMND, G3DCONNECT));
            vjcmnds.put("step", new cmndData(CMND, G3DSTEP));
            vjcmnds.put("init", new cmndData(CMND, G3DINIT));
            vjcmnds.put("version", new cmndData(CMND, G3DSHOWREV));
            vjcmnds.put("setsem", new cmndData(CMND, G3DSETSEM));
            vjcmnds.put("run", new cmndData(CMND, G3DRUN));
            vjcmnds.put("reverse", new cmndData(CMND, G3DREVERSE));
            vjcmnds.put("batch", new cmndData(CMND, G3DBATCH));
            vjcmnds.put("showing", new cmndData(CMND, G3DSHOWING));

            vjcmnds.put("g3dinit", new cmndData(CMND, G3DINIT));
            vjcmnds.put("g3di", new cmndData(SHOWFIELD, G3DI));
            vjcmnds.put("g3ds", new cmndData(STATUSFIELD, G3DS));
            vjcmnds.put("g3dp", new cmndData(PREFS, G3DP));
            vjcmnds.put("g3dgi", new cmndData(INTEGER, G3DGI));
            vjcmnds.put("g3dgf", new cmndData(FLOAT, G3DGF));
            vjcmnds.put("g3df", new cmndData(FLOAT, G3DF));
            vjcmnds.put("g3dpnt", new cmndData(POINT, G3DPNT));
            vjcmnds.put("g3drot", new cmndData(POINT, G3DROT));
            vjcmnds.put("g3dscl", new cmndData(POINT, G3DSCL));
            vjcmnds.put("g3daxis", new cmndData(STRING, G3DAXIS));
            vjcmnds.put("volmapdir", new cmndData(STRING, VOLMAPDIR));
            vjcmnds.put("g3dversion", new cmndData(STRING, G3DVERSION));
        }
        // setShowing(true);
        exppanel.addExpListener(this); // register as a vnmrbg listener
        showObliqueObj.setCallback(true,false);

    }

   // Methods required for the implementation of ExpListenerIF
    
    public void updateValue(Vector v) {
        if(DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall"))
        	Messages.postDebug("JGLComMgr.updateValue "+v);
        pntObj.checkUpdate(v);
        rotObj.checkUpdate(v);
        fltObj.checkUpdate(v);
        prefsObj.checkUpdate(v);
        if(statObj.callbackSet())
            statObj.checkUpdate(v);
        showObliqueObj.checkUpdate(v);
        String s = v.toString();
        int n=s.indexOf("g3dup");
        if(n>0){
        	String typestr=s.substring(7,9);
        	int newtype=Integer.parseInt(typestr);
        	int oldtype=type|dim;
        	//if((status & AUTOXFER)>0 && newtype !=type && showing){
        	if((status & AUTOXFER)>0&& showing && newtype!=oldtype ){
        	    if(DebugOutput.isSetFor("glxfer")||DebugOutput.isSetFor("glall"))
        	         Messages.postDebug("JGLComMgr.updateValue requesting data xfer type:"+newtype+" "+v);
        	    sendDataRequest();
         	}
       	    type=newtype & DTYPE;
        }
    }

    public void updateValue() {
        //System.out.println("JGLComMgr.updateValue");
    }

    public void sendDataRequest(){
        String str = "jFunc(" + G3D + "," + GETDATA + ")";
        sendToVnmr(str);

    }
    // Methods required for the implementation of JGLComListenerIF

    public void setComCmd(int code, int value) {
        for (int i = 0; i < listenerList.size(); i++) {
            ((JGLComListenerIF) listenerList.get(i)).setComCmd(code, value);
        }
    }

    public void setStringValue(int id, String value) {
        for (int i = 0; i < listenerList.size(); i++) {
            ((JGLComListenerIF) listenerList.get(i)).setStringValue(id, value);
        }
    }

    public void setFloatValue(int id, int index, float value) {
        for (int i = 0; i < listenerList.size(); i++) {
            ((JGLComListenerIF) listenerList.get(i)).setFloatValue(id, index,
                    value);
        }
    }

    public void setIntValue(int id, int index, int value) {
        for (int i = 0; i < listenerList.size(); i++) {
            ((JGLComListenerIF) listenerList.get(i)).setIntValue(id, index,
                    value);
        }
    }

    public void setPointValue(int id, Point3D value) {
        for (int i = 0; i < listenerList.size(); i++) {
            ((JGLComListenerIF) listenerList.get(i)).setPointValue(id, value);
        }
    }

    public void setData(int flags, JGLData data) {
        for (int i = 0; i < listenerList.size(); i++) {
            ((JGLComListenerIF) listenerList.get(i)).setData(flags, data);
        }
    }

    // Methods required for the implementation of DropTargetListener

    public void dragEnter(DropTargetDragEvent e) {
    }

    public void dragExit(DropTargetEvent e) {
    }

    public void dragOver(DropTargetDragEvent e) {
    }

    public void dropActionChanged(DropTargetDragEvent e) {
    }

    public void drop(DropTargetDropEvent e) {
        try {
            Transferable tr = e.getTransferable();
            Object obj;

            if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)
                    || tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {

                // Catch drag of String (probably from JFileChooser)
                // Create a ShufflerItem and continue with code below this.
                if (tr.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                    obj = tr.getTransferData(DataFlavor.stringFlavor);

                    // The object, being a String, is the fullpath of the
                    // dragged item.
                    String fullpath = (String) obj;

                    // This assumes that the browser is the only place that
                    // a string is dragged from. If more places come into
                    // being, we will have to figure out what to do
                    ShufflerItem item = new ShufflerItem(fullpath, "BROWSER");
                    // Replace the obj with the ShufflerItem, and continue
                    // below.
                    obj = item;
                }
                // Not string, get the dragged object (could be from the
                // locator)
                else {
                    obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
                }

                if (obj instanceof ShufflerItem) {
                    ShufflerItem item = (ShufflerItem) obj;
                    String fullpath = item.getFullpath();
                    File file = new File(fullpath);
                    if (!file.exists()) {
                        Messages.postError("JGL: File not found " + fullpath);
                        return;
                    }
                    if (item.objType.equals("computed_dir"))
                        fullpath += "/pseudo3D.fdf";

                    if (!fullpath.endsWith(".fdf")) {
                        Messages.postError("JGL: File not fdf type"
                                + item.filename);
                        return;
                    }
                    getFDFFile(fullpath);
                }
            }
        } catch (IOException io) {
            io.printStackTrace();
        } catch (UnsupportedFlavorException ufe) {
            ufe.printStackTrace();
        }
        e.rejectDrop();
    } // drop

    // other public methods

    /**
     * @param l
     */
    public void addListener(JGLComListenerIF l) {
        listenerList.add(l);
    }

    /**
     * @param l
     */
    public void removeListener(JGLComListenerIF l) {
        listenerList.remove(l);
    }

    /**
     * @param comp
     */
    public void setDropTarget(Component comp) {
        new DropTarget(comp, this);
    }

    /**
     * @return
     */
    public int getShow() {
        return show;
    }

    /**
     * @return
     */
    public int getStatus() {
        return status;
    }

    /**
     * @param index
     * @return
     */
    public int getStatusValue(int index) {
        switch (index) {
        case STATINDEX:
            return getStatusValue(STATFIELD, STATBITS);
        case LOCKSINDEX:
            return getStatusValue(LOCKSFIELD, LOCKSBITS);
        case COMINDEX:
            return getStatusValue(COMFIELD, COMBITS);
        case AUTOINDEX:
            return getStatusValue(AUTOFIELD, AUTOBITS);
        case ANIMATIONINDEX:
            return getStatusValue(ANIMATIONFIELD, ANIMATIONBITS);
        case MMODEINDEX:
            return getStatusValue(MMODEFIELD, MMODEBITS);
        }
        return 0;
    }
    /**
     * @param newval
     */
    public void setStatusValue(int newval) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.setStatusValue: 0x"
                    + Integer.toHexString(newval).toUpperCase());
        setStatusField(newval, LOCKSINDEX, LOCKSFIELD, LOCKSBITS);
        setStatusField(newval, COMINDEX, COMFIELD, COMBITS);
        setStatusField(newval, AUTOINDEX, AUTOFIELD, AUTOBITS);
        setStatusField(newval, ANIMATIONINDEX, ANIMATIONFIELD, ANIMATIONBITS);
        setStatusField(newval, MMODEINDEX, MMODEFIELD, MMODEBITS);
        updateStatus();
    }
    
    /**
     * @param index
     * @param value
     */
    public void setStatusField(int index, int value, int mask) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.setStatusField(" + index + ","
                    + value + "," + mask + ")");
        int statval = 0;
        switch (index) {
        case STATINDEX:
            statval = setStatusBitField(STATFIELD, STATBITS, value, mask);
            break;
        case LOCKSINDEX:
            statval = setStatusBitField(LOCKSFIELD, LOCKSBITS, value, mask);
            break;
        case COMINDEX:
            statval = setStatusBitField(COMFIELD, COMBITS, value, mask);
            break;
        case AUTOINDEX:
            statval = setStatusBitField(AUTOFIELD, AUTOBITS, value, mask);
            break;
        case ANIMATIONINDEX:
            statval = setStatusBitField(ANIMATIONFIELD, ANIMATIONBITS, value, mask);
            break;
        case MMODEINDEX:
            statval = setStatusBitField(MMODEFIELD, MMODEBITS, value, mask);
            break;
        }
        if (connected) {
            String str = "g3ds[" + index + "]=" + statval;
            sendToVnmr(str);
            if(!statObj.callbackSet()){
                //Messages.postDebug("JGLComMgr.setStatusValue waiting for stat callback");
                updateStatus();
                repaint();
            }
            else if (DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall")||DebugOutput.isSetFor("glcom"))
                Messages.postDebug("JGLComMgr.setStatusValue waiting for status callback");
        }
    }

    /**
     * @param index
     * @return
     */
    public int getPrefsValue(int index) {
        switch (index) {
        case VJINDEX:
            return getPrefsValue(VJFIELD, VJBITS);
        case VTOOLSINDEX:
            return getPrefsValue(VTOOLSFIELD, VTOOLSBITS);
        case V3DTOOLSINDEX:
            return getPrefsValue(V3DTOOLSFIELD, V3DTOOLSBITS);
        }
        return 0;
    }
    /**
     * @param newval
     */
    public void setPrefsValue(int newval) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.setPrefsValue: 0x"
                    + Integer.toHexString(newval).toUpperCase());
        setPrefsField(newval, V3DTOOLSINDEX, V3DTOOLSFIELD, V3DTOOLSBITS);
        setPrefsField(newval, VJINDEX, VJFIELD, VJBITS);
        updatePrefs();
    }

    /**
     * @param index
     * @param value
     */
    public void setPrefsField(int index, int value, int mask) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.setPrefsField(" + index + ","
                    + value + "," + mask + ")");
        int setval = 0;
        switch (index) {
        case VJINDEX:
        	setval = setPrefsBitField(VJFIELD, VJBITS, value, mask);
            break;
        case VTOOLSINDEX:
        	setval = setPrefsBitField(VTOOLSFIELD, VTOOLSBITS, value, mask);
            break;
        case V3DTOOLSINDEX:
        	setval = setPrefsBitField(V3DTOOLSFIELD, V3DTOOLSBITS, value, mask);
            break;
        }
        if (connected) {
            String str = "g3dp[" + index + "]=" + setval;
            sendToVnmr(str);
            updatePrefs();
            repaint();
        }
    }

    /**
     * Do something special when certain parameter values change
     * @param obj
     */
    public void processCallback(PnewObj obj){
        if (DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.processCallback("+obj.semName()+") value:"+obj.pvalue);
            
        if(obj==statObj){
            obj.setCallback(false,true);           
            updateStatus();
            repaint();
        }        
        else if(obj==showObliqueObj){
            try {
                int value = Integer.valueOf(obj.pvalue);
                if(value>0)
                    setStatusField(LOCKSINDEX, 8, 8);  // SHOWORTHO
                else
                    setStatusField(LOCKSINDEX, 0, 8);
                
            } catch (NumberFormatException ex) {
            }
        }        
    }

    /**
     * send request for pnew update
     * @param wait
     */
    public synchronized void updateVpnt(boolean wait) {
        pntObj.requestUpdate(wait);
    }

    /**
     * send request for pnew update
     * @param wait
     */
    public synchronized void updateVrot(boolean wait) {
        rotObj.requestUpdate(wait);
    }

    /**
     * set g3dpnt in vnmrbg Note: using vnmrsem.acquire() to block causes vj to
     * lock up (not sure why)
     * 
     * @param p
     */
    public synchronized boolean sendVPntToVnmr(Point3D p, boolean wait) {
        if (wait) {
            int permits = vnmrsem.drainPermits();
            if (permits == 0) {
                if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                    Messages.postDebug("JGLComMgr.sendVPntToVnmr <waiting for graphics sem>");
                return false;
            }
        }
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendVPntToVnmr wait=" + wait);
        String str = "aipSetg3dpnt(" + p.x + "," + p.y + "," + p.z + "," + p.w
                + ") ";
        sendToVnmr(str);
        pntObj.requestUpdate(wait);
        vpnt = new Point3D(p);
        return true;
    }

    /**
     * set g3drot in vnmrbg - called as a result of a drag operation of a
     * rotation vector
     * 
     * @param p
     */
    public synchronized boolean sendVRotToVnmr(Point3D p, boolean wait) {
        if (wait) {
            int permits = vnmrsem.drainPermits();
            if (permits == 0) {
                if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                    Messages
                            .postDebug("JGLComMgr.sendVRotToVnmr <waiting for permit>");
                return false;
            }
        }
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendVRotToVnmr wait=" + wait);
        String str = "aipSetg3drot(" + p.x + "," + p.y + "," + p.z + ") ";
        sendToVnmr(str);
        rotObj.requestUpdate(wait);
        vrot = new Point3D(p);
        return true;
    }

    /**
     * set g3drot in vnmrbg - called as a result of a mouse-mode drag in the 3d
     * graphics window
     * 
     * @param p
     */
    public synchronized boolean sendVFltToVnmr(int index, double value,
            boolean wait) {
        if (wait) {
            int permits = vnmrsem.drainPermits();
            if (permits == 0) {
                if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                    Messages
                            .postDebug("JGLComMgr.sendVFltToVnmr <waiting for permit>");
                return false;
            }
        }
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendVFltToVnmr");
        String str = "aipSetg3df(" + index + "," + value + ") ";
        sendToVnmr(str);
        fltObj.requestUpdate(wait);
        return true;
    }

    /**
     * set vpnt in both vnmrj and vnmrbg - called as a result of a slider change
     * in a vnmrj panel - vc: vnmrjcmd('g3dpnt',index,value)
     * 
     * @param p
     */
    public void setVPntFromCmnd(int index, float value) {
        switch (index) {
        case XFRAME:
            vpnt.x = value;
            setSliceAxisFromFrame(index);
            if ((show & SHOWAXIS) == SHOWX && (status & SLICELOCK) > 0)
                vpnt.w = vpnt.x;
            break;
        case YFRAME:
            vpnt.y = value;
            setSliceAxisFromFrame(index);
            if ((show & SHOWAXIS) == SHOWY && (status & SLICELOCK) > 0)
                vpnt.w = vpnt.y;
            break;
        case ZFRAME:
            vpnt.z = value;
            setSliceAxisFromFrame(index);
            if ((show & SHOWAXIS) == SHOWZ && (status & SLICELOCK) > 0)
                vpnt.w = vpnt.z;
            break;
        case SFRAME:
            vpnt.w = value;
            if ((status & SLICELOCK) > 0)
                switch (show & SHOWAXIS) {
                case SHOWX:
                    vpnt.x = value;
                    break;
                case SHOWY:
                    vpnt.y = value;
                    break;
                case SHOWZ:
                    vpnt.z = value;
                    break;
                }
            break;
        }
        sendPointValue(G3DPNT, vpnt);
        sendVPntToVnmr(vpnt, true);
        if (connected)
            repaint();
    }

    /**
     * set vrot in both vnmrj and vnmrbg - called as a result of a slider change
     * in a panel - vc: vnmrjcmd('g3drot',x,y,z)
     * 
     * @param p
     */
    public void setVRotFromCmnd(int index, float value) {
        switch (index) {
        case 1:
            vrot.x = value;
            break;
        case 2:
            vrot.y = value;
            break;
        case 3:
            vrot.z = value;
            break;
        }
        sendPointValue(G3DROT, vrot);
        sendVRotToVnmr(vrot, true);
        if (connected)
            repaint();
    }

    /**
     * @param newval
     */
    public void setShowValue(int newval) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.setShowValue: 0x"
                    + Integer.toHexString(newval).toUpperCase());
        setShowField(newval, DTYPEINDEX, DTYPEFIELD, DTYPEBITS);
        setShowField(newval, PTYPEINDEX, PTYPEFIELD, PTYPEBITS);
        setShowField(newval, LTYPEINDEX, LTYPEFIELD, LTYPEBITS);
        setShowField(newval, CTYPEINDEX, CTYPEFIELD, CTYPEBITS);
        setShowField(newval, PALETTEINDEX, PALETTEFIELD, PALETTEBITS);
        setShowField(newval, AXISINDEX, AXISFIELD, AXISBITS);
        setShowField(newval, TEXOPTSINDEX, TEXOPTSFIELD, TEXOPTSBITS);
        setShowField(newval, SHADERINDEX, SHADERFIELD, SHADERBITS);
        setShowField(newval, CTMODEINDEX, CTMODEFIELD, CTMODEBITS);
        setShowField(newval, EFFECTSINDEX,EFFECTSFIELD, EFFECTSBITS);
        updateShow();
        if (connected)
            repaint();
    }

    /**
     * @param index
     * @param value
     */
    public void setShowField(int index, int value, int mask) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.setShowField(" + index + "," + value
                    + "," + mask + ")");
        int showval = 0;
        switch (index) {
        case DTYPEINDEX:
            showval = setShowBitField(DTYPEFIELD, DTYPEBITS, value, mask);
            break;
        case PTYPEINDEX:
            showval = setShowBitField(PTYPEFIELD, PTYPEBITS, value, mask);
            break;
        case LTYPEINDEX:
            showval = setShowBitField(LTYPEFIELD, LTYPEBITS, value, mask);
            break;
        case CTYPEINDEX:
            showval = setShowBitField(CTYPEFIELD, CTYPEBITS, value, mask);
            break;
        case PALETTEINDEX:
            showval = setShowBitField(PALETTEFIELD, PALETTEBITS, value, mask);
            break;
        case AXISINDEX:
            showval = setShowBitField(AXISFIELD, AXISBITS, value, mask);
            break;
        case TEXOPTSINDEX:
            showval = setShowBitField(TEXOPTSFIELD, TEXOPTSBITS, value, mask);
            break;
        case SHADERINDEX:
            showval = setShowBitField(SHADERFIELD, SHADERBITS, value, mask);
            break;
        case CTMODEINDEX:
            showval = setShowBitField(CTMODEFIELD, CTMODEBITS, value, mask);
            break;
        case EFFECTSINDEX:
            showval = setShowBitField(EFFECTSFIELD, EFFECTSBITS, value, mask);
            break;
        }
        if (connected) {
            String str = "g3di[" + index + "]=" + showval;
            //System.out.println(str);
            sendToVnmr(str);
            if(index<=6 && !batch){
            	String savestr=null;
            	switch(show & SHOWPTYPE){
            	case SHOW1D: savestr="1d"; break;
            	case SHOW1DSP: savestr="1dp"; break;
            	case SHOW2D: savestr="2d"; break;
            	case SHOW2DSP: savestr="2dp"; break;
            	case SHOW3D: savestr="3d"; break;
            	}
            	if(savestr!=null){
            		String sstr="glsave_"+savestr+"[" + index + "]=g3di[" + index + "]";
            		//System.out.println(sstr);
            		sendToVnmr(sstr);
            	}
            	//sendToVnmr("g3dsaveproj");
            }
            updateShow();
            repaint();
        }
    }

    /**
     * @param index
     * @param value
     */
    public void setShowField(int index, int value) {
        setShowField(index, value, 0);
    }

    /**
     * @param index
     * @return
     */
    public int getShowValue(int index) {
        switch (index) {
        case DTYPEINDEX:
            return getShowValue(DTYPEFIELD, DTYPEBITS);
        case PTYPEINDEX:
            return getShowValue(PTYPEFIELD, PTYPEBITS);
        case LTYPEINDEX:
            return getShowValue(LTYPEFIELD, LTYPEBITS);
        case CTYPEINDEX:
            return getShowValue(CTYPEFIELD, CTYPEBITS);
        case PALETTEINDEX:
            return getShowValue(PALETTEFIELD, PALETTEBITS);
        case AXISINDEX:
            return getShowValue(AXISFIELD, AXISBITS);
        case TEXOPTSINDEX:
            return getShowValue(TEXOPTSFIELD, TEXOPTSBITS);
        case SHADERINDEX:
            return getShowValue(SHADERFIELD, SHADERBITS);
        case CTMODEINDEX:
            return getShowValue(CTMODEFIELD, CTMODEBITS);
        case EFFECTSINDEX:
            return getShowValue(EFFECTSFIELD, EFFECTSBITS);
        }
        return 0;
    }

    /**
     * @param ins
     * @param code
     */
    public void graphicsCmd(DataInputStream ins, int code) {
        try {
            int value = ins.readInt();
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.graphicsCmd(" + code + ","
                        + value + ")");
            switch (code) {
            case SETSEM:
                if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                    Messages.postDebug("JGLComMgr.comCmd G3DSETSEM:" + value);
                vnmrsem.release();
                break;
            case MMAPTEST:
                mmapTest();
                break;
            case GETPOINT:
                getPointData(value, ins);
                break;
            case READFDF:
                sendNewData(XFER | XFERREAD);
                break;
            case BGNXFER:
                sendNewData(XFER | XFERBGN);
                break;
            case GETDATA:
            	sendNewData(XFER | XFERREAD);
            case GETFDF:
                getData(value, ins);
                break;
            case GETERROR:
                getError(value);
                sendNewData(XFER | XFERERROR);
                break;
            default:
                break;
            }
        } catch (Exception e) {
            System.out.println(e);
            Messages.writeStackTrace(e);
        }
    }

    /**
     * Decoder for all commands.
     * @param code      command identifier
     * @param value     command value
     */
    public void comCmd(int code, int value) {
        switch (code) {
        case G3DSHOWING:
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.comCmd G3DSHOWING:" + value);
            showing=(value==0)?false:true;
           // System.out.println("type:"+type+" autoxfer:"+((status & AUTOXFER)>0)+" initialized:"+initialized);
            if((status & AUTOXFER)>0 && type!=0 && showing && !initialized){
                if (DebugOutput.isSetFor("glxfer")||DebugOutput.isSetFor("glall"))
                    Messages.postDebug("JGLComMgr.comCmd request autoxfer dtype:" + type);
                sendDataRequest();
                //String str = "jFunc(" + G3D + "," + GETDATA + ")";
                //sendToVnmr(str);
            }
            sendStatusValue(8, value);
            initialized=true;
            break;
        case G3DBATCH:
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.comCmd G3DBATCH:" + value);
            if(value==0){
            	batch=false;
            	repaint();
            }
            else
            	batch=true;
            break;
        case G3DSETSEM:
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.comCmd G3DSETSEM:" + value);
            vnmrsem.release();
            break;
        case G3DRUN:
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall")){
            	Messages.postDebug("JGLComMgr.comCmd G3DRUN:" + value);
            }
            if (value > 0 ){
	           //statObj.setCallback(true,true);
	            statObj.setCallback(false,true);
 	            setStatusField(STATINDEX, STATRUNNING, STATRUNNING);
 	            statObj.requestUpdate(false);
            }
            else{
                statObj.setCallback(false,true);
                setStatusField(STATINDEX, 0, STATRUNNING);
            }
            return;
        case G3DREVERSE:
            if (value > 0)
                setStatusField(STATINDEX, STATREVERSE, STATREVERSE);
            else
                setStatusField(STATINDEX, 0, STATREVERSE);
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.comCmd G3DREVERSE:" + value);
            return;
        case G3DINIT:
            break;
        case G3DCONNECT:
            connected = (value == 0) ? false : true;
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.comCmd CONNECT:" + value);
            if (connected) {
                updateShow();
                updateStatus();
                updatePrefs();
                repaint();
            }
            break;
        case G3DSTEP:
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.comCmd G3DSTEP:" + value);
            break;
        case G3DSTATUS:
            break;
        case G3DPREFS:
            break;
        case G3DSHOW:
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.comCmd G3DSHOW:" + value);
            show = value;
            break;
        case G3DRESET:
            if (connected && listenerList.size() == 0) {
                if ((value & RESETVPNT) > 0) {
                    vpnt = new Point3D(0.5, 0.5, 0.5, 0.5);
                    sendVPntToVnmr(vpnt, false);
                }
                if ((value & RESETVROT) > 0) {
                    vrot = new Point3D(0.0, 0.0, 0.0);
                    sendVRotToVnmr(vrot, false);
                }
            }
            break;
        }
        sendComCmd(code, value);
    }

    /**
     * @param str
     *            com socket String passed from ExpPanel
     * @return true if str is a JGLComMgr command, false if not
     */
    public boolean comCmd(String str) {
        QuotedStringTokenizer tok = new QuotedStringTokenizer(str, " ,)(\n");
        String key = null;
        int index = 0;
        int ivalue = 0;
        float value;
        int mask = 0;
        String sarg="";

        if (tok.hasMoreTokens()){
        	 key = tok.nextToken();
             if(key.equals("vnmrjcmd") && tok.hasMoreTokens())
            	 key = tok.nextToken();
        }
        if (key==null)
        	return false;
        if (key.equals("g3d")) {
            if (!tok.hasMoreTokens())
                return false;
            key = tok.nextToken();
        }
        if (vjcmnds.containsKey(key)) {
            cmndData cmnd = (cmndData) vjcmnds.get(key);
            try {
                switch (cmnd.type) {
                case CMND:
                    if (tok.hasMoreTokens())
                        ivalue = Integer.parseInt(tok.nextToken());
                    comCmd(cmnd.id, ivalue);
                    return true;
                case STATUSFIELD:
                case SHOWFIELD:
                case PREFS:
                    if (!tok.hasMoreTokens())
                        throw new NumberFormatException();
                    index = Integer.parseInt(tok.nextToken());
                    if (tok.hasMoreTokens())
                        ivalue = Integer.parseInt(tok.nextToken());
                    if (tok.hasMoreTokens())
                        mask = Integer.parseInt(tok.nextToken());
                    switch(cmnd.type){
                    case SHOWFIELD:
                        setShowField(index, ivalue, mask);
                        break;
                    case STATUSFIELD:
                        setStatusField(index, ivalue, mask);
                        break;
                    case PREFS:
                        setPrefsField(index, ivalue, mask);
                        break;
                    }
                    return true;
                case POINT:
                    if (!tok.hasMoreTokens())
                        throw new NumberFormatException();
                    index = Integer.parseInt(tok.nextToken());
                    if (!tok.hasMoreTokens())
                        throw new NumberFormatException();
                    value = Float.parseFloat(tok.nextToken());
                    switch (cmnd.id) {
                    case G3DPNT:
                        setVPntFromCmnd(index, value);
                        break;
                    case G3DROT:
                        setVRotFromCmnd(index, value);
                        break;
                    }
                    break;
                case STRING:
                   // if (!tok.hasMoreTokens())
                       //throw new NumberFormatException();
                    switch (cmnd.id) {
                    default:
                        if (tok.hasMoreTokens())
                            sarg=tok.nextToken("'\n");
                        sendStringValue(key, cmnd.id, sarg);
                        break;
                    case G3DVERSION:
                        if (tok.hasMoreTokens())                            
                            version=tok.nextToken(" '\n");
                    	break;
                    case VOLMAPDIR:
                        if (tok.hasMoreTokens())                            
                            setVolMapFile(tok.nextToken(" '\n"));
                        else
                            setVolMapFile(null);
                        break;
                    }
                    break;
                case FLOAT:
                    if (!tok.hasMoreTokens())
                        throw new NumberFormatException();
                    index = Integer.parseInt(tok.nextToken());
                    if (!tok.hasMoreTokens())
                        throw new NumberFormatException();
                    value=Float.parseFloat(tok.nextToken());
                    sendFloatValue(key, cmnd.id, index, value);
                    if(index==G3DSWIDTH){
                        sendVFltToVnmr(G3DSWIDTH,value,true);
                    }
                    break;
                case INTEGER:
                    if (!tok.hasMoreTokens())
                        throw new NumberFormatException();
                    index = Integer.parseInt(tok.nextToken());
                    if (!tok.hasMoreTokens())
                        throw new NumberFormatException();
                    ivalue=(int)Double.parseDouble(tok.nextToken());
                    sendIntValue(key, cmnd.id, index, ivalue);
                    break;
                default:
                    return false;
                }
            } catch (NumberFormatException er) {
                System.err.println("G3D command syntax error:" + str);
                return false;
            }
        }
        return false;
    }

    public void setVolMapFile(String s) {
        if(s!=null)
            volmapfile = s+"/volmap" + viewport;
        else{
        	String sdir=System.getProperty("userdir");
            volmapfile=sdir+"/volmap" + viewport;
        }
        if (DebugOutput.isSetFor("gldata")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.setVolMapFile(" + volmapfile + ")");
    }

    // private methods

    private void sendToVnmr(String str) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendToVnmr: " + str);
        if (exppanel != null)
            exppanel.sendToVnmr(str);
    }

    private void sendNewData(int flags) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendNewData(" + flags + ")");
        setData(flags, this);
    }

    private void sendComCmd(int code, int value) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendComCmd(" + code + ",0x"
                    + Integer.toHexString(value).toUpperCase() + ")");
        setComCmd(code, value);
    }

    private void sendStringValue(String key, int id, String value) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendStringValue(" + id + "," + value
                    + ")");
        setStringValue(id, value);
        if (connected) {
            String str = key + "='" + value + "'";
            sendToVnmr(str);
            repaint();
        }
    }

    private void sendIntValue(String key, int id, int index, int value) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendIntValue(" + key + "," + index
                    + "," + value + ")");
        setIntValue(id, index, value);
        if (connected) {
            String str = key + "[" + index + "]=" + value;
            sendToVnmr(str);
            // repaint();
        }
    }

    private void sendFloatValue(String key, int id, int index, float value) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendFloatValue(" + key + "," + index
                    + "," + value + ")");
        setFloatValue(id, index, value);
        if (connected) {
            String str = key + "[" + index + "]=" + value;
            sendToVnmr(str);
            // repaint();
        }
    }

    /**
     * send new Point3D value to all JGLComListenerIF implementers
     */
    private void sendPointValue(int code, Point3D value) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendPointValue(" + code + "," + value
                    + ")");
        setPointValue(code, value);
    }

    private void getError(int errcode) {
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("GETERROR " + errcode);
    }

    private void getFDFFile(String path) {
        havedata = false;
        newdata = true;

        step = 0;

        String str = "imagedir='" + path + "'\n";
        sendToVnmr(str);
        str = "jFunc(" + G3D + "," + GETFDF + ")";
        sendToVnmr(str);
    }

    private void updateShow() {
        sendComCmd(G3DSHOW, show);
    }

    private void updateStatus() {
         sendComCmd(G3DSTATUS, status);
    }

    private void updatePrefs() {
    	sendComCmd(G3DPREFS, prefs);
    }

    private void repaint() {
        if(!batch){
        	if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
        		Messages.postDebug("JGLComMgr.repaint");
        	sendComCmd(G3DREPAINT, 0);
        }
    }

    private void sendStatusValue(int index, int value) {
        String str = "g3ds[" + index + "]=" + value;
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendStatusValue: " + str);
        sendToVnmr(str);
    }

    private void sendPrefsValue(int index, int value) {
        String str = "g3dp[" + index + "]=" + value;
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendPrefsValue: " + str);
        sendToVnmr(str);
    }

    private boolean setStatusField(int newval, int index, int field, int bits) {
        if (getField(newval, field, bits) != getField(status, field, bits)) {
            int value = getValue(newval, field, bits);
            setStatusBitField(field, bits, value, 0);
            sendStatusValue(index, value);
            return true;
        }
        return false;
    }

    private boolean setPrefsField(int newval, int index, int field, int bits) {
        if (getField(newval, field, bits) != getField(prefs, field, bits)) {
            int value = getValue(newval, field, bits);
            setPrefsBitField(field, bits, value, 0);
            sendPrefsValue(index, value);
            return true;
        }
        return false;
    }

    private void sendShowValue(int index, int value) {
        String str = "g3di[" + index + "]=" + value;
        if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
            Messages.postDebug("JGLComMgr.sendShowValue: " + str);
        sendToVnmr(str);
    }

    private boolean setShowField(int newval, int index, int field, int bits) {
        if (getField(newval, field, bits) != getField(show, field, bits)) {
            int value = getValue(newval, field, bits);
            setShowBitField(field, bits, value, 0);
            sendShowValue(index, value);
            return true;
        }
        return false;
    }

    private void setSliceAxisFromFrame(int frame) {
        int sliceplane = show & SHOWAXIS;
        if (frame != oldframe && (status & AUTOSELECT) > 0) {
            switch (frame) {
            case ZFRAME:
                if (sliceplane != SHOWZ)
                    setShowField(AXISINDEX, 0);
                break;
            case XFRAME:
                if (sliceplane != SHOWX)
                    setShowField(AXISINDEX, 2);
                break;
            case YFRAME:
                if (sliceplane != SHOWY)
                    setShowField(AXISINDEX, 1);
                break;
            }
        }
        oldframe = frame;
    }

    /**
     * executed when cursors are moved in 2D window
     * 
     * @param value
     * @param ins
     *            call hierarchy aipOrthoSlices.C extractCursorMoved
     *            graphics3D.C sendVpntToVnmrj
     */
    private void getPointData(int value, DataInputStream ins) {
        try {
            int frame = ins.readInt();
            vpnt.x = ins.readFloat();
            vpnt.y = ins.readFloat();
            vpnt.z = ins.readFloat();
            setSliceAxisFromFrame(frame);
            if ((status & SLICELOCK) > 0)
                switch (show & SHOWAXIS) {
                case SHOWX:
                    vpnt.w = vpnt.x;
                    break;
                case SHOWY:
                    vpnt.w = vpnt.y;
                    break;
                case SHOWZ:
                    vpnt.w = vpnt.z;
                    break;
                }
            if (DebugOutput.isSetFor("glcom")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.getPointData(" + vpnt + ")");
            sendPointValue(value, vpnt); // update JGLComListeners
            sendVPntToVnmr(vpnt, true);
            repaint();
        } catch (Exception e) {
            System.out.println(e);
            Messages.writeStackTrace(e);
        }
    }

    private synchronized void getData(int code, DataInputStream ins) {
        try {
            float y, m = 1, b = 0;
            float ave = 1.0f;
            float sd = 1.0f;
            slice = 0;
            np = ins.readInt();
            trace = ins.readInt();
            traces = ins.readInt();
            slices = ins.readInt();
            data_size = ins.readInt();
            rp = ins.readFloat(); // rp
            lp = ins.readFloat(); // lp
            mn = ins.readFloat(); // ymin
            mx = ins.readFloat(); // ymax
            ave = ins.readFloat(); // mean
            sd = ins.readFloat();
            sx = ins.readFloat();
            sy = ins.readFloat();
            sz = ins.readFloat();

            float max = sx > sy ? sx : sy;
            max = max > sz ? max : sz;
            sx /= max;
            sy /= max;
            sz /= max;
            type = code & DTYPE;
            dim = code & DIM;
            int old_type=data_type;
            data_type=type|dim;

            int i = 0, j = 0, k = 0, l = 0;
            image = false;
            boolean yrev = false;
            mmapped = (code & MMAPPED) > 0 ? true : false;
            float ym = 1.0f;

            havedata = false;
            newdata = true;

            step = 0;

            vertexData = null;

            Runtime r = Runtime.getRuntime();
            r.gc();

            if (DebugOutput.isSetFor("gldata")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("JGLComMgr.getData dim:" + dim + " type:"
                        + (type) + " MMAP:" + mmapped + " kbytes:"
                        + (data_size * 4 / 1000) + " np:" + np + " traces:"
                        + traces + " slices:" + slices + " min:"
                        + fltformat1.format(mn) + " max:"
                        + fltformat1.format(mx));

            step = 1;
            complex = ((type & COMPLEX) > 0) ? true : false;
            image = (type == IMAGE) ? true : false;
            yrev = (image && dim < 2) ? true : false;

            ymax = 1.0f;
            ymin = 0.0f;

            m = 1.0f / (mx - mn);
            b = 0;

            int ss = np * traces;
            int ws = complex ? 2 : 1;
            if (complex)
                np /= 2;
            int dl = np * ws;
            ym = (float) (ave + 6 * sd);
            ym = (ym > mx) ? mx : ym;
            if (image) {
                m = 1.0f / (ym - mn);
            }
            stdev = sd * m / 4;
            mean = ave * m;
            ymax = 1 / m;
            ymin = 0;

            if (data_size <= 0) {
                System.out.println("ERROR data size <=0");
                throw new Exception();
            }

            if (mmapped) {
                String vmfile = volmapfile;
                if (vmfile == null) {
                    System.out.println("ERROR could not open mmap file");
                    mmapped=false;
                    throw new Exception();
                }
                RandomAccessFile file = new RandomAccessFile(vmfile, "r");
                FileChannel rwCh = file.getChannel();
                // long fileSize = rwCh.size();
                rwCh.close();
             } else {
                vertexData = new float[data_size];
                for (l = 0; l < slices; l++) {
                    for (j = 0; j < traces; j++) {
                        if (yrev)
                            k = traces - 1 - j;
                        else
                            k = j;
                        if (complex) {
                            for (i = 0; i < np; i++) {
                                y = ins.readFloat();
                                vertexData[l * ss + k * dl + i] = y;
                                y = ins.readFloat();
                                vertexData[l * ss + k * dl + i + np] = y;
                            }
                        } else {
                            for (i = 0; i < np; i++) {
                                y = ins.readFloat();
                                vertexData[l * ss + k * dl + i] = y;
                            }
                        }
                    }
                }
            }

            if (DebugOutput.isSetFor("gldata")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("AUTOSCALE " + " mx:"
                        + fltformat1.format(mx) + " mn:"
                        + fltformat1.format(mn) + " ave:"
                        + fltformat1.format(ave) + " sd:"
                        + fltformat1.format(sd) + " ym:"
                        + fltformat1.format(ym) + " scale:"
                        + fltformat1.format(mx / ym));

            havedata = true;
            newdata = true;
            step = 1;
            sendNewData(XFER | XFEREND);
            if ((status & AUTORESET) > 0) {
                comCmd(G3DRESET, RESETALL);
                comCmd(G3DCONNECT, 1);
            } 
            else if(old_type !=data_type) {
                comCmd(G3DRESET, RESETOFFSET);
                comCmd(G3DCONNECT, 1);
            } 
            else{
                comCmd(G3DCONNECT, 1);
                if (connected && listenerList.size() == 0)
                    sendVPntToVnmr(vpnt,false);
            }

        } catch (Exception e) {
            System.out.println("JGLComMgr.getData Exception");
            sendNewData(XFER | XFERERROR);
        }
    }

    static void showBufferData(ByteBuffer buf, String name) {
        // Displays byte buffer contents

        // Save position
        int pos = buf.position();
        // Set position to zero
        buf.position(0);
        System.out.println("Data for " + name);
        while (buf.hasRemaining()) {
            System.out.print(buf.getFloat() + " ");
        }// end while loop
        System.out.println();// new line
        // Restore position and return
        buf.position(pos);
    }// end showBufferData

    private void mmapTest() {
        try {
            RandomAccessFile file = new RandomAccessFile(
                    "/export/home/tmp/test", "rw");
            FileChannel rwCh = file.getChannel();

            long fileSize = rwCh.size();
            ByteBuffer mapFile = rwCh.map(FileChannel.MapMode.READ_WRITE, 0,
                    fileSize);
            rwCh.close();

            mapFile.order(ByteOrder.nativeOrder());
            mapFile.position(0);
            mapFile.putFloat(10.0f);
            // Display contents of memory map
            showBufferData(mapFile, "mapFile");

        } catch (Exception e) {
            return;
        }
    }

    private int setBits(int w, int m, int b) {
        int r = w & (~m);
        return r | b;
    }

    private int setShowBitField(int field, int bits, int value, int msk) {
        int mask = (1 << bits) - 1;
        if (msk > 0)
            mask &= msk;
        int val = value & mask;
        mask <<= field;
        val <<= field;
        show &= (~mask);
        show |= val;
        return getShowValue(field, bits);
    }

    private int setStatusBitField(int field, int bits, int value, int msk) {
        int mask = (1 << bits) - 1;
        if (msk > 0)
            mask &= msk;
        int val = value & mask;
        mask <<= field;
        val <<= field;
        status &= (~mask);
        status |= val;
        return getStatusValue(field, bits);
    }

    private int setPrefsBitField(int field, int bits, int value, int msk) {
        int mask = (1 << bits) - 1;
        if (msk > 0)
            mask &= msk;
        int val = value & mask;
        mask <<= field;
        val <<= field;
        prefs &= (~mask);
        prefs |= val;
        return getPrefsValue(field, bits);
    }

    private int getField(int val, int field, int bits) {
        int mask = (1 << bits) - 1;
        mask <<= field;
        return val & mask;
    }

    private int getValue(int val, int field, int bits) {
        int mask = (1 << bits) - 1;
        val >>= field;
        return val & mask;
    }

    private int getStatusValue(int field, int bits) {
        int mask = (1 << bits) - 1;
        int val = status >> field;
        return val & mask;
    }
    private int getPrefsValue(int field, int bits) {
        int mask = (1 << bits) - 1;
        int val = prefs >> field;
        return val & mask;
    }

    private int getShowValue(int field, int bits) {
        int mask = (1 << bits) - 1;
        int val = show >> field;
        return val & mask;
    }

    // private class cmndData
    class cmndData {
        public int id;
        public int type;

        public cmndData(int t, int i) {
            id = i;
            type = t;
        }
    }

    class PnewObj extends VObjAdapter {
        public String pname = null;
        public String pvalue=null;
        int next = 0;
        int last = 0;
        boolean callback=false;
        boolean semname=false;
        boolean semobj=true;

        PnewObj(String s) {
            pname = s;
        }
        PnewObj(String s,boolean b) {
            pname = s;
            semobj=b;
        }
        public String semName(){
            String name=pname;
            if(semname)
                name+="_sem";
            return name;

        }
        public void setCallback(boolean b,boolean s){
            callback=b;
            semname=s;
        }
        public boolean callbackSet(){
            return callback;
        }
        public void reset() {
            last = next = 0;
        }
        public boolean waiting(){
            return (last==next)?false:true;
        }
        public void requestUpdate(boolean wait) {
            if (exppanel == null)
                return;
            if (last == next || !wait) {
                next++;
                if (DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall"))
                    Messages.postDebug("PnewObj.requestUpdate(" + pname + "," +wait+") SENDING PNEW:"
                            + next);                
                if(callback){
                    String str="vnmrjcmd('pnew','"+semName()+"')";
                    sendToVnmr(str);
                }
                else
                    exppanel.processComData("pnew 1 " + pname);
            } else if (DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall"))
                Messages.postDebug("PnewObj.requestUpdate(" + pname + ") WAITING FOR VALUE: "
                        + next);
        }

        public void checkUpdate(Vector v) {
            if (exppanel == null)
                return;
            String s = v.toString();
            String m=semName();
            if (s.indexOf(m) >= 0) {
                if(semobj){
                    if (last == next-1){
                        if(DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall"))
                            Messages.postDebug("PnewObj.asyncQueryParam(" + pname + ","
                                + next + ") REQUESTING VALUE FOR:"+next);
                        exppanel.asyncQueryParam(this, "$VALUE=" + next);
                    }
                }
                else{
                    if(DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall"))
                        Messages.postDebug("PnewObj.asyncQueryParam(" + pname + ","
                            + next + ") REQUESTING VALUE FOR:"+pname);
                    exppanel.asyncQueryParam(this, "$VALUE=" + pname);                                    
                }
            }
        }

        public void setValue(ParamIF pf) {
            try {
                pvalue=pf.value;
                if (DebugOutput.isSetFor("glpnew")||DebugOutput.isSetFor("glall"))
                    Messages.postDebug("PnewObj.setValue(" + pname + ") RECEIVED VALUE :"
                            + pvalue);
               if(semobj){
                    last = Integer.valueOf(pvalue);
                    next=last;
               }
               if(callback)
                   processCallback(this);
                
            } catch (NumberFormatException ex) {
            }
        }
    }
}

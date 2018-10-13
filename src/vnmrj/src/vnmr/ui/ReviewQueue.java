/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.ui;

import java.awt.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;
import java.beans.*;
import javax.swing.tree.*;
import vnmr.ui.shuf.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;
import com.sun.xml.tree.*;

/**
 * The review queue interface.
 */
public class ReviewQueue implements VObjDef, ExpListenerIF
{
    SessionShare        sshare;
    ButtonIF            expIF=null;
    RQBuilder     mgr;
    boolean             showStatus=true;
    MouseAdapter        ma=null;
    VElement            new_elem=null;
    static public boolean show_errors=false;

    int dragMode = 0;

    static public String new_protocol_file  = "INTERFACE/DefaultProtocol.xml";
    static public String REVIEWFILE  = "review.xml";
    
    static public String SET      = "set";
    static public String SELECTION= "selection";
    static public String TYPE     = "type";
    static public String NESTING  = "nesting";
    static public String GET      = "get";
    static public String READ     = "read";
    static public String ADD      = "add";
    static public String INSERT   = "insert";
    static public String WRITE    = "write";
    static public String MOVE     = "move";
    static public String COPY     = "copy";
    static public String DELETE   = "delete";
    static public String SETIDS   = "setids";
    static public String DISPLAY  = "display";
    static public String DISPLAYPREV  = "display_prev";
    static public String DISPLAYNEXT  = "display_next";
    static public String REQUESTDISPLAY  = "requestDisplay";
    static public String REQUEST  = "requestdisplay";
    static public String UNDISPLAY  = "undisplay";
    static public String LOADIMGS  = "loadimgs";
    static public String RELOADIMGS  = "reloadimgs";
    static public String REMOVENODE     = "removeNode";
    static public String READNODES     = "readNodes";
    static public String DRAGMODE     = "dragMode";
    static public String DISPLAYMODE   = "displaymode";
    static public String LAYOUT     = "layout";
    static public String SETSELECT     = "setRQselection";
    static public String TOGGLE     = "togglepanel";

    static public String NOTIFY     = "notify";
    static public String LOADFILE     = "loadFile";
    static public String LOADDIR     = "loadDir";
    static public String REMOVENODES     = "removeNodes";
    static public String COPYNODES     = "copyNodes";
    static public String MOVENODES     = "moveNodes";
    static public String SORT     = "sort";
    static public String RT     = "rt";

    static final  int    SINGLE   = 1;
    static final  int    MULT     = 2;

    String rqmacro="RQaction";

    String rqtype = null;

    /**
     * constructor
     */
    public ReviewQueue() 
    {
        mgr=new RQBuilder(this);

    } // ReviewQueue()
    
    public String getRqmacro() {
	return rqmacro;
    }

    public void setRqmacro(String str) {
	rqmacro = str;
    }

    public String getRQtype() {
	if(rqtype == null && Util.getRQPanel() != null) {
	   rqtype = Util.getRQPanel().getRQtype();
	}
	return rqtype;
    }

    public RQBuilder getMgr(){
        return mgr;
    }
    // ExpListenerIF interface
    /** called from ExpPanel (pnew)   */
    public void  updateValue(Vector v){
    }

    /** called from ExperimentIF for first time initialization  */
    public void  updateValue(){
    }

    public int getDragMode() {
	return dragMode;
    }

    //============ private Methods  ==================================
    private ButtonIF getExpIF(){
        return Util.getViewArea().getDefaultExp();
    }

    private void setDebug(String s){
        if(DebugOutput.isSetFor("RQ"))
            Messages.postDebug("ReviewQueue "+s);
    }

    private void postWarning(String s)
    {
        if(show_errors || DebugOutput.isSetFor("RQERRORS"))
            Messages.postWarning(s);
        else
            Messages.logWarning(s);
    }

    private void postError(String s)
    {
        if(show_errors || DebugOutput.isSetFor("RQERRORS"))
            Messages.postError(s);
        else
            Messages.logError(s);
    }

    boolean isFilenode(VElement src){
        return mgr.isFilenode(src);
    }

    //----------------------------------------------------------------
    /** clear all tree items. */
    //----------------------------------------------------------------
    public void clearTree(){
        mgr.clearTree();
    }
 
    //----------------------------------------------------------------
    /** Save the tree as an XML file. */
    //----------------------------------------------------------------
    public void saveReview(String path){
	if(path != null)
        mgr.save(path);
    }

    //============ Methods called from RQBuilder ===============

    //----------------------------------------------------------------
    /** Return name of review in review dir. */
    //----------------------------------------------------------------
    public String reviewName(String dir){
       return dir+"/"+REVIEWFILE;
    }

    //----------------------------------------------------------------
    /** Return true if review can be loaded. */
    //----------------------------------------------------------------
    public boolean testRead(String dir){
        if(!FileUtil.fileExists(reviewName(dir))){
            postWarning("review file not found "+dir);
            return false;
        }
        return true;
    }

    //----------------------------------------------------------------
    /** Click in treenode. */
    //----------------------------------------------------------------
    public void wasClicked(String id, int mode){
        String str=null;
        switch(mode){
        case RQBuilder.CLICK:
            str=rqmacro+"('click','"+id+"')";
            break;
        case RQBuilder.COPY:
            str=rqmacro+"('copy','"+id+"')";
            break;
        case RQBuilder.MOVE:
            str=rqmacro+"('move','"+id+"')";
            break;
        default:
            return;
        }
        setDebug(str);
        Util.sendToVnmr(str);
    }

    //----------------------------------------------------------------
    /** Double click in treenode. */
    //----------------------------------------------------------------
    public void wasDoubleClicked(String id){
        String str=rqmacro+"('doubleclick','"+id+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }

    //----------------------------------------------------------------
    /** Tree was modified. */
    //----------------------------------------------------------------
    public void setDragAndDrop(String id){
        String str=rqmacro+"('dnd','"+id+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }
            
    int getScope(String str){
        if(str.equals("nodes"))
            return MULT;
        else if(str.equals("filenodes"))
            return MULT;
        else if(str.equals("node"))
            return SINGLE;
        else if(str.equals("filenode"))
            return SINGLE;
        return 0;
    }

    int getType(String str){
        if(str.equals("new"))
            return RQBuilder.NEW;
        else if(str.equals("nodes"))
            return RQBuilder.ANY;
        else if(str.equals("filenodes"))
            return RQBuilder.FILENODES;
        else if(str.equals("node"))
            return RQBuilder.ANY;
        else if(str.equals("filenode"))
            return RQBuilder.FILENODES;
        return 0;
    }

    int getCondition(String str){
        if(str.equals("in"))
            return RQBuilder.EQ;
        else if(str.equals("<"))
            return RQBuilder.LT;
        else if(str.equals("<="))
            return RQBuilder.LE;
        else if(str.equals(">"))
            return RQBuilder.GT;
        else if(str.equals(">="))
            return RQBuilder.GE;
        else if(str.equals("first"))
            return RQBuilder.FIRST;
        else if(str.equals("last"))
            return RQBuilder.LAST;
        else if(str.equals("all"))
            return RQBuilder.ALL;
        else if(str.equals("after"))
            return RQBuilder.AFTER;
        else if(str.equals("into"))
            return RQBuilder.INTO;
        return 0;
    }

    //----------------------------------------------------------------
    /** Process a Vnmrbg command  */
    //----------------------------------------------------------------
    public synchronized void processCommand(String str) {
        QuotedStringTokenizer tok=new QuotedStringTokenizer(str,";");
        setDebug("RQ "+str);
        while(tok.hasMoreTokens()){
            String cmd=tok.nextToken();
            process(cmd);
        }
    }
    
    public boolean deleteFile(String fn) {
        if(fn==null) return false;
	File f = new File(fn);
        if(f==null) return false; 
	else return f.delete();
    }

    public void setRQtype(String type) {
	if(rqtype == null)
	rqtype = type;
    }

    //----------------------------------------------------------------
    /** Process a Vnmrbg command  */
    //----------------------------------------------------------------
    private void process(String str) {
        QuotedStringTokenizer tok=new QuotedStringTokenizer(str);
        String cmd=tok.nextToken().trim();
        String retstr="";
        if(cmd.equals(NOTIFY)){
           String vp;
           String fn;
           if (tok.hasMoreTokens())
             vp = tok.nextToken().trim();
           else
               return;
           if (tok.hasMoreTokens())
             fn=tok.nextToken().trim();
           else
               return;
           String action=tok.nextToken().trim();
/*
	   if(action.equalsIgnoreCase("recondisplay")) {
		String vcmd = rqmacro+"('reconDisplay')";
        	setDebug(vcmd);
        	Util.sendToVnmr(vcmd);
		deleteFile(fn);
		return;
	   }
*/
	   if(!vp.equals("3")) {
		deleteFile(fn);
		return;
	   }
/*
	   if(action.equals("unselecDisplay")) {
                mgr.uncheckdisplay("-1");
	   } else 
*/
	   if(action.equals("drop2frame") && tok.hasMoreTokens()) {
                String fpath =tok.nextToken().trim();
		if(tok.hasMoreTokens()) {
                   String frms =tok.nextToken().trim() + "-";
                   mgr.drop2frame(fpath, frms);
		}
	   } else 
/*
           if(cmd.equals(LOADFILE) ||
	   cmd.equals(LOADDIR) ||
	   cmd.equals(REMOVENODES) ||
	   cmd.equals(COPYNODES) ||
	   cmd.equals(MOVENODES) ||
	   cmd.equals(READNODES) ||
	   cmd.equals(SORT) ) 
*/
	   {
                String path=FileUtil.openPath(fn);
                if(path == null){
                    postError("cannot load review file "+fn);
                    return;
                }

                mgr.newTree(path);
		rqmacro = mgr.getRqmacro();
		//System.out.println("Rqmacro " +rqmacro);
	    }
	    deleteFile(fn);
        } else if(cmd.equals(TOGGLE)){
            if (tok.hasMoreTokens())
                Util.getRQPanel().togglePanel(tok.nextToken().trim());
        } else if(cmd.equals(DRAGMODE) && tok.hasMoreTokens()){
	    dragMode = Integer.valueOf(tok.nextToken().trim()).intValue();
        } else if(cmd.equals(READ)){
            String fn=tok.nextToken().trim();
                String path=FileUtil.openPath(fn);
                if(path == null){
                    postError("cannot load review file "+fn);
                    return;
                }
                mgr.newTree(path);
		rqmacro = mgr.getRqmacro();
		//System.out.println("Rqmacro " +rqmacro);
        }
        else if(cmd.equals(WRITE)){
            String fn=tok.nextToken().trim();
            String path=FileUtil.savePath(fn);
            if(path == null){
                postError("could not write review file "+fn);
                return;
            }
            mgr.save(path);
        } else if(cmd.equals(RT)){
            String path=tok.nextToken().trim();
            mgr.setRTGroup(path);
	} 
    }

    public void doRemoveNode(String key){
	String cmd = rqmacro+"('remove','"+ key +"')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

    public void doSetValue(String key, String name, String value){
	String cmd = rqmacro+"('setvalue','"+ key +"','"+name+"', '"+value+"')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

    public void doMoveNode(String key1, String key2){
	String cmd = rqmacro+"('moveNode','"+key1+"', '"+key2+"')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

    public void doCopyNode(String key1, String key2){
	String cmd = rqmacro+"('copyNode','"+key1+"', '"+key2+"')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

    public void doDoubleClicked(String key, String action){
	String cmd = rqmacro+"('"+action+"', '"+key+"')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

    public void doLoadData(String path, String key){
	String cmd = rqmacro+"('loadData','"+path+"','dndRQ','"+key+"')";
	Messages.postDebug("cmd " + cmd);
        //setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

    public void doRemoveImg(int x, int y, int but){
	String cmd = rqmacro+"('remove', "+x+", "+y+", "+but+")";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }
    
    public void doRQdnd(String key, int x, int y){
	String cmd = rqmacro+"('RQdnd', '"+key+"',"+x+", "+y+")";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }
    
/*
    public void doDnd(String path){
	String cmd = rqmacro+"('dnd', '"+path+"')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }
*/
    
    public void doModDnd(String path, int x, int y, String mod){
	String cmd = rqmacro+"('moddnd', '"+path+"',"+x+", "+y+",'"+mod+"')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }
    
    public void doDnd(String path, int x, int y){
	String cmd = rqmacro+"('dnd', '"+path+"',"+x+", "+y+")";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }
    
    public void doInit(){
	String cmd = rqmacro+"('init')";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

    public void doMoveFrame(Point src, Point dst, int but){
	String cmd = rqmacro+"('moveFrame', "+src.x+", "+src.y+", "+dst.x+", "+dst.y+")";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }
    
    public void doCopyFrame(Point src, Point dst, int but){
	String cmd = rqmacro+"('copyFrame', "+src.x+", "+src.y+", "+dst.x+", "+dst.y+")";
        setDebug(cmd);
        Util.sendToVnmr(cmd);
    }

} // class ReviewQueue

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
import java.awt.event.*;
import java.util.*;
import java.io.File;
import javax.swing.*;
import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;

/**
 * The study queue interface.
 */
public class StudyQueue implements VObjDef
{
    SessionShare        sshare;
    ButtonIF            expIF=null;
    ProtocolBuilder     mgr;
    boolean             showStatus=true;
    MouseAdapter        ma=null;
    VElement            new_elem=null;
    static public boolean show_errors=false;

    static public String new_action_file    = "INTERFACE/DefaultAction.xml";
    static public String new_protocol_file  = "INTERFACE/DefaultProtocol.xml";
    static public String STUDYFILE  = "study.xml";

    static public String START    = "start";
    static public String PAUSE    = "pause";
    static public String STOP     = "stop";
    static public String NORMAL_MODE = "NormalMode";
    static public String SUBMIT_MODE = "SubmitMode";
    static public String SET      = "set";
    static public String NESTING  = "nesting";
    static public String VALIDATE = "validate";
    static public String GET      = "get";
    static public String READ     = "read";
    static public String READDEL  = "readDelete";
    static public String ADD      = "add";
    static public String WRITE    = "write";
    static public String NWRITE   = "nwrite";
    static public String MOVE     = "move";
    static public String PMOVE    = "pmove";
    static public String LMOVE    = "lmove";
    static public String SHOW     = "show";
    static public String COPY     = "copy";
    static public String DELETE   = "delete";
    static public String SETIDS   = "setids";
    static public String ADD_QUEUE = "addQueue";
    static public String WATCH    = "watch";
    static final  int    SINGLE   = 1;
    static final  int    MULT     = 2;

    private String sqmacro;

    String sqId = "";

    /** Used in debugging to monitor efficiency. */
    private long m_timeUsed = 0;


    /**
     * constructor
     */
    public StudyQueue() {
        sqmacro = Util.getLabel("mSQaction", "sqaction");
        mgr = new ProtocolBuilder(this);
    } // StudyQueue()

    public void setMacro(String str) {
        sqmacro = str;
    }

    public String getMacro() {
        return sqmacro;
    }

    public void setSqId(String id) {
        sqId = id;
    }

    public String getSqId() {
        return sqId;
    }

    public ProtocolBuilder getMgr(){
        return mgr;
    }

    // ============ private Methods ==================================

    private void setDebug(String s){
        if(DebugOutput.isSetFor("SQ"))
            Messages.postDebug("StudyQueue: "+s);
    }

    private void postWarning(String s) {
        if (show_errors || DebugOutput.isSetFor("SQERRORS"))
            Messages.postWarning(s);
        else
            Messages.logWarning(s);
    }

    private void postError(String s) {
        if (show_errors || DebugOutput.isSetFor("SQERRORS"))
            Messages.postError(s);
        else
            Messages.logError(s);
    }

    boolean isLocked(VElement src){
        String locked=src.getAttribute(ProtocolBuilder.ATTR_LOCK);
        if(locked != null && locked.equals("on"))
            return true;
        if(mgr.isProtocol(src)){
            ArrayList list=mgr.getElements(src, ProtocolBuilder.ANY);
            if(list!=null){
                for(int i=0;i<list.size();i++){
                    VElement obj=(VElement)list.get(i);
                    if(obj !=src && isLocked(obj))
                        return true;
                }
            }
        }
        return false;
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
    public void saveStudy(String path){
        mgr.save(path);
    }

    //============ Methods called from ProtocolBuilder ===============

    //----------------------------------------------------------------
    /** Return name of study in study dir. */
    //----------------------------------------------------------------
    public String studyName(String dir){
       return dir+"/"+STUDYFILE;
    }

    //----------------------------------------------------------------
    /** Return true if study can be loaded. */
    //----------------------------------------------------------------
    public boolean testRead(String dir){
        if(mgr.isExecuting()){
            postWarning("cannot load study while queue is executing");
            return false;
        }
        if(!FileUtil.fileExists(studyName(dir))){
            postWarning("study file not found "+dir);
            return false;
        }
        return true;
    }

    //----------------------------------------------------------------
    /** Return true if element can be moved. */
    //----------------------------------------------------------------
    public boolean testMove(VElement src, VElement dst){
        String sid=src.getAttribute(ProtocolBuilder.ATTR_ID);
        String did=dst.getAttribute(ProtocolBuilder.ATTR_ID);
        if(did.length()==0)
            did="null";
        String msg="test SQ move "+sid+" -> "+did;
        if(isLocked(src)){
            setDebug(msg+" <fail - source is locked>");
            postWarning("cannot move a locked node");
            return false;
        }

        ArrayList list=mgr.getElementsAfter(dst, ProtocolBuilder.ANY);
        if(list==null){
            setDebug(msg+" <pass>");
            return true;
        }
        for(int i=0;i<list.size();i++){
            VElement obj=(VElement)list.get(i);
            if(isLocked(obj)){
                setDebug(msg+" <fail - destination locked>");
                postWarning("cannot move a node into a locked portion of tree");
                return false;
            }
        }
        setDebug(msg+" <pass>");
        return true;
    }

    //----------------------------------------------------------------
    /** Return true if element can be deleted. */
    //----------------------------------------------------------------
    public boolean testDelete(VElement src){
        //if(isLocked(src)){
        //    postWarning("cannot delete a locked tree node");
        //    return false;
        //}
        String id=src.getAttribute(ProtocolBuilder.ATTR_ID);
        if(id != null && id.length()>0)
            requestDelete(id);
        return false;  // ! let EM do the actual delete
    }

    //----------------------------------------------------------------
    /** Return true if element can be added here. */
    //----------------------------------------------------------------
    public boolean testAdd(VElement elem){
        ArrayList list=mgr.getElementsAfter(elem, ProtocolBuilder.ANY);
        if(list==null)
            return true;
        for(int i=0;i<list.size();i++){
            VElement obj=(VElement)list.get(i);
            if(isLocked(obj)){
                postWarning("cannot add a node to a locked portion of the tree");
                return false;
            }
        }
        return true;
    }

    //----------------------------------------------------------------
    /** Move requested. */
    //----------------------------------------------------------------
    public void requestMove(String src,String dst){
        String str=sqmacro+"('testmove','"+src+"','"+dst+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }

    //----------------------------------------------------------------
    /** Delete requested. */
    //----------------------------------------------------------------
    public void requestDelete(String id){
        String str=sqmacro+"('delete','"+id+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }

    //----------------------------------------------------------------
    /** Click in treenode. */
    //----------------------------------------------------------------
    public void wasClicked(String id, String dst, int mode){
        String str=null;
        switch(mode){
        case ProtocolBuilder.CLICK:
            str=sqmacro+"('click','"+id+"')";
            break;
        case ProtocolBuilder.COPY:
            str=sqmacro+"('copy','"+id;
            if(dst!=null){
                str+="','";
                str+=dst;
            }
            str+="')";
            break;
        case ProtocolBuilder.MOVE:
            str=sqmacro+"('move','"+id;
            if(dst!=null){
                str+="','";
                str+=dst;
            }
            str+="')";
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
        String str=sqmacro+"('doubleclick','"+id+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }

    //----------------------------------------------------------------
    /** Tree was modified. */
    //----------------------------------------------------------------
    public void setDragAndDrop(String id){
        String str=sqmacro+"('dnd','"+id+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }
    
    public void setDragAndDrop(String id, String mod){
	if(mod.length() < 1) mod = "dnd"; 
        String str=sqmacro+"('"+mod+"','"+id+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }
    
    public ArrayList<String> getActionNodeIds() {
        ArrayList<VElement> elementList;
        elementList = mgr.getElements(null, ProtocolBuilder.ACTIONS);
        ArrayList<String> ids = new ArrayList<String>();
        for (VElement elem : elementList) {
            ids.add(elem.getAttribute(ProtocolBuilder.ATTR_ID));
        }
        return ids;
    }

//    public String getAttribute(String id, String attr) {
//        return mgr.getAttribute(id, attr);
//    }

    public void setAttribute(String id, String attr, String value) {
        VElement velem =  mgr.getElement(id);
        if (velem != null) {
            mgr.setAttribute(velem, attr, value);
        }
    }

    //================ Methods called from Vnmrbg ====================
    VElement getElement(String id){
        VElement obj=mgr.getElement(id);
        if(obj==null)
            postError("SQ Id "+id+" not found");
        return obj;
    }

    int getScope(String str){
        if(str.equals("nodes"))
            return MULT;
        else if(str.equals("actions"))
            return MULT;
        else if(str.equals("protocols"))
            return MULT;
        else if(str.equals("node"))
            return SINGLE;
        else if(str.equals("action"))
            return SINGLE;
        else if(str.equals("protocol"))
            return SINGLE;
        return 0;
    }

    int getType(String str){
        if(str.equals("new"))
            return ProtocolBuilder.NEW;
        else if(str.equals("nodes"))
            return ProtocolBuilder.ANY;
        else if(str.equals("actions"))
            return ProtocolBuilder.ACTIONS;
        else if(str.equals("protocols"))
            return ProtocolBuilder.PROTOCOLS;
        else if(str.equals("node"))
            return ProtocolBuilder.ANY;
        else if(str.equals("action"))
            return ProtocolBuilder.ACTIONS;
        else if(str.equals("protocol"))
            return ProtocolBuilder.PROTOCOLS;
        return 0;
    }

    int getCondition(String str){
        if(str.equals("in"))
            return ProtocolBuilder.EQ;
        else if(str.equals("<"))
            return ProtocolBuilder.LT;
        else if(str.equals("<="))
            return ProtocolBuilder.LE;
        else if(str.equals(">"))
            return ProtocolBuilder.GT;
        else if(str.equals(">="))
            return ProtocolBuilder.GE;
        else if(str.equals("first"))
            return ProtocolBuilder.FIRST;
        else if(str.equals("last"))
            return ProtocolBuilder.LAST;
        else if(str.equals("all"))
            return ProtocolBuilder.ALL;
        else if(str.equals("after"))
            return ProtocolBuilder.AFTER;
        else if(str.equals("into"))
            return ProtocolBuilder.INTO;
        return 0;
    }

    //----------------------------------------------------------------
    /** Process a Vnmrbg command  */
    //----------------------------------------------------------------
    public void processCommand(String str) {
        QuotedStringTokenizer tok=new QuotedStringTokenizer(str,";");
        setDebug("SQ: " + str);
        while(tok.hasMoreTokens()){
            long now = System.currentTimeMillis();
            String cmd=tok.nextToken();
            process(cmd);
            long timeUsed = System.currentTimeMillis() - now;
            m_timeUsed += timeUsed;
            Messages.postDebug("SQTiming",
                               "... \"" + cmd + "\" time used="
                               + Fmt.f(3, timeUsed / 1000.0) + " s");
        }
        Messages.postDebug("SQTiming",
                           "Total time used="
                           + Fmt.f(3, m_timeUsed / 1000.0) + " s");
    }

    /************************************************** <pre>
     Process a Vnmrbg "SQ" command.

     SQ commands are sent from the VNMR background engine to the SQ
     through the vnmrj-Vnmrbg socket interface. Usually, they are generated by
     the EM in response to a change in the status of the experiment queue
     but (for testing purposes) may also be entered from the command line.

     General syntax:

     vnmrjcmd('SQ <command> <arg> .. <arg> [<macro>]')

     If <macro> is specified, the SQ sends a command to the Background engine
     to execute the macro after other actions specified by the command are
     carried out.

     refer to studyQueueAPI.spec for additional details

<pre>**************************************************/
    private void process(String str) {
        QuotedStringTokenizer tok = new QuotedStringTokenizer(str);
        String cmd = tok.nextToken().trim();
        String retstr = "";
        if (cmd.equals("no-op")) {
            // Does nothing
        }
        // syntax:    vnmrjcmd('SQ start [<macro>]')
        else if (cmd.equals(START)) {
            mgr.setExecuting(true);
            mgr.setPaused(false);
        }
        // syntax:    vnmrjcmd('SQ pause [<macro>]')
        else if (cmd.equals(PAUSE)) {
            mgr.setExecuting(false);
            mgr.setPaused(true);
        }
        // syntax:    vnmrjcmd('SQ stop [<macro>]')
        else if (cmd.equals(STOP)) {
            mgr.setExecuting(false);
            mgr.setPaused(false);
        }
        // syntax:    vnmrjcmd('SQ NormalMode [<macro>]')
        else if (cmd.equalsIgnoreCase(NORMAL_MODE)) {
            mgr.setMode(NORMAL_MODE);
        }
        // syntax:    vnmrjcmd('SQ SubmitMode [<macro>]')
        else if (cmd.equalsIgnoreCase(SUBMIT_MODE)) {
            mgr.setMode(SUBMIT_MODE);
        }
        // syntax:    vnmrjcmd('SQ read filename.xml [<macro>]')
        else if (cmd.equals(READ)) {
            String fn = tok.nextToken().trim();
            if (mgr.isExecuting()) {
                postWarning("cannot load a new study while queue is executing");
                return;
            } else {
                String path = FileUtil.openPath(fn);
                if (path == null) {
                    postError("cannot load study file " + fn);
                    return;
                }
                mgr.newTree(path);
		// set sqdirs[jviewport]=path
		sendSQpath(path);
            }
        }
        // syntax:    vnmrjcmd('SQ readDelete filename.xml [<macro>]')
        else if (cmd.equals(READDEL)) {
            String fn = tok.nextToken().trim();
            File fp = new File(fn);

            if (mgr.isExecuting()) {
                postWarning("cannot load a new study while queue is executing");
                fp.delete();
                return;
            } else {
                String path = FileUtil.openPath(fn);
                if (path == null) {
                    fp.delete();
                    postError("cannot load study file " + fn);
                    return;
                }
                mgr.newTree(path);
		// set sqdirs[jviewport]=path
		path = tok.nextToken().trim();
		sendSQpath(path);
                fp.delete();
            }
        }
        // syntax:    vnmrjcmd('SQ write filename.xml [<macro>]')
        else if (cmd.equals(WRITE)) {
            String fn = tok.nextToken().trim();
            String path = FileUtil.savePath(fn);
            if (path == null) {
                postError("could not write study file " + fn);
                return;
            }
            mgr.save(path);
	    // set sqdirs[jviewport]=path
	    sendSQpath(path);
        }
        // syntax:    vnmrjcmd('SQ nwrite filename.xml [<macro>]')
        else if (cmd.equals(NWRITE)) {
            String id;
            String path;
            if (tok.hasMoreTokens())
                id = tok.nextToken().trim();
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }

            if (tok.hasMoreTokens())
                path = FileUtil.savePath(tok.nextToken().trim());
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            VElement dst = mgr.getElement(id);
            if (dst == null) {
                if ( ! id.equals("tmpstudy"))
                   postError("invalid node id " + id);
                return;
            }
            mgr.writeElement(dst, path);
        }
        // syntax:    vnmrjcmd('SQ setids')
        else if (cmd.equals(SETIDS)) {
            mgr.setIds();
        }
        // syntax:    vnmrjcmd('SQ nesting = {true,false}")
        else if (cmd.equals(NESTING)) {
            String token;
            if (tok.hasMoreTokens())
                token = tok.nextToken().trim();
            else {
                postError("insufficient command arguments " + "SQ " + cmd);
                return;
            }
            if (!token.equals("=")) {
                postError("syntax error " + "SQ " + str);
                return;
            }
            if (tok.hasMoreTokens())
                token = tok.nextToken().trim();
            else {
                postError("insufficient command arguments " + "SQ " + cmd);
                return;
            }
            if (token.equals("no") || token.equals("false"))
                mgr.setAllowNesting(false);
            else if (token.equals("yes") || token.equals("true"))
                mgr.setAllowNesting(true);
        }
        // syntax:    vnmrjcmd('SQ validate {move,copy,all,none}")
        else if (cmd.equals(VALIDATE)) {
            String token;
            if (tok.hasMoreTokens())
                token = tok.nextToken().trim();
            else {
                postError("insufficient command arguments " + "SQ " + cmd);
                return;
            }
            mgr.setValidateMove(false);
            mgr.setValidateCopy(false);
            if (token.equals("all")) {
                mgr.setValidateMove(true);
                mgr.setValidateCopy(true);
                return;
            } else if (token.equals("move")) {
                mgr.setValidateMove(true);
                return;
            } else if (token.equals("copy")) {
                mgr.setValidateCopy(true);
                return;
            } else if (token.equals("none")) {
                mgr.setValidateMove(false);
                mgr.setValidateCopy(false);
                return;
            } else {
                postError("syntax error " + "SQ " + str);
                return;
            }
        }
        // syntax:    vnmrjcmd('SQ delete <id>")
        else if (cmd.equals(DELETE)) {
            String id;
            if (tok.hasMoreTokens())
                id = tok.nextToken();
            else {
                postError("insufficient command arguments " + "SQ " + cmd);
                return;
            }
            if (getCondition(id) == ProtocolBuilder.ALL) {
                mgr.clearTree();
		// set sqdirs[jviewport]=''
		sendSQpath("");
            } else {
                VElement obj = mgr.getElement(id);
                if (obj == null) {
                    if ( ! id.equals("tmpstudy")) {
                       postError("node " + id + " not found " + "SQ " + cmd);
                    }
                    return;
                }
                mgr.deleteElement(obj);
            }
        }
        else if (cmd.equals(ADD_QUEUE)) {
            String queueDir = tok.nextToken();
            mgr.addQueue(queueDir);
        }
        // syntax: vnmrjcmd('SQ add <file> [<cond>] [<dst>] [<macro>]')
        //         vnmrjcmd('SQ add new <type> [<cond>] [<dst>] [<macro>]')
        else if (cmd.equals(ADD)) {
            String id = tok.nextToken().trim();
            String fn = null;
            String type = "protocol";
            if (id.equals("new")) {
                type = tok.nextToken();
                Messages.postDebug("SQ", "--- SQ ADD: type=" + type);
                if (type.equals("action"))
                    fn = new_action_file;
                else
                    fn = new_protocol_file;
            } else
                fn = id;
            String path = FileUtil.openPath(fn);
            if (path == null) {
                postError("cannot read protocol file " + fn);
                return;
            }
            id = null;
            if (tok.hasMoreTokens()) {
                id = tok.nextToken();
                int cond = getCondition(id);
                if (cond != 0) {
                    mgr.setInsertMode(cond);
                    id = tok.nextToken().trim();
                }
            }
            VElement dst;
            if (id == null)
                dst = mgr.lastElement();
            else
                dst = mgr.getElement(id);
            if (dst == null) {
                if ( ! id.equals("tmpstudy"))
                   postError("invalid node id " + id);
                mgr.setInsertMode(0);
                return;
            }
            mgr.insertProtocol(dst, path);
            mgr.setInsertMode(0);
            new_elem = mgr.getSelected();
        }
        // syntax: vnmrjcmd('SQ move <src> [<cond>] <dst> [true,false] [<macro>]')
        else if (cmd.equals(MOVE)) {
            String id;
            boolean ignorelock = false;
            if (tok.hasMoreTokens())
                id = tok.nextToken().trim();
            else {
//                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            VElement src = mgr.getElement(id);
            if (tok.hasMoreTokens())
                id = tok.nextToken().trim();
            else {
//                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            int cond = getCondition(id);
            if (cond != 0) {
                mgr.setInsertMode(cond);
                if (tok.hasMoreTokens())
                    id = tok.nextToken().trim();
            }
            VElement dst = mgr.getElement(id);
            if (dst == null) {
                if ( ! id.equals("tmpstudy"))
                   postError("invalid node id " + id);
                mgr.setInsertMode(0);
                return;
            }
            if (tok.hasMoreTokens()) {
                String s = tok.nextToken().trim();
                if (s.equals("false"))
                    ignorelock = true;
                else if (s.equals("true"))
                    ignorelock = false;
                else
                    retstr = s;
            }
            mgr.moveElement(src, dst, ignorelock);
            mgr.setInsertMode(0);
        }
        // syntax: vnmrjcmd('SQ {lmove,pmove} <src> [<cond>] <dst> [<macro>]')
        else if (cmd.equals(PMOVE) || cmd.equals(LMOVE)) {
            String id;
            if (tok.hasMoreTokens())
                id = tok.nextToken().trim();
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            VElement src = mgr.getElement(id);
            if (tok.hasMoreTokens())
                id = tok.nextToken().trim();
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            int cond = getCondition(id);
            if (cond != 0) {
                mgr.setInsertMode(cond);
                id = tok.nextToken().trim();
            }
            VElement dst = mgr.getElement(id);
            if (dst == null) {
                if ( ! id.equals("tmpstudy"))
                   postError("invalid node id " + id);
                mgr.setInsertMode(0);
                return;
            }
            boolean bpmove = false;
            if (cmd.equals(PMOVE))
                bpmove = true;
            boolean ballownesting = mgr.allowNesting();
            if (bpmove && !ballownesting)
                mgr.setAllowNesting(true);
            ArrayList alist = (ArrayList) mgr.getHiddenNodes().clone();
            if (!bpmove)
                mgr.showElementAll("true");
            mgr.moveElement(src, dst, true);
            mgr.setHiddenNodes(alist);
            if (bpmove)
                mgr.setAllowNesting(ballownesting);
            else
                mgr.hideElements();
            mgr.setInsertMode(0);
        }
        // syntax: vnmrjcmd('SQ show <attr> <id> [<macro>]')
        else if (cmd.equals(SHOW)) {
            String value;
            if (tok.hasMoreTokens())
                value = tok.nextToken().trim();
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            ArrayList aListElem = new ArrayList();
            while (tok.hasMoreTokens()) {
                String id = tok.nextToken().trim();
                VElement src = mgr.getElement(id);
                if (src == null)
                {
                   if ( ! id.equals("tmpstudy"))
                      postError("invalid node id " + id);
                }
                else
                {
                    aListElem.add(src);
                }
            }
            mgr.showElement(aListElem, value);
        }
        // syntax: vnmrjcmd('SQ copy <id> <cond> <dst> [<macro>]')
        else if (cmd.equals(COPY)) {
            String id_src;
            String id_dst;
            if (tok.hasMoreTokens())
                id_src = tok.nextToken().trim();
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            if (tok.hasMoreTokens())
                id_dst = tok.nextToken().trim();
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            int cond = getCondition(id_dst);
            if (cond != 0) {
                mgr.setInsertMode(cond);
                id_dst = tok.nextToken().trim();
            }
            VElement dst = mgr.getElement(id_dst);
            if (dst == null) {
                if ( ! id_dst.equals("tmpstudy"))
                   postError("invalid node id " + id_dst);
                return;
            }
            VElement src = mgr.getElement(id_src);
            if (src == null) {
                src = mgr.readElement(id_src, dst);
                if (src == null) {
                    if ( ! id_src.equals("tmpstudy"))
                       postError("invalid node id " + id_src);
                    return;
                }
            } else
                mgr.copyElement(src, dst);
        }
        // syntax: vnmrjcmd('SQ {get,set} <type> [<cond> <id>] <attr> <val> [<macro>]')
        else if (cmd.equals(SET) || cmd.equals(GET)) {
            String arg;
            // first token
            if (tok.hasMoreTokens())
                arg = tok.nextToken().trim();
            else {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }
            // second token
            if (!tok.hasMoreTokens()) {
                postError("insufficient command arguments SQ " + cmd);
                return;
            }

            int type = 0;
            int scope = 0;
            int cond = getCondition(arg);
            String apar = null;
            String vpar = null;
            String id = null;
            VElement obj = null;

            switch (cond) {
            case ProtocolBuilder.FIRST:
            case ProtocolBuilder.ALL:
                arg = tok.nextToken();
                type = getType(arg);
                scope = getScope(arg);
                apar = tok.nextToken();
                vpar = tok.nextToken();
                break;
            default:
                type = getType(arg);
                if (type == 0 || type == ProtocolBuilder.NEW) {
                    if (type == ProtocolBuilder.NEW) {
                        if (new_elem == null) {
                            postError("must call SQ add before using new");
                            return;
                        }
                        obj = new_elem;
                    } else {
                        // e.g. set p2.a2 Lock on
                        obj = mgr.getElement(arg);
                        if (obj == null) {
                            // NB: This can happen when chempack adds protocols
                            // Ugly but harmless error
                            Messages.postDebug("SQ",
                                               "unknown identifier SQ " + str);
                            return;
                        }
                    }
                    cond = ProtocolBuilder.ONE;
                    type = ProtocolBuilder.ANY;
                    scope = SINGLE;
                } else {
                    // e.g. set actions > p2.a2 Lock on
                    scope = getScope(arg);
                    arg = tok.nextToken().trim();
                    cond = getCondition(arg);
                    if (scope == SINGLE) {
                        // e.g. set actions after p2.a2 Lock on
                        if (cond == ProtocolBuilder.GT)
                            cond = ProtocolBuilder.AFTER;
                        else if (cond == ProtocolBuilder.LT)
                            cond = ProtocolBuilder.BEFORE;
                    }
                    if (cond == 0) {
                        id = arg;
                        cond = ProtocolBuilder.ONE;
                    } else
                        id = tok.nextToken();
                    obj = mgr.getElement(id);
                }
                apar = tok.nextToken();
                vpar = tok.nextToken();
                if (vpar.startsWith("\"")) {
                    try {
                        vpar = vpar.substring(1) + tok.nextToken("\"");
                    } catch (NoSuchElementException e) {
                        vpar = vpar.substring(1, vpar.length() - 1);
                    }
                }
                break;
            } // switch

            if (apar == null || vpar == null) {
                postError("syntax error " + "SQ " + str);
                return;
            }
            ArrayList list;
            // looking for nodes in id will return a null list if id does not exist
            if ((obj == null) && (cond == ProtocolBuilder.EQ) )
               list = new ArrayList();
            else
               list = mgr.getElements(obj, cond, type);
            if (cmd.equals(SET)) {
                for (int i = 0; i < list.size(); i++) {
                    obj = (VElement) list.get(i);
                    mgr.setAttribute(obj, apar, vpar);
                }
                //mgr.invalidateTree();
            } else { // get
                if (scope == SINGLE) {
                    if (list.size() > 1) {
                        postError("syntax error " + "SQ " + str);
                        return;
                    }
                    String alist = apar + "=`";
                    String vlist = vpar + "=`";
                    if (list.size() == 1) {
                        obj = (VElement) list.get(0);
                        list = mgr.getAttributes(obj);
                        for (int i = 0; i < list.size(); i += 2) {
                            String name = (String) list.get(i);
                            String value = (String) list.get(i + 1);
                            alist += name;
                            vlist += value;
                            if (i < list.size() - 2) {
                                alist += "`,`";
                                vlist += "`,`";
                            }
                        }
                    }
                    alist += "`";
                    vlist += "`";
                    retstr = alist + " " + vlist;
                } else {
                    retstr = vpar + "=`";
                    for (int i = 0; i < list.size(); i++) {
                        obj = (VElement) list.get(i);
                        String value = obj.getAttribute(apar);
                        retstr += value;
                        if (i < list.size() - 1)
                            retstr += "`,`";
                    }
                    retstr += "`";
                }
            }
        } else if (cmd.equals(WATCH)) {
            // E.g.: vnmrjcmd('SQ watch auto ', cursqexp, autodir, svfdir)
            ArrayList<String> args = new ArrayList<String>();
            while (tok.hasMoreTokens()) {
                args.add(tok.nextToken());
            }
             if (!SQUpdater.processCommand(this, args.toArray(new String[0]))) {
                 Messages.postDebug("Bad format for 'watch' cmd: " + str);
             }
//            if (tok.countTokens() != 3) {
//                Messages.postDebug("Bad format for 'watch' cmd: " + str);
//            } else {
//                String studydir = tok.nextToken();
//                String autodir = tok.nextToken();
//                String datadir = tok.nextToken();
//                SQUpdater.startUpdates(this, studydir, autodir, datadir);
//            }
        } else {
            postError("command not recognized " + "SQ " + cmd);
            return;
        }
        while (tok.hasMoreTokens())
            retstr += " " + tok.nextToken().trim();
        if (retstr.length() > 0) {
            setDebug(retstr);
            Util.sendToVnmr(retstr);
        }
    }

    public void sendSQpath(String path){
        String str=sqmacro+"('sqpath','"+path+"')";
        setDebug(str);
        Util.sendToVnmr(str);
    }
    

} // class StudyQueue

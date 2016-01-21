/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**
 ** A TableModel built from file(s) of given type:
 *
 * FileTableModel(String path, String type);
 * FileTableModel(Vector paths, String type);
 *
 * For a given type, the constructor recursively go through
 * the path(s) for specific type of files or directories, and
 * build a tableModel with the names of files/directories, or
 * the content of files.
 *
 * each row has a rowValue (element of m_rowValues) depending on
 * input type as well as row index. If rowValue is a path, this path
 * may be used to build a new tableModel. The possible type(s) of
 * this path is given in m_outputTypes.
 *
 ** types (input as well as output) can be one of the following:
 *
 * "records" - list of records (directories)
 *             {"Time saved", " Data ID", "User", "Console", "Records"}
 *
 * "s_auditTrailFiles" - list of session audit trail files
 *                       {"Time begin", "Time terminated", "User", "Console"}
 *
 * "cmdHistory" - content of cmdHistory file(s)
 *                {"Time", "Command", "Description"}
 *
 * "s_auditTrail" - content of session audit trail
 *                  {"Time", "Command", "Input", "Output"}
 *
 * "d_auditTrail" - content of record audit trail
 *                  {"Time", "Console", "User", "Command", "Input", "Output"}
 *
 * "recordChecksumInfo" - content of checksum.flag
 *                  {"Time", "Info"}
 *
 * "a_auditTrailFiles" - list of auditing audit trails
 *                  {"Time begin", "Time terminated"}
 *
 * "a_auditTrails" - content of auditing audit trail
 *                  {"Time", "User", "Action", "Info"}
 *
 * "s_userProfiles" - content of auditing audit trail
 *                  {"User", "ID", "Group", "Level"}
 *
 * "u_auditTrailFiles" - list of user account audit trail files
 *                       {"Time began", "Time terminated"}
 *
 * "u_auditTrails" - content of user account audit trail files
 *                       {"Time", "Admin", "User", "Action"}
 *
 * "x_auditTrailFiles" - list of unix audit trail files
 *                       {"Time began", "Time terminated", "Console"}
 *
 * "x_auditTrails_recDele" - content of unix audit trail files
 *                       {"Time", "Action", "User", "Path", "Owner", "Info"}
 *
 * "x_auditTrails_failure" - content of unix audit trail files
 *                       {"Time", "Action", "User", "Path", "Owner", "Info"}
 *
 * "x_auditTrails_openWrite" - content of unix audit trail files
 *                       {"Time", "Action", "User", "Path", "Owner", "Info"}
 *
 * "x_auditTrails_unlink" - content of unix audit trail files
 *                       {"Time", "Action", "User", "Path", "Owner", "Info"}
 *
 * "x_auditTrails_rename" - content of unix audit trail files
 *                       {"Time", "Action", "User", "Path", "Owner", "Info"}
 *
 * "x_auditTrails_creat" - content of unix audit trail files
 *                       {"Time", "Action", "User", "Path", "Owner", "Info"}
 *
 * "recordTrashInfo" - content of .trash 
 *             {"Time saved", " Data ID", "User", "Console", "Records"}
 *
 * "recordTrashInfo2" - content of .trash 
 *             {"Time deleted", "Fullpath", "Operator", "Owner", "Sample name", "Seqfil"}
 *
 ** type of row values:
 *
 * "records" - full path
 * "s_auditTrailFiles" - full path
 * "cmdHistory" - command(s)
 * "s_auditTrail" - output path
 * "d_auditTrail" - output path
 * "recordChecksumInfo" - info
 * "a_auditTrails" - action
 * "a_auditTrailFiles" - full path 
 * "s_userProfiles" - login name
 * "u_auditTrailFiles" - full path
 * "u_auditTrails" - info
 * "x_auditTrailFiles" - full path
 * "x_auditTrails_xxx" - action+path
 * "recordTrashInfo" - info2 
 * "recordTrashInfo2" - full path 
 *
 ** output types:
 *
 * "records" - "cmdHistory", "d_auditTrail", ...
 * "s_auditTrailFiles" - "cmdHistory", "s_auditTrail"
 * "cmdHistory" - null
 * "s_auditTrail" - "cmdHistory", "s_auditTrail", ...
 * "d_auditTrail" - "cmdHistory", "d_auditTrail", ...
 * "recordChecksumInfo" - null
 * "a_auditTrails" - null
 * "a_auditTrailFiles" - "a_auditTrails" 
 * "s_userProfiles" - null
 * "u_auditTrailFiles" - "u_auditTrails"
 * "u_auditTrails" - null
 * "x_auditTrailFiles" - "x_auditTrails"
 * "x_auditTrails_xxx" - null
 * "recordTrashInfo" - null
 * "recordTrashInfo2" - null
 *
 * @author He Liu
 */

package  vnmr.part11;

import java.util.*;
import java.io.*;
import javax.swing.table.*;
import vnmr.util.*;
import vnmr.ui.*;

public class FileTableModel extends DefaultTableModel
{

    boolean debug = false;
    // input type.
    private String m_type = null;
    // input path(s).
    private Vector m_paths = new Vector();
    // row values of the given type.
    private Vector m_rowValues = new Vector();
    // output types of the given type.
    private Vector m_outputTypes = new Vector();
    // table header of the given type.
    private Vector m_header = new Vector();
    // 2D vectors of table content.
    private Vector m_values2D = new Vector();

    private Vector m_flags = new Vector();
    private int m_goodRows = 0;
    private int m_badRows = 0;
    Hashtable m_months = null;

    private void makeMonths() {
       m_months = new Hashtable();
       m_months.put("Jan", "1");
       m_months.put("Feb", "2");
       m_months.put("Mar", "3");
       m_months.put("Apr", "4");
       m_months.put("May", "5");
       m_months.put("Jun", "6");
       m_months.put("Jul", "7");
       m_months.put("Aug", "8");
       m_months.put("Sep", "9");
       m_months.put("Oct", "10");
       m_months.put("Nov", "11");
       m_months.put("Dec", "12");
    }

    public FileTableModel() {
	makeMonths();
    }

    public FileTableModel(String path, String type) {

    m_type = type;
    m_paths.clear();
    m_paths.addElement(path);
    makeMonths();

    buildFileTableModel();
    setOutputTypes();
    }

    public FileTableModel(Vector paths, String type) {

    m_type = type;
    m_paths.clear();
    for(int i=0; i<paths.size(); i++)
    m_paths.addElement(paths.elementAt(i));
    makeMonths();

    buildFileTableModel();
    setOutputTypes();
    }

    public void updateModel(Vector paths, String type)
    {
        m_type = type;
        m_paths.clear();
        for(int i=0; i<paths.size(); i++)
        {
            m_paths.addElement(paths.elementAt(i));
        }
        buildFileTableModel();
        setOutputTypes();
    }

    public TableModel getTableModel() {
    return(this);
    }

    public int getGoodRows() {
    return(m_goodRows);
    }

    public int getBadRows() {
    return(m_badRows);
    }

    public String getType() {
    return(m_type);
    }

    public Class getColumnClass(int column)
    {
        String strColumn = getColumnName(column);
        if (strColumn != null && strColumn.indexOf("Time") >= 0 &&
	strColumn.indexOf("terminated") == -1)
            return Date.class;

        return super.getColumnClass(column);
    }

    public String getRowValue(int row) {
    if(row >= 0 && row < this.m_rowValues.size())
    return((String)this.m_rowValues.elementAt(row));
    else return(null);
    }

    public Vector getFlags() {
    return(m_flags);
    }

    public Vector getOutputTypes() {
    return(m_outputTypes);
    }

    private void setOutputTypes() {

        if (m_type != null)
            m_type = m_type.trim();
        if(m_type == null || m_type.length() == 0)
            return;

    if (m_outputTypes == null)
    {
        m_outputTypes = new Vector();
    }
    m_outputTypes.clear();

    if(m_type.equals("cmdHistory")) {
        m_outputTypes = null;

    } else if(m_type.equals("records") || m_type.endsWith("auditTrail")) {
        m_outputTypes.addElement("d_auditTrail");
        m_outputTypes.addElement("cmdHistory");
        //m_outputTypes.addElement("log");

    } else if(m_type.equals("s_auditTrailFiles")) {
        m_outputTypes.addElement("s_auditTrail");
        m_outputTypes.addElement("cmdHistory");

    } else if(m_type.equals("recordChecksumInfo")) {
            m_outputTypes = null;
    } else if(m_type.equals("a_auditTrailFiles")) {
            m_outputTypes.addElement("a_auditTrails");
    } else if(m_type.equals("a_auditTrails")) {
            m_outputTypes = null;
    } else if(m_type.equals("s_userProfiles")) {
            m_outputTypes = null;
    } else if(m_type.equals("u_auditTrailFiles")) {
        m_outputTypes.addElement("u_auditTrails");
    } else if(m_type.equals("u_auditTrails")) {
        m_outputTypes = null;
    } else if(m_type.equals("x_auditTrailFiles")) {
        m_outputTypes.addElement("x_auditTrails_recDele");
        m_outputTypes.addElement("x_auditTrails_failure");
        m_outputTypes.addElement("x_auditTrails_openWrite");
        m_outputTypes.addElement("x_auditTrails_unlink");
        m_outputTypes.addElement("x_auditTrails_creat");
        m_outputTypes.addElement("x_auditTrails_rename");
    } else if(m_type.startsWith("x_auditTrails_")) {
        m_outputTypes = null;
    } else if(m_type.equals("recordTrashInfo")) {
            m_outputTypes = null;
    } else if(m_type.equals("recordTrashInfo2")) {
            m_outputTypes = null;
        } else {
        m_outputTypes = null;
    }
    }

    private void makeHeader() {

    m_header.clear();

    if(m_type.equals("records")) {

        m_header.addElement("Time saved");
        m_header.addElement("Data ID");
        m_header.addElement("User");
        m_header.addElement("Console");
        m_header.addElement("Records");

    } else if(m_type.equals("s_auditTrailFiles")) {

        m_header.addElement("Time begin");
        m_header.addElement("Time terminated");
        m_header.addElement("User");
        m_header.addElement("Console");

    } else if(m_type.equals("cmdHistory")) {

        m_header.addElement("Time");
        m_header.addElement("Command");
        m_header.addElement("Description");

    } else if(m_type.equals("s_auditTrail")) {

        m_header.addElement("Time");
        m_header.addElement("Command");
        m_header.addElement("Input");
        m_header.addElement("Output");

    } else if(m_type.equals("d_auditTrail")) {

        m_header.addElement("Time");
        m_header.addElement("Console");
        m_header.addElement("User");
        //m_header.addElement("Function");
        m_header.addElement("Command");
        m_header.addElement("Input");
        m_header.addElement("Output");

    } else if(m_type.equals("recordChecksumInfo")) {

        m_header.addElement("Time");
        m_header.addElement("Info");

    } else if(m_type.equals("a_auditTrailFiles")) {

        m_header.addElement("Time begin");
        m_header.addElement("Time terminated");

    } else if(m_type.equals("a_auditTrails")) {

        m_header.addElement("Time");
        m_header.addElement("User");
        m_header.addElement("Action");
        m_header.addElement("Info");

    } else if(m_type.equals("s_userProfiles")) {

        m_header.addElement("User");
        m_header.addElement("Full Name");
        m_header.addElement("Level");
        m_header.addElement("Access");

    } else if(m_type.equals("u_auditTrailFiles")) {

        m_header.addElement("Time began");
        m_header.addElement("Time terminated");

    } else if(m_type.equals("u_auditTrails")) {

        m_header.addElement("Time");
        m_header.addElement("admin");
        m_header.addElement("User");
        m_header.addElement("Action");

    } else if(m_type.equals("x_auditTrailFiles")) {

        m_header.addElement("Time began");
        m_header.addElement("Time terminated");
        m_header.addElement("Console");

    } else if(m_type.startsWith("x_auditTrails_")) {

        m_header.addElement("Time");
        m_header.addElement("Action");
        m_header.addElement("User");
        m_header.addElement("Path");
        m_header.addElement("Owner");
        m_header.addElement("Info");

    } else if(m_type.equals("recordTrashInfo")) {

        m_header.addElement("Time saved");
        m_header.addElement("Data ID");
        m_header.addElement("User");
        m_header.addElement("Console");
        m_header.addElement("Records");

    } else if(m_type.equals("recordTrashInfo2")) {

        m_header.addElement("Time deleted");
        m_header.addElement("Fullpath");
        m_header.addElement("Operator");
        m_header.addElement("Owner");
        m_header.addElement("Sample name");
        m_header.addElement("Seqfil");

    } else {
        m_header.addElement("none");
    }
    }

    // build a tableModel of given type

    private boolean buildFileTableModel() {

        if (m_type != null)
            m_type = m_type.trim();
        if(m_type == null || m_type.length() == 0)
            return false;

    // make vector m_header.
    makeHeader();

    // clear up m_values2D and m_rowValues.
    for(int i=0; i<m_values2D.size(); i++) {
        m_values2D.clear();
    }
    m_rowValues.clear();
    m_flags.clear();

    // build a 2D vector m_values2D from given path(s) for the table.
    for(int i=0; i<m_paths.size(); i++) {

        String path = (String)m_paths.elementAt(i);
            if (path != null)
                path=path.trim();
            if(path==null || path.length()==0)
                continue;

        File file=new File(path);
        if(file == null) continue;

        build2DVector(path);
    }

    /* add current session to s_auditTrailFiles */
    if(m_type.equals("s_auditTrailFiles") && !(Util.getAppIF() instanceof VAdminIF)) {
        Vector row = new Vector();
            row.addElement("current");
            row.addElement("current");
        String usr=System.getProperty("user.name");
            row.addElement(usr);
        String host = Util.getHostName();
            row.addElement(host);
            m_values2D.addElement(row);
        String value =
        FileUtil.savePath( "USER/PERSISTENCE");
        m_rowValues.addElement(value);
            m_flags.add(new Boolean(false));
    }

    if(m_type.equals("records")) {
        int bad = 0;
        for(int i=0; i<m_flags.size(); i++) {
        if(((Boolean)m_flags.elementAt(i)).booleanValue()) bad++;
        }
        m_badRows = bad;
        m_goodRows = m_flags.size() - bad;
    } else {
        m_goodRows = m_flags.size();
        m_badRows = 0;
    }

    if(m_values2D.size() == 0) {
        Vector row = new Vector();
        for(int i=0; i<m_header.size(); i++) row.addElement("");
        m_values2D.addElement(row);
        m_rowValues.addElement("");
        m_flags.add(new Boolean(false));
    }

    // build tableModel from m_values2D and m_header.
    setDataVector(m_values2D, m_header);

    return (true);
    }

    private void build2DVector(String path)
    {

    if(debug)
    System.out.println("FileTableModel:build2DVector " + path +" "+ m_type);

    if(m_type.equals("cmdHistory")) build2DVector_cmdHistory(path);
    else if(m_type.equals("a_auditTrails")) build2DVector_a_auditTrails(path);
    else if(m_type.equals("a_auditTrailFiles")) build2DVector_a_auditTrailFiles(path);
    else if(m_type.endsWith("auditTrail")) build2DVector_auditTrail(path);
    else if(m_type.equals("s_auditTrailFiles")) build2DVector_s_auditTrailFiles(path);
    else if(m_type.equals("s_userProfiles")) build2DVector_s_userProfiles(path);
    else if(m_type.equals("records")) build2DVector_records(path);
    else if(m_type.equals("recordChecksumInfo")) build2DVector_recordChecksumInfo(path);
    else if(m_type.equals("u_auditTrailFiles")) build2DVector_u_auditTrailFiles(path);
    else if(m_type.equals("u_auditTrails")) build2DVector_u_auditTrails(path);
    else if(m_type.equals("x_auditTrailFiles")) build2DVector_x_auditTrailFiles(path);
    else if(m_type.startsWith("x_auditTrails_")) build2DVector_x_auditTrails(path);
    else if(m_type.equals("recordTrashInfo")) build2DVector_recordTrashInfo(path);
    else if(m_type.equals("recordTrashInfo2")) getRecordTrashInfo2(path);
    }

    private void build2DVector_cmdHistory(String path)
    {

    File file=new File(path);

    if(file.isFile() && (path.endsWith("cmdHistory") || path.endsWith(".vaudit")))
        getCmdHistory(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_cmdHistory(child);
                  }
            }
    }
    }

    private String getDescription(String cmds) {
    // only for the first cmd even there is more than one.
    // is there any other delimiters other than " ,=?:(" ?

    String first;
    StringTokenizer tok;
    tok=new StringTokenizer(cmds, " ,=?:(");
    if(tok.hasMoreTokens()) {
        first = tok.nextToken();
        if(cmds.indexOf(first + "?") != -1 || cmds.indexOf(first + " ?") != -1)
           return VPropertyBundles.getValue("paramResources", first);
        else if(cmds.indexOf(first + "=") != -1 || cmds.indexOf(first + " =") != -1)
           return VPropertyBundles.getValue("paramResources", first);
        else 
           return VPropertyBundles.getValue("cmdResources", first);
    } else return cmds;
    }

    private boolean getCmdHistory(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0 ||
            line.indexOf("cmd") == -1)
            continue;
            StringTokenizer tok;
        tok=new StringTokenizer(line, " ");
        if(tok.countTokens() < 3 )
            continue;
            String time=tok.nextToken();
            String key=tok.nextToken();
            String cmds="";
            if(key.equals("cmd") && tok.hasMoreElements()){
            while(tok.hasMoreElements()){
                cmds+=tok.nextToken(" \t\"");
                if(tok.hasMoreElements())
                cmds+=" ";
            }
            Vector row = new Vector();
            row.addElement(convertTime(time.trim()));
            row.addElement(cmds.trim());
            row.addElement(getDescription(cmds.trim()));
            m_rowValues.addElement(cmds.trim());
            m_values2D.addElement(row);
            m_flags.add(new Boolean(false));
            }
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private void build2DVector_auditTrail(String path)
    {

    File file=new File(path);

    if(file.isFile() && (path.endsWith("auditTrail") || path.endsWith(".vaudit")))
        getAuditTrail(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_auditTrail(child);
                  }
            }
    }
    }

    private boolean getAuditTrail(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    Vector row = null;

    try
    {
        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0 ||
            (line.indexOf("audit") == -1 &&
            line.indexOf("dest path") == -1 &&
            line.indexOf("orig path") == -1) )
            continue;
            StringTokenizer tok;
        tok=new StringTokenizer(line, " ");
        if(m_type.equals("s_auditTrail")) {
          if(tok.countTokens() < 2 )
            continue;
              String time=tok.nextToken();
              String key=tok.nextToken();
          if(key.startsWith("audit")) {
                String func= tok.nextToken();
            String cmds = "";
                if(tok.hasMoreElements()){
              while(tok.hasMoreElements()){
                cmds+=tok.nextToken(" \t\"");
                if(tok.hasMoreElements())
                cmds+=" ";
              }
            }
            row = new Vector();
            row.addElement(convertTime(time.trim()));
            //row.addElement(func.trim());
            row.addElement(cmds.trim());
          } else if(key.equals("path:") && time.equals("orig")) {
            String input = "";
            if(tok.hasMoreElements()){
              while(tok.hasMoreElements()){
                input+=tok.nextToken(" \t\"");
                if(tok.hasMoreElements())
                input+=" ";
              }
            }
            if(input.length() <= 0) input = "-";
            row.addElement(input.trim());
          } else if(key.equals("path:") && time.equals("dest")) {
            String output = "";
            if(tok.hasMoreElements()){
              while(tok.hasMoreElements()){
                output+=tok.nextToken(" \t\"");
                if(tok.hasMoreElements())
                output+=" ";
              }
            }
            if(output.length() <= 0) output = "-";
            row.addElement(output.trim());
            m_rowValues.addElement(output.trim());
            m_values2D.addElement(row);
            m_flags.add(new Boolean(false));
          }
            } else if(m_type.equals("d_auditTrail")) {
          if(tok.countTokens() < 2 )
            continue;
              String time=tok.nextToken();
              String key=tok.nextToken();
          if(key.startsWith("audit")) {
                String console= tok.nextToken();
                String user= tok.nextToken();
                String func= tok.nextToken();
            String cmds = "";
                if(tok.hasMoreElements()){
              while(tok.hasMoreElements()){
                cmds+=tok.nextToken(" \t\"");
                if(tok.hasMoreElements())
                cmds+=" ";
              }
            }
            row = new Vector();
            row.addElement(convertTime(time.trim()));
            row.addElement(console.trim());
            row.addElement(user.trim());
            //row.addElement(func.trim());
            row.addElement(cmds.trim());
          } else if(key.equals("path:") && time.equals("orig")) {
            String input = "-";
            if(tok.hasMoreElements())
            input = tok.nextToken();
            row.addElement(input.trim());
          } else if(key.equals("path:") && time.equals("dest")) {
            String output = "-";
            if(tok.hasMoreElements())
            output = tok.nextToken();
            row.addElement(output.trim());
            m_rowValues.addElement(output.trim());
            m_values2D.addElement(row);
            m_flags.add(new Boolean(false));
          }
        }
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private void build2DVector_s_auditTrailFiles(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".vaudit"))
        get_s_auditTrailFile(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_s_auditTrailFiles(child);
                  }
            }
    }
    }

    private boolean get_s_auditTrailFile(String path) {

    String name = path.substring(path.lastIndexOf(File.separator) + 1);
    StringTokenizer tok;
    // The filename contains information separated by '^' chars
    tok=new StringTokenizer(name, "^");
    int cnt = tok.countTokens();
    if(tok.countTokens() != 4) {
        /* previous version used '_' for token separator.  Check for old file type */
        StringTokenizer tok2 = new StringTokenizer(name, "_");
        cnt = tok2.countTokens();
        if(tok2.countTokens() == 4)
            // If 4 tokens, it must be using '_', so use this tokenizer below
            tok = tok2;
        else {
            // Must just be a bad line
            Messages.postDebug("Auditing Panel Skipping bad filename\n        " 
                    + path);
            return false;
        }
    }

    String logintime=tok.nextToken();
    String logouttime=tok.nextToken();
    String console=tok.nextToken();
    String user=tok.nextToken();
    user = user.substring(0, user.lastIndexOf(".vaudit"));
    Vector row = new Vector();
    row.addElement(convertTime(logintime.trim()));
    row.addElement(convertStrTime(logouttime.trim()));
    row.addElement(user.trim());
    row.addElement(console.trim());

    m_rowValues.addElement(path);
    m_values2D.addElement(row);
    m_flags.add(new Boolean(false));

    return true;
    }

    private void build2DVector_records(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.indexOf(".REC") != -1 &&
    path.indexOf("acqfil") != -1) {

        path = path.substring(0, path.lastIndexOf("acqfil") + 5);
        getRecords(path);

    } else if(file.isFile() && path.indexOf(".REC") != -1 &&
    path.indexOf("datdir") != -1) {

        path = path.substring(0, path.lastIndexOf("datdir") + 8);
        getRecords(path);

    } else if(path.indexOf(".REC") != -1 &&
    (path.indexOf("acqfil") != -1 || path.indexOf("datdir") != -1))
        getRecords(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_records(child);
                  }
            }
    }
    }

    private boolean getRecords(String path) {

    String time_saved = "?";
    String time_run = "?";
    String dataid = "?";
    String name = "?";
    String console = "?";
    String user = "?";
    String operator = "";
    Boolean bflag = null;

    // get invalid file flag
    String flag = path +"/checksum.flag";
    File file=new File(flag);
    if(file.exists()) bflag = new Boolean(true);
    else bflag = new Boolean(false);

    // get records name

    String projName = "";
    String recName = "";
    String datName = "";
        StringTokenizer tok;
    tok=new StringTokenizer(path, File.separator);
    while(tok.hasMoreElements()){

        String str =tok.nextToken();
        if(str.endsWith(".REC")) recName = projName + "/" + str;
        if(str.startsWith("acqfil") || str.startsWith("datdir")) datName = str;
	projName = str;
    }
    name = recName + "/" + datName;

    // get time_saved, dataid, username and console from curpar

    FileReader fr;
    try {
            fr=new FileReader(path + "/curpar");
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0)
            continue;
        tok=new StringTokenizer(line, " ");
        if(tok.countTokens() == 11 ) {
                String param =tok.nextToken();
            line=text.readLine();
            if(param.equals("time_saved") && (line !=null)) {
              StringTokenizer t = new StringTokenizer(line, " ");
              if(t.countTokens() > 1) {
            t.nextToken();
                String str = t.nextToken();
            if(str.length() > 2)
            time_saved = str.substring(1,str.length()-1);
              }
            }
            if(param.equals("time_run") && (line !=null)) {
              StringTokenizer t = new StringTokenizer(line, " ");
              if(t.countTokens() > 1) {
            t.nextToken();
                String str = t.nextToken();
            if(str.length() > 2)
            time_run = str.substring(1,str.length()-1);
              }
            }
            if(param.equalsIgnoreCase("dataid") && (line !=null)) {
              StringTokenizer t = new StringTokenizer(line, " ");
              if(t.countTokens() > 1) {
            t.nextToken();
                String str = t.nextToken();
            if(str.length() > 2)
            dataid = str.substring(1,str.length()-1);
              }
            }
            if(param.equals("username") && (line !=null)) {
              StringTokenizer t = new StringTokenizer(line, " ");
              if(t.countTokens() > 1) {
            t.nextToken();
                String str = t.nextToken();
            if(str.length() > 2)
            user = str.substring(1,str.length()-1);
              }
            }
            if(param.equals("operator_") && (line !=null)) {
              StringTokenizer t = new StringTokenizer(line, " ");
              if(t.countTokens() > 1) {
            t.nextToken();
                String str = t.nextToken();
            if(str.length() > 2)
            operator = ":"+str.substring(1,str.length()-1);
              }
            }
            if(param.equals("console") && (line !=null)) {
              StringTokenizer t = new StringTokenizer(line, " ");
              if(t.countTokens() > 1) {
            t.nextToken();
                String str = t.nextToken();
            if(str.length() > 2)
            console = str.substring(1,str.length()-1);
              }
            }
            line=text.readLine();
        }
        }
        Vector row = new Vector();
	if(time_saved.length() <= 1) time_saved = time_run;
        row.addElement(convertTime(time_saved.trim()));
        row.addElement(dataid);
        row.addElement(user+operator);
        row.addElement(console);
        row.addElement(name);
        m_rowValues.addElement(path);
        m_values2D.addElement(row);
        m_flags.addElement(bflag);
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private void build2DVector_recordChecksumInfo(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".flag"))
        getRecordChecksumInfo(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_recordChecksumInfo(child);
                  }
            }
    }
    }

    private boolean getRecordChecksumInfo(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0)
            continue;
            StringTokenizer tok;
        tok=new StringTokenizer(line, "|");
        if(tok.countTokens() < 2 )
            continue;
            String time=tok.nextToken();
            String info="";
        while(tok.hasMoreElements()){
            info+=tok.nextToken();
            if(tok.hasMoreElements())
            info+=" ";
        }
        Vector row = new Vector();
            row.addElement(convertTime(time.trim()));
        row.addElement(info.trim());
        m_rowValues.addElement(info.trim());
        m_values2D.addElement(row);
        m_flags.add(new Boolean(false));
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private void build2DVector_a_auditTrails(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".aaudit"))
        get_a_auditTrail(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_a_auditTrails(child);
                  }
            }
    }
    }

    private boolean get_a_auditTrail(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0
            || line.indexOf("aaudit") == -1)
            continue;
            StringTokenizer tok;
        tok=new StringTokenizer(line, " ");
        if(tok.countTokens() < 1 )
            continue;
        String str = "";
        String time = "";
        while(tok.hasMoreElements() &&
            !(str = tok.nextToken()).equals("aaudit")){
            time+=str;
            time+=" ";
        }

            if(str.equals("aaudit") && tok.hasMoreElements()) {
	    String user = tok.nextToken();
            String cmds="";
                if(tok.hasMoreElements()) cmds+=tok.nextToken();
                if(tok.hasMoreElements()) {
            cmds+=" ";
            cmds+=tok.nextToken();
            }
                String info="";
            while(tok.hasMoreElements()){
              info+=tok.nextToken();
              if(tok.hasMoreElements())
              info+=" ";
            }
            Vector row = new Vector();
            row.addElement(convertTime(time.trim()));
            row.addElement(user.trim());
            row.addElement(cmds.trim());
            row.addElement(info.trim());
            m_rowValues.addElement(cmds.trim());
            m_values2D.addElement(row);
            m_flags.add(new Boolean(false));
        }
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private Date convertTime(String str)
    {
    // input str has the form D2002-04-05T10:53:25
    // or 20010119T145434
        Calendar calendar = Calendar.getInstance();
        Date date = new Date();
    if(str.indexOf("open") != -1 || str.indexOf("current") != -1
        || str.indexOf("not_terminated") != -1) { 
           calendar.set(0, 0, 0, 0, 0, 0);
           return calendar.getTime();
    }
                                                                               
    if(str.length() == 20 && str.startsWith("D") &&
        str.indexOf("T") == 11) {
                                                                               
        StringTokenizer tok=new StringTokenizer(str, "D-T:\n");
        if(tok.countTokens() < 4 ) {
           calendar.set(0, 0, 0, 0, 0, 0);
           return calendar.getTime();
	}
        String year=tok.nextToken();
        int month = (new Integer(tok.nextToken())).intValue();
        String day=tok.nextToken();
        //String time=tok.nextToken();
        int hour = Integer.parseInt(tok.nextToken());
        int minute = Integer.parseInt(tok.nextToken());
        int second = Integer.parseInt(tok.nextToken());
        calendar.set(Integer.parseInt(year), month-1, Integer.parseInt(day), hour,
                     minute, second);
                                                                               
        return calendar.getTime();
                                                                               
    } else if(str.length() == 15 && str.indexOf("T") == 8) {
        String year = str.substring(0,4);
        String mon = str.substring(4,6);
        int month = (new Integer(mon)).intValue();
        String day = str.substring(6,8);
        String time = str.substring(9,11)+":"+str.substring(11,13)
        +":"+str.substring(13,15);
        int hour = Integer.parseInt(str.substring(9,11));
        int minute = Integer.parseInt(str.substring(11,13));
        int second = Integer.parseInt(str.substring(13,15));
                                                                               
        calendar.set(Integer.parseInt(year), month-1, Integer.parseInt(day), hour,
                     minute, second);
                                                                               
            return calendar.getTime();
                                                                               
    } else if(str.length() == 14) {
        String year = str.substring(0,4);
        String mon = str.substring(4,6);
        int month = (new Integer(mon)).intValue();
        String day = str.substring(6,8);
        String time = str.substring(8,10)+":"+str.substring(10,12)
        +":"+str.substring(12,14);
        int hour = Integer.parseInt(str.substring(8,10));
        int minute = Integer.parseInt(str.substring(10,12));
        int second = Integer.parseInt(str.substring(12,14));
                                                                               
        calendar.set(Integer.parseInt(year), month-1, Integer.parseInt(day), hour,
                     minute, second);
                                                                               
            return calendar.getTime();
                                                                               
    } else {
        StringTokenizer tok=new StringTokenizer(str, " _-");
	if(tok.countTokens() == 6) {
           tok.nextToken();
	   Object m=m_months.get(tok.nextToken());
	   if(m == null) {
             calendar.set(0, 0, 0, 0, 0, 0);
             return calendar.getTime();
	   } 
           int month=Integer.parseInt((String)m);
           int day=Integer.parseInt(tok.nextToken());
           String time=tok.nextToken();
           tok.nextToken();
           int year=Integer.parseInt(tok.nextToken());
           tok=new StringTokenizer(time, ":");
	   if(tok.countTokens() == 3) {
              int hour = Integer.parseInt(tok.nextToken());
              int minute = Integer.parseInt(tok.nextToken());
              int second = Integer.parseInt(tok.nextToken());
              calendar.set(year, month-1, day, hour, minute, second);
              return calendar.getTime();

	   } else {
             calendar.set(0, 0, 0, 0, 0, 0);
             return calendar.getTime();
	   }
	} else {
           calendar.set(0, 0, 0, 0, 0, 0);
           return calendar.getTime();
	}
    }
    }

    private String convertStrTime(String str)
    {
    // input str has the form D2002-04-05T10:53:25
    // or 20010119T145434
        Calendar calendar = Calendar.getInstance();
        Date date = new Date();
    if(str.indexOf("open") != -1 || str.indexOf("current") != -1
        || str.indexOf("not_terminated") != -1) {
           calendar.set(0, 0, 0, 0, 0, 0);
           return calendar.getTime().toString();
    }
                                                                               
    if(str.length() == 20 && str.startsWith("D") &&
        str.indexOf("T") == 11) {
                                                                               
        StringTokenizer tok=new StringTokenizer(str, "D-T:\n");
        if(tok.countTokens() < 4 ) {
           calendar.set(0, 0, 0, 0, 0, 0);
           return calendar.getTime().toString();
	}
        String year=tok.nextToken();
        int month = (new Integer(tok.nextToken())).intValue();
        String day=tok.nextToken();
        //String time=tok.nextToken();
        int hour = Integer.parseInt(tok.nextToken());
        int minute = Integer.parseInt(tok.nextToken());
        int second = Integer.parseInt(tok.nextToken());
        calendar.set(Integer.parseInt(year), month-1, Integer.parseInt(day), hour,
                     minute, second);
                                                                               
        return calendar.getTime().toString();
                                                                               
    } else if(str.length() == 15 && str.indexOf("T") == 8) {
        String year = str.substring(0,4);
        String mon = str.substring(4,6);
        int month = (new Integer(mon)).intValue();
        String day = str.substring(6,8);
        String time = str.substring(9,11)+":"+str.substring(11,13)
        +":"+str.substring(13,15);
        int hour = Integer.parseInt(str.substring(9,11));
        int minute = Integer.parseInt(str.substring(11,13));
        int second = Integer.parseInt(str.substring(13,15));
                                                                               
        calendar.set(Integer.parseInt(year), month-1, Integer.parseInt(day), hour,
                     minute, second);
                                                                               
            return calendar.getTime().toString();
                                                                               
    } else if(str.length() == 14) {
        String year = str.substring(0,4);
        String mon = str.substring(4,6);
        int month = (new Integer(mon)).intValue();
        String day = str.substring(6,8);
        String time = str.substring(8,10)+":"+str.substring(10,12)
        +":"+str.substring(12,14);
        int hour = Integer.parseInt(str.substring(8,10));
        int minute = Integer.parseInt(str.substring(10,12));
        int second = Integer.parseInt(str.substring(12,14));
                                                                               
        calendar.set(Integer.parseInt(year), month-1, Integer.parseInt(day), hour,
                     minute, second);
                                                                               
            return calendar.getTime().toString();
                                                                               
    } else {
        StringTokenizer tok=new StringTokenizer(str, " _-");
	if(tok.countTokens() == 6) {
           tok.nextToken();
	   Object m=m_months.get(tok.nextToken());
	   if(m == null) {
             calendar.set(0, 0, 0, 0, 0, 0);
             return calendar.getTime().toString();
	   }
           int month=Integer.parseInt((String)m);
           int day=Integer.parseInt(tok.nextToken());
           String time=tok.nextToken();
           tok.nextToken();
           int year=Integer.parseInt(tok.nextToken());
           tok=new StringTokenizer(time, ":");
	   if(tok.countTokens() == 3) {
              int hour = Integer.parseInt(tok.nextToken());
              int minute = Integer.parseInt(tok.nextToken());
              int second = Integer.parseInt(tok.nextToken());
              calendar.set(year, month-1, day, hour, minute, second);
              return calendar.getTime().toString();

	   } else {
             calendar.set(0, 0, 0, 0, 0, 0);
             return calendar.getTime().toString();
	   }
	} else {
           calendar.set(0, 0, 0, 0, 0, 0);
           return calendar.getTime().toString();
	}
    }
    }

    private void build2DVector_s_userProfiles(String path)
    {
    // path is /vnmr/adm/users/profiles/system/ + user

    File file=new File(path);

    if(file.isFile())
        get_s_userProfiles(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_s_userProfiles(child);
                  }
            }
    }
    }

    private boolean get_s_userProfiles(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    String user = path.substring(path.lastIndexOf(File.separator) + 1);
    String name = "";
    String level = "";
    String access = "";

    try
    {
        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0 )
            continue;
            StringTokenizer tok;
        tok=new StringTokenizer(line, " ");
        if(tok.countTokens() < 2 )
            continue;
        String key=tok.nextToken();
        String value = tok.nextToken();
        if(key.equals("name")) name = value;
        if(key.equals("usrlvl")) level = value;
        if(key.equals("access")) access = value;
        }
        Vector row = new Vector();
        row.addElement(user.trim());
        row.addElement(name.trim());
        row.addElement(level.trim());
        row.addElement(access.trim());
        m_rowValues.addElement(user.trim());
        m_values2D.addElement(row);
        m_flags.add(new Boolean(false));
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private void build2DVector_u_auditTrailFiles(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".uat"))
        get_u_auditTrailFile(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_u_auditTrailFiles(child);
                  }
            }
    }
    }

    private boolean get_u_auditTrailFile(String path) {

    String name = path.substring(path.lastIndexOf(File.separator) + 1);
        StringTokenizer tok;
    tok=new StringTokenizer(name, ".");
    if(tok.countTokens() < 3 )
    return false;

    String timeb=tok.nextToken();
    String timet=tok.nextToken();
    Vector row = new Vector();
    row.addElement(convertTime(timeb.trim()));
    row.addElement(convertStrTime(timet.trim()));

    m_rowValues.addElement(path);
    m_values2D.addElement(row);
    m_flags.add(new Boolean(false));

    return true;
    }

    private void build2DVector_x_auditTrailFiles(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".cond"))
        get_x_auditTrailFile(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_x_auditTrailFiles(child);
                  }
            }
    }
    }

    private boolean get_x_auditTrailFile(String path) {

    String name = path.substring(path.lastIndexOf(File.separator) + 1);
        StringTokenizer tok;
    tok=new StringTokenizer(name, ".");
    if(tok.countTokens() < 4 )
    return false;

    String timeb=tok.nextToken();
    String timet=tok.nextToken();
    String console=tok.nextToken();
    Vector row = new Vector();
    row.addElement(convertTime(timeb.trim()));
    row.addElement(convertStrTime(timet.trim()));
    row.addElement(console.trim());

    m_rowValues.addElement(path);
    m_values2D.addElement(row);
    m_flags.add(new Boolean(false));

    return true;
    }

    private void build2DVector_u_auditTrails(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".uat"))
        get_u_auditTrail(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_u_auditTrails(child);
                  }
            }
    }
    }

    private boolean get_u_auditTrail(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);
    String time;
    String user;
    String info;
    String admin;

    try
    {
        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0)
            continue;
            StringTokenizer tok;
        tok=new StringTokenizer(line, "|");
        if(tok.countTokens() < 3 )
            continue;
        if(tok.countTokens() == 3) {
           time = tok.nextToken();
           user = tok.nextToken();
           admin = System.getProperty("user.name"); 
           info = tok.nextToken();
        } else {
           time = tok.nextToken();
           admin = tok.nextToken();
           user = tok.nextToken();
           info = tok.nextToken();
        }

        Vector row = new Vector();
        row.addElement(convertTime(time.trim()));
        row.addElement(admin.trim());
        row.addElement(user.trim());
        row.addElement(info.trim());
        m_rowValues.addElement(info.trim());
        m_values2D.addElement(row);
        m_flags.add(new Boolean(false));
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private void build2DVector_x_auditTrails(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".cond"))
        get_x_auditTrail(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_x_auditTrails(child);
                  }
            }
    }
    }

    private void get_x_auditTrail(String path) {

    if(m_type.equals("x_auditTrails_recDele")) get_x_auditTrails_recDele(path);
    else if(m_type.equals("x_auditTrails_failure")) get_x_auditTrails_failure(path);
    else if(m_type.equals("x_auditTrails_openWrite")) get_x_auditTrails_event(path, "open");
    else if(m_type.equals("x_auditTrails_unlink")) get_x_auditTrails_event(path, "unlink");
    else if(m_type.equals("x_auditTrails_creat")) get_x_auditTrails_event(path,"creat");
    else if(m_type.equals("x_auditTrails_rename")) get_x_auditTrails_event(path,"rename");

    }

    private boolean get_x_auditTrails_event(String path, String event) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
        int count = 0;

        String time = null;
        String action = null;
        String user = null;
        String filepath = null;
        String owner = null;
        String info = null;
	Vector paths = new Vector();

        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0)
            continue;
	//skip from header to return if not the event interested
        if(line.startsWith("header") &&
            (line.indexOf(event) == -1 ||
            (event.equals("open") && line.indexOf("write") == -1))) {

            while(!line.startsWith("return"))
                        line=text.readLine();
            count = 0;
	    paths.clear();
                    continue;
        }
            StringTokenizer tok;
        tok=new StringTokenizer(line, ",");
        if(tok.countTokens() < 1 )
            continue;
        String key = tok.nextToken();
        if(key.equals("header") && tok.countTokens() > 3) {
            tok.nextToken();
            tok.nextToken();
            action = new String();
            time = new String();
            action = tok.nextToken();
            String str = "";
            while(tok.hasMoreElements()){
                      str=tok.nextToken();
                      if(str.indexOf(":") == -1) action += " " + str;
                      if(str.indexOf(":") != -1) {
            time = str;
            break;
              }
                    }

            if(action.startsWith("unlink")) action = "delete";

            count++;
            count++;
        } else if(key.equals("path") && tok.countTokens() > 0) {
            filepath = new String();
            filepath = tok.nextToken();
	    paths.addElement(filepath + ", ");
            count++;
        } else if(key.equals("attribute") && tok.countTokens() > 1) {
            tok.nextToken();
            owner = new String();
            owner = tok.nextToken();
            count++;
        } else if(key.equals("subject") && tok.countTokens() > 3) {
            tok.nextToken();
            tok.nextToken();
            tok.nextToken();
            user = new String();
            user = tok.nextToken();
            count++;
        } else if(key.equals("return") && tok.countTokens() > 0) {
            info = new String();
            info = "";
            while(tok.hasMoreElements()){
              info+=tok.nextToken();
              if(tok.hasMoreElements())
              info+=" ";
            }
            count++;
            if(count >= 6) {
            Vector row = new Vector();
            row.addElement(convertTime(time.trim()));
            row.addElement(action.trim());
            row.addElement(user.trim());

	    String allPath = ""; 
	    for(int i=0; i<paths.size(); i++) 
	    allPath += paths.elementAt(i);
            row.addElement(allPath.substring(0, allPath.length()-2));

            row.addElement(owner.trim());
            row.addElement(info.trim());
            m_rowValues.addElement(action +" "+ allPath);
            m_values2D.addElement(row);
            m_flags.add(new Boolean(false));
            }
            count = 0;
	    paths.clear();
        }
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private boolean get_x_auditTrails_recDele(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
        int count = 0;
        String time = null;
        String action = "delete";
        String user = null;
        String filepath = null;
        String owner = null;
        String info = null;

        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0)
            continue;
        if(line.startsWith("header") && line.indexOf("unlink") == -1) {
            while(!line.startsWith("return"))
                line=text.readLine();
            count = 0;
            continue;
        }
            StringTokenizer tok;
        tok=new StringTokenizer(line, ",");
        if(tok.countTokens() < 1 )
            continue;
        String key = tok.nextToken();
        if(key.equals("header") && tok.countTokens() > 3) {
            time = new String();
            String str = "";
            while(tok.hasMoreElements()){
              str=tok.nextToken();
              if(str.indexOf(":") != -1) time = str;
            }
            count++;
        } else if(key.equals("path") && tok.countTokens() > 0) {
            filepath = new String();
            String str = tok.nextToken();
	    if(str.indexOf(".REC") != -1) {
		filepath = str;
                count++;
            } else {
                while(!line.startsWith("return")) line=text.readLine();
                count = 0;
                continue;
            }
        } else if(key.equals("attribute") && tok.countTokens() > 1) {
            tok.nextToken();
            owner = new String();
            owner = tok.nextToken();
            count++;
        } else if(key.equals("subject") && tok.countTokens() > 3) {
            tok.nextToken();
            tok.nextToken();
            tok.nextToken();
            user = new String();
            user = tok.nextToken();
            count++;
        } else if(key.equals("return") && tok.countTokens() > 0) {
            info = new String();
            info = "";
            while(tok.hasMoreElements()){
              info+=tok.nextToken();
              if(tok.hasMoreElements())
              info+=" ";
            }
            count++;
            if(count >= 5) {
            Vector row = new Vector();
            row.addElement(convertTime(time.trim()));
            row.addElement(action.trim());
            row.addElement(user.trim());
            row.addElement(filepath.trim());
            row.addElement(owner.trim());
            row.addElement(info.trim());
            m_rowValues.addElement(action +" "+ filepath);
            m_values2D.addElement(row);
            m_flags.add(new Boolean(false));
            }
            count = 0;
        }
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private boolean get_x_auditTrails_failure(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
        int count = 0;

        String time = null;
        String action = null;
        String user = null;
        String filepath = null;
        String owner = null;
        String info = null;

        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0)
            continue;
            StringTokenizer tok;
        tok=new StringTokenizer(line, ",");
        if(tok.countTokens() < 1 )
            continue;
        String key = tok.nextToken();
        if(key.equals("header") && tok.countTokens() > 3) {
            tok.nextToken();
            tok.nextToken();
            action = new String();
            time = new String();
            action = tok.nextToken();
            String str = "";
            while(tok.hasMoreElements()){
                      str=tok.nextToken();
                      if(str.indexOf(":") == -1) action += " " + str;
                      if(str.indexOf(":") != -1) {
            time = str;
            break;
              }
                    }

            if(action.startsWith("unlink")) action = "delete";

            count++;
            count++;
        } else if(key.equals("path") && tok.countTokens() > 0) {
            filepath = new String();
            filepath = tok.nextToken();
            count++;
        } else if(key.equals("attribute") && tok.countTokens() > 1) {
            tok.nextToken();
            owner = new String();
            owner = tok.nextToken();
            count++;
        } else if(key.equals("subject") && tok.countTokens() > 3) {
            tok.nextToken();
            tok.nextToken();
            tok.nextToken();
            user = new String();
            user = tok.nextToken();
            count++;
        } else if(key.equals("return") && tok.countTokens() > 0) {
            info = new String();
            info = "";
            while(tok.hasMoreElements()){
              info+=tok.nextToken();
              if(tok.hasMoreElements())
              info+=" ";
            }

            count++;
            if(info.indexOf("failure") != -1 && count >= 6) {

              Vector row = new Vector();
              row.addElement(convertTime(time.trim()));
              row.addElement(action.trim());
              row.addElement(user.trim());
              row.addElement(filepath.trim());
              row.addElement(owner.trim());
              row.addElement(info.trim());
              m_rowValues.addElement(action +" "+ filepath);
              m_values2D.addElement(row);
              m_flags.add(new Boolean(false));
            }
            count = 0;
        }
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private void build2DVector_a_auditTrailFiles(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".aaudit"))
        get_a_auditTrailFile(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_a_auditTrailFiles(child);
                  }
            }
    }
    }

    private boolean get_a_auditTrailFile(String path) {

    String name = path.substring(path.lastIndexOf(File.separator) + 1);
        StringTokenizer tok;
    tok=new StringTokenizer(name, ".");
    if(tok.countTokens() < 3 )
    return false;

    String timeb=tok.nextToken();
    String timet=tok.nextToken();
    Vector row = new Vector();
    row.addElement(convertTime(timeb.trim()));
    row.addElement(convertStrTime(timet.trim()));

    m_rowValues.addElement(path);
    m_values2D.addElement(row);
    m_flags.add(new Boolean(false));

    return true;
    }

    private void build2DVector_recordTrashInfo(String path)
    {

    File file=new File(path);

    if(file.isFile() && path.endsWith(".trash"))
        getRecordTrashInfo(path);

    else {
        File[] children = file.listFiles();
            if(children != null) {

                  for(int i = 0; i < children.length; i++) {
                    String child = children[i].getPath();
            build2DVector_recordTrashInfo(child);
                  }
            }
    }
    }

    private boolean getRecordTrashInfo(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileTableModel: file not found "+path);
            return false;
    }

    BufferedReader text = new BufferedReader(fr);

    try
    {
	    int count = 0;
	    String fullpath = "?";
	    String recName = "?";
	    String user = "?";
	    String owner = "?";
	    String recUser = "?";
	    String dataid = "?";
	    String console = "?";
	    String seqfil = "?";
	    String samplename = "?";
	    String time_saved = "?";
	    String time_run = "?";
	    String time_delete = "?";

        String line=null;
        while((line=text.readLine()) !=null){
            if(line.startsWith("#") || line.length() == 0)
            continue;
            StringTokenizer tok;
            tok=new StringTokenizer(line, "|");
            if(tok.countTokens() < 1 )
            continue;

            String key=tok.nextToken().trim();

	    if(key.equals("path")) {
		count = 0;
		if(tok.hasMoreTokens()) fullpath = tok.nextToken().trim();
		if(fullpath.length() == 0) fullpath = "?";
		count++;
	    }

	    else if(key.equals("record name")) {
		if(tok.hasMoreTokens()) recName = tok.nextToken().trim();
		if(recName.length() == 0) recName = "?";
		count++;
	    }

            else if(key.equals("del operator")) {
                if(tok.hasMoreTokens()) user = tok.nextToken().trim();
		if(user.length() == 0) user = "?";
                count++;
            }

            else if(key.equals("owner")) {
                if(tok.hasMoreTokens()) owner = tok.nextToken().trim();
		if(owner.length() == 0) owner = "?";
                count++;
            }

	    else if(key.equals("acq operator")) {
		if(tok.hasMoreTokens()) recUser = tok.nextToken().trim();
		if(recUser.length() == 0) recUser = "?";
		count++;
	    }

	    else if(key.equalsIgnoreCase("dataid")) {
		if(tok.hasMoreTokens()) dataid = tok.nextToken().trim();
		if(dataid.length() == 0) dataid = "?";
		count++;
	    }

	    else if(key.equals("console")) {
		if(tok.hasMoreTokens()) console = tok.nextToken().trim();
		if(console.length() == 0) console = "?";
		count++;
	    }

            else if(key.equals("seqfil")) {
                if(tok.hasMoreTokens()) seqfil = tok.nextToken().trim();
		if(seqfil.length() == 0) seqfil = "?";
                count++;
	    }

            else if(key.equals("samplename")) {
                if(tok.hasMoreTokens()) samplename = tok.nextToken().trim();
		if(samplename.length() == 0) samplename = "?";
                count++;
            }

	    else if(key.equals("time_saved")) {
		if(tok.hasMoreTokens()) time_saved = tok.nextToken().trim();
		if(time_saved.length() == 0) time_saved = "?";
		count++;
	    }
	    else if(key.equals("time_run")) {
		if(tok.hasMoreTokens()) time_run = tok.nextToken().trim();
		if(time_run.length() == 0) time_run = "?";
		count++;
	    }

            else if(key.equals("time_delete")) {
                if(tok.hasMoreTokens()) time_delete = tok.nextToken().trim();
		if(time_delete.length() == 0) time_delete = "?";
                count++;
            }

	    if(count == 11) {

            	Vector row = new Vector();
	        if(time_saved.length() <= 1) time_saved = time_run;
            	row.addElement(convertTime(time_saved));
            	row.addElement(dataid);
            	row.addElement(recUser);
            	row.addElement(console);
            	row.addElement(recName);
            	m_rowValues.addElement(fullpath+":"+time_delete+":"+user
			+":"+owner+":"+samplename+":"+seqfil);
            	m_values2D.addElement(row);
            	m_flags.add(new Boolean(false));

		count = 0;
	    }
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileTableModel Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    private boolean getRecordTrashInfo2(String info) {

            StringTokenizer tok=new StringTokenizer(info, ":");
            if(tok.countTokens() >= 6 ) {

		String fullpath = tok.nextToken().trim();
		String time_delete = tok.nextToken().trim();
		String user = tok.nextToken().trim();
		String owner = tok.nextToken().trim();
		String samplename = tok.nextToken().trim();
		String seqfil = tok.nextToken().trim();

            	Vector row = new Vector();
            	row.addElement(convertTime(time_delete));
            	row.addElement(fullpath);
            	row.addElement(user);
            	row.addElement(owner);
            	row.addElement(samplename);
            	row.addElement(seqfil);
            	m_rowValues.addElement(fullpath);
            	m_values2D.addElement(row);
            	m_flags.add(new Boolean(false));
    		return true;
	    }

    	return false;
    }
}


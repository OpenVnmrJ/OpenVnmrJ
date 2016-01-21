/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**
 ** A Menu built from file(s):
 *
 * FileMenu(String file);
 * FileMenu(Vector files);
 *
 * file(s) contains path(s) and types for FileTables
 *
 * @author He Liu
 */
package vnmr.part11;

import java.awt.*;
import java.util.*;
import java.io.*;
import javax.swing.*;
import java.awt.event.*;
import javax.swing.JComboBox;

public class FileMenu extends JComboBox
{

    // input path(s).
    private Vector m_paths = new Vector();

    // output file paths and types.
    private Hashtable m_values = new Hashtable();

    public FileMenu() {
    }

    public FileMenu(String path) {

    m_paths.clear();
    m_paths.addElement(path);

    buildFileMenu();
    }

    public FileMenu(Vector paths) {

    m_paths.clear();
    for(int i=0; i<paths.size(); i++)
    m_paths.addElement(paths.elementAt(i));

    buildFileMenu();
    }

    public JComboBox getMenu() {
    return(this);
    }

    public Hashtable getValues() {
    return(this.m_values);
    }

    public String getSelectedPath(String key) {
    if(key != null && m_values.containsKey(key)) {
        MenuValue value = (MenuValue)this.m_values.get(key);
        return((String)value.getPath());
    } else
            return(null);
    }

    public String getSelectedType(String key) {
    if(key != null && m_values.containsKey(key)) {
        MenuValue value = (MenuValue)this.m_values.get(key);
        return((String)value.getType());
    } else
        return(null);
    }

    public String getSelectedPath() {
    String key = (String)getSelectedItem();
    if(key != null && m_values.containsKey(key)) {
        MenuValue value = (MenuValue)this.m_values.get(key);
        return((String)value.getPath());
    } else
            return(null);
    }

    public String getSelectedType() {
        String key = (String)getSelectedItem();
    if(key != null && m_values.containsKey(key)) {
        MenuValue value = (MenuValue)this.m_values.get(key);
        return((String)value.getType());
    } else
        return(null);
    }

    public void updateFileMenu(Vector paths)
    {
        m_paths.clear();
        for(int i=0; i<paths.size(); i++)
        {
            m_paths.addElement(paths.elementAt(i));
        }

        buildFileMenu();
    }

    // build a menu from given file

    private boolean buildFileMenu() {

    removeAllItems();

    // clear up m_values
    m_values.clear();

    // build menu and m_values from given path(s).
    for(int i=0; i<m_paths.size(); i++) {

        String path = (String)m_paths.elementAt(i);
            if (path != null)
                path=path.trim();
            if(path==null || path.length()==0)
                continue;

        File file = new File(path);
        if(file == null) continue;

        buildMenu(path);
    }

    if(getItemCount() == 0) {
        addItem("none");
        MenuValue value = new MenuValue("none", "none");
        m_values.put("none", value);
    }

    return(true);
    }

    private boolean buildMenu(String path) {

    FileReader fr;
    try {
            fr=new FileReader(path);
    }
    catch (java.io.FileNotFoundException e1){
        System.out.println("FileMenu: file not found "+path);
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
        if(tok.countTokens() < 3 )
            continue;
            String filepath=tok.nextToken();
            String label=tok.nextToken();
            String type=tok.nextToken();
        MenuValue value = new MenuValue(filepath.trim(), type.trim());
        addItem(label.trim());
        m_values.put(label.trim(), value);
        }
    }
    catch(java.io.IOException e){
        System.out.println("FileMenu Error: "+e.toString());
        return false;
    }

    try {
        fr.close();
    } catch(IOException e) { }

    return true;
    }

    class MenuValue {
    protected String path = null;
    protected String type = null;

    public MenuValue(String p, String t) {
        path = p;
        type = t;
    }

    protected String getPath() {
        return(path);
    }

    protected String getType() {
        return(type);
    }
    }
}


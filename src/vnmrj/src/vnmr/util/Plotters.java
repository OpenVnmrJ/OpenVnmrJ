/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import javax.swing.*;
import javax.swing.text.*;
import javax.swing.border.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.beans.*;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.templates.*;

public class Plotters extends ModalPopup
   implements ActionListener, VObjDef
{

    private SessionShare      sshare;
    private ButtonIF          vnmrIF;
    protected static Plotters plotters;

    String xmlfile;

    protected static long m_lastModified = 0;

    protected final static String PERSIS_PLOTTERS_FILE = "USER/PERSISTENCE/plotters";
    protected final static String PERSIS_PRINTERS_FILE = "USER/PERSISTENCE/printers";
    protected final static String DEVICE_FILE = FileUtil.SYS_VNMR + "/devicenames";

    public Plotters(SessionShare ss, ButtonIF ex, AppIF appIf, String xfile, String title,
                    String strhelpfile) {
        super(ss, ex, appIf, title, FileUtil.openPath("INTERFACE/"+xfile), 0, 0,
              true, strhelpfile, "", "", "");

        sshare=ss;
        vnmrIF=ex;
        m_strHelpFile = strhelpfile;
        if(title == null || title.equals(""))
            setTitle("Select devices");
        setLocationRelativeTo(getParent());
    }

    public static Plotters getPlotters(SessionShare ss, ButtonIF ex,  AppIF appIf,
                                       String xfile, String title, String strhelpfile)
    {
        if (plotters == null)
            plotters = new Plotters(ss, ex, appIf, xfile, title, strhelpfile);
        return plotters;
    }

    protected void buildPanel()
    {
        try {
            // If the persistence file that has the list of the printers, and plotters
            // does not exist, then getDeviceList() and write the persistence file
            String path = FileUtil.openPath(PERSIS_PLOTTERS_FILE);
            String path2 = FileUtil.openPath(PERSIS_PRINTERS_FILE);
            if (path == null || path2 == null)
                getDeviceLists();
            else
            {
                // The devicefile which gets written by calling 'adddevices' has the
                // same timestamp as the last time, if not then getDeviceList() and
                // update the persistence file, so that the 'Printers' dialog would
                // show the updated info
                File objFile = new File(DEVICE_FILE);
                long lastModified = objFile.lastModified();
                if (lastModified != m_lastModified)
                    getDeviceLists();
            }

            // Build the xml file
            super.buildPanel();
        }
        catch(Exception e) {
            Messages.writeStackTrace(e);
        }
    }

    //----------------------------------------------------------------
    /** extract printers and plotters from /vnmr/devicenames. */
    //----------------------------------------------------------------
    public static void getDeviceLists() {
        String path=DEVICE_FILE;
        String name=null;
        String use=null;
        Vector printers=new Vector();
        Vector plotters=new Vector();

        FileReader fr;
        try {
            fr=new FileReader(path);
        }
        catch (java.io.FileNotFoundException e1){
            System.out.println("plotters Error: file not found : "+path);
            return ;
        }
        File objFile = new File(path);
        m_lastModified = objFile.lastModified();

        // parse the file and extract key-chval pairs. Build menu list.

        BufferedReader text=new BufferedReader(fr);
        try {
            String line=null;
            while((line=text.readLine()) !=null){
                StringTokenizer tok=new StringTokenizer(line);
                if(!tok.hasMoreTokens())
                    continue;
                String key=tok.nextToken();
                if(key.startsWith("#"))
                    continue;
                if(key.equals("Name") && tok.hasMoreTokens())
                    name=tok.nextToken();
                else if(key.equals("Use") && tok.hasMoreTokens()){
                    use=tok.nextToken();
                    if(name != null){
                        if(use.equals("Plotter"))
                            plotters.add(name);
                        else if(use.equals("Printer"))
                            printers.add(name);
                        else if(use.equals("Both")){
                            printers.add(name);
                            plotters.add(name);
                        }
                    }
                }
            }
            text.close();
            int i;
            String item;
            path=FileUtil.savePath(PERSIS_PLOTTERS_FILE);
            if(path==null){
                System.out.println("Plotters: could not create file : "+path);
                return;
            }
            FileWriter fw;
            BufferedWriter out;
            try {
                fw=new FileWriter(path);
                out=new BufferedWriter(fw);

                for(i=0;i<plotters.size();i++){
                    item=plotters.elementAt(i).toString();
                    String str=item+" "+item;
                    out.write(str,0,str.length());
                    out.newLine();
                }
                out.flush();
                out.close();
            }
            catch (java.io.IOException e1){
                System.out.println("could not create file : "+path);
            }

            path=FileUtil.savePath(PERSIS_PRINTERS_FILE);
            if(path==null){
                System.out.println("Plotters: could not create file : "+path);
                return;
            }
            try {
                fw=new FileWriter(path);
                out=new BufferedWriter(fw);
                for(i=0;i<printers.size();i++){
                    item=printers.elementAt(i).toString();
                    String str=item+" "+item;
                    out.write(str,0,str.length());
                    out.newLine();
                }
                out.flush();
                out.close();
            }
            catch (java.io.IOException e1){
                System.out.println("could not create file : "+path);
            }

        }
        catch(java.io.IOException e){
            System.out.println("plot setup Error: "+e.toString());
        }
    }

    public void setVnmrIf(ButtonIF e) {
        vnmrIF = e;
    }

 }

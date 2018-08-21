/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.print.*;
import javax.print.*;
import javax.print.event.*;
import javax.print.attribute.*;
import javax.print.attribute.standard.*;

import vnmr.util.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VPrint
{

    public static final String CHROMATICITY = "chromaticity";
    public static final String COPIES = "copies";
    public static final String FIDELITY = "fidelity";
    public static final String SIDES = "sides";
    public static final String MEDIA = "media";
    public static final String LAYOUT = "layout";
    public static final String[] m_aStrFlavor = {"jpeg", "gif", "postscript",
                                                   "pcl", "pdf"};

    protected PrintRequestAttributeSet aset;
    protected DocFlavor m_docflavor;


    public VPrint()
    {
        setRequest(null);
    }

    public void print(String strFile, String strLayout)
    {
        DocFlavor flavor = DocFlavor.INPUT_STREAM.JPEG;
        print(strFile, strLayout, flavor);
    }

    public void print(String strFile, String strLayout, DocFlavor flavor)
    {
        m_docflavor = flavor;
        if (strFile == null || strFile.trim().length() == 0)
        {
            Messages.postDebug("Could not open file " + strFile);
            return;
        }

        /* Create a set which specifies how the job is to be printed */
        setRequest(strLayout);

        /* Locate print services which can print a GIF in the manner specified */
        PrintService[] pservices = lookupPrintService(flavor, aset);
        strFile = getFile(flavor, strFile);
        if (pservices.length > 0) {
            /* Create a Print Job */
            DocPrintJob printJob = pservices[0].createPrintJob();
            printJob.addPrintJobListener(new VPrintJobAdapter(strFile));

            /* Create a Doc implementation to pass the print data */
            Doc doc = new InputStreamDoc(strFile, flavor);

            /* Print the doc as specified */
            try {
                printJob.print(doc, aset);
            } catch (Exception e) {
                Messages.postDebug(e.toString());
            }
        } else {
            Messages.postError("Printing: No suitable printers");
        }

    }

    protected PrintService[] lookupPrintService(DocFlavor flavor, PrintRequestAttributeSet aset)
    {
        PrintService[] pservices = PrintServiceLookup.lookupPrintServices(flavor, aset);
        if (pservices.length > 0)
            return pservices;

        int nAttr = aset.toArray().length;
        String strflavor;
        boolean bPrint = false;
        for (int i = 0; i < m_aStrFlavor.length; i++)
        {
            if (bPrint)
                break;
            strflavor = m_aStrFlavor[i];
            flavor = null;
            if (strflavor.equals("jpeg"))
                flavor = DocFlavor.INPUT_STREAM.JPEG;
            else if (strflavor.equals("gif"))
                flavor = DocFlavor.INPUT_STREAM.GIF;
            else if (strflavor.equals("postscript"))
                flavor = DocFlavor.INPUT_STREAM.POSTSCRIPT;
            else if (strflavor.equals("pcl"))
                flavor = DocFlavor.INPUT_STREAM.PCL;
            else if (strflavor.equals("pdf"))
                flavor = DocFlavor.INPUT_STREAM.PDF;

            m_docflavor = flavor;
            pservices = PrintServiceLookup.lookupPrintServices(flavor, aset);
            if (pservices.length > 0)
                bPrint = true;
            else
            {
                for (int j = 0; j < nAttr; j++)
                {
                    Attribute[] newAttr = aset.toArray();
                    if (newAttr != null && newAttr.length > 0)
                    {
                        Class category = newAttr[0].getCategory();
                        aset.remove(category);
                        //System.out.println("compromising attribute " + category.getName());
                    }
                    pservices = PrintServiceLookup.lookupPrintServices(flavor, aset);
                    if (pservices.length > 0)
                        bPrint = true;
                }
            }
        }
        return pservices;
    }

    protected String getFile(DocFlavor flavor, String strFile)
    {
        if (!m_docflavor.equals(flavor))
        {
            String strdocflavor = m_docflavor.toString();
            for (int i = 0; i < m_aStrFlavor.length; i++)
            {
                String strflavor = m_aStrFlavor[i];
                if (strdocflavor.indexOf(strflavor) >= 0)
                {
                    strdocflavor = strflavor;
                    if (strdocflavor.equals("postscript"))
                        strdocflavor = "ps";
                    String strpath = strFile+".bak";
                    String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, FileUtil.SYS_VNMR+"/bin/convert " +
                                    strFile + " " + strdocflavor + ":" + strpath};
                    WUtil.runScript(cmd);
                    strFile = strpath;
                    break;
                }
            }
        }
        return strFile;
    }

    protected PrintRequestAttributeSet setRequest(String strLayout)
    {
        aset = new HashPrintRequestAttributeSet();

        if(strLayout != null ) {
            if (strLayout.equalsIgnoreCase( "landscape")) {
                aset.add(OrientationRequested.LANDSCAPE);
            }
            if(strLayout.equalsIgnoreCase( "Portrait")) {
                aset.add(OrientationRequested.PORTRAIT);
            }
        }
        else
            aset.add(OrientationRequested.PORTRAIT);

        //aset.add(new JobName("VnmrJ Printing",null));
        return aset;
    }

    class VPrintJobAdapter extends PrintJobAdapter
    {

        protected String m_strfile;

        public VPrintJobAdapter(String strfile)
        {
            super();
            m_strfile = strfile;
        }

        public void printJobCompleted(PrintJobEvent e)
        {
            Messages.postInfo("Printing...Done");
            File objfile = new File(m_strfile);
            objfile.delete();
        }

        public void printJobFailed(PrintJobEvent e)
        {
            Messages.postWarning("Printing...Failed");
        }

        public void printJobCancelled(PrintJobEvent e)
        {
            Messages.postInfo("Printing...Cancelled");
        }

        public void printJobRequiresAttention(PrintJobEvent e)
        {
            Messages.postError("Printing...Printer error ");
        }
    }

}

class InputStreamDoc implements Doc {
    private String filename;
    private DocFlavor docFlavor;
    private InputStream stream;

    public InputStreamDoc(String name, DocFlavor flavor) {
        filename = name;
        docFlavor = flavor;
    }

    public DocFlavor getDocFlavor() {
        return docFlavor;
    }

    /* No attributes attached to this Doc - mainly useful for MultiDoc */
    public DocAttributeSet getAttributes() {
        return null;
    }

    /* Since the data is to be supplied as an InputStream delegate to
    * getStreamForBytes().
    */
    public Object getPrintData() throws IOException {
        return getStreamForBytes();
    }

    /* Not possible to return a GIF as text */
    public Reader getReaderForText()
        throws UnsupportedEncodingException, IOException {
       return null;
    }

    /* Return the print data as an InputStream.
    * Always return the same instance.
    */
    public InputStream getStreamForBytes() throws IOException {
        synchronized(this) {
            if (stream == null) {
                stream = new FileInputStream(filename);
            }
            return stream;
        }
    }
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;
import javax.swing.table.*;

import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 *
 *
 *
 */

public class DFFileTableModel extends DefaultTableModel
{

    protected String[] m_aStrHeader = {
            vnmr.util.Util.getLabel("_admin_Directory","Directory"), 
            vnmr.util.Util.getLabel("_admin_Size","Size")+" (kb)", 
            vnmr.util.Util.getLabel("_admin_Used","Used")+" (kb)", 
            vnmr.util.Util.getLabel("_admin_Available","Available")+" (kb)", 
            vnmr.util.Util.getLabel("_admin_Capacity","Capacity")+" (%)"
            };

    protected Vector m_vecData = new Vector();

    public DFFileTableModel()
    {
        super();
        setColumnIdentifiers(m_aStrHeader);
        initVectors();
        setDataVector(m_vecData, convertToVector(m_aStrHeader));
    }

    public void resetHeader()
    {
        setColumnIdentifiers(m_aStrHeader);
    }

    protected void initVectors()
    {
        String line = null;
        String strDir = "";
        ArrayList aListDir = new ArrayList();
        Process prcs = null;
        try
        {
            String strSpace = "df -k";
            if (Util.islinux())
                strSpace = "df -P";
            boolean bwindows = Util.iswindows();
            if (bwindows)
                strSpace = "df -kPw";
            String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, strSpace };
            Runtime rt = Runtime.getRuntime();
            prcs = rt.exec(cmd);

            InputStream istrm = prcs.getInputStream();
            BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));
            int nRow = 0;

            while((line = bfr.readLine()) != null)
            {
                //System.out.println( line );
                Vector vecData = new Vector();
                ArrayList aListData = WUtil.strToAList(line);
                if (aListData == null || aListData.isEmpty() ||
                        (nRow == 0 && !aListData.get(0).equals("Filesystem")))
                    continue;

                // Name is the first column
                int nLength = aListData.size();
                String strValue = (String)aListData.get(nLength-1);
                if (bwindows)
                    strValue = UtilB.unixPathToWindows(strValue);
                vecData.add(strValue);

                for (int i = 0; i < nLength; i++)
                {
                    if (nRow == 0)
                        continue;

                    strValue = (String)aListData.get(i);
                    StringBuffer sbValue = new StringBuffer(strValue);
                    int nIndex = sbValue.indexOf("%");
                    if (nIndex >= 0)
                    {
                        sbValue.deleteCharAt(nIndex);
                        strValue = sbValue.toString();
                    }

                    // the first column is the filesystem, don't add this
                    // the fifth column is the name of the directory, added
                    // this already.
                    if (((i != 0 && nLength > 5) || nLength <= 5) &&
                        i != nLength-1)
                    {
                        try
                        {
                            Integer n1 = new Integer(strValue);
                            vecData.add(n1);
                        }
                        catch (Exception e) { }
                    }
                }

                // the first line is the tags such as "file system",
                // don't add these since the column headers are already added.
                if (nRow != 0)
                    m_vecData.add(vecData);
                nRow++;
            }
        }
        catch (IOException e)
        {
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

    }
}


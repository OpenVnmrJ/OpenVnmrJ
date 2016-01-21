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
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;

import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class WInvestigator extends ModalEntryDialog
{

    public static  String Investigator;

    public WInvestigator(String strhelpfile)
    {
        super(vnmr.util.Util.getLabel("_admin_Investigator_List"), 
                vnmr.util.Util.getLabel("_admin_Investigator_Names:"), 
                strhelpfile);

        // Fill Investigator for use below
        String dir = FileUtil.vnmrDir(FileUtil.SYS_VNMR, "IMAGING");
        Investigator = FileUtil.vnmrDir(dir, "CHOICEFILES") + File.separator + "pis";
    }

    public void setVisible(boolean bShow)
    {
        if (bShow)
        {
            inputText.setText("");
            dolayout();
            setLocationRelativeTo(getParent());
        }
        super.setVisible(bShow);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd.equals("ok"))
        {
            saveInvesti();
            setVisible(false);
        }
        super.actionPerformed(e);
    }

    protected void dolayout()
    {
        
        String strPath = FileUtil.openPath(Investigator);
        StringBuffer sbData = new StringBuffer();
        if (strPath == null)
            return;

        BufferedReader reader = WFileUtil.openReadFile(strPath);
        if (reader == null)
            return;

        String strLine;
        try
        {
            while ((strLine = reader.readLine()) != null)
            {
                QuotedStringTokenizer strTok = 
                    new QuotedStringTokenizer(strLine, " ");
                if (strTok.hasMoreTokens()) {
                    String str = strTok.nextToken().trim();
                    // Put quotes in in case any items are mutiple words
                    sbData.append("\"" + str + "\"").append("  ");
                }
            }
            reader.close();
            inputText.setText(sbData.toString());
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }
    }

    public void saveInvesti()
    {
        String strValue = inputText.getText();
        String strPath = FileUtil.openPath(Investigator);
        if (strPath == null)
            strPath = FileUtil.savePath(Investigator);

        if (strPath == null)
        {
            Messages.logError("File " + Investigator + " not found, please check for write permission");
            return;
        }

        try
        {
            StringBuffer sbData = new StringBuffer();
            QuotedStringTokenizer strTok = 
                new QuotedStringTokenizer(strValue, ", \t\n");
            while (strTok.hasMoreTokens())
            {
                String strUser = strTok.nextToken().trim();
                sbData.append("\"").append(strUser).append("\"").append("  ");
                sbData.append("\"").append(strUser).append("\"").append("\n");
            }

            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbData);
        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
        }

    }


}

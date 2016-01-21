/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.awt.image.*;
// import com.sun.image.codec.jpeg.*;

import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.util.*;

/**
 * <p>Title: VCaptureButton </p>
 * <p>Description: Captures the image of the parent frame or the dialog to a jpeg file. </p>
 * <p>Copyright: Copyright (c) 2002</p>
 */

public class VCaptureButton extends VButton
{

    /** Indicates if the screen capture is in progess. */
    protected boolean   m_bInProgress = false;

    /** The parent frame of this button (this could be a jframe or a jdialog) */
    protected Object    m_frame = null;

    /** The directory where the image file should be saved. */
    protected String    m_strDir = "";

    /** The prefix of the filename, all the filenames would begin with this name. */
    protected String    m_strPrefix = "";

    /** The type of the action i.e. save or print.  */
    protected String    m_strType = "";

    private final static String[] m_types = {"Save", "Print" };

    public VCaptureButton(SessionShare sshare, ButtonIF vif, String typ)
    {
        super(sshare, vif, typ);
    }


    /** The array of the attributes that are displayed in the edit template.*/
    private final static Object[][] attributes = {
    {new Integer(LABEL), 	"Label of item:"},
    {new Integer(ICON), 	"Icon of item:"},
    {new Integer(VARIABLE),	"Vnmr variables:    "},
    {new Integer(SHOW),	"Enable condition:"},
    {new Integer(CMD),      "Vnmr command:"},
    {new Integer(STATPAR),	"Status parameter:"},
    {new Integer(STATSHOW),	"Enable status values:"},
    {new Integer(PANEL_FILE),	"Directory:"},
    {new Integer(PREFIX),	"File Name Prefix:"},
    {new Integer(PANEL_TYPE),	"Action:","radio",m_types}
    };

    public Object[][] getAttributes()  { return attributes; }


    public void actionPerformed(ActionEvent e)
    {
        if (inModalMode || inEditMode || vnmrIf == null || isActive < 0)
            return;

        // get the parent frame where this button resides,
        // this is either a jdialog or a jframe.
        Object objParent = getFrame();
        if (objParent == null)
            return;

        boolean bDialog = (objParent instanceof JDialog) ? true : false;
        Dimension dimCanvas = bDialog ? ((JDialog)objParent).getSize()
                                    : ((JFrame)objParent).getSize();
        Point pointCanvas = bDialog ? ((JDialog)objParent).getLocationOnScreen()
                                    : ((JFrame)objParent).getLocationOnScreen();

        if (m_strType != null && m_strType.equals(m_types[1]))
        {
            CaptureImage.doPrint(dimCanvas, pointCanvas);
        }
        else
        {
            doCapture(dimCanvas, pointCanvas);
        }
    }

    public String getAttribute(int nAttr)
    {
        switch (nAttr)
        {
            case PANEL_FILE:
                return m_strDir;
            case PREFIX:
                return m_strPrefix;
            case PANEL_TYPE:
                return m_strType;
            default:
                return super.getAttribute(nAttr);
        }
    }

    public void setAttribute(int nAttr, String strValue)
    {
        switch (nAttr)
        {
            case PANEL_FILE:
                strValue = (strValue == null) ? "" : strValue;
                m_strDir = strValue;
                break;
            case PREFIX:
                strValue = (strValue == null) ? "" : strValue;
                m_strPrefix = strValue;
                break;
            case PANEL_TYPE:
                m_strType = strValue;
                break;
            default:
                super.setAttribute(nAttr, strValue);
                break;
        }
    }

    protected Object getFrame()
    {
        if (m_frame == null)
        {
            Container container = getParent();
            while (container != null)
            {
                if (container instanceof JFrame ||
                        container instanceof JDialog)
                {
                    m_frame = container;
                    break;
                }
                container = container.getParent();
            }
        }
        return m_frame;
    }

    /**
     *  Captures the screen of the given dimension and location to the file.
     *  The file is named as follows:
     *  m_strDir/m_strPrefixDate.jpeg
     *  The m_strDir is the given dir, the default is vnmrsys,
     *  the m_strPrefix is the given prefix for the file e.g. imgFile
     *  Date is the current date and time stamp that's added to the filename.
     *
     *  @param dim  the dimension of the screen object to be captured.
     *  @param point    the location on the screen.
     */
    protected void doCapture(final Dimension dim, final Point point)
    {
        if (m_bInProgress)
            return;

        final String strPath = FileUtil.openPath(m_strDir);
        if (strPath == null)
        {
            Messages.postDebug("File not found " + m_strDir);
            return;
        }

        new Thread(new Runnable()
        {
            public void run()
            {
                m_bInProgress = true;
                String strDate = WUtil.getDate();
                String strFile = strPath+File.separator+m_strPrefix+strDate+".jpeg";
                CaptureImage.doCapture(strFile, dim, point);
                m_bInProgress = false;
                Messages.postInfo("Captured Image to " + strFile);
            }
        }).start();
    }


}

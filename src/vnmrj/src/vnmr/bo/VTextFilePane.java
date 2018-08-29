/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
import javax.swing.text.StyledEditorKit;
import javax.swing.text.html.HTMLEditorKit;

import java.io.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VTextFilePane extends VScrollTextPane implements VEditIF {

    private FocusAdapter fl;
    private String m_filePath = null;
    private int m_fileSize = 0;
    private String m_fileContents = "";
    private boolean m_plainEditorSet = false;
    private boolean m_htmlEditorSet = false;

    public VTextFilePane(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);

        fl = new FocusAdapter() {
              public void focusLost(FocusEvent evt) {
                  entryAction();
             }

             public void focusGained(FocusEvent e)
             {
                if (!twin.isEditable())
                    transferFocus();
             }
        };
        twin.setText("");
        twin.addFocusListener(fl);
    }

    public void updateValue() {
        String setVal = getAttribute(SETVAL);
        String showVal = getAttribute(SHOW);
        if (getVnmrIF() == null)
            return;
        if (setVal != null) {
            getVnmrIF().asyncQueryParam(this, setVal);
        }
        if (twin.showVal != null) {
        }
    }

    public void setValue(ParamIF pf) {
        if (pf != null) {
            boolean newFile = (m_filePath == null
                               || !m_filePath.equals(pf.value));
            m_filePath = pf.value;
            BufferedReader in;
            boolean isHtml = gettype(m_filePath);
            //int viewTop = -twin.getY();
            //int viewHeight = twin.getParent().getHeight();
            //int viewBottom = viewTop + viewHeight;
            //int textBottom = twin.getHeight();
            //int textBop = twin.getParent().getY();
            /*System.out.println("viewTop=" + viewTop
                               + ", viewHeight=" + viewHeight
                               + ", viewBottom=" + viewBottom
                               + ", textBottom=" + textBottom);/*CMP*/
            //showTop |= viewBottom < textBottom;

            try {
                in = new BufferedReader(new FileReader(m_filePath));
            } catch(FileNotFoundException e2) {
                twin.setText("Cannot read LC log file: " + m_filePath);
                return;
            }
            try {
                if (isHtml) {
                    if (!m_htmlEditorSet) {
                        m_htmlEditorSet = true;
                        twin.setEditorKitForContentType("text/html",
                                                        new HTMLEditorKit());
                    }
                    twin.setContentType("text/html");
                } else {
                    if (!m_plainEditorSet) {
                        m_plainEditorSet = true;
                        twin.setEditorKitForContentType("text/plain",
                                                        new StyledEditorKit());
                    }
                    twin.setContentType("text/plain");
                }
                //twin.read(in, null);
                String strLine;
                StringBuffer sbData = new StringBuffer();
                while ((strLine = in.readLine()) != null)
                {
                    sbData.append(strLine).append("\n");
                }
                String sData = sbData.toString();
                if (newFile || !m_fileContents.equals(sData)) {
                    m_fileContents = sData;
                    twin.setText(sData);
                    boolean fileGrew = !newFile && sData.length() > m_fileSize;
                    m_fileSize = sData.length();
                    if (newFile || !fileGrew) {
                        // This scrolls to top of window
                        twin.setCaretPosition(0);
                    }
                }
                /*
                 * Various attempts to scroll that don't work so hot:
                 *
                twin.setBounds(new Rectangle(0, -90, twin.getWidth(),
                                             twin.getHeight()));
                ((JViewport)twin.getParent())
                        .setViewPosition(new Point(0, 100));
                System.out.println("Width=" + twin.getParent().getWidth()
                        + ", Height=" + twin.getParent().getHeight());
                ((JViewport)twin.getParent())
                        .scrollRectToVisible
                        (new Rectangle(0, 90, twin.getParent().getWidth(),
                                       twin.getParent().getHeight()));
                */
            } catch(IOException ee) {
            } catch (Exception e) {
                //Messages.writeStackTrace(e);
                twin.setText("Error loading LC log file: " + m_filePath);
            }

            try {
                in.close();
            } catch(IOException ee) { }
        }
    }

    protected boolean gettype(String strPath)
    {
        boolean isHtml = false;
        try
        {
            BufferedReader reader = new BufferedReader(new FileReader(strPath));
            String strLine;

            while ((strLine = reader.readLine()) != null)
            {
                strLine = strLine.trim();
                if (strLine.equals("") || strLine.startsWith("<!"))
                    continue;
                strLine = strLine.toLowerCase();
                if (strLine.startsWith("<html>"))
                {
                    isHtml = true;
                    break;
                }
            }
            reader.close();
        } catch (FileNotFoundException fnfe) {
            Messages.postDebug("VTextFilePane.gettype: File not found: "
                    + strPath);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return isHtml;
    }

    private void entryAction() {
        if (!twin.isEditable() || m_filePath == null
            || m_filePath.trim().length() == 0)
        {
            return;             // Don't write file if text not editable!
        }
        FileWriter out;
        String text = twin.getText();
        try {
            out = new FileWriter(m_filePath);
            out.write(text);
            if (!text.endsWith("\n")) {
                // Vnmrbg has trouble with unterminated lines
                out.write("\n");
            }
            out.close();
        } catch(IOException e) {}
    }

    public Object[][] getAttributes() { return attributes; }

    private final static String[] yes_no = {"yes", "no"};
    private final static Object[][] attributes = {
        {new Integer(VARIABLE), "File name variables:"},
        {new Integer(SETVAL),   "Value of file path:"},
        {new Integer(CMD),      "Vnmr command:"},
        {new Integer(SHOW),     "Enable condition:"},
        {new Integer(EDITABLE), "Editable:", "radio", yes_no},
        {new Integer(WRAP),     "Wrap lines:", "radio", yes_no},
    };
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
}

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
import java.io.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VTextFileWin extends VText implements VEditIF {

    private FocusAdapter fl;
    private String fileName;
    protected String value;
    private boolean  bAlphatext = false;

    public VTextFileWin(SessionShare sshare, ButtonIF vif, String typ) {
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
        twin.removeFocusListener(twin.getFocusListener());
        twin.addFocusListener(fl);
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
        case VARIABLE:
            boolean bAlpha = false;
            if (c != null) {
                StringTokenizer tok = new StringTokenizer(c, " ,\n");
                while (tok.hasMoreTokens()) {
                    String  d = tok.nextToken();
                    if (d.equals("alphatext")) {
                        bAlpha = true;
                        break;
                    }
                }
            }
            if (bAlpha || bAlphatext) {
                ButtonIF b = getVnmrIF();
                if (b != null) {
                    ExpPanel exp = (ExpPanel) b;
                    if (bAlpha)
                        exp.addAlphatextListener(this);
                    else if (bAlphatext)
                        exp.removeAlphatextListener(this);
                }
                bAlphatext = bAlpha;
            }
            break;
        }
        super.setAttribute(attr, c);
    }

    public void updateValue() {
        String setVal = getAttribute(SETVAL);
        String showVal = getAttribute(SHOW);
        if (getVnmrIF() == null)
            return;
        if (setVal != null) {
            // We sometime need to be called here even from Admin panel
            // There is no vnmrbg in admin, so we cannot make a call to
            // resolve parameters.  If admin, assume we have a file path
            // already in the form "$VALUE=path"and just go on with setValue.  Note, ExpSelector
            // just happens to have a static function to see if we are
            // in admin.
            if(ExpSelector.isAdmin()) {
                // Remove the "$VALUE=" if any
                if(setVal.startsWith("$VALUE=")) 
                    setVal = setVal.substring(7);
                // Strip off any single quotes if any
                if(setVal.startsWith("\'"))
                    setVal = setVal.substring(1);
                if(setVal.endsWith("\'"))
                    setVal = setVal.substring(0, setVal.length() -1);
                // Just set this value
                setValue(setVal);
            }
            else
                getVnmrIF().asyncQueryParam(this, setVal);
        }
        if (twin.showVal != null) {
        }
    }

    public void setValue(String f) {
        if (f.length() < 1)
            return;
        fileName = f;
        if (Util.iswindows())
            fileName = UtilB.unixPathToWindows(fileName);
        String filePath = FileUtil.openPath(fileName);
        if (filePath == null) {
            value = "";
            twin.setText("");
            return;
        }
        BufferedReader in;
        try {
            in = new BufferedReader(new FileReader(filePath));
        } catch(FileNotFoundException e2) {
            value = "";
            twin.setText("");
            return;
        }
        try { 
            twin.read(in, null);
            value = twin.getText();
        } catch(IOException ee) { }

        try {
            in.close();
        } catch(IOException ee) { }
        /*
          String line;
          twin.setText("");
          try {
          while ((line=in.readLine()) != null) {
          twin.append(line);
          twin.append("\n");
          }
          in.close();
          } catch(IOException e) {}
        **/
    }

    public void setValue(ParamIF pf) {
        if (pf != null )
            setValue(pf.value);
    }

    private void entryAction() {
        if (!twin.isEditable() || fileName == null) {
            return;		// Don't write file if text not editable!
        }
        if (Util.iswindows())
            fileName = UtilB.unixPathToWindows(fileName);
        String filePath = FileUtil.savePath(fileName);
        if (filePath == null) {
            return;
        }
        FileWriter out;
        String text = twin.getText();
        if (text == null)
            text = "";
        if (!text.equals(value))
            {
                try {
                    out = new FileWriter(filePath);
                    out.write(text);
                    if (!text.endsWith("\n")) {
                        // Vnmrbg has trouble with unterminated lines
                        out.write("\n");
                    }
                    out.close();
                    value = text;
                } catch(IOException e) {}
            }
    }

    public Object[][] getAttributes() { return attributes; }

    private final static String[] yes_no = {"yes", "no"};
    private final static Object[][] attributes = {
        {new Integer(VARIABLE),	"File name variables:"},
        {new Integer(SETVAL),	"Value of file path:"},
        {new Integer(CMD),	"Vnmr command:"},
        {new Integer(SHOW),	"Enable condition:"},
        {new Integer(EDITABLE),	"Editable:", "radio", yes_no},
        {new Integer(WRAP),	"Wrap lines:", "radio", yes_no},
    };
    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}
}

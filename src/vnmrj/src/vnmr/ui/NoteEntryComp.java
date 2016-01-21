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
import java.util.StringTokenizer;
import javax.swing.*;
import javax.swing.border.*;

import vnmr.bo.VObjDef;
import vnmr.bo.VTextFileWin;
import vnmr.ui.ExpPanel;
import vnmr.util.Util;

/**
 *
 * @author Mark Cao
 */
public class NoteEntryComp extends JScrollPane {
    /**
     * constructor
     */
    VTextFileWin vtw;

    public NoteEntryComp( SessionShare sshare ) {
	setBorder(BorderFactory.createBevelBorder(BevelBorder.LOWERED));
        setMinimumSize( new Dimension( 0,0 ) );
        Util.setNoteEntryComp(this);

        ExpPanel ep = (ExpPanel)Util.getDefaultExp();
        String type = new String(" ");
        vtw = new VTextFileWin(sshare, ep, type);
	vtw.setAttribute(VObjDef.FONT_NAME,"PlainText");
	vtw.setAttribute(VObjDef.FONT_SIZE,"PlainText");
	vtw.setAttribute(VObjDef.FONT_STYLE,"PlainText");
	vtw.setAttribute(VObjDef.FGCOLOR,"PlainText");
        vtw.setAttribute(VObjDef.VARIABLE,"curexp");
        vtw.setAttribute(VObjDef.EDITABLE,"yes");
        vtw.setAttribute(VObjDef.WRAP,"yes");
        setViewportView(vtw);
System.out.println("NoteEntryComp");
    }

    public void setText(String fln) {
        vtw.setAttribute(VObjDef.SETVAL,"$VALUE=curexp+`/"+fln+"`");
        vtw.updateValue();
    }

    public void processCommand(String str) {
	StringTokenizer tok = new StringTokenizer(str," ,\n");
        String cmd = tok.nextToken();
        if (cmd.equals("update")) {
	    String fln = tok.nextToken();
            setText(fln);
            return;
        }
        System.out.println("NoteEntryComp.processCommands(): No such cmd");
        System.out.println("       str='"+str+"'");
    }
} // class NoteEntryComp

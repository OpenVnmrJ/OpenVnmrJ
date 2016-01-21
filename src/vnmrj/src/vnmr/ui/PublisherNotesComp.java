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
import java.io.*;
import java.util.StringTokenizer;
import javax.swing.*;
import javax.swing.border.*;

import  vnmr.util.*;

/**
 * publisher notes component
 *
 */
public class PublisherNotesComp extends JScrollPane {

    JEditorPane jep;

    /**
     * constructor
     */
    public PublisherNotesComp() {
	setBackground(Global.BGCOLOR);
        setMinimumSize( new Dimension( 0,0 ) );
	Border insideBorder = BorderFactory.createLineBorder(Color.red);
	Border outsideBorder = BorderFactory.createEmptyBorder(1, 1, 1, 1);
	setViewportBorder(BorderFactory.
                           createCompoundBorder(outsideBorder,insideBorder));
        jep = new JEditorPane();
	jep.setEditable(false);
        try {
	    jep.setPage("file:/vnmr/bootup_message");
        } catch(IOException ioe) { }
        setViewportView(jep);

        Util.setPublisherNotesComp(this);
    } // PublisherNotesComp()

    public PublisherNotesComp( SessionShare sshare ) {
        this();		/* just to be compatible with VToolPanel */
    }

    public void setText(String fln) {
        try {
	    String path=FileUtil.openPath("MANUAL/"+fln);
	    jep.setPage("file:"+path);
        } catch(IOException ioe) { 
           System.out.println("Publisher: file not found '"+fln+"'");
        }
    }

    public void processCommand(String str) {
	StringTokenizer tok = new StringTokenizer(str," ,\n");
        String cmd = tok.nextToken();
        if (cmd.equals("update")) {
	    setText(tok.nextToken());
            return;
        }
        System.out.println("PublisherNotesComp.processCommands(): No such cmd");
        System.out.println("       str='"+str+"'");
    }
} // class PublisherNotesComp

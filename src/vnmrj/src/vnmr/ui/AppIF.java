/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;
import javax.swing.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.ui.shuf.*;

/**
 * An AppIF object stores the "session share" object, and is the abstract
 * base for top-level user-interface components.
 *
 */
public abstract class AppIF extends JLayeredPane {
    /** session share */
    protected SessionShare sshare = null;

    /**
     * constructor
     * @param appFrame application Frame
     * @param user user
     */
    public AppIF(AppInstaller appInstaller, User user) {
	sshare = new SessionShare(appInstaller, user);
    } // AppIF()

    public AppIF() {
    }

    /**
     * get session share
     * @return session share
     */
    public SessionShare getSessionShare() {
	return sshare;
    } // getSessionShare()


    public void notifyLogout() { }

    public void closeUI() { }

    public void suspendUI() { }

    public void resumeUI() { }

    public void sendCmdToVnmr(String data) { }

    public void sendToVnmr(String data) { }

    public void sendToAllVnmr(String data) { }

    public void exitVnmr() {}

    public void windowClosing() {}

    public void initLayout() {}
    public void initLayout(JFrame f) {}

    public void switchLayout(int k) {}

    public void enableResize() {}

    public void disableResize() {}

    public void openUiLayout(String name) {}

    public void openUiLayout(String name, int vpNum) {}

    public void saveUiLayout(String name) {}

    public void showSmsPanel(boolean b, boolean mode) {}

    public void showJMolPanel(boolean b) {}
    public void showJMolPanel(int id, boolean b) {}

    public ParameterPanel getParamPanel(String p) {
	return null;
    }

    public ParamIF syncQueryParam(String p) {
	return null;
    }

    public ParamIF syncQueryVnmr(int type, String p) {
	return null;
    }

    public ParamIF syncQueryExpr(int type, String p) {
	return null;
    }

    public void setInputFocus() {
	if(commandArea != null)
	   commandArea.setFocus();
    }

    public void setInputFocus(String s) { }

    public void setInputPrompt(int vpId, String m) {
	// if (commandArea != null)
	//    commandArea.setPrompt(m);
    }

    public void setInputData(String m) {
	if (commandArea != null)
	    commandArea.setOutput(m);
    }

    public void processXKeyEvent(KeyEvent e) {
        if (commandArea != null)
            commandArea.processXKeyEvent(e);
    }

    public void appendMessage(String m) {
	if (messageArea != null)
	    messageArea.append(m);
    }

    public void clearMessage() {
	if (messageArea != null)
	    messageArea.clear();
    }

    public void setMessageColor(Color c) {
	if (messageArea != null)
	    messageArea.setForeground(c);
    }

    public void setButtonBar(SubButtonPanel b) {
	if (displayPalette != null)
		displayPalette.setToolBar(b);
    }

    public DisplayPalette getCsiButtonPalette() {
        return null;
    }

    public void setCsiButtonBar(SubButtonPanel b) { }

    public void showCsiButtonPalette(boolean b) { }

    public boolean isResizing() {
        return false;
    }

    public boolean inVpAction() {
        return false;
    }

    public int getVpId() {
        return 0;
    }

    public void setVpLayout(boolean b) { }

    public boolean setViewPort(int vpNum) { return true; }

    public boolean setViewPort(int vpNum, int status) { return true; }

    public boolean setSmallLarge(int vpNum) { return true; }

    public void setMenuBar(JComponent b) { }

    public void setToolBar(JComponent b) { }

    public void showImagePanel(boolean bShow, String imageFile) { }

    public void processMousePressed(MouseEvent e) {}
    public void processMouseReleased(MouseEvent e) {}

//    public HoldingArea 	   holdingArea;
    public VTabbedToolPanel      vToolPanel = null;
//    public Shuffler    	   shuffler;
    public DisplayPalette  displayPalette;
    public ExpViewArea     expViewArea;
    public ControlPanel    controlPanel;
    public JComponent      paramPanel;
    public ExpStatusBar    statusBar;
    public PulseTool       pulseTool;
/*
    public VjMenuBar       topMenuBar;
*/
    public MenuPanel       topMenuBar;
    public VjToolBar       topToolBar;
    public CommandInput    commandArea;
    public MessageTool	   messageArea;
    public JButton         pulseHandle;

    /* Rudy's object */
//    public WalkupTopBar	   walkupTopBar;
//    public SolventSampleComp solventSampleComp;
    public NoteEntryComp   noteEntryComp;
    public RobotViewComp   robotViewComp;
//    public AutomationComp  automationComp;
//    public ExpQueueComp    expQueueComp;
    public ExpControlButton expControlButton;
    public PublisherNotesComp  publisherNotesComp;
} // class AppIF

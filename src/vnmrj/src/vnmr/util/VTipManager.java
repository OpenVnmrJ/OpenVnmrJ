/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.awt.event.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.border.*;
import java.io.*;

// This toolTip manager provides heavy weight popup only

public class VTipManager extends MouseAdapter implements MouseMotionListener  {
    javax.swing.Timer enterTimer, exitTimer, insideTimer;
    String toolTipText;
    Point  preferredLocation;
    JComponent tipComp;
    MouseEvent mouseEvent;
    boolean showImmediately;
    final static VTipManager sharedInstance = new VTipManager();
    JWindow tipWindow = new JWindow();

    boolean enabled = true;
    boolean needNewTip = false;
    private FocusListener focusChangeListener = null;
    private JLabel tipLabel;
    private Dimension screenSize;
    private Point tipLoc = new Point();

    // PENDING(ges)
    protected boolean lightWeightPopupEnabled = true;
    protected boolean heavyWeightPopupEnabled = false;

    VTipManager() {

        enterTimer = new javax.swing.Timer(1500, new insideTimerAction());
        enterTimer.setRepeats(false);
        exitTimer = new javax.swing.Timer(500, new outsideTimerAction());
        exitTimer.setRepeats(false);
        insideTimer = new javax.swing.Timer(4000, new stillInsideTimerAction());
        insideTimer.setRepeats(false);

        screenSize = Toolkit.getDefaultToolkit().getScreenSize();
    	tipWindow = new JWindow();
    	tipLoc = new Point(0, 0);

        tipLabel = new JLabel(" tip ");
	tipWindow.getContentPane().add(tipLabel, BorderLayout.CENTER);
	tipWindow.getContentPane().setBackground(new Color(250, 240, 230));
	tipWindow.getRootPane().setBorder(new LineBorder(Color.gray));

        // Set enabled flag as per persistence.
        readPersistence();
    }

    public void setEnabled(boolean flag) {
        enabled = flag;
        if (!flag) {
            hideTipWindow();
        }
    }

    /**
     * Returns true if this object is enabled.
     *
     * @return true if this object is enabled
     */
    public boolean isEnabled() {
        return enabled;
    }

    public void setActive(boolean s) {
        enabled = s;
        if (!s)
            hideTipWindow();
        // Write a persistence file to be read by the set cmd in the xml file
        writePersistence();
    }


    public void setLightWeightPopupEnabled(boolean aFlag){
    }

    public boolean isLightWeightPopupEnabled() {
        return false;
    }


    /**
     * Specifies the initial delay value.
     *
     * @param milliseconds  the number of milliseconds
     *        to delay (after the cursor has paused) before displaying the
     *        tooltip
     */
    public void setInitialDelay(int milliseconds) {
        enterTimer.setInitialDelay(milliseconds);
    }

    /**
     * Returns the initial delay value.
     *
     * @return an int representing the initial delay value
     */
    public int getInitialDelay() {
        return enterTimer.getInitialDelay();
    }

    /**
     * Specifies the dismisal delay value.
     *
     * @param milliseconds  the number of milliseconds
     *        to delay (after the cursor has moved on) before taking away
     *        the tooltip
     */
    public void setDismissDelay(int milliseconds) {
        insideTimer.setInitialDelay(milliseconds);
    }

    /**
     * Returns the dismisal delay value.
     *
     * @return an int representing the dismisal delay value
     * @see #setDismissDelay
     */
    public int getDismissDelay() {
        return insideTimer.getInitialDelay();
    }

    /**
     * Specifies the time to delay before reshowing the tooltip.
     *
     * @param milliseconds  the time in milliseconds
     *        to delay before reshowing the tooltip if the cursor stops again
     * @see #getReshowDelay
     */
    public void setReshowDelay(int milliseconds) {
        exitTimer.setInitialDelay(milliseconds);
    }

    /**
     * Returns the reshow delay value.
     *
     * @return an int representing the reshow delay value
     * @see #setReshowDelay
     */
    public int getReshowDelay() {
        return exitTimer.getInitialDelay();
    }

    void showTipWindow() {
        if (!enabled)
            return;
        if(tipComp == null || !tipComp.isShowing())
            return;
	if (needNewTip) {
            toolTipText = ((VTooltipIF)tipComp).getTooltip(mouseEvent);
	    tipLabel.setText(toolTipText);
	    tipLabel.validate();
	    needNewTip = false;
	}
        Point screenLocation = tipComp.getLocationOnScreen();
	int x, y;
        Dimension size = tipLabel.getPreferredSize();

        if (preferredLocation != null) {
                x = screenLocation.x + preferredLocation.x;
                y = screenLocation.y + preferredLocation.y;
        } else {
                x = screenLocation.x + mouseEvent.getX() + 15;
                y = screenLocation.y + mouseEvent.getY() + 15;

                if (x + size.width > screenSize.width) {
                    x -= size.width;
                }
                if (y + size.height > screenSize.height) {
                    y -= (size.height + 10);
                }
        }
	if (x > tipLoc.x)
		tipLoc.x = x;
	else if ((tipLoc.x - x) > 30)
		tipLoc.x = x;
	if (y > tipLoc.y)
		tipLoc.y = y;
	else if ((tipLoc.y - y) > 50)
		tipLoc.y = y;

  	tipWindow.setBounds(tipLoc.x, tipLoc.y, size.width+8, size.height+8);
	tipWindow.validate();
	if (!tipWindow.isShowing())
	       tipWindow.setVisible(true);
        insideTimer.restart();
    }

    void hideTipWindow() {
        insideTimer.stop();
	if (tipWindow.isShowing()) {
	    tipWindow.setVisible(false);
        }
	tipLoc.x = 0;
	tipLoc.y = 0;
    }

    /**
     * Returns a shared ToolTipManager instance.
     *
     * @return a shared ToolTipManager object
     */
    public static VTipManager sharedInstance() {
        return sharedInstance;
    }

    public static VTipManager tipManager() {
        return sharedInstance;
    }

    public void registerComponent(JComponent component) {
	if (tipWindow == null)
	    return;
        component.removeMouseListener(this);
        component.addMouseListener(this);
    }

    /**
     * Remove a component from tooltip control.
     *
     * @param component  a JComponent object
     */
    public void unregisterComponent(JComponent component) {
        component.removeMouseListener(this);
    }


    public void mouseEntered(MouseEvent event) {
        JComponent component = (JComponent)event.getSource();
	if (!(component instanceof VTooltipIF))
            return;

        preferredLocation = null;
        exitTimer.stop();

        if (tipComp != null) {
            enterTimer.stop();
        }

	if (component instanceof JTable) {
            component.addMouseMotionListener(this);
	    tipLoc.x = 0;
	    tipLoc.y = 0;
    	    needNewTip = true;
	    hideTipWindow();
	    showImmediately = false;
	}

        toolTipText = ((VTooltipIF)component).getTooltip(event);
	tipLabel.setText(toolTipText);
	tipLabel.validate();

        tipComp = component;
	mouseEvent = event;
        if (showImmediately) {
	    showTipWindow();
        } else {
	    enterTimer.start();
        }
    }

    // implements java.awt.event.MouseListener
    public void mouseExited(MouseEvent event) {
        enterTimer.stop();
	if (tipComp != null && (tipComp instanceof JTable)) {
	        tipComp.removeMouseMotionListener(this);
        	hideTipWindow();
        	tipComp = null;
		showImmediately = false;
	        tipLoc.x = 0;
	        tipLoc.y = 0;
		return;
	}
        tipComp = null;
        toolTipText = null;
        exitTimer.start();
    }

    // implements java.awt.event.MouseListener
    public void mousePressed(MouseEvent event) {
        enterTimer.stop();
        hideTipWindow();
        showImmediately = false;
    }

    // implements java.awt.event.MouseMotionListener
    public void mouseDragged(MouseEvent event) {
    }

    // implements java.awt.event.MouseMotionListener
    public void mouseMoved(MouseEvent event) {
        JComponent component = (JComponent)event.getSource();
        String newText = component.getToolTipText(event);
        Point  newPreferredLocation = component.getToolTipLocation(event);

        if (!tipWindow.isShowing()) {
	    enterTimer.restart();
            mouseEvent = event;
	    needNewTip = true;
	    return;
	}
        if (newText != null || newPreferredLocation != null) {
            mouseEvent = event;
            if (((newText != null && newText.equals(toolTipText)) || newText == null) &&
                ((newPreferredLocation != null && newPreferredLocation.equals(preferredLocation))
                 || newPreferredLocation == null)) {
                if (tipWindow.isShowing()) {
                    insideTimer.start();
		}
                else {
                    enterTimer.restart();
                }
            } else {
                toolTipText = newText;
		tipLabel.setText(toolTipText);
                preferredLocation = newPreferredLocation;
                if (showImmediately) {
                    showTipWindow();
                } else {
                    enterTimer.restart();
                }
            }
        } else {
            toolTipText = null;
            preferredLocation = null;
            hideTipWindow();
            enterTimer.stop();
            exitTimer.start();
        }
    }

    protected void writePersistence() {
	String filepath;
        FileWriter fw;
        PrintWriter os;

	filepath = FileUtil.savePath("USER/PERSISTENCE/ToolTip");
        if (filepath == null) {
            return;
        }

	try {
              fw = new FileWriter(filepath);
              os = new PrintWriter(fw);
              if(enabled)
                  os.println("tooltip on");
              else
                  os.println("tooltip off");

              os.close();
	}
	catch (Exception e) {
            Messages.postError("Problem writing " + filepath);
            Messages.writeStackTrace(e);
	}
    }

    protected void readPersistence() {
        String filepath;
        BufferedReader in;
        String line;
        String value;
        StringTokenizer tok;
        boolean flag;


        filepath = FileUtil.openPath("USER/PERSISTENCE/ToolTip");
        if(filepath==null) {
            return;
        }
        try {
            in = new BufferedReader(new FileReader(filepath));
            line = in.readLine();
            if(!line.startsWith("tooltip")) {
                in.close();
                File file = new File(filepath);
                // Remove the corrupted file.
                file.delete();
                value = "on";
            }
            else {
                tok = new StringTokenizer(line, " \t\n");
                value = tok.nextToken();
                if (tok.hasMoreTokens()) {
                    value = tok.nextToken();
                }
                else
                    value = "on";
                in.close();
            }
        }
        catch (IOException e) {
            // No error output here.
            value = "on";
        }

        if(value.equals("on"))
            flag = true;
        else
            flag = false;

        tipWindow.setVisible(true);

        setEnabled(flag);
    }

    static public void deletePersistence() {
	String filepath;
        File file;

	filepath = FileUtil.savePath("USER/PERSISTENCE/ToolTip");
        if (filepath != null) {
            file = new File(filepath);
            if (file.exists())
               file.delete();
        }

    }

    protected class insideTimerAction implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            if(tipComp != null && tipComp.isShowing()) {
                showImmediately = true;
                showTipWindow();
            }
        }
    }

    protected class outsideTimerAction implements ActionListener {
        public void actionPerformed(ActionEvent e) {
	    enterTimer.stop();
            showImmediately = false;
            hideTipWindow();
        }
    }

    protected class stillInsideTimerAction implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            enterTimer.stop();
            showImmediately = false;
            hideTipWindow();
        }
    }
}

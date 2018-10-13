/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.util;

import java.awt.Component;
import java.awt.KeyboardFocusManager;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowFocusListener;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;

import javax.swing.Timer;


/**
 * This class implements a workaround for a Java/Linux bug where
 * keyboard events are sometimes not sent to the widget with
 * the focus. This seems to be a problem on Linux only (not Windows).
 * The bug occurs most often when the focus moves to and from a pop-up
 * window.
 *
 * If you have the focus on a text field in one window, then switch focus
 * to another Java window, then return to the first window: the text field
 * again has the focus (you get the blinking cursor), but you cannot type
 * into the field. Sometimes just clicking in the field restores keyboard
 * focus; sometimes you need to select another widget in the same window
 * and then reselect the original text field.
 *
 * It happens consistently when the conditions are repeated exactly, but
 * seems to depend on the details of what is in the windows and/or how
 * they are brought up.
 */
public class LinuxFocusBug
    implements WindowFocusListener, PropertyChangeListener {

    protected boolean m_windowJustGotFocus = false;

    private LinuxFocusBug(Window window) {
        window.addWindowFocusListener(this);
        KeyboardFocusManager focusManager =
            KeyboardFocusManager.getCurrentKeyboardFocusManager();
        focusManager.addPropertyChangeListener(this);
    }

    /**
     * Work around the lost-keyboard-focus bug in a given window.
     * @param window The window.
     */
    static public void workAroundFocusBugInWindow(Window window) {
        if (!DebugOutput.isSetFor("NoFocusBugFix")) {
            new LinuxFocusBug(window);
        }
    }

    @Override
    public void windowGainedFocus(WindowEvent e) {
        m_windowJustGotFocus = true;
        //Messages.postDebug("Window got focus");
    }

    @Override
    public void windowLostFocus(WindowEvent e) {
    }

    @Override
    /**
     * If our window just gained focus, and we get notified that a widget
     * within the window has gained the focus, force that widget to be
     * given the keyboard input.
     */
    public void propertyChange(PropertyChangeEvent e) {
        if (DebugOutput.isSetFor("NoFocusBugFix")) {
            return;
        }
        if (DebugOutput.isSetFor("LinuxFocusBug+")) {
            Object old = e.getOldValue();
            String sold = old == null ? "null"
                    : old.toString().replaceFirst("\\[.*", "");
            Object onew = e.getNewValue();
            String snew = onew == null ? "null"
                    : onew.toString().replaceFirst("\\[.*", "");
            Messages.postDebug(e.getPropertyName() + ": "
                               + sold
                               + " --> "
                               + snew);
        }
        if (m_windowJustGotFocus ) {
            if ("focusOwner".equals(e.getPropertyName())) {
                Object src = e.getNewValue();
                if (src instanceof Component) {
                    final Component comp = (Component)src;
                    Messages.postDebug("LinuxFocusBug",
                                       "Transferring focus to " + comp);
                    m_windowJustGotFocus = false;

                    // NB: Grabbing the focus immediately seems to be a
                    // problem on some systems. It seems to keep a button click
                    // from being effective if clicking the button brought the
                    // focus to this window. So we delay a little before
                    // grabbing the focus.
                    int delay = 250; // ms
                    ActionListener focuser = new ActionListener() {
                        public void actionPerformed(ActionEvent e) {
                            forceKeyboardFocus(comp);
                        }
                    };
                    Timer timer = new Timer(delay, focuser);
                    timer.setRepeats(false);
                    timer.start();
                }
            }
        }
    }

    /**
     * Force keyboard focus to the given component, but only if it already
     * has the focus (and the action, therefore, seems pointless -- but on
     * some buggy Linuxes a component may have the focus but not have keyboard
     * focus).
     * @param comp The component that should get keyboard focus.
     */
    private void forceKeyboardFocus(Component comp) {
        if (comp.isFocusOwner()) {
            comp.transferFocus();
            comp.requestFocusInWindow();
        }
    }

}

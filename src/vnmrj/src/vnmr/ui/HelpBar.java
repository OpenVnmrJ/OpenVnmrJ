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
import javax.swing.*;
import javax.swing.Timer;

import vnmr.bo.VLabel;

public class HelpBar extends VLabel {
    private Timer timer;
    private int   status;


    public HelpBar() {
         super(null, null, null);
         // setBackground(new Color(119, 214, 252));
         // setForeground(new Color(59, 59, 59));
         setForeground(Color.black);
         setOpaque(false);
         this.status = 0;
         setTitlebarObj(true);
    }

    public void setMessage(String text) {
         if (timer == null) {
             ActionListener updateAction = new ActionListener() {
                public void actionPerformed(ActionEvent ae) {
                     // updateInfo();
                     close();
                }
             };
             timer = new Timer(9000, updateAction);
             timer.setRepeats(false);
         }
         if (text == null || text.length() < 2) {
             timer.stop();
             showText = ""; 
             // setVisible(false);

             repaint();
             return;
             /*********
             if (status < 0) {
                 if (!isVisible())
                    return;
             }
             status = -1;
             timer.setDelay(100);
             *********/
         }
         else {
             // setText(text);
             showText = text;
             repaint();
            /*****
             setVisible(true);
             status = 0;
             timer.setDelay(100);
            ****/
         }
         timer.restart();
    }

    private void close() {
         // setVisible(false);
         showText = ""; 
         repaint();
    }

    private void updateInfo() {
         Point pt = getLocation();
         Dimension dim = getSize();
         if (status < 0) {
              if (pt.y >= dim.height) {
                   setVisible(false);
                   timer.stop();
              }
              else {
                  pt.y += 2;
                  setLocation(pt.x, pt.y);
              } 
              return;
         }
         if (status == 0) {
              if (pt.y >= dim.height)
                  pt.y = dim.height - 2;
              else {
                  pt.y = pt.y - 2;
                  if (pt.y < 0) {
                       pt.y = 0;
                       status = 1;
                       timer.setDelay(8000);
                  }
              }
              setLocation(pt.x, pt.y);
              setVisible(true);
              return;
         }
         else {
             status = -1;
             timer.setDelay(100);
         }
    }
}


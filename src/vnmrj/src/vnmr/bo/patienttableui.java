/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import javax.swing.UIManager;
import java.awt.*;

import vnmr.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2000
 * Company:
 * @author
 * @version 1.0
 */

public class patienttableui {
  boolean packFrame = false;
  static String ptalignPath = "/vnmr/bin/ptalign";
  static String ptalignTermDev = "/dev/term/a";

  /**Construct the application*/
  public patienttableui() {
    IPatTabFrame frame = new IPatTabFrame(ptalignPath,ptalignTermDev);
    //Validate frames that have preset sizes
    //Pack frames that have useful preferred size info, e.g. from their layout
    if (packFrame) {
      frame.pack();
    }
    else {
      frame.validate();
    }
    //Center the window
    Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
    Dimension frameSize = frame.getSize();
    if (frameSize.height > screenSize.height) {
      frameSize.height = screenSize.height;
    }
    if (frameSize.width > screenSize.width) {
      frameSize.width = screenSize.width;
    }
    frame.setLocation((screenSize.width - frameSize.width) / 2, (screenSize.height - frameSize.height) / 2);
    frame.setVisible(true);
  }
  /**Main method*/
  public static void main(String[] args) {
    try {
      UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
    }
    catch(Exception e) {
        Messages.writeStackTrace(e);
    }
    /*---------------------------------------------------------------*/
    // Get arguments.
    /*---------------------------------------------------------------*/
    if (args.length > 1) {
        ptalignPath = args[0];
        ptalignTermDev = args[1];
    }
    new patienttableui();
  }
}

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
// import javax.swing.*;

/*
 * This class will be used as clock to update VObj
*/

public class VObjTimer implements ActionListener {

    protected static VObjTimer instance = null;
    private java.util.List <VObjTimerListener> objList = null;
    private javax.swing.Timer updateTimer;
    private boolean bAccess = false;
    // private int  updateCount = 0;
    

    public VObjTimer() {
         if (objList == null)
            objList = Collections.synchronizedList(new LinkedList<VObjTimerListener>());
         updateTimer = new javax.swing.Timer(4000, this);
         updateTimer.setRepeats(false);
    }
    
    public static void addListener(VObjTimerListener obj) {
         if (instance == null)
            instance = new VObjTimer();
         instance.addObj(obj);
    }

    public static void removeListener(VObjTimerListener obj) {
         if (instance == null)
            return;
         instance.removeObj(obj);
    }

    protected void addObj(VObjTimerListener obj) {
         bAccess = true;
         // updateCount = 0;
         if (obj != null && !objList.contains(obj)) {
             objList.add(obj);
         }
         // if (!updateTimer.isRunning())
             updateTimer.restart();
         bAccess = false;
    }

    protected void removeObj(VObjTimerListener obj) {
         objList.remove(obj);
    }

    /***********
    private boolean clearList() {
         Iterator itr = objList.iterator();
         if (bAccess)
             return false;
         while (itr.hasNext()) {
            VObjTimerListener obj = (VObjTimerListener)itr.next();
            JComponent jobj = (JComponent)obj;
            if (!jobj.isShowing()) {
                objList.remove(obj);
                return true;
            }
         }
         return false;
    }
    ***********/

    public void actionPerformed(ActionEvent e) {

         if (objList.size() <= 0)
             return;
         if (bAccess)
             return;
         Iterator<VObjTimerListener> itr;
         itr = objList.iterator();
         while (itr.hasNext()) {
             VObjTimerListener obj = (VObjTimerListener)itr.next();
             obj.timerRinging();
         }
         objList.clear();
         /*********
         updateCount++;
         if (updateCount > 2) {
             while (true) {
                 if (!clearList())
                     break;
             }
         }
         if (updateCount < 4)
             updateTimer.restart();
         *********/
    }
}

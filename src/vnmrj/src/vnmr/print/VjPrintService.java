/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.Component;
import java.lang.Runnable;
import java.awt.print.*;


public class VjPrintService implements  VjPrintEventListener, Runnable {

     private VjPrinterJob printJob;
     private VjPrintEventListener printEventListener;
     private boolean bTest;
     private static Component topDialog = null;

     public VjPrintService(Printable prtObj, boolean testOnly) {
           this.bTest = testOnly;
           createPrintJob();
     }

     public VjPrintService(Printable prtObj) {
           this(prtObj, false);
     }

     private void createPrintJob() {
           printJob = new VjPrinterJob();
           printJob.setPrintEventListener(this);
     }

     public void showDialog() {
           if (printJob == null)
               createPrintJob();
           printJob.printDialog();
     }

     public void print() {
          if (printEventListener != null) {
              VjPrintEvent ej = new VjPrintEvent(this, VjPrintDef.PRINT);
              printEventListener.printEventPerformed(ej);
          }
     }

     public void run() {
          showDialog();
     }

     public void closeDialog() {
          if (printJob != null)
              printJob.closeDialog();
     }

     public void printEventPerformed(VjPrintEvent e) {
          if (bTest) {
              if (e.getStatus() == VjPrintDef.APPROVE)
                 return;
              closeDialog();
              System.exit(0);
          }
          if (e.getStatus() == VjPrintDef.APPROVE) {
               print();
          }
     }

     public void setPrintEventListener(VjPrintEventListener  l) {
           printEventListener = l;
     }


     public VjPrinterJob getPrintJob() {
          return printJob;
     }

     public static void main(String[] args) {
         VjPrintService pc = new VjPrintService(null, true);
         VjPrintUtil.setTestMode(true);
         pc.showDialog();
    }

    public static void setTopDialog(Component c) {
         topDialog = c;
    } 

    public static Component getTopDialog() {
         return topDialog;
    }

} // endof VjPrintService


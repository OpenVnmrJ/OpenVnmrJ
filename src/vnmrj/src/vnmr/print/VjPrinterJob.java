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
import java.awt.Frame;
import java.awt.GraphicsEnvironment;
import java.awt.GraphicsConfiguration;
import javax.print.DocFlavor;
import javax.print.PrintService;
import javax.print.attribute.PrintRequestAttributeSet;
import javax.print.attribute.HashPrintRequestAttributeSet;

public class VjPrinterJob extends Component implements VjPrintEventListener {


   private VjPrintEventListener printEventListener;
   private ServicePopup  prtSrvPopup;

   protected PrintRequestAttributeSet attributes = null;



    public VjPrinterJob()
    {
    }

    public void closeDialog() {
         if (prtSrvPopup != null)
             prtSrvPopup.closeDialog();
    }

    public void printEventPerformed(VjPrintEvent e) {
         if (e.getStatus() != VjPrintDef.APPROVE)
            return;
         if (printEventListener == null)
            return;

         printEventListener.printEventPerformed(e);
    }


    public void setPrintEventListener(VjPrintEventListener  e) {
          printEventListener = e;
    }

    private void createServicePopup() {
          final GraphicsConfiguration gc =
            GraphicsEnvironment.getLocalGraphicsEnvironment().
            getDefaultScreenDevice().getDefaultConfiguration();

          PrintService[] services = null;

          prtSrvPopup = new ServicePopup(gc, 10, 100, services, 
                 DocFlavor.SERVICE_FORMATTED.PAGEABLE, attributes,
                 (Frame)vnmr.util.Util.getMainFrame());

          prtSrvPopup.setPrintEventListener(this);       
    }

    private void vjPrintDialog() {
          attributes = new HashPrintRequestAttributeSet();
          if (prtSrvPopup == null) {
              createServicePopup();
              if (prtSrvPopup == null)
                  return;
          }
          else
              prtSrvPopup.setOriginalAttributeSet(attributes);
          prtSrvPopup.showDialog();
     }


    public boolean printDialog()  {
        if (printEventListener != null) {
            vjPrintDialog();
        }
        return false;
    }
}

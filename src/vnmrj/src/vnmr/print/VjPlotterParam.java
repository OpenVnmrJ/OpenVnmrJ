/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;


import vnmr.sms.SmsInfoObj;
import vnmr.util.ParamIF;


public class VjPlotterParam extends SmsInfoObj  {
     public static final int PLOTTER = 1;
     public static final int PRINTER = 2;
     private int  type;

     public VjPlotterParam(String str, int type) {
        super(str, 0);
        this.type = type;
     }

     public void setValue(ParamIF pf) {
        if (pf != null && pf.value != null) {
              if (type == PLOTTER)
                 VjPrintUtil.setVnmrPlotter(pf.value);
              if (type == PRINTER)
                 VjPrintUtil.setVnmrPrinter(pf.value);
        }
    }
}


/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package accounting;

import java.awt.*;
import java.text.*;
import java.util.*;
import javax.swing.BorderFactory;
import javax.swing.JSpinner;
import javax.swing.SpinnerDateModel;

public class DaySpinner extends JSpinner {
    
  static  Font f = new Font("Serif",Font.PLAIN,12);

  public DaySpinner(String dateString) {
    setBorder(BorderFactory.createEmptyBorder());
    setModel( new SpinnerDateModel() );
    setFont(f);
    JSpinner.DateEditor daySpin = new JSpinner.DateEditor(this, dateString);
    this.setEditor(daySpin);
  }
  public DaySpinner(String disp,String init) {
    this(disp);
    int k=0;
    GregorianCalendar c = new GregorianCalendar();
    if (disp.compareTo("EEE")==0) {
      DateFormatSymbols dfs = new DateFormatSymbols();
      String[] days = dfs.getShortWeekdays();
      for (int i=0; i<8; i++) {
          k=i;
//          System.out.println(days[i]);
          if (init.compareTo(days[i])==0) break;
      }
      c.set(Calendar.DAY_OF_WEEK,k);
//      System.out.println("initial value is " + c.getTime());
    }
    else {
      int hh = new Integer(init.substring(0, 2)).intValue();
      int mm = new Integer(init.substring(3, 5)).intValue();
//      System.out.println("hh="+hh+" mm="+mm);
      c.set(Calendar.HOUR_OF_DAY, hh);
      c.set(Calendar.MINUTE, mm);
//      System.out.println("initial value is " + c.getTime());
    }
    setValue(c.getTime());
  }
}

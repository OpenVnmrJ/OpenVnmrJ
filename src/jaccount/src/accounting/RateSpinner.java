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
import javax.swing.BorderFactory;
import javax.swing.JSpinner;
import javax.swing.SpinnerNumberModel;

public class RateSpinner extends JSpinner {
    
  static  Font f = new Font("Serif",Font.PLAIN,12);
  
  public RateSpinner(Float rate) {
    setBorder(BorderFactory.createEmptyBorder());
    setFont(f);
    SpinnerNumberModel snm = new SpinnerNumberModel();
    snm.setValue( rate );
    snm.setStepSize( new Float(0.25) );
    snm.setMinimum( new Float(0.00) );
    setModel(snm);
    JSpinner.NumberEditor rateSpin = new JSpinner.NumberEditor(this, new String("0.00"));
    this.setEditor(rateSpin);
  }
}
/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * GButton.java
 *
 * Created on May 13, 2006, 5:03 AM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */

package accounting;

import java.awt.*;
import javax.swing.*;

/**
 *
 * @author frits
 */
public class GButton extends JButton{
    
    /** Creates a new instance of GButton */
    public GButton(String str) {
        super(str);
    }
    
    public void paintComponent(Graphics g) {
        Color b;
        float h = (float)(getHeight() / 3.0);
        float zero = (float)0.0;
        Graphics2D g2 = (Graphics2D) g;
        b = getBackground().darker();
        if (model.isPressed()) b = b.darker();
        GradientPaint gp = new GradientPaint(zero,-h,b,
                zero,h,Color.WHITE,true);
        g2.setPaint(gp);
        g2.fill(this.getVisibleRect());
        setContentAreaFilled(false);
        super.paintComponent(g);
    }
}

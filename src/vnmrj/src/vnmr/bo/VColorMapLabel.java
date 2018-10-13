/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.bo;

import java.awt.*;
import javax.swing.*;


public class VColorMapLabel extends JLabel  {

    private String valueStr;
    private FontMetrics fm;
    private int textWidth;
    private int orientInt;
    private int fontAscent;
    private static double deg90 = Math.toRadians(90.0);

    public VColorMapLabel(int orient, String text)
    {
         super("M ");
         orientInt = orient;
         if (orient == SwingConstants.HORIZONTAL) {
              setHorizontalAlignment(SwingConstants.CENTER);
              setText(text);
         }
         valueStr = text;
         fontAscent = 10;
    }

    public void paint(Graphics g) {
        if (orientInt == SwingConstants.HORIZONTAL) {
            super.paint(g);
            return;
        }
        if (valueStr == null)
            return;
        Graphics2D g2d = (Graphics2D) g;
        if (fm == null) {
            fm = getFontMetrics(getFont());
            fontAscent = fm.getAscent() + 1;
        }
        textWidth = fm.stringWidth(valueStr); 
        int y;
        Dimension  size = getSize();
        y = (size.height - textWidth) / 2 + textWidth;
        g2d.translate(fontAscent, y);
        g2d.rotate(-deg90);
        g2d.drawString(valueStr, 0, 0);
    }
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
import java.awt.*;
import javax.swing.border.*;

public class VTitledBorder extends TitledBorder {
    protected int edge_spacing = 0;

    public VTitledBorder(Border border,                     
			 String title,
			 int titleJustification,
			 int titlePosition,
			 Font titleFont,
			 Color titleColor) {
	super(border, title, titleJustification,
	      titlePosition, titleFont, titleColor);
    }

    /**
     * Paints the border for the specified component with the 
     * specified position and size.
     * @param c the component for which this border is being painted
     * @param g the paint graphics
     * @param x the x position of the painted border
     * @param y the y position of the painted border
     * @param width the width of the painted border
     * @param height the height of the painted border
     */
    public void paintBorder(Component c, Graphics g, int x, int y,
			    int width, int height) {

        String titleStr = getTitle();
        if (titleStr == null || titleStr.length() < 1) {
            getBorder().paintBorder(c, g, x, y, width, height);
            return;
        }

        Border border = getBorder();

        Rectangle grooveRect = new Rectangle(x + edge_spacing, y + edge_spacing,
                                             width - (edge_spacing * 2),
                                             height - (edge_spacing * 2));
        Font font = g.getFont();
        Color color = g.getColor();

        g.setFont(getFont(c));

        FontMetrics fm = g.getFontMetrics();
        int         fontHeight = fm.getHeight();
        int         descent = fm.getDescent();
        int         ascent = fm.getAscent();
        Point       textLoc = new Point();
        int         diff, titlePos;
        int         stringWidth = fm.stringWidth(titleStr);
        Insets      insets;

        if(border != null)
            insets = border.getBorderInsets(c);
        else
            insets = new Insets(0, 0, 0, 0);

        titlePos = getTitlePosition();
        switch (titlePos) {
            case ABOVE_TOP:
                diff = ascent + descent + (Math.max(edge_spacing,
                                TEXT_SPACING*2) - edge_spacing);
                grooveRect.y += diff;
                grooveRect.height -= diff;
                textLoc.y = grooveRect.y - (descent + TEXT_SPACING);
                break;
            case TOP:
            case DEFAULT_POSITION:
                diff = Math.max(0, ((ascent/2) + TEXT_SPACING) - edge_spacing);
                grooveRect.y += diff;
                grooveRect.height -= diff;
                textLoc.y = (grooveRect.y - descent) +
                (insets.top + ascent + descent)/2;
                break;
            case BELOW_TOP:
                textLoc.y = grooveRect.y + insets.top + ascent + TEXT_SPACING;
                break;
            case ABOVE_BOTTOM:
                textLoc.y = (grooveRect.y + grooveRect.height) -
                (insets.bottom + descent + TEXT_SPACING);
                break;
            case BOTTOM:
                grooveRect.height -= fontHeight/2;
                textLoc.y = ((grooveRect.y + grooveRect.height) - descent) +
                        ((ascent + descent) - insets.bottom)/2;
                break;
            case BELOW_BOTTOM:
                grooveRect.height -= fontHeight;
                textLoc.y = grooveRect.y + grooveRect.height + ascent +
                        TEXT_SPACING;
                break;
        }

        switch (getTitleJustification()) {
            case LEFT:
            case DEFAULT_JUSTIFICATION:
                textLoc.x = grooveRect.x + TEXT_INSET_H + insets.left;
                break;
            case RIGHT:
                textLoc.x = (grooveRect.x + grooveRect.width) -
                        (stringWidth + TEXT_INSET_H + insets.right);
                break;
            case CENTER:
                textLoc.x = grooveRect.x +
                        ((grooveRect.width - stringWidth) / 2);
                break;
        }
        if(border != null && !(border instanceof EmptyBorder)) {
            if (((titlePos == TOP || titlePos == DEFAULT_POSITION) &&
                  (grooveRect.y > textLoc.y - ascent)) ||
                 (titlePos == BOTTOM &&
                  (grooveRect.y + grooveRect.height < textLoc.y + descent))) {
                Rectangle clipRect = new Rectangle();
                // save original clip
                Rectangle saveClip = g.getClipBounds();

                // paint strip left of text
                clipRect.setBounds(saveClip);
                if (intersection(clipRect, x, y, textLoc.x-1-x, height)) {
                    g.setClip(clipRect);
                    border.paintBorder(c, g, grooveRect.x, grooveRect.y,
                                  grooveRect.width, grooveRect.height);
                }
                // paint strip right of text
                clipRect.setBounds(saveClip);
                if (intersection(clipRect, textLoc.x+stringWidth+1, y,
                               x+width-(textLoc.x+stringWidth+1), height)) {
                    g.setClip(clipRect);
                    border.paintBorder(c, g, grooveRect.x, grooveRect.y,
                                  grooveRect.width, grooveRect.height);
                }
                if (titlePos == TOP || titlePos == DEFAULT_POSITION) {
                    // paint strip below text
                    clipRect.setBounds(saveClip);
                    if (intersection(clipRect, textLoc.x-1, textLoc.y+descent,
                                        stringWidth+2, y+height-textLoc.y-descent)) {
                        g.setClip(clipRect);
                        border.paintBorder(c, g, grooveRect.x, grooveRect.y,
                                  grooveRect.width, grooveRect.height);
                    }

                } else { // titlePos == BOTTOM
                  // paint strip above text
                    clipRect.setBounds(saveClip);
                    if (intersection(clipRect, textLoc.x-1, y,
                          stringWidth+2, textLoc.y - ascent - y)) {
                        g.setClip(clipRect);
                        border.paintBorder(c, g, grooveRect.x, grooveRect.y,
                                  grooveRect.width, grooveRect.height);
                    }
                }

                // restore clip
                g.setClip(saveClip);
            }
            else {
                border.paintBorder(c, g, grooveRect.x, grooveRect.y,
                                  grooveRect.width, grooveRect.height);
            }
           /*********
            if(!(border instanceof EmptyBorder)){
	        g.setColor(c.getBackground());
            g.fillRect(textLoc.x - TEXT_SPACING, textLoc.y - (fontHeight-descent),
                   stringWidth + (2 * TEXT_SPACING), fontHeight - descent);
           *********/
        }
        ((Graphics2D)g).setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING,RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        g.setColor(getTitleColor());
        g.drawString(titleStr, textLoc.x, textLoc.y);

        g.setFont(font);
        g.setColor(color);
    }

        /** 
     * Reinitialize the insets parameter with this Border's current Insets. 
     * @param c the component for which this border insets value applies
     * @param insets the object to be reinitialized
     */
    public Insets getBorderInsets(Component c, Insets insets) {
        FontMetrics fm;
        int         descent = 0;
        int         ascent = 16;

        Border border = getBorder();
        if (border != null) {
            if (border instanceof AbstractBorder) {
                insets = ((AbstractBorder)border).getBorderInsets(c, insets);
            } else {
                // Can't reuse border insets because the Border interface
                // can't be enhanced.
                insets = border.getBorderInsets(c);
            }
        } else {
            insets.left = insets.top = insets.right = insets.bottom = 0;
        }

        insets.left += edge_spacing + TEXT_SPACING;
        insets.right += edge_spacing + TEXT_SPACING;
        insets.top += edge_spacing + TEXT_SPACING;
        insets.bottom += edge_spacing + TEXT_SPACING;

        if(c == null || getTitle() == null || getTitle().equals(""))    {
            return insets;
        }

        Font font = getFont(c);

        fm = c.getFontMetrics(font);

	if(fm != null) {
  	   descent = fm.getDescent();
	   ascent = fm.getAscent();
	}

        switch (getTitlePosition()) {
          case ABOVE_TOP:
              insets.top += ascent + descent
                            + (Math.max(edge_spacing, TEXT_SPACING*2)
                            - edge_spacing);
              break;
          case TOP:
          case DEFAULT_POSITION:
              insets.top += ascent + descent;
              break;
          case BELOW_TOP:
              insets.top += ascent + descent + TEXT_SPACING;
              break;
          case ABOVE_BOTTOM:
              insets.bottom += ascent + descent + TEXT_SPACING;
              break;
          case BOTTOM:
              insets.bottom += ascent + descent;
              break;
          case BELOW_BOTTOM:
              insets.bottom += ascent + TEXT_SPACING;
              break;
        }
        return insets;
    }

    private static boolean intersection(Rectangle dest,
                                       int rx, int ry, int rw, int rh) {
        int x1 = Math.max(rx, dest.x);
        int x2 = Math.min(rx + rw, dest.x + dest.width);
        int y1 = Math.max(ry, dest.y);
        int y2 = Math.min(ry + rh, dest.y + dest.height);
        dest.x = x1;
        dest.y = y1;
        dest.width = x2 - x1;
        dest.height = y2 - y1;

        if (dest.width <= 0 || dest.height <= 0) {
            return false;
        }
        return true;
    }

}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.geom.*;

import javax.swing.*;
import javax.swing.plaf.*;
import vnmr.util.*;

public class BasicClockDialUI extends ClockDialUI {

    private final double ln10 = Math.log(10.0);
    private int xMax = 0;
    private int yMax = 0;
    private int markRadius = 0;
    private int xShift = 0;
    private int yShift = 0;


    public BasicClockDialUI() { }

    public static ComponentUI createUI(JComponent c) {
        return new BasicClockDialUI();
    }

    public void paint(Graphics g1, JComponent c) {
        ClockDial vd = (ClockDial)c;
        Graphics2D g = (Graphics2D)g1;

        //  We don't want to paint outside of the insets or borders
        Insets insets = vd.getInsets();
        int width = vd.getWidth()-insets.left-insets.right;
        int height = vd.getHeight()-insets.top-insets.bottom;
        int radius = (Math.min(width, height) - 1) / 2;
        if (radius <= 1) {
            return;
        }
        int diam = radius * 2;
        xShift = width/2 + insets.left;
        yShift = height/2 + insets.top;
        g.translate(xShift, yShift);

        //  Draw the face background and rim
        g.setColor(vd.getBackground());
        g.fillOval(-radius, -radius, diam, diam);
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
                RenderingHints.VALUE_ANTIALIAS_ON);
        g.setColor(vd.getBackground().darker());
        g.drawArc(-radius, -radius, diam, diam, 45, 180);
        g.setColor(vd.getBackground().brighter());
        g.drawArc(-radius, -radius, diam, diam, 225, 180);

        int value = vd.getValue();

        double lhLength;
        double shLength;
        double mod;
        double theta;

        int tickWidth = 2 + radius / 15;
        if (vd.isTicked()) {
            // Draw the tick marks
            double r1 = radius - 1;
            double r2 = r1 - tickWidth;
            int x1, y1, x2, y2;
            theta = Math.PI/2;
            for (int i=0; i<10; i++, theta -= Math.PI / 5) {
                double dtheta = 1.5 / r1;
                if (theta < -Math.PI / 4 && theta > -5 * Math.PI / 4) {
                    dtheta = -dtheta;
                }
                x1 = (int)(r1 * Math.cos(theta + dtheta));
                y1 = (int)(-r1 * Math.sin(theta + dtheta));
                x2 = (int)(r2 * Math.cos(theta + dtheta));
                y2 = (int)(-r2 * Math.sin(theta + dtheta));
                g.setColor(vd.getBackground().brighter());
                g.drawLine(x1, y1, x2, y2);
                x1 = (int)(r1 * Math.cos(theta - dtheta));
                y1 = (int)(-r1 * Math.sin(theta - dtheta));
                x2 = (int)(r2 * Math.cos(theta - dtheta));
                y2 = (int)(-r2 * Math.sin(theta - dtheta));
                g.setColor(vd.getBackground().darker());
                g.drawLine(x1, y1, x2, y2);
            }
        }

        int maxDisplay = vd.getMaximum();
        if (maxDisplay == 0) {
            maxDisplay = 1;	// Avoid divide by 0
        }

        int lslice;
        if (vd.isLongHand() ) {
            shLength = (int)(radius * 0.72);
            lslice = (int)shLength;
        } else {
            shLength = radius - tickWidth;
            lslice = (int)shLength - tickWidth;
        }
        if (shLength <= 0) {
            shLength = 1;
        }
        if (lslice <= 0) {
            lslice = 1;
        }
        if (vd.isShortHand()) {
            // Draw the short hand
            double fat = shLength / 20 + 2;
            mod = (double)(value) / maxDisplay;
            theta = Math.PI / 2 - mod * 2 * Math.PI;
            if (vd.isPieSlice()) {
                // Draw the pie slice
                g.setColor(vd.getMaxColor());
                int angle = (int)(-360*mod);
                g.fillArc(-lslice, -lslice,
                        2*lslice, 2*lslice,
                        90, angle);
                if (vd.isMaxMarker()) {
                    // Draw max short-hand marker
                    mod = (double)(vd.getMaxReading()) / maxDisplay;
                    angle = (int)(-360 * (mod));
                    g.drawArc(-lslice, -lslice,
                            2*lslice, 2*lslice,
                            90, angle);
                    double maxtheta = Math.PI / 2 - mod * 2 * Math.PI;
                    g.drawLine(0, 0,
                            (int)(lslice * Math.cos(maxtheta)),
                            (int)(-lslice * Math.sin(maxtheta)));
                    if ( !vd.isLongHand() ) {
                        xMax = (int)(lslice * Math.cos(maxtheta));
                        yMax = (int)(-lslice * Math.sin(maxtheta));
                        markRadius = tickWidth/2;
                        g.fillOval(xMax - markRadius, yMax - markRadius,
                                2*markRadius, 2*markRadius);
                    }
                }
            }

            g.setColor(c.getForeground());
            drawHand(g, shLength, fat, theta);

            if (vd.isMaxMarker() && !vd.isPieSlice()) {
                // Draw the max level
                mod = (double)(vd.getMaxReading()) / maxDisplay;
                theta = Math.PI / 2 - mod * 2 * Math.PI;
                double len2 = shLength - fat;
                g.setColor(vd.getMaxColor());
                xMax = (int)(len2 * Math.cos(theta));
                yMax = (int)(-len2 * Math.sin(theta));
                markRadius = (int)fat;
                g.fillOval(xMax - markRadius, yMax - markRadius,
                        2 * markRadius, 2 * markRadius);
            }
        }

        if (vd.isLongHand()) {
            //  Draw the long hand
            lhLength = radius - 2;
            if (lhLength <= 0) {
                lhLength = 1;
            }
            mod = ((value * 10) % maxDisplay) / (double)maxDisplay;

            if (vd.isColorBars()) {
                // Draw color bands for the long hand
                double win = 2 * shLength;
                double wout = 2 * (radius - tickWidth - 2);
                double xin = -(win / 2);
                double xout = -(wout / 2);
                theta = 90 - mod * 360; // Theta in degrees!
                double arc1 = 90 - theta;
                double arc2 = arc1 - 360;

                // Calculate the colors
                int band = (int)(10 * ((double)value / maxDisplay));
                band = Math.max(0, band);
                if (band <= 9) {
                    GeneralPath gp = new GeneralPath();
                    gp.append(new Arc2D.Double(xout, xout, wout, wout,
                            theta, arc1, Arc2D.OPEN), false);
                    gp.append(new Arc2D.Double(xin, xin, win, win,
                            theta + arc1, -arc1, Arc2D.OPEN), true);
                    g.setColor(vd.getBarColor(band));
                    g.fill(gp);
                }
                if (band > 0) {
                    GeneralPath gp = new GeneralPath();
                    gp.append(new Arc2D.Double(xout, xout, wout, wout,
                            theta, arc2, Arc2D.OPEN), false);
                    gp.append(new Arc2D.Double(xin, xin, win, win,
                            theta + arc2, -arc2, Arc2D.OPEN), true);
                    g.setColor(vd.getBarColor(band - 1));
                    g.fill(gp);
                }
            }

            // Draw the hand
            double fat = lhLength / 40;
            theta = Math.PI / 2 - mod * 2 * Math.PI; // Theta in radians
            g.setColor(c.getForeground());
            drawHand(g, lhLength, fat, theta);

            if (vd.isMaxMarker()) {
                // Draw the max level
                mod = (vd.getMaxReading() * 10) % maxDisplay;
                mod /= maxDisplay;
                double len2 = lhLength - tickWidth / 2;
                theta = Math.PI / 2 - mod * 2 * Math.PI;
                g.setColor(vd.getMaxColor());
                xMax = (int)(len2 * Math.cos(theta));
                yMax = (int)(-len2 * Math.sin(theta));
                markRadius = tickWidth / 2;

                if ((vd.getMaxReading() - value) < (maxDisplay/20)) {
                    g.fillOval(xMax - markRadius, yMax - markRadius,
                            2*markRadius, 2*markRadius);
                } else {
                    g.drawOval(xMax - markRadius, yMax - markRadius,
                            2*markRadius, 2*markRadius);
                }
            }
            if (vd.isMinMarker()) {
                // Draw the min level
                mod = (vd.getMinReading() * 10) % maxDisplay;
                mod /= maxDisplay;
                double len2 = lhLength - 2*tickWidth - 1;
                theta = Math.PI / 2 - mod * 2 * Math.PI;
                if ((value - vd.getMinReading()) < (maxDisplay / 20)) {
                    g.setColor(Color.green);
                } else {
                    g.setColor(Color.green.darker());
                }
                g.fillOval((int)(len2 * Math.cos(theta)) - tickWidth / 2,
                        (int)(-len2 * Math.sin(theta)) - tickWidth / 2,
                        tickWidth, tickWidth);
            }
        }

        if (vd.isDigitalReadout()) {
            double scale = vd.getScaling();
            FontMetrics metrics = g.getFontMetrics(g.getFont());
            String str;

            double ratio = 1000.0 / (maxDisplay * scale);
            int prec = (int)Math.rint(Math.log(ratio) / ln10);
            prec = Math.max(0, prec);
            str = Fmt.f(prec, value * scale);
            // NB: getStringBounds() results are only approximate!
            Rectangle2D strBox = metrics.getStringBounds(str, g);
            /*System.out.println("Bounds=("+strBox.getWidth()+"x"+
			       strBox.getHeight()+")");/*CMP*/
            int dx = (int)strBox.getWidth() / 2;
            int dy = (int)strBox.getHeight() / 2;
            int dX = dx + 2;
            int dY = dy + 2;
            g.setColor(c.getBackground().brighter());
            g.fillRect(-dX, -dY, 2*dX+2, 2*dY);
            g.setColor(c.getBackground().brighter());
            g.drawLine(-dX, -dY, dX+2, -dY);
            g.drawLine(-dX, -dY, -dX, dY);
            g.setColor(c.getBackground().darker());
            g.drawLine(-dX, dY, dX+2, dY);
            g.drawLine(dX+2, -dY, dX+2, dY);
            g.setColor(c.getForeground());
            g.drawString(str, -dx, dy);
        }
        g.translate(-xShift, -yShift);
    }

    private void drawHand(Graphics2D g,
            double length, double fat, double theta) {
        double lengthRatio = (length - 2*fat) / length;
        double x1 = length * Math.cos(theta); // Point of hand
        double y1 = -length * Math.sin(theta);
        theta += Math.PI / 2;
        double x2 = fat * Math.cos(theta); // Side of hand by center
        double y2 = -fat * Math.sin(theta);
        double x3 = x2 + lengthRatio * x1; // "base" of pointy part
        double y3 = y2 + lengthRatio * y1;

        GeneralPath gp = new GeneralPath();
        gp.append(new Line2D.Double(x1, y1, x3, y3), true);
        double deg = theta * 180 / Math.PI;
        gp.append(new Arc2D.Double(-fat, -fat, 2 * fat, 2 * fat,
                deg, 180, Arc2D.OPEN), true);
        gp.append(new Line2D.Double(x3 - 2 * x2, y3 - 2 * y2, x1, y1), true);
        g.fill(gp);
    }
}

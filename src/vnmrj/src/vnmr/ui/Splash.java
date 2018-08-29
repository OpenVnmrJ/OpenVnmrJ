/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GradientPaint;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GraphicsEnvironment;
import java.awt.Paint;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.lang.reflect.InvocationTargetException;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.Icon;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

import vnmr.util.Util;


public class Splash extends JFrame implements ActionListener {

    static private Splash sm_frame = null;
    static private Icon sm_bkgIcon = null;
    static private String sm_msg = "";
    static private Font sm_msgFont = null;
    static private Color sm_msgColor = Color.RED;
    static private Color sm_topColor = Color.LIGHT_GRAY;
    static private Color sm_bottomColor = Color.GRAY;
    static private boolean sm_showCancel = false;


    /**
     * @param args
     */
    public static void main(String[] args) {
        Splash.getFrame(
                        new ImageIcon("/vnmr/iconlib/Splash_VnmrJ.png"),
                        "Demo Splash Screen ...",
                        new Font("Serif", Font.BOLD | Font.ITALIC, 25),
                        new Color(0xcc9900).darker().darker(),
                        true);
    }

    /**
     * Show a spash frame with given background, and a message in a
     * given font and color.
     * To use a background from an image file use, e.g.,
     * <tt>new ImageIcon("/vnmr/iconlib/Splash_VnmrJ.png")</tt>.
     * Fonts can be constructed with <tt>new Font(String name, int style,
     * int size)</tt>.  The fonts named Serif, SansSerif, Monospaced,
     * Dialog, and DialogInput are always available in Java.
     * Styles are Font.PLAIN, Font.BOLD, Font.ITALIC,
     * and (Font.BOLD | Font.ITALIC).
     * @param bkgIcon An image to use for the background; if null, uses a
     * default shaded background, sized 500x500.
     * @param msg The text to appear at the bottom of the frame.
     * @param msgFont The font to use for the text.
     * @param msgColor The color of the text.
     * @param showCancel If true, a button is shown that kills the app.
     * @return A handle to the splash frame.
     */
    static public Splash getFrame(Icon bkgIcon,
                                  String msg, 
                                  Font msgFont,
                                  Color msgColor,
                                  boolean showCancel) {
        sm_bkgIcon = (bkgIcon == null
                      || bkgIcon.getIconWidth() <= 0
                      || bkgIcon.getIconHeight() <= 0)
                ? null
                : bkgIcon;
        sm_msg = msg;
        sm_msgFont = msgFont;
        sm_msgColor = msgColor;
        sm_showCancel = showCancel;
        return getFrame();
    }

    static private Splash getFrame() {
        if (sm_frame == null) {
            if (!SwingUtilities.isEventDispatchThread()) {
                Runnable showSplashEvent = new Runnable() {
                        public void run() {
                            getFrame(); // Call me again in Event Thread
                        }
                    };
                try {
                    SwingUtilities.invokeAndWait(showSplashEvent);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                } catch (InvocationTargetException e) {
                    e.printStackTrace();
                }
                // NB: We are NOT in the event thread here
                try {
                    // NB: If we start doing intensive GUI building in the
                    // Event Thread before the splash is shown, it may
                    // not get a turn to be painted.
                    // TODO: Try to detect when splash gets drawn
                    Thread.sleep(500); // Give the splash screen time to show
                } catch (InterruptedException ie) {}
                return sm_frame;
            }

            // At this point, we are executing in the Event Thread
            sm_frame = new Splash();
        }
        return sm_frame;
    }

    private Splash() {
        Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension screenDim = tk.getScreenSize();
        setDefaultCloseOperation(EXIT_ON_CLOSE);
        setUndecorated(true);
        Container content = getContentPane();

        JLabel bkgPane;
        Dimension size;
        if (sm_bkgIcon != null) {
            bkgPane = new JLabel(sm_bkgIcon);
        } else {
            Icon bkgIcon = new BackgroundIcon(sm_topColor, sm_bottomColor);
            bkgPane = new JLabel(bkgIcon);
        }
        size = bkgPane.getPreferredSize();
        bkgPane.setOpaque(true);
        content.add(bkgPane, BorderLayout.NORTH);

        JPanel fgPane = new JPanel();
        fgPane.setOpaque(false);
        fgPane.setPreferredSize(size);
        bkgPane.setLayout(new java.awt.FlowLayout(0, 0, 0));
        bkgPane.add(fgPane);

        JLabel lb = new JLabel(" " + sm_msg);
        if (sm_msgFont != null) {
            lb.setFont(sm_msgFont);
        }

        // NB: In the released Java 1.6, this won't even compile with the
        // reference to com.sun.java.swing.SwingUtilities2.
        //if (System.getProperty("java.version").startsWith("1.5")) {
        //    // Turn on anti-aliasing in the message label
        //    // (com.sun.java.swing.SwingUtilities2 is not in Java 1.6)
        //    lb.putClientProperty
        //            (com.sun.java.swing.SwingUtilities2.AA_TEXT_PROPERTY_KEY,
        //             Boolean.TRUE);
        //}
        
        lb.setForeground(sm_msgColor);
        fgPane.setLayout(new BorderLayout());
        fgPane.add(lb, BorderLayout.SOUTH);

        Box topPanel = Box.createHorizontalBox();
        topPanel.add(Box.createHorizontalGlue());
        fgPane.add(topPanel, BorderLayout.NORTH);

        if (sm_showCancel) {
            Icon quitIcon = new QuitIcon();
            JButton quitButton = new JButton(quitIcon);
            quitButton.setPreferredSize
                    (new Dimension(quitIcon.getIconWidth(),
                                   quitIcon.getIconHeight()));
            quitButton.setBorder(BorderFactory.createEmptyBorder());
            quitButton.setOpaque(false);
            quitButton.setToolTipText(Util.getTooltipString("Abort Loading"));
            quitButton.addActionListener(this);
            topPanel.add(quitButton);
        }
        setSize(size);
        Point center =
            GraphicsEnvironment.getLocalGraphicsEnvironment().getCenterPoint();
        setLocation(center.x - size.width / 2, center.y - size.height / 2);
        setVisible(true);
        validate();
    }

    /**
     * Called when the Quit button is pressed to kill the program.
     */
    public void actionPerformed(ActionEvent e) {
        dispose();
        System.exit(0);
    }


    class QuitIcon implements Icon {

        private int mm_width = 23;

        private int mm_height = 23;

        private int mm_border = 2;


        public int getIconHeight() {
            return mm_height;
        }

        public int getIconWidth() {
            return mm_width;
        }

        public void paintIcon(Component c, Graphics g, int x, int y) {
            g.setColor(Color.BLACK);
            int x0 = mm_border;
            int x1 = mm_width - mm_border - 1;
            int y0 = mm_border;
            int y1 = mm_height - mm_border - 1;
            g.drawRect(x0, y0, x1 - x0, y1-y0);

            g.setColor(Color.WHITE);
            x0++; y0++; x1--; y1--;
            g.fillRect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);

            x0++; y0++; x1--; y1--;
            g.setColor(Color.RED);
            g.drawLine(x0, y0, x1, y1);
            g.drawLine(x0 + 1, y0, x1, y1 - 1);
            g.drawLine(x0, y0 + 1, x1 - 1, y1);
            g.drawLine(x0, y1, x1, y0);
            g.drawLine(x0 + 1, y1, x1, y0 + 1);
            g.drawLine(x0, y1 - 1, x1 - 1, y0);
        }
    }


    /**
     * Paints a default gradient background.
     */
    class BackgroundIcon implements Icon {

        private int mm_width = 500;

        private int mm_height = 500;

        private Paint mm_paint;


        public BackgroundIcon(Color topColor, Color bottomColor) {
            mm_paint = new GradientPaint(0, 0, topColor,
                                         0, mm_height - 1, bottomColor);
        }

        public int getIconHeight() {
            return mm_height;
        }

        public int getIconWidth() {
            return mm_width;
        }

        public void paintIcon(Component c, Graphics g, int x, int y) {
            Graphics2D g2 = (Graphics2D)g;
            g2.setPaint(mm_paint);
            Rectangle rect = new Rectangle(x, y, mm_width, mm_height);
            g2.fill(rect);
        }
    }

}

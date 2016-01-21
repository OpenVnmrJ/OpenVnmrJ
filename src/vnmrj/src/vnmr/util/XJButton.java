/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.awt.*;
import java.awt.image.*;
import javax.swing.*;
import javax.swing.plaf.*;
import vnmr.ui.*;

public class XJButton extends JButton
{
    static private int refWidth;
    static private int refHeight;
    static private Image refImage = null;
    static private int[] refRGBArray;
    static private Map<String, BufferedImage> m_bkgImageMap = new HashMap<String, BufferedImage>(); 
    static private BufferedImage refBufImg = null;
    protected boolean isEditing = false;
    protected boolean is3D = false;
    protected boolean isFocused = false;
    protected boolean bTest = false;
    protected Color bg;
    private BufferedImage bkgImage = null;

    public XJButton()
    {
        initUI();
    }

    public XJButton(Icon icon)
    {
        super(icon);
        initUI();
    }

    public XJButton(String text)
    {
        super(text);
        initUI();
    }

    public XJButton(Action a)
    {
        super(a);
        initUI();
    }

    public XJButton(String text, Icon icon)
    {
        super(text, icon);
        initUI();
    }

    private void  initUI() {
        set3D(false);
        setBorder(new VButtonBorder());
        bg = null;
        if(refImage == null) {
            refImage = Util.getImage("convexGrayButton.jpg");
            try {
                MediaTracker tracker = new MediaTracker(this);
                tracker.addImage(refImage, 0);
                tracker.waitForID(0);
            } catch(Exception e) { }
            if(refImage != null) {
                refWidth = refImage.getWidth(this);
                refHeight = refImage.getHeight(this);
                refBufImg = new BufferedImage(refWidth, refHeight,
                        BufferedImage.TYPE_INT_RGB);
                Graphics2D big = refBufImg.createGraphics();
                big.drawImage(refImage, 0, 0, this);
                refRGBArray = refBufImg.getRGB(0, 0, refWidth, refHeight, null,
                        0, refWidth);
                int iMiddle = (refWidth + (refHeight * refWidth)) / 2;
                Color refBkg = new Color(refRGBArray[iMiddle]);
            }
        }
        setOpaque(false);
    }

    public void setBorderInsets(Insets insets) {
        VButtonBorder b = (VButtonBorder) getBorder();
        if (b != null)
            b.setBorderInsets(insets);
    }

    public void changeFocus(boolean s) {
        isFocused = s;
        repaint();
    }

    private BufferedImage makeBackgroundImage(Color bg) {
        BufferedImage img = null;
        if(is3D && refImage != null && (bg != null ||
                 (bg = Util.getParentBackground(this)) != null)) {
            String strColor = bg.toString();
            img = m_bkgImageMap.get(strColor);
            if(img == null) {
                img = new BufferedImage(refWidth, refHeight,
                        BufferedImage.TYPE_INT_RGB);
                int len = refRGBArray.length;
                int iMiddle = (refWidth + len) / 2;
                Color refBkg = new Color(refRGBArray[iMiddle]);
                int refGray = refBkg.getBlue();
                int[] bkgRGBArray = new int[len];
                for(int i = 0; i < len; i++) {
                    Color thisRefColor = new Color(refRGBArray[i]);
                    int thisGray = refRGBArray[i] & 0xff;
                    int brightness = (100 * (thisGray - refGray)) / refGray;
                    Color thisBkg = Util.changeBrightness(bg, brightness);
                    bkgRGBArray[i] = thisBkg.getRGB();
                }
               img.setRGB(0, 0, refWidth, refHeight, bkgRGBArray, 0, refWidth);
                m_bkgImageMap.put(strColor, img);
            }
        }

        bkgImage = img;
        return img;
    }

    public BufferedImage makeBackgroundImage() {
        if (bg == null)
           bg = getBackground();
        return makeBackgroundImage(bg);
    }


    public void paintComponent(Graphics g) {
        Dimension ps = getSize();
        // Try to make bkgImage here because the first time we draw the
        // image may not have been made yet. If our bkg color is null,
        // we can't make bkgImage until we have a parent container.
        if(is3D && (bkgImage != null || makeBackgroundImage() != null)) {
            g.drawImage(bkgImage, 1, 1, ps.width - 2, ps.height - 2, this);
        } else {
            g.setColor(getBackground());
            g.fillRect(0, 0, ps.width, ps.height);
        }
        setContentAreaFilled(false);
        super.paintComponent(g);

        if(isEditing) {
            g.setColor(isFocused ? Color.yellow : Color.green);
            g.drawRect(0, 0, ps.width - 1, ps.height - 1);
        }
    }

    public void setBackground(Color c) {
        bg = c;
        makeBackgroundImage(bg);
    }

    protected void set3D(boolean b) {
        String mod = System.getProperty("force3dlook");
        if(mod != null) {
            if(mod.equals("no"))
                is3D = b;
            else
                is3D = true;
            return;
        }
        mod = Util.getOption("force3dlook", "yes");

        if(mod != null && mod.equals("no"))
            is3D = b;
        else
            is3D = true;
    }

    public void setTestMode(boolean s) {
        bTest = s;
    }

}

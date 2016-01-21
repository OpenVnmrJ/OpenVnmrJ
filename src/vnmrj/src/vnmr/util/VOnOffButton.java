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
import java.awt.event.*;
import java.beans.*;
import javax.swing.*;
import java.awt.dnd.*;

import vnmr.ui.*;

// public class VOnOffButton extends VJButton {
public class VOnOffButton extends JButton implements PropertyChangeListener {

    public static final int UP = 1;
    public static final int DOWN = 2;
    public static final int RECT = 3;
    public static final int LEFT = 4;
    public static final int RIGHT = 5;
    public static final int ROUND = 6;
    public static final int SQUARE = 7;
    public static final int ICON = 8;
    public static final int LINE = 9;
    public static final int DOT = 10;

    private int type, orgType;
    private int width, height;
    private int orient = SwingConstants.VERTICAL;
    private boolean showComp;
    private boolean bMoveable;
    private boolean bDragable;
    private boolean bDraging;
    private boolean bEntered;
    private boolean bShowBorder;
    private JComponent comp;
    private String name = null;
    private JPopupMenu popup;


    public VOnOffButton(ImageIcon img, JComponent ctlComp, boolean show) {
        // super(null, img, true);
        super(null, img);
        this.comp = ctlComp;
        this.showComp = show;
        this.type = ICON;
        this.bMoveable = true;

        addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                comp.setVisible(showComp);
                setVisible(false);
                getParent().validate();
            }
        });
    } // VOnOffButton

    public VOnOffButton(int typ, JComponent ctlComp) {
        // super(true);
        super(null, null);
        this.type = typ;
        this.orgType = typ;
        this.comp = ctlComp;
        this.showComp = true;
        this.bMoveable = true;
        this.bShowBorder = true;
        setOpaque(true);
        if (type == ICON) {
            getIconImage();
            DisplayOptions.addChangeListener(this);
        }

        if(typ == LINE || typ == DOT || typ == ICON)
            setCtrlProc();
        else
            setListener();
    } // VOnOffButton

    @Override
    public boolean isContentAreaFilled() {
        return bShowBorder;
    }

    @Override
    public boolean isBorderPainted() {
        return bShowBorder;
    }

    private void showBorder(boolean b) {
        bShowBorder = b;
        setOpaque(b);
        // setBorderPainted(b);
        //  setContentAreaFilled(b);
    }

    private void getIconImage() {
        if (type != ICON)
             return;
        ImageIcon icon = null;
        if (orient == SwingConstants.VERTICAL)
            icon = Util.getHorBumpIcon();
        else
            icon = Util.getVerBumpIcon();
        if (icon != null) {
            showBorder(false);
            setIcon(icon);
            width = icon.getIconWidth() + 4;
            height = icon.getIconHeight();
        }
    }

    public void setOrientation(int n) {
        if (n != orient) {
             orient = n;
             getIconImage();
        }
    }

    public void propertyChange(PropertyChangeEvent evt) {
        if (DisplayOptions.isUpdateUIEvent(evt)) {
             getIconImage();
        }
    }

    public void setBg(Color bg) {
        setBackground(bg);
        comp.setBackground(bg);
        repaint();
    }

    public void setMoveable(boolean b) {
        bMoveable = b;
        if (b)
            setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
        else
            setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
    }

    public void setDragable(boolean b) {
        bDragable = b;
        if (b) {
            setCursor(DragSource.DefaultMoveDrop);
        }
        else
            setMoveable(bMoveable);
    }

    private void doAction(JMenuItem s) {
        String str = s.getText();
        VnmrjIF vif = Util.getVjIF();
        if(vif != null)
            vif.openComp(name, str);
    }

    private void setListener() {
        addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if(comp.isVisible()) {
                    comp.setVisible(false);
                    type = RECT;
                } else {
                    comp.setVisible(true);
                    type = orgType;
                }
                getParent().validate();
            }
        });
    }

    private void moveTool(MouseEvent ev) {
        if (bDragable)
            return;
        if (name == null || !bMoveable)
            return;
        VnmrjIF vjIf = Util.getVjIF();
        if (vjIf == null)
            return;
        int y = ev.getY();
        int x = ev.getX();
        if(y < -6) {
            vjIf.openComp(name, "moveUp");
            return;
        }
        if(y > height + 6) {
            vjIf.openComp(name, "moveDown");
            return;
        }
    }

    private void dragTool(MouseEvent ev) {
        if (name == null || !bDragable)
            return;
        VnmrjIF vjIf = Util.getVjIF();
        if (vjIf == null)
            return;
        int y = ev.getY();
        int x = ev.getX();
        if ((y > -2) && (y < height + 2))
            return;
        Point loc = getLocationOnScreen();
        vjIf.moveComp(name, loc.x + x, loc.y + y);
    }

    private void popupMenu() {
        popup.show(this, width + 6, 2);
    }

    private void setCtrlProc() {
        setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
        height = 12;
        addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                bDraging = false;
                bEntered = true;
                if(e.getButton() == MouseEvent.BUTTON3) {
                    popupMenu();
                }
            }

            public void mouseReleased(MouseEvent e) {
                if (e.getButton() == MouseEvent.BUTTON1) {
                     dragTool(e);
                }
                bDraging = false;
                if (type == ICON && !bEntered)
                    showBorder(false);
            }

            public void mouseEntered(MouseEvent evt) {
                bEntered = true;
                if (type == ICON)
                    showBorder(true);
            }

            public void mouseExited(MouseEvent evt) {
                bEntered = false;
                if (type == ICON && !bDraging)
                    showBorder(false);
            }
        });

        addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent e) {
                bDraging = true;
                moveTool(e);
            }
        });

        popup = new JPopupMenu();
        JMenuItem mi = new JMenuItem("Close");
        mi.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                doAction((JMenuItem)ae.getSource());
            }
        });
        popup.add(mi);
    }

    public void setType(int s) {
        type = s;
    }

    public void setName(String s) {
        name = s;
        if(s !=null && s.startsWith("Hardware"))
            setCursor(Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR));
    }

    public void setShow(boolean s) {
        comp.setVisible(s);
        if(s)
            type = orgType;
        else
            type = RECT;
        getParent().validate();
    }

    public void paint(Graphics g) {
        super.paint(g);
        Dimension psize = getSize();
        width = psize.width;
        height = psize.height;

        if(type == ICON)
            return;
        Color bgColor = getBackground();
        Color fgColor = Util.getInputFg();
        boolean pressed = getModel().isPressed() && getModel().isArmed();
        int y = height / 2 - 4;
        if(y < 0)
            y = 0;
        int x = width / 2;
        int x1 = x - 2;
        int x2 = 0;
        if(x1 < 0)
            x1 = 0;
        int y1 = y + 6;
        if(type == DOT) {
            // g.setColor(bgColor.darker());
            g.setColor(fgColor);
            x = width / 2 - 1;
            y1 = height / 4;
            y = y1;
            for(x1 = 0; x1 < 3; x1++) {
                g.drawLine(x, y, x, y + 1);
                g.drawLine(x + 1, y, x + 1, y + 1);
                y += y1;
            }
            return;
        }
        if(type == LINE) {
            // g.setColor(bgColor.darker());
            g.setColor(fgColor);
            x = 3;
            y = 3;
            while(y < height) {
                g.drawLine(x, y, x, y + 1);
                y += 3;
            }
            x = 5;
            y = 4;
            while(y < height) {
                g.drawLine(x, y, x, y + 1);
                y += 3;
            }
            return;
        }
        if(type == UP || type == DOWN) {
            x1 = 1;
            x2 = x + x - 1;
            if(x2 < 2)
                return;
            y = psize.height / 2 - x2 / 2;
            if(y < 0)
                y = 0;
            y1 = y + psize.width;
        }
        if(type == UP) {
            if(pressed)
                g.setColor(bgColor.darker());
            else
                g.setColor(bgColor.brighter());
            g.drawLine(x, y, x1, y1);
            if(pressed)
                g.setColor(bgColor.brighter());
            else
                g.setColor(bgColor.darker());
            g.drawLine(x1, y1, x2, y1);
            g.drawLine(x, y, x2, y1);
        }
        if(type == DOWN) {
            if(pressed)
                g.setColor(bgColor.darker());
            else
                g.setColor(bgColor.brighter());
            g.drawLine(x1, y, x2, y);
            g.drawLine(x1, y, x, y1);
            if(pressed)
                g.setColor(bgColor.brighter());
            else
                g.setColor(bgColor.darker());
            g.drawLine(x, y1, x2, y);
        }
        if(type == RECT) {
            x2 = psize.width - 2;
            y1 = psize.height - 2;
            if(x2 < 1 || y1 < 1)
                return;
            if(pressed)
                g.setColor(bgColor.darker());
            else
                g.setColor(bgColor.brighter());
            g.drawLine(2, 1, x2, 1);
            g.drawLine(2, 1, 2, y1);
            if(pressed)
                g.setColor(bgColor.brighter());
            else
                g.setColor(bgColor.darker());
            g.drawLine(2, y1, x2, y1);
            g.drawLine(x2, 1, x2, y1);
        }
    }
}

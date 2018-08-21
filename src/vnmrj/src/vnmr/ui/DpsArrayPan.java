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
import java.util.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.Timer;
import javax.swing.border.BevelBorder;
import javax.swing.border.Border;

import vnmr.bo.*;
import vnmr.util.*;

public class DpsArrayPan extends JPanel {
    private JLabel arraydimLabel;
    private Vector arrayQ;
    private Timer repeatTimer = null;
    private DpsArray timerObj = null;
    private boolean doUp = false;
    private ButtonIF expIf;

    public DpsArrayPan(ButtonIF eif) {
        this.expIf = eif;
        arraydimLabel = new JLabel("arraydim: ");
        setLayout(new SimpleVLayout());
        add(arraydimLabel);
        arrayQ = new Vector();
        arrayQ.setSize(5);
    }

    public void setArrayDim(String str) {
        arraydimLabel.setText(str);
    }

    public void clearArray() {
        DpsArray obj;
        for(int k = 0; k < arrayQ.size(); k++) {
            obj = (DpsArray)arrayQ.elementAt(k);
            if(obj != null)
                obj.setVisible(false);
        }
    }

    public void processData(String str) {
        VStrTokenizer tok = new VStrTokenizer(str, " ,\n");
        if(!tok.hasMoreTokens())
            return;
        int id, type, min, max, k;
        String d = tok.nextToken();
        if(d.equals("new")) {
            d = tok.nextToken();
            if(d == null)
                return;
            id = Integer.parseInt(d);
            d = tok.nextToken();
            if(d == null)
                return;
            type = Integer.parseInt(d);
            d = tok.nextToken();
            if(d == null)
                return;
            min = Integer.parseInt(d);
            d = tok.nextToken();
            if(d == null)
                return;
            max = Integer.parseInt(d);
            addArray(id, type, min, max);
            return;
        }
        if(d.equals("name")) {
            d = tok.nextToken();
            if(d == null)
                return;
            id = Integer.parseInt(d);
            d = str.substring(tok.getPosition());
            setArrayName(id, d);
            return;
        }
        if(d.equals("val")) {
            d = tok.nextToken();
            if(d == null)
                return;
            id = Integer.parseInt(d);
            d = tok.nextToken();
            if(d == null)
                return;
            k = Integer.parseInt(d); // array index
            d = str.substring(tok.getPosition());
            setArrayVal(id, k, d);
            return;
        }
        if(d.equals("dim")) {
            setArrayDim(str.substring(tok.getPosition()));
            return;
        }
        if(d.equals("end")) {
            max = 0;
            for(id = 0; id < arrayQ.size(); id++) {
                DpsArray obj = (DpsArray)arrayQ.elementAt(id);
                if(obj != null && obj.isVisible()) {
                    k = obj.getNameWidth();
                    if(k > max)
                        max = k;
                }
            }
            for(id = 0; id < arrayQ.size(); id++) {
                DpsArray obj = (DpsArray)arrayQ.elementAt(id);
                if(obj != null && obj.isVisible()) {
                    obj.setNameWidth(max);
                }
            }
        }

    }

    public void addArray(int id, int type, int min, int max) {

        if(arrayQ.size() <= id) {
            arrayQ.setSize(id + 3);
        }
        DpsArray obj = (DpsArray)arrayQ.elementAt(id);
        if(obj == null) {
            obj = new DpsArray(id, type, min, max);
            arrayQ.setElementAt(obj, id);
            add(obj);
        } else {
            obj.setType(type);
            obj.setArrayBoundary(min, max);
            obj.resetNameSize();
        }
        obj.setIndex(min);
        obj.setVisible(true);
    }

    public void setArrayName(int id, String label) {
        if(arrayQ.size() <= id)
            return;
        DpsArray obj = (DpsArray)arrayQ.elementAt(id);
        if(obj == null)
            return;
        obj.setName(label);
    }

    public void setArrayVal(int id, int index, String val) {
        if(arrayQ.size() <= id)
            return;
        DpsArray obj = (DpsArray)arrayQ.elementAt(id);
        if(obj == null)
            return;
        obj.setValue(index, val);
    }

    public void setArrayBoundary(int id, int min, int max) {
        if(arrayQ.size() <= id)
            return;
        DpsArray obj = (DpsArray)arrayQ.elementAt(id);
        if(obj == null)
            return;
        obj.setArrayBoundary(min, max);
    }

    public void startRepeatTimer(DpsArray comp, boolean b) {
        doUp = b;
        if(repeatTimer == null) {
            dpsDummy obj = new dpsDummy();
            repeatTimer = new Timer(100, obj);
            repeatTimer.setInitialDelay(500);
        }
        timerObj = comp;
        if(repeatTimer != null)
            repeatTimer.restart();
    }

    public void stopRepeatTimer() {
        timerObj = null;
        if(repeatTimer != null)
            repeatTimer.stop();
    }

    public void updateArrayVal(int id, int type, int index) {
        String str = "dps('-vj', 'array', " + id + ", " + type + ", " + index
                + ")\n";
        expIf.sendToVnmr(str);
    }

    public void sendRedoCmd() {
        String str = "dps('-vj', 'button', 'Redo'"+")\n";
        expIf.sendToVnmr(str);
    }

    private class DpsArray extends JComponent {
        private int id;
        private int type;
        private int min, max;
        private int curIndex;
        private int oldIndex;
        private JLabel arrayName;
        private JLabel arrayValue;
        private JTextField arrayIndex;
        private JButton upWidget, downWidget;

        public DpsArray(int num, int type, int min, int max) {
            this.id = num;
            this.type = type;
            this.min = min;
            this.max = max;
            this.curIndex = min;
            this.oldIndex = min;
            arrayName = new JLabel(" ");
            arrayIndex = new JTextField(4);
            arrayIndex.setBorder(BorderFactory
                    .createBevelBorder(BevelBorder.LOWERED));

            arrayIndex.setOpaque(true);
            arrayValue = new JLabel("  ");
            upWidget = new JButton(Util.getImageIcon("up.gif"));
            downWidget = new JButton(Util.getImageIcon("down.gif"));
            upWidget.setBorder(new VButtonBorder());
            downWidget.setBorder(new VButtonBorder());
            upWidget.setActionCommand("up");
            downWidget.setActionCommand("down");
            setLayout(new DpsArrayLayout());
            add(arrayName);
            add(upWidget);
            add(downWidget);
            add(arrayIndex);
            add(arrayValue);
            // arrayIndex.setBackground(Color.lightGray);

            arrayName.setHorizontalAlignment(JTextField.TRAILING);

            arrayIndex.addKeyListener(new KeyAdapter() {
                public void keyReleased(KeyEvent e) {
                    updateIndex();
                }
            });

            ActionListener al = new ActionListener() {
                public void actionPerformed(ActionEvent evt) {
                    indexUpDn(evt);
                }
            };

            MouseAdapter ml = new MouseAdapter() {
                public void mousePressed(MouseEvent e) {
                    JButton but = (JButton)e.getSource();
                    String cmd = but.getActionCommand();
                    boolean up = false;
                    if(cmd.equals("up"))
                        up = true;
                    startRepeat(up);
                }

                public void mouseReleased(MouseEvent e) {
                    stopRepeat();
                    // stopRepeatTimer();
                }
            };

            upWidget.addActionListener(al);
            downWidget.addActionListener(al);

            upWidget.addMouseListener(ml);
            downWidget.addMouseListener(ml);

        }

        public void setName(String str) {
            arrayName.setText(str);
        }

        public void setValue(int index, String str) {
            if(index != curIndex)
                return;
            arrayValue.setText(str);
        }

        public void setType(int k) {
            type = k;
        }

        public void setArrayBoundary(int n, int m) {
            min = n;
            max = m;
        }

        public void resetNameSize() {
            arrayName.setPreferredSize(null);
        }

        public int getNameWidth() {
            Dimension dim = arrayName.getPreferredSize();
            return dim.width;
        }

        public void setNameWidth(int w) {
            Dimension dim = arrayName.getPreferredSize();
            dim.width = w;
            arrayName.setPreferredSize(dim);
        }

        public void setIndex(String str) {
            boolean illegal = false;
            int a = 0;
            if(str.length() <= 0)
                return;
            try {
                a = Integer.parseInt(str);
            } catch(NumberFormatException er) {
                a = curIndex;
                illegal = true;
            }

            if(a < min) {
                a = min;
                illegal = true;
            }
            if(a > max) {
                a = max;
                illegal = true;
            }
            if(illegal)
                arrayIndex.setText("" + a);
            if(a != curIndex) {
                curIndex = a;
                updateArrayVal(id, type, a - min);
            }
        }

        public void setIndex(int i) {
            if(i < min)
                return;
            if(i > max)
                return;
            curIndex = i;
            arrayIndex.setText("" + i);
        }

        public void updateIndex() {
            oldIndex = curIndex;
            String str = arrayIndex.getText().trim();
            setIndex(str);
            if (oldIndex != curIndex) {
                sendRedoCmd();
                oldIndex = curIndex;
            }
        }

        public void indexUpDn(boolean up) {
            int index = curIndex;
            if(up) {
                if(curIndex < max)
                    index = curIndex + 1;
            } else {
                if(curIndex > min)
                    index = curIndex - 1;
            }
            if(index != curIndex) {
                arrayIndex.setText("" + index);
                curIndex = index;
                updateArrayVal(id, type, index - min);
            }
        }

        public void indexUpDn(ActionEvent e) {
            String cmd = e.getActionCommand();
            int index = curIndex;
            if(cmd.equals("up")) {
                indexUpDn(true);
            } else if(cmd.equals("down")) {
                indexUpDn(false);
            }
        }

        public void startRepeat(boolean b) {
            oldIndex = curIndex;
            startRepeatTimer(this, b);
        }

        public void stopRepeat() {
            stopRepeatTimer();
            if (oldIndex != curIndex) {
                sendRedoCmd();
                oldIndex = curIndex;
            }
        }

    }

    private class DpsArrayLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension preferredLayoutSize(Container target) {
            int h = 0;
            int w = 0;
            Dimension dm;
            int nmembers = target.getComponentCount();
            for(int i = 0; i < nmembers; i++) {
                Component comp = target.getComponent(i);
                dm = comp.getPreferredSize();
                if(dm.height > h)
                    h = dm.height;
                w += dm.width;
            }

            return new Dimension(w, h);
        } // preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public void layoutContainer(Container target) {
            synchronized(target.getTreeLock()) {
                Dimension pSize = target.getSize();
                Dimension dm;
                int y = 0;
                int x = 4;
                int w = 0;
                int nmembers = target.getComponentCount();
                for(int i = 0; i < nmembers; i++) {
                    Component comp = target.getComponent(i);
                    if(comp.isVisible()) {
                        dm = comp.getPreferredSize();
                        if(i == nmembers - 1) { // last one
                            w = pSize.width - x;
                            if(w > dm.width)
                                dm.width = w;
                        }
                        if(comp instanceof JButton) {
                            y = (pSize.height - dm.height) / 2;
                            if(y < 0)
                                y = 0;
                        } else
                            y = 0;
                        comp
                                .setBounds(new Rectangle(x, y, dm.width,
                                        dm.height));
                        x = x + dm.width;
                    }
                }
            }
        }
    } // DpsArrayLayout

    private class dpsDummy extends JComponent implements ActionListener {
        public dpsDummy() {
            setOpaque(false);
        }

        public void actionPerformed(ActionEvent e) {
            if(timerObj != null)
                timerObj.indexUpDn(doUp);
        }

        public void paint(Graphics g) {
        }
    }
}

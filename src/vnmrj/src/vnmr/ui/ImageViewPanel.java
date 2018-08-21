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
import java.awt.event.*;
import java.awt.image.BufferedImage;
import java.io.*;
import javax.swing.*;
import javax.imageio.ImageIO;

import vnmr.util.*;

public class ImageViewPanel extends JLayeredPane implements ActionListener {
    private JScrollPane scrollPane;
    private JLabel  imgLabel;
    private JButton closeBtn;
    private JButton loadBtn;
    private SpecPanel specPane;
    private String loadCmd;

    public ImageViewPanel() {
        buildUi();
    }

    private void buildUi() {
        setLayout(new HelpOverlayLayout());
        closeBtn = new JButton("Close");
        closeBtn.setActionCommand("close");
        closeBtn.addActionListener(this);
        add(closeBtn, JLayeredPane.PALETTE_LAYER);
        loadBtn = new JButton("Update");
        loadBtn.setActionCommand("load");
        loadBtn.addActionListener(this);
        loadBtn.setVisible(false);
        add(loadBtn, JLayeredPane.PALETTE_LAYER);

        imgLabel = new JLabel("  No Image ");
        scrollPane =  new JScrollPane(imgLabel,
                       JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                       JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        add(scrollPane, JLayeredPane.DEFAULT_LAYER);
        specPane = new SpecPanel();
        specPane.setVisible(false);
        add(specPane, JLayeredPane.DEFAULT_LAYER);
    }

    public void actionPerformed(ActionEvent  e) {
        String cmd = e.getActionCommand();

        if (cmd.equals("close")) {
            setVisible(false);
            specPane.setVisible(false);
            return;
        }
        if (cmd.equals("load")) {
            if (specPane.isVisible()) {
                String mess = new StringBuffer().append("jFunc(")
                   .append(VnmrKey.SAVESPECDATA).append(", 1)\n").toString();
                Util.sendToVnmr(mess);
            }
            return;
        }
    }

    public void showImage(String name) {
         if (name == null)
              return;
         if (name.equals("spec") || name.equals("spec2")) {
             loadCmd = name;
             specPane.setVisible(true);
             loadBtn.setVisible(true);
             scrollPane.setVisible(false);
             return;
         }
         if (name.equals("specData")) {
            if (specPane.isVisible())
               specPane.load(loadCmd);
            return;
         }

         if (!scrollPane.isVisible()) {
             specPane.setVisible(false);
             loadBtn.setVisible(false);
             scrollPane.setVisible(true);
        }
         String path = FileUtil.openPath(name);
         if (path == null) {
              imgLabel.setIcon(null);
              imgLabel.setText("   Could not open file  ' "+name+" ' ");
              return;
         }
         BufferedImage img = null;
         try {
             img = ImageIO.read(new File(path));
         }
         catch (IOException e) {
             img = null;
         }
         if (img == null) {
             imgLabel.setIcon(null);
             imgLabel.setText("    No Image ");
             return;
         }

         ImageIcon icon = new ImageIcon(img);
         imgLabel.setText(null);
         imgLabel.setIcon(icon);
    }

    private class HelpOverlayLayout implements LayoutManager {

       public void addLayoutComponent(String name, Component comp) {}

       public void removeLayoutComponent(Component comp) {}

       public Dimension preferredLayoutSize(Container target) {
           return new Dimension(0, 0);
       }

       public Dimension minimumLayoutSize(Container target) {
           return new Dimension(0, 0);
       }

       public void layoutContainer(Container target) {
           synchronized (target.getTreeLock()) {
              Dimension dim = target.getSize();
              Dimension d = closeBtn.getPreferredSize();
              int x = dim.width - d.width - 20;
              int y = 4;
              closeBtn.setBounds(x, y, d.width, d.height);
              d = loadBtn.getPreferredSize();
              x = x - d.width - 10;
              loadBtn.setBounds(x, y, d.width, d.height);
              scrollPane.setBounds(0, 0, dim.width, dim.height);
              specPane.setBounds(0, 0, dim.width, dim.height);
           }
       }
    }

}


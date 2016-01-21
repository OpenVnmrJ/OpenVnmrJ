/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.awt.image.BufferedImage;
import vnmr.util.Util;


public class VjPrintPreview extends JDialog implements ActionListener {

     private static VjPrintPreview previewDialog;
     private static int GAP = 4;
     private PreviewPanel viewPanel;
     private JButton printButton;
     private JButton saveButton;
     private JButton closeButton;
     private boolean reported;
     private int returnValue;
     private Dimension paperSize; 
     private int    marginX, marginY; 
     private float  paperRatio = 0.01f; 
     private float  imgWRatio; 
     private float  imgHRatio; 
     private float  marginXRatio; 
     private float  marginYRatio; 
     protected VjPrintEventListener printEventListener;
     

     public VjPrintPreview() {
        Container c = getContentPane();
        c.setLayout(new BorderLayout());
        viewPanel = new PreviewPanel();
        c.add(viewPanel, BorderLayout.CENTER);
        JPanel pnlSouth = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 5));
        String printStr = Util.getLabel("blPrint", "Print");
        printButton = new JButton(printStr);
        printButton.addActionListener(this);
        String saveStr = Util.getLabel("blSave", "Save");
        saveButton = new JButton(saveStr);
        saveButton.addActionListener(this);
        String closeStr = Util.getLabel("blClose", "Close");
        closeButton = new JButton(closeStr);
        closeButton.addActionListener(this);
 
        pnlSouth.add(printButton);
        pnlSouth.add(saveButton);
        pnlSouth.add(closeButton);
        c.add(pnlSouth, BorderLayout.SOUTH);

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
               closeView();
            }
        });
        setAlwaysOnTop(true);
        setLocation(10, 10);
        paperSize = new Dimension(1000, 1000); 
        marginX = 0;
        marginY = 0; 
        marginXRatio = 0.1f;
        marginYRatio = 0.1f;
        viewPanel.setPreferredSize(new Dimension(400, 500));
        pack();
     }

     public static void setPreviewImage(BufferedImage img) {
         if (previewDialog == null)
            previewDialog = new VjPrintPreview();
         previewDialog.setImage(img);
     }

     public static void showDialog(int state) {
         if (previewDialog == null)
            previewDialog = new VjPrintPreview();
         previewDialog.openDialog(state);
     }

     public static void closeDialog() {
         if (previewDialog != null)
             previewDialog.closeView();
     }

     public static void setPreviewSize(Dimension dp, Dimension dv) {
         if (previewDialog == null)
             return;
         previewDialog.setViewSize(dp, dv);
     }

     public static void setMargin(int left, int right, int top, int bottom) {
         if (previewDialog == null)
             return;
         previewDialog.setViewMargin(left, right, top, bottom);
     }

     protected void setImage(BufferedImage img) {
         viewPanel.setImage(img);
         if (isShowing())
            repaint();
     }

     public void setViewSize(Dimension dp, Dimension dv) {
         Dimension ds = Toolkit.getDefaultToolkit().getScreenSize();
         Dimension d0 = viewPanel.getSize();
         Dimension d1 = this.getSize();
         int dx = d1.width - d0.width;
         int dy = d1.height - d0.height;
         float sw, sh;
         float dw, dh, rw, rh, pr;
         float r2;

         if (dp.width <= 0  || dv.width <= 0)
             return;
         paperSize.width = dp.width;
         paperSize.height = dp.height;
         rw = (float) dp.width;
         rh = (float) dp.height;
         pr = rh / rw;
         imgWRatio = (float) dv.width / rw; 
         imgHRatio = (float) dv.height / rh;
         marginXRatio = (float) marginX / rw;
         marginYRatio = (float) marginY / rh; 
         if (pr == paperRatio)
             return;
         paperRatio = pr;
         sw =  (float) ds.width * 0.5f;
         if (ds.height > 1000)
             sh = (float) ds.height - 400.0f;
         else
             sh = (float) ds.height - 280.0f;
         if (sh < 500.0f)
             sh = (float) ds.height - 120.0f;
         r2 = 1.0f;
         dw = rw;
         dh = rh;
         while (r2 > 0.1f) {
              dw = rw * r2;
              dh = dw * pr;
              if (dw <= sw && dh <= sh)
                   break;
              r2 = r2 - 0.05f;
         }
         if (dw < 30.0f || dh < 30.0f)
             return;
         d1.width = (int) dw + dx + GAP * 2;
         d1.height = (int) dh + dy + GAP * 2;
         this.setSize(d1);
     }

     public void openDialog(int state) {
         if (state == VjPrintDef.PRINT) {
            printButton.setVisible(true);
            previewDialog.setTitle("Print Preview");
            saveButton.setVisible(false);
         }
         else {
            printButton.setVisible(false);
            previewDialog.setTitle("Save Preview");
            saveButton.setVisible(true);
         }
         reported = false;
         returnValue = VjPrintDef.PREVIEW_CANCEL;
         setVisible(true);
         repaint();
     }

     public void closeView() {
         setVisible(false); 
         if (reported)
             return;
         reported = true;
         if (printEventListener != null) {
             VjPrintEvent ej = new VjPrintEvent(this, returnValue);
             printEventListener.printEventPerformed(ej);
         }
     }

     public void setViewMargin(int left, int right, int top, int bottom) {
         marginX = left;
         marginY = top;
         marginXRatio = (float) marginX / (float) paperSize.width; 
         marginYRatio = (float) marginY / (float) paperSize.height; 
     }

     public void actionPerformed(ActionEvent e) {
           Object source = e.getSource();

           returnValue = VjPrintDef.PREVIEW_CANCEL;
           if (source == printButton) {
               returnValue = VjPrintDef.PREVIEW_APPROVE;
           }
           else if (source == saveButton) {
               returnValue = VjPrintDef.PREVIEW_APPROVE;
           }

           closeView();
           setImage(null);
     }

     public static void setPrintEventListener(VjPrintEventListener  l) {
           if (previewDialog != null)
              previewDialog.printEventListener = l;
     }

     public void printEventPerformed(VjPrintEvent e) {

           if (e.getStatus() == VjPrintDef.APPROVE) {
           }
     }

     private class PreviewPanel extends JPanel {
        private Image  viewImg;

        public PreviewPanel() {
        }

        protected void setImage(BufferedImage img) {
            viewImg = (Image) img;
            if (img == null)
                return;
            Dimension ds = Toolkit.getDefaultToolkit().getScreenSize();
            int iw = img.getWidth(null);
            if (iw > (ds.width / 2)) {
               viewImg = img.getScaledInstance(ds.width / 3, -1, Image.SCALE_SMOOTH);
            }
        }

        public void paint(Graphics g) {
            if (viewImg == null)
                return;
            int iw = viewImg.getWidth(null);
            int ih = viewImg.getHeight(null);
            Dimension dim = getSize();
            int gw = dim.width - GAP * 2;
            int gh = dim.height - GAP * 2;
            int x, y, pw, ph;
            int imw, imh;
            int x2, y2;
            float f2;
            float rh;
            float fw;
            float fh;

            f2 = 1.0f;
            fw = (float) gw;
            fh = (float) gh;
            while (true) {
                rh = fw * f2 * paperRatio;
                if (rh <= fh)
                   break;
                f2 = f2 - (rh - fh) / rh;
                if (f2 < 0.2f)
                   break;
            } 
            fw = f2 * (float) gw;
            fh = fw * paperRatio;

            if (marginXRatio > 0.8f)
                marginXRatio = 0.1f;
            if (marginYRatio > 0.8f)
                marginYRatio = 0.1f;
            x = (int)(fw * marginXRatio) + GAP;
            y = (int)(fh * marginYRatio) + GAP;

            pw = (int) fw;
            ph = (int) fh;
            if (imgWRatio <= 1.0f)
               imw = (int) (fw * imgWRatio);
            else
               imw = (int) fw - x * 2;
            if (imgHRatio <= 1.0f)
               imh = (int) (fh * imgHRatio);
            else
               imh = (int) fh - y * 2;
            g.setColor(Color.black);
            g.drawRect(GAP, GAP, pw, ph); 
            x2 = (pw - imw) / 2;
            if (x2 < x)
               x2 = x;
            y2 = (ph - imh) / 2;
            if (y2 < y)
               y2 = y; 
            g.drawImage(viewImg, x2, y2, x2+imw, y2+imh, 0, 0, iw, ih, null);
        } 
     }

} // endof VjPrintService


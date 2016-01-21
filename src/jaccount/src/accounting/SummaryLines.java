/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * SummaryLines.java
 *
 * Created on April 8, 2006, 11:38 AM
 *
 */

package accounting;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.awt.print.*;
import java.net.*;
import java.util.*;
import javax.swing.*;

/**
 *
 * @author frits
 */
public class SummaryLines extends JPanel{
    
    private ArrayList summaries;
    private Font      f;
    private ImageIcon logo;
    private JLabel    logoLabel;
    private JLabel    oneLabel;
    private JLabel[]  headerLabel = new JLabel[3];
    private JPanel    p,pSouth;
    int               nLines;
    int               pageWidth;
    /** Creates a new instance of SummaryLines */
    public SummaryLines() {
        nLines=0;
        setLayout( new BorderLayout() );
        p = new JPanel( new SummaryLayout() );
        JScrollPane jsp = new JScrollPane(p);
        p.setBackground(Color.WHITE);
        super.add(jsp);
                
        logo = AProps.getInstance().logo();
        logoLabel = new JLabel(logo);    p.add(logoLabel);
        try {
        headerLabel[0] = new JLabel("Summary for Spectrometer: "+
                java.net.InetAddress.getLocalHost().getHostName() );
        } catch (Exception e) {
            e.printStackTrace();
        }
        f = new Font("Ariel",Font.BOLD, 16);
        headerLabel[0].setFont(f);
        p.add(headerLabel[0]);
        headerLabel[1] = new JLabel(new Date().toLocaleString());
        f = new Font("Ariel",Font.PLAIN,12);
        headerLabel[1].setFont(f);
        p.add(headerLabel[1]);
        
        pSouth = new JPanel();
        JButton bPrint = new JButton("Print Summary");
        bPrint.addActionListener( new PrintSummary() );
        pSouth.add(bPrint);
        JButton bXls = new JButton("Save for Excel");
        bXls.addActionListener( new XlsSummary() );
        pSouth.add(bXls);
        
        this.add(pSouth,BorderLayout.SOUTH);        
        
        pageWidth = this.getWidth();
        summaries = new ArrayList();
    }
    
    public void add(String[] oneLine) {
        oneLabel = new JLabel(oneLine[0]);
        oneLabel.setFont(f);
        p.add(oneLabel);
        summaries.add(oneLabel);
        
        oneLabel = new JLabel(oneLine[2]);
        oneLabel.setHorizontalAlignment(SwingConstants.RIGHT);
        oneLabel.setFont(f); 
        p.add(oneLabel);
        summaries.add(oneLabel);
        
//        System.out.println("\nnLabels="+nLines+" y="+(10+nLines*30));
        nLines++;
    }
    
    private class SummaryLayout implements LayoutManager {
        public void addLayoutComponent(String str, Component c) {
        }

        public void layoutContainer(Container parent) {
            JLabel tmpLabel;
            int pageWidth = parent.getWidth();
            int deltaY = 15;

            int xIW = logo.getIconWidth();
            int yIH = logo.getIconHeight();
            logoLabel.setBounds(pageWidth-xIW-20,20,xIW,yIH);
            int nLines = yIH/deltaY+2;  // 1 for roundoff, 1 for y-offset o flogo

            headerLabel[0].setBounds(100, yIH-2*deltaY+10, pageWidth-100, 2*deltaY);
            headerLabel[1].setBounds(100, yIH-deltaY+20,   pageWidth-100, deltaY);
            int n = summaries.size();
            for (int i=0; i<n; i+=2) {
                tmpLabel = (JLabel)summaries.get(i);
                tmpLabel.setBounds(100,            30+nLines*deltaY,pageWidth/2, deltaY);
                tmpLabel = (JLabel)summaries.get(i + 1);
                tmpLabel.setBounds(100+pageWidth/2,30+nLines*deltaY,pageWidth/2-200, deltaY);
//                tmpLabel = (JLabel)summaries.get(i + 2);
//                tmpLabel.setBounds(100+2*pageWidth/3,100+nLines*deltaY,3*pageWidth/4, deltaY);
                nLines++;
            }
        }
        public Dimension minimumLayoutSize( Container parent ) {
             return ( new Dimension(0,0));
        }
        public Dimension preferredLayoutSize( Container parent ) {
            return ( new Dimension(425,parent.getHeight()));
        }
        public void removeLayoutComponent( Component c  ) {
        }
    }
    
    private class PrintSummary implements ActionListener {
        public void actionPerformed(ActionEvent ae) {
            System.out.println("PrintSummary.actionPerformed()");
            PrinterJob pj = PrinterJob.getPrinterJob();
            if ( ! pj.printDialog() )
                return;
            PageFormat pf = pj.defaultPage();
            PrintBillList pob = new PrintBillList();
            pj.setPrintable( pob, pf);
            try {
                pj.print();
            } catch (PrinterException pe) {
            }
            System.out.println("PrintSummmary.actionPerformed(): Done");
        }
    }
    
    private class PrintBillList implements Printable {
        
        JLabel xGlobal;
        String[] header;
        private int      MAX_LINES=40;
        int              j, k = 0, ySofar = 0;
        int              pageWidth, pageHeight;
        
        public PrintBillList() {
        }
  
        public int print(Graphics g, PageFormat pf, int pageIndex) {
            k = 160* pageIndex;
            if (pageIndex > 0) {
                if ( (summaries==null) || (k >=  summaries.size()) || (pageIndex > 5) ) {
                    return Printable.NO_SUCH_PAGE;
                }
            }
            pageWidth = (int)pf.getImageableWidth();
            pageHeight = (int)pf.getImageableHeight();

            Graphics2D g2d = (Graphics2D)g;
            g2d.translate(pf.getImageableX(), pf.getImageableY());
//------------------  header ------------------------------------------------
            placeLogo(g2d);
            Font f = new Font("Ariel",Font.BOLD,12);
            g2d.setFont(f);
            g2d.setColor(Color.BLACK);
            g2d.drawString(headerLabel[0].getText(),       10, ySofar-24);
            f = new Font("Ariel",Font.PLAIN,10);
            g2d.setFont(f);
            g2d.drawString(headerLabel[1].getText(),       10, ySofar-12);

            if (pageIndex == 0) {
                k=0;
            }
            else {
                g2d.drawLine(0,ySofar+10,(int)pageWidth,ySofar+10);
                g2d.drawLine(0,ySofar+12,(int)pageWidth,ySofar+12);
            }
            ySofar += 26;
//----------------------------------------------------------------------------
            System.out.println("index starts next at "+k);
            f = new Font("Ariel",Font.PLAIN,10);
            g2d.setFont(f);
            
            xGlobal = (JLabel)summaries.get(k); k++;    // account number
            j=0;
            while (xGlobal != null && (j<MAX_LINES)) {
                g2d.drawString(xGlobal.getText(),10, ySofar);
                xGlobal = (JLabel)summaries.get(k);     k++;    // how much
                g2d.drawString(xGlobal.getText(),(int)(2*pageWidth/3),ySofar);
      
                if ((k+2) > summaries.size()) {
                    xGlobal= null;
                }
                else 
                    xGlobal = (JLabel)summaries.get(k); k++;    // next account number
                ySofar += 12;
                j++;
            }
            k--;               // at the end three is one too many
            System.out.println("End of Page: k="+k);
            f = new Font("Ariel",Font.ITALIC, 12);
            g2d.setFont(f);
            g2d.drawString("page "+(pageIndex+1),(int)(pageWidth*0.9),(int)(pageHeight-20));
            return Printable.PAGE_EXISTS;
        } 

        private void placeLogo(Graphics2D g2d) {
            int xIW, yIH;
            int yOff = 0;
//    System.out.println("logo is '"+aProps.get("logo"));
            logo = AProps.getInstance().logo();
            xIW = logo.getIconWidth();
            yIH = logo.getIconHeight();
            if (yIH < 70) yOff = 70 - yIH;
            g2d.drawImage((Image)logo.getImage(), (int)(pageWidth-xIW),
                               0,xIW,yIH,Color.WHITE, null);
            ySofar = yOff +yIH;
        }
    }
    
    private class XlsSummary implements ActionListener {
        public void actionPerformed (ActionEvent ae) {
            String s;
            System.out.println("Starting XlsSummary");
            File fln = new File(AProps.getInstance().getRootDir()+"/adm/accounting/sum.csv");
            try {
                BufferedWriter fw  = new BufferedWriter( new FileWriter(fln) );
                fw.write(headerLabel[0].getText(), 0, headerLabel[0].getText().length());
                fw.newLine();
                s = new String("\""+headerLabel[1].getText()+"\"");
                fw.write(headerLabel[1].getText(),0,headerLabel[1].getText().length());
                fw.newLine();
                fw.newLine();
                s = new String("Account, Amount, Paid");
                fw.write(s,0,s.length());
                fw.newLine();
                for (int i=0; i<summaries.size(); i+=2) {
                    s = new String( ((JLabel)(summaries.get(i))).getText()+","+((JLabel)(summaries.get(i+1))).getText());
                    fw.write(s,0,s.length());
                    fw.newLine();
                }
                fw.close();
            } catch (Exception e) {
                e.printStackTrace();
            }       
            System.out.println("Done XlsSummary to "+ fln.getAbsolutePath());
        }
    }
}

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
import javax.swing.event.ChangeListener;
import javax.swing.event.ChangeEvent;
import javax.swing.plaf.basic.BasicSplitPaneUI;
import vnmr.util.*;

public class VjPlotterConfig extends ModelessDialog implements ActionListener {

    private JTabbedPane tpTabs;
    private JSplitPane splitPan;
    private boolean bAdmin;
    private boolean bSystemOnly;
    private boolean bTest;
    private boolean bViewOnly;
    private static boolean doExit = false;
    private static boolean bDebug = false;
    private int spliterTableX;
    private int spliterTypeX;
    private VjPlotterLayout layoutPan;
    private VjPlotterType typePan;
    private VjPlotterTable tablePan;
    VjPlotterParam plotterParam;
    VjPlotterParam printerParam;

    public VjPlotterConfig(boolean admin, boolean systemOnly) {
         super("VNMR Plotter Configuration");
         this.bAdmin = admin;
         this.bSystemOnly = systemOnly;
         this.bTest = false;
         String title = Util.getLabel("_VNMR_Plotter_Configuration","VNMR Plotter Configuration");
         this.setTitle(title);
         buildGUi();
         setAlwaysOnTop(false);
         // queryVnmrPlotter();
         setVisible(true);
         String[] psnames = VjPrintUtil.lookupPrintNames();
         if (psnames == null ||  psnames.length < 1) {
            JOptionPane.showMessageDialog(null,
                       "There is no Printer services.\n Please use Linux System Administration printing tool to setup printer. ", 
                       "Configure VNMR devices", JOptionPane.WARNING_MESSAGE);

         }
    }

    public VjPlotterConfig(boolean admin) {
        this(admin, false);
    }

    public static void main(String[] args) {
         boolean sysAdmin = false;
         boolean sysOnly = false;
         for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("admin"))
               sysAdmin = true;
            if (args[i].equalsIgnoreCase("systemonly"))
               sysOnly = true;
            if (args[i].equalsIgnoreCase("debug"))
               bDebug = true;
         }
         doExit = true; 
         VjPlotterConfig pc = new VjPlotterConfig(sysAdmin, sysOnly); 
         pc.bTest = true;
         pc.setVisible(true);
    }

    private void buildGUi() {
         splitPan = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true);
         splitPan.setOneTouchExpandable(true);
         splitPan.setUI(new BasicSplitPaneUI());
         // splitPan.setDividerSize(8);

         if (!bAdmin && bSystemOnly)
             bViewOnly = true;
         else
             bViewOnly = false;
         tpTabs = new JTabbedPane();

         typePan = new VjPlotterType(bAdmin, bViewOnly, this);

         tablePan = new VjPlotterTable(bAdmin, bViewOnly, this);

         tablePan.setLayoutHandler(typePan);

         // tpTabs.addTab("Vnmr Plotter", tablePan);
         // tpTabs.addTab("Page Layout", typePan);

         // splitPan.setLeftComponent(tpTabs);

         splitPan.setLeftComponent(tablePan);

         layoutPan = new VjPlotterLayout(bAdmin, bViewOnly, this);
         splitPan.setRightComponent(layoutPan);

         Container c = getContentPane();
         // c.setLayout(new BorderLayout());
         c.add(splitPan, BorderLayout.CENTER);

         addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
                 if (doExit)
                    System.exit(0);
            }

            public void windowOpened(WindowEvent event) {
                  queryVnmrPlotter();
            }

         });

         tpTabs.addChangeListener(new ChangeListener() {
            public void stateChanged(ChangeEvent e) {
                tabChanged();
            }
         });

         historyButton.setActionCommand("history");
         historyButton.addActionListener(this);
         historyButton.setVisible(false);
         undoButton.setActionCommand("undo");
         undoButton.addActionListener(this);
         undoButton.setVisible(false);
         closeButton.setActionCommand("close");
         closeButton.addActionListener(this);
         abandonButton.setActionCommand("abandon");
         abandonButton.addActionListener(this);

         VjPrintUtil.setPlotterTable(tablePan);
         VjPrintUtil.setPlotterType(typePan);
         VjPrintUtil.setPlotterConfig(this);

         if (doExit)
             setLocation(20, 60);
         else
             setLocation(20, 10);
         pack();

         Dimension ds = Toolkit.getDefaultToolkit().getScreenSize();
         int w = ds.width - 80;
         int h = ds.height - 300;
         if (ds.height < 900)
             h = ds.height - 120;
   
         Dimension d0 = getPreferredSize();
         if (d0.width > 100 && d0.width < w)
             w = d0.width;
         setSize(w, h);
         spliterTableX = w / 3;
         spliterTypeX = w / 3;
         // splitPan.setDividerLocation(spliterTableX);
         // setPreferredSize(new Dimension(w, h));
    }

    public static boolean debugMode() {
         return bDebug;
    }

    public void rebuild() {
         boolean bNewLayout = typePan.rebuild();
         tablePan.rebuild(bNewLayout);
         tablePan.setLayoutHandler(typePan);
         queryVnmrPlotter();
    }

    public VjPlotterType getLayoutHandler() {
        return typePan;
    }

    public void actionPerformed( ActionEvent e )
    {
        String cmd = e.getActionCommand();

        if (cmd.equals("undo")) {

            return;
        }
        if (cmd.equals("history")) {

            return;
        }
        if (cmd.equals("close")) {
           
            layoutPan.saveInfo();
            if (!tablePan.saveToFile())
                return;

            if (!doExit)
                setVisible(false);
            return;
        }
        if (cmd.equals("abandon")) {
            if (doExit)
                System.exit(0);
            else
                setVisible(false);
            return;
        }
 
    }

    private void tabChanged() {
        Component comp = tpTabs.getSelectedComponent();
        if (comp instanceof VjPlotterTable) {
            spliterTypeX = splitPan.getDividerLocation();
            splitPan.setDividerLocation(spliterTableX);
            layoutPan.setEditable(false);
            if (typePan.isNewList()) {
                tablePan.setLayoutList(typePan.getLayoutList());
                typePan.setNewList(false);
            }
        }
        else {
            String name = tablePan.getSelectedPageLayout();
            if (name != null)
                 typePan.showPageType(name);
            spliterTableX = splitPan.getDividerLocation();
            splitPan.setDividerLocation(spliterTypeX);
            layoutPan.setEditable(true);
        }
    }

    public void showTab(Component comp) {
        // tpTabs.setSelectedComponent(comp);
    }

    public void setPageLayout(String type) {
        if (type == null)
            return;
        setPageAttribute(typePan.getPageLayout(type, true));
    }

    public void setPageAttribute(VjPageAttributes a) {
        layoutPan.setPageAttribute(a);
    }

    public void setPlotterDevice(VjPlotterObj obj) {
        layoutPan.setPlotterDevice(obj);
    }

    public void queryVnmrPlotter() {
       if (plotterParam == null)
           plotterParam = new VjPlotterParam("$VALUE=plotter", VjPlotterParam.PLOTTER);
       if (printerParam == null)
           printerParam = new VjPlotterParam("$VALUE=printer", VjPlotterParam.PRINTER);
       printerParam.updateValue();
       plotterParam.updateValue();
    }

    public boolean isTest() {
        return bTest;
    }


} // end of VjPlotterConfig


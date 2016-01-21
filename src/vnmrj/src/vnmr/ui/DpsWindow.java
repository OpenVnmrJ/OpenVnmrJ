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
import java.util.*;
import java.io.*;
import javax.swing.*;

import vnmr.util.*;

public class DpsWindow extends JFrame {
    private JPanel toolPan;
    private JPanel infoPan;
    private DpsGraphPanel graphPan;
    private DpsDataPanel dataPan;
    private JSplitPane splitPan;
    private DpsOptions optionPanel;
    private static boolean  bTest = false;
    private static boolean  bDebug = false;
    private double seqTime;
    private Vector<String> channelName;
    private Vector<String> channelFile;
    private int[] channelItems;
    private int[] channelLines;
    public static int DPSCHANNELS = 14;
    

    public DpsWindow() {
        buildGUi();
    }

    private void buildGUi() {
        Container c = getContentPane();
        optionPanel = new DpsOptions(this);
        DpsUtil.setOptionPanel(optionPanel);
        DpsUtil.setDpsWindow(this);
        createToolPanel();
        createInfoPanel();
        createScopePanel();
        // c.add(toolPan, BorderLayout.NORTH);
        c.add(splitPan, BorderLayout.CENTER);
        c.add(infoPan, BorderLayout.SOUTH);

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
                 if (bTest)
                    System.exit(0);
                 else {
                    savePreference();
                    clear();
                 }
            }

            public void windowOpened(WindowEvent event) {
            }
        });

        pack();
        setPreference();
    }


    private XVButton addTool(String label, String cmd, ActionListener l)
    {
         XVButton but = new XVButton(Util.getLabel("bl"+label, label));
         but.addActionListener(l);
         but.setActionCommand(cmd);
         toolPan.add(but);
         return but;
    }

    private void createToolPanel() {
         if (toolPan != null)
             return;
         toolPan = new JPanel();
         SimpleHLayout layout = new SimpleHLayout();
         layout.setHgap(10);
         toolPan.setLayout(layout);
         ActionListener listener = new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                toolAction(e);
            }
         };

         addTool("Print...", DpsCmds.Print, listener);
         addTool("Zoom In", DpsCmds.ZoomIn, listener);
         addTool("Zoom Out", DpsCmds.ZoomOut, listener);
         addTool("Fit Width", DpsCmds.FitWidth, listener);
         addTool("Fit Height", DpsCmds.FitHeight, listener);
         addTool("Expand", DpsCmds.Expand, listener);
         addTool("Shrink", DpsCmds.Shrink, listener);
         addTool("Options...", DpsCmds.Option, listener);
    }

    private void createInfoPanel() {
         infoPan = new JPanel();
         infoPan.setBorder(BorderFactory.createEtchedBorder());
         DpsUtil.setInfoPanel(infoPan); 
    }

    private void createScopePanel() {
         splitPan = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,false);;
         dataPan = new DpsDataPanel();
         DpsUtil.setDataPanel(dataPan); 
         graphPan = new DpsGraphPanel();
         splitPan.setLeftComponent(dataPan);
         splitPan.setRightComponent(graphPan);
         // splitPan.setDividerSize(6);
    }

    public void clear() {
         graphPan.clear();
         dataPan.clear();
    }

    private void setPreference() {
        int w = 1200;
        int h = 800;
        int x = 100;
        int y = 20;
        int spx = 300;
        @SuppressWarnings("rawtypes")
	Hashtable hs = null;

        if (!bTest) {
            SessionShare ss = Util.getSessionShare();
            if (ss != null)
               hs = ss.userInfo();
        }
        if (hs != null) {
            Integer ix = (Integer) hs.get("dpsWinWidth");
            if (ix != null)
                w = ix.intValue();
            ix = (Integer) hs.get("dpsWinHeight");
            if (ix != null)
                h = ix.intValue();
            ix = (Integer) hs.get("dpsWinLocX");
            if (ix != null)
                x = ix.intValue();
            ix = (Integer) hs.get("dpsWinLocY");
            if (ix != null)
                y = ix.intValue();
            ix = (Integer) hs.get("dpsWinLocX2");
            if (ix != null)
                spx = ix.intValue();
        }
        splitPan.setDividerLocation(spx);
        splitPan.setLastDividerLocation(spx);
        setLocation(x, y);
        setSize(w, h);
    }

    @SuppressWarnings("unchecked")
    private void savePreference() {
        @SuppressWarnings("rawtypes")
	Hashtable hs = null;
        SessionShare ss = Util.getSessionShare();
        if (ss != null)
            hs = ss.userInfo();
        if (hs == null)
            return;
        Point pt = getLocation();
        hs.put("dpsWinLocX", new Integer(pt.x));
        hs.put("dpsWinLocY", new Integer(pt.y));
        Dimension dm = getSize();
        hs.put("dpsWinWidth", new Integer(dm.width));
        hs.put("dpsWinHeight", new Integer(dm.height));
        int x = splitPan.getDividerLocation();
        hs.put("dpsWinLocX2", new Integer(x));
    }

    private void toolAction(ActionEvent e )
    {
        String cmd = e.getActionCommand();
        if (cmd != null)
            graphPan.execCmd(cmd);
    }

    public JPanel getToolPanel() {
        if (toolPan == null)
             createToolPanel();
        return toolPan;
    }

    public void openDpsInfo(String infoPath) {
        String path =  FileUtil.openPath(infoPath);
        if (path == null)
            return;
        BufferedReader fin = null;
        String line, data;
        int    id, k;

        bDebug = false;
        if (channelName == null)
            channelName = new Vector<String>();
        else 
            channelName.clear();
        if (channelFile == null)
            channelFile = new Vector<String>();
        else 
            channelFile.clear();
        if (channelItems == null)
            channelItems = new int[DPSCHANNELS];
        if (channelLines == null)
            channelLines = new int[DPSCHANNELS];
        channelName.setSize(DPSCHANNELS);
        channelFile.setSize(DPSCHANNELS);
        for (id = 0; id < DPSCHANNELS; id++) {
            channelItems[id] = 0;
            channelLines[id] = 0;
        }
        try {
            fin = new BufferedReader(new FileReader(path));
            while ((line = fin.readLine()) != null) {
                StringTokenizer tok = new StringTokenizer(line, " \t\r\n");
                if (!tok.hasMoreTokens())
                    continue;
                data = tok.nextToken();
                if (data.equals("seqtime")) {
                    if (tok.hasMoreTokens())
                        seqTime = Double.parseDouble(tok.nextToken());
                    continue;
                }
                if (data.equals("name")) {
                    id = 999;
                    if (tok.hasMoreTokens())
                        id = Integer.parseInt(tok.nextToken());
                    if (id < DPSCHANNELS && tok.hasMoreTokens())
                        channelName.setElementAt(tok.nextToken(), id);
                    continue;
                }
                if (data.equals("item_num")) {
                    id = 999;
                    k = 0;
                    if (tok.hasMoreTokens())
                        id = Integer.parseInt(tok.nextToken());
                    if (tok.hasMoreTokens())
                        k = Integer.parseInt(tok.nextToken());
                    if (id >= 0 && id < DPSCHANNELS)
                        channelItems[id] = k;
                    continue;
                }
                if (data.equals("line_num")) {
                    id = 999;
                    k = 0;
                    if (tok.hasMoreTokens())
                        id = Integer.parseInt(tok.nextToken());
                    if (tok.hasMoreTokens())
                        k = Integer.parseInt(tok.nextToken());
                    if (id >= 0 && id < DPSCHANNELS)
                        channelLines[id] = k;
                    continue;
                }
                if (data.equals("file")) {
                    id = 999;
                    if (tok.hasMoreTokens())
                        id = Integer.parseInt(tok.nextToken());
                    if (id < DPSCHANNELS && tok.hasMoreTokens())
                        channelFile.setElementAt(tok.nextToken(), id);
                    continue;
                }
                if (data.equals("debug")) {
                    id = 0;
                    if (tok.hasMoreTokens())
                        id = Integer.parseInt(tok.nextToken());
                    if (id > 0)
                        bDebug = true;
                    continue;
                }
                if (data.equals("seqname")) {
                    if (tok.hasMoreTokens())
                        setTitle(tok.nextToken());
                }
            }
        } catch (Exception e) { }
        finally {
            try {
               if (fin != null) {
                   fin.close();
                   if (!bDebug) {
                      File f = new File(path);
                      f.delete();
                   }
               }
            }
            catch (Exception e2) {}
        }
        DpsUtil.setSeqTime(seqTime);
        DpsChannelName channelPan = DpsUtil.getNamePanel();    
        DpsScopePanel scopePan = DpsUtil.getScopePanel();
        DpsOptions optionPan = DpsUtil.getOptionPanel();
        channelPan.startAdd();
        scopePan.startAdd();
        for (k = 0; k < DPSCHANNELS; k++) {
            data = channelName.elementAt(k);
            if (data != null) {
                 channelPan.addChannel(k, data);
                 optionPan.addChannel(k, data);
            }
            data = channelFile.elementAt(k);
            if (data != null)
                 scopePan.addChannel(k, data, channelItems[k], channelLines[k]);
        }
        channelPan.finishAdd();
        scopePan.finishAdd();
        getContentPane().validate();
        scopePan.repaint();
        if (bTest) {
            dataPan.setLabel(path);
            dataPan.setFileData(path);
        }
    }

    public static void main(String[] args) {
         DpsWindow pan;

         bTest = true;
         pan = new DpsWindow();
         pan.setVisible(true);
         pan.openDpsInfo("/home/hung/vnmrsys/exp1/.dpsViewInfo");
    }
}


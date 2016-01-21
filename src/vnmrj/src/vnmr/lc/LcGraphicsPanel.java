/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.LayoutManager;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.util.ArrayList;
import java.util.List;

import javax.swing.BorderFactory;
import javax.swing.JPanel;

import vnmr.bo.VContainer;
import vnmr.bo.VSplitPane;
import vnmr.templates.LayoutBuilder;
import vnmr.ui.ExpPanel;
import vnmr.ui.SessionShare;
import vnmr.util.ButtonIF;
import vnmr.util.FileUtil;
import vnmr.util.Fmt;
import vnmr.util.Messages;
import vnmr.util.VSeparator;


public class LcGraphicsPanel
    extends VContainer implements LayoutManager, ComponentListener {

    private VLcPlot vLcPlot = null;
    private VMsPlot vMsPlot = null;
    private VPdaPlot vPdaPlot = null;
    private VSplitPane splitPane = null;
    private VSplitPane[] splitPanes = null;
    private JPanel edgePanel = null;
    private List<Component> componentList = null;

    public LcGraphicsPanel(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        String filepath = FileUtil.openPath("INTERFACE/LcGraphicsPanel.xml");
        if (filepath == null) {
            Messages.postError("LC panel spec not found:"
                               + " INTERFACE/LcGraphicsPanel.xml");
            return;
        }
        try {
            LayoutBuilder.build(this, vif, filepath);
        } catch (Exception e) {
            Messages.postError(e.toString());
            Messages.writeStackTrace(e);
        }
        //setLayout(new java.awt.BorderLayout());
        setLayout(this);

        componentList = new ArrayList<Component>();
        synchronized(getTreeLock()) {
            fillComponentList(componentList, this);
        }

        ArrayList<Component> alSplitPanes = new ArrayList<Component>();
        Component[] comps = componentList.toArray(new Component[0]);
        for (int i = 0; i < comps.length; i++) {
            if (comps[i] instanceof VSplitPane) {
                if (splitPane == null) {
                    splitPane = (VSplitPane)comps[i];
                }
                alSplitPanes.add(comps[i]);
            }
        }
        int n = alSplitPanes.size();
        splitPanes = new VSplitPane[n];
        for (int i = 0; i < n; i++) {
            splitPanes[i] = (VSplitPane)alSplitPanes.get(i);
        }

        edgePanel = new JPanel();
        edgePanel.setBackground(null);
        edgePanel.setBorder(BorderFactory.createRaisedBevelBorder());
        add(edgePanel);
        //if (splitPane != null) {
        //    ((ExpPanel)vif).addExpListener(splitPane);
        //}
        addComponentListener(this);
    }

    // ComponentListener interface:
    public void componentHidden(ComponentEvent e) {
        //System.out.println("LcGraphicsPanel Hidden");
    }

    public void componentMoved(ComponentEvent e) {
        //System.out.println("LcGraphicsPanel Moved");
    }

    public void componentResized(ComponentEvent e){
        //System.out.println("LcGraphicsPanel Resized");
        for (int i = 0; i < splitPanes.length; i++) {
            splitPanes[i].initDivider();
        }
        // NB: Maybe only remove if splitPanes were big enough?
        removeComponentListener(this);
    }

    public void componentShown(ComponentEvent e) {
        //System.out.println("LcGraphicsPanel Shown");
    }
    // End ComponentListener interface.


    /**
     * Makes a list containing the given component, and its children,
     * recursively.
     * @param comp The top level component that contains all the others.
     * @param list The List to fill.  Not necessarily empty.
     */
    private void fillComponentList(List<Component> list, Component comp) {
        list.add(comp);
        if (comp instanceof Container) {
            Container container = (Container)comp;
            int count = container.getComponentCount();
            Component[] components = container.getComponents();
            for (int i=0; i<count; ++i) {
                fillComponentList(list, components[i]);
            }
        }
    }

    /**
     * Configure the panel to show the specified plots.
     * The LC chromatogram is always shown (if this panel is).
     * @param showPda If true, display the PDA spectrum.
     * @param showMs If true, display the MS spectrum.
     */
    public void configure(boolean showPda, boolean showMs) {
        getPdaPlot().setVisible(showPda);
        getMsPlot().setVisible(showMs);
        splitPanes[0].setDividerVisible(showPda || showMs);
        splitPanes[1].setVisible(showPda || showMs);
        splitPanes[1].setDividerVisible(showPda && showMs);
    }

    public void showMs(MsCorbaClient corbaClient, int timeMilliseconds,
                       String filename, String filename2, int tDisplayed) {
        //remove(vLcPlot);
        //add(vMsPlot, java.awt.BorderLayout.CENTER);
        VMsPlot plot = getMsPlot();
        if (plot != null) {
            plot.plotMassSpectrum(corbaClient, timeMilliseconds,
                                  filename, filename2, tDisplayed);
            repaint();
        }
    }

    public void showPda(double time_min, VPdaImage pdaImage) {
        float[][] data = pdaImage.getPdaSpectrum(time_min);
        VPdaPlot pdaPlot = getPdaPlot();
        if (pdaPlot != null) {
            pdaPlot.setTitle("Spectrum at " + Fmt.f(2, time_min));
            pdaPlot.setXArray(data[0]);
            pdaPlot.setYArray(data[1]);
        }
    }

    public void setMsDataRate(MsCorbaClient corbaClient, int rate_ms) {
        VMsPlot plot = getMsPlot();
        if (plot != null) {
            plot.showCurrentMsData(corbaClient, rate_ms);
        }
    }

    public void setPdaDataRate(PdaCorbaClient corbaClient, int rate_ms) {
        VPdaPlot plot = getPdaPlot();
        if (plot != null) {
            //uses callback no longer valid
            //plot.showCurrentPdaData(corbaClient, rate_ms);
        }
    }

    public VLcPlot.LcPlot getPlot() {
        if (vLcPlot == null && splitPane != null) {
            splitPane.loadPanes();
            Object[] comps = componentList.toArray();
            for (int i = 0; i < comps.length; i++) {
                if (comps[i] instanceof VLcPlot) {
                    vLcPlot = (VLcPlot)comps[i];
                    break;
                }
            }
        }
        if (vLcPlot == null) {
            return null;
        } else {
            return vLcPlot.getLcPlot();
        }
    }

    public VMsPlot getMsPlot() {
        if (vMsPlot == null && splitPane != null) {
            splitPane.loadPanes();
            Object[] comps = componentList.toArray();
            for (int i = 0; i < comps.length; i++) {
                if (comps[i] instanceof VMsPlot) {
                    vMsPlot = (VMsPlot)comps[i];
                    return vMsPlot;
                }
            }
        }
        return vMsPlot;
    }

    public VPdaPlot getPdaPlot() {
        if (vPdaPlot == null && splitPane != null) {
            splitPane.loadPanes();
            Object[] comps = componentList.toArray();
            for (int i = 0; i < comps.length; i++) {
                if (comps[i] instanceof VPdaPlot) {
                    vPdaPlot = (VPdaPlot)comps[i];
                    return vPdaPlot;
                }
            }
        }
        return vPdaPlot;
    }

    /*
     * LayoutManager methods:
     */

    public void addLayoutComponent(String name, Component comp) {}

    public void removeLayoutComponent(Component comp) {}

    public Dimension preferredLayoutSize(Container target) {
        return new Dimension(0, 0); // unused
    }

    public Dimension minimumLayoutSize(Container target) {
        return new Dimension(0, 0); // unused
    }

    /**
     * Do the layout
     * @param target component to be laid out
     */
    public void layoutContainer(Container target) {
        final int EDGE_WD = 7;
        synchronized (target.getTreeLock()) {
            if (splitPane == null || edgePanel == null) {
                return;
            }
            Dimension targetSize = target.getSize();
            Insets insets = target.getInsets();
            int width = targetSize.width - insets.left - insets.right;
            int height = targetSize.height - insets.top - insets.bottom;

            // Initialize to values for SOUTH position
            int edgeX = insets.left;
            int edgeY = insets.top;
            int edgeWd = width;
            int edgeHt = EDGE_WD;
            int panX = insets.left;
            int panY = insets.top;
            int panWd = width;
            int panHt = height;
            int panelPos = ((ExpPanel)vnmrIf).getSplitPosition();
            if (panelPos == VSeparator.NORTH) {
                panHt -= EDGE_WD;
                edgeY += height - EDGE_WD;
            } else if (panelPos == VSeparator.WEST) {
                panWd -= EDGE_WD;
                edgeWd = EDGE_WD;
                edgeHt = height;
                edgeX += width - EDGE_WD;
            } else if (panelPos == VSeparator.EAST) {
                panWd -= EDGE_WD;
                panX += EDGE_WD;
                edgeWd = EDGE_WD;
                edgeHt = height;
            } else if (panelPos == VSeparator.SOUTH) {
                panY += EDGE_WD;
                panHt -= EDGE_WD;
            }
            edgePanel.setBounds(edgeX, edgeY, edgeWd, edgeHt);
            splitPane.setBounds(panX, panY, panWd, panHt);
        }
    }

}

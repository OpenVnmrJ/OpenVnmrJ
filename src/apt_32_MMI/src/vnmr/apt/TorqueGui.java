/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;

import vnmr.bo.Plot;
import vnmr.bo.PlotPoint;
import vnmr.util.VButtonBorder;



public class TorqueGui extends ProbeTuneGui {

    private Plot m_torquePlot;


    public TorqueGui(String probeName, String usrProbeName,
                     String sysTuneDir, String usrTuneDir,
                     String host, int port, String console) {

        super(probeName, usrProbeName, null, sysTuneDir,
              usrTuneDir, host, port, console);
        setTitle("Torque Measurement");
        initPlot();
        //Dimension dim = getPreferredSize();
        //dim.width += 500;
        //setSize(dim);
        Dimension pdim = m_torquePlot.getSize();
        Messages.postDebug("Plot size: " + pdim.width + "x" + pdim.height);/*CMP*/

        Messages.postDebug("Created TorqueGui");
    }


    /**
     *
     */
    protected void initializeGui() {
        // Build GUI
        JPanel leftPanel;
        //JPanel motorButtonArea;
        Box motorButtonArea;
        String tooltip;

        cmdAbort.setFocusable(false);
        cmdTuneFreq.setFocusable(false);
        cmdRfChan.setFocusable(false);
        cmdMatchThresh.setFocusable(false);
        cmdStepTune.setFocusable(false);
        cmdSetCenter.setFocusable(false);
        cmdSetSpan.setFocusable(false);
//        cmdSetNumberOfPoints.setFocusable(false);
        //cmdSetPredefinedTune.setFocusable(false);
        //cmdSetPlotType.setFocusable(false);

        cmdInit.setFocusable(false);
        cmdInit.setActionCommand("Initialize");
        tooltip = ("<html>Move the motors to determinine the sensitivities<br>"
                   + "to the \"tune\" and \"match\" capacitors");
        cmdInit.setToolTipText(tooltip);

        cmdRfcal.setFocusable(false);
        cmdRfcal.setActionCommand("RfCal");
        tooltip = ("<html>Calibrate the RF reflection measurements<br>"
                   + "against a \"short\", \"open\", and \"load\".");
        cmdRfcal.setToolTipText(tooltip);

        cmdInfo.setFocusable(false);
        pnlInfo.add(lblInfo);
        pnlInfo.setBorder(BorderFactory.createEtchedBorder());

        leftPanel = new JPanel();
        leftPanel.setLayout(new GridBagLayout());
        GridBagConstraints c = new GridBagConstraints();
        //c.fill = GridBagConstraints.NONE;
        c.fill = GridBagConstraints.HORIZONTAL;
        //c.insets = new Insets(2, 4, 2, 4);
        c.insets = new Insets(1, 1, 1, 1);
        //leftPanel.add(cmdSetStart);
        //leftPanel.add(cmdSetStop);
        int row;
        c.gridx = 0;
        c.gridy = row = 0;

        c.gridwidth = 4;
        leftPanel.add(motorMsg, c);
        c.gridy = ++row;
        int doubleColumnStart = row;

        //((FlowLayout)pnlCmd.getLayout()).setAlignment(FlowLayout.LEFT);
        pnlCmd.setLayout(new BorderLayout(5, 10));
        pnlCmd.setBorder(BorderFactory.createEmptyBorder(5, 0, 10, 0));
        pnlCmd.add(lblCmd, BorderLayout.WEST);
        pnlCmd.add(txtCmd, BorderLayout.CENTER);
        leftPanel.add(pnlCmd, c);
        c.gridy = ++row;

        JPanel pnlIndex = new JPanel();
        pnlIndex.setLayout(new FlowLayout(FlowLayout.LEFT));
        pnlIndex.setBorder(BorderFactory.createEmptyBorder(0, 0, 10, 0));
        //pnlIndex.setBackground(Color.cyan)/*CMP*/;
        pnlIndex.add(new JLabel("Index: "));
        JButton button;
        for (int i = 0; i < 7; i++) {
            button = new JButton(String.valueOf(i));
            button.setBorder(new VButtonBorder());
            button.setActionCommand("motor step " + i + " -400000");
            button.addActionListener(this);
            pnlIndex.add(button);
        }
        leftPanel.add(pnlIndex, c);
        c.gridy = ++row;

        JPanel pnlErase = new JPanel();
        pnlErase.setLayout(new FlowLayout(FlowLayout.LEFT));
        pnlErase.setBorder(BorderFactory.createEmptyBorder(0, 0, 10, 0));
        //pnlErase.setBackground(Color.cyan)/*CMP*/;
        pnlErase.add(new JLabel("Erase: "));
        for (int i = 0; i < 7; i++) {
            button = new JButton(String.valueOf(i));
            button.setBorder(new VButtonBorder());
            button.setActionCommand("Gui EraseTrace " + i);
            button.addActionListener(this);
            pnlErase.add(button);
        }
        leftPanel.add(pnlErase, c);
        c.gridy = ++row;

        c.gridwidth = 1;
        leftPanel.add(cmdInfo, c);
        c.gridx = 1;
        c.gridwidth = 2;
        leftPanel.add(cmdQuit, c);
        c.gridx = 3;
        c.gridwidth = 1;
        leftPanel.add(cmdAbort, c);
        cmdAbort.setForeground(Color.RED);
        c.gridy = ++row;

        c.gridx = 0;
        c.gridwidth = 4;
        JPanel tp = new JPanel(new GridLayout(1,1), false);
        tp.setBorder(BorderFactory.createEmptyBorder(5, 2, 5, 5));
        tp.add(pnlStatus);
        pnlStatus.setBorder(BorderFactory.createLoweredBevelBorder());
        pnlStatus.add(lblStatus);
        leftPanel.add(tp, c);
        displayStatus(STATUS_INITIALIZE);

        //rightPanel = new VerticalPanel();
        //rightPanel.add(txtStart);
        //rightPanel.add(txtStop);
        c.gridx = 1;
        c.gridy = row = doubleColumnStart;

        motorButtonArea = Box.createHorizontalBox();
        int[] motors = {0, 1, 2, 3, 4, 5, 6};
        for (int i : motors) {
            MotorPanel motorPanel = new MotorPanel(i, false);
            motorButtonArea.add(motorPanel);
            m_motorPanels.add(motorPanel);
        }

        // Use dummy north panel to force good width of plot
        JPanel dummy = new JPanel();
        dummy.setPreferredSize(new Dimension(780, 1));
        getContentPane().add(dummy, "North");

        getContentPane().add(leftPanel, "West");

        getContentPane().add(plotPane, "Center");
        plotPane.setLayout(new BorderLayout());
        showPlot();

        getContentPane().add(motorButtonArea, "South");

        // Initialize the interface values
        //m_startFreq = 280e6;
        //m_stopFreq = 320e6;

        //getNP();
        //getStartFreq();
        //getStopFreq();

            // Initialize input fields
            //String strCenter = "";
            //String strSpan = "";
            //try {
            //    double stopFreq = Double.parseDouble(strStopFreq);
            //    double startFreq = Double.parseDouble(strStartFreq);
            //    double center = (startFreq + stopFreq) / 2;
            //    double span = stopFreq - startFreq;
            //    strCenter = "" + center;
            //    strSpan = "" + span;
            //} catch (NumberFormatException nfe) {
            //}
            //txtNumberOfPoints.setText(strNp);
            //txtCenter.setText(strCenter);
            //txtSpan.setText(strSpan);
        //getData(); // Initialize plot

        cbChName.addActionListener(this);
        cbChName.setActionCommand(CHAN_NAME);
        cbChName.setEditable(false);
        txtTuneFreq.addActionListener(this);
        txtTuneFreq.setActionCommand(TUNE_FREQ);
        txtMatchThresh.addActionListener(this);
        txtMatchThresh.setActionCommand(MATCH_THRESH);
        txtStepTune.addActionListener(this);
        txtStepTune.setActionCommand(TRACK_TUNE);
        txtCenter.addActionListener(this);
        txtCenter.setActionCommand(CENTER);
        txtSpan.addActionListener(this);
        txtSpan.setActionCommand(SPAN);
//        txtNumberOfPoints.addActionListener(this);
//        txtNumberOfPoints.setActionCommand(NUMBER_PTS);
        cbPredefinedTune.addActionListener(this);
        cbPredefinedTune.setActionCommand(PRE_TARGET);
        cbPredefinedTune.setEditable(true);
        txtCmd.addActionListener(this);
        txtCmd.setActionCommand(COMMAND);
        //cmdSavePhase.addActionListener(this);
        //cmdSavePhase.setActionCommand(SAVE_PHASE);
        cmdPhase0.addActionListener(this);
        cmdPhase0.setActionCommand(PHASE0);
        cmdPhase1.addActionListener(this);
        cmdPhase1.setActionCommand(PHASE1);
        cmdPhase2.addActionListener(this);
        cmdPhase2.setActionCommand(PHASE2);
        cmdPhase3.addActionListener(this);
        cmdPhase3.setActionCommand(PHASE3);
        cmdPhaseAuto.addActionListener(this);
        cmdPhaseAuto.setActionCommand(PHASE_AUTO);
        txtProbeDelay.addActionListener(this);
        txtProbeDelay.setActionCommand(DELAY);

        cmdAbort.addActionListener(this);
        cmdTuneFreq.addActionListener(this);
        cmdMatchThresh.addActionListener(this);
        cmdStepTune.addActionListener(this);
        cmdSetCenter.addActionListener(this);
        cmdSetSpan.addActionListener(this);
//        cmdSetNumberOfPoints.addActionListener(this);
        cmdSetPredefinedTune.addActionListener(this);
        cmdGoPredefinedTune.addActionListener(this);
        cmdInit.addActionListener(this);
        cmdRfcal.addActionListener(this);
        cmdInfo.addActionListener(this);
        cmdQuit.addActionListener(this);

        cmdModeOn.setSelected(true);
        cmdModeOff.setSelected(false);
    }

    private void initPlot() {
        m_torquePlot = new Plot();
        m_torquePlot.clearLegends();
        m_torquePlot.setGrid(true);
        m_torquePlot.setGridColor(GRID_COLOR);
        m_torquePlot.setPlotBackground(CANVAS_COLOR);
        m_torquePlot.setXAxis(true);
        m_torquePlot.setYAxis(true);
        m_torquePlot.setXLabel("Position (steps)");
        //m_torquePlot.setYLabel("Torque (in\u22c5oz)");
        m_torquePlot.setYLabel("Torque)");
        m_torquePlot.setFillButton(true);
        m_torquePlot.setWalking(true);
        for (int i = 0; i < 7; i++) {
            m_torquePlot.addLegendButton(i, i, String.valueOf(i));
        }
        Container container = getPlotContainer();
        container.add(m_torquePlot, "Center");
        container.remove(absPlot);
        container.remove(polarPlot);
    }

    /**
     * @param mark The name of the desired mark.
     */
    protected void setMarksStyle(String mark) {
        m_torquePlot.setMarksStyle(mark);
    }


    protected void addTorqueData(StringTokenizer toker) {
        try {
            int dataset = Integer.parseInt(toker.nextToken());
            double position = Integer.parseInt(toker.nextToken());
            double torque = Double.parseDouble(toker.nextToken());
            boolean connected = true;
            if (m_torquePlot != null) {
                m_torquePlot.addPoint(dataset, position, torque, connected);
            }
            //m_torquePlot.fillPlot();
            Messages.postDebug("TorqueData","added torque point: "
                               + dataset + ", " + position + ", " + torque);
        } catch (NumberFormatException nfe) {

        }
    }

    protected void shiftTorqueData(StringTokenizer toker) {
        try {
            int dataset = Integer.parseInt(toker.nextToken());
            double shift = Integer.parseInt(toker.nextToken());
            Messages.postDebug("shiftTorqueData: " + dataset + ", " + shift);
            ArrayList<ArrayList<PlotPoint>> data = m_torquePlot.getPoints();
            List<PlotPoint> points = data.get(dataset);
            for (PlotPoint point : points) {
                point.x -= shift;
            }
            m_torquePlot.setPoints(data);
            Messages.postDebug("TorqueData","shifted torque points: "
                               + dataset + ", " + shift);
        } catch (NumberFormatException nfe) {

        }
    }

    protected void setData(String cmd) {
        Messages.postDebug("setData");
    }

    public void exec(String cmd) {
        StringTokenizer toker = new StringTokenizer(cmd, " ,\t");
        String key = "";
        if (toker.hasMoreTokens()) {
            key = toker.nextToken();
        }
        Messages.postDebug("GuiCommands", "TorqueGui.exec(\"" + key + " ...\")");

        if (key.equalsIgnoreCase("EraseTrace")) {
            // This erases the given trace data from the torque display
            try {
                int dataset = Integer.parseInt(toker.nextToken());
                m_torquePlot.clear(dataset);
            } catch (IndexOutOfBoundsException ioobe) {
            } catch (NumberFormatException nfe) {
            }
        } else {
            super.exec(cmd);
        }

    }
}

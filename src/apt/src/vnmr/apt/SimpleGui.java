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
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.event.WindowEvent;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;

import vnmr.util.Fmt;


public class SimpleGui extends ProbeTuneGui {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    protected JLabel lblProbeName;
    //protected JLabel lblChannelNumber;

    public SimpleGui(String probeName, String usrProbeName,
                     String sysTuneDir, String usrTuneDir,
                     String host, int port, String console) {

        super(probeName, usrProbeName, null,
              sysTuneDir, usrTuneDir, host, port, console);
        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
    }

    public void windowClosing(WindowEvent e) {
        dispose();
    }

    public void displayChannel(String name) {
        String[] toks = name.split("/chan#", 2);
        if (toks.length == 2) {
            lblProbeName.setText("Probe: " + toks[0]);
            //lblChannelNumber.setText("Channel # " + toks[1]);
        }
    }

    protected void displayDipFreq(double freq_hz) {
        if (Double.isNaN(freq_hz)) {
            txtFreq.setText("Dip Frequency: --");
        } else {
            txtFreq.setText("Dip Frequency: " + Fmt.f(3, freq_hz / 1.e6)
                            + " MHz");
        }
    }

    protected void displayRefl(double refl) {
        if (Double.isNaN(refl)) {
            txtReflection.setText("Dip Minimum: --");
        } else {
            double refl_db = 20 * Math.log10(Math.abs(refl));
            txtReflection.setText("Dip Minimum: " 
                                  + Fmt.f(1, refl_db) + " dB");
        }
    }

    protected void initializeGui() {
        // Build GUI
        JPanel leftPanel;
        Box motorButtonArea;
        lblProbeName = new JLabel();
        //lblChannelNumber = new JLabel();

        leftPanel = new JPanel();
        leftPanel.setLayout(new GridBagLayout());
        GridBagConstraints c = new GridBagConstraints();
        c.fill = GridBagConstraints.HORIZONTAL;
        c.insets = new Insets(1, 1, 1, 1);
        int row;
        c.gridx = 0;
        c.gridy = row = 0;

        c.gridwidth = 4;
        leftPanel.add(lblProbeName, c);
        c.gridy = ++row;
        //leftPanel.add(lblChannelNumber, c);
        //c.gridy = ++row;
        leftPanel.add(motorMsg, c);
        c.gridy = ++row;
        leftPanel.add(sweepMsg, c);
        c.gridy = ++row;

        c.gridx = 0;
        c.gridwidth = 4;
        JPanel tp = new JPanel(new GridLayout(1,1), false);
        tp.setBorder(BorderFactory.createEmptyBorder(5, 2, 5, 5));
        tp.add(pnlStatus);
        pnlStatus.setBorder(BorderFactory.createLoweredBevelBorder());
        pnlStatus.add(lblStatus);
        pnlStatus.setPreferredSize(new Dimension(210, 30));
        leftPanel.add(tp, c);
        displayStatus(STATUS_INITIALIZE);
        c.gridy = ++row;

//        leftPanel.add(cbChName, c);
//        c.gridy = ++row;

        c.gridwidth = 3;

        leftPanel.add(txtFreq, c);
        c.gridy = ++row;
        leftPanel.add(txtReflection, c);
        c.gridy = ++row;
        leftPanel.add(txtTargetMatch, c);
        c.gridy = ++row;

        c.gridwidth = 1;
        cmdQuit.setText("Close");
        cmdQuit.setActionCommand(CLOSE);
        leftPanel.add(cmdQuit, c);
        c.gridy = ++row;

        absPlot.setBackground(null);
        setPlot("normal");

        motorButtonArea = Box.createHorizontalBox();
        int[] motors = {0, 1, 2, 3, 4, 5, 6};
        for (int i : motors) {
            MotorPanel motorPanel = new MotorPanel(i, true);
            motorPanel.setVisible(false);
            motorButtonArea.add(motorPanel);
            m_motorPanels.add(motorPanel);
        }

//        // Use dummy north panel to force good width of plot
//        JPanel dummy = new JPanel();
//        dummy.setPreferredSize(new Dimension(780, 1));
//        getContentPane().add(dummy, "North");

        // Use dummy east panel to force good height of plot
        JPanel dummy2 = new JPanel();
        dummy2.setPreferredSize(new Dimension(1, 400));
        getContentPane().add(dummy2, "East");

        getContentPane().add(leftPanel, "West");

        getContentPane().add(plotPane, "Center");
        plotPane.setLayout(new BorderLayout());
        plotPane.add(northPane, "North");
        showPlot();

        getContentPane().add(motorButtonArea, "South");

        cbChName.addActionListener(this);
        cbChName.setActionCommand(CHAN_NAME);
        cbChName.setEditable(false);
        cmdQuit.addActionListener(this);

        cmdModeOff.setSelected(false);
    }

}
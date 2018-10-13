/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.apt;


import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.io.BufferedReader;
import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.ButtonGroup;
import javax.swing.Icon;
import javax.swing.JButton;
import javax.swing.JFormattedTextField;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JRadioButton;
import javax.swing.JTextField;

import vnmr.util.Fmt;
import vnmr.wizard.JWizard;
import vnmr.wizard.JWizard.PageType;
import vnmr.wizard.event.WizardEvent;
import vnmr.wizard.event.WizardListener;
import vnmr.wizard.navigator.Navigator;


public class MtuneCal implements WizardListener, ActionListener, FocusListener {

    private static final String FREQ_SELECT_PANEL = "FreqSelectPanel";
    private static final String CAL_OPEN_PANEL = "CalOpenPanel";
    private static final String CAL_LOAD_PANEL = "CalLoadPanel";
    private static final String CAL_SHORT_PANEL = "CalShortPanel";
    private static final String CAL_PROBE_PANEL = "CalProbePanel";
    private static final String CAL_OPEN_DATA_PANEL = "CalOpenDataPanel";
    private static final String CAL_LOAD_DATA_PANEL = "CalLoadDataPanel";
    private static final String CAL_SHORT_DATA_PANEL = "CalShortDataPanel";
    private static final String CAL_PROBE_DATA_PANEL = "CalProbeDataPanel";
    private static final String FINISH_PANEL = "FinishPanel";
    private static final String NEXT_MESSAGE = "To continue click \"Next\"";

    private static final String COLLECT_OPEN_DATA = "CollectOpenData";
    private static final String COLLECT_LOAD_DATA = "CollectLoadData";
    private static final String COLLECT_SHORT_DATA = "CollectShortData";
    private static final String COLLECT_PROBE_DATA = "CollectProbeData";
    private static final String DELETE_CAL = "DeleteCalFile";
    private static final String CUSTOM_BAND = "CustomBand";

    /** A regex that matches any calibration file. */
    private static final RegexFileFilter CAL_NAME_FILE_FILTER
        = new RegexFileFilter(MtuneControl.getCalNameTemplate(null, null,
                                                              null, null,
                                                              null));

    protected ProbeTuneGui m_gui;
    private String m_probeName;
    private ArrayList<JPanel> m_wizPanels = new ArrayList<JPanel>();
    //private ArrayList<FreqBand> m_freqBands = new ArrayList<FreqBand>();
    private SortedSet<CalBand> m_calBands = null;
    private Map<CalBand,JRadioButton> m_bandButtons
            = new HashMap<CalBand, JRadioButton>();
    private CalBand m_band = null;
    private JFormattedTextField m_customStart;
    private JFormattedTextField m_customStop;
    private JFormattedTextField m_customRfchan;
    private JTextField m_customName;
    private JTextField m_customPortName;
    //private JLabel m_dataLabel;
    private JWizard m_wizard;
    private Icon m_calIcon;
    private Icon m_openIcon;
    private Icon m_loadIcon;
    private Icon m_shortIcon;
    private Icon m_probeIcon;
    private JRadioButton m_customButton;


    public MtuneCal(ProbeTuneGui parent, String probeName) {
        m_gui = parent;
        m_probeName = probeName;
        m_gui.setCalPopup(this);

        //setFreqBands();
        m_calBands = MtuneCal.getCalBands(probeName);

        Class<?> cl = getClass();
        m_calIcon = TuneUtilities.getIcon("Calibration.png", cl);
        m_openIcon = TuneUtilities.getIcon("Open.png", cl);
        m_loadIcon = TuneUtilities.getIcon("Load.png", cl);
        m_shortIcon = TuneUtilities.getIcon("Short.png", cl);
        m_probeIcon = TuneUtilities.getIcon("Probe.png", cl);
        m_wizard = new JWizard("ProTune RF Calibration",
                               parent, m_calIcon);
        m_wizard.setNavigator(new CalNavigator());
        JPanel wPanel;
        int idx = -1;

        wPanel = JWizard.getWizardPanel(null);
        populateFreqSelectPanel(wPanel);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Choose band to calibrate", FREQ_SELECT_PANEL,
                               wPanel, PageType.FIRST);

        // The following panels are created empty
        // and populated only when they appear
        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrate Open", CAL_OPEN_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrating Open", CAL_OPEN_DATA_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrate Load", CAL_LOAD_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrating Load", CAL_LOAD_DATA_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrate Short", CAL_SHORT_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrating Short", CAL_SHORT_DATA_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrate Probe", CAL_PROBE_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibrating Probe", CAL_PROBE_DATA_PANEL,
                               m_wizPanels.get(idx), PageType.NORMAL);

        wPanel = JWizard.getWizardPanel(null);
        m_wizPanels.add(wPanel);
        idx++;
        m_wizard.addCustomPanel( "Calibration Complete", FINISH_PANEL,
                               m_wizPanels.get(idx), PageType.LAST);

//        wPanel = JWizard.getWizardPanel();
//        m_wizPanels.add(wPanel);
//        idx++;
//        m_wizard.addCustomPanel( "Dummy Panel", FINISH_PANEL,
//                               m_wizPanels.get(idx), PageType.LAST);

        m_wizard.addWizardListener( this );
        m_wizard.showWizard();
    }

    private void populateFreqSelectPanel(JPanel wPanel) {
        wPanel.removeAll();
        wPanel.setLayout(new BoxLayout(wPanel, BoxLayout.Y_AXIS));

        JPanel radioPanel = new JPanel();
        radioPanel.setLayout(new BoxLayout(radioPanel, BoxLayout.Y_AXIS));
        radioPanel.setBackground(null);
        wPanel.add(radioPanel);

        // See which band should be selected by default
        CalBand selectedBand = null;
        long date = Long.MAX_VALUE;
        for (CalBand band : m_calBands) {
            if (band.getFileDate() < date) {
                date = band.getFileDate();
                selectedBand = band;
            }
        }

        ButtonGroup group = new ButtonGroup();
        for (CalBand band : m_calBands) {
            JRadioButton radioButton = new JRadioButton(band.getLabel());
            String tooltip = "<html><table><tr>"
                    + "<td width=200>Name: frequency range</td>"
                    + "<td width=90>RF channel</td>"
                    + "<td width=110>Calibration date</td>"
                    + "<td>Type<br>G=generic<br>P=probe-specific<br>C=custom</td>"
                    ;
            radioButton.setToolTipText(tooltip);
            radioButton.setBackground(null);
            radioButton.setAlignmentX(Component.LEFT_ALIGNMENT);
            radioButton.setActionCommand("BAND_" + band.getSpec());
            radioButton.addActionListener(this);
            group.add(radioButton);
            radioPanel.add(radioButton);
            m_bandButtons.put(band, radioButton);
            if (band == selectedBand) {
                radioButton.setSelected(true);
                m_band = band;
            }
        }
        CalBand band = new CalBand("0 0 0 \"Custom\"");
        m_customButton = new JRadioButton(band.getLabel());
        m_customButton.setBackground(null);
        m_customButton.setAlignmentX(Component.LEFT_ALIGNMENT);
        m_customButton.setActionCommand(CUSTOM_BAND);
        m_customButton.addActionListener(this);
        group.add(m_customButton);
        radioPanel.add(m_customButton);
        m_bandButtons.put(band, m_customButton);
        if (selectedBand == null) {
            m_customButton.setSelected(true);
        }

        JPanel customPanel = new JPanel();
        customPanel.setLayout(new BoxLayout(customPanel, BoxLayout.LINE_AXIS));
        customPanel.setBackground(null);
        customPanel.setAlignmentX(Component.LEFT_ALIGNMENT);
        customPanel.setBorder(BorderFactory.createMatteBorder(0, 25, 30, 0,
                                                              (Color)null));
        //customPanel.setMaximumSize(new Dimension(400, 20));
        radioPanel.add(customPanel);

        JLabel label = new JLabel("Name");
        String tooltip = "<html>Name of the calibration band, e.g.<br>"
                + "Low Band or 19F Band";
        label.setToolTipText(tooltip);
        customPanel.add(label);
        m_customName = new JTextField(8);
        m_customName.setToolTipText(tooltip);
        m_customName.addFocusListener(this);
        m_customName.setActionCommand(CUSTOM_BAND);
        m_customName.addActionListener(this);
        //m_customName.setMaximumSize(new Dimension(40, 20));
        customPanel.add(m_customName);
        customPanel.add(new JLabel(" Start"));
        NumberFormat amountFormat = NumberFormat.getNumberInstance();
        m_customStart = new JFormattedTextField(amountFormat);
        m_customStart.addFocusListener(this);
        m_customStart.setActionCommand(CUSTOM_BAND);
        m_customStart.addActionListener(this);
        //m_customStart.setMaximumSize(new Dimension(35, 20));
        m_customStart.setColumns(4);
        customPanel.add(m_customStart);

        customPanel.add(new JLabel(" Stop"));
        m_customStop = new JFormattedTextField(amountFormat);
        m_customStop.addFocusListener(this);
        m_customStop.setActionCommand(CUSTOM_BAND);
        m_customStop.addActionListener(this);
        //m_customStop.setMaximumSize(new Dimension(35, 20));
        m_customStop.setColumns(4);
        customPanel.add(m_customStop);

        customPanel.add(new JLabel(" RF Chan"));
        m_customRfchan = new JFormattedTextField(amountFormat);
        m_customRfchan.addFocusListener(this);
        m_customRfchan.setActionCommand(CUSTOM_BAND);
        m_customRfchan.addActionListener(this);
        m_customRfchan.setColumns(2);
        customPanel.add(m_customRfchan);

        label = new JLabel(" Port Name");
        tooltip = "<html>Name of the probe port for this band, e.g.<br>"
                + "X-CHANNEL or 19F/1H<br>"
                + "(defaults to the band name)";
        label.setToolTipText(tooltip);
        customPanel.add(label);
        m_customPortName = new JTextField(8);
        m_customPortName.addFocusListener(this);
        m_customPortName.setActionCommand(CUSTOM_BAND);
        m_customPortName.addActionListener(this);
        m_customPortName.setToolTipText(tooltip);
        //m_customPortName.setMaximumSize(new Dimension(40, 20));
        customPanel.add(m_customPortName);
        customPanel.add(Box.createHorizontalGlue());


//        JPanel buttonPanel = new JPanel();
//        buttonPanel.setBorder(BorderFactory.createLineBorder(null, 10));
//        buttonPanel.setBackground(null);
//        wPanel.add(buttonPanel);

        JButton button = new JButton("Delete Selected Calibration...");
        button.setAlignmentX(Component.LEFT_ALIGNMENT);
        button.setActionCommand(DELETE_CAL);
        button.addActionListener(this);
        wPanel.add(button);

    }

    private void populatePanel(String name) {
        String msg;
        if (FREQ_SELECT_PANEL.equals(name)) {
            m_wizard.setIcon(m_calIcon);
            m_wizard.setMessage(NEXT_MESSAGE);
        } else if (CAL_OPEN_PANEL.equals(name)) {
            msg = "<html>Disconnect \"" + m_band.getPortName()
                    + "\" RF cable at probe,"
                    + "<br>and leave cable end OPEN...";
            populateCalPanel(COLLECT_OPEN_DATA, msg, m_openIcon);
        } else if (CAL_OPEN_DATA_PANEL.equals(name)) {
            populateDataPanel(m_openIcon);
        } else if (CAL_LOAD_PANEL.equals(name)) {
            msg = "<html>Connect 50 \u03a9 LOAD to end of \""
                    + m_band.getPortName()
                    + "\" RF cable...";
            populateCalPanel(COLLECT_LOAD_DATA, msg, m_loadIcon);
        } else if (CAL_LOAD_DATA_PANEL.equals(name)) {
            populateDataPanel(m_loadIcon);
        } else if (CAL_SHORT_PANEL.equals(name)) {
            msg = "<html>Connect SHORT to end of \""
                    + m_band.getPortName()
                    + "\" RF cable...";
            populateCalPanel(COLLECT_SHORT_DATA, msg, m_shortIcon);
        } else if (CAL_SHORT_DATA_PANEL.equals(name)) {
            populateDataPanel(m_shortIcon);
        } else if (CAL_PROBE_PANEL.equals(name)) {
            msg = "<html>Reconnect cable to \""
                    + m_band.getPortName()
                    + "\" probe port...";
            populateCalPanel(COLLECT_PROBE_DATA, msg, m_probeIcon);
        } else if (CAL_PROBE_DATA_PANEL.equals(name)) {
            populateDataPanel(m_probeIcon);
        } else if (FINISH_PANEL.equals(name)) {
            populateFinishPanel();
        }
    }

    private void populateCalPanel(String cmd, String msg, Icon icon) {
        int idx = m_wizard.getCurrentScreenIndex();
        JPanel calPanel = m_wizPanels.get(idx);
        calPanel.removeAll();
        calPanel.setLayout(new BoxLayout(calPanel, BoxLayout.Y_AXIS));

        calPanel.add(Box.createVerticalGlue());
        JLabel label;

//        JPanel panel0= new JPanel();
//        panel0.setBackground(null);
//        label = new JLabel("<html>Calibrating RF reflection on band \""
//                                  + m_band.getName() + "\".");
//        panel0.add(label);
//        calPanel.add(panel0);

        JPanel panel1 = new JPanel();
        panel1.setBackground(null);
        label = new JLabel(msg);
        panel1.add(label);
        calPanel.add(panel1);

        JPanel panel2 = new JPanel();
        panel2.setBackground(null);
        label = new JLabel("<html>...then, click \"Next\" when ready:");
        panel2.add(label);
        calPanel.add(panel2);

        m_wizard.setIcon(icon);
        m_wizard.setMessage("");
        m_wizard.setNextEnabled(true);
        m_wizard.setBackEnabled(true);
    }

    private void populateDataPanel(Icon icon) {
        int idx = m_wizard.getCurrentScreenIndex();
        JPanel calPanel = m_wizPanels.get(idx);
        calPanel.removeAll();
        calPanel.setLayout(new BoxLayout(calPanel, BoxLayout.Y_AXIS));

        calPanel.add(Box.createVerticalGlue());

        JPanel panel1 = new JPanel();
        panel1.setBackground(null);
        String msg = "<html>Collecting calibration data for \""
                + m_band.getName() + "\" ...";
        JLabel label = new JLabel(msg);
        panel1.add(label);
        calPanel.add(panel1);

        // TODO: Update progress bar with each completed scan?
        JProgressBar progress = new JProgressBar();
        progress.setIndeterminate(true);
        progress.setMaximumSize(new Dimension(200, 50));
        calPanel.add(progress);

        //JPanel panel2 = new JPanel();
        //panel2.setBackground(null);
        //label = new JLabel("<html>...then, click \"Next\" when ready:");
        //panel2.add(label);
        //calPanel.add(panel2);

        m_wizard.setIcon(icon);
        m_wizard.setMessage("");
        m_wizard.setNextEnabled(false);
        m_wizard.setBackEnabled(false);
    }

    private void populateFinishPanel() {
        int idx = m_wizard.getCurrentScreenIndex();
        JPanel finPanel = m_wizPanels.get(idx);
        finPanel.removeAll();
        JLabel label = new JLabel("<html>Click \"Finish\""
                                  + " to save calibration for "
                                  + "<br>" + m_band.getShortLabel());
        JPanel panel = new JPanel();
        panel.setBackground(null);
        panel.setLayout(new FlowLayout());
        panel.add(label);
        finPanel.setLayout(new BoxLayout(finPanel, BoxLayout.Y_AXIS));
        finPanel.add(Box.createVerticalGlue());
        finPanel.add(panel);

        String overlapMsg = getOverlapCalMessage(m_probeName,
                                                 m_band.getStart(),
                                                 m_band.getStop(),
                                                 m_band.getRfchan());
        if (overlapMsg != null) {
            JPanel panel2 = new JPanel();
            panel2.setBackground(null);
            panel2.setLayout(new FlowLayout());
            panel2.add(new JLabel(overlapMsg));
            finPanel.add(Box.createVerticalGlue());
            finPanel.add(panel2);
        }
        m_wizard.setIcon(m_calIcon);
        m_wizard.setMessage("");
        m_wizard.setNextEnabled(true);
        m_wizard.setBackEnabled(true);
    }

    public static String getOverlapCalMessage(String probeName,
                                              double start, double stop,
                                              int rfchan) {

        SortedSet<CalBand> overlaps = getOverlappingCalBands(probeName,
                                                             start, stop,
                                                             rfchan);
        String msg = null;
        if (!overlaps.isEmpty()) {
            msg = "<html>"
                + "The following existing calibrations overlap this range:"
                + "<br>";
            for (CalBand band : overlaps) {
                msg += band.getShortLabel() + "<br>";
            }
        }
        return msg;
    }

    public static SortedSet<CalBand> getOverlappingCalBands(String probeName,
                                                            double start,
                                                            double stop,
                                                            int rfchan) {
        // Set of all existing calibration files for this probe and rfchannel:
        SortedSet<CalBand> bands = getExistingCalBands(probeName, rfchan);
        Iterator<CalBand> itr = bands.iterator();
        while (itr.hasNext()) {
            CalBand band = itr.next();
            double f0 = band.getStart();
            double f1 = band.getStop();
            if (f0 > stop || f1 < start || (f0 == start && f1 == stop)) {
                // This one doesn't overlap
                itr.remove();
            }
        }
        return bands;
    }

    /**
     * This is called when the wizard's Cancel button is clicked.
     * @param we The event that triggered this call.
     */
    public void wizardCancelled(WizardEvent we) {
        m_gui.setCalPopup(null);
        // NB: This reseting of the correct mode is unreliable
        //m_gui.sendCommand("restoreRawDataMode");
        m_gui.sendCommand("setRawDataMode 0");
    }

    /**
     * This is called when the wizard's Finish button is clicked.
     * @param we The event that triggered this call.
     */
    public void wizardComplete(WizardEvent we) {
        m_gui.sendCommand("SaveAllCalData " + m_band.getSpec());
        m_gui.setCalPopup(null);
        // NB: This reseting of the correct mode is unreliable
        //m_gui.sendCommand("restoreRawDataMode");
        m_gui.sendCommand("setRawDataMode 0");
   }

    /**
     * This is called just before the screen changes.
     * @param we The event that triggered this call.
     */
    public void wizardScreenChanging(WizardEvent we) {
        JWizard wiz = (JWizard)we.getSource();
        String name = wiz.getCurrentScreenName();
        if (we.getTrigger() == "next") {
            if (FREQ_SELECT_PANEL.equals(name)) {
                m_gui.sendCommand("saveRawDataMode");
                m_gui.sendCommand("setRawDataMode 1");
                m_gui.sendCommand("setCalSweep " + (m_band.getStart() / 1e6)
                                  + " " + (m_band.getStop() / 1e6)
                                  + " " + m_band.getRfchan());
            } else if (CAL_OPEN_PANEL.equals(name)) {
                //m_wizard.setNextEnabled(false);
                m_gui.sendCommand("CollectCalData open");
            } else if (CAL_LOAD_PANEL.equals(name)) {
                //m_wizard.setNextEnabled(false);
                m_gui.sendCommand("CollectCalData load");
            } else if (CAL_SHORT_PANEL.equals(name)) {
                //m_wizard.setNextEnabled(false);
                m_gui.sendCommand("CollectCalData short");
            } else if (CAL_PROBE_PANEL.equals(name)) {
                //m_wizard.setNextEnabled(false);
                m_gui.sendCommand("CollectCalData probe");
            }
        }
    }

    /**
     * This is called just after the screen changes.
     * @param we The event that triggered this call.
     */
    public void wizardScreenChanged(WizardEvent we) {
        JWizard wiz = (JWizard)we.getSource();
        String name = wiz.getCurrentScreenName();
        populatePanel(name);
        if (we.getTrigger() != "next") {
            m_gui.sendCommand("abort");
        }
    }

    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();

        // If we selected a cal band, remember it
        for (CalBand band : m_calBands) {
            String spec = band.getSpec();
            if (cmd.equals("BAND_" + spec)) {
                m_band = band;
                m_wizard.setNextEnabled(true);
                return;
            }
        }

        // "cmd" is not a band name - deal with other commands
        if (CUSTOM_BAND.equals(cmd)) {
            validateCustomBand();
        } else if (COLLECT_OPEN_DATA.equals(cmd)) {
            JButton button = (JButton)e.getSource();
            button.setEnabled(false);
            m_gui.sendCommand("CollectCalData open");
        } else if (COLLECT_LOAD_DATA.equals(cmd)) {
            JButton button = (JButton)e.getSource();
            button.setEnabled(false);
            m_gui.sendCommand("CollectCalData load");
        } else if (COLLECT_SHORT_DATA.equals(cmd)) {
            JButton button = (JButton)e.getSource();
            button.setEnabled(false);
            m_gui.sendCommand("CollectCalData short");
        } else if (COLLECT_PROBE_DATA.equals(cmd)) {
            JButton button = (JButton)e.getSource();
            button.setEnabled(false);
            m_gui.sendCommand("CollectCalData probe");
        } else if (DELETE_CAL.equals(cmd)) {
            deleteSelectedCalibration();
        }
    }

    private void validateCustomBand() {
        try {
            String spec = getCustomStart() + " " + getCustomStop();
            spec += " " + getCustomRfchan();
            spec +=  " \"" + getCustomName() + "\"";
            spec +=  " \"" + getCustomPortName() + "\"";
            m_band = new CalBand(spec, CalBand.SRC_CAL_EXISTS);
            m_wizard.setNextEnabled(true);
        } catch (NullPointerException npe) {
            m_band = null;
            m_wizard.setNextEnabled(false);
            //Messages.postError("Need valid name and range for custom band");
        }
    }

    private double getCustomStart() throws NullPointerException {
        double rtn = ((Number)m_customStart.getValue()).doubleValue();
        if (rtn < 5) {
            throw new NullPointerException("Start frequency too low");
        }
        return rtn;
    }

    private double getCustomStop() throws NullPointerException {
        double rtn = ((Number)m_customStop.getValue()).doubleValue();
        if (rtn < 10) {
            throw new NullPointerException("Stop frequency too low");
        }
        return rtn;
    }

    private int getCustomRfchan() throws NullPointerException {
        double rtn = ((Number)m_customRfchan.getValue()).intValue();
        if (rtn < 1 || rtn > 5) {
            throw new NullPointerException("Illegal RF channel");
        }
        return (int)rtn;
    }

    private String getCustomName() throws NullPointerException {
        String rtn = m_customName.getText();
        if (rtn.length() == 0) {
            throw new NullPointerException("Missing name for calibration band");
        }
        return rtn;
    }

    private String getCustomPortName() throws NullPointerException {
        String rtn = m_customPortName.getText().trim();
        if (rtn.length() == 0) {
            rtn = getCustomName(); // Throws NullPointerException if empty.
        }
        return rtn;
    }

    private void deleteSelectedCalibration() {
        if (m_band == null) {
            return;
        }
        String path = MtuneControl.getExactCalFilePath(m_band.getStart(),
                                                       m_band.getStop(),
                                                       m_band.getRfchan());
        if (path == null) {
            return;
        }
        String msg = "<html>Are you sure you want to delete the calibration "
                + "file for<br>" + m_band.getShortLabel() + "?";
        String title = "Confirm File Deletion";
        int n = JOptionPane.showConfirmDialog(m_wizard, msg, title,
                                              JOptionPane.OK_CANCEL_OPTION);
        if (n == JOptionPane.OK_OPTION) {
            if (MtuneControl.deleteCalFile(m_band)) {
                JRadioButton button = m_bandButtons.get(m_band);
                button.setText(m_band.getLabel());
                button.repaint();/*CMP*/
            }
//            if (deleteCalBand(m_band)) {
//                JRadioButton button = m_bandButtons.get(m_band);
//                button.setVisible(false);
//            }
        }
    }

//    /**
//     * If this RF calibration band is defined in the probe "bands" file,
//     * remove it from the file.
//     * @param band The band to remove.
//     * @return True if the band was removed.
//     */
//    private boolean deleteCalBand(CalBand band) {
//        // TODO Auto-generated method stub/*CMP*/
//        return false;
//    }

    /**
     * The calibration data for the current termination has been collected.
     */
    public void calDataSet() {
        m_wizard.next();
    }

    /**
     * Collect the possible calibration bands from various sources.
     * <ul><li>Already existing calibration files.
     * <li>Calibration bands specified in "/vnmr/tune/probeName/bands".
     * <li>Generic bands for the system "h1freq" and "probeConnect"
     * configuration. (Only if no probe-specific bands are specified.)
     * </ul>
     * @param probeName The probe name from the Vnmr "probe" parameter.
     * @return The list of bands in order of "start" frequency.
     */
    private static SortedSet<CalBand> getCalBands(String probeName) {
        SortedSet<CalBand> calBands;

//        // NB: Band names in existing calibration files take precedence
//        calBands = getExistingCalBands(probeName);
//        SortedSet<CalBand> tmpBands;
//        tmpBands = getProbeCalBands(probeName);
//        if (tmpBands.size() > 0) {
//            calBands.addAll(tmpBands);
//        } else {
//            calBands.addAll(getGenericCalBands());
//        }

        // NB: Probe-specific or generic band names take precedence
        // Get possible probe-specific calibration bands
        calBands = getProbeCalBands(probeName);
        if (calBands.size() == 0) {
            // Get generic calibration bands
            calBands = getGenericCalBands();
        }
        calBands.addAll(getExistingCalBands(probeName));
        return calBands;
    }

    private static SortedSet<CalBand>
    getExistingCalBands(String probeName, FilenameFilter filter) {

        TreeSet<CalBand> calBands = new TreeSet<CalBand>();
        ArrayList<File> dirs = MtuneControl.getCalDirs(probeName);
        if (dirs != null) for (File dir : dirs) {
            File[] files = dir.listFiles(filter);
            for (File file : files) {
                try {
                    String spec = getCalFileSpec(file);
                    calBands.add(new CalBand(spec, CalBand.SRC_CAL_EXISTS));
                } catch (Exception e) {
                }
            }
        }
        return calBands;
    }

    private static SortedSet<CalBand> getExistingCalBands(String probeName,
                                                          int rfchan) {
        RegexFileFilter filter
        = new RegexFileFilter(MtuneControl.getCalNameTemplate(null, null,
                                                              rfchan,
                                                              null, null));
        return getExistingCalBands(probeName, filter);
    }

    private static SortedSet<CalBand> getExistingCalBands(String probeName) {
        return getExistingCalBands(probeName, CAL_NAME_FILE_FILTER);
    }

    private static String getCalFileSpec(File file) {
        String spec = "";
        ZipFile zipFile = null;
        try {
            zipFile = new ZipFile(file);
            ZipEntry entry = zipFile.getEntry("Spec");
            InputStream in = zipFile.getInputStream(entry);
            BufferedReader reader;
            reader = new BufferedReader(new InputStreamReader(in));
            spec = reader.readLine();
        } catch (Exception e) {
            Messages.postError("Error reading calibration Spec file in \""
                               + file.getPath() + "\": " + e);
        } finally {
            try { zipFile.close(); } catch (Exception e) { }
        }
        return spec;
    }

    private static SortedSet<CalBand> getGenericCalBands() {
        TreeSet<CalBand> calBands = new TreeSet<CalBand>();
        double h1freq = Integer.valueOf(System.getProperty("apt.H1freq", "400"));
        boolean isF19 = !"0".equals(System.getProperty("apt.isF19", "0"));

        String spec;
        spec = Fmt.f(0, h1freq * 0.075) + " " + Fmt.f(0, h1freq * 0.462)
                + " " + 2 + " \"Low Band\" \"X CHANNEL\"";
        calBands.add(new CalBand(spec, CalBand.SRC_GENERIC));

        if (isF19) {
            spec = Fmt.f(0, h1freq - 10) + " " + Fmt.f(0, h1freq + 10)
                + " " + 1 + " \"Proton\" \"1H/19F\"";
            calBands.add(new CalBand(spec, CalBand.SRC_GENERIC));

            double f19freq = h1freq * 0.941;
            spec = Fmt.f(0, f19freq - 10) + " " + Fmt.f(0, f19freq + 10)
                + " " + 3 + " \"Fluorine\" \"1H/19F\"";
            calBands.add(new CalBand(spec, CalBand.SRC_GENERIC));

        } else {
            spec = Fmt.f(0, h1freq * 0.84) + " " + Fmt.f(0, h1freq * 1.08)
                + " " + 1 + " \"High Band\" \"PROTON\"";
            calBands.add(new CalBand(spec, CalBand.SRC_GENERIC));
        }
        return calBands;
    }

    /**
     * Looks for a file /vnmr/tune/[probeName]/bands containing a list
     * of calibration bands for this probe.
     * This file may not exist if the probe just uses the generic bands
     * for the spectrometer frequency.
     * The "band" file can have comment lines beginning with "#".
     * The format of the entry lines is specified in
     * {@link CalBand#CalBand(String)}.
     * @param probeName The probe and parent directory name.
     * @return The set of calibration bands.
     */
    private static SortedSet<CalBand> getProbeCalBands(String probeName) {
        TreeSet<CalBand> calBands = new TreeSet<CalBand>();
        String sysdir = ProbeTune.getVnmrSystemDir();
        String path = sysdir + "/tune/" + probeName + "/bands";
        BufferedReader in = TuneUtilities.getReader(path);
        if (in != null) {
            String line = null;
            try {
                while ((line = in.readLine()) != null) {
                    if (line.trim().startsWith("#")) {
                        continue; // Skip comment lines
                    }
                    CalBand band = new CalBand(line, CalBand.SRC_PROBE_BANDS);
                    if (band != null) {
                        calBands.add(band);
                    }
                }
            } catch (IOException ioe) {
                Messages.postError("Cannot read file \"" + path + "\"");
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad calibration band in file \"" + path
                                   + "\": " + line);
            } finally {
                try {
                    in.close();
                } catch (Exception e) {}
            }
        }
        return calBands;
    }


    class CalNavigator implements Navigator {

        private JWizard m_parent;

        public String getNextScreen(String currentName, int direction) {
            System.err.println("getNextScreen " + currentName + " " + direction);/*CMP*/
            Map<Integer, String> wizardScreens = m_parent.getWizardScreens();
            int index = m_parent.getCurrentScreenIndex();
            if (direction == BACK) {
                if (CAL_LOAD_PANEL.equals(currentName)) {
                    m_parent.setCurrentScreenIndex(index -= 2);
                    return CAL_OPEN_PANEL;
                } else if (CAL_SHORT_PANEL.equals(currentName)) {
                    m_parent.setCurrentScreenIndex(index -= 2);
                    return CAL_LOAD_PANEL;
                } else if (CAL_PROBE_PANEL.equals(currentName)) {
                    m_parent.setCurrentScreenIndex(index -= 2);
                    return CAL_SHORT_PANEL;
                } else if (FINISH_PANEL.equals(currentName)) {
                    m_parent.setCurrentScreenIndex(index -= 2);
                    return CAL_PROBE_PANEL;
                } else {
                    m_parent.setCurrentScreenIndex(--index);
                    return wizardScreens.get(index);
                }
            } else if (direction == NEXT) {
                m_parent.setCurrentScreenIndex(++index);
                return wizardScreens.get(index);
            }
            return "Unknown";
        }

        public void init(JWizard parent) {
            m_parent = parent;
        }
    }


    public void focusGained(FocusEvent e) {
        focusLost(e);
    }

    public void focusLost(FocusEvent e) {
        m_customButton.setSelected(true);
        validateCustomBand();
    }


//    class FreqBand {
//        private String mm_name;
//        private double mm_start_Hz;
//        private double mm_stop_Hz;
//        private boolean mm_isDefault;
//
//        public FreqBand(String name, double start_Hz, double stop_Hz,
//                        boolean selectByDefault) {
//            mm_name = name;
//            mm_start_Hz = start_Hz;
//            mm_stop_Hz = stop_Hz;
//            mm_isDefault = selectByDefault;
//        }
//
//        public String getName() {
//            return mm_name;
//        }
//
//        public double getStart() {
//            return mm_start_Hz;
//        }
//
//        public double getStop() {
//            return mm_stop_Hz;
//        }
//
//        public Pattern getFilePattern() {
//            String strPattern = "cal_[0-9]+_[0-9]+_";
//            strPattern += MtuneControl.getCalFreqName(getStart()) + "_";
//            strPattern += MtuneControl.getCalFreqName(getStop()) + ".zip";
//            return Pattern.compile(strPattern);
//        }
//
//        public long getFileDate() {
//
//            return 0;
//        }
//
//        public String getLabel() {
//            long time = getFileDate();
//            String strDate = "";
//            if (time != 0) {
//                DateFormat df = DateFormat.getDateInstance(DateFormat.SHORT);
//                strDate = df.format(new Date(time));
//            }
//            String label = "<html><table><tr>"
//                    + "<td width=300>" + getName() + ": "
//                    + Fmt.f(1, getStart() / 1e6) + " to "
//                    + Fmt.f(1, getStop() / 1e6) + " MHz" + "</td>"
//                    + "<td width=100>" + strDate + "</td>"
//                    + "</tr></table>";
//            return label;
//        }
//
//        public boolean isSelectedByDefault() {
//            return mm_isDefault;
//        }
//
//    }


}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

package vnmr.apt;

import java.awt.event.*;
import java.awt.*;

import javax.swing.*;
import javax.swing.border.Border;

import java.net.*;
import java.io.*;
import java.util.*;
import java.util.List;

import vnmr.apt.ChannelInfo.Limits;
import vnmr.bo.Plot;
import vnmr.util.*;

public class ProbeTuneGui extends JFrame
    implements WindowListener, ActionListener, Executer, AptDefs {

    protected static final long serialVersionUID = 42L;

    protected PrintWriter cmdSender = null;

    static final int traceNumber = 1;

    /** Keep track of X scaling */
    protected double[] m_xMinMax = new double[2];

    // Codes to address different motors
    static final int TUNE_MOTOR = 0;
    static final int MATCH_MOTOR = 1;

    // Commands
    static final String CHAN_NAME     = "Channel Name";
    static final String ABORT_TUNE    = "Abort";
    static final String START         = "Start";
    static final String STOP          = "Stop";
    static final String TUNE_FREQ     = "Tune to";
    static final String TUNE_PROBE    = "Tune Probe";
    static final String RF_CHAN       = "RF Chan";
    static final String MATCH_THRESH  = "Threshold";
    static final String TRACK_TUNE    = "Refresh";
    static final String CENTER        = "Center";
    static final String SPAN          = "Span";
//    static final String NUMBER_PTS    = "# Points";
    static final String PRE_TUNE      = "Go to";
    static final String PRE_TARGET	  = "Draw Targ";
    static final String SAVE_TUNE     = "Save";
    static final String PLOT_TYPE     = "Polar";
    static final String USE_HISTORY   = "UseRefs";
    static final String COMMAND       = "Command";
    static final String PHASE0        = "Phase0";
    static final String PHASE1        = "Phase1";
    static final String PHASE2        = "Phase2";
    static final String PHASE3        = "Phase3";
    static final String PHASE_AUTO    = "PhaseAuto";
    static final String SAVE_PHASE    = "SavePhase";
    static final String DELAY         = "Delay";
    static final String TUNE_OFF      = "Tune off";
    static final String TUNE_ON       = "Tune on";
    static final String RAW_DATA_OFF  = "Raw data off";
    static final String RAW_DATA_ON   = "Raw data on";
    static final String HIGH_BAND     = "htune";
    static final String LOW_BAND      = "xtune";
    static final String SAVECAL_INF   = "saveCalData Inf";
    static final String SAVECAL_50    = "saveCalData 50";
    static final String SAVECAL_0     = "saveCalData 0";
    static final String CLEARCAL      = "clearCal";
    static final String INFO          = "Info";
    static final String QUIT          = "Quit";
    static final String CLOSE         = "Close";
    static final String GUI_ESCAPE    = "gui "; //$NON-NLS-1$

//    static final String CALMSG_INF   = "<html>Is the cable disconnected and open?<br><br>";
//    static final String CALMSG_50   = "<html>Is the cable connected with 50 Ohm plug?<br><br>";
//    static final String CALMSG_0   = "<html>Is the cable connected with shorting plug?<br><br>";
//    static final String CAL_MESSAGE   = "<html>Are you sure you want to overwrite this <br>" +
//                                        "calibration? (press Update Cal Data when done)";

    static final Color GRID_COLOR = Color.WHITE;
    static final Color CANVAS_COLOR = new Color(0xe8e8e8);

    protected Plot polarPlot = new Plot();
    protected Plot absPlot = new Plot();

    public boolean cancel = false;
    //public double[] reflectionData2;
    public double m_startFreq = 280e6;
    public double m_stopFreq = 320e6;
    public int m_np = 512;
    public double m_targetFreq = Double.NaN;
    public double m_targetMatch = Double.NaN; // Square of reflection coeff
    protected Icon m_targetIcon = null;
    protected Icon m_crossIcon = null;
    //public double tuneFrequency;
    //public int numberOfPoints;
    protected double minFreq = 10e6;
    protected double maxFreq = 1000e6;
    protected int m_iChannel = 0; // Default channel number
    protected boolean m_isPolarPlot = false;
    protected boolean m_sweepOk = false;
    protected boolean m_motorOk = false;
    protected int m_lastStatus = 0;

    protected JLabel motorMsg = new JLabel("Motor Status");
    protected JLabel sweepMsg = new JLabel("Sweep Status");
    protected JButton cmdAbort = new JButton(ABORT_TUNE);
    protected JButton cmdTuneFreq = new JButton(TUNE_PROBE);
    protected JButton cmdRfChan = new JButton(RF_CHAN);
    protected JButton cmdMatchThresh = new JButton(MATCH_THRESH);
    protected JButton cmdStepTune = new JButton(TRACK_TUNE);
    protected JButton cmdSetCenter = new JButton(CENTER);
    protected JButton cmdSetSpan = new JButton(SPAN);
//    protected JButton cmdSetNumberOfPoints = new JButton(NUMBER_PTS);
    protected JButton cmdSetPredefinedTune = new JButton(SAVE_TUNE);
    protected JButton cmdGoPredefinedTune = new JButton(PRE_TUNE);
    //protected JToggleButton cmdSetPlotType = new JToggleButton(PLOT_TYPE);

    protected JButton cmdRfcal = new JButton("Calibrate RF Reflection...");
    protected JButton cmdInit = new JButton("Measure Motor Sensitivities");

    protected JComboBox cbChName = new JComboBox();
    //protected JTextField txtStart = new JTextField(8);
    //protected JTextField txtStop = new JTextField(8);

    protected JTextField txtTuneFreq = new JTextField(6);
    protected JTextField txtRfChan = new JTextField(6);
    protected JTextField txtMatchThresh = new JTextField(6);
    protected JTextField txtStepTune = new JTextField("1");
    protected JTextField txtCenter = new JTextField(6);
    protected JTextField txtSpan = new JTextField(6);
//    protected JTextField txtNumberOfPoints = new JTextField(6);
    protected JComboBox cbPredefinedTune = new JComboBox();

    protected JLabel lblTuneFreqUnits = new JLabel("MHz");
    protected JLabel lblMatchThreshUnits = new JLabel("dB");
    protected JLabel lblStepTuneUnits = new JLabel("times");
    protected JLabel lblCenterUnits = new JLabel("MHz");
    protected JLabel lblSpanUnits = new JLabel("MHz");

    protected JPanel pnlCmd = new JPanel();
    protected JLabel lblCmd = new JLabel("Cmd:");
    protected JTextField txtCmd = new JTextField(20);

//    protected JPanel pnlRec = new JPanel();
//    protected JLabel lblRec = new JLabel("Rec :");
//    protected JLabel txtRec = new JLabel();

    protected JPanel pnlMode = new JPanel();
    //protected JLabel lblMode = new JLabel("Tune Switch:");
    protected JRadioButton cmdModeOff = new JRadioButton("Tune Mode Off", true);
    protected JRadioButton cmdModeOn = new JRadioButton("Tune Mode On", false);

    protected JPanel pnlBand = new JPanel();
    //protected JLabel lblBand = new JLabel("Tune Band:");
    protected JRadioButton cmdBandHigh = new JRadioButton("Tune Band High",
                                                          false);
    protected JRadioButton cmdBandLow = new JRadioButton("Tune Band Low",
                                                         false);

    protected JPanel pnlRawData = new JPanel();
    protected JRadioButton cmdRawDataOff = new JRadioButton("Corrected Data",
                                                            true);
    protected JRadioButton cmdRawDataOn = new JRadioButton("Raw Data", false);

    protected JPanel pnlPlotMode = new JPanel();
    protected JRadioButton cmdPlotModeOff = new JRadioButton("Absval Plot",
                                                             true);
    protected JRadioButton cmdPlotModeOn = new JRadioButton("Polar Plot",
                                                            false);

    protected JPanel pnlUseHistory = new JPanel();
    protected JRadioButton cmdHistoryOn
    = new JRadioButton("Use Ref Positions", true);
    protected JRadioButton cmdHistoryOff
    = new JRadioButton("Ignore Ref Positions", false);

    protected JLabel txtTuneKhzPerStep = new JLabel("Tune ( KHz / Step ) :");
    protected JLabel txtTuneReflPerStep
    = new JLabel("Tune ( % Reflection / Step ) :");
    protected JLabel txtMatchKhzPerStep = new JLabel("Match ( KHz / Step ) :");
    protected JLabel txtMatchReflPerStep
    = new JLabel("Match ( % Reflection / Step ) :");
    protected JLabel txtFreq = new JLabel(" ");
    protected JLabel txtReflection = new JLabel(" ");
    //protected JLabel txtPhaseDerivative = new JLabel("dPhase/dHz");
    protected JLabel txtTargetMatch =  new JLabel(" ");

    protected JPanel pnlStatus = new JPanel();
    protected JLabel lblStatus = new JLabel("Initializing...", JLabel.CENTER);

    protected JPanel pnlInfo = new JPanel();
    protected JLabel lblInfo = new JLabel();
    protected JButton cmdInfo = new JButton(INFO);
    protected JButton cmdQuit = new JButton(QUIT);

    protected JPanel plotPane = new JPanel();
    protected JPanel northPane = new JPanel();

    protected JLabel lblQuadPhase = new JLabel("Cal Phase: " + DSPC
                                               + "--" + DEG);
    protected JToggleButton cmdPhase0 = new JToggleButton("0");
    protected JToggleButton cmdPhase1 = new JToggleButton("1");
    protected JToggleButton cmdPhase2 = new JToggleButton("2");
    protected JToggleButton cmdPhase3 = new JToggleButton("3");
    protected JToggleButton cmdPhaseAuto = new JToggleButton("A");

    protected JLabel lblSpace = new JLabel("     ");
    protected JLabel lblProbeDelay = new JLabel("Phase Delay: 0 ns");
    protected JLabel lblConsoleType = new JLabel("Console");
    protected JTextField txtProbeDelay = new JTextField(5);

    protected List<MotorPanel> m_motorPanels = new ArrayList<MotorPanel>();

    protected ReflectionData m_reflectionData;

    protected MtuneCal m_calPopup = null;

    protected String m_probeName;
    protected String m_usrProbeName;

    /** The round-trip time from probe port to coil (ns). */
    protected double m_probeDelay = 0;

    /** Whether initialization is completed in the master ProbeTune. */
    private boolean m_sysInitialized = false;

    /**
     * The value of the Vnmr "Console" parameter.
     */
    private String m_console;


    public ProbeTuneGui(String probe, String usrProbe, String mode,
                        String sysTuneDir, String usrTuneDir,
                        String host, int port, String console) {

        m_probeName = probe;
        m_usrProbeName = usrProbe;
        if (console != null && console.length() > 0) {
            console = (console.substring(0, 1).toUpperCase()
                    + console.substring(1));
        }
        lblConsoleType.setText(console + " Console");
        m_console = console;

        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        // Define the icon for this window
        setTitle("ProTune");
        Class<?> cl = getClass();
        setIconImage(TuneUtilities.getImage("proTune.gif", cl));

        // Get icons that may be used in the plots
        m_targetIcon = TuneUtilities.getIcon("target.png", cl);
        m_crossIcon = TuneUtilities.getIcon("cross.png", cl);

        // Establish communication
        Socket socket = TuneUtilities.getSocket("ProbeTuneGui", host, port);
        if (socket != null) {
            try {
                cmdSender = new PrintWriter(socket.getOutputStream(), true);
            } catch (IOException ioe) {
                Messages.postError("ProbeTuneGui: "
                                   + "IOException connecting to ProbeTune");
            }
        }
        CommandListener listener = new CommandListener(socket, this);
        listener.start();

        //Set up channel selection
        File fileTest = new File(sysTuneDir);
        if (!fileTest.isDirectory()) {
            Messages.postError("No tune directory for probe: " + sysTuneDir);
        } else {
            loadChannelMenu(sysTuneDir, usrTuneDir, m_probeName, mode);
            loadSampleMenu(usrTuneDir);
        }

        initializeGui();

        addWindowListener(this);

        pack();
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        Dimension frameSize = this.getSize();
        // Put on right side
        setLocation(screenSize.width - frameSize.width,
                    (screenSize.height - frameSize.height) / 2);

        setVisible(true);

        // Make the initial GUI looks nicer
        txtCmd.requestFocusInWindow();
        //displayStatus(STATUS_READY);
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
        leftPanel.add(sweepMsg, c);
        c.gridy = ++row;

        if (!m_probeName.equals(m_usrProbeName)) {
            String label = "Probe ID: " + m_usrProbeName;
            leftPanel.add(new JLabel(label), c);
            c.gridy = ++row;
        }

        ButtonGroup group = new ButtonGroup();
        cmdBandHigh.addActionListener(this);
        cmdBandHigh.setActionCommand(HIGH_BAND);
        group.add(cmdBandHigh);
        cmdBandLow.addActionListener(this);
        cmdBandLow.setActionCommand(LOW_BAND);
        group.add(cmdBandLow);

        group = new ButtonGroup();
        cmdModeOff.addActionListener(this);
        cmdModeOff.setActionCommand(TUNE_OFF);
        group.add(cmdModeOff);
        cmdModeOn.addActionListener(this);
        cmdModeOn.setActionCommand(TUNE_ON);
        group.add(cmdModeOn);

        group = new ButtonGroup();
        cmdRawDataOff.addActionListener(this);
        cmdRawDataOff.setActionCommand(RAW_DATA_OFF);
        group.add(cmdRawDataOff);
        cmdRawDataOn.addActionListener(this);
        cmdRawDataOn.setActionCommand(RAW_DATA_ON);
        group.add(cmdRawDataOn);

        group = new ButtonGroup();
        cmdPlotModeOff.addActionListener(this);
        cmdPlotModeOff.setActionCommand(PLOT_TYPE);
        group.add(cmdPlotModeOff);
        cmdPlotModeOn.addActionListener(this);
        cmdPlotModeOn.setActionCommand(PLOT_TYPE);
        group.add(cmdPlotModeOn);

        group = new ButtonGroup();
        cmdHistoryOn.addActionListener(this);
        cmdHistoryOn.setActionCommand(USE_HISTORY);
        group.add(cmdHistoryOn);
        cmdHistoryOff.addActionListener(this);
        cmdHistoryOff.setActionCommand(USE_HISTORY);
        group.add(cmdHistoryOff);

        int lblHeight = motorMsg.getPreferredSize().height;
        //Dimension dim = cmdModeOff.getPreferredSize(); // This is the largest!
        Dimension dim = cmdBandHigh.getPreferredSize(); // This is the largest!
        dim.height = lblHeight;
        

        //dim.width = cmdRawDataOff.getPreferredSize().width;
        // NB: The above gives the same "dim" but a different layout
        // than the following!
        int dimy = dim.height;
        dim = cmdRawDataOff.getPreferredSize(); // Widest label
        dim.height = dimy;

        cmdRawDataOff.setPreferredSize(dim);
        cmdRawDataOn.setPreferredSize(dim);
        pnlRawData.setLayout(new GridLayout());
        pnlRawData.add(cmdRawDataOff);
        pnlRawData.add(cmdRawDataOn);

        cmdPlotModeOff.setPreferredSize(dim);
        cmdPlotModeOn.setPreferredSize(dim);
        pnlPlotMode.setLayout(new GridLayout());
        pnlPlotMode.add(cmdPlotModeOff);
        pnlPlotMode.add(cmdPlotModeOn);

        cmdHistoryOn.setPreferredSize(dim);
        cmdHistoryOff.setPreferredSize(dim);
        pnlUseHistory.setLayout(new GridLayout());
        pnlUseHistory.add(cmdHistoryOn);
        pnlUseHistory.add(cmdHistoryOff);

        //((FlowLayout)pnlBand.getLayout()).setVgap(0);
        //pnlBand.add(lblBand);
        cmdBandHigh.setPreferredSize(dim);
        cmdBandLow.setPreferredSize(dim);
        pnlBand.setLayout(new GridLayout());
        pnlBand.add(cmdBandHigh);
        pnlBand.add(cmdBandLow);

        //((FlowLayout)pnlMode.getLayout()).setVgap(0);
        //pnlMode.add(lblMode);
        cmdModeOff.setPreferredSize(dim);
        cmdModeOn.setPreferredSize(dim);
        pnlMode.setLayout(new GridLayout());
        pnlMode.add(cmdModeOn);
        pnlMode.add(cmdModeOff);

//        leftPanel.add(cmdAbort, c);
//        //cmdAbort.setBackground(new Color(0xffc0cb)); // Pink
//        cmdAbort.setForeground(Color.RED);
//        c.gridy = ++row;

        leftPanel.add(cbChName, c);
        c.gridy = ++row;

        c.gridwidth = 1;

        int doubleColumnStart = row;
        leftPanel.add(cmdTuneFreq, c);
        c.gridy = ++row;
        if (isRfChanSelectable()) {
            leftPanel.add(cmdRfChan, c);
            c.gridy = ++row;
        }
        leftPanel.add(cmdMatchThresh, c);
        c.gridy = ++row;
        leftPanel.add(cmdStepTune, c);
        c.gridy = ++row;
        leftPanel.add(cmdSetCenter, c);
        c.gridy = ++row;
        leftPanel.add(cmdSetSpan, c);
        c.gridy = ++row;
//        leftPanel.add(cmdSetNumberOfPoints, c);
//        c.gridy = ++row;
        leftPanel.add(cmdGoPredefinedTune, c);
        c.gridy = ++row;

        c.gridwidth = 4;

        leftPanel.add(pnlRawData, c);
        c.gridy = ++row;
        leftPanel.add(pnlPlotMode, c);
        c.gridy = ++row;
        leftPanel.add(pnlUseHistory, c);
        c.gridy = ++row;

        //c.gridwidth = 1;
        int tuneModeRow = row;

        //c.gridwidth = 4;

        // if (ProbeTune.isTuneModeSwitchable(m_console)) {
        //     //lblMode.setPreferredSize(dim);
        //     leftPanel.add(pnlMode, c);
        //     c.gridy = ++row;
        // }

        // if (ProbeTune.isTuneBandSwitchable(m_console)) {
        //     leftPanel.add(pnlBand, c);
        //     c.gridy = ++row;
        // }

        //c.gridwidth = 4;

        leftPanel.add(cmdRfcal, c);
        c.gridy = ++row;

        //((FlowLayout)pnlCmd.getLayout()).setAlignment(FlowLayout.LEFT);
        pnlCmd.setLayout(new BorderLayout(5, 10));
        pnlCmd.setBorder(BorderFactory.createEmptyBorder(5, 0, 10, 0));
        pnlCmd.add(lblCmd, BorderLayout.WEST);
        pnlCmd.add(txtCmd, BorderLayout.CENTER);
        leftPanel.add(pnlCmd, c);
        c.gridy = ++row;

        c.gridwidth = 3;
        leftPanel.add(cmdInit, c);
        c.gridy = ++row;
        cmdInit.setForeground(Color.RED);
        Font font = cmdInit.getFont();
        float size = font.getSize2D() - 2;
        font = font.deriveFont(size).deriveFont(Font.PLAIN);
        cmdInit.setFont(font);

//        //SDM added Recieved
//        pnlRec.add(lblRec);
//        pnlRec.add(txtRec);
//        leftPanel.add(pnlRec, c);
        c.gridy = ++row;

//        leftPanel.add(txtBacklash, c);
//        c.gridy = ++row;
        leftPanel.add(txtTuneKhzPerStep, c);
        c.gridy = ++row;
        leftPanel.add(txtTuneReflPerStep, c);
        c.gridy = ++row;
//        leftPanel.add(txtBacklash2, c);
//        c.gridy = ++row;
        leftPanel.add(txtMatchKhzPerStep, c);
        c.gridy = ++row;
        leftPanel.add(txtMatchReflPerStep, c);
        c.gridy = ++row;
        leftPanel.add(txtFreq, c);
        c.gridy = ++row;
        leftPanel.add(txtReflection, c);
        c.gridy = ++row;
        //leftPanel.add(txtPhaseDerivative, c);
        leftPanel.add(txtTargetMatch, c);
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

        int tripleColumnStart = row;
        c.gridwidth = 1;
        leftPanel.add(txtTuneFreq, c);
        c.gridy = ++row;
        if (isRfChanSelectable()) {
            leftPanel.add(txtRfChan, c);
            c.gridy = ++row;
        }
        leftPanel.add(txtMatchThresh, c);
        c.gridy = ++row;
        leftPanel.add(txtStepTune, c);
        c.gridy = ++row;
        leftPanel.add(txtCenter, c);
        c.gridy = ++row;
        leftPanel.add(txtSpan, c);
        c.gridy = ++row;
//        leftPanel.add(txtNumberOfPoints, c);
//        c.gridy = ++row;
        leftPanel.add(cbPredefinedTune, c);
        c.gridy = ++row;

        c.gridy = row = tuneModeRow;
        c.gridwidth = 4;
        
        c.gridy = row = tripleColumnStart;
        c.gridx = 2;
        leftPanel.add(lblTuneFreqUnits, c);
        c.gridy = ++row;
        if (isRfChanSelectable()) {
            c.gridy = ++row;// no units label for rf chan
        }
        leftPanel.add(lblMatchThreshUnits, c);
        c.gridy = ++row;
        leftPanel.add(lblStepTuneUnits, c);
        c.gridy = ++row;
        leftPanel.add(lblCenterUnits, c);
        c.gridy = ++row;
        leftPanel.add(lblSpanUnits, c);
        c.gridy = ++row;
//        c.gridy = ++row;
        leftPanel.add(cmdSetPredefinedTune, c);
        c.gridy = ++row;

        //polarPlot.setPreferredSize(new Dimension(630, 525));
        //polarPlot.setXFillRange(-1, 1);
        //polarPlot.setYFillRange(-1, 1);
        //polarPlot.setXRange(-1, 1);
        //polarPlot.setYRange(-1, 1);
        polarPlot.setAspectRatio(1);

        //polarPlot.setBackground(new Color(237, 243, 187));
        //polarPlot.setBackground(new Color(0xdd, 0xdd, 0xdd));
        polarPlot.setBackground(null);
        //absPlot.setPreferredSize(new Dimension(630, 525));
        //absPlot.setMinimumSize(new Dimension(630, 525));
        //absPlot.setBackground(Color.LIGHT_GRAY);
        //absPlot.setBackground(new Color(0xdd, 0xdd, 0xdd));
        absPlot.setBackground(null);
        setPlot("normal");

        //motorButtonArea = new JPanel(new GridLayout(0, 4, 3, 3));

        motorButtonArea = Box.createHorizontalBox();
        int[] motors = {0, 1, 2, 3, 4, 5, 6}; // FIXME: Temporary
        for (int i : motors) {
            MotorPanel motorPanel = new MotorPanel(i, false);
            motorButtonArea.add(motorPanel);
            m_motorPanels.add(motorPanel);
        }

        getContentPane().add(leftPanel, "West");

        getContentPane().add(plotPane, "Center");
        plotPane.setPreferredSize(new Dimension(600, 575));
        plotPane.setLayout(new BorderLayout());
        northPane.setLayout(new BorderLayout());
        plotPane.add(northPane, "North");
        JPanel northCenter = new JPanel();
        northPane.add(northCenter, "Center");
        //cmdSavePhase.setBorder(new VButtonBorder());
        //northPane.add(cmdSavePhase);
        if (ProbeTune.isQuadPhaseProblematic(m_console)) {
            northCenter.add(lblQuadPhase);
            group = new ButtonGroup();
            group.add(cmdPhase0);
            cmdPhase0.setBorder(new VButtonBorder());
            northCenter.add(cmdPhase0);
            group.add(cmdPhase1);
            cmdPhase1.setBorder(new VButtonBorder());
            northCenter.add(cmdPhase1);
            group.add(cmdPhase2);
            cmdPhase2.setBorder(new VButtonBorder());
            northCenter.add(cmdPhase2);
            group.add(cmdPhase3);
            cmdPhase3.setBorder(new VButtonBorder());
            northCenter.add(cmdPhase3);
            group.add(cmdPhaseAuto);
            cmdPhaseAuto.setSelected(true);
            cmdPhaseAuto.setBorder(new VButtonBorder());
            northCenter.add(cmdPhaseAuto);
            northCenter.add(lblSpace);
        }
        northCenter.add(lblProbeDelay);
        northCenter.add(txtProbeDelay);
        if (!"vnmrs".equalsIgnoreCase(m_console)) {
            //northPane.add(lblSpace);
            northPane.add(lblConsoleType, "East");
        }
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
    }

    private boolean isRfChanSelectable() {
        return !ProbeTune.isTuneModeSwitchable(m_console)
                && !ProbeTune.isTuneBandSwitchable(m_console);
    }

    protected void setPlot(String type) {
        if (type.equals("normal")) {
            absPlot.clearLegends();
            absPlot.setGrid(true);
            absPlot.setGridColor(GRID_COLOR);
            absPlot.setPlotBackground(CANVAS_COLOR);
            absPlot.setXAxis(true);
            absPlot.setYAxis(true);
            absPlot.setXLabel("Frequency (MHz)");
            absPlot.setYLabel("Reflection Amplitude");
            absPlot.setFillButton(true);
            //absPlot.setMarksStyle("dots");
            //absPlot.setMarksStyle("points");
            absPlot.setMarksStyle("none");
            absPlot.setMarkColor(Color.BLACK, 0);

            polarPlot.clearLegends();
            polarPlot.setGrid(true);
            polarPlot.setGridColor(GRID_COLOR);
            polarPlot.setPlotBackground(CANVAS_COLOR);
            polarPlot.setXAxis(true);
            polarPlot.setYAxis(true);
            polarPlot.setXLabel("Real Reflection");
            polarPlot.setYLabel("Imaginary Reflection");
            polarPlot.setFillButton(true);
            //polarPlot.setXFillRange(-1, 1);
            //polarPlot.setYFillRange(-1, 1);
            //polarPlot.setMarksStyle("dots");
            //polarPlot.setMarksStyle("points");
            polarPlot.setMarksStyle("none");
            polarPlot.setMarkColor(Color.BLACK, 0);
        } else if (type.equals("cal")) {
            absPlot.setXLabel("Motor steps");
            absPlot.setYLabel("Reflection or Frequency");
            absPlot.clearLegends();
            absPlot.addLegend(0, "Tune Frequency");
            absPlot.addLegend(1, "Tune Reflection");
        }
    }

    /**
     * Reads the channel files and determines the names
     * for the predefined tune positions.
     * @param path The path of the channel file directory.
     */
    private void loadSampleMenu(String path){
        SortedSet<String> fnames = new TreeSet<String>(); // List of file names
        if (path == null) {
            return;
        }
        File[] files = new File(path).listFiles(CHAN_FILE_FILTER);
        if (files == null) {
            return;
        }
        for (File file : files) {
            fnames.add(file.getName());
        }

        for (String file : fnames) {
            String fpath = path + File.separator + file;
            BufferedReader input = null;
            try {
                input = new BufferedReader(new FileReader(fpath));
                String line = null;
                while ((line = input.readLine()) != null) {
                    StringTokenizer toker = new StringTokenizer(line);
                    String lowerLine = line.toLowerCase();
                    if(lowerLine.startsWith("sample ")) {
                        toker.nextToken(); //throw away key
                        String sample= toker.nextToken();
                        sample= sample.substring(4); //take off beginning samp
                        cbPredefinedTune.addItem(sample);
                    }
                }
            } catch (IOException ioe){
                Messages.postDebugWarning("Could not read channel file: \""
                                          + fpath + "\".");
            } catch (NullPointerException npe) {
            } finally {
                try {
                    input.close();
                } catch (Exception e) {}
            }
        }
    }

    /**
     * Reads the channel files and determines the ranges for the
     * channel selection menu.
     * Gets the list of channels from the system tune/probe directory;
     * Gets the range for each channel from the user file if possible,
     * otherwise the system file.
     */
    private void loadChannelMenu(String syspath, String path,
                                 String probe, String modeName) {
        Messages.postDebug("Initialization",
                           "loadChannelMenu: syspath=" + syspath);
        if (syspath == null) {
            return;
        }
        File[] files = null;
        Limits[] ranges = null;
        if (modeName == null) {
            files = new File(syspath).listFiles(CHAN_FILE_FILTER);
            Arrays.sort(files, new PFileNameComparator<File>());
        } else {
            TuneMode mode = new TuneMode(syspath, modeName);
            List<ChannelSpec> chanSpecs = mode.getChanList();
            int n = chanSpecs.size();
            files = new File[n];
            ranges = new Limits[n];
            for (int i = 0; i < n; i++) {
                files[i] = new File(chanSpecs.get(i).getName());
                ranges[i] = chanSpecs.get(i).getFreqRange_MHz();
            }
        }
        if (files == null) {
            return;
        }

        int n = files.length;
        for (int i = 0; i < n; i++) {
            String file = files[i].getName();
            String entry = probe + File.separator + file;
            String range = getChannelRange(path, file);
            if (range.length() == 0) {
                range = getChannelRange(syspath, file);
            }
            if (ranges != null && ranges.length > i && ranges[i].isSet()) {
                range = Fmt.f(0, ranges[i].getMin() / 1e6) + "-"
                        + Fmt.f(0, ranges[i].getMax() / 1e6);
            }
            cbChName.addItem(entry + "  " + range);
        }
    }

    /**
     * Get the range of a tune channel from the channel file.
     * @param dir The directory of the chan file.
     * @param file The name of the chan file.
     * @return A string expressing the range.
     */
    private String getChannelRange(String dir, String file) {
        String range = "";
        String fpath = dir + File.separator + file;
        BufferedReader input = null;
        try {
            input = new BufferedReader(new FileReader(fpath));
            String line = null;
            while ((line = input.readLine()) != null) {
                StringTokenizer toker = new StringTokenizer(line);
                String lowerLine = line.toLowerCase();
                if (lowerLine.startsWith("range ")
                        && toker.countTokens() == 3)
                {
                    try {
                        toker.nextToken(); //throw away key
                        double start = Double.valueOf(toker.nextToken());
                        double stop = Double.valueOf(toker.nextToken());
                        range = Fmt.f(0, start / 1e6) + "-"
                                + Fmt.f(0, stop / 1e6);
                    } catch (NumberFormatException nfe) {
                    }
                }
            }
        } catch (IOException ioe){
            Messages.postDebugWarning("Could not read channel file: \""
                                      + fpath + "\".");
        } catch (NullPointerException npe) {
        } finally {
            try {
                input.close();
            } catch (Exception e) {}
        }
        return range;
    }

    /**
     * Display d(kHz)/d(nsteps) on the panel.
     * @param widget The label widget to set.
     * @param hzPerStep The current value in Hz / step.
     * @param label The label string.
     */
    public void setKhzPerStep(JLabel widget, double hzPerStep, String label) {
        // Convert to kHz
        widget.setText(label + " ( KHz / Step ) :  " + Fmt.g(3, hzPerStep / 1000));
    }

    /**
     * Display d(reflection)/d(nsteps) on the panel.
     * @param widget Which JLabel to set.
     * @param value The current value in Match / step.
     * @param label An identifying prefix to specify which motor.
     */
    public void setMatchPerStep(JLabel widget, double value, String label) {
        widget.setText(label + " ( % Reflection / Step ) :  "
                       + Fmt.g(3, value * 100));
    }

    /**
     * Display backlash in steps on the panel.
     * @param widget Which JLabel to set.
     * @param backlash The value to display.
     * @param label An identifying prefix to specify which motor.
     */
    public void setBacklash(JLabel widget, double backlash, String label) {
        // Round to 1 decimal digit
        widget.setText(label + " Backlash :  " + Fmt.f(1, backlash) + " steps");
    }

    public void displayTuneMode(int mode) {
        if (mode == 0) {
            cmdModeOff.setSelected(true);
            //displayStatus(STATUS_NOTREADY);
        } else {
            cmdModeOn.setSelected(true);
            //displayStatus(STATUS_READY);
        }
    }

    private void showCenter() {
        double center = (m_stopFreq + m_startFreq) / 2e6; // MHz
        txtCenter.setText(Fmt.fg(1, center));
    }

    private void showSpan() {
        double span = (m_stopFreq - m_startFreq) / 1e6; // MHz
        txtSpan.setText(Fmt.fg(1, span));
    }

    private void displayCenter(double centerNew) {
        double centerOld = (m_stopFreq + m_startFreq) / 2;
        if (centerOld != centerNew) {
            double span = m_stopFreq - m_startFreq;
            m_startFreq = centerNew - span / 2;
            m_stopFreq = m_startFreq + span;
            txtCenter.setText("" + (centerNew / 1e6));
        }
    }

    private void displaySpan(double spanNew) {
        double spanOld = m_stopFreq - m_startFreq;
        if (spanOld != spanNew) {
            double center = (m_stopFreq + m_startFreq) / 2;
            m_startFreq = center - spanNew / 2;
            m_stopFreq = m_startFreq + spanNew;
            txtSpan.setText("" + (spanNew / 1e6));
        }
    }

    public void displayTargetFreq() {
        String display = "";
        if (!Double.isNaN(m_targetFreq) && m_targetFreq > 0) {
            display = Fmt.f(3, m_targetFreq / 1e6, false, false);
        }
        txtTuneFreq.setText(display);
        absPlot.clearIcons();
        absPlot.addIcon(m_targetIcon,
                        m_targetFreq / 1.e6,
                        Math.sqrt(m_targetMatch));
        absPlot.repaint();
        // TODO: update target frequency on polar plot
    }

    public void displayTargetMatch(double freq, double reflection2) {
        double match = 10 * Math.log(reflection2) / Math.log(10);
        double x = freq / 1e6;
        String strFreq = (Double.isNaN(x) ? "--" : Fmt.f(3, x)) + " MHz: ";
        String strMatch = (Double.isNaN(x) || Double.isNaN(match))
                          ? "" : Fmt.f(1, match) + " dB";
        txtTargetMatch.setText("Match at " + strFreq + strMatch);
    }

    public void displayPhase(String quadPhase, boolean isFixed) {
        Messages.postDebug("QuadPhase", "ProbeTuneGui.displayPhase("
                           + quadPhase + ", isFixed=" + isFixed + ")");
        try {
            int i = Integer.parseInt(quadPhase);
            quadPhase = i < 0 ? DSPC + "--" : Fmt.d(3, 90 * i, false, DSPC);
            if (isFixed) {
                switch (i) {
                case 0:
                    cmdPhase0.setSelected(true);
                    break;
                case 1:
                    cmdPhase1.setSelected(true);
                    break;
                case 2:
                    cmdPhase2.setSelected(true);
                    break;
                case 3:
                    cmdPhase3.setSelected(true);
                    break;
                default:
                    cmdPhaseAuto.setSelected(true);
                    break;
                }
            } else {
                //cmdPhaseAuto.setSelected(true);
            }
        } catch (NumberFormatException nfe) {
        }
        lblQuadPhase.setText("Cal Phase: " + quadPhase + DEG);
    }

    public void displayProbeDelay(String delay_ns) {
        double d = 0;
        try {
            d = Double.valueOf(delay_ns);
            delay_ns = Fmt.g(2, d) + " ns";
            m_probeDelay = d;
            lblProbeDelay.setText("Phase Delay: " + delay_ns);
        } catch (NumberFormatException nfe) {
        }
    }

    public void displayChannel(String name) {
        String oldName = null;
        Object item = cbChName.getSelectedItem();
        if (item != null) {
            oldName = item.toString();
        }
        if (oldName == null || !oldName.startsWith(name)) {
            clearLastStep();
            if (m_sysInitialized) {
                // NB: Avoid the normal action on channel selection.
                String cmd = cbChName.getActionCommand();
                cbChName.setActionCommand(null);
                int nchoices = cbChName.getItemCount();
                for (int i = 0; i < nchoices; i++) {
                    String choice = cbChName.getItemAt(i).toString();
                    if (choice.startsWith(name)) {
                        cbChName.setSelectedItem(choice);
                        break;
                    }
                }
                cbChName.setActionCommand(cmd);
            }
        }
        initializeProbe(name);
    }

    private void initializeProbe(String channelName) {
        int end = channelName.indexOf("/");
        if (end < 0) {
            end = channelName.indexOf("\\");
        }
        if (end > 0) {
            String probeName = channelName.substring(0, end);
            m_probeName = probeName;
            checkRfCalPermission(probeName);
        }
    }

    private void checkRfCalPermission(String probeName) {
        String subdir = "/tune/tunecal_" + probeName;
        File dir = null;
        // FIXME: "ProbeTune.x" breaks if ProbeTune is in a different Java VM
        if (ProbeTune.useProbeId()) {
            //String tunecal_dir = "tune" + File.separator + "tunecal_"+probeName;
            dir = ProbeTune.getProbe().blobWrite(".", "tune", true, false);
        } else {
            dir = new File(ProbeTune.getVnmrSystemDir() + subdir);
        }
        if (dir == null || !dir.canWrite()) {
            cmdRfcal.setEnabled(false);
            cmdRfcal.setToolTipText("No permission to save calibration files");
        }
    }

    // Future site of calibration display:
    public void displayPlot() {
        //setPlot("cal");
    }

    public void displayFitCircle(double x, double y, double r,
                                 double theta0, double deltaTheta) {
        /*System.err.println("displayFitCircle");/*DBG*/
        if (m_isPolarPlot) {
            Color faintColor = TuneUtilities.changeBrightness(Color.RED, 15);
            Color arcColor = Color.RED;
            polarPlot.addCircle(x, y, r, faintColor, 0);
            polarPlot.addCircle(x, y, r, theta0, deltaTheta, arcColor, 0);
        } else {
            // Display this fit some day (need the freq vs. angle formula)
        }
    }

    public void displayPlot(ReflectionData data) {
        if (m_sweepOk) {
            if (m_isPolarPlot) {
                displayPolarPlot(data);
            } else {
                displayAbsPlot(data);
            }
        }
    }

    public void displayAbsPlot(ReflectionData data) {
        //setPlot("normal");
        int size = data.size;
        double f0 = data.startFreq;
        double f1 = data.stopFreq;

        double[] xr = absPlot.getXRange();
        if (xr[0] == m_xMinMax[0] && xr[1] == m_xMinMax[1]) {
            absPlot.setXRange(f0, f1);
        }
        m_xMinMax[0] = f0;
        m_xMinMax[1] = f1;
        absPlot.clearIcons();
        absPlot.addIcon(m_targetIcon,
                        m_targetFreq / 1.e6,
                        Math.sqrt(m_targetMatch));

        absPlot.setShowZeroY(true);
        absPlot.clear(0);
        absPlot.clear(2);
        double[] x = new double[size];
        double[] y = new double[size];
        for (int i = 0; i < size; i++) {
            x[i] = (f0 + i * (f1 - f0) / (size - 1)) / 1.e6;
            y[i] = Math.sqrt(data.reflection2[i]);
        }
        absPlot.setPoints(0, x, y, true);
        maybeShowWarning(absPlot, data);
    }

    public void displayPolarPlot(ReflectionData data) {
        //setPlot("normal");
        int size = data.size;
        polarPlot.clear(0);
        double[] x = new double[size];
        double[] y = new double[size];
        for (int i = 0; i < size; i++) {
            x[i] = data.real[i];
            y[i] = data.imag[i];
        }
        polarPlot.setPoints(0, x, y, true);

        polarPlot.clearIcons();
        polarPlot.clearCircles();
        drawPolarTargetCircle();
        polarPlot.addCircle(0, 0, 0.25, GRID_COLOR);
        polarPlot.addCircle(0, 0, 0.5, GRID_COLOR);
        polarPlot.addCircle(0, 0, 0.75, GRID_COLOR);
        polarPlot.addCircle(0, 0, 1, GRID_COLOR);
        polarPlot.setGridColor(GRID_COLOR);
        // This is a shameless hack that makes the polar plot show
        // the first time it is displayed (when there is already data
        // in the abs plot and the Polar Plot On radio button is pressed).
        String txt = lblStatus.getText();
        lblStatus.setText(txt + "   ");
        lblStatus.setText(txt);
        maybeShowWarning(polarPlot, data);
    }

    /**
     * Display any warning message for the given plot.
     * The given data, will indicate if a warning is needed.
     * @param plot The plot that the message applies to.
     * @param data The data being displayed.
     */
    private void maybeShowWarning(Plot plot, ReflectionData data) {
        if (data.m_calFailed) {
            plot.setTitleColor(Color.RED);
            plot.setTitle("Warning: No RF Calibration");
        } else {
            plot.setTitle(null);
        }
    }

    private void drawPolarTargetCircle() {
        polarPlot.clearCircles(Color.BLUE);
        polarPlot.addCircle(0, 0, Math.sqrt(m_targetMatch), Color.BLUE);
    }

    public void displayReflectionAt(double freq, double real, double imag) {
        if (m_isPolarPlot) {
            polarPlot.addIcon(m_crossIcon, real, imag);
        } else {
            double freq_mhz = freq / 1e6;
            double refl = Math.hypot(real, imag);
            absPlot.addIcon(m_crossIcon, freq_mhz, refl);
        }
    }

    public void displayVertex(double x, double y) {
        absPlot.clear(2);
        if (x != 0) {
           Messages.postDebug("AbsPlot", //$NON-NLS-1$
                              "ProbeTuneGui.displayVertex at x=" + x //$NON-NLS-1$
                               + ", y=" + y); //$NON-NLS-1$
            displayDipFreq(x);
            displayRefl(y);
            absPlot.setMarkColor(y > 0 ? Color.GREEN.darker() : Color.RED, 2);
            absPlot.setImpulses(true, 2);
            absPlot.setImpulseWidth(5);
            absPlot.setMarksStyle("none", 2); //$NON-NLS-1$
            absPlot.addPoint(2, x / 1.e6, Math.abs(y), false);
            //absPlot.addPoint(0, x / 1.e6, Math.abs(y), false);
            if (DebugOutput.isSetFor("captureVertex")) {
                TuneUtilities.appendLog("dipFreq", String.valueOf(x));
            }
        }
    }

    protected void displayDipFreq(double freq_hz) {
        if (Double.isNaN(freq_hz)) {
            txtFreq.setText("Dip Freq: --");
        } else {
            txtFreq.setText("Dip Freq: " + Fmt.f(3, freq_hz / 1.e6));
        }
    }

    protected void displayRefl(double refl) {
        if (Double.isNaN(refl)) {
            txtReflection.setText("Dip Refl: --");
        } else {
            double refl_db = 20 * Math.log10(Math.abs(refl));
            txtReflection.setText("Dip Refl: " + Fmt.f(4, refl)
                                  + " = " + Fmt.f(1, refl_db) + " dB");
        }
    }

    protected void displayStatus(int status) {
        int newstt = status;

        switch(status) {
        case STATUS_PREVIOUS:        //Keep Running message
            if (m_lastStatus == STATUS_RUNNING
                || m_lastStatus == STATUS_INITIALIZE)
            {
                newstt = m_lastStatus;
            }
            /* FALL THROUGH */
        case STATUS_READY:
            /* Check if Protune is really ready to tune. The followings
             * are the criteria:
             * 1.  Sweep communication is ok
             * 2.  Motor communication is ok
             * 3.  Tune switch is ON
             * If any of the above is false, Protune is not ready.
             */
            if ((cmdModeOff.isEnabled() && cmdModeOff.isSelected())
                || (!m_sweepOk) || (!m_motorOk))
            {
                newstt = STATUS_NOTREADY;
            }
            break;
        case STATUS_RUNNING:
        case STATUS_NOTREADY:
        case STATUS_MLNOTREADY:
        case STATUS_INITIALIZE:
        case STATUS_INDEXING:
            break;
        default:
            newstt = m_lastStatus;
            break;
        }

        if (newstt != m_lastStatus) {
            switch(newstt) {
            case STATUS_PREVIOUS:
                newstt = m_lastStatus;
                break;
            case STATUS_READY:
                pnlStatus.setBackground(STATUS_READY_COLOR);
                lblStatus.setText(STATUS_READY_MSG);
                break;
            case STATUS_RUNNING:
                pnlStatus.setBackground(STATUS_RUNNING_COLOR);
                lblStatus.setText(STATUS_RUNNING_MSG);
                break;
            case STATUS_NOTREADY:
                pnlStatus.setBackground(STATUS_NOTREADY_COLOR);
                lblStatus.setText(STATUS_NOTREADY_MSG);
                break;
            case STATUS_MLNOTREADY:
                pnlStatus.setBackground(STATUS_MLNOTREADY_COLOR);
                lblStatus.setText(STATUS_MLNOTREADY_MSG);
                break;
            case STATUS_INITIALIZE:
                pnlStatus.setBackground(STATUS_INITIALIZE_COLOR);
                lblStatus.setText(STATUS_INITIALIZE_MSG);
                break;
            case STATUS_INDEXING:
                pnlStatus.setBackground(STATUS_INDEXING_COLOR);
                lblStatus.setText(STATUS_INDEXING_MSG);
                break;
            }
            m_lastStatus = newstt;
        }
    }

    protected void displayMotorPosition(int gmi, int position) {
        MotorPanel motorPanel = getMotorPanel(gmi);
        if (motorPanel != null) {
            motorPanel.setPosition(position);
        }
    }

    protected void displayMotorMotion(int gmi, int direction) {
        Messages.postDebug("MotorDisplay",
                           "displayMotorMotion(gmi=" + gmi
                           + ", dir=" + direction + ")");
        MotorPanel motorPanel = getMotorPanel(gmi);
        if (motorPanel != null) {
            motorPanel.setMotion(direction);
        }
    }

    protected void displayMotorLimits(int gmi, int min, int max) {
        MotorPanel motorPanel = getMotorPanel(gmi);
        if (motorPanel != null) {
            motorPanel.setPositionLimits(min, max);
        }
    }

    protected void displayMotorName(int gmi, String name) {
        MotorPanel motorPanel = getMotorPanel(gmi);
        if (motorPanel != null) {
            motorPanel.setName(name);
        }
    }

    protected void displayMotorEmphasis(int gmi, boolean emphasized) {
        MotorPanel motorPanel = getMotorPanel(gmi);
        if (motorPanel != null) {
            motorPanel.setEmphasized(emphasized);
        }
    }

    protected void displayMotorPresent(int gmi, boolean isUsed) {
        MotorPanel motorPanel = getMotorPanel(gmi);
        if (motorPanel != null) {
            motorPanel.setEnabled(isUsed);
        }
    }

    protected void displayMotorBacklash(int gmi, int backlash) {
        MotorPanel motorPanel = getMotorPanel(gmi);
        if (motorPanel != null) {
            motorPanel.setBacklash(backlash);
        }
    }


    protected MotorPanel getMotorPanel(int gmi) {
        MotorPanel rtn = null;
        for (MotorPanel panel : m_motorPanels) {
            if (panel.getMotorNumber() == gmi) {
                rtn = panel;
                break;
            }
        }
        return rtn;
    }

    public void actionPerformed(ActionEvent e) {
        if (!m_sysInitialized) {
            String msg = "Protune is initializing, request canceled";
            String title = "Message";
            JOptionPane.showMessageDialog(this, msg, title,
                                          JOptionPane.WARNING_MESSAGE);
            return;
        }

        setProbeDelay();
        String cmd = e.getActionCommand();
        if (cmd == null || cmd.trim().length() == 0) {
            return;
        }

        if (cmd.equals(QUIT)) {
            abort(false);
            cmdSender.println("exit");
            return;
        } else if (cmd.equals(ABORT_TUNE)) {
            abort(true);
            return;
        } else if (cmd.equals(CLOSE)) {
            dispose();
            return;
        } else if (cmd.equals(CHAN_NAME)) {
            String cbChNametxt= cbChName.getSelectedItem().toString();
            StringTokenizer toker = new StringTokenizer(cbChNametxt);
            String name= toker.nextToken();
            setChannel(name);
        } else if (cmd.equals("comboBoxEdited")){
            //Do Nothing
        } else if (cmd.equals(TUNE_OFF)) {
            cmdSender.println("setTuneMode 0");
        } else if (cmd.equals(TUNE_ON)) {
            cmdSender.println("setTuneMode 1000000");
        } else if (cmd.equals(HIGH_BAND)) {
            cmdSender.println("motor " + HIGH_BAND);
        } else if (cmd.equals(LOW_BAND)) {
            cmdSender.println("motor " + LOW_BAND);
        } else if (cmd.equals(INFO)) {
            cmdSender.println("DisplayInfo");
        } else if (cmd.equals(RAW_DATA_OFF)) {
            cmdSender.println("setRawDataMode 0");
            getData();
        } else if (cmd.equals(RAW_DATA_ON)) {
            cmdSender.println("setRawDataMode 1");
            getData();

        } else if (cmd.equals(TUNE_FREQ)) {
            try {
                double tuneFreq = Double.parseDouble(txtTuneFreq.getText());
                if (tuneFreq < 1e6) {
                    tuneFreq *= 1e6;
                }
                setMatchThreshold(txtMatchThresh.getText());
                displayTuneFrequency(tuneFreq);
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad tune frequency: "
                                   + txtTuneFreq.getText());
            }

        } else if (cmd.equals(TUNE_PROBE)) {
            if(txtTuneFreq.getText().startsWith("samp")){

                Messages.postDebug("PredefinedTune", "samp tune frequency: "
                                   + txtTuneFreq.getText());
                //handle predefined tuning list
                setPredefinedTune(txtTuneFreq.getText());

            } else {

                try {
                    double tuneFreq = Double.parseDouble(txtTuneFreq.getText());
                    if (tuneFreq < 1e6) {
                        tuneFreq *= 1e6;
                    }
                    int rfChan = 0;
                    try {
                        rfChan = Integer.parseInt(txtRfChan.getText());
                    } catch (NumberFormatException nfe) {
                        // leave rfChan=0 (auto selection)
                    }
                    setMatchThreshold(txtMatchThresh.getText());
                    tuneTo(tuneFreq, rfChan);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad tune frequency: "
                                       + txtTuneFreq.getText());
                }

            }

        } else if (cmd.equals(MATCH_THRESH)) {
            setMatchThreshold(txtMatchThresh.getText());

        } else if (cmd.equals(TRACK_TUNE)) {
            try {
                int nsteps = Integer.parseInt(txtStepTune.getText());
                //stepMotor(0, TUNE_MOTOR, nsteps);
                cmdSender.println("trackTune " + nsteps);
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad number of steps: "
                                   + txtStepTune.getText());
            }

        } else if (cmd.equals(CENTER)
                || cmd.equals(SPAN)
                || cmd.equals(RF_CHAN))
        {
            // Set a new sweep
            try {
                int rfChan = 0;
                try {
                    rfChan = Integer.parseInt(txtRfChan.getText().trim());
                } catch (NumberFormatException nfe) {
                    // leave rfChan=0 (auto selection)
                    txtRfChan.setText("");
                }
                double center = (m_startFreq + m_stopFreq) / 2;
                try {
                    center = Double.parseDouble(txtCenter.getText().trim());
                } catch (NumberFormatException nfe) {
                    // leave center unchanged
                    showCenter();
                }
                double span = (m_stopFreq - m_startFreq);
                try {
                    span = Double.parseDouble(txtSpan.getText().trim());
                } catch (NumberFormatException nfe) {
                    // leave span unchanged
                    showSpan();
                }
                if (center < 1e6) {
                    center *= 1e6;
                }
                if (span < 1e6) {
                    span *= 1e6;
                }
                double span2 = span / 2;
                if (minFreq <= center - span2 && center + span2 <= maxFreq) {
                    // Need to keep start and stop freqs up-to-date
                    m_startFreq = center - span2;
                    m_stopFreq = center + span2;
                    Messages.postDebug("GuiCommands", "Setting sweep: center="
                                       + center + ", span=" + span
                                       + "; start=" + m_startFreq
                                       + ", stop=" + m_stopFreq);
                    setSweep(center, span, m_np, rfChan);
                } else {
                    Messages.postError("Sweep frequency is out of range: "
                                       + "center=" + center + ", span=" + span
                                       + "; start=" + (center - span2)
                                       + ", stop=" + (center + span2));
                }
            } catch (NumberFormatException nfe) {
                Messages.postError("Bad center or span frequency: \""
                                   + txtCenter.getText() + "\" or \""
                                   + txtSpan.getText() + "\"");
            }
            getData(); // Redisplay plot

//        } else if (cmd.equals(NUMBER_PTS)) {
//            String sNP = txtNumberOfPoints.getText();
//            try {
//                int np = Integer.parseInt(sNP);
//                setNP(np);
//            } catch (NumberFormatException nfe) {
//                Messages.postError("Bad NP field: \"" + sNP + "\"");
//            }
//            getData(); // Redisplay plot
//
        } else if(cmd.equals(PRE_TUNE)) {
            Messages.postDebug("PredefinedTune", "setting predefined tune");
            //handle predefined tuning list here
            Object item = cbPredefinedTune.getSelectedItem();
            if (item != null) {
                setPredefinedTune(item.toString());
            }
        } else if(cmd.equals(PRE_TARGET)) {
            Messages.postDebug("PredefinedTune", "Draw target (button) ");
            getPredefinedTune(cbPredefinedTune.getSelectedItem().toString());
        } else if(cmd.equals(SAVE_TUNE)){
            Messages.postDebug("PredefinedTune", "Saving tune value ");
            String sel = cbPredefinedTune.getSelectedItem().toString();
            cmdSender.println("saveTune " + sel);
            for (int i=0; i<cbPredefinedTune.getItemCount(); i++) {
                if(sel.equals(cbPredefinedTune.getItemAt(i))) {
                    break;
                } else if (i==cbPredefinedTune.getItemCount()-1) {
                    cbPredefinedTune.addItem(sel);
                    break;
                }
            }
        } else if (cmd.equals(COMMAND)) {
            String command = txtCmd.getText();
            if (command.toLowerCase().startsWith(GUI_ESCAPE)) {
                exec(command.substring(GUI_ESCAPE.length()));
            } else {
                cmdSender.println(command);
            }

        } else if (cmd.equals(PHASE0)) {
            setPhase("0");
            cmdSender.println("trackTune 1");

        } else if (cmd.equals(PHASE1)) {
            setPhase("1");
            cmdSender.println("trackTune 1");

        } else if (cmd.equals(PHASE2)) {
            setPhase("2");
            cmdSender.println("trackTune 1");

        } else if (cmd.equals(PHASE3)) {
            setPhase("3");
            cmdSender.println("trackTune 1");

        } else if (cmd.equals(PHASE_AUTO)) {
            setPhase("");
            cmdSender.println("trackTune 1");

        } else if (cmd.equals(DELAY)) {
            setProbeDelay();

        } else if (cmd.equals(USE_HISTORY)) {
            int useHistory = cmdHistoryOn.isSelected() ? 1 : 0;
            cmdSender.println("UseRefs " + useHistory);

        } else if (cmd.equals(PLOT_TYPE)) {
            showPlot();
            redrawCurrentPlot();
        } else if (cmd.equals("Initialize")) {
            String msg = "<html>Are you sure you want to remeasure<br>"
                + "motor sensitivities and replace the user<br>"
                + "channel files?";
            String title = "Channel Initialization";
            int n = JOptionPane.showConfirmDialog(this, msg, title,
                                                  JOptionPane.OK_CANCEL_OPTION);
            if (n == JOptionPane.OK_OPTION) {
                cmdSender.println(cmd);
                getData();
            }
        } else if (cmd.equals("RfCal")) {
            calibrateRF();
        } else {
            if (cmd.toLowerCase().startsWith(GUI_ESCAPE)) {
                exec(cmd.substring(GUI_ESCAPE.length()));
            } else {
                cmdSender.println(cmd);
            }
        }
    }

    protected Container getPlotContainer() {
        //Container container = getContentPane();
        return plotPane;
    }

    protected void showPlot() {
        m_isPolarPlot = cmdPlotModeOn.isSelected();
        Container container = getPlotContainer();
        if (m_isPolarPlot) {
            container.remove(absPlot);
            container.add(polarPlot, "Center");
        } else {
            container.remove(polarPlot);
            container.add(absPlot, "Center");
        }
    }

//    /**
//     * set receive text
//     */
//    private void setRec(String recText){
//        txtRec.setText(recText);
//        txtCmd.setText("");
//    }

    /**
     * Ask for reflection data
     */
    private void getData() {
        cmdSender.println("getData");
    }

//    /**
//     * Set the number of points
//     */
//    private void setNP(int n) {
//        m_np = n;
//        cmdSender.println("setNP " + n);
//    }

    /**
     * Set all parameters of the sweep
     * @param rfchan TODO
     */
    private void setSweep(double center, double span, int np, int rfchan) {
        cmdSender.println("setSweep " + center + " " + span + " " + np
                          + " " + rfchan);
    }

    private void setPhase(String phase) {
        phase = phase.trim();
        String command = "setPhase " + phase;
        cmdSender.println(command);
    }

    private void setProbeDelay() {
        String delay_ns = txtProbeDelay.getText();
        if (delay_ns.length() > 0) {
            setProbeDelay(delay_ns);
            txtProbeDelay.setText("");
        }
    }

    private void setProbeDelay(String delay_ns) {
        delay_ns = delay_ns.trim();
        try {
            double delay = Double.valueOf(delay_ns);
            if (m_probeDelay  != delay) {
                m_probeDelay = delay;
                String command = "setProbeDelay " + delay_ns;
                cmdSender.println(command);
                getData(); // TODO: Better to display same data w/ new delay
            }
        } catch (NumberFormatException nfe) {
            if (delay_ns.length() > 0) {
                Messages.postError("Bad Probe Delay value: \""
                                   + delay_ns + "\"");
            }
        }
    }

   /**
     * Tune to a predefined tuning point
     */
    private void setPredefinedTune(String sampleName) {
        cmdSender.println("setPredefinedTune " + sampleName);
    }

    /**
     * Tune to a predefined tuning point
     */
    private void getPredefinedTune(String sampleName) {
        cmdSender.println("getPredefinedTune " + sampleName);
    }

    /**
     * Tune to a given frequency
     */
    private void tuneTo(double frequency, int rfChan) {
        cmdSender.println("TuneTo " + frequency + " " + rfChan);
    }

    /**
     * Set the target tune frequency to a given value.
     * @param frequency The frequency in Hz.
     */
    private void displayTuneFrequency(double frequency) {
        cmdSender.println("displayTargetFreq " + frequency);

    }

    /**
     * Set the match tolerance to a given level.
     * @param strThresh Maximum reflection allowed in dB, or "fine", etc.
     */
    private void setMatchThreshold(String strThresh) {
        cmdSender.println("setMatchThresh " + strThresh);
        absPlot.clearIcons();
        double thresh_pwr = m_targetMatch;
        if (TuneCriterion.isStandardCriterion(strThresh)) {
            thresh_pwr = TuneCriterion.getCriterion(strThresh).getTarget_pwr();
        } else {
            try {
                thresh_pwr = Double.parseDouble(strThresh);
                thresh_pwr = Math.pow(10, -Math.abs(thresh_pwr / 10));
            } catch (NumberFormatException nfe) {}
        }
        m_targetMatch = thresh_pwr;
        if (m_targetFreq > 0) {
            absPlot.addIcon(m_targetIcon,
                            m_targetFreq / 1.e6,
                            Math.sqrt(m_targetMatch));
        }
        absPlot.repaint();

        drawPolarTargetCircle();
        polarPlot.repaint();
    }

    /**
     * Switch to the channel with the specified name and display data.
     * The name is of the form "probeName/chan#N".
     * @param name The name of the channel to switch to.
     */
    private void setChannel(String name) {
        int n = 1 + name.lastIndexOf('#');
        if (n <= 0) {
            Messages.postError("No channel number in string: \""
                               + name + "\"");
        } else {
            try {
                m_iChannel = Integer.parseInt(name.substring(n));
                Messages.postDebug("SetChannel", "setChannel " + name);
                cmdSender.println("setChannel " + name);
                cmdSender.println("trackTune 1");
            } catch (NumberFormatException nfe) {
                Messages.postError("Illegal channel number: \""
                                   + name.substring(n) + "\"");
            }
        }
    }

    /**
     * Abort any iterative tuning and turn off all motors.
     */
    private void abort(boolean msgFlag) {
        if (msgFlag) {
            cmdSender.println("abort");
        } else {
            cmdSender.println("abort quiet");
        }
    }

    /**
     * Stop the GUI, any motor activity, and the ProbeTune process.
     */
    private void quit() {
        abort(false);
        cmdSender.println("exit");
        try {
            Thread.sleep(2000);
        } catch (InterruptedException ie) {
        }
        // If we were started in the ProbeTune JVM,
        // ProbeTune.exit() should shut us down before we get here:
        System.exit(0);
    }

    public void windowClosing(WindowEvent e) {
        quit();
    }

    public void windowClosed(WindowEvent e) {
    }
    public void windowActivated(WindowEvent e) {
    }
    public void windowDeactivated(WindowEvent e) {
    }
    public void windowDeiconified(WindowEvent e) {
    }
    public void windowIconified(WindowEvent e) {
    }
    public void windowOpened(WindowEvent e) {
    }

//    public static void main(String[] args) {
//        new ProbeTuneGui(args[0], args[0], "localhost", 0);
//    }

//    /** For debug */
//    protected void displayDataString(String sData) {
//        StringTokenizer toker = new StringTokenizer(sData, " ,\t");
//        ProbeTune.printDebugMessage(4, "Data string to GUI: ");
//        for (int i = 0; i < 4 && toker.hasMoreTokens(); i++) {
//            ProbeTune.printDebugMessage(4, toker.nextToken() + "  ");
//        }
//        ProbeTune.printlnDebugMessage(4, "");
//    }

    private void setLastStep(int gmi, int step) {
        MotorPanel panel = getMotorPanel(gmi);
        if (panel != null) {
            if (step > 0) {
                panel.highlightPlusButton();
            } else if (step < 0){
                panel.highlightMinusButton();
            }
        }
    }

    private void clearLastStep() {
        // Maybe change appearance of step buttons.
    }

    /**
     * Deal with messages from master control.
     * @param cmd The command to execute.
     */
    public void exec(String cmd) {
        //ProbeTune.printlnErrorMessage(5, "ProbeTuneGui: got command \"" +cmd);
        StringTokenizer toker = new StringTokenizer(cmd, " ,\t");
        String key = "";
        if (toker.hasMoreTokens()) {
            key = toker.nextToken();
        }
        Messages.postDebug("GuiCommands", "GUI.exec(\"" + key + " ...\")");
        if (key.equals("displayFitCircle")) {
            try {
                final double x = Double.parseDouble(toker.nextToken());
                final double y = Double.parseDouble(toker.nextToken());
                final double r = Double.parseDouble(toker.nextToken());
                final double theta0 = Double.parseDouble(toker.nextToken());
                final double deltaTheta = Double.parseDouble(toker.nextToken());
                SwingUtilities.invokeLater(new Runnable() {
                        public void run() {
                            displayFitCircle(x, y, r, theta0, deltaTheta);
                        }
                    }
                                           );
            } catch (NumberFormatException nfe) {
                // Don't try to display illegal circle
            } catch (NoSuchElementException nsee) {
                // Don't try to display illegal circle
            }
        } else if (key.equals("setData")) {
            //ProbeTune.printlnErrorMessage(5, "ProbeTuneGui: " + cmd);/*DBG*/
            /*ProbeTune.printlnErrorMessage(5, "ProbeTuneGui: GOT DATA, 2*npts="
                               + toker.countTokens());/*DBG*/
            if (toker.hasMoreTokens()) {
                setData(cmd);
            }
        } else if (key.equals("addTorqueDatum")) {
            // "addTorqueDatum 1 230 1.234"
            addTorqueData(toker);
        } else if (key.equals("shiftTorqueData")) {
            // "shiftTorqueData 1 2345"
            shiftTorqueData(toker);
        } else if (key.equals("displayReflectionAt")) {
            if (toker.countTokens() == 3) {
                try {
                    final double freq;
                    final double real;
                    final double imag;
                    freq = Double.parseDouble(toker.nextToken());
                    real = Double.parseDouble(toker.nextToken());
                    imag = Double.parseDouble(toker.nextToken());
                    SwingUtilities.invokeLater(new Runnable() {
                        public void run() {
                            displayReflectionAt(freq, real, imag);
                            displayTargetMatch(freq, real * real + imag * imag);
                        }
                    }
                    );
                } catch (NumberFormatException nfe) {
                }
            }
        } else if (key.equals("setTargetFreq")) {
            if (toker.countTokens() == 2) {
                String strFreq = toker.nextToken();
                try {
                    m_targetFreq = Double.parseDouble(strFreq);
                } catch (NumberFormatException nfe) {
                }
            }
            displayTargetFreq();
        } else if (key.equals("setVertex")) {
            double x = 0;
            double y = 0;
            if (toker.countTokens() == 2) {
                String strX = toker.nextToken();
                try {
                    x = Double.parseDouble(strX);
                } catch (NumberFormatException nfe) {
                }
                String strY = toker.nextToken();
                try {
                    y = Double.parseDouble(strY);
                } catch (NumberFormatException nfe) {
                }
            }
            displayVertex(x, y);
        } else if (key.equals("showCal")) {
            setPlot("cal");
        } else if (key.equals("showSweepLimits")) {
            if (toker.countTokens() == 2) {
                try {
                    double center = Double.parseDouble(toker.nextToken());
                    double span = Double.parseDouble(toker.nextToken());
                    double start = center - span / 2;
                    double stop = center + span / 2;
                    m_startFreq = start;
                    m_stopFreq = stop;
                    showCenter();
                    showSpan();
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad command: " + cmd);
                }
            }
        } else if (key.equals("setSpan")) {
            //ProbeTune.printlnErrorMessage(5, "ProbeTuneGui: " + cmd);
            if (toker.hasMoreTokens()) {
                String sSpan = toker.nextToken();
                try {
                    double span = Double.parseDouble(sSpan) * 1e6;
                    displaySpan(span);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad Span field: \"" + sSpan + "\"");
                }
            }
        } else if (key.equals("setCenter")) {
            if (toker.hasMoreTokens()) {
                String sCenter = toker.nextToken();
                try {
                    double center = Double.parseDouble(sCenter) * 1e6;
                    displayCenter(center);
                } catch (NumberFormatException nfe) {
                    Messages.postError("Bad Center field: \"" + sCenter + "\"");
                }
            }
        } else if (key.equals("setStart")) {
            //ProbeTune.printlnErrorMessage(5, "ProbeTuneGui: " + cmd);
            if (toker.hasMoreTokens()) {
                String sStart = toker.nextToken();
                try {
                    m_startFreq = Double.parseDouble(sStart);
                } catch (NumberFormatException nfe) {
                }
                displaySpan(m_stopFreq - m_startFreq);
                displayCenter((m_stopFreq + m_startFreq) / 2);
                //txtSpan.setText(Fmt.fg(3, (m_stopFreq - m_startFreq) / 1e6));
                //txtCenter.setText(Fmt.fg(3, (m_stopFreq + m_startFreq) / 2e6));
            }
        } else if (key.equals("setStop")) {
            //ProbeTune.printlnErrorMessage(5, "ProbeTuneGui: " + cmd);
            if (toker.hasMoreTokens()) {
                String sStop = toker.nextToken();
                try {
                    m_stopFreq = Double.parseDouble(sStop);
                } catch (NumberFormatException nfe) {
                }
                displaySpan(m_stopFreq - m_startFreq);
                displayCenter((m_stopFreq + m_startFreq) / 2);
                //txtSpan.setText(Fmt.fg(3, (m_stopFreq - m_startFreq) / 1e6));
                //txtCenter.setText(Fmt.fg(3, (m_stopFreq + m_startFreq) / 2e6));
            }
//        } else if (key.equals("setNp")) {
//            if (toker.hasMoreTokens()) {
//                String sNpNew = toker.nextToken();
//                try {
//                    int npNew = Integer.parseInt(sNpNew);
//                    if (m_np != npNew) {
//                        //setNP(np);
//                        m_np = npNew;
//                        txtNumberOfPoints.setText("" + npNew);
//                    }
//                } catch (NumberFormatException nfe) {
//                    Messages.postError("Bad NP field: \"" + sNpNew + "\"");
//                }
//            }
        } else if (key.equals("motorOk")) {
            boolean ok = false;
            if (toker.hasMoreTokens()) {
                if (toker.nextToken().startsWith("y")) {
                    ok = true;
                }
            }
            m_motorOk = ok;
            if (ok) {
                motorMsg.setText("Motor communication OK");
                motorMsg.setForeground(Color.BLACK);
                displayStatus(STATUS_READY);
            } else {
                motorMsg.setText("No motor communication");
                motorMsg.setForeground(Color.RED);
                displayStatus(STATUS_NOTREADY);
            }
        } else if (key.equals("sweepOk")) {
            boolean ok = false;
            if (toker.hasMoreTokens()) {
                if (toker.nextToken().startsWith("y")) {
                    ok = true;
                }
            }
            m_sweepOk = ok;
            if (ok) {
                sweepMsg.setText("Reflection data OK");
                sweepMsg.setForeground(Color.BLACK);
                //displayStatus(STATUS_PREVIOUS);
            } else {
                sweepMsg.setText("No reflection data");
                sweepMsg.setForeground(Color.RED);
                //displayStatus(STATUS_NOTREADY);
            }
        } else if (key.equals("setHzPerStep")) {
            try {
                double value = Double.valueOf(toker.nextToken());
                String motor = toker.nextToken();
                if (motor.equalsIgnoreCase("Tune")) {
                    // Tuning motor
                    motor = "Tune";
                    setKhzPerStep(txtTuneKhzPerStep, value, motor);
                } else if (motor.equalsIgnoreCase("Match")) {
                    // Match motor
                    motor = "Match";
                    setKhzPerStep(txtMatchKhzPerStep, value, motor);
                }
            } catch (NumberFormatException nfe) {
            } catch (NoSuchElementException nsee) {
            }
        } else if (key.equals("setMatchPerStep")) {
            try {
                double value = Double.valueOf(toker.nextToken());
                String motor = toker.nextToken();
                if (motor.equalsIgnoreCase("Tune")) {
                    // Tuning motor
                    motor = "Tune";
                    setMatchPerStep(txtTuneReflPerStep, value, motor);
                } else if (motor.equalsIgnoreCase("Match")) {
                    // Match motor
                    motor = "Match";
                    setMatchPerStep(txtMatchReflPerStep, value, motor);
                }
            } catch (NumberFormatException nfe) {
            } catch (NoSuchElementException nsee) {
            }
        } else if (key.equals("setLastStep")) {
            int gmi = -1;
            int step = 0;
            if (toker.countTokens() == 2) {
                String strMotor = toker.nextToken();
                try {
                    gmi = Integer.parseInt(strMotor);
                } catch (NumberFormatException nfe) {
                }
                String strStep = toker.nextToken();
                try {
                    step = Integer.parseInt(strStep);
                } catch (NumberFormatException nfe) {
                }
            }
            setLastStep(gmi, step);
        } else if (key.equals("setRefl")) {
            double value = 0;
            if (toker.hasMoreTokens()) {
                String sValue = toker.nextToken();
                try {
                    value = Double.parseDouble(sValue);
                } catch (NumberFormatException nfe) {
                }
            }
            displayRefl(value);
        } else if (key.equals("setStatus")) {
            if (toker.hasMoreTokens()) {
                int stt = 0;
                try {
                    stt = Integer.parseInt(toker.nextToken());
                } catch (NumberFormatException nfe) {
                }
                displayStatus(stt);
            }
        } else if (key.equals("setReceived")) {
//            if (toker.hasMoreTokens()) {
//                String recCmd= toker.nextToken();
//                String cmdTxtCmd= txtCmd.getText();
//
//                StringTokenizer cmdTxtToker = new StringTokenizer(cmdTxtCmd, " ");
//
//                while(cmdTxtToker.hasMoreTokens()){
//                        String cmdWord = cmdTxtToker.nextToken();
//                        if(recCmd.startsWith(cmdWord) || (cmdWord.startsWith("getflag") || (cmdWord.startsWith("version")))){
//                                while(toker.hasMoreTokens()){
//                                        recCmd= recCmd + " " + toker.nextToken();
//                                }
//                                setRec(recCmd);
//                                break;
//                        }
//                }
//            }
        } else if (key.equals("setMLStatus")) {
            if (toker.hasMoreTokens()) {
                int stt = 0;
                try {
                    stt = Integer.parseInt(toker.nextToken());
                } catch (NumberFormatException nfe) {
                }
                if (stt == -1) displayStatus(STATUS_MLNOTREADY);
                else displayStatus(STATUS_PREVIOUS);
            }
        } else if (key.equals("displayBandSwitch")) {
            if (toker.hasMoreTokens()) {
                int band = 0;
                try {
                    band = Integer.parseInt(toker.nextToken());
                    switch (band) {
                    case 0:
                        //TODO cmdBandHigh.setSelected(true);
                        //TODO cmdBandLow.setSelected(false);
                        break;
                    case 1:
                        //TODO cmdBandHigh.setSelected(false);
                        //TODO cmdBandLow.setSelected(true);
                        break;
                    default:
                        //TODO cmdBandHigh.setSelected(false);
                        //TODO cmdBandLow.setSelected(false);
                    }
                } catch (NumberFormatException nfe) {
                }
            }
        } else if (key.equals("displayPhase")) {
            String phase = "--";
            boolean isFixed = false;
            if (toker.hasMoreTokens()) {
                phase = toker.nextToken();
            }
            if (toker.hasMoreTokens()) {
                isFixed = "fixed".equalsIgnoreCase(toker.nextToken());
            }
            displayPhase(phase, isFixed);
        } else if (key.equals("displayProbeDelay")) {
            String delay = "--";
            if (toker.hasMoreTokens()) {
                delay = toker.nextToken();
            }
            displayProbeDelay(delay);
        } else if (key.equals("displayChannel")) {
            if (toker.hasMoreTokens()) {
                displayChannel(toker.nextToken("").trim()); // Rest of string
            }
        } else if (key.equals("displayMatchTolerance")) {
            if (toker.hasMoreTokens()) {
                String strTol = toker.nextToken();
                try {
                    double tol_db = Math.abs(Double.parseDouble(strTol));
                    m_targetMatch = new TuneCriterion(tol_db).getTarget_pwr();
                    txtMatchThresh.setText("-" + Fmt.f(0, tol_db));
                } catch (NumberFormatException nfe) {
                    if (TuneCriterion.isStandardCriterion(strTol)) {
                        txtMatchThresh.setText(strTol);
                        m_targetMatch
                           = TuneCriterion.getCriterion(strTol).getTarget_pwr();
                    }
                }
                absPlot.clearIcons();
                absPlot.addIcon(m_targetIcon,
                                m_targetFreq / 1.e6,
                                Math.sqrt(m_targetMatch));
                absPlot.repaint();
            }
        } else if (key.equals("displayTuneMode")) {
            if (toker.countTokens() == 1) {
                String strMode = toker.nextToken();
                try {
                    int mode = Integer.parseInt(strMode);
                    displayTuneMode(mode);
                } catch (NumberFormatException nfe) {
                }
            }
        } else if (key.equals("displayCommand")) {
            if (toker.countTokens() == 1) {
                txtCmd.setText(toker.nextToken());
            }
        } else if (key.equals("displayMotorPosition")) {
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int position = Integer.parseInt(toker.nextToken());
                displayMotorPosition(gmi, position);
            } catch (NoSuchElementException nsee) {
                postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("displayMotorMotion")) {
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int direction = Integer.parseInt(toker.nextToken());
                displayMotorMotion(gmi, direction);
            } catch (NoSuchElementException nsee) {
                postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("displayMotorLimits")) {
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int min = Integer.parseInt(toker.nextToken());
                int max = Integer.parseInt(toker.nextToken());
                displayMotorLimits(gmi, min, max);
            } catch (NoSuchElementException nsee) {
                postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("displayMotorName")) {
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                String name = toker.nextToken("");
                displayMotorName(gmi, name);
            } catch (NoSuchElementException nsee) {
                //postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("displayMotorEmphasis")) {
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                boolean emphasized = Boolean.parseBoolean(toker.nextToken());
                displayMotorEmphasis(gmi, emphasized);
            } catch (NoSuchElementException nsee) {
                postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("displayMotorPresent")) {
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                boolean isUsed = Boolean.parseBoolean(toker.nextToken());
                displayMotorPresent(gmi, isUsed);
            } catch (NoSuchElementException nsee) {
                postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("displayMotorBacklash")) {
            try {
                int gmi = Integer.parseInt(toker.nextToken());
                int backlash = Integer.parseInt(toker.nextToken());
                displayMotorBacklash(gmi, backlash);
            } catch (NoSuchElementException nsee) {
                postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("displayRawDataMode")) {
            try {
                int mode = Integer.parseInt(toker.nextToken());
                if (mode == 0 && !cmdRawDataOff.isSelected()) {
                    cmdRawDataOff.setSelected(true);
                } else if (mode != 0 && !cmdRawDataOn.isSelected()) {
                    cmdRawDataOn.setSelected(true);
                }
            } catch (NoSuchElementException nsee) {
                postCommandError(cmd);
            } catch (NumberFormatException nfe) {
                postCommandError(cmd);
            }
        } else if (key.equals("popup")) {
            String type = "info";
            String title = "";
            String msg = "";
            if (toker.hasMoreTokens()) {
                type = toker.nextToken();
            }
            while (toker.hasMoreTokens()) {
                String subkey = toker.nextToken();
                if ("title".equals(subkey) && toker.hasMoreTokens()) {
                    title = toker.nextToken();
                    title = title.replaceAll("_", " ");
                } else if ("msg".equals(subkey) && toker.hasMoreTokens()) {
                    // Grab the rest of the string
                    msg = toker.nextToken("").trim();
                    msg = msg.replaceAll("<br>", NL);
                }
            }
            showPopup(type, title, msg);
        } else if (key.equalsIgnoreCase("SetMark")){
            String mark = "points";
            if (toker.hasMoreTokens()) {
                mark = toker.nextToken();
            }
            setMarksStyle(mark);
        } else if (key.equalsIgnoreCase("RfCal")) {
            calibrateRF();
        } else if (key.equalsIgnoreCase("CalDataSet")) {
            if (m_calPopup != null) {
                m_calPopup.calDataSet();
            }
        } else if (key.equalsIgnoreCase("UseRefs")) {
            if (toker.hasMoreTokens()) {
                boolean isOn = !toker.nextToken().equals("0");
                cmdHistoryOn.setSelected(isOn);
                cmdHistoryOff.setSelected(!isOn);
            }
        } else if (key.equalsIgnoreCase("setSysInitialized")) {
            m_sysInitialized = !toker.nextToken().equals("0");
        } else {
            Messages.postError("ProbeTuneGui got unrecognized command: \""
                               + key + "\"");
        }
    }

    /**
     * @param mark The name of the desired mark.
     */
    protected void setMarksStyle(String mark) {
        absPlot.setMarksStyle(mark);
        polarPlot.setMarksStyle(mark);
    }

    protected void addTorqueData(StringTokenizer toker) {
        // Don't expect to get this except in TorqueGui
    }

    protected void shiftTorqueData(StringTokenizer toker) {
        // Don't expect to get this except in TorqueGui
    }

    /**
     * @param cmd The command string, containing the data.
     */
    protected void setData(String cmd) {
        m_reflectionData = new ReflectionData(cmd);
        if (DebugOutput.isSetFor("captureData")) {
            TuneUtilities.writeFile("reflData", cmd);
        }
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                displayPlot(m_reflectionData);
            }
        }
        );
    }

    public void showPopup(String type, String title, String msg) {
        int msgType = JOptionPane.INFORMATION_MESSAGE;
        if ("warn".equals(type)) {
            msgType = JOptionPane.WARNING_MESSAGE;
        } else if ("question".equals(type)) {
            msgType = JOptionPane.QUESTION_MESSAGE;
        } else if ("info".equals(type)) {
            msgType = JOptionPane.INFORMATION_MESSAGE;
        } else if ("error".equals(type)) {
            msgType = JOptionPane.ERROR_MESSAGE;
        } else if ("plain".equals(type)) {
            msgType = JOptionPane.PLAIN_MESSAGE;
        }

        JOptionPane.showMessageDialog(this, msg, title, msgType);
    }

    /**
     * Reprocess the data to redraw the plot.
     * TODO: take into account any change in phase correction, etc.
     *  ...(SweepControl will need to save raw data)
     */
    private void redrawCurrentPlot() {
        // TODO: Just send "displayData to ProTune; it does getData if needed
        if (m_reflectionData == null) {
            getData();
        } else {
            cmdSender.println("displayData");
        }
    }

    public boolean sendCommand(String command) {
        if (cmdSender == null) {
            return false;
        } else {
            cmdSender.println(command);
            return true;
        }
    }

    private void postCommandError(String cmd) {
        Messages.postError("Bad command to ProbeTuneGui: " + cmd);
    }

    public void calibrateRF() {
        new MtuneCal(this, m_probeName);
    }

    public void setCalPopup(MtuneCal calPopup) {
        m_calPopup = calPopup;
    }




    /**
     * A panel that contains the components for controlling and viewing
     * the state of one motor.
     * <pre>
     *  name label:      #n {slave to m|tune|match|switch}
     *  position bar:    [progress bar with position label]
     *                   black=>not moving, red=>CCW(-), green=>CW(+)
     *  position entry:  [entry]
     *  step size entry: [entry]
     *  step buttons:    [-] [+]
     * </pre>
     */
    class MotorPanel extends JPanel implements ActionListener {

        private static final String CMD_GOTO = "goto";
        private static final String CMD_STEPSIZE = "stepsize";
        private static final String CMD_PLUS_STEP = "plusstep";
        private static final String CMD_MINUS_STEP = "minusstep";

        private final Border mm_plainBorder
                = BorderFactory.createMatteBorder(1, 1, 1, 1, Color.white);
        private final Border mm_emphasisBorder
        = BorderFactory.createMatteBorder(1, 1, 1, 1, Color.black);

        /** Global index of motor this panel represents. */
        private int mm_gmi;
        private JLabel mm_nameLabel = new JLabel();
        private JProgressBar mm_positionBar = new JProgressBar();
        private JLabel mm_positionBarText = new JLabel("0");
        private JLabel mm_positionLabel = new JLabel("Go To:");
        private JTextField mm_positionEntry = new JTextField(4);
        private JLabel mm_backlashLabel = new JLabel("Backlash:");
        private JLabel mm_backlashEntry = new JLabel("0");
        private JLabel mm_stepSizeLabel = new JLabel("Step Size:");
        private JTextField mm_stepSizeEntry = new JTextField(3);
        private JButton mm_stepMinusButton = new JButton();
        private JButton mm_stepPlusButton = new JButton();
        private Icon mm_minusIcon = TuneUtilities.getIcon("minus.png",
                                                          getClass());
        private Icon mm_plusIcon = TuneUtilities.getIcon("plus.png",
                                                         getClass());

        /**
         * Create a control panel for a given motor.
         * @param gmi Global index of motor.
         */
        MotorPanel(int gmi) {
            this(gmi, false);
        }

        /**
         * Create a control panel for a given motor.
         * @param gmi Global index of motor.
         * @param isSimple TODO
         */
        MotorPanel(int gmi, boolean isSimple) {
            mm_gmi = gmi;

            Box panel = Box.createVerticalBox();
            add(panel);

            JPanel labelPanel = new JPanel();
            labelPanel.setBackground(null);
            labelPanel.setLayout(new BorderLayout());
            labelPanel.add(mm_nameLabel, BorderLayout.WEST);
            setName("");
            panel.add(labelPanel);

            // NB: Call setStringPainted(false) on the progress bar and draw
            // our own label on top of the bar. That's just because I don't
            // know how to control the color of the progress bar's text.
            mm_positionBar.setPreferredSize(new Dimension(100, 20));
            mm_positionBar.setStringPainted(false);
            mm_positionBar.setBorderPainted(true);
            setPositionLimits(0, 10000);
            setPosition(0);
            setMotion(0);
            panel.add(mm_positionBar);
            mm_positionBarText.setOpaque(false);
            mm_positionBarText.setHorizontalAlignment(SwingConstants.CENTER);
            mm_positionBar.setLayout(new BorderLayout());
            mm_positionBar.add(mm_positionBarText, BorderLayout.CENTER);

            if (!isSimple) {
                JPanel entryPanel = new JPanel();
                entryPanel.setBackground(null);
                entryPanel.setLayout(new BorderLayout());
                entryPanel.add(mm_positionLabel, BorderLayout.WEST);
                mm_positionEntry.setHorizontalAlignment(JTextField.RIGHT);
                mm_positionEntry.setActionCommand(CMD_GOTO);
                mm_positionEntry.addActionListener(this);
                entryPanel.add(mm_positionEntry, BorderLayout.EAST);
                panel.add(entryPanel);

                JPanel backlashPanel = new JPanel();
                backlashPanel.setBackground(null);
                backlashPanel.setLayout(new BorderLayout());
                backlashPanel.add(mm_backlashLabel, BorderLayout.WEST);
                mm_backlashEntry.setHorizontalAlignment(JTextField.RIGHT);
                //mm_backlashEntry.setActionCommand(CMD_BACKLASH);
                //mm_backlashEntry.addActionListener(this);
                //setBacklash(50);
                backlashPanel.add(mm_backlashEntry, BorderLayout.EAST);
                panel.add(backlashPanel);

                JPanel stepsizePanel = new JPanel();
                stepsizePanel.setBackground(null);
                stepsizePanel.setLayout(new BorderLayout());
                stepsizePanel.add(mm_stepSizeLabel, BorderLayout.WEST);
                mm_stepSizeEntry.setHorizontalAlignment(JTextField.RIGHT);
                mm_stepSizeEntry.setActionCommand(CMD_STEPSIZE);
                mm_stepSizeEntry.addActionListener(this);
                setStepSize(50);
                stepsizePanel.add(mm_stepSizeEntry, BorderLayout.EAST);
                panel.add(stepsizePanel);

                JPanel stepPanel = new JPanel();
                stepPanel.setBackground(null);
                mm_stepMinusButton.setIcon(mm_minusIcon);
                mm_stepPlusButton.setIcon(mm_plusIcon);
                mm_stepMinusButton.setActionCommand(CMD_MINUS_STEP);
                mm_stepPlusButton.setActionCommand(CMD_PLUS_STEP);
                mm_stepMinusButton.addActionListener(this);
                mm_stepPlusButton.addActionListener(this);
                Dimension dim = new Dimension(30, 20);
                mm_stepMinusButton.setPreferredSize(dim);
                mm_stepPlusButton.setPreferredSize(dim);
                stepPanel.add(mm_stepMinusButton);
                stepPanel.add(mm_stepPlusButton);
                panel.add(stepPanel);
            }
        }

        public void setName(String name) {
            mm_nameLabel.setText("# " + mm_gmi + ":" + name);
        }

        public int getMotorNumber() {
            return mm_gmi;
        }

        public void setPositionLimits(int min, int max) {
            mm_positionBar.setMinimum(min);
            mm_positionBar.setMaximum(max);
        }

        public void setPosition(int position) {
            mm_positionBar.setValue(position);
            //mm_positionBar.setString("" + position);
            mm_positionBarText.setText("" + position);
        }

        public void setMotion(int direction) {
            Color fg = new Color(200, 200, 200);
            Color bg = Color.white;
            if (direction < 0) {
                fg = Color.red;
                bg = Color.pink;
            } else if (direction > 0) {
                fg = Color.green;
                bg = new Color(200, 255, 200);
            }
            mm_positionBar.setForeground(fg);
            mm_positionBar.setBackground(bg);
        }

        /**
         * Just validates the string typed into the entry box.
         */
        public void setStepSize() {
            try {
                Integer.parseInt(mm_stepSizeEntry.getText());
                mm_stepSizeEntry.setBackground(Color.white);
            } catch (NumberFormatException nfe) {
                mm_stepSizeEntry.setBackground(Color.pink);
            }
        }

        public void setStepSize(int stepsize) {
            mm_stepSizeEntry.setText("" + Math.abs(stepsize));
        }

        public void setBacklash(int backlash) {
            mm_backlashEntry.setText("" + Math.abs(backlash));
        }

        /**
         * Get the current step size for this motor.
         * This is the absolute value of the number of steps taken if the
         * "+" or "-" step button is pressed.
         * @return The step size.
         */
        public int getStepSize() {
            int rtn = 0;
            try {
                rtn = Integer.parseInt(mm_stepSizeEntry.getText());
                mm_stepSizeEntry.setBackground(Color.white);
            } catch (NumberFormatException nfe) {
                mm_stepSizeEntry.setBackground(Color.pink);
            }
            return Math.abs(rtn);
        }

        public void setEnabled(boolean enabled) {
            setVisible(enabled);
//            mm_nameLabel.setEnabled(enabled);
//            mm_positionBar.setEnabled(enabled);
//            mm_positionBarText.setEnabled(enabled);
//            mm_positionLabel.setEnabled(enabled);
//            mm_positionEntry.setEnabled(enabled);
//            mm_backlashLabel.setEnabled(enabled);
//            mm_backlashEntry.setEnabled(enabled);
//            mm_stepSizeLabel.setEnabled(enabled);
//            mm_stepSizeEntry.setEnabled(enabled);
//            mm_stepMinusButton.setEnabled(enabled);
//            mm_stepPlusButton.setEnabled(enabled);
        }

        public void setEmphasized(boolean isEmphasized) {
            if (isEmphasized) {
                setBackground(new Color(200, 200, 255));
            } else {
                setBackground(null);
            }
        }

        public void actionPerformed(ActionEvent event) {
            String command = event.getActionCommand();
            if (CMD_GOTO.equals(command)) {
                try {
                    int position = Integer.parseInt(mm_positionEntry.getText());
                    cmdSender.println("gotoGMI " + mm_gmi + " " + position);
                    cmdSender.println("trackTune 1");
                } catch (NumberFormatException nfe) {
                }
            } else if (CMD_STEPSIZE.equals(command)) {
                int size = getStepSize();
                try {
                    size = Integer.parseInt(mm_stepSizeEntry.getText());
                    size = Math.abs(size);
                } catch (NumberFormatException nfe) {
                }
                mm_stepSizeEntry.setText("" + size);
            } else if (CMD_PLUS_STEP.equals(command)) {
                cmdSender.println("stepGMI " + mm_gmi + " " + getStepSize());
            } else if (CMD_MINUS_STEP.equals(command)) {
                cmdSender.println("stepGMI " + mm_gmi + " -" + getStepSize());
            }
        }

        public void highlightMinusButton() {
            mm_stepPlusButton.setBorder(mm_plainBorder);
            mm_stepMinusButton.setBorder(mm_emphasisBorder);
        }

        public void highlightPlusButton() {
            mm_stepMinusButton.setBorder(mm_plainBorder);
            mm_stepPlusButton.setBorder(mm_emphasisBorder);
        }
    }
}


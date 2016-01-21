/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 *
 */

package vnmr.cryomon;
import javax.swing.plaf.metal.*;

import java.io.*;
import java.lang.management.ManagementFactory;
import java.util.*;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.plaf.metal.MetalLookAndFeel;
import javax.swing.border.*;
import java.text.*;

import vnmr.cryomon.CryoMonitorSocketControl;

import vnmr.bo.Plot;

import java.awt.*;
import java.awt.event.*;

import static javax.swing.ScrollPaneConstants.*;

import static java.awt.Color.WHITE;

public class CryoMonitorControls extends JFrame
       implements CryoMonitorDefs, WindowListener, ActionListener,ChangeListener {
    private Container setupPane;
    
    //Strings
    private static final String CMD_RABBIT = "SEND_RABBIT";
    private static final String CMD_HEHIST = "HEHIST";
    private static final String CMD_HEREAD = "HEREAD";
    private static final String CMD_N2HIST = "N2HIST";
    private static final String CMD_N2READ = "N2READ";
    private static final String CHECK_ERRORS = "checkErrors";
    private static final String ADVANCED = "advanced";
    private static final String	ABOUT_CONT = "cryomon";
    private static final String	PANEL_VERN = "1.5";
    private static final String	PANEL_DATE = "November, 2010";
    private static final String CLOSE_COMMAND = "Close";
    private static final String READ_LOG = "readLog";
    private static final String LOG_TO_HIST = "LogToHist";
    private static final String SAVE_HIST = "saveHistory";
    private static final String CLEAR_HIST = "clearHist";
    private static final String CLEAR_MSGS = "clearMsgs";
    private static final String CLEAR_LOG = "clearLog";
    private static final String SEL_N2      = "selN2";
    private static final String SEL_HE      = "selHe";
    private static final String SEL_LVL     = "selLvl";
    private static final String RESTART 	= "RESTART";
  
    private   static final int CRYO_PORT      = 23;
    protected static final int FIRST          = 0;
    protected static final int LAST           = 1;
    protected static final int FILL           = 2;
    protected static final int PANEL_WIDTH    = 580;   // width of main panel
    protected static final int PANEL_HEIGHT   = 115;   // height of main panel
    protected static final int PLOT_WIDTH     = 800;   // width of plot 
    protected static final int PLOT_HEIGHT    = 300;   // height of plot
    
    protected static final int STATUS_UPDATE  = 600;   // check status every 10 minutes
    protected static final int DATA_UPDATE    = 6*8;   // update fill data file every 8 hours

    //protected static final int STATUS_UPDATE  = 60;   // check status every 10 minutes
    //protected static final int DATA_UPDATE    = 60*8;   // update fill data file every 8 hours
   //version UID
    private static final long serialVersionUID= 82679L;
    
    Font bold= new Font(null, Font.BOLD, 12);


    // Current Settings
    String ipAddr = null;

    // Display Components
    private JCheckBoxMenuItem advanced;
    private JPanel fullPanel;

    // Main Panel
    private JPanel mainPanel;
    
    private JLabel lblN2, lblHe,lblLevel;
    private JButton cmdHistN2, cmdHistHe;
    public JProgressBar pbN2, pbHe;
    private JButton cmdReadN2, cmdReadHe;
    private JTextField txtHeMeas,txtN2Meas;
    private JComboBox selHeMeas,selN2Meas,selLevelMode,selLevelType;
    
    private String firstRead[]={"","",""};
    private String lastRead[]={"","",""};
    private String fillDate[]={"","",""};
    
    //Advanced Panel
       
    private JPanel advPanel;
    public JTextField txtsCryocon=null;
    public JTextField txtsRabbit=null;
    public JTextField txtsMaxHist=null;

    public JSlider sldrLevel=null;
    private JButton cmdSendRabbit=null;
    private JButton cmdSaveLog,cmdClearHist,cmdClearMsgs,cmdClearLog,cmdConnection,cmdRebuildHist;
    private JLabel lblCmdLine,lblClear;//,lblMaxLines;
    private JCheckBox chkRestartServer;
    private JTextArea textArea;
    private JScrollPane msgScrollPane;
    
    //Plot Popup Frame
    private JFrame hePlotFrame=null;
    private JPanel hePlotPane;
    private JFrame n2PlotFrame=null;
    private JPanel n2PlotPane;

	// Settings file and Title Logo Image file
	
	private static String DataDir=null;
	private static String iconFile= null;
	private static CryoMonitorSocketControl rabbit23=null;
	
	//History Graphs
	private MyPlot hePlot=null;
	private MyPlot n2Plot=null;
	private boolean f_noDataFile= false;
	private static boolean nogui= false;
	//private static boolean vnmrj= false;
	private boolean f_readLog= false;
	private boolean f_log2hist= false;
	private static boolean server= false;
	private static boolean connected= false;
	private static boolean restart=true;
	private static boolean window_closed=false;
	private String m_logFile= null;
	
	private boolean error_request=false;
	private Date last_date=null;
    protected int dataUpdate=DATA_UPDATE;
    protected int dataCount=0;
    protected int initialDelay=2000;
    public static boolean debug = false;
	static boolean first=true;
   
    private Process serverProcess=null;
    
    Dimension btnSize=new Dimension(80,25);
    
    private String processID="";

    private static StatusPoller m_statusPoller = null;
    
	public CryoMonitorControls() {

		super("CryoMonitor");
		setVisible(false);
		
		DataFileManager.init(DataDir);
		
		processID=ManagementFactory.getRuntimeMXBean().getName();
 		if(!nogui){
 			processID+="@gui";
 		}
		
		if(server){
			DataFileManager.killServer(); // kills previous server process
			DataFileManager.waitReconnectTime(); // adds minimum delay to reconnect socket
					
			if(DataFileManager.setLock(processID)){
				rabbit23 = new CryoMonitorSocketControl("V-CryogenMonitor", CRYO_PORT,
						DataDir, STATUS_UPDATE, DATA_UPDATE);
				rabbit23.setCryoPanel(this);
				connected=rabbit23.isConnected();
			}
			if(!connected){
				server=false;
				restart=false;
				cryomonNotCommunicating();  // leads to exit
			}
	    	DataFileManager.writeMsgLog("New Server", debug);   	
		}

		this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		this.addWindowListener(this);
		if (iconFile != null)
			this.setIconImage(new ImageIcon(iconFile, "768AS icon").getImage());

		// Set up the content pane.
		setupPane = this.getContentPane();
		BorderLayout layout = new BorderLayout();
		setupPane.setLayout(layout);

		// Set up the main panel with scroll bars
		fullPanel = new JPanel();
		SpringLayout mainLayout = new SpringLayout();
		fullPanel.setLayout(mainLayout);
		fullPanel.setPreferredSize(new Dimension(600, 260));	
    	
		DataFileManager.readPrefs(); // read preferences from a file
		
		// Create and add the components.
		createMainPanel();
		buildMenuBar();
		if(server){
			
			createAdvancedPanel();

			SpringLayout.Constraints p1cons;
			SpringLayout.Constraints p4cons;
	
			// set panel constraints
			p1cons = mainLayout.getConstraints(mainPanel);			
			p4cons = mainLayout.getConstraints(advPanel);
	
			// Set up Springs
			p4cons.setX(p1cons.getX());
	
			p1cons.setY(Spring.constant(12));
			p4cons.setY(Spring.sum(p1cons.getConstraint(SpringLayout.SOUTH), Spring
					.constant(12)));
	
			fullPanel.add(mainPanel);
			fullPanel.add(advPanel);
			
			//setMaxHistory();
			advanced.setState(false);
		}
		else{
			fullPanel.add(mainPanel);
		}
		validate();

		setupPane.add(fullPanel);
		setupPane.setPreferredSize(new Dimension(500, 500));

		//this.setSize(new Dimension(500, 500));

		setResizable(false);

	    fullPanel.setPreferredSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
	    setupPane.setPreferredSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
	    this.setSize(new Dimension(PANEL_WIDTH, 300));	
		this.pack();
		this.setVisible(!nogui);
		this.addWindowListener(this);

		if (!nogui) {
			if (server)
				setTitle("CryoMonitor - Server");
			else
				setTitle("CryoMonitor");

			createPlotPanels();
			plotFillData();
			if (server && connected && rabbit23 != null) {
				//cmdConnection.setEnabled(false);
				//cmdConnection.setText(" Server Running ");
				rabbit23.pollStatus();
			} else
				pollStatus();
			if (!DataFileManager.isLocked()) {
				JOptionPane.showMessageDialog(this,
						"Cryogen Server Not Running",
						"Cryogen Monitor",
						JOptionPane.WARNING_MESSAGE);
			}
		} else {
			if (connected && rabbit23 != null)
				rabbit23.pollStatus();
		}
		return;
	}
   
    private void createPlotPanels(){   	
    	//set up History plots
        hePlot= new MyPlot();
        hePlot.setPreferredSize(new Dimension(PLOT_WIDTH, PLOT_HEIGHT));
        hePlot.setBackground(null);
        hePlot.setGrid(true);
        hePlot.setGridColor(Color.WHITE);
        hePlot.setPlotBackground(Color.LIGHT_GRAY);
        hePlot.setXAxis(true);
        hePlot.setYAxis(true);
        hePlot.setFillButton(false);
        
        hePlotPane= new JPanel();
        hePlotFrame= new JFrame();

        hePlotPane.setLayout( new BorderLayout() );
        hePlotPane.add(hePlot);
        
        hePlotFrame.setContentPane(hePlotPane);
        hePlotFrame.setSize(PLOT_WIDTH, PLOT_HEIGHT);
        Dimension screenSize =
            Toolkit.getDefaultToolkit().getScreenSize();
        hePlotFrame.setLocation(screenSize.width/4 - 150,
                screenSize.height/4 - 75);
		hePlot.setMarkColor(FILL_COLOR, 2);
		hePlot.setMarksStyle("none",2);
		hePlot.addLegend(2,"fill");
		hePlot.setMarkColor(WARN_COLOR, 3);
		hePlot.addLegend(3,"warn");
		hePlot.setMarksStyle("none",3);
		hePlot.setMarkColor(ALERT_COLOR, 4);
		hePlot.addLegend(4,"alert");
		hePlot.setMarksStyle("none",4);
		hePlot.setFillButton(true);
        hePlot.setTitle("Helium");
        hePlot.setYLabel("% Capacity");

        hePlot.setMarkColor(Color.BLACK, 0);
        hePlot.setMarkColor(Color.BLACK, 1);

		hePlot.setYRange(0.0, 100.0);
		hePlot.setYFullRange(0.0, 100.0);
		hePlot.setYFillRange(0.0, 100.0);

   		hePlot.setMarksStyle("none",0);
   		hePlot.setMarksStyle("dots",1);
   		hePlot.setConnected(true,0);
   		hePlot.setConnected(false,1);
		hePlot.setXTimeOfDay(true);
		hePlot.setXLabel("Date");
   		     
        n2Plot= new MyPlot();

        n2Plot.setPreferredSize(new Dimension(PLOT_WIDTH, PLOT_HEIGHT));
        n2Plot.setBackground(null);
        n2Plot.clearLegends();
        n2Plot.setGrid(true);
        n2Plot.setGridColor(Color.WHITE);
        n2Plot.setPlotBackground(Color.LIGHT_GRAY);
        n2Plot.setXAxis(true);
        n2Plot.setYAxis(true);
        n2Plot.setYLabel("% Capacity");
        n2Plot.setFillButton(false);
        
        n2PlotPane= new JPanel();
        n2PlotFrame= new JFrame();

        n2PlotPane.setLayout( new BorderLayout() );
        n2PlotPane.add(n2Plot);
        
        n2PlotFrame.setContentPane(n2PlotPane);
        n2PlotFrame.setSize(PLOT_WIDTH, PLOT_HEIGHT);
        n2PlotFrame.setLocation(screenSize.width/4 - PLOT_HEIGHT,
                screenSize.height/4 - 150);
 
        n2Plot.setMarksStyle("dots",0);
        n2Plot.setMarkColor(Color.BLACK, 0);
        
        n2Plot.setMarkColor(FILL_COLOR, 2);
        n2Plot.setMarksStyle("none",2);
        n2Plot.addLegend(2,"fill");
        n2Plot.setMarkColor(WARN_COLOR, 3);
        n2Plot.addLegend(3,"warn");
        n2Plot.setMarksStyle("none",3);
        n2Plot.setMarkColor(ALERT_COLOR, 4);
        n2Plot.addLegend(4,"alert");
        n2Plot.setMarksStyle("none",4);
        
        n2Plot.setTitle("Nitrogen");
        n2Plot.setYLabel("% Capacity");
   		
        n2Plot.setFillButton(true);

		n2Plot.setYRange(0.0, 100.0);
		n2Plot.setYFullRange(0.0, 100.0);
		n2Plot.setYFillRange(0.0, 100.0);
		
        n2Plot.setXTimeOfDay(true);
        n2Plot.setXLabel("Date");
       
    }
    
    private void openHePlot(){
        if(f_noDataFile)
        	writeAdvanced("Data File not found\n");
        else
        	plotFillData();        
        hePlotFrame.setVisible(true);
    }
    
    private void openN2Plot(){
        if(f_noDataFile)
        	writeAdvanced("Data File not found\n");
         else
        	plotFillData();
        n2PlotFrame.setVisible(true);
    }

    private void createMainPanel(){
        //set Fonts for Main Panel
    	
    	mainPanel = new JPanel();
        mainPanel.setFont(bold);
        SpringLayout mainLayout = new SpringLayout();
        mainPanel.setLayout(mainLayout);
        //mainPanel.setLayout(new GridLayout(2,6));
        TitledBorder border= BorderFactory.createTitledBorder("Cryogen Levels");
        border.setTitleFont(bold);
        mainPanel.setBorder(border);
        mainPanel.setPreferredSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
        //mainPanel.setMaximumSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
        mainPanel.setMinimumSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
        
        
        //Set up progress bar colors
        UIManager.put("ProgressBar.selectionBackground", Color.BLUE);
        UIManager.put("ProgressBar.selectionForeground", Color.BLUE);
        
        
        String sels[]={"First","Last","Fill"};
        // Create components
        lblN2 = new JLabel("N2");
        lblN2.setFont(bold);
        pbN2= new JProgressBar(0, 100);
        pbN2.setStringPainted(true);
        cmdHistN2 = new JButton("History");
        cmdHistN2.setPreferredSize(btnSize);
        cmdHistN2.setFont(bold);
        cmdHistN2.setActionCommand(CMD_N2HIST);
        cmdReadN2 = new JButton("Read");
        cmdReadN2.setPreferredSize(btnSize);
        cmdReadN2.setFont(bold);
        cmdReadN2.setActionCommand(CMD_N2READ);
        selN2Meas = new JComboBox(sels);
        selN2Meas.setSelectedIndex(LAST);
        selN2Meas.setEditable(false);
        selN2Meas.setActionCommand(SEL_N2);
        txtN2Meas = new JTextField(10);
        txtN2Meas.setEditable(false);

        lblHe = new JLabel("He");
        lblHe.setFont(bold);
        pbHe= new JProgressBar(0, 100);
        pbHe.setStringPainted(true);
        cmdHistHe = new JButton("History");
        cmdHistHe.setPreferredSize(btnSize);
        cmdHistHe.setFont(bold);
        cmdHistHe.setActionCommand(CMD_HEHIST);
        cmdReadHe = new JButton("Read");
        cmdReadHe.setPreferredSize(btnSize);
        cmdReadHe.setFont(bold);
        cmdReadHe.setActionCommand(CMD_HEREAD);
        selHeMeas = new JComboBox(sels);
        selHeMeas.setSelectedIndex(LAST);
        selHeMeas.setEditable(false);
        selHeMeas.setActionCommand(SEL_HE);
        txtHeMeas = new JTextField(10);
        txtHeMeas.setEditable(false);
        
        cmdHistHe.addActionListener(this);
        cmdHistN2.addActionListener(this);
        cmdReadHe.addActionListener(this);
        cmdReadN2.addActionListener(this);
        
        selHeMeas.addActionListener(this);
        selN2Meas.addActionListener(this);
 
        mainPanel.add(lblN2);
        mainPanel.add(pbN2);
        mainPanel.add(cmdHistN2);
        mainPanel.add(cmdReadN2);
        mainPanel.add(selN2Meas);
        mainPanel.add(txtN2Meas);
               
        mainPanel.add(lblHe);
        mainPanel.add(pbHe);
        mainPanel.add(cmdHistHe);
        mainPanel.add(cmdReadHe);
        mainPanel.add(selHeMeas);
        mainPanel.add(txtHeMeas);
        
        //Set Constraints
        SetSpringsMain(mainLayout);
        //System.out.println(border.getMinimumSize(mainPanel));
     }
    
    private void createAdvancedPanel(){
    	advPanel = new JPanel();
    	advPanel.setVisible(false);
        SpringLayout advLayout = new SpringLayout();
        advPanel.setLayout(advLayout);
        TitledBorder border=BorderFactory.createTitledBorder("Advanced");
        border.setTitleFont(bold);
        advPanel.setBorder(border);
        
        int height=370;
        advPanel.setPreferredSize(new Dimension(PANEL_WIDTH, height));
        advPanel.setMaximumSize(new Dimension(PANEL_WIDTH, height));
        advPanel.setMinimumSize(new Dimension(PANEL_WIDTH, height));
        
        String sels[]={"Nitrogen","Helium"};
        selLevelMode = new JComboBox(sels);
        selLevelMode.setSelectedIndex(NITROGEN);
        selLevelMode.setActionCommand(SEL_LVL);
        selLevelMode.addActionListener(this);
        advPanel.add(selLevelMode);

        String lvls[]={"Fill","Warning","Alert"};       
        selLevelType = new JComboBox(lvls);
        selLevelType.setSelectedIndex(SEL_FILL);
        selLevelType.setActionCommand(SEL_LVL);
        selLevelType.addActionListener(this);
        advPanel.add(selLevelType);
 
        lblLevel = new JLabel("Level ");
        advPanel.add(lblLevel);
        
        sldrLevel = new JSlider(0,100);
        sldrLevel.setMajorTickSpacing(10);
        sldrLevel.setMinorTickSpacing(1);
        sldrLevel.setPaintTicks(true);
        sldrLevel.setPaintLabels(true);
        sldrLevel.setPreferredSize(new Dimension(270,50));
        Font font = new Font("Serif", Font.ITALIC, 10);
        sldrLevel.setFont(font);
        sldrLevel.addChangeListener(this);
        advPanel.add(sldrLevel);
        
        lblClear = new JLabel("Clear");
        advPanel.add(lblClear);

        cmdClearHist = new JButton("History");
        cmdClearHist.setPreferredSize(btnSize);
        cmdClearHist.setActionCommand(CLEAR_HIST);
        cmdClearHist.addActionListener(this);
        advPanel.add(cmdClearHist);

        cmdClearMsgs = new JButton("Messages");
        cmdClearMsgs.setPreferredSize(btnSize);
        cmdClearMsgs.setActionCommand(CLEAR_MSGS);
        cmdClearMsgs.addActionListener(this);
        advPanel.add(cmdClearMsgs);

        cmdClearLog = new JButton("Log");
        cmdClearLog.setPreferredSize(btnSize);
        cmdClearLog.setActionCommand(CLEAR_LOG);
        cmdClearLog.addActionListener(this);
        advPanel.add(cmdClearLog);
        
//        cmdConnection=new JButton("Start Server");
//        cmdConnection.setActionCommand(CONNECT);
//        cmdConnection.addActionListener(this);
//        cmdConnection.setPreferredSize(new Dimension(140,25));
//        advPanel.add(cmdConnection);
        
        chkRestartServer=new JCheckBox("Restart Server On Exit",true);
        chkRestartServer.setActionCommand(RESTART);
        chkRestartServer.addActionListener(this);       
        advPanel.add(chkRestartServer);
        
        // These controls are only visible in "server" mode
        
        lblCmdLine = new JLabel("Command");
        advPanel.add(lblCmdLine);
        
        cmdSendRabbit = new JButton("Send ");
        cmdSendRabbit.setPreferredSize(btnSize);
        cmdSendRabbit.setAlignmentX(Component.CENTER_ALIGNMENT);
        cmdSendRabbit.setActionCommand(CMD_RABBIT);
        cmdSendRabbit.addActionListener(this);
        advPanel.add(cmdSendRabbit);
        
        txtsRabbit = new JTextField(6);
        txtsRabbit.setHorizontalAlignment(JTextField.RIGHT);
        txtsRabbit.setEditable(true);
        advPanel.add(txtsRabbit);
        
        cmdSaveLog = new JButton("Save Log");
        cmdSaveLog.setPreferredSize(new Dimension(100,25));
        cmdSaveLog.setActionCommand(READ_LOG);
        cmdSaveLog.addActionListener(this);     
        advPanel.add(cmdSaveLog);
   
        cmdRebuildHist = new JButton("Log->Hist");
        cmdRebuildHist.setPreferredSize(new Dimension(100,25));
        cmdRebuildHist.setActionCommand(LOG_TO_HIST);
        cmdRebuildHist.addActionListener(this);     
        advPanel.add(cmdRebuildHist);
        
        textArea = new JTextArea(10, 42);
        textArea.setEditable(false);
        textArea.setBackground(WHITE);
        msgScrollPane = new JScrollPane(textArea,
        		VERTICAL_SCROLLBAR_AS_NEEDED,
        		HORIZONTAL_SCROLLBAR_AS_NEEDED);
        advPanel.add(msgScrollPane);
    	setLevelValue();

        advPanel.setVisible(false);
        
    	SetSpringsAdvanced(advLayout);   	
    }
    
    public void writeAdvanced(String msg) {
    	if(connected && !nogui){
    		textArea.append(DataFileManager.getDateString()+" "+msg);
    	}
    }   
    
    private void buildMenuBar() {
        JMenuBar menuBar = new JMenuBar();
        menuBar.setOpaque(true);
        menuBar.setBorder(BorderFactory.createEmptyBorder());
        this.setJMenuBar(menuBar);

        // File menu
        JMenu fileMenu = new JMenu("File");
        menuBar.add(fileMenu);

        JMenuItem quit = new JMenuItem("Quit");
        //menuItem.setMnemonic(KeyEvent.VK_Q);
        quit.setActionCommand(CLOSE_COMMAND);
        quit.addActionListener(this);

        fileMenu.add(quit);
        
        if(server){
        	//  Options menu
	        JMenu optionsMenu = new JMenu("Options");
	        menuBar.add(optionsMenu);
	        if(server){
	        	JMenuItem showErrors = new JMenuItem("Show Errors");
	        	showErrors.setMnemonic(KeyEvent.VK_E);
	        	showErrors.setActionCommand(CHECK_ERRORS);
	        	showErrors.addActionListener(this);
		        optionsMenu.add(showErrors);
	        }
	        
	        advanced = new JCheckBoxMenuItem("Advanced");
	        advanced.setState(false);
	        advanced.setActionCommand(ADVANCED);
	        advanced.addActionListener(this);
	        optionsMenu.add(advanced);
        }
        
        //  Help menu
        JMenu helpMenu = new JMenu("Help");
        menuBar.add(helpMenu);
        
        JMenuItem aboutCont = new JMenuItem("About CryoMonitor ...");
        aboutCont.setActionCommand(ABOUT_CONT);
        aboutCont.addActionListener(this);
        helpMenu.add(aboutCont);       
    }

    void SetSpringsMain(SpringLayout mainLayout) {
    	
    	SpringLayout.Constraints P11, P12, P13, P14, P15, P16;
    	SpringLayout.Constraints P21, P22, P23, P24, P25, P26;
 
        P11 = mainLayout.getConstraints(lblN2);
        P12 = mainLayout.getConstraints(pbN2);
        P13 = mainLayout.getConstraints(cmdHistN2);
        P14 = mainLayout.getConstraints(cmdReadN2);
        P15 = mainLayout.getConstraints(selN2Meas);
        P16 = mainLayout.getConstraints(txtN2Meas);

        P22 = mainLayout.getConstraints(pbHe);
        P23 = mainLayout.getConstraints(cmdHistHe);       
        P24 = mainLayout.getConstraints(cmdReadHe);
        P25 = mainLayout.getConstraints(selHeMeas);
        P26 = mainLayout.getConstraints(txtHeMeas);
        
        P21 = mainLayout.getConstraints(lblHe);
                
        Spring sp0=Spring.constant(5);
        Spring sp1=Spring.constant(8);
        Spring sp2=Spring.constant(12);
        Spring sp3=Spring.constant(15);
              
        //set X 
        P11.setX(sp1);
        P12.setX(Spring.sum(P11.getConstraint(SpringLayout.EAST),sp0));
        P13.setX(Spring.sum(P12.getConstraint(SpringLayout.EAST),sp0));
        P14.setX(Spring.sum(P13.getConstraint(SpringLayout.EAST),sp0));
        P15.setX(Spring.sum(P14.getConstraint(SpringLayout.EAST),sp0));
        P16.setX(Spring.sum(P15.getConstraint(SpringLayout.EAST),sp0));
        
        P21.setX(P11.getX());
        P22.setX(P12.getX());
        P23.setX(P13.getX());
        P24.setX(P14.getX());
        P25.setX(P15.getX());
        P26.setX(P16.getX());
    
        //set Y 
        P11.setY(sp1);
        P12.setY(sp0);
        P13.setY(sp0);
        P14.setY(sp0);
        P15.setY(sp0);
        P16.setY(sp0);

        P21.setY(Spring.sum(P13.getConstraint(SpringLayout.SOUTH),sp1));
        P22.setY(Spring.sum(P22.getConstraint(SpringLayout.SOUTH),sp2));
        P23.setY(P22.getY());
        P24.setY(P22.getY());
        P25.setY(P22.getY());
        P26.setY(P22.getY());
    }
    
    private void SetSpringsAdvanced(SpringLayout advLayout){
    	SpringLayout.Constraints P01,P02,P03, P04;
    	SpringLayout.Constraints P11,P12,P13,P14,P15;
    	SpringLayout.Constraints P21,P22,P23,P24,P25;
    	SpringLayout.Constraints P31;
    	
        Spring sp1=Spring.constant(5);
        Spring sp2=Spring.constant(20);
        Spring sp3=Spring.constant(12);
   	
    	P01 = advLayout.getConstraints(selLevelMode);
        P02 = advLayout.getConstraints(selLevelType);
        P03 = advLayout.getConstraints(lblLevel);
        P04 = advLayout.getConstraints(sldrLevel);
        
        P11 = advLayout.getConstraints(lblClear);
        P12 = advLayout.getConstraints(cmdClearHist);
        P13 = advLayout.getConstraints(cmdClearMsgs);     
        P14 = advLayout.getConstraints(cmdClearLog);
        
        P15 = advLayout.getConstraints(chkRestartServer);       	
        
        //  place items, X
        
        P01.setX(Spring.constant(10));
        P02.setX(Spring.sum(P01.getConstraint(SpringLayout.EAST),sp1));
        P03.setX(Spring.sum(P02.getConstraint(SpringLayout.EAST),sp1));
        P04.setX(Spring.sum(P03.getConstraint(SpringLayout.EAST),sp1));

        P11.setX(Spring.constant(10));
        P12.setX(Spring.sum(P11.getConstraint(SpringLayout.EAST),sp1));
        
        P13.setX(Spring.sum(P12.getConstraint(SpringLayout.EAST),sp1));
        P14.setX(Spring.sum(P13.getConstraint(SpringLayout.EAST),sp1));
        P15.setX(Spring.sum(P14.getConstraint(SpringLayout.EAST),sp2));
 
        P01.setY(sp1);
        P02.setY(sp1);
        P03.setY(sp3);
        P04.setY(sp1);
       
        P11.setY(Spring.sum(P01.getConstraint(SpringLayout.SOUTH),Spring.constant(34)));               
        P12.setY(Spring.sum(P01.getConstraint(SpringLayout.SOUTH),Spring.constant(30)));
        P13.setY(P12.getY());
        P14.setY(P12.getY());
        P15.setY(P12.getY());
         
        // show lines 2 & 3 only if connected
        
        P21 = advLayout.getConstraints(lblCmdLine);
        P22 = advLayout.getConstraints(txtsRabbit);
        P23 = advLayout.getConstraints(cmdSendRabbit);
        P24 = advLayout.getConstraints(cmdSaveLog);
        P25 = advLayout.getConstraints(cmdRebuildHist);
        
        P31 = advLayout.getConstraints(msgScrollPane);
   
    	P21.setX(Spring.constant(10));
        P22.setX(Spring.sum(P21.getConstraint(SpringLayout.EAST),sp1));
        P23.setX(Spring.sum(P22.getConstraint(SpringLayout.EAST),sp1));
        P24.setX(Spring.sum(P23.getConstraint(SpringLayout.EAST),sp1));
        P25.setX(Spring.sum(P24.getConstraint(SpringLayout.EAST),sp1));
    
    	P21.setY(Spring.sum(P11.getConstraint(SpringLayout.SOUTH),Spring.constant(30)));
        P22.setY(P21.getY());
        P23.setY(P21.getY());
        P24.setY(P21.getY());
        P25.setY(P21.getY());
          
        P31.setY(Spring.sum(P21.getConstraint(SpringLayout.SOUTH),sp2)); 
        
    }
    
    // implementation of ActionListener
    public void actionPerformed(ActionEvent ae) {
        String cmd = ae.getActionCommand();
        
        if (cmd.equals(CMD_RABBIT)) 
            sendRabbit();
        else if (cmd.equals(CMD_HEHIST))
        	openHePlot();
        else if (cmd.equals(CMD_N2HIST))
        	openN2Plot();
        else if (cmd.equals(CMD_N2READ))
        	getData();
        else if (cmd.equals(CMD_HEREAD))
        	readHelium();
        else if (cmd.equals(SEL_HE))
        	setMeasTxt();
        else if (cmd.equals(SEL_N2))
        	setMeasTxt();
        else if (cmd.equals(SEL_LVL))
        	setLevelValue();
        else if (cmd.equals(ABOUT_CONT))
        	getPanelVersion();
        else if (cmd.equals(ADVANCED))
        	setAdvanced();
        else if (cmd.equals(READ_LOG))
        	readLog();
         else if (cmd.equals(LOG_TO_HIST))
        	LogToHist();
       else if (cmd.equals(SAVE_HIST))
        	saveHistory();
        else if (cmd.equals(CLEAR_HIST))
        	clearHistory();
        else if (cmd.equals(CLEAR_MSGS))
        	clearMessages();
        else if (cmd.equals(CLEAR_LOG))
        	clearLog();
        else if (cmd.equals(RESTART)){
        	restart=chkRestartServer.isSelected();
        }
        else if (cmd.equals(CHECK_ERRORS)){
        	error_request=true;
        	getErrors();
        }
    	else if (cmd.equals(CLOSE_COMMAND)) {
        	if(connected)
        		DataFileManager.writeMsgLog("Window closed by user", debug);
        	window_closed=true;
            this.dispose();
            System.exit(0);
        }       
    }    
 
    private void setLevelValue(){
    	int mode=selLevelMode.getSelectedIndex();
    	int type=selLevelType.getSelectedIndex();
    	double value=DataFileManager.levelValue[mode][type];
     	sldrLevel.setValue((int)value);
    }
    
    private void setMeasTxt(){
    	switch(selN2Meas.getSelectedIndex()){
    	case FIRST: 
    		txtN2Meas.setText(firstRead[NITROGEN]);
    		break;
    	case LAST: 
    		txtN2Meas.setText(lastRead[NITROGEN]);
    		break;
    	case FILL: 
    		txtN2Meas.setText(fillDate[NITROGEN]);
    		break;
    	}
    	switch(selHeMeas.getSelectedIndex()){
    	case FIRST: 
    		txtHeMeas.setText(firstRead[HELIUM]);
    		break;
    	case LAST: 
    		txtHeMeas.setText(lastRead[HELIUM]);
    		break;
    	case FILL: 
    		txtHeMeas.setText(fillDate[HELIUM]);
    		break;
    	}
    }
    private void setAdvanced(){
    	boolean state;
    	state= advanced.getState();
    	advPanel.setVisible(state);
    	setLevelValue();
    	if(state){
            fullPanel.setPreferredSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
            int height=connected?510:270;
            lblCmdLine.setVisible(connected);
	        cmdSendRabbit.setVisible(connected);
	        txtsRabbit.setVisible(connected);
	        cmdSaveLog.setVisible(connected);
	        textArea.setVisible(connected);
	        msgScrollPane.setVisible(connected);
	        
	        chkRestartServer.setVisible(true);
            setupPane.setPreferredSize(new Dimension(PANEL_WIDTH, height));
            
            this.setSize(new Dimension(PANEL_WIDTH, height));
            this.pack();
            this.setVisible(true);
    	} else {
            fullPanel.setPreferredSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
            setupPane.setPreferredSize(new Dimension(PANEL_WIDTH, PANEL_HEIGHT));
            this.setSize(new Dimension(PANEL_WIDTH, 550));	
            this.pack();
            this.setVisible(true);
    	}
    }
    
    /**
     * Send current time and date to cryo monitor
     * called on initial connection to synchronize clocks
     */
    private void setTime(){
    	Calendar rightNow = Calendar.getInstance();
    	rightNow = Calendar.getInstance();
		Date today=rightNow.getTime();
		DataFileManager.writeMsgLog("SyncTime", debug);
		Format f = new SimpleDateFormat("yy");
	    String s = "Y"+f.format(today);
	    sendCommand(CRYO_PORT, 1000, s+"\n", false);
	    f = new SimpleDateFormat("MM");
	    s = "M"+f.format(today);
	    sendCommand(CRYO_PORT, 1000, s+"\n", false);
	    f = new SimpleDateFormat("dd");
	    s = "D"+f.format(today);
	    sendCommand(CRYO_PORT, 1000, s+"\n", false);
	    f = new SimpleDateFormat("HH");
	    s = "h"+f.format(today);
	    sendCommand(CRYO_PORT, 1000, s+"\n", false);
	    f = new SimpleDateFormat("mm");
	    s = "m"+f.format(today);
	    sendCommand(CRYO_PORT, 1000, s+"\n", false);
    }
    private void getTime(){
    	sendCommand(CRYO_PORT, 3000, CMD_CLOCK, false);
    	writeAdvanced("Sent: " + CMD_CLOCK);

    }
    
    private void getParams(){
    	sendCommand(CRYO_PORT, 3000, CMD_PARAMS, false);
    }
    
    private void getErrors(){
    	sendCommand(CRYO_PORT, 3000, CMD_ERRORS, false);
    }

    private void readHelium(){
    	if(connected){
	    	sendCommand(CRYO_PORT, 3000, CMD_MEAS_HE, false);
	    	getData();
    	}
    	else
    		updateUI();
    }

    private void getVersion(){
    	sendCommand(CRYO_PORT, 3000, CMD_VERSION, false);
    }

    public void cryomonNotCommunicating(){
		//String msg="Cryogen Monitor Not Communicating";
		String tstring=DataFileManager.getDateString();
    	restart=false;
    	if(rabbit23 !=null && rabbit23.isConnected()){
        	DataFileManager.writeMsgLog("Cryogen Monitor Not Communicating", debug);   	

    		rabbit23.disconnect();
    		rabbit23.setCryoPanel(null);
            DataFileManager.removeLock();

    	}
		JOptionPane.showMessageDialog(this,
				tstring+" : "+"Please Contact Service",
				"Cryogen Monitor Not Communicating",
				JOptionPane.ERROR_MESSAGE);
        this.dispose();
        System.exit(0);
    }

    public boolean getReadLogFlag(){
    	return f_readLog;
    }
    
    public void setReadLogFlag(boolean readLog){
    	f_readLog= readLog;
    }
    public boolean getLog2HistFlag(){
    	return f_log2hist;
    }
    
    public void setLog2HistFlag(boolean readLog){
    	f_log2hist= readLog;
    }
    
    public String getLogFile(){
    	if(m_logFile!=null){
    		return m_logFile;
    	} else {
    		return DataFileManager.logFile;
    	}
    }
    /** Listen to the slider. */
    public void stateChanged(ChangeEvent e) {
        JSlider source = (JSlider)e.getSource();
        if (!source.getValueIsAdjusting()) {
            int value = (int)source.getValue();
        	int mode=selLevelMode.getSelectedIndex();
        	int type=selLevelType.getSelectedIndex();
        	DataFileManager.levelValue[mode][type]=value;
        	if(hePlotFrame==null )
        		return;
        	plotFillData();
        	//updateUI();
        	DataFileManager.writePrefs();
         }
    }
    private void clearLog(){
    	int n = JOptionPane.showConfirmDialog(this,
    			"Delete CryoMonitor Flash log ?",
    			"Verify Delete",
    			JOptionPane.YES_NO_OPTION);
    	if(n==0){
 	    	sendCommand(CRYO_PORT, 3000, CMD_CLEAR_LOG, false);  // clear log
	    	sendCommand(CRYO_PORT, 3000, CMD_MEAS_HE, false); // take a reading
 	    	//waitTime(10000);
	    	//getData();
    	}
    }
    private void LogToHist(){
    	setLog2HistFlag(true);
    	final JFrame readLog= new JFrame();
    	readLog.setTitle("Generate History from Log File");

    	JFileChooser fc= new JFileChooser();
    	fc.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
    	int returnVal= fc.showDialog(readLog, "Read");
    	
    	if(returnVal!=JFileChooser.APPROVE_OPTION)
    		return;
    	File file= fc.getSelectedFile();
    	
    	try {
    		ArrayList<DataObject> array=DataFileManager.readLogData(file.getCanonicalPath());
    		DataFileManager.writeFillData(array);
    		plotFillData();

		} catch (IOException e) {
			// TODO Auto-generated catch block
			//e.printStackTrace();
		}
    }
   
    private void readLog(){
    	final JFrame saveLog= new JFrame();
    	saveLog.setTitle("Save Log to File");

    	JFileChooser fc= new JFileChooser();
    	fc.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
    	int returnVal= fc.showDialog(saveLog, "Save");
    	
    	if(returnVal!=JFileChooser.APPROVE_OPTION)
    		return;
		File file= fc.getSelectedFile();
		setLogFile(file.getPath());
		setReadLogFlag(true);
		sendCommand(CRYO_PORT, 3000, CMD_READ_LOG, false);
    }

    private void clearHistory(){
    	int n = JOptionPane.showConfirmDialog(this,
    			"Delete Fill History File?",
    			"Verify Delete",
    			JOptionPane.YES_NO_OPTION);
    	if(n==0){
    		DataFileManager.clearFillData();
    		getData();
    	}
    }

    private void clearMessages(){
    	int n = JOptionPane.showConfirmDialog(this,
    			"Delete Message File?",
    			"Verify Delete",
    			JOptionPane.YES_NO_OPTION);
    	if(n==0){
    		DataFileManager.clearMsgs();
    	}
    }

    private void saveHistory(){
    	JFrame saveLog= new JFrame();
    	saveLog.setTitle("Save History to File");

    	JFileChooser fc= new JFileChooser();
    	fc.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
    	int returnVal= fc.showDialog(saveLog, "Save");
    	
    	if(returnVal!=JFileChooser.APPROVE_OPTION)
    		return;
		File file= fc.getSelectedFile();
		String path=file.getPath();
		if(!path.equals(DataFileManager.dataFile)){
	    	ArrayList<DataObject> array=DataFileManager.readFillData();
	    	DataFileManager.writeFillData(path,array);
		}
		else
			popupMsg("Cryogen History Save Error", "File In Use", false);			
    }
   
    private void getPanelVersion(){
    	String version="Software Revision: " + PANEL_VERN;
    	String date=PANEL_DATE;
    	String msg=version+" "+date+"\n";
    	String type=DataFileManager.getMonitorType();
    	if(type !=null)
    		msg+="Monitor Type: "+type;
    	popupMsg("Cryogen Monitor ", msg, false);
    }
    
    private void setLogFile(String path){
    	m_logFile= path;
    }
     
    private void sendRabbit(){
    	if(connected){
	    	writeAdvanced("Sent: " + txtsRabbit.getText()+"\n");
	     	rabbit23.sendToCryoBay(txtsRabbit.getText()+"\n");
    	}
    }

    synchronized private void sendCommand(int portNum, int timeOut, 
    	String command, boolean direct){
    	if(connected && portNum==CRYO_PORT){
    		if (rabbit23.isConnected()){
    			rabbit23.sendToCryoBay(command);
    	    	writeAdvanced("Sent: " + command);
    		}   		
    	} 
    }
    private static void waitTime(long ms){
		try {
			Thread.sleep((long) (ms));
		}
		catch (Exception e){
			System.out.println("could not sleep:"+e);
		}
    }

    public void writeError(String info){
    	textArea.append(info);
    }
    
    public void setTime(String time){
    	//writeAdvanced("Received: " + time + "\n");
    	if(last_date==null){ // on initial connection
            getVersion();
            getErrors();
	    	//getParams();	//send W command
            //getData();  
		}
        last_date=CryoMonitorSocketControl.getDate(time);
    	if(last_date !=null){ // subsequent calls (every 10 minutes)
        	SimpleDateFormat fmt = new SimpleDateFormat("HH");
        	try {
	        	String tstr=fmt.format(last_date);
	        	int hr=Integer.parseInt(tstr);
	        	fmt = new SimpleDateFormat("mm");
	        	tstr=fmt.format(last_date);
	        	int min=Integer.parseInt(tstr);
	        	//System.out.println("hr:"+hr+" min:"+min);
	        	if(first || (hr==12 && min > 30 && min < 45)) // resync time once a day at 1:30
	        		setTime();
	        	first=false;
	        }
        	catch (Exception e){
        		System.out.println("setTime ERROR "+e);
        	}
    	}
    	
    }
    public static boolean isServer(){
     	return server;
    }

    public static boolean cryoActive(){
    	return DataFileManager.isLocked();
    }
    public void writeInfo(String info){
    	if(!nogui)
    		textArea.append(info);
    }

    public void setConnected(boolean status){
 		writeInfo("\nCryogen Monitor connection OK\n");
    }
    public void setErrors(String msg){
		if(!msg.startsWith("EH")){
			msg=msg.replace("EH*","");
			msg=msg.replace("\n","");
        	DataFileManager.writeMsgLog("Cryogen Monitor Errors:"+msg, debug);   	

			if(!nogui)
			popupMsg("Cryogen Monitor Errors", msg, false);
		}
		else if(error_request)
			popupMsg("Cryogen Monitor Errors", "No errors reported", false);
		error_request=false;
    }
	public void setStatus(int s){
		boolean active=cryoActive();
		cmdReadHe.setEnabled(active);
		cmdReadN2.setEnabled(active);     				
		if(isServer()){
 			cmdReadHe.setText("Read");
 			cmdReadN2.setText("Read");
 			if((s & PROBE1_ON)>0 && (s & PROBE1_READ_ERROR)>0){
 	 			cmdReadHe.setText("Error");
 	 			cmdReadHe.setEnabled(false); 				
 			}
 			else if((s & PROBE2_ON)>0 && (s & PROBE2_READ_ERROR)>0){
 	 			cmdReadHe.setText("Error");
 	 			cmdReadHe.setEnabled(false); 				
 			}
 			else if((s & PROBE1_ON)==0 && (s & PROBE2_ON)==0){
 	 			cmdReadHe.setText("Off");
 	 			cmdReadHe.setEnabled(false); 								
 			}
 	 		if((s & N2_ON)>0){
 	    		if((s&N2_HW_ERROR)>0)
 	    			cmdReadN2.setText("Error");
 	  		}
 	 		else{
    			cmdReadN2.setText("Off");
  	 		}
 		}
		else{
 			cmdReadHe.setText("Update");
 			cmdReadN2.setText("Update");     				
		}
    }

    private void setLevel(double level, JProgressBar pb,int type){
		int value = (int)level;
		pb.setValue(value);
		pb.setString(value + "%");
		if(value<DataFileManager.levelValue[type][SEL_ALERT]){ //red
			pb.setForeground(ALERT_COLOR);
		} else if(value<DataFileManager.levelValue[type][SEL_WARN]) { //yellow
			pb.setForeground(WARN_COLOR);
		} else { //green
			pb.setForeground(FILL_COLOR);
		}
    }
   
    private Date getFillDate(DataObject date1, DataObject date2, DataObject last, double lvl1, double lvl2, double lvl3, double max){
       	long time1=date1.getTime();
       	long time2=date2.getTime();
       	long delta=time2-time1;
       	double m=(lvl2-lvl1)/delta;
       	if(m<0){
       		double time=last.getTime();
       		double fill=(max-lvl3)/m+time;
	 		return new Date((long)fill*1000);
       	}
       	return null;
    }
    private void setDates(ArrayList<DataObject> array){
       	int n = array.size();
       	DataObject first=array.get(0);
      	DataObject last=array.get(n-1);
      	SimpleDateFormat fmt=new SimpleDateFormat("MM/dd/yy HH:mm");
      	Date date1=first.getDate();
      	Date date2=last.getDate();
      	firstRead[NITROGEN]=fmt.format(date1);
      	lastRead[NITROGEN]=fmt.format(date2);
      	int i;
       	
     	// get Helium first and last values from status bits in the data
      	
     	firstRead[HELIUM]=fmt.format(date1);
       	lastRead[HELIUM]=fmt.format(date2);
       	
       	setLevel(last.getHeLevel(),pbHe,HELIUM);
    	setLevel(last.getN2Level(),pbN2,NITROGEN);
    	setStatus(last.getStatus());
    	
       	DataObject data;
      	for(i=0;i<n;i++){
      		data=array.get(i);
      		if((data.getStatus()&HE_NEW_READ)>0){
      			firstRead[HELIUM]=fmt.format(data.getDate());
      			break;
      		}    			
      	}
      	for(i=n-1;i>=0;i--){
      		data=array.get(i);
      		if((data.getStatus()&HE_NEW_READ)>0){
      			lastRead[HELIUM]=fmt.format(data.getDate());
      			setLevel(data.getHeLevel(),pbHe,HELIUM);
      			break;
      		}    			
      	}
 
     	// Estimate the date for the next Helium fill

     	double value1,value2;
    	DataObject data1=last;
    	DataObject data2=last;
      	i=n-1;
      	Date fill=null;
      	// find the last span in history where values are decreasing
      	double fill_level=DataFileManager.levelValue[HELIUM][SEL_FILL];
      	while(fill == null && data1 !=first){ 
      		data2=data1;
           	value1=value2=data2.getHeLevel();
           	// find start and end of span
          	while(value1>=value2 && data1 !=first) {
          		i--;
          		data1=array.get(i);
          		value1=data1.getHeLevel();
          	}
          	// calculate the intercept to FILL_LEVEL from value/time slope in span
          	fill=getFillDate(data1,data2,last,value1,value2,last.getHeLevel(),fill_level);
       	}
      	if(fill !=null)
      		fillDate[HELIUM]=fmt.format(fill);     	
      	else
      		fillDate[HELIUM]="??";
      	
     	// Estimate the date for the next Nitrogen fill
      	
      	data1=data2=last;      	
      	i=n-1;
      	fill=null;
      	fill_level=DataFileManager.levelValue[NITROGEN][SEL_FILL];
      	while(fill == null && data1 !=first){
      		data2=data1;
           	value1=value2=data2.getN2Level();
          	while(value1>=value2 && data1 !=first) {
          		i--;
          		data1=array.get(i);
          		value1=data1.getN2Level();
          	}
          	fill=getFillDate(data1,data2,last,value1,value2,last.getN2Level(),fill_level);
       	}
      	if(fill !=null)
      		fillDate[NITROGEN]=fmt.format(fill);      	
      	else
      		fillDate[NITROGEN]="??";
       	setMeasTxt();
    }
 
    /**
     * update user interface
     */
    public void updateUI(){
    	if(connected)
    		DataFileManager.writeMsgLog("Updated history file", debug);   	
    	plotFillData();
    }
    /**
     * plot Helium and Nitrogen Fill history
     * @param filename
     */
    private synchronized void plotFillData(){
    	if(nogui)
    		return;
    	ArrayList<DataObject> array=DataFileManager.readFillData();

    	int n = array.size();
    	if(n==0)
    		return;
     	DataObject first=array.get(0);
    	long time1=first.getTime()*1000;
    	
     	DataObject last=array.get(n-1);
     	setDates(array);

   	    long time2=last.getTime()*1000;
   	    
   	    if(time1==time2){
   	    	time1-=500;
   	    	time2+=500;
   	    }

     	hePlot.clear(false);
     	n2Plot.clear(false);
 		hePlot.setXFillRange(time1, time2);		
		n2Plot.setXFillRange(time1, time2);
		   	
    	for(int i=0;i<n;i++){
    		DataObject data=array.get(i);
    		long tm=data.getTime()*1000; // get time in seconds
     		double newhlvl=data.getHeLevel();    		
    		hePlot.addPoint(0,tm,newhlvl,true,true);
    		int info=data.getStatus();
     		if((info & HE_NEW_READ)>0 || n==1)
    			hePlot.addPoint(1,tm,newhlvl,true,false);
     		n2Plot.addPoint(0,tm,data.getN2Level(),true,true);
    	}
    	double[] xpts = {time1,time2};
    	double[] HeFill = {DataFileManager.levelValue[HELIUM][SEL_FILL],DataFileManager.levelValue[HELIUM][SEL_FILL]};
    	double[] N2Fill = {DataFileManager.levelValue[NITROGEN][SEL_FILL],DataFileManager.levelValue[NITROGEN][SEL_FILL]};
    	hePlot.setPoints(2, xpts, HeFill, true);
    	n2Plot.setPoints(2, xpts, N2Fill, true);
    	double[] HeWarn = {DataFileManager.levelValue[HELIUM][SEL_WARN],DataFileManager.levelValue[HELIUM][SEL_WARN]};
    	double[] N2Warn = {DataFileManager.levelValue[NITROGEN][SEL_WARN],DataFileManager.levelValue[NITROGEN][SEL_WARN]};
    	hePlot.setPoints(3, xpts, HeWarn, true);
    	n2Plot.setPoints(3, xpts, N2Warn, true);
    	double[] HeAlert = {DataFileManager.levelValue[HELIUM][SEL_ALERT],DataFileManager.levelValue[HELIUM][SEL_ALERT]};
    	double[] N2Alert = {DataFileManager.levelValue[NITROGEN][SEL_ALERT],DataFileManager.levelValue[NITROGEN][SEL_ALERT]};
    	hePlot.setPoints(4, xpts, HeAlert, true);
    	n2Plot.setPoints(4, xpts, N2Alert, true);
    }

    /**
     * pop up a message dialog
     * @param title
     * @param msg
     * @param needInfo
     */
    public void popupMsg(String title, String msg, boolean needInfo) {
        final JFrame warn;
        JButton ok;
        JTextArea message;

        warn = new JFrame();
        warn.setTitle(title);

        message = new JTextArea(msg);
        message.setEditable(false);
        ok = new JButton("  Ok  ");
        
        ok.setActionCommand("Ok");
        ok.addActionListener( new ActionListener() {	
            public void actionPerformed(ActionEvent ae) {
                 warn.dispose();
            }
        } );
        
        warn.getContentPane().setLayout( new BorderLayout() );
        warn.getContentPane().add(message, "North");
        warn.getContentPane().add(ok, "South");
        	
        warn.setSize(300, 100);
        Dimension screenSize =
            Toolkit.getDefaultToolkit().getScreenSize();

        warn.setLocation(screenSize.width/4 - 150,
                screenSize.height/4 - 75);
        
        warn.setResizable(true);
        warn.setVisible(true);
    }
    
    public void getData(){
    	if(connected){
	    	getTime();		//send t command
	    	getParams();	//send W command
    	}
    	else
    		updateUI();
    }
    
    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     * note: Cannot use this function for application cleanup if running with -nogui option
     *       because it is only called if the window is actually shown (also, isn't called
     *       as a result of a SIGTERM event) 
     */
    public void windowClosing(WindowEvent ev) {
    	if(connected)
    		DataFileManager.writeMsgLog("Window closed by user", debug);
    	window_closed=true;
        this.dispose();
        System.exit(0);
    }

    public void windowClosed(WindowEvent e) {
    	//System.out.println("WindowClosed");
    }  
    public void windowOpened(WindowEvent e) {}
    public void windowIconified(WindowEvent e) {}
    public void windowDeiconified(WindowEvent e) {}
    public void windowActivated(WindowEvent e) {
    }
    public void windowDeactivated(WindowEvent e) {}
    public void windowGainedFocus(WindowEvent e) {}
    public void windowLostFocus(WindowEvent e) {}
     
    public  class MyPlot extends Plot {
        protected String _formatTime(double dtime, double step_ms) {
            if (_dateFormat == null) {
                _dateFormat = new SimpleDateFormat();
            }
            if (step_ms < 4 * 60 * 60 * 1000) { // Step < 4 hr
                _dateFormat.applyPattern("MM/dd H:mm");
            } else if (step_ms < 24 * 60 * 60 * 1000) { // Step < 1 day
                 _dateFormat.applyPattern("MM/dd H:mm");
            } else {                // Step >= 1 day
                _dateFormat.applyPattern("MM/dd/yy");
            }
            String str = _dateFormat.format(new Date((long)dtime));
            return str;
        }
        public synchronized void setFillButton(boolean visible) {
        	super.setFillButton(visible);
        	if(_fillButton!=null)
        		_fillButton.setText("  Fit  ");
        }

    }
    /**
     * CryoMonitorControls shutdown hook class
     * Evoked when:
     *  1. CryoMonitorControls application terminates normally (GUI or no GUI)
     *  2. System.exit is called from within the VM 
     *  3. The VM is started from a command shell and the shell terminates (due to ctrl-c etc.)
     * Cleanup tasks (if running as master):
     *  1. Disconnect socket to Cryogen monitor
     *  2. Start up background server if applicable
     *  3. remove cryomon.lock file
     */
	public static class CryoShutdown extends Thread {
		public void run() {
			if (connected) {
		    	DataFileManager.writeMsgLog("CryoShutdown", debug);   	
				rabbit23.disconnect();
				rabbit23.setCryoPanel(null);
				connected=false;
				DataFileManager.removeLock();
				if (!nogui && restart && window_closed) { // if a background process don't restart on kill
					ProcessBuilder pb = new ProcessBuilder("cryomon",
							"-master", "-nogui");
					try {
						Process p=pb.start();
						p.waitFor();
					} catch (Exception e) {
						System.err
								.println("Error: could not start background server");
					}
				}
			} else {
				if (m_statusPoller != null) {
					m_statusPoller.quit();
					m_statusPoller.interrupt();
				}
			}
		}
	}
    
    public void pollStatus() {
        m_statusPoller = new StatusPoller(STATUS_UPDATE);
        m_statusPoller.start();
    }

    /**
     * This thread just sends status requests every so often.
     */
    class StatusPoller extends Thread {
        private boolean m_quit = false;
        private int m_rate_ms;
 
        public StatusPoller(int period_s) {
            setName("CryoMonitorControls.StatusPoller");
            m_rate_ms = period_s*1000;
        }

        public void quit() {
        	//System.out.println("StatusPoller.quit()");
            m_quit = true;
         }

		public synchronized void run() {
			while (!m_quit) {
				try {
					if (initialDelay > 0){
						Thread.sleep(initialDelay);
						initialDelay=0;
					}
					else {
						if (dataCount == 0) {
							updateUI();
						}
						dataCount++;
						if (dataCount == dataUpdate)
							dataCount = 0;
						Thread.sleep(m_rate_ms);
					}
				} catch (InterruptedException ie) {
				}
			}
		}
    }

    /**
     * Create the GUI and show it.  For thread safety,
     * this method should be invoked from the
     * event-dispatching thread.
     */
    private static void createAndShowGUI() {
        //Make sure we have nice window decorations.
        JFrame.setDefaultLookAndFeelDecorated(true);

        CryoMonitorControls newDisp = new CryoMonitorControls();
        CryoShutdown sh = new CryoShutdown();
        Runtime.getRuntime().addShutdownHook(sh);
    }

    public static void main(String[] args) {
        //Schedule a job for the event-dispatching thread:
        //creating and showing this application's GUI.
        int len = args.length;
        nogui=false;
        
        boolean master=false;
        boolean vnmrj=false;
      
        String sysdir=null;
        
        
        String look = System.getProperty("laf");
        if (look == null || look.length() < 1)
            look = "metal";

        for (int i = 0; i < len; i++) {
            if (args[i].equalsIgnoreCase("-sysdir") && i + 1 < len) {
            	sysdir= args[++i];
            } else if (args[i].equalsIgnoreCase("-vnmrj")) {
            	vnmrj=true;
            } else if (args[i].equalsIgnoreCase("-noGUI")) {
            	nogui=true;
            	master=true; // no point in running a reader in the background
            } else if (args[i].equalsIgnoreCase("-debug")) {
            	debug=true;
            	CryoMonitorSocket.debug=true;
            	CryoMonitorSocketControl.debug=true;
            }
            else if (args[i].equalsIgnoreCase("-master")) {
            	master=true;
            }
        }
        
        if(!nogui){
			try {
				String lf=UIManager.getSystemLookAndFeelClassName();
		        if(vnmrj){
		        	lf="metal";
		        	String OCEAN_THEME_CLASS = "javax.swing.plaf.metal.OceanTheme";
		        	MetalTheme theme= (MetalTheme)Class.forName(OCEAN_THEME_CLASS).newInstance();
		        	MetalLookAndFeel.setCurrentTheme(theme);
		        }
		        UIManager.put("Slider.paintValue", false);		        
		        UIManager.setLookAndFeel(lf);
				}catch(Exception e){ 
		
				}
        }
		if (sysdir == null) {
			String ostype=System.getProperty("os.name");
			if(ostype.equalsIgnoreCase("Linux")){
				DataDir = "/vnmr/cryo/cryomon";
			}
			else{
				sysdir = "c:" + File.separator + "cryomon";
				DataDir = sysdir + File.separator + "Data";
				iconFile = sysdir + File.separator + "VarianIcon.gif";
			}
		} else {
			DataDir=sysdir + "/cryo/cryomon";
		}
		
		File dir=new File(DataDir);
		boolean canwrite=dir.canWrite();
        if(canwrite && master){
        	server=true;
        }
        else{
        	server=false;
        	if(nogui)
        	System.out.println("Could not start Cryomon server: write access denied");
        }
        
        javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                 createAndShowGUI();
            }
        });
    } 
}





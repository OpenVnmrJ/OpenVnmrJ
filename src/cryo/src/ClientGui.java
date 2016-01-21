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
 *
 */
/*
 *
 * Varian, Inc. and its contributors.  Use, disclosure and
 * reproduction is prohibited without prior consent.
 */

import java.awt.event.*;
import javax.swing.*;
import javax.swing.border.Border;
import java.awt.*;
import java.io.*;
import java.text.*;
import cryoaccess.*;
import org.omg.CORBA.*;

/**
 * A class to interface to PC clients over CORBA.
 */
public class ClientGui  {

    public static void main(String[] args) {
        ORB orb = null;
        orb = ORB.init(args, null);
        if (orb != null) {
            try {
                new ClientGui(orb);
            } catch (Exception e) {
                System.err.println(e);
                System.exit(-1);
            }
        } else {
            System.err.println("can't initiate orb");
            System.exit(-1);
        }
    }     /*end of main*/


    private ORB orb;
    private org.omg.CORBA.Object obj;
    private CryoBay cryoBay;

    public ClientGui(ORB o) throws Exception {
        ShutdownFrame sf;
        BufferedReader reader;
        boolean modulePresent;
        File file;
        CryoThread update;

        orb = o;

        obj = null;
        cryoBay = null;

        //System.out.println("running test client.");

        // instantiate ModuleAccessor
        file = new File("/vnmr/acqqueue/cryoBay.CORBAref");
        if ( file.exists() ) {
            reader = new BufferedReader( new FileReader(file) );
            obj = orb.string_to_object( reader.readLine() );
        }

        if (obj == null) {
            throw new Exception("string_to_object is null: cryoBay.CORBAref");
        }

        //System.out.println("Got object.");

        cryoBay = CryoBayHelper.narrow(obj);

        if (cryoBay == null) {
            throw new Exception("cryoBay is null");
        }

        if ( cryoBay._non_existent() ) {
            throw new Exception("cryoBay is not running");
        }

        sf = new ShutdownFrame(cryoBay);
        update = new CryoThread(cryoBay, sf, this);
        sf.show();
	update.start();

    }       /*end of constructor*/

    public ORB getOrb(){
        return orb;
    }

}          /*end of TestClientGui Class*/


    class ShutdownFrame extends JFrame implements WindowListener, ActionListener {

        JButton cmdClose;
        JLabel lblBStatus;
        JLabel lblBHeater;
        JLabel lblBTemp;
        JLabel lblBCli;
        JTextField lblStatus;
        JTextField lblHeater;
        JTextField lblTemp;
        JTextField lblCli;


	public CryoBay cryoB;


        ShutdownFrame(CryoBay cb) {
	    super("CryoBay Monitor");
	    cryoB= cb;

              cmdClose = new JButton("Close"){
		    public JToolTip createToolTip(){
			return new JToolTip();
		    }
		};

            cmdClose.setToolTipText("Close program");


            cmdClose.addActionListener(this);
            addWindowListener(this);

            GridBagConstraints gbc = new GridBagConstraints();
            Border loweredbevel = BorderFactory.createLoweredBevelBorder();

            lblBStatus= new JLabel("Status: ");
            lblBHeater= new JLabel("Heater: ");
            lblBTemp=   new JLabel("  Temp: ");
            lblBCli=    new JLabel("   CLI: ");

            lblStatus = new JTextField(17);
            lblStatus.setEditable(false);
	    lblStatus.setOpaque(true);
            lblStatus.setBorder(loweredbevel);

            lblHeater = new JTextField(17);
            lblHeater.setEditable(false);
	    lblHeater.setOpaque(true);
            lblHeater.setBorder(loweredbevel);

	    lblTemp = new JTextField(17);
            lblTemp.setEditable(false);
	    lblTemp.setOpaque(true);
            lblTemp.setBorder(loweredbevel);

            lblCli = new JTextField(17);
            lblCli.setEditable(false);
	    lblCli.setOpaque(true);
            lblCli.setBorder(loweredbevel);

            JPanel lblPanel = new JPanel();
            lblPanel.setLayout(new GridBagLayout());
            gbc.insets = new Insets(2, 5, 2, 5);
            setGbc(gbc, 0, 0, 1, 1);
            lblPanel.add(lblBStatus, gbc);
            setGbc(gbc, 0, 1, 1, 1);
            lblPanel.add(lblBHeater, gbc);
            setGbc(gbc, 0, 2, 1, 1);
            lblPanel.add(lblBTemp, gbc);
            setGbc(gbc, 0, 3, 1, 1);
            lblPanel.add(lblBCli, gbc);

            JPanel valPanel = new JPanel();
            valPanel.setLayout(new GridBagLayout());
            gbc.insets = new Insets(2, 5, 2, 5);
            setGbc(gbc, 0, 0, 1, 1);
            valPanel.add(lblStatus, gbc);
            setGbc(gbc, 0, 1, 1, 1);
            valPanel.add(lblHeater, gbc);
            setGbc(gbc, 0, 2, 1, 1);
            valPanel.add(lblTemp, gbc);
            setGbc(gbc, 0, 3, 1, 1);
            valPanel.add(lblCli, gbc);

            JPanel buttonPanel = new JPanel();
            buttonPanel.setLayout(new GridBagLayout());
            gbc.anchor= GridBagConstraints.CENTER;
            setGbc(gbc, 0, 5, 1, 1);
            buttonPanel.add(cmdClose, gbc);

             // finally, add the panels to the content pane
            getContentPane().setLayout(new GridBagLayout());
            gbc.insets = new Insets(10, 10, 10 , 10);
            gbc.anchor = GridBagConstraints.CENTER;
            setGbc(gbc, 0, 0, 1, 4);
            getContentPane().add(lblPanel, gbc);
            setGbc(gbc, 1, 0, 1, 4);
            getContentPane().add(valPanel, gbc);
            setGbc(gbc, 0, 5, 0, 0);
            getContentPane().add(buttonPanel, gbc);

            setSize(300, 220);
            Dimension screenSize =
                Toolkit.getDefaultToolkit().getScreenSize();
            setLocation(screenSize.width/2 - 300,
                        screenSize.height/2 - 220);
            setResizable(true);




        }

        private void setGbc(GridBagConstraints gbc,
                        int x, int y, int w, int h) {
            gbc.gridx = x;
            gbc.gridy = y;
            gbc.gridwidth = w;
            gbc.gridheight = h;

        }

        public void actionPerformed(ActionEvent ae) {
            String cmd = ae.getActionCommand();

            if (cmd.equals("Close") ){
                cmdDisconnect();
            }
        }

        public void windowClosing(WindowEvent e) {
            cmdDisconnect();
        }

        // following not used but need to be "implemented"
        public void windowActivated(WindowEvent e) {}
        public void windowClosed(WindowEvent e) {}
        public void windowDeactivated(WindowEvent e) {}
        public void windowDeiconified(WindowEvent e) {}
        public void windowIconified(WindowEvent e) {}
        public void windowOpened(WindowEvent e) {}


    private void cmdDisconnect(){
        //close main window
        this.dispose();
        System.exit(0);
    }

    public CryoBay reconnectServer(ORB o, ReconnectThread rct){

        BufferedReader reader;
        File file;
        ORB orb;
        org.omg.CORBA.Object obj;

        orb = o;

        obj = null;
        cryoB = null;


        try{
            // instantiate ModuleAccessor
            file = new File("/vnmr/acqqueue/cryoBay.CORBAref");
            if ( file.exists() ) {
                reader = new BufferedReader( new FileReader(file) );
                obj = orb.string_to_object( reader.readLine() );
            }

            if(obj!=null){
                cryoB = CryoBayHelper.narrow(obj);
            }

            if (cryoB != null) {
                if ( !(cryoB._non_existent()) ) {
                    //System.out.println("reconnected!!!!");
                    rct.reconnected=true;
                }
            }
        }catch(Exception e){
            //System.out.println("Got error: " + e);
        }
        return cryoB;




    }


    private void warning(String msg) {
        final JDialog warn;
        JButton ok;
        JLabel message;

        warn = new JDialog();
        warn.setTitle(msg);

        message = new JLabel("CryoBay: " + msg);
        ok = new JButton("Ok");

        ok.addActionListener( new ActionListener() {
            public void actionPerformed(ActionEvent ae) {
                warn.dispose();
            }
        } );

        warn.getContentPane().setLayout( new BorderLayout() );
        warn.getContentPane().add(message, "North");
        warn.getContentPane().add(ok, "South");

        warn.setSize(200, 80);
        Dimension screenSize =
            Toolkit.getDefaultToolkit().getScreenSize();
        warn.setLocation(screenSize.width/2 - 100,
                         screenSize.height/2 - 40);
        warn.setResizable(false);
        warn.show();
    }


    }        /* end of shutdown frame class */

class CryoThread extends Thread{

    CryoBay cryo;
    ShutdownFrame frame;
    ClientGui clientGui;
    ORB orb;
    ReconnectThread reconnectThread;

    String[] statusString= {"SYSTEM OFF", "COOLING", "READY", "WARMING", "FAULT", "FAULT- He Pressure",
                            "FAULT- T drift", "RESTARTING", "STOPPING", "EXITING", "Maintenance Needed",
                            "FAULT- Vacuum", "FAULT- Low cooling power", "FAULT- Compressor"};
    int status;
    double heater;
    double cli;
    double temp;
    boolean stop= false;

    public CryoThread(CryoBay cb, ShutdownFrame sf, ClientGui cg){
	cryo= cb;
	frame = sf;
        clientGui=cg;

    }

    public void updateStates(){
        NumberFormat nf= NumberFormat.getInstance();
        nf.setMaximumFractionDigits(2);
        nf.setMinimumFractionDigits(2);

        try{
            //System.out.println("getting variables.");
            status= cryo.cryoGetStatusCORBA();
            frame.lblStatus.setText(statusString[status] + " ");
            heater= cryo.cryoGetHeaterCORBA();
            frame.lblHeater.setText(nf.format(heater) + " watts");
            temp= cryo.cryoGetTempCORBA();
            frame.lblTemp.setText(nf.format(temp) + " deg");
            cli= cryo.cryoGetCliCORBA();
            if(Double.isNaN(cli)){
                frame.lblCli.setText(cli + " ");
            } else frame.lblCli.setText(nf.format(cli) + " ");

        } catch (org.omg.CORBA.COMM_FAILURE cf){
            // stop thread and try to reconnect to the server
            frame.lblStatus.setText("FAILURE!! Server connected?");
            stop=true;
            return;
        }

    }

    public void run(){

        //System.out.println("running cryothread");

	while(!stop){
	    updateStates();
	    try{
		sleep(1000);
	    } catch(Exception e){
		System.out.println("cannot sleep!");
	    }
	}
        //System.out.println("reconnect thread starting...");
        reconnectThread= new ReconnectThread(cryo, frame, clientGui);
        reconnectThread.start();

    }

}


class ReconnectThread extends Thread{

    CryoBay cryo;
    ShutdownFrame frame;
    ClientGui clientGui;
    CryoThread cryoThread;
    ORB orb;
    boolean reconnected= false;

    public ReconnectThread(CryoBay cb, ShutdownFrame sf, ClientGui cg){
	cryo= cb;
	frame = sf;
        clientGui=cg;
        orb= clientGui.getOrb();

    }

    public void run(){

	while(!reconnected){
            cryo= frame.reconnectServer(orb, this);
            try{
		sleep(1000);
	    } catch(Exception e){
		System.out.println("cannot sleep!");
	    }
	}
        //System.out.println("starting cryothread");
        cryoThread= new CryoThread(cryo, frame, clientGui);
        cryoThread.start();


    }

}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.util.*;
import java.io.*;

import org.omg.CORBA.ORB;
import org.omg.PortableServer.*;
import pdaaccess.*;
import vnmr.ui.*;
import vnmr.util.*;


/**
 * This class handles communication with the Bear Mass Spectrometer
 */
public class PdaCorbaClient extends CorbaClient implements Callback{

    public final static String PDA_REF_NAME = "pda330";

    private Pda330 m_accessor = null;
    private String m_pdaDataFile = null;
    private int m_numScans = 0;
    private double m_minRetentionTime = 0;
    private double m_maxRetentionTime = 0;
    static protected LcControl m_control;
    private POA rootPOA;
    private PmlListenerImpl pmlListenerImpl;
    private PmlListener pmlListener;
    private ORB orb;
    private VPdaPlot m_vpdaPlot;
    private ButtonIF m_vif;
    private LcRun m_lcRun;
    private LcDataListener m_pdaDataListener;
    private boolean m_gotDataError = false;
    private StatusPdaThread pdaStatThread;

    int[] daconstSlitWidth = {0, 1, 2, 3, 4};
    int[] daconstBandwidth = {0, 1, 2, 3, 4};
    boolean firsttime= true;
    PrintWriter out;
    long sunTime;
    long deltaTime;

        
    /**
     * Construct a "dead" version, just so that we can call methods
     * that could be static, if you could specify static methods
     * in an interface.
     */
    public PdaCorbaClient() {
    }

    /**
     * Calls the base class constructor and then connects to the
     * CORBA server.
     */
    public PdaCorbaClient(LcControl control, ButtonIF vif, VPdaPlot vpdaPlot)
        throws Exception {

        super(vif);
        initAccessor();
        orb= sendOrb();
        m_vif= vif;

        rootPOA = POAHelper.narrow(orb.resolve_initial_references("RootPOA"));
        pmlListenerImpl = new PmlListenerImpl(this);
        rootPOA.activate_object(pmlListenerImpl);
        pmlListener = pmlListenerImpl._this(orb);

        rootPOA.the_POAManager().activate();
        m_accessor.registerPmlListener(pmlListener);

        m_control = control;

        m_vpdaPlot = vpdaPlot;

        m_lcRun = m_control.getLcRun();

        pdaStatThread = new StatusPdaThread(1000, "uv");
        pdaStatThread.start();
    }

    public UvDetector getDetector(String name) {
        return new Detector(m_expPanel, name);
    }

    /**
     * Gets a "dead" detector, just so that we can call methods
     * that could be static, if you could specify static methods
     * in an abstract class.
     */
    public UvDetector getDetector() {
        return new Detector(m_expPanel);
    }

    /**
     * Set up to get data from a particular file on the PDA.
     */
    /*
    public boolean openPdaFile(String fname) {
        if (!initAccessor()) {
            return false;
        }

        m_pdaDataFile = fname;
        try {
            if (! m_accessor.setFileNameCORBA(fname)) {
                return false;
            }
            m_numScans = m_accessor.getNumberOfScansCORBA();
            m_minRetentionTime = m_accessor.getRetentionTimeCORBA(1);
            m_maxRetentionTime = m_accessor.getRetentionTimeCORBA(m_numScans);
        } catch (Exception e) {
            Messages.postError("CORBA Error: Cannot contact PDA server");
            Messages.writeStackTrace(e);
            m_pdaDataFile = null;
            return false;
        }
        return true;
    }
    */

    private boolean initAccessor() {
        if (m_accessor == null) {
            m_accessor = (Pda330)getAccessor(PDA_REF_NAME);
        }
        return (m_accessor != null);
    }

    /**
     * Get a CORBA accessor given the CORBA Object made from the
     * reference string.
     * @param refName The base name of the reference string.  Not used
     * in this implementation.
     * @param corbaObj The CORBA Object for the reference string.
     * @return The object containing methods to call, or null on error.
     */
    protected Object getAccessor(String refName,
                                         org.omg.CORBA.Object corbaObj) {
        /*Messages.postDebug("PdaCorbaClient.getAccessor("
                           + refName + ", " + corbaObj);/*CMP*/
        Object accessor = null;
        try {
            accessor = Pda330Helper.narrow(corbaObj);
        } catch (Exception e) {
            Messages.postError("PdaCorbaClient.getAccessor: " + e);
            Messages.writeStackTrace(e);
        }
        return accessor;
    }

    /**
     * Set the file to be used to retrieve scans by index.
     * @param filename The name of the file (without the path).
     * @return The time in minutes.
     */
    /*
    public boolean setFileName(String filename) {
        if (!initAccessor()) {
            return false;
        }
        return m_accessor.setFileNameCORBA(filename);
    }
    */

    /**
     * Get the retention time for a given PDA scan.
     * @param index The number of the scan (starting with 1).
     * @return The time in minutes.
     */
    /*
    public double getTimeOfScan(int index) {
        if (!initAccessor()) {
            return -1;
        }

        return m_accessor.getRetentionTimeCORBA(index) / 60000.0;
    }
    */

    /**
     * Find the index of the PDA spectrum that is the
     * closest match to the given time.
     */
    //public int getClosestIndex(int timeMilliseconds) {
        /*
         * We implicitly assume a non-linear relationship, or else a
         * simple linear lookup would be faster.  It does use the
         * linear lookup to get a "start" value, then looks up or down
         * (as necessary) until the closest time is found.  It is also
         * assumed that the time-index relationship is monotonic.  As
         * of today (30 August 02) I the algorithm is not "air-tight"
         * with respect to what happens at the endpoints.
         */
        /*
        int index = -1;
        if (!initAccessor()) {
            return index;
        }

        int nextUp, nextDown;
        double scaleFactor;

        if (m_pdaDataFile == null) {
            return index;
        }
        Messages.postDebug("get closest index of " + timeMilliseconds
                           + " milliseconds");
        if (timeMilliseconds >= m_maxRetentionTime) {
            return m_numScans;
        }

        if (timeMilliseconds <= m_minRetentionTime) {
            return 1;               // style violation
        }

        scaleFactor = (double) (timeMilliseconds - m_minRetentionTime);
        if (m_maxRetentionTime != m_minRetentionTime) {
            scaleFactor /= (double) (m_maxRetentionTime - m_minRetentionTime);
        }
        index = (int) (m_numScans * scaleFactor);

        if (index < 1) {
            index = 1;
        } else if (index > m_numScans) {
            index = m_numScans;
        }

        // nextUp =  pdaAccessor.getRetentionTimeCORBA(index + 1);
        //nextDown =  pdaAccessor.getRetentionTimeCORBA(index - 1);

        //while( (timeMilliseconds > nextUp) && (index < numScans) ) {
        //    index++;
        //    nextUp =  pdaAccessor.getRetentionTimeCORBA(index + 1);
        //}

        //while( (timeMilliseconds < nextDown) && (index > 1) ) {
        //    index--;
        //    nextDown =  pdaAccessor.getRetentionTimeCORBA(index - 1);
        // }
        Messages.postDebug("closest index = " + index);
        return index;
    } */



    private String getPdaMethod(LcMethod method) {


        int interval;            // 100's of miliseconds  {1, 2, 4, 8, 16, 32}
        int slitwidth;           // nm 1, 2, 4, 8, 16 == {0, 1, 2, 3, 4} ** should be same value as bandwidth
        double runtime;           // length of the run
        int minwave;             // minimum wavelength for data 190-750 nm
        int maxwave;             // maximum wavelength for data 240-800 nm
        int monwave;             // wavelength of analog output and individual spectrum 190-800
        int bandwidth;           // nm 2, 2, 4, 8, 16 == {0, 1, 2, 3, 4}  ** should be same value as slitwidth

        String pdaMethod;        // bandwidth, interval, runtime, minwave, maxwave, monwave, slitwidth 

        if (method == null) {
            return null;
        }

        interval = 8;  //hard coded to 8 ms
        slitwidth = method.getBandwidth();
        runtime =  (int)(method.getEndTime());
        if (runtime <= 0) {
            runtime = 20.0;
        }
        minwave = method.getLambdaMin();
        maxwave = method.getLambdaMax();
        monwave = method.getLambda();
        bandwidth = method.getBandwidth();
        
        pdaMethod = bandwidth + ", " + interval + ", " + runtime + ", " + minwave + ", " +
            maxwave + ", " + monwave + ", " + slitwidth + "";
        String msg = "uvLambda " + monwave + " nm";
        m_expPanel.processStatusData(msg);
        return pdaMethod;
    }


    

    /**
     * PDA Commands
     */

    public boolean baselinePda() {
        try {
            m_accessor.pdaBaselineCORBA();
        } catch (Exception e) {
            Messages.postError("CORBA Error: "
                               + "Cannot baseline from PDA server");
            Messages.writeStackTrace(e);
            return false;
        }
        return true;
    }
 
  
    public boolean lampOnPda() {
        try { 
            m_accessor.pdaLampOnCORBA();
        } catch (Exception e) {
            Messages.postError("CORBA Error: "
                               + "Cannot turn on lamp from PDA server");
            Messages.writeStackTrace(e);
            return false;
        }
        return true;
        
    }

    public boolean lampOffPda() {
        try{
            m_accessor.pdaLampOffCORBA();
        } catch (Exception e) {
            Messages.postError("CORBA Error: "
                               + "Cannot turn off lamp from PDA server");
            Messages.writeStackTrace(e);
            return false;
        }
        return true;
    }
 
    public boolean resetPda() {
        try{
            m_accessor.pdaResetCORBA();
        } catch (Exception e) {
            Messages.postError("CORBA Error: "
                               + "Cannot reset from PDA server");
            Messages.writeStackTrace(e);
            return false;
        }
        return true;
    }

    public boolean downloadMethodPda() {
        String pdaMethod = "getting method";
        try{
            pdaMethod = getPdaMethod(m_control.getCurrentMethod());
            m_accessor.pdaLoadMethodCORBA(pdaMethod);
        } catch (Exception e) {
            Messages.postError("CORBA Error: " + pdaMethod + " " + e);
            //  + "Cannot load Method to PDA server");
            Messages.writeStackTrace(e);
            return false;
        }
        return true;
    }

     public void validatePda() {
        try{
            m_accessor.pdaValidateCORBA();
         } catch (Exception e) {
             Messages.postError("CORBA Error: Cannot Validate PDA");
                               //  + "Cannot load Method to PDA server");
            Messages.writeStackTrace(e);
        }
        return;
    }                                    

    public int getPdaStatePda() {
        try{
            return m_accessor.pdagetPDAstateCORBA();
        } catch (Exception e) {
            //Messages.postError("CORBA Error: "
            //                   + "Cannot get Pda State from PDA server");
            //Messages.writeStackTrace(e);
            return 19;
        }

    }

     public int getLampStatePda() {
        try{
            return m_accessor.pdagetLampstateCORBA();
        } catch (Exception e) {
            Messages.postError("CORBA Error: "
                               + "Cannot get Lamp State from PDA server");
            Messages.writeStackTrace(e);
        }
        return -1;
    }

     public String getErrorPda() {
        try{
            return m_accessor.pdagetErrorCORBA();
        } catch (Exception e) {
            Messages.postError("CORBA Error: "
                               + "Cannot get error from PDA server");
            Messages.writeStackTrace(e);
        }
        return "Error";
    }

    public void resetD2Pda() {
        Calendar rightNow= Calendar.getInstance();
        int month= rightNow.get(2) + 1;
        int day= rightNow.get(5);
        int year= rightNow.get(1);
        String sMonth;
        String sDay;
        
        if (month < 10) {
            sMonth= "0" + month;
        } else {
            sMonth= "" + month;
        }
        
        if (day <10) {
            sDay= "0" + day;
        } else {
            sDay= "" + day;
        }
        
        String date= sMonth + sDay + year;
        Messages.postError("Date: " + date);
        m_accessor.pdaResetD2CORBA(date);
    }

     public void resetHgPda() {
        Calendar rightNow= Calendar.getInstance();
        int month= rightNow.get(2) + 1;
        int day= rightNow.get(5);
        int year= rightNow.get(1);
        String sMonth;
        String sDay;
        
        if (month < 10) {
            sMonth= "0" + month;
        } else {
            sMonth= "" + month;
        }
        
        if (day <10) {
            sDay= "0" + day;
        } else {
            sDay= "" + day;
        }
        
        String date= sMonth + sDay + year;
        Messages.postError("Date: " + date);
        m_accessor.pdaResetHgCORBA(date);
    }

    public void saveDataPda(float data[], long pcTime) {

        try{
           if (out != null) {
               out.print(pcTime + " ");
               for(int i=0; i<data.length; i++) {
                   out.print(data[i] + " ");
               }
               out.println(" ");
           }
        } catch (Exception e) {
           Messages.postError("Error writing PDA data to file: " + e);
        }
        
    }


    public void saveDataPda(String dir) {

        File dataFile = new File(dir, "pdaData.pda");
        
        try{
            out = new PrintWriter(new BufferedWriter(new FileWriter(dataFile, false)));
             for(int i=0; i<m_vpdaPlot.m_scan.x.length; i++) {
                out.print(m_vpdaPlot.m_scan.x[i] + " ");
             }
             out.println(" ");
        } catch (Exception e) {
            Messages.postError("Error getting File output stream for PDA data." + e);
        }

    }
                           


    // implementation of Callback
    public void retrieveMessage(String action, String text) {
        String msg;
        String nums[];
        
        msg= "PdaValidate On";
        m_expPanel.processStatusData(msg);
        nums= text.split(" ");
        msg= "PdaD2Count " + "D2 Lamp has turned on " + nums[0] + " times for " + nums[1] +
                " hours since installed on " + nums[2];
        m_expPanel.processStatusData(msg);
        msg= "PdaHgCount " + "Hg Lamp has turned on " + nums[3] + " times for " + nums[4] +
                " hours since installed on " + nums[5];
        m_expPanel.processStatusData(msg);
        msg= "PdaCalbDate " + "Last wavelength calibration was performed on: " + nums[6];
        m_expPanel.processStatusData(msg);
        msg= "PdaD2Energy " + "D2 Lamp Energy at 250nm with 4nm slitwidth is " + nums[7] + " counts.";          
        m_expPanel.processStatusData(msg);
        Messages.postDebug("lampDiag", "lampDiag: " + text);
    }

    /**
     * The PDA sends data to this method whenever it is available.
     * @param action Not used.
     * @param pcTime The date-stamp of the data.
     * @param data An array containing either wavelengths or absorbance.
     */
    public void retrieveData(String action, long pcTime, float data[]) {

        long adcTime= 0;
        long prevTime= 0;
        
        //System.out.println("Data: " + data[0]);
        if(data[0]>100) {
            // this is wavelength data
            m_vpdaPlot.setXArray(data);
            m_vif.sendToVnmr("lccmd('pdaSaveData', svfdir+lcid)");
            sunTime= System.currentTimeMillis();
            deltaTime= sunTime - pcTime;
            firsttime= false;
        } else {
            // this is absorbance data
            m_vpdaPlot.setYArray(data);
            try{
                if(m_lcRun != null) {
                    adcTime= m_lcRun.getAdcStartTime_ms();
                    prevTime = m_lcRun.getPrevSegTime_ms();
                } else {
                    adcTime = sunTime; //time when first start run
                }
            } catch (Exception e) {
                Messages.postError("Error getting times: " + e);
            }
            long timeStamp= (pcTime + deltaTime - (adcTime - prevTime));
            double time_min = timeStamp / 60000.0;
            //saveDataPda(data, timeStamp);
            if (m_pdaDataListener != null) {
                m_pdaDataListener.processUvData(time_min, data);
                m_gotDataError = false;
            } else if (!m_gotDataError) {
                Messages.postError("PdaCorbaClient.retrieveData(): "
                                   + " Got data but no place to send it.");
                m_gotDataError = true;
            }
        }
        //if (firsttime) {
        //    firsttime= false;
        //    m_vif.sendToVnmr("lccmd('pdaSaveData', svfdir+lcid)");
        //} else {
        //    saveDataPda(data);
        //}
        //long sunTime= System.currentTimeMillis();
        //long deltaTime= sunTime - pcTime;
    }

    protected int checkAccessStatus(int status) {
        return 0;
    }


    class StatusPdaThread extends Thread {
        private int m_rate = 0;
        private volatile boolean m_quit = false;
         private String m_statusName = "";

        private String error;
        private String olderr = "";
        private int oldLampstate = -1;
        private int oldPDAstate = -1;

        /**
         * @param rate Number of milliseconds between status queries.
         * @param statusName What to call this module in the status string.
         */
        StatusPdaThread(int rate, String statusName) {
            m_rate = rate;
            m_statusName = statusName;
        }

       
        public void setQuit(boolean quit) {
            m_quit = quit;
        }

        public void setRate(int rate) {
            m_rate = rate;
        }

        public void updateStates() {
            int lampState=-1;
            int pdaState=-1;
            String lampStatus="";
            String pdaStatus="";
         
            lampState= getLampStatePda();
            pdaState= getPdaStatePda();

            if(oldLampstate != lampState) {
                oldLampstate = lampState;
                switch (lampState) {
                  case 0: //Lamp is Off
                          lampStatus = "Off";
                          break;
                  case 1: //Lamp is On
                          lampStatus = "On";
                          break;
                  case 2: //Turning Lamp On
                          lampStatus = "Turning On";
                          break;
                  case 3: //Turning Lamp off
                          lampStatus = "Turning Off";
                          break;
                }
                if(pdaState < 3) {
                    //PDA is still booting and initializing code, so lampstate unknown
                    lampStatus = "Lamp Not Ready";
                }
                String msg = m_statusName + "LampStatus " + lampStatus;
                m_expPanel.processStatusData(msg);
                Messages.postDebug("lampStatus", "lampStatus: " + lampStatus);
            }
            
            if(oldPDAstate != pdaState) {
                if(oldPDAstate == 7) {
                    // Bring up spectrum display at end of Run
                     m_vif.sendToVnmr("lccmd('pdaGetImage', svfdir +lcid)");
                }
                oldPDAstate = pdaState;
                if (pdaState==2) {
                    pdaStatus = "Loading Code";
                } else if (pdaState==6) {
                    //PDA ready and waiting
                    pdaStatus = "Ready";
                } else if(pdaState == 7) {
                    //PDA Running
                    pdaStatus = "Running";
                } else if(pdaState == 13) {
                    pdaStatus = "Activating";
                } else if(pdaState == 15) {
                    pdaStatus = "Checking Lamp";
                } else if(pdaState == 17) {
                    //PDA in Baseline
                    pdaStatus = "Baseline";
                } else{
                    //PDA is not ready
                    pdaStatus = "Not Ready";
                    if(!firsttime) {
                        try{
                            out.close();
                        } catch (Exception e) {
                            Messages.postError("Error closing PDA Data file." + e);
                        }
                    }
                    firsttime=true;
                }
                String msg = m_statusName + "Status " + pdaStatus;
                m_expPanel.processStatusData(msg);
            }
        
        }

        public void updateErrors() {
            error = getErrorPda();
            if (error.startsWith("P") && !(error.equals(olderr)) ) {
                olderr = error;
                Messages.postError("PDA Error:"+ error);
            }
        }

       
        public synchronized void run() {
            while (!m_quit) {
                if (getPdaStatePda()!=19) {
                    updateStates();
                    updateErrors();
                } else {
                    String msg = m_statusName + "Status " + "Not Ready";
                    m_expPanel.processStatusData(msg);
                    msg = m_statusName + "LampStatus " + "Not Ready";
                    m_expPanel.processStatusData(msg);
                }
                try {
                    Thread.sleep(m_rate);
                } catch (InterruptedException ie) {
                    // This is OK
                }
                if (m_rate == 0) {
                    setQuit(true);
                }
            }
            ////m_corbaClient.setScanOn(false);
           
            Messages.postDebug("PdaData",
                               "PdaUpdateThread.run(): Exiting");
        }
    }

    
    public class Detector extends UvDetector {

        /** Name of detector. */
        private String mm_name = null;

        /** The time between status reports, in ms. */
        private int mm_statusPeriod = 500;

        /**
         * Construct a "dead" version, just so that we can call methods
         * that could be static, if you could specify static methods
         * in an abstract class.
         */
        public Detector(StatusManager manager) {
            super(manager);
        }

        /**
         * Normal constructor.  Starts up status polling.
         */
        public Detector(StatusManager manager, String name) {
            super(manager);
            readStatus(mm_statusPeriod);   // Start the status polling
        }

        /**
         * Return the name of this detector, typically the model number, as
         * a string.
         */
        public String getName() {
            return mm_name;
        }

        /**
         * Read the status from the detector periodically.  This will include
         * the absorption at selected wavelengths, if possible.  These
         * wavelengths may be changed by downloadMethod().
         * @param period The time between samples in ms. If 0, read status once.
         */
        public boolean readStatus(int period) {
            mm_statusPeriod = period;
            if (pdaStatThread != null) {
                pdaStatThread.setRate(period);
            } else {
                pdaStatThread = new StatusPdaThread(mm_statusPeriod, "uv");
            }
            return true;
        }

        /** See if the detector is ready to start a run. */
        public boolean isReady() {
            return true;            // TODO: Implement isReady()
        }

        /**
         * Open communication with the detector.  This just starts the
         * status polling, as communication is always possible unless
         * there is some problem.
         */
        public boolean connect() {
            return readStatus(mm_statusPeriod);
        }

        /**
         * Shut down communication with the detector.
         * Just shuts down the status polling.
         */
        public void disconnect() {
            readStatus(0);
        }

        /** Determine if detector communication is OK. */
        public boolean isConnected() {
            return m_accessor != null;
        }

        /** Turn on the lamp. */
        public boolean lampOn() {
            return lampOnPda();
        }

        /** Turn off the lamp. */
        public boolean lampOff() {
            return lampOffPda();
        }

        /** Make the current absorption the zero reference. */
        public boolean autoZero() {
            return baselinePda();
        }

        /** Download a (constant) run method with maximum run time. */
        public boolean downloadMethod(LcMethod params) {
            String pdaMethod = "getting method";
            try{
                pdaMethod = getPdaMethod(params);
                m_accessor.pdaLoadMethodCORBA(pdaMethod);
            } catch (Exception e) {
                Messages.postError("CORBA Error: " + pdaMethod + " " + e);
                //  + "Cannot load Method to PDA server");
                Messages.writeStackTrace(e);
                return false;
            }
            return true;
        }

        public boolean isDownloaded(LcMethod params) {
            return false;
        }

        /**
         * Start a run, if possible.
         * This is a no-op for the 330, which just starts if it is
         * ready and it gets a hardware trigger from the pump.
         */
        public boolean start() {
            return isConnected();
        }

        /**
         * Stop a run and save data.
         * The 300 doesn't have a clean stop (that doesn't interrupt the
         * absorption monitoring).
         */
        public boolean stop() {
            return reset();
        }

        /**
         * Stop a run in such a way that it can be resumed.
         * The 330 can't do this at all.  So just let the data keep
         * being collected.
         */
        public boolean pause() {
            return false;
        }

        /**
         * Restart a paused run.
         * We can't do this, just return true if it's already running.
         */
        public boolean restart() {
            return getPdaStatePda() == 7;
        }

        /** Reset the detector state to be ready to start a new run. */
        public boolean reset() {
            return resetPda();
        }

        /**
         * Selects who deals with the data.
         * @return Returns true.
         */
        public boolean setDataListener(LcDataListener listener) {
            m_pdaDataListener = listener;
            return true;
        }
        
        /**
         * Selects channels and wavelengths to monitor for the chromatogram.
         * This is a no-op; data is only available through the analog output.
         */
        public boolean setChromatogramChannels(float[] lambdas, int[] traces,
                                               LcRun chromatogramManager) {
            return false;
        }
        
        /**
         * Gets the interval between data points.  Since this detector
         * interface does not support direct return of data, returns 0.
         * @return Zero, indicating data obtained only through ADC.
         */
        public int getDataInterval() {
            return 0;
        }

        /**
         * Get the string that uniquely identifies this detector type to the
         * software.
         * @return Returns "330".
         */
        public String getCanonicalIdString() {
            return "330";
        }

        /**
         * Get the label shown to the user to identify the detector type.
         * @return Returns "335 PDA".
         */
        public String getIdString() {
            return "330 PDA";
        }

        /**
         * Get the wavelength range this type of detector is good for.
         * @return Two ints, the min and max wavelengths, respectively, in nm.
         */
        public int[] getMaxWavelengthRange() {
            int[] rtn = {200, 800};
            return rtn;
        }

        /**
         * @return Returns true, since this detector can return spectra.
         */
        public boolean isPda() {
            return true;
        }

        /**
         * The number of "monitor" wavelengths this detector can support.
         * These may be monitored either from getting status or from
         * reading an analog-out value, but must be obtainable outside
         * of a run.
         * @return Returns 1.
         */
        public int getNumberOfMonitorWavelengths() {
            return 1;
        }

        /**
         * The number of "analog outputs" this detector supports, and
         * that we want the software to support.
         * @return Returns 1; the analog output is supported.
         */
        public int getNumberOfAnalogChannels() {
            return 1;
        }

    }


    
}

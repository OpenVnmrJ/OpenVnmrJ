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

import lcaccess.*;
import vnmr.ui.*;
import vnmr.util.*;


/**
 * A CORBA client that is used for talking to GPIB modules for LC.
 */
public class LcCorbaClient extends CorbaClient implements LcStatusListener {

    static private ArrayList<LcGpibModule> m_moduleTable
        = new ArrayList<LcGpibModule>();
    static private int m_gpibAccessorState = ERROR_UNKNOWN;
    static protected LcControl m_control;

    static public final int MAX_GPIB_ADDRESS = 15;
    static public final String V9050_DETECTOR = "9050";
    static public final String V9012_PUMP = "9012";

    static public final int IDX_9050 = 0;
    static public final int IDX_9012 = 1;
    static public final int N_MODULES = 2; // Keep me updated

    // UPDATE ME: This arrray needs N_MODULES elements.
    static final private String[] m_moduleName = {
        "9050 Detector",
        "9012 Pump",
    };

    public static final int PUMP_OFF = 0;
    public static final int PUMP_READY = 1;
    public static final int PUMP_RUNNING = 2;
    public static final int PUMP_STOPPED = 3;
    public static final int PUMP_NOT_READY = 4;
    public static final int PUMP_EQUILIBRATING = 5;
    public static final int PUMP_WAITING = 6;

    /**
     * This holds the handle of the one LcCorbaClient used by all modules.
     */
    static private LcCorbaClient sm_gpibClientObject = null;

    private int m_pumpAddress = -1;

    private Thread[] m_threads = new Thread[MAX_GPIB_ADDRESS + 1];
    private Set<LcStatusListener> m_pumpListenerList
        = new HashSet<LcStatusListener>();
    private Set<LcStatusListener> m_detectorListenerList
        = new HashSet<LcStatusListener>();
    private int[] m_corbaErrorType = new int[N_MODULES];

    // Last downloaded methods
    private MethodLine[] m_uvMethod = null;
    private MethodLine[] m_pumpMethod = null;

    /**
     * Construct a "dead" version, just so that we can call methods
     * that could be static, if you could specify static methods
     * in an interface.
     */
    public LcCorbaClient() {
    }

    /**
     * An LcCorbaClient factory method.
     * If the client is not already cached, creates it.
     * This method is synchronized so that only one client object can
     * get created.
     */
    synchronized static public LcCorbaClient getClient(LcControl control,
                                                       ButtonIF vif) {
        if (sm_gpibClientObject == null) {
            sm_gpibClientObject = new LcCorbaClient(control, vif);
        }
        return sm_gpibClientObject;
    }
        
    /**
     * Private constructor is only called through getClient().
     * This is one client for all GPIB modules.
     */
    private LcCorbaClient(LcControl control, ButtonIF vif) {
        super(vif);
        m_refPrefix = "gpib";
        m_control = control;
        for (int i = 0; i < N_MODULES; ++i) {
            m_corbaErrorType[i] = ERROR_OK;
        }
        if (m_moduleTable.size() == 0) {
            initModuleTable();
        }
        //initModuleTable();
        //startStatusThreads();
    }

    private boolean isEqualMethod(MethodLine[] m1, MethodLine[] m2) {
        if (m1 == null || m2 == null) {
            return false;
        }
        int n = m1.length;
        int m = m2.length;
        boolean rtn = (m == n);
        for (int i = 0; rtn && i < n; i++) {
            // TODO: It would be good if MethodLine.equals() were like this:
            rtn = ((m1[i].time == m2[i].time)
                   && (m1[i].opCode == m2[i].opCode)
                   && (m1[i].value == m2[i].value));
            //rtn = m1[i].equals(m2[i]);
        }
        return rtn;
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
     * Start getting status from the pump.
     * In future, this should return an LC pump interface, so that
     * different pumps can be supported.  See getDetector().
     */
    public void getPump(String name) {
        LcGpibModule[] ids =
                (LcGpibModule[])m_moduleTable.toArray(new LcGpibModule[0]);
        int address = 0;
        for (int i = 0; i < ids.length; i++) {
            if (ids[i].label.indexOf(name) >= 0) {
                address = ids[i].address;
                break;
            }
        }
        if (address > 0) {
            String cmd = "setStatusRate 60"; // Status reports / minute
            sendMessage(name, address, cmd);
            m_pumpAddress = address;
        } else {
            LcMsg.postError("No connection to " + name + " pump");
        }
    }

    static public String getFileStatusMsg(int fstat) {
        // NB: These messages are taken verbatim from the Bear S/W
        switch (fstat) {
          default: return null;
          case 1:  return "No Create File";
          case 2:  return "No Open File";
          case 3:  return "No Read File";
          case 4:  return "No Write File";
          case 5:  return "No Send File";
          case 6:  return "No Recv File";
          case 7:  return "Bad Table Name";
          case 8:  return "No Lock Table";
          case 9:  return "No Unlock Table";
          case 10: return "No Erase Table";
          case 11: return "No Read Table";
          case 12: return "No Write Table";
          case 13: return "Bad Method";
          case 14: return "No Copy Method";
          case 15: return "No Preset Method";
          case 16: return "Bad Event Mask";
          case 17: return "No Set Alarm";
          case 18: return "No Clr Alarm";
          case 19: return "Begin Transfer";
          case 20: return "No Rewind";
          case 21: return "Alarm Set";
          case 22: return "Alarm Cleared";
          case 23: return "Table Copied";
          case 24: return "Table Preset";
          case 25: return "Downloaded";
          case 26: return "Uploaded";
          case 27: return "No Send Request";
          case 28: return "No First Status";
          case 29: return "No Transfer";
          case 30: return "No Second Status";
        }
    }

    public boolean addPumpStatusListener(LcStatusListener listener) {
        return m_pumpListenerList.add(listener);
    }

    public boolean addDetectorStatusListener(LcStatusListener listener) {
        return m_detectorListenerList.add(listener);
    }

    public boolean removePumpStatusListener(LcStatusListener listener) {
        return m_pumpListenerList.remove(listener);
    }

    public void clearPumpStatusListeners() {
        m_pumpListenerList.clear();
    }

   public boolean removeDetectorStatusListener(LcStatusListener listener) {
        return m_detectorListenerList.remove(listener);
    }

    private void notifyDetectorListeners(int fstatus, String msg) {
        Object[] oList = m_detectorListenerList.toArray();
        for (int i = 0; i < oList.length; i++) {
            ((LcStatusListener)oList[i]).detectorFileStatusChanged(fstatus, msg);
        }
    }

    public void detectorFileStatusChanged(int status, String msg) {
        Messages.postDebug("DetectorStatus", "Detector File Status: " + msg);
    }

    private void notifyPumpListeners(Map<String, Object> state,
                                     int fstatus, String msg) {
        Object[] oList = m_pumpListenerList.toArray();
        for (int i = 0; i < oList.length; i++) {
            if (fstatus >= 0) {
                ((LcStatusListener)oList[i]).pumpFileStatusChanged(fstatus, msg);
            }
            ((LcStatusListener)oList[i]).pumpStateChanged(state);
        }
    }

    public void pumpFileStatusChanged(int status, String msg) {
        Messages.postDebug("PumpStatus", "Pump File Status: " + msg);
    }

    /**
     * Dummy 
     * @param state The new state of the pump.
     * */
    public void pumpStateChanged(Map<String, Object> state) {}

    /**
     * Get a list of modules connected to the GPIB bus.
     * These are strings giving both the module name and the
     * GPIB address, similar to:
     * <br>"9050 Detector [01]"
     */
    public String[] getModuleIds() {
        if (m_moduleTable.size() == 0) {
            initModuleTable();
        }
        
        LcGpibModule[] ids =
                (LcGpibModule[])m_moduleTable.toArray(new LcGpibModule[0]);
        String[] labels = new String[ids.length];
        String dets = "";
        String pumps = "";
        for (int i = 0; i < ids.length; i++) {
            labels[i] = ids[i].label;
            if (ids[i].type.equals("Detector")) {
                dets += "\"" + labels[i] + "\" "
                        + ids[i].refName + "\n";
            } else if (ids[i].type.equals("Pump")) {
                pumps += "\"" + labels[i] + "\" "
                        + ids[i].refName + "\n";
            }
        }
        if (dets.length() > 0) {
            String path = FileUtil.savePath("SYSTEM/ACQQUEUE/lcDetectors");
            dets += "None none\n";
            writeFile(path, dets);
        }
        if (pumps.length() > 0) {
            String path = FileUtil.savePath("SYSTEM/ACQQUEUE/lcPumps");
            pumps += "None none\n";
            writeFile(path, pumps);
        }

        return labels;
    }

    public boolean writeFile(String filepath, String content) {
        BufferedWriter fw = null;
        try {
            fw = new BufferedWriter(new FileWriter(filepath));
        } catch (IOException ioe) {
            Messages.postError("Cannot open output file: " + filepath);
            return false;
        }
        PrintWriter pw = new PrintWriter(fw);
        pw.print(content);
        pw.close();
        return true;
    }        

    public void mainAccessorChanged() {
        startStatusThreads();
    }

    public void startStatusThreads() {
        Messages.postDebug("gpibStatus", "startStatusThreads()");
        if (m_moduleTable.size() == 0) {
            initModuleTable();
        }
        String cmd = "setStatusRate 100";
        
        LcGpibModule[] ids =
                (LcGpibModule[])m_moduleTable.toArray(new LcGpibModule[0]);
        Messages.postDebug("gpibStatus", "startStatusThreads(): "
                           + "init'ed module table; got "
                           + ids.length + " IDs");
        String[] labels = new String[ids.length];
        for (int i = 0; i < ids.length; i++) {
            Messages.postDebug("gpibStatus", "startStatusThreads(): id.label="
                               + ids[i].label);
            String label = ids[i].label;
            if (label.indexOf(V9050_DETECTOR) >= 0
                || label.indexOf("310") >= 0)
            {
                //Messages.postDebug("gpibStatus", "Start 9050 status");
                //send9050Message(ids[i].address, cmd);
            } else if (label.indexOf(V9012_PUMP) >= 0
                       || label.indexOf("230") >= 0)

            {
                Messages.postDebug("gpibStatus", "Start 9012 status");
                send9012Message(ids[i].address, cmd);
            }
        }
    }

    protected Object getAccessor(String refName) {
        if (refName == null) {
            return null;
        }
        m_refPrefix = "gpib";
        return super.getAccessor(refName);
    }

    //protected void invalidateAccessor(String refName) {
    //    if (refName != null && ! refName.equals(GPIB_MODULES)) {
    //        invalidateAccessor(GPIB_MODULES);
    //    }
    //    super.invalidateAccessor(refName);
    //}

    private void corbaOK(int idx) {
        if (m_corbaErrorType[idx] != ERROR_OK) {
            Messages.postInfo("... CORBA to " + m_moduleName[idx] + " is OK.");
            m_corbaErrorType[idx] = 0;
        }
    }

    private void corbaError(int idx, int type, String msg) {
        if (m_corbaErrorType[idx] != type) {
            m_corbaErrorType[idx] = type;
            String sType = "";
            switch (type) {
            case ERROR_COMM_FAILURE:
                sType = "Communication failure";
                break;
            case ERROR_NO_ACCESSOR:
                sType = "Initialization failure";
                break;
            case ERROR_BAD_ADDRESS:
                sType = "Invalid GPIB address";
                break;
            case ERROR_BAD_METHOD:
                sType = "Invalid method";
                break;
            case ERROR_MISC_FAILURE:
                sType = "Unknown failure";
                break;
            }
            Messages.postError("CORBA to " + m_moduleName[idx] + ": "
                               + sType + " " + msg + " ...");
        }
    }

    /**
     * Initialize info about what modules are connected to the GPIB.
     */
    private void initModuleTable() {
        Messages.postDebug("gpibStatus", "LcCorbaClient.initModuleTable()");
         // Force making new accessor
        //invalidateAccessor(GPIB_MODULES, ERROR_UNKNOWN);
        m_corbaOK = false;
        ModuleAccessor accessor
                = (ModuleAccessor)getAccessor(GPIB_MODULES);
        if (accessor == null) {
            if (m_gpibAccessorState != getAccessorState(GPIB_MODULES)) {
                m_gpibAccessorState = getAccessorState(GPIB_MODULES);
                Messages.postDebug("No communication with CORBA server");
            }
            return;
        }
        m_moduleTable.clear();
        for (int i = 1; i <= MAX_GPIB_ADDRESS; i++) {
            String name = null;
            javax.swing.Timer timer;
            timer = Messages.setTimeout(CORBA_CONNECT_TIMEOUT,
                                        "CORBA GPIB server not responding."
                                        + " Still trying ...");
            try {
                Messages.postDebug(//"CorbaInit",
                                   "LcCorbaClient.initModuleTables: "
                                   + "getModuleName " + i
                                   + " from accessor ...");
                name = accessor.getModuleName(i);
                Messages.postDebug(//"CorbaInit",
                                   "LcCorbaClient.initModuleTables: "
                                   + "... got name: " + name);
                if (! timer.isRunning()) {
                    Messages.postInfo("... CORBA GPIB server OK");
                }
                timer.stop();
                corbaOK();
                //if (! timer.isRunning()) {
                //    Messages.postDebug("... CORBA GPIB server OK");
                //}
                timer.stop();
                m_corbaOK = true;
            } catch (org.omg.CORBA.COMM_FAILURE ccf) {
                timer.stop();
                Messages.postError("CORBA communication failure");
                corbaError(ERROR_COMM_FAILURE, "");
                invalidateAccessor(GPIB_MODULES, ERROR_COMM_FAILURE);
                break;
            } catch (org.omg.CORBA.SystemException cse) {
                timer.stop();
                Messages.postError("CORBA SystemException");
                corbaError(ERROR_COMM_FAILURE, "");
                invalidateAccessor(GPIB_MODULES, ERROR_COMM_FAILURE);
                break;
            } catch (Exception e) {
                Messages.postError("CORBA failed to connect: " + e);
                break;
            } finally {
                Messages.postDebug("*** initModuleTable: finally ***");
            }
            Messages.postDebug("*** initModuleTable: after finally ***");
            if (name != null && name.length() > 0) {
                m_moduleTable.add(new LcGpibModule(name, i));
            }
        }
    }

    /**
     * Send a message to a given module
     */
    public boolean sendMessage(String module, String msg) {
        if (V9012_PUMP.equals(module)) {
            return sendMessage(module, m_pumpAddress, msg);
        }
        return false;
    }

    /**
     * Send a message to a given module at a given GPIB address.
     */
    public boolean sendMessage(String module, int address, String msg) {
        Messages.postDebug("gpibCmd",
                           "LcCorbaClient.sendMessage(module=" + module
                           + ", addr=" + address + ", msg=" + msg);
        boolean status = false;
        if (!m_corbaOK) {
            return false;
        }
        if (module == null) {
            Messages.postDebug("LcCorbaClient.sendMessage(): null module name");
            status = false;
        } else if (module.equals(V9050_DETECTOR)) {
            status = send9050Message(address, msg);
        } else if (module.equals(V9012_PUMP)) {
            status = send9012Message(address, msg);
        } else {
            Messages.postDebug("LcCorbaClient.sendMessage(): Unknown module: "
                               + module);
            status = false;
        }
        return status;
    }

    /**
     * Send a message to the 9050 detector at a given GPIB address.
     * <br>Messages:
     * <br>lampOn
     * <br>lampOff
     * <br>start
     * <br>stop
     * <br>reset
     * <br>activateMethod methodNumber
     * <br>setAttenuation value
     * <br>setStatusRate updatesPerMinute statusVariable
     * <br>
     * Setting the status rate to a non-zero value also starts a
     * separate thread that reads the status.
     */
    private boolean send9050Message(int addr, String msg) {
        if (msg == null) {
            Messages.postDebug("LcCorbaClient.send9050Message():"
                               + " Null message for detector");
            return false;
        }
        if (addr <= 0 || addr > MAX_GPIB_ADDRESS) {
            Messages.postDebug("LcCorbaClient.send9050Message(): "
                               + "Bad GPIB detector address: " + addr);
            return false;
        }
            
        String cmd = msg;
        StringTokenizer toker = new StringTokenizer(msg);
        if (toker.hasMoreTokens()) {
            cmd = toker.nextToken();
        }

        boolean status = true;
        Detector9050 accessor = (Detector9050)getAccessor(V9050_DETECTOR);
        try {
            if (accessor == null) {
                Messages.postDebug("LcCorbaClient.send9050Message(): "
                                   + "No accessor for 9050 detector");
                status = false;
            } else if (cmd.equals("lampOn")) {
                accessor.setLampOn(addr);
            } else if (cmd.equals("lampOff")) {
                accessor.setLampOff(addr);
            } else if (cmd.equals("start")) {
                accessor.start(addr);
            } else if (cmd.equals("stop")) {
                accessor.stop(addr);
            } else if (cmd.equals("reset")) {
                accessor.reset(addr);
            } else if (cmd.equals("autozero")) {
                accessor.autozero(addr);
            } else if (cmd.equals("activateMethod")) {
                if (toker.hasMoreTokens()) {
                    int method = 0;
                    try {
                        method = Integer.parseInt(toker.nextToken());
                        accessor.activateMethod(addr, method);
                    } catch (NumberFormatException nfe) {
                        Messages.postDebug("LcCorbaClient.send9050Message(): "
                                           + "Garbled detector method number: "
                                           + msg);
                        status = false;
                    } catch (InvalidValueException ive) {
                        Messages.postDebug("LcCorbaClient.send9050Message(): "
                                           + "Bad detector method number:"
                                           + msg);
                        status = false;
                    }
                }
            } else if (cmd.equals("setStatusRate")) {
                if (toker.hasMoreTokens()) {
                    int rate = 0;   // Updates per minute
                    try {
                        rate = Integer.parseInt(toker.nextToken());
                        // Sets rate remote server queries module for status
                        accessor.requestStatus(addr, rate);
                    } catch (NumberFormatException nfe) {
                        Messages.postDebug("LcCorbaClient.send9050Message(): "
                                           + "Bad detector status rate string:"
                                           + msg);
                        status = false;
                    } catch (InvalidValueException ive) {
                        Messages.postDebug("LcCorbaClient.send9050Message(): "
                                           + "Bad detector status rate value:"
                                           + msg);
                        status = false;
                    }
                    Status9050Thread statusThread =
                            (Status9050Thread)m_threads[addr];
                    String name = "uv";
                    if (toker.hasMoreTokens()) {
                        name = toker.nextToken();
                    }
                    if (rate == 0) {
                        if (statusThread != null) {
                            statusThread.setQuit(true);
                            statusThread.interrupt();
                            m_threads[addr] = statusThread = null;
                        }
                    } else {
                        int delay = (60 * 1000) / rate;
                        if (statusThread == null) {
                            statusThread = new Status9050Thread(delay,
                                                                accessor,
                                                                addr,
                                                                name);
                            statusThread.start();
                            m_threads[addr] = statusThread;
                        } else {
                            statusThread.setRate(delay);
                            statusThread.interrupt();
                        }
                    }
                }
            } else if (cmd.equals("downloadMethod")) {
                m_uvMethod = get9050Method(m_control.getCurrentMethod());
                accessor.downloadMethod(addr, m_uvMethod);
                addDetectorStatusListener(this);
            } else {
                Messages.postDebug("Unknown " + V9050_DETECTOR
                                   + "detector command: \"" + msg + "\"");
                status = false;
            }
            corbaOK(IDX_9050);
        } catch (InvalidAddressException iae) {
            corbaError(IDX_9050, ERROR_BAD_ADDRESS, "");
            invalidateAccessor(V9050_DETECTOR, ERROR_BAD_ADDRESS);
            status = false;
        } catch (InvalidMethodException ime) {
            Messages.postDebug("Invalid 9050 detector method");
            corbaOK(IDX_9050);
            status = false;
        } catch (org.omg.CORBA.SystemException cse) {
            corbaError(IDX_9050, ERROR_COMM_FAILURE, "");
            invalidateAccessor(V9050_DETECTOR, ERROR_COMM_FAILURE);
            status = false;
        }

        return status;
    }

    private MethodLine[] get9050Method(LcMethod method) {

        final int TIME_CONSTANT = 0;
        final int LAMBDA = 1;
        final int ATTENUATION = 2;
        final int AUTOZERO = 3;
        final int PEAK_WIDTH = 4;
        final int PEAK_RATIO = 5;
        final int PEAK_STATE = 6;
        final int RELAY_PERIOD = 7;
        final int RELAY_TRIGGER = 8;
        //final int PEAK_PULSE = 9; // Not used
        final int RELAYS = 10;
        final int END_TIME = 11;

        if (method == null) {
            return null;
        }
        ArrayList<MethodLine> alMethod = new ArrayList<MethodLine>();
        alMethod.add(new MethodLine(0, TIME_CONSTANT, 1000));
        alMethod.add(new MethodLine(0, LAMBDA, method.getLambda()));
        alMethod.add(new MethodLine(0, ATTENUATION, 100));
        alMethod.add(new MethodLine(0, AUTOZERO, 0));
        alMethod.add(new MethodLine(0, PEAK_WIDTH, 4));
        alMethod.add(new MethodLine(0, PEAK_RATIO, 8));
        alMethod.add(new MethodLine(0, PEAK_STATE, 1));
        alMethod.add(new MethodLine(0, RELAY_PERIOD, 4));
        alMethod.add(new MethodLine(0, RELAY_TRIGGER, 3));
        alMethod.add(new MethodLine(0, RELAYS, 0));
        int endTime = (int)(100 * method.getEndTime());
        alMethod.add(new MethodLine(endTime, END_TIME, 0));

        return alMethod.toArray(new MethodLine[0]);
    }

    /**
     * Send a message to the 9012 pump at a given GPIB address.
     * <br>Messages:
     * <br>purge
     * <br>prime
     * <br>pump
     * <br>start
     * <br>stop
     * <br>reset
     * <br>activateMethod methodNumber
     * <br>setStatusRate updatesPerMinute statusVariable
     * <br>
     * Setting the status rate to a non-zero value also starts a
     * separate thread that reads the status.
     */
    private boolean send9012Message(int addr, String msg) {
        if (msg == null) {
            Messages.postDebug("LcCorbaClient.send9012Message():"
                               + " Null message for pump");
            return false;
        }
        if (addr <= 0 || addr > MAX_GPIB_ADDRESS) {
            Messages.postDebug("LcCorbaClient.send9012Message(): "
                               + "Bad GPIB pump address: " + addr);
            return false;
        }
            
        String cmd = msg;
        StringTokenizer toker = new StringTokenizer(msg);
        if (toker.hasMoreTokens()) {
            cmd = toker.nextToken();
        }

        boolean status = true;
        Pump9012 accessor = (Pump9012)getAccessor(V9012_PUMP);
        try {
            if (accessor == null) {
                Messages.postDebug("LcCorbaClient.send9012Message(): "
                                   + "No accessor for 9012 pump");
                status = false;
            } else if (cmd.equals("pump")) {
                accessor.pump(addr);
            } else if (cmd.equals("start")) {
                accessor.start(addr);
            } else if (cmd.equals("stop")) {
                accessor.stop(addr);
            } else if (cmd.equals("reset")) {
                // NB: In general, takes 2 resets to reset completely
                accessor.reset(addr);
                accessor.reset(addr);
            } else if (cmd.equals("activateMethod")) {
                if (toker.hasMoreTokens()) {
                    int method = 0;
                    try {
                        method = Integer.parseInt(toker.nextToken());
                        accessor.activateMethod(addr, method);
                    } catch (NumberFormatException nfe) {
                        Messages.postDebug("LcCorbaClient.send9012Message(): "
                                           + "Bad pump method number string:"
                                           + msg);
                        status = false;
                    } catch (InvalidValueException ive) {
                        Messages.postDebug("LcCorbaClient.send9012Message(): "
                                           + "Bad pump method number:" + msg);
                        status = false;
                    }
                }
            } else if (cmd.equals("downloadMethod")) {
                m_pumpMethod = get9012Method(m_control.getCurrentMethod());
                accessor.downloadMethod(addr, m_pumpMethod);
                addPumpStatusListener(this);
            } else if (cmd.equals("setStatusRate")) {
                if (toker.hasMoreTokens()) {
                    int rate = 0;   // Updates per minute
                    try {
                        rate = Integer.parseInt(toker.nextToken());
                        // Sets rate remote server queries module for status
                        accessor.requestStatus(addr, rate);
                    } catch (NumberFormatException nfe) {
                        Messages.postDebug("LcCorbaClient.send9012Message(): "
                                           + "Bad pump status rate string:"
                                           + msg);
                        status = false;
                    } catch (InvalidValueException ive) {
                        Messages.postDebug("LcCorbaClient.send9012Message(): "
                                           + "Bad pump status rate:" + msg);
                        status = false;
                    }
                    Status9012Thread statusThread = getPumpStatusThread(addr);
                    String name = "pump";
                    if (toker.hasMoreTokens()) {
                        name = toker.nextToken();
                    }
                    if (rate == 0) {
                        if (statusThread != null) {
                            statusThread.setQuit(true);
                            statusThread.interrupt();
                            m_threads[addr] = statusThread = null;
                        }
                    } else {
                        int delay = (60 * 1000) / rate;
                        if (statusThread == null) {
                            statusThread = new Status9012Thread(delay,
                                                                accessor,
                                                                addr,
                                                                name);
                            statusThread.start();
                            m_threads[addr] = statusThread;
                        } else {
                            statusThread.setRate(delay);
                            statusThread.interrupt();
                        }
                    }
                }
            } else {
                Messages.postDebug("Unknown " + V9012_PUMP
                                   + " pump command: \"" + msg + "\"");
                status = false;
            }
            corbaOK(IDX_9012);
        } catch (InvalidAddressException iae) {
            corbaError(IDX_9012, ERROR_BAD_ADDRESS, "");
            invalidateAccessor(V9012_PUMP, ERROR_BAD_ADDRESS);
            status = false;
        } catch (InvalidMethodException ime) {
            Messages.postDebug("Invalid 9012 pump method");
            corbaOK(IDX_9012);
            status = false;
        } catch (org.omg.CORBA.SystemException se) {
            corbaError(IDX_9012, ERROR_COMM_FAILURE, "");
            invalidateAccessor(V9012_PUMP, ERROR_COMM_FAILURE);
            status = false;
        }

        return status;
    }

    public boolean isPumpDownloaded(LcMethod params) {
        MethodLine[] aMethod = get9012Method(params);
        return isEqualMethod(aMethod, m_pumpMethod);
    }

    /**
     * Get the status thread for the pump.
     * @return The status thread.
     */
    private Status9012Thread getPumpStatusThread() {
        return getPumpStatusThread(m_pumpAddress);
    }

    /**
     * Get the status thread for the pump at the given GPIB address.
     * @param addr The GPIB address of the pump.
     * @return The status thread.
     */
    private Status9012Thread getPumpStatusThread(int addr) {
        Status9012Thread thread = (Status9012Thread)m_threads[addr];
        return thread;
    }

    public Map<String,Object> getPumpState() {
        return getPumpStatusThread().getPumpState();
    }

    static public MethodLine[] get9012Method(LcMethod method) {
        final int PMAX = 0;
        final int PMIN = 1;
        final int RELAYS = 2;
        final int FLOW = 3;
        final int PERCENTS = 4;
        final int END_TIME = 5;
        final int EQUIL_TIME = 6;

        if (method == null) {
            return null;
        }
        ArrayList<MethodLine> alMethod = new ArrayList<MethodLine>();
        alMethod.add(new MethodLine(0, PMAX, method.getMaxPressure()));
        alMethod.add(new MethodLine(0, PMIN, method.getMinPressure()));
        int relays = method.getInitialRelays();
        alMethod.add(new MethodLine(0, RELAYS, relays));
        int flow = (int)Math.round(1000 * method.getInitialFlow());
        alMethod.add(new MethodLine(0, FLOW, flow));
        double[] pct = method.getInitialPercents();
        int pcts = (int)pct[2] | ((int)pct[1] << 8) | ((int)pct[0] << 16);
        alMethod.add(new MethodLine(0, PERCENTS, pcts));
        int t = (int)(100 * method.getEquilTime());
        alMethod.add(new MethodLine(0, EQUIL_TIME, t));
        int endTime = (int)(100 * method.getEndTime());
        int endAction = method.getEndAction().equals("pumpOff")
                ? Pump9012.ENDACTION_STOP
                : Pump9012.ENDACTION_HOLD;

        Messages.postDebug("lcPumpMethod",
                           "Pump Method: "
                           + "MaxPressure=" + Fmt.f(2, method.getMaxPressure())
                           + ", Min=" + Fmt.f(2, method.getMinPressure())
                           + ", Relays=" + Integer.toString(relays, 2)
                           + ", Flow=" + Fmt.f(3, flow / 1000.0)
                           + ", Pcts=" + Fmt.f(1, pct[0])
                           + ", " + Fmt.f(1, pct[1])
                           + ", " + Fmt.f(1, pct[2])
                           + ", Equil=" + Fmt.f(2, t / 100.0)
                           );
        
        int nRows = method.getRowCount();
        for (int i = 1; i < nRows; i++) {
            int time = (int)(100 * method.getTime(i));
            if (time < 0) {
                Messages.postDebug("LC method bad -- internal error");
                return null;
            } else if (time > endTime) {
                Messages.postDebug("LC method has entries after end time");
                break;
            }

            int r = method.getRelays(i);
            if (relays != r) {
                relays = r;
                alMethod.add(new MethodLine(time, RELAYS, relays));
                Messages.postDebug("lcPumpMethod",
                                   "T=" + Fmt.f(2, time / 100.0)
                                   + ": relays=" + Integer.toString(relays, 2));
            }

            // TODO: Not allowed to vary the flow for now.
            int f = (int)Math.round(1000 * method.getInitialFlow());
            if (flow != f) {
                flow = f;
                alMethod.add(new MethodLine(time, FLOW, flow));
                Messages.postDebug("lcPumpMethod",
                                   "T=" + Fmt.f(2, time / 100.0)
                                   + ": flow=" + Fmt.f(3, flow / 1000.0));
            }

            pct = method.getPercents(i);
            if (pct != null) {
                pcts = (int)pct[2] | ((int)pct[1] << 8) | ((int)pct[0] << 16);
                alMethod.add(new MethodLine(time, PERCENTS, pcts));
                Messages.postDebug("lcPumpMethod",
                                   "T=" + Fmt.f(2, time / 100.0)
                                   + ": Pcts=" + Fmt.f(1, pct[0])
                                   + ", " + Fmt.f(1, pct[1])
                                   + ", " + Fmt.f(1, pct[2]));
            }
        }
        
        alMethod.add(new MethodLine(endTime, END_TIME, endAction));
        Messages.postDebug("lcPumpMethod",
                           "T=" + Fmt.f(2, endTime / 100.0)
                           + ", End action="
                           + (endAction == Pump9012.ENDACTION_STOP
                              ? "stop" : "hold"));

        return alMethod.toArray(new MethodLine[0]);
    }

    /**
     * Get the object for calling methods for a given module.
     * @param refName The base name of the reference string for
     * the module, e.g. "9050".
     * @param corbaObj The CORBA Object for the reference string.
     * @return The object containing methods to call, or null on error.
     */
    protected Object getAccessor(String refName,
                                 org.omg.CORBA.Object corbaObj) {
        Object accessor = null;
        try {
            if (refName == null) {
                return null;
            } else if (refName.equals("Access")) {
                accessor = ModuleAccessorHelper.narrow(corbaObj);
            } else if (refName.equals("9050")) {
                accessor = Detector9050Helper.narrow(corbaObj);
            } else if (refName.equals("9012")) {
                accessor = Pump9012Helper.narrow(corbaObj);
            } else {
                Messages.postDebug("LcCorbaClient.getAccessor(): No helper for "
                                   + refName);
                return null;
            }
        } catch (org.omg.CORBA.BAD_PARAM cbp) {
            Messages.postDebug("Cannot narrow CORBA accessor: " + cbp);
        } catch (org.omg.CORBA.COMM_FAILURE ccf) {
            Messages.postDebug("Cannot narrow CORBA accessor: " + ccf);
        }
        return accessor;
    }

    /**
     * Check if we have contact with the CORBA server.
     * @param status The previous access status: ACCESS_STATUS_OK
     * or ACCESS_STATUS_NO_CONTACT.
     * @return The current access status.
     */
    protected int checkAccessStatus(int status) {
        ModuleAccessor accessor = (ModuleAccessor)getAccessor(GPIB_MODULES);
        if (accessor == null && status != ACCESS_STATUS_NO_CONTACT) {
            status = ACCESS_STATUS_NO_CONTACT;
            Messages.postError("No contact with CORBA server ...");
        } else if (accessor != null) {
            try {
                accessor.getNumberOfConnectedModules();
                corbaOK();
            } catch (org.omg.CORBA.COMM_FAILURE cfe) {
                corbaError(ERROR_COMM_FAILURE, "");
                invalidateAccessor(GPIB_MODULES, ERROR_COMM_FAILURE);
            } catch (org.omg.CORBA.SystemException cse) {
                corbaError(ERROR_MISC_FAILURE, "");
                invalidateAccessor(GPIB_MODULES, ERROR_MISC_FAILURE);
            } finally {
                Messages.postDebug("LcCorbaClient",
                                   "*** StatusCorbaAccessThread: "
                                   + "in finally ***");
            }
            Messages.postDebug("LcCorbaClient",
                               "*** StatusCorbaAccessThread: "
                               + "after finally ***");
        }
        return status;
    }


    class Status9050Thread extends Thread {
        private int m_rate = 0;
        //private Detector9050 m_accessor = null;
        private int m_address = 0;
        private String m_statusName = "";
        private volatile boolean m_quit = false;

        /**
         * @param rate Number of milliseconds between status queries.
         * @param accessor The CORBA accessor for the status requests.
         * @param address The GPIB address of the module .
         * @param statusName What to call this module in the status string.
         */
        Status9050Thread(int rate, Detector9050 accessor,
                         int address, String statusName) {
            m_rate = rate;
            //m_accessor = accessor;
            m_address = address > MAX_GPIB_ADDRESS ? 0 : Math.max(0, address);
            m_statusName = statusName;
            if (m_address == 0) {
                Messages.postDebug("LcCorbaClient.Status9050Thread.<init>: "
                                   + "Invalid GPIB detector address: "
                                   + address);
            }
        }

        public void setQuit(boolean quit) {
            m_quit = quit;
        }

        public void setRate(int rate) {
            m_rate = rate;
        }

        public synchronized void run() {
            boolean firstTime = true;
            int methodState = -1;
            int lampState = -1;
            int lambda = -1;
            int runtime = -1;
            double absorb = -1;
            int zero = -999999;
            long tZero = 0;
            int[] status = null;
            int fileStatus = -1;
            int[] fstatus = null;
            boolean lampOnOk = false;
            boolean lampOffOk = false;
            boolean autoZeroOk = false;
            boolean resetOk = false;
            boolean downloadOk = false;
            boolean startOk = false;
            boolean stopOk = false;

            if (m_address == 0) {
                return;         // Can't deal with this
            }

            while (!m_quit) {
                try {
                    Thread.sleep(m_rate);
                } catch (InterruptedException ie) {
                    // This is OK
                }
                Detector9050 accessor
                        = (Detector9050)getAccessor(V9050_DETECTOR);
                if (accessor == null) {
                    continue;   // Try again later
                }
                try {
                    status = accessor.getLatestStatus(m_address);
                    if (status == null) {
                        Messages.postDebug("LcCorbaClient.Status9050Thread.run:"
                                           + " Detector status not available");
                    } else if (status.length < 24) {
                        if (status.length == 0) {
                            int updatesPerMinute = (60 * 1000) / m_rate;
                            try {
                                accessor.requestStatus(m_address,
                                                       updatesPerMinute);
                            } catch (InvalidValueException ive) {
                                Messages.postDebug("Status9050Thread.run: "
                                                   + "Bad status rate value:"
                                                   + updatesPerMinute);
                                m_rate = 1000;
                            }
                        } else {
                            Messages.postDebug("LcCorbaClient.Status9050Thread:"
                                               + " Short detector status: "
                                               + status.length + " words");
                        }
                    } else if (status.length > 35) {
                        Messages.postDebug("LcCorbaClient.Status9050Thread.run:"
                                           + " Long detector status: "
                                           + status.length + " words");
                    } else {
                        // Detector ID
                        if (firstTime) {
                            m_expPanel.processStatusData("uvId 310 / 9050");
                            m_expPanel.processStatusData("uvTitle UV");
                            firstTime = false;
                        }

                        // Lamp status
                        if (lampState != status[9]) {
                            lampState = status[9];
                            String state;
                            switch (lampState) {
                              case 0:
                                state = "On";
                                break;
                              case 1:
                                state = "Starting";
                                break;
                              case 2:
                                state = "Off";
                                break;
                              default:
                                state = "LampState=" + lampState;
                                break;
                            }
                            String msg = m_statusName + "LampStatus " + state;
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);
                        }

                        // Read run time
                        if (runtime != status[29]) {
                            runtime = status[29];
                            String msg = m_statusName + "Time "
                                    + (runtime / 100.0) + " min";
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);
                        }

                        // Read wavelength
                        {
                            lambda = status[15];
                            String msg = m_statusName + "Lambda "
                                    + lambda + " nm";
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);
                        }

                        // Check for auto zero
                        if (status[17] != zero) {
                            zero = status[17];
                            tZero = System.currentTimeMillis();
                            m_control.setUvOffset();
                            Messages.postDebug("Zero","zero=" + zero);
                        }

                        // Read absorption
                        {
                            String msg = m_statusName + "Attn ";
                            if (status[23] > 2) { // Is detector ready?
                                // NB: Cannot get absorption from CORBA.
                                //     Get latest value read from SLIM.
                                if (tZero != 0 &&
                                    System.currentTimeMillis() - tZero < 2000)
                                {
                                    m_control.setUvOffset();
                                }
                                absorb = m_control.getUvAbsorption();
                                msg += Fmt.f(4, absorb) + " AU";
                            }
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("lcAdc",
                                               "gpibStatus: " + msg);
                        }

                        // Read method status
                        if (methodState != status[23]) {
                            methodState = status[23];
                            String state;
                            switch (methodState) {
                              case 0:
                                if (lampState == 1 || lampState == 2) {
                                    state = "Lamp_Off";
                                } else {
                                    state = "Not_Ready";
                                }
                                break;
                              case 1:
                                state = "Initialize";
                                break;
                              case 2:
                                state = "Baseline";
                                break;
                              case 3:
                                state = "Ready";
                                break;
                              case 4:
                                state = "Run";
                                break;
                              case 5:
                                state = "Stop";
                                break;
                              default:
                                state = "State=" + methodState;
                                break;
                            }
                            String msg = m_statusName + "Status " + state;
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);
                            if (methodState >= 3) {
                                m_expPanel.processStatusData(m_statusName
                                                             + "LampOn true");
                            } else if (methodState == 0) {
                                m_expPanel.processStatusData(m_statusName
                                                             + "LampOn false");
                            }
                        }
                    }

                    // Which actions are OK to do in the current state
                    boolean ok = (methodState == 0 && lampState == 2);
                    if (ok != lampOnOk) {
                        lampOnOk = ok;
                        m_expPanel.processStatusData("uvLampOnOk " + ok);
                    }
                    ok = (methodState == 3
                          || methodState == 5
                          || (methodState == 0 && lampState == 0));
                    if (ok != lampOffOk) {
                        lampOffOk = ok;
                        m_expPanel.processStatusData("uvLampOffOk " + ok);
                    }
                    //ok = (methodState == 3);
                    ok = true;
                    if (ok != autoZeroOk) {
                        autoZeroOk = ok;
                        m_expPanel.processStatusData("uvAutoZeroOk " + ok);
                    }
                    //ok = (methodState == 2
                    //      || methodState == 3
                    //      || methodState == 5);
                    ok = true;
                    if (ok != resetOk) {
                        resetOk = ok;
                        m_expPanel.processStatusData("uvResetOk " + ok);
                    }
                    ok = (methodState == 0
                          || methodState == 3
                          || methodState == 5);
                    if (ok != downloadOk) {
                        downloadOk = ok;
                        m_expPanel.processStatusData("uvDownloadOk " + ok);
                    }
                    ok = (methodState == 3
                          || methodState == 5);
                    if (ok != startOk) {
                        startOk = ok;
                        m_expPanel.processStatusData("uvStartOk " + ok);
                    }
                    ok = (methodState == 4);
                    if (ok != stopOk) {
                        stopOk = ok;
                        m_expPanel.processStatusData("uvStopOk " + ok);
                    }

                    // Check the file status (result of method download)
                    fstatus = accessor.getFileStatus(m_address);
                    if (fstatus == null) {
                        Messages.postDebug("LcCorbaClient.Status9050Thread.run:"
                                           + " Detector file status not found");
                    } else if (fstatus.length != 2) {
                        Messages.postDebug("LcCorbaClient.Status9050Thread.run:"
                                           +
                                           " Detector file status bad length: "
                                           + fstatus.length + " words");
                    } else {
                        if (fileStatus != fstatus[1]) {
                            fileStatus = fstatus[1];
                            String msg = getFileStatusMsg(fileStatus);
                            if (msg != null) {
                                notifyDetectorListeners(fileStatus, msg);
                            }
                        }
                    }
                    corbaOK(IDX_9050);
                } catch (InvalidAddressException iae) {
                    corbaError(IDX_9050, ERROR_BAD_ADDRESS, "");
                    invalidateAccessor(V9050_DETECTOR, ERROR_BAD_ADDRESS);
                } catch (org.omg.CORBA.SystemException se) {
                    corbaError(IDX_9050, ERROR_COMM_FAILURE, "");
                    invalidateAccessor(V9050_DETECTOR, ERROR_COMM_FAILURE);
                }
            }

            // Reset status to "unknown"?
        }
    }


    class Status9012Thread extends Thread {
        private int m_rate = 0;
        //private Pump9012 m_accessor = null;
        private int m_address = 0;
        private String m_statusName = "";
        private String m_sPcts = "";
        private int m_pctA = -1;
        private int m_pctB = -1;
        private int m_pctC = -1;
        private int m_flow = -1;
        private volatile boolean m_quit = false;
        private Map<String, Object> state = new TreeMap<String, Object>();

        /**
         * @param rate Number of milliseconds between status queries.
         * @param accessor The CORBA accessor for the status requests.
         * @param address The GPIB address of the module .
         * @param statusName What to call this module in the status string.
         */
        Status9012Thread(int rate, Pump9012 accessor,
                         int address, String statusName) {
            m_rate = rate;
            //m_accessor = accessor;
            m_address = address > MAX_GPIB_ADDRESS ? 0 : Math.max(0, address);
            m_statusName = statusName;
            if (m_address == 0) {
                Messages.postDebug("LcCorbaClient.Status9012Thread.<init>: "
                                   + "Invalid GPIB pump address: " + address);
            }
        }

        public Map<String, Object> getPumpState() {
            return state;
        }

        public void setQuit(boolean quit) {
            m_quit = quit;
        }

        public void setRate(int rate) {
            m_rate = rate;
        }

        public synchronized void run() {
            boolean firstTime = true;
            int methodState = -1;
            int pumpState = -1;
            int pressure = -1;
            int runtime = -1;
            int fileStatus = -1;
            int[] fstatus = null;
            int[] status = null;
            LcMsg.postDebug("gpibStatus",
                            "LcCorbaClient.Status9012Thread.run()");

            if (m_address == 0) {
                return;         // Can't deal with this
            }

            while (!m_quit) {
                try {
                    Thread.sleep(m_rate);
                } catch (InterruptedException ie) {
                    // This is OK
                }                    
                Pump9012 accessor = (Pump9012)getAccessor(V9012_PUMP);
                if (accessor == null) {
                    continue;   // Try again later
                }
                boolean change = false;
                try {
                    LcMsg.postDebug("gpibStatus",
                                    "LcCorbaClient.Status9012Thread: "
                                    + "get status");
                    status = accessor.getLatestStatus(m_address);
                    if (DebugOutput.isSetFor("pumpStatus+")) {
                        Messages.postDebug("Pump Status:");
                        for (int i = 0; i < status.length; i++) {
                            System.out.println(i + "\t: " + status[i]);
                        }
                    }
                    if (status == null) {
                        Messages.postDebug("LcCorbaClient.Status9012Thread.run:"
                                           + " Pump status not available");
                    } else if (status.length < 16) {
                        if (status.length == 0) {
                            int updatesPerMinute = (60 * 1000) / m_rate;
                            try {
                                accessor.requestStatus(m_address,
                                                       updatesPerMinute);
                            } catch (InvalidValueException ive) {
                                Messages.postDebug("Status9012Thread.run: "
                                                   + "Bad status rate value:"
                                                   + updatesPerMinute);
                                m_rate = 1000;
                            }
                        } else {
                            Messages.postDebug("LcCorbaClient.Status9012Thread:"
                                               + " Short pump status: "
                                               + status.length + " words");
                        }
                    } else if (status.length > 27) {
                        Messages.postDebug("LcCorbaClient.Status9012Thread.run:"
                                           + " Long pump status: "
                                           + status.length + " words");
                    } else {
                        // Pump ID
                        if (firstTime) {
                            m_expPanel.processStatusData("pumpId 230 / 9012");
                            firstTime = false;
                        }

                        // Read run time
                        if (runtime != status[19]) {
                            runtime = status[19];
                            String msg = m_statusName + "Time "
                                    + Fmt.f(2, runtime / 100.0) + " min";
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);
                        }

                        // Read pressure
                        if (pressure != status[6]) {
                            pressure = status[6];
                            String msg = m_statusName + "Press "
                                    + pressure + " atm";
                            m_expPanel.processStatusData(msg);
                            state.put("pressure", new Double(pressure));
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);
                        }

                        // Read flow rate
                        if (m_flow != status[5] || pumpState != status[3]) {
                            m_flow = status[5];
                            pumpState = status[3];
                            String msg = m_statusName + "Flow ";
                            if (pumpState == 0) {
                                msg += "Stopped";
                            } else {
                                msg += (m_flow / 1000.0) + " mL/min";
                            }
                            m_expPanel.processStatusData(msg);
                            state.put("flow", new Integer(m_flow));
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);

                            // Send message for pump status
                            msg = m_statusName + "Pump ";
                            if (pumpState == 0) {
                                msg += "Stopped";
                            } else if (pumpState == 1) {
                                msg += "Running";
                            } else if (pumpState == 2) {
                                msg += "Purging";
                            } else if (pumpState == 3) {
                                msg += "Priming";
                            }
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);

                            change = true;
                        }

                        // Read percents
                        if (m_pctA != status[7]) {
                            m_pctA = status[7];
                            String msg = m_statusName + "PctA "
                                    + m_pctA + " %A";
                            m_expPanel.processStatusData(msg);
                            state.put("percentA", new Double(m_pctA));
                            change = true;
                        }
                        if (m_pctB != status[8]) {
                            m_pctB = status[8];
                            String msg = m_statusName + "PctB "
                                    + m_pctB + " %B";
                            m_expPanel.processStatusData(msg);
                            state.put("percentB", new Double(m_pctB));
                            change = true;
                        }
                        if (m_pctC != status[9]) {
                            m_pctC = status[9];
                            String msg = m_statusName + "PctC "
                                    + m_pctC + " %C";
                            m_expPanel.processStatusData(msg);
                            state.put("percentC", new Double(m_pctC));
                            change = true;
                        }
                            
                        String sPcts = (m_statusName + "Pct "
                                        + m_pctA + "%A, "
                                        + m_pctB + "%B, "
                                        + m_pctC + "%C"
                                        + "    "
                                        + (m_flow / 1000.0) + " mL/min"
                                        + "    "
                                        + pressure + " atm");
                        if (!sPcts.equals(m_sPcts)) {
                            m_sPcts = sPcts;
                            m_expPanel.processStatusData(sPcts);
                        }
                        //Messages.postDebug("Pcts=" + m_sPcts);

                        if (methodState != status[15]) {
                            methodState = status[15];
                            state.put("state", new Integer(methodState));
                            change = true;
                            String strMethodState;
                            switch (methodState) {
                              case PUMP_OFF:
                                strMethodState = "Pump_Off";
                                break;
                              case PUMP_READY:
                                strMethodState = "Ready";
                                break;
                              case PUMP_RUNNING:
                                strMethodState = "Running";
                                break;
                              case PUMP_STOPPED:
                                strMethodState = "Stopped";
                                break;
                              case PUMP_NOT_READY:
                                strMethodState = "Not_Ready'";
                                break;
                              case PUMP_EQUILIBRATING:
                                strMethodState = "Equilibrate";
                                break;
                              case PUMP_WAITING:
                                strMethodState = "Waiting";
                                break;
                              default:
                                strMethodState = "State=" + methodState;
                                break;
                            }
                            String msg = m_statusName
                                    + "Status " + strMethodState;
                            m_expPanel.processStatusData(msg);
                            Messages.postDebug("gpibStatus",
                                               "gpibStatus: " + msg);
                        }
                    }
                    if (change) {
                        notifyPumpListeners(state, -1, null);
                    }

                    // Check the file status (result of method download)
                    fstatus = accessor.getFileStatus(m_address);
                    if (fstatus == null) {
                        Messages.postDebug("LcCorbaClient.Status9012Thread.run:"
                                           + " Pump File Status not available");
                    } else if (fstatus.length != 2) {
                        Messages.postDebug("LcCorbaClient.Status9012Thread.run:"
                                           + " Pump File Status wrong length: "
                                           + fstatus.length + " words");
                    } else {
                        if (fileStatus != fstatus[1]) {
                            fileStatus = fstatus[1];
                            String msg = getFileStatusMsg(fileStatus);
                            if (msg != null) {
                                notifyPumpListeners(state, fileStatus, msg);
                            }
                        }
                    }
                    corbaOK(IDX_9012);
                } catch (InvalidAddressException iae) {
                    corbaError(IDX_9012, ERROR_BAD_ADDRESS, "");
                    invalidateAccessor(V9012_PUMP, ERROR_BAD_ADDRESS);
                } catch (org.omg.CORBA.SystemException se) {
                    corbaError(IDX_9012, ERROR_COMM_FAILURE, "");
                    invalidateAccessor(V9012_PUMP, ERROR_COMM_FAILURE);
                }
                //Thread.sleep(m_rate);
            }

            // Reset status to "unknown"?
        }
    }

    public class Detector extends UvDetector {

        /** GPIB address of detector. */
        private int mm_gpibAddress = -1;

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
            LcGpibModule[] ids =
                    (LcGpibModule[])m_moduleTable.toArray(new LcGpibModule[0]);
            for (int i = 0; i < ids.length; i++) {
                if (ids[i].label.indexOf(name) >= 0) {
                    mm_gpibAddress = ids[i].address;
                    mm_name = name;
                    break;
                }
            }
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
            int rate = period <= 0 ? 0 : 60000 / period;
            rate = Math.min(rate, 100); // Max rate is 100 / minute
            String cmd = "setStatusRate " + rate;
            return sendMessage(mm_name, mm_gpibAddress, cmd);
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
            return mm_gpibAddress > 0;
        }

        /** Turn on the lamp(s). */
        public boolean lampOn() {
            return sendMessage(mm_name, mm_gpibAddress, "lampOn");
        }

        /** Turn off the lamp(s). */
        public boolean lampOff() {
            return sendMessage(mm_name, mm_gpibAddress, "lampOff");
        }

        /** Make the current absorption the zero reference. */
        public boolean autoZero() {
            return sendMessage(mm_name, mm_gpibAddress, "autozero");
        }

        /** Download a (constant) run method with maximum run time. */
        public boolean downloadMethod(LcMethod params) {
            // "parms" arg is not used, as "sendMessage" grabs the
            // methodEditor on its own.
            return sendMessage(mm_name, mm_gpibAddress, "downloadMethod");
        }

        public boolean isDownloaded(LcMethod params) {
            if (m_uvMethod == null) {
                return false;
            }
            MethodLine[] aMethod = get9050Method(params);
            return isEqualMethod(aMethod, m_uvMethod);
        }

        /**
         * Start a run, if possible.
         */
        public boolean start() {
            return sendMessage(mm_name, mm_gpibAddress, "start");
        }

        /**
         * Stop a run and save data, continue monitoring selected
         * wavelength(s).
         */
        public boolean stop() {
            return sendMessage(mm_name, mm_gpibAddress, "stop");
        }

        /** Stop a run in such a way that it can be resumed. */
        public boolean pause() {
            return sendMessage(mm_name, mm_gpibAddress, "stop");
        }

        /** Restart a paused run. */
        public boolean restart() {
            return sendMessage(mm_name, mm_gpibAddress, "start");
        }

        /** Reset the detector state to be ready to start a new run. */
        public boolean reset() {
            return sendMessage(mm_name, mm_gpibAddress, "reset");
        }

        /**
         * Selects who deals with the data.
         * @return Returns false, as data
         * must be retrieved through the analog-output.
         */
        public boolean setDataListener(LcDataListener listener) {
            return false;
        }
        
        /**
         * Selects channels and wavelengths to monitor for the chromatogram.
         * @return Returns "false", as data is only available through
         * the analog output.
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
         * @return Returns "9050".
         */
        public String getCanonicalIdString() {
            return "9050";
        }

        /**
         * Get the label shown to the user to identify the detector type.
         * @return Returns "335 PDA".
         */
        public String getIdString() {
            return "310/9050 UV-Vis";
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
         * @return Returns false, since this detector cannot return spectra.
         */
        public boolean isPda() {
            return false;
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

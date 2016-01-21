/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.util.*;
import java.io.*;

import org.omg.CORBA.ORB;
import lcaccess.*;
import vnmr.ui.*;


/**
 * This class holds a panel used for editing an LC run method.
 */
public abstract class CorbaClient {

    /**
     * The CORBA orb only ever needs to be made once because it
     * doesn't depend on anything.
     */
    static protected ORB m_corbaOrb = null;

    /**
     * A thread to check communication with the CORBA server.
     * Only one of these, however many devices we have a CORBA
     * connection to.
     */
    static protected StatusCorbaAccessThread m_accessStatusThread = null;

    /** Whether we have a connection to the CORBA server */
    static protected boolean m_corbaOK = false;

    /** Accessors for the various modules are cached here. */
    static protected HashMap<String, Object> m_accessorTable
        = new HashMap<String, Object>();

    /** This table gives the current error status on the accessors. */
    static protected HashMap<String, Integer> m_accessorStateTable
        = new HashMap<String, Integer>();

    /** Handle for the ExpPanel is needed to send H/W bar status. */
    protected ExpPanel m_expPanel = null;

    /** The prefix for the reference string file name. */
    protected String m_refPrefix = "";

    /** Keep track of last error message so we don't repeat ourself */
    //private int failureType = 0;


    /** How long to silently wait for CORBA server to connect, in ms. */
    static protected final int CORBA_CONNECT_TIMEOUT = 1000;

    static protected final int ACCESS_STATUS_OK = 0;
    static protected final int ACCESS_STATUS_NO_CONTACT = 1;

    static protected final int ERROR_UNKNOWN = -1;
    static protected final int ERROR_OK = 0;
    static protected final int NO_REF_FILE = 1;
    static protected final int CANT_OPEN_REF_FILE = 2;
    static protected final int UNREADABLE_REF_FILE = 3;
    static protected final int BAD_REF_STRING = 4;
    static protected final int CANT_NARROW_CORBA_OBJ = 5;
    static protected final int ERROR_COMM_FAILURE = 6;
    static protected final int ERROR_NO_ACCESSOR = 7;
    static protected final int ERROR_BAD_ADDRESS = 8;
    static protected final int ERROR_BAD_METHOD = 9;
    static protected final int ERROR_MISC_FAILURE = 10;

    static public final String GPIB_MODULES = "Access";

    /** Interval between status queries (ms) */
    static private final int ACCESS_STATUS_RATE = 5000;

    private int m_corbaErrorType = 0;


    /**
     * Construct a "dead" version, just so that we can call methods
     * that could be static, if you could specify static methods
     * in an interface.
     */
    public CorbaClient() {
    }

    /**
     * Constructor sets the ButtonIF and starts up the status thread.
     */
    public CorbaClient(ButtonIF vif) {
        m_expPanel = (ExpPanel)vif;
        if (m_accessStatusThread == null) {
            m_accessStatusThread
                    = new StatusCorbaAccessThread(ACCESS_STATUS_RATE);
            m_accessStatusThread.start();
        }
    }

    /**
     * Override this in a subclass if there is something to be done
     * when a new GPIB accessor is obtained.
     */
    public void mainAccessorChanged() {
    }

    protected void corbaOK() {
        if (m_corbaErrorType != ERROR_OK) {
            Messages.postInfo("... basic CORBA connection is OK.");
            m_corbaErrorType = ERROR_OK;
            mainAccessorChanged();
        }
    }

    protected void corbaError(int type, String msg) {
        if (m_corbaErrorType != type) {
            m_corbaErrorType = type;
            String sType = "";
            switch (type) {
            case ERROR_COMM_FAILURE:
                sType = "Communication failure";
                break;
            case ERROR_MISC_FAILURE:
                sType = "Unknown failure";
                break;
            }
            Messages.postError("Basic CORBA connection: " + sType + " "
                               + msg + " ...");
        }
    }

    /**
     * Get the CORBA orb.
     * After the first time, just return m_corbaOrb.
     */
    protected ORB getOrb() {
        if (m_corbaOrb == null) {
            m_corbaOrb = ORB.init((String[])null, (Properties)null);
            if (m_corbaOrb == null) {
                Messages.postDebug("LcCorbaClient.getOrb: "
                                   + "Cannot initialize CORBA ORB");
            }
        }
        return m_corbaOrb;
    }

    protected ORB sendOrb(){
        return m_corbaOrb;
    }

    /**
     * Get the object for calling methods for a given module.
     * This method must get the correct "helper" to narrow
     * the "corbaObj" into a particular accessor.
     * @param refName The base name of the reference string for
     * the module, e.g. "9050".
     * @param corbaObj The CORBA Object for the reference string.
     * @return The object containing methods to call, or null on error.
     */
    abstract protected Object getAccessor(String refName,
                                                  org.omg.CORBA.Object object);

    /**
     * Check if we have contact with the CORBA server.
     * @param status The previous access status: ACCESS_STATUS_OK
     * or ACCESS_STATUS_NO_CONTACT.
     * @return The current access status.
     */
    abstract protected int checkAccessStatus(int status);

    /**
     * Get the object for calling methods for a given module.
     * @param refName The base name of the reference string for
     * the module, e.g. "9050".
     * @return The object containing methods to call, or null on
     * error.
     */
    synchronized protected Object getAccessor(String refName) {
        boolean newAccessor = false;
        ORB orb = getOrb();
        if (orb == null) {
            Messages.postDebug("LcCorbaClient.getAccessor(): "
                               + "Does not have a CORBA ORB");
            return null;
        }
        Object accessor = m_accessorTable.get(refName);
        int failureType = ERROR_OK;
        try {
            failureType = m_accessorStateTable.get(refName);
        } catch (Exception e) {
        }
        boolean ok = true;
        if (failureType != ERROR_OK) {
            // We failed last time we tried to get an accessor
            //failureType = ((Integer)accessor).intValue();
            /*Messages.postDebug("accessor \"" + refName
                               + "\": failureType=" + failureType);/*CMP*/
            accessor = null;    // Should be unnecessary
        }
        if (accessor == null) {
            newAccessor = true;
            String abstractFilepath = "SYSTEM/ACQQUEUE/"
                    + m_refPrefix + refName + ".CORBAref";
            Messages.postDebug("CorbaInit", "CorbaClient.getAccessor: "
                               + "find ref file: " + abstractFilepath);
            String filepath = FileUtil.openPath(abstractFilepath);
            if (filepath == null) {
                if (failureType != NO_REF_FILE) {
                    Messages.postDebug("No CORBA reference file: "
                                       + abstractFilepath);
                    //Messages.writeStackTrace(new Exception("DEBUG"));
                }
                failureType = NO_REF_FILE;
                ok = false;
            }
            BufferedReader reader = null;
            if (ok) {
                Messages.postDebug("CorbaInit", "CorbaClient.getAccessor: "
                                   + "open ref file");
                try {
                    reader = new BufferedReader
                            (new FileReader(filepath));
                } catch (FileNotFoundException fnfe) {
                    if (failureType != CANT_OPEN_REF_FILE) {
                        Messages.postDebug("Cannot open CORBA ref string file: "
                                           + filepath);
                    }
                    failureType = CANT_OPEN_REF_FILE;
                    ok = false;
                }
            }
            String refString = null;
            if (ok) {
                Messages.postDebug("CorbaInit", "CorbaClient.getAccessor: "
                                   + "read ref file");
                try {
                    refString = reader.readLine();
                } catch (IOException ioe) {
                    if (failureType != UNREADABLE_REF_FILE) {
                        Messages.postDebug("Cannot read CORBA ref string file: "
                                           + filepath);
                        try {
                            reader.close();
                        } catch (IOException ioe2) {}
                    }
                    failureType = UNREADABLE_REF_FILE;
                    ok = false;
                }
            }

            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException ioe) {}
            }

            if (ok) {
                org.omg.CORBA.Object corbaObj = null;
                Messages.postDebug("CorbaInit", "CorbaClient.getAccessor: "
                                   + "Convert ref string to CORBA.Object");
                try {
                    corbaObj = orb.string_to_object(refString);
                } catch (org.omg.CORBA.BAD_PARAM cbp) {
                } catch (org.omg.CORBA.SystemException cse) {
                }
                if (corbaObj == null) {
                    if (failureType != BAD_REF_STRING) {
                        Messages.postDebug("Cannot create CORBA Object for "
                                           + refName + " from reference string");
                    }
                    failureType = BAD_REF_STRING;
                    ok = false;
                } else {
                    Messages.postDebug("CorbaInit", "CorbaClient.getAccessor: "
                                       + "Get accessor from CORBA.Object");
                    accessor = getAccessor(refName, corbaObj);
                }
                if (accessor == null) {
                    if (failureType != CANT_NARROW_CORBA_OBJ) {
                        Messages.postDebug("Null CORBA accessor created for "
                                           + refName + " by narrowing");
                    }
                    failureType = CANT_NARROW_CORBA_OBJ;
                    ok = false;
                }
            }
            if (ok) {
                Messages.postDebug("CorbaInit", "CorbaClient.getAccessor: "
                                   + "... CORBA Object for " + refName + " OK");
            }
        }

        Messages.postDebug("CorbaInit",
                           "... CORBA Object for " + refName + " OK");

        //if (accessor == null) {
            /*Messages.postDebug("save failureType = " + failureType);/*CMP*/
            //m_accessorStateTable.put(refName, failureType);
            //invalidateAccessor(refName);
            //} else {
            m_accessorStateTable.put(refName, failureType);
            m_accessorTable.put(refName, accessor);
            //}
        //if (accessor instanceof Integer) {
        //    return null;
        //} else {
            return accessor;
        //}
    }

    /**
     * Get the current error state of an accessor.
     */
    public int getAccessorState(String refName) {
        int state = ERROR_UNKNOWN;
        try {
            state = m_accessorStateTable.get(refName);
        } catch (Exception e) {
        }
        return state;
    }

    /**
     * Remove cached version of accessor for a given module.
     */
    protected void invalidateAccessor(String refName, int errorType) {
        m_accessorTable.remove(refName);
        m_accessorStateTable.put(refName, errorType);
        //if (!(m_accessorTable.get(refName) instanceof Integer)) {
        //    m_accessorTable.remove(refName);
        //}
        //m_badAccessorTable.put(refName, m_accessorTable.remove(refName));
    }

    class StatusCorbaAccessThread extends Thread {
        private int m_rate = 0;
        private volatile boolean m_quit = false;
        private int status = ACCESS_STATUS_OK;

        /**
         * @param rate Number of milliseconds between status queries.
         */
        StatusCorbaAccessThread(int rate) {
            m_rate = rate;
            setName("CorbaStatusPoller");
        }

        public void setQuit(boolean quit) {
            m_quit = quit;
        }

        public void setRate(int rate) {
            m_rate = rate;
        }

        public synchronized void run() {
            /*
             * In future, will get ref strings for all modules from
             * this.  For now, just do something to see if we can
             * communicate.
             */
            while (!m_quit) {
                status = checkAccessStatus(status);
                try {
                    Thread.sleep(m_rate);
                } catch (InterruptedException ie) {
                    // This is OK
                }
            }
        }
    }


}

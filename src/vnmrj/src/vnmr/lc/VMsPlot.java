/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import java.awt.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import msaccess.*;

/**
 * This VObj object displays a plot of a mass spectrum.
 */
public class VMsPlot extends VPlot {

    private MsCorbaClient m_corbaClient = null;
    private boolean firsttime = true;
    private MsUpdateThread updateThread = null;
    private double m_firstMass = -1;
    private double m_lastMass = -1;
    private boolean m_lastScanNull = false;
    private boolean m_isActive = false;


    public VMsPlot(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);

        try {
            GridBagConstraints gbc;
            String refString = null;
            String refFile = "msacc.ref";
            int[] rawData;

            org.omg.CORBA.Object obj = null;        
            String[] args = null;

            m_plot.setXAxis(true);
            m_plot.setXLabel("Mass / Charge");
            m_plot.setYAxis(true);
            m_plot.setYLabel("Intensity");
            //m_plot.setGrid(false);
            m_plot.setImpulses(true);
        
        } catch (Exception e) {
            Messages.postError("Error creating Mass Spec plot");
            Messages.writeStackTrace(e);
        }
    }

    public void setActive(boolean b) {
        m_isActive = b;
    }

    public boolean isActive() {
        return m_isActive;
    }

    public void plotMassSpectrum(MsCorbaClient corbaClient,
                                 int timeMilliseconds,
                                 String filename, String filename2,
                                 int tDisplayed_ms) {
        if (filename == null) {
            return;
        }
        filename = filename.replace('/', '\\');
        int[] rawData = null;
        double minutes = -2;
        int index = -2;

        boolean ok = false;
        try {
            ok = corbaClient.openMsFile(filename);
        } catch (org.omg.CORBA.SystemException se) {
            Messages.postDebug("plotMassSpectrum: cannot open file \""
                               + filename + "\": " + se);
        } catch (Exception e) {
            Messages.postError("plotMassSpectrum() failed: " + e);
            Messages.writeStackTrace(e);
            return;
        }
        if (!ok && filename2 != null) {
            Messages.postDebug("Failed to open PC file: " + filename + " ...");
            // Try filename2
            filename2 = filename2.replace('/', '\\');
            Messages.postDebug("... trying: " + filename2);
            try {
                ok = corbaClient.openMsFile(filename2);
            } catch (org.omg.CORBA.SystemException se) {
                Messages.postDebug("plotMassSpectrum: cannot open file \""
                                   + filename2 + "\": " + se);
            } catch (Exception e) {
                Messages.postError("plotMassSpectrum() failed: " + e);
                Messages.writeStackTrace(e);
                return;
            }
        }
        if (!ok) {
            Messages.postDebug("Failed to open PC file: " + filename);
            if (filename2 != null) {
                Messages.postDebug("... and file: " + filename2);
            }
            return;
        }
        // CORBA commands below will refer to file opened above

        try {
            // TODO: Use MSWorkstation API to get closest index
            index = corbaClient.getClosestIndex(timeMilliseconds);
            minutes = corbaClient.getTimeOfScan(index);
            Messages.postDebug("MsData", "plotMassSpectrum: t="
                               + Fmt.f(2, timeMilliseconds / 60000.0)
                               + ", index=" + index
                               + ", tscan=" + Fmt.f(2, minutes)
                               + ", tdelta="
                               + Fmt.f(2, (tDisplayed_ms - timeMilliseconds)
                                       / 60000.0));
            rawData = corbaClient.getData(index);
        } catch (org.omg.CORBA.SystemException se) {
            Messages.postDebug("plotMassSpectrum: cannot get data \""
                               + filename + "\": " + se);
        } catch (Exception e) {
            Messages.postError("plotMassSpectrum() failed: " + e);
            Messages.writeStackTrace(e);
            return;
        }

        if (rawData == null) {
            return;             // Already reported an error
        } else if (rawData.length == 0) {
            Messages.postError("MS data error: zero points received");
            return;
        } else if (rawData.length % 2 != 0) {
            Messages.postError("MS data error: odd number of points ("
                               + rawData.length + ")");
            return;
        }

        drawPlot(rawData, m_plot);
        minutes += (tDisplayed_ms - timeMilliseconds) / 60000.0;
        m_plot.setTitle("Retention Time = " + Fmt.f(2, minutes) + " minutes");
        repaint();
    }

    /**
     * Set the given plot to show the given centroid data.
     * The data is an integer array, with all masses multiplied by 256.
     * The first two elements are first mass and last mass, respectively.
     * The 2n following elements give n mass-intensity pairs.
     * @param data First mass, last mass, mass-intensity pairs.
     * @param plot The Plot in which to display data.
     */
    private void drawPlot(int[] data, Plot plot) {
        int len = data.length;
        if (len < 2 || len % 2 != 0) {
            // Illegal number of elements for data array
            Messages.postDebug("VMsPlot.drawPlot: bad data length: " + len);
            return;
        }

        plot.clear(false);
        for (int i = 2; i < len; i += 2) {
            plot.addPoint(0, data[i] / 256.0, data[i + 1], false);
        }
        double firstMass = data[0] / 256.0;
        double lastMass = data[1] / 256.0;
        boolean badRange = (firstMass < 0
                            || firstMass >= lastMass
                            || lastMass > 1500);
        if (len > 2) {
            // NB: Sometimes the firstMass and lastMass don't get set
            // in the scan.  (If it's just running a default scan?)
            // So we tend to get masses reported from outside the
            // range firstMass to lastMass.  We detect that here,
            // so we can ensure that the "full" plot shows all the data.
            double minMass = data[2] / 256.0;
            double maxMass = data[len - 2] / 256.0;
            if (minMass < firstMass) {
                firstMass = minMass;
                badRange = true;
            }
            if (maxMass > lastMass) {
                lastMass = maxMass;
                badRange = true;
            }
        }
        if (badRange) {
            Messages.postDebug("MsData", "Set auto-range on 'full'");
            // This erases any idea of what the "full" range is;
            // the "full" button will just show all the data.
            plot.setXFillRange(0, 0);
        }
        if (firstMass != m_firstMass || lastMass != m_lastMass) {
            // The mass range has changed since last plot
            m_firstMass = firstMass;
            m_lastMass = lastMass;
            if (badRange) {
                // This turns on auto-ranging immediately
                plot.unsetXRange();
            } else {
                // This sets a fixed range
                plot.setXRange(firstMass, lastMass);
                // This makes the 'full' button snap to this range
                plot.setXFillRange(firstMass, lastMass);
            }
        }
    }

    public void showCurrentMsData(MsCorbaClient corbaClient, Plot plot) {
        if (!isActive()) {
            return;
        }
        plot.clear(false);
        plot.setTitle("Current MS Scan");
        plot.setShowZeroY(true);
        if (firsttime) {
            firsttime = false;
            corbaClient.setScanOn(true);
        }
        Scan scan = corbaClient.getData();
        if (scan == null) {
            if (!m_lastScanNull) {
                Messages.postDebug("Null MS scan returned");
                m_lastScanNull = true;
            }
            return;
        }
        m_lastScanNull = false;
        int len = scan.intensity.length;
        Messages.postDebug("MsData",
                           "MS Data: type=" + scan.spectrumType
                           //+ " (centroid=" + Bear1200.CENTROID + ")"
                           + ", len=" + len
                           + ", 1st mass=" + (scan.firstMass / 256.0)
                           + ", last mass=" + (scan.lastMass / 256.0));
        double firstMass = scan.firstMass / 256.0;
        double lastMass = scan.lastMass / 256.0;
        if (scan.spectrumType == Bear1200.CENTROID) {
            if (len != scan.mass.length) {
                Messages.postDebug("showCurrentMsData: "
                                   + "X and Y arrays are different lengths");
                len = Math.min(len, scan.mass.length);
            }
            for (int i = 0; i < len; i++) {
                Messages.postLog("MsData+",
                                 "mass[" + i + "]=" + (scan.mass[i] / 256.0)
                                 + ", inten=" + scan.intensity[i]);
                plot.addPoint(0, scan.mass[i]/256.0, scan.intensity[i], false);
            }
            if (len > 0) {
                // NB: Sometimes the firstMass and lastMass don't get set
                // in the scan.  (If it's just running a default scan?)
                // So we tend to get masses reported from outside the
                // range firstMass to lastMass.  We detect that here,
                // and make sure that the "full" plot shows the data.
                double minMass = scan.mass[0] / 256.0;
                double maxMass = scan.mass[len - 1] / 256.0;
                boolean badRange = firstMass >= lastMass;
                if (minMass < firstMass) {
                    firstMass = minMass;
                    badRange = true;
                }
                if (maxMass > lastMass) {
                    lastMass = maxMass;
                    badRange = true;
                }
                if (badRange) {
                    Messages.postDebug("MsData", "Set auto-range on 'full'");
                    // This erases any idea of what the "full" range is
                    plot.setXFillRange(0, 0);
                    if (firstMass != m_firstMass || lastMass != m_lastMass) {
                        // This turns on auto-ranging immediately
                        plot.unsetXRange();
                        m_firstMass = firstMass;
                        m_lastMass = lastMass;
                    }
                } else if (firstMass != m_firstMass || lastMass != m_lastMass) {
                    // This sets a fixed range
                    plot.setXRange(firstMass, lastMass);
                    // This makes the 'full' button snap to this range
                    plot.setXFillRange(firstMass, lastMass);
                    m_firstMass = firstMass;
                    m_lastMass = lastMass;
                }
            }
            plot.repaint();
        } else {
            // Ignore
            plot.setTitle("*** Cannot Display Profile Data ***");
            plot.repaint();
        }
    }

    /**
     * Display the current MS data continuously.
     * @param corbaClient Communication connection to the MS.
     * @param delay Time between updates in ms.
     */
    public void showCurrentMsData(MsCorbaClient corbaClient, int delay) {
        m_lastScanNull = false;
        if (updateThread != null) {
            updateThread.setQuit(true);
            updateThread.interrupt();
            updateThread = null;
        }
        if (delay > 0) {
            updateThread = new MsUpdateThread(corbaClient, delay);
            updateThread.start();
        }
    }

    class MsUpdateThread extends Thread {
        private MsCorbaClient m_corbaClient = null;
        private int m_delay = 0;
        private volatile boolean m_quit = false;

        public MsUpdateThread(MsCorbaClient corbaClient, int delay) {
            m_corbaClient = corbaClient;
            m_delay = delay;
            setName("Update MS Graph");
        }

        public void setQuit(boolean quit) {
            m_quit = quit;
        }

        public synchronized void run() {
            while (!m_quit) {
                try {
                    showCurrentMsData(m_corbaClient, m_plot);
                    Thread.sleep(m_delay);
                } catch (InterruptedException ie) {
                    // This is OK
                }
            }
            m_corbaClient.setScanOn(false);
            firsttime = true;
        }
    }
}

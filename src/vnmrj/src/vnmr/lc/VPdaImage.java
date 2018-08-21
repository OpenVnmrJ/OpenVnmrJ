/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import javax.swing.*;
import java.awt.*;
import java.util.*;
import javax.swing.event.MouseInputAdapter;
import java.awt.geom.*;
import java.awt.event.*;
import java.beans.PropertyChangeEvent;
import java.io.*;
import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;


/**
 * This VObj object displays a plot of a PDA spectrum.
 */
public class VPdaImage extends VPlot implements ComponentListener {
    
    /** Constant Defintions */
    private static final int CMAP_SIZE = 2001;
    private static final int DIVIDER_WIDTH = 8;
    private static final String PERSISTENCE_FILE = "PdaImage";
    private static final String VERTICAL_PROPERTY = "VerticalSplit";
    private static final String HORIZONTAL_PROPERTY = "HorizontalSplit";

    private float[] m_time = null;
    private float[] m_lambda = null;
    private float m_timeOffset = 0;
    private ArrayList<float[]> m_spectra = null;
    private int m_height;
    private int m_width;
    private CursedPlot2D m_plot2D;
    private Plot m_xPlot;
    private Plot m_yPlot;
    private VPdaPlot m_vxPlot;
    private VPdaPlot m_vyPlot;
    private String m_dir;
    private long m_lastDataTime = 0;

    protected double m_whiteLevel = 2.5; // Max absorption at 2.5 AU

    private VPdaImage m_graph;

    //private VContainer m_pdaContainer;
    private Container m_pdaContainer;
    private JDialog m_pdaPopUp;
    private int[] m_colormap = new int[CMAP_SIZE];


    /**
     * Main program reads in a binary PDA data file and dumps the contents
     * to the screen in the format of the old ASCII PDA data file.
     * Usage: java vnmr.lc.VPdaImage [filepath]
     * If "filepath" is a directory, the file pdaData.dat is assumed.
     * With no argument, looks for pdaData.dat in the current directory.
     */
    public static void main(String[] args) {
        String path = "pdaData.dat";
        if (args.length > 0) {
            path = args[0];
        }
    
        File file = new File(path);
        if (file.isDirectory()) {
            file = new File(file, "pdaData.dat");
        }
        new VPdaImage(file);
    }

    /**
     * This constructor prints out a binary pdaData.dat file as
     * an ASCII pdaData.pda file.
     */
    public VPdaImage(File file) {
        if (!file.canRead()) {
            System.err.println("Cannot read file: " + file.getPath());
            return;
        }
        getDataBinary(file);
        System.err.println("File: " + file.getPath());
        System.out.println("Wavelengths:");
        for (int i = 0; i < m_width; i++) {
            System.out.print(m_lambda[i] + " ");
        }
        System.out.println();
        System.out.println("Intensity:");
        for (int i = 0; i < m_height; i++) {
            long time_ms = (long)(60000.0 * m_time[i]);
            System.out.print(time_ms + " ");
            float[] spectrum = m_spectra.get(i);
            for (int j = 0; j < m_width; j++) {
                System.out.print(spectrum[j] + " ");
            }
            System.out.println();
        }
    }        

    public VPdaImage(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        LcMsg.postDebug("VPdaImage", "VPdaImage.<init>");
        JSplitPane horzSplitPane = null;
        JSplitPane vertSplitPane = null;
        JPanel bottomPanel = new JPanel();

        makeColormap();

        m_graph = this;
        m_plot2D = new CursedPlot2D(m_colormap);
        m_vxPlot = new VPdaPlot(sshare, vif, typ);
        if(m_vxPlot != null) {
            m_xPlot = m_vxPlot.getPlot();
        }
        m_vyPlot = new VPdaPlot(sshare, vif, typ);
        if(m_vyPlot != null) {
            m_yPlot = m_vyPlot.getPlot();
        }
        Font font = DisplayOptions.getFont("GraphTextFont");
        if (font != null) {
            float size = font.getSize2D();
            Font titleFont = font.deriveFont(Font.BOLD, size);
            this.setAttribute(VObjDef.FONT_STYLE, "GraphTextFont");
            m_plot2D.setLabelFont(font);
            m_plot2D.setTitleFont(titleFont);
            m_vxPlot.setAttribute(VObjDef.FONT_STYLE, "GraphTextFont");
            m_xPlot.setLabelFont(font);
            m_xPlot.setTitleFont(titleFont);
            m_vyPlot.setAttribute(VObjDef.FONT_STYLE, "GraphTextFont");
            m_vyPlot.setAttribute(VObjDef.FONT_NAME, "GraphTextFont");
            m_vyPlot.setAttribute(VObjDef.FONT_SIZE, "GraphTextFont");
            m_vyPlot.setAttribute(VObjDef.FGCOLOR, "GraphTextFont");
            m_yPlot.setLabelFont(font);
            m_yPlot.setTitleFont(titleFont);
        }

        try {
            m_plot2D.setPreferredSize(new Dimension(600, 400));
            m_plot2D.setMinimumSize(new Dimension(0, 0));
            m_plot2D.setFillButton(true);
            //m_plot2D.setFillButton(false); 
            m_plot2D.setOpaque(false);
            DisplayOptions.addChangeListener(this);
             
            m_plot2D.setForeground(Color.BLACK);
            m_plot2D.setXAxis(true);
            m_plot2D.setXLabel("Time (min)");
            m_plot2D.setYAxis(true);
            m_plot2D.setYLabel("Wavelength (nm)");
            m_plot2D.setGrid(false); 
            m_plot2D.setTitle("PDA Spectral Chromatogram");
            m_plot2D.repaint();
        
        } catch (Exception e) {
            LcMsg.postError("Error creating PDA image plot");
            LcMsg.writeStackTrace(e);
        }

        try {
            m_xPlot.setXLabel("Time (min)");
            m_xPlot.setYLabel("Absorption (AU)");
            m_xPlot.setTitle("Chromatogram");
            m_xPlot.setForeground(Color.BLACK);
            m_xPlot.setPreferredSize(new Dimension(300, 150));
            m_xPlot.setMinimumSize(new Dimension(0, 0));
            m_vxPlot.setAttribute(GRAPHFGCOL, "GraphForeground");
            m_vxPlot.setAttribute(TICKCOLOR, "GraphTick");
            m_vxPlot.setAttribute(GRIDCOLOR, "GraphGrid");
            m_vxPlot.setAttribute(GRAPHBGCOL, "GraphBackground");
            m_vxPlot.setAttribute(BGCOLOR, "VJBackground");
            m_yPlot.setXLabel("Wavelength (nm)");
            m_yPlot.setYLabel("Absorption (AU)");
            m_yPlot.setTitle("PDA Spectrum");
            m_yPlot.setForeground(Color.BLACK);
            m_yPlot.setPreferredSize(new Dimension(300, 150));
            m_yPlot.setMinimumSize(new Dimension(0, 0));
            m_vyPlot.setAttribute(GRAPHFGCOL, "GraphForeground");
            m_vyPlot.setAttribute(TICKCOLOR, "GraphTick");
            m_vyPlot.setAttribute(GRIDCOLOR, "GraphGrid");
            m_vyPlot.setAttribute(GRAPHBGCOL, "GraphBackground");
            m_vyPlot.setAttribute(BGCOLOR, "VJBackground");
            
        } catch (Exception e) {
            LcMsg.postError("Error creating PDA plots " + e);
        }
        

        m_pdaPopUp = new VPdaPopUp("PDA 2D Plot");
        m_pdaContainer = new JPanel();
        m_pdaContainer.setLayout(new BorderLayout());
        vertSplitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT,
                                       m_plot2D, bottomPanel);
        vertSplitPane.setName(VERTICAL_PROPERTY);
        vertSplitPane.setUI(new javax.swing.plaf.metal.MetalSplitPaneUI());
        vertSplitPane.setContinuousLayout(true);
        vertSplitPane.setOneTouchExpandable(true);
        vertSplitPane.setDividerSize(DIVIDER_WIDTH);
        vertSplitPane.setResizeWeight(0.75);
        m_pdaContainer.add(vertSplitPane, BorderLayout.CENTER);
        horzSplitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,
                                       m_xPlot, m_yPlot);
        horzSplitPane.setName(HORIZONTAL_PROPERTY);
        horzSplitPane.setUI(new javax.swing.plaf.metal.MetalSplitPaneUI());
        horzSplitPane.setContinuousLayout(true);
        horzSplitPane.setOneTouchExpandable(true);
        horzSplitPane.setDividerSize(DIVIDER_WIDTH);
        horzSplitPane.setResizeWeight(0.5);
        bottomPanel.setLayout(new BorderLayout());
        bottomPanel.setBackground(Color.YELLOW);
        bottomPanel.setOpaque(true);
        bottomPanel.add(horzSplitPane, BorderLayout.CENTER);
        Container basePane = m_pdaPopUp.getContentPane();
        basePane.add(m_pdaContainer, BorderLayout.CENTER);

        vertSplitPane.addPropertyChangeListener(this);
        horzSplitPane.addPropertyChangeListener(this);

        m_pdaPopUp.setMinimumSize(new Dimension(400, 300));
        if (!FileUtil.setGeometryFromPersistence(PERSISTENCE_FILE, m_pdaPopUp)){
            LcMsg.postDebug("VPdaImage",
                            "VPdaImage.<init>: No persistence file");
            m_pdaPopUp.setLocation(new Point(0, 0));
            m_pdaPopUp.setSize(700, 650);
        }
        Dimension size = m_pdaPopUp.getSize();
        Dimension minSize = m_pdaPopUp.getMinimumSize();
        m_pdaPopUp.setSize(Math.max(size.width, minSize.width),
                           Math.max(size.height, minSize.height));
        Messages.postDebug("VPdaImage",
                            "VPdaImage.<init>: popup size="
                           + m_pdaPopUp.getSize());
        
        int location = FileUtil.readIntFromPersistence(PERSISTENCE_FILE,
                                                       VERTICAL_PROPERTY,
                                                       -1);
        if (location < 0) {
            vertSplitPane.setDividerLocation(0.65);
        } else {
            vertSplitPane.setDividerLocation(location);
        }

        location = FileUtil.readIntFromPersistence(PERSISTENCE_FILE,
                                                   HORIZONTAL_PROPERTY,
                                                   -1);
        if (location < 0) {
            horzSplitPane.setDividerLocation(0.5);
        } else {
            horzSplitPane.setDividerLocation(location);
        }

        m_pdaPopUp.addComponentListener(this);
        LcMsg.postDebug("VPdaImage", "VPdaImage.<init>: DONE");
    }

    public void propertyChange(PropertyChangeEvent evt) {
        if (evt.getPropertyName().equals(JSplitPane.DIVIDER_LOCATION_PROPERTY)){
            Component source = (Component)evt.getSource();
            FileUtil.writeValueToPersistence(PERSISTENCE_FILE, // File
                                             source.getName(), // Key
                                             evt.getNewValue()); // Value
        }
    }

    /* ComponetListener interface */
    public void componentHidden(ComponentEvent e) {
    }
    public void componentMoved(ComponentEvent e) {
        writePersistence();
    }
    public void componentResized(ComponentEvent e) {
        writePersistence();
    }
    public void componentShown(ComponentEvent e) {
    }

    private void writePersistence() {
        FileUtil.writeGeometryToPersistence(PERSISTENCE_FILE, m_pdaPopUp);
    }

    /**
     * Fills the array m_colormap with pixel values to represent increasing
     * intensity.
     * This is used to convert intensities into the pixel values passed to
     * m_plot2D.
     * The top of the colormap is the highest intensity.
     */
    private void makeColormap() {
        float h;                // hue

        for (int i = 0; i < m_colormap.length; i++) {
            h = (float)i / (m_colormap.length - 1);
            m_colormap[i] = normalizedIntensityToColor(h);
        }
    }

    /**
     * Given an intensity on the scale 0 to 1, returns an RGB color.
     * The color is packed into an int with B in the low byte,
     * as specified in java.awt.Color.getRGB().
     * @param intensity The normalized intensity.
     * @return The color that represents the given intensity.
     */
    private static int normalizedIntensityToColor(float intensity) {

        // NB: Table of { Intensity, Hue, Saturation, Brightness }
        // (Sorted by intensity normalized to scale 0 to 1)
        // Cycle hue through red->yellow->green->blue->red with intensity
        final float[][] ctab = {{0.00F, 0.00F, 0.00F, 0.05F},
                                {0.20F, 0.00F, 0.60F, 0.90F},
                                {0.40F, 0.17F, 0.60F, 0.90F},
                                {0.47F, 0.50F, 0.60F, 0.90F},
                                {0.70F, 0.66F, 0.60F, 0.90F},
                                {0.80F, 0.90F, 0.60F, 0.90F},
                                {1.00F, 1.00F, 0.00F, 1.00F},
        };

        // Find rous in table bounding the given intensity
        intensity = Math.min(intensity, 1);
        int r1 = 0;
        while (ctab[++r1][0] < intensity);
        int r0 = r1 - 1;

        // Interpolate in table for h, s, and b
        float delta = (intensity - ctab[r0][0]) / (ctab[r1][0] - ctab[r0][0]);
        float h = ctab[r0][1] + (ctab[r1][1] - ctab[r0][1]) * delta;
        float s = ctab[r0][2] + (ctab[r1][2] - ctab[r0][2]) * delta;
        float b = ctab[r0][3] + (ctab[r1][3] - ctab[r0][3]) * delta;

        /*if (Math.abs(intensity % .05) < 0.001) {
            System.out.println("i=" + Fmt.f(3, intensity)
                               + ", h=" + Fmt.f(3, h)
                               + ", s=" + Fmt.f(3, s)
                               + ", b=" + Fmt.f(3, b));
        }/*DBG*/
        return Color.HSBtoRGB(h, s, b);
    }

    public void setDirectory(String directory) {
        m_dir = directory;
    }

    public Plot2D getImage() {
        return m_plot2D;
    }

    public void setOffset(float timeOffset) {
        float diff = timeOffset - m_timeOffset;
        if (m_time != null && diff != 0) {
            int len = m_time.length;
            for (int i = 0; i < len; i++) {
                m_time[i] = m_time[i] + diff;
            }
        }
        m_timeOffset = timeOffset;
    }

    public void showCurrentPdaImage() {

        LcMsg.postDebug("VPdaImage", "VPdaImage.showCurrentPdaImage");
        File dataFile;
        boolean isBinaryData = true;
        if(m_dir != null) {
            dataFile = new File(m_dir, "pdaData.dat");
            if (!dataFile.canRead()) {
                dataFile = new File(m_dir, "pdaData.pda");
                isBinaryData = false;
            }
        } else {
            LcMsg.postError("Error getting data file!");
            dataFile = new File("pdaData.pda");
        }      
        if (!dataFile.canRead()) {
            LcMsg.postError("No 2D PDA data available");
            LcMsg.postDebug("VPdaImage.showCurrentPdaImage: "
                            + "cannot read file: " + m_dir
                            + "/pdaData.[dat|pda]");
            return;
        }
         
        m_plot2D.clear(false);
        String title = new File(m_dir).getName();
        if (title == null || title.length() == 0) {
            title = "Spectral Chromatogram";
        }
        m_plot2D.setTitle(title);
        
        int[] pixels = getData(dataFile, isBinaryData); // Sets m_time, m_lambda
        LcMsg.postDebug("VPdaImage",
                        "VPdaImage.showCurrentPdaImage: getData() returned "
                        + (pixels == null ? "null" : "successfully"));
        // Create the image that is the 2D plot
        m_plot2D.setPlot2D(m_time, m_lambda, pixels);

        int npts = m_lambda.length;
        int len = m_time.length;
        
        float firstLambda = m_lambda[0];
        float lastLambda = m_lambda[npts - 1];
        double yFirst = (double) firstLambda;
        double yLast = (double) lastLambda;

        if (npts >= 2) {
            float diff1 = lastLambda - m_lambda[npts - 2];
            float diff2 = m_lambda[1] - firstLambda;
            if (Math.abs((diff2 - diff1) / diff1) > 0.01) {
                // For now, just a tick every 100 nm
                int lastVal = 100 * (int)Math.round(firstLambda / 100);
                for (int i = 1; i < npts; i++) {
                    int val = 100 * (int)Math.round(m_lambda[i] / 100);
                    if (val != lastVal) {
                        double idx = i - 1;
                        idx += ((val - m_lambda[i - 1])
                                / (m_lambda[i] - m_lambda[i - 1]));
                        double posn = firstLambda;
                        posn += (idx / (npts - 1)) * (yLast - yFirst);
                        m_plot2D.addYTick(Integer.toString(val), posn);
                        lastVal = val;
                    }
                }
            }
        }

        m_plot2D.setYFillRange(yFirst, yLast);
        m_plot2D.setYFullRange(yFirst, yLast);

        double firstTime = m_time[0];
        double lastTime = m_time[len - 1];
        double xFirst = firstTime;
        double xLast = lastTime;
        m_plot2D.setXFillRange(xFirst, xLast);
        m_plot2D.setXFullRange(xFirst, xLast);
        //m_pdaPopUp.setState(Frame.NORMAL);
        m_pdaPopUp.setVisible(true);
        m_pdaPopUp.toFront();
        
        m_plot2D.repaint();
        LcMsg.postDebug("VPdaImage", "VPdaImage.showCurrentPdaImage: DONE");
    }

    private boolean updateData() {
        File dataFile;
        boolean isBinaryData = true;
        if (m_dir != null) {
            dataFile = new File(m_dir, "pdaData.dat");
            if (!dataFile.canRead()) {
                dataFile = new File(m_dir, "pdaData.pda");
                isBinaryData = false;
            }
        } else {
            dataFile = new File("pdaData.pda");
        }

        if (m_lastDataTime != dataFile.lastModified()) {
            // Data needs updating
            // This sets m_time, m_lambda
            int[] pixels = getData(dataFile, isBinaryData);
            m_lastDataTime = dataFile.lastModified();
            return (pixels != null);
        } else {
            return true;
        }
    }

    public float[][]  getPdaSpectrum(double time_min) {
        float[][] rtn = null;
        if (updateData()) {
            rtn = new float[2][];
            rtn[0] = m_lambda;
            int idx = Util.getNearest(m_time, time_min);
            rtn[1] = m_spectra.get(idx);
        }
        return rtn;
    }

    /**
     * @param x 
     * @param y The wavelength of the absorption vs. time trace.
     */
    protected void drawPdaPlots(double x, double y) {
        if (x < m_time[0]) {
            x = m_time[0];
        } else if (x > m_time[m_height - 1]) {
            x = m_time[m_height - 1];
        }
        if (y < m_lambda[0]) {
            y = m_lambda[0];
        } else if (y > m_lambda[m_width - 1]) {
            y = m_lambda[m_width - 1];
        }

        // NB: offsetY points to the spectrum
        int offsetY = Util.getNearest(m_time, x);

        // NB: Wavelength of trace = m_lambda[offsetX]
        int offsetX = Util.getNearest(m_lambda, y);

        m_plot2D.setYLabel("Wavelength= " + m_lambda[offsetX] + " nm");
        m_plot2D.repaint();

        try {
            // This is the plot of intensity vs. time at given lambda
            m_xPlot.setTitle("Chromatogram at " + Fmt.f(0, y) + " nm");
            m_vxPlot.setXArray(m_time);
            float xData[]= new float[m_time.length];
            for(int i=0; i<m_time.length; i++) {
                float[] spectrum = m_spectra.get(i);
                xData[i]= spectrum[offsetX];
            }
            m_vxPlot.setYArray(xData);
        } catch (Exception e) {
            LcMsg.postError("VPdaImage.drawPdaPlots: "
                               + "Error extracting time series: " + e);
        }

        try {
            // This is the plot of intensity vs. lambda at given time
            m_yPlot.setTitle("Spectrum at " + Fmt.f(2, x) + " min");
            m_vyPlot.setXArray(m_lambda);
            float[] spectrum = m_spectra.get(offsetY);
            m_vyPlot.setYArray(spectrum);
        } catch (Exception e) {
            LcMsg.postError("VPdaImage.drawPdaPlots: "
                               + "Error extracting wavelength series: " + e);
        }
    }

    /**
     * Read PDA data from a file.
     * @param file The file to read from.
     * @param isBinary True if the data is in binary format.
     */
    private int[] getData(File file, boolean isBinary) {
        boolean ok;
        if (isBinary) {
            ok = getDataBinary(file);
        } else {
            ok = getDataAscii(file);
        }
        if (ok) {
            m_whiteLevel = getMaxZDatum();
            m_plot2D.setZScaleRange(0, m_whiteLevel);
            return fillPixelArray();
        } else {
            return null;
        }
    }

    /**
     * Fills the arrays m_lambda[], m_time[], and the ArrayList m_spectra.
     * @param file The binary data file.
     * @return The 2-D pixel color data (w x h) packed into a linear array.
     */
    private boolean getDataBinary(File file) {
        // TODO: Use DataInputStream instead of handling byte->data ourself

        LcMsg.postDebug("pdaTiming", "VPdaImage.getDataBinary: starting");

        // Open file and fill intensity array
        m_spectra = new ArrayList<float[]>();
        ArrayList<Float> times = new ArrayList<Float>();
        BufferedInputStream in = null;
        try {
            try {
                LcMsg.postDebug("PdaTiming", "openPdaDataFile(): " + file);
                in = new BufferedInputStream(new FileInputStream(file));
            } catch (FileNotFoundException fnfe) {
                // Should not happen. (Should have been checked already.)
                LcMsg.postError("Cannot read PDA Data file: "
                                   + file.getPath());
                return false;
            }
            LcMsg.postDebug("PdaTiming", "VPdaImage.getDataBinary: "
                               + "opened data file");

            //Read and convert data from the file
            // Read the "header line"
            int nfile = in.available();
            if (nfile < 4) {
                LcMsg.postError("PDA data file way too small ("
                                   + in.available() + " bytes)"
                                   + ", path=" + file.getPath());
                return false;
            }
            byte[] bWidth = new byte[4];
            in.read(bWidth, 0, 4);
            m_width = (bWidth[0] << 24)
                    + ((bWidth[1] & 0xff) << 16)
                    + ((bWidth[2] & 0xff) << 8)
                    + (bWidth[3] & 0xff);
            Messages.postDebug("VPdaImage", "VPdaImage.getDataBinary: "
                               + "number of lambdas=" + m_width
                               + ", bytes available=" + in.available());
            if (in.available() < m_width * 4) {
                LcMsg.postError("PDA data file too small for " + m_width
                                   + " wavelengths"
                                   + ", path=" + file.getPath());
                return false;
            }
            m_lambda = new float[m_width];
            byte[] bLambda = new byte[m_width * 4];
            in.read(bLambda, 0, m_width * 4);
            int k = 0;
            for (int j = 0; j < m_width; j++) {
                int iLambda = (bLambda[k++] << 24)
                        + ((bLambda[k++] & 0xff) << 16)
                        + ((bLambda[k++] & 0xff) << 8)
                        + (bLambda[k++] & 0xff);
                m_lambda[j] = Float.intBitsToFloat(iLambda);
            }
            double ndats = (((double)nfile / 4 - 1 - m_width)
                            / (double)(m_width + 1));
            Messages.postDebug("VPdaImage", "VPdaImage.getDataBinary: "
                               + "lambda(min)=" + m_lambda[0]
                               + ", lambda(min)=" + m_lambda[m_width - 1]
                               + ", expected data lines=" + ndats);

            // Read the "data lines"
            byte[] bTime = new byte[4];
            byte[] bData = new byte[m_width * 4];
            for (int i = 0;  true; i++) {
                int available = in.available();
                if (available == 0) {
                    break;      // Successful finish
                } else if (available < (m_width + 1) * 4) {
                    LcMsg.postError("PDA data file wrong length; "
                                       + available
                                       + " bytes in partial last line"
                                       + ", path=" + file.getPath());
                    break;      // Disorderly finish
                }
                in.read(bTime, 0, 4);
                int iTime = (bTime[0] << 24)
                        + ((bTime[1] & 0xff) << 16)
                        + ((bTime[2] & 0xff) << 8)
                        + (bTime[3] & 0xff);
                times.add(Float.intBitsToFloat(iTime));
                float[] intensityData = new float[m_width];
                m_spectra.add(intensityData);
                in.read(bData, 0, m_width * 4);
                k = 0;
                for(int j = 0; j < m_width; j++) {
                    int iData = (bData[k++] << 24)
                            + ((bData[k++] & 0xff) << 16)
                            + ((bData[k++] & 0xff) << 8)
                            + (bData[k++] & 0xff);
                    intensityData[j] = Float.intBitsToFloat(iData);
                }
            }
        } catch (IOException ioe) {
            LcMsg.postError("VPdaImage.getDataBinary: error reading data: "
                               + ioe);
            return false;
        } catch (Exception e) {
            LcMsg.postError("VPdaImage.getDataBinary: " + e);
            LcMsg.writeStackTrace(e);
        }
        try {
            in.close();
        } catch (IOException ioe) {
        }
        m_height = m_spectra.size();
        m_time = new float[m_height];
        for (int i = 0; i < m_height; i++) {
            m_time[i] = m_timeOffset + times.get(i);
        }
        LcMsg.postDebug("pdaTiming",
                           "VPdaImage.getDataBinary: intensities done");

        return true;
    }

    /**
     * Fills the arrays m_lambda[], m_time[], and the ArrayList m_spectra.
     * @param dataFile The ASCII data file.
     * @return The 2-D pixel color data (w x h) packed into a linear array.
     */
    private boolean getDataAscii(File dataFile) {
        // TODO: Obsolete this some day (all new data is written in binary)

        LcMsg.postDebug("pdaTiming", "VPdaImage.getDataAscii: starting");

        // Open file and fill intensity array
        m_spectra = new ArrayList<float[]>();
        ArrayList<Float> times = new ArrayList<Float>();
        try {
             
            BufferedReader in = new BufferedReader(new FileReader(dataFile));
            //Read and convert data from the file
            in.readLine();      // Skip the "Wavelengths" label
            String line = in.readLine().trim();
            String[] lambdaData = line.split(" ");
            m_width = lambdaData.length;
            m_lambda = new float[m_width];
            for(int j = 0; j < lambdaData.length; j++) {
                m_lambda[j] = Float.parseFloat(lambdaData[j]);
            }
            in.readLine();      // Skip the "Intensity" label

            // Read the data lines
            boolean errMsgPrinted = false;
            for (int i = 0;  (line = in.readLine()) != null; i++) {
                float[] intensityData = new float[m_width];
                m_spectra.add(intensityData);
                StringTokenizer toker = new StringTokenizer(line);
                long timeL = Integer.parseInt(toker.nextToken());
                times.add((float)(timeL / 60000.0));
                //m_time[i] = (float)(timeL / 60000.0);
                for(int j = 0; j < m_width; j++) {
                    if (toker.hasMoreTokens()) {
                        intensityData[j] = Float.parseFloat(toker.nextToken());
                    } else {
                        intensityData[j] = 0;
                        if (!errMsgPrinted) {
                            errMsgPrinted = true;
                            LcMsg.postError("Bad pdaData file. Expect " + m_width
                                            + " data pts per line. Line "
                                            + (i + 4) + " has " + j);
                        }
                    }
                }
            }
            in.close();
        } catch (Exception e) {
            LcMsg.postError("VPdaImage.getDataAscii: " + e);
            LcMsg.writeStackTrace(e);
        }
        m_height = m_spectra.size();
        m_time = new float[m_height];
        for (int i = 0; i < m_height; i++) {
            m_time[i] = m_timeOffset + times.get(i);
        }
        LcMsg.postDebug("pdaTiming",
                           "VPdaImage.getDataAscii: intensities done");

        return true;
    }

    private int[] fillPixelArray() {
        int[] pixData = new int[m_width * m_height];
        float slope = (float)(1 / m_whiteLevel);
        for(int y = 0; y < m_height; y++) {
            int p = y + m_height * (m_width - 1);
            float[] intensityData = m_spectra.get(y);
            for (int x = 0; x < m_width; x++, p -= m_height) {
                double cindex = intensityData[x] * slope;
                cindex = Math.max(0, Math.min(1, cindex));
                int nIdx = (int)(0.5 + cindex * (m_colormap.length - 1));
                pixData[p] = m_colormap[nIdx];
                
                //pixData[y+m_height*(m_width-x-1)]= tempData[x+m_width*y];
            }
        }
        LcMsg.postDebug("pdaTiming",
                           "VPdaImage.fillPixelArray: pixels done");
        return pixData;
    }

    public double getValueAt(double time, double lambda) {
        int timeIndex = Util.getNearest(m_time, time);
        int lambdaIndex = Util.getNearest(m_lambda, lambda);
        float[] spectrum = m_spectra.get(timeIndex);
        double z = spectrum[lambdaIndex];
        return z;
    }
    
    private void drawPlot(int[] data, Plot2D plotImg) {
        int nPts = m_width;

        plotImg.clear(false);

        /*for (int i = 0; i < nPts; i++) {
          plot.addPoint(0, (double) data[2 * i] / 256.0,
          (double) data[2 * i + 1], false);
          }*/
        //plot.addArray(0, (double) data[], (double) data[], false);
        
    }

    protected void rescalePlotIntensities(double zMin, double zMax) {
        m_whiteLevel = zMax;
        int[] pixels = fillPixelArray();
        m_plot2D.setTopScale(m_whiteLevel);
        m_plot2D.setPlot2D(m_time, m_lambda, pixels);
        m_plot2D.repaint();
    }

    protected double getMaxZDatum() {
        float zmax = m_spectra.get(0)[0];
        for(int y = 0; y < m_height; y++) {
            float[] intensityData = m_spectra.get(y);
            for (int x = 0; x < m_width; x++) {
                if (zmax < intensityData[x]) {
                    zmax = intensityData[x];
                }
            }
        }
        return zmax;
    }    

    protected class VPdaPopUp extends ModelessDialog implements ActionListener {

        public VPdaPopUp(String title) {
            super(title);
            undoButton.setVisible(false);
            historyButton.setVisible(false);
            abandonButton.setVisible(false);
            helpButton.setVisible(false);
            setBgColor(Util.getBgColor());
            closeButton.setActionCommand("Close");
            closeButton.addActionListener(this);            
        }

        public void actionPerformed(ActionEvent ae) {
            String cmd = ae.getActionCommand();

            if (cmd == "Close") {
                this.setVisible(false);
            }
        }

    }


    protected class CursedPlot2D extends Plot2D {
        protected Point2D.Double leftPoint;
        protected Point2D.Double rightPoint;
        protected MouseMotionListener ptMouseMotionListener = null;

        public CursedPlot2D(int[] scale) {
            super(scale);
            init(true);
        }

        public CursedPlot2D(int[] scale, boolean installListeners) {
            super(scale);
            init(installListeners);
        }

        protected void init(boolean installListeners) {
            setBackground(Util.getBgColor());
            leftPoint = new Point2D.Double();
            rightPoint = new Point2D.Double();
            removeMouseListeners();

            if (installListeners) {
                MouseInputListener mouseListener = new MouseInputListener();
                addMouseMotionListener(mouseListener);
                addMouseListener(mouseListener);
            }
            m_bLeftCursorOn=true; //SDL fix, should use setShowValue from xml
            m_bTopCursorOn=true;
        }

        protected void removeMouseListeners() {
            MouseMotionListener[] list = (MouseMotionListener[])
                    (getListeners(MouseMotionListener.class));
            if (list.length > 0) {
                ptMouseMotionListener = list[0];
                removeMouseMotionListener(ptMouseMotionListener);
            }
            MouseListener[] list2 = (MouseListener[])
                    (getListeners(MouseListener.class));
            if (list2.length > 0) {
                removeMouseListener(list2[0]);
            }
        }

        /**
         * Draw the cursors on the plot.
         * First calls super._drawPlot() to draw the rest of the plot.
         */
        protected synchronized void _drawPlot(Graphics graphics,
                                              boolean clearfirst,
                                              Rectangle drawRectangle) {
            super._drawPlot(graphics, clearfirst, drawRectangle);

            // Draw cursors
            if (m_bLeftCursorOn) {
                int leftPix = _ulx + (int)((leftPoint.x  - _xMin) * _xscale);
                if (leftPix > _ulx && leftPix < _lrx) {
                    graphics.setColor(m_lcColor);
                    graphics.drawLine(leftPix, _uly + 1, leftPix, _lry - 1);
                }
            }
            if (m_bRightCursorOn) {
                int rightPix = _ulx + (int)((rightPoint.x  - _xMin) * _xscale);
                if (rightPix > _ulx && rightPix < _lrx) {
                    graphics.setColor(m_rcColor);
                    graphics.drawLine(rightPix, _uly + 1, rightPix, _lry - 1);
                }
            }
            if(m_bTopCursorOn) {
                int topPix = _uly + (int)((leftPoint.y  - _yMin) * _yscale);
                if (topPix > _uly && topPix < _lry) {
                    graphics.setColor(m_lcColor);
                    graphics.drawLine(_ulx + 1, topPix, _lrx - 1, topPix);
                }
            }
        }

        public void moveCursor(int nCursor, String strValue) {
            try {
                double val = Double.parseDouble(strValue);
                moveCursor(nCursor, val);
            } catch (NumberFormatException nfe) {}
        }

        public void moveCursor(int nCursor, double val) {
            double left = leftPoint.x;
            double right = rightPoint.x;
            if (nCursor == LEFT_CURSOR) {
                leftPoint.x = val;
                m_strLCValue = Double.toString(leftPoint.x);
                if (rightPoint.x < leftPoint.x) {
                    rightPoint.x = leftPoint.x;
                    m_strRCValue = Double.toString(rightPoint.x);
                }
            } else if (nCursor == RIGHT_CURSOR) {
                rightPoint.x = val;
                m_strRCValue = Double.toString(rightPoint.x);
                if (leftPoint.x > rightPoint.x) {
                    leftPoint.x = rightPoint.x;
                    m_strLCValue = Double.toString(leftPoint.x);
                }
            } else {
                return;
            }
            if (left != leftPoint.x || right != rightPoint.x) {
                repaint();
            }
        }

        protected void fillButtonClick() {
            super.fillButtonClick();
            rescalePlotIntensities(0, getMaxZDatum());
        }

        protected class MouseInputListener extends MouseInputAdapter {

            protected static final int ABOVE = 1;
            protected static final int LEFT = 2;
            protected static final int BELOW = 4;
            protected static final int RIGHT = 8;
            protected static final int POSITION = 0xf;
            protected static final int BUTTON1 = 16;
            protected static final int BUTTON2 = 32;
            protected static final int BUTTON3 = 64;
            protected static final int BUTTON = 0x70;

            protected boolean otherCursorChanged;
            protected boolean movingCursors = false;
            protected int cursorType = 0; // LEFT, BELOW, or 0
            protected int actionMode = 0; // OR of LEFT, BELOW, etc.
            protected double cursorXOnData;
            protected double cursorYOnData;
            protected double cursorZOnData;
            protected double xDataRange[];
            protected double yDataRange[];
            protected double zDataRange[];

            public void mouseMoved(MouseEvent e) {
                checkCursorIcon(e);
            }

            private void checkCursorIcon(MouseEvent e) {
                int off = mouseOffCanvas(e);
                //int but = mouseButton(e);
                actionMode = off;
                if (!offsetDragEnabled) {
                    return;
                }
                Cursor curs;
                if (off == LEFT) {
                    /*
                     * This if-else stuff is rather sloppy.
                     * When (say) the left button goes up, we get
                     * isLeftMouseButton() == true, but we want the
                     * S_RESIZE_CURSOR to go off at this point.  It will,
                     * only because we have already set
                     * cursorType == (LEFT | BUTTON1),
                     * so the (cursorType != LEFT) clause gets executed.
                     * If this method got called on drag, it wouldn't work.
                     */
                    /*SDL if (SwingUtilities.isRightMouseButton(e)
                      && cursorType != (LEFT | BUTTON3))
                      {
                      curs = Cursor.getPredefinedCursor
                      (Cursor.N_RESIZE_CURSOR);
                      m_plot.setCursor(curs);
                      cursorType = LEFT | BUTTON3;
                      } else if (SwingUtilities.isLeftMouseButton(e)
                      && cursorType != (LEFT | BUTTON1))
                      {
                      curs = Cursor.getPredefinedCursor
                      (Cursor.S_RESIZE_CURSOR);
                      m_plot.setCursor(curs);
                      cursorType = LEFT | BUTTON1;
                      } else if (cursorType != LEFT) {
                      curs = VCursor.getCursor("verticalMove");
                      m_plot.setCursor(curs);
                      cursorType = LEFT;
                      } SDL */
                    
                } else if (off == BELOW) {
                    /*if (SwingUtilities.isRightMouseButton(e)
                      && cursorType != (BELOW | BUTTON3))
                      {
                      curs = Cursor.getPredefinedCursor
                      (Cursor.E_RESIZE_CURSOR);
                      m_plot.setCursor(curs);
                      cursorType = BELOW | BUTTON3;
                      } else if (SwingUtilities.isLeftMouseButton(e)
                      && cursorType != (BELOW | BUTTON1))
                      {
                      curs = Cursor.getPredefinedCursor
                      (Cursor.W_RESIZE_CURSOR);
                      m_plot.setCursor(curs);
                      cursorType = BELOW | BUTTON1;
                      } else if (cursorType != BELOW) {
                      curs = VCursor.getCursor("horizontalMove");
                      m_plot.setCursor(curs);
                      cursorType = BELOW;
                      }*/
                } else if (off == RIGHT) {
                      curs = VCursor.getCursor("northResize");
                      m_plot2D.setCursor(curs);
                      cursorType = RIGHT;
                } else if (cursorType != 0) {
                    curs = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
                    m_plot2D.setCursor(curs);
                    cursorType = 0;
                }
                   
            }

            public void mousePressed(MouseEvent e) {
                checkCursorIcon(e);
                if (actionMode == 0) {
                    otherCursorChanged = false;
                    mousePosition(e, MOUSE_PRESS);
                    sendCursorCmds(e, MOUSE_PRESS);
                } else if (actionMode == LEFT) {
                    setYOffset(e);
                } else if (actionMode == BELOW) {
                    setXOffset(e);
                } else if (actionMode == RIGHT) {
                    setZOffset(e);
                }
            }

            private void setXOffset(MouseEvent e) {
                cursorXOnData = xPixToData(e.getX());
                xDataRange = m_plot.getXRange();
            }

            private void setYOffset(MouseEvent e) {
                cursorYOnData = yPixToData(e.getY());
                yDataRange = m_plot.getYRange();
            }

            private void setZOffset(MouseEvent e) {
                cursorZOnData = zPixToData(e.getY());
                zDataRange = getZRange();
            }

            public void mouseDragged(MouseEvent e) {
                
                if (actionMode == 0) {
                    mousePosition(e, MOUSE_DRAG);
                    sendCursorCmds(e, MOUSE_DRAG);

                    double my = yPixToData(e.getY());
                    int xOffset = (int)((my - m_lambda[0]) * (m_lambda.length)
                                        / (m_lambda[m_lambda.length - 1]
                                           - m_lambda[0]));
                    if (xOffset < 0) {
                        xOffset= 0;
                    } else if(xOffset >= m_lambda.length) {
                        xOffset= m_lambda.length-1;
                    }
                    m_plot2D.setYLabel("Wavelength= " + m_lambda[xOffset]
                                       + " nm");
                    m_plot2D.repaint();
                } else if (actionMode == LEFT) {
                    moveYOffset(e);
                } else if (actionMode == BELOW) {
                    moveXOffset(e);
                } else if (actionMode == RIGHT) {
                    moveZOffset(e);
                }
            }

            private void moveXOffset(MouseEvent e) {
                if (xDataRange == null) {
                    LcMsg.postDebug("moveXOffset(): xDataRange=null");
                    return;
                }
                double x = xPixToData(e.getX());
                double r[] = getXRange();
                if (SwingUtilities.isRightMouseButton(e)) {
                    double xmax = r[0] + (cursorXOnData - r[0])
                            * (r[1] - r[0]) / (x - r[0]);
                    double factor = (xmax - r[0]) / (xDataRange[1] - r[0]);
                    if (x < r[0] || factor > 10) {
                        xmax = r[0] + 10 * (xDataRange[1] - r[0]);
                    } else if (factor < 0.1) {
                        xmax = r[0] + 0.1 * (xDataRange[1] - r[0]);
                    }
                    if (cursorXOnData > r[0]) {
                        //m_plot.setXRange(r[0], xmax);
                    }
                } else if (SwingUtilities.isLeftMouseButton(e)) {
                    double xmin = r[1] - (r[1] - cursorXOnData)
                            * (r[1] - r[0]) / (r[1] - x);
                    double factor = (r[1] - xmin) / (r[1] - xDataRange[0]);
                    if (x > r[1] || factor > 10) {
                        xmin = r[1] - 10 * (r[1] - xDataRange[0]);
                    } else if (factor < 0.1) {
                        xmin = r[1] - 0.1 * (r[1] - xDataRange[0]);
                    }
                    if (cursorXOnData < r[1]) {
                        //m_plot.setXRange(xmin, r[1]);
                    }
                } else {
                    x += xDataRange[0] - r[0];
                    double delta = x - cursorXOnData;
                    //m_plot.setXRange(xDataRange[0] - delta,
                    //                 xDataRange[1] - delta);
                }
                m_plot.repaint();
            }

            private void moveYOffset(MouseEvent e) {
                if (yDataRange == null) {
                    LcMsg.postDebug("moveYOffset(): yDataRange=null");
                    return;
                }
                double y = yPixToData(e.getY());
                double r[] = getYRange();
                if (SwingUtilities.isRightMouseButton(e)) {
                    double ymax = r[0] + (cursorYOnData - r[0])
                            * (r[1] - r[0]) / (y - r[0]);
                    double factor = (ymax - r[0]) / (yDataRange[1] - r[0]);
                    if (y < r[0] || factor > 10) {
                        ymax = r[0] + 10 * (yDataRange[1] - r[0]);
                    } else if (factor < 0.1) {
                        ymax = r[0] + 0.1 * (yDataRange[1] - r[0]);
                    }
                    if (cursorYOnData > r[0]) {
                        //m_plot.setYRange(r[0], ymax);
                    }
                } else if (SwingUtilities.isLeftMouseButton(e)) {
                    double ymin = r[1] - (r[1] - cursorYOnData)
                            * (r[1] - r[0]) / (r[1] - y);
                    double factor = (r[1] - ymin) / (r[1] - yDataRange[0]);
                    if (y > r[1] || factor > 10) {
                        ymin = r[1] - 10 * (r[1] - yDataRange[0]);
                    } else if (factor < 0.1) {
                        ymin = r[1] - 0.1 * (r[1] - yDataRange[0]);
                    }
                    if (cursorYOnData < r[1]) {
                        //m_plot.setYRange(ymin, r[1]);
                    }
                } else {
                    y += yDataRange[0] - r[0];
                    double delta = y - cursorYOnData;
                    //m_plot.setYRange(yDataRange[0] - delta,
                    //yDataRange[1] - delta);
                }
                m_plot.repaint();
            }

            public void moveZOffset(MouseEvent e) {
                if (zDataRange == null) {
                    LcMsg.postDebug("moveZOffset(): zDataRange=null");
                    return;
                }
                double z = zPixToData(e.getY());
                double[] r = getZRange();
                if (SwingUtilities.isLeftMouseButton(e)
                    && cursorZOnData > r[0])
                {
                    // Calculate new top of range
                    double zmax = r[0] + (cursorZOnData - r[0])
                            * (r[1] - r[0]) / (z - r[0]);
                    double factor = (zmax - r[0]) / (zDataRange[1] - r[0]);
                    if (z < r[0] || factor > 10) {
                        zmax = r[0] + 10 * (zDataRange[1] - r[0]);
                    } else if (factor < 0.1) {
                        zmax = r[0] + 0.1 * (zDataRange[1] - r[0]);
                    }
                    m_plot2D.setZScaleRange(r[0], zmax);
                }
            }
     
            public void mouseReleased(MouseEvent e) {
                if (actionMode == 0) {
                    sendCursorCmds(e, MOUSE_RELEASE);
                    otherCursorChanged = false;
                } else if (actionMode == RIGHT) {
                    rescalePlotIntensities(0, m_plot2D.getZRange()[1]);
                }
                checkCursorIcon(e);
                
                /*double mx = xPixToData(e.getX());
                int yOffset = (int) (mx*(m_time.length)/m_time[m_time.length-1]);
                if (yOffset < 0) {
                    yOffset= 0;
                } else if(yOffset >= m_time.length) {
                    yOffset= m_time.length-1;
                }
                
                double my = yPixToData(e.getY());
                int xOffset = (int) ( (my-m_lambda[0])*(m_lambda.length)/(m_lambda[m_lambda.length-1]-m_lambda[0]));
                if (xOffset < 0) {
                    xOffset= 0;
                } else if(xOffset >= m_lambda.length) {
                    xOffset= m_lambda.length-1;
                }

                m_plot2D.setYLabel("Wavelength= " + m_lambda[xOffset] + " nm");
                m_plot2D.repaint();
                drawPdaPlots(xOffset, yOffset);*/
            }


            public void mouseClicked(MouseEvent me) {
                if (me.getClickCount() == 2
                    && (SwingUtilities.isLeftMouseButton(me)
                        || SwingUtilities.isRightMouseButton(me)))
                {
                    int off = mouseOffCanvas(me);
                    if (off == 0) {
                        expand();
                    }
                    repaint();
                }
            }

            private void expand() {
                //setXRange(leftPoint.x, rightPoint.x);
            }

            protected void mousePosition(MouseEvent e, int nMouseAction) {
                Point2D.Double point;
                if (SwingUtilities.isLeftMouseButton(e)) {
                    movePointToMouse(e, leftPoint);
                    m_strLCValue = Double.toString(leftPoint.x);
                    if (rightPoint.x < leftPoint.x) {
                        rightPoint.x = leftPoint.x;
                        m_strRCValue = Double.toString(rightPoint.x);
                        otherCursorChanged = true;
                    }
                    /*
                } else if (SwingUtilities.isRightMouseButton(e)) {
                    movePointToMouse(e, rightPoint);
                    m_strRCValue = Double.toString(rightPoint.x);
                    if (leftPoint.x > rightPoint.x) {
                        leftPoint.x = rightPoint.x;
                        m_strLCValue = Double.toString(leftPoint.x);
                        otherCursorChanged = true;
                    }
                    */
                } else if (SwingUtilities.isMiddleMouseButton(e)) {
                    double ydat = 0;
                    double ydatMax = 0;
                    if (nMouseAction == MOUSE_PRESS
                        || nMouseAction == MOUSE_DRAG)
                    {
                        double x = xPixToData(e.getX());
                        double y = yPixToData(e.getY());
                        double z = getValueAt(x, y);
                        if (z > 0) {
                            m_whiteLevel = (float)z;
                        } else {
                            m_whiteLevel /= 2;
                        }
                        int[] pixels = fillPixelArray();
                        m_plot2D.setTopScale(m_whiteLevel);
                        m_plot2D.setPlot2D(m_time, m_lambda, pixels);
                    }
                } else {
                    return;
                }
                repaint();
            }

            protected void movePointToMouse(MouseEvent e,
                                            Point2D.Double point) {
                int x = e.getX();
                int y = e.getY();
                if (x <= _ulx) {
                    x = _ulx;
                } else if (x >= _lrx) {
                    x = _lrx;
                }

                // Convert from pixel to user coordinates.
                point.x = _xMin + (x + 0.5 - _ulx) / _xscale;
                point.y = _yMin + (y + 0.5 - _uly) / _yscale;
            }

            /**
             * Returns a bitmapped integer indicating where the
             * MouseEvent is relative to the canvas.
             * Returns 0 if the event is on the canvas, otherwise
             * returns a bitwise OR of the following values:
             * ABOVE, LEFT, BELOW, RIGHT.
             */
            protected int mouseOffCanvas(MouseEvent e) {
                int rtn = 0;
                int x = e.getX();
                int y = e.getY();

                if (x < _ulx) {
                    rtn |= LEFT;
                } else if (isInScaleRect(x, y)) {
                    rtn |= RIGHT;
                }
                if (y < _uly) {
                    rtn |= ABOVE;
                } else if (y > _lry) {
                    rtn |= BELOW;
                }
                return rtn;
            }

            /**
             * Send the appropriate command to Vnmr for a mouse event.
             * Changing one cursor may cause a command to be sent for
             * the other cursor also.  The logic is rather ad hoc; it
             * is designed for VnmrCmds that follow certain rules.
             * The MOUSE_PRESS command should activate the cursor if
             * it is not already active, The MOUSE_RELEASE command
             * should be equivalent to MOUSE_PRESS except for not
             * changing the active status.  If MOUSE_DRAG is defined,
             * it should also not change the active status.  The
             * "other" cursor never gets sent a MOUSE_PRESS, so its
             * active status should not change, but it should change
             * position to be consistent with the first cursor.
             */
            public void sendCursorCmds(MouseEvent e, int nMouseAction) {
                if (SwingUtilities.isLeftMouseButton(e)) {
                    m_objMin.sendVnmrCmd(nMouseAction);
                    drawPdaPlots(xPixToData(e.getX()),
                                 yPixToData(e.getY()));
                    if (otherCursorChanged) {
                        int cmd = nMouseAction;
                        if (cmd == MOUSE_PRESS) {
                            cmd = MOUSE_RELEASE;
                        } else if (cmd == MOUSE_RELEASE) {
                            otherCursorChanged = false;
                        }
                        m_objMax.sendVnmrCmd(cmd);
                    }
                } else if (SwingUtilities.isRightMouseButton(e)) {
                    m_objMax.sendVnmrCmd(nMouseAction);
                    if (otherCursorChanged) {
                        int cmd = nMouseAction;
                        if (cmd == MOUSE_PRESS) {
                            cmd = MOUSE_RELEASE;
                        } else if (cmd == MOUSE_RELEASE) {
                            otherCursorChanged = false;
                        }
                        m_objMin.sendVnmrCmd(cmd);
                    }
                }
            }

        }
    }
}

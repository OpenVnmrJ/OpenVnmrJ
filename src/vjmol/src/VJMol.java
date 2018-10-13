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

package vjmol;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.awt.image.*;
import javax.swing.plaf.basic.*;
import javax.swing.border.*;
import javax.swing.table.*;
import org.openscience.jmol.*;
import org.openscience.jmol.io.*;
import org.openscience.jmol.app.*;
import org.openscience.jmol.render.*;
import Acme.JPM.Encoders.*;
import com.obrador.*;
import com.lowagie.text.Document;
import com.lowagie.text.pdf.*;


/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class VJMol extends JSplitPane implements ActionListener
{

    protected Rectangle rectClip = new Rectangle();
    protected DisplayControl displayControl;
    protected MeasurementList measurementList;
    protected VJMolPanel m_vjmolPanel;
    protected JTextField m_txfFile;
    protected VJMolMeasure m_measure;
    protected JComboBox m_cmbAtomLabels;
    protected JCheckBox m_chkHydrogens;
    protected JCheckBox m_chkVectors;
    protected JCheckBox m_chkMeasurements;
    protected JCheckBox m_chkAntiAlias;
    protected JComboBox m_cmbView;
    protected JTable m_table;
    protected JScrollPane m_scrollPane;
    protected JButton m_btnMeasure;
    protected JPanel m_pnlMeasure;
    protected JButton m_btnVJmol;
    protected JPanel m_pnlVJmolBtn;
    protected JFileChooser m_fileBrowser;
    protected JComboBox m_cmbMode;
    protected JTextField m_txfImage;
    protected JComboBox m_cmbFormat;
    protected String m_strPath;
    protected String m_strSettings;
    protected String m_strImageDir;
    protected String m_strMeasure;
    protected Color m_bgColor;
    protected String m_strDefaultLookandfeel;
    protected TitledBorder m_border = new TitledBorder("");
    protected String[] m_aStrAtomLabels = {"None", "Atomic Symbols", "Atom Types",
                                           "Atom Numbers"};
    protected String[] m_aStrView = {"Front", "Top", "Bottom", "Right", "Left"};
    protected String[] m_aStrMeasure = {"Measure Distance", "Measure Angle",
                                        "Measure Dihedral"};
    protected String[] m_aStrFormat = {"BMP", "JPG", "PPM", "PNG", "PDF"};


    public VJMol(String strPath, String strSettings, String strImageDir)
    {
        this(strPath, strSettings, strImageDir, false);
    }

    public VJMol(String strPath, String strSettings, String strImageDir, boolean bShow)
    {
        String strJvmVersion = System.getProperty("java.version");
        m_vjmolPanel = new VJMolPanel();
        displayControl = new DisplayControl(strJvmVersion, m_vjmolPanel);
        m_vjmolPanel.start();
        initializeMeasurements();
        m_strDefaultLookandfeel = UIManager.getLookAndFeel().getID();
        setUI(new BasicSplitPaneUI());
        //System.setOut(System.err);

        dolayout(bShow);
        updateSize();
        setOpaque(false);
        setContinuousLayout(true);
        setOneTouchExpandable(true);
        m_strPath = strPath;
        m_strSettings = strSettings;
        m_strImageDir = strImageDir;
        m_bgColor = Color.black;
        m_pnlMeasure.setVisible(false);
        setSettings();
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (cmd == null)
            return;

        if (cmd.equals("hydrogens"))
            setShowHydrogens(!displayControl.getShowHydrogens());
        else if (cmd.equals("vectors"))
            setShowVectors(!displayControl.getShowVectors());
        else if (cmd.equals("measurements"))
            setShowMeasurements(!displayControl.getShowMeasurements());
        else if (cmd.equals("antialias"))
            setAntiAlias(!displayControl.getWantsAntialias());
        else if (cmd.equals("atomlabels"))
            setShowAtomLabels();
        else if (cmd.equals("view"))
            setView();
        else if (cmd.equals("mode"))
            setMode();
        else if (cmd.equals("show"))
            showFilebrowser();
        else if (cmd.equals("distance") || cmd.equals("angle") ||
                 cmd.equals("dihedral"))
            setMeasurement(cmd);
        else if (cmd.equals("measurecmd"))
            m_measure.MeasurePressed();
        else if (cmd.equals("image"))
            saveImage();

        saveSettings();
    }

    protected void dolayout(boolean bShow)
    {
        JPanel pnlMain = new JPanel(new BorderLayout());
        JPanel pnl = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints(0, 0, 1, 1, 0, 0.2,
                                                        GridBagConstraints.NORTHWEST,
                                                        GridBagConstraints.HORIZONTAL,
                                                        new Insets(0,0,0,0), 1, 1);
        if (bShow)
            pnl.add(new JLabel("File:"), gbc);
        m_fileBrowser = new JFileChooser(System.getProperty("user.dir"));
        gbc.gridx = 1;
        gbc.weightx = 0.2;
        JPanel pnl2 = new JPanel(new GridBagLayout());
        if (bShow)
            pnl.add(pnl2, gbc);
        m_txfFile = new JTextField();
        JButton btn = new JButton("Browse...");
        gbc.gridx = 0;
        pnl2.add(m_txfFile, gbc);
        gbc.gridx = 1;
        gbc.weightx = 0;
        pnl2.add(btn, gbc);
        m_txfFile.addActionListener(this);
        btn.addActionListener(this);
        btn.setActionCommand("show");
        btn.setBorder(BorderFactory.createEmptyBorder());

        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.weightx = 0;
        pnl.add(new JLabel("Display:"), gbc);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.2;
        //gbc.fill = GridBagConstraints.NONE;
        pnl2 = new JPanel(new GridBagLayout());
        m_chkHydrogens = new JCheckBox("H");
        pnl2.add(m_chkHydrogens, gbc);
        m_chkHydrogens.addActionListener(this);
        m_chkHydrogens.setActionCommand("hydrogens");
        m_chkHydrogens.setBorder(null);
        m_chkHydrogens.setToolTipText("Show Hydrogens");

        gbc.gridx = 1;
        m_chkAntiAlias = new JCheckBox("Anti-Alias");
        pnl2.add(m_chkAntiAlias, gbc);
        m_chkAntiAlias.addActionListener(this);
        m_chkAntiAlias.setActionCommand("antialias");
        m_chkAntiAlias.setBorder(null);
        m_chkAntiAlias.setToolTipText("Anti-Alias");

        gbc.gridx = 2;
        m_chkMeasurements = new JCheckBox("Measurements");
        pnl2.add(m_chkMeasurements, gbc);
        m_chkMeasurements.addActionListener(this);
        m_chkMeasurements.setActionCommand("measurements");
        m_chkMeasurements.setBorder(null);
        m_chkMeasurements.setToolTipText("Show Measurements");
        gbc.gridx = 1;
        gbc.gridy = 1;
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        pnl.add(pnl2, gbc);

        gbc.gridx = 0;
        gbc.gridy = 2;
        gbc.weightx = 0;
        pnl.add(new JLabel("Atom Type:"), gbc);
        gbc.gridx = 1;
        gbc.weightx = 0.2;
        m_cmbAtomLabels = new JComboBox(m_aStrAtomLabels);
        pnl.add(m_cmbAtomLabels, gbc);
        m_cmbAtomLabels.addActionListener(this);
        m_cmbAtomLabels.setActionCommand("atomlabels");

        gbc.gridx = 0;
        gbc.gridy = 3;
        gbc.weightx = 0;
        pnl.add(new JLabel("View:"), gbc);
        gbc.gridx = 1;
        gbc.weightx = 0.2;
        m_cmbView = new JComboBox(m_aStrView);
        pnl.add(m_cmbView, gbc);
        m_cmbView.addActionListener(this);
        m_cmbView.setActionCommand("view");

        gbc.gridx = 0;
        gbc.gridy = 4;
        gbc.weightx = 0;
        pnl.add(new JLabel("Mode:"), gbc);
        gbc.gridx = 1;
        gbc.weightx = 0.2;
        String[] aStrMode = {"Rotate", "Zoom", "Translate", "Select", "Measure"};
        m_cmbMode = new JComboBox(aStrMode);
        m_cmbMode.addActionListener(this);
        m_cmbMode.setActionCommand("mode");
        pnl.add(m_cmbMode, gbc);

        gbc.gridx = 0;
        gbc.gridy = 5;
        gbc.weightx = 0;
        pnl.add(new JLabel("Save Image:"), gbc);
        pnl2 = new JPanel(new GridBagLayout());
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0.2;
        m_txfImage = new JTextField();
        pnl2.add(m_txfImage, gbc);
        gbc.weightx = 0;
        m_cmbFormat = new JComboBox(m_aStrFormat);
        gbc.gridx = 1;
        pnl2.add(m_cmbFormat, gbc);
        gbc.gridy = 5;
        gbc.weightx = 0.2;
        pnl.add(pnl2, gbc);
        m_txfImage.addActionListener(this);
        m_txfImage.setActionCommand("image");

        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0;
        pnl2 = new JPanel(new GridBagLayout());
        pnl2.add(new JLabel("Measure:"), gbc);
        ButtonGroup btnGroup2 = new ButtonGroup();
        gbc.gridx = 1;
        gbc.gridy = 0;
        gbc.weightx = 0.2;
        //gbc.fill = GridBagConstraints.NONE;
        JRadioButton rdMeasure = new JRadioButton("Distance");
        pnl2.add(rdMeasure, gbc);
        btnGroup2.add(rdMeasure);
        rdMeasure.addActionListener(this);
        rdMeasure.setActionCommand("distance");
        rdMeasure.setBorder(null);
        rdMeasure.setSelected(true);
        gbc.gridx = 2;
        rdMeasure = new JRadioButton("Angle");
        pnl2.add(rdMeasure, gbc);
        btnGroup2.add(rdMeasure);
        rdMeasure.addActionListener(this);
        rdMeasure.setActionCommand("angle");
        rdMeasure.setBorder(null);
        gbc.gridx = 3;
        rdMeasure = new JRadioButton("Dihedral");
        pnl2.add(rdMeasure, gbc);
        btnGroup2.add(rdMeasure);
        rdMeasure.addActionListener(this);
        rdMeasure.setActionCommand("dihedral");
        rdMeasure.setBorder(null);
        gbc.gridx = 1;
        gbc.gridy = 6;
        //gbc.fill = GridBagConstraints.HORIZONTAL;
        m_pnlMeasure = new JPanel(new BorderLayout());
        m_pnlMeasure.add(pnl2, BorderLayout.NORTH);

        m_scrollPane = new JScrollPane(m_table);
        m_table.setPreferredScrollableViewportSize(new Dimension(100, 70));
        m_scrollPane.setBackground(Color.blue);
        m_pnlMeasure.add(m_scrollPane, BorderLayout.CENTER);

        m_pnlVJmolBtn = new JPanel(new BorderLayout());
        m_btnVJmol = new JButton("x");
        m_pnlVJmolBtn.add(m_btnVJmol, BorderLayout.WEST);
        m_pnlVJmolBtn.setBackground(Color.black);
        m_btnVJmol.setBorder(BorderFactory.createEmptyBorder(0, 2, 0, 2));
        JPanel pnl3 = new JPanel(new BorderLayout());
        pnl3.add(m_pnlVJmolBtn, BorderLayout.NORTH);
        pnl3.add(m_vjmolPanel, BorderLayout.CENTER);

        JScrollPane scrollPane2 = new JScrollPane(pnlMain);
        pnlMain.add(pnl, BorderLayout.CENTER);
        pnl2 = new JPanel(new BorderLayout());
        pnl2.add(m_pnlMeasure, BorderLayout.CENTER);
        pnlMain.add(pnl2, BorderLayout.SOUTH);
        setLeftComponent(pnl3);
        setRightComponent(scrollPane2);
    }

    public String showMol(boolean bShow, String strPath)
    {
        setVisible(bShow);
        String strMsg = showMol(strPath);
        return strMsg;
    }

    public String showMol(String strPath)
    {
        String strMsg = null;
        if (strPath != null && !strPath.equals(""))
        {
            m_strPath = strPath;
            if (!strPath.equals(m_txfFile.getText().trim()))
            {
                strMsg = displayControl.openFile(strPath);
                m_txfFile.setText(strPath);
                m_fileBrowser.setCurrentDirectory(new File(strPath));
            }
            updateSize();
        }
        return strMsg;
    }

    public JPanel getVJMolPanel()
    {
        return m_vjmolPanel;
    }

    public JButton getVJmolButton()
    {
        return m_btnVJmol;
    }

    public String getMol()
    {
        String strMol = "";
        String strFile = m_txfFile.getText();
        if (strFile != null && !strFile.equals(""))
        {
            int nIndex = strFile.lastIndexOf(File.separator);
            if (nIndex >= 0 && nIndex+1 < strFile.length())
                strMol = strFile.substring(nIndex+1);
        }
        return strMol;
    }

    protected void initializeMeasurements()
    {
        JFrame frame = new JFrame();
        m_measure = new VJMolMeasure(frame, displayControl);
        displayControl.setMeasureWatcher(m_measure);
        setTable(m_measure);
        measurementList = new MeasurementList(frame, displayControl);
        m_measure.setMeasurementList(measurementList);

        m_measure.setMeasurement("distance");
        m_table.removeColumn(m_table.getColumn("x"));
        m_table.removeColumn(m_table.getColumn("y"));
        m_table.removeColumn(m_table.getColumn("z"));
    }

    protected void setTable(Container container)
    {
        int nCompCount = container.getComponentCount();
        for (int i = 0; i < nCompCount; i++)
        {
            Component comp = container.getComponent(i);
            if (comp instanceof JTable)
                m_table = (JTable)comp;
            else if (comp instanceof Container)
                setTable((Container)comp);
        }
    }

    protected void setShowHydrogens(boolean bShow)
    {
        m_chkHydrogens.setSelected(bShow);
        displayControl.setShowHydrogens(bShow);
    }

    protected void setShowVectors(boolean bShow)
    {
        m_chkVectors.setSelected(bShow);
        displayControl.setShowVectors(bShow);
    }

    protected void setShowMeasurements(boolean bShow)
    {
        m_chkMeasurements.setSelected(bShow);
        displayControl.setShowMeasurements(bShow);
    }

    protected void setAntiAlias(boolean bShow)
    {
        m_chkAntiAlias.setSelected(bShow);
        displayControl.setWantsAntialias(bShow);
    }

    protected void setShowAtomLabels()
    {
        int nType = m_cmbAtomLabels.getSelectedIndex();
        if (nType == 0)
            displayControl.setStyleLabel(DisplayControl.NOLABELS);
        else if (nType == 1)
            displayControl.setStyleLabel(DisplayControl.SYMBOLS);
        else if (nType == 2)
            displayControl.setStyleLabel(DisplayControl.TYPES);
        else if (nType == 3)
            displayControl.setStyleLabel(DisplayControl.NUMBERS);
    }

    protected void setView()
    {
        int nType = m_cmbView.getSelectedIndex();
        if (nType == 0)
            displayControl.rotateFront();
        else if (nType == 1)
            displayControl.rotateToX(90);
        else if (nType == 2)
            displayControl.rotateToX(-90);
        else if (nType == 3)
            displayControl.rotateToY(90);
        else if (nType == 4)
            displayControl.rotateToY(-90);
    }

    protected void setMode()
    {
        String cmd = (String)m_cmbMode.getSelectedItem();
        boolean bShow = false;
        int nMode = DisplayControl.ROTATE;
        boolean bSelection = false;
        if (cmd != null)
        {
            cmd = cmd.toLowerCase();
            if (cmd.equals("measure"))
            {
                bShow = true;
                nMode = DisplayControl.MEASURE;
            }
            else if (cmd.equals("zoom"))
                nMode = DisplayControl.ZOOM;
            else if (cmd.equals("translate"))
                nMode = DisplayControl.XLATE;
            else if (cmd.equals("select"))
            {
                nMode = DisplayControl.PICK;
                bSelection = true;
            }
        }

        m_pnlMeasure.setVisible(bShow);
        displayControl.setModeMouse(nMode);
        displayControl.setSelectionHaloEnabled(bSelection);
    }

    protected void setMeasurement(String cmd)
    {
        AbstractTableModel model = (AbstractTableModel)m_table.getModel();
        model.fireTableDataChanged();
        m_measure.setMeasurement(cmd);
        m_strMeasure = cmd;

        /*if (cmd != null)
        {
            if (cmd.equals("distance"))
                m_btnMeasure.setText(m_aStrMeasure[0]);
            else if (cmd.equals("angle"))
                m_btnMeasure.setText(m_aStrMeasure[1]);
            else if (cmd.equals("dihedral"))
                m_btnMeasure.setText(m_aStrMeasure[2]);
        }*/
    }

    protected void saveImage()
    {
        String strPath = m_txfImage.getText();
        if (strPath == null || strPath.equals(""))
            return;

        StringTokenizer strTok = new StringTokenizer(strPath);
        if (strTok.hasMoreTokens())
            strPath = strTok.nextToken();

        strPath = new StringBuffer().append(m_strImageDir).append(
                  File.separator).append(strPath).toString();
        try
        {
            Image image = m_vjmolPanel.getImage();
            File file = new File(strPath);
            FileOutputStream os = new FileOutputStream(strPath);
            String strFormat = (String)m_cmbFormat.getSelectedItem();

            if (strFormat.equals("JPG")) {
                JpegEncoder jc = new JpegEncoder(image, 100, os);
                jc.Compress();
            } else if (strFormat.equals("PPM")) {
                PpmEncoder pc = new PpmEncoder(image, os);
                pc.encode();
            } else if (strFormat.equals("GIF")) {
                GifEncoder gc = new GifEncoder(image, os, true);
                gc.encode();
            } else if (strFormat.equals("PNG")) {
                PngEncoder png = new PngEncoder(image);
                byte[] pngbytes = png.pngEncode();
                os.write(pngbytes);
            } else if (strFormat.equals("BMP")) {
                BMPFile bmp = new BMPFile();
                bmp.saveBitmap(os, image);
            }
            else if (strFormat.equals("PDF"))
            {
                Document document = new Document();
                PdfWriter writer = PdfWriter.getInstance(document, os);

                document.open();

                int w = m_vjmolPanel.getWidth();
                int h = m_vjmolPanel.getHeight();
                PdfContentByte cb = writer.getDirectContent();
                PdfTemplate tp = cb.createTemplate(w, h);
                Graphics2D g2 = tp.createGraphics(w, h);
                g2.setStroke(new BasicStroke(0.1f));
                tp.setWidth(w);
                tp.setHeight(h);

                m_vjmolPanel.print(g2);
                g2.dispose();
                cb.addTemplate(tp, 72, 720 - h);
                document.close();
            }

            os.flush();
            os.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

    }

    public String saveImage(String strPath, boolean bPrint, boolean bGif)
    {
        if (bGif)
            m_vjmolPanel.saveGifImage(strPath, bPrint);
        else
            strPath = m_vjmolPanel.saveImage(strPath, bPrint);

        return strPath;
    }

    protected void showFilebrowser()
    {
        setLookandfeel("metal");
        SwingUtilities.updateComponentTreeUI(m_fileBrowser);
        int nValue = m_fileBrowser.showOpenDialog(this);
        if (nValue == JFileChooser.APPROVE_OPTION)
        {
            File file = m_fileBrowser.getSelectedFile();
            String strValue = file.getAbsolutePath();
            // set the file name in the entry
            m_txfFile.setText(strValue);
            displayControl.openFile(strValue);
            setView();
            setLookandfeel(m_strDefaultLookandfeel);
        }
        else if (nValue == JFileChooser.CANCEL_OPTION)
        {
            setLookandfeel(m_strDefaultLookandfeel);
        }
    }

    protected void setSettings()
    {
        displayControl.setStyleAtom(DisplayControl.NONE);
        displayControl.setStyleBond(DisplayControl.SHADING);
        displayControl.setShowHydrogens(false);
        displayControl.setModeMouse(DisplayControl.ROTATE);
        if (m_strPath != null)
            displayControl.openFile(m_strPath);
        m_chkAntiAlias.setSelected(true);
        m_chkMeasurements.setSelected(true);

        try
        {
            File file = new File(m_strSettings);
            if (!file.exists())
                return;

            FileInputStream inputstream = new FileInputStream(m_strSettings);

            Properties properties = new Properties();
            properties.load(inputstream);

            String strValue = properties.getProperty("hydrogens");
            if (strValue != null && strValue.equals("true"))
                setShowHydrogens(true);
            else
                setShowHydrogens(false);
            strValue = properties.getProperty("atomlabels");
            if (strValue != null)
            {
                m_cmbAtomLabels.setSelectedItem(strValue);
                setShowAtomLabels();
            }
            strValue = properties.getProperty("measurements");
            if (strValue != null && strValue.equals("true"))
                setShowMeasurements(true);
            else
                setShowMeasurements(false);
            strValue = properties.getProperty("antialias");
            if (strValue != null && strValue.equals("false"))
                setAntiAlias(false);
            else
                setAntiAlias(true);
            /*strValue = properties.getProperty("vectors");
            if (strValue != null && strValue.equals("true"))
                m_chkVectors.doClick();*/
            strValue = properties.getProperty("view");
            if (strValue != null)
            {
                m_cmbView.setSelectedItem(strValue);
                setView();
            }
            strValue = properties.getProperty("mode");
            if (strValue != null)
            {
                m_cmbMode.setSelectedItem(strValue);
                setMode();
            }
            strValue = properties.getProperty("image");
            if (strValue != null)
                m_cmbFormat.setSelectedItem(strValue);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    protected void saveSettings()
    {
        StringBuffer sbData = new StringBuffer();
        sbData.append("hydrogens ").append(
            displayControl.getShowHydrogens()).append("\n");
        sbData.append("atomlabels ").append(
            m_cmbAtomLabels.getSelectedItem()).append("\n");
        sbData.append("measurements ").append(
            displayControl.getShowMeasurements()).append("\n");
        sbData.append("antialias ").append(
            displayControl.getWantsAntialias()).append("\n");
        /*sbData.append("measurementmode ").append(m_strMeasure).append("\n");
        sbData.append("vectors ").append(
            displayControl.getShowVectors()).append("\n");*/
        sbData.append("view ").append(m_cmbView.getSelectedItem()).append("\n");
        sbData.append("mode ").append(m_cmbMode.getSelectedItem()).append("\n");
        sbData.append("image ").append(m_cmbFormat.getSelectedItem()).append("\n");

        try
        {
            BufferedWriter writer = new BufferedWriter(new FileWriter(m_strSettings));
            writer.write(sbData.toString());
            writer.flush();
            writer.close();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void setPref(JComponent comp, Font font, Color color, Color fgColor,
                        Color bgColor, Color bgColor2)
    {
        int nCompCount = comp.getComponentCount();
        for (int i = 0; i < nCompCount; i++)
        {
            Component comp2 = comp.getComponent(i);
            if (comp2 instanceof JComponent)
                setPref((JComponent)comp2, font, color, fgColor, bgColor, bgColor2);
            comp2.setFont(font);
            comp2.setForeground(color);
            if (!comp2.equals(m_pnlVJmolBtn))
                comp2.setBackground(bgColor);
        }
        comp.setFont(font);
        comp.setForeground(color);
        if (comp.equals(m_pnlVJmolBtn))
            comp.setBackground(bgColor2);
        else
            comp.setBackground(bgColor);
        if (comp.equals(this))
        {
            int r = 255 - bgColor2.getRed();
            int g = 255 - bgColor2.getGreen();
            int b = 255 - bgColor2.getBlue();
            if (fgColor == null)
            {
                fgColor = new Color(r, g, b);
                if (r == 255 && g == 255 && b == 255)
                    fgColor = Color.yellow;
            }
            displayControl.setMeasureFont(font);
            displayControl.setLabelFontSize(font.getSize());
            setfgColor(fgColor);
            m_bgColor = bgColor2;
            displayControl.setColorBackground(bgColor2);
        }
    }

    protected void setfgColor(Color fgColor)
    {
        displayControl.setColorAngle(fgColor);
        displayControl.setColorDistance(fgColor);
        displayControl.setColorDihedral(fgColor);
        displayControl.setColorLabel(fgColor);
    }

    protected void setLookandfeel(String lookandfeel)
    {
        try
        {
            // lookandfeel can be motif or metal
            String lf = UIManager.getSystemLookAndFeelClassName();
            UIManager.LookAndFeelInfo[] lists = UIManager.getInstalledLookAndFeels();
            if (lookandfeel != null) {
                lookandfeel = lookandfeel.toLowerCase();
                int nLength = lists.length;
                String look;
                for (int k = 0; k < nLength; k++) {
                    look = lists[k].getName().toLowerCase();
                    if (look.indexOf(lookandfeel) >= 0) {
                        lf = lists[k].getClassName();
                        break;
                    }
                }
            }
            UIManager.setLookAndFeel(lf);
        }
        catch (Exception e)
        {
            System.err.print(e.toString());
        }
    }

    public void updateSize()
    {
        setDividerLocation(0.63);
        m_vjmolPanel.updateSize();
        validate();
        repaint();
    }

    public static void main(String[] args)
    {
        VJMol vjmol = new VJMol("", "mol", ".", true);
        JFrame frame = new JFrame();
        Container container = frame.getContentPane();
        container.add(vjmol);
        frame.pack();
        frame.show();

        frame.addWindowListener(new WindowAdapter()
        {
            public void windowClosing(WindowEvent e)
            {
                System.exit(0);
            }
        });
    }

    class VJMolPanel extends JPanel implements ComponentListener
    {
        protected BufferedImage biScreenBuf;
        protected Graphics2D g2ScreenBuf;
        protected Dimension dimCurrent;

        public VJMolPanel()
        {
            allocScreenBuf(getSize());
            setOpaque(false);
        }

        public void start() {
            addComponentListener(this);
        }

        public void setBackground(Color color)
        {
            super.setBackground(color);
            setVisible(false);
            setVisible(true);
        }

        public void componentHidden(java.awt.event.ComponentEvent e) {
        }

        public void componentMoved(java.awt.event.ComponentEvent e) {
        }

        public void componentResized(java.awt.event.ComponentEvent e) {
            updateSize();
            repaint();
        }

        public void componentShown(java.awt.event.ComponentEvent e) {
            updateSize();
        }

        private void updateSize() {
            dimCurrent = getSize();
            if (dimCurrent.width <= 0)
                dimCurrent.width = 200;
            if (dimCurrent.height <= 0)
                dimCurrent.height = 200;
            allocScreenBuf(dimCurrent);
            if ((dimCurrent.width == 0) || (dimCurrent.height == 0))
                dimCurrent = null;
            displayControl.setScreenDimension(dimCurrent);
            displayControl.scaleFitToScreen();
        }

        private void allocScreenBuf(Dimension dim) {
            if (g2ScreenBuf != null)
                g2ScreenBuf.dispose();
            if (dim.width == 0 || dim.height == 0) {
                g2ScreenBuf = null;
                biScreenBuf = null;
            } else {
                biScreenBuf = new BufferedImage(dimCurrent.width, dimCurrent.height,
                                                BufferedImage.TYPE_INT_ARGB);
                g2ScreenBuf = biScreenBuf.createGraphics();
            }
        }

        protected BufferedImage getImage()
        {
            BufferedImage bufferedImage = new BufferedImage(dimCurrent.width,
                                                            dimCurrent.height,
                                                            BufferedImage.TYPE_INT_ARGB);
            Graphics g = bufferedImage.createGraphics();
            g.setClip(0, 0, dimCurrent.width, dimCurrent.height);
            paint(g);

            return bufferedImage;
        }

        protected String saveImage(String strPath, boolean bPrint)
        {
            String strMol = getMol();
            Color fgColor = displayControl.getColorAngle();
            if (strMol == null || strMol.equals(""))
                return "";

            strPath = new StringBuffer().append(strPath).append(
                       File.separator).append(strMol).toString();
            if (bPrint)
            {
                displayControl.setColorBackground(Color.white);
                strPath = strPath+".plot";
                setfgColor(Color.black);
            }
            else
                strPath = strPath+".display";

            BufferedImage bufferedImage = getImage();

            try
            {
                File file = new File(strPath);
                if (!file.exists())
                {
                    String strFile = file.getParent();
                    if (strFile != null)
                    {
                        file = new File(strFile);
                        if (!file.exists())
                            file.mkdirs();
                    }
                }

                bufferedImage.flush();
                FileOutputStream fileoutputstream = new FileOutputStream(strPath);
                PpmEncoder ppmEncoder = new PpmEncoder(bufferedImage, fileoutputstream);
                ppmEncoder.encode();

                fileoutputstream.flush();
                fileoutputstream.close();
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }

            if (bPrint)
            {
                displayControl.setColorBackground(m_bgColor);
                setfgColor(fgColor);
            }

            return strPath;
        }

        protected void saveGifImage(String strPath, boolean bPrint)
        {
            Image image = Toolkit.getDefaultToolkit().createImage(strPath);
            MediaTracker mediaTracker = new MediaTracker(this);
            mediaTracker.addImage(image, 1);
            try
            {
                mediaTracker.waitForID(1);

                int nColor = m_bgColor.getRGB();
                if (bPrint)
                    nColor = Color.white.getRGB();
                Image newImage = filter(image, nColor);
                FileOutputStream fileoutputstream = new FileOutputStream(strPath);
                GifEncoder gifEncoder = new GifEncoder(newImage, fileoutputstream);
                gifEncoder.encode();
                fileoutputstream.flush();
                fileoutputstream.close();
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }

        public Image filter(Image img, int nColor) {
            int width = img.getWidth(null);
            int height = img.getHeight(null);
            if (width <= 0)
                width = 100;
            if (height <= 0)
                height = 100;

            BufferedImage b = new BufferedImage(width,height,BufferedImage.TYPE_INT_ARGB);
            b.createGraphics().drawImage(img,0,0,null);
            for (int y = 0;y < b.getHeight();y++) {
                for (int x = 0;x < b.getWidth();x++) {
                    if (b.getRGB(x,y) == nColor) {
                        b.setRGB(x,y,0xffffff);
                    }
                }
            }
            return createImage(b.getSource());
        }

        public void paint(Graphics g) {
            g.getClipBounds(rectClip);
            if (g2ScreenBuf != null)
            {
                displayControl.render(g2ScreenBuf, rectClip);
                g.drawImage(biScreenBuf, 0, 0, null);
            }
        }
    }

    class VJMolMeasure extends Measure
    {
        public VJMolMeasure(JFrame frame, DisplayControl control)
        {
            super(frame, control);
        }

        protected void setMeasurement(String cmd)
        {
            if (cmd == null)
                return;
            if (cmd.equals("distance"))
                measure = DISTANCE;
            else if (cmd.equals("angle"))
                measure = ANGLE;
            else if (cmd.equals("dihedral"))
                measure = DIHEDRAL;
            initialize();
        }

        public void firePicked(int measured)
        {
            super.firePicked(measured);
            action = ADD;
            int nLength = 2;
            if (measure == ANGLE)
                nLength = 3;
            else if (measure == DIHEDRAL)
                nLength = 4;

            boolean bShow = true;
            for (int i = 0; i < nLength; i++)
            {
                if (selection[i] < 0)
                    bShow = false;
            }

            if (bShow)
            {
                Vector vecMeasurements = displayControl.getDistanceMeasurements();
                if (nLength == 3)
                    vecMeasurements = displayControl.getAngleMeasurements();
                else if (nLength == 4)
                    vecMeasurements = displayControl.getDihedralMeasurements();

                Enumeration e = vecMeasurements.elements();
                while (e.hasMoreElements()) {
                    MeasurementInterface measurement = (MeasurementInterface) e.nextElement();
                    if (measurement instanceof Distance)
                    {
                        if (((Distance)measurement).sameAs(selection[0], selection[1]))
                        {
                            //measurementList.deleteMatchingDistance(selection[0], selection[1]);
                            //bShow = false;
                            action = DELETE;
                            break;
                        }
                    }
                    else if (measurement instanceof Angle)
                    {
                        if (((Angle)measurement).sameAs(selection[0], selection[1],
                                                        selection[2]))
                        {
                            /*measurementList.deleteMatchingAngle(selection[0],
                                                                selection[1],
                                                                selection[2]);*/
                            //bShow = false;
                            action = DELETE;
                            break;
                        }
                    }
                    else if (measurement instanceof Dihedral)
                    {
                        if (((Dihedral)measurement).sameAs(selection[0], selection[1],
                                                           selection[2], selection[3]))
                        {
                            /*measurementList.deleteMatchingDihedral(selection[0],
                                                                   selection[1],
                                                                   selection[2],
                                                                   selection[3]);*/
                            //bShow = false;
                            action = DELETE;
                            break;
                        }
                    }

                }

            }

            if (bShow)
                m_measure.MeasurePressed();
        }

        private void initialize()
        {
            action = ADD;
            for (int i = 0; i < 4; i++) {
                selection[i] = -1;
            }
            m_table.setRowSelectionInterval(0, 0);
            AbstractTableModel model = (AbstractTableModel)m_table.getModel();
            model.fireTableDataChanged();
            switch (measure) {
                case ANGLE :
                    m_border.setTitle("Pick three atoms to display the angle");
                    break;
                case DIHEDRAL :
                    m_border.setTitle("Pick four atoms to display the dihedral");
                    break;
                default :
                    m_border.setTitle("Pick two atoms to display the distance");
                    break;
            }
            /*if (action == DELETE) {
                mButton.setText(JmolResourceHandler.getInstance()
                                .getString("Measure.deleteLabel"));
            } else {
                mButton.setText(JmolResourceHandler.getInstance()
                                .getString("Measure.addLabel"));
            }*/
            oldMode = displayControl.getModeMouse();
            displayControl.setModeMouse(DisplayControl.MEASURE);
            disableActions();
        }
    }

}

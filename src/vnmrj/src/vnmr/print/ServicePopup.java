/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Copyright 2000-2007 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Sun designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Sun in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 */

package vnmr.print;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dialog;
import java.awt.FlowLayout;
import java.awt.Frame;
import java.awt.GraphicsConfiguration;
import java.awt.GridBagLayout;
import java.awt.GridBagConstraints;
import java.awt.Insets;
import java.awt.KeyboardFocusManager;
import java.awt.Font;
import java.awt.event.*;
import java.awt.Point;
import java.io.FilePermission;
import java.net.URI;
import java.net.URL;
import java.text.DecimalFormat;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.Vector;
import java.util.Hashtable;
import javax.print.*;
import javax.print.attribute.*;
import javax.print.attribute.standard.*;
import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.event.PopupMenuEvent;
import javax.swing.event.PopupMenuListener;
import javax.swing.text.NumberFormatter;
import java.awt.event.KeyEvent;
import java.net.URISyntaxException;

import vnmr.ui.SessionShare;
import vnmr.util.Util;
import vnmr.bo.VColorChooser;
import vnmr.util.SimpleHLayout;

/**
 * A class which implements a cross-platform print dialog.
 *
 */
public class ServicePopup extends JDialog implements ActionListener,
                        VjPrintEventListener {

    private static final Insets panelInsets = new Insets(6, 6, 6, 6);
    private static final Insets compInsets = new Insets(3, 6, 3, 6);
    public  static final String strBundle = "sun.print.resources.serviceui";

    public  static ResourceBundle messageRB;
    private JTabbedPane tpTabs;
    private JButton btnCancel, btnApprove;
    private JButton btnPreview;
    private PrintService[] services;
    private PrintRequestAttributeSet asOriginal;
    private HashPrintRequestAttributeSet asCurrent;
    private PrintService psCurrent = null;
    private DocFlavor docFlavor;
    private JCheckBox cbPrintToFile;
    private VjPrintFileDialog vjPrintDialog;
    private JTextField lineWidth;
    private JTextField spectrumWidth;
    private boolean bPreview;
    private boolean bChangeMenu;
    private int status;
    private int printerNum;
    // @SuppressWarnings("rawtypes")
	private Hashtable<String, Object> hs = null;
    
    private GeneralPanel pnlGeneral;
    private PageSetupPanel pnlPageSetup;
    private AppearancePanel pnlAppearance;

    private GraphicsConfiguration gc;
    private Dialog parentDialog;
    private Frame parentFrame;
    private String printerName;
    private String lblStr;
    private boolean isAWT = false;
    private boolean bUpdating = false;
    private VjPrintEventListener printEventListener;
    private JRadioButton rbMonochrome, rbColor;
    private JRadioButton rbGraphics, rbVnmrj;
    private JComboBox cbName;
    private JComboBox cbSize, cbSource;
    private JComboBox cbFormat;
    private JFormattedTextField leftMargin, rightMargin,
                                topMargin, bottomMargin;
    private JRadioButton rbDraft, rbNormal, rbHigh;
    private VColorChooser bgColorDialog, fgColorDialog;
    private JLabel bgColorLabel, fgColorLabel;
    private JCheckBox cbFitPaper;
    private JCheckBox cbCursorLines;
    private IconRadioButton rbPortrait, rbLandscape,
                            rbRevPortrait, rbRevLandscape;
    private SpinnerNumberModel copiesModel;

    public static final String[] formatLabels = {"PCL", "POSTSCRIPT" };
    public static final String[] formatValues = {"pcl", "ps" };

    static {
        initResource();
    }

    /**
     * Constructor for the "standard" print dialog (containing all relevant
     * tabs)
     */
    public ServicePopup(GraphicsConfiguration gc,
                         int x, int y,
                         PrintService[] services,
                         DocFlavor flavor,
                         PrintRequestAttributeSet attributes,
                         Dialog dialog)
    {
        super(dialog, getMsg("dialog.printtitle"), false, gc);
        this.gc = gc;
        this.parentDialog = dialog;
        this.parentFrame = null;
        initPrintDialog(x, y, services, flavor, attributes);
    }



    /**
     * Constructor for the "standard" print dialog (containing all relevant
     * tabs)
     */
    public ServicePopup(GraphicsConfiguration gc,
                         int x, int y,
                         PrintService[] services,
                         DocFlavor flavor,
                         PrintRequestAttributeSet attributes,
                         Frame frame)
    {
        super(frame, getMsg("dialog.printtitle"), false, gc);
        this.gc = gc;
        this.parentDialog = null;
        this.parentFrame = frame;
        initPrintDialog(x, y, services, flavor, attributes);
    }


    public void setOriginalAttributeSet(PrintRequestAttributeSet attributes)
    {
        asOriginal = attributes;
        // asCurrent = new HashPrintRequestAttributeSet(attributes);
    }


    /**
     * Initialize print dialog.
     */
    void initPrintDialog(int x, int y,
                         PrintService[] services1,
                         DocFlavor flavor,
                         PrintRequestAttributeSet attributes)
    {
        String title = Util.getLabel("_Print_Screen", "Print Screen");
        setTitle(title);
        this.services = services1;
        this.asOriginal = attributes;
        this.asCurrent = new HashPrintRequestAttributeSet(attributes);
        String defName = null;

        // psCurrent = PrintServiceLookup.lookupDefaultPrintService();

        if (psCurrent != null) {
           defName = psCurrent.getName();
        }
        
        // check if default is in the list
        boolean bMatch = false;
        if (services1 != null && services1.length > 0) {
            for (int i = 0; i < services1.length; i++) {
               PrintService ps = services1[i];
               if (ps != null) {
                   if (psCurrent == null) {
                        psCurrent = ps;
                        bMatch = true;
                        break;
                   }
                   if (defName != null) {
                       if (defName.equals(ps.getName())) {
                           bMatch = true;
                           break;
                       }
                   }
               }
            }
        }
        if (!bMatch) {
            if (services1 != null && services1.length > 0)
               psCurrent = services1[0];
        }
        this.docFlavor = flavor;
        SunPageSelection pages =
            (SunPageSelection)attributes.get(SunPageSelection.class);
        if (pages != null) {
            isAWT = true;
        }

        Container c = getContentPane();
        c.setLayout(new BorderLayout());

        tpTabs = new JTabbedPane();
        tpTabs.setBorder(new EmptyBorder(5, 5, 5, 5));

        // String gkey = getMsg("tab.general");
        String gkey = Util.getLabel("_General", "General");
        int gmnemonic = getVKMnemonic("tab.general");
        pnlGeneral = new GeneralPanel();
        tpTabs.add(gkey, pnlGeneral);
        tpTabs.setMnemonicAt(0, gmnemonic);

        // String pkey = getMsg("tab.pagesetup");
        String pkey = Util.getLabel("_Page_Setup", "Page Setup");
        int pmnemonic = getVKMnemonic("tab.pagesetup");
        pnlPageSetup = new PageSetupPanel();
        tpTabs.add(pkey, pnlPageSetup);
        tpTabs.setMnemonicAt(1, pmnemonic);

        // String akey = getMsg("tab.appearance");
        String akey = Util.getLabel("_Appearance", "Appearance");
        int amnemonic = getVKMnemonic("tab.appearance");
        pnlAppearance = new AppearancePanel();
        tpTabs.add(akey, pnlAppearance);
        tpTabs.setMnemonicAt(2, amnemonic);

        c.add(tpTabs, BorderLayout.CENTER);

        updatePrinterList();
        updatePanels();

        JPanel pnlSouth = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 5));
        lblStr = Util.getLabel("_Print", "Print");
        btnApprove = createExitButton(lblStr, this);
        pnlSouth.add(btnApprove);
        getRootPane().setDefaultButton(btnApprove);
        lblStr = Util.getLabel("_Preview", "Preview");
        btnPreview = new JButton(lblStr);
        btnPreview.addActionListener(this);
        pnlSouth.add(btnPreview);
        lblStr = Util.getLabel("blCancel", "Cancel");
        btnCancel = createExitButton(lblStr, this);
        handleEscKey(btnCancel);
        pnlSouth.add(btnCancel);
        c.add(pnlSouth, BorderLayout.SOUTH);
        if (printerNum < 1) {
            if (cbPrintToFile == null || (!cbPrintToFile.isSelected()))
               btnApprove.setEnabled(false);
        }

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
                dispose(VjPrintDef.CANCEL);
            }
        });

        getAccessibleContext().setAccessibleDescription(getMsg("dialog.printtitle"));
        setResizable(false);
        setLocation(x, y);
        // setAlwaysOnTop(true);
        pack();
        setFromSession();
    }


   /**
     * Constructor for the solitary "page setup" dialog
     */
    public ServicePopup(GraphicsConfiguration gc,
                         int x, int y,
                         PrintService ps,
                         DocFlavor flavor,
                         PrintRequestAttributeSet attributes,
                         Dialog dialog)
    {
        // super(dialog, getMsg("dialog.pstitle"), true, gc);
        super(dialog, getMsg("dialog.pstitle"), false, gc);
        this.gc = gc;
        this.parentDialog = dialog;
        this.parentFrame = null;
        initPageDialog(x, y, ps, flavor, attributes);
    }

    /**
     * Constructor for the solitary "page setup" dialog
     */
    public ServicePopup(GraphicsConfiguration gc,
                         int x, int y,
                         PrintService ps,
                         DocFlavor flavor,
                         PrintRequestAttributeSet attributes,
                         Frame frame)
    {
        // super(frame, getMsg("dialog.pstitle"), true, gc);
        super(frame, getMsg("dialog.pstitle"), false, gc);
        this.gc = gc;
        this.parentDialog = null;
        this.parentFrame = frame;
        initPageDialog(x, y, ps, flavor, attributes);
    }


    /**
     * Initialize "page setup" dialog
     */
    void initPageDialog(int x, int y,
                         PrintService ps,
                         DocFlavor flavor,
                         PrintRequestAttributeSet attributes)
    {
        this.docFlavor = flavor;
        this.asOriginal = attributes;
        this.asCurrent = new HashPrintRequestAttributeSet(attributes);
        this.psCurrent = ps;

        Container c = getContentPane();
        c.setLayout(new BorderLayout());

        pnlPageSetup = new PageSetupPanel();
        c.add(pnlPageSetup, BorderLayout.CENTER);

        pnlPageSetup.updateInfo();

        JPanel pnlSouth = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 5));
        lblStr = Util.getLabel("blOk", "Ok");
        btnApprove = createExitButton(lblStr, this);
        pnlSouth.add(btnApprove);
        getRootPane().setDefaultButton(btnApprove);
        lblStr = Util.getLabel("blClear", "Clear");
        btnCancel = createExitButton(lblStr, this);
        handleEscKey(btnCancel);
        pnlSouth.add(btnCancel);
        c.add(pnlSouth, BorderLayout.SOUTH);

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent event) {
                dispose(VjPrintDef.CANCEL);
            }
        });

        getAccessibleContext().setAccessibleDescription(getMsg("dialog.pstitle"));
        setResizable(false);
        setLocation(x, y);
        // setAlwaysOnTop(true);
        pack();
    }

    /**
     * Performs Cancel when Esc key is pressed.
     */
    private void handleEscKey(JButton btnCancel1) {
        Action cancelKeyAction = new AbstractAction() {
            public void actionPerformed(ActionEvent e) {
                dispose(VjPrintDef.CANCEL);
            }
        };
        KeyStroke cancelKeyStroke =
            KeyStroke.getKeyStroke((char)KeyEvent.VK_ESCAPE, 0);
        InputMap inputMap =
            btnCancel1.getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW);
        ActionMap actionMap = btnCancel1.getActionMap();

        if (inputMap != null && actionMap != null) {
            inputMap.put(cancelKeyStroke, "cancel");
            actionMap.put("cancel", cancelKeyAction);
        }
    }


    /**
     * Returns the current status of the dialog (whether the user has selected
     * the "Print" or "Cancel" button)
     */
    public int getStatus() {
        return status;
    }

    /**
     * Returns an AttributeSet based on whether or not the user cancelled the
     * dialog.  If the user selected "Print" we return their new selections,
     * otherwise we return the attributes that were passed in initially.
     */
    public PrintRequestAttributeSet getAttributes() {
        if (status == VjPrintDef.APPROVE) {
            return asCurrent;
        } else {
            return asOriginal;
        }
    }

    /**
     * Returns a PrintService based on whether or not the user cancelled the
     * dialog.  If the user selected "Print" we return the user's selection
     * for the PrintService, otherwise we return null.
     */
    public PrintService getPrintService() {
        if (status == VjPrintDef.APPROVE) {
            return psCurrent;
        } else {
            return null;
        }
    }

    public PrintService getCurrentPrintService() {
        return psCurrent;
    }


    private void updatePrinterList() {
        if (bUpdating)
            return;
        bUpdating = true;
        PrintService[] svrs = VjPrintUtil.lookupPrintServices();
        if (svrs != null)
            services = svrs;
        String[] psnames = VjPrintUtil.lookupPrintNames(svrs);
        if (psnames == null || psnames.length < 1) {
            bUpdating = false;
            return;
        }
        String s = getString(VjPrintDef.PRINTER_NAME);
        bChangeMenu = true;
        cbName.removeAllItems();
        for (int n = 0; n < psnames.length; n++) {
            if (psnames[n] != null)
                cbName.addItem(psnames[n]);
        }
        if (s == null)
            s = VjPrintUtil.getDefaultPrinterName();
        if (s != null) {
            if (VjPrintUtil.setComboxSelectItem(cbName, s))
                 pnlPageSetup.updateInfo();
        }
        printerNum = cbName.getItemCount();
        if (printerNum > 0 && btnApprove != null)
            btnApprove.setEnabled(true);
        bChangeMenu = false;
        bUpdating = false;
    }

    public void showDialog() {
        boolean bUpdate = false;
        bPreview = false;
        if (!isVisible()) {
            bUpdate = true;
            // updatePrinterList();
        }
        setVisible(true);
        toFront();
        if (bUpdate)
            updatePrinterList();
    }


    public void closeDialog() {
        setVisible(false);
        if (vjPrintDialog != null)
            vjPrintDialog.closeDialog();
    }

    private String getFormat() {
        String s = null;
        if (cbFormat != null)
            s = (String)cbFormat.getSelectedItem();
        if (s != null) {
            hs.put(VjPrintDef.PRINT_FORMATLABEL, s);
            for (int n = 0; n < formatLabels.length; n++) {
                if (s.equals(formatLabels[n])) {
                     return formatValues[n];
                }
            }
        }
        s = "ps";
/**
        if (psCurrent == null)
            return s;
        if (psCurrent.isDocFlavorSupported(DocFlavor.INPUT_STREAM.POSTSCRIPT)) 
            return s;
        if (psCurrent.isDocFlavorSupported(DocFlavor.INPUT_STREAM.PCL)) 
            return "pcl";
        if (psCurrent.isDocFlavorSupported(DocFlavor.INPUT_STREAM.PNG)) 
            return "png";
        if (psCurrent.isDocFlavorSupported(DocFlavor.INPUT_STREAM.JPEG)) 
            return "jpg";
        if (psCurrent.isDocFlavorSupported(DocFlavor.INPUT_STREAM.GIF)) 
            return "gif";
        if (psCurrent.isDocFlavorSupported(DocFlavor.INPUT_STREAM.PDF)) 
            return "pdf";
**/
        return s;
    }

    private void notifyListener() {
        if (printEventListener == null)
            return;
        VjPrintEvent e = new VjPrintEvent(this, status);
        printEventListener.printEventPerformed(e);
    }

    /**
     * Sets the current status flag for the dialog and disposes it (thus
     * returning control of the parent frame back to the user)
     */
    public void dispose(int s) {
        status = s;
        saveToSession();
        if (!isVisible())
            return;
        setVisible(false);
        notifyListener();
        // super.dispose();
    }

    public void setPrintEventListener(VjPrintEventListener  e) {
           printEventListener = e;
    }


    public void printEventPerformed(VjPrintEvent e) {
        if (e.getStatus() != VjPrintDef.CANCEL)
            status = VjPrintDef.APPROVE;
        else
            status = VjPrintDef.CANCEL;
        notifyListener();
    }

    public void actionPerformed(ActionEvent e) {
        Object source = e.getSource();
        boolean approved = false;
        KeyboardFocusManager km = KeyboardFocusManager.getCurrentKeyboardFocusManager();
        Component comp = km.getFocusOwner();
        if (comp instanceof JTextField)
            return;
        bPreview = false;
        if (source == btnApprove) {
            approved = true;
            if (pnlGeneral != null) {
                if (pnlGeneral.isPrintToFileRequested()) {
                    saveToSession();
                    showVJFileDialog();
                    setVisible(false);
                    return;
                } else {
                    asCurrent.remove(Destination.class);
                }
            }
        }
        else if (source == btnPreview) {
            approved = true;
            bPreview = true;
            saveToSession();
            if (pnlGeneral != null) {
                 asCurrent.remove(Destination.class);
            }
            status = VjPrintDef.APPROVE;
            notifyListener();
            return;
        }
        dispose(approved ? VjPrintDef.APPROVE : VjPrintDef.CANCEL);
    }

    private void showVJFileDialog() {
        if (Util.isImagingUser()) {
        }
        else {
            if (vjPrintDialog == null) {
                if (parentFrame != null)
                    vjPrintDialog = new VjPrintFileDialog(gc, parentFrame, this);
                else
                    vjPrintDialog = new VjPrintFileDialog(gc, parentDialog, this);
            }
            vjPrintDialog.setPrintEventListener(this);
            vjPrintDialog.setPrintAttributeSet(asCurrent);
            vjPrintDialog.showDialog();
        } 
    }

    /**
     * Updates each of the top level panels
     */
    private void updatePanels() {
        pnlGeneral.updateInfo();
        pnlPageSetup.updateInfo();
        pnlAppearance.updateInfo();
    }

    public static void initResource() {
        java.security.AccessController.doPrivileged(
            new java.security.PrivilegedAction<Object>() {
                public Object run() {
                    try {
                        messageRB = ResourceBundle.getBundle(strBundle);
                        return null;
                    } catch (java.util.MissingResourceException e) {
                        // throw new Error("Fatal: Resource for ServiceUI " +
                        //                "is missing");
                    }
                    return null;
                }
            }
        );
    }



    /**
     * Returns message string from resource
     */
    public static String getMsg(String key) {
        if (messageRB != null) {
           try {
               String s = messageRB.getString(key);
               return s;
           } catch (java.util.MissingResourceException e) {
            // throw new Error("Fatal: Resource for ServiceUI is broken; " +
            //                "there is no " + key + " key in resource");
           }
        }
        if (key.equals("dialog.overwrite"))
           return "This file already exists. Would you like to overwrite the existing file?";
        if (key.equals("dialog.printtitle"))
           return "Print Screen";
        if (key.equals("label.numcopies"))
           return "Number of copies:";
        if (key.equals("radiobutton.rangepages"))
           return "Pages";
        if (key.equals("label.priority"))
           return "Priority:";
        if (key.equals("label.rangeto"))
           return "To";
        if (key.equals("label.jobname"))
           return "Job Name:";
        if (key.equals("label.jobname.mnemonic"))
           return "U";
        if (key.equals("label.username"))
           return "User Name:";
        if (key.equals("accepting-jobs"))
           return "Accepting jobs";
        if (key.equals("auto-select"))
           return "Automatically Select";

        if (key.equals("dialog.owtitle"))
           return "Print To File";
        if (key.equals("dialog.writeerror"))
           return "Could not write to file ";
        if (key.equals("dialog.pstitle"))
           return "Page Setup";
        if (key.equals("dialog.noprintermsg"))
           return "No printer found";

        if (key.equals("invoice"))
           return "Invoice";
        if (key.equals("tab.general.vkMnemonic"))
           return "71";
        if (key.equals("label.username.mnemonic"))
           return "U";

        if (key.equals("na-letter"))
           return "Letter";
        if (key.equals("na-legal"))
           return "Legal";
        if (key.equals("executive"))
           return "Executive";
        if (key.equals("ledger"))
           return "Ledger";
        if (key.equals("iso-a0"))
           return "A0 (ISO/DIN & JIS)";
        if (key.equals("iso-a1"))
           return "A1 (ISO/DIN & JIS)";
        if (key.equals("iso-a2"))
           return "A2 (ISO/DIN & JIS)";
        if (key.equals("iso-a3"))
           return "A3 (ISO/DIN & JIS)";
        if (key.equals("iso-a4"))
           return "A4 (ISO/DIN & JIS)";
        if (key.equals("iso-a5"))
           return "A5 (ISO/DIN & JIS)";
        if (key.equals("iso-a6"))
           return "A6 (ISO/DIN & JIS)";
        if (key.equals("iso-b0"))
           return "B0 (ISO/DIN)";
        if (key.equals("iso-b1"))
           return "B1 (ISO/DIN)";
        if (key.equals("iso-b2"))
           return "B2 (ISO/DIN)";
        if (key.equals("iso-b3"))
           return "B3 (ISO/DIN)";
        if (key.equals("iso-b4"))
           return "B4 (ISO/DIN)";
        if (key.equals("iso-b5"))
           return "B5 (ISO/DIN)";
        if (key.equals("iso-c3"))
           return "C3 (ISO/DIN)";
        if (key.equals("iso-c4"))
           return "C4 (ISO/DIN)";
        if (key.equals("iso-c5"))
           return "C5 (ISO/DIN)";
        if (key.equals("iso-c6"))
           return "C6 (ISO/DIN)";
        if (key.equals("folio"))
           return "Folio";
        if (key.equals("jis-b5"))
           return "B5 (JIS)";
        if (key.equals("oufuko-postcard"))
           return "Double Postcard (JIS)";
        if (key.equals("na-number-10-envelope"))
           return "No. 10 Envelope";
        if (key.equals("monarch-envelope"))
           return "Monarch Envelope";
        if (key.equals("iso-designated-long"))
           return "ISO Designated Long";
        if (key.equals("tab.pagesetup.vkMnemonic"))
           return "83";
        if (key.equals("tab.appearance.vkMnemonic"))
           return "65";

        return key;
    }

    /**
     * Returns mnemonic character from resource
     */
    private static char getMnemonic(String key) {
        String str = getMsg(key + ".mnemonic");
        if ((str != null) && (str.length() > 0)) {
            return str.charAt(0);
        } else {
            return (char)0;
        }
    }

    /**
     * Returns the mnemonic as a KeyEvent.VK constant from the resource.
     */
    private static int getVKMnemonic(String key) {
        String str = getMsg(key + ".vkMnemonic");
        if ((str != null) && (str.length() > 0)) {
            try {
                return Integer.parseInt(str);
            } catch (NumberFormatException nfe) {}
        }
        return 0;
    }

    /**
     * Creates a new JButton and sets its text, and ActionListener
     */
    private static JButton createExitButton(String key, ActionListener al) {
        // String str = getMsg(key);
        JButton btn = new JButton(key);
        btn.addActionListener(al);
        // btn.getAccessibleContext().setAccessibleDescription(str);
        return btn;
    }

    /**
     * Creates a new JCheckBox and sets its text, mnemonic, and ActionListener
     */
    private static JCheckBox createCheckBox(String key, ActionListener al) {
        // JCheckBox cb = new JCheckBox(getMsg(key));
        JCheckBox cb = new JCheckBox(key);
        // cb.setMnemonic(getMnemonic(key));
        cb.addActionListener(al);

        return cb;
    }

    /**
     * Creates a new JRadioButton and sets its text, mnemonic,
     * and ActionListener
     */
    private static JRadioButton createRadioButton(String key,
                                                  ActionListener al)
    {
        // JRadioButton rb = new JRadioButton(getMsg(key));
        // rb.setMnemonic(getMnemonic(key));
        JRadioButton rb = new JRadioButton(key);
        rb.addActionListener(al);

        return rb;
    }

  /**
   * Creates a  pop-up dialog for "no print service"
   */
    public static void showNoPrintService(GraphicsConfiguration gc)
    {
        Frame dlgFrame = new Frame(gc);
        JOptionPane.showMessageDialog(dlgFrame,
                                      getMsg("dialog.noprintermsg"));
        dlgFrame.dispose();
    }

    /**
     * Sets the constraints for the GridBagLayout and adds the Component
     * to the given Container
     */
    private static void addToGB(Component comp, Container cont,
                                GridBagLayout gridbag,
                                GridBagConstraints constraints)
    {
        gridbag.setConstraints(comp, constraints);
        cont.add(comp);
    }

    /**
     * Adds the AbstractButton to both the given ButtonGroup and Container
     */
    private static void addToBG(AbstractButton button, Container cont,
                                ButtonGroup bg)
    {
        bg.add(button);
        cont.add(button);
    }

    private Float getFloat(String key)
    {
        Object obj = hs.get(key);
        if (obj == null || !(obj instanceof Float))
             return null;
        return (Float) obj;
    }

    private Float setFloat(String value, String def)
    {
         String s = value.trim();
         Float retVal = null;

         try {
             if (s.length() > 0)
                 retVal = new Float(s);
         }
         catch (NumberFormatException e0) {
             retVal = null;
         }
         if (retVal != null)
             return retVal;

         try {
             retVal = new Float(def);
         }
         catch (NumberFormatException e1) {
              retVal = null;
         }
         return retVal;
    }


    private String getString(String key)
    {
        if (hs == null)
           return null;

        Object obj = hs.get(key);
        if (obj == null || !(obj instanceof String))
             return null;
        return (String) obj;
    }

    private void saveString(String key, String val)
    {
        if (hs == null || val == null)
            return;
        hs.put(key, val);
    }

    protected void setColorChoice() {
        if (rbMonochrome.isSelected()) {
            fgColorLabel.setEnabled(true);
            fgColorDialog.setActive(true);
            bgColorLabel.setEnabled(false);
            bgColorDialog.setActive(false);
        }
        else {
            fgColorLabel.setEnabled(false);
            fgColorDialog.setActive(false);
            bgColorLabel.setEnabled(true);
            bgColorDialog.setActive(true);
        }
    }


    @SuppressWarnings("unchecked")
	private void setFromSession()
    {
        if (pnlGeneral == null)
           return;
        SessionShare sshare = Util.getSessionShare();
        if (sshare == null)
           return;
        hs = sshare.userInfo();
        if (hs == null)
            return;
        int x = 100;
        int y = 100;
        boolean bDefault = true;

        Float fv = getFloat(VjPrintDef.PRINT_LOCX);
        if (fv != null)
            x = fv.intValue();
        fv = getFloat(VjPrintDef.PRINT_LOCY);
        if (fv != null)
            y = fv.intValue();
        setLocation(x, y);
        btnPreview.setEnabled(true);
        String s;
        if (cbPrintToFile != null) {
            bDefault = false;
            s = getString(VjPrintDef.DESTIONATION);
            if (s != null && s.equals(VjPrintDef.FILE))
                bDefault = true;
            cbPrintToFile.setSelected(bDefault);
            btnPreview.setEnabled(!bDefault);
            
        }
        if (cbFitPaper != null) {
            bDefault = true;
            s = getString(VjPrintDef.PRINT_RESHAPE);
            if (s != null && s.equals(VjPrintDef.NO))
               bDefault = false;
            cbFitPaper.setSelected(bDefault);
        }
        if (cbCursorLines != null) {
            bDefault = false;
            s = getString(VjPrintDef.PRINT_CURSOR_LINE);
            if (s != null && s.equals(VjPrintDef.YES))
               bDefault = true;
            cbCursorLines.setSelected(bDefault);
        }

        if (rbMonochrome != null) {
            bDefault = true;
            s = getString(VjPrintDef.PRINT_COLOR);
            if (s != null && s.equals(VjPrintDef.COLOR))
                bDefault = false;
            rbMonochrome.setSelected(bDefault);
            setColorChoice();
        }
        if (cbName != null && printerNum > 0) {
            s = getString(VjPrintDef.PRINTER_NAME);
            if (s != null) 
               VjPrintUtil.setComboxSelectItem(cbName, s);
        }
        if (cbSize != null) {
            s = getString(VjPrintDef.PRINT_PAPER);
            if (s != null) 
               VjPrintUtil.setMediaComboxSelectItem(cbSize, s);   
               // VjPrintUtil.setComboxSelectItem(cbSize, s);
        }
        if (cbFormat != null) {
            s = getString(VjPrintDef.PRINT_FORMATLABEL);
            if (s == null) 
               s = formatLabels[1];
            VjPrintUtil.setComboxSelectItem(cbFormat, s);
        }
        if (rbLandscape != null) {
            bDefault = true;
            s = getString(VjPrintDef.PRINT_ORIENTATION);
            if (s != null && s.equals(VjPrintDef.PORTRAIT)) 
                bDefault = false;
            rbLandscape.setSelected(bDefault);
        }
        if (bgColorDialog != null) {
            s = getString(VjPrintDef.PRINT_BG);
            if (s != null)
                bgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, s);
            else
                bgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, "white");
            s = getString(VjPrintDef.PRINT_FG);
            if (s != null)
                fgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, s);
            else
                fgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, "black");
        }
        if (rbDraft != null) {
            s = getString(VjPrintDef.PRINT_RESOLUTION);
            if (s != null) {
                if (s.equals(VjPrintDef.DRAFT_RES))
                   rbDraft.setSelected(true);
                else if (s.equals(VjPrintDef.HI_RES))
                   rbHigh.setSelected(true);
                else
                   rbNormal.setSelected(true);
            }
            else
                rbNormal.setSelected(true);
        }
        if (leftMargin != null) {
            fv = getFloat(VjPrintDef.PRINT_LEFT_MARGIN);
            if (fv != null)
                leftMargin.setValue(fv);
            fv = getFloat(VjPrintDef.PRINT_RIGHT_MARGIN);
            if (fv != null)
                rightMargin.setValue(fv);
            fv = getFloat(VjPrintDef.PRINT_TOP_MARGIN);
            if (fv != null)
                topMargin.setValue(fv);
            fv = getFloat(VjPrintDef.PRINT_BOTTOM_MARGIN);
            if (fv != null)
                bottomMargin.setValue(fv);
        }
        if (spectrumWidth != null) {
            fv = getFloat(VjPrintDef.PRINT_SPWIDTH);
            if (fv != null)
                spectrumWidth.setText(fv.toString());
            fv = getFloat(VjPrintDef.PRINT_LINEWIDTH);
            if (fv != null)
                lineWidth.setText(fv.toString());
        }
    }

    private void saveMedia()
    {
        VjMediaSizeObj obj = (VjMediaSizeObj)cbSize.getSelectedItem();
        if (obj == null)
            return;
        MediaSize ms = obj.getMediaSize();
        if (ms == null)
            ms = MediaSize.NA.LETTER;
        double w = VjPaperMedia.getPixelWidth(ms);
        double h = VjPaperMedia.getPixelHeight(ms);
        hs.put(VjPrintDef.PRINT_WIDTH, new Float(w));
        hs.put(VjPrintDef.PRINT_HEIGHT, new Float(h));
    }

    @SuppressWarnings("unchecked")
	private void saveToSession()
    {
        printerName = null;
        if (pnlGeneral == null)
           return;
        SessionShare sshare = Util.getSessionShare();
        if (sshare == null)
           return;
        hs = sshare.userInfo();
        if (hs == null)
            return;
        String s;
        Float fval;
        Point pt = this.getLocation();
        hs.put(VjPrintDef.PRINT_LOCX, new Float((float)pt.x));
        hs.put(VjPrintDef.PRINT_LOCY, new Float((float)pt.y));
        hs.put(VjPrintDef.DESTIONATION, VjPrintDef.PRINTER);
        hs.put(VjPrintDef.PRINT_MARGIN_UNIT, VjPrintDef.MM);
        hs.put(VjPrintDef.FILE_MARGIN_UNIT, VjPrintDef.MM);
        if (cbPrintToFile != null && cbPrintToFile.isSelected())
            hs.put(VjPrintDef.DESTIONATION, VjPrintDef.FILE);
        if (cbFitPaper != null && cbFitPaper.isSelected())
            hs.put(VjPrintDef.PRINT_RESHAPE, VjPrintDef.YES);
        else
            hs.put(VjPrintDef.PRINT_RESHAPE, VjPrintDef.NO);
        if (cbCursorLines != null && cbCursorLines.isSelected())
            hs.put(VjPrintDef.PRINT_CURSOR_LINE, VjPrintDef.YES);
        else
            hs.put(VjPrintDef.PRINT_CURSOR_LINE, VjPrintDef.NO);
        if (rbMonochrome.isSelected())
            hs.put(VjPrintDef.PRINT_COLOR, VjPrintDef.MONO);
        else
            hs.put(VjPrintDef.PRINT_COLOR, VjPrintDef.COLOR);
        if (rbGraphics.isSelected())
            hs.put(VjPrintDef.PRINT_AREA, VjPrintDef.GRAPHICS);
        else
            hs.put(VjPrintDef.PRINT_AREA, VjPrintDef.VNMRJ);

        if (bPreview)
            hs.put(VjPrintDef.PRINT_PREVIEW, VjPrintDef.YES);
        else
            hs.put(VjPrintDef.PRINT_PREVIEW, VjPrintDef.NO);
        if (cbName != null) {
            printerName = (String)cbName.getSelectedItem();
            if (printerName != null)
                hs.put(VjPrintDef.PRINTER_NAME, printerName);
        }
        if (cbSize != null) {
            VjMediaSizeObj mobj = (VjMediaSizeObj)cbSize.getSelectedItem();
            if (mobj != null)
                hs.put(VjPrintDef.PRINT_PAPER, mobj.getMediaName());

            // s = (String)cbSize.getSelectedItem();
            // if (s != null)
            //     hs.put(VjPrintDef.PRINT_PAPER, s);
        }
        if (rbLandscape != null) {
            if (rbLandscape.isSelected())
                hs.put(VjPrintDef.PRINT_ORIENTATION, VjPrintDef.LANDSCPAE);
            else
                hs.put(VjPrintDef.PRINT_ORIENTATION, VjPrintDef.PORTRAIT);
        }
        if (bgColorDialog != null) {
            s = bgColorDialog.getAttribute(vnmr.bo.VObjDef.VALUE);
            if (s != null)
                hs.put(VjPrintDef.PRINT_BG, s);
            s = fgColorDialog.getAttribute(vnmr.bo.VObjDef.VALUE);
            if (s != null)
                hs.put(VjPrintDef.PRINT_FG, s);
        }
        if (spectrumWidth != null) {
            s = spectrumWidth.getText();
            fval = setFloat(s, "1");
            if (fval != null)
                hs.put(VjPrintDef.PRINT_SPWIDTH, fval);
            s = lineWidth.getText();
            fval = setFloat(s, "1");
            if (fval != null)
                hs.put(VjPrintDef.PRINT_LINEWIDTH, fval);
        }
        if (leftMargin != null) {
            fval = (Float)leftMargin.getValue();
            hs.put(VjPrintDef.PRINT_LEFT_MARGIN, new Float(fval.floatValue()));
            fval = (Float)rightMargin.getValue();
            hs.put(VjPrintDef.PRINT_RIGHT_MARGIN, new Float(fval.floatValue()));
            fval = (Float)topMargin.getValue();
            hs.put(VjPrintDef.PRINT_TOP_MARGIN, new Float(fval.floatValue()));
            fval = (Float)bottomMargin.getValue();
            hs.put(VjPrintDef.PRINT_BOTTOM_MARGIN, new Float(fval.floatValue()));
        }
        s = VjPrintDef.NORMAL_RES;
        if (rbDraft != null) {
            if (rbDraft.isSelected())
                s = VjPrintDef.DRAFT_RES;
            else if (rbHigh.isSelected())
                s = VjPrintDef.HI_RES;
        }
        hs.put(VjPrintDef.PRINT_RESOLUTION, s);
        float f = 1.0f;
        if (copiesModel != null) {
            f = (float)copiesModel.getNumber().floatValue();
            if (f < 1.0f)
                f = 1.0f;
        }
        hs.put(VjPrintDef.PRINT_COPIES, new Float(f));
        hs.put(VjPrintDef.PRINT_FORMAT, getFormat());
        saveMedia();
    }


    /**
     * The "General" tab.  Includes the controls for PrintService,
     * PageRange, and Copies/Collate.
     */
    private class GeneralPanel extends JPanel {

        private PrintServicePanel pnlPrintService;
        private PrintAreaPanel pnlPrintArea;
        private CopiesPanel pnlCopies;

        public GeneralPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);

            c.fill = GridBagConstraints.BOTH;
            c.insets = panelInsets;
            c.weightx = 1.0;
            c.weighty = 1.0;

            c.gridwidth = GridBagConstraints.REMAINDER;
            pnlPrintService = new PrintServicePanel();
            addToGB(pnlPrintService, this, gridbag, c);

            c.gridwidth = GridBagConstraints.RELATIVE;
            new PrintRangePanel();

            pnlPrintArea = new PrintAreaPanel();
            addToGB(pnlPrintArea, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            pnlCopies = new CopiesPanel();
            addToGB(pnlCopies, this, gridbag, c);
        }

        public boolean isPrintToFileRequested() {
            return (pnlPrintService.isPrintToFileSelected());
        }

        //  class GeneralPanel 
        public void updateInfo() {
            pnlPrintService.updateInfo();
            // pnlPrintRange.updateInfo();
            pnlCopies.updateInfo();
        }
    }

    private class PrintServicePanel extends JPanel
        implements ActionListener, ItemListener, PopupMenuListener
    {
        private String strTitle;
        private FilePermission printToFilePermission;
        private JButton btnProperties;
        // private JComboBox cbName;
        private JLabel lblType, lblStatus, lblInfo;
        private ServiceUIFactory uiFactory;
        private boolean changedService = false;
        private boolean filePermission;

        public PrintServicePanel() {
            super();

            if (psCurrent != null) {
               uiFactory = VjPrintUtil.getServiceUIFactory(psCurrent);
            }

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Print_Service", "Print Service");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            cbName = new JComboBox();

            printerNum = cbName.getItemCount();
            if (printerNum > 0)
                cbName.setSelectedIndex(0);
            cbName.addItemListener(this);
            cbName.addPopupMenuListener(this);

            c.fill = GridBagConstraints.BOTH;
            c.insets = compInsets;

            c.weightx = 0.0;
            lblStr = Util.getLabel("_Name", "Name")+":";
            JLabel lblName = new JLabel(lblStr, JLabel.TRAILING);
            // lblName.setDisplayedMnemonic(getMnemonic("label.psname"));
            lblName.setLabelFor(cbName);
            addToGB(lblName, this, gridbag, c);
            c.weightx = 1.0;
            c.gridwidth = GridBagConstraints.RELATIVE;
            addToGB(cbName, this, gridbag, c);
            c.weightx = 0.0;
            c.gridwidth = GridBagConstraints.REMAINDER;
            lblStr = Util.getLabel("_Properties", "Properties");
            btnProperties = new JButton(lblStr);
            btnProperties.addActionListener(this);
            addToGB(btnProperties, this, gridbag, c);

            c.weighty = 1.0;
            // lblStr = getMsg("label.status");
            lblStr = Util.getLabel("_Status", "Status")+":";
            lblStatus = addLabel(lblStr, gridbag, c);
            lblStatus.setLabelFor(null);

            // lblStr = getMsg("label.pstype");
            lblStr = Util.getLabel("_Type", "Type")+":";
            lblType = addLabel(lblStr, gridbag, c);
            lblType.setLabelFor(null);

            c.gridwidth = 1;
            lblStr = Util.getLabel("_Info", "Info")+":";
            addToGB(new JLabel(lblStr, JLabel.TRAILING),
                    this, gridbag, c);
            c.gridwidth = GridBagConstraints.RELATIVE;
            lblInfo = new JLabel();
            lblInfo.setLabelFor(null);

            addToGB(lblInfo, this, gridbag, c);

            c.ipadx = 10;
            c.gridwidth = GridBagConstraints.REMAINDER;
            lblStr = Util.getLabel("_Print_to_file", "Print to file");
            cbPrintToFile = createCheckBox(lblStr, this);
            Font ft = cbPrintToFile.getFont(); 
            int fh = ft.getSize();
            ft = ft.deriveFont((float) (fh + 3));
            cbPrintToFile.setFont(ft);
            addToGB(cbPrintToFile, this, gridbag, c);

            filePermission = allowedToPrintToFile();
            // cbPrintToFile.setSelected(filePermission);
        }

        public boolean isPrintToFileSelected() {
            return cbPrintToFile.isSelected();
        }

        private JLabel addLabel(String text,
                                GridBagLayout gridbag, GridBagConstraints c)
        {
            c.gridwidth = 1;
            addToGB(new JLabel(text, JLabel.TRAILING), this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            JLabel label = new JLabel();
            addToGB(label, this, gridbag, c);

            return label;
        }

        public void actionPerformed(ActionEvent e) {
            Object source = e.getSource();

            if (source == btnProperties) {
                if (uiFactory != null) {
                    JDialog dialog = (JDialog)uiFactory.getUI(
                                               ServiceUIFactory.MAIN_UIROLE,
                                               ServiceUIFactory.JDIALOG_UI);

                    if (dialog != null) {
                        dialog.setVisible(true);
                    } else {
                        // REMIND: may want to notify the user why we're
                        //         disabling the button

                        btnProperties.setEnabled(false);
                    }
                }
            }
            if (source == cbPrintToFile) {
                if (cbPrintToFile.isSelected()) {
                    btnPreview.setEnabled(false);
                    btnApprove.setEnabled(true);
                }
                else {
                    btnPreview.setEnabled(true);
                    if (printerNum < 1)
                       btnApprove.setEnabled(false);
                }
                return;
            }
        }

        private void updatePrinter() {
            String name = (String)cbName.getSelectedItem();
            if (bChangeMenu && psCurrent != null)
                return;
            if (name == null || (name.length() < 1))
                return;
            saveString(VjPrintDef.PRINTER_NAME, name);
            PrintService srv = null;
            if (services != null) {
                for (int n = 0; n < services.length; n++) {
                    if (services[n] != null) {
                        if (name.equals(services[n].getName())) {
                             srv = services[n];
                             break;
                        }
                    }
                }
            }
            if (srv == null)
                return;
            if (srv.equals(psCurrent))
                return;
            psCurrent = srv;
            uiFactory = VjPrintUtil.getServiceUIFactory(psCurrent);
            changedService = true;
            boolean bDest = false;
            Media media = (Media)VjPrintUtil.getDefaultAttributeValue(
                                  psCurrent, Media.class);
            if (media != null && (media instanceof MediaSizeName)) {
                // System.out.println(" default paper:  "+media.toString());
            }

            Destination dest =
                            (Destination)asOriginal.get(Destination.class);
            // to preserve the state of Print To File
            if (dest != null || isPrintToFileSelected()) {
               //  bDest = VjPrintUtil.isCategorySupported(psCurrent, Destination.class);
                 bDest = true;
            }
            if (bDest) {
                if (dest != null) {
                    asCurrent.add(dest);
                } else {
                    dest = (Destination)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, Destination.class);
                    // "dest" should not be null. The following code
                    // is only added to safeguard against a possible
                    // buggy implementation of a PrintService having a
                    // null default Destination.
                    if (dest == null) {
                        try {
                            dest =
                                new Destination(new URI("file:out.prn"));
                        } catch (URISyntaxException ue) { }
                    }

                    if (dest != null) {
                        asCurrent.add(dest);
                    }
                }
            } else {
                asCurrent.remove(Destination.class);
            }
        }

        public void itemStateChanged(ItemEvent e) {
            if (e.getStateChange() != ItemEvent.SELECTED)
                return;
            if (e.getSource() == cbName) {
                updatePrinter();
                return;
            }
        }

        public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
            changedService = false;
        }

        public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
            if (changedService) {
                changedService = false;
                updatePanels();
            }
        }

        public void popupMenuCanceled(PopupMenuEvent e) {
        }

        /**
         * We disable the "Print To File" checkbox if this returns false
         */
        private boolean allowedToPrintToFile() {
            if (psCurrent == null)
                return true;
            try {
                throwPrintToFile();
                return true;
            } catch (SecurityException e) {
                return false;
            }
        }

        /**
         * Break this out as it may be useful when we allow API to
         * specify printing to a file. In that case its probably right
         * to throw a SecurityException if the permission is not granted.
         */
        private void throwPrintToFile() {
            SecurityManager security = System.getSecurityManager();
            if (security != null) {
                if (printToFilePermission == null) {
                    printToFilePermission =
                        new FilePermission("<<ALL FILES>>", "read,write");
                }
                security.checkPermission(printToFilePermission);
            }
        }

        //  class PrintServicePanel
        public void updateInfo() {
            // Class<Destination> dstCategory = Destination.class;
            // boolean dstSupported = false;
            // boolean dstSelected = false;
            boolean dstAllowed = filePermission ?
                allowedToPrintToFile() : false;

            // setup Destination (print-to-file) widgets
            /**
            dstSupported = true;
            Destination dst = (Destination)asCurrent.get(dstCategory);
            if (dst != null) {
                dstSelected = true;
            }
            cbPrintToFile.setEnabled(dstSupported && dstAllowed);
            cbPrintToFile.setSelected(dstSelected && dstAllowed
                                      && dstSupported);
            ****/

            if (psCurrent == null) {
                cbPrintToFile.setEnabled(dstAllowed);
                cbPrintToFile.setSelected(dstAllowed);
                btnProperties.setEnabled(false);
                return;
            }
            

            // setup PrintService information widgets
            Attribute type = VjPrintUtil.getAttribute(psCurrent,
                                 PrinterMakeAndModel.class);
            if (type != null) {
                lblType.setText(type.toString());
            }
            Attribute attr = VjPrintUtil.getAttribute(
                                 psCurrent, PrinterIsAcceptingJobs.class);
            if (attr != null) {
                lblStatus.setText(getMsg(attr.toString()));
            }
            Attribute info = VjPrintUtil.getAttribute(
                                 psCurrent, PrinterInfo.class);
            if (info != null) {
                lblInfo.setText(info.toString());
            }
            btnProperties.setEnabled(uiFactory != null);
        }
    }

    private class PrintRangePanel extends JPanel
        implements ActionListener, FocusListener
    {
        // private String strTitle = getMsg("border.printrange");
        private String strTitle;
        private final PageRanges prAll = new PageRanges(1, Integer.MAX_VALUE);
        private JRadioButton rbAll, rbPages, rbSelect;
        private JFormattedTextField tfRangeFrom, tfRangeTo;
        private JLabel lblRangeTo;
        private boolean prSupported;

        public PrintRangePanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Print_Range", "Print Range");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            c.fill = GridBagConstraints.BOTH;
            c.insets = compInsets;
            c.gridwidth = GridBagConstraints.REMAINDER;

            ButtonGroup bg = new ButtonGroup();
            JPanel pnlTop = new JPanel(new FlowLayout(FlowLayout.LEADING));
            // rbAll = createRadioButton("radiobutton.rangeall", this);
            lblStr = Util.getLabel("_Range_All", "Range All");
            rbAll = createRadioButton(lblStr, this);
            rbAll.setSelected(true);
            bg.add(rbAll);
            pnlTop.add(rbAll);
            addToGB(pnlTop, this, gridbag, c);

            // Selection never seemed to work so I'm commenting this part.
            /*
            if (isAWT) {
                JPanel pnlMiddle  =
                    new JPanel(new FlowLayout(FlowLayout.LEADING));
                lblStr = Util.getLabel("_Range_All", "Range All");
                rbSelect = createRadioButton(lblStr, this);
                bg.add(rbSelect);
                pnlMiddle.add(rbSelect);
                addToGB(pnlMiddle, this, gridbag, c);
            }
            */

            JPanel pnlBottom = new JPanel(new FlowLayout(FlowLayout.LEADING));
            // rbPages = createRadioButton("radiobutton.rangepages", this);
            lblStr = Util.getLabel("_Range_Pages", "Range Pages");
            rbPages = createRadioButton(lblStr, this);
            bg.add(rbPages);
            pnlBottom.add(rbPages);
            DecimalFormat format = new DecimalFormat("####0");
            format.setMinimumFractionDigits(0);
            format.setMaximumFractionDigits(0);
            format.setMinimumIntegerDigits(0);
            format.setMaximumIntegerDigits(5);
            format.setParseIntegerOnly(true);
            format.setDecimalSeparatorAlwaysShown(false);
            NumberFormatter nf = new NumberFormatter(format);
            nf.setMinimum(new Integer(1));
            nf.setMaximum(new Integer(Integer.MAX_VALUE));
            nf.setAllowsInvalid(true);
            nf.setCommitsOnValidEdit(true);
            tfRangeFrom = new JFormattedTextField(nf);
            tfRangeFrom.setColumns(4);
            tfRangeFrom.setEnabled(false);
            tfRangeFrom.addActionListener(this);
            tfRangeFrom.addFocusListener(this);
            tfRangeFrom.setFocusLostBehavior(
                JFormattedTextField.PERSIST);
            tfRangeFrom.getAccessibleContext().setAccessibleName(
                                          getMsg("radiobutton.rangepages"));
            pnlBottom.add(tfRangeFrom);
            // lblStr = getMsg("label.rangeto");
            lblStr = Util.getLabel("_Range_To", "Range To")+":";
            lblRangeTo = new JLabel(lblStr);
            lblRangeTo.setEnabled(false);
            pnlBottom.add(lblRangeTo);
            NumberFormatter nfto;
            try {
                nfto = (NumberFormatter)nf.clone();
            } catch (CloneNotSupportedException e) {
                nfto = new NumberFormatter();
            }
            tfRangeTo = new JFormattedTextField(nfto);
            tfRangeTo.setColumns(4);
            tfRangeTo.setEnabled(false);
            tfRangeTo.addFocusListener(this);
            tfRangeTo.getAccessibleContext().setAccessibleName(
                                          getMsg("label.rangeto"));
            pnlBottom.add(tfRangeTo);
            addToGB(pnlBottom, this, gridbag, c);
        }

        public void actionPerformed(ActionEvent e) {
            Object source = e.getSource();
            SunPageSelection select = SunPageSelection.ALL;

            setupRangeWidgets();

            if (source == rbAll) {
                asCurrent.add(prAll);
            } else if (source == rbSelect) {
                select = SunPageSelection.SELECTION;
            } else if (source == rbPages ||
                       source == tfRangeFrom ||
                       source == tfRangeTo) {
                updateRangeAttribute();
                select = SunPageSelection.RANGE;
            }

            if (isAWT) {
                asCurrent.add(select);
            }
        }

        public void focusLost(FocusEvent e) {
            Object source = e.getSource();

            if ((source == tfRangeFrom) || (source == tfRangeTo)) {
                updateRangeAttribute();
            }
        }

        public void focusGained(FocusEvent e) {}

        private void setupRangeWidgets() {
            boolean rangeEnabled = (rbPages.isSelected() && prSupported);
            tfRangeFrom.setEnabled(rangeEnabled);
            tfRangeTo.setEnabled(rangeEnabled);
            lblRangeTo.setEnabled(rangeEnabled);
        }

        private void updateRangeAttribute() {
            String strFrom = tfRangeFrom.getText();
            String strTo = tfRangeTo.getText();

            int min;
            int max;

            try {
                min = Integer.parseInt(strFrom);
            } catch (NumberFormatException e) {
                min = 1;
            }

            try {
                max = Integer.parseInt(strTo);
            } catch (NumberFormatException e) {
                max = min;
            }

            if (min < 1) {
                min = 1;
                tfRangeFrom.setValue(new Integer(1));
            }

            if (max < min) {
                max = min;
                tfRangeTo.setValue(new Integer(min));
            }

            PageRanges pr = new PageRanges(min, max);
            asCurrent.add(pr);
        }

        //  class PrintRangePanel
        /********
        public void updateInfo() {
            Class<PageRanges> prCategory = PageRanges.class;
            SunPageSelection select = SunPageSelection.SELECTION;
            prSupported = false;

            if (isAWT || VjPrintUtil.isCategorySupported(psCurrent,prCategory))
                  prSupported = true;

            int min = 1;
            int max = 1;

            PageRanges pr = (PageRanges)asCurrent.get(prCategory);
            if (pr != null) {
                if (!pr.equals(prAll)) {
                    select = SunPageSelection.RANGE;

                    int[][] members = pr.getMembers();
                    if ((members.length > 0) && (members[0].length > 1)) {
                         min = members[0][0];
                         max = members[0][1];
                    }
                }
                if (isAWT) {
                    select = (SunPageSelection)asCurrent.get(
                                                SunPageSelection.class);
                }
            }

            if (select == SunPageSelection.ALL) {
                rbAll.setSelected(true);
            } else if (select == SunPageSelection.SELECTION) {
                // Comment this for now -  rbSelect is not initialized
                // because Selection button is not added.
                // See PrintRangePanel above.

                //rbSelect.setSelected(true);
            } else { // RANGE
                rbPages.setSelected(true);
            }
            tfRangeFrom.setValue(new Integer(min));
            tfRangeTo.setValue(new Integer(max));
            rbAll.setEnabled(prSupported);
            rbPages.setEnabled(prSupported);
            setupRangeWidgets();
        }
        **********/
    }

    private class PrintAreaPanel extends JPanel
    {
        private String strTitle;

        public PrintAreaPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Print_Area", "Print Area");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            c.fill = GridBagConstraints.BOTH;
            c.insets = compInsets;
            c.gridwidth = GridBagConstraints.REMAINDER;

            ButtonGroup bg = new ButtonGroup();
            JPanel pnlTop = new JPanel(new FlowLayout(FlowLayout.LEADING));
            lblStr = Util.getLabel("_Viewports", "Viewports");
            rbGraphics = new JRadioButton(lblStr);
            rbGraphics.setSelected(true);
            bg.add(rbGraphics);
            pnlTop.add(rbGraphics);
            addToGB(pnlTop, this, gridbag, c);

            JPanel pnlBottom = new JPanel(new FlowLayout(FlowLayout.LEADING));
            lblStr = Util.getLabel("_VNMRJ_Window", "VNMRJ Window");
            rbVnmrj = new JRadioButton(lblStr);
            bg.add(rbVnmrj);
            pnlBottom.add(rbVnmrj);
            addToGB(pnlBottom, this, gridbag, c);
        }
     
    }  // PrintArea

    private class CopiesPanel extends JPanel
        implements ActionListener, ChangeListener
    {
        // private String strTitle = getMsg("border.copies");
        private String strTitle;
        // private SpinnerNumberModel snModel;
        private JSpinner spinCopies;
        private JLabel lblCopies;
        private JCheckBox cbCollate;
        private boolean scSupported;

        public CopiesPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Copies", "Copies");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            c.fill = GridBagConstraints.HORIZONTAL;
            c.insets = compInsets;

            // lblStr = getMsg("label.numcopies"); 
            lblStr = Util.getLabel("_Number_of_copies", "Number of copies")+":";
            lblCopies = new JLabel(lblStr, JLabel.TRAILING);
            // lblCopies.setDisplayedMnemonic(getMnemonic("label.numcopies"));
            lblCopies.getAccessibleContext().setAccessibleName(
                                             getMsg("label.numcopies"));
            addToGB(lblCopies, this, gridbag, c);

            copiesModel = new SpinnerNumberModel(1, 1, 99, 1);
            spinCopies = new JSpinner(copiesModel);
            lblCopies.setLabelFor(spinCopies);
            // REMIND
            ((JSpinner.NumberEditor)spinCopies.getEditor()).getTextField().setColumns(3);
            spinCopies.addChangeListener(this);
            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(spinCopies, this, gridbag, c);

            lblStr = Util.getLabel("_Collate", "Collate");
            cbCollate = createCheckBox(lblStr, this);
            cbCollate.setEnabled(false);
            addToGB(cbCollate, this, gridbag, c);
        }

        public void actionPerformed(ActionEvent e) {
            if (cbCollate.isSelected()) {
                asCurrent.add(SheetCollate.COLLATED);
            } else {
                asCurrent.add(SheetCollate.UNCOLLATED);
            }
        }

        public void stateChanged(ChangeEvent e) {
            updateCollateCB();

            asCurrent.add(new Copies(copiesModel.getNumber().intValue()));
        }

        private void updateCollateCB() {
            int num = copiesModel.getNumber().intValue();
            if (isAWT) {
                cbCollate.setEnabled(true);
            } else {
                cbCollate.setEnabled((num > 1) && scSupported);
            }
        }

        //  class CopiesPanel
        public void updateInfo() {
            Class<Copies> cpCategory = Copies.class;
            // Class<CopiesSupported> csCategory = CopiesSupported.class;
            CopiesSupported cs = null;
            Copies cp = null;
            boolean cpSupported = true;
            scSupported = true;

            // setup Copies spinner
            // cpSupported = VjPrintUtil.isCategorySupported(psCurrent, cpCategory);

            if (psCurrent == null)
                return;
            cs = (CopiesSupported)VjPrintUtil.getAttributeValues(
                                    psCurrent, cpCategory, null, null);
            cp = (Copies)asCurrent.get(cpCategory);
            if (cp == null)
                 cp = (Copies)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, cpCategory);
            if (cs == null)
                cs = new CopiesSupported(1, 1);
            if (cp == null)
                cp = new Copies(1);
            spinCopies.setEnabled(cpSupported);
            lblCopies.setEnabled(cpSupported);

            int[][] members = cs.getMembers();
            int min, max;
            if ((members.length > 0) && (members[0].length > 0)) {
                min = members[0][0];
                max = members[0][1];
            } else {
                min = 1;
                // max = Integer.MAX_VALUE;
                max = 99;
            }
            copiesModel.setMinimum(new Integer(min));
            copiesModel.setMaximum(new Integer(max));

            int value = cp.getValue();
            if ((value < min) || (value > max)) {
                value = min;
            }
            copiesModel.setValue(new Integer(value));

            Class<SheetCollate> scCategory = SheetCollate.class;
            SheetCollate sc = null;
            // setup Collate checkbox
            scSupported = 
                       VjPrintUtil.isCategorySupported(psCurrent,scCategory);
            sc = (SheetCollate)asCurrent.get(scCategory);
            if (sc == null) {
                sc = (SheetCollate)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, scCategory);
                if (sc == null)
                   sc = SheetCollate.UNCOLLATED;
            }
            cbCollate.setSelected(sc == SheetCollate.COLLATED);
            updateCollateCB();
        }
    }




    /**
     * The "Page Setup" tab.  Includes the controls for MediaSource/MediaTray,
     * OrientationRequested, and Sides.
     */
    private class PageSetupPanel extends JPanel {

        private MediaPanel pnlMedia;
        private OrientationPanel pnlOrientation;
        private MarginsPanel pnlMargins;

        public PageSetupPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);

            c.fill = GridBagConstraints.BOTH;
            c.insets = panelInsets;
            c.weightx = 1.0;
            c.weighty = 1.0;

            c.gridwidth = GridBagConstraints.REMAINDER;
            pnlMedia = new MediaPanel();
            addToGB(pnlMedia, this, gridbag, c);

            pnlOrientation = new OrientationPanel();
            c.gridwidth = GridBagConstraints.RELATIVE;
            addToGB(pnlOrientation, this, gridbag, c);

            pnlMargins = new MarginsPanel();
            pnlOrientation.addOrientationListener(pnlMargins);
            pnlMedia.addMediaListener(pnlMargins);
            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(pnlMargins, this, gridbag, c);
        }

        // class PageSetupPanel
        public void updateInfo() {
            pnlMedia.updateInfo();
            pnlOrientation.updateInfo();
            pnlMargins.updateInfo();
        }
    }

    private class MarginsPanel extends JPanel
                               implements ActionListener, FocusListener {

        // private String strTitle = getMsg("border.margins");
        private String strTitle;
        private JLabel lblLeft, lblRight, lblTop, lblBottom;
        private int units = MediaPrintableArea.MM;
        // storage for the last margin values calculated, -ve is uninitialised
        private float lmVal = -1f,rmVal = -1f, tmVal = -1f, bmVal = -1f;
        // storage for margins as objects mapped into orientation for display
        private Float lmObj,rmObj,tmObj,bmObj;

        public MarginsPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();
            c.fill = GridBagConstraints.HORIZONTAL;
            c.weightx = 1.0;
            c.weighty = 0.0;
            c.insets = compInsets;

            setLayout(gridbag);
            strTitle = Util.getLabel("_Margins", "Margins");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

/**
            String unitsKey = "label.millimetres";
            String defaultCountry = Locale.getDefault().getCountry();
            if (defaultCountry != null &&
                (defaultCountry.equals("") ||
                 defaultCountry.equals(Locale.US.getCountry()) ||
                 defaultCountry.equals(Locale.CANADA.getCountry()))) {
                unitsKey = "label.inches";
                units = MediaPrintableArea.INCH;
            }
            units = MediaPrintableArea.INCH;
            unitsKey = "label.inches";

            String unitsMsg = getMsg(unitsKey);
**/
            String unitStr = " ("+Util.getLabel("_mm", "mm")+")";

            DecimalFormat format;
            if (units == MediaPrintableArea.MM) {
                format = new DecimalFormat("###.##");
                format.setMaximumIntegerDigits(3);
            } else {
                format = new DecimalFormat("##.##");
                format.setMaximumIntegerDigits(2);
            }

            format.setMinimumFractionDigits(1);
            format.setMaximumFractionDigits(2);
            format.setMinimumIntegerDigits(1);
            format.setParseIntegerOnly(false);
            format.setDecimalSeparatorAlwaysShown(true);
            NumberFormatter nf = new NumberFormatter(format);
            nf.setMinimum(new Float(0.0f));
            nf.setMaximum(new Float(999.0f));
            nf.setAllowsInvalid(true);
            nf.setCommitsOnValidEdit(true);

            leftMargin = new JFormattedTextField(nf);
            leftMargin.setHorizontalAlignment(JTextField.CENTER);
            leftMargin.addFocusListener(this);
            leftMargin.addActionListener(this);
            // leftMargin.getAccessibleContext().setAccessibleName(
            //                                  getMsg("label.leftmargin"));
            rightMargin = new JFormattedTextField(nf);
            rightMargin.setHorizontalAlignment(JTextField.CENTER);
            rightMargin.addFocusListener(this);
            rightMargin.addActionListener(this);
            // rightMargin.getAccessibleContext().setAccessibleName(
            //                                   getMsg("label.rightmargin"));
            topMargin = new JFormattedTextField(nf);
            topMargin.setHorizontalAlignment(JTextField.CENTER);
            topMargin.addFocusListener(this);
            topMargin.addActionListener(this);
            // topMargin.getAccessibleContext().setAccessibleName(
            //                                   getMsg("label.topmargin"));
            bottomMargin = new JFormattedTextField(nf);
            bottomMargin.setHorizontalAlignment(JTextField.CENTER);
            bottomMargin.addFocusListener(this);
            bottomMargin.addActionListener(this);
            // bottomMargin.getAccessibleContext().setAccessibleName(
            //                                 getMsg("label.bottommargin"));
            c.gridwidth = GridBagConstraints.RELATIVE;
            // lblStr = getMsg("label.leftmargin")+" "+unitStr;
            lblStr = Util.getLabel("_left", "Left")+unitStr;
            lblLeft = new JLabel(lblStr, JLabel.LEADING);
            // lblLeft.setDisplayedMnemonic(getMnemonic("label.leftmargin"));
            lblLeft.setLabelFor(leftMargin);
            addToGB(lblLeft, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            lblStr = Util.getLabel("_right", "Right")+unitStr;
            lblRight = new JLabel(lblStr, JLabel.LEADING);
            lblRight.setLabelFor(rightMargin);
            addToGB(lblRight, this, gridbag, c);

            c.gridwidth = GridBagConstraints.RELATIVE;
            addToGB(leftMargin, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(rightMargin, this, gridbag, c);

            // add an invisible spacing component.
            addToGB(new JPanel(), this, gridbag, c);

            c.gridwidth = GridBagConstraints.RELATIVE;
            lblStr = Util.getLabel("_top", "Top")+unitStr;
            lblTop = new JLabel(lblStr, JLabel.LEADING);
            addToGB(lblTop, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            lblStr = Util.getLabel("_bottom", "Bottom")+unitStr;
            lblBottom = new JLabel(lblStr, JLabel.LEADING);
            // lblBottom.setDisplayedMnemonic(getMnemonic("label.bottommargin"));
            lblBottom.setLabelFor(bottomMargin);
            addToGB(lblBottom, this, gridbag, c);

            c.gridwidth = GridBagConstraints.RELATIVE;
            addToGB(topMargin, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(bottomMargin, this, gridbag, c);

            Float fv;
            if (units == MediaPrintableArea.MM)
                fv = new Float(20.0f);
            else
                fv = new Float(1.0f);
            leftMargin.setValue(fv);
            rightMargin.setValue(fv);
            topMargin.setValue(fv);
            bottomMargin.setValue(fv);
        }

        public void actionPerformed(ActionEvent e) {
            // Object source = e.getSource();
            // updateMargins(source);
        }

        public void focusLost(FocusEvent e) {
            // Object source = e.getSource();
            // updateMargins(source);
        }

        public void focusGained(FocusEvent e) {}

        /* Get the numbers, use to create a MPA.
         * If its valid, accept it and update the attribute set.
         * If its not valid, then reject it and call updateInfo()
         * to re-establish the previous entries.
         */
        /**********
        public void updateMargins(Object source) {
            if (!(source instanceof JFormattedTextField)) {
                return;
            } else {
                JFormattedTextField tf = (JFormattedTextField)source;
                Float val = (Float)tf.getValue();
                if (val == null) {
                    return;
                }
                if (tf == leftMargin && val.equals(lmObj)) {
                    return;
                }
                if (tf == rightMargin && val.equals(rmObj)) {
                    return;
                }
                if (tf == topMargin && val.equals(tmObj)) {
                    return;
                }
                if (tf == bottomMargin && val.equals(bmObj)) {
                    return;
                }
            }
            if (psCurrent == null)
                return;

            Float lmTmpObj = (Float)leftMargin.getValue();
            Float rmTmpObj = (Float)rightMargin.getValue();
            Float tmTmpObj = (Float)topMargin.getValue();
            Float bmTmpObj = (Float)bottomMargin.getValue();

            float lm = lmTmpObj.floatValue();
            float rm = rmTmpObj.floatValue();
            float tm = tmTmpObj.floatValue();
            float bm = bmTmpObj.floatValue();

            // adjust for orientation
            Class<OrientationRequested> orCategory = OrientationRequested.class;
            OrientationRequested or = OrientationRequested.LANDSCAPE;
            or = (OrientationRequested)asCurrent.get(orCategory);

            if (or == null) {
               or = (OrientationRequested)VjPrintUtil.getDefaultAttributeValue(
                                  psCurrent, orCategory);
            }

            float tmp;
            if (or == OrientationRequested.REVERSE_PORTRAIT) {
                tmp = lm; lm = rm; rm = tmp;
                tmp = tm; tm = bm; bm = tmp;
            } else if (or == OrientationRequested.LANDSCAPE) {
                tmp = lm;
                lm = tm;
                tm = rm;
                rm = bm;
                bm = tmp;
            } else if (or == OrientationRequested.REVERSE_LANDSCAPE) {
                tmp = lm;
                lm = bm;
                bm = rm;
                rm = tm;
                tm = tmp;
            }
            MediaPrintableArea mpa;
            if ((mpa = validateMargins(lm, rm, tm, bm)) != null) {
                asCurrent.add(mpa);
                lmVal = lm;
                rmVal = rm;
                tmVal = tm;
                bmVal = bm;
                lmObj = lmTmpObj;
                rmObj = rmTmpObj;
                tmObj = tmTmpObj;
                bmObj = bmTmpObj;
            } else {
                if (lmObj == null || rmObj == null ||
                    tmObj == null || rmObj == null) {
                    return;
                } else {
                    leftMargin.setValue(lmObj);
                    rightMargin.setValue(rmObj);
                    topMargin.setValue(tmObj);
                    bottomMargin.setValue(bmObj);

                }
            }
        }
        **********/

        /*
         * This method either accepts the values and creates a new
         * MediaPrintableArea, or does nothing.
         * It should not attempt to create a printable area from anything
         * other than the exact values passed in.
         * But REMIND/TBD: it would be user friendly to replace margins the
         * user entered but are out of bounds with the minimum.
         * At that point this method will need to take responsibility for
         * updating the "stored" values and the UI.
         */
         /****************
        private MediaPrintableArea validateMargins(float lm, float rm,
                                                   float tm, float bm) {

            Class<MediaPrintableArea> mpaCategory = MediaPrintableArea.class;
            MediaPrintableArea mpaMax = null;
            MediaSize mediaSize = null;
            Media media = null;

            if (psCurrent == null)
                return null;
            media = (Media)asCurrent.get(Media.class);
            if (media == null || !(media instanceof MediaSizeName)) {
                media = (Media)VjPrintUtil.getDefaultAttributeValue(
                                  psCurrent, Media.class);
            }
            if (media != null && (media instanceof MediaSizeName)) {
                MediaSizeName msn = (MediaSizeName)media;
                mediaSize = MediaSize.getMediaSizeForName(msn);
            }
            if (mediaSize == null) {
                mediaSize = new MediaSize(8.5f, 11f, Size2DSyntax.INCH);
            }

            if (media != null && psCurrent != null) {
                PrintRequestAttributeSet tmpASet =
                    new HashPrintRequestAttributeSet(asCurrent);
                tmpASet.add(media);

                Object values =
                    VjPrintUtil.getAttributeValues(psCurrent,mpaCategory,
                                          docFlavor, tmpASet);
                if (values instanceof MediaPrintableArea[] &&
                    ((MediaPrintableArea[])values).length > 0) {
                    mpaMax = ((MediaPrintableArea[])values)[0];

                }
            }
            if (mpaMax == null) {
                mpaMax = new MediaPrintableArea(0f, 0f,
                                                mediaSize.getX(units),
                                                mediaSize.getY(units),
                                                units);
            }

            float wid = mediaSize.getX(units);
            float hgt = mediaSize.getY(units);
            float pax = lm;
            float pay = tm;
            float paw = wid - lm - rm;
            float pah = hgt - tm - bm;

            if (paw <= 0f || pah <= 0f || pax < 0f || pay < 0f ||
                pax < mpaMax.getX(units) || paw > mpaMax.getWidth(units) ||
                pay < mpaMax.getY(units) || pah > mpaMax.getHeight(units)) {
                return null;
            } else {
                return new MediaPrintableArea(lm, tm, paw, pah, units);
            }
        }
        ****************/

        /* This is complex as a MediaPrintableArea is valid only within
         * a particular context of media size.
         * So we need a MediaSize as well as a MediaPrintableArea.
         * MediaSize can be obtained from MediaSizeName.
         * If the application specifies a MediaPrintableArea, we accept it
         * to the extent its valid for the Media they specify. If they
         * don't specify a Media, then the default is assumed.
         *
         * If an application doesn't define a MediaPrintableArea, we need to
         * create a suitable one, this is created using the specified (or
         * default) Media and default 1 inch margins. This is validated
         * against the paper in case this is too large for tiny media.
         */
        //  class MarginsPanel
        public void updateInfo() {

            if (isAWT) {
                leftMargin.setEnabled(false);
                rightMargin.setEnabled(false);
                topMargin.setEnabled(false);
                bottomMargin.setEnabled(false);
                lblLeft.setEnabled(false);
                lblRight.setEnabled(false);
                lblTop.setEnabled(false);
                lblBottom.setEnabled(false);
                return;
            }

            Class<MediaPrintableArea> mpaCategory = MediaPrintableArea.class;
            MediaPrintableArea mpa = null;
            MediaPrintableArea mpaMax = null;
            MediaSize mediaSize = null;
            Media media = null;

            if (psCurrent == null)
                return;
            mpa = (MediaPrintableArea)asCurrent.get(mpaCategory);
            media = (Media)asCurrent.get(Media.class);
            if (media == null || !(media instanceof MediaSizeName)) {
                media = (Media)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, Media.class);
            }
            if (media != null && (media instanceof MediaSizeName)) {
                MediaSizeName msn = (MediaSizeName)media;
                mediaSize = MediaSize.getMediaSizeForName(msn);
            }
            if (mediaSize == null) {
                mediaSize = new MediaSize(8.5f, 11f, Size2DSyntax.INCH);
            }

            if (media != null && psCurrent != null) {
                PrintRequestAttributeSet tmpASet =
                    new HashPrintRequestAttributeSet(asCurrent);
                tmpASet.add(media);

                Object values =
                    VjPrintUtil.getAttributeValues(psCurrent, mpaCategory,
                                         docFlavor, tmpASet);
                if (values instanceof MediaPrintableArea[] &&
                    ((MediaPrintableArea[])values).length > 0) {
                    mpaMax = ((MediaPrintableArea[])values)[0];

                } else if (values instanceof MediaPrintableArea) {
                    mpaMax = (MediaPrintableArea)values;
                }
            }
            if (mpaMax == null) {
                mpaMax = new MediaPrintableArea(0f, 0f,
                                                mediaSize.getX(units),
                                                mediaSize.getY(units),
                                                units);
            }

            /*
             * At this point we now know as best we can :-
             * - the media size
             * - the maximum corresponding printable area
             * - the media printable area specified by the client, if any.
             * The next step is to create a default MPA if none was specified.
             * 1" margins are used unless they are disproportionately
             * large compared to the size of the media.
             */

            float wid = mediaSize.getX(MediaPrintableArea.INCH);
            float hgt = mediaSize.getY(MediaPrintableArea.INCH);
            float maxMarginRatio = 5f;
            float xMgn, yMgn;
            if (wid > maxMarginRatio) {
                xMgn = 1f;
            } else {
                xMgn = wid / maxMarginRatio;
            }
            if (hgt > maxMarginRatio) {
                yMgn = 1f;
            } else {
                yMgn = hgt / maxMarginRatio;
            }

            if (mpa == null) {
                mpa = new MediaPrintableArea(xMgn, yMgn,
                                             wid-(2*xMgn), hgt-(2*yMgn),
                                             MediaPrintableArea.INCH);
                asCurrent.add(mpa);
            }
            float pax = mpa.getX(units);
            float pay = mpa.getY(units);
            float paw = mpa.getWidth(units);
            float pah = mpa.getHeight(units);
            float paxMax = mpaMax.getX(units);
            float payMax = mpaMax.getY(units);
            float pawMax = mpaMax.getWidth(units);
            float pahMax = mpaMax.getHeight(units);


            boolean invalid = false;

            // If the paper is set to something which is too small to
            // accommodate a specified printable area, perhaps carried
            // over from a larger paper, the adjustment that needs to be
            // performed should seem the most natural from a user's viewpoint.
            // Since the user is specifying margins, then we are biased
            // towards keeping the margins as close to what is specified as
            // possible, shrinking or growing the printable area.
            // But the API uses printable area, so you need to know the
            // media size in which the margins were previously interpreted,
            // or at least have a record of the margins.
            // In the case that this is the creation of this UI we do not
            // have this record, so we are somewhat reliant on the client
            // to supply a reasonable default
            wid = mediaSize.getX(units);
            hgt = mediaSize.getY(units);
            if (lmVal >= 0f) {
                invalid = true;

                if (lmVal + rmVal > wid) {
                    // margins impossible, but maintain P.A if can
                    if (paw > pawMax) {
                        paw = pawMax;
                    }
                    // try to centre the printable area.
                    pax = (wid - paw)/2f;
                } else {
                    pax = (lmVal >= paxMax) ? lmVal : paxMax;
                    paw = wid - pax - rmVal;
                }
                if (tmVal + bmVal > hgt) {
                    if (pah > pahMax) {
                        pah = pahMax;
                    }
                    pay = (hgt - pah)/2f;
                } else {
                    pay = (tmVal >= payMax) ? tmVal : payMax;
                    pah = hgt - pay - bmVal;
                }
            }
            if (pax < paxMax) {
                invalid = true;
                pax = paxMax;
            }
            if (pay < payMax) {
                invalid = true;
                pay = payMax;
            }
            if (paw > pawMax) {
                invalid = true;
                paw = pawMax;
            }
            if (pah > pahMax) {
                invalid = true;
                pah = pahMax;
            }

            if ((pax + paw > paxMax + pawMax) || (paw <= 0f)) {
                invalid = true;
                pax = paxMax;
                paw = pawMax;
            }
            if ((pay + pah > payMax + pahMax) || (pah <= 0f)) {
                invalid = true;
                pay = payMax;
                pah = pahMax;
            }

            if (invalid) {
                mpa = new MediaPrintableArea(pax, pay, paw, pah, units);
                asCurrent.add(mpa);
            }

            /* We now have a valid printable area.
             * Turn it into margins, using the mediaSize
             */
            lmVal = pax;
            tmVal = pay;
            rmVal = mediaSize.getX(units) - pax - paw;
            bmVal = mediaSize.getY(units) - pay - pah;

            lmObj = new Float(lmVal);
            rmObj = new Float(rmVal);
            tmObj = new Float(tmVal);
            bmObj = new Float(bmVal);

            /* Now we know the values to use, we need to assign them
             * to the fields appropriate for the orientation.
             * Note: if orientation changes this method must be called.
             */
            Class<OrientationRequested> orCategory = OrientationRequested.class;
            OrientationRequested or = null;
            or = (OrientationRequested)asCurrent.get(orCategory);

            if (or == null) {
                or = (OrientationRequested)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, orCategory);
                if (or == null)
                    or = OrientationRequested.REVERSE_LANDSCAPE;
            }

            Float tmp;

            if (or == OrientationRequested.REVERSE_PORTRAIT) {
                tmp = lmObj; lmObj = rmObj; rmObj = tmp;
                tmp = tmObj; tmObj = bmObj; bmObj = tmp;
            }  else if (or == OrientationRequested.LANDSCAPE) {
                tmp = lmObj;
                lmObj = bmObj;
                bmObj = rmObj;
                rmObj = tmObj;
                tmObj = tmp;
            }  else if (or == OrientationRequested.REVERSE_LANDSCAPE) {
                tmp = lmObj;
                lmObj = tmObj;
                tmObj = rmObj;
                rmObj = bmObj;
                bmObj = tmp;
            }
/**
            leftMargin.setValue(lmObj);
            rightMargin.setValue(rmObj);
            topMargin.setValue(tmObj);
            bottomMargin.setValue(bmObj);
**/
        }
    }

    private class MediaPanel extends JPanel implements ItemListener {

        // private String strTitle = getMsg("border.media");
        private String strTitle;
        private JLabel lblSize, lblSource;
        private JLabel lblFormat;
        // private JComboBox cbSize, cbSource;
        private Vector<MediaSizeName> sizes = new Vector<MediaSizeName>();
        private Vector<Media> sources = new Vector<Media>();
        private MarginsPanel pnlMargins = null;

        public MediaPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Media", "Media");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            cbSize = new JComboBox();
            cbSource = new JComboBox();
            cbFormat = new JComboBox();

            c.fill = GridBagConstraints.BOTH;
            c.insets = compInsets;
            c.weighty = 1.0;

            c.weightx = 0.0;
            // lblSize = new JLabel(getMsg("label.size"), JLabel.TRAILING);
            lblStr = Util.getLabel("_Paper_Size", "Paper Size")+":";
            lblSize = new JLabel(lblStr, JLabel.TRAILING);
            // lblSize.setDisplayedMnemonic(getMnemonic("label.size"));
            lblSize.setLabelFor(cbSize);
            addToGB(lblSize, this, gridbag, c);
            c.weightx = 1.0;
            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(cbSize, this, gridbag, c);

            c.weightx = 0.0;
            c.gridwidth = 1;
            // lblSource = new JLabel(getMsg("label.source"), JLabel.TRAILING);
            lblStr = Util.getLabel("_Paper_Source", "Paper Source")+":";
            lblSource = new JLabel(lblStr, JLabel.TRAILING);
            // lblSource.setDisplayedMnemonic(getMnemonic("label.source"));
            lblSource.setLabelFor(cbSource);
            addToGB(lblSource, this, gridbag, c);
            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(cbSource, this, gridbag, c);

            c.weightx = 0.0;
            c.gridwidth = 1;
            lblStr = Util.getLabel("_Format", "Format")+":";
            lblFormat = new JLabel(lblStr, JLabel.TRAILING);
            lblFormat.setLabelFor(cbFormat);
            addToGB(lblFormat, this, gridbag, c);
            c.weightx = 1.0;
            c.gridwidth = GridBagConstraints.REMAINDER;
            addToGB(cbFormat, this, gridbag, c);
            int n;
            for (n = 0; n < formatLabels.length; n++)
                cbFormat.addItem(formatLabels[n]);
            MediaSize mediaSizes[] = VjPaperMedia.paperMedium;
            String mediaNames[] = VjPaperMedia.paperNames;
            for (n = 0; n < mediaSizes.length; n++) {
                MediaSize ms = mediaSizes[n];
                if (ms != null) {
                    String name = VjPrintUtil.getMediaResource(
                           ms.getMediaSizeName().toString(), mediaNames[n]);
                    VjMediaSizeObj obj = new VjMediaSizeObj(ms, name);
                    cbSize.addItem(obj);
                    sizes.add(ms.getMediaSizeName());
                }
            }
            cbSize.addItemListener(this);
            if (cbSize.getMaximumRowCount() < 9)
                cbSize.setMaximumRowCount(9);
        }

        private String getMediaName(String key) {
            try {
                // replace characters that would be invalid in
                // a resource key with valid characters
                String newkey = key.replace(' ', '-');
                newkey = newkey.replace('#', 'n');

                return getMsg(newkey);
            } catch (java.util.MissingResourceException e) {
                return key;
            }
        }

        public void itemStateChanged(ItemEvent e) {
            Object source = e.getSource();

            if (psCurrent == null)
                return;
            if (e.getStateChange() == ItemEvent.SELECTED) {
                if (source == cbSize) {
                    int index = cbSize.getSelectedIndex();

                    if ((index >= 0) && (index < sizes.size())) {
                        if ((cbSource.getItemCount() > 1) &&
                            (cbSource.getSelectedIndex() >= 1))
                        {
                            int src = cbSource.getSelectedIndex() - 1;
                            MediaTray mt = (MediaTray)sources.get(src);
                            asCurrent.add(new SunAlternateMedia(mt));
                        }
                        asCurrent.add(sizes.get(index));
                    }
                } else if (source == cbSource) {
                    int index = cbSource.getSelectedIndex();

                    if ((index >= 1) && (index < (sources.size() + 1))) {
                       asCurrent.remove(SunAlternateMedia.class);
                       MediaTray newTray = (MediaTray)sources.get(index - 1);
                       Media m = (Media)asCurrent.get(Media.class);
                       if (m == null || m instanceof MediaTray) {
                           asCurrent.add(newTray);
                       } else if (m instanceof MediaSizeName) {
                           MediaSizeName msn = (MediaSizeName)m;
                           Media def = (Media)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, Media.class);
                           if (def instanceof MediaSizeName && def.equals(msn)) {
                               asCurrent.add(newTray);
                           } else {
                               /* Non-default paper size, so need to store tray
                                * as SunAlternateMedia
                                */
                               asCurrent.add(new SunAlternateMedia(newTray));
                           }
                       }
                    } else if (index == 0) {
                        asCurrent.remove(SunAlternateMedia.class);
                        if (cbSize.getItemCount() > 0) {
                            int size = cbSize.getSelectedIndex();
                            asCurrent.add(sizes.get(size));
                        }
                    }
                }
            // orientation affects display of margins.
                if (pnlMargins != null) {
                     pnlMargins.updateInfo();
                }
            }
        }


        /* this is ad hoc to keep things simple */
        public void addMediaListener(MarginsPanel pnl) {
            pnlMargins = pnl;
        }

        private boolean isNewMedia(String name, Vector<MediaSizeName> v) {
            for (int i = 0; i < v.size(); i++) {
                Media m = v.get(i);
                if (m != null) {
                    String s = getMediaName(m.toString());
                    if (name.equals(s))
                        return false;
                }
            }
            return true;
        }
                
 
		public void updateInfo() {
            Class<Media> mdCategory = Media.class;
            Class<SunAlternateMedia> amCategory = SunAlternateMedia.class;
            boolean mediaSupported = false;

            if (psCurrent == null)
                return;
            mediaSupported = VjPrintUtil.isCategorySupported(
                                  psCurrent,mdCategory);
            if (!mediaSupported)
                return;

            // cbSize.removeItemListener(this);
            // cbSize.removeAllItems();

            cbSource.removeItemListener(this);
            cbSource.removeAllItems();
            cbSource.addItem(getMediaName("auto-select"));

            // sizes.clear();
            sources.clear();

            if (mediaSupported) {
                Object values =
                    VjPrintUtil.getAttributeValues(psCurrent, mdCategory,
                                        docFlavor, asCurrent);
                if (values instanceof Media[]) {
                    Media[] media = (Media[])values;

                    for (int i = 0; i < media.length; i++) {
                        Media medium = media[i];

                        if (medium instanceof MediaSizeName) {
                            String name = getMediaName(medium.toString());
                            if (isNewMedia(name, sizes)) {
                                // sizes.add(medium);
                                // cbSize.addItem(name);
                            }
                        } else if (medium instanceof MediaTray) {
                            sources.add(medium);
                            cbSource.addItem(getMediaName(medium.toString()));
                        }
                    }
                }
            }

            boolean msSupported = (mediaSupported && (sizes.size() > 0));
            lblSize.setEnabled(msSupported);
            cbSize.setEnabled(msSupported);

            if (isAWT) {
                cbSource.setEnabled(false);
                lblSource.setEnabled(false);
            } else {
                cbSource.setEnabled(mediaSupported);
            }

            if (mediaSupported) {
                Media medium = (Media)asCurrent.get(mdCategory);

               // initialize size selection to default
                Media defMedia = (Media)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, mdCategory);
                if (defMedia instanceof MediaSizeName) {
                    // cbSize.setSelectedIndex(sizes.size() > 0 ? sizes.indexOf(defMedia) : -1);
                }

                if (medium == null ||
                    !VjPrintUtil.isValueSupported(psCurrent, medium,
                                             docFlavor, asCurrent)) {

                    medium = defMedia;

                    if (medium == null) {
                        if (sizes.size() > 0) {
                            medium = sizes.get(0);
                        }
                    }
                    if (medium != null)
                        asCurrent.add(medium);
                }
                if (medium != null) {
                    if (medium instanceof MediaSizeName) {
                        // MediaSizeName ms = (MediaSizeName)medium;
                        // cbSize.setSelectedIndex(sizes.indexOf(ms));
                    } else if (medium instanceof MediaTray) {
                        MediaTray mt = (MediaTray)medium;
                        cbSource.setSelectedIndex(sources.indexOf(mt) + 1);
                    }
                } else {
                    // cbSize.setSelectedIndex(sizes.size() > 0 ? 0 : -1);
                    cbSource.setSelectedIndex(0);
                }

                SunAlternateMedia alt = (SunAlternateMedia)asCurrent.get(amCategory);
                if (alt != null) {
                    Media md = alt.getMedia();
                    if (md instanceof MediaTray) {
                        MediaTray mt = (MediaTray)md;
                        cbSource.setSelectedIndex(sources.indexOf(mt) + 1);
                    }
                }

                int selIndex = cbSize.getSelectedIndex();
                if ((selIndex >= 0) && (selIndex < sizes.size())) {
                  asCurrent.add(sizes.get(selIndex));
                }

                selIndex = cbSource.getSelectedIndex();
                if ((selIndex >= 1) && (selIndex < (sources.size()+1))) {
                    MediaTray mt = (MediaTray)sources.get(selIndex-1);
                    if (medium instanceof MediaTray) {
                        asCurrent.add(mt);
                    } else {
                        asCurrent.add(new SunAlternateMedia(mt));
                    }
                }
            }
           // cbSize.addItemListener(this);
            cbSource.addItemListener(this);
        }
    }

    private class OrientationPanel extends JPanel
        implements ActionListener
    {
        // private String strTitle = getMsg("border.orientation");
        private String strTitle;
        private MarginsPanel pnlMargins = null;
        private boolean needUpdate = false;

        public OrientationPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Orientation", "Orientation");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            c.fill = GridBagConstraints.BOTH;
            c.insets = compInsets;
            c.weighty = 1.0;
            c.gridwidth = GridBagConstraints.REMAINDER;

            ButtonGroup bg = new ButtonGroup();
            lblStr = Util.getLabel("_Portrait", "Portrait");
            rbPortrait = new IconRadioButton(lblStr,
                                             "printPortrait.png", true,
                                             bg, this);
            rbPortrait.addActionListener(this);
            addToGB(rbPortrait, this, gridbag, c);

            lblStr = Util.getLabel("_Landscape", "Landscape");
            rbLandscape = new IconRadioButton(lblStr,
                                              "printLandscape.png", false,
                                              bg, this);
            rbLandscape.addActionListener(this);
            addToGB(rbLandscape, this, gridbag, c);

            /********
            // lblStr = getMsg("radiobutton.revportrait");
            lblStr = Util.getLabel("_Reverse_Portrait", "Reverse Portrait");
            rbRevPortrait = new IconRadioButton(lblStr,
                                                "orientRevPortrait.png", false,
                                                bg, this);
            rbRevPortrait.addActionListener(this);
            addToGB(rbRevPortrait, this, gridbag, c);

            // lblStr = getMsg("radiobutton.revlandscape");
            lblStr = Util.getLabel("_Reverse_Landscape", "Reverse Landscape");
            rbRevLandscape = new IconRadioButton(lblStr,
                                                 "orientRevLandscape.png", false,
                                                 bg, this);
            rbRevLandscape.addActionListener(this);
            addToGB(rbRevLandscape, this, gridbag, c);
            ********/
        }

        public void actionPerformed(ActionEvent e) {
            Object source = e.getSource();

            if (rbPortrait.isSameAs(source)) {
                asCurrent.add(OrientationRequested.PORTRAIT);
            } else if (rbLandscape.isSameAs(source)) {
                asCurrent.add(OrientationRequested.LANDSCAPE);
            } else if (rbRevPortrait.isSameAs(source)) {
                asCurrent.add(OrientationRequested.REVERSE_PORTRAIT);
            } else if (rbRevLandscape.isSameAs(source)) {
                asCurrent.add(OrientationRequested.REVERSE_LANDSCAPE);
            }
            // orientation affects display of margins.
            if (pnlMargins != null) {
                pnlMargins.updateInfo();
            }
        }

        /* This is ad hoc to keep things simple */
        void addOrientationListener(MarginsPanel pnl) {
            pnlMargins = pnl;
        }

        public void updateInfo() {
            Class<OrientationRequested> orCategory = OrientationRequested.class;
            boolean pSupported = true;
            boolean lSupported = true;
            boolean rpSupported = false;
            boolean rlSupported = false;
            boolean bOrientSet = false;

            if (!needUpdate)
                return;
            if (psCurrent == null)
                return;
            // bOrientSet = VjPrintUtil.isCategorySupported(psCurrent,orCategory);
            if (bOrientSet) {
                Object values =
                    VjPrintUtil.getAttributeValues(psCurrent, orCategory,
                                       docFlavor, asCurrent);

                if (values instanceof OrientationRequested[]) {
                    OrientationRequested[] ovalues =
                        (OrientationRequested[])values;

                    for (int i = 0; i < ovalues.length; i++) {
                        OrientationRequested value = ovalues[i];

                        if (value == OrientationRequested.PORTRAIT) {
                            pSupported = true;
                        } else if (value == OrientationRequested.LANDSCAPE) {
                            lSupported = true;
                        } else if (value == OrientationRequested.REVERSE_PORTRAIT) {
                            rpSupported = true;
                        } else if (value == OrientationRequested.REVERSE_LANDSCAPE) {
                            rlSupported = true;
                        }
                    }
                }

                rbPortrait.setEnabled(pSupported);
                rbLandscape.setEnabled(lSupported);
                rbRevPortrait.setEnabled(rpSupported);
                rbRevLandscape.setEnabled(rlSupported);

                OrientationRequested or = (OrientationRequested)asCurrent.get(orCategory);
                if (or == null ||
                    !VjPrintUtil.isValueSupported(psCurrent, or, docFlavor, asCurrent)) {

                    or = (OrientationRequested)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, orCategory);
                    // need to validate if default is not supported
                    if (!VjPrintUtil.isValueSupported(psCurrent, or, docFlavor, asCurrent)) {
                        or = null;
                        values =
                            VjPrintUtil.getAttributeValues(psCurrent,
                                         orCategory, docFlavor, asCurrent);
                        if (values instanceof OrientationRequested[]) {
                            OrientationRequested[] orValues =
                                                (OrientationRequested[])values;
                            if (orValues.length > 1) {
                                // get the first in the list
                                or = orValues[0];
                            }
                        }
                    }

                    if (or == null) {
                        or = OrientationRequested.PORTRAIT;
                    }
                    asCurrent.add(or);
                }

                if (or == OrientationRequested.PORTRAIT)
                    rbPortrait.setSelected(true);
                else
                    rbLandscape.setSelected(true);
/**
                if (or == OrientationRequested.PORTRAIT) {
                    rbPortrait.setSelected(true);
                } else if (or == OrientationRequested.LANDSCAPE) {
                    rbLandscape.setSelected(true);
                } else if (or == OrientationRequested.REVERSE_PORTRAIT) {
                    rbRevPortrait.setSelected(true);
                } else {  if (or == OrientationRequested.REVERSE_LANDSCAPE)
                    rbRevLandscape.setSelected(true);
                }
**/
	    } else { 
                rbPortrait.setEnabled(pSupported);
                rbLandscape.setEnabled(lSupported);
                rbRevPortrait.setEnabled(rpSupported);
                rbRevLandscape.setEnabled(rlSupported);
            }
        }
    }



    /**
     * The "Appearance" tab.  Includes the controls for Chromaticity,
     * PrintQuality, JobPriority, JobName, and other related job attributes.
     */
    private class AppearancePanel extends JPanel {

        private ChromaticityPanel pnlChromaticity;
        private QualityPanel pnlQuality;
        private JobAttributesPanel pnlJobAttributes;
        
        public AppearancePanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);

            c.fill = GridBagConstraints.BOTH;
            c.insets = panelInsets;
            c.weightx = 1.0;
            c.weighty = 1.0;

            c.gridwidth = GridBagConstraints.RELATIVE;
            pnlChromaticity = new ChromaticityPanel();
            addToGB(pnlChromaticity, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            pnlQuality = new QualityPanel();
            addToGB(pnlQuality, this, gridbag, c);

            c.gridwidth = 1;
            // pnlSides = new SidesPanel();

            //addToGB(pnlSides, this, gridbag, c);

            c.gridwidth = GridBagConstraints.REMAINDER;
            pnlJobAttributes = new JobAttributesPanel();
            // addToGB(pnlJobAttributes, this, gridbag, c);

            lblStr = Util.getLabel("_Keep_cursor_lines", "Keep cursor lines");
            cbCursorLines = new JCheckBox(lblStr);
            addToGB(cbCursorLines, this, gridbag, c);

            lblStr = Util.getLabel("_Reshape_to_fit_page", "Reshape to fit page");
            cbFitPaper = new JCheckBox(lblStr);
            addToGB(cbFitPaper, this, gridbag, c);

        }

        public void updateInfo() {
            pnlChromaticity.updateInfo();
            pnlQuality.updateInfo();
            // pnlSides.updateInfo();
            pnlJobAttributes.updateInfo();
        }
    }

    private class ChromaticityPanel extends JPanel
        implements ActionListener
    {
        // private String strTitle = "Appearance";
        private String strTitle;

        public ChromaticityPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Appearance", "Appearance");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            c.fill = GridBagConstraints.BOTH;
            c.gridwidth = GridBagConstraints.REMAINDER;
            c.weighty = 1.0;

            ButtonGroup bg = new ButtonGroup();
            // rbMonochrome = createRadioButton("radiobutton.monochrome", this);
            lblStr = Util.getLabel("_Monochrome", "Monochrome");
            rbMonochrome = createRadioButton(lblStr, this);
            // rbMonochrome.setSelected(true);
            bg.add(rbMonochrome);
            addToGB(rbMonochrome, this, gridbag, c);
            // rbColor = createRadioButton("radiobutton.color", this);
            lblStr = Util.getLabel("_Color", "Color");
            rbColor = createRadioButton(lblStr, this);
            bg.add(rbColor);
            addToGB(rbColor, this, gridbag, c);

            JPanel panb = new JPanel();
            panb.setLayout(new SimpleHLayout());
            lblStr = Util.getLabel("_Background_Color", "Background Color")+":";
            JLabel label;
            bgColorLabel = new JLabel(lblStr);
            panb.add(bgColorLabel);
            bgColorDialog = new VColorChooser(null, null, null);
            bgColorDialog.setLocation(170, 0);
            panb.add(bgColorDialog);
            addToGB(panb, this, gridbag, c);

            JPanel panf = new JPanel();
            panf.setLayout(new SimpleHLayout());
            lblStr = Util.getLabel("_Foreground_Color", "Foreground Color")+":";
            fgColorLabel = new JLabel(lblStr);
            panf.add(fgColorLabel);
            fgColorDialog = new VColorChooser(null, null, null);
            fgColorDialog.setLocation(170, 0);
            panf.add(fgColorDialog);
            addToGB(panf, this, gridbag, c);

            JPanel pansp = new JPanel();
            pansp.setLayout(new SimpleHLayout());
            lblStr = Util.getLabel("_Spectrum_Line_Width", "Spectrum Line Width")+":";
            label = new JLabel(lblStr);
            pansp.add(label);
            spectrumWidth = new JTextField("1", 4);
            spectrumWidth.setHorizontalAlignment(JTextField.CENTER);
            spectrumWidth.setLocation(170, 0);
            pansp.add(spectrumWidth);
            lblStr = Util.getLabel("_pixels", "pixels");
            label = new JLabel(lblStr);
            pansp.add(label);
            addToGB(pansp, this, gridbag, c);

            JPanel panlp = new JPanel();
            panlp.setLayout(new SimpleHLayout());
            lblStr = Util.getLabel("_Graphics_Line_Width", "Graphics Line Width")+":";
            label = new JLabel(lblStr);
            panlp.add(label);
            lineWidth = new JTextField("1", 4);
            lineWidth.setHorizontalAlignment(JTextField.CENTER);
            lineWidth.setLocation(170, 0);
            panlp.add(lineWidth);
            lblStr = Util.getLabel("_pixels", "pixels");
            label = new JLabel(lblStr);
            panlp.add(label);
            addToGB(panlp, this, gridbag, c);

            rbMonochrome.setSelected(true);
            setColorChoice();
        }

        public void actionPerformed(ActionEvent e) {
            Object source = e.getSource();

            // REMIND: use isSameAs if we move to a IconRB in the future
            if (source == rbMonochrome) {
                asCurrent.add(Chromaticity.MONOCHROME);
            } else if (source == rbColor) {
                asCurrent.add(Chromaticity.COLOR);
            }
            setColorChoice();
        }

        public void updateInfo() {
            Class<Chromaticity> chCategory = Chromaticity.class;
            boolean monoSupported = true;
            boolean colorSupported = true;
            boolean bChromaticity = false;

            if (psCurrent == null)
                return;
            // bChromaticity = VjPrintUtil.isCategorySupported(psCurrent,chCategory);
            if (bChromaticity) {
                Object values =
                    VjPrintUtil.getAttributeValues(psCurrent, chCategory,
                                          docFlavor, asCurrent);

                if (values instanceof Chromaticity[]) {
                    Chromaticity[] cvalues = (Chromaticity[])values;

                    for (int i = 0; i < cvalues.length; i++) {
                        Chromaticity value = cvalues[i];

                        if (value == Chromaticity.MONOCHROME) {
                            monoSupported = true;
                        } else if (value == Chromaticity.COLOR) {
                            colorSupported = true;
                        }
                    }
                }
            }

            rbMonochrome.setEnabled(monoSupported);
            rbColor.setEnabled(colorSupported);

            Chromaticity ch = (Chromaticity)asCurrent.get(chCategory);
            if (ch == null) {
                ch = (Chromaticity)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, chCategory);
            }
            if (ch == null)
               ch = Chromaticity.COLOR;

            if (ch == Chromaticity.MONOCHROME) {
                rbMonochrome.setSelected(true);
            } else { // if (ch == Chromaticity.COLOR)
                rbColor.setSelected(true);
            }
            setColorChoice();
        }
    }

    private class QualityPanel extends JPanel
        implements ActionListener
    {
        // private String strTitle = getMsg("border.quality");
        private String strTitle;
        // private JRadioButton rbDraft, rbNormal, rbHigh;

        public QualityPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Quality", "Quality");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            c.fill = GridBagConstraints.BOTH;
            c.gridwidth = GridBagConstraints.REMAINDER;
            c.weighty = 1.0;
            c.ipadx = 10;

            ButtonGroup bg = new ButtonGroup();
            // rbDraft = createRadioButton("radiobutton.draftq", this);
            lblStr = Util.getLabel("_Draft", "Draft");
            rbDraft = createRadioButton(lblStr, this);
            bg.add(rbDraft);
            addToGB(rbDraft, this, gridbag, c);
            // rbNormal = createRadioButton("radiobutton.normalq", this);
            lblStr = Util.getLabel("_Normal", "Normal");
            rbNormal = createRadioButton(lblStr, this);
            rbNormal.setSelected(true);
            bg.add(rbNormal);
            addToGB(rbNormal, this, gridbag, c);
            // rbHigh = createRadioButton("radiobutton.highq", this);
            lblStr = Util.getLabel("_High", "High");
            rbHigh = createRadioButton(lblStr, this);
            bg.add(rbHigh);
            addToGB(rbHigh, this, gridbag, c);
        }

        public void actionPerformed(ActionEvent e) {
            Object source = e.getSource();

            if (source == rbDraft) {
                asCurrent.add(PrintQuality.DRAFT);
            } else if (source == rbNormal) {
                asCurrent.add(PrintQuality.NORMAL);
            } else if (source == rbHigh) {
                asCurrent.add(PrintQuality.HIGH);
            }
        }

        public void updateInfo() {
            Class<PrintQuality> pqCategory = PrintQuality.class;
            boolean draftSupported = true;
            boolean normalSupported = true;
            boolean highSupported = true;
            boolean bQuality = false;

            if (psCurrent == null)
                return;
            // bQuality = VjPrintUtil.isCategorySupported(psCurrent, pqCategory);
            if (bQuality) {
                Object values =
                    VjPrintUtil.getAttributeValues(psCurrent, pqCategory,
                                        docFlavor, asCurrent);

                if (values instanceof PrintQuality[]) {
                    PrintQuality[] qvalues = (PrintQuality[])values;

                    for (int i = 0; i < qvalues.length; i++) {
                        PrintQuality value = qvalues[i];

                        if (value == PrintQuality.DRAFT) {
                            draftSupported = true;
                        } else if (value == PrintQuality.NORMAL) {
                            normalSupported = true;
                        } else if (value == PrintQuality.HIGH) {
                            highSupported = true;
                        }
                    }
                }
            }

            rbDraft.setEnabled(draftSupported);
            rbNormal.setEnabled(normalSupported);
            rbHigh.setEnabled(highSupported);

            PrintQuality pq = (PrintQuality)asCurrent.get(pqCategory);
            if (pq == null) {
                pq = (PrintQuality)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, pqCategory);
            }
            if (pq == null)
                pq = PrintQuality.NORMAL;

            if (pq == PrintQuality.DRAFT) {
                rbDraft.setSelected(true);
            } else if (pq == PrintQuality.NORMAL) {
                rbNormal.setSelected(true);
            } else { // if (pq == PrintQuality.HIGH)
                rbHigh.setSelected(true);
            }
        }


    }
    private class JobAttributesPanel extends JPanel
        implements ActionListener, ChangeListener, FocusListener
    {
        // private String strTitle = getMsg("border.jobattributes");
        private String strTitle;
        private JLabel lblPriority, lblJobName, lblUserName;
        private JSpinner spinPriority;
        private SpinnerNumberModel snModel;
        private JCheckBox cbJobSheets;
        private JTextField tfJobName, tfUserName;

        public JobAttributesPanel() {
            super();

            GridBagLayout gridbag = new GridBagLayout();
            GridBagConstraints c = new GridBagConstraints();

            setLayout(gridbag);
            strTitle = Util.getLabel("_Jobs", "Jobs");
            setBorder(VjPrintUtil.createTitledBorder(strTitle));

            c.fill = GridBagConstraints.NONE;
            c.insets = compInsets;
            c.weighty = 1.0;

            lblStr = Util.getLabel("_Job_Sheets", "Job Sheets");
            cbJobSheets = createCheckBox(lblStr, this);
            c.anchor = GridBagConstraints.LINE_START;
            addToGB(cbJobSheets, this, gridbag, c);

            JPanel pnlTop = new JPanel();
            // lblStr = (getMsg("label.priority");
            lblStr = Util.getLabel("_Priority", "Priority");
            lblPriority = new JLabel(lblStr, JLabel.TRAILING);
            // lblPriority.setDisplayedMnemonic(getMnemonic("label.priority"));

            pnlTop.add(lblPriority);
            snModel = new SpinnerNumberModel(1, 1, 100, 1);
            spinPriority = new JSpinner(snModel);
            lblPriority.setLabelFor(spinPriority);
            // REMIND
            ((JSpinner.NumberEditor)spinPriority.getEditor()).getTextField().setColumns(3);
            spinPriority.addChangeListener(this);
            pnlTop.add(spinPriority);
            c.anchor = GridBagConstraints.LINE_END;
            c.gridwidth = GridBagConstraints.REMAINDER;
            pnlTop.getAccessibleContext().setAccessibleName(
                                       getMsg("label.priority"));
            addToGB(pnlTop, this, gridbag, c);

            c.fill = GridBagConstraints.HORIZONTAL;
            c.anchor = GridBagConstraints.CENTER;
            c.weightx = 0.0;
            c.gridwidth = 1;
            char jmnemonic = getMnemonic("label.jobname");
            // lblStr = getMsg("label.jobname");
            lblStr = Util.getLabel("_Job_Name", "Job Name");
            lblJobName = new JLabel(lblStr, JLabel.TRAILING);
            // lblJobName.setDisplayedMnemonic(jmnemonic);
            addToGB(lblJobName, this, gridbag, c);
            c.weightx = 1.0;
            c.gridwidth = GridBagConstraints.REMAINDER;
            tfJobName = new JTextField();
            lblJobName.setLabelFor(tfJobName);
            tfJobName.addFocusListener(this);
            tfJobName.setFocusAccelerator(jmnemonic);
            tfJobName.getAccessibleContext().setAccessibleName(
                                             getMsg("label.jobname"));
            addToGB(tfJobName, this, gridbag, c);

            c.weightx = 0.0;
            c.gridwidth = 1;
            char umnemonic = getMnemonic("label.username");
            // lblStr = getMsg("label.username");
            lblStr = Util.getLabel("_User_Name", "User Name");
            lblUserName = new JLabel(lblStr, JLabel.TRAILING);
            // lblUserName.setDisplayedMnemonic(umnemonic);
            addToGB(lblUserName, this, gridbag, c);
            c.gridwidth = GridBagConstraints.REMAINDER;
            tfUserName = new JTextField();
            lblUserName.setLabelFor(tfUserName);
            tfUserName.addFocusListener(this);
            tfUserName.setFocusAccelerator(umnemonic);
            tfUserName.getAccessibleContext().setAccessibleName(
                                             getMsg("label.username"));
            addToGB(tfUserName, this, gridbag, c);
        }

        public void actionPerformed(ActionEvent e) {
            if (cbJobSheets.isSelected()) {
                asCurrent.add(JobSheets.STANDARD);
            } else {
                asCurrent.add(JobSheets.NONE);
            }
        }

        public void stateChanged(ChangeEvent e) {
            asCurrent.add(new JobPriority(snModel.getNumber().intValue()));
        }

        public void focusLost(FocusEvent e) {
            Object source = e.getSource();

            if (source == tfJobName) {
                asCurrent.add(new JobName(tfJobName.getText(),
                                          Locale.getDefault()));
            } else if (source == tfUserName) {
                asCurrent.add(new RequestingUserName(tfUserName.getText(),
                                                     Locale.getDefault()));
            }
        }

        public void focusGained(FocusEvent e) {}

        // class JobAttributesPanel
        public void updateInfo() {
            Class<JobSheets> jsCategory = JobSheets.class;
            Class<JobPriority> jpCategory = JobPriority.class;
            Class<JobName> jnCategory = JobName.class;
            Class<RequestingUserName> unCategory = RequestingUserName.class;
            boolean jsSupported = false;
            boolean jpSupported = false;
            boolean jnSupported = false;
            boolean unSupported = false;

            if (psCurrent == null)
                return;
            // setup JobSheets checkbox
            jsSupported = VjPrintUtil.isCategorySupported(psCurrent, jsCategory);
            JobSheets js = (JobSheets)asCurrent.get(jsCategory);
            if (js == null) {
                js = (JobSheets)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, jsCategory);
            }
            if (js == null)
                js = JobSheets.NONE;
            cbJobSheets.setSelected(js != JobSheets.NONE);
            cbJobSheets.setEnabled(jsSupported);

            // setup JobPriority spinner
            jpSupported = VjPrintUtil.isCategorySupported(psCurrent,jpCategory);
            JobPriority jp = (JobPriority)asCurrent.get(jpCategory);
            if (jp == null) {
                jp = (JobPriority)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, jpCategory);
            }
            if (jp == null)
               jp = new JobPriority(1);
            int value = jp.getValue();
            if ((value < 1) || (value > 100)) {
                value = 1;
            }
            snModel.setValue(new Integer(value));
            lblPriority.setEnabled(jpSupported);
            spinPriority.setEnabled(jpSupported);

            // setup JobName text field
            jnSupported = VjPrintUtil.isCategorySupported(psCurrent,jnCategory);
            JobName jn = (JobName)asCurrent.get(jnCategory);
            if (jn == null) {
                jn = (JobName)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, jnCategory);
                if (jn == null)
                    jn = new JobName("", Locale.getDefault());
            }
            tfJobName.setText(jn.getValue());
            tfJobName.setEnabled(jnSupported);
            lblJobName.setEnabled(jnSupported);

            // setup RequestingUserName text field
            unSupported = VjPrintUtil.isCategorySupported(psCurrent,unCategory);
            RequestingUserName un = (RequestingUserName)asCurrent.get(unCategory);
            if (un == null) {
                un = (RequestingUserName)VjPrintUtil.getDefaultAttributeValue(
                                    psCurrent, unCategory);
                if (un == null)
                   un = new RequestingUserName("", Locale.getDefault());
            }
            tfUserName.setText(un.getValue());
            tfUserName.setEnabled(unSupported);
            lblUserName.setEnabled(unSupported);
        }
    }

    /**
     * A special widget that groups a JRadioButton with an associated icon,
     * placed to the left of the radio button.
     */
    private class IconRadioButton extends JPanel {

        private JRadioButton rb;
        private JLabel lbl;

        public IconRadioButton(String key, String img, boolean selected,
                               ButtonGroup bg, ActionListener al)
        {
            super(new FlowLayout(FlowLayout.LEADING));

            Icon icon = Util.getImageIcon(img);
            if (icon != null) {
                lbl = new JLabel(icon);
                add(lbl);
            }
            rb = createRadioButton(key, al);
            rb.setSelected(selected);
            addToBG(rb, this, bg);
        }

        public void addActionListener(ActionListener al) {
            rb.addActionListener(al);
        }

        public boolean isSameAs(Object source) {
            return (rb == source);
        }

        public void setEnabled(boolean enabled) {
            rb.setEnabled(enabled);
            lbl.setEnabled(enabled);
        }

        public boolean isSelected() {
            return rb.isSelected();
        }

        public void setSelected(boolean selected) {
            rb.setSelected(selected);
        }
    }
}

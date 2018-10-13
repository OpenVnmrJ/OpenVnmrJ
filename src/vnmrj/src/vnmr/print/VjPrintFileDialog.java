/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.Dialog;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.GraphicsConfiguration;
import java.awt.event.*;
import java.awt.Point;
import java.awt.Toolkit;
import java.util.Hashtable;
import java.io.*;
import javax.swing.*;
import javax.print.attribute.*;
import javax.print.attribute.standard.Destination;
import javax.print.attribute.standard.MediaSize;
import vnmr.ui.SessionShare;
import vnmr.util.Util;
import vnmr.util.UtilB;
import vnmr.util.FileUtil;
import vnmr.util.SimpleHLayout;
import vnmr.util.SimpleH2Layout;
import vnmr.util.SimpleVLayout;
import vnmr.bo.VColorChooser;


public class VjPrintFileDialog extends JDialog
                implements ActionListener {

    private JTextField fileName;
    private JTextField lineWidth;
    private JTextField spectrumWidth;
    private JTextField resolution;
    private JComboBox formatMenu;
    private JComboBox paperMenu;
    private JButton cancelButton;
    private JButton saveButton;
    private JButton previewButton;
    private JRadioButton monoRadio;
    private JRadioButton colorRadio;
    private JRadioButton portraitRadio;
    private JRadioButton landscapeRadio;
    private JLabel fgColorLabel;
    private JLabel bgColorLabel;
    private JCheckBox cursorLineCheck;
    private JCheckBox reshpeCheck;
    private boolean  bReturnValue;
    private boolean  bPreview;
    private boolean  bButtonClosing;
    private VjPrintFileChooser fileChooser;
    private VColorChooser bgColorDialog;
    private VColorChooser fgColorDialog;
    private String  nameFromChooser;
    private String  fileChooserDir;
    private String  tmpPrtFile;
    private String  lblStr;
    private String mediaNames[];
	private Hashtable<String, Object>  hs;
    private VjPrintEventListener printEventListener;
    private PrintRequestAttributeSet attributes;
    private MediaSize mediaSizes[];
    private File currentChooserDir = null;

    public static final String[] formatLabels = {"BITMAP", "GIF", "JPEG",
                  "PCL", "PDF", "PNG", "PPM", "POSTSCRIPT", "TIFF"};
    public static final String[] formatValues = {"bmp", "gif", "jpg",
                  "pcl", "pdf", "png", "ppm", "ps", "tif"};
 
    public static final String saveCmd = "Save";
    public static final String previewCmd = "Preview";
    public static final String cancelCmd = "Cancel";
    public static final String showfileCmd = "showfile";
    


    public VjPrintFileDialog(GraphicsConfiguration gc, Frame frame,
                                          ServicePopup dlg) {
        super(frame, "Print File Setup", false, gc);
        buildDialog();
    }

    public VjPrintFileDialog(GraphicsConfiguration gc, Dialog dialog,
                                         ServicePopup  dlg) {
        super(dialog, "Print File Setup", false, gc);
        buildDialog();
    }

    private void buildDialog() {
        int n, w;
        Dimension dim;
        Container cont = getContentPane();
        cont.setLayout(new BorderLayout());

        JPanel topPanel = new JPanel();
        topPanel.setLayout(new SimpleVLayout(6,4,4,4,false));
        cont.add(topPanel, BorderLayout.CENTER);

        ButtonGroup radioGroup;
        JPanel panel = new JPanel();
        topPanel.add(panel);
        w = 0;
        // panel.setLayout(new SimpleHLayout());
        panel.setLayout(new BorderLayout(4, 0));
        lblStr = Util.getLabel("_File_Name", "File Name")+": ";
        JLabel label_1 = new JLabel(lblStr);
        dim = label_1.getPreferredSize();
        if (dim.width > w)
            w = dim.width;
        panel.add(label_1, BorderLayout.WEST);
        fileName = new JTextField("", 20);
        panel.add(fileName, BorderLayout.CENTER);
        lblStr = Util.getLabel("_Browse", "Browse")+"...";
        JButton button = new JButton(lblStr);
        button.setActionCommand(showfileCmd);
        button.addActionListener(this);
        panel.add(button, BorderLayout.EAST);
        
        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        lblStr = Util.getLabel("_Format", "Format")+": ";
        JLabel label_2 = new JLabel(lblStr);
        panel.add(label_2);
        dim = label_2.getPreferredSize();
        if (dim.width > w)
            w = dim.width;
        formatMenu = new JComboBox();
        panel.add(formatMenu);
        for (n = 0; n < formatLabels.length; n++)
            formatMenu.addItem(formatLabels[n]);
        formatMenu.setLocation(80, 0);
        formatMenu.setSelectedItem("POSTSCRIPT");
        formatMenu.setActionCommand("format");
        formatMenu.addActionListener(this);

        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        lblStr = Util.getLabel("_Size", "Size")+": ";
        JLabel label_3 = new JLabel(lblStr);
        panel.add(label_3);
        dim = label_3.getPreferredSize();
        if (dim.width > w)
            w = dim.width;
        paperMenu = new JComboBox();
        mediaSizes = VjPaperMedia.paperMedium;
        mediaNames = VjPaperMedia.paperNames;
        for (n = 0; n < mediaSizes.length; n++) {
            MediaSize ms = mediaSizes[n];
            if (ms != null) {
                String name = VjPrintUtil.getMediaResource(
                        ms.getMediaSizeName().toString(), mediaNames[n]);
                VjMediaSizeObj obj = new VjMediaSizeObj(ms, name);
                paperMenu.addItem(obj);
            }
        }
 
        if (mediaSizes.length > 8) {
            if (paperMenu.getMaximumRowCount() < 9)
                 paperMenu.setMaximumRowCount(9);
        }
        paperMenu.setLocation(80, 0);
        paperMenu.setActionCommand("size");
        paperMenu.addActionListener(this);
        panel.add(paperMenu);

        if (w > 1) {
           dim.width = w;
           label_1.setPreferredSize(new Dimension(w+6, dim.height));
           label_2.setPreferredSize(dim);
           label_3.setPreferredSize(dim);
           w += 4;
           paperMenu.setLocation(w, 0);
           formatMenu.setLocation(w, 0);
        }

        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        // label = new JLabel("Orientation: ");
        //  panel.add(label);

        lblStr = Util.getLabel("_Portrait", "Portrait");
        portraitRadio = new JRadioButton(lblStr);
        panel.add(portraitRadio);
        portraitRadio.setActionCommand("portrait");
        portraitRadio.addActionListener(this);

        lblStr = Util.getLabel("_Landscape", "Landscape");
        landscapeRadio = new JRadioButton(lblStr);
        landscapeRadio.setLocation(170, 0);
        panel.add(landscapeRadio);
        landscapeRadio.setActionCommand("landscape");
        landscapeRadio.addActionListener(this);

        radioGroup = new ButtonGroup();
        radioGroup.add(portraitRadio);
        radioGroup.add(landscapeRadio);
        
        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());

        lblStr = Util.getLabel("_Color", "Color");
        colorRadio = new JRadioButton(lblStr);
        colorRadio.setActionCommand("colorimage");
        colorRadio.addActionListener(this);
        panel.add(colorRadio);

        lblStr = Util.getLabel("_Monochrome", "Monochrome");
        monoRadio = new JRadioButton(lblStr);
        monoRadio.setActionCommand("monoimage");
        monoRadio.addActionListener(this);
        monoRadio.setLocation(170, 0);
        panel.add(monoRadio);

        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());

        lblStr = Util.getLabel("_Background", "Background")+": ";
        bgColorLabel = new JLabel(lblStr);
        panel.add(bgColorLabel);
        bgColorDialog =  new VColorChooser(null, null, null);
        bgColorDialog.setLocation(170, 0);
        panel.add(bgColorDialog);

        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());

        lblStr = Util.getLabel("_Foreground", "Foreground")+": ";
        fgColorLabel = new JLabel(lblStr);
        panel.add(fgColorLabel);
        fgColorDialog =  new VColorChooser(null, null, null);
        fgColorDialog.setLocation(170, 0);
        panel.add(fgColorDialog);

        radioGroup = new ButtonGroup();
        radioGroup.add(colorRadio);
        radioGroup.add(monoRadio);

        monoRadio.setSelected(true);
        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        lblStr = Util.getLabel("_Spectrum_Line_Width", "Spectrum Line Width")+": ";

        JLabel label = new JLabel(lblStr);
        panel.add(label);
        spectrumWidth = new JTextField("1", 6);
        spectrumWidth.setHorizontalAlignment(JTextField.CENTER);
        spectrumWidth.setLocation(170, 0);
        panel.add(spectrumWidth);
        lblStr = Util.getLabel("_pixels", "pixels");
        label = new JLabel(lblStr);
        panel.add(label);
        
        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        lblStr = Util.getLabel("_Graphics_Line_Width", "Graphics Line Width")+": ";

        label = new JLabel(lblStr);
        panel.add(label);
        lineWidth = new JTextField("1", 6);
        lineWidth.setHorizontalAlignment(JTextField.CENTER);
        lineWidth.setLocation(170, 0);
        panel.add(lineWidth);
        lblStr = Util.getLabel("_pixels", "pixels");
        label = new JLabel(lblStr);
        panel.add(label);
        
        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        lblStr = Util.getLabel("_Image_Resolution", "Image Resolution") +": ";
        label = new JLabel(lblStr);
        panel.add(label);
        resolution = new JTextField("90", 6);
        resolution.setHorizontalAlignment(JTextField.CENTER);
        panel.add(resolution);
        resolution.setLocation(170, 0);
        lblStr = Util.getLabel("_dots_per_inch", "dots per inch");
        label = new JLabel(lblStr);
        panel.add(label);

        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        n = Toolkit.getDefaultToolkit().getScreenResolution();
        lblStr = Util.getLabel("_Screen_resolution_is", "Screen resolution is");
        String res = lblStr+" "+n;
        lblStr = Util.getLabel("_dots_per_inch", "dots per inch");
        res = res + " "+ lblStr;
        label = new JLabel(res);
        panel.add(label);

        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        lblStr = Util.getLabel("_Keep_cursor_lines", "Keep cursor lines");
        cursorLineCheck = new JCheckBox(lblStr);
        panel.add(cursorLineCheck);

        panel = new JPanel();
        topPanel.add(panel);
        panel.setLayout(new SimpleHLayout());
        lblStr = Util.getLabel("_Reshape_to_fit_page", "Reshape to fit page");
        reshpeCheck = new JCheckBox(lblStr);
        panel.add(reshpeCheck);
 
        String okStr = Util.getLabel("blSave", saveCmd);
        saveButton = new JButton(okStr);
        String cancelStr = Util.getLabel("blCancel", cancelCmd);
        cancelButton = new JButton(cancelStr);
        String previewStr = Util.getLabel("_Preview", previewCmd);
        previewButton = new JButton(previewStr);
        if(Util.labelExists("blmsave")) {
            okStr = Util.getLabel("blmSave");
            char chOk = okStr.charAt(0);
            saveButton.setMnemonic(chOk);
        }
        if(Util.labelExists("blmCancel")) {
            cancelStr = Util.getLabel("blmCancel");
            char chCancel = cancelStr.charAt(0);
            cancelButton.setMnemonic(chCancel);
        }

        saveButton.setActionCommand(saveCmd);
        saveButton.addActionListener(this);
        cancelButton.setActionCommand(cancelCmd);
        cancelButton.addActionListener(this);
        previewButton.setActionCommand(previewCmd);
        previewButton.addActionListener(this);

        JPanel bottomPanel = new JPanel();
        bottomPanel.setBorder(BorderFactory.createEmptyBorder(0, 5, 10, 5));
        bottomPanel.setLayout(new SimpleH2Layout(SimpleH2Layout.CENTER, 20, 5, false));
        bottomPanel.add(saveButton);
        bottomPanel.add(previewButton);
        bottomPanel.add(cancelButton);
        cont.add(bottomPanel, BorderLayout.SOUTH);

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                if (!bButtonClosing)
                    bReturnValue = false;
                notifyListener();
            }
        });
        
        setLocation(10, 200);
        setFromSession();
        pack();

    }

    public void setPrintAttributeSet(PrintRequestAttributeSet attr)
    {
        attributes = attr;
    }

    public void updatePaperMenu() {
       /****************
        if (srvPopup == null)
           return;
        PrintService psService = srvPopup.getCurrentPrintService();
        if (psService == null)
           return;
        Object values =
              VjPrintUtil.getAttributeValues(psService, Media.class,
                                                 null, null);
        if (values == null || !(values instanceof Media[]))
           return;
        Media[] media = (Media[])values;
        if (media.length < 1)
           return;
        paperMenu.removeAllItems();
        messageRB = ServicePopup.messageRB;
        mediumSizes = new MediaSizeName[media.length];
        if (mediumSizes == null)
            mediumSizes = new Vector<MediaSizeName>();
        else
            mediumSizes.clear();
        for (int i = 0; i < media.length; i++) {
           Media medium = media[i];
           if (medium instanceof MediaSizeName) {
               String name = getMediaName(medium.toString());
               if (isNewMedia(name, mediumSizes)) {
                    paperMenu.addItem(name);
                    mediumSizes.add((MediaSizeName) medium);
                  // mediumSizes[i] = (MediaSizeName) medium;
               }
           }
        }
        ***************/
    }

    public void showDialog() {
         bReturnValue = false;
         nameFromChooser = null;
         bButtonClosing = false;
         setVisible(true);
         // updatePaperMenu();
         String s = getString(VjPrintDef.FILE_PAPER);
         if (s != null) {
             VjPrintUtil.setMediaComboxSelectItem(paperMenu, s);
         }
         VjPrintService.setTopDialog(this);
    }

    public void closeDialog() {
         setVisible(false);
         /*********
         if (fileChooser != null) {
             fileChooser.setVisible(false);
             fileChooser = null;
         }
         *********/
    }

    public boolean getValue() {

         return bReturnValue;
    }

    private void emptyFilePopup() {
         JOptionPane.showMessageDialog(this,
                "File name entry is empty. Please enter file name",
                ServicePopup.getMsg("dialog.owtitle"),
                JOptionPane.WARNING_MESSAGE);
    }

    private void errorFilePopup(String f) {
         JOptionPane.showMessageDialog(this,
                ServicePopup.getMsg("dialog.writeerror")+" "+f,
                ServicePopup.getMsg("dialog.owtitle"),
                JOptionPane.WARNING_MESSAGE);
    }

    private void showFileDialog() {
        String filePath;
        File fileDest = null;

        if (fileChooserDir == null) {
            filePath = FileUtil.usrdir()+File.separator+"plot";
            fileChooserDir = FileUtil.savePath(filePath+File.separator+"xxx", false);
            if (fileChooserDir != null)
                fileChooserDir = filePath;
            else
                fileChooserDir = FileUtil.usrdir();
        }

        if (currentChooserDir == null) {
            currentChooserDir = new File(fileChooserDir);
            if (!currentChooserDir.exists() || !currentChooserDir.isDirectory()) {
                 fileChooserDir = FileUtil.usrdir();
                 currentChooserDir = new File(fileChooserDir);
                 if (!currentChooserDir.exists() || !currentChooserDir.isDirectory())
                     currentChooserDir = null;
            }
        }
        if (fileChooser == null) {
            fileChooser = new VjPrintFileChooser();
            fileChooser.setApproveButtonText("Ok");
        }
        if (currentChooserDir != null)
            fileChooser.setCurrentDirectory(currentChooserDir);

        int ret = fileChooser.showDialog(this, null);
        currentChooserDir = fileChooser.getCurrentDirectory();
        fileDest = fileChooser.getSelectedFile();

        if (ret != JFileChooser.APPROVE_OPTION) {
            return;
        }
        if (fileDest == null)
            return;
        filePath = fileDest.getAbsolutePath();
        if (filePath == null || filePath.length() < 1) {
            emptyFilePopup();
            return;
        }
        if (Util.iswindows())
             filePath = UtilB.escapeBackSlashes(filePath);
        File f = new File(filePath);
        if (f.isDirectory())
             filePath = new StringBuffer().append(filePath).append(File.separator).toString();
        nameFromChooser = filePath;
        fileName.setText(filePath);
        fileChooserDir = fileDest.getParent();
    }

    private void notifyListener() {
       if (!isVisible())
          return;

       if (printEventListener != null) {
          VjPrintEvent e = new VjPrintEvent(this, VjPrintDef.CANCEL);
          if (bReturnValue)
               e.setStatus(VjPrintDef.APPROVE);
          printEventListener.printEventPerformed(e);
       }
    }

    private String extendFileName(String name) {
        if (name.length() < 1) {
           return name;
        }
        String fname = (String)formatMenu.getSelectedItem();
        String extStr = "ps";
        if (fname != null) {
            for (int n = 0; n < formatLabels.length; n++) {
                if (fname.equals(formatLabels[n])) {
                     extStr = formatValues[n].toLowerCase();
                     break;
                }
            }
        }
        if (!name.endsWith("."))
            extStr = "."+extStr;
        if (name.endsWith(extStr))
            return name;
        fname = name+extStr;
        return fname;
    }

    private boolean validateFile() {
        String data = fileName.getText().trim();
        if (data.length() < 1) {
            emptyFilePopup();
            return false; 
        }
        boolean toCheck = true;
        if (nameFromChooser != null) {
            // if (nameFromChooser.equals(data))
            //   toCheck = false;
        }

        String fname = extendFileName(data);
        
        File f = new File(fname);
        if (toCheck) {
            if (f.exists()) {
               int val;
               val = JOptionPane.showConfirmDialog(this,
                         "File: "+fname+".\n"+
                         ServicePopup.getMsg("dialog.overwrite"),
                         ServicePopup.getMsg("dialog.owtitle"),
                         JOptionPane.YES_NO_OPTION);
                if (val != JOptionPane.YES_OPTION) {
                    return false;
                }
            }
            try {
                if (f.createNewFile()) {
                    f.delete();
                }
            }  catch (IOException ioe) {
                errorFilePopup(fname);
                return false;
            } catch (SecurityException se) { return false; }

        }
        File pFile = f.getParentFile();
        boolean goodFile = true;

        if (f.exists() && (!f.isFile() || !f.canWrite()))
             goodFile = false;
        if (pFile != null) {
             if (pFile.exists() && !pFile.canWrite())
                 goodFile = false;
        }
        if (!goodFile) {
             errorFilePopup(fname);
             return false;
        }
        if (attributes != null) {
            
            Class<Destination> dstCategory = Destination.class;
            if (attributes.containsKey(dstCategory))
                attributes.remove(dstCategory);
            File tmpf = null;
            String tmpFile = new StringBuffer().append("plot").append(File.separator).append("prttmp").
               append(System.currentTimeMillis()).append(".ps").toString();
            String dataFile = FileUtil.savePath(tmpFile);
            if (dataFile == null) {
                 try {
                     tmpf = File.createTempFile("prttmp", ".ps");
                 }
                 catch (IOException err) {
                     tmpf = null;
                 }
            }
            else
                 tmpf = new File(dataFile);

            if (tmpf != null) {
                tmpPrtFile = tmpf.getAbsolutePath();
                attributes.add(new Destination(tmpf.toURI()));
            }
            else {
                tmpPrtFile = f.getAbsolutePath();
                attributes.add(new Destination(f.toURI()));
            }
        }
 
        return true;
    }

    private void setColorChoices() {
        if (monoRadio.isSelected()) {
              fgColorDialog.setActive(true);
              fgColorLabel.setEnabled(true);
              bgColorDialog.setActive(false);
              bgColorLabel.setEnabled(false);
        }
        else {
              fgColorDialog.setActive(false);
              fgColorLabel.setEnabled(false);
              bgColorDialog.setActive(true);
              bgColorLabel.setEnabled(true);
        }
    }
        

    public void actionPerformed(ActionEvent e) {
        String cmd = e.getActionCommand();
        setColorChoices();
        if (!this.isShowing()) {
            return;
        }
        bButtonClosing = false;
        bPreview = false;
        if (cmd.equals(showfileCmd))
        {
            showFileDialog();
            return;
        }
        if (cmd.equals(saveCmd))
        {
            if (!validateFile())
                return;
            saveToSession();
            bReturnValue = true;
            bButtonClosing = true;
            notifyListener();
            if (VjPrintUtil.isTestMode())
               return;
            setVisible(false);
            return;
        }
        if (cmd.equals(cancelCmd))
        {
            if (VjPrintUtil.isTestMode())
                System.exit(0);
            bButtonClosing = true;
            bReturnValue = false;
            saveToSession();
            notifyListener();
            setVisible(false);
            return;
        }
        if (cmd.equals(previewCmd))
        {
            // if (!validateFile())
            //    return;
            bReturnValue = true;
            bPreview = true;
            saveToSession();
            notifyListener();
            return;
        }
    }

    public void setPrintEventListener(VjPrintEventListener  e) {
           printEventListener = e;
    }
   
    private Float getFloat(String key)
    {
        Object obj = hs.get(key);
        if (obj == null || !(obj instanceof Float))
             return null;
        return (Float) obj;
    }

    private String getFloatString(String key, String def)
    {
        Object obj = hs.get(key);
        if (obj == null || !(obj instanceof Float))
             return def;
        Float fv = (Float) obj;
        return fv.toString();
    }


    private Float setFloat(String value, String def)
    {
         String s = value.trim();
         Float retVal;
         if (s.length() < 1)
             s = def;
         try {
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


    @SuppressWarnings("unchecked")
	private void  setFromSession()
    {
        SessionShare sshare = Util.getSessionShare();
        landscapeRadio.setSelected(true);
        monoRadio.setSelected(true);
        reshpeCheck.setSelected(true);
        cursorLineCheck.setSelected(false);

        if (sshare == null)
           return;
        hs = sshare.userInfo();
        if (hs == null)
            return;
        Float fv;
        String mess = getString(VjPrintDef.FILE_NAME);
        if (mess != null)
            fileName.setText(mess);
        mess = getString(VjPrintDef.FILE_FORMATLABEL);
        if (mess != null)
             formatMenu.setSelectedItem(mess);
        mess = getString(VjPrintDef.FILE_PAPER);
        if (mess != null)
             VjPrintUtil.setMediaComboxSelectItem(paperMenu, mess);
        mess = getString(VjPrintDef.FILE_COLOR);
        if (mess != null) {
             if (mess.equals(VjPrintDef.COLOR))
                   colorRadio.setSelected(true);
        }
        mess = getString(VjPrintDef.FILE_ORIENTATION);
        if (mess != null) {
             if (mess.equals(VjPrintDef.PORTRAIT))
                portraitRadio.setSelected(true);
        }
        if (cursorLineCheck != null) {
            mess = getString(VjPrintDef.FILE_CURSOR_LINE);
            if (mess != null && mess.equals(VjPrintDef.YES))
                cursorLineCheck.setSelected(true);
        }
        mess = getString(VjPrintDef.FILE_RESHAPE);
        if (mess != null) {
            if (mess.equals(VjPrintDef.YES))
                reshpeCheck.setSelected(true);
            else
                reshpeCheck.setSelected(false);
        }
        mess = getString(VjPrintDef.FILE_DIR);
        if (mess != null)
            fileChooserDir = mess;
        mess = getString(VjPrintDef.FILE_FG);
        if (mess != null)
            fgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, mess);
        else
            fgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, "black");
        mess = (String)hs.get(VjPrintDef.FILE_BG);
        if (mess != null)
            bgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, mess);
        else
            bgColorDialog.setAttribute(vnmr.bo.VObjDef.VALUE, "white");

        spectrumWidth.setText(getFloatString(VjPrintDef.FILE_SPWIDTH, "1"));
        lineWidth.setText(getFloatString(VjPrintDef.FILE_LINEWIDTH, "1"));
        mess = getString(VjPrintDef.FILE_RESOLUTION);
        if (mess != null)
            resolution.setText(mess);
            
        setColorChoices();
        int x = 100;
        int y = 200;
        fv = getFloat(VjPrintDef.FILE_LOCX);
        if (fv != null)
            x = fv.intValue();
        fv = getFloat(VjPrintDef.FILE_LOCY);
        if (fv != null)
            y = fv.intValue();
        setLocation(x, y);
    }

    private void saveMedia()
    {
        VjMediaSizeObj obj = (VjMediaSizeObj)paperMenu.getSelectedItem();
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
	private void  saveToSession()
    {
        SessionShare sshare = Util.getSessionShare();
        if (sshare == null)
           return;
        hs = sshare.userInfo();
        if (hs == null)
            return;
        String mess = fileName.getText();
        if (mess != null)
            hs.put(VjPrintDef.FILE_NAME, mess.trim());
        if (bPreview)
            hs.put(VjPrintDef.FILE_PREVIEW, VjPrintDef.YES);
        else
            hs.put(VjPrintDef.FILE_PREVIEW, VjPrintDef.NO);
        mess = spectrumWidth.getText();
        Float fv = setFloat(mess, "1");
        if (fv != null)
            hs.put(VjPrintDef.FILE_SPWIDTH, fv);
        mess = lineWidth.getText();
        fv = setFloat(mess, "1");
        if (fv != null)
            hs.put(VjPrintDef.FILE_LINEWIDTH, fv);
        mess = resolution.getText().trim();
        if (mess != null)
            hs.put(VjPrintDef.FILE_RESOLUTION, mess);
        mess = (String)formatMenu.getSelectedItem();
        if (mess != null) {
            hs.put(VjPrintDef.FILE_FORMATLABEL, mess);
            String s = "ps";
            for (int n = 0; n < formatLabels.length; n++) {
                if (mess.equals(formatLabels[n])) {
                     s = formatValues[n];
                     break;
                }
            } 
            hs.put(VjPrintDef.FILE_FORMAT, s);
        }
        VjMediaSizeObj mobj = (VjMediaSizeObj)paperMenu.getSelectedItem();
        if (mobj != null)
            hs.put(VjPrintDef.FILE_PAPER, mobj.getMediaName());
        if (monoRadio.isSelected())
            hs.put(VjPrintDef.FILE_COLOR, VjPrintDef.MONO);
        else
            hs.put(VjPrintDef.FILE_COLOR, VjPrintDef.COLOR);
        if (portraitRadio.isSelected())
            hs.put(VjPrintDef.FILE_ORIENTATION, VjPrintDef.PORTRAIT);
        else
            hs.put(VjPrintDef.FILE_ORIENTATION, VjPrintDef.LANDSCPAE);
        if (reshpeCheck.isSelected())
            hs.put(VjPrintDef.FILE_RESHAPE, VjPrintDef.YES);
        else
            hs.put(VjPrintDef.FILE_RESHAPE, VjPrintDef.NO);
        if (cursorLineCheck != null && cursorLineCheck.isSelected())
            hs.put(VjPrintDef.FILE_CURSOR_LINE, VjPrintDef.YES);
        else
            hs.put(VjPrintDef.FILE_CURSOR_LINE, VjPrintDef.NO);
        Point pt = this.getLocation();
        hs.put(VjPrintDef.FILE_LOCX, new Float((float)pt.x));
        hs.put(VjPrintDef.FILE_LOCY, new Float((float)pt.y));

        if (fileChooserDir != null)
            hs.put(VjPrintDef.FILE_DIR, fileChooserDir);

        mess = bgColorDialog.getAttribute(vnmr.bo.VObjDef.VALUE);
        if (mess != null)
            hs.put(VjPrintDef.FILE_BG, mess);
        mess = fgColorDialog.getAttribute(vnmr.bo.VObjDef.VALUE);
        if (mess != null)
            hs.put(VjPrintDef.FILE_FG, mess);
        if (tmpPrtFile != null)
            hs.put(VjPrintDef.FILE_TMPNAME, tmpPrtFile);
        else
            hs.put(VjPrintDef.FILE_TMPNAME, "");
        hs.put(VjPrintDef.FILE_HEIGHT, new Float(0));
        hs.put(VjPrintDef.FILE_WIDTH, new Float(0));
        saveMedia();
    }

} // end of VjPrintFileDialog


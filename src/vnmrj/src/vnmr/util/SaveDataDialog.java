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
import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.lang.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.templates.*;
import vnmr.admin.ui.*;
import vnmr.admin.util.*;

public class SaveDataDialog extends ModalDialog
                      implements ActionListener, VObjDef {

    private static SaveDataDialog saveDataDialog = null;
    private boolean debug = false;

// these are needed for a modal dialog to talk to vnmr
    private SessionShare sshare;
    private ButtonIF vnmrIf;
    protected Hashtable allLongKeysTypes = null;
    protected ImageIcon images[];

    protected static TemplatePanel tempPane = null;
    protected static ResultPanel resultPane = null;
    protected static RecordFilesPanel recfilePane = null;
    protected static JTabbedPane tabbedPane = null;
    protected static DataDirPanel dirPane = null;

    public static Color infoColor = Color.blue;
    public static Color actionColor = Color.black;
    public static Color FDAColor = Color.blue;
    public static Color highlightColor = Color.yellow;

    protected String m_userName = null;

    protected static final String DATADIR = "PROFILES/data/";
    protected static final String P11DIR = "PROFILES/p11/";
    protected static final String CHILDDIR = "USER/PROPERTIES/projectDirs";

    protected static final String TEMPLATEDIR = "PROFILES/templates/";

    protected static int part11Mode = 0;
    // 0 non-p11, 1 both, 2 p11 only.

    public SaveDataDialog(SessionShare sshare, ButtonIF vif, String title, String helpFile) {
        super(title);

        this.sshare = sshare;
        this.vnmrIf = vif;
        m_strHelpFile = helpFile;

        part11Mode = getPart11Mode();

    	if(debug) System.out.println(new StringBuffer().append("part11Mode ").
                                     append( part11Mode).toString());

        m_userName = System.getProperty("user.name");

        dirPane = new DataDirPanel();

        tempPane = new TemplatePanel();

        resultPane = new ResultPanel();

        JPanel dataPane = new JPanel();
        dataPane.setLayout(new BoxLayout(dataPane, BoxLayout.Y_AXIS));
        dataPane.add(dirPane);
        dataPane.add(tempPane);
        dataPane.add(resultPane);

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BoxLayout(mainPanel, BoxLayout.Y_AXIS));

        if(part11Mode != 0) {

            tabbedPane = new JTabbedPane();

            tabbedPane.addTab("Save Data Setup", null, dataPane, "");

            tabbedPane.setSelectedIndex(0);
            recfilePane = new RecordFilesPanel();

            tabbedPane.addTab("Optional files", null, recfilePane, "");

            tabbedPane.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
            tabbedPane.setForeground(actionColor);

            mainPanel.add(tabbedPane);
        }
        else mainPanel.add(dataPane);
        
        if(DebugOutput.isSetFor("savedatasetup"))
            Messages.postDebug("SaveDataDialog: Creating panel");


// a panel to hold tempPane and tabbedPane.

        mainPanel.setBorder(BorderFactory.createEmptyBorder(0,10,0,10));

        // Add the panel top of the dialog.
        getContentPane().add(mainPanel, BorderLayout.NORTH);

        // destroy the dialog if closed from Window menu.
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent we) {
                //setVisible(false);
                //destroy();
                okAction();
            }
        });

        addKeyListener(this);

        // Set the buttons up with Listeners
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");

        // Make the frame fit its contents.
        pack();

    } // constructor

    public void destroy() {

        vnmrIf = null;
        tempPane.destroy();
        dirPane.destroy();
        saveDataDialog = null;
        removeAll();
        System.gc();
        System.runFinalization();
    }

    private void okAction() {
        String currenttemplate = tempPane.getCurrenttemplate();
        if(currenttemplate != null) {
                ((ExpPanel)vnmrIf).sendCmdToVnmr(new StringBuffer().append("svfname = '" ).
                                                 append( currenttemplate ).append( "'\n" ).toString());
        }
        String currentsvfdir = dirPane.getCurrentDir();
        if(currentsvfdir != null) {
                ((ExpPanel)vnmrIf).sendCmdToVnmr(new StringBuffer().append("svfdir = '" ).
                                                 append( currentsvfdir ).append( "'\n" ).toString());
        }
        dirPane.saveChildDirs();
        dirPane.writeRemovedChildDirs();
    	dirPane.createDirs();
    	dirPane.removeChildDirs();
        tempPane.savetemplate();
      	tempPane.writeRemovedTemps();
    	resultPane.sendAutoSave();
        if(recfilePane != null) recfilePane.saveOptionalFiles();
            setVisible(false);
    }
    /************************************************** <pre>
     * Summary: Listener for the 3 ModalDialog buttons.
     *
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e)  {
        String cmd = e.getActionCommand();
        // OK
        if(cmd.equals("ok")) {
            okAction();
            if(recfilePane != null) recfilePane.resetOpt();
        }
        // Cancel
        else if(cmd.equals("cancel")) {
            if(recfilePane != null) recfilePane.resetOpt();
            setVisible(false);
        }
        // Help
        else if(cmd.equals("help")) {
        // Do not call setVisible(false);  That will cause
        // the Block to release and the code which create
        // this object will try to use filename.  This way
        // the panel stays up and the Block stays in effect.
            //Messages.postError("Help Not Implemented Yet");
            displayHelp();
        }
    }

    private void updatePanels() {
    if(dirPane != null) dirPane.updateDirPane();
    if(tempPane != null) tempPane.updateTemplate();
    if(resultPane != null) resultPane.reformat();
    if(recfilePane != null) recfilePane.updatePanel();
    }

    public static SaveDataDialog getSaveDataDialog(SessionShare sshare, ButtonIF vif,
                                                   String title, Component comp,
                                                   String helpFile) {

        if(saveDataDialog == null) {
            saveDataDialog = new SaveDataDialog(sshare, vif, title, helpFile);

            // Set this dialog on top of the component passed in.
            saveDataDialog.setLocationRelativeTo(comp);
            saveDataDialog.updatePanels();
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveDataDialog: setting setVisible(true)");
            saveDataDialog.setVisible(true);
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveDataDialog: complete");
        } else {
        // update available disk space.
            saveDataDialog.updatePanels();
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveDataDialog: setting setVisible(true)");
            saveDataDialog.setVisible(true);
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveDataDialog: complete");
        }

        return saveDataDialog;
    }

    /************************************************** <pre>
     * Summary: Show the dialog, wait for results, get and return results.
     *
     *	    The Dialog will be positioned over the component passed in.
     *	    This way, the cursor will not need to be moved.
     *	    If null is passed in, the dialog will be positioned in the
     *	    center of the screen.
     *
     </pre> **************************************************/
    public void showDialogAndSetPath() {

    // Show the dialog and wait for the results. (Blocking call)
        showDialogWithThread();

    // If I don't do this, and the user hits a 'return' on the keyboard
    // the dialog comes visible again.
        transferFocus();
    }

    private int getPart11Mode()
    {

        String part11Root = FileUtil.openPath("SYSTEM/PART11");

        if(debug)
            System.out.println(new StringBuffer().append("part11Root " ).
                               append( part11Root).toString());

        if(part11Root == null) return 0;

        String path = FileUtil.openPath(new StringBuffer().append(part11Root ).
                                        append( "/part11Config").toString());

        if(path == null) return 0;
        else {
          BufferedReader in;
          String inLine;

          try {
            in = new BufferedReader(new FileReader(path));
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() <= 0 || inLine.startsWith("#")
			|| inLine.startsWith("@") ) continue;
                inLine.trim();
                StringTokenizer tok = new StringTokenizer(inLine, " :\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    if(key.equals("dataType") && tok.hasMoreTokens()) {
                        String value = tok.nextToken();
                        if(value.equalsIgnoreCase("non-FDA")) return 0;
                        else if(value.equalsIgnoreCase("both")) return 1;
                        else if(value.equalsIgnoreCase("FDA")) return 2;
                    }
                }
            }
            in.close();
          } catch(IOException e) { System.err.println(new StringBuffer().append("ERROR: read " ).
                                                           append( path).toString()); }

      return 0;
    }
    }

    class ResultPanel extends JPanel {

      VTextMsg result = null;
      protected VCheck autoSave = null;

      public ResultPanel() {

// create label and panel for displaying result

        JLabel resultLabel = new JLabel("Data will be save to:", JLabel.LEFT);
        resultLabel.setForeground(infoColor);

// result is a MyTextMsg obj. to display vnmr results.

        result = new MyTextMsg(sshare, vnmrIf, null);
        result.setPreferredSize(new Dimension(280, 20));
        result.setForeground(infoColor);
        result.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                reformat();
            }
        });

	JPanel autoSavePane = makeAutoSaveCheck();

        setLayout(new GridLayout(0, 1));
        add(resultLabel);
        add(result);
	add(autoSavePane);

        setBorder(new CompoundBorder(BorderFactory.createEtchedBorder(),
                        BorderFactory.createEmptyBorder(0,5,5,5)));

        //setBorder(BorderFactory.createEmptyBorder(5,5,5,5));

    //reformat();
      }

      private JPanel makeAutoSaveCheck() {

	autoSave = new VCheck(sshare, vnmrIf, null);
	autoSave.setAttribute(LABEL, "Auto save");
	autoSave.setModalMode(true);
        autoSave.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
	autoSave.setAttribute(VARIABLE, "dofidsave");
        autoSave.setAttribute(SETVAL, "if dofidsave='y' then $VALUE=1 else $VALUE=0 endif");
	autoSave.setAttribute(CMD, "dofidsave='y'");
	autoSave.setAttribute(CMD2, "dofidsave='n'");
        autoSave.updateValue();

	JPanel pane = new JPanel();
        pane.setLayout(new GridLayout(1, 1));
	pane.add(autoSave);
        pane.setBorder(BorderFactory.createEmptyBorder(0,5,5,5));
	return pane;
      }

      public void sendAutoSave() {
	if(autoSave.isSelected()) {
	    String cmd = autoSave.getAttribute(CMD);
	    if (cmd != null)
	    ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
	} else {
	    String cmd = autoSave.getAttribute(CMD2);
	    if (cmd != null)
	    ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
	}
      }

      public String getResultText() {
    	return result.getText();
      }

    /** Formats and displays filename. */
      public void reformat() {

    	String currenttemplate = tempPane.getCurrenttemplate();
    	String currentDir = dirPane.getCurrentDir();

	if(currentDir == null) {

            result.setAttribute(SETVAL, "$VALUE=svfdir+'/'");
            result.updateValue();
	    return;
	}

    	String suffix = ".fid";
	if(dirPane.isP11Dir(currentDir)) suffix = ".REC";

        try {

          if(currentDir == null && suffix.equals(".fid")) {
          ((ExpPanel)vnmrIf).sendCmdToVnmr(new StringBuffer().append("Svfname(svfdir+'/'+svfname,'").
                                                append(suffix).append("'):$file\n").toString());
          } else if(currentDir == null) {
           ((ExpPanel)vnmrIf).sendCmdToVnmr(new StringBuffer().append("Svfname(svfdir+'/'+svfname,'").
                                                 append(suffix).append("'):$file\n").toString());
          } else {
                String name =new StringBuffer().append( currentDir ).append( "/" ).
                                  append( currenttemplate).toString();

    if(debug)
    System.out.println(new StringBuffer().append("currentDir/currenttemplate " ).
                       append( name).toString());

                ((ExpPanel)vnmrIf).sendCmdToVnmr(new StringBuffer().append("Svfname('" ).
                                                      append( name ).append( "','").
                                                      append(suffix).append("'):$file\n").toString());
          }
            result.setForeground(infoColor);
            result.setAttribute(SETVAL, "$VALUE=$file");
            result.updateValue();
            String filename = result.getText();
            result.setText(filename);
        } catch (IllegalArgumentException iae) {
            result.setForeground(Color.red);
            result.setText(new StringBuffer().append("Error: " ).
                           append( iae.getMessage()).toString());
        }
      }

      public void destroy() {
        result.destroy();
        removeAll();
      }

      class MyTextMsg extends VTextMsg {

        public MyTextMsg(SessionShare sshare, ButtonIF vif, String typ) {
      	super(sshare, vif, typ);
        }

        public void setValue(ParamIF pf) {
          if (pf != null) {
            value = pf.value;
            if (precision != null) {
                // Set precision, in case we're getting this direct from pnew
                value = Util.setPrecision(value, precision);
            }
            setText(value);
          }
        }
      }
    }

    class TemplatePanel extends JPanel {

      private String currenttemplate = null;
      private String currenttemplatekey = null;
      private Vector tempkeys;
      private Hashtable temps = new Hashtable();
// make templates a JComboBox
      private JComboBox templateList_label = null;
// show selected template as editable text field
      private JTextField template = null;
// button to save user templates
      private JButton saveButton = null;

      private JButton removeTempButton = null;
      private Hashtable removedTemps = new Hashtable();

    private JTextField keyEntry;

      public TemplatePanel() {

// make label and panel for filename templates

        JLabel TempLabel = new JLabel("Select a filename template:", JLabel.LEFT);
    	TempLabel.setForeground(infoColor);

// read templates from userdir/templates/vnmrj/filename_templates
// if not found, create standard ones.

	readTemps();

// template labels are uneditable ComboBox

        templateList_label = new JComboBox( tempkeys );
        templateList_label.setRenderer(new DirComboBoxRenderer());
    	templateList_label.setForeground(actionColor);
        templateList_label.setSelectedItem(currenttemplatekey);
	templateList_label.setPreferredSize(new Dimension(240, 20));
	templateList_label.setMaximumRowCount(6);
        templateList_label.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox)e.getSource();
            currenttemplatekey = (String)cb.getSelectedItem();
            currenttemplate = (String) temps.get(currenttemplatekey);
            showtemp();
        if(resultPane != null) resultPane.reformat();
          }
        });

// show current template in an editable TextField

        template = new JTextField(currenttemplate);
        template.setForeground(actionColor);
	template.setPreferredSize(new Dimension(340, 20));
        template.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            currenttemplate = template.getText();
        if(resultPane != null) resultPane.reformat();
            if(!saveButton.isEnabled())
                saveButton.setEnabled(true);
	    setRemoveTempButton();
            keyEntry.setEnabled(true);
	    int ind = currenttemplate.indexOf("_");
	    int keyind = currenttemplatekey.indexOf("_");
	    String keystr;
	    if(keyind > 0) keystr =
		currenttemplatekey.substring(0, keyind);
	    else keystr = currenttemplatekey;
	    String str = "";
	    if(ind > 0) str =
		currenttemplate.substring(ind);
            keyEntry.setText(new StringBuffer().append(keystr).append(str).toString());
          }
        });

	JLabel blabel = new JLabel("with label:", JLabel.LEFT);
        blabel.setForeground(actionColor);

        keyEntry = new JTextField();
        keyEntry.setText("new template");
        keyEntry.setEnabled(false);

// a button to save new template

        saveButton = new JButton("Save new template");
        saveButton.setForeground(actionColor);
        saveButton.setEnabled(false);

        saveButton.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {

	    String key = keyEntry.getText();
            if(key != null && key.length() > 0) {

		if(temps.containsKey(key)) {
	    	    String tempStr = (String)temps.get(currenttemplatekey);
		    template.setText(tempStr);
		    Messages.postError(new StringBuffer().append(key ).
                               append(" already exists.").toString());
		} else {
                    currenttemplatekey = key;
                    currenttemplate = template.getText();

                    temps.put(currenttemplatekey, currenttemplate);

		    tempkeys.add(currenttemplatekey);
		    sortStrVector(tempkeys);
		    templateList_label.setModel(new DefaultComboBoxModel(tempkeys));
                    templateList_label.setSelectedItem(currenttemplatekey);
            	}

            	saveButton.setEnabled(false);
	    	setRemoveTempButton();
		keyEntry.setText("new template");
                keyEntry.setEnabled(false);
            } else Messages.postInfo("need a name for new template.");
          }
        });

        removeTempButton = new JButton("remove selected template");
        removeTempButton.setForeground(actionColor);
	setRemoveTempButton();

        removeTempButton.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {

             if(currenttemplatekey == null || currenttemplatekey.length() == 0)
             Messages.postInfo("template is not selected");
             else if(temps.containsKey(currenttemplatekey)) {
                temps.remove(currenttemplatekey);
                tempkeys.remove(currenttemplatekey);
		removedTemps.put(currenttemplatekey, currenttemplate);
                if(tempkeys.size() > 0) {
                    currenttemplatekey = (String)tempkeys.elementAt(0);
                    currenttemplate = (String)temps.get(currenttemplatekey);

                    templateList_label.setModel(new DefaultComboBoxModel(tempkeys));
                    templateList_label.setSelectedItem(currenttemplatekey);

                } else {
                    currenttemplatekey = "";
                    currenttemplate = "";
		    template.setText(currenttemplate);
                }
    		if(resultPane != null) resultPane.reformat();
	    	setRemoveTempButton();

            } else Messages.postInfo(new StringBuffer().append(currenttemplatekey ).
                                     append( " is not in the list.").toString());
          }
        });

// create a JPanel to hold templateList_label and template

        JPanel tempPanel = new JPanel();
        tempPanel.setLayout(new BoxLayout(tempPanel, BoxLayout.X_AXIS));
        tempPanel.add(templateList_label);
        tempPanel.add(template);

        JPanel buttonPane = new JPanel();
        buttonPane.setLayout(new BoxLayout(buttonPane, BoxLayout.X_AXIS));
        buttonPane.add(Box.createRigidArea(new Dimension(240, 20)));
        buttonPane.add(saveButton);
        buttonPane.add(blabel);
        buttonPane.add(keyEntry);

        JPanel removeButtonPane = new JPanel();
        removeButtonPane.setLayout(new BoxLayout(removeButtonPane,
		BoxLayout.X_AXIS));
        removeButtonPane.add(Box.createRigidArea(new Dimension(240, 20)));
        removeButtonPane.add(removeTempButton);

        setLayout(new GridLayout(0, 1));
        add(TempLabel);
        add(tempPanel);
        add(buttonPane);
        add(removeButtonPane);
	add(Box.createRigidArea(new Dimension(20, 40)));

        setBorder(new CompoundBorder(BorderFactory.createEtchedBorder(),
                        BorderFactory.createEmptyBorder(0,5,5,5)));

	//setBorder(BorderFactory.createEmptyBorder(5,5,5,5));

    	if(resultPane != null) resultPane.reformat();
      }

      public void setRemoveTempButton() {

	if(!currenttemplate.equals((String)temps.get(currenttemplatekey)))
	    removeTempButton.setEnabled(false);
	else
	    removeTempButton.setEnabled(true);
      }

      public void updateTemplate() {
	readTemps();
	templateList_label.setModel(new DefaultComboBoxModel(tempkeys));
        templateList_label.setSelectedItem(currenttemplatekey);
        saveButton.setEnabled(false);
	setRemoveTempButton();
	keyEntry.setText("new template");
        keyEntry.setEnabled(false);
    	showtemp();
      }

      public void writeRemovedTemps() {
        String path = FileUtil.savePath("USER/PROPERTIES/filename_templates.del");

        if (path == null)
        {
            Messages.postDebug("Permission denied for writing file " + path);
            return;
        }

	saveHashToFile(removedTemps, null, path, true);
      }

      public void readTemps() {

	// current values are also set in Read_templates.
          Read_templates();

          tempkeys= new Vector();
          for(Enumeration en = temps.keys(); en.hasMoreElements(); ) {
            tempkeys.add((String)en.nextElement());
          }

	  sortStrVector(tempkeys);
      }

      private void Read_templates() {

    	temps.clear();
	currenttemplatekey = null;
	currenttemplate = null;

        String fileNameTemp = FileUtil.openPath("USER/PROPERTIES/filename_templates");
    if(fileNameTemp == null)
        fileNameTemp = FileUtil.openPath(TEMPLATEDIR+m_userName);

        if(fileNameTemp == null) {
            temps.put("seqfil", "$seqfil$_");
            temps.put("solvent", "$solvent$_");
            temps.put("DATE", "%DATE%_");
            temps.put("date", "%YR%%MOCH%%DAY%_");
            currenttemplatekey = "seqfil";
            currenttemplate = "$seqfil$_";
            return;
        } else {
            BufferedReader in;
            String inLine;

        try {
            in = new BufferedReader(new FileReader(fileNameTemp));
          // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() <= 0 || inLine.startsWith("#")
			|| inLine.startsWith("@") ) continue;
                inLine.trim();
            // Create a Tokenizer object to work with including ':'
                StringTokenizer tok = new StringTokenizer(inLine, ":,\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    String temp = tok.nextToken();
                    temps.put(key, temp);
                    if(currenttemplatekey == null) currenttemplatekey = key;
                    if(currenttemplate == null) currenttemplate = temp;
                }
            }
            in.close();
        } catch(IOException e) { System.err.println(new StringBuffer().append("ERROR: read " ).
                                                         append( fileNameTemp).toString()); }
      }
      }

      public String getCurrenttemplate() {
    return currenttemplate;
      }

      public void showtemp() {
    	if(template == null) return;
        try {
            template.setForeground(actionColor);
            template.setText(currenttemplate);
        } catch (IllegalArgumentException iae) {
            template.setForeground(Color.red);
            template.setText(new StringBuffer().append("Error: " ).
                             append( iae.getMessage()).toString());
        }
      }

      public void savetemplate()
      {

        String path = FileUtil.savePath("USER/PROPERTIES/filename_templates");

        if (path == null)
        {
            Messages.postDebug("Permission denied for writing file " + path);
            return;
        }

	saveHashToFile(temps, currenttemplatekey, path, false);
      }

      public void destroy() {
        temps = null;
        templateList_label = null;
        template = null;
        saveButton = null;
        removeTempButton = null;
        removeAll();
      }
    }

    class RecordFilesPanel extends JPanel {

    String[] keys;

    Hashtable recFiles = null;
    JCheckBox[] optBoxes = new JCheckBox[0];
    JCheckBox incCheck = null;
    JTextField incEntry = null;

    String incPath = null;
    boolean incInc = false;

    public RecordFilesPanel() {

        recFiles = new Hashtable();
        getStandardFiles();
        getOptionalFiles();
        getKeys();

        JLabel stdLabel = new JLabel("Standard files: ");
        stdLabel.setForeground(infoColor);

        JPanel stdFilePane = new JPanel();
        stdFilePane.setLayout(new BoxLayout(stdFilePane, BoxLayout.Y_AXIS));
        Font font = DisplayOptions.getFont("Dialog", Font.PLAIN, 14);
        for(int i=0; i<keys.length; i++)
        if(recFiles.get(keys[i]).equals("std")) {
            JLabel text = new JLabel(keys[i]);
                text.setForeground(infoColor);
            text.setFont(font);
            stdFilePane.add(text);
        }

        JPanel stdPane = new JPanel();
        stdPane.setLayout(new BoxLayout(stdPane, BoxLayout.Y_AXIS));
        stdPane.add(stdLabel);
        stdPane.add(stdFilePane);

        JLabel optLabel = new JLabel("Optional files: ");
        optLabel.setForeground(infoColor);

        int N_opt = 0;
        for(int i=0; i<keys.length; i++)
        if(!(recFiles.get(keys[i]).equals("std"))) N_opt++;

        optBoxes = new JCheckBox[N_opt];

        JPanel optFilePane = new JPanel();
        optFilePane.setLayout(new BoxLayout(optFilePane, BoxLayout.Y_AXIS));
        int k = 0;
        for(int i=0; i<keys.length; i++)
        if(!(recFiles.get(keys[i]).equals("std"))) {
            optBoxes[k] = new JCheckBox(keys[i]);
            optBoxes[k].setFont(font);
            optBoxes[k].setForeground(actionColor);
            optFilePane.add(optBoxes[k]);
            optBoxes[k].addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        JCheckBox cb = (JCheckBox)e.getSource();
                String key = cb.getText();
                recFiles.remove(key);
                if(cb.isSelected()) {
                recFiles.put(key, "opt");
                } else {
                recFiles.put(key, "null");
                }
                    }
                    });
    if(debug) System.out.println(new StringBuffer().append("N_opt ").append( N_opt ).
                                 append(" k " ).append(k).toString());
            k++;
            }

        JPanel optPane = new JPanel();
        optPane.setLayout(new BoxLayout(optPane, BoxLayout.Y_AXIS));
        optPane.add(optLabel);
        optPane.add(optFilePane);

        incCheck = new JCheckBox("Include files in this directory: ");
        incCheck.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
        incCheck.setForeground(actionColor);
        incCheck.setBackground(this.getBackground());
        incCheck.setSelected(incInc);

        incEntry = new JTextField(incPath);
        incEntry.setForeground(actionColor);
        incEntry.setBackground(this.getBackground());
	incEntry.setPreferredSize(new Dimension(300, 20));

        JPanel incPane = new JPanel();
        incPane.setLayout(new BoxLayout(incPane, BoxLayout.Y_AXIS));
        incPane.add(incCheck);
        incPane.add(incEntry);

        setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
        add(stdPane);
        add(Box.createRigidArea(new Dimension(0, 10)));
        add(optPane);
        add(Box.createRigidArea(new Dimension(0, 10)));
        add(incPane);

        stdFilePane.setBorder(BorderFactory.createEmptyBorder(0,50,0,5));
        optFilePane.setBorder(BorderFactory.createEmptyBorder(0,50,0,5));
        setBorder(BorderFactory.createEmptyBorder(20,5,5,5));

	updatePanel();
    }

    public void resetOpt() {
        recFiles = null;
    }

    public void updatePanel() {
        if(recFiles == null) {

          recFiles = new Hashtable();
          getStandardFiles();
          getOptionalFiles();
          getKeys();

        }
          incCheck.setSelected(incInc);
          incEntry.setText(incPath);

          if(optBoxes.length > 0) {
        int k = 0;
        int nLength = keys.length;
        for(int i=0; i<nLength; i++)
            if(recFiles.get(keys[i]).equals("opt")) {
                optBoxes[k].setSelected(true);
                k++;
            } else if(recFiles.get(keys[i]).equals("null")) {
                optBoxes[k].setSelected(false);
                k++;
            }
          }
    }

        private void initStandardFiles() {

    if(debug)
    System.out.println("initStandardFiles");

        recFiles.put("fid", "std");
        recFiles.put("procpar", "std");
        recFiles.put("log", "std");
        recFiles.put("text", "std");
        recFiles.put("data", "null");
        recFiles.put("phasefile", "null");
        recFiles.put("cmd history", "null");
        recFiles.put("global", "null");
        recFiles.put("conpar", "null");
        recFiles.put("shims", "null");
        recFiles.put("pulse sequence", "null");
        recFiles.put("waveforms", "null");
        recFiles.put("fdf", "null");
        recFiles.put("user macros", "null");
    }

        private void getStandardFiles() {

      String part11Root = System.getProperty("sysdir");
      if(part11Root != null) part11Root += "/p11";
      else part11Root = FileUtil.SYS_VNMR + "/p11";

      if(debug)
      System.out.println(new StringBuffer().append("getStandardFiles part11Root " ).
                         append( part11Root).toString());

          if(part11Root == null) {
        initStandardFiles();
        return;
      }

      String path = FileUtil.openPath(new StringBuffer().append(part11Root ).
                                      append( "/part11Config").toString());

          if(path == null) {
        initStandardFiles();
        return;
      }
      else {
            BufferedReader in;
            String inLine;

            try {
              in = new BufferedReader(new FileReader(path));
              while ((inLine = in.readLine()) != null) {
                if (inLine.length() <= 0 || inLine.startsWith("#")
			|| inLine.startsWith("@") ) continue;
                inLine.trim();
                StringTokenizer tok = new StringTokenizer(inLine, ":\n", false);
                if (tok.countTokens() == 4) {
                    String keyword1 = tok.nextToken();
                    String keyword2 = tok.nextToken();
                    String key = tok.nextToken();
                    String value = tok.nextToken();
                    if(keyword1.equals("file") && keyword2.equals("standard")
                       && value.equals("yes")) {
                        recFiles.remove(key);
                        recFiles.put(key, "std");
                    } else if(keyword1.equals("file") && keyword2.equals("standard")
                            && value.equals("no")) {
                      recFiles.remove(key);
                      recFiles.put(key, "null");
                    }
                }
              }
              in.close();
            } catch(IOException e) { System.err.println(new StringBuffer().append("ERROR: read " ).
                                                             append( path).toString()); }

        return;
      }
        }

        private void getOptionalFiles() {

      String path = FileUtil.openPath("PROPERTIES/recConfig");
      incPath = new StringBuffer().append(System.getProperty("userdir")).
                     append("/includingFiles").toString();

          if(path == null) {
        return;
      }
      else {
            BufferedReader in;
            String inLine;

            try {
              in = new BufferedReader(new FileReader(path));
              while ((inLine = in.readLine()) != null) {
                if (inLine.length() <= 0 || inLine.startsWith("#")
			|| inLine.startsWith("@") ) continue;
                inLine.trim();
                StringTokenizer tok = new StringTokenizer(inLine, ":\n", false);
                if (tok.countTokens() == 4) {
                    String keyword1 = tok.nextToken();
                    String keyword2 = tok.nextToken();
                    String key = tok.nextToken();
                    String value = tok.nextToken();
                    String v = (String)recFiles.get(key);
                    if(keyword2.equals("include")) {
                        incPath = key;
                        if(value.equals("yes")) incInc = true;
                        else incInc = false;
                    } else if((v == null || !v.equals("std")) && keyword1.equals("file")
                              && keyword2.equals("optional") && value.equals("yes")) {
                        recFiles.remove(key);
                        recFiles.put(key, "opt");
                    } else if((v == null || !v.equals("std")) && keyword1.equals("file")
                              && keyword2.equals("optional") && value.equals("no")) {
                        recFiles.remove(key);
                        recFiles.put(key, "null");
                    }
                }
              }
              in.close();
            } catch(IOException e) { System.err.println(new StringBuffer().append("ERROR: read " ).
                                                             append( path).toString()); }

        return;
      }
        }

    private void getKeys() {

        int n = recFiles.size();

            keys = new String[n];
            int ct = 0;
            for(Enumeration en = recFiles.keys(); en.hasMoreElements(); ) {
                keys[ct] = (String) en.nextElement();
                ct++;
            }
    }

        public void saveOptionalFiles() {

      String strFile = "USER/PROPERTIES/recConfig";

          String path = FileUtil.savePath(strFile);

    if(debug)System.out.println(new StringBuffer().append("saveOptionalFiles path ").
                                append(path).toString());

          if (path == null)
          {
            Messages.postDebug("Permission denied for writing file " + strFile);
            return;
          }

          try
          {
            FileWriter fw = new FileWriter(path);

            for(int i=0; i<keys.length; i++)
            {
                if(recFiles.get(keys[i]).equals("opt"))
                {
                    String str =new StringBuffer().append( "file:optional:" ).
                                     append( keys[i] ).append(":yes\n").toString();
                    if(debug) System.out.println(new StringBuffer().append("optional file " ).
                                                      append(str).toString());
                    fw.write(str, 0, str.length());
                }
            }

        String str =new StringBuffer().append( "file:include:").
                         append(incEntry.getText() ).append(":no\n").toString();
        if(incCheck.isSelected())
            str =new StringBuffer().append( "file:include:").
                      append(incEntry.getText() ).append(":yes\n").toString();
        fw.write(str, 0, str.length());

            fw.flush();
            fw.close();
          }
          catch(IOException e)
          {
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
          }
        }
    }

    public static void sortStrVector(Vector v) {

        for (int i=0; i < v.size(); i++) {
            for (int j=i; j > 0 &&
		((String)v.elementAt(j-1)).compareTo((String)v.elementAt(j))
		> 0; j--) {

                // Swap rows j and j-1
                Object a = v.elementAt(j);
                v.remove(j);
                v.add(j, v.elementAt(j-1));
                v.remove(j-1);
                v.add(j-1, a);
            }
        }
    }

    protected String readFirstLine(String path) {

	if(path != null) {

          BufferedReader in;
          String inLine;

          try {
            in = new BufferedReader(new FileReader(path));
          // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() <= 0 || inLine.startsWith("#")
			|| inLine.startsWith("@") ) continue;
                inLine.trim();
                if(inLine.indexOf(":") != -1 || inLine.indexOf("=") != -1)
                    return inLine;
            }
            in.close();
          } catch(IOException e) { System.err.println(new StringBuffer().append("ERROR: read ").
                                                           append( path).toString()); }

    	}
    	return "";
    }

    protected Hashtable readHash(String path) {

	Hashtable hash = new Hashtable();

	if(path != null) {

          BufferedReader in;
          String inLine;

          try {
            in = new BufferedReader(new FileReader(path));
          // Read one line at a time.
            while ((inLine = in.readLine()) != null) {
                if (inLine.length() <= 0 || inLine.startsWith("#")
			|| inLine.startsWith("@") ) continue;
                inLine.trim();
            // Create a Tokenizer object to work with including ':'
                StringTokenizer tok;
                if(inLine.indexOf(":") == -1)
                    tok = new StringTokenizer(inLine, " ,\n", false);
                else
                    tok = new StringTokenizer(inLine, ":,\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    String dir = tok.nextToken();
                    if(!hash.containsKey(key))
                        hash.put(key, dir);
                }
            }
            in.close();
          } catch(IOException e) { System.err.println(new StringBuffer().append("ERROR: read ").
                                                           append( path).toString()); }

    	}
    	return hash;
    }

    protected void saveHashToFile(Hashtable hash, String selectedKey, String path,
	boolean append) {
    // save selected item first.

        if (path == null || hash == null) return;

        try
        {
            FileWriter fw = new FileWriter(path, append);

	    String key;
	    String value;
	    String str;

	    if(selectedKey != null) {
		value = (String)hash.get(selectedKey);
	    	if(value != null) {
	    	    str =new StringBuffer().append( selectedKey ).append(":").
                          append( value ).append("\n").toString();
	    	    fw.write(str, 0, str.length());
		}
	    }

	    for(Enumeration en = hash.keys(); en.hasMoreElements(); ) {
	    	key = (String) en.nextElement();
		if(key != null && !key.equals(selectedKey)) {
		    value = (String)hash.get(key);
		    if(value != null) {
                    	str =new StringBuffer().append( key ).append( ":" ).
                                  append( value ).append( "\n").toString();
                    	if(debug)System.out.println(new StringBuffer().append("save " ).
                                                         append(str ).append(" to ").
                                                         append(path).toString());
                    	fw.write(str, 0, str.length());
		    }
		}
            }
            fw.flush();
            fw.close();
        }
        catch(IOException e)
        {
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

  class DataDirPanel extends JPanel {

    protected Hashtable p11ParentDirs = null;
    protected Hashtable dataParentDirs = null;
    protected Hashtable parentDirs_ds = null;
    protected Hashtable childDirs = null;
    protected Hashtable allKeysTypes = null;
    protected Hashtable allKeysSpaces = null;
    protected Hashtable allKeysPaths = null;
    protected Vector allKeys = null;
    protected Hashtable allLongKeysPaths = null;
    protected Vector allLongKeys = null;

    protected Hashtable removedChildDirs = new Hashtable();
    private JButton removeDirButton;

    private JComboBox dirMenu;
    private JButton newDirButton;
    private JTextField pathEntry;
    private JTextField keyEntry;
    private String currentDirKey = null;
    private String currentDir = null;

    public DataDirPanel() {

	getImages();

    	makeDirPane();

    }

    private void getImages() {

	images = new ImageIcon[4];
        images[0] = Util.getImageIcon("blueDir.gif");
        images[1] = Util.getImageIcon("blueChildDir.gif");
	images[2] = Util.getImageIcon("yellowDir.gif");
	images[3] = Util.getImageIcon("yellowChildDir.gif");
    }

    private void makeDirPane() {

	updateDirHashtables();

	JLabel tlabel = new JLabel("Select a data directory:", JLabel.LEFT);
        tlabel.setForeground(infoColor);

	JLabel blabel = new JLabel("with label:", JLabel.LEFT);
        blabel.setForeground(actionColor);

	JLabel blank = new JLabel(" ");

	dirMenu = new JComboBox(allLongKeys);

	DirComboBoxRenderer renderer = new DirComboBoxRenderer();
        dirMenu.setRenderer(renderer);
        dirMenu.setMaximumRowCount(6);
        dirMenu.setPreferredSize(new Dimension(240, 20));
	if(currentDirKey != null) {
            dirMenu.setSelectedItem(currentDirKey);
	}

        dirMenu.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox)e.getSource();
            currentDirKey = (String)cb.getSelectedItem();
            currentDir = (String) allLongKeysPaths.get(currentDirKey);
	    pathEntry.setText(currentDir);

            if(resultPane != null) resultPane.reformat();

	    setRemoveButton(currentDir);

          }
        });

	keyEntry = new JTextField();
	keyEntry.setText("new project");
        keyEntry.setEnabled(false);

	pathEntry = new JTextField();

        pathEntry = new JTextField(currentDir);
        pathEntry.setForeground(actionColor);
        pathEntry.setPreferredSize(new Dimension(280, 20));
        pathEntry.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            currentDir = pathEntry.getText();

            if(resultPane != null) resultPane.reformat();

            if(!newDirButton.isEnabled())
                newDirButton.setEnabled(true);

	    setRemoveButton(currentDir);

            keyEntry.setEnabled(true);
	    int ind = currentDir.lastIndexOf("/");
	    String str = "";
	    if(ind > 0) str = currentDir.substring(ind+1);
	    keyEntry.setText(str);
          }
        });

	newDirButton = new JButton("Save new directory");
        newDirButton.setForeground(actionColor);
        newDirButton.setEnabled(false);
        newDirButton.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {

	    String key = keyEntry.getText();
	    String path = pathEntry.getText();
            if(key != null && key.length() > 0) {

		if(childDirs.containsKey(key)) {
		    path = (String)allLongKeysPaths.get(currentDirKey);
		    pathEntry.setText(path);
		    Messages.postError(new StringBuffer().append(key ).
                               append(" already exists.").toString());
		} else if(!isChild(path)) {
		    path = (String)allLongKeysPaths.get(currentDirKey);
		    pathEntry.setText(path);
		    Messages.postError(new StringBuffer().append(path ).
                               append( " is not a child of parent data dirs.").toString());
		} else {

		    childDirs.put(key,path);
		    updateAllLists();

                    currentDirKey = key + getFreeSpace(path);
                    currentDir = path;

		    dirMenu.setModel(new DefaultComboBoxModel(allLongKeys));
        	    dirMenu.setSelectedItem(currentDirKey);
		}
            	newDirButton.setEnabled(false);
		setRemoveButton(currentDir);
		keyEntry.setText("new project");
            	keyEntry.setEnabled(false);
            } else Messages.postInfo("need a name for new directory.");
          }
        });

	removeDirButton = new JButton("Remove selected directory");
        removeDirButton.setForeground(actionColor);
	setRemoveButton(currentDir);
        removeDirButton.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {

            if(currentDirKey == null || currentDirKey.length() == 0) {
            	Messages.postInfo("directory is not selected.");
	    } else if(!removeIsOk(currentDir)) {
		if(isParent(currentDir))
            	Messages.postError("Cannot remove a parent directory.");
	        else if(isP11Dir(currentDir))
            	Messages.postError("Cannot remove a p11 directory.");
	        else if(!canWrite(currentDir))
            	Messages.postError("Permission denied.");
	    } else if(hasChild(currentDir)) {
		ConfirmPopup popup = new ConfirmPopup("Remove Directory");
		popup.setLocationRelativeTo(tempPane);
		popup.setVisible(true);
	    } else if(childDirs.contains(currentDir)) {
		removeDirAction();
            } else Messages.postInfo(new StringBuffer().append(currentDir ).
                                     append( " is not in the list.").toString());
          }
        });

	JPanel menuPane = new JPanel();
	menuPane.setLayout(new BoxLayout(menuPane, BoxLayout.X_AXIS));
	menuPane.add(dirMenu);
	menuPane.add(pathEntry);

	JPanel buttonPane = new JPanel();
	buttonPane.setLayout(new BoxLayout(buttonPane, BoxLayout.X_AXIS));
	buttonPane.add(Box.createRigidArea(new Dimension(240, 20)));
	buttonPane.add(newDirButton);
	buttonPane.add(blabel);
	buttonPane.add(keyEntry);

	JPanel removeButtonPane = new JPanel();
	removeButtonPane.setLayout(new BoxLayout(removeButtonPane,
		BoxLayout.X_AXIS));
	removeButtonPane.add(Box.createRigidArea(new Dimension(240, 20)));
	removeButtonPane.add(removeDirButton);

        setLayout(new GridLayout(0, 1));
	add(tlabel);
	add(menuPane);
	add(buttonPane);
	if(part11Mode != 2)
	add(removeButtonPane);
	add(Box.createRigidArea(new Dimension(20, 20)));

        setBorder(new CompoundBorder(BorderFactory.createEtchedBorder(),
                        BorderFactory.createEmptyBorder(0,5,5,5)));

        //setBorder(BorderFactory.createEmptyBorder(5,5,5,5));
    }

    public void writeRemovedChildDirs() {
        String path = FileUtil.savePath("USER/PROPERTIES/projectDirs.del");

        if (path == null)
        {
            Messages.postDebug("Permission denied for writing file " + path);
            return;
        }

	saveHashToFile(removedChildDirs, null, path, true);
    }

    public void removeChildDirs() {
        for(Enumeration en = removedChildDirs.keys(); en.hasMoreElements(); ) {
            String key = (String) en.nextElement();
	    String path = (String) removedChildDirs.get(key);
	    if(fileExists(path) && canWrite(path)) {
        	String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, "rm -rf "+path};
        	WUtil.runScriptInThread(cmd);
	    }
	}
    }

    public void removeDirAction() {

	if(currentDirKey == null || currentDir == null) return;

	String key;
	int ind = currentDirKey.lastIndexOf(" (");
	if(ind > 0) key = currentDirKey.substring(0,ind);
	else key = currentDirKey;

	if(childDirs.containsKey(key)) {
	  childDirs.remove(key);
	  updateAllLists();
	  if(hasChild(currentDir)) removedChildDirs.put(key, currentDir);

	  if(allLongKeys.size() > 0) {
            currentDirKey = (String)allLongKeys.elementAt(0);
            currentDir = (String)allLongKeysPaths.get(currentDirKey);

          } else {
            currentDirKey = "";
            currentDir = "";
	  }
          dirMenu.setModel(new DefaultComboBoxModel(allLongKeys));
          dirMenu.setSelectedItem(currentDirKey);
	  pathEntry.setText(currentDir);
    	  if(resultPane != null) resultPane.reformat();
          setRemoveButton(currentDir);
        }
    }

    private boolean removeIsOk(String path) {

	if(path == null) return false;

	// allow to remove any empty child dir
	if(!isParent(path) && !fileExists(path))
	    return true;

	else if(!isParent(path) && !isP11Dir(path) && canWrite(path))
	    return true;
	else return false;
    }

    private void setRemoveButton(String path) {

	if(currentDirKey == null || currentDir == null) return;

	if(!path.equals((String)allLongKeysPaths.get(currentDirKey)))
            removeDirButton.setEnabled(false);

	else if(removeIsOk(path))
		removeDirButton.setEnabled(true);
	else
            removeDirButton.setEnabled(false);
    }

    public boolean canWrite(String path) {
	if(path == null) return false;
	File file = new File(path);
	return file.canWrite();
    }

    public boolean hasChild(String path) {

	if(path == null) return false;

	File file = new File(path);
	if(file == null) return false;

	String children[] = file.list();
	if(children != null && children.length > 0) return true;
	else return false;
    }

    public void destroy() {
	removeAll();
    }

    private void updateAllLists() {

    	getallKeysValues();
    	sortallKeys();
    	makeallLongKeys();

    }

    public boolean isParent(String path) {

	if(path == null) return false;

        for(Enumeration en = p11ParentDirs.keys(); en.hasMoreElements(); ) {
            String key = (String) en.nextElement();
	    if(path.equals((String) p11ParentDirs.get(key)))
	    return true;
	}

        for(Enumeration en = dataParentDirs.keys(); en.hasMoreElements(); ) {
            String key = (String) en.nextElement();
	    if(path.equals((String) dataParentDirs.get(key)))
	    return true;
	}
	return false;
    }

    public boolean isChild(String path) {
	if(part11Mode != 0)
        for(Enumeration en = p11ParentDirs.keys(); en.hasMoreElements(); ) {
            String key = (String) en.nextElement();
	    if(path.indexOf((String) p11ParentDirs.get(key)) != -1)
	    return true;
	}

	if(part11Mode != 2)
        for(Enumeration en = dataParentDirs.keys(); en.hasMoreElements(); ) {
            String key = (String) en.nextElement();
	    if(path.indexOf((String) dataParentDirs.get(key)) != -1)
	    return true;
	}
	return false;
    }

    private void updateDirHashtables() {

    	getp11ParentDirs();
    	getdataParentDirs();
    	getparentDirs_ds();
    	getchildDirs();
	updateAllLists();
	initCurrent();
    }

    public void updateDirPane() {

	updateDirHashtables();
	dirMenu.setModel(new DefaultComboBoxModel(allLongKeys));
        dirMenu.setSelectedItem(currentDirKey);
	pathEntry.setText(currentDir);
        newDirButton.setEnabled(false);
	setRemoveButton(currentDir);
	keyEntry.setText("new project");
        keyEntry.setEnabled(false);

    }

    public String getCurrentDir() {
	return currentDir;
    }

    protected void getp11ParentDirs() {

    	p11ParentDirs = readP11ParentDirs();
    }

    protected void getdataParentDirs() {

        dataParentDirs = readDataDirs();
    }

    protected void getchildDirs() {

        childDirs = readChildDirs();

    }

    protected void initCurrent() {

	String line = readDirFirstLine();
        StringTokenizer tok;
        if(line.indexOf(":") == -1)
        tok = new StringTokenizer(line, " ,\n", false);
        else
        tok = new StringTokenizer(line, ":,\n", false);
        if (tok.countTokens() > 1) {
            currentDirKey = tok.nextToken();
            currentDir = tok.nextToken();
	    currentDirKey += getFreeSpace(currentDir);
	} else if(allLongKeys != null && allLongKeys.size() > 0) {
	    currentDirKey = (String)allLongKeys.elementAt(0);
	    currentDir = (String)allLongKeysPaths.get(currentDirKey);
	}
    }

    protected void getparentDirs_ds() {

	parentDirs_ds = new Hashtable();

	for(Enumeration en = p11ParentDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String path = (String) p11ParentDirs.get(key);
	    String space = getfreeds(path);
	    if(space.length() > 0 && !parentDirs_ds.containsKey(path))
	    parentDirs_ds.put(path, space);
        }

	for(Enumeration en = dataParentDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String path = (String) dataParentDirs.get(key);
	    String space = getfreeds(path);
	    if(space.length() > 0 && !parentDirs_ds.containsKey(path))
	    parentDirs_ds.put(path, space);
        }

    }

    protected String getfreeds(String path) {

        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, "df -b "+path};
        WMessage msg = WUtil.runScript(cmd);
	String str = "";
        if(msg.isNoError()) {
            str = msg.getMsg();
        }
	if(str.length() > 0) {
	    StringTokenizer tok = new StringTokenizer(str, " \n", false);
	    if(tok.countTokens() > 1) {
		tok.nextToken();
		String ds = tok.nextToken();
		String space = new StringBuffer().append(" (").append(String.valueOf((Integer.parseInt(ds))/1000)).
                            append( " Mb)").toString();
		return space;
	    }
	}
	return "";
    }

    protected Hashtable readP11ParentDirs() {

	String path = FileUtil.openPath(P11DIR+m_userName);

    	if(debug) System.out.println(new StringBuffer().append("Part11Dirs path " ).
                                     append(path).toString());

	return(readHash(path));
    }

    protected String readDirFirstLine() {
	String path = FileUtil.openPath(CHILDDIR);
	return readFirstLine(path);
    }

    protected Hashtable readChildDirs() {

	String path = FileUtil.openPath(CHILDDIR);

    	if(debug) System.out.println(new StringBuffer().append("childDirs path " ).
                                     append(path).toString());

	return(readHash(path));
    }

    protected Hashtable readDataDirs() {

	String path = FileUtil.openPath(DATADIR+m_userName);

    	if(debug) System.out.println(new StringBuffer().append("dataDirs path " ).
                                     append(path).toString());

	return(readHash(path));
    }

    public boolean isP11Dir(String path) {

	if(path == null) return false;

	for(Enumeration en = p11ParentDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    if(path.indexOf((String) p11ParentDirs.get(key)) != -1)
		return true;
	}

	return false;
    }

    protected void getallKeysValues() {
	allKeysPaths = new Hashtable();
	allKeysSpaces = new Hashtable();
	allKeysTypes = new Hashtable();

	if(part11Mode != 0)
	for(Enumeration en = p11ParentDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String path = (String) p11ParentDirs.get(key);
	    if(!allKeysPaths.containsKey(key))
	    allKeysPaths.put(key,path);
	    if(!allKeysSpaces.containsKey(key))
	    allKeysSpaces.put(key, getFreeSpace(path));
	    if(!allKeysTypes.containsKey(key))
	    allKeysTypes.put(key,"P11PARENT");
        }

	if(part11Mode != 2)
	for(Enumeration en = dataParentDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String path = (String) dataParentDirs.get(key);
	    if(!allKeysPaths.containsKey(key))
	    allKeysPaths.put(key,path);
	    if(!allKeysSpaces.containsKey(key))
	    allKeysSpaces.put(key, getFreeSpace(path));
	    if(!allKeysTypes.containsKey(key))
	    allKeysTypes.put(key,"DATAPARENT");
        }

	for(Enumeration en = childDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String path = (String) childDirs.get(key);
	    if(part11Mode != 2 && !isP11Dir(path)) {
	    	if(!allKeysTypes.containsKey(key))
	    	allKeysTypes.put(key, "DATACHILD");
	        if(!allKeysPaths.containsKey(key))
	        allKeysPaths.put(key,path);
	        if(!allKeysSpaces.containsKey(key))
	        allKeysSpaces.put(key, getFreeSpace(path));
	    } else if(part11Mode != 0 && isP11Dir(path)) {
	    	if(!allKeysTypes.containsKey(key))
	    	allKeysTypes.put(key,"P11CHILD");
	        if(!allKeysPaths.containsKey(key))
	        allKeysPaths.put(key,path);
	        if(!allKeysSpaces.containsKey(key))
	        allKeysSpaces.put(key, getFreeSpace(path));
	    }
        }

    }

    protected void sortallKeys() {
	allKeys = new Vector();

	// sort p11 dirs and data dirs separately

 	Vector p11Dirs = new Vector();
 	Vector dataDirs = new Vector();

	for(Enumeration en = allKeysTypes.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String type = (String)allKeysTypes.get(key);
	    if(type.startsWith("P11")) {
	 	p11Dirs.add(key);
	    } else if(type.startsWith("DATA")) {
	 	dataDirs.add(key);
	    }
	}

	sortStrVector(p11Dirs);
	sortStrVector(dataDirs);

	if(part11Mode != 0)
	for(int i=0; i<p11Dirs.size(); i++) allKeys.add(p11Dirs.elementAt(i));
	if(part11Mode != 2)
	for(int i=0; i<dataDirs.size(); i++) allKeys.add(dataDirs.elementAt(i));
    }

    public boolean fileExists(String path) {
	File file = new File(path);
	return file.exists();
    }

    public void createDirs() {
	for(Enumeration en = childDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String path = (String) childDirs.get(key);
	    if(!fileExists(path) && !isP11Dir(path)) {
        	String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, "mkdir -p "+path};
        	WUtil.runScriptInThread(cmd);
	    }
	}

    }

    public void saveChildDirs() {

        String path = FileUtil.savePath(CHILDDIR);

        if (path == null)
        {
            Messages.postDebug("Permission denied for writing file " + path);
            return;
        }

	Hashtable hash = new Hashtable();
	if(part11Mode != 0)
	for(Enumeration en = p11ParentDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String value = (String) p11ParentDirs.get(key);
	    hash.put(key, value);
	}
	if(part11Mode != 2)
	for(Enumeration en = dataParentDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String value = (String) dataParentDirs.get(key);
	    hash.put(key, value);
	}
	for(Enumeration en = childDirs.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    String value = (String) childDirs.get(key);
	    hash.put(key, value);
	}

	int ind = currentDirKey.lastIndexOf(" (");
	String selected;
	if(ind > 0) selected = currentDirKey.substring(0,ind);
	else selected = currentDirKey;
	saveHashToFile(hash, selected, path, false);
   }

    private String getFreeSpace(String path) {
	for(Enumeration en = parentDirs_ds.keys(); en.hasMoreElements(); ) {
	    String key = (String) en.nextElement();
	    if(path.indexOf(key) != -1)
		return(String)parentDirs_ds.get(key);
	}
	return new String("");
    }

    protected void makeallLongKeys() {
	allLongKeys = new Vector();
	allLongKeysPaths = new Hashtable();
	allLongKeysTypes = new Hashtable();
    int nLength = allKeys.size();

	for(int i=0; i<nLength; i++) {
	    String key = (String)allKeys.elementAt(i);
	    String path = (String)allKeysPaths.get(key);
	    String type = (String)allKeysTypes.get(key);
	    String space = (String)allKeysSpaces.get(key);
	    if(key != null && space != null) {
            String keyspace = new StringBuffer().append(key).append(space).toString();
	    	allLongKeys.add(keyspace);
	        if(path != null) allLongKeysPaths.put(keyspace, path);
	    	if(type != null) allLongKeysTypes.put(keyspace, type);
	    }
	}
    }

  }

  class DirComboBoxRenderer extends JLabel
                                implements ListCellRenderer {

    public DirComboBoxRenderer() {
        setOpaque(true);
        setHorizontalAlignment(LEFT);
        setVerticalAlignment(CENTER);
    }

    public Component getListCellRendererComponent(
        JList list,
        Object value,
        int index,
        boolean isSelected,
        boolean cellHasFocus)
    {

        if (isSelected) {
            setBackground(UIManager.getColor("ComboBox.selectionBackground"));
            setForeground(UIManager.getColor("ComboBox.selectionForeground"));
        } else {
            setBackground(UIManager.getColor("ComboBox.background"));
            setForeground(UIManager.getColor("ComboBox.foreground"));
        }

        String key = (String)value;

        if(allLongKeysTypes != null && key != null ) {
            String type = (String)allLongKeysTypes.get(key);
            if(type != null) {
                if(type.startsWith("P11PARENT")) setIcon(images[0]);
                else if(type.startsWith("P11CHILD")) setIcon(images[1]);
                else if(type.startsWith("DATAPARENT")) setIcon(images[2]);
                else if(type.startsWith("DATACHILD")) setIcon(images[3]);
            }
        }

        setText(key);
        return this;
    }
  }

  class ConfirmPopup extends ModalDialog {
	JLabel msg =
	new JLabel("Directory is not empty. Delete it anyway???");

	public ConfirmPopup(String title) {
	  super(title);

	  JPanel pane = new JPanel();
	  pane.add(msg);
          pane.setBorder(BorderFactory.createEmptyBorder(20,20,20,20));

          getContentPane().add(pane, BorderLayout.NORTH);

	  okButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		dirPane.removeDirAction();
		setVisible(false);
	    }
	  });

	  cancelButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent evt) {
		setVisible(false);
	    }
	  });

	  pack();

	}
  }

}

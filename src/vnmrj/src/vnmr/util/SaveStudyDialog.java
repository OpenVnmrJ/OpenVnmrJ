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
import javax.swing.event.*;
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

public class SaveStudyDialog extends ModalDialog
                      implements ActionListener, VObjDef {

    private static SaveStudyDialog saveStudyDialog = null;
    private boolean debug = false;

// these are needed for a modal dialog to talk to vnmr
    private SessionShare sshare;
    private ButtonIF vnmrIf;
    protected Hashtable allLongKeysTypes = null;
    protected ImageIcon images[];

    protected static TemplatePanel tempPane = null;
    protected static TemplatePanel tempPane2 = null;
    protected static TemplatePanel tempPane3 = null;
    protected static ResultPanel resultPane = null;
    protected static ResultPanel resultPane2 = null;
    protected static ResultPanel resultPane3 = null;
    protected static ResultPanel resultPane4 = null;
    protected static RecordFilesPanel recfilePane = null;
    protected static JTabbedPane tabbedPane = null;
    protected static DataDirPanel dirPane = null;
    protected static DataDirPanel dirPane2 = null;

    public static Color infoColor = Color.blue;
    public static Color actionColor = Color.black;
    public static Color FDAColor = Color.blue;
    public static Color highlightColor = Color.yellow;

    protected String m_userName = null;

    protected static final String DATADIR = "PROFILES/data/";
    protected static final String P11DIR = "PROFILES/p11/";
/*
    protected static final String CHILDDIR = "USER/PROPERTIES/projectDirs";
    protected static final String STUDYCHILDDIR = "USER/PROPERTIES/studyDirs";
*/
    protected static final String CHILDDIR = "USER/PERSISTENCE/projectDirs";
    protected static final String STUDYCHILDDIR = "USER/PERSISTENCE/studyDirs";

    protected static final String TEMPLATEDIR = "PROFILES/templates/";
    protected static final String STEMPLATEDIR = "PROFILES/studytemplates/";
/*
    protected static final String USRTEMPLATES1 = "USER/PROPERTIES/studyname_templates";
    protected static final String USRTEMPLATES2 = "USER/PROPERTIES/autoname_templates";
    protected static final String USRTEMPLATES3 = "USER/PROPERTIES/filename_templates";
*/
    protected static final String USRTEMPLATES1 = "USER/PERSISTENCE/studyname_templates";
    protected static final String USRTEMPLATES2 = "USER/PERSISTENCE/autoname_templates";
    protected static final String USRTEMPLATES3 = "USER/PERSISTENCE/filename_templates";

    protected static final String STUDYTEMPLATEDIR = "PROPERTIES/studyname_templates";
    protected static final String FILETEMPLATEDIR = "PROPERTIES/filename_templates";

    String dirParam = "globalauto";
    String tempParam1 = "sqname";
    String tempParam2 = "autoname";
    String dirParam2 = "svfdir";
    String tempParam3 = "svfname";

    MyTextMsg2 vnmrValue = null;

    protected static int part11Mode = 0;
    // 0 non-p11, 1 both, 2 p11 only.

    String m_appTypes = null;

    public SaveStudyDialog(SessionShare sshare, ButtonIF vif, String title, String helpFile) {
        super(title);

        this.sshare = sshare;
        this.vnmrIf = vif;
        m_strHelpFile = helpFile;

        vnmrValue = new MyTextMsg2(sshare, vnmrIf, "");

        part11Mode = getPart11Mode();

        if(debug) System.out.println("part11Mode " + part11Mode);

        JPanel rPane = null;
        if(part11Mode != 0) {

            recfilePane = new RecordFilesPanel();

            rPane = new JPanel();
            rPane.setLayout(new BoxLayout(rPane, BoxLayout.Y_AXIS));
            rPane.add(recfilePane);
            rPane.add(Box.createRigidArea(new Dimension(100, 150)));
        }
        if(DebugOutput.isSetFor("savedatasetup"))
            Messages.postDebug("SaveStudyDialog: Getting LoginService");

        LoginService loginService = LoginService.getDefault();
        String user = System.getProperty("user.name");
        ArrayList apptypes = loginService.getUser(user).getAppTypes();
        if(apptypes.size() > 0) m_appTypes = (String)apptypes.get(0);

        m_userName = System.getProperty("user.name");

        JPanel studyPane = new JPanel();
        studyPane.setLayout(new BoxLayout(studyPane, BoxLayout.Y_AXIS));

        JPanel nonstudyPane = new JPanel();
        nonstudyPane.setLayout(new BoxLayout(nonstudyPane, BoxLayout.Y_AXIS));

        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BoxLayout(mainPanel, BoxLayout.Y_AXIS));

        if(m_appTypes.equals(Global.WALKUPIF) && part11Mode > 0) {
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: Walkup and Part11");

           dirParam = "globalauto";
           tempParam1 = "sqname";

           dirPane = new DataDirPanel(Util.getLabel("_Select_automation_directory"), STUDYCHILDDIR, "study", 0);
           studyPane.add(dirPane);

           tempPane = new TemplatePanel(Util.getLabel("_Select_study_name_template"), 
                USRTEMPLATES1, "study");
           studyPane.add(tempPane);

           //resultPane = new ResultPanel("Studies will be saved to:", "study");
           resultPane = new ResultPanel(Util.getLabel("_Example_of_study_path_in_current_automation"), "study");
           studyPane.add(resultPane);

           resultPane4 = new ResultPanel(Util.getLabel("_Example_of_study_path_in_next_autonation"), "nextautostudy");
           studyPane.add(resultPane4);

           dirParam2 = "svfdir";
           tempParam2 = "autoname";
           tempParam3 = "svfname";

           dirPane2 = new DataDirPanel(Util.getLabel("_Select_data_directory"), CHILDDIR, "data", 1);

           nonstudyPane.add(dirPane2);

           tempPane3 = new TemplatePanel(Util.getLabel("_Select_data_name_template"), 
                USRTEMPLATES2, "data");
           nonstudyPane.add(tempPane3);

           resultPane2 = new ResultPanel(Util.getLabel("_Example_of_automation_data_path"), "studydata");
           nonstudyPane.add(resultPane2);

           resultPane3 = new ResultPanel(Util.getLabel("_Example_of_non-automation_data_path"), "data");
           nonstudyPane.add(resultPane3);

           tabbedPane = new JTabbedPane();

           tabbedPane.addTab(Util.getLabel("_Studies"), null, studyPane, "");
           tabbedPane.addTab(Util.getLabel("_Data"), null, nonstudyPane, "");
           tabbedPane.addTab(Util.getLabel("_Optional_files"), null, rPane, "");

           tabbedPane.setSelectedIndex(0);
           tabbedPane.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
           tabbedPane.setForeground(actionColor);

           tabbedPane.addChangeListener(new ChangeListener() {
              public void stateChanged(ChangeEvent e) {
                String t = tabbedPane.getTitleAt(tabbedPane.getSelectedIndex());
                if(resultPane2 != null && t != null &&
                        t.equals("Data")) resultPane2.reformat();
                if(resultPane3 != null && t != null &&
                        t.equals("Data")) resultPane3.reformat();
              }
           });

           mainPanel.add(tabbedPane);
        } else if(m_appTypes.equals(Global.WALKUPIF) || 
                  m_appTypes.equals(Global.IMGIF)) {
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: " + m_appTypes + " mode");

            // If Walkup or imaging, use the same general panel layout
            // Just change the title.
            String selectString="";
           
            if(m_appTypes.equals(Global.WALKUPIF)) {
                selectString = Util.getLabel("_Select_automation_directory");
            } else if(m_appTypes.equals(Global.IMGIF)) {
                selectString = Util.getLabel("_Select_study_directory");
            }

            // Now create the panel for both walkup and imaging
            dirParam = "globalauto";
            tempParam1 = "sqname";
            tempParam2 = "autoname";

            dirPane = new DataDirPanel(selectString, STUDYCHILDDIR, "study", 0);
            studyPane.add(dirPane);

            tempPane = new TemplatePanel(Util.getLabel("_Select_study_name_template"), 
                                         USRTEMPLATES1, "study");
            studyPane.add(tempPane);

            tempPane2 = new TemplatePanel(Util.getLabel("_Select_data_name_template"), 
                                          USRTEMPLATES2, "studydata");
            studyPane.add(tempPane2);

            resultPane = new ResultPanel(Util.getLabel("_Example_of_saved_study"), "study");
            studyPane.add(resultPane);

            resultPane3 = new ResultPanel(Util.getLabel("_Example_of_saved_data"), "studydata");
            studyPane.add(resultPane3);

            dirParam2 = "svfdir";
            tempParam3 = "svfname";

            dirPane2 = new DataDirPanel(Util.getLabel("_Select_data_directory"), CHILDDIR, "data", 0);
            nonstudyPane.add(dirPane2);

            tempPane3 = new TemplatePanel(Util.getLabel("_Select_data_name_template"), 
                                          USRTEMPLATES3, "data");
            nonstudyPane.add(tempPane3);

            resultPane2 = new ResultPanel(Util.getLabel("_Example_of_saved_data"), "data");
            nonstudyPane.add(resultPane2);

            tabbedPane = new JTabbedPane();

            tabbedPane.addTab(Util.getLabel("_Study_Data"), null, studyPane, "");
            tabbedPane.addTab(Util.getLabel("_Non_Study_Data"), null, nonstudyPane, "");

            tabbedPane.setSelectedIndex(0);
            tabbedPane.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
            tabbedPane.setForeground(actionColor);

            tabbedPane.addChangeListener(new ChangeListener() {
                    public void stateChanged(ChangeEvent e) {
                        String t = tabbedPane.getTitleAt(tabbedPane.getSelectedIndex());
                        if(resultPane3 != null && t != null &&
                           t.equals("Studies")) resultPane3.reformat();
                        else if(resultPane2 != null && t != null &&
                                t.equals("Non Studies")) resultPane2.reformat();
                    }
                });

            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: Adding tabbedPane to mainPanel");

            mainPanel.add(tabbedPane);

        } else if(m_appTypes.equals(Global.LCIF)) {
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: " + m_appTypes + " mode");

           dirParam2 = "svfdir";
           tempParam3 = "svfname";

           dirPane2 = new DataDirPanel(Util.getLabel("_Select_data_directory"), CHILDDIR, "data", 0);
           nonstudyPane.add(dirPane2);

           mainPanel.add(nonstudyPane);

        } else if(part11Mode > 0) {
           
           dirParam2 = "svfdir";
           tempParam3 = "svfname";

           dirPane2 = new DataDirPanel(Util.getLabel("_Select_data_directory"), CHILDDIR, "data", 1);
           nonstudyPane.add(dirPane2);

           tempPane3 = new TemplatePanel(Util.getLabel("_Select_data_name_template"), 
                USRTEMPLATES3, "data");
           nonstudyPane.add(tempPane3);

           resultPane2 = new ResultPanel(Util.getLabel("_Example_of_saved_data"), "data");
           nonstudyPane.add(resultPane2);

           tabbedPane = new JTabbedPane();

           tabbedPane.addTab(Util.getLabel("_Data"), null, nonstudyPane, "");
           tabbedPane.addTab(Util.getLabel("_Optional_files"), null, rPane, "");

           tabbedPane.setSelectedIndex(0);
           tabbedPane.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
           tabbedPane.setForeground(actionColor);

           mainPanel.add(tabbedPane);

        } else {
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: " + m_appTypes + " mode");

           dirParam2 = "svfdir";
           tempParam3 = "svfname";

           dirPane2 = new DataDirPanel(Util.getLabel("_Select_data_directory"), CHILDDIR, "data", 0);
           nonstudyPane.add(dirPane2);

           tempPane3 = new TemplatePanel(Util.getLabel("_Select_data_name_template"), 
                USRTEMPLATES3, "data");
           nonstudyPane.add(tempPane3);

           resultPane2 = new ResultPanel(Util.getLabel("_Example_of_saved_data"), "data");
           nonstudyPane.add(resultPane2);

           mainPanel.add(nonstudyPane);
        } 

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

        if(DebugOutput.isSetFor("savedatasetup"))
            Messages.postDebug("SaveStudyDialog: Adding Listeners");

        // Set the buttons up with Listeners
        okButton.addActionListener(this);
        okButton.setActionCommand("ok");
        cancelButton.addActionListener(this);
        cancelButton.setActionCommand("cancel");
        helpButton.addActionListener(this);
        helpButton.setActionCommand("help");

        // Make the frame fit its contents.
        pack();

        if(DebugOutput.isSetFor("savedatasetup"))
            Messages.postDebug("SaveStudyDialog: Panel creation complete");


    } // constructor

    public void destroy() {

        vnmrIf = null;
        if(tempPane != null) tempPane.destroy();
        if(tempPane2 != null) tempPane2.destroy();
        if(tempPane3 != null) tempPane3.destroy();
        if(dirPane != null) dirPane.destroy();
        if(dirPane2 != null) dirPane2.destroy();
        saveStudyDialog = null;
        removeAll();
        System.gc();
        System.runFinalization();
    }

    private void okAction() {
        if(dirPane != null) {
          String currentsvfdir = dirPane.getCurrentDir();
          if(currentsvfdir != null) {
                String cmd = "svaction('ok','"+dirParam+"','"+currentsvfdir+"')";
                ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
          }
          dirPane.saveChildDirs();
          dirPane.writeRemovedChildDirs();
          dirPane.createDirs();
          //dirPane.removeChildDirs();
        }
        if(dirPane2 != null) {
          String currentsvfdir = dirPane2.getCurrentDir();
          if(currentsvfdir != null) {
                String cmd = "svaction('ok','"+dirParam2+"','"+currentsvfdir+"')";
                ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
          }
          dirPane2.saveChildDirs();
          dirPane2.writeRemovedChildDirs();
          dirPane2.createDirs();
          //dirPane2.removeChildDirs();
        }
        if(tempPane != null) {
          String currenttemplate = tempPane.getCurrenttemplate();
          if(currenttemplate != null) {
                String cmd = "svaction('ok','"+tempParam1+"','"+currenttemplate+"')";
                ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
          }
          tempPane.savetemplate();
          tempPane.writeRemovedTemps();
        }
        if(tempPane2 != null) {
          String currenttemplate2 = tempPane2.getCurrenttemplate();
          if(currenttemplate2 != null) {
                String cmd = "svaction('ok','"+tempParam2+"','"+currenttemplate2+"')";
                ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
          }
          tempPane2.savetemplate();
          tempPane2.writeRemovedTemps();
        } 
        if(tempPane3 != null) {
          String currenttemplate3 = tempPane3.getCurrenttemplate();
          if(currenttemplate3 != null && part11Mode > 0 && m_appTypes.equals(Global.WALKUPIF)) {
                String cmd = "svaction('ok','"+tempParam2+"','$studyid$/data/"+currenttemplate3+"')";
                ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
                cmd = "svaction('ok','"+tempParam3+"','"+currenttemplate3+"')";
                ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
          } else if(currenttemplate3 != null) {
                String cmd = "svaction('ok','"+tempParam3+"','"+currenttemplate3+"')";
                ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
          }
          tempPane3.savetemplate();
          tempPane3.writeRemovedTemps();
        }
        if(recfilePane != null) recfilePane.saveOptionalFiles();
            setVisible(false);

        ((ExpPanel)vnmrIf).sendCmdToVnmr("svaction('done')");
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
    if(dirPane2 != null) dirPane2.updateDirPane();
    if(tempPane != null) tempPane.updateTemplate();
    if(resultPane != null) resultPane.reformat();
    if(resultPane2 != null) resultPane2.reformat();
    if(tempPane2 != null) tempPane2.updateTemplate();
    if(tempPane3 != null) tempPane3.updateTemplate();
    if(recfilePane != null) recfilePane.updatePanel();
    if(resultPane3 != null) resultPane3.reformat();
    if(resultPane4 != null) resultPane4.reformat();
    }

    public static SaveStudyDialog getSaveStudyDialog(SessionShare sshare, ButtonIF vif,
                                                     String title, Component comp,
                                                     String helpFile) {

        if(saveStudyDialog == null) {
            saveStudyDialog = new SaveStudyDialog(sshare, vif, title, helpFile);

            // Set this dialog on top of the component passed in.
            saveStudyDialog.setLocationRelativeTo(comp);
            saveStudyDialog.updatePanels();
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: setting setVisible(true)");
            saveStudyDialog.setVisible(true);
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: complete");
        } else {
        // update available disk space.
            saveStudyDialog.updatePanels();
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: setting setVisible(true)");
            saveStudyDialog.setVisible(true);
            if(DebugOutput.isSetFor("savedatasetup"))
                Messages.postDebug("SaveStudyDialog: complete");
            
        }

        return saveStudyDialog;
    }

    private int getPart11Mode()
    {

    String part11Root = FileUtil.openPath("SYSTEM/PART11");

    if(debug)
    System.out.println("part11Root " + part11Root);

        if(part11Root == null) return 0;

    String path = FileUtil.openPath(part11Root + "/part11Config");

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
                StringTokenizer tok = new StringTokenizer(inLine, File.pathSeparator+" \n", false);
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
          } catch(IOException e) { System.err.println("ERROR: read " + path);}

      return 0;
    }
    }

    class ResultPanel extends JPanel {

      VTextMsg result = null;
      String type = "data";

      public ResultPanel(String label, String t) {

        type = t;
// create label and panel for displaying result

        JLabel resultLabel = new JLabel(label, JLabel.LEFT);
        resultLabel.setForeground(infoColor);

// result is a MyTextMsg obj. to display vnmr results.

        result = new MyTextMsg(sshare, vnmrIf, type);
        result.setPreferredSize(new Dimension(280, 20));
        result.setForeground(infoColor);
        result.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                reformat();
            }
        });

        setLayout(new GridLayout(0, 1));
        add(resultLabel);
        add(result);

        setBorder(new CompoundBorder(BorderFactory.createEtchedBorder(),
                        BorderFactory.createEmptyBorder(0,5,5,5)));

        //setBorder(BorderFactory.createEmptyBorder(5,5,5,5));

    //reformat();
      }

      public String getResultText() {
        String strValue = result.getText();
        if (Util.iswindows())
            strValue = UtilB.unixPathToWindows(strValue);
        return strValue;
      }

    /** Formats and displays filename. */
      public void reformat() {

        String sqdir = null;
        String sqname = null;
        String svfdir = null;
        String autoname = null;
        String svfname = null;
        String temp3 = null;
        String name = null;

        if(type.equals("study") || type.equals("nextautostudy")) {

          if(dirPane != null) sqdir = dirPane.getCurrentDir();
          if(tempPane != null) sqname = tempPane.getCurrenttemplate();

          if (sqdir == null) sqdir ="";
          if (sqname == null) sqname ="";
          if(sqdir.length() > 0 && sqname.length() > 0) {
             String cmd = "svaction('"+type+"','" + sqdir + "','"+sqname+"'):$VALUE";
             result.setAttribute(SETVAL, cmd);
             result.updateValue();
          }

        } else if(m_appTypes.equals(Global.WALKUPIF) && part11Mode > 0 &&
                type.equals("studydata")) {

          if(dirPane2 != null) svfdir = dirPane2.getCurrentDir();
          if(tempPane != null) sqname = tempPane.getCurrenttemplate();
          if(tempPane3 != null) autoname = tempPane3.getCurrenttemplate();
        
          if (svfdir == null) svfdir ="";
          if (sqname == null) sqname ="";
          if (autoname == null) autoname ="";
          
          if(svfdir.length() > 0 && sqname.length() > 0 && autoname.length() > 0) {
            String cmd = "svaction('p11studydata','"+svfdir+"','"+sqname+"','data/"+autoname+"'):$VALUE";
            result.setAttribute(SETVAL, cmd);
            result.updateValue();
          }
        } else if(type.equals("studydata")) {

          if(dirPane != null) sqdir = dirPane.getCurrentDir();
          if(tempPane != null) sqname = tempPane.getCurrenttemplate();
          if(tempPane2 != null) autoname = tempPane2.getCurrenttemplate();
        
          if (sqdir == null) sqdir ="";
          if (sqname == null) sqname ="";
          if (autoname == null) autoname ="";
          
        // do this because autoname may contain studyid which is from sqname.
          if(autoname.startsWith("$studyid$/")) 
             autoname = autoname.substring(10);

          if(sqdir.length() > 0 && sqname.length() > 0 && autoname.length() > 0) {
            String cmd = "svaction('studydata','"+sqdir+"','"+sqname+"','"+autoname+"'):$VALUE";
            result.setAttribute(SETVAL, cmd);
            result.updateValue();
          }
        } else {

          if(dirPane2 != null) svfdir = dirPane2.getCurrentDir();
          if(tempPane3 != null) svfname = tempPane3.getCurrenttemplate();

          if (svfdir == null) svfdir ="";
          if (svfname == null) svfname ="";

          if(svfdir.length() > 0 && svfname.length() > 0) {
            String cmd = "svaction('data','"+svfdir+"','"+svfname+"'):$VALUE";
            result.setAttribute(SETVAL, cmd);
            result.updateValue();
          }
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
            if (Util.iswindows())
                value = UtilB.unixPathToWindows(value);
            setText(value);
/*
            if(type.equals("studydata") && resultPane3 != null) {
               resultPane3.reformat();
            }
            if(type.equals("data") && resultPane2 != null) {
               resultPane2.reformat();
            }
*/
          }
        }
      }
    }

    class TemplatePanel extends JPanel {

      private String currenttemplate = "";
      private String currenttemplatekey = "";
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

      private String tempfile = null;
      private String type = "data";

      public TemplatePanel(String label, String file, String t) {

        type = t;
        tempfile = file;

// make label and panel for filename templates

        JLabel TempLabel = new JLabel(label, JLabel.LEFT);
        TempLabel.setForeground(infoColor);

// read templates from userdir/templates/vnmrj/filename_templates
// if not found, create standard ones.

        readTemps(type);

// template labels are uneditable ComboBox

        templateList_label = new JComboBox( tempkeys );
        templateList_label.setRenderer(new DirComboBoxRenderer());
        templateList_label.setForeground(actionColor);
        //templateList_label.setSelectedItem(currenttemplatekey);
        initCurrentTemp();
        templateList_label.setPreferredSize(new Dimension(240, 20));
        templateList_label.setMaximumRowCount(6);
        templateList_label.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox)e.getSource();
            currenttemplatekey = (String)cb.getSelectedItem();
            currenttemplate = (String) temps.get(currenttemplatekey);
            showtemp();
            setRemoveTempButton();
        if(resultPane != null) resultPane.reformat();
        if(resultPane3 != null) resultPane3.reformat();
        if(resultPane2 != null) resultPane2.reformat();
        if(resultPane4 != null) resultPane4.reformat();
          }
        });

// show current template in an editable TextField

        template = new JTextField(currenttemplate);
        template.setForeground(actionColor);
        template.setPreferredSize(new Dimension(340, 20));
        template.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            currenttemplate = template.getText();
            if(!saveButton.isEnabled())
                saveButton.setEnabled(true);
            setRemoveTempButton();
            keyEntry.setEnabled(true);
            int ind = currenttemplate.lastIndexOf("_");
            int keyind = currenttemplatekey.lastIndexOf("_");
            String keystr;
            if(keyind > 0) keystr =
                currenttemplatekey.substring(0, keyind);
            else keystr = currenttemplatekey;
            String str = "";
            if(ind > 0) str =
                currenttemplate.substring(ind);
            keyEntry.setText(keystr+str);
        if(resultPane != null)  resultPane.reformat();
        if(resultPane2 != null) resultPane2.reformat();
        if(resultPane3 != null) resultPane3.reformat();
        if(resultPane4 != null) resultPane4.reformat();
          }
        });

        template.addMouseListener(new MouseAdapter() {
            public void mouseReleased(MouseEvent evt) {
                if(!saveButton.isEnabled()) {
                   saveButton.setEnabled(true);
                }
                if(!keyEntry.isEnabled()) {
                   keyEntry.setEnabled(true);
                }
            }
        });

        JLabel blabel = new JLabel("as: ", JLabel.LEFT);
        blabel.setForeground(actionColor);

        keyEntry = new JTextField();
        //keyEntry.setText("new template");
        keyEntry.setText("");
        keyEntry.setEnabled(false);

// a button to save template

        saveButton = new JButton(Util.getLabel("_Save_template"));
        saveButton.setForeground(actionColor);
        saveButton.setEnabled(false);

        saveButton.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {

            String key = keyEntry.getText();
            if(key != null && key.length() > 0) {

                currenttemplatekey = key;
                currenttemplate = template.getText();

                if(temps.containsKey(key)) temps.remove(key);

                temps.put(currenttemplatekey, currenttemplate);

                if(!tempkeys.contains(key)) {
                    tempkeys.add(currenttemplatekey);
                    sortStrVector(tempkeys);
                }
                templateList_label.setModel(new DefaultComboBoxModel(tempkeys));
                templateList_label.setSelectedItem(currenttemplatekey);

                saveButton.setEnabled(false);
                setRemoveTempButton();
                //keyEntry.setText("new template");
                keyEntry.setText("");
                keyEntry.setEnabled(false);
            } else Messages.postInfo("need a name for template.");
          }
        });

        removeTempButton = new JButton(Util.getLabel("_Remove_selected_template"));
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
                if(resultPane3 != null) resultPane3.reformat();
                if(resultPane2 != null) resultPane2.reformat();
                if(resultPane4 != null) resultPane4.reformat();
                setRemoveTempButton();

            } else Messages.postInfo(currenttemplatekey + " is not in the list.");
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
        if(resultPane3 != null) resultPane3.reformat();
        if(resultPane2 != null) resultPane2.reformat();
        if(resultPane4 != null) resultPane4.reformat();
      }

    protected void initCurrentTemp() {
        currenttemplate = "";
        currenttemplatekey = "";
        if(type.equals("study")) {
            vnmrValue.setAttribute(SETVAL, "svaction('getValue', 'tempPane','"+ tempParam1 +"'):$VALUE");
        } else if(type.equals("studydata")) {
            vnmrValue.setAttribute(SETVAL, "svaction('getValue', 'tempPane2','"+ tempParam2 +"'):$VALUE");
        } else {
            vnmrValue.setAttribute(SETVAL, "svaction('getValue', 'tempPane3','"+ tempParam3 +"'):$VALUE");
        }
        vnmrValue.updateValue();
    }

    public void setTempValue(String v) {
        currenttemplate = v;
        currenttemplatekey = "";
        showtemp();
        if(temps != null && temps.size() > 0) {
         if(currenttemplate.length() < 1) {
            currenttemplatekey = "seqfil";
            currenttemplate = (String)temps.get(currenttemplatekey);
            if(currenttemplate == null) {
              currenttemplatekey = "DATE";
              currenttemplate = (String)temps.get(currenttemplatekey);
            }
            if(currenttemplate == null) {
               Enumeration en = temps.keys();
               currenttemplatekey = (String)en.nextElement();
               currenttemplate = (String)temps.get(currenttemplatekey);
            }
            showtemp();
            if(type.equals("study")) {
                   ((ExpPanel)vnmrIf).sendCmdToVnmr(tempParam1+"='"+currenttemplate+"'");
            } else if(type.equals("studydata")) {
                   ((ExpPanel)vnmrIf).sendCmdToVnmr(tempParam2+"='"+currenttemplate+"'");
            } else {
                   ((ExpPanel)vnmrIf).sendCmdToVnmr(tempParam3+"='"+currenttemplate+"'");
            }
         } else {
          for(Enumeration en = temps.keys(); en.hasMoreElements(); ) {
            String key = (String)en.nextElement();
            String value = (String)temps.get(key);
            if(value.equals(currenttemplate)) {
                 currenttemplatekey = key;
            }
          }
         }
        }
        if(currenttemplatekey.length() > 0) { 
            templateList_label.setSelectedItem(currenttemplatekey);
        } else {
            currenttemplatekey = currenttemplate;
            temps.put(currenttemplatekey, currenttemplate);

            if(!tempkeys.contains(currenttemplatekey)) {
                    tempkeys.add(currenttemplatekey);
                    sortStrVector(tempkeys);
            }
            templateList_label.setModel(new DefaultComboBoxModel(tempkeys));
            templateList_label.setSelectedItem(currenttemplatekey);
        }
    }

      public void setRemoveTempButton() {

        if(!currenttemplate.equals((String)temps.get(currenttemplatekey)))
            removeTempButton.setEnabled(false);
        else
            removeTempButton.setEnabled(true);
      }

      public void updateTemplate() {
        readTemps(type);
        templateList_label.setModel(new DefaultComboBoxModel(tempkeys));
        initCurrentTemp();
        //templateList_label.setSelectedItem(currenttemplatekey);
        saveButton.setEnabled(false);
        setRemoveTempButton();
        //keyEntry.setText("new template");
        keyEntry.setText("");
        keyEntry.setEnabled(false);
        showtemp();
      }

      public void writeRemovedTemps() {
        String path = FileUtil.savePath(tempfile +".del");

        if (path == null)
        {
            Messages.postDebug("Permission denied for writing file " + path);
            return;
        }

        saveHashToFile(removedTemps, null, path, true);
      }

      public void readTemps(String type) {

        // current values are also set in Read_templates.
        if(type.equals("study")) {

            Read_studytemplates();
            Readuser_studytemplates();

        } else {
            Read_templates();
            Readuser_templates();
        }

          tempkeys= new Vector();
          for(Enumeration en = temps.keys(); en.hasMoreElements(); ) {
            tempkeys.add((String)en.nextElement());
          }

          sortStrVector(tempkeys);
      }

      private void Readuser_studytemplates() {
        String fileNameTemp = FileUtil.openPath(tempfile);

        if(fileNameTemp != null)
        {
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
                StringTokenizer tok = new StringTokenizer(inLine, File.pathSeparator+",\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    String temp = tok.nextToken();
                    if(!temps.containsKey(key))
                    temps.put(key, temp);
                }
            }
            in.close();
        } catch(IOException e) { System.err.println("ERROR: read " + fileNameTemp); }
      }
      }

      private void Read_studytemplates() {
        temps.clear();
        currenttemplatekey = "";
        currenttemplate = "";

    String fileNameTemp = null;
    if(m_appTypes.equals(Global.IMGIF))
        fileNameTemp = FileUtil.openPath(STEMPLATEDIR+m_userName+".img");
    else if(m_appTypes.equals(Global.WALKUPIF))
        fileNameTemp = FileUtil.openPath(STEMPLATEDIR+m_userName+".walkup");
    else
        fileNameTemp = FileUtil.openPath(STEMPLATEDIR+m_userName);

    if(fileNameTemp == null)
        fileNameTemp = FileUtil.openPath(STUDYTEMPLATEDIR);

        if(fileNameTemp == null && m_appTypes.equals(Global.IMGIF)) {
            temps.put("date", "s_%YR%%MOCH%%DAY%_%R3%");
            temps.put("DATE", "s_%DATE%_%R3%");
            temps.put("name", "s_$name$_%R3%");
            temps.put("ID", "s_$ident$_%R3%");
            temps.put("seqfil", "s_$seqfil$_%R3%");
/*
            currenttemplatekey = "date";
            currenttemplate = "s_%YR%%MOCH%%DAY%_%R3%";
*/
        } else if(fileNameTemp == null && m_appTypes.equals(Global.WALKUPIF)) {
            temps.put("DATE", "s_%DATE%_%R3%");
            temps.put("sample number", "s_$loc$_");
            temps.put("rack&zone", "s_$vrack$_$vzone$_$loc$_");
/*
            currenttemplatekey = "DATE";
            currenttemplate = "s_%DATE%_%R3%";
*/
        } else if(fileNameTemp == null) {
            temps.put("date", "s_%YR%%MOCH%%DAY%_%R3%");
            temps.put("DATE", "s_%DATE%_%R3%");
            temps.put("seqfil", "s_$seqfil$_%R3%");
            temps.put("sample", "s_$samplename$");
/*
            currenttemplatekey = "date";
            currenttemplate = "s_%YR%%MOCH%%DAY%_%R3%";
*/
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
                StringTokenizer tok = new StringTokenizer(inLine, File.pathSeparator+",\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    String temp = tok.nextToken();
                    if(!temps.containsKey(key))
                    temps.put(key, temp);
/*
                    if(currenttemplatekey == null) currenttemplatekey = key;
                    if(currenttemplate == null) currenttemplate = temp;
*/
                }
            }
            in.close();
        } catch(IOException e) { System.err.println("ERROR: read " + fileNameTemp); }
       }
      }

      private void Readuser_templates() {
        String fileNameTemp = FileUtil.openPath(tempfile);
        if(fileNameTemp != null)
        {
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
                StringTokenizer tok = new StringTokenizer(inLine, File.pathSeparator+",\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    String temp = tok.nextToken();
                    if(temps.containsKey(key)) continue;
                    if(m_appTypes.equals(Global.WALKUPIF) && part11Mode > 0
                        && temp.indexOf("$studyid$/data/") != -1) {
                        temp = temp.substring(15);
                    }
                    temps.put(key, temp);
                }
            }
            in.close();
        } catch(IOException e) { System.err.println("ERROR: read " + fileNameTemp); }
       }
      }

      private void Read_templates() {

        temps.clear();
        currenttemplatekey = "";
        currenttemplate = "";

    String fileNameTemp = null;
    if(m_appTypes.equals(Global.IMGIF))
        fileNameTemp = FileUtil.openPath(TEMPLATEDIR+m_userName+".img");
    else if(m_appTypes.equals(Global.WALKUPIF))
        fileNameTemp = FileUtil.openPath(TEMPLATEDIR+m_userName+".walkup");
    else 
        fileNameTemp = FileUtil.openPath(TEMPLATEDIR+m_userName);

     if(fileNameTemp == null)
        fileNameTemp = FileUtil.openPath(FILETEMPLATEDIR);

        if(fileNameTemp == null && m_appTypes.equals(Global.IMGIF)) {
            temps.put("seqfil", "data/$seqfil$_");
            temps.put("comment", "data/$comment$_");
            temps.put("time_run", "data/$time_run$_");
/*
            currenttemplatekey = "seqfil";
            currenttemplate = "data/$seqfil$_";
*/
        } else if(fileNameTemp == null && m_appTypes.equals(Global.WALKUPIF)
                && part11Mode > 0) {
            temps.put("pslabel", "$pslabel$_");
            temps.put("time_run", "$time_run$_");
            temps.put("solvent", "$$solvent$_");
        } else if(fileNameTemp == null && m_appTypes.equals(Global.WALKUPIF)) {
            temps.put("pslabel", "$studyid$/data/$pslabel$_");
            temps.put("time_run", "$studyid$/data/$time_run$_");
            temps.put("solvent", "$studyid$/data/$solvent$_");
/*
            currenttemplatekey = "pslabel";
            currenttemplate = "$studyid$/data/$pslabel$_";
*/
        } else if(fileNameTemp == null) {
            temps.put("seqfil", "$seqfil$_");
            temps.put("solvent", "$solvent$_");
            temps.put("date", "%YR%%MOCH%%DAY%_");
            temps.put("DATE", "%DATE%_");
/*
            currenttemplatekey = "seqfil";
            currenttemplate = "$seqfil$_";
*/
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
                StringTokenizer tok = new StringTokenizer(inLine, File.pathSeparator+",\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    String temp = tok.nextToken();
                    if(temps.containsKey(key)) continue;
                    if(m_appTypes.equals(Global.WALKUPIF) && part11Mode > 0
                        && temp.indexOf("$studyid$/data/") != -1) {
                        temp = temp.substring(15);
                    }
                    temps.put(key, temp);
/*
                    if(currenttemplatekey == null) currenttemplatekey = key;
                    if(currenttemplate == null) currenttemplate = temp;
*/
                }
            }
            in.close();
        } catch(IOException e) { System.err.println("ERROR: read " + fileNameTemp); }
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
            template.setText("Error: " + iae.getMessage());
        }
      }

      public void savetemplate()
      {

        String path = FileUtil.savePath(tempfile);

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
    if(debug) System.out.println("N_opt " + N_opt +" k " +k);
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
        for(int i=0; i<keys.length; i++)
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

        if(m_appTypes.equals(Global.IMGIF)) {
        recFiles.put("fid", "std");
        recFiles.put("procpar", "std");
        recFiles.put("log", "null");
        recFiles.put("text", "null");
        recFiles.put("fdf", "std");
        } else {
        recFiles.put("fid", "std");
        recFiles.put("procpar", "std");
        recFiles.put("log", "std");
        recFiles.put("text", "std");
        recFiles.put("fdf", "null");
        }
        recFiles.put("data", "null");
        recFiles.put("phasefile", "null");
        recFiles.put("cmd history", "null");
        recFiles.put("global", "null");
        recFiles.put("conpar", "null");
        recFiles.put("shims", "null");
        recFiles.put("pulse sequence", "null");
        recFiles.put("waveforms", "null");
        recFiles.put("user macros", "null");
    }

        private void getStandardFiles() {

      String path = FileUtil.openPath("SYSTEM/PART11/part11Config");

      if(path == null) {
        if(m_appTypes.equals(Global.IMGIF)) {
           path = FileUtil.openPath("SYSTEM/imaging/templates/vnmrj/properties/recConfig");
        //System.out.println("imaging properties path " + path);
        } else {
           path = FileUtil.openPath("SYSTEM/PROPERTIES/recConfig");
        //System.out.println("properties path " + path);
        }
      }

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
                StringTokenizer tok = new StringTokenizer(inLine, File.pathSeparator+"\n", false);
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
            } catch(IOException e) { System.err.println("ERROR: read " + path);}

            return;
        }
        }

        private void getOptionalFiles() {

      String path = FileUtil.openPath("PROPERTIES/recConfig");
      incPath = System.getProperty("userdir")+"/includingFiles";

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
                StringTokenizer tok = new StringTokenizer(inLine, File.pathSeparator+"\n", false);
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
            } catch(IOException e) { System.err.println("ERROR: read " + path);}

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

    if(debug)System.out.println("saveOptionalFiles path "+path);

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
                    String str = "file:optional:" + keys[i] +":yes\n";
                    if(debug) System.out.println("optional file " +str);
                    fw.write(str, 0, str.length());
                }
            }

        String str = "file:include:"+incEntry.getText() +":no\n";
        if(incCheck.isSelected())
            str = "file:include:"+incEntry.getText() +":yes\n";
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
                if(inLine.indexOf(File.pathSeparator) != -1 || inLine.indexOf("=") != -1)
                    return inLine;
            }
            in.close();
          } catch(IOException e) { System.err.println("ERROR: read " + path);}

        }
        return "";
    }

    public boolean canWrite(String path) {
        if(path == null) return false;
        else if(path.indexOf("$") != -1) return true;

        File file = new File(path);
        if(file == null) return false;
        else return file.canWrite();
    }

    public boolean hasChild(String path) {

        if(path == null) return false;
        else if(path.indexOf("$") != -1) return false;

        File file = new File(path);
        if(file == null) return false;

        String children[] = file.list();
        if(children != null && children.length > 0) return true;
        else return false;
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
                    str = selectedKey + File.pathSeparator + value +"\n";
                    fw.write(str, 0, str.length());
                }
            }

            for(Enumeration en = hash.keys(); en.hasMoreElements(); ) {
                key = (String) en.nextElement();
                if(key != null && !key.equals(selectedKey)) {
                    value = (String)hash.get(key);
                    if(value != null) {
                        str = key + File.pathSeparator + value + "\n";
                        if(debug)System.out.println("save " +str +" to "+path);
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
    private String currentDirKey = "";
    private String currentDir = "";
    private String type = "data";
    private String dirfile = null;
    int p11 = 0;
// p11 = 0, list only non p11 data dirs
// p11 = 1, list both p11 and non p11 dirs

    public DataDirPanel(String label, String file, String t, int p) {

        type = t;
        dirfile = file;
        p11 = p;
        getImages();

        makeDirPane(label);

    }

    public String getDir(String key) {
            if(key.equals("autodir")) {
               if(resultPane != null) {
                  String path = resultPane.getResultText();
                  return path.substring(0,path.lastIndexOf("/auto_"));
               } else return "";
            } else {
               return (String) allLongKeysPaths.get(key);
            }
    }

    private void getImages() {

        images = new ImageIcon[4];
        images[0] = Util.getImageIcon("blueDir.gif");
        images[1] = Util.getImageIcon("blueChildDir.gif");
        images[2] = Util.getImageIcon("yellowDir.gif");
        images[3] = Util.getImageIcon("yellowChildDir.gif");
    }

    private void makeDirPane(String label) {

        updateDirHashtables();

        JLabel tlabel = new JLabel(label, JLabel.LEFT);
        tlabel.setForeground(infoColor);

        JLabel blabel = new JLabel("as: ", JLabel.LEFT);
        blabel.setForeground(actionColor);

        JLabel blank = new JLabel(" ");

        dirMenu = new JComboBox(allLongKeys);

        DirComboBoxRenderer renderer = new DirComboBoxRenderer();
        dirMenu.setRenderer(renderer);
        dirMenu.setMaximumRowCount(6);
        dirMenu.setPreferredSize(new Dimension(240, 20));
/*
        if(currentDirKey != null) {
            dirMenu.setSelectedItem(currentDirKey);
        }
*/
        dirMenu.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            JComboBox cb = (JComboBox)e.getSource();
            currentDirKey = (String)cb.getSelectedItem();
            currentDir = getDir(currentDirKey);
            pathEntry.setText(currentDir);

            if(resultPane != null) resultPane.reformat();
            if(resultPane3 != null) resultPane3.reformat();
            if(resultPane2 != null) resultPane2.reformat();
            if(resultPane4 != null) resultPane4.reformat();
            setRemoveButton(currentDir);

          }
        });

        keyEntry = new JTextField();
        //keyEntry.setText("new project");
        keyEntry.setText("");
        keyEntry.setEnabled(false);

        pathEntry = new JTextField();

        pathEntry = new JTextField(currentDir);
        pathEntry.setForeground(actionColor);
        pathEntry.setPreferredSize(new Dimension(280, 20));
        pathEntry.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {
            currentDir = pathEntry.getText();

            if(!newDirButton.isEnabled())
                newDirButton.setEnabled(true);

            setRemoveButton(currentDir);

            keyEntry.setEnabled(true);
            int ind = currentDir.lastIndexOf(File.separator);
            String str = "";
            if(ind > 0) str = currentDir.substring(ind+1);
            keyEntry.setText(str);
            if(resultPane != null) resultPane.reformat();
            if(resultPane3 != null) resultPane3.reformat();
            if(resultPane2 != null) resultPane2.reformat();
            if(resultPane4 != null) resultPane4.reformat();
          }
        });

        pathEntry.addMouseListener(new MouseAdapter() {
            public void mouseReleased(MouseEvent evt) {
                if(!newDirButton.isEnabled()) {
                   newDirButton.setEnabled(true);
                }
                if(!keyEntry.isEnabled()) {
                   keyEntry.setEnabled(true);
                }
            }
        });

        newDirButton = new JButton(Util.getLabel("_Save_directory"));
        newDirButton.setForeground(actionColor);
        newDirButton.setEnabled(false);
        newDirButton.addActionListener(new ActionListener() {
          public void actionPerformed(ActionEvent e) {

            String key = keyEntry.getText();
            String path = pathEntry.getText();
            if(key != null && key.length() > 0) {

                if(part11Mode > 0 && !isChild(path)) {
                    path = getDir(currentDirKey);
                    pathEntry.setText(path);
                    Messages.postError(path + " is not a child of parent data dirs.");
                } else {

                    if(childDirs.containsKey(key)) childDirs.remove(key);

                    childDirs.put(key,path);
                    updateAllLists();

                    currentDirKey = key + getFreeSpace(path);
                    currentDir = path;

                    dirMenu.setModel(new DefaultComboBoxModel(allLongKeys));
                    dirMenu.setSelectedItem(currentDirKey);
                }
                newDirButton.setEnabled(false);
                setRemoveButton(currentDir);
                //keyEntry.setText("new project");
                keyEntry.setText("");
                keyEntry.setEnabled(false);
            } else Messages.postInfo("need a name for new directory.");
          }
        });

        removeDirButton = new JButton(Util.getLabel("_Remove_selected_directory"));
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
                ConfirmPopup popup = new ConfirmPopup("Remove Directory", type);
                popup.setLocationRelativeTo(tempPane);
                popup.setVisible(true);
            } else if(childDirs.contains(currentDir)) {
                removeDirAction();
            } else Messages.postInfo(currentDir + " is not in the list.");
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
        String path = FileUtil.savePath(dirfile+".del");

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
            currentDir = getDir(currentDirKey);

          } else {
            currentDirKey = "";
            currentDir = "";
          }
          dirMenu.setModel(new DefaultComboBoxModel(allLongKeys));
          dirMenu.setSelectedItem(currentDirKey);
          pathEntry.setText(currentDir);
          if(resultPane != null) resultPane.reformat();
          if(resultPane2!= null) resultPane2.reformat();
          if(resultPane3 != null) resultPane3.reformat();
          if(resultPane4 != null) resultPane4.reformat();
          setRemoveButton(currentDir);
        }
    }

    private boolean removeIsOk(String path) {

        if(path == null) return false;
        if(path.indexOf("$") != -1) return false;

        // allow to remove any empty child dir
        if(!isParent(path) && !fileExists(path))
            return true;

        else if(!isParent(path) && !isP11Dir(path) && canWrite(path))
            return true;
        else return false;
    }

    private void setRemoveButton(String path) {

        if(currentDirKey == null || currentDir == null) return;

        String dir = getDir(currentDirKey);
        if(!path.equals(dir))
            removeDirButton.setEnabled(false);

        else if(removeIsOk(path))
                removeDirButton.setEnabled(true);
        else
            removeDirButton.setEnabled(false);
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

    private void updateDirHashtables() {

        if(p11 > 0) getp11ParentDirs();
        else p11ParentDirs = new Hashtable();
        getdataParentDirs();
        getparentDirs_ds();
        getchildDirs(dirfile);
        updateAllLists();
        initCurrentDir();
    }

    public void updateDirPane() {

        updateDirHashtables();
        dirMenu.setModel(new DefaultComboBoxModel(allLongKeys));
        //dirMenu.setSelectedItem(currentDirKey);
        initCurrentDir();
        pathEntry.setText(currentDir);
        newDirButton.setEnabled(false);
        setRemoveButton(currentDir);
        //keyEntry.setText("new project");
        keyEntry.setText("");
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

    protected void getchildDirs(String file) {

        childDirs = readChildDirs(file);

        if(childDirs.size() > 0) {
           for(Enumeration en = dataParentDirs.keys(); en.hasMoreElements(); ) {
            String key = (String) en.nextElement();
            if(childDirs.contains(key)) childDirs.remove(key);
           }    
        } else if(type.equals("study") || m_appTypes.equals(Global.WALKUPIF)) { 
           for(Enumeration en = dataParentDirs.keys(); en.hasMoreElements(); ) {
            String key = (String) en.nextElement();
            String path = (String) dataParentDirs.get(key);
            if(path.length() > 0)
            {
                childDirs.put("Studies", path + "/studies");
                return;
            } 
           }
        }
    }

    protected void initCurrentDir() {
        currentDir = "";
        currentDirKey = "";
        if(type.equals("study")) {
            vnmrValue.setAttribute(SETVAL, "svaction('getValue', 'dirPane','"+ dirParam +"'):$VALUE");
        } else {
            vnmrValue.setAttribute(SETVAL, "svaction('getValue', 'dirPane2','"+ dirParam2 +"'):$VALUE");
        }
        vnmrValue.updateValue();
    }


    public void setDirValue(String v) {
        currentDir = v;
        currentDirKey = "";
        pathEntry.setText(currentDir);
        if(allLongKeysPaths != null && allLongKeysPaths.size() > 0) {
           if(currentDir.length() < 1) {
               currentDirKey = "Studies";
               currentDir = getDir(currentDirKey);
               if(currentDir == null) {
                  currentDirKey = "User study directory";
                  currentDir = getDir(currentDirKey);
               }
               if(currentDir == null) {
                 Enumeration en = allLongKeysPaths.keys();
                 currentDirKey = (String)en.nextElement(); 
                 currentDir = getDir(currentDirKey);
               }
               pathEntry.setText(currentDir);
               if(type.equals("study")) {
                   ((ExpPanel)vnmrIf).sendCmdToVnmr(dirParam+"='"+currentDir+"'");
               } else {
                   ((ExpPanel)vnmrIf).sendCmdToVnmr(dirParam2+"='"+currentDir+"'");
               }
           } else {
               for(Enumeration en = allLongKeysPaths.keys(); en.hasMoreElements(); ) {
                 String key = (String)en.nextElement();
                 String value = getDir(key);
                 if(value.equals(currentDir)) {
                     currentDirKey = key;
                 }
               }
           } 
        }
        if(currentDirKey.length() < 1) { 
            currentDirKey = currentDir.substring(1+currentDir.lastIndexOf("/"));
            childDirs.put(currentDirKey,currentDir);
            updateAllLists();
            dirMenu.setModel(new DefaultComboBoxModel(allLongKeys));
            dirMenu.setSelectedItem(currentDirKey);
        } else {
            dirMenu.setSelectedItem(currentDirKey);
        }
    }

    protected void initCurrent() {

        String line = readDirFirstLine(dirfile);

        if(line.equals("") && allLongKeys != null && allLongKeys.size() > 0) {
            if(type.equals("study")) {
               for(int i=0; i<allLongKeys.size(); i++) {
                 String key = (String)allLongKeys.elementAt(i);
                 if(key.indexOf("studies") != -1) {
                     currentDirKey = key;
                     currentDir = getDir(currentDirKey);
                     return;
                 }
               }
            } else {
               for(int i=0; i<allLongKeys.size(); i++) {
                 String key = (String)allLongKeys.elementAt(i);
                 if(key.indexOf("studies") == -1) {
                     currentDirKey = key;
                     currentDir = getDir(currentDirKey);
                     return;
                 }
               }
            }
        }

        StringTokenizer tok;
        if(line.indexOf(File.pathSeparator) == -1)
        tok = new StringTokenizer(line, " ,\n", false);
        else
        tok = new StringTokenizer(line, File.pathSeparator+",\n", false);
        if (tok.countTokens() > 1) {
            currentDirKey = tok.nextToken();
            currentDir = tok.nextToken();
            currentDirKey += getFreeSpace(currentDir);
        } else if(allLongKeys != null && allLongKeys.size() > 0) {
            currentDirKey = (String)allLongKeys.elementAt(0);
            currentDir = getDir(currentDirKey);
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
/*
        String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, "df -k "+path};
        if (Util.iswindows())
            cmd[2] = "df -k " + UtilB.windowsPathToUnix(path);
        WMessage msg = WUtil.runScript(cmd);
        String str = "";
        if(msg.isNoError()) {
            str = msg.getMsg();
        }
        if(str.length() > 0) {
            StringTokenizer tok = new StringTokenizer(str, " \n", false);
            String ds = "0";
            if(tok.countTokens() > 1) {
                tok.nextToken();
                ds = tok.nextToken();
                // for windows, get the fourth column
                if (Util.iswindows())
                {
                    if (tok.hasMoreTokens())
                        tok.nextToken();
                    if (tok.hasMoreTokens())
                        ds = tok.nextToken();
                }
                String space = " ("+String.valueOf((Integer.parseInt(ds))/1000)
                        + " Mb)";
                return space;
            }
        }
*/
        return "";
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

    protected Hashtable readHash(String path, int p11) {
    // p11 < 0, keep only dirs that are children of existing parent dirs.
    // p11 = 0, dirs are writable.
    // p11 = 1, all dirs.

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
                if(inLine.indexOf(File.pathSeparator) == -1)
                    tok = new StringTokenizer(inLine, " ,\n", false);
                else
                    tok = new StringTokenizer(inLine, File.pathSeparator+",\n", false);
                if (tok.countTokens() > 1) {
                    String key = tok.nextToken();
                    String dir = tok.nextToken();
                    if(!hash.containsKey(key)) {
                        if(p11 == 0 || p11 == 1) {
                           if(key.equalsIgnoreCase("private"))
                                hash.put("User data directory", dir);
                           else
                                hash.put(key, dir);
                        } else if(isChild(dir) ) {
                           if(key.equalsIgnoreCase("private")) {
                                hash.put("User data directory", dir);
                           } else
                                hash.put(key, dir);
                        } 
                    }
                }
            }
            in.close();
          } catch(IOException e) { System.err.println("ERROR: read " + path);}

        }
        return hash;
    }

    protected Hashtable readP11ParentDirs() {

        String path = FileUtil.openPath(P11DIR+m_userName);

        if(path == null) return(new Hashtable());

        if(debug) System.out.println("Part11Dirs path " +path);

        return(readHash(path, p11));
    }

    protected String readDirFirstLine(String file ) {
        String path = FileUtil.openPath(file);
        if(path == null) return("");
        return readFirstLine(path);
    }

    protected Hashtable readChildDirs(String file) {

        String path = FileUtil.openPath(file);

        if(path == null) return(new Hashtable());

        if(debug) System.out.println("childDirs path " +path);

        //return(readHash(path, -1));
        return(readHash(path, p11));
    }

    protected Hashtable readDataDirs() {

        String path = FileUtil.openPath(DATADIR+m_userName);

        if(path == null) return(new Hashtable());

        if(debug) System.out.println("dataDirs path " +path);

        return(readHash(path, p11));
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
        if(path == null) return false;
        if(path.indexOf("$") != -1) return true;
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

        String path = FileUtil.savePath(dirfile);

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

        if(currentDirKey == null) return; 
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

        for(int i=0; i<allKeys.size(); i++) {
            String key = (String)allKeys.elementAt(i);
            String path = (String)allKeysPaths.get(key);
            String type = (String)allKeysTypes.get(key);
            String space = (String)allKeysSpaces.get(key);
            if(key != null && space != null) {
                allLongKeys.add(key+space);
                if(path != null) allLongKeysPaths.put(key+space, path);
                if(type != null) allLongKeysTypes.put(key+space, type);
            }
        }
/*
        if(m_appTypes.equals(Global.WALKUPIF) && part11Mode > 0 &&
           type.equals("data")) {
           allLongKeys.add("autodir");
        }
*/
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
          } else if(value.equals("autodir") && dirPane != null) {
             String path = dirPane.getCurrentDir();
             if(dirPane.isP11Dir(path)) setIcon(images[1]);
             else setIcon(images[3]);
          }
        }

            setText(key);
        return this;
    }
    }


  class ConfirmPopup extends ModalDialog {
        JLabel msg =
        new JLabel("Directory is not empty. Remove it from the menu anyway?");
        String type;

        public ConfirmPopup(String title, String t) {
          super(title);

          type = t;
          JPanel pane = new JPanel();
          pane.add(msg);
          pane.setBorder(BorderFactory.createEmptyBorder(20,20,20,20));

          getContentPane().add(pane, BorderLayout.NORTH);

          okButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if(type.equals("study") && dirPane != null) dirPane.removeDirAction();
                if(type.equals("data") && dirPane2 != null) dirPane2.removeDirAction();
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

    class PrefPanel extends JPanel {

      protected VCheck autoSave = null;
      protected VCheck recSave = null;

      public PrefPanel() {

        setBorder(BorderFactory.createEmptyBorder(10,10,10,10));
        setLayout(new GridLayout(2, 2));

        JPanel pane = new JPanel();
        pane.setLayout(new BoxLayout(pane, BoxLayout.Y_AXIS));

        JPanel autoSavePane = makeAutoSaveCheck();
        pane.add(autoSavePane);
        JPanel recSavePane = makeRecSaveCheck();
        pane.add(recSavePane);

        add(pane);
        add(Box.createRigidArea(new Dimension(10, 10)));
        add(Box.createRigidArea(new Dimension(10, 10)));
        add(Box.createRigidArea(new Dimension(10, 10)));
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
        autoSave.setBackground(this.getBackground());

        JPanel pane = new JPanel();
        pane.setLayout(new GridLayout(1, 1));
        pane.add(autoSave);
        pane.setBorder(BorderFactory.createEmptyBorder(0,5,5,5));
        return pane;
      }

      private JPanel makeRecSaveCheck() {

        recSave = new VCheck(sshare, vnmrIf, null);
        recSave.setAttribute(LABEL, Util.getLabel("_Save_as_records"));
        recSave.setModalMode(true);
        recSave.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
        recSave.setAttribute(VARIABLE, "recordSave");
        recSave.setAttribute(SETVAL, "$VALUE=recordSave");
        recSave.setAttribute(CMD, "recordSave=2");
        recSave.setAttribute(CMD2, "recordSave=0");
        recSave.setBackground(this.getBackground());

        JPanel pane = new JPanel();
        pane.setLayout(new GridLayout(1, 1));
        pane.add(recSave);
        pane.setBorder(BorderFactory.createEmptyBorder(0,5,5,5));
        return pane;
      }

      public void updatePanel() {
        autoSave.updateValue();
        recSave.updateValue();
      }

      public void sendPreferences() {

        if(autoSave.isSelected()) {
            String cmd = autoSave.getAttribute(CMD);
            if (cmd != null)
            ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
        } else {
            String cmd = autoSave.getAttribute(CMD2);
            if (cmd != null)
            ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
        }

        if(recSave.isSelected()) {
            String cmd = recSave.getAttribute(CMD);
            if (cmd != null)
            ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
        } else {
            String cmd = recSave.getAttribute(CMD2);
            if (cmd != null)
            ((ExpPanel)vnmrIf).sendCmdToVnmr(cmd);
        }

        if(resultPane3 != null) resultPane3.reformat();
      }

      public int getAutosave() {
        if(autoSave.isSelected()) return 1;
        else return 0;
      }

      public int getRecsave() {
        if(recSave.isSelected()) return 1;
        else return 0;
      }

    }

    class MyTextMsg2 extends VTextMsg {

        public MyTextMsg2(SessionShare sshare, ButtonIF vif, String typ) {
        super(sshare, vif, typ);
        }

        public void setValue(ParamIF pf) {
          if (pf != null) {
            String str = pf.value;
            int ind = str.lastIndexOf("/");
            if(ind == str.length()-1) value = str.substring(0,ind);
            else value = str;
            StringTokenizer tok = new StringTokenizer(value, File.pathSeparator+" \n", false);
            if(tok.countTokens() > 1) {
                String name = tok.nextToken();
                String strValue = "";
                if (!Util.iswindows())
                    strValue = tok.nextToken();
                else
                {
                    strValue = tok.nextToken(File.pathSeparator+"\n");
                    strValue = UtilB.unixPathToWindows(strValue);
                }
                if(name.equals("dirPane")) dirPane.setDirValue(strValue);
                if(name.equals("dirPane2")) dirPane2.setDirValue(strValue);
                if(name.equals("tempPane")) tempPane.setTempValue(strValue);
                if(name.equals("tempPane2")) tempPane2.setTempValue(strValue);
                if(name.equals("tempPane3")) tempPane3.setTempValue(strValue);
            } else if(tok.countTokens() > 0) {
                String name = tok.nextToken();
                if(name.equals("dirPane"))  dirPane.setDirValue("");
                if(name.equals("dirPane2")) dirPane2.setDirValue("");
                if(name.equals("tempPane")) tempPane.setTempValue("");
                if(name.equals("tempPane2")) tempPane2.setTempValue("");
                if(name.equals("tempPane3")) tempPane3.setTempValue("");
            }
          }
        }
    }
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.awt.*;
import java.awt.event.*;
import java.beans.*;

import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.text.*;
import javax.swing.border.*;

import java.io.*;
import java.util.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;
import vnmr.admin.ui.*;


public class UserDirectoryDialog extends ModalDialog
implements ActionListener
{

    public static final int BUILD_ALL = 1;
    public static final int INITALIZE = 2;

    protected VAdminIF m_adminIF = null;

    protected java.util.Timer timer;
    protected String labelString;       // The label for the window
    protected int delay;                // the delay time between blinks
    protected MouseAdapter m_mlTxf;

    //i18n
    //public static final String INFOSTR = "Enter directory or select directory from the right";
    public static final String INFOSTR = Util.getAdmLabel("_admin_Enter_directory_or_select_directory_from_the_right");

    public UserDirectoryDialog()
    {
        this(BUILD_ALL);
    }

   public UserDirectoryDialog(int nBuild)
   {
      super( "View Directories" );
      initBlink();
      try
      {
         //UIManager.setLookAndFeel( "javax.swing.plaf.metal.MetalLookAndFeel" );
      }
      catch( Exception exc )
      {
     System.out.println("Error loading L&F: " + exc);
      }

      AppIF appIf = Util.getAppIF();
      if (appIf instanceof VAdminIF)
        m_adminIF = (VAdminIF)appIf;

      if (nBuild == BUILD_ALL)
      {
        JPanel left = new JPanel();
        left.setOpaque( true );
        left.setLayout( new BorderLayout() );
        left.add( new ConstraintsPanel(), BorderLayout.CENTER );

        JPanel right = new JPanel();
        right.setLayout( new BorderLayout() );
        right.setOpaque( true );
        //right.setBackground( Color.white );

        JTreeTableAdmin treeTable = new JTreeTableAdmin( new FileSystemModelAdmin());
        right.add( new JScrollPane( treeTable ), BorderLayout.CENTER );

        JSplitPane pane = new JSplitPane( JSplitPane.HORIZONTAL_SPLIT, left, right );
        pane.setContinuousLayout( true );
        pane.setOneTouchExpandable( true );
        getContentPane().add( pane, BorderLayout.CENTER );
      }

      m_mlTxf = new MouseAdapter()
      {
        public void mouseClicked(MouseEvent e)
        {
            if (timer != null)
                timer.cancel();
            if (e.getSource() instanceof JTextField)
            {
                JTextField txf2 = (JTextField)e.getSource();
                String strTxt = txf2.getText();
                if (strTxt != null && strTxt.equals(INFOSTR))
                    txf2.setText("");
            }
        }
      };
    }

    public JPanel makeDataDirPanel(String strDir, String strDefFile, boolean bPart11)
    {
        return new DisplayParentDirectory(strDir, strDefFile, bPart11);
    }

    public JPanel makeUserDirPanel()
    {
        return new DisplayUserDirectory();
    }

    public JPanel makeUserTemplate(String strDir, String strDefFile)
    {
        return new DisplayTemplate(strDir, strDefFile);
    }

    public JPanel makeFileTable()
    {
        JPanel right = new JPanel();
        right.setLayout( new BorderLayout() );
        right.setOpaque( true );
        //right.setBackground( Color.white );

        JTreeTableAdmin treeTable = new JTreeTableAdmin( new FileSystemModelAdmin());
        right.add( new JScrollPane( treeTable ), BorderLayout.CENTER );

        return right;
    }

    protected void initBlink()
    {
        String blinkFrequency = null;
        delay = (blinkFrequency == null) ? 400 :
            (1000 / Integer.parseInt(blinkFrequency));
        labelString = null;
        if (labelString == null)
            labelString = "Blink";
    }

    protected boolean setDir(JTextField txf, String strValue)
    {
        String strTxt = txf.getText();
        String strName = txf.getName();
        boolean bSet = false;

        if ((strTxt == null || strTxt.trim().length() == 0
                || strTxt.equals(INFOSTR))
                && strName.equalsIgnoreCase("value"))
        {
            if (timer != null)
            {
                timer.cancel();
                txf.setForeground(Color.black);
            }

            bSet = true;
            txf.setText(strValue);
            txf.grabFocus();
        }
        return bSet;
    }

   private void registerActionListeners()
   {
      okButton.addActionListener( this );
      okButton.setActionCommand( "ok" );
      cancelButton.addActionListener(this);
      cancelButton.setActionCommand("cancel");
      helpButton.addActionListener(this);
      helpButton.setActionCommand("help");
   }

   public void actionPerformed( ActionEvent e )
   {
      String cmd = e.getActionCommand();

      if( cmd.equals( "ok" ))
         System.out.println( "do ok" );
      else if( cmd.equals( "cancel" ))
         this.setVisible( false );
      else  if( cmd.equals( "help" ))
          displayHelp();
   }

   class ConstraintsPanel extends JPanel
   {
      public ConstraintsPanel()
      {
         setLayout( new BoxLayout( this, BoxLayout.Y_AXIS ));
         add( new DisplayUserDirectory());
         add( Box.createVerticalStrut( 15 ));
         add( new DisplayParentDirectory("", "", false));
         add( Box.createVerticalStrut( 15 ));
         add( new DisplayTemplate("", ""));
         add( Box.createVerticalStrut( 15 ));
         add( new DisplayResults());
         add( Box.createVerticalStrut( 15 ));
      }
   }

    class DisplayParentDirectory extends DisplayPanel implements FocusListener,
                                                        PropertyChangeListener
    {
        /* Define instruction labels */
        JLabel label = null;
        //i18n
        //JLabel infoLabel1 = new JLabel( "Data may be saved in the following parent directories. The Label field is " );
        //JLabel infoLabel2 = new JLabel( "presented as the choice to the user in the \"Data save\" pop-up. It could " );
        //JLabel infoLabel3 = new JLabel( "be the same as the directory name or some descriptive text. " );
        //JLabel infoLabel4 = new JLabel( "refers to the users home direictory (e.g., /export/home/wanda.) $vnmrsystem refers to the" );
        //JLabel infoLabel5 = new JLabel( "parent directory of VnmrJ software. $vnmruser refers to the users vnmrj directory." );

        JLabel infoLabel1 = new JLabel(Util.getAdmLabel("_admin_infoLabel1"));
        JLabel infoLabel2 = new JLabel( Util.getAdmLabel("_admin_infoLabel2") );
        JLabel infoLabel3 = new JLabel( Util.getAdmLabel("_admin_infoLabel3") );
        JLabel infoLabel4 = new JLabel( Util.getAdmLabel("_admin_infoLabel4") );
        JLabel infoLabel5 = new JLabel( Util.getAdmLabel("_admin_infoLabel5") );
        
        protected JPanel  m_pnlDisplay;

        protected int m_nRow = 0;
        protected TextFieldValues m_objTxfValue = new TextFieldValues();

        // Directory where all the user files are.
        protected String m_strPathDir = "";

        // File where the default values for this panel are.
        protected String m_strDefDirFile = "";
        protected boolean m_bPart11Pnl = false;
        protected boolean m_bDefaultFile = true;

        protected GridBagLayout m_gbl = new GridBagLayout();
        protected GridBagConstraints m_gbc = new GridBagConstraints();


        public DisplayParentDirectory(String strDir, String strDefFile, boolean bPart11)
        {
            super();
            setLayout(m_gbl );

            m_gbc.anchor = GridBagConstraints.NORTHWEST;
            m_gbc.fill = GridBagConstraints.HORIZONTAL;

            m_strPathDir = strDir;
            m_strDefDirFile = strDefFile;
            m_bPart11Pnl = bPart11;
            setLayout(new BorderLayout());
            m_pnlDisplay = new JPanel(m_gbl);
            JScrollPane spDisplay = new JScrollPane(m_pnlDisplay);
            add(m_pnlDisplay, BorderLayout.CENTER);

            JComponent pnlTool = getToolBar();
            add(pnlTool, BorderLayout.NORTH);

            layoutUIComponents(FileUtil.openPath(m_strDefDirFile), true);
            VItemArea1.addChangeListener(this);
            VUserToolBar.addChangeListener(this);
        }

        protected void layoutUIComponents(String strPath, boolean bDefaultFile)
        {
            //gbc.weightx = 0.5;

            showInstructions( m_gbl, m_gbc, 0, 0, 7, infoLabel1 );
            showInstructions( m_gbl, m_gbc, 0, 1, 7, infoLabel2 );
            showInstructions( m_gbl, m_gbc, 0, 2, 7, infoLabel3 );
            /*showInstructions( gbl, gbc, 0, 3, 7, infoLabel4 );
            showInstructions( gbl, gbc, 0, 4, 7, infoLabel5 );*/
            showInstructions( m_gbl, m_gbc, 0, 5, 1, new JLabel( "" ));
            
            //i18n
            //label = new JLabel( "LABEL" );
            label = new JLabel( Util.getAdmLabel("_adm_LABEL") );
            label.setForeground( Color.black );
            m_gbc.gridx = 1; m_gbc.gridy = 5;
            //gbc.ipadx = 10;
            m_gbl.setConstraints( label, m_gbc );
            m_pnlDisplay.add( label );

            label = new JLabel( "     " );
            m_gbc.gridx = 2; m_gbc.gridy = 5;
            m_gbc.ipadx = 0;
            m_gbc.gridwidth = 5;
            m_gbc.weightx = 0;
            m_gbl.setConstraints( label, m_gbc );
            //add( label );
            
            //i18n
            //showInstructions( m_gbl, m_gbc, 2, 5, 1, new JLabel( "DIRECTORY" ));
            showInstructions( m_gbl, m_gbc, 2, 5, 1, new JLabel( Util.getAdmLabel("_adm_DIRECTORY") ));

            m_nRow = 5;

            m_bDefaultFile = bDefaultFile;
            m_objTxfValue.clearArrays();
            displayNewTxf(strPath);

            m_pnlDisplay.setBorder( new CompoundBorder(
            				//i18n            		
                            //BorderFactory.createTitledBorder( "  Parent Directories  "),
                            BorderFactory.createTitledBorder( Util.getAdmLabel("_adm_Parent_Directories")),
                            BorderFactory.createEmptyBorder( 10, 10, 10, 10 )));
        }

        public String getDefaultFile(String type)
        {
            return m_strDefDirFile;
        }

        public void setDefaultFile(String strFile)
        {
            m_strDefDirFile = strFile;
        }

        public String getPathDir()
        {
            return m_strPathDir;
        }

        public void setPathDir(String strDir)
        {
            m_strPathDir = strDir;
        }

        public boolean isPart11Pnl()
        {
            return m_bPart11Pnl;
        }

        public boolean isDefaultFile()
        {
            return m_bDefaultFile;
        }

        protected TextFieldValues getTxfValues()
        {
            return m_objTxfValue;
        }

        protected void displayNewTxf(String strLabel, String strValue)
        {
            strLabel = (strLabel != null) ? strLabel.trim() : "";
            strValue = (strValue != null) ? strValue.trim() : "";

            JCheckBox  chk1   = new JCheckBox(Util.getImageIcon("boxGray.gif"));
            final DataField txf1   = new DataField(strLabel);
            final DataField txf2   = new DataField(strValue);
            m_nRow = m_nRow + 1;

            txf1.setName("label");
            txf2.setName("value");

            // new field
            if (strLabel.equals("") && strValue.equals(""))
            {
                txf2.setText(INFOSTR);
                txf2.addMouseListener(m_mlTxf);
                if (timer != null)
                    timer.cancel();

                timer = new java.util.Timer();
                timer.schedule(new TimerTask() {
                    public void run() {WUtil.blink(txf2, WUtil.FOREGROUND);}
                }, delay, delay);
            }

            /* 1st line of text field*/
            m_gbc.weightx = 0;
            showComp( m_gbl, m_gbc, 0, m_nRow, 1, chk1 );
            m_gbc.weightx = 1;
            showComp( m_gbl, m_gbc, GridBagConstraints.RELATIVE, m_nRow, 1, txf1 );
            //showSpaces( gbl, gbc, 2, 6 );
            showComp( m_gbl, m_gbc, GridBagConstraints.RELATIVE, m_nRow, 1, txf2 );
            m_gbc.weightx = 0;
            txf1.addFocusListener(this);
            txf2.addFocusListener(this);

            // Add the textfields to the respective arrays, so that they
            // can be retreived later for writing to the file.
            m_objTxfValue.addToLabel(txf1);
            m_objTxfValue.addToValue(txf2);
        }

        public void focusGained( FocusEvent e )
        {
            //System.out.println("focus gained");
        }

        public void focusLost( FocusEvent e )
        {
            Object objFocus = e.getSource();
            boolean bSaveCmd = false;
            if (objFocus instanceof DataField)
            {
                DataField compFocus = (DataField)objFocus;
                Component comp = e.getOppositeComponent();
                if (comp instanceof JButton)
                {
                    String strTxt = ((JButton)comp).getText();
                    if (strTxt != null && strTxt.equals(WGlobal.SAVEUSER))
                        bSaveCmd = true;
                }

                // if the focus is not in the panel then write the file.
                /*if ((comp == null || !m_pnlDisplay.equals(comp.getParent()))
                        && !bSaveCmd)
                {
                    String strPrevValue = compFocus.getValue();
                    String strTxt = compFocus.getText();
                    if (!strPrevValue.equals(strTxt))
                    {
                        // save the new data by writing to file
                        AppIF appIf = Util.getAppIF();
                        if (appIf instanceof VAdminIF)
                            ((VAdminIF)appIf).getUserToolBar().doSave();
                        compFocus.setValue(strTxt);
                    }
                }*/
            }
        }

        private void showInstructions( GridBagLayout gbl, GridBagConstraints gbc,
                                        int gridx, int gridy, int gridwidth,
                                        JLabel instruction)
        {
            gbc.gridwidth = gridwidth;
            gbc.gridx = gridx;  gbc.gridy = gridy;
            gbl.setConstraints( instruction, gbc );
            instruction.setForeground( Color.black );
            m_pnlDisplay.add( instruction );
        }

        private void showComp( GridBagLayout gbl, GridBagConstraints gbc,
                                    int gridx, int gridy, int gridwidth,
                                    JComponent comp)
        {
            gbc.gridx = gridx;    gbc.gridy = gridy;
            gbl.setConstraints( comp, gbc );
            m_pnlDisplay.add( comp );
        }

        private void showSpaces( GridBagLayout gbl, GridBagConstraints gbc,
                                    int gridx, int gridy)
        {
            JLabel spaces = new JLabel( "     " );
            gbc.gridx = gridx;   gbc.gridy = gridy;
            gbl.setConstraints( spaces, gbc );
            m_pnlDisplay.add( spaces );
        }
    }

    class DisplayUserDirectory extends JPanel
    {
        JLabel hmlabel = new JLabel( "User home directories are made in the directory: " );
        JLabel vjlabel = new JLabel( "VnmrJ related files are stored in the directory: " );
        JLabel vjlabel2 = new JLabel( "(This directory may be referred to with $vnmruser)" );

        private JTextField hmdir = new JTextField( 20 );
        private JTextField vjdir = new JTextField( 20 );


        public DisplayUserDirectory()
        {
            GridBagLayout gbl = new GridBagLayout();
            GridBagConstraints gbc = new GridBagConstraints();
            setLayout( gbl );

            gbc.anchor = GridBagConstraints.NORTHWEST;
            gbc.fill = GridBagConstraints.HORIZONTAL;

            hmlabel.setForeground( Color.black );
            add( hmlabel, gbc );
            add( Box.createHorizontalStrut( 10 ), gbc );
            gbc.gridwidth = GridBagConstraints.REMAINDER;
            add( hmdir, gbc );
            add( Box.createVerticalStrut( 15 ), gbc );

            gbc.gridwidth = 1;
            vjlabel.setForeground( Color.black );
            add( vjlabel, gbc );
            add( Box.createHorizontalStrut( 10 ), gbc );
            gbc.gridwidth = GridBagConstraints.REMAINDER;
            add( vjdir, gbc );
            add( Box.createVerticalStrut( 0 ), gbc );

            gbc.gridwidth = 1;
            vjlabel2.setForeground( Color.black );
            add( vjlabel2, gbc );
            setBorder( new CompoundBorder(
                            //i18n
                            //BorderFactory.createTitledBorder(" User_Directories "),
                            BorderFactory.createTitledBorder( Util.getAdmLabel("_admin_User_Directories")),
                            BorderFactory.createEmptyBorder( 10, 10, 10, 10 )));
        }
    }

    class DisplayTemplate extends DisplayPanel implements FocusListener,
                                                            PropertyChangeListener
    {

        protected JPanel  m_pnlDisplay;

        protected int m_nRow = 0;
        protected TextFieldValues m_objTxfValue = new TextFieldValues();

        // Directory where user files are.
        protected String m_strPathDir = "";

        // File which has the deafult values for this panel.
        protected String m_strDefDirFile = "";

        protected boolean m_bPart11Pnl = false;
        protected boolean m_bDefaultFile = true;

        protected GridBagLayout m_gbl = new GridBagLayout();
        protected GridBagConstraints m_gbc = new GridBagConstraints();


        public DisplayTemplate(String strDir, String strDefDirFile)
        {
            setLayout(m_gbl );

            m_gbc.anchor = GridBagConstraints.NORTHWEST;
            m_gbc.fill = GridBagConstraints.HORIZONTAL;

            m_strPathDir = strDir;
            m_strDefDirFile = strDefDirFile;

            setLayout(new BorderLayout());
            m_pnlDisplay = new JPanel(m_gbl);
            JScrollPane spDisplay = new JScrollPane(m_pnlDisplay);
            add(spDisplay, BorderLayout.CENTER);

            JComponent pnlTool = getToolBar();
            add(pnlTool, BorderLayout.NORTH);

            layoutUIComponents(FileUtil.openPath(m_strDefDirFile), true);
            VItemArea1.addChangeListener(this);
            VUserToolBar.addChangeListener(this);
        }

        protected void layoutUIComponents(String strPath, boolean bDefaultFile)
        {
            JLabel label	  = null;

            //i18n
            //label = new JLabel( "File names can be constructed from a template. The LABEL field is " );
            label = new JLabel( Util.getAdmLabel("_admin_File_names_can_be_constructed_from_a_template._The_LABEL_field_is_") );
            label.setForeground( Color.black );
            //m_gbc.weightx = 0.5;
            showInstruction( m_gbl, m_gbc, 0, 0, 7, label );
            
            //i18n
            //label = new JLabel( "presented as the choice to the user in the \"Data save\" pop-up." );
            label = new JLabel( Util.getAdmLabel("_admin_presented_as_the_choice_to_the_user_in_the_Data_save_pop-up.") );
            showInstruction( m_gbl, m_gbc, 0, 1, 7, label );
            showInstruction( m_gbl, m_gbc, 0, 2, 1, new JLabel( "" ));

            label = new JLabel( Util.getAdmLabel("_adm_LABEL") );
            label.setForeground( Color.black );
            m_gbc.gridx = 1; m_gbc.gridy = 2;
            m_gbc.ipadx = 10;
            m_gbl.setConstraints( label, m_gbc );
            m_pnlDisplay.add( label );

            label = new JLabel( "     " );
            m_gbc.gridx = 2; m_gbc.gridy = 2;
            m_gbc.ipadx = 0;   // reset to default
            m_gbc.gridwidth = 5;
            m_gbl.setConstraints( label, m_gbc );
            //add( label );

            showInstruction( m_gbl, m_gbc, 2, 2, 1, new JLabel( Util.getAdmLabel("_admin_TEMPLATE") ));

            m_nRow = 3;
            m_bDefaultFile = bDefaultFile;
            m_objTxfValue.clearArrays();
            displayNewTxf(strPath);

            m_pnlDisplay.setBorder( new CompoundBorder(
                                BorderFactory.createTitledBorder( Util.getAdmLabel("_admin_User_Directories")),
                                BorderFactory.createEmptyBorder( 10, 10, 10, 10 )));
        }

        public String getDefaultFile(String type)
        {
            if (type == null)
                type = "";
            type = type.trim();

            String strfile = m_strDefDirFile;

            if (type.equals(Global.IMGIF))
                strfile = WDirTabPane.IMGTEMPLATE;
            else if (type.equals(Global.WALKUPIF))
                strfile = WDirTabPane.WALKUPTEMPLATE;

            return strfile;
        }

        public void setDefaultFile(String strFile)
        {
            m_strDefDirFile = strFile;
        }

        public String getPathDir()
        {
            return m_strPathDir;
        }

        public void setPathDir(String strDir)
        {
            m_strPathDir = strDir;
        }

        public boolean isPart11Pnl()
        {
            return m_bPart11Pnl;
        }

        public boolean isDefaultFile()
        {
            return m_bDefaultFile;
        }

        protected TextFieldValues getTxfValues()
        {
            return m_objTxfValue;
        }

        public void focusGained( FocusEvent e )
        {
            //System.out.println("focus gained");
        }

        public void focusLost( FocusEvent e )
        {
            Object objFocus = e.getSource();
            boolean bSaveCmd = false;
            if (objFocus instanceof DataField)
            {
                DataField compFocus = (DataField)objFocus;
                Component comp = e.getOppositeComponent();
                if (comp instanceof JButton)
                {
                    String strTxt = ((JButton)comp).getText();
                    if (strTxt != null && strTxt.equals(WGlobal.SAVEUSER))
                        bSaveCmd = true;
                }

                // if the focus is not in the panel then write the file.
                /*if ((comp == null || !m_pnlDisplay.equals(comp.getParent()))
                        && !bSaveCmd)
                {
                    String strPrevValue = compFocus.getValue();
                    String strTxt = compFocus.getText();
                    if (!strPrevValue.equals(strTxt))
                    {
                        // save the data by writing it to the file
                        AppIF appIf = Util.getAppIF();
                        if (appIf instanceof VAdminIF)
                            ((VAdminIF)appIf).getUserToolBar().doSave();
                        compFocus.setValue(strTxt);
                    }
                }*/
            }
        }

        protected void displayNewTxf(String strLabel, String strValue)
        {
            strLabel = (strLabel == null) ? "" : strLabel.trim();
            strValue = (strValue == null) ? "" : strValue.trim();

            JCheckBox chkBox1 = new JCheckBox(Util.getImageIcon("boxGray.gif"));
            DataField txf1   = new DataField(strLabel);
            DataField txf2   = new DataField(strValue);
            JPanel     pnlTxf = new JPanel(m_gbl);
            m_nRow = m_nRow + 1;

            txf1.setName("label");
            txf2.setName("value");

            /* 1st line of text field*/
            m_gbc.weightx = 0;
            showComp( m_gbl, m_gbc, 0, m_nRow, 1, chkBox1 );
            m_gbc.weightx = 1;
            showComp( m_gbl, m_gbc, GridBagConstraints.RELATIVE, m_nRow, 1, txf1 );
            //showSpaces( gbl, gbc, 2, 6 );
            showComp( m_gbl, m_gbc, GridBagConstraints.RELATIVE, m_nRow, 1, txf2 );
            m_gbc.weightx = 0;
            txf1.addFocusListener(this);
            txf2.addFocusListener(this);

            m_objTxfValue.addToLabel(txf1);
            m_objTxfValue.addToValue(txf2);
        }

        private void showInstruction( GridBagLayout gbl, GridBagConstraints gbc,
                                        int gridx, int gridy, int gridwidth,
                                        JLabel instruction)
        {
            gbc.gridwidth = gridwidth;
            gbc.gridx = gridx;  gbc.gridy = gridy;
            gbl.setConstraints( instruction, gbc );
            instruction.setForeground( Color.black );
            m_pnlDisplay.add( instruction );
        }

        private void showComp( GridBagLayout gbl, GridBagConstraints gbc,
                                    int gridx, int gridy, int gridwidth,
                                    JComponent comp)
        {
            gbc.gridx = gridx;    gbc.gridy = gridy;
            gbl.setConstraints( comp, gbc );
            m_pnlDisplay.add( comp );
        }

        private void showSpaces( GridBagLayout gbl, GridBagConstraints gbc,
                                    int gridx, int gridy)
        {
            JLabel spaces = new JLabel( "     " );
            gbc.gridx = gridx;   gbc.gridy = gridy;
            gbl.setConstraints( spaces, gbc );
            m_pnlDisplay.add( spaces );
        }
    }

    class DisplayResults extends JPanel
    {
        JLabel text = new JLabel( "Sample file name using the information from the current experiment and the default selection is:" );
        JLabel emptyLabel = new JLabel( "" );

        public DisplayResults()
        {
            GridBagLayout gbl = new GridBagLayout();
            GridBagConstraints gbc = new GridBagConstraints();
            setLayout( gbl );

            gbc.anchor = GridBagConstraints.NORTHWEST;
            gbc.fill = GridBagConstraints.HORIZONTAL;

            text.setForeground( Color.black );
            add( text, gbc );
            gbc.gridy = 0;
            gbc.gridwidth = GridBagConstraints.REMAINDER;
            add( emptyLabel, gbc );
            add( Box.createVerticalStrut( 0 ), gbc );
            add( emptyLabel, gbc );

            /*	 gbc.gridy = 1;
            VTextMsg result = new VTextMsg(sshare, vnmrif, null);
            result.setPreferredSize(new Dimension(300, 30));
            result.setForeground(Color.black);
            add( result, gbc );
            */
            setBorder( new CompoundBorder(
                        BorderFactory.createTitledBorder( "  Results  "),
                        BorderFactory.createEmptyBorder( 10, 10, 10, 10 )));
        }
    }

    class DataField extends JTextField
    {
        protected String m_strValue = "";

        public DataField(String strTxt)
        {
            super(strTxt);
            m_strValue = strTxt;
        }

        public String getValue()
        {
            return m_strValue;
        }

        public void setValue(String value)
        {
            m_strValue = value;
        }
    }

    /**
     *  This class contains an array of textfield corresponding to labels,
     *  and another array of textfields corresponding to values. All the
     *  texfields are added to their respective arrays, so that the labels
     *  and the values entered in the textfields could be retrieved later
     *  for writing to the file.
     */
    class TextFieldValues
    {
        protected ArrayList m_aListTxfLabel = new ArrayList();
        protected ArrayList m_aListTxfValue = new ArrayList();

        public void addToLabel(DataField txf)
        {
            m_aListTxfLabel.add(txf);
        }

        public void addToValue(DataField txf)
        {
            m_aListTxfValue.add(txf);
        }

        public ArrayList getTxfLabel()
        {
            return m_aListTxfLabel;
        }

        public ArrayList getTxfValue()
        {
            return m_aListTxfValue;
        }

        public void clearArrays()
        {
            m_aListTxfLabel.clear();
            m_aListTxfValue.clear();
        }

        public void clearLabelArray()
        {
            m_aListTxfLabel.clear();
        }

        public void clearValueArray()
        {
            m_aListTxfValue.clear();
        }

    }

    /**
     *  This class has some common functions for the user directory panel.
     */
    abstract class DisplayPanel extends JPanel implements PropertyChangeListener
    {

        /**
         *  Displays new set of text field (including label and value) in the panel.
         *  @param strLabel label to be displayed in the label textfield.
         *  @param strValue value to be displayed in the value textfield.
         */
        protected abstract void displayNewTxf(String strLabel, String strValue);

        protected abstract void layoutUIComponents(String strPath, boolean bDefaultFile);

        public abstract String getDefaultFile(String type);

        public abstract String getPathDir();

        public abstract boolean isPart11Pnl();

        public abstract boolean isDefaultFile();

        protected abstract TextFieldValues getTxfValues();

        protected LoginService m_loginservice;

        /** The file cache object.  */
        protected WFileCache m_objCache;


        public DisplayPanel()
        {
            m_objCache = new WFileCache(100);
        }

        /**
         *  Returns the tool bar for the panel.
         */
        protected JComponent getToolBar()
        {
            JToolBar tbarDir = new JToolBar();
            JButton btnNew = new JButton();
            //m_btnSave = new JButton("Save File");
            //i118n
            //btnNew.setText("New Label");
            btnNew.setText(Util.getAdmLabel("_adm_New_Label"));
            btnNew.setActionCommand("new");
            //m_btnSave.setActionCommand("save");

            ActionListener alTool = new ActionListener()
            {
                public void actionPerformed(ActionEvent e)
                {
                    doAction(e);
                }
            };

            btnNew.addActionListener(alTool);
            // m_btnSave.addActionListener(alTool);

            tbarDir.setFloatable(false);
            tbarDir.add(btnNew);
            /*tbarDir.add(new JLabel("        "));
            tbarDir.add(m_btnSave);*/

            return tbarDir;
        }

        public void propertyChange(PropertyChangeEvent e)
        {
            String strPropName = e.getPropertyName();
            if (strPropName.equals(WGlobal.USER_CHANGE))
            {
                String strName = (String)e.getNewValue();
                String strAccount = null;
                VDetailArea objDetailArea = m_adminIF.getDetailArea1();
                if (objDetailArea != null)
                    strAccount = objDetailArea.getItemValue("itype");
                if (strAccount == null)
                    strAccount = "";
                strAccount = strAccount.trim();

                String strDir = getPathDir()+strName;
                if (this instanceof DisplayTemplate)
                {
                    if (strAccount.equals(Global.IMGIF))
                        strDir = strDir+".img";
                    else if (strAccount.equals(Global.WALKUPIF))
                        strDir = strDir+".walkup";
                }
                String strPath = FileUtil.openPath(strDir);
                boolean bDefaultFile = false;
                if (strPath == null)
                {
                    strPath = FileUtil.openPath(getDefaultFile(strAccount));
                    bDefaultFile = true;
                }
                clear(this);
                layoutUIComponents(strPath, bDefaultFile);
            }
            else if (strPropName.indexOf(WGlobal.IMGDIR) >= 0)
            {
                String strName = (String)e.getNewValue();
                StringTokenizer strTokenizer = new StringTokenizer(strPropName);
                String strInterface = null;
                boolean bDefaultFile = false;
                if (strTokenizer.hasMoreTokens())
                    strTokenizer.nextToken();
                if (strTokenizer.hasMoreTokens())
                    strInterface = strTokenizer.nextToken();
                if (strInterface == null)
                    strInterface = "";
                strInterface = strInterface.trim();

                String strDir = getPathDir()+strName;
                if (this instanceof DisplayTemplate)
                {
                    if (strInterface.equals(WGlobal.IMGDIR))
                        strDir = strDir+".img";
                    else if (strInterface.equals(WGlobal.WALKUPDIR))
                        strDir = strDir+".walkup";
                }
                String strPath = FileUtil.openPath(strDir);
                if (strPath == null)
                {
                    String strAccount = Global.WALKUPIF;
                    if (strInterface.equals(WGlobal.IMGDIR))
                        strAccount = Global.IMGIF;
                    else if (strInterface.equals(WGlobal.WALKUPDIR))
                        strAccount = Global.WALKUPIF;
                    strPath = FileUtil.openPath(getDefaultFile(strAccount));
                    bDefaultFile = true;
                }
                clear(this);
                layoutUIComponents(strPath, bDefaultFile);
            }
            else if (strPropName.indexOf(WGlobal.SAVEUSER_NOERROR) >= 0)
            {
                Object objValue = e.getNewValue();
                String strName = "";
                if (objValue instanceof String)
                    strName = (String)objValue;
                else if (objValue instanceof WItem)
                    strName = ((WItem)objValue).getText();

                VDetailArea objDetail = m_adminIF.getDetailArea1();
                String strAccount = null;
                if (objDetail != null)
                    strAccount = objDetail.getItemValue("itype");
                if (strAccount == null)
                     strAccount = "";
                strAccount = strAccount.trim();

                String strDir = getPathDir()+strName;
                if (this instanceof DisplayTemplate)
                {
                    if (strAccount.equals(Global.IMGIF))
                        strDir = strDir+".img";
                    else if (strAccount.equals(Global.WALKUPIF))
                        strDir = strDir+".walkup";
                }
                TextFieldValues objTxf = getTxfValues();
                writeFile(strName, strDir, objTxf.getTxfLabel(), objTxf.getTxfValue());
            }
            else if (strPropName.indexOf(WGlobal.DELETE_USER) >= 0)
            {
                clear(this);
                String strPath = FileUtil.openPath(getDefaultFile(""));
                layoutUIComponents(strPath, true);
            }
        }

        public LoginService getloginservice()
        {
            if (m_loginservice == null)
            {
                m_loginservice = LoginService.getDefault();
            }
            return m_loginservice;
        }

        public void doAction(ActionEvent e)
        {
            String cmd = e.getActionCommand();
            // the new button creates a new set of texfields
            if (cmd.equals("new"))
            {
                Container container = getParent();
                if (container != null)
                    container.setVisible(false);
                displayNewTxf("", "");
                if (container != null)
                    container.setVisible(true);
            }
        }

        /**
         *  Displays new set of textfields.
         */
        protected void displayNewTxf(String strPath)
        {
            HashMap hmPnl = null;
            // Get the hashmap for this file from cache,
            // the cache always returns the latest hashmap of the file.
            if (strPath != null)
                hmPnl = (HashMap)m_objCache.getData(strPath);

            // display the fields in the panel
            displayPnlFields(hmPnl);
        }

        /**
         *  Reads the file, and sets the label and the values in the hashmap.
         */
        protected void setHM(String strPath, HashMap hmPnl)
        {
            BufferedReader reader = WFileUtil.openReadFile(strPath);
            boolean bFileEmpty = true;
            boolean bPart11Pnl = isPart11Pnl();
            boolean bDefaultFile = isDefaultFile();

            // if the file is empty, then create an empty set of textfields.
            if (reader == null)
            {
                //displayNewTxf("","");
                return;
            }

            String strLine = null;
            StringTokenizer strTok;

            try
            {
                // read the file, and create a new set of textfields,
                // with the values from the file.
                while((strLine = reader.readLine()) != null)
                {
                    String strLabel = "";
                    String strValue = "";
                    bFileEmpty = false;

                    if (strLine.startsWith("#") || strLine.startsWith("@") ||
                        strLine.startsWith("%") || strLine.startsWith("\"@"))
                        continue;

                    if (strLine.startsWith("\""))
                        strTok = new StringTokenizer(strLine, "\"");
                    else
                        strTok = new StringTokenizer(strLine, File.pathSeparator+",\n");

                    if(!strTok.hasMoreTokens())
                        continue;

                    strLabel = strTok.nextToken();

                    // the format for the part11 Default file is a little different
                    // only parse out the part11Dir, and don't display other defaults
                    if (bPart11Pnl && bDefaultFile)
                    {
                        if (!strLabel.equalsIgnoreCase("part11Dir"))
                            continue;
                    }

                    if (strTok.hasMoreTokens())
                    {
                        while(strTok.hasMoreTokens())
                        {
                            strValue += strTok.nextToken();
                            if(strTok.hasMoreTokens())
                                strValue += " ";
                        }
                    }

                    // append "/data/$name" to part11Dir
                    if (bPart11Pnl && bDefaultFile && strLabel.equalsIgnoreCase("part11Dir"))
                    {
                        StringBuffer sbDir = new StringBuffer(strValue);
                        if (sbDir.lastIndexOf(File.separator) < sbDir.length()-1)
                            sbDir.append(File.separator);
                        sbDir.append("data"+File.separator+"$");
                        sbDir.append(WGlobal.NAME);
                        strValue = sbDir.toString();
                    }

                    //displayNewTxf(strLabel, strValue);
                    if (strLabel != null && strLabel.trim().length() > 0)
                        hmPnl.put(strLabel, strValue);
                }
                reader.close();
            }
            catch(Exception e)
            {
                Messages.writeStackTrace(e);
                //e.printStackTrace();
                Messages.postDebug(e.toString());
            }
        }

        
        /**
         *  Displays the labels and the values for the panel.
         */
        protected void displayPnlFields(HashMap hmPnl)
        {
            if (hmPnl == null || hmPnl.isEmpty())
                return;

            Iterator keySetItr = hmPnl.keySet().iterator();
            String strLabel = "";
            String strValue = "";

            // if the file is empty, then create an empty set of textfields.
            if (hmPnl == null || hmPnl.isEmpty())
            {
                displayNewTxf("","");
                return;
            }

            Container container = getParent();
            if (container != null)
                container.setVisible(false);
            try
            {
                // Get each set of label and value, and display them.
                while(keySetItr.hasNext())
                {
                    strLabel = (String)keySetItr.next();
                    strValue = (String)hmPnl.get(strLabel);

                    displayNewTxf(strLabel, strValue);
                }

                if (container != null)
                    container.setVisible(true);
                revalidate();
                repaint();
            }
            catch (Exception e)
            {
                Messages.writeStackTrace(e);
                //e.printStackTrace();
                Messages.postDebug(e.toString());
            }
        }

        /**
         *  Write the file by writing the values from the labels, and the values list.
         *  @param strFile      the file to be written.
         *  @param aListLabels  arraylist of texfields of labels.
         *  @param aListValues  arraylist of texfields of values.
         */
        protected void writeFile(String strUser, String strFile, ArrayList aListLabels,
                                    ArrayList aListValues)
        {
            String strPath = FileUtil.openPath(strFile);
            String strLabel = "";
            String strValue = "";
            StringBuffer sbValues = new StringBuffer();
            JTextField txfLabel = null;
            JTextField txfValue = null;
            boolean bNewFile = false;
            int nFDAMode = Util.getPart11Mode();

            // if it's the part11 pnl and the mode is nonFDA,
            // then don't write the file for this panel
	    // the other way is not true.
            if (this instanceof DisplayParentDirectory)
            {
                if ( (isPart11Pnl() && nFDAMode == Util.NONFDA) )
                {
                    return;
                }
            }

            if (strPath == null)
            {
                strPath = FileUtil.savePath(strFile);
                bNewFile = true;
            }

            if (strPath == null || aListLabels == null)
                return;

            if (aListValues == null)
                aListValues = new ArrayList();

            // Get the list of the textfields for values and labels.
            int nLblSize = aListLabels.size();
            int nValueSize = aListValues.size();
            ArrayList<String> labelList = new ArrayList<String>();

            // Get the value from each textfield, and add it to the buffer.
            for (int i = 0; i < nLblSize; i++)
            {
                txfLabel = (JTextField)aListLabels.get(i);
                txfValue = (i < nValueSize) ? (JTextField)aListValues.get(i) : null;

                strLabel = (txfLabel != null) ? txfLabel.getText() : null;
                strValue = (txfValue != null) ? txfValue.getText() : "";
                
                // We need to be sure they don't have two data directories with
                // the same labels.  Save the label list if we are writing to
                // the "data" file.
                if(strFile.indexOf("data") != -1) {
                    if(labelList.contains(strLabel)) {
                        Messages.postError("The Data Directory specifications "
                                    + "must not have duplicate Label names. "
                                    + "The Label \"" + strLabel + "\" is duplicated.  "
                                    + "Skipping the second instance.  Please "
                                    + "specify a new Label.");
                        continue;
                    }
                    else
                        labelList.add(strLabel);
                }

                if (strLabel == null || strLabel.trim().length() <= 0
                        || strValue.equals(INFOSTR))
                    continue;

                // for user template tab, don't need to parse the value
                if (!(this instanceof DisplayTemplate))
                    strValue = getValue(strUser, strValue);
                strLabel = strLabel.trim();
                if (strValue != null)
                    strValue = strValue.trim();

                //sbValues.append("\"");
                sbValues.append(strLabel);
                //sbValues.append("\"  ");
                
                sbValues.append(File.pathSeparator);
                sbValues.append(strValue);
                sbValues.append("\n");
            }

            if (Util.isPart11Sys())
                writeAuditTrail(strPath, strUser, sbValues);

            // write the data to the file.
            BufferedWriter writer = WFileUtil.openWriteFile(strPath);
            WFileUtil.writeAndClose(writer, sbValues);

            // if it's a template file, then make it writable for everyone.
            if (bNewFile)
            {
                String strCmd = "chmod 755 ";
                if (this instanceof DisplayTemplate)
                    strCmd = "chmod 777 ";
                if (Util.iswindows())
                    strPath = UtilB.windowsPathToUnix(strPath);
                String[] cmd = {WGlobal.SHTOOLCMD, WGlobal.SHTOOLOPTION, strCmd + strPath};
                WUtil.runScriptInThread(cmd);
            }
        }

        protected void writeAuditTrail(String strPath, String strUser, StringBuffer sbValues)
        {
            BufferedReader reader = WFileUtil.openReadFile(strPath);
            String strLine;
            ArrayList aListData = WUtil.strToAList(sbValues.toString(), false, "\n");
            StringBuffer sbData = sbValues;
            String strPnl = (this instanceof DisplayTemplate) ? "Data Template "  :
                                "Data Dir ";
            if (reader == null)
            {
                Messages.postDebug("Error opening file " + strPath);
                return;
            }

            try
            {
                while ((strLine = reader.readLine()) != null)
                {
                    // if the line in the file is not in the arraylist,
                    // then that line has been deleted
                    if (!aListData.contains(strLine))
                        WUserUtil.writeAuditTrail(new Date(), strUser, "Deleted " + strPnl + strLine);

                    // remove the lines that are also in the file or those which
                    // have been deleted.
                    aListData.remove(strLine);
                }

                // Traverse through the remaining new lines in the arraylist,
                // and write it to the audit trail
                for (int i = 0; i < aListData.size(); i++)
                {
                    strLine = (String)aListData.get(i);
                    WUserUtil.writeAuditTrail(new Date(), strUser, "Added " + strPnl + strLine);
                }
                reader.close();
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
        }

        protected void clear(JComponent comp)
        {
            int nCompCount = comp.getComponentCount();
            ArrayList aListComps = new ArrayList();

            for (int i = 0; i < comp.getComponentCount(); i++)
            {
                Component compChild = comp.getComponent(i);
                if (compChild instanceof JComponent)
                {
                    JComponent jcomp = (JComponent)compChild;
                    if (jcomp instanceof JCheckBox || jcomp instanceof JTextField)
                        aListComps.add(comp);
                    else
                        clear(jcomp);
                }
            }
            clear(aListComps);
        }

        protected void clear(ArrayList aListComps)
        {
            for (int i = 0; i < aListComps.size(); i++)
            {
                JComponent comp = (JComponent)aListComps.get(i);
                comp.removeAll();
            }

        }

        /**
         *  Returns the value in the proper format: "value".
         */
        protected String getValue(String strUser, String strValue)
        {

            ArrayList aListValues = WUtil.strToAList(strValue);
            String strNewValue = "";

            if (strValue == null || strValue.trim().length() <= 0)
                return "";

            String strPath = FileUtil.openPath("SYSPROF"+File.separator+strUser) + File.pathSeparator +
                                FileUtil.openPath("USRPROF"+File.separator+strUser);
            HashMap hmUser = WFileUtil.getHashMap(strPath);

            for(int i = 0; i < aListValues.size(); i++)
            {
                strNewValue = (String)aListValues.get(i);
                if (strNewValue == null)
                    strNewValue = "";

                // if the value is of the form: $home, then parse the value
                if (hmUser != null && strNewValue.indexOf('$') >= 0)
                {
                    strValue = WFileUtil.parseValue(strNewValue, hmUser);
                    if (strValue != null && strValue.trim().length() > 0)
                        strNewValue = strValue;
                }
            }

            return strNewValue;
        }

        protected class WFileCache extends WMRUCache
        {
            public WFileCache(int nMax)
            {
                super(nMax);
            }

            public WFile readFile(String strPath)
            {
                File objFile = new File(strPath);
                HashMap hmConts = new HashMap();
                setHM(strPath, hmConts);

                return new WFile(objFile.lastModified(), hmConts);
            }

        }
    }
}


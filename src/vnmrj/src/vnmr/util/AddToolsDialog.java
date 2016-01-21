/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.awt.event.*;

import java.awt.dnd.*;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.*;

import java.io.*;
import java.util.List;
import java.util.*;
import java.beans.*;

import vnmr.bo.*;
import vnmr.ui.*;



public class AddToolsDialog extends ModelessDialog
implements ActionListener, ListSelectionListener, DragSourceListener,
DragGestureListener, PropertyChangeListener
{
    private static JList   m_currentToolList;
    private static JList   m_imgList;
    private static JList   m_updateToolList       = new JList();
    private static Font    m_font;

    private static ArrayList<String> m_arrRecOrderedList
            = new ArrayList<String>();
    private static Vector<String> m_vecMkOrderedList = new Vector<String>();

    private VjToolBar  m_Toolbar 		= null;
    protected ExpPanel m_objExpIf;
    private static JComponent m_resetComp	= null;

    private static final      int BOX_HEIGHT 	= 500;
    private static final      int BOX_WIDTH  	= 600;

    private static boolean    m_bIsToolsEditable   = false;
    public  static boolean    m_bIsNewToolDrag     = false;
    private static boolean    m_bIsSystemDefault   = false;
    private static boolean    m_bIsLocalTools      = false;
    private static boolean    m_bRemoveNewTool     = false;
    private static boolean    m_bIsundotool	   = false;
    private static boolean    m_bIsdeltool 	   = false;
    private static boolean    m_bIsundeltool 	   = false;


    private static GridBagLayout gbl 	        = new GridBagLayout();
    private static DragSource    dragSource     = new DragSource();

   DragGestureRecognizer dragRecognizer
    = dragSource.createDefaultDragGestureRecognizer(
                    null,
                        DnDConstants.ACTION_COPY_OR_MOVE,
                    this );

    Border border = new BevelBorder( BevelBorder.RAISED );

    private final static JLabel tooltipLabel  = new JLabel( Util.getLabel("_Tool_Tip" ));
    private final static JLabel commandLabel  = new JLabel( Util.getLabel("_Command" ));
    private final static JLabel toolLabel     = new JLabel( Util.getLabel("_Tool_Label" ));
    private final static JLabel m_setCommandLabel = new JLabel(Util.getLabel("_Set_Command"));
    private final static JLabel space         = new JLabel( "  " );

    private static String     m_strPreCommand            = "";
    private static String     m_strPreLabel              = "";
    private static String     m_strPreToolTip            = "";
    private static String     m_strPreSetCommand         = "";

    // Undo fields
    private static String     m_strUndoTooltip	    = "";
    private static String     m_strUndoCommand	    = "";
    private static String     m_strUndoLabel	    = "";
    private static String     m_strUndoSetComm      = "";
    private static String     m_strUndoIcon         = "";
    private static String     m_strCurrentTooltip   = "";
    private static String     m_strCurrentCommand   = "";
    private static String     m_strCurrentLabel     = "";
    private static String     m_strCurrSetComm      = "";
    private static String     m_strCurrIcon             = "";

    private static int        m_preHighlightIndex     = -1;
    private static int        m_imgIndex              = -1;
    private static int        m_mkHLImgIndex          = -1;
    private static int        m_highlightIndex        = -1;
    private static int 	      m_delcounter	      = 0;
    private int               toolBarY1 = 0;
    private int               toolBarY2 = 0;
    private boolean           bEnterTarget = false;

    private final static JTextField tooltipField    = new JTextField();
    private final static JTextField commandField    = new JTextField();
    private final static JTextField toolLabelField  = new JTextField();
    private final static JTextField m_setCommandField = new JTextField();

    private final static JComboBox  availtoolbars   = new JComboBox();

    private static DefaultListModel listModel 	    = new DefaultListModel();

    private final static JButton newtool = new JButton( "    New Tool    " );
    // private final static JLabel newtool = new JLabel( "    New Tool    " );


    public AddToolsDialog( VjToolBar tb, ExpPanel expIf, String strhelpfile )
    {
        super( Util.getLabel("_Tools_Editor") );
        setSize( BOX_WIDTH, BOX_HEIGHT );
        setLocation( 300, 200 );

        m_Toolbar = tb;
        m_strHelpFile = strhelpfile;

        GridBagConstraints gbc = new GridBagConstraints();

        JPanel northPanel = new JPanel();
        //getContentPane().add( mainPanel, BorderLayout.CENTER );
        getContentPane().add( northPanel, BorderLayout.NORTH );
        northPanel.setLayout( gbl );

        setToolBarCombo();
        setImageList();
        Vector<String> toollist = setToolList();
        showUIComponent( gbc, northPanel, toollist );

        // Put the scrollable icon-list in the expandable center section
        JPanel mainPanel = new JPanel();
        mainPanel.setLayout(new BoxLayout(mainPanel, BoxLayout.X_AXIS));
        JScrollPane pnl = new JScrollPane( m_imgList );
        Dimension dim = pnl.getPreferredSize();
        dim.height = 2000;
        pnl.setMaximumSize(dim);
        mainPanel.add(pnl);
        mainPanel.add(Box.createHorizontalGlue());
        getContentPane().add(mainPanel, BorderLayout.CENTER );

        setActionListeners();
        clearUndoFields();
        unabledUndoActions();
        m_bIsToolsEditable = true;
        m_Toolbar.rewriteDefaultXMLFile();

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                m_Toolbar.setEditMode(false);
                m_bIsToolsEditable = false;
            }
            public void windowOpened(WindowEvent e) {
                m_Toolbar.setEditMode(true);
            }
            public void windowClosed(WindowEvent e) {
                m_Toolbar.setEditMode(false);
                m_bIsToolsEditable = false;
            }
         });

        addComponentListener(new ComponentAdapter() {
            public void componentShown(ComponentEvent evt) {
                m_Toolbar.setEditMode(true);
            }

            public void componentHidden(ComponentEvent evt) {
                m_Toolbar.setEditMode(false);
            }
        });

        Container cntPane = this.getContentPane();
        if (cntPane != null)
            setBgColor(cntPane.getBackground());

        setUiColor();
        DisplayOptions.addChangeListener(this);
   }

    public void setVisible(boolean bVisible)
    {
        // when the dialog comes up, disable the 'return to snapshot',
        // since there is no snapshot available.
        if (bVisible)
        {
            String strTxt = Util.getLabel("mlHistReturnToSnapshot");
            historyButton.setEnabled(strTxt, false);
        }
        else
            m_Toolbar.setComponent(null);
        super.setVisible(bVisible);
    }

    private void setUiColor() {
        Color bgColor = UIManager.getColor("Panel.background");
        m_imgList.setBackground( bgColor );
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        if (DisplayOptions.isUpdateUIEvent(evt)) {
             SwingUtilities.updateComponentTreeUI(this);
             return;
        }
        setUiColor();

        /*******
        Container cntPane = this.getContentPane();
        if (cntPane != null)
            setBgColor(cntPane.getBackground());
        *******/
    }

    public void updateAllValue()
    {
        //updateAllValue(mainPanel);

        validate();
        repaint();
    }

    public void updateAllValue (JComponent panel){
        int nums = panel.getComponentCount();
        for (int i = 0; i < nums; i++) {
             Component comp = panel.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
                obj.updateValue();
            }
            else if (comp instanceof JComponent)
            {
                JComponent pnlTab = (JComponent)comp;
                if (pnlTab.getComponentCount() > 0)
                    updateAllValue(pnlTab);
            }
        }
    }


   //=========================================================================
   //			Action handlers.
   //=========================================================================

   public void setActionListeners()
   {
    toolFieldsListeners();

    m_currentToolList.addListSelectionListener( this );
        // m_imgList.setBackground( Color.white );
        m_imgList.addMouseListener( new DoubleClicker( m_imgList ) );

    historyButton.setActionCommand("history");
        historyButton.addActionListener(this);
        undoButton.setActionCommand("undo");
        undoButton.addActionListener(this);
        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("abandon");
        abandonButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

        historyButton.addPopListener(new HistoryButtonListener());

        setCloseEnabled(true);
        setAbandonEnabled(true);
        setUndoEnabled(true);

    dragRecognizer.setComponent( newtool );
   }

   private void toolFieldsListeners()
   {
      tooltipField.setFont( m_font );
      // tooltipField.setBackground( Color.white );
      tooltipField.addFocusListener( new FocusListener()
      {
          public void focusGained( FocusEvent e ) {}
          public void focusLost( FocusEvent e )
          {
              JComponent tool = m_Toolbar.getSelectedComponent();
              if( tool != null )
              {
                  JTextField f = ( JTextField ) e.getSource();
                  m_Toolbar.updateTooltip( f.getText() );
                  if (tool instanceof VToolBarButton)
                      ((VToolBarButton)tool).setDnDWithinToolBar(false);
                  keepTrackDataFields( false );
                  unabledUndoActions();
              }
         }
      });

      commandField.setFont( m_font );
      // commandField.setBackground( Color.white );
      commandField.addFocusListener( new FocusListener()
      {
          public void focusGained( FocusEvent e ) {}

          public void focusLost( FocusEvent e )
          {
              JComponent tool = m_Toolbar.getSelectedComponent();
              if( tool != null && getToolEditStatus())
              {
                  JTextField f = ( JTextField ) e.getSource();
                  m_Toolbar.updateCommand( f.getText());
                  if (tool instanceof VToolBarButton)
                      ((VToolBarButton)tool).setDnDWithinToolBar(false);
                  keepTrackDataFields( false );
                  unabledUndoActions();
              }
          }
      });

      m_setCommandField.setFont( m_font );
      // m_setCommandField.setBackground( Color.white );
      m_setCommandField.addFocusListener( new FocusListener()
      {
     public void focusGained( FocusEvent e ) {}

     public void focusLost( FocusEvent e )
     {
            JComponent tool = m_Toolbar.getSelectedComponent();
        if( tool != null )
            {
           JTextField f = ( JTextField ) e.getSource();
               m_Toolbar.updateSetCommand(f.getText());
               if (tool instanceof VToolBarButton)
               ((VToolBarButton)tool).setDnDWithinToolBar(false);
               keepTrackDataFields( false );
               unabledUndoActions();
        }
     }
      });

      toolLabelField.setFont( m_font );
      // toolLabelField.setBackground( Color.white );
      toolLabelField.addFocusListener( new FocusListener()
      {
          public void focusGained( FocusEvent e )  {}
          public void focusLost( FocusEvent e )
          {
              JComponent tool = m_Toolbar.getSelectedComponent();
              if( tool != null )
              {
                  JTextField f = ( JTextField ) e.getSource();
                  String label = f.getText();
                  int counter = 0;
                  StringTokenizer token = new StringTokenizer( label );
                  while( token.hasMoreTokens() )
                  {
                      if( counter == 0 )
                          label = token.nextToken();/* Get rid of the leading spaces. */
                      else
                          label += " " + token.nextToken();

                      counter++;
                  }

                  m_Toolbar.setToolLabel( label );
                  //toolbar.updateToolList( f.getText() );
                  highlightTool( m_Toolbar.getHighlightIndex() );
                  m_imgList.clearSelection();

                  if (tool instanceof VToolBarButton)
                      ((VToolBarButton)tool).setDnDWithinToolBar(false);
                  keepTrackDataFields( false );
                  unabledUndoActions();
              }
          }
      });
   }


   /*
    * Implementing action listener for combo box of the available toolbars
    */

   public void valueChanged( ListSelectionEvent e )
   {
//        Object selectedValues[];
//        String selValue = null;
//
//        selectedValues = m_currentToolList.getSelectedValues();
//
//        for( int i = 0; i < selectedValues.length; i++ )
//             selValue = selectedValues[i].toString();

        //toolbar.setSelectedToolInfo( selValue, currentToolList.getSelectedIndex() );

        m_strUndoTooltip = tooltipField.getText();
        m_strUndoCommand = commandField.getText();
        m_strUndoLabel   = toolLabelField.getText();
        m_strUndoSetComm = m_setCommandField.getText();
        m_strUndoIcon  = getToolIcon();
        m_strCurrentTooltip = tooltipField.getText();
        m_strCurrentCommand = commandField.getText();
        m_strCurrentLabel   = toolLabelField.getText();
        m_strCurrSetComm = m_setCommandField.getText();
        m_strCurrIcon = getToolIcon();
   }

   public void toolChanged(JComponent comp)
   {
        if (comp != null)
        {
            VToolBarButton btn = (VToolBarButton) comp;
            setResetComp(btn);
            m_Toolbar.setSelectedToolInfo(btn);

            m_strUndoTooltip = tooltipField.getText();
            m_strUndoCommand = commandField.getText();
            m_strUndoLabel   = toolLabelField.getText();
            m_strUndoSetComm = m_setCommandField.getText();
            m_strUndoIcon = getToolIcon();

            m_strCurrentTooltip = tooltipField.getText();
            m_strCurrentCommand = commandField.getText();
            m_strCurrentLabel   = toolLabelField.getText();
            m_strCurrSetComm = m_setCommandField.getText();
            m_strCurrIcon = getToolIcon();
        }
   }

   public void actionPerformed( ActionEvent e )
   {
      String cmd = e.getActionCommand();

      if(cmd.equals("undo"))
         performUndo();
      else if(cmd.equals("close"))
      {
          //m_Toolbar.saveChanges();
          //saveVP();

          m_Toolbar.saveChanges();
          setButtonBorder();
          setVisible(false);
          dragRecognizer.setComponent( null );
          m_Toolbar.disposeToolEditor();
          m_bIsToolsEditable = false;
          dispose();
      }
      else if(cmd.equals("abandon"))
      {
          setButtonBorder();
          m_bRemoveNewTool = false;
          m_Toolbar.resetToDefaultTools();
          setVisible(false);
          dragRecognizer.setComponent( null );
          m_Toolbar.disposeToolEditor();
          m_bIsToolsEditable = false;
          dispose();
      }
      else if (cmd.equals("help"))
          displayHelp();
   }

   private void setButtonBorder()
   {
       JComponent objComp = m_Toolbar.getSelectedComponent();
       if (objComp instanceof VToolBarButton)
           ((VToolBarButton)objComp).setBtn();
   }


   //==========================================================================
   //			Handle image files.
   //==========================================================================

   private void setImageList()
   {
       // Get list of paths to all images
       List<String> imagePaths = getImageFiles();

       // Put image names into a sorted set (which also deletes duplicates)
       SortedSet<String> imgSet = new TreeSet<String>();
       for (String str : imagePaths) {
           str = new File(str).getName();
           imgSet.add(str);
       }

       // Put the ones that are valid into a Vector for the JList
        Vector<ListItem> data = new Vector<ListItem>();
        for (String img : imgSet) {
            ImageIcon icon = Util.getVnmrImageIcon( img );
            if (icon != null) {
                int w = icon.getIconWidth();
                int h = icon.getIconHeight();
                if (w > 2 && w <= 32 && h > 2 && h <= 32) {
                    data.addElement(new ListItem(img, icon));
                }
            }
        }

        m_imgList = new JList();
        m_imgList.setCellRenderer( new ListItemRenderer() );
        m_imgList.setListData( data );
        // m_imgList.setBackground( Color.white );
        m_imgList.setToolTipText("Double click to set icon");
    }


    /**
     *  Read in image file names from the iconlib directories.
     */
    private List<String> getImageFiles()
    {
        List<String> imagePaths = new ArrayList<String>();

        String imgDir = FileUtil.openPath("USER/ICONLIB");
        String sysImgDir = FileUtil.openPath("SYSTEM/ICONLIB");
        if(imgDir !=null) {
            // Put stuff into imagePaths
            listPath(new File(imgDir), imagePaths);
        }
        if (sysImgDir != null) {
            // Put more stuff into imagePaths
            listPath(new File(sysImgDir), imagePaths);
        }
        if (imgDir == null && sysImgDir == null) {
            System.out.println("Icon library not found");
        }
        return imagePaths;
    }

    private void listPath( File imagePath, List<String> imagePaths )
    {
       File imgFiles[]; /* list of image files in the directory */

       /* Create list of files in the directory */
       imgFiles = imagePath.listFiles();

       for( int i = 0, n = imgFiles.length; i < n; i++ )
       {
           imagePaths.add( imgFiles[i].toString() );

          if( imgFiles[i].isDirectory() )
          {
             listPath( imgFiles[i], imagePaths );
          }
       }
    }

    public AddToolsDialog getToolsEditor()
    {
    return this;
    }

    //===========================================================================
    //           Methods for performing undo.
    //===========================================================================

    private void performUndo()
    {
        swapData();

        if( m_bRemoveNewTool )
        {
            m_Toolbar.removeNewTool();
            tooltipField.setText( "" );
            commandField.setText( "" );
            toolLabelField.setText( "" );
            m_setCommandField.setText("");
            m_bRemoveNewTool = false;
            m_bIsundotool = true;
        }
        else
        {
            if( m_bIsundotool && !( m_Toolbar.isDnDWithinToolBar() ))
            {
                updateToolsOrder( m_Toolbar.getUndoTools() );
                highlightTool( m_Toolbar.getUndoIndex() );
                //JButton tool = ( JButton ) resetComp;
                String label  = "";
                if (m_resetComp instanceof VToolBarButton)
                {
                    VToolBarButton tool = (VToolBarButton) m_resetComp;
                    label = (tool.getAttribute(VObjDef.ICON) == null) ? ""
                            : tool.getAttribute(VObjDef.ICON);
                }
                if (label == null || label.equals(""))
                    label = "New Tool";
                m_Toolbar.undoTool( label );
                m_bRemoveNewTool = true;
                m_bIsundotool = false;
            }
        }

        if( m_Toolbar.isDnDWithinToolBar() )
            m_Toolbar.undoToolPos( m_resetComp );

        if( m_bIsSystemDefault )
        {
            m_Toolbar.undoSystemInitial();
            m_bIsLocalTools = true;
            m_bIsSystemDefault = false;
        }
        else
        {
            if( m_bIsLocalTools )
            {
                m_Toolbar.setToSystemInitial( false );
                m_bIsSystemDefault = true;
                m_bIsLocalTools = false;
            }
        }

        if( m_bIsdeltool )
        {
            m_Toolbar.taggleDelete( "tmpFile1.xml",++m_delcounter );
            m_bIsdeltool = false;
            setDelToolFlag( false );
            m_bIsundeltool = true;
        }
        else
        {
            if( m_bIsundeltool )
            {
                m_Toolbar.taggleDelete( "tmpFile2.xml", ++m_delcounter );
                clearUndoFields();
                m_bIsdeltool = true;
                setDelToolFlag( true );
                m_bIsundeltool = false;
            }
        }
   }

   /* Copy current fields' data for undo action. */
   private void swapData()
   {
	    String tmp_tooltip   = "";
        String tmp_command   = "";
        String tmp_label     = "";
        String tmp_setCommand = "";
        String tmp_icon = "";

        tooltipField.setText( m_strUndoTooltip );
        commandField.setText( m_strUndoCommand );
        // if a label is given, set the label
        if (m_strUndoLabel != null && m_strUndoLabel.length() > 0)
        {
            toolLabelField.setText( m_strUndoLabel );
            m_Toolbar.setToolLabel(m_strUndoLabel);
            clearImageSelection();
        }
        // else highlight the icon.
        else if (m_strUndoIcon != null && m_strUndoIcon.length() > 0)
        {
            clearImageSelection();
            selectImageInList(m_strUndoIcon);
            toolLabelField.setText("");
            m_Toolbar.setToolLabel(m_strUndoIcon);
        }

        m_setCommandField.setText(m_strUndoSetComm);

        tmp_tooltip = m_strCurrentTooltip;
        tmp_command = m_strCurrentCommand;
        tmp_label   = m_strCurrentLabel;
        tmp_setCommand = m_strCurrSetComm;
        tmp_icon = m_strCurrIcon;

        /* Assign undo data to current data */
        m_strCurrentTooltip = m_strUndoTooltip;
        m_strCurrentCommand = m_strUndoCommand;
        m_strCurrentLabel   = m_strUndoLabel;
        m_strCurrSetComm = m_strUndoSetComm;
        m_strCurrIcon = m_strUndoIcon;

        /* Assign current data to undo data */
        m_strUndoTooltip = tmp_tooltip;
        m_strUndoCommand = tmp_command;
        m_strUndoLabel   = tmp_label;
        m_strUndoSetComm = tmp_setCommand;
        m_strUndoIcon = tmp_icon;
   }

   private void mkSnapshot()
   {
      // After making the snapshot, enable the 'return to snapshot'.
      String strTxt = Util.getLabel("mlHistReturnToSnapshot");
      historyButton.setEnabled(strTxt, true);

      m_Toolbar.markCurrentChanges();
      m_Toolbar.markCurrentComp();
      m_Toolbar.mkCurrentToolIndex();
      m_highlightIndex = m_Toolbar.getHighlightIndex();
      m_preHighlightIndex = m_highlightIndex;
      highlightTool( m_highlightIndex );

      if( !m_imgList.isSelectionEmpty() )
         m_mkHLImgIndex = m_imgIndex;
      else
         m_mkHLImgIndex = -1;

      /* mark ordered list tools */
      if( m_vecMkOrderedList.size() > 0 )
          m_vecMkOrderedList.removeAllElements();
      for( int i = 0; i < m_arrRecOrderedList.size(); i++ )
          m_vecMkOrderedList.addElement(m_arrRecOrderedList.get( i ));

      m_strPreCommand = commandField.getText();
      m_strPreToolTip = tooltipField.getText();
      m_strPreLabel   = toolLabelField.getText();
      m_strPreSetCommand = m_setCommandField.getText();
   }

   private void returnSnapshot()
   {
      updateToolsOrder( m_vecMkOrderedList );
      highlightTool( m_preHighlightIndex );
      m_Toolbar.resetTools();
      m_Toolbar.resetToPreComp();

      if( m_mkHLImgIndex != -1 )
      {
         m_imgList.setSelectedIndex( m_mkHLImgIndex );
         m_imgList.ensureIndexIsVisible( m_mkHLImgIndex );
      }
      else
         m_imgList.clearSelection();

      commandField.setText( m_strPreCommand );
      toolLabelField.setText( m_strPreLabel );
      tooltipField.setText( m_strPreToolTip );
      m_setCommandField.setText(m_strPreSetCommand);
   }

   private void returnSystemDefault()
   {
      m_Toolbar.setToSystemInitial( true );
      m_bIsSystemDefault = true;
   }

   private void clearUndoFields()
   {
      m_strUndoTooltip = "";
      m_strUndoCommand = "";
      m_strUndoLabel   = "";
      m_strUndoSetComm = "";
      m_strUndoIcon = "";

      m_strCurrentTooltip = "";
      m_strCurrentCommand = "";
      m_strCurrentLabel   = "";
      m_strCurrSetComm = "";
      m_strCurrIcon = "";

      tooltipField.setText( "" );
      toolLabelField.setText( "" );
      commandField.setText( "" );
      m_setCommandField.setText("");
   }

   private void unabledUndoActions()
   {
      m_bIsSystemDefault = false;
      m_bIsLocalTools    = false;
      m_bIsundotool      = false;
      m_bRemoveNewTool   = false;
      m_bIsdeltool	      = false;
      m_bIsundeltool     = false;
   }

   private void resetToInit()
   {
      Vector<String> defaultTools = m_Toolbar.getDefaultTools();

      toolLabelField.setText( "" );
      tooltipField.setText( "" );
      commandField.setText( "" );
      m_setCommandField.setText("");
      updateToolsOrder( defaultTools );
      m_bRemoveNewTool = false;
      m_Toolbar.resetToDefaultTools();
      updateToolsOrder( defaultTools );
      m_imgList.clearSelection();
   }

   public void setDelToolFlag( boolean flag )
   {
       m_bIsdeltool = flag;
   }

   //===========================================================================
   //           Update tool's fields methods.
   //===========================================================================

   public void keepTrackDataFields( boolean isUndoDnD )
   {
       // Copy last action to preStates
       m_strUndoTooltip   = m_strCurrentTooltip;
       m_strUndoCommand   = m_strCurrentCommand;
       m_strUndoLabel     = m_strCurrentLabel;
       m_strUndoSetComm = m_strCurrSetComm;
       m_strUndoIcon = m_strCurrIcon;

       // Reset new data to the currentStates
       m_strCurrentTooltip   = tooltipField.getText();
       m_strCurrentCommand   = commandField.getText();
       m_strCurrentLabel     = toolLabelField.getText();
       m_strCurrSetComm  = m_setCommandField.getText();
       m_strCurrIcon     = getToolIcon();

       if( isUndoDnD )
       {
           m_bRemoveNewTool = true;
           m_bIsSystemDefault = false;
           m_bIsLocalTools    = false;
           m_bIsundotool      = false;
       }
       else
           m_bRemoveNewTool = false;
   }

    public void updateToolsOrder( Vector<String> orderedList )
    {
        listModel.removeAllElements();

        for( int i = 0; i < orderedList.size(); i++ )
           listModel.addElement( orderedList.elementAt( i ) );

        m_updateToolList.validate();
        m_updateToolList.repaint();

        /* Record ordered list */
        if( m_arrRecOrderedList.size() > 0 )
           m_arrRecOrderedList.clear();

        for( int j = 0; j < orderedList.size(); j++ )
           m_arrRecOrderedList.add(orderedList.get( j ));
    }

    public void showCommand( String command )
    {
        commandField.setText( command );
    }

    public void showSetCommand(String command)
    {
    m_setCommandField.setText(command);
    }

    public static void resetTooltip( String newTooltip )
    {
        tooltipField.setText( newTooltip );
    }

    public boolean selectImageInList(String imageName)
    {
        int idx = getIndexOf(imageName, m_imgList);
        if (idx >= 0) {
            m_imgList.setSelectedIndex(idx);
            m_imgList.ensureIndexIsVisible(idx);
            return true;
        }
        return false;
    }

    /**
     * Get the index of the icon with the given name in the given JList.
     * @param imageName The name to look for.
     * @param imgList The list to look in.
     * @return The index of the matching item, or -1;
     */
    private int getIndexOf(String imageName, JList imgList) {
        int idx = -1;
        if (imageName == null)
            return idx;
        ListModel model = imgList.getModel();
        int len = model.getSize();
        for (int i = 0; i < len; i++) {
            ListItem item = (ListItem)model.getElementAt(i);
            if (item != null) {
               if (imageName.equals(item.getName())) {
                   idx = i;
                   break;
               }
            }
        }
        return idx;
    }

    /**
     * See if a given name is the name of a valid icon file.
     * @param name The name to check.
     * @return True if there is an icon of that name in either the user
     * or system iconlib.
     */
    public boolean isIconName(String name) {
        return getIndexOf(name, m_imgList) >= 0;
    }

    public void highlightTool( int selectionMode )
    {
        m_highlightIndex = selectionMode;
        m_currentToolList.setSelectedIndex( selectionMode );
        m_currentToolList.ensureIndexIsVisible( selectionMode );
    }

    public String getImgFile( int selectedIndex )
    {
        ListModel model = m_imgList.getModel();
        int len = model.getSize();
        String name = null;
        if (selectedIndex < len && selectedIndex >= 0) {
            name = ((ListItem)model.getElementAt(selectedIndex)).getName();
        }
        return name;
    }

    public void clearImageSelection()
    {
        m_imgList.clearSelection();
    }

    public void clearLabelField()
    {
        toolLabelField.setText( "" );
    }

    public void showLabel( String label )
    {
        toolLabelField.setText( label );
        toolLabelField.validate();
    }

    public boolean getRmNewTool()
    {
    return m_bRemoveNewTool;
    }

    public void setResetComp( JComponent comp )
    {
        m_resetComp = comp;
    }

    public static boolean getToolEditStatus()
    {
        return m_bIsToolsEditable;
    }

    public void setToolEditStatus( boolean flag )
    {
        m_bIsToolsEditable = flag;
    }

    public boolean isNewToolDragged()
    {
    return m_bIsNewToolDrag;
    }

    public void unableNewToolDrag()
    {
    m_bIsNewToolDrag = false;
    }

    private void showUIComponent( GridBagConstraints gbc,
                                 JPanel mainPanel,
                                 Vector<String> toollist )
    {
        gbc.anchor = GridBagConstraints.WEST;
        gbc.fill = GridBagConstraints.NONE;

        gbc.gridx = 0;
        gbc.gridy = 1;
        gbc.gridwidth = 1;
        gbc.gridheight = 1;
        gbc.weightx = 0;
        newtool.setFont(DisplayOptions.getFont("TimesRoman", Font.ITALIC, 15));
        newtool.setForeground( Color.blue );
        // EtchedBorder border = new EtchedBorder();
        // newtool.setBorder( border );
		mainPanel.removeAll();
        mainPanel.add( newtool, gbc );

        gbc.fill = GridBagConstraints.HORIZONTAL;

        insertSpace( gbc, mainPanel, 1, 0, 10 );
        insertSpace( gbc, mainPanel, 2, 0, 80 );
        insertSpace( gbc, mainPanel, 3, 0, 10 );

        gbc.fill = GridBagConstraints.NONE;
        insertSpace( gbc, mainPanel, 0, 2, 40 );

        /* VC command area */
        gbc.gridx = 1;
        gbc.gridy = 1;
        mainPanel.add( commandLabel, gbc );
        gbc.gridx = 2;
        gbc.weightx = 80;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        mainPanel.add( commandField, gbc );
        gbc.weightx = 0;

        insertSpace( gbc, mainPanel, 3, 1, 10 );

        /* Set Command area */
        gbc.gridx = 1;
        gbc.gridy = GridBagConstraints.RELATIVE;
        mainPanel.add(m_setCommandLabel, gbc);
        gbc.gridx = 2;
        gbc.weightx = 80;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        mainPanel.add(m_setCommandField, gbc);
        gbc.weightx = 0;

        insertSpace(gbc, mainPanel, 3, 2, 10);

        /* Tool label area */
        gbc.gridx = 0;
        gbc.gridy = GridBagConstraints.RELATIVE;
        gbc.fill = GridBagConstraints.NONE;
        Font font = DisplayOptions.getFont("Dialog", Font.PLAIN, 12);
        JLabel text = new JLabel( Util.getLabel("_To_create_a_new_tool") );
        text.setFont(font);
        // text.setForeground( Color.darkGray );
        mainPanel.add( text, gbc );
        gbc.gridx = 1;
        mainPanel.add( toolLabel, gbc );
        gbc.gridx = 2;
        gbc.weightx = 80;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        mainPanel.add( toolLabelField, gbc );
        gbc.weightx = 0;

        gbc.gridx = 3;
        mainPanel.add( space, gbc );

        /* Tool tip area */
        gbc.gridx = 0;
        gbc.gridy = GridBagConstraints.RELATIVE;
        gbc.fill = GridBagConstraints.NONE;
        text = new JLabel( Util.getLabel("_drag_and_drop_the_icon_to_the_tool_bar.") + "    ");
        text.setFont(font);
        // text.setForeground( Color.darkGray );
        mainPanel.add( text, gbc );
        gbc.gridx = 1;
        mainPanel.add( tooltipLabel, gbc );
        gbc.gridx = 2;
        gbc.weightx = 80;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        mainPanel.add( tooltipField, gbc );
        gbc.weightx = 0;

        insertSpace( gbc, mainPanel, 3, 3, 10 );

        gbc.gridx = 0;
        gbc.gridy = 4;
        gbc.weighty = 30;
        gbc.weighty = 0;
        mainPanel.add( new JLabel( "  " ), gbc );

        insertSpace( gbc, mainPanel, 1, 4, 80 );
        insertSpace( gbc, mainPanel, 2, 4, 20 );

        /* Current tool list area */
//        gbc.gridx = 0;
//        gbc.gridy = 5;
//        gbc.weightx = 10;
//        gbc.weighty = 30;
//        text = new JLabel( "Current Tools List: " );
//        text.setFont( new Font( "Dialog", Font.PLAIN, 15 ));
//        mainPanel.add( text, gbc );
//
//        insertSpace( gbc, mainPanel, 1, 5, 80 );

        gbc.gridx = 0;
        gbc.gridy = 5;
        gbc.weighty = 0;
        text = new JLabel( Util.getLabel("_Image_Selection_Area") );
        text.setFont(DisplayOptions.getFont( "Dialog", Font.PLAIN, 15));
        mainPanel.add( text, gbc );

        insertSpace( gbc, mainPanel, 0, 6, 10 );

        gbc.weighty = 30;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.gridx = 0;
        gbc.gridy = GridBagConstraints.RELATIVE;

        gbc.gridx = 3;
        gbc.gridy = GridBagConstraints.RELATIVE;
        gbc.weighty = 0;
        mainPanel.add( space, gbc );
    }

    private void insertSpace( GridBagConstraints gbc, JPanel panel,
                  int gx, int gy, int wx )
    {
    gbc.gridx = gx;
    gbc.gridy = gy;
    //gbc.weightx = wx;
    panel.add( space, gbc );
    }

    private Vector<String> setToolList()
    {
        m_currentToolList = new JList( listModel );
        // m_currentToolList.setBackground( Color.white );

        Vector<String> defaultList = m_Toolbar.getDefaultTools();
        listModel.removeAllElements();

        for( int i = 0; i < defaultList.size(); i++ )
           listModel.addElement( defaultList.elementAt( i ));

        return defaultList;
    }

    private void setToolBarCombo()
    {
        availtoolbars.addItem( " Default Tool Bar ");
        availtoolbars.addItem( " Bayes ");
        availtoolbars.setFont( m_font );
    }

    private String getToolIcon()
    {
    String strIcon = "";
    if (m_resetComp instanceof VToolBarButton)
    {
        strIcon = ((VToolBarButton)m_resetComp).getAttribute(VObjDef.ICON);
        if (strIcon == null)
        return "";
        int nIndex = (strIcon != null) ? strIcon.indexOf('.') : -1;
        if ( nIndex < 0 || !(strIcon.substring(nIndex+1).equals("gif")))
        strIcon = "";
    }
    return strIcon;
    }

    /**
     * Override the method in the modeless dialog.
     * @param e The KeyEvent that generated this call.
     */
    public void keyPressed(KeyEvent e)
    {
    // override the method in the modeless dialog.
    // do nothing because escape and return keys in the modeless dialog,
    // do abandon and exit, respectively, and we don't want that behavior
    // for the tools editor.
    }


    //===========================================================================
   //           DragGestureListener and DragSourceListener implementations.
   //===========================================================================

   public void dragGestureRecognized( DragGestureEvent e )
   {
      m_bIsNewToolDrag = true;
      Component comp = e.getComponent();

      LocalRefSelection ref = new LocalRefSelection( comp );
      try {
          dragSource.startDrag( e, DragSource.DefaultCopyNoDrop, ref, this );
      } catch (InvalidDnDOperationException idoe) {
          Messages.postDebug("AddToolsDialog.dragGestureRecognized: " + idoe);
          return;
      }
      JComponent vt = Util.getUsrToolBar();
      if (vt != null && vt.isVisible()) {
          Point pt = vt.getLocationOnScreen();
          toolBarY1 = pt.y;
          toolBarY2 = toolBarY1 + vt.getHeight();
      }
      else {
          toolBarY1 = 0;
          toolBarY2 = 0;
      }
    }

    public void dragDropEnd( DragSourceDropEvent e ) {}
    public void dragEnter( DragSourceDragEvent e ) {
       DragSourceContext ds = e.getDragSourceContext();
       int y = e.getY();
       if (y >= toolBarY1 && y <= toolBarY2) {
            ds.setCursor(DragSource.DefaultCopyDrop);
            bEnterTarget = true;
       }
    }

    public void dragExit( DragSourceEvent e ) {
       if (bEnterTarget) {
           DragSourceContext ds = e.getDragSourceContext();
           ds.setCursor(DragSource.DefaultCopyNoDrop);
           bEnterTarget = false;
       }
    }

    public void dragOver( DragSourceDragEvent e ) {}
    public void dropActionChanged( DragSourceDragEvent e ) {}


    /********************************************************** <pre>
     * Summary: Inner Class Listener for selection of item from menu
     *
     *
     </pre> **********************************************************/

    public class HistoryButtonListener implements PopListener {
        public void popHappened(String popStr) {
            if (popStr.equals(Util.getLabel("mlHistReturnInitState")))
                resetToInit();
            else if (popStr.equals(Util.getLabel("mlHistMakeSnapshot")))
                mkSnapshot();
            else if (popStr.equals(Util.getLabel("mlHistReturnToSnapshot")))
                returnSnapshot();
            else if (popStr.equals(Util.getLabel("mlHistReturnToDefault")))
                returnSystemDefault();
        }
    }
}


/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.awt.*;
import java.awt.event.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;

import java.util.*;
import java.io.*;
import java.net.*;
import java.beans.*;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.*;
import javax.swing.plaf.basic.*;

import vnmr.util.*;
import vnmr.templates.*;
import vnmr.bo.*;
import vnmr.sms.*;


public class VjToolBar extends JToolBar
    implements DragGestureListener, DragSourceListener, DropTargetListener,
    PropertyChangeListener, VObjDef
{
    private Vector<String> m_vecDefaultTools	= new Vector<String>();
    private static ArrayList  m_vecAvailToolsV	= new ArrayList();
    private static ArrayList  m_vecPositionV	= new ArrayList();
    private static Vector     m_vecCurrentOrderedPos = new Vector();
    private static ArrayList  m_vecSortedPos	= new ArrayList();
    private static Vector<String> m_vecUndotoollist = new Vector<String>();
    private static ArrayList  m_vecToolsLoc	= new ArrayList();
    private static Vector     m_vecUndoPosList	= new Vector();
    private static Vector     m_vecUndolist	= new Vector();
    private static Vector     m_vecDeltoollist	= new Vector();

    private static HashMap  m_hmTools 	= new HashMap();

    private final  static int	YPOS		= 2;
    private static boolean	m_bIsDefaultTools  = true;
    private static boolean      m_bIsnewtool       = false;
    private static boolean      m_bMvToTrashCan    = false;
    private static boolean      bImagingApp = false;
    private boolean             m_bInEditMode      = false;
	private boolean             m_bVpEditMode      = false;

    private VjToolBar	  m_objToolbar 	= null;
    protected boolean m_bInitialize = false;
    private static AddToolsDialog m_objToolsEditor = null;
    private static MouseAdapter   m_objMouseAdapter = null;
    private ModelessPopup m_objVPEditor;
    private Color bgColor;
    private Color hiliColor;

    private static JComponent m_resetComp      = null;
    private static JComponent m_mkComp         = null;
    private TPanel   m_pnlDisplay     = new TPanel();
    // private JToolBar   m_pnlDisplay     = new JToolBar();
    private JPanel   vpPanel     = new JPanel();
	private static JPanel     m_pnlVpDisplay;
    private VpObj vpNumObj;
    private boolean  bUpdateVpObj = true;
    private static int		  m_nNewtoolDnD	   = 0;
    private static int		  m_nDndPos 	   = -1;
    private static int		  m_nDndPosY 	   = -1;

    private static int		  m_nIndex	= 0;
    private static int  	  m_nUndotoolIndex = -1;
    private static int 		  m_nToolIndex 	= -1;

    private final static String  m_strToolbarfile    = "DefaultToolBar.xml";
    private final static String  m_strMkchangesfile  = "MarkChanges.xml";
    private final static String  m_strTmpfile1       = "tmpFile1.xml";
    private final static String  m_strTmpfile2       = "tmpFile2.xml";
	private final static String  m_strVpfile         = "viewport.xml";

    private final static Border  m_lineBorder = BorderFactory.createLineBorder(Color.pink);
    private final static Border  m_raisedBorder = BorderFactory.createRaisedBevelBorder();

    private final static int FIVE_SEC = 5000;

    private ExpPanel m_objExpIF = null;
    private SessionShare m_objSShare = null;
    protected StringBuffer m_sbData = null;

    /* Drag and drop variables */
    private DragSource        m_objDragSource        = new DragSource();
    DropTarget m_objDropTarget = new DropTarget(
                                            this,  // component
                                            DnDConstants.ACTION_COPY_OR_MOVE,
                                            this );  // DropTargetListener

    private ToolBarLayout toobarLayout;

    private VOnOffButton  ctrlButton;

	public final static String DEFAULT_TOOLBAR = "Vnmrj ToolBar";
	public final static String VP_TOOLBAR = "Viewport ToolBar";

    //=====================================================================
    //  Tool bar construction and initialization.
    //=====================================================================

    public VjToolBar()
    {
        this(null, null);
    }

    public VjToolBar(SessionShare objSShare, ExpPanel expIF)
    {
        super();
        // Util.setBgColor(this);
        setFloatable(false);
        m_objToolbar = this; // ???
        m_objExpIF = expIF;
        m_objSShare = objSShare;
        bImagingApp = Util.isImagingUser();
        if (Util.getViewportMenu() == null) // if there is no VpMenu.xml
            bImagingApp = true;
        bgColor = Util.getToolBarBg();
        setLayout(new toolLayout());

        ctrlButton = new VOnOffButton(VOnOffButton.ICON, this);
        ctrlButton.setPreferredSize(new Dimension(8, 16));
        add(ctrlButton);
        add(m_pnlDisplay);
        m_pnlDisplay.setOpaque(false);
        m_pnlDisplay.setBorder(null);
        vpPanel.setBorder(null);
        vpPanel.setLayout(new VpPanLayout());

        vpNumObj = new VpObj("$VALUE=jviewports[1]", 1);

        addMouseListener( new MouseAdapter()
        {
            public void mouseClicked( MouseEvent e ) {}
        });

        m_objMouseAdapter = new MouseAdapter()
        {
            public void mouseClicked( MouseEvent e )
            {
				JComponent comp = null;

                   if (m_objToolsEditor == null)
                       return;
				if( m_objToolsEditor.getToolEditStatus())
				{
					if( e.getClickCount() == 1   )
					{
					    comp = ( JComponent ) e.getSource();
					    resetToolsInfo( comp );
					}
					m_bIsnewtool = false;
					m_resetComp = comp;
				}
			}
        };

        // if (bImagingApp)
        //      add(vpPanel, BorderLayout.EAST);

        toobarLayout = new ToolBarLayout(this);
        m_pnlDisplay.setLayout(toobarLayout);
		m_pnlDisplay.setName(DEFAULT_TOOLBAR);
        m_objToolbar.setBorder(null);
        m_pnlDisplay.addMouseListener(m_objMouseAdapter);
        vpPanel.setName(VP_TOOLBAR);
        showDefaultTools();
        m_bIsDefaultTools = false;
        DisplayOptions.addChangeListener(this);
        hiliColor = new Color(0, 133, 213);
    }

    public VOnOffButton getControlButton() {
        return ctrlButton;
    }

    public void setExpIf(ExpPanel expIF)
    {
        m_objExpIF = expIF;
    }

    public ExpPanel getExpIf()
    {
        return m_objExpIF;
    }

    public int getRowHeight() {
        return toobarLayout.getRowHeight();
    }

    public int getActualWidth() {
        return toobarLayout.getActualWidth();
    }

    public int getGap() {
        return toobarLayout.getGap();
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        if (DisplayOptions.isUpdateUIEvent(evt)) {
           bgColor = Util.getToolBarBg();
           setBackground(bgColor);
           m_pnlDisplay.setBackground(bgColor);
           vpPanel.setBackground(bgColor);
           toobarLayout.saveLocations(m_pnlDisplay);
        }
    }

    public String getToolbarfile()
    {
        return m_strToolbarfile;
    }

    public String getVpfile()
    {
        return m_strVpfile;
    }

    public ModelessPopup getVpEditor()
    {
        if (m_objVPEditor == null)
        {
            String strFile = FileUtil.openPath("INTERFACE/VpDialog.xml");
            if (m_objExpIF != null && strFile != null)
            {
                Object frame = m_objExpIF.getModelessPopup(strFile);
                if (frame != null)
                    m_objVPEditor = (ModelessPopup)frame;
            }
        }
        return m_objVPEditor;
    }

	public JComponent getToolBar()
	{
	    return m_pnlDisplay;
	}

	public JComponent getVPToolBar()
	{
            if (bImagingApp)
	        return vpPanel;
            return null;
	}

    public void updateValue()
    {
        updateAllValue(m_pnlDisplay);
        updateAllValue(vpPanel);
        if (bUpdateVpObj)
            vpNumObj.updateValue();
    }

	public void updateValue(Vector params)
	{
	    updateValue(params, m_pnlDisplay);
		updateValue(params, vpPanel);
		if (m_objToolsEditor != null)
            m_objToolsEditor.updateAllValue();
        m_bInitialize = true;
            if (bUpdateVpObj)
                vpNumObj.updateValue();
	}

    public void updateValue (Vector params, JComponent toolbar) {
        int k = toolbar.getComponentCount();
        for (int i = 0; i < k; i++) {
            Component comp = toolbar.getComponent(i);
            if (comp instanceof ExpListenerIF) {
                ExpListenerIF e = (ExpListenerIF)comp;
                e.updateValue(params);
            }
            else if (comp instanceof JToolBar || comp instanceof JPanel)
				updateValue(params, (JComponent)comp);
        }
		updateAllValue(params, toolbar);
    }

    public void updateAllValue(Vector params, JComponent toolbar)
    {
        int nums = toolbar.getComponentCount();
        for (int i = 0; i < nums; i++) {
            Component comp = toolbar.getComponent(i);
            if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
                String vnmrVar = obj.getAttribute(VARIABLE);
                if (vnmrVar == null)
                    continue;

                StringTokenizer strTokenizer = new StringTokenizer(vnmrVar," ,\n");
                while (strTokenizer.hasMoreTokens())
                {
                    String var = strTokenizer.nextToken();
                    if (params.contains(var.trim()) || !m_bInitialize)
                    {
                        obj.updateValue();
                        break;
                    }
                }
            }
        }
    }

    public void updateAllValue (JComponent toolbar){
        int nums = toolbar.getComponentCount();
        for (int i = 0; i < nums; i++) {
            Component comp = toolbar.getComponent(i);
            if (comp instanceof JToolBar)
                updateAllValue((JComponent)comp);
            else if (comp instanceof VObjIF) {
                VObjIF obj = (VObjIF) comp;
                obj.updateValue();
            }
        }
    }

    private void removeAllVpTools()
    {
       int nBtnCount = vpPanel.getComponentCount();
       while (vpPanel.getComponentCount() > 0) {
             Component objBtn = vpPanel.getComponent(0);
             if (objBtn instanceof VObjIF)
		vpPanel.remove(objBtn);
	}
        vpPanel.validate();
        m_objToolbar.repaint();
    }

    private void setVpToolAttr() {
        int n = vpPanel.getComponentCount();
        for (int i = 0; i < n; i++) {
            Component obj = vpPanel.getComponent(i);
            if (obj instanceof VButton) {
                ((VButton)obj).setToolBarButton(true);
            }
        }
    }

    public void showDefaultTools()
    {
        m_sbData = new StringBuffer();
        final String toolbarPath = FileUtil.openPath(m_sbData.append("INTERFACE/").
                                  append(m_strToolbarfile).toString());
        m_sbData = new StringBuffer();
	final String vpPath  = FileUtil.openPath(m_sbData.append("INTERFACE/" ).
                                  append( m_strVpfile).toString());
        try
        {
            LayoutBuilder.build(m_pnlDisplay, m_objExpIF, toolbarPath);
            toobarLayout.saveLocations(m_pnlDisplay);
            //m_pnlDisplay.validate();

            if (vpPath != null) {
                LayoutBuilder.build(vpPanel, m_objExpIF, vpPath);
                setVpToolAttr();
            }

            m_objToolbar.validate();
            m_objToolbar.repaint();
        }
        catch (Exception e)
        {
            m_sbData = new StringBuffer();
            String strError =m_sbData.append( "Error building file: " ).
                                  append( toolbarPath).toString();
            Messages.writeStackTrace(e, strError);
            Messages.postError(strError);
        }
    }

    public void resetTools()
    {
        m_sbData = new StringBuffer();
		String xmlPath = FileUtil.openPath(m_sbData.append("USER/PERSISTENCE/").
                                           append(m_strMkchangesfile).toString());
        try
        {
            if (xmlPath != null /*&& !m_bMvToTrashCan*/)
            {
				removeAllTools();
				LayoutBuilder.build(m_pnlDisplay, m_objExpIF, xmlPath);
                toobarLayout.saveLocations(m_pnlDisplay);
            }
        }
        catch (Exception e)
        {
            m_sbData = new StringBuffer();
            String strError =m_sbData.append( "Error building file: " ).
                                  append( xmlPath).toString();
            Messages.writeStackTrace(e, strError);
            Messages.postError(strError);
        }

       /** Remove all tools which are not in mark list **/
       /*if( m_nNewtoolDnD - m_nMarkDnDMenu != 0 )
          removeTools( m_nNewtoolDnD-m_nMarkDnDMenu );*/
    }

    public void resetToDefaultTools()
    {
         if( m_objToolsEditor == null || m_objToolsEditor.getRmNewTool() == false)
         {
	    removeAllTools();
			//removeAllVpTools();
            m_sbData = new StringBuffer();
            String toolbarPath = FileUtil.openPath(m_sbData.append("INTERFACE/").
                                     append(m_strToolbarfile).toString());
            m_sbData = new StringBuffer();
	    String vptoolbarPath = FileUtil.openPath(m_sbData.append("INTERFACE/").
                                     append(m_strVpfile).toString());
            try
            {
                LayoutBuilder.build(m_pnlDisplay, m_objExpIF, toolbarPath);
                toobarLayout.saveLocations(m_pnlDisplay);
            }
            catch (Exception e)
            {
                m_sbData = new StringBuffer();
			    String strError =m_sbData.append( "Error building file: " ).
                                      append( toolbarPath).toString();
			    Messages.writeStackTrace(e, strError);
			    Messages.postError(strError);
            }

            m_bIsDefaultTools = false;
            vpPanel.setVisible(true);
            validateToolbar();
            updateAllValue(m_pnlDisplay);
            updateAllValue(vpPanel);
         }
    }

	public void resetToVpTools()
	{
	    if(m_objToolsEditor == null || !m_objToolsEditor.getRmNewTool())
	   {
                m_sbData = new StringBuffer();
		    String strVpPath = FileUtil.openPath(m_sbData.append("INTERFACE/").append(m_strVpfile).toString());
	        try
	        {
                    if (strVpPath != null)
                    {
                        removeAllVpTools();
                        LayoutBuilder.build(vpPanel, m_objExpIF, strVpPath);
                        setVpToolAttr();
                    }
	        }
	        catch (Exception e) {
	 	    e.printStackTrace();
	        }

	        m_bIsDefaultTools = false;
                ActionHandler vpactionHandler = Util.getVpActionHandler();
                if (vpactionHandler != null)
                {
                    int nvpId = Util.getViewArea().getActiveWindow();
                    vpactionHandler.fireAction(new ActionEvent(this, 0, String.valueOf(nvpId+1)));
                }
                //validateToolbar();
                m_objToolbar.validate();
                m_objToolbar.repaint();
                updateAllValue(m_pnlDisplay);
                updateAllValue(vpPanel);
	    }
	}

    public Vector getCurrentToolList()
    {
        return  m_vecCurrentOrderedPos;
    }

    public void resetToolsInfo( JComponent comp )
    {
        // m_reset is the previously selected button, reset the border on the previous selected button
        if (m_resetComp != null) {
            if (m_resetComp instanceof VToolBarButton)
                ((VToolBarButton)m_resetComp).setBtn();
        }

        if (!(comp instanceof VToolBarButton))
            return;
        // comp is the current button.
        VToolBarButton btn = ( VToolBarButton ) comp;
        m_objToolsEditor.showCommand(btn.getAttribute(VObjDef.CMD));
        m_objToolsEditor.showSetCommand(btn.getAttribute(VObjDef.SET_VC));
        AddToolsDialog.resetTooltip( btn.getAttribute(VObjDef.TOOL_TIP));
        m_objToolsEditor.keepTrackDataFields(false);

        /* Update image area in add tools dialog */
        String strLabel = btn.getAttribute(VObjDef.ICON);
        if (m_objToolsEditor.selectImageInList(strLabel)) {
            // m_objToolsEditor.clearLabelField();
            m_objToolsEditor.showLabel(strLabel);
        }
        else
        {
            m_objToolsEditor.clearImageSelection();
            m_objToolsEditor.showLabel(strLabel);
        }

        m_nToolIndex = getIndex( btn );
        m_objToolsEditor.highlightTool( m_nToolIndex );
    }

    private int getIndex( VToolBarButton tool )
    {

       String compStr = (tool.getAttribute(VObjDef.ICON) == null) ? ""
                : tool.getAttribute(VObjDef.ICON);

       for( int i = 0; i < m_vecCurrentOrderedPos.size(); i++ )
        {
            String str = ( String ) m_vecCurrentOrderedPos.elementAt( i );

            m_sbData = new StringBuffer();
            if( str.equals(m_sbData.append(" " ).append( compStr ).toString()))
               return i;
         }

       return -1;
    }

    public void markCurrentChanges()
    {
        m_sbData = new StringBuffer();
        String mkchanges = FileUtil.savePath(m_sbData.append("USER/PERSISTENCE/").
                                             append(m_strMkchangesfile).toString());
        LayoutBuilder.writeToFile(m_pnlDisplay, mkchanges, true);
    }

    public void markCurrentComp()
    {
        m_mkComp = m_resetComp;
    }

    public int mkCurrentToolIndex()
    {
        return m_nToolIndex;
    }

	public void validateToolbar()
	{
	    m_pnlDisplay.validate();
		vpPanel.revalidate();
		m_objToolbar.validate();
		m_objToolbar.repaint();
	}

    public void resetToPreComp()
    {
        m_resetComp = m_mkComp;
    }


    //===============================================================
    //  Utility methods of xml files.
    //===============================================================


    public void rewriteDefaultXMLFile()
    {
        m_sbData = new StringBuffer();
        String toolbarPath = FileUtil.savePath(m_sbData.append("INTERFACE/").
                                append(m_strToolbarfile).toString());
        File objFile = new File(toolbarPath);
        LayoutBuilder.writeToFile(m_pnlDisplay, toolbarPath, true);
    }

    //===============================================================
    //  Utility methods for the add tools to tool bar.
    //===============================================================

    public void updateTooltip( String strTooltip )
    {
        VToolBarButton btn = (VToolBarButton) getSelectedComponent();
        if (btn != null)
            btn.setAttribute(VObjDef.TOOL_TIP, strTooltip);
    }

    public void updateCommand( String strNewCommand )
    {
       VToolBarButton btn = (VToolBarButton) getSelectedComponent();
       if (btn != null)
           btn.setAttribute(VObjDef.CMD, strNewCommand);
    }

    public void updateSetCommand(String strNewCommand)
    {
        VToolBarButton btn = (VToolBarButton) getSelectedComponent();
        if (btn != null)
            btn.setAttribute(VObjDef.SET_VC, strNewCommand);
    }

    public void setToolLabel( String strLabel )
    {
        VToolBarButton btn = (VToolBarButton) getSelectedComponent();
        if (btn != null)
            btn.setAttribute(VObjDef.ICON, strLabel);
    }

    public void setComponent(JComponent objComp)
    {
        m_resetComp = objComp;
        if (m_bInEditMode) {
            if (m_objToolsEditor != null && m_objToolsEditor.isVisible()) {
				m_objToolsEditor.toFront();
            }
        }
    }

    public int getHighlightIndex()
    {
    return m_nToolIndex;
    }

    public int getUndoIndex()
    {
    return m_nUndotoolIndex;
    }

    public void updateToolImg( String strImg )
    {
        if( m_resetComp != null )
        {
           VToolBarButton btn = ( VToolBarButton ) m_resetComp;
           btn.setAttribute(VObjDef.ICON, strImg);
           btn.setText( "" );
           btn.setName( strImg );

           // m_objToolsEditor.clearLabelField();
           m_objToolsEditor.showLabel(strImg);
           updateListTools(strImg);
        }
    }

    public void updateListTools( String imageFile )
    {
        VToolBarButton btn  = ( VToolBarButton ) m_resetComp;
        m_objToolsEditor.updateToolsOrder( m_vecCurrentOrderedPos );
        m_objToolsEditor.highlightTool( m_nToolIndex );
    }


    /*
     *  Save final changes before exiting the add tools dialog.
     */
    public void saveChanges()
    {
       // save the changes in the xml file.
       m_sbData = new StringBuffer();
       String toolbarPath = FileUtil.savePath(m_sbData.append("INTERFACE/").
                                              append(m_strToolbarfile).toString());
       LayoutBuilder.writeToFile(m_pnlDisplay, toolbarPath, true);
       m_nNewtoolDnD = 0;
	   //saveVpChanges();

       // this builds the xml file that was saved, so that everything is updated.
       resetToDefaultTools();

    }

	public void saveVpChanges()
	{
        saveChanges();
        m_sbData = new StringBuffer();
		String vpPath = FileUtil.savePath(m_sbData.append("INTERFACE/").
                                          append(m_strVpfile).toString());
		LayoutBuilder.writeToFile(vpPanel, vpPath, true);

		resetToVpTools();

	}

	/**
	 *  Adds the viewport buttons to the toolbar.
	 */
    public void addVP(ArrayList aListVP)
    {
        int nCompCount = vpPanel.getComponentCount();
        Component comp;
        VToolBarButton btnTool;
        ArrayList aListComp = new ArrayList();

        removeAllVpTools();

        int nVP = aListVP.size();
        String strLabel;
		int x = 0;
		int y = YPOS;
        int h = getRowHeight();
        if (h < 10)
			h = 25;
        if (y > h + YPOS)
			y = (y / h) * h + YPOS;
		int w = (m_resetComp == null || m_resetComp.getWidth() <= 0) ? 60
					: m_resetComp.getWidth();

        ArrayList aListBtns = getVpBtns(nVP);
        VToolBarButton btn;

        // Put the viewport buttons on the toolbar
        for (int i = 0; i < nVP; i++)
        {
            HashMap hmVpAttrs = (HashMap)aListVP.get(i);
			strLabel = (String)hmVpAttrs.get(new Integer(VObjDef.ICON));
			if (i > 0)
			    x = x + w;
            //addTools(strLabel, strLabel, x, y, vpPanel, "", "", false);
            btn = new VToolBarButton(m_objSShare, m_objExpIF, "toolBarButton");
            setComponent(btn);
			setAttrs(hmVpAttrs, btn);
            initToolInfo(btn, strLabel, vpPanel, btn.getAttribute(CMD),
                  "Viewport Selection", x, y, btn.getAttribute(CMD2));

            /*if (i < aListBtns.size())
            {
                btn = (VToolBarButton)aListBtns.get(i);
                btn.setAttribute(VObjDef.ICON, strLabel);
            }*/
        }
		validateToolbar();
    }

    protected ArrayList getVpBtns(int nVp)
    {
        int nCompCount = vpPanel.getComponentCount();
        ArrayList aListBtns = new ArrayList();
        Component comp;

        for (int i = 0; i < nCompCount; i++)
        {
            comp = vpPanel.getComponent(i);
            if (comp instanceof VToolBarButton)
            {
                if (aListBtns.size() < nVp)
                    aListBtns.add(comp);
                /*else
                    ((VToolBarButton)comp).setAttribute(VObjDef.SHOW, "$VALUE=-1");*/
            }
        }
        return aListBtns;
    }

    /**
     * Sets the viewport toolbar visible if any of the viewport buttons are visible.
     */
    public void setVpVisible()
    {
        boolean bShow = false;
        int nCompCount = vpPanel.getComponentCount();
        for (int i = 0; i < nCompCount; i++)
        {
            Component comp = vpPanel.getComponent(i);
            if (comp instanceof VToolBarButton && comp.isVisible())
            {
                bShow = true;
                break;
            }
        }
        vpPanel.setVisible(bShow);
    }

	/**
	 *  Sets the attributes for each toolbar button.
	 */
	protected void setAttrs(HashMap hmVpAttrs, VToolBarButton btn)
	{
		if (hmVpAttrs == null || hmVpAttrs.isEmpty())
			return;

	    Iterator iterator = hmVpAttrs.keySet().iterator();
		Integer key;
		int nAttr;
		String strValue;
		while (iterator.hasNext())
		{
		    key = (Integer)iterator.next();
			strValue = (String)hmVpAttrs.get(key);
			nAttr = key.intValue();
			btn.setAttribute(nAttr, strValue);
		}
	}



    /**********************************************************************************
     *       Utilities methods to handle changes in the tool bar editor.              *
     **********************************************************************************/

   /*
    *  To update a tool's information selected from current tool list in
    *  tool editor
    */
   public void setSelectedToolInfo(VToolBarButton btn)
   {
      if (btn != null)
      {
          btn.setSelected(true);
          String strLabel = (btn.getAttribute(VObjDef.ICON) == null) ? "" : btn.getAttribute(VObjDef.ICON);
          if (m_objToolsEditor.selectImageInList(strLabel)) {
             // m_objToolsEditor.clearLabelField();
             m_objToolsEditor.showLabel(strLabel);
          }
          else
          {
             m_objToolsEditor.showLabel(strLabel);
             m_objToolsEditor.clearImageSelection();
          }
          AddToolsDialog.resetTooltip(btn.getAttribute(VObjDef.TOOL_TIP));
          m_objToolsEditor.showCommand(btn.getAttribute(VObjDef.CMD));
          m_objToolsEditor.showSetCommand(btn.getAttribute(VObjDef.SET_VC));
      }
   }

   private void setToolsInfo()
    {
      String str = "";

      m_vecCurrentOrderedPos.removeAllElements();
      m_vecUndotoollist.removeAllElements();

      VToolBarButton btn = (VToolBarButton)m_resetComp;
      String strName = (btn.getAttribute(VObjDef.ICON) == null) ? ""
            : btn.getAttribute(VObjDef.ICON);
      m_objToolsEditor.selectImageInList(strName);
   }

   public boolean getDnDNewToolStatus()
   {
      m_bIsnewtool = true;
      return m_bIsnewtool;
   }

   public void deleteTmpFiles()
   {
      m_sbData = new StringBuffer();
      String delfile = FileUtil.openPath(m_sbData.append("USER/PERSISTENCE/").
                                         append(m_strTmpfile1).toString());
      if( delfile != null )
     clearFilePath( delfile );

      m_sbData = new StringBuffer();
      delfile = FileUtil.openPath(m_sbData.append("USER/PERSISTENCE/").
                                  append(m_strTmpfile2).toString());
      if( delfile != null )
     clearFilePath( delfile );

      m_sbData = new StringBuffer();
      delfile = FileUtil.openPath(m_sbData.append("USER/PERSISTENCE/").
                                  append(m_strMkchangesfile).toString());
      if( delfile != null )
     clearFilePath( delfile );
   }

   private void clearFilePath( String delfile )
   {
      File tmpfile = new File( delfile );
      if( tmpfile.exists() )
      tmpfile.delete();
   }

   public void clearFields()
   {
      m_objToolsEditor.clearLabelField();
      m_objToolsEditor.clearImageSelection();
      m_objToolsEditor.showCommand( "" );
      m_objToolsEditor.resetTooltip( "" );
      m_objToolsEditor.showSetCommand("");
   }

   public boolean isInToolsList( String toolLabel )
   {
      boolean isChanged = false;
      String label = "";

      for( int i = 0; i < m_vecCurrentOrderedPos.size(); i++ )
      {
         label = ( String ) m_vecCurrentOrderedPos.elementAt( i );

         m_sbData = new StringBuffer();
         if(label.equals(m_sbData.append(" ").append( toolLabel ).toString()))
           isChanged = true;
      }

      return isChanged;
   }

   private String getToolLabel( int loc )
   {
      String gotName = "";

      for( int i = 0; i < m_hmTools.size(); i++ )
      {
         String str = ( String ) m_vecToolsLoc.get( i );
         StringTokenizer token = new StringTokenizer( str );
         String name = token.nextToken();

         str = token.nextToken();
         int num = Integer.parseInt( str );

         if( loc == num ) {
             gotName = name;
             break;
         }
      }

      return gotName;
   }

   public Vector<String> getDefaultTools()
   {
       return m_vecDefaultTools;
   }

   private void sortTools()
   {
      int count = m_vecPositionV.size();
      int incr = count / 2;

      while( incr >= 1 )
      {
         for( int i = incr; i < count; i++ )
     {
        String pos = ( String ) m_vecPositionV.get( i );
        int temp = Integer.parseInt( pos );
        int j = i;

        while( j >= incr &&
           temp < Integer.parseInt( ( String ) m_vecPositionV.get( j-incr ) ))
        {
            String position = ( String ) m_vecPositionV.get( j-incr );

        String pos_j1 = ( String ) m_vecPositionV.get( j );
        String pos_j2 = ( String ) m_vecPositionV.get( j-incr );
        m_vecPositionV.set( j,pos_j2 );
        j -= incr;
         }
         m_vecPositionV.set( j,pos );
      }
      incr /= 2;
       }
   }

   public boolean isDnDWithinToolBar()
   {
    if (m_resetComp != null && m_resetComp instanceof VToolBarButton)
        return ((VToolBarButton)m_resetComp).getDnDWithinToolBar();
    return false;
   }

   public JComponent getSelectedComponent()
   {
        if (m_resetComp != null && (m_resetComp instanceof VToolBarButton))
            return m_resetComp;
        else
            return null;
   }

   public AddToolsDialog getToolsEditor()
   {
    return m_objToolsEditor;
   }

   public void processVnmrCmd( String cmd )
   {
       StringTokenizer strTok = new StringTokenizer(cmd, " ,\n");
       String strhelpfile = "";
       while (strTok.hasMoreTokens())
       {
           strhelpfile = strTok.nextToken();
           if (strhelpfile.startsWith("help:"))
           {
               strhelpfile = strhelpfile.substring(5);
               break;
           }
        }
       if( m_objToolsEditor == null )
       {
           m_objToolsEditor = new AddToolsDialog( m_objToolbar, m_objExpIF, strhelpfile );
       }
       else if( m_objToolsEditor.getToolEditStatus() == false )
       {
           m_objToolsEditor = new AddToolsDialog( m_objToolbar, m_objExpIF, strhelpfile );
           m_objToolsEditor.setToolEditStatus( true );
       }
       if (isShowing()) {
           Point loc = getLocationOnScreen();
           m_objToolsEditor.setLocation(loc.x + 100, loc.y + 60);
       }
       m_objToolsEditor.updateAllValue();
       m_objToolsEditor.setVisible( true );
   }

    public void setEditMode(boolean s) {
        if (!s && m_bInEditMode)
	{
            toobarLayout.saveLocations(m_pnlDisplay);
	    m_pnlDisplay.validate();
	}
        if (toobarLayout != null)
           toobarLayout.setEditMode(s);
        m_bInEditMode = s;
        m_pnlDisplay.repaint();
    }

	public void setVpEditMode(boolean s)
	{
	    if (!s && m_bVpEditMode)
		{
		    toobarLayout.saveLocations(m_pnlDisplay);
		}
		m_bVpEditMode = s;
	}

	public boolean isEditing() {
		return m_bInEditMode;
    }

	public boolean isVpEditing()
	{
	    return m_bVpEditMode;
	}

    public void disposeToolEditor()
    {
        deleteTmpFiles();
        m_objToolsEditor.dispose();
    }

   public void updateToolList( String newLabel )
   {
      m_sbData = new StringBuffer();
      m_vecCurrentOrderedPos.setElementAt(m_sbData.append( " " ).
                                          append( newLabel).toString(), m_nToolIndex );
      m_objToolsEditor.updateToolsOrder( m_vecCurrentOrderedPos );
      updateToolsHashTable( newLabel );
   }

   private boolean cmpToolsLabel( String comparedLabel )
   {
    String label = "";
    boolean isupdated = true;

    for( int i = 0; i < m_vecCurrentOrderedPos.size(); i++ )
    {
       label = ( String ) m_vecCurrentOrderedPos.elementAt( i );

       if( comparedLabel.equals( label ))
           isupdated = false;
    }

    return isupdated;
   }

   private void updateToolsHashTable( String newToolLabel )
   {
      String name = "";
      String value = "";
      String resetValue = "";

      for( int i = 0; i < m_vecAvailToolsV.size(); i++ )
      {
        name = ( String ) m_vecAvailToolsV.get( i );
        m_sbData = new StringBuffer();
        value = m_sbData.append(" ").append(( String )m_hmTools.get( name)).toString();

        if( cmpToolsLabel( value ))
           resetValue = name;
      }

      m_hmTools.put( resetValue, newToolLabel );
   }

   private void sort()
   {
      int count = m_vecSortedPos.size();
      int incr = count / 2;

      while( incr >= 1 )
      {
         for( int i = incr; i < count; i++ )
     {
        Point pt = ( Point ) m_vecSortedPos.get( i );
        int temp = pt.x;
        int j = i;

        while( j >= incr && temp < (( Point ) m_vecSortedPos.get( j-incr )).x )
        {

           Point point = ( Point ) m_vecSortedPos.get( j-incr );

           Point pt_j1 = ( Point ) m_vecSortedPos.get( j );
           Point pt_j2 = ( Point ) m_vecSortedPos.get( j-incr );
           m_vecSortedPos.set( j,pt_j2 );
           j -= incr;
        }
        m_vecSortedPos.set( j,pt );
      }
      incr /= 2;
       }
    }

    public void setNewToolInfo( String label, String command)
    {
        if (m_bIsnewtool)
    {
        String str = "";
            String orgLabel = "";
            VToolBarButton tool = ( VToolBarButton ) m_resetComp;
        if (tool == null) return;

            if( tool.getName() != null && !(tool.getName().equals( "" )))
            {
        tool.setIcon( null );
        //m_bIsnewtool = false;
        //m_nToolIndex = getToolIndex( tool );
        orgLabel = tool.getName();
        tool.setName( "" );
            }

            tool.setAttribute(VObjDef.ICON, label);
            tool.setAttribute(VObjDef.TOOL_TIP, label);
            tool.setAttribute(VObjDef.CMD, command);
        tool.setAttribute(VObjDef.SET_VC, "");
            m_objToolsEditor.clearImageSelection();

            /* Update hash table key-value */
            String value = "";
            String key   = "";
            for( int i = 0; i < m_vecAvailToolsV.size(); i++ )
            {
        key = ( String ) m_vecAvailToolsV.get( i );
        value = ( String )m_hmTools.get( key );

        if( value.equals( "New Tool" ) )
            m_hmTools.put( key, label );
        else if( value.equals( orgLabel ))
            m_hmTools.put( key, label );
            }

            m_objToolsEditor.resetTooltip( label );
            m_objToolsEditor.showLabel( label );
            m_objToolsEditor.showCommand( command );
        m_objToolsEditor.showSetCommand("");
            m_objToolsEditor.setResetComp(( JComponent ) tool );
    }
   }

    //==============================================================================
    //                   Methods to handle undo.
    //==============================================================================

    public void undoTool( String label )
    {
        m_nNewtoolDnD++;
        addTools( label, label, m_nDndPos, m_nDndPosY, m_pnlDisplay, "", "", false);
        /*m_objToolbar.validate();
        m_objToolbar.repaint();*/
		validateToolbar();
    }

    public void undoToolPos( JComponent undoComp )
    {
        Point pt = undoComp.getLocation();
        VToolBarButton btn = ( VToolBarButton ) undoComp;
        String strLabel =  btn.getAttribute(VObjDef.ICON);
        if (strLabel == null)
            strLabel = "";

        if( pt.x != btn.getPreviousPos() )
        {
			undoComp.setLocation( btn.getPreviousPos(), YPOS );
			m_objToolsEditor.updateToolsOrder( m_vecUndoPosList );
			m_objToolsEditor.selectImageInList(strLabel);
			btn.setPreviousPos(pt.x);
        }
        else
        {
			m_objToolsEditor.updateToolsOrder( m_vecCurrentOrderedPos );
        }
    }

    public Vector<String> getUndoTools()
    {
       return m_vecUndotoollist;
    }

    /** Delete a tool by dragging and dropping to the trash can. **/
    public void deleteTool( JComponent tool )
    {
        m_pnlDisplay.remove( tool );
        /*m_objToolbar.validate();
        m_objToolbar.repaint();*/
		validateToolbar();

        recordCurrentToolBar( m_strTmpfile2 );
        removeAllTools();
        clearFields();

        /* Re-assign tools */
        m_sbData = new StringBuffer();
        String userpath = FileUtil.openPath(m_sbData.append("USER/PERSISTENCE/").
                                            append(m_strTmpfile2).toString());
        File path = new File( userpath );

        if( !path.exists() )
		    return;
        else
        {
            try
            {
				LayoutBuilder.build(m_pnlDisplay, m_objExpIF, userpath);
            }
			catch (Exception e)
            {
                m_sbData = new StringBuffer();
				String strError =m_sbData.append( "Error writing persistence file " ).
                                      append( userpath).toString();
                Messages.writeStackTrace(e, strError);
				Messages.postError(strError);
			}

            /*m_objToolbar.validate();
            m_objToolbar.repaint();*/
			validateToolbar();
            m_bIsDefaultTools = false;
        }

        m_objToolsEditor.setDelToolFlag( true );
        m_objToolsEditor.showCommand("");
        m_objToolsEditor.showLabel("");
        m_objToolsEditor.showSetCommand("");
        m_objToolsEditor.resetTooltip("");
        m_bMvToTrashCan = true;
    }

    public void removeNewTool()
    {
        if (m_resetComp instanceof VToolBarButton)
        {
            m_pnlDisplay.remove(m_resetComp);
            /*m_objToolbar.validate();
            m_objToolbar.repaint();*/
			validateToolbar();
        }
    }

    public void taggleDelete( String filepath, int init )
    {
        String str = "";
        removeAllTools();

        m_sbData = new StringBuffer();
        String toolbarPath = FileUtil.openPath(m_sbData.append("USER/PERSISTENCE/").
                                               append(filepath).toString());
        if(toolbarPath != null)
        {
            try
            {
                LayoutBuilder.build(m_pnlDisplay, m_objExpIF, toolbarPath);
            }
            catch (Exception e)
            {
                m_sbData = new StringBuffer();
				String strError =m_sbData.append( "Error building file " ).
                                      append( toolbarPath).toString();
                Messages.writeStackTrace(e, strError);
				Messages.postError(strError);
            }
        }
        /*m_objToolbar.validate();
        m_objToolbar.repaint();*/
		validateToolbar();

        if( init == 1 )
        {
			m_vecDeltoollist.removeAllElements();
			for( int i = 0; i < m_vecCurrentOrderedPos.size(); i++ )
			{
				str = ( String ) m_vecCurrentOrderedPos.elementAt( i );
				m_vecDeltoollist.addElement( str );
			}
		}

        if( init == 2 )
        {
			m_vecUndolist.removeAllElements();
			for( int j = 0; j < m_vecCurrentOrderedPos.size(); j++ )
			{
				str = ( String ) m_vecCurrentOrderedPos.elementAt( j );
				m_vecUndolist.addElement( str );
			}
        }

        if( init % 2 == 0 )
			m_objToolsEditor.updateToolsOrder( m_vecUndolist );
        else
			m_objToolsEditor.updateToolsOrder( m_vecDeltoollist );

        m_bMvToTrashCan = true;
    }

    private void removeAllTools()
    {
        int nBtnCount = m_pnlDisplay.getComponentCount();
        while(m_pnlDisplay.getComponentCount() > 0)
        {
            Component objBtn = m_pnlDisplay.getComponent(0);
		    if (objBtn instanceof VObjIF)
                m_pnlDisplay.remove(objBtn);
        }
        m_objToolbar.validate();
        m_objToolbar.repaint();
        m_nIndex = 0;
        m_nToolIndex = -1;
        m_nNewtoolDnD = 0;
        //m_nMarkDnDMenu = 0;
        m_bIsDefaultTools = true;

        m_vecAvailToolsV.clear();
        m_vecPositionV.clear();
        m_vecDefaultTools.clear();
        m_vecCurrentOrderedPos.removeAllElements();
        m_hmTools.clear();
    }


    public void setToSystemInitial( boolean rectool )
    {
        // Save current tools on the tool bar for undo purpose.
        if( rectool )
		    recordCurrentToolBar( m_strTmpfile1 );
		removeAllTools();
        m_bMvToTrashCan = true;

        m_sbData = new StringBuffer();
        String sysPath = FileUtil.openPath(m_sbData.append("SYSTEM/INTERFACE/").
                                           append(m_strToolbarfile).toString());

        if( sysPath != null )
        {
			try
			{
				LayoutBuilder.build(m_pnlDisplay, m_objExpIF, sysPath);
                toobarLayout.saveLocations(m_pnlDisplay);
			}
			catch (Exception e)
			{
                m_sbData = new StringBuffer();
				String strError =m_sbData.append( "Error building file: " ).
                                      append( sysPath).toString();
				Messages.writeStackTrace(e, strError);
				Messages.postError(strError);
			}
			/*m_objToolbar.validate();
			m_objToolbar.repaint();*/
			validateToolbar();
			m_bIsDefaultTools = false;
		}
    }

    /* Get undo tool index */
   private int getHLIndex( Vector list, String label )
   {
      String str = "";

      for( int i = 0; i < list.size(); i++ )
      {
     str = ( String ) list.elementAt( i );
     m_sbData = new StringBuffer();
     if(str.equals(m_sbData.append(" " ).append( label ).toString()))
        return i;
      }

      return -1;
   }

   public void recordCurrentToolBar( String tmpDir )
   {
      m_sbData = new StringBuffer();
      String tmpPath = FileUtil.savePath(m_sbData.append("USER/PERSISTENCE/").
                                         append( tmpDir).toString());
      File objFile = new File(tmpPath);
      LayoutBuilder.writeToFile(m_pnlDisplay, tmpPath, true);
   }

   public void undoSystemInitial()
   {
      removeAllTools();

      m_sbData = new StringBuffer();
      String toolbarPath = FileUtil.openPath(m_sbData.append("USER/PERSISTENCE/").
                                             append(m_strTmpfile1).toString());
      try
      {
          LayoutBuilder.build(m_pnlDisplay, m_objExpIF, toolbarPath);
      }
      catch (Exception e)
      {
          m_sbData = new StringBuffer();
		  String strError =m_sbData.append( "Error building file: " ).
                                append( toolbarPath).toString();
          Messages.writeStackTrace(e, strError);
          Messages.postError(strError);
      }
      /*m_objToolbar.validate();
      m_objToolbar.repaint();*/
	  validateToolbar();
      m_bIsDefaultTools = false;
   }

   //==============================================================================
   //   DragGestureListener implementation follows ...
   //============================================================================
    public void dragGestureRecognized( DragGestureEvent e )
    {
       /*recordCurrentToolBar( m_strTmpfile1 );
       String str = "";
       // record current tools positions before dnd.
       m_vecUndoPosList.removeAllElements();
       for( int i = 0; i < m_vecCurrentOrderedPos.size(); i++ )
       {
          str = ( String ) m_vecCurrentOrderedPos.elementAt( i );
          m_vecUndoPosList.addElement( str );
       }
       m_nUndotoolIndex = m_nToolIndex;

       if(( m_objToolsEditor != null ) && m_objToolsEditor.getToolEditStatus() )
       {
          Component comp = e.getComponent();

          LocalRefSelection ref = new LocalRefSelection( comp );
          m_objDragSource.startDrag( e, DragSource.DefaultMoveDrop, ref, this );
       }*/
    }

    //=================================================================
    //   DragSourceListener implementation follows ...
    //=================================================================

    public void dragDropEnd( DragSourceDropEvent e ) {}
    public void dragEnter( DragSourceDragEvent e ) {}
    public void dragExit( DragSourceEvent e ) {}
    public void dragOver( DragSourceDragEvent e ) {}
    public void dropActionChanged( DragSourceDragEvent e ) {}

    //==================================================================
    //   DropTargetListener implementation Follows ...
    //==================================================================

    public void dragEnter( DropTargetDragEvent e ) {}
    public void dragExit( DropTargetEvent e ) {}
    public void dragOver( DropTargetDragEvent e ) {}
    public void dropActionChanged( DropTargetDragEvent e ) {}

    public void drop( DropTargetDropEvent e )
    {
        String dropLabel = "";
        Point pt = e.getLocation();

        if (m_objToolsEditor == null)
            return;
        if( m_objToolsEditor.isNewToolDragged() )
        {
            try
            {
                Transferable tr = e.getTransferable();
                if( tr.isDataFlavorSupported( LocalRefSelection.LOCALREF_FLAVOR ))
                {
                    Object obj = tr.getTransferData( LocalRefSelection.LOCALREF_FLAVOR );
                    JComponent comp = ( JComponent ) obj;
                    // reset the border of the previous selection
                    if (m_resetComp instanceof VToolBarButton)
                        ((VToolBarButton)m_resetComp).setBtn();

                    // set m_resetComp to the current component.
                    m_resetComp = comp;

                    dropLabel = "New Tool";
                    m_objToolsEditor.showLabel( dropLabel );
                    m_objToolsEditor.resetTooltip(dropLabel);
                    m_objToolsEditor.showCommand("");
                    m_objToolsEditor.showSetCommand("");

                    e.acceptDrop( DnDConstants.ACTION_MOVE );
                    e.getDropTargetContext().dropComplete( true );

                    m_nNewtoolDnD++;
                    m_nDndPos = pt.x;

                    m_bIsnewtool = true;
                    m_objToolsEditor.setResetComp( comp );
                    m_objToolsEditor.keepTrackDataFields( true );
                    int y = YPOS;
                    int h = getRowHeight();
                    if (h < 10)
                        h = 25;
                    if (pt.y > h + YPOS) {
                        y = (pt.y / h) * h + YPOS;
                    }
                    m_nDndPosY = y;

                    addTools( dropLabel, dropLabel, pt.x, y, m_pnlDisplay, "", "", false );
                    m_objToolsEditor.unableNewToolDrag();
                    validate();
                    m_objToolsEditor.clearImageSelection();
                    setToolsInfo();
                }
                return;
            }
            catch( IOException io )
            {
                Messages.writeStackTrace(io);
                e.rejectDrop();
            }
            catch( UnsupportedFlavorException ufe )
            {
                Messages.writeStackTrace(ufe);
                e.rejectDrop();
            }
        }
        else   /** DnD within the tool bar **/
        {
            try
            {
                Transferable tr = e.getTransferable();
                if( tr.isDataFlavorSupported( LocalRefSelection.LOCALREF_FLAVOR ))
                {
                    Object obj = tr.getTransferData( LocalRefSelection.LOCALREF_FLAVOR );
                    JComponent comp = ( JComponent ) obj;
                    // reset the border of the previous selection
                    if (m_resetComp instanceof VToolBarButton)
                        ((VToolBarButton)m_resetComp).setBtn();

                    // set m_resetComp to the current component.
                    m_resetComp = comp;

                    VToolBarButton btn = null;
                    String strSubtype = null;
                    Point initpos = m_resetComp.getLocation();
                    int y = YPOS;
                    int h = getRowHeight();
                    if (h < 10)
                        h = 25;
                    if (pt.y > h + YPOS) {
                        y = (pt.y / h) * h + YPOS;
                    }
                    comp.setLocation( pt.x, y );

                    if (comp instanceof VToolBarButton)
                        btn = (VToolBarButton) comp;
                    if (btn != null) {
                        strSubtype = btn.getAttribute(SUBTYPE);
                        btn.setDefLoc( pt.x, y );
                    }
                    if (strSubtype == null || strSubtype.indexOf(Global.VIEWPORT) < 0)
                    {
                        if (btn != null)
                            btn.setPreviousPos(initpos.x);

                        m_objToolsEditor.keepTrackDataFields( false );
                        m_objToolsEditor.setResetComp( comp );

                        e.acceptDrop(DnDConstants.ACTION_MOVE);
                        e.getDropTargetContext().dropComplete( true );
                        String strLabel = "";
                        if( btn != null )
                        {
                            if (btn.getAttribute(VObjDef.ICON) != null)
                                strLabel = btn.getAttribute(VObjDef.ICON);
                            btn.setBorder(m_lineBorder);
                            btn.setDnDWithinToolBar(true);
                        }
                        if (m_objToolsEditor.isIconName(strLabel)) {
                            m_objToolsEditor.clearLabelField();
                            m_objToolsEditor.clearImageSelection();
                        }
                        else /* for text buttons */
                        {
                            m_objToolsEditor.showLabel( strLabel );
                        }
                        m_bIsnewtool = false;
                        m_objToolsEditor.keepTrackDataFields( false );
                        setToolsInfo();
                    }
                }
            }
            catch( IOException io )
            {
                Messages.writeStackTrace(io);
                e.rejectDrop();
            }
            catch( UnsupportedFlavorException ufe )
            {
                Messages.writeStackTrace(ufe);
                e.rejectDrop();
            }
        }
    }

   private void addTools(String strLabel, String strToolTip, int nXpos,
							int nYpos, JComponent compDisplay, String strVc,
							String strSetVC, boolean bIsImage)
   {
        m_nIndex++;
        VToolBarButton objNewTool;
        objNewTool = new VToolBarButton(m_objSShare, m_objExpIF, "toolBarButton");
        initToolInfo(objNewTool, strLabel, compDisplay, strVc, strToolTip, nXpos, nYpos, strSetVC);
        setComponent(objNewTool);
    }

    private void initToolInfo(VToolBarButton objNewTool, String strLabel, JComponent compDisplay,
								String strVC, String strToolTip, int x, int y, String strSetVC)
    {
		objNewTool.setAttribute(VObjDef.ICON, strLabel);
        objNewTool.setLocation(x, y);
		objNewTool.setBounds(x, y, 50, 25);
	objNewTool.setDefLoc(x, y);
        objNewTool.setAttribute(VObjDef.BGCOLOR, null);
        objNewTool.setAttribute(VObjDef.TOOL_TIP, strToolTip);
        objNewTool.setAttribute(VObjDef.CMD, strVC);
        objNewTool.setAttribute(VObjDef.SET_VC, strSetVC);
        objNewTool.setDnDWithinToolBar(false);   // DnD within tool bar.
        // Set the previous selected button to normal border.
        if (m_resetComp != null && m_resetComp instanceof VToolBarButton)
            ((VToolBarButton)m_resetComp).setBtn();
        // Set the border of the current button as selected.
        objNewTool.setBorder(m_lineBorder);

        compDisplay.add(objNewTool);
    }

     private void setVpNum(String s) {
            String d = s.trim();
            int  num = 0;
            if (d.length() > 0) {
               try {
                   num = Integer.parseInt(d);
               }
               catch (NumberFormatException er) { return; }
            }
            if (num < 1)
                return;
            ExpViewArea ev = Util.getViewArea();
            if (ev != null) 
               ((ExpViewIF)ev).setCanvasNum(num); 
     }

/*
class VToolBar extends JToolBar
{
	public void setMargin(Insets m)
	{
		super.setMargin(new Insets(0, 0, 0, 0));
	}
}
*/

     private class VpObj extends SmsInfoObj {
        public VpObj(String str, int n) {
            super(str, n);
        }

        public void setValue(ParamIF pf) {
           if (pf != null && pf.value != null) {
                setVpNum(pf.value);
                bUpdateVpObj = false;
           }
        }
    }

    private class TPanel extends JPanel {
         public void paint(Graphics g) {
             super.paint(g);
             if (m_bInEditMode) {
                  Dimension dim = getSize();
                  g.setColor(hiliColor);
                  g.drawRect(0, 0, dim.width - 1, dim.height - 1);
                  g.drawRect(1, 1, dim.width - 3, dim.height - 3);
             }
         }
    }

    private class VpPanLayout implements LayoutManager {
        private int  minW = 40;

        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        public Dimension preferredLayoutSize(Container target) {
           synchronized (target.getTreeLock()) {
                int w = 0;
                int h = 10;
                int i;
                Dimension d;
                int nmembers = target.getComponentCount();
                Component comp;
                
                for (i = 0; i < nmembers; i++) {
                    comp = target.getComponent(i);
                    if (comp != null && comp.isVisible()) {
                        d = comp.getPreferredSize();
                        if (d.width < minW)
                            d.width = minW;
                        w = w + d.width + 16;
                        if (d.height > h)
                            h = d.height;
                    }
                }
                return new Dimension(w, h+2);
            }
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Insets insets = target.getInsets();
                int cw = target.getSize().width - insets.left - insets.right;
                int ch = target.getSize().height - insets.top - insets.bottom;
                int count = target.getComponentCount();
                Dimension d;
                int x = 1;
                int y = 1;
                ch -= 2;
                if (ch > 30)
                    ch = 30;
                for (int i = 0; i < count; i++) {
                    Component comp = target.getComponent(i);
                    if (comp != null && comp.isVisible()) {
                        d = comp.getPreferredSize();
                        if (d.width < minW)
                            d.width = minW;
                        d.width = d.width + 12;
                        comp.setBounds( x, y, d.width, ch);
                        comp.validate();
                        x = x + d.width + 4;
                    }
                }
            }
        }
    }

    private class toolLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {
        }

        public void removeLayoutComponent(Component comp) {
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public Dimension preferredLayoutSize(Container target) {
             Dimension dim = m_pnlDisplay.getPreferredSize();
             if (dim == null)
                 dim = new Dimension(20, 24);
             if (dim.height < 24)
                 dim.height = 24;
             dim.width = dim.width + 12;
             return dim;
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                  Dimension dim = target.getSize();
                  Insets insets = target.getInsets();
                  int dw = dim.width;
                  int x = dim.width;
                  int y = 0;
                  int x0 = insets.left;

                  if (dim.width < 10 || dim.height < 12)
                      return;
                  ctrlButton.setBounds(x0, 0, 8, dim.height);
                  x0 += 12;
                  m_pnlDisplay.setBounds(x0, 0, dim.width - x0, dim.height);
            }
        }
    }
}

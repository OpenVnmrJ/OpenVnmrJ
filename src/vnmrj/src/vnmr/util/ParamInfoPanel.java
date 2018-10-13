/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;
import static vnmr.bo.EasyJTable.ACTION_SCROLL_TO_BOTTOM;
import static vnmr.bo.EasyJTable.REASON_ANY;
import static vnmr.bo.EasyJTable.REASON_INITIAL_VIEW;
import static vnmr.bo.EasyJTable.STATE_ANY;
import static vnmr.bo.EasyJTable.STATE_BOTTOM_SHOWS;

import java.awt.*;
import java.awt.event.*;

import javax.swing.*;
import javax.swing.border.*;
import javax.swing.filechooser.*;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableCellRenderer;

import java.util.*;
import java.beans.*;
import java.io.File;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.templates.*;
public class ParamInfoPanel extends ModelessDialog
    implements  VObjDef, ActionListener, PropertyChangeListener, Types
{
    private static final long serialVersionUID = 1L;
    private String itemName="";
    private String panelEditorGeometryFile="PanelEditorGeometry";
    private String panelEditorPersistenceFile="PanelEditor";

    private boolean initialized=false;
    private ArrayList<VObjIF> edits=new ArrayList<VObjIF>();

    private ParamPanel editPanel;
    private VObjIF editVobj=null;
    private JLabel lbs[];
    private VUndoableText txts[];
    private JPanel pans[];
    private JLabel vlbs[];
    private JComponent vwidgs[];
    private JPanel vpans[];
    private boolean vwidgIsSet[];
    private JPanel panel=null;
    private JPanel headerPan=null;
    private JPanel prefPan=null;
    private JPanel ctlPan=null;
    private JPanel attrPan=null;
    private JScrollPane attrScroll=null;
    private JPanel editablePan=null;
    private VUndoableText snapSize=null;
    private VUndoableText locX=null;
    private VUndoableText locY=null;
    private VUndoableText locW=null;
    private VUndoableText locH=null;
    private JLabel typeOfItem=null;
    private JLabel itemSource=null;
    private JLabel itemLabel=null;
    private JLabel InLabel=null;
    private VUndoableText itemEntry;
    private JComboBox saveDirectory;
    private boolean changed_page = false;
 
    private JComboBox typeList;
    private JButton saveButton=null;
    private JButton reloadButton=null;
    private JButton clearButton=null;
    private JButton revertButton=null;

    private JRadioButton editableOn;
    private JRadioButton editableOff;
    private RadioGroup tabgrp=null;
    private JCheckBox autoscroll=null;
    
    private JRadioButton autosaveButton;
    private JRadioButton warnsaveButton;
    private JRadioButton usersaveButton;
    
    private JCheckBox edit_tools=null;
    private JCheckBox edit_panels=null;
    private JCheckBox edit_status=null;
    private JCheckBox edit_actions=null;


    private String panelName = null;
    private String panelType = "generic";
    private String fileName = null;
    private StyleChooser textStyle=null;
    private int attrWidth=26;

    private final int  ILABEL = 0;   // label
    private final int  VAR = 1;      // VNMR variables
    private final int  IVAL = 2;     // value for this item
    private final int  SHOWX = 3;    // show condition
    private final int  CMD_1 = 4;    // cmd for entry, choice, button,radio, check
    private final int  CMD_2 = 5;    // cmd for reset check or toggle
    private final int  CHOICE = 6;   // choice labels
    private final int  CHVALS = 7;   // choice values
    private final int  DIGITS = 8;   // number of digits
    private final int  STATVARS = 9; // status variables
    private final int  numItems = 10;

    private int selectedType = ParamInfo.PARAMLAYOUT;
    private final int  SAVE_IN_LAYOUT   = 0;

    private final int  AUTO_SAVE  = 2;
    private final int  WARN_SAVE  = 1;
    private final int  USER_SAVE  = 0;
    
    private SaveWarnDialog warnDialog=null;
    
    private int save_policy=USER_SAVE;

    private boolean isNewType = true;
    private int lastSaveIndex = 0;
    private boolean inAddMode=true;

    private int textLen = 20;
    private int windowWidth = 720;
    private int scrollH = 30;
    private int etcH = 130; // the panelHeight - scrollH
    private int sbW = 0;
    private boolean inSetMode = false;
    private SessionShare sshare;
    private AppIF appIf;
    private Font font = DisplayOptions.getFont("Dialog", Font.PLAIN, 12);
    private AttributeChangeListener attributeChangeListener;
    private String openpath=null;
    private String savepath=null;
    private ItemEditor editor=null;
    private SimpleH2Layout xLayout = new SimpleH2Layout(SimpleH2Layout.LEFT,
                                                       4, 0, true, true);
    private SimpleH2Layout tLayout = new SimpleH2Layout(SimpleH2Layout.LEFT,
                                                       4, 0, false, true);
     /** Constructor. */
    public ParamInfoPanel(String strhelpfile) {
        super(null);
        DisplayOptions.addChangeListener(this);
        m_strHelpFile = strhelpfile;
        Container contentPane = getContentPane();
        attributeChangeListener = new AttributeChangeListener();
        setTitle(Util.getLabel("peTitle"));
        Toolkit tk = Toolkit.getDefaultToolkit();
        Dimension screenDim = tk.getScreenSize();
        if (screenDim.width > 1000)
            setLocation(400, 30);
        else
            setLocation(300, 30);     
        lbs = new JLabel[numItems];
        txts = new VUndoableText[numItems];
        pans = new JPanel[numItems];
        vlbs = new JLabel[100];
        vwidgs = new JComponent[100];
        vpans = new JPanel[100];
        vwidgIsSet = new boolean[100];

        pans[ILABEL] = new JPanel();
        pans[ILABEL].setLayout(xLayout);
        lbs[ILABEL] = new JLabel(Util.getLabel(VObjDef.LABEL));
        txts[ILABEL] = new VUndoableText(" ", textLen);
        pans[ILABEL].add(lbs[ILABEL]);
        pans[ILABEL].add(txts[ILABEL]);

        pans[CHOICE] = new JPanel();
        pans[CHOICE].setLayout(xLayout);
        lbs[CHOICE] = new JLabel(Util.getLabel(VObjDef.SETCHOICE));
        txts[CHOICE] = new VUndoableText(" ", textLen);
        pans[CHOICE].add(lbs[CHOICE]);
        pans[CHOICE].add(txts[CHOICE]);

        pans[CHVALS] = new JPanel();
        pans[CHVALS].setLayout(xLayout);
        lbs[CHVALS] = new JLabel(Util.getLabel(VObjDef.SETCHVAL));
        txts[CHVALS] = new VUndoableText("", textLen);
        pans[CHVALS].add(lbs[CHVALS]);
        pans[CHVALS].add(txts[CHVALS]);

        pans[VAR] = new JPanel();
        pans[VAR].setLayout(xLayout);
        lbs[VAR] = new JLabel(Util.getLabel(VObjDef.VARIABLE));
        txts[VAR] = new VUndoableText("", textLen);
        pans[VAR].add(lbs[VAR]);
        pans[VAR].add(txts[VAR]);

        pans[CMD_1] = new JPanel();
        pans[CMD_1].setLayout(xLayout);
        lbs[CMD_1] = new JLabel(Util.getLabel(VObjDef.CMD));
        txts[CMD_1] =  new VUndoableText("", textLen);
        pans[CMD_1].add(lbs[CMD_1]);
        pans[CMD_1].add(txts[CMD_1]);

        pans[CMD_2] = new JPanel();
        pans[CMD_2].setLayout(xLayout);
        lbs[CMD_2] = new JLabel(Util.getLabel(VObjDef.CMD2));
        txts[CMD_2] =  new VUndoableText("", textLen);
        pans[CMD_2].add(lbs[CMD_2]);
        pans[CMD_2].add(txts[CMD_2]);

        pans[IVAL] = new JPanel();
        pans[IVAL].setLayout(xLayout);
        lbs[IVAL] = new JLabel(Util.getLabel(VObjDef.SETVAL));
        txts[IVAL] =  new VUndoableText("", textLen);
        pans[IVAL].add(lbs[IVAL]);
        pans[IVAL].add(txts[IVAL]);

        pans[SHOWX] = new JPanel();
        pans[SHOWX].setLayout(xLayout);
        lbs[SHOWX] = new JLabel(Util.getLabel(VObjDef.SHOW));
        txts[SHOWX] =  new VUndoableText("", textLen);
        pans[SHOWX].add(lbs[SHOWX]);
        pans[SHOWX].add(txts[SHOWX]);

        pans[DIGITS] = new JPanel();
        pans[DIGITS].setLayout(xLayout);
        lbs[DIGITS] = new JLabel(Util.getLabel(VObjDef.STATPAR));
        txts[DIGITS] =  new VUndoableText("", textLen);
        pans[DIGITS].add(lbs[DIGITS]);
        pans[DIGITS].add(txts[DIGITS]);

        pans[STATVARS] = new JPanel();
        pans[STATVARS].setLayout(xLayout);
        lbs[STATVARS] = new JLabel(Util.getLabel(VObjDef.NUMDIGIT));
        txts[STATVARS] =  new VUndoableText("", textLen);
        pans[STATVARS].add(lbs[STATVARS]);
        pans[STATVARS].add(txts[STATVARS]);

        JPanel typePan = new JPanel();
 
        JLabel lb=new JLabel(Util.getLabel("peFile")+": ");
        lb.setPreferredSize(new Dimension(48, 25));
        typePan.add(lb);

        itemSource = new JLabel("");
        typePan.add(itemSource);
        typePan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0,true));
        itemSource.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        itemSource.setPreferredSize(new Dimension(windowWidth-250, 25));
        
        JPanel editPan = new JPanel();
        editPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        editPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));
        editPan.add(new JLabel("Edit:"));
        
        edit_panels=new JCheckBox("Panels");
        edit_panels.setActionCommand("editcheck");
        edit_panels.addActionListener(this); 
        editPan.add(edit_panels);
        edit_actions=new JCheckBox("Actions");
        edit_actions.setActionCommand("editcheck");
        edit_actions.addActionListener(this); 
        editPan.add(edit_actions);
        edit_tools=new JCheckBox("Tools");
        edit_tools.setActionCommand("editcheck");
        edit_tools.addActionListener(this); 
        editPan.add(edit_tools);
        edit_status=new JCheckBox("Status");
        edit_status.setActionCommand("editcheck");
        edit_status.addActionListener(this); 
        editPan.add(edit_status);

        typePan.add(editPan);
        JPanel fontPan = new JPanel();
        fontPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));

        lb = new JLabel(Util.getLabel("peType")+": ");
        lb.setPreferredSize(new Dimension(48, 25));
        fontPan.add(lb);
        typeOfItem = new JLabel("");

        fontPan.add(typeOfItem);

        lb = new JLabel(Util.getLabel("peStyle")+": ");

        fontPan.add(lb);

        typeOfItem.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        typeOfItem.setPreferredSize(new Dimension(100, 25));
        
        textStyle=new StyleChooser("style");
        textStyle.setOpaque(true);
        textStyle.setEditable(false);
       
        fontPan.add(textStyle);

        addButton(Util.getLabel("peEditStyles"), fontPan, this, "editStyles");
        addButton(Util.getLabel("peEditItems"), fontPan, this, "editItems");
        
        addButton("Browser...", fontPan, this, "openBrowser");
        if(!FillDBManager.locatorOff())
            addButton("Locator...", fontPan, this, "openLocator");

        headerPan=new JPanel();
        headerPan.setLayout(new AttrLayout());
        headerPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        headerPan.add(fontPan);
        headerPan.add(typePan);

        editablePan = new JPanel();
        editablePan.setLayout(tLayout);
        lb = new JLabel(Util.getLabel(VObjDef.EDITABLE));
        ButtonGroup editGroup = new ButtonGroup();
        editableOn = new JRadioButton(Util.getLabel("mlTrue"));
        editableOff = new JRadioButton(Util.getLabel("mlFalse"));
        editGroup.add(editableOn);
        editGroup.add(editableOff);
        editablePan.add(lb);
        editablePan.add(editableOn);
        editablePan.add(editableOff);
        editableOff.setSelected(true);
        editablePan.setVisible(false);

        attrPan=new JPanel();
        attrPan.setBorder(new BevelBorder(BevelBorder.LOWERED));
        attrPan.setLayout(new AttrLayout());

        attrPan.add(pans[ILABEL]);
        attrPan.add(pans[VAR]);
        attrPan.add(pans[IVAL]);
        attrPan.add(pans[SHOWX]);
        attrPan.add(pans[CMD_1]);
        attrPan.add(pans[CMD_2]);
        attrPan.add(pans[CHOICE]);
        attrPan.add(pans[CHVALS]);
        attrPan.add(pans[DIGITS]);
        attrPan.add(pans[STATVARS]);
        int wht=(numItems+3)*attrWidth;
        attrPan.setPreferredSize(new Dimension(windowWidth-8,wht));
        attrScroll = new JScrollPane(attrPan);
        attrScroll.setPreferredSize(new Dimension(windowWidth-2,wht));

        prefPan = new JPanel();

        prefPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 10, 0, false));
        prefPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        prefPan.setPreferredSize(new Dimension(400, 30));

        JPanel snapPan = new JPanel();
        snapPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));

        lb = new JLabel(Util.getLabel("Grid"));
        snapSize = new VUndoableText("5", 4);
        snapPan.add(lb);
        snapPan.add(snapSize);

        prefPan.add(snapPan);

        JPanel locPan = new JPanel();
        locPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));
        lb = new JLabel("X ");
        locPan.add(lb);
        locX = new VUndoableText("", 4);
        locPan.add(locX);

        lb = new JLabel("Y ");
        locPan.add(lb);
        locY = new VUndoableText("", 4);
        locPan.add(locY);

        lb = new JLabel("W ");
        locPan.add(lb);
        locW = new VUndoableText("", 4);
        locPan.add(locW);

        lb = new JLabel("H ");
        locPan.add(lb);
        locH = new VUndoableText("", 4);
        locPan.add(locH);

        prefPan.add(locPan);
        
        autoscroll=new JCheckBox("Scroll");
        autoscroll.setActionCommand("autoscroll");
        autoscroll.addActionListener(this);
        autoscroll.setToolTipText("Automatically scroll panels when moving components");

        prefPan.add(autoscroll);        
        
        JPanel savePan = new JPanel();
        savePan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        savePan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 5, 0, false));
        savePan.add(new JLabel("Save:"));
        
        autosaveButton=new JRadioButton("Auto");
        autosaveButton.setActionCommand("autosave");
        autosaveButton.addActionListener(this); 
        autosaveButton.setToolTipText("Automatically save edited panels and pages");
        savePan.add(autosaveButton);

        warnsaveButton=new JRadioButton("Warn");
        warnsaveButton.setActionCommand("warnsave");
        warnsaveButton.addActionListener(this); 
        warnsaveButton.setToolTipText("Issue a warning when switching panels with unsaved edits");
        savePan.add(warnsaveButton);

        usersaveButton=new JRadioButton("Manual");
        usersaveButton.setActionCommand("usersave");
        usersaveButton.addActionListener(this); 
        usersaveButton.setToolTipText("User is responsible for saving items manually");
        savePan.add(usersaveButton);
        
        ButtonGroup saveButtons=new ButtonGroup();
        saveButtons.add(autosaveButton);
        saveButtons.add(warnsaveButton);
        saveButtons.add(usersaveButton);
        prefPan.add(savePan);
 
        
        ctlPan = new JPanel();
        
        reloadButton=addButton(Util.getLabel("blLoad"), ctlPan, this, "LoadItem");
        saveButton=addButton(Util.getLabel("blSave"), ctlPan, this, "saveItem");
        revertButton=addButton(Util.getLabel("Revert"), ctlPan, this, "revertItem");
        revertButton.setToolTipText("Remove local item - revert to system default");
 
        itemLabel=new JLabel(Util.getLabel("peType"));
        itemLabel.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
        ctlPan.add(itemLabel);

        itemEntry=new VUndoableText(itemName,12);
        itemEntry.setActionCommand("itemName");
        itemEntry.addActionListener(this);
        itemEntry.setPreferredSize(new Dimension(300, 25));
        ctlPan.add(itemEntry);

        InLabel=new JLabel(Util.getLabel("peDir"));
        InLabel.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));

        ctlPan.add(InLabel);

        saveDirectory=new JComboBox();
        saveDirectory.setActionCommand("setSaveDirectory");
        saveDirectory.setEditable(true);
        saveDirectory.setPreferredSize(new Dimension(150, 25));
        ctlPan.add(saveDirectory);

        lb = new JLabel(Util.getLabel("peType"));
        lb.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));
        ctlPan.add(lb);

        typeList=new JComboBox();
        typeList.setActionCommand("setItemType");
        typeList.setEditable(true);
        typeList.setModel(new DefaultComboBoxModel());
        typeList.setPreferredSize(new Dimension(130, 25));        
        ctlPan.add(typeList);    
        clearButton=addButton(Util.getLabel("blClear"), ctlPan, this, "ClearItem");
        
        ctlPan.setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 4, 0, false));
        ctlPan.setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        ctlPan.setPreferredSize(new Dimension(windowWidth-2, 35));

        panel=new JPanel();
        panel.setLayout(new PanelLayout());
        panel.add(headerPan);
        panel.add(attrScroll);
        panel.add(prefPan);
        panel.add(ctlPan);

        contentPane.add(panel,BorderLayout.CENTER);

        initPanel();
        initAction();
        getEditItems();
        addWindowListener(new WindowAdapter() {
            public void  windowClosing(WindowEvent e) {
                savePrefAttr();
                saveObj();
                saveGeometry();
                writePersistence();
                setVisible(false);
                if(editor !=null){
                    editor.saveContent();
                    editor.setVisible(false);
                }
                if(warnDialog !=null){
                    warnDialog.setVisible(false);
                }
                ParamInfo.setEditMode(false);
            }
            public void  windowDeactivated(WindowEvent e) {
                savePrefAttr();
            }
        });
/*
        setResizable(false);
*/
        historyButton.setEnabled(false);
        undoButton.setEnabled(false);
        historyButton.setActionCommand("edit");
        historyButton.setEnabled(false);
        historyButton.addActionListener(this);
        undoButton.setActionCommand("undo");
        undoButton.addActionListener(this);
        closeButton.setActionCommand("Close");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("Cancel");
        abandonButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
        revertButton.addActionListener(this);
        revertButton.setActionCommand("revertItem");

        setAbandonEnabled(true);
        setCloseEnabled(true);
 
        setBgColor();
        pack();

        typeList.addActionListener(this);
        saveDirectory.addActionListener(this);
        addGeometryListener();
    }

    private static void setDebug(String s){
        if(DebugOutput.isSetFor("ParamInfoPanel"))
            Messages.postDebug("ParamInfoPanel : "+s);
    }
    public ModelessDialog getEditor(){
        return editor;
    }
    public void addGeometryListener(){
        addComponentListener(new ComponentAdapter() {
            public void componentMoved(ComponentEvent ce) {
                saveGeometry();
            }
        });
    }
 
    public void writePersistence() {
        if(openpath!=null)
        FileUtil.writeValueToPersistence(panelEditorPersistenceFile, // File
                "openpath", // Key
                openpath); // Value
        if(savepath!=null)
            FileUtil.writeValueToPersistence(panelEditorPersistenceFile, // File
                    "savepath", // Key
                    savepath); // Value
        String estr=Integer.toString(ParamInfo.getEditItems());
        FileUtil.writeValueToPersistence(panelEditorPersistenceFile, // File
                "edit_types", // Key
                estr); // Value

    }
    public void readPersistence() {
        openpath=FileUtil.readStringFromPersistence(panelEditorPersistenceFile,"openpath");
        savepath=FileUtil.readStringFromPersistence(panelEditorPersistenceFile,"savepath");
        String estr=FileUtil.readStringFromPersistence(panelEditorPersistenceFile,"edit_types");
        if(estr!=null){
            int i=Integer.parseInt(estr);
            ParamInfo.setEditItems(i);
            getEditItems();
        }
    }

    public void saveGeometry(){
        FileUtil.writeGeometryToPersistence(panelEditorGeometryFile, this);
    }

    public void restoreGeometry(){
        if(initialized)
            return;
        FileUtil.setGeometryFromPersistence(panelEditorGeometryFile, this);
        initialized=true;
    }

    private void setBgColor(){
        textStyle.setBackground(null);
    }

    public void setVisible(boolean b){
        if(b){
            if(!initialized){
                restoreGeometry();
                readPersistence();
            }
        }
        super.setVisible(b);

    }
    /** PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent evt){
        super.propertyChange(evt);
    }

    public void setChangedPage(boolean b){
        if(b != changed_page){
            changed_page=b;
            updateItemButtons();
        }
    }

    /** get edit object's location and size. */
    public void getObjectGeometry() {

        JComponent comp=(JComponent)editVobj;

        if(comp==null){
            locX.setText("");
            locY.setText("");
            locH.setText("");
            locW.setText("");
            return;
        }

        if(!comp.isShowing())
            return;

        Point pt = comp.getLocation();
        Point pt2;
        Container pp = comp.getParent();
        while (pp != null) {
             if (pp instanceof ParamPanel)
                break;
             pt2 = pp.getLocation();
             pt.x += pt2.x;
             pt.y += pt2.y;
             pp = pp.getParent();
        }

        locX.setText(Integer.toString(pt.x));
        locY.setText(Integer.toString(pt.y));

        Dimension d=comp.getPreferredSize();
        locW.setText(Integer.toString(d.width));
        locH.setText(Integer.toString(d.height));
    }

    /** set edit object's location and size. */
    public void setObjectGeometry()
    {
       JComponent comp=(JComponent)editVobj;
       if(comp ==null || !comp.isShowing())
           return;
       String s;
       Dimension d = comp.getSize();
       int x = 0;
       int y = 0;
        try {
            s = locX.getText().trim();
            x = Integer.parseInt(s);
            s = locY.getText().trim();
            y = Integer.parseInt(s);
            s = locH.getText().trim();
            d.height=Integer.parseInt(s);
            s = locW.getText().trim();
            d.width=Integer.parseInt(s);
       }
       catch (NumberFormatException e) { }

       comp.setPreferredSize(d);

       Point p0 = editPanel.getLocationOnScreen();
       Point p2 = new Point(x+p0.x,y+p0.y);

       comp.setLocation(p2);
       ParamEditUtil.relocateObj(editVobj);
    }
    
    private void abandon(){
        // Remove the last appended statement and restore to the
        // previous position
        sshare = Util.getSessionShare();
        ControlPanel ctl = Util.getControlPanel();
        if(ctl!=null){
            editPanel=ctl.getParamPanel().getParamLayout();
            if (editPanel != null)
                editPanel.restore();
            ActionBar action=(ActionBar)ctl.getActionPanel();
            if(action !=null)
                action.restore();
        }
        saveGeometry();
        writePersistence();

        this.setVisible(false);
        if(editor!=null)
            editor.setVisible(false);
        if(warnDialog!=null)
            warnDialog.setVisible(false);
        ParamEditUtil.setEditPanel((ParamPanel) null);
       // if (ctl != null)
        //    ctl.setEditMode(false);
        ParamInfo.setEditMode(false);

        appIf = Util.getAppIF();
        if (sshare != null ) {
            LocatorHistoryList lhl = sshare.getLocatorHistoryList();
            if(lhl!=null)
                lhl.setLocatorHistoryToPrev();
        }
        if (appIf != null)
            appIf.repaint();
    }
    /** ActionListener interface. */
    public void actionPerformed(ActionEvent  evt)
    {
        ControlPanel ctl;
        String cmd = evt.getActionCommand();

        if (cmd.equals("setSaveDirectory")){
            if(!inAddMode){
                int index=saveDirectory.getSelectedIndex();
                if(index<0)  // new name
                    updateSaveDirMenu();
                lastSaveIndex=saveDirectory.getSelectedIndex();
                setChangedPage(true);
            }
            return;
        }
        else if (cmd.equals("setItemType")){
            if(!inAddMode){
                panelType=(String)typeList.getSelectedItem();
            }
            return;
        }
        else if (cmd.equals("itemName")) {
            itemName=itemEntry.getText().trim();
            updateItemButtons();
            return;
        }
        else if (cmd.equals("revertItem")) {
            revertItem();
        }
        else if (cmd.equals("editcheck")) {
            setEditItems();
            return;
        }
        else if (cmd.equals("saveItem")) {
            switch(selectedType){
            case ParamInfo.PARAMLAYOUT:
                saveLayout();
                break;
            case ParamInfo.TOOLPANEL:
                saveToolPanel();
                break;
            case ParamInfo.PAGE:
                savePage();
                break;
            case ParamInfo.ACTION:
                saveAction();
                break;
            case ParamInfo.ITEM:
                saveItem();
                break;
            case ParamInfo.PARAMPANEL:
                saveParamPanel();
                break;
            }
            return;
        }
        else if (cmd.equals("LoadItem")) {
            switch(selectedType){
            case ParamInfo.PARAMLAYOUT:
                loadItem();
                break;
            case ParamInfo.PAGE:
                loadItem();
                break;
            case ParamInfo.ACTION:
                loadItem();
                break;
            case ParamInfo.ITEM:
                loadItem();
                break;
            case ParamInfo.PARAMPANEL:
            case ParamInfo.TOOLPANEL:
                reloadPanel();
                break;
            }
        }
        else if (cmd.equals("autoscroll")) {
            boolean b=autoscroll.isSelected();
            ParamEditUtil.setAutoScroll(b);
        }
        else if(cmd.equals("autosave")){
            autosaveButton.setSelected(true);
            save_policy=AUTO_SAVE;
        }
        else if(cmd.equals("warnsave")){
            warnsaveButton.setSelected(true);
            save_policy=WARN_SAVE;
        }
        else if(cmd.equals("usersave")){
            usersaveButton.setSelected(true);
            save_policy=USER_SAVE;
        }
        else if (cmd.equals("ClearItem")) {
            switch(selectedType){
            case ParamInfo.PARAMLAYOUT:
                if (editPanel != null){
                    setItemEdited(editPanel);
                    editPanel.clearAll();
                }
                break;
            case ParamInfo.PAGE:
                if (editVobj != null) {
                    JComponent jobj = (JComponent)editVobj;
                    if(jobj.getComponentCount()==0){
                        editVobj.destroy();
                        editPanel.removeTab(editVobj);
                        setItemEdited(editPanel);
                    }
                    else{
                        jobj.removeAll();
                        ParamEditUtil.setEditObj(editVobj);
                    }
                }
                break;
            case ParamInfo.ITEM:
                if (editVobj != null) {
                    JComponent cobj = (JComponent)editVobj;
                    JComponent pobj = (JComponent)cobj.getParent();
                    setItemEdited(editVobj);
                    editVobj.destroy();
                    pobj.remove(cobj);
                    ParamEditUtil.setEditObj((VObjIF)pobj);
                    pobj.repaint();
                }
                break;
            }
            updateItemButtons();
            return;
        }
        else if (cmd.equals("Close")) {
            // Remove the last appended statement and restore to the
            // previous position
            sshare = Util.getSessionShare();
            saveObj();
            saveGeometry();
            writePersistence();
            this.setVisible(false);
            if(editor !=null){
                editor.saveContent();
                editor.setVisible(false);
            }

            ParamEditUtil.setEditPanel((ParamPanel) null);
            //ctl = Util.getControlPanel();
           // if (ctl != null)
           //     ctl.setEditMode(false);
            ParamInfo.setEditMode(false);

            if (sshare != null ) {
                LocatorHistoryList lhl = sshare.getLocatorHistoryList();
                if(lhl != null)
                    lhl.setLocatorHistoryToPrev();
            }
            appIf = Util.getAppIF();
            if (appIf != null)
                appIf.repaint();
            return;
        }
        else if(cmd.equals("Cancel")) {
            abandon();
            return;
        }
        else if (cmd.equals("editStyles")) {
            // open the DisplayOptions dialog
            Util.showDisplayOptions();
            return;
        }
        
        else if (cmd.equals("openLocator")) {
            Util.getDefaultExp().sendToVnmr("vnmrjcmd('toolpanel','Locator','open')");
            return;
        }
        else if (cmd.equals("openBrowser")) {
            Util.getDefaultExp().sendToVnmr("vnmrjcmd('LOC browserPanel')");
            return;
        }
        else if (cmd.equals("editItems")) {
            if(editor==null){
                editor=new ItemEditor("Item Editor"); 
            }
            else if(editor.isVisible())
                editor.setVisible(false);
            else
                editor.setVisible(true);
            return;
        }
        else if (cmd.equals("help"))
            displayHelp();
    }
    private void getEditItems(){
        int current_edits=ParamInfo.getEditItems();
        edit_panels.setSelected(((current_edits & ParamInfo.PARAMPANELS)>0));
        edit_actions.setSelected(((current_edits & ParamInfo.ACTIONBAR)>0));
        edit_tools.setSelected(((current_edits & ParamInfo.TOOLPANELS)>0));
        edit_status.setSelected(((current_edits & ParamInfo.STATUSBAR)>0));
    }

    private void setEditItems() {
        int current_edits=0;
        if(edit_panels.isSelected())
            current_edits+=ParamInfo.PARAMPANELS;
        if(edit_actions.isSelected())
            current_edits+=ParamInfo.ACTIONBAR;
        if(edit_tools.isSelected())
            current_edits+=ParamInfo.TOOLPANELS;
        if(edit_status.isSelected())
            current_edits+=ParamInfo.STATUSBAR;
        ParamInfo.setEditItems(current_edits);
    }

    /** set the panel to be edited */
    public void setEditPanel(ParamPanel obj,boolean clrvobj) {
        if(editPanel !=null && edits.size()>0){
            checkEdits();
        }
        editPanel = obj;
        if(clrvobj)
            editVobj = null;
        fileName=editPanel.getPanelFile();
        panelName = editPanel.getPanelName();
        String title=Util.getLabel("peEdit")+" "+ panelName;
        if (obj == null){
            setTitle(title);
            return;
        }

        if (isLayoutPanel(obj) ){
            typeList.removeAllItems();

            ShufDBManager dbManager = ShufDBManager.getdbManager();
            ArrayList list = dbManager.attrList.getAttrValueList(
                    PTYPE, Shuf.DB_PANELSNCOMPONENTS);
            typeList.setModel(new DefaultComboBoxModel(new Vector(list)));
            typeList.setMaximumRowCount(list.size());
            panelType = editPanel.getPanelType();
            typeList.setSelectedItem(panelType);
            setTitle(title+" "+Util.getLabel("peFor")+" "+editPanel.getLayoutName());  
        }
        else {
            setTitle(title);
        }
       
        obj.setSnap(true);
        ParamEditUtil.setSnap(true);
        String d = (String) snapSize.getText();
        d.trim();
        if (d.length() < 1) {
            d = "5";
            snapSize.setText(d);
        }
        if (d.length() > 0) {
            ParamEditUtil.setSnapGap(d);
            obj.setSnapGap(d);
        }
        obj.setGrid(true);
        Color c=Util.getGridColor();
        obj.setGridColor(c);

        makeSaveDirMenu();
        setDebug("setEditPanel : "+fileName);
    }

    ParamPanel getParamPanel(VObjIF obj){
        Component comp = (Component)obj;
        while (comp != null) {
            if (comp instanceof ParamPanel) {
                 return (ParamPanel)comp;                
            }
            comp = comp.getParent();
        }
        return null;
    }

    ParamLayout getParamLayout(VObjIF obj){
        Component comp = (Component)obj;
        while (comp != null) {
            if (comp instanceof ParamLayout) {
                 return (ParamLayout)comp;                
            }
            comp = comp.getParent();
        }
        return null;
    }
    VGroup getPage(VObjIF obj){
        ParamLayout panel=getParamLayout(obj);
        if(panel==null)
            return null;
        Component comp = (Component)obj;
        while (comp != null && comp != panel) {
            if (ParamPanel.isTabGroup(comp)) {
                 return (VGroup)comp;                
            }
            comp = comp.getParent();
        }
        return null;
    }
    ActionBar getAction(VObjIF obj){
        Component comp = (Component)obj;
        while (comp != null) {
            if(comp instanceof ActionBar)
                return (ActionBar)comp; 
            comp = comp.getParent();
        }
        return null;
    }
    
    void checkEdits(){
        if(DebugOutput.isSetFor("ParamInfoSave"))
            saveDebug();
        switch(save_policy){
        case AUTO_SAVE:
            autoSave();
            break;
        case WARN_SAVE:
            warnSave();
            break;
        case USER_SAVE:
            break;
        }
    }

    void autoSave() {
        for (int i = 0; i < edits.size(); i++) {
            VObjIF obj = edits.get(i);
            if (obj instanceof ActionBar)
                saveAction();
            else if (obj == editPanel)
                saveLayout();
            else{
                String name=defaultItemName(obj);
                saveObj();
                savePage(obj,name);
            }          
        }
        edits.clear();       
    }
    
    void warnSave() {
        if(warnDialog==null){
            warnDialog=new SaveWarnDialog("");
            Point p=getLocation();
            Dimension d=getSize();
            p.x+=d.width;
            warnDialog.setLocation(p);
        }
        //warnDialog.setTitle("Unsaved Components of <"+editPanel.getPanelName()+">");
        warnDialog.setTitle("Unsaved Components");
        warnDialog.showEdits();
    }
    
    void saveDebug(){
        for(int i=0;i<edits.size();i++){
            VObjIF obj=edits.get(i);
            if(editPanel instanceof ActionBar)
                Messages.postDebug("Save Needed: Action panel "+editPanel.getPanelName());
            else 
                if(editPanel instanceof ParamLayout){
                if(obj==editPanel)
                    Messages.postDebug("Save Needed: Panel "+editPanel.getPanelName());
                else
                    Messages.postDebug("Save Needed: "+ editPanel.getPanelName()+" Page "+obj.getAttribute(LABEL));
            }
        }
    }
    void addEditItem(VObjIF obj){
        if(!edits.contains(obj)){
            if(obj instanceof ParamPanel)               
                System.out.println("Adding panel:"+((ParamPanel)obj).getPanelName());
            else
                System.out.println("Adding page:"+obj.getAttribute(LABEL));
            edits.add(obj);
            if(save_policy==WARN_SAVE)
                warnSave();
        }
    }
    void rmvEditItem(VObjIF obj){
        if(!edits.contains(obj)){
            //setDebug("Removing edit item:"+obj.getAttribute(LABEL));
            edits.remove(obj);
        }
    }    
    public void setItemEdited(VObjIF vobj){
        if(vobj==null)
            return;
        VObjIF edit=null;
        switch(selectedType){
        case ParamInfo.PAGE:
            addEditItem(vobj);
            break;
        case ParamInfo.PARAMLAYOUT:
            addEditItem(vobj);
            break;
        case ParamInfo.ITEM:
            edit=getPage(vobj);
            if(edit!=null){
                addEditItem(edit);
                break;
            } // note: intentional fall-through
        case ParamInfo.ACTION:
            edit=getAction(vobj);
            if(edit!=null)
                addEditItem(edit);
            break;
        }
    }
    
    /** Set the active edit object. */
    public void setEditElement(VObjIF vobj) {
        boolean newobj=false;

        if(vobj != editVobj){
            if(editVobj != null)
                saveObj();
            itemName=defaultItemName(vobj);
            newobj=true;
            tabgrp=null;
        }

        editVobj = vobj;

        if(!newobj){
            return;
        }
        itemEntry.setText(itemName);

        inSetMode = true;
        getObjectGeometry();

        int oldType=selectedType;

        if (vobj == null) {
            editPanel=null;
        }
        else{
            typeOfItem.setText(vobj.getAttribute(TYPE));
            String data = vobj.getAttribute(FONT_STYLE);
            textStyle.setStyleObject(vobj);
            if(data != null && textStyle.isType(data))
                textStyle.setType(data);
            else
                textStyle.setDefaultType();
            if(vobj instanceof ParamPanel)
                selectedType=((ParamPanel)vobj).getType();
            else if(isActionPanel(vobj))
                selectedType=ParamInfo.ACTION;
            else if(isTabPage(vobj))
                selectedType=ParamInfo.PAGE;
            else
                selectedType=ParamInfo.ITEM;
            
            String spath=getSourcePath(itemName);
            if(spath==null)
                itemSource.setText("");
            else
                itemSource.setText(spath);            
            if (vobj instanceof VEditIF)
                showVariableFields((VEditIF)vobj);
            else
                showDefaultFields(vobj);
        }
        isNewType=(oldType==selectedType)?false:true;
        updateItemButtons();
        
        revertButton.setEnabled(userFileExists());
        adjustAttrPanelSize();
        inSetMode = false;
        setDebug("setEditElement : "+itemName);

    }

    //================ private methods ==================================

    private void postInfo(String s){
        if(DebugOutput.isSetFor("pedit"))
            Messages.postInfo(s);
        setDebug(s);
    }

    private boolean hasReference(VObjIF obj){
        if(obj==null)
            return false;
        if(itemName==null || itemName.length()==0)
            return false;
        String fname=itemName;
        String ext=FileUtil.fileExt(fname);
        if(ext !=null && ext.contains(".xml"))
            FileUtil.setOpenAll(false);
        else
            FileUtil.setOpenAll(true);
        
        fname=fname.replaceFirst(".xml","");
        if(editPanel !=null && isTabPage(obj)){
            if(FileUtil.findReference(editPanel.getLayoutName(),fname+".xml")){
                FileUtil.setOpenAll(true);
                return true;
            }
        }
        FileUtil.setOpenAll(true);
        return FileUtil.fileExists("PANELITEMS/"+itemName+".xml");
    }

    private String getSavePath(String fname){
        String base=FileUtil.usrdir();
        String spath=null;
        switch(selectedType){
        case ParamInfo.ITEM:
            spath=FileUtil.fullPath(base,"PANELITEMS/"+fname+".xml");
            break;
        case ParamInfo.PARAMPANEL:
            spath=FileUtil.fullPath(base,"INTERFACE/"+fname+".xml");
            break;
        case ParamInfo.TOOLPANEL:
            spath=FileUtil.fullPath(base,"TOOLPANELS/"+fname+".xml");
            break;
        default:
            if (editPanel != null) {
                String path=editPanel.getLayoutName();
                if(path!=null)
                    spath=FileUtil.getLayoutPath(path,fname+".xml"); 
            }
            break;
        }        
        return spath;
    }
    private String getSourcePath(String name){
        String spath=null;
        String fname=name;
        if(fname==null)
            return null;
        String ext=FileUtil.fileExt(fname);
        if(ext!=null && ext.contains(".xml"))
            FileUtil.setOpenAll(false);
        else
            FileUtil.setOpenAll(true);
        fname=fname.replaceFirst(".xml","");
        if(name!=null && name.length()>0) {
            switch(selectedType){
            case ParamInfo.ITEM:
                spath=FileUtil.openPath("PANELITEMS/"+fname+".xml");
                break;
            case ParamInfo.PARAMPANEL:
                spath=FileUtil.openPath("INTERFACE/"+fname+".xml");
                break;
             default:
                if (editPanel != null) {
                    String path=editPanel.getLayoutName();
                    if(path!=null)
                        spath=FileUtil.getLayoutPath(path,fname+".xml"); 
                }
                break;
            }
        }
        FileUtil.setOpenAll(true);
        return spath;
    }

    private String defaultItemName(VObjIF obj){
        String name="";
        if(obj==null){
            return "";
        }
        if(obj instanceof ParamPanel){
            name=((ParamPanel)obj).getPanelFile();
            if(name !=null)
                name=name.replace(".xml","");
        }
        else if(isActionPanel(obj)){
            ActionBar action=getAction(obj);
            name=action.getPanelFile();
            name=name.replace(".xml","");
        }
        else if((obj instanceof VGroup) || (obj instanceof VTabbedPane)){
            name=obj.getAttribute(REFERENCE);
            if(name==null || name.length()==0){
                name=obj.getAttribute(LABEL);
                JComponent p=(JComponent)obj;
                if(name==null || name.length()==0){
                    int nLength = p.getComponentCount();
                    for (int i = 0; i < nLength; i++) {
                        Component comp = p.getComponent(i);
                        if (comp instanceof VTab) {
                            name=((VObjIF)comp).getAttribute(LABEL);
                            if(name!=null && name.length()>0)
                                 break;
                        }
                    }
                }
                // if main group has no label try children ..
                if(name==null || name.length()==0){
                    int nLength = p.getComponentCount();
                    for (int i = 0; i < nLength; i++) {
                        Component comp = p.getComponent(i);
                        if (comp instanceof VLabel || comp instanceof VGroup) {
                            name=((VObjIF)comp).getAttribute(LABEL);
                            if(name!=null && name.length()>0)
                                 break;
                        }
                    }
                }
            }
        }
        if(name!=null && name.length()>=0){
            String s="";
            StringTokenizer tok=new StringTokenizer(name);
            while(tok.hasMoreTokens())
                s+=tok.nextToken();
            name=s.trim();
        }
        String spath=getSourcePath(name);
        if(spath !=null){
            name=FileUtil.pathName(spath);
            name=name.replace(".xml","");
        }
        return name;
    }

    private void showVariableFields(VEditIF vobj) {
        int i;

        // Remove existing fields

        attrPan.removeAll();

        // Put in the fields for this object

        Object attrs[][] = vobj.getAttributes();
        if(attrs==null)
            return;
        int nats = attrs.length;

        for (i = 0; i < nats; i++) {

            vwidgIsSet[i] = true;
            int attrCode = ((Integer) attrs[i][0]).intValue();
            if (vpans[i] == null)
                vpans[i] = new JPanel();
            else
                vpans[i].removeAll();

            vlbs[i] = new JLabel(((String) attrs[i][1]).trim());
            vpans[i].add(vlbs[i]);
            String data = vobj.getAttribute(attrCode);
            String type = "edit";
            String enabled = "true";
            if (attrs[i].length > 2) {
                String str = (String) attrs[i][2];
                if (str.equals("true") || str.equals("false"))
                    enabled = str;
                else
                    type = str;
            }

            if (type.equals("edit")) {
                vpans[i].setLayout(xLayout);
                VUndoableText jtxt = new VUndoableText("", textLen);
                vwidgs[i] = jtxt;
                if (data != null)
                    jtxt.setText(data);
                if (enabled.equals("true")) {
                    jtxt.setActionCommand(String.valueOf(attrCode));
                    jtxt.setEditable(true);
                } else {
                    jtxt.setEditable(false);
                }
                jtxt.addActionListener(attributeChangeListener);
            } else if (type.equals("menu") || type.equals("style")
                    || type.equals("color")) {
                // Use menu to take attribute value
                JComboBox jcombo;
                vpans[i].setLayout(tLayout);
                if (type.equals("menu"))
                    jcombo = new JComboBox((Object[]) attrs[i][3]);
                else {
                    jcombo = new StyleChooser(type);
                    if (type.equals("color") && data == null)
                        data = "transparent";
                }
                jcombo.setOpaque(true);
                jcombo.setEditable(false);
                jcombo.setBackground(null);

                vwidgs[i] = jcombo;
                vwidgIsSet[i] = false;
                jcombo.setActionCommand(String.valueOf(attrCode));
                jcombo.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent evt) {
                        comboAction(evt);
                    }
                });
                if (data != null) {
                    int n = jcombo.getItemCount();
                    for (int j = 0; j < n; j++) {
                        String str = ((String) jcombo.getItemAt(j));
                        if (data.equalsIgnoreCase(str)) {
                            jcombo.setSelectedIndex(j);
                            vwidgIsSet[i] = true;
                            break;
                        }
                    }
                }
            } else if (type.equals("radio")) {

                // Use button group to take attribute value

                vpans[i].setLayout(tLayout);
                String[] names = (String[]) attrs[i][3];
                int n = names.length;
                RadioGroup rgroup = new RadioGroup(attrCode);
                vwidgs[i] = rgroup;
                for (int j = 0; j < n; j++) {
                    rgroup.add(names[j]);
                }
                if (vobj instanceof VGroup) {
                    if (attrCode == TAB)
                        tabgrp = rgroup;
                }
                if (data != null) {
                    rgroup.setSelectedItem(data);
                }
            }
            vpans[i].add(vwidgs[i]);
            attrPan.add(vpans[i]);
        }
        //setBgColor(getComponents(), Util.getBgColor());
    }

    private void showDefaultFields(VObjIF vobj) {
        int i;

        // Remove existing fields

        attrPan.removeAll();

        // Put in the correct fields;
        // make inactive

        i = ILABEL;
        if (vobj != null && vobj instanceof VText) {
            editablePan.setVisible(true);
            pans[ILABEL].setVisible(false);
            attrPan.add(editablePan);
            i = 1;
            String ed = vobj.getAttribute(EDITABLE);
            if (ed != null && ed.equals("yes"))
                editableOn.setSelected(true);
            else
                editableOff.setSelected(true);
        } else {
            editablePan.setVisible(false);
            pans[ILABEL].setVisible(true);
        }
        while (i < numItems) {
            if (i == CMD_2) {
                if (vobj != null && !(vobj instanceof VEntry))
                    attrPan.add(pans[i]);
            } else if (i == DIGITS) {
                if (vobj != null && vobj instanceof VEntry)
                    attrPan.add(pans[i]);
            } else
                attrPan.add(pans[i]);
            txts[i].setText("");
            txts[i].setEnabled(false);
            lbs[i].setEnabled(false);
            i++;
        }

        if (vobj == null)
            return;

        txts[VAR].setEnabled(true);
        lbs[VAR].setEnabled(true);
        String data = vobj.getAttribute(VARIABLE);
        if (data != null)
            txts[VAR].setText(data);
        txts[SHOWX].setEnabled(true);
        lbs[SHOWX].setEnabled(true);
        data = vobj.getAttribute(SHOW);
        if (data != null)
            txts[SHOWX].setText(data);

        if ((vobj instanceof VLabel) || (vobj instanceof VButton) ||
            (vobj instanceof VRadio) || (vobj instanceof VToggle) ||
            (vobj instanceof VCheck) || (vobj instanceof VTab) ||
            (vobj instanceof VMenu)) {
            txts[ILABEL].setEnabled(true);
            lbs[ILABEL].setEnabled(true);
            data = vobj.getAttribute(LABEL);
            if (data != null)
                txts[ILABEL].setText(data);
        }
        if (vobj instanceof VMenu) {
            txts[CHOICE].setEnabled(true);
            lbs[CHOICE].setEnabled(true);
            txts[CHVALS].setEnabled(true);
            lbs[CHVALS].setEnabled(true);
            data = vobj.getAttribute(SETCHOICE);
            if (data != null)
                txts[CHOICE].setText(data);
            data = vobj.getAttribute(SETCHVAL);
            if (data != null)
                txts[CHVALS].setText(data);
        }
        if (vobj instanceof VScroll) {
            txts[CHVALS].setEnabled(true);
            lbs[CHVALS].setEnabled(true);
            data = vobj.getAttribute(SETCHVAL);
            if (data != null)
                txts[CHVALS].setText(data);
        }
        if ((vobj instanceof VLabel) || (vobj instanceof VTab)) {
            return;
        }
        if (!(vobj instanceof VButton)) {
            txts[IVAL].setEnabled(true);
            lbs[IVAL].setEnabled(true);
            data = vobj.getAttribute(SETVAL);
            if (data != null)
                txts[IVAL].setText(data);
        }
        txts[CMD_1].setEnabled(true);
        lbs[CMD_1].setEnabled(true);
        data = vobj.getAttribute(CMD);
        if (data != null)
            txts[CMD_1].setText(data);
        if ((vobj instanceof VCheck) || (vobj instanceof VToggle)
            || (vobj instanceof VRadio)) {
            txts[CMD_2].setEnabled(true);
            lbs[CMD_2].setEnabled(true);
            data = vobj.getAttribute(CMD2);
            if (data != null)
                txts[CMD_2].setText(data);
        }
        if (vobj instanceof VEntry) {
            txts[DIGITS].setEnabled(true);
            lbs[DIGITS].setEnabled(true);
            data = vobj.getAttribute(NUMDIGIT);
            if (data != null)
                txts[DIGITS].setText(data);
        }
        //setBgColor(getComponents(), Util.getBgColor());
    }

    private void saveObj() {
        if (editVobj instanceof VEditIF) {
            VEditIF vobj = (VEditIF) editVobj;
            Object attrs[][] = vobj.getAttributes();
            if(attrs!=null){
                int nSize = attrs.length;
                for (int i = 0; i < nSize; i++) {
                    String str = null;
                    int attrval = ((Integer) attrs[i][0]).intValue();
                    if (vwidgs[i] instanceof JTextField) {
                        str = ((JTextField) vwidgs[i]).getText();
                    } else if (vwidgs[i] instanceof JComboBox) {
                        str = (String) ((JComboBox) vwidgs[i]).getSelectedItem();
                    } else if (vwidgs[i] instanceof RadioGroup) {
                        str = (String) ((RadioGroup) vwidgs[i]).getSelectedItem();
                    } else if (vwidgs[i] instanceof VColorChooser) {
                        str = ((VColorChooser) vwidgs[i]).getAttribute(KEYVAL);
                    }
                    if (vwidgIsSet[i]) {
                        if (str != null && str.length() > 0)
                            vobj.setAttribute(attrval, str);
                        else
                            vobj.setAttribute(attrval, null);
                    }
                }
            }
        } else {
            if (txts[ILABEL].isEnabled()) {
                setObjAttr(LABEL, ILABEL);
            }
            if (txts[CHOICE].isEnabled()) {
                setObjAttr(SETCHOICE, CHOICE);
            }
            if (txts[CHVALS].isEnabled()) {
                setObjAttr(SETCHVAL, CHVALS);
            }
            if (txts[VAR].isEnabled()) {
                setObjAttr(VARIABLE, VAR);
            }
            if (txts[CMD_1].isEnabled()) {
                setObjAttr(CMD, CMD_1);
            }
            if (txts[CMD_2].isEnabled()) {
                setObjAttr(CMD2, CMD_2);
            }
            if (txts[IVAL].isEnabled()) {
                setObjAttr(SETVAL, IVAL);
            }
            if (txts[SHOWX].isEnabled()) {
                setObjAttr(SHOW, SHOWX);
            }
            if (txts[DIGITS].isEnabled()) {
                setObjAttr(NUMDIGIT, DIGITS);
            }
        }
        ParamEditUtil.clearDummyObj();
    }

    private void updateSaveDirMenu(){
        if (editPanel == null)
            return;
        String edit=(String)saveDirectory.getSelectedItem(); // edit box
        if(lastSaveIndex==SAVE_IN_LAYOUT)
            editPanel.setLayoutName(edit);
        else
            editPanel.setDefaultName(edit);
        if(editPanel.getLayoutName()!=null)
            setTitle("Edit "+ panelName+" panel for "+editPanel.getLayoutName());
        //else
       //   setTitle("Edit "+ panelName);
        if(editVobj==editPanel){
            showVariableFields(editPanel); // update default edit field
        }
        makeSaveDirMenu();
    }

    private void updateLocator(String file, String path) {
        if(FillDBManager.locatorOff())
            return;
        String usr = System.getProperty("user.name");
        AddToLocatorViaThread addThread = new AddToLocatorViaThread(
                Shuf.DB_PANELSNCOMPONENTS, file, path, usr, true);
        addThread.setPriority(Thread.MIN_PRIORITY);
        addThread.setName("AddToLocatorViaThread");
        addThread.start();
    }

 
    /** save a parameter panel. */
    private void saveLayout(VObjIF obj){
        ParamLayout layout=getParamLayout(obj);
        if (layout == null)
            return;
        String name=layout.getPanelFile();
        LayoutBuilder.clrShufflerString();
        LayoutBuilder.setShufflerString(ETYPE,"panels");
        LayoutBuilder.setShufflerString(PTYPE,panelType);
        LayoutBuilder.setShufflerString(NAME,panelName);

        String dir= null;
        if(lastSaveIndex==SAVE_IN_LAYOUT)
            dir=layout.getSaveDir();
        else
            dir=layout.getDefaultDir();

        if(dir==null){
            Messages.postError("error saving panel "+name);
            return;
        }

        dir+="/"+name;
        LayoutBuilder.writeToFile(layout, dir, true);
        updateLocator(name, dir);
        postInfo("Saving Folder : "+name);
    }

    /** save a parameter panel. */
    private void saveLayout(){
        saveObj();
        saveLayout(editPanel);
        rmvEditItem(editPanel);
    }
    /** save action panel. */
    private void saveAction(VObjIF obj){
        ActionBar action=getAction(obj);
        if(action == null){
            Messages.postError("error saving Action panel for "+fileName);
            return;
        }
        String name=action.getPanelFile();
        String dir= null;
        if(lastSaveIndex==SAVE_IN_LAYOUT)
            dir=action.getSaveDir();
        else
            dir=action.getDefaultDir();

        if(dir==null){
            Messages.postError("error saving  "+name);
            return;
        }
        dir+="/"+name;
        LayoutBuilder.writeToFile(action, dir, true);
        postInfo("Saving Action : "+name);
    }

    /** save action panel. */
    private void saveAction(){
        if(editPanel == null || editVobj == null)
            return;
        saveObj();
        saveAction(editPanel);
        rmvEditItem(editPanel);
    }
    
    /** save a page. */
    private void restorePage(VObjIF obj){
        ParamLayout layout=getParamLayout(obj);
        if (layout == null)
            return;
        String pagename=defaultItemName(obj);
        
        String file=pagename;
        file+=".xml";

        String path=FileUtil.getLayoutPath(layout.getLayoutName(), file);
 
        if(path==null){
            Messages.postError("error restoring page "+file);
            return;
        }
        layout.reloadObject(obj, path, true,false);
    }
    /** save a page. */
    private void savePage(VObjIF obj, String pagename){
        ParamLayout layout=getParamLayout(obj);
        if (layout == null)
            return;

        LayoutBuilder.clrShufflerString();
        LayoutBuilder.setShufflerString(ETYPE,"pages");
        LayoutBuilder.setShufflerString(PTYPE,panelType);
        
        String file=pagename;
        LayoutBuilder.setShufflerString(NAME,file);
        file+=".xml";

        String dir= null;

        if(lastSaveIndex==SAVE_IN_LAYOUT)
            dir=editPanel.getSaveDir();
        else
            dir=editPanel.getDefaultDir();

        if(dir==null){
            Messages.postError("error saving page "+file);
            return;
        }
        String path=dir+"/"+file;

        obj.setAttribute(REFERENCE,FileUtil.fileName(pagename));
        obj.setAttribute(USEREF,"no");
        LayoutBuilder.writeToFile((JComponent)obj, path, false);
        obj.setAttribute(USEREF,"yes");
        updateLocator(file, path);
        ParamEditUtil.setChangedPage(false);
        changed_page=false;
        
        String name=layout.getPanelFile();

        if(name != null && lastSaveIndex==SAVE_IN_LAYOUT){  // changed_panel
            LayoutBuilder.clrShufflerString();
            LayoutBuilder.setShufflerString(ETYPE,"panels");
            LayoutBuilder.setShufflerString(PTYPE,panelType);
            LayoutBuilder.setShufflerString(NAME,panelName);
            String fname=name;
            String ext=FileUtil.fileExt(itemName);
            if(ext !=null)
                fname=FileUtil.fileName(name)+ext+".xml";
            path=dir+"/"+fname;
            postInfo("Saving Folder : "+fname);
            LayoutBuilder.writeToFile(layout, path, true);
        }
        postInfo("Saving Page : "+file);
    }
    /** save selected page. */
    private void savePage(){
        if(editPanel == null || editVobj == null)
            return;
        saveObj();
        itemName=itemEntry.getText().trim();
        savePage(editVobj,itemName);
        updateItemButtons();
        rmvEditItem(editVobj);
    }

    /** save an item. */
    private void saveItem(){
        if(editPanel == null || editVobj == null)
            return;
        saveObj();
        LayoutBuilder.clrShufflerString();
        if((editVobj instanceof VGroup) || (editVobj instanceof VTabbedPane))
            LayoutBuilder.setShufflerString(ETYPE,"groups");
        else
            LayoutBuilder.setShufflerString(ETYPE,"elements");
        LayoutBuilder.setShufflerString(PTYPE,panelType);
        itemName=itemEntry.getText().trim();
        String file=itemName;
        LayoutBuilder.setShufflerString(NAME,file);
        file+=".xml";
        String dir=FileUtil.savePath("PANELITEMS");
        String path=dir+"/"+file;
        postInfo("Saving Item : "+file);

        editVobj.setAttribute(REFERENCE,FileUtil.fileName(itemName));
        editVobj.setAttribute(USEREF,"no");
        LayoutBuilder.writeToFile((JComponent)editVobj, path, false);

        updateItemButtons();
 
        updateLocator(file, path);
    }

    /** save a ParamPanel. */
    private void saveParamPanel(){
        if(editPanel == null || editVobj == null)
            return;
        saveObj();
        itemName=itemEntry.getText().trim();
        String file=itemName;
        file+=".xml";
        String dir=FileUtil.savePath("INTERFACE");
        String path=dir+"/"+file;
        postInfo("Saving Panel : "+file);

        editVobj.setAttribute(REFERENCE,FileUtil.fileName(itemName));
        editVobj.setAttribute(USEREF,"no");
        LayoutBuilder.writeToFile((JComponent)editVobj, path, false);

        updateItemButtons();
     }

    /** save a tool panel. */
    private void saveToolPanel(){
        if (editPanel == null)
            return;
        String dir= getSavePath(itemName);

        if(dir==null){
            Messages.postError("error saving panel "+dir);
            return;
        }
        LayoutBuilder.writeToFile(editPanel, dir, true);
    }
    private void setDefaultItemName(){
        itemName=defaultItemName(editVobj);
        itemEntry.setText(itemName);
        updateItemButtons();
    }

    private boolean isTabGroup(VObjIF obj){
        if(obj !=null && obj instanceof VGroup){
            String label=obj.getAttribute(LABEL);
            if(label !=null && label.length()>0)
                return true;
        }
        return false;
    }
    private boolean isTabPage(VObjIF obj){
        if(obj !=null && obj instanceof VGroup){
            if(((VGroup)obj).isTabGroup())
                return true;
        }
        return false;
    }

    private boolean isActionPanel(VObjIF obj){
        if(obj instanceof ActionBar)
            return true;
        if(obj !=null && obj instanceof VGroup){
            Container p=((VGroup)obj).getParent();
            if(p !=null  && p instanceof ActionBar)
                return true;
        }
        return false;
    }
    private boolean isParamLayout(VObjIF obj){
        if(obj instanceof ParamLayout)
            return true;
        if(obj !=null && obj instanceof VGroup){
            Container p=((VGroup)obj).getParent();
            if(p !=null  && p instanceof ParamLayout)
                return true;
        }
        return false;
    }
    private boolean isLayoutPanel(VObjIF obj){
        if(isActionPanel(obj) || isParamLayout(obj))
            return true;
        return false;
    }
 
    private void reloadPanel(){
        if(editVobj==null || editPanel==null)
            return;
        if(itemName==null || itemName.length()==0)
            return;

//        String name=itemEntry.getText().trim();
//        String ext=FileUtil.fileExt(name);
//        if(ext!=null && ext.contains(".xml"))
//            FileUtil.setOpenAll(false);
//        name=name.replaceFirst(".xml","");
//        String fname=name+".xml";
//        String msgName=name;
        String path=getSelectedPath();
        //String path=FileUtil.openPath("INTERFACE/"+fname);
        VObjIF vobj=null;
        if(path!=null){
            postInfo("Reloading : "+path);                 
            vobj=editPanel.reloadContent(path);
        }
        if(path==null || vobj==null){
            Messages.postError("could not load "+path);
        }
        setEditElement(vobj);
        editVobj=vobj;
        FileUtil.setOpenAll(true);       
    }
    
     private void loadItem(){
        if(editVobj==null || editPanel==null)
            return;
        if(itemName==null || itemName.length()==0)
            return;
        String path=null;

        String name=itemEntry.getText().trim();
        String ext=FileUtil.fileExt(name);
        if(ext!=null && ext.contains(".xml"))
            FileUtil.setOpenAll(false);
        name=name.replaceFirst(".xml","");
        String fname=name+".xml";
        String msgName=name;
        boolean tabpage=isTabPage(editVobj);
        boolean actionpanel=isActionPanel(editVobj);
        if(tabpage||(editVobj==editPanel)||actionpanel)
            path=FileUtil.getLayoutPath(editPanel.getLayoutName(),fname);
        
        if(path==null)
            path=FileUtil.openPath("PANELITEMS/"+fname);
        
        if(path!=null){
            postInfo("Reloading : "+path);
            if(editVobj==editPanel){
                editPanel.restore();
            }
            else{
                boolean expand_page=false;
                if(tabpage ){
                    String oldref=editVobj.getAttribute(REFERENCE);
                    if(!name.equals(oldref))
                        expand_page=true;
                }                       
                VObjIF vobj=editPanel.reloadObject(editVobj,path,expand_page,true);
                if(vobj!=editVobj)
                    setEditElement(vobj);
                editVobj=vobj;
                if(vobj !=null){
                    if(tabpage){
                        vobj.setAttribute(USEREF,"yes");
                        setChangedPage(false);
                    }
                    else
                        vobj.setAttribute(USEREF,"no");
                 }
            }
        }
        else{
            Messages.postError("could not load "+msgName);
        }
        itemEntry.setText(name);
        FileUtil.setOpenAll(true);
        
    }
    
    private String getSelectedPath(){
        String spath=null;
        switch(selectedType){
        case ParamInfo.PARAMLAYOUT:
        case ParamInfo.ACTION:
        case ParamInfo.PAGE:
        case ParamInfo.TOOLPANEL:
            spath=getSourcePath(itemName);
            break;
        case ParamInfo.PARAMPANEL:
            spath=FileUtil.openPath("INTERFACE/"+itemName+".xml");
            break;
        case ParamInfo.ITEM:
            spath=FileUtil.openPath("PANELITEMS/"+itemName+".xml");
            break;
        }
        //if(spath!=null)
            return spath;
        //return getSavePath(itemName);
    }
    private void revertItem(){
        String spath=getSelectedPath();       
        if(spath==null || !spath.contains(FileUtil.usrdir()))
            return;
        File file=new File(spath);
        file.delete();
        switch(selectedType){
        case ParamInfo.PARAMPANEL:
        case ParamInfo.TOOLPANEL:
            reloadPanel();
            break;
        }
        updateItemButtons();
    }
    private boolean userFileExists(){
        String spath=getSelectedPath();
        if(spath==null)
            return false;
        return spath.contains(FileUtil.usrdir());
    }
    private void updateItemButtons(){
        String itemStr="";
        String spath=null;
        switch(selectedType){
        case ParamInfo.PARAMLAYOUT:
            clearButton.setEnabled(true);
            saveButton.setEnabled(true);
            spath=getSourcePath(itemName);
            if(spath!=null&&FileUtil.pathName(spath).equals(fileName))
                reloadButton.setEnabled(true);
            else    
                reloadButton.setEnabled(false);
            itemStr=Util.getLabel("peFolder");
            break;
        case ParamInfo.TOOLPANEL:
            clearButton.setEnabled(true);
            saveButton.setEnabled(true);
            spath=getSourcePath(itemName);

            if(spath!=null&&FileUtil.pathName(spath).equals(fileName))
                reloadButton.setEnabled(true);
            else    
                reloadButton.setEnabled(false);
            if(spath==null)
                spath=getSavePath(itemName);
            itemStr=Util.getLabel("ToolPanel");
            break;
         case ParamInfo.ACTION:
            clearButton.setEnabled(true);
            saveButton.setEnabled(true);
            spath=getSourcePath(itemName);
            if(spath!=null&&FileUtil.pathName(spath).equals(fileName))
                reloadButton.setEnabled(true);
            else    
                reloadButton.setEnabled(false);
            itemStr=Util.getLabel("Action");
            break;
        case ParamInfo.PAGE:
            spath=getSourcePath(itemName);
            itemStr=Util.getLabel("pePage");
            if(editVobj==null)
                clearButton.setEnabled(false);
            else
                clearButton.setEnabled(true);
            if(tabgrp != null){
                if(isTabGroup(editVobj))
                    tabgrp.setEnabled(true);
                else
                    tabgrp.setEnabled(false);
            }

            boolean b=(editVobj!=null && itemName!=null && itemName.length()>0);
            
            reloadButton.setEnabled(hasReference(editVobj));
            saveButton.setEnabled(b);
            break;
        case ParamInfo.PARAMPANEL:
            clearButton.setEnabled(true);
            saveButton.setEnabled(true);
            spath=getSourcePath(itemName);
            reloadButton.setEnabled(true);
            break;
        }
        itemLabel.setText(itemStr);
        //spath=getSourcePath(itemName);
        if(spath==null)
            itemSource.setText("");
        else
            itemSource.setText(spath);

        if(isNewType)
            updateSaveDir();
        revertButton.setEnabled(userFileExists());
        getEditItems();
    }

    private void makeSaveDirMenu(){
        if (editPanel == null)
            return;
        inAddMode=true;
        saveDirectory.removeAllItems();
        saveDirectory.addItem(editPanel.getLayoutName());
        saveDirectory.addItem(editPanel.getDefaultName());
        saveDirectory.setSelectedIndex(lastSaveIndex);
        inAddMode=false;
    }

    private void updateSaveDir(){
        inAddMode=true;
        switch(selectedType){
        case ParamInfo.PARAMLAYOUT:
        case ParamInfo.PAGE:
        case ParamInfo.ACTION:
            InLabel.setVisible(true);
            saveDirectory.setSelectedIndex(lastSaveIndex);
            saveDirectory.setVisible(true);
            break;
        case ParamInfo.PARAMPANEL:
        case ParamInfo.ITEM:
        case ParamInfo.TOOLPANEL:
            saveDirectory.setVisible(false);
            InLabel.setVisible(false);
            break;
        }
        inAddMode=false;
    }
    private void savePrefAttr() {
        sshare = Util.getSessionShare();
        if (sshare != null) {
            sshare.userInfo().put("snap", "on");
            sshare.userInfo().put("snapSize", snapSize.getText());
            if(ParamEditUtil.getAutoScroll())
                sshare.userInfo().put("autoscroll", "true");
            else
                sshare.userInfo().put("autoscroll", "false"); 
            sshare.userInfo().put("savepolicy",Integer.toString(save_policy));
        }
    }

    private void initPanel() {
        sshare = Util.getSessionShare();
        if (sshare != null) {
            String data = (String) sshare.userInfo().get("snap");
            data = (String) sshare.userInfo().get("snapSize");
            if (data == null)
                data = "5";
            try {
                Integer.parseInt(data);
            }
            catch (NumberFormatException e) {
                data = "5";
            }
            snapSize.setText(data);
            data=(String) sshare.userInfo().get("autoscroll");
            if(data !=null && data.equals("true"))
                ParamEditUtil.setAutoScroll(true);
            else
                ParamEditUtil.setAutoScroll(false);
            autoscroll.setSelected(ParamEditUtil.getAutoScroll());
            data = (String) sshare.userInfo().get("savepolicy");
            if(data!=null)
                save_policy=Integer.parseInt(data);
            switch(save_policy){
            case USER_SAVE: usersaveButton.setSelected(true); break;
            case WARN_SAVE: warnsaveButton.setSelected(true); break;
            case AUTO_SAVE: autosaveButton.setSelected(true); break;
            }
        }
    }

    private void adjustAttrPanelSize() {
        Dimension dim;
        int   h = 0;
        int   w = 0;
        int   k;
        int   n = attrPan.getComponentCount();
        for ( k = 0; k < n; k++) {
            Component m = attrPan.getComponent(k);
            dim = m.getPreferredSize();
            h += dim.height+2;
        }
        w = windowWidth - 8;
        if (h > scrollH)
            w = w - sbW;
        attrPan.setPreferredSize(new Dimension(w, h));
        attrPan.repaint();
    }

    private JButton addButton(String label, Container p, Object obj, String cmd)
    {
        JButton but = new JButton(label);
        but.setActionCommand(cmd);
        but.addActionListener((ActionListener) obj);
        but.setFont(font);
        p.add(but);
        return but;
    }

    private void setObjAttr(int attr, int id) {
        if (editVobj == null)
            return;
        String data = txts[id].getText().trim();
        if (data == null || data.length() < 1)
            editVobj.setAttribute(attr, null);
        else
            editVobj.setAttribute(attr, data);
    }

    private void comboAction(ActionEvent e) {
        if (editVobj != null) {
            String cmd = e.getActionCommand();
            JComboBox combo = (JComboBox)e.getSource();
            String sel = (String)combo.getSelectedItem();
            try {
                int attr = Integer.parseInt(cmd);
                editVobj.setAttribute(attr, sel);
            } catch (NumberFormatException exception) {
                Messages.postError("ParamInfoPanel.comboAction");
            }
            //setItemEdited(editVobj);
        }
    }

    private void initAction() {
        txts[CHOICE].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(SETCHOICE, CHOICE);
            }
        });
        txts[CHVALS].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(SETCHVAL, CHVALS);
            }
        });
        txts[VAR].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(VARIABLE, VAR);
            }
        });
        txts[CMD_1].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(CMD, CMD_1);
            }
        });
        txts[CMD_2].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(CMD2, CMD_2);
            }
        });
        txts[IVAL].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(SETVAL, IVAL);
            }
        });
        txts[SHOWX].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(SHOW, SHOWX);
            }
        });
        txts[ILABEL].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(LABEL, ILABEL);
            }
        });
        txts[DIGITS].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(NUMDIGIT, DIGITS);
            }
        });

        txts[STATVARS].addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                   setObjAttr(STATPAR, STATVARS);
            }
        });

        snapSize.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                String s = snapSize.getText().trim();
                if (s.length() < 1)
                    s = "0";
                int k = 1;
                try {
                    k = Integer.parseInt(s);
                }
                catch (NumberFormatException e) {
                    k = 0;
                }
                if (k < 1) {
                    snapSize.setText("5");
                    k = 5;
                }
                if (editPanel != null)
                   editPanel.setSnapGap(k);
                ParamEditUtil.setSnapGap(k);
                if(editor !=null)
                    editor.setSnapGrid(k);
            }
        });
        locX.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if (editVobj == null)
                    return;
                saveObj();
                setObjectGeometry();
            }
        });
        locY.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if (editVobj == null)
                    return;
                saveObj();
                setObjectGeometry();
            }
        });
        locW.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if (editVobj == null)
                    return;
                saveObj();
                setObjectGeometry();
            }
        });
        locH.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if (editVobj == null)
                    return;
                saveObj();
                setObjectGeometry();
            }
        });
        editableOn.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if (editVobj != null)
                   editVobj.setAttribute(EDITABLE, "yes");
            }
        });
        editableOff.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                if (editVobj != null)
                   editVobj.setAttribute(EDITABLE, "no");
            }
        });
    }

    //================ private classes ==================================

    private class SaveWarnDialog extends ModelessDialog 
    implements ActionListener 
    {
        /**
         * 
         */
        private static final long serialVersionUID = 1L;
        private JButton saveButton=new JButton("Save");
        private JButton restoreButton=new JButton("Restore");
        private JPanel panel=null;
        protected TableCellRenderer m_messageCellRenderer;
        private JScrollPane scrollPane = null;
        EasyJTable m_table=null;

        public SaveWarnDialog(String title){
            super(title);
            setVisible(false);
            panel=new JPanel();
            panel.setLayout(new BorderLayout());
            panel.setPreferredSize(new Dimension(200,200));
            
            m_table = new EasyJTable(new DefaultTableModel(0, 1));
            
            scrollPane = new JScrollPane(m_table);
            m_messageCellRenderer = new MessageCellRenderer();
            m_table.setDefaultRenderer(m_table.getColumnClass(0),
                                       m_messageCellRenderer);
            m_table.setDefaultEditor(m_table.getColumnClass(0), null);
            m_table.setTableHeader(null);
            m_table.setShowGrid(false);
            m_table.setRowMargin(0);
            int[] policy =
                {REASON_INITIAL_VIEW | STATE_ANY | ACTION_SCROLL_TO_BOTTOM,
                 REASON_ANY | STATE_BOTTOM_SHOWS | ACTION_SCROLL_TO_BOTTOM};
            m_table.setScrollPolicy(policy);
           
            panel.add(scrollPane,BorderLayout.CENTER);

            add(panel);

            saveButton.setEnabled(true);
            saveButton.setActionCommand("Save");
            saveButton.addActionListener(this);
            buttonPane.add(saveButton, 1);            

            restoreButton.setEnabled(true);
            restoreButton.addActionListener(this);
            restoreButton.setActionCommand("Revert");
            buttonPane.add(restoreButton, 2);            
            
            closeButton.setActionCommand("Close");
            closeButton.addActionListener(this);        
 
            setCloseEnabled(true);
            setUndoEnabled(false);
            setAbandonEnabled(false);
            historyButton.setEnabled(false);

            pack();
        }
        public void setList(ArrayList<VObjIF> list){
            DefaultTableModel tableModel = (DefaultTableModel)m_table.getModel();
            tableModel.setRowCount(0);
            int nLength = list.size();
            for (int i = 0; i < nLength; i++)
            {
                VObjIF[] obj = {(VObjIF)list.get(i)};
                m_table.addRow(obj, false);
            }
            // m_table.positionScroll(REASON_INITIAL_VIEW);
            if (nLength > 1)
               m_table.scrollToRowLater(nLength - 1);
        }

        @Override
        public void actionPerformed(ActionEvent evt) {
            String cmd = evt.getActionCommand();
            if (cmd.equals("Close")) {
                this.setVisible(false);
            }
            if (cmd.equals("Revert")) {
                saveObj();
                ArrayList<VObjIF> objs=new ArrayList<VObjIF>();
                for (int i = 0; i < edits.size(); i++) {
                    VObjIF obj = edits.get(i);
                    if(m_table.isRowSelected(i))
                        objs.add(obj);
                }
                for (int i = 0; i < objs.size(); i++) {
                    VObjIF obj = objs.get(i);
                    if (obj instanceof ParamPanel)
                        ((ParamPanel)obj).restore();
                    else{
                        VObjIF p=getParamLayout(obj);
                        if(p !=null)
                            restorePage(obj);
                    }
                    edits.remove(obj);
                }

                showEdits();
                if(edits.size()==0)
                    setVisible(false);
            }
            if (cmd.equals("Save")) {
                saveObj();
                ArrayList<VObjIF> objs=new ArrayList<VObjIF>();
                for (int i = 0; i < edits.size(); i++) {
                    VObjIF obj = edits.get(i);
                    if(m_table.isRowSelected(i))
                        objs.add(obj);
                }
                for (int i = 0; i < objs.size(); i++) {
                    VObjIF obj = objs.get(i);
                    if (obj instanceof ActionBar)
                        saveAction(obj);
                    else if (obj instanceof ParamLayout)
                        saveLayout(obj);
                    else{
                        String name=defaultItemName(obj);
                        savePage(obj,name);
                    }
                    edits.remove(obj);
                }
                showEdits();
                if(edits.size()==0)
                    setVisible(false);
            }
        }
        public void updateButtons(){
            if(edits.size()>0){
                saveButton.setEnabled(true);
                undoButton.setEnabled(true);
            }
            else{
                saveButton.setEnabled(false);
                undoButton.setEnabled(false);                
            }
        }
        public void showEdits(){
            setList(edits);
            m_table.selectAll();
            updateButtons();
            if(edits.size()>0)
                setVisible(true);
            //else
            //    setVisible(false);
        }
        class MessageCellRenderer extends JTextField
        implements TableCellRenderer {
        private static final long serialVersionUID = 1L;

        public MessageCellRenderer() {
            setOpaque(true);
            setPreferredSize(new Dimension(100,20));
        }

        public Component getTableCellRendererComponent(
            JTable table,
            Object value,
            boolean isSelected,
            boolean cellHasFocus, int row, int column)
        {
            setOpaque(true);
            setBorder(BorderFactory.createEtchedBorder());
            String panelname="";
            String msg;
            VObjIF obj = (VObjIF)value;
            if (obj instanceof ActionBar){
                panelname=((ActionBar)obj).getPanelName();
                msg = panelname +" Action Panel\n";
            }
            else if (obj instanceof ParamLayout) {
                panelname=((ParamLayout)obj).getPanelName();
                msg=panelname +" Page List\n";
            }
            else{
                ParamLayout p=getParamLayout(obj);
                if(p!=null)
                    panelname=((ParamLayout)p).getPanelName();
                msg=panelname +" Page <"+obj.getAttribute(LABEL)+">\n";
            }
            if(isSelected)
                selectAll();
           // setTabSize(2);
            setText(msg);
            
            String var ="Info";
            Color c=DisplayOptions.getColor(var);
            Color bg = DisplayOptions.getColor(var + "Bkg");
            Color curbg = bg;
            Color curfg;

            if (bg == null) {
                bg=Util.getBgColor();
            }
            if (c == null) {
                c = Util.getContrastingColor(bg);
            }
            setForeground(c);
            curfg = c;
            setBackground(bg);
            curbg = bg;
            
            Font f=DisplayOptions.getFont(var,var,var);
            setFont(f);
            if (isSelected) {
                Color[] fgbg = Util.getSelectFgBg(curfg, curbg);
                setForeground(fgbg[0]);
                setBackground(fgbg[1]);
            }

            int width = m_table.getColumnModel().getColumn(column).getWidth();
            setSize(width, 1000);

            int height = m_table.getRowHeight(row);
            double rowheight = getPreferredSize().getHeight();
            if (height != rowheight)
                m_table.setRowHeight(row, (int)rowheight);

            return this;
        }
    }  // class MessageCellRenderer       
    }
    
    private class ItemEditor extends ModelessDialog 
        implements ActionListener 
    {
        /**
         * 
         */
        private static final long serialVersionUID = 1L;
        ParamPanel layout = null;
        private JScrollPane scrollPane;
        private String itemEditorGeometryFile = "ItemEditorGeometry";
        private String itemEditorContentFile="ItemEditorContent";
        private JToggleButton testButton=new JToggleButton("Test");
        private JButton openButton=new JButton("Open..");
        private JButton saveButton=new JButton("Save..");
        private String mTitle;
        
        public ItemEditor(String title) {
            super(title);
            setVisible(false);
            mTitle=title;

            historyButton.setEnabled(false);
            undoButton.setEnabled(false);
            
            testButton.setEnabled(true);
            testButton.setSelected(false);
            
            testButton.setActionCommand("test");
            testButton.addActionListener(this);
            buttonPane.add(testButton, 1);

            saveButton.setEnabled(true);
            saveButton.setActionCommand("save");
            saveButton.addActionListener(this);
            buttonPane.add(saveButton, 2);
            
            openButton.setActionCommand("open");
            openButton.addActionListener(this);

            buttonPane.add(openButton, 3);

            closeButton.setActionCommand("Close");
            closeButton.addActionListener(this);
            
            abandonButton.setActionCommand("Abandon");
            abandonButton.addActionListener(this);
            
            helpButton.setActionCommand("help");
            helpButton.addActionListener(this);

            layout = new ParamPanel();//(Util.getSessionShare(), getExpIF());
            //layout.setUseMenu(false);
            layout.setPanelName("Items");
            Dimension dim=new Dimension(400,300);
            layout.setPreferredSize(dim);
            layout.setSnapGap(ParamEditUtil.getSnapGap());
            layout.setGridColor(Util.getGridColor());
            //setEditMode(true);
            scrollPane = new JScrollPane(layout,
                    JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                    JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
            add(scrollPane);
                        setCloseEnabled(true);
                        setAbandonEnabled(true);
            pack();
            restoreGeometry();
            Component comp=restoreContent();
            setEditMode(true);
            layout.adjustSize();

            setContentVisible();
            if(comp !=null)
                comp.setVisible(true);    

            addGeometryListener();
            
            setVisible(true);
        }

        public void setSnapGrid(int k) {
            layout.setSnapGap(k);
        }

        private void setContentVisible() {
            // setEditMode should set all top level groups visible but for some reason doesn't
            int k = layout.getComponentCount();
            for (int i = 0; i < k; i++) {
                Component comp = layout.getComponent(i);
                comp.setVisible(true);
            }
        }

        public void saveContent() {
            String filepath = FileUtil.savePath("USER/PERSISTENCE/"+itemEditorContentFile);
            if(filepath !=null){
                File tmpFile=new File(filepath);
                String f = tmpFile.getAbsolutePath();
                layout.setAttribute(USEREF,"false");
                ParamEditUtil.clearDummyObj(); // remove selection outline objects
                LayoutBuilder.writeToFile(layout, f,true);
            }
        }

        public Component restoreContent(){
            String filepath = FileUtil.openPath("USER/PERSISTENCE/"+itemEditorContentFile);
            if(filepath==null)
                return null;
            try{
                Component comp=LayoutBuilder.build(layout, getExpIF(), filepath, true);
                return comp;
            }
            catch(Exception be) {
                Messages.writeStackTrace(be);
                return null;
            }
        }
        public void setEditMode(boolean s) {
            layout.setEditMode(s);
            layout.setEditModeToAll(s);
        }

        @Override
        public void actionPerformed(ActionEvent evt) {
            String cmd = evt.getActionCommand();
            if (cmd.equals("Close")) {
                this.setVisible(false);
            }
            if (cmd.equals("Abandon")) {
                abandon();
            }
            if (cmd.equals("save")) {
                save();
            }
            if (cmd.equals("open")) {
                open();
            }
            if (cmd.equals("test")) {
                test();
            }
        }
        
        private void test() {
            if(testButton.isSelected())
                layout.setEditModeToAll(false);
            else
                layout.setEditModeToAll(true);
            layout.validate();
            layout.repaint();
        }

        private void save() {
            JFrame frame= new JFrame();
            frame.setTitle("Save Items");       
            JFileChooser fc= new JFileChooser();            
            fc.setFileSelectionMode(JFileChooser.FILES_ONLY);
            fc.setAcceptAllFileFilterUsed(false);
            fc.addChoosableFileFilter(new XMLFilter());

            if(savepath!=null)
                fc.setSelectedFile(new File(savepath));
            int returnVal= fc.showSaveDialog(frame);            
            if(returnVal==JFileChooser.APPROVE_OPTION){
                File file= fc.getSelectedFile();
                savepath=file.getPath();
                if(openpath==null)
                    openpath=savepath;
                layout.setAttribute(USEREF,"false");
                ParamEditUtil.clearDummyObj(); // removes selection outline objects
                LayoutBuilder.writeToFile(layout, savepath,true);
                setTitle(mTitle+"-"+file.getName());
            }
        }
        private void open() {
            JFrame frame= new JFrame();
            frame.setTitle("Open Items");       
            JFileChooser fc= new JFileChooser();            
            fc.setFileSelectionMode(JFileChooser.FILES_ONLY);
            fc.setAcceptAllFileFilterUsed(false);
            fc.addChoosableFileFilter(new XMLFilter());

            if(openpath!=null)
                fc.setSelectedFile(new File(openpath));
            int returnVal= fc.showOpenDialog(frame);            
            if(returnVal==JFileChooser.APPROVE_OPTION){
                File file= fc.getSelectedFile();
                openpath=file.getPath();
                if(savepath==null)
                    savepath=openpath;
                try{
                    setTitle(mTitle+"-"+file.getName());
                    layout.removeAll();
                    LayoutBuilder.build(layout, getExpIF(), openpath, true);
                    layout.setEditModeToAll(true);
                    setContentVisible();
                    layout.adjustSize();
                    layout.validate();
                    layout.repaint();
                }
                catch(Exception be) {
                    Messages.writeStackTrace(be);
                }
            }
        }
        private void abandon() {
            layout.removeAll();
            restoreContent();
            layout.setEditModeToAll(true);
            setContentVisible();
            this.setVisible(false);
        }

        private ButtonIF getExpIF() {
            return Util.getViewArea().getDefaultExp();
        }

        public void addGeometryListener() {
            addComponentListener(new ComponentAdapter() {
                public void componentResized(ComponentEvent ce) {
                    changedSize();
                }
                public void componentMoved(ComponentEvent ce) {
                    saveGeometry();
                }
            });
        }

        public void changedSize() {
            layout.adjustSize();
            FileUtil.writeGeometryToPersistence(itemEditorGeometryFile, this);
        }

        public void saveGeometry() {
            FileUtil.writeGeometryToPersistence(itemEditorGeometryFile, this);
        }

        public void restoreGeometry() {
            FileUtil.setGeometryFromPersistence(itemEditorGeometryFile, this);          
        }
        public class XMLFilter extends FileFilter {          
            public boolean accept(File f) {
                if (f.isDirectory()) {
                    return true;
                }        
                String extension = getExtension(f);
                if (extension != null) {
                    if (extension.equals("xml")){
                        return true;
                    } else {
                        return false;
                    }
                }        
                return false;
            }        
            public String getExtension(File f) {
                String ext = null;
                String s = f.getName();
                int i = s.lastIndexOf('.');
         
                if (i > 0 &&  i < s.length() - 1) {
                    ext = s.substring(i+1).toLowerCase();
                }
                return ext;
            }
            public String getDescription() {
                return "XML Files";
            }
        }
    }

    private class RadioGroup extends JPanel implements ActionListener {
        private static final long serialVersionUID = 1L;
        ButtonGroup group=null;
        private boolean first=true;
        int attr;
        RadioGroup(int a){
            group=new ButtonGroup();
            attr=a;
            setLayout(new SimpleH2Layout(SimpleH2Layout.LEFT, 4, 2, false));
        }
        public void add(String s){
            JRadioButton button=new JRadioButton(s);
            if(first)
                button.setSelected(true);
            group.add(button);
            super.add(button);
            button.setOpaque(false);
            button.setBackground(null);
            button.setActionCommand(s);
            button.addActionListener(this);
            button.setRequestFocusEnabled(false);
            first=false;
        }
        public void setSelectedItem(String s){
            Enumeration elist=group.getElements();
            JRadioButton selected=null;
            while(elist.hasMoreElements()){
                JRadioButton button=(JRadioButton)elist.nextElement();
                if(s.equals(button.getActionCommand()))
                    selected=button;
                button.setSelected(false);
            }
            if(selected !=null)
                selected.setSelected(true);
            else
                Messages.postError("illegal attribute choice "+s);
        }
        public String getSelectedItem(){
            String s="";
            if(group.getSelection()!=null)
                s= group.getSelection().getActionCommand();
            return s;
        }
        public void actionPerformed(ActionEvent evt) {
            if (editVobj != null) {
                String str=getSelectedItem();
                if(attr==TAB){
                   VObjIF obj=editVobj;
                   if(str.equals("yes")) 
                       ParamEditUtil.unParent();
                   editVobj.setAttribute(USEREF,str);
                   editVobj.setAttribute(TAB,str);
                   editVobj=null;  // force setEditElement to reload object
                   setEditElement(obj);
                }
                ParamEditUtil.setChangedPage(true);
                editVobj.setAttribute(attr,  str);
            }
        }
        public void setEnabled(boolean enabled){
            Component[] comps = getComponents();
            int nSize = comps.length;
            for(int j=0;j<nSize;j++){
                comps[j].setEnabled(enabled);
            }
            super.setEnabled(enabled);
        }

    }  // class RadioGroup

    private class AttributeChangeListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            VObjIF vobj = editVobj;
            if(vobj == null)
                vobj = editPanel;
            if(vobj == null)
                return;
            if(vobj==editVobj)
                ParamEditUtil.setChangedPage(true);
            String cmd = e.getActionCommand();
            JTextField obj = (JTextField) e.getSource();
            try {
                int attr = Integer.parseInt(cmd);
                vobj.setAttribute(attr, obj.getText());

                if(vobj instanceof ParamPanel){
                    makeSaveDirMenu();
                }

                else if((vobj instanceof VGroup) && (attr == LABEL || attr == TAB)) {
                    setDefaultItemName();
                    String ref = vobj.getAttribute(REFERENCE);
                    if(ref == null || ref.length() == 0)
                        vobj.setAttribute(REFERENCE, itemName);
                }

                if (vobj instanceof VGroup && attr == COUNT)
                {
                    ((VGroup)vobj).showPanel();
                }
                setItemEdited(editVobj);


            }   catch(NumberFormatException exception) {
                Messages.postError("ParamInfoPanel number format exception");
            }
        }

    } //class AttributeChangeListener

    private class AttrLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}
        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int   w = 0;
            int   h = 0;
            int   k;
            int   n = target.getComponentCount();
            for ( k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height+2;
            }
            return new Dimension(w, h);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim;
                int   n = target.getComponentCount();
                int   w = 0;
                int   h = 4;
                int   k;

                for ( k = 0; k < n; k++) {
                    Component m = target.getComponent(k);
                    if(m instanceof Container)
                        m=((Container)m).getComponent(0);
                    dim = m.getPreferredSize();
                    if (dim.width > w)
                        w = dim.width;
                }

                SimpleH2Layout.setFirstWidth(w);

                Dimension dim0 = target.getSize();
                for (k = 0; k < n; k++) {
                    Component obj = target.getComponent(k);
                    if (obj.isVisible()) {
                        dim = obj.getPreferredSize();
                        obj.setBounds(2, h, dim0.width-6, dim.height);
                        h += dim.height;
                    }
                }
            }
        }
    } // class AttrLayout

    private class PanelLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        public Dimension preferredLayoutSize(Container target) {
            Dimension dim;
            int w = 0;
            int h = 0;
            int k;
            int n = target.getComponentCount();
            for (k = 0; k < n; k++) {
                Component m = target.getComponent(k);
                dim = m.getPreferredSize();
                if (dim.width > w)
                    w = dim.width;
                h += dim.height + 2;
            }
            return new Dimension(w, h);
        }

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(0, 0); // unused
        }

        public void layoutContainer(Container target) {
            synchronized (target.getTreeLock()) {
                Dimension dim;
                int n = target.getComponentCount();
                int h = 0;
                int hs = 0;
                int k;

                for (k = 0; k < n; k++) {
                    Component m = target.getComponent(k);
                    dim = m.getPreferredSize();
                    if (m != attrScroll)
                        hs += dim.height;
                }
                if (hs > 500) // something wrong
                    hs = etcH;
                else
                    etcH = hs;

                Dimension dim0 = target.getSize();
                windowWidth = dim0.width;
                hs = dim0.height - hs - 4;
                if (hs < 20)
                    hs = 20;
                scrollH = hs;
                if (sbW == 0) {
                    JScrollBar jb = attrScroll.getVerticalScrollBar();
                    if (jb != null) {
                        dim = jb.getPreferredSize();
                        sbW = dim.width;
                    } else
                        sbW = 20;
                }
                /*
                 * if (editVobj != null) adjustAttrPanelSize();
                 */

                for (k = 0; k < n; k++) {
                    Component obj = target.getComponent(k);
                    if (obj.isVisible()) {
                        dim = obj.getPreferredSize();
                        if (obj == attrScroll) {
                            obj.setBounds(2, h, dim0.width - 4, hs);
                            h += hs;
                        } else {
                            if (dim.height > 500) {
                                if (obj == headerPan)
                                    dim.height = 100;
                                else if (obj == prefPan)
                                    dim.height = 40;
                                else if (obj == ctlPan)
                                    dim.height = 40;
                            }
                            obj.setBounds(2, h, dim0.width - 4, dim.height);
                            h += dim.height;
                        }
                    }
                }
            }
        }
    } // class PanelLayout
    
    private class StyleChooser extends JComboBox implements ActionListener {
        private static final long serialVersionUID = 1L;
        private String sel = null;
        Hashtable types = new Hashtable();
        private VObjIF sobj = null;
        private boolean color_only = false;
        private boolean style_only = false;
        Color orgBg;

        boolean inAddMode = true;

        public StyleChooser(String type) {
            inAddMode = true;
            orgBg = getBackground();
            setOpaque(true);
            if (type.equals("color"))
                setColorOnly(true);
            else if (type.equals("style"))
                setStyleOnly(true);
            ArrayList list;
            if(style_only)
                list=DisplayOptions.getTypes(DisplayOptions.FONT);
            else
                list= DisplayOptions.getSymbols();
            int nLength = list.size();
            for (int i = 0; i < nLength; i++) {
                String s = (String) list.get(i);
                addItem(s);
                types.put(s, s);
            }
            if (color_only) {
                addItem("transparent");
                types.put("transparent", "transparent");
            }
            addActionListener(this);
            setRenderer(new StyleMenuRenderer());
            setPreferredSize(new Dimension(150, 22));
            //setOpaque(false);
            inAddMode = false;
        }

        public void actionPerformed(ActionEvent evt) {
            if (inAddMode)
                return;
            sel = (String) getSelectedItem();
            StyleData style = new StyleData(sel);
            setFont(style.font);
            //setBackground(style.bg);
            setForeground(style.fg);
            if (!inSetMode && sobj != null)
                setStyle(sobj);
            repaint();
        }

        public void setColorOnly(boolean c) {
            color_only = c;
        }
        public void setStyleOnly(boolean c) {
            style_only = c;
        }

        public void setStyleObject(VObjIF v) {
            sobj = v;
        }

        public String getType() {
            return (String) getSelectedItem();
        }

        public void setType(String s) {
            setSelectedItem(s);
        }

        public void setDefaultType() {
            if (getItemCount() < 1)
                return;
            setSelectedIndex(0);
        }

        public boolean isType(String s) {
            return types.containsKey(s);
        }

        public void setStyle(VObjIF obj) {
            if (obj == null)
                return;
            sel = (String) getSelectedItem();
            obj.setAttribute(FONT_STYLE, sel);
            obj.setAttribute(FGCOLOR, sel);
            obj.changeFont();
        }

        public void setColor(VObjIF obj) {
            if (obj == null)
                return;
            sel = (String) getSelectedItem();
            obj.setAttribute(FGCOLOR, sel);
        }

        class StyleData {
            public Color fg;

            public Color bg;

            public Font font;

            public StyleData(String sel) {
                if (color_only) {
                    font = DisplayOptions.getFont(null, null);
                    bg = DisplayOptions.getColor(sel);
                    if (intensity(bg) <= 1)
                        fg = Color.white;
                    else
                        fg = Color.black;
                } else {
                    font = DisplayOptions.getFont(sel, sel);
                    fg = DisplayOptions.getColor(sel);
                    bg = orgBg;
                    if (contrast(fg, bg) < 0.3) {
                        bg = DisplayOptions.getColor(sel);
                        if (intensity(bg) <= 1)
                            fg = Color.white;
                        else
                            fg = Color.black;
                    }
                }
            }

            double intensity(Color c) {
                if (c == null) {
                    return Double.NaN;
                }
                float f[] = new float[3];
                c.getColorComponents(f);
                double r = f[0];
                double g = f[1];
                double b = f[2];
                double mag = (r * r + g * g + b * b);
                return mag;
            }

            double contrast(Color c1, Color c2) {
                if (c1 == null || c2 == null) {
                    return Double.NaN;
                }
                float f1[] = new float[3];
                float f2[] = new float[3];
                c1.getColorComponents(f1);
                c2.getColorComponents(f2);
                double r = f1[0] - f2[0];
                double g = f1[1] - f2[1];
                double b = f1[2] - f2[2];
                double mag = (r * r + g * g + b * b);
                return mag;
            }
        }

        // cell renderer for menu items

        class StyleMenuRenderer extends JLabel implements ListCellRenderer {
            private static final long serialVersionUID = 1L;

            public StyleMenuRenderer() {
                setOpaque(true);
            }

            public Component getListCellRendererComponent(JList list,
                    Object value, int index, boolean isSelected,
                    boolean cellHasFocus) {
                if (value == null) {
                    setText(" ");
                    return this;
                }
                String sel = value.toString();
                StyleData style = new StyleData(sel);
                setText(sel);
                setFont(style.font);
                setBackground(style.bg);
                setForeground(style.fg);
                return this;
            }
        } // class StyleMenuRenderer
    } // class StyleChooser
}

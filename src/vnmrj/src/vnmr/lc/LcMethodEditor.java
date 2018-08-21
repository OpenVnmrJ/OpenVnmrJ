/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc;

import javax.swing.border.Border;
import javax.swing.event.*;
import javax.swing.filechooser.FileFilter;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.TableCellEditor;
import javax.swing.table.TableCellRenderer;
import javax.swing.*;

import java.awt.*;
import java.awt.event.*;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.*;
import java.util.regex.Pattern;

import static javax.swing.event.TableModelEvent.HEADER_ROW;

import vnmr.ui.ExpPanel;
import vnmr.ui.VNMRFrame;
import vnmr.util.*;
import vnmr.bo.*;

import static javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_AS_NEEDED;
import static javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED;
import static javax.swing.ScrollPaneConstants.HORIZONTAL_SCROLLBAR_NEVER;

import static vnmr.bo.VObjDef.*;

/**
 * This class holds a panel used for editing an LC run method.
 */
public class LcMethodEditor
    extends ModelessDialog
    implements ActionListener, ComponentListener, ChangeListener {

    /** The name of the persistence file for FileChooser dialog. */
    public static final String CHOOSER_PERSISTENCE = "LcMethodFileChooser";

    /** The name of the persistence file for this popup. */
    private static final String PERSISTENCE_FILE = "LcMethodEditor";

    /** Option for the "checkForChanges" popup. */
    private static final String SAVE_OPTION = "Save";
    /** Option for the "checkForChanges" popup. */
    private static final String SAVEAS_OPTION = "Save As...";
    /** Option for the "checkForChanges" popup. */
    private static final String DISCARD_OPTION = "Discard";
    /** Option for the "checkForChanges" popup. */
    private static final String CANCEL_OPTION = "Cancel";
    /** Option for confirmation popup. */
    private static final String YES_OPTION = "Yes";
    /** Option for confirmation popup. */
    private static final String NO_OPTION = "No";

    // TODO: Add commands for Edit menu choices

    /** The action command for the Undo button. */
    private static final String UNDO_COMMAND = "Undo";

    /** The action command for the Close button. */
    private static final String CLOSE_COMMAND = "Close";

    /** The action command for the Save button. */
    private static final String SAVE_COMMAND = "Save";

    /** The action command for the Save As... button. */
    private static final String SAVEAS_COMMAND = "SaveAs";

    /** The action command for the Abandon button. */
    private static final String ABANDON_COMMAND = "Abandon";

    /** The action command for the Add Row button. */
    private static final String ADD_ROW_COMMAND = "AddRow";

    /** The action command for the Add Row button. */
    private static final String DELETE_ROW_COMMAND = "DeleteRow";

    /**
     * This pattern is used to recognize a parameter file based on its contents.
     * A regexp pattern for the first line of a parameter file and the
     * first field on the second line.
     * <br>First line:<br>
     * A legal MAGICAL identifier followed by 10 space-separated numbers:
     *  "uint uint float float float uint uint uint uint uint"
     * No leading spaces; trailing spaces OK.
     * <br>Second line:<br>
     * An unsigned int followed by a space.
     * <br>
     * The remainder of the file is ignored.
     */
    private static final Pattern PARAMETER_FILE_PATTERN
        = Pattern.compile("^[_a-zA-Z][_a-zA-Z0-9]* +"
                          + "[0-9]+ +[0-9]+ +"
                          + "[-+eE.0-9]+ +[-+eE.0-9]+ +[-+eE.0-9]+ +"
                          + "[0-9]+ +[0-9]+ +[0-9]+ +[0-9]+ +[0-9]+ *$."
                          + "[0-9]+ .*",
                          Pattern.MULTILINE | Pattern.DOTALL);


    private static File sm_currentChooserDirectory = null;


    /** The background color of this popup. */
    private Color m_background;

    /** The LC Method Table. */
    private EasyJTable m_table;

    /** The TableModel for the MethodTable. */
    private LcMethodTableModel m_tableModel;

    /** The component that edits tri-state cells (Booleans) in the table. */
    private TristateCellEditor m_cellEditor;

    /** Handle for communication with VnmrBG. */
    private ButtonIF m_vif;

    /** The LC Method being edited by this editor. */
    private LcMethod m_method;

    /** The panel holding the components for this editor. */
    private LcMethodVarsPanel m_varsPanel;

    /** The File Chooser for this editor. Created when needed. */
    private static JFileChooser sm_fileChooser = null;

    /** Track the number of changes to the method. */
    private long m_changeIndex = 0;

    /** Remember the last change that was saved. */
    private long m_saveIndex = 0;

    /** A list of change descriptions, indexed by m_changeIndex. */
    private Map<Long, String> m_changeList = new TreeMap<Long, String>();

    /** Need to enable/disable this button. */
    ExpButton m_saveButton;

    /** Need to enable/disable this button. */
    JButton m_addRowButton;

    /** Need to enable/disable this button. */
    JButton m_deleteRowButton;

    JTextField m_filepathLabel;
    


    public LcMethodEditor(ButtonIF vif, LcMethod method) {
        super("");
        //setTitle(method.getName());
        m_vif = vif;
        //m_method = method;
        //m_tableModel = new LcMethodTableModel(m_method);

        // Hook up ModelessDialog buttons
        // But first, a FEW that don't work
        //setAbandonEnabled(false);
        setHistoryEnabled(false);
        setHelpEnabled(false);
        setUndoEnabled(false);
        // TODO: Add commands for Edit menu choices
        setUndoAction(UNDO_COMMAND, this);
        setCloseAction(CLOSE_COMMAND, this);
        setAbandonAction(ABANDON_COMMAND, this);

        layoutPanel();

        // Finally, pack (but don't show) and set window geometry
        pack();
        if (!FileUtil.setGeometryFromPersistence(PERSISTENCE_FILE, this)) {
            Dimension frameSize = getSize();
            Dimension screenSize= Toolkit.getDefaultToolkit().getScreenSize();
            setLocation(screenSize.width/2 - frameSize.width/2,
                        screenSize.height/2 - frameSize.height/2);
        }
        setResizable(true);

        m_background = Util.getBgColor();
        setBackground(m_background);
        buttonPane.setBackground(null);
        abandonButton.setBackground(null);
        closeButton.setBackground(null);
        helpButton.setBackground(null);
        historyButton.setBackground(null);
        undoButton.setBackground(null);
        getContentPane().setBackground(null);

        // Track layout changes for persistence; save changes on window closing
        addComponentListener(this);
        addWindowListener(new MethodWindowListener());

        setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
        initializeMethod(method);
    }

    public LcConfiguration getConfiguration() {
        return m_method.getConfiguration();
    }

    public void setTitle(String methodName) {
        super.setTitle("Edit Method: " + methodName);
    }

    private void layoutPanel() {
        JLabel label;

        buildMenuBar();

        // Base pane
        JPanel basePane = new JPanel();
        basePane.setBackground(null);
        basePane.setLayout(new BorderLayout());
        getContentPane().add(basePane, BorderLayout.CENTER);

        // Method Name Line
        Box nameBox = Box.createHorizontalBox();
        //nameBox.setBorder(BorderFactory.createRaisedBevelBorder());
        m_filepathLabel = new JTextField("-----");
        m_filepathLabel.setEditable(false);
        m_filepathLabel.setBorder(BorderFactory.createRaisedBevelBorder());
        nameBox.add(m_filepathLabel);
        basePane.add(nameBox, BorderLayout.NORTH);

        // Method Table base panel
        JPanel methodTableBase = new JPanel();
        Border in = BorderFactory.createMatteBorder(1, 1, 1, 1,
                                                    getBackground().darker());
        Border out = BorderFactory.createEmptyBorder(0, 20, 0, 5);
        methodTableBase.setBorder(BorderFactory.createCompoundBorder(out, in));
        methodTableBase.setBackground(null);
        methodTableBase.setLayout(new BorderLayout());
        basePane.add(methodTableBase, BorderLayout.CENTER);

        // Add Method Table controls
        Border narrowBorder = new VButtonBorder();

        Box methodTableControlPanel = Box.createHorizontalBox();
        methodTableControlPanel.setBorder(BorderFactory.
                                          createEmptyBorder(3, 5, 3, 1));
        methodTableBase.add(methodTableControlPanel, BorderLayout.NORTH);

        label = new JLabel(Util.getLabel("lclMethodTable", "Method Table: "));
        methodTableControlPanel.add(label);
        methodTableControlPanel.add(Box.createHorizontalStrut(5));

        m_addRowButton = new JButton(Util.getLabel("lcblAddMethodRow",
                                                   "Add Row"));
        m_addRowButton.setBackground(null);
        m_addRowButton.setBorder(narrowBorder);
        m_addRowButton.setActionCommand(ADD_ROW_COMMAND);
        m_addRowButton.addActionListener(this);
        methodTableControlPanel.add(m_addRowButton);
        methodTableControlPanel.add(Box.createHorizontalStrut(5));

        m_deleteRowButton = new JButton(Util.getLabel("lcblDeleteMethodRow",
                                                      "Delete Rows"));
        m_deleteRowButton.setBackground(null);
        m_deleteRowButton.setBorder(narrowBorder);
        m_deleteRowButton.setActionCommand(DELETE_ROW_COMMAND);
        m_deleteRowButton.addActionListener(this);
        methodTableControlPanel.add(m_deleteRowButton);
        methodTableControlPanel.add(Box.createHorizontalGlue());

        // Add Method Table
        m_table = configureTable(methodTableBase);
        setBackground(getBackground()); // Calls setBgColor()

        // Base panel for method controls
        JPanel westPanel = new JPanel();
        westPanel.setBackground(null);
        westPanel.setLayout(new BorderLayout());
        basePane.add(westPanel, BorderLayout.WEST);

        // Method controls
        m_varsPanel = new LcMethodVarsPanel(null, m_vif, "lcMethodVars",
                                            "INTERFACE/LcMethodVarsPanel.xml");
        ExpPanel.addExpListener(m_varsPanel);
        m_varsPanel.setActionListener(this);
        //m_varsPanel.setValues(m_method);
        m_varsPanel.setBackground(null);
        m_varsPanel.updateValue();
        JScrollPane varsScrollPane
                = new JScrollPane(m_varsPanel,
                                  VERTICAL_SCROLLBAR_AS_NEEDED,
                                  HORIZONTAL_SCROLLBAR_NEVER);
        varsScrollPane.setBackground(null);
        varsScrollPane.getViewport().setBackground(null);
        westPanel.add(varsScrollPane, BorderLayout.CENTER);
    }

    /**
     * Make an EasyJTable for the variable LC Method parameters,
     * configure it appropriately, and put it in the given panel.
     * @param panel The panel to put the table into.
     */
    private EasyJTable configureTable(JPanel panel) {
        EasyJTable table = new EasyJTable(m_tableModel);

        table.setDefaultRenderer(Boolean.class, new TristateCellRenderer());
        m_cellEditor = new TristateCellEditor();
        table.setDefaultEditor(Boolean.class, m_cellEditor);
        m_cellEditor.getModel().addChangeListener(this);

        DefaultTableCellRenderer scanRenderer = new DefaultTableCellRenderer();
        scanRenderer.setHorizontalAlignment(SwingConstants.CENTER);
        scanRenderer.setPreferredSize(new Dimension(150, table.getRowHeight()));
        table.setDefaultRenderer(MsScanMenu.class, scanRenderer);
        JComboBox scanChoices = new MsScanMenu();
        DefaultCellEditor msScanEditor = new DefaultCellEditor(scanChoices);
        table.setDefaultEditor(MsScanMenu.class, msScanEditor);
        scanChoices.addActionListener(this);

        // TODO: Fix the border colors on the individual header "buttons".
        // This puts a border around the whole header -- a partial fix.
        table.getTableHeader()
                .setBorder(BorderFactory.createRaisedBevelBorder());

        table.setAutoResizeMode(JTable.AUTO_RESIZE_SUBSEQUENT_COLUMNS);
        table.getTableHeader().setReorderingAllowed(false);
        JScrollPane tableScrollPane;
        tableScrollPane = new JScrollPane(table,
                                          VERTICAL_SCROLLBAR_AS_NEEDED,
                                          HORIZONTAL_SCROLLBAR_AS_NEEDED);
        tableScrollPane.setBackground(null);
        table.setAutoColumnWidths(true);
        table.setRowHeight(20);
        //table.setIntercellSpacing(new Dimension(10, 0));
        panel.add(tableScrollPane, BorderLayout.CENTER);

        return table;
    }

    /**
     * This method gets change events from the TristateTableCheckBoxes in
     * the method table (technically, from the LcTristateButtonModel).
     * This is needed because focus change events are not always generated
     * after the checkbox values change (before closing the editor).
     * Perhaps the focus is not being managed properly!.
     */
    public void stateChanged(ChangeEvent ce) {
        int row = m_table.getEditingRow();
        int col = m_table.getEditingColumn();
        Boolean state = m_cellEditor.getModel().getState();
        Messages.postDebug("LcMethodEditor",
                           "LcMethodEditor.stateChanged: "
                           + "Tristate checkbox state=" + state
                           + ", row=" + row
                           + ", col=" + col
                           + ", source=" + ce.getSource()
                           );
        if (row >= 0 && col >= 0) {
            ((LcMethodTableModel)m_table.getModel())
                    .setValueAt(state, row, col);
            //((LcMethodTableModel)m_table.getModel()).notifyActionListeners("xxx", "Parameter modified in table");
        }
    }

    private void buildMenuBar() {
        JMenuBar menuBar = new JMenuBar();
        menuBar.setOpaque(true);
        menuBar.setBackground(Util.getMenuBarBg());
        menuBar.setBorder(BorderFactory.createEmptyBorder());
        setJMenuBar(menuBar);

        // File menu
        JMenu fileMenu = new JMenu("File");
        menuBar.add(fileMenu);
        fileMenu.setMnemonic(KeyEvent.VK_F);

        JMenuItem menuItem;
        JMenu menu;             // For submenus
        menu = new JMenu("Open");
        menu.setMnemonic(KeyEvent.VK_O);
        fileMenu.add(menu);
        menu.setBackground(Util.getBgColor());
        addFileItems(menu, "", "lc/lcmethods");
        menu.addMenuListener(new OpenMenuListener());

        menuItem = new JMenuItem("Save");
        menuItem.setMnemonic(KeyEvent.VK_S);
        menuItem.setActionCommand(SAVE_COMMAND);
        menuItem.addActionListener(this);
        menuItem.setBackground(Util.getBgColor());
        fileMenu.add(menuItem);

        menuItem = new JMenuItem("Save As...");
        menuItem.setMnemonic(KeyEvent.VK_A);
        menuItem.setActionCommand(SAVEAS_COMMAND);
        menuItem.addActionListener(this);
        menuItem.setBackground(Util.getBgColor());
        fileMenu.add(menuItem);

        fileMenu.addSeparator();

        menuItem = new JMenuItem("Close");
        menuItem.setMnemonic(KeyEvent.VK_C);
        menuItem.setActionCommand(CLOSE_COMMAND);
        menuItem.addActionListener(this);
        menuItem.setBackground(Util.getBgColor());
        fileMenu.add(menuItem);

        final int MENUBAR_TO_TOOLBAR_MARGIN = 100;
        Box toolBar = Box.createHorizontalBox();
        toolBar.add(Box.createHorizontalStrut(MENUBAR_TO_TOOLBAR_MARGIN));

        ExpButton ebutton = new ExpButton(0);
        ebutton.setIconData("New_24.gif");
        ebutton.setTooltip("New");
        ebutton.setActionCommand("open:NewMethod");
        ebutton.addActionListener(this);
        toolBar.add(ebutton);

        ebutton = new ExpButton(0);
        ebutton.setIconData("folder_open_file_24.gif");
        ebutton.setTooltip("Open...");
        ebutton.setActionCommand("open:");
        ebutton.addActionListener(this);
        toolBar.add(ebutton);

        m_saveButton = new ExpButton(0);
        m_saveButton.setIconData("save_24.gif");
        m_saveButton.setTooltip("Save");
        m_saveButton.setActionCommand(SAVE_COMMAND);
        m_saveButton.addActionListener(this);
        m_saveButton.setEnabled(false);
        toolBar.add(m_saveButton);

        ebutton = new ExpButton(0);
        ebutton.setIconData("saveas_24.png");
        ebutton.setTooltip("Save As...");
        ebutton.setActionCommand(SAVEAS_COMMAND);
        ebutton.addActionListener(this);
        toolBar.add(ebutton);

        toolBar.add(Box.createHorizontalGlue());
        menuBar.add(toolBar);
        //Util.setAllBackgrounds(menuItem, Util.getBgColor());
    }

    /**
     * Add menu items to a given menu for all the files in a given directory.
     * @param baseMenu The menu to which the items are added.
     * @param baseName The directory path prepended to file names.
     * @param dir The directory searched for files. If not absolute, looks
     * relative to both the sytem and user directories.
     */
    private void addFileItems(JMenu baseMenu, String baseName, String baseDir) {
        // baseName is rooted at lcmethods, and has values like
        //   "" or "pfizer/examples"
        // baseDir is rooted at userdir, so we have things like
        //   "lc/lcmethods" or "lc/lcmethods/pfizer/examples"
        if (baseName.length() > 0) {
            baseName = baseName + File.separator;
        }
        baseMenu.removeAll();

        String[] dirs = FileUtil.getAllVnmrDirs(baseDir);
        if (baseDir.length() > 0) {
            baseDir = baseDir + File.separator;
        }

        // First, add subdirectories to baseMenu
        SortedSet<String> subdirs = new TreeSet<String>(); // List of dir names
        for (String dir : dirs) {
            File[] dirfiles = new File(dir).listFiles(new DirectoryFilter());
            for (File subdir : dirfiles) {
                subdirs.add(subdir.getName());
            }
        }
        for (String subdir : subdirs) {
            JMenu submenu = new JMenu(subdir);
            submenu.setBackground(Util.getBgColor());
            baseMenu.add(submenu);
            addFileItems(submenu, baseName + subdir, baseDir + subdir);
        }

        // Next, add regular files to baseMenu (with actions)
        SortedSet<String> fnames = new TreeSet<String>(); // List of file names
        for (String dir : dirs) {
            File[] files = new File(dir).listFiles(new PlainFileFilter());
            for (File file : files) {
                fnames.add(file.getName());
            }
        }
        for (String file : fnames) {
            JMenuItem item = new JMenuItem(file);
            item.setActionCommand("open:" + baseName + file);
            item.addActionListener(this);
            item.setBackground(Util.getBgColor());
            baseMenu.add(item);
        }
    }

    /**
     * Override method in ModlessDialog to set various table colors in
     * accordance with the background color.
     */
    protected void setBgColor(Color bg) {
        super.setBgColor(bg);
        if (m_table != null) {
            m_table.setSelectionForeground(m_table.getForeground());
            m_table.setSelectionBackground(Util.changeBrightness(bg, -10));
            m_table.setGridColor(Util.changeBrightness(bg, +15));
            m_table.setHeaderBackground(Util.changeBrightness(bg, -5));
        }
    }
        

    /**
     * Get the method being edited.
     */
    public LcMethod getMethod() {
        return m_method;
    }

    /**
     * Set the method being edited.
     */
    public boolean setMethod(LcMethod method) {
        boolean rtn = false;
        if (checkForChanges(SAVE_OPTION)) {
            recordChange("New method opened");
            recordCheckpoint();
            initializeMethod(method);
            rtn = true;
        }
        return rtn;
    }

    /**
     * Update the persistence file for this popup.
     */
    private void writePersistence() {
        FileUtil.writeGeometryToPersistence(PERSISTENCE_FILE, this);
    }

    /** ActionListener interface. */
    public void actionPerformed(ActionEvent ae) {
        String cmd = ae.getActionCommand();

        if (cmd.equals(ADD_ROW_COMMAND)) {
            //recordChange("Row(s) inserted");
            addRow();
        } else if (cmd.equals(DELETE_ROW_COMMAND)) {
            //recordChange("Row(s) deleted");
            deleteRows();
            m_table.optimizeColumnWidths();
        } else if (cmd.equals("plotMethod")) {
            // TODO: Implement method plotting
            //cmdPlotMethod_Click();
        } else if (cmd.equals(SAVE_COMMAND)) {
            saveAction();
        } else if (cmd.equals(SAVEAS_COMMAND)) {
            saveAsAction();
        } else if (cmd.equals(CLOSE_COMMAND)) {
            closeAction();
        } else if (cmd.equals(ABANDON_COMMAND)) {
            abandonAction();
        } else if (cmd.equals(UNDO_COMMAND)) {
            // TODO: Implement Undo
        } else if (cmd.equals("Print")) {
            // TODO: Implement Print
        } else if (cmd.startsWith("open:")) {
            String method = cmd.substring(5).trim();
            openMethod(method);
        } else if (cmd.startsWith("VObjAction")) {
            // NB: recordChange() done in trackParameter()
            VObjIF vobj = (VObjIF)ae.getSource();
            vobjAction(vobj, cmd);
            Messages.postDebug("VLcTableItem",
                               "LcMethodEditor: calling m_varsPanel.setValues");
            m_varsPanel.setValues(m_method);
            refreshTableView();
            m_table.optimizeColumnWidths();
        } else if (cmd.startsWith("select")) {
            // NB: recordChange() done in tableSelectionAction()
            tableSelectionAction(cmd);
            m_table.optimizeColumnWidths();
        } else if (cmd.startsWith("TableChanged")) {
            // Start tokenizing after the keyword
            StringTokenizer toker = new StringTokenizer(cmd.substring(13));
            String parname = toker.nextToken();
            String msg = toker.nextToken("").trim(); // Remainder of string.
            if (m_method.isPercentParameter(parname)) {
                reconcilePercents(parname);
                m_varsPanel.setValues(m_method);
                refreshTableView();
            }
            if (!"null".equals(parname)) {
                LcMethodParameter param = m_method.getParameter(parname);
                if (param != null) {
                    msg = msg + " (" + param.getLabel() + ")";
                }
            }
            m_table.optimizeColumnWidths();
            recordChange(msg);
        }
    }

    /**
     * Sets the Method path label and the path to save the file to.
     */
    private void setPath() {
        if (m_method instanceof LcCurrentMethod) {
            m_method.setPath(null);
            m_filepathLabel.setForeground(Color.RED);
            Font font = m_filepathLabel.getFont();
            font = font.deriveFont(Font.BOLD);
            m_filepathLabel.setFont(font);
            m_filepathLabel.setText("This is the currently loaded method");
        } else {
            m_filepathLabel.setForeground(Color.BLACK);
            Font font = m_filepathLabel.getFont();
            font = font.deriveFont(Font.PLAIN);
            m_filepathLabel.setFont(font);
            m_filepathLabel.setText(m_method.getPath());
        }
    }

    /**
     * Associate a method with this editor
     * and do the necessary initialization.
     * @param method The name of the method to load.
     */
    private void initializeMethod(LcMethod method) {
        m_method = method;
        setTitle(m_method.getName());
        setPath();
        m_tableModel = new LcMethodTableModel(m_method);
        m_table.setModel(m_tableModel);
        m_varsPanel.setValues(m_method);
        refreshTableView();

        boolean editable = !(m_method instanceof LcCurrentMethod);
        m_varsPanel.setEnabled(m_method, editable);
        m_table.setEnabled(editable);
        m_addRowButton.setEnabled(editable);
        m_deleteRowButton.setEnabled(editable);

        m_tableModel.addActionListener(this);
    }

    /**
     * Read in a new method, replacing the previous method.
     * @param methodName The name, or could be the full path, of the
     * method to read in.
     */
    public boolean openMethod(String methodName) {
        boolean rtn = false;
        if (checkForChanges(SAVE_OPTION)) {
            if (methodName == null || methodName.length() == 0) {
                methodName = chooseMethodToOpen("Choose LC Method to Edit");
//                 maybeMakeFileChooser();
//                 sm_fileChooser.setDialogTitle("Open LC Method for Editing");
//                 methodName = null;
//                 int n = sm_fileChooser.showOpenDialog(VNMRFrame.getVNMRFrame());
//                 if (n == JFileChooser.APPROVE_OPTION) {
//                     methodName = sm_fileChooser.getSelectedFile().getPath();
//                 }
            }
            rtn = false;
            if (methodName != null) {
                recordChange("New method opened");
                recordCheckpoint();
                initializeMethod(new LcMethod(getConfiguration(), methodName));
                rtn = true;
            }
        }
        return rtn;
    }

    /**
     * Handle an action sent by some component in the MethodEditor panel.
     * @param vobj The component that generated the action.
     * @param actionCmd The ActionCommand that the component sent.
     */
    private void vobjAction(VObjIF vobj, String actionCmd) {
        StringTokenizer toker = new StringTokenizer(actionCmd);
        String cmd = "TrackParameter";
        toker.nextToken();      // Toss out "VObjAction" token
        if (toker.hasMoreTokens()) {
            cmd = toker.nextToken();
        }
        if (cmd.equals("TrackParameter")) {
            trackParameter(vobj);
        } else if (cmd.equals("SaveMethodAs")) {
            System.err.println("vobjAction(SaveMethodAs) called");
            saveMethod(null);
        }
    }

    /**
     * Handle an action sent by a table selection component.
     * The command is of the form:
     * <pre> select lcParamName true/false
     * </pre>
     * Indicating whether the named parameter should be in the table
     * (if the last token is "true") or not.
     * @param actionCmd The ActionCommand that the component sent.
     */
    private void tableSelectionAction(String actionCmd) {
        StringTokenizer toker = new StringTokenizer(actionCmd);
        String parname = "";
        boolean tabled = false;
        toker.nextToken();      // Toss out "select" token
        if (toker.hasMoreTokens()) {
            parname = toker.nextToken();
        }
        if (toker.hasMoreTokens()) {
            tabled = toker.nextToken().equals("true");
        }
        LcMethodParameter param = m_method.getParameter(parname);
        if (param != null) {
            if (tabled) {
                recordChange(parname + " added to table");
                m_tableModel.addColumn(param);
            } else {
                recordChange(parname + " removed from table");
                m_tableModel.deleteColumn(param);
                param.setTabled(tabled);
            }
            param.setTabled(tabled);
            refreshTableView();
            if (m_method.isPercentParameter(parname)) {
                reconcilePercents(null);
            }
        }
    }

    /**
     * Updates the display of the table based on the table model.
     */
    private void refreshTableView() {
        m_table.tableChanged(new TableModelEvent(m_tableModel, HEADER_ROW));
        /*
        // This stuff doesn't work to set header borders:
        int ncols = m_table.getColumnCount();
        for (int i = 0; i < ncols; i++) {
            TableColumn col = m_table.getColumnModel().getColumn(i);
            TableCellRenderer hr = col.getHeaderRenderer();
            if (hr == null) {
                hr = m_table.getTableHeader().getDefaultRenderer();
            }
            if (hr != null) {
                Component rc = hr.getTableCellRendererComponent
                        (m_table, "#", false, false, -1, i);
                if (rc instanceof JComponent) {
                    ((JComponent)rc).setBorder
                            (BorderFactory.createRaisedBevelBorder());
                }
            }
        }
        m_table.repaint();
        */
    }

    /**
     * Handle a change in a single-value component (not in the method table).
     * @param vobj The component whose value has changed.
     */
    private void trackParameter(VObjIF vobj) {
        String value = vobj.getAttribute(VALUE);
        if (value == null) {
            value = "";
        }
        String fullParname = vobj.getAttribute(PANEL_PARAM);
        if (fullParname != null) {
            // This parameter tracks the value. Set param to new value.
            Messages.postDebug("LcMethodEditor",
                               "LcMethodEditor: Set " + fullParname
                               + "=" + value);
            int idx = LcMethodParameter.getParIndex(fullParname);
            String parname = LcMethodParameter.stripParIndex(fullParname);
            
            LcMethodParameter lcpar = m_method.getParameter(parname);
            if (lcpar == null) {
                Messages.postDebug("LcMethodEditor.trackParameter: "
                                   + "parameter " + parname + " not found");
            } else if (!lcpar.isTabled()) {
                if (vobj instanceof VTextWin && parname.equals(fullParname)) {
                    String oldval = lcpar.getLongStringValue();
                    if (!value.equals(oldval)) {
                        lcpar.setLongStringValue(value);
                    }
                } else {
                    String oldval = lcpar.getStringValue(idx);
                    Messages.postDebug("LcMethodEditor",
                                       "LcMethodEditor.trackParameter: "
                                       +"fullParname=" + fullParname
                                       + ", value=" + value
                                       + ", lcpar=" + lcpar.getLabel()
                                       + ", oldval=" + oldval);
                    if (!value.equals(oldval)) {
                        lcpar.setValue(idx, value); // Set one element.
                        Messages.postDebug("LcMethodEditor",
                                           "...lcpar=" + lcpar);
                    }

                    // If percent changes, update all the percent fields
                    if (m_method.isPercentParameter(parname)) {
                        reconcilePercents(parname);
                    } else if (m_method.isTraceParameter(parname)) {
                        if (value.equals("none")) {
                            unselectTraceParams(idx);
                        }
                    }
                }

                // Update all the fields in case other parameters had to change
                m_varsPanel.setValues(m_method);
                refreshTableView();
                recordChange("Changed parameter (" + lcpar.getLabel() + ")");
            }
        }
    }

    /**
     * Remove any trace attributes for the given trace from the method table.
     * @param idx The trace index (from 0).
     */
    private void unselectTraceParams(int idx) {
        char letter = (char)((short)'A' + (short)idx);
        unselect("lcPeakDetect" + letter);
        unselect("lcThreshold" + letter);
    }

    /**
     * Remove the given parameter from the method table, if it is in it.
     * @param parname The name of the parameter.
     */
    private void unselect(String parname) {
        LcMethodParameter par = m_method.getParameter(parname);
        if (par != null && par.isTabled()) {
            String cmd = "select " + parname + " false";
            tableSelectionAction(cmd);
        }
    }

    /**
     * Make sure the percent concentrations in the current method add to 100%
     * at all times.
     * @param parname The name of a percent that will not be changed, or null.
     */
    private void reconcilePercents(String parname) {
        m_method.reconcilePercents(parname);
    }


    /**
     * Delete all selected rows from the table.
     */
    private void deleteRows() {
        int[] indices = m_table.getSelectedRows();
        Arrays.sort(indices);   // Make sure rows are in ascending order
        if (indices.length > 0) {
            m_tableModel.deleteRows(indices);
            refreshTableView();
        }
    }

    /**
     * Add a row to the table below the last selected row.
     */
    private void addRow() {
        int row = getRowInsertionIndex();
        m_tableModel.addRow(row);
        refreshTableView();
        m_table.setRowSelectionInterval(row, row);
    }

    /**
     * Figure out where a row should be added.
     * After the last selected row, or if none selected,
     * at the end of the table.
     */
    private int getRowInsertionIndex() {
        int[] indices = m_table.getSelectedRows();
        Arrays.sort(indices);   // Make sure rows are in ascending order
        int len = indices.length;
        int idx = len == 0 ? m_method.getRowCount() : indices[len - 1] + 1;
        return idx;
    }

    /**
     * Save the current method in a parameter file.
     * If the supplied name is null, pop up a dialog to get the name.
     * If the name is an absolute path, the method is stored there,
     * otherwise it is stored relative to the user's lcmethod directory.
     * @param name The name of the method.
     */
    private void saveMethod(String filename) {
        System.err.println("LcMethodEditor.saveMethod(" + filename + ")");
    }

    /**
     * If the method has changed, the user gets a popup to choose whether
     * to save the changes.
     * @return Returns true if changes have been saved, false if changes
     * are to be discarded.
     */
    public boolean maybeSaveIfChanged() throws IOException {
        boolean rtn = checkForChanges(SAVE_OPTION);
        if (rtn) {
            recordCheckpoint();
            if (!m_method.writeMethodToFile()) {
                throw new IOException("Method could not be saved");
            }
        }
        return rtn;
    }

    /**
     * Action performed when the Apply button is clicked (if there is one).
     * Save changes.
     */
    private void applyAction() {
        Messages.postDebug("LcMethodEditorActionListener",
                           "applyAction");
        // TODO: Write out changes.
    }

    /**
     * The Cancel button was clicked. Exit without saving changes.
     */
    private void abandonAction() {
        Messages.postDebug("LcMethodEditorActionListener",
                           "abandonAction");
        if (checkForChanges(DISCARD_OPTION)) {
            recordChange("Editing abandoned");
            recordCheckpoint();
            setVisible(false);
        }
    }

    /**
     * The Save button was clicked. Save changes.
     * @return Returns true if method was successfully saved.
     */
    private boolean saveAction() {
        Messages.postDebug("LcMethodEditorActionListener",
                           "saveAction");
        String name = m_method.getName();
        String path = m_method.getPath();
        if (name != null
            && (name = name.trim()).length() != 0
            && !name.equals("NewMethod")
            && path != null)
        {
            recordCheckpoint();
            return m_method.writeMethodToFile();
        } else {
            boolean rtn = false;
            name = saveAsAction();
            return rtn;
        }
    }

    /**
     * The SaveAs button was clicked. Save changes.
     * @return True if save was done.
     */
    private String saveAsAction() {
        Messages.postDebug("LcMethodEditorActionListener",
                           "saveAsAction");
        maybeMakeFileChooser();
        sm_fileChooser.setDialogTitle("Save LC Method");
        String path = null;
        int returnVal = sm_fileChooser.showSaveDialog(this);
        if (returnVal == JFileChooser.APPROVE_OPTION) {
            path = sm_fileChooser.getSelectedFile().getPath();
            if (path != null) {
                boolean ok = !(new File(path).exists());
                if (!ok) {
                    String[] options = {YES_OPTION,
                                        NO_OPTION };
                    String message = "The file " + path
                            + " already exists. Overwrite?";
                    String title = null;
                    int choice = showOptionDialog
                            (this,
                             message,
                             title,
                             JOptionPane.DEFAULT_OPTION,
                             JOptionPane.QUESTION_MESSAGE,
                             null,
                             options,
                             NO_OPTION);
                    switch (choice) {
                    case JOptionPane.YES_OPTION:
                        ok = true;
                        break;
                    case JOptionPane.NO_OPTION:
                    default:
                        path = null; // Indicates cancel enclosing operation
                        break;
                    }
                }
                if (ok) {
                    recordCheckpoint();
                    m_method.writeMethodToFile(path);

                    // Now switch to editing the new file
                    m_method.setName(path);
                    openMethod(path);
                }
            }
        }
        return path;
    }

    /**
     * The Close button was clicked. Save changes and exit.
     */
    private void closeAction() {
        Messages.postDebug("LcMethodEditorActionListener",
                           "closeAction");
        // Fires the componentListener method "componentHidden()".
        if (checkForChanges(SAVE_OPTION)) {
            recordChange("Editing closed");
            recordCheckpoint();
            setVisible(false);
        }
    }

    /**
     * Track changes to the method.
     * @param squib A user-readable general description of the change.
     */
    private void recordChange(String squib) {
        m_changeIndex++;
        m_changeList.put(m_changeIndex, squib);
        m_saveButton.setEnabled(true);
    }

    /**
     * Reset the saveIndex, so that we don't worry about discarding changes
     * before this point.
     */
    private void recordCheckpoint() {
        m_saveIndex = m_changeIndex;
        m_saveButton.setEnabled(false);
    }

    /**
     * Returns true if all changes have been saved, or if it's OK to
     * discard changes.
     * @param defaultOption The button that initially has the focus:
     * SAVE_OPTION, SAVEAS_OPTION, DISCARD_OPTION, or CANCEL_OPTION.
     * @return Returns true if changes have been dealt with, false if
     * the enclosing operation should be cancelled.
     */
    private boolean checkForChanges(String defaultOption) {
        boolean rtn = false;
        if (m_saveIndex == m_changeIndex) {
            Messages.postDebug("LcMethodChanges", "No changes to method");
            rtn = true;
        } else {
            if (DebugOutput.isSetFor("LcMethodChanges")) {
                Messages.postDebug("Changes to method:");
                for (long i = m_saveIndex + 1; i <= m_changeIndex; i++) {
                    Messages.postDebug("... " + m_changeList.get(i));
                }
            }

            String[] options = {SAVE_OPTION,
                                SAVEAS_OPTION,
                                DISCARD_OPTION,
                                CANCEL_OPTION};
            String message = "The method \"" + m_method.getName()
                    + "\" has been modified."
                    + "\nDo you want to save your changes?";
            String title = null;
            int choice = showOptionDialog
                    (this,
                     message,
                     title,
                     JOptionPane.DEFAULT_OPTION,
                     JOptionPane.QUESTION_MESSAGE,
                     null,
                     options,
                     defaultOption);
            switch (choice) {
            case 0:
                rtn = saveAction();
                break;
            case 1:
                rtn = saveAsAction() != null;
                break;
            case 2: // Discard
                rtn = true;
                break;
            default: // Cancel
                rtn = false;
                break;
            }
        }
        return rtn;
    }

    /**
     * Pop up a file chooser to select an existing method to open.
     * Initially opens the chooser in the user's lcmethods directory.
     * @param title The title to put on the popup, or null for no title.
     * @return The full path of the method file, or null if none chosen.
     */
    static public String chooseMethodToOpen(String title) {
        return chooseMethodToOpen(title, null);
    }

    /**
     * Pop up a file chooser to select an existing method to open.
     * @param title The title to put on the popup, or null for no title.
     * @param dirpath The initial directory to open the chooser in.
     * @return The full path of the method file, or null if none chosen.
     */
    static public String chooseMethodToOpen(String title, String dirpath) {
        maybeMakeFileChooser(); // Makes sm_fileChooser if necessary
        String methodPath = null;
        sm_fileChooser.setDialogTitle(title);
        if (dirpath == null) {
            if (sm_currentChooserDirectory == null) {
                File homeDir = new File(FileUtil.savePath("USER/lc/lcmethods"));
                sm_fileChooser.setCurrentDirectory(homeDir);
            } else {
                sm_fileChooser.setCurrentDirectory(sm_currentChooserDirectory);
            }
        } else {
            sm_fileChooser.setCurrentDirectory(new File(dirpath));
        }
        int n = sm_fileChooser.showOpenDialog(VNMRFrame.getVNMRFrame());
        
        if (n == JFileChooser.APPROVE_OPTION) {
            methodPath = sm_fileChooser.getSelectedFile().getPath();
        }
        if (dirpath == null) {
            // Remember current directory for bringing up chooser with no
            // directory specified.
            sm_currentChooserDirectory = sm_fileChooser.getCurrentDirectory();
        }
        return methodPath;
    }

    /**
     * Construct the fileChooser if it does not exist yet.
     */
    static private void maybeMakeFileChooser() {
        if (sm_fileChooser == null) {
            // This creates parent directories of "xxxx" as a side effect:
            FileUtil.savePath("USER/lc/lcmethods/xxxx");
            File homeDir = new File(FileUtil.savePath("USER/lc/lcmethods"));
            sm_fileChooser = new JFileChooser(new HomeFileSystemView(homeDir));
            sm_fileChooser.setFileFilter(new MethodFileFilter());
            sm_fileChooser.setCurrentDirectory(homeDir);
            sm_fileChooser.addComponentListener(new ComponentAdapter() {
                    public void componentResized(ComponentEvent ce) {
                        Messages.postDebug("FileChooser resized");
                        FileUtil.writeGeometryToPersistence(CHOOSER_PERSISTENCE,
                                                            sm_fileChooser);
                    }
                });
            Util.setAllBackgrounds(sm_fileChooser, Util.getBgColor());
        }
        // NB: The position isn't persistent; let JFileChooser choose location
        FileUtil.setGeometryFromPersistence(CHOOSER_PERSISTENCE, sm_fileChooser);
    }

    /* componetListener interface */
    /**
     * Update the persistence file for the current window size.
     */
    public void componentResized(ComponentEvent ce) {
        /*
         * When the JTable has AUTO_RESIZE_whatever,
         * the horizontal scrollbar will never appear; or, if it is set
         * to HORIZONTAL_SCROLLBAR_ALWAYS, it still doesn't let you scroll.
         * This is true even if the table is too wide to display after the
         * columns are shrunk to their minimum widths.
         * TODO: Move table column resizing to EasyJTable?
         */
        Component component = ce.getComponent();
        Messages.postDebug("LcMethodEditorComponentListener",
                           "componentResized: " + component);
        if /* (component == tableView) {
            setCalculatedColumnWidths();
            } else if */ (component == this) {
            writePersistence();
        }
    }

    /**
     * Update the persistence file for the current window location.
     */
    public void componentMoved(ComponentEvent ce) {
        Messages.postDebug("LcMethodEditorComponentListener",
                           "componentMoved");
        Component component = ce.getComponent();
        if (component == this) {
            writePersistence();
        }
    }

    /**
     */
    public void componentShown(ComponentEvent ce) {
        Messages.postDebug("LcMethodEditorComponentListener",
                           "componentShown");
    }

    /**
     * Save any changes when the window is closed.
     */
    public void componentHidden(ComponentEvent ce) {
        Messages.postDebug("LcMethodEditorComponentListener",
                           "componentHidden");
        applyAction();
    }

    /**
     * Equivalent to JOptionPane.showOptionDialog(), except that it sets
     * the background of the dialog to the current VJ background.
     */
    public static int showOptionDialog(Component parentComponent,
                                       Object message,
                                       String title,
                                       int optionType,
                                       int messageType,
                                       Icon icon,
                                       Object[] options,
                                       Object initialValue) {

        JOptionPane pane = new JOptionPane(message,
                                           messageType,
                                           optionType,
                                           icon,
                                           options,
                                           initialValue);
        setBackgroundRecursively(pane, Util.getBgColor());
        JDialog dialog = pane.createDialog(parentComponent, title);
        dialog.setVisible(true);
        Object selectedValue = pane.getValue();
        if(selectedValue == null) {
            return JOptionPane.CLOSED_OPTION;
        }
        if (options == null) {
            // There is not an array of option buttons:
            if(selectedValue instanceof Integer) {
                return ((Integer)selectedValue).intValue();
            }
            return JOptionPane.CLOSED_OPTION;
        }
        for (int counter = 0, maxCounter = options.length;
             counter < maxCounter;
             counter++)
        {
            // There is an array of option buttons:
            if (options[counter].equals(selectedValue)) {
                return counter;
            }
        }
        return JOptionPane.CLOSED_OPTION;
    }

    /**
     * Utility method to set the background of a component and of everything
     * it contains.
     * @param component The component to color.
     * @param bg The background color to set.
     */
    public static void setBackgroundRecursively(Component component, Color bg) {
        component.setBackground(bg);
        if (component instanceof Container) {
            Container container = (Container)component;
            Component[] components = container.getComponents();
            for (Component comp : components) {
                setBackgroundRecursively(comp, bg);
            }
        }
    }


    /**
     * A FilenameFilter that selects for directories that are not hidden.
     */
    class DirectoryFilter implements FilenameFilter {
        public boolean accept(File dir, String name) {
            boolean bShow = true;
            File file = new File(dir, name);
            if (file.isHidden() || !file.isDirectory()) {
                bShow = false;
            }
            return bShow;
        }
    }


    /**
     * A FilenameFilter that selects for regular files that are not hidden.
     */
    class PlainFileFilter implements FilenameFilter {
        public boolean accept(File dir, String name) {
            boolean bShow = true;
            File file = new File(dir, name);
            if (file.isHidden() || file.isDirectory()) {
                bShow = false;
            }
            return bShow;
        }
    }


    /**
     * Listens for window events to prevent losing edits on closing.
     */
    class MethodWindowListener extends WindowAdapter {

        public void windowClosing(WindowEvent we) {
            Messages.postDebug("LcMethodEditor", "windowClosing");
            if (checkForChanges(SAVE_OPTION)) {
                recordChange("Editing closed");
                recordCheckpoint();
                setVisible(false);
            } else {
                // Since we set DO_NOTHING_ON_CLOSE we don't need to
                // do anything to _prevent_ closing.
            }
        }
    }


    /**
     * Listens for the file/open> menu selection, so we can update it.
     */
    class OpenMenuListener implements MenuListener {

        public void menuSelected(MenuEvent me) {
            Messages.postDebug("LcMethodEditor", "refreshing open> menu");
            JMenu menu = (JMenu)me.getSource();
            addFileItems(menu, "", "lc/lcmethods");
        }

        public void menuDeselected(MenuEvent me) {
        }

        public void menuCanceled(MenuEvent me) {
        }
    }

    /**
     * Filter that accepts directories and LcMethod files.
     * LcMethod files are those that look like parameter files inside.
     */
    static class MethodFileFilter extends FileFilter {

        public String getDescription() {
            return "LC Method files";
        }

        public boolean accept(File file) {
            final int MAX_LINE_LEN = 128;

            if (file.isDirectory()) {
                return true;
            }

            // Check if first line looks like it's a parameter file
            boolean ok = true;
            BufferedReader in = null;
            try {
                in = new BufferedReader(new FileReader(file));
                char[] cLine = new char[MAX_LINE_LEN];
                // Avoid reading really long line from some binary file
                int nRead = in.read(cLine, 0, MAX_LINE_LEN);
                if (nRead < 20) {
                    ok = false; // File is too short
                }
                String line = null;
                if (ok) {
                    line = new String(cLine, 0, nRead);
                    int end = line.indexOf('\n');
                    if (end < 20) {
                        ok = false; // First line too short or too long (end=-1)
                    }
                }

                // Does the first line look like a parameter file?
                if (ok) {
                    if (!PARAMETER_FILE_PATTERN.matcher(line).matches()) {
                        ok = false;
                    }
                }

                // TODO: Check if correct parameters are in the file?

            } catch (IOException ioe) {
                ok = false;
            }
            try { in.close(); } catch (Exception e) {}
            
            return ok;
        }
    }


    class TristateCellRenderer implements TableCellRenderer {

        private LcTristateCheckBox mm_checkBox;

        public TristateCellRenderer() {
            mm_checkBox = new LcTristateCheckBox();
        }

        public Component getTableCellRendererComponent(JTable table,
                                                       Object value,
                                                       boolean isSelected,
                                                       boolean hasFocus,
                                                       int row,
                                                       int column) {
            mm_checkBox.setStateSilently((Boolean)value);
            return mm_checkBox;
        }
    }


    class TristateCellEditor
        extends AbstractCellEditor implements TableCellEditor {

        private LcTristateCheckBox mm_checkBox;

        public TristateCellEditor() {
            mm_checkBox = new LcTristateCheckBox();
        }

        public Component getTableCellEditorComponent(JTable table,
                                                     Object value,
                                                     boolean isSelected,
                                                     int row,
                                                     int column) {
            mm_checkBox.setStateSilently((Boolean)value);
            return mm_checkBox;
        }

        public Object getCellEditorValue() {
            return mm_checkBox.getState();
        }

        public LcTristateButtonModel getModel() {
            return (LcTristateButtonModel)mm_checkBox.getModel();
        }
    }

}

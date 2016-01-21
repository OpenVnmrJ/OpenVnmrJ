/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.bo;

import java.awt.*;
import java.text.*;
import java.util.*;
import java.util.List;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;
// import javax.swing.border.BevelBorder;
import javax.swing.event.*;
// import javax.swing.plaf.*;
import vnmr.util.*;
import vnmr.ui.*;

public class VFileMenu extends JComboBox
    implements VObjIF, VObjDef, VEditIF, DropTargetListener, ExpListenerIF,
               PropertyChangeListener, ComboBoxTitleIF, ActionComponent,
               VObjTimerListener
{
    protected String titleStr = null;
    private String type = null;
    private String fileName = null;
    private String fileType="file";
    private String fg = null;
    private String bg = "VJBackground";
    private String selVars = null;
    private String menuVars = null;
    private String vnmrCmd = null;
    private String vnmrCmd2 = null;
    protected String m_strDotFiles;
    protected String m_strShowDirs = "yes";
    private String showVal = null;
    private String setVal = null;
    private String fontName = null;
    private String fontStyle = null;
    private String fontSize = null;
    private Color  fgColor = null;
    private Color  bgColor = Util.getBgColor();
    private Font   font = null;
    private Font   font2 = null;
    private String keyStr = null;
    private MouseAdapter ml;
    // private MouseAdapter mle;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean isFocused = false;
    private boolean bFocused = false; // editor focus status
    private boolean inChangeMode = false;
    private boolean inAddMode = false;
    private ButtonIF vnmrIf;
    private SessionShare sshare;
    private boolean inModalMode = false;
    private Vector<String> chvals;
    private boolean fileExpr = false;
    private boolean menuValid=false;
    private boolean menuUpdate=false;
    private boolean debug=false;
    private boolean bNeedSetDef=false;
    private boolean bMenuUp=false;
    private boolean m_bDefault = false;
    private boolean panel_enabled=true;
    private boolean bWaitContent = false;
    private boolean bWaitShow = false;
    private boolean bWaitValue = false;
    private boolean bTrackVp = false;
    private boolean bUpdateValue = false;
    private boolean bListBoxAdded = false;
    private boolean bValueChanged = false;
    private boolean bModalClose = true;
    protected String m_strEditable = "No";
    protected ActionListener m_editorActionListener;
    protected FocusListener m_editorFocusListener;
    private File m_objFile;
    private ArrayList<File> m_objFileList = new ArrayList<File>();
    private long m_lTime = 0;
    protected String m_strDefault = "";
    protected String m_viewport = null;
    protected String objName = null;
    protected String menuFile = null;
    protected String menuFilePath = null;

    private FontMetrics fm = null;
    private float fontRatio = 1;
    private int isActive = 1;
    private int rWidth = 0;
    private int rWidth2 = 0;
    private int nWidth = 0;
    private int nHeight = 0;
    private int rHeight = 0;
    private int fontH = 0;
    private int fontRH = 0;
    private Insets myInset = new Insets(2,2,2,2);
    public static int uiMinWidth = 28;

    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);
    private String m_useExtraItems = "no";
    private ComboBoxEditor cbEditor;
    protected boolean bInPopup = false;
    protected boolean bSelMenu = false;
    // protected ComboBoxUI menuUi;
    protected static boolean bDebug = DebugOutput.isSetFor("VFileMenu");


    /**
     * Map of lists of extra menu entries keyed by fileName of this menu.
     * A list is shared by all filemenus that have the same fileName.
     */
    static private Map<String, List<String>> m_extraItemLists
        = new TreeMap<String, List<String>>();

    /** Max number of entries in each extraItem list. */
    static private final int MAX_EXTRAS = 5;

    /**
     * Name of local parameter tracked through the ActionCommand.
     * Accessible through set/getAttribute(PANEL_PARAM).
     */
    private String m_parameter = null;

    /**
     * ActionCommand used to handle value changes locally; not through Vnmr.
     * Accessible through set/getAttribute(ACTIONCMD).
     */
    private String m_actionCmd = null;

    /**
     * Java listeners that get sent the ActionCommand on value changes.
     */
    private Set<ActionListener> m_actionListenerList
        = new TreeSet<ActionListener>();

    protected final static String[] m_aStrShow = {"yes", "no"};


    public VFileMenu(SessionShare ss, ButtonIF vif, String typ) {
        this.sshare = ss;
        this.type = typ;
        this.vnmrIf = vif;
        chvals=new Vector<String>();
        // setBorder(BorderFactory.createBevelBorder(BevelBorder.RAISED));
        // addItemListener(this);
        //setBackground(null);
        // setBorder(new VButtonBorder());

        setBgColor(bg);

        setMaximumRowCount(12);

        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                if(inEditMode) {
                   int clicks = evt.getClickCount();
                   int modifier = evt.getModifiers();
                   if ((modifier & (1 << 4)) != 0) {
                      if (clicks >= 2) {
                        Component comp = (Component)evt.getSource();
                        if (!(comp instanceof VObjIF))
                            comp = comp.getParent();
                        if (comp instanceof VObjIF)
                            ParamEditUtil.setEditObj((VObjIF) comp);
                      }
                   }
                }
            }

            public void mouseReleased(MouseEvent evt) {
                if(inEditMode) {
                    Component comp = (Component)evt.getSource();
                    if (!(comp instanceof VObjIF))
                         comp = comp.getParent();
                    if (comp instanceof VObjIF)
                         ParamEditUtil.setEditObj((VObjIF) comp);
                }
            }
        };
        
        // Mouse listener to catch mouse exiting text item.  
        // Treat mouse exit same as lost focus

       /**********
        mle = new MouseAdapter() {
            public void mouseExited(MouseEvent evt) {
                // I am having a hell of a time getting the filemenu item to
                // properly update the variable it sets.  Before adding
                // this MouseExited, it did not update the variable at all
                // without a <cr>.  Then just adding performEditorAction()
                // upon mouseExit, it worked most of the time.  Adding the
                // sleep seemed to help and adding transferFocus seemed to
                // help more.  Now, sometimes, it resets the field to some
                // previous value without warning.  Feel free to fix this
                // if you know how.
                try {
                    Thread.sleep(100);
                }
                catch(Exception e) {
                    
                }
                performEditorAction();
                transferFocus();
            }
        };
       **********/


        new DropTarget(this, this);
        DisplayOptions.addChangeListener(this);

        addKeyListener(new KeyAdapter()
        {
            public void keyPressed(KeyEvent e)
            {
                if (e.getKeyCode() == KeyEvent.VK_ENTER)
                {
                    doAction();
                    hidePopup();
                }
            }
        });

        m_editorActionListener = new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                performEditorAction();
            }
        };

        m_editorFocusListener = new FocusAdapter()
        {
            public void focusGained(FocusEvent e)
            {
                bFocused = true;
                setFocusedObj();
            }

            public void focusLost(FocusEvent e)
            {
                bFocused = false;
                removeFocusedObj();
                performEditorAction();
            }
        };

        addPopupMenuListener(new PopupMenuListener() {
            public void popupMenuWillBecomeVisible(PopupMenuEvent e) {
                bMenuUp = true;
                if (bListBoxAdded)
                    bInPopup = false;
                else
                    bInPopup = true;
            }

            public void popupMenuWillBecomeInvisible(PopupMenuEvent e) {
                bMenuUp = false;
                if (bInPopup)
                    execCmd();
            }

            public void popupMenuCanceled(PopupMenuEvent e) {
                bInPopup = false;
            }
        });

        super.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent ev) {
                if (bMenuUp)
                    bValueChanged = true;
                // Messages.postDebug("VFileMenu",
                //                    "VFileMenu.actionPerformed " + vnmrCmd);
//                doAction();
		  if(m_bDefault) m_bDefault = false;
            }
        });

    }

    public boolean selectWithKeyChar(char keyChar)
    {
        boolean bItemMatched = super.selectWithKeyChar(keyChar);

        if (bItemMatched)
        {
            sendVC();
            sendActionCmd();
        }
        return bItemMatched;
    }

    public void initEditor()
    {
        if (editor != null)
        {
            JTextField txfEditor = (JTextField)editor.getEditorComponent();
            if (txfEditor == null)
                return;
            if (!isEditorListenerAdded(txfEditor.getFocusListeners(),
                                       m_editorFocusListener))
                txfEditor.addFocusListener(m_editorFocusListener);
           /****
            if (!isEditorListenerAdded(txfEditor.getActionListeners(),
                                       m_editorActionListener))
                editor.addActionListener(m_editorActionListener);

            if (!isEditorListenerAdded(txfEditor.getMouseListeners(),
                    mle))
                txfEditor.addMouseListener(mle);
           ****/
        }
    }

    protected boolean isEditorListenerAdded(EventListener[] arrEl,
                                            EventListener eventListener)
    {
        boolean bListener = false;
        int nSize = arrEl.length;
        for (int i = 0; i < nSize; i++)
        {
            EventListener el = arrEl[i];
            if (el == eventListener)
            {
                bListener = true;
                break;
            }
        }
        return bListener;
    }

    public void setSelMode(boolean b) {
        bSelMenu = b;
    }

    public void updateUI() {
        bListBoxAdded = false;
        super.updateUI();
    }

    protected void doAction() {
        if (!bMenuUp)
            return;
        if (inModalMode)
            return;
        if (inAddMode || inChangeMode ||  inEditMode)
            return;
        if ((vnmrCmd == null || vnmrIf == null) && m_actionCmd == null)
            return;
        if (isActive < 0)
            return;
    }

    protected void performEditorAction()
    {
        if (inModalMode || inAddMode ||  inEditMode)
            return;
        if (!isEditable() || editor == null)
            return;
        String strValue = (String) editor.getItem();
        String itemValue = (String) getSelectedItem();
        if (strValue != null && !strValue.equals("") &&
            !strValue.equals(itemValue))
        {
            int nLength = getItemCount();
            boolean bLabel = false;
            for (int i = 0; i < nLength; i++)
            {
                String strLabel = (String)getItemAt(i);
                if (strValue.equals(strLabel))
                {
                    bLabel = true;
                    break;
                }
            }
            if (!bLabel)
            {
                addItem(strValue);
                chvals.add(strValue);
            }
            setSelectedItem(strValue);
            sendVC();
            sendActionCmd();
        }
    }

    private void execCmd() {
        if (!bInPopup) { // may be from arrow key
            return;
        }
        bInPopup = false;
        if (inModalMode || inAddMode || inChangeMode ||  inEditMode)
            return;
        if ((vnmrCmd == null || vnmrIf == null) && m_actionCmd == null)
            return;
        if (isActive < 0)
            return;
        sendVC();
        sendActionCmd();
    }

    public void actionPerformed(ActionEvent e)
    {
        bInPopup = true;
        execCmd();
    }

    public void enterPopup() {
        bInPopup = true;
    }

    public void exitPopup() {
        bInPopup = false;
    }

    public void listBoxMouseAdded() {
        bListBoxAdded = true;
    }

    //----------------------------------------------------------------
    /** set the menu choice. */
    //----------------------------------------------------------------
    private void setSelectedChoice(String s) {
        if(inAddMode)
            return;
        // Messages.postDebug("VFileMenu",
        //                    new StringBuffer("VFileMenu.setSelectedChoice ").
        //                    append(s).toString());
        m_bDefault = true;
        int nLength = chvals.size();
        boolean inList = false;
        for (int i = 0; i < nLength ; i++) {
            String s2 = chvals.get(i);
            if (s2 != null && s2.equals(s)) {
                if (i < getItemCount()) {
                    setSelectedIndex(i);
                }
                m_bDefault = false;
                inList = true;
                break;
            }
        }
        if ("yes".equals(m_useExtraItems)) {
            addExtraItem(s, !inList);
            if (!inList) {
                // This puts it in the current menu
                addItem(s);
                chvals.add(s);
            }
        }

        // show the current selection
        repaint();
    }

    /**
     * Remember an "extra" menu choice for FileMenus looking at our directory.
     * It is added to the list iff force is true, otherwise it is just
     * moved to the high priority list position iff it is already in the
     * extras list.
     * <p><b>Works only for type="directory" elastic="yes".</b>
     * @param s The choice to add to the menu.
     * @param force If true, force addition to the extras list.
     */
    private void addExtraItem(String s, boolean force) {
        // NB: CURRENTLY WORKS ONLY FOR type="directory" elastic="yes"
        // TODO: Validate choice -- is it a file?
        if (s == null || s.trim().length() == 0) { // Don't add junk choices
            return;
        }
        List<String> extraList = m_extraItemLists.get(fileName);
        if (extraList == null) {
            extraList = new ArrayList<String>();
            m_extraItemLists.put(fileName, extraList);
        }
        // Put it at the end of the extras list -- removing any duplicate
        boolean removed = extraList.remove(s);
        if (force || removed) {
            extraList.add(s);
            if (extraList.size() > MAX_EXTRAS) {
                extraList.remove(0);
            }
        }
    }

    /**
     * Get the list of "extra" menu choices for FileMenus looking at our
     * directory.
     * Lists only the last MAX_EXTRAS choices referenced, in the order
     * they were used.
     * <p><b>Works only for type="directory" elastic="yes".</b>
     * @return The list of "extra" choices.
     */
    private List<String> getExtraItems() {
        List<String> extraList = m_extraItemLists.get(fileName);
        if (extraList == null) {
            extraList = new ArrayList<String>();
            m_extraItemLists.put(fileName, extraList);
        }
        return extraList;
    }
        

    //----------------------------------------------------------------
    /** Read a list of menu labels from a file or directory */
    //----------------------------------------------------------------
    private boolean getChoices(String s) {
        if(inAddMode)
            return false;
        if (s != null)
            s=s.trim();
        boolean bVisible = true;
        if (s == null || s.length() <= 0)
            bVisible = false;
        else {
            if(fileType.equals("directory"))
                bVisible = getMenuDir(s);
            else
                bVisible = getMenuFile(s);
        }

        if (!bVisible) {
            if (!inEditMode)
                setVisible(bVisible);
        }
        else
            setVisible(bVisible);
        rWidth = 0;
        return bVisible;
    }

    //----------------------------------------------------------------
    /** Read a list of menu labels from a directory. */
    //----------------------------------------------------------------
    private boolean getMenuDir(String s) {
        if(menuValid || inAddMode)
            return false;
        if (s == null) {
            return false;
        }
        String[] dirs = FileUtil.getAllVnmrDirs(s);
        if (dirs.length == 0) {
            Messages.postDebug("VFileMenu: directory not found " + s);
            return false;
        }
        if (bDebug) {
            Messages.postDebug("VFileMenu: getMenuDir(): " + s);
            for (int i = 0; i < dirs.length; i++) {
                Messages.postDebug("  dirs[" + i + "]=" + dirs[i]);
            }
        }
        inAddMode = true;

        String oldValue = getAttribute(VALUE);
        removeAllItems();
        chvals.clear();

        m_objFileList.clear();
        // Sort file names in local collation sequence
        Collator comp = Collator.getInstance();
        comp.setStrength(Collator.IDENTICAL);
        SortedSet<String> sortedSet = new TreeSet<String>(comp);
        m_objFile = null;
        m_lTime = 0;
        for (int i = 0; i < dirs.length; i++) {
            File dirFile = new File(dirs[i]);
            m_objFileList.add(dirFile);
            // Remember latest time any directory modified
            // Change in any directory will have a later time
            long t = dirFile.lastModified();
            if (m_lTime < t) {
                m_lTime = t;
            }
            File[] files = dirFile.listFiles(new FilenameFilter()
                {
                    public boolean accept(File dir, String name)
                    {
                        boolean isDir = new File(dir, name).isDirectory();
                        boolean bShow = true;
                        if (("no".equals(m_strShowDirs) && isDir) ||
                            (m_strDotFiles != null &&
                            !m_strDotFiles.trim().equals("") &&
                            !m_strDotFiles.equalsIgnoreCase("true") &&
                            !m_strDotFiles.equalsIgnoreCase("yes") &&
                            (name != null && name.startsWith("."))))
                        {
                            bShow = false;
                        }
                        return bShow;
                    }
                });

            // Collect this batch of files in sorted list
            for (int j = 0; j < files.length; j++) {
                String fn = files[j].getName().trim();
                fn = fn.replace('\\','/');
                sortedSet.add(fn);
            }
        }

        while (!sortedSet.isEmpty()) {
            String fn = sortedSet.first();
            sortedSet.remove(fn);
            addItem(fn);
            chvals.add(fn);
        }
        if ("yes".equals(m_useExtraItems)) {
            List<String> extraItems = getExtraItems();
            for (String item : extraItems) {
                addItem(item);
                chvals.add(item);
            }
        }

        menuValid = true;
        inAddMode = false;
        setAttribute(VALUE, oldValue); // Try to reset to previous value
        sendValueQuery();              // Check for new value
        return true;
    }

    //----------------------------------------------------------------
    /** Read a list of menu labels and chvals from a file. */
    //----------------------------------------------------------------
    private boolean getMenuFile(String s) {
        if(menuValid || inAddMode)
            return false;
        if (s == null)
            return false;
        String strPath=FileUtil.openPath(s);
        if (Util.iswindows())
            strPath = UtilB.unixPathToWindows(strPath);

        if(strPath==null){
            Messages.postDebug(new StringBuffer("VFileMenu: file not found ").
                               append(s).toString());
            return false;
        }
        menuFile = s;
        m_objFile = new File(strPath);

        long fileTime = m_objFile.lastModified();

        if (fileTime <= m_lTime) {
            if (strPath.equals(menuFilePath)) {
               menuValid = true;
               return true;
            }
        }
        m_lTime = fileTime;

        FileReader fr;
        try {
            fr=new FileReader(strPath);
        }
        catch (java.io.FileNotFoundException e1){
            menuFile = null;
            menuFilePath = null;
            Messages.writeStackTrace(e1, "VFileMenu: file not found: " + s);
            return false;
        }
        if (bDebug) {
            Messages.postDebug("VFileMenu",
                           new StringBuffer("VFileMenu.getMenuFile ").
                           append(s).toString());
        }

        // parse the file and extract key-chval pairs. Build menu list.
        // note: could use Properties.load method here except list order is
        //       not retained (because Properties extends Hashtable)

        menuFilePath = strPath;
        inAddMode=true;
        String oldValue = getAttribute(VALUE);
        removeAllItems();
        chvals.clear();
        BufferedReader text=new BufferedReader(fr);
        boolean bSuccess = buildMenu(text);
        try {
            fr.close();
        } catch(IOException e) { }

        if(getItemCount() <= 0) {
            addItem("<none>");
            setEnabled(false);
            bSuccess = false;
        } else {
            setEnabled(true);
        }
        menuValid=true;
        inAddMode=false;
        setAttribute(VALUE, oldValue); // Try to reset to previous value
        sendValueQuery();              // Check for new value
        return bSuccess;
    }

    private boolean buildMenu(BufferedReader text)
    {
        try
        {
            String line=null;
            while((line=text.readLine()) !=null){
              if(line.startsWith("#"))
                    continue;
              StringTokenizer tok;
              if(line.startsWith("`") && line.endsWith("`")) { 
                tok=new StringTokenizer(line,"`");
                if(!tok.hasMoreTokens()) continue;
                String key=tok.nextToken();
 	        String cval ="";
                if(tok.hasMoreElements()){
                    while(tok.hasMoreElements()){
                        cval+=tok.nextToken(" `\t");
                        if(tok.hasMoreElements())
                            cval+=" ";
                    }
                }
                // addItem(key.trim());
                addItem(Util.getLabelString(key.trim()));
                chvals.add(cval.trim());
              } else if(line.startsWith("|") && line.endsWith("|")) { 
                tok=new StringTokenizer(line,"|");
                if(!tok.hasMoreTokens()) continue;
                String key=tok.nextToken();
 	        String cval ="";
                if(tok.hasMoreElements()){
                    while(tok.hasMoreElements()){
                        cval+=tok.nextToken(" |\t");
                        if(tok.hasMoreElements())
                            cval+=" ";
                    }
                }
                // addItem(key.trim());
                addItem(Util.getLabelString(key.trim()));
                chvals.add(cval.trim());
              } else if(line.startsWith("'") && line.endsWith("'")) { 
                tok=new StringTokenizer(line,"'");
                if(!tok.hasMoreTokens()) continue;
                String key=tok.nextToken();
 	        String cval ="";
                if(tok.hasMoreElements()){
                    while(tok.hasMoreElements()){
                        cval+=tok.nextToken(" '\t");
                        if(tok.hasMoreElements())
                            cval+=" ";
                    }
                }
                // addItem(key.trim());
                addItem(Util.getLabelString(key.trim()));
                chvals.add(cval.trim());
	      } else {
		if(line.startsWith("\""))
                    tok=new StringTokenizer(line,"\"");
                else
                    tok=new StringTokenizer(line);
                if(!tok.hasMoreTokens())
                    continue;
                String key=tok.nextToken();
                String cval="";
                if(tok.hasMoreElements()){
                    while(tok.hasMoreElements()){
                        cval+=tok.nextToken(" \t\"");
                        if(tok.hasMoreElements())
                            cval+=" ";
                    }
                }
                // addItem(key.trim());
                addItem(Util.getLabelString(key.trim()));
		if(cval.trim().length()>0) 
                   chvals.add(cval.trim());
		else 
                   chvals.add(key.trim());
	      }
            }
        }
        catch(java.io.IOException e){
            inAddMode=false;
            Messages.writeStackTrace(e);
            return false;
        }
        return true;
    }

    /**
     * Set foreground color of the edit field to effect editable items and
     * set the combobox itself to effect the non-editable items.
     */
    private void setFgColor(String c){
        fg = c;
        if (c == null || c.length() == 0 || c.equals("default"))
             fgColor = UIManager.getColor("ComboBox.foreground");
        else
             fgColor = DisplayOptions.getColor(c);
        Component editField= getEditor().getEditorComponent();
        if (editField != null)
            editField.setForeground(fgColor);
        setForeground(fgColor);
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt){
        changeFont();
        setFgColor(fg);
        setBgColor(bg);
        SwingUtilities.updateComponentTreeUI(this);
    }

    // VObjIF interface

    public void setDefLabel(String s) {
    }

    public void setDefColor(String c) {
        setFgColor(c);
    }

    public boolean isRequestFocusEnabled()
    {
        if (!isEditable())
            return Util.isFocusTraversal();
        return super.isRequestFocusEnabled();
    }

    public boolean isfileExpr(String strfile)
    {
       /***
        return (strfile != null && (strfile.startsWith("$")
                                    || strfile.startsWith("if")
                                    || strfile.startsWith("exists")));
       ****/
       if (strfile == null)
          return false;
       if (strfile.indexOf('$') >= 0)
          return true;
       if (strfile.indexOf('=') >= 0)
          return true;
       if (strfile.indexOf(':') >= 0)
          return true;
       return false;
    }

    public void setEditStatus(boolean s) {
        isEditing = s;
        repaint();
    }

    public void setEditMode(boolean s) {
        boolean oldMode = inEditMode;
        inEditMode = s;
        if (s) {
            setVisible(true);
            addMouseListener(ml);
/*
            if (editor != null)
                editor.getEditorComponent().addMouseListener(ml);
*/
            defDim = getPreferredSize();
            if (font != null) {
                setFont(font);
                fontH = rHeight;
                fontRH = fm.getHeight();
                myInset.top = (defDim.height - fontRH) / 2;
                if (myInset.top < 0)
                    myInset.top = 0;
            }
            fontRatio = 1.0f;
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
            rWidth2 = rWidth;
        }
        else {
            if (getItemCount() <= 0 && !isEditable())
                setVisible(false);
            else
                setVisible(true);
            removeMouseListener(ml);
/*
            if (editor != null)
                editor.getEditorComponent().removeMouseListener(ml);
*/
            if (oldMode)
                adjustFont(curDim.width, curDim.height);
            isFocused = s;
        }
    }

    /****
    public Insets getInsets() {
        return myInset;
    }
    ****/

    private void setDefSize() {
        if (rWidth <= 0)
            calSize();
        if (isPreferredSizeSet()) {
            defDim = getPreferredSize();
            if (defDim != null) {
                bNeedSetDef = false;
                return;
            }
        }
            
        defDim.width = rWidth + 6;
        defDim.height = rHeight + 8;
        setPreferredSize(defDim);
    }

    public void changeFont() {
        font=DisplayOptions.getFont(fontName,fontStyle,fontSize);
        setFont(font);
    font2 = null;
    rHeight = font.getSize();
        fontH = rHeight;
        fm = getFontMetrics(font);
        fontRH = fm.getHeight();
    calSize();
        fontRatio = 1.0f;
        if (!inEditMode) {
             if ((curDim.width > 0) && (rWidth > curDim.width)) {
         if (bNeedSetDef) {
                        rWidth = 0;
                        setDefSize();
                 }
                 adjustFont(curDim.width, curDim.height);
             }
        }
        repaint();
    }

    public void changeFocus(boolean s) {
        if (!inEditMode) {
            if (!s && bFocused) {
               performEditorAction();
               bFocused = false;
            }
            return;
        }
        isFocused = s;
        repaint();
    }

    public String getAttribute(int attr) {
        int             k;
        String s;
        switch (attr) {
        case TYPE:
            return type;
        case LABEL:
            return titleStr;
        case KEYSTR:
            return keyStr;
        case PANEL_FILE:
            return fileName;
        case PANEL_TYPE:
            return fileType;
        case FGCOLOR:
            return fg;
        case BGCOLOR:
            return bg;
        case SHOW:
            return showVal;
        case FONT_NAME:
            return fontName;
        case FONT_STYLE:
            return fontStyle;
        case FONT_SIZE:
            return fontSize;
        case CMD:
            return vnmrCmd;
        case SETVAL:
            return setVal;
        case VARIABLE:
            return selVars;
        case VAR2:
            return menuVars;
        case STATSHOW:
            return m_strDotFiles;
        case DISPLAY:
            return m_strShowDirs;
        case ELASTIC:
            return m_useExtraItems;
        case EDITABLE:
            return m_strEditable;
        case VALUE:
            k = getSelectedIndex();
            if (k < 0 || k >= chvals.size()) {
                if (!isEditable() || editor == null)
                    return null;
                String strValue = (String) editor.getItem();
                return strValue;
            }
            return chvals.elementAt(k);
        case ACTIONCMD:
            return m_actionCmd;
        case PANEL_PARAM:
            return m_parameter;
       case TRACKVIEWPORT:
            return m_viewport;
       case PANEL_NAME:
            return objName;
        default:
            return null;
        }
    }

    public void setAttribute(int attr, String c) {
        Vector v;
        switch (attr) {
        case TYPE:
            type = c;
            break;
        case LABEL:
            titleStr = c;
            /****
            if (bSelMenu) {
                ((VComboBoxUI)menuUi).setTitle(c);
            }
            ****/
            inChangeMode = true;
            setSelectedItem(c);
            inChangeMode = false;
            break;
        case PANEL_FILE:
            menuValid = false;
            if (c != null && c.length() > 0)
               fileName = c;
            else
               fileName = null;
            fileExpr= false;
            if (isfileExpr(c)) {
               fileExpr=true;
            } else {
               // getChoices(fileName);
            }
            break;
        case PANEL_TYPE:
            fileType = c;
            menuValid=false;
            break;
        case FGCOLOR:
            setFgColor(c);
            repaint();
            break;
        case BGCOLOR:
            bg = "default".equals(c) ? "VJBackground" : c;
            setBgColor(bg);
            repaint();
            break;
        case SHOW:
            showVal = c;
            break;
        case FONT_NAME:
            fontName = c;
            break;
        case FONT_STYLE:
            fontStyle = c;
            break;
        case FONT_SIZE:
            fontSize = c;
            break;
        case VARIABLE:
            selVars = c;
            break;
        case VAR2:
            menuVars = c;
            if (c != null && c.length() > 0)
               menuUpdate=true;
            else
               menuUpdate= false;
            break;
        case STATSHOW:
            m_strDotFiles = c;
            break;
        case DISPLAY:
            m_strShowDirs = c;
            break;
        case ELASTIC:
            m_useExtraItems = c;
            break;
        case SETVAL:
            setVal = c;
            break;
        case CMD:
            vnmrCmd = c;
            break;
        case EDITABLE:
            if (c != null)
                m_strEditable = c;
            boolean bEditable = false;
            if (c != null && (c.equalsIgnoreCase("true") ||
                              c.equalsIgnoreCase("yes")))
                bEditable = true;
            setEditable(bEditable);
            if (bEditable)
                initEditor();
            break;
        case VALUE:
            inChangeMode = true;
            setSelectedChoice(c);
            inChangeMode = false;
            break;
        case ENABLED:
            panel_enabled=c.equals("false")?false:true;
            setEnabled(panel_enabled);
            break;
        case ACTIONCMD:
            m_actionCmd = c;
            break;
        case PANEL_PARAM:
            m_parameter = c;
            break;
        case TRACKVIEWPORT:
            m_viewport = c;
            bTrackVp = false;
            if (c != null) {
                if (c.toLowerCase().startsWith("y"))
                    bTrackVp = true;
            }
            break;
        case PANEL_NAME:
            objName = c;
            break;
        case MODALCLOSE:
            if (c != null) {
                if (c.equalsIgnoreCase("no") || c.equalsIgnoreCase("false"))
                    bModalClose = false;
            }
            break;
        }
    }

    /**
     * Set the background color.
     * @param bg The color to set.
     */
    private void setBgColor(String bg) {
    	if(bg == null || bg.length() == 0 || 
    			bg.equals("transparent")||bg.equals("VJBackground"))
    		bgColor = UIManager.getColor("ComboBox.background");
    	else
    		bgColor = DisplayOptions.getColor(bg);
        setBackground(bgColor);
    }

    public ButtonIF getVnmrIF() {
        return vnmrIf;
    }

    public void setVnmrIF(ButtonIF vif) {
        if (!vif.equals(vnmrIf))
            menuValid = false;
        vnmrIf = vif;
    }

    private void setFocusedObj() {
        VObjUtil.setFocusedObj((VObjIF)this);
    }

    private void removeFocusedObj() {
        VObjUtil.setFocusedObj((VObjIF)this);
    }

    private void  sendContentQuery(){
        if (fileName != null) {
           if (bWaitContent) {
              bWaitContent = false; // skip every other query
              return;
           }
           menuValid = false;
           bWaitContent = true;
           vnmrIf.asyncQueryParamNamed("content", this, fileName);
        }
        else {
           menuValid = true;
           setVisible(false);
        }
    }

    private void sendValueQuery(){
        if (setVal == null || bWaitValue)
            return;
        bWaitValue = true;
        vnmrIf.asyncQueryParam(this, setVal);
    }

    private void  sendShowQuery(){
        if (showVal == null || bWaitShow)
            return;
        bWaitShow = true;
        vnmrIf.asyncQueryShow(this, showVal);
    }

    // ExpListenerIF interface

    private boolean isUpdateParam(String vqStr, Vector params){
        if (vqStr == null)
           return false;
        StringTokenizer tok=new StringTokenizer(vqStr, " ,\n");
        while (tok.hasMoreTokens()) {
            String var = tok.nextToken();
            for (int k = 0; k < params.size(); k++) {
                if (var.equals(params.elementAt(k)))
                    return true;
            }
        }
        return false;
    }

    private void updateContent() {
        bUpdateValue = true;
        if (fileExpr)
            sendContentQuery();
        else {
            getChoices(fileName);
            VObjTimer.addListener(this);
        }
    }

    public void  updateValue(Vector params){
        if (inEditMode || inAddMode)
            return;
        if (bTrackVp)
            vnmrIf = Util.getActiveView();
        if (vnmrIf == null)
            return;
        bUpdateValue = true;
        if (isUpdateParam(menuVars, params))
        {
            menuValid = false;
            updateContent();
        }
        if (!isUpdateParam(selVars, params))
            return;
        if (showVal != null)
            sendShowQuery();
        else
            sendValueQuery();
        bUpdateValue = false;
    }

    public void updateValue() {
        if (inAddMode || inEditMode)
            return;
        if (bTrackVp)
            vnmrIf = Util.getActiveView();
        if (vnmrIf == null)
            return;

        bUpdateValue = true;
        bWaitContent = false;
        bWaitShow = false;
        bWaitValue = false;
        if(fileName != null) {
            if (!menuValid)
                updateContent();
            else
            {
                if (fileExpr)
                   sendContentQuery();
                else
                   VObjTimer.addListener(this);
             }
        }
        else {
            if (getItemCount() <= 0 && !isEditable())
                setVisible(false);
            else
                setVisible(true);
        }

        if (showVal != null)
            sendShowQuery();
        else
            sendValueQuery();
        bUpdateValue = false;
    }

    public void setValue(ParamIF pf) {
        if(pf == null || pf.value == null)
            return;
        if(pf.name.equals("null") || !pf.name.equals("content")){
            bWaitValue = false;
            inChangeMode=true;
            setSelectedChoice(pf.value);
            inChangeMode=false;
        }
        else if(pf.name.equals("content")){
            bUpdateValue = true;
            bWaitContent = false;
            menuValid = false;
            getChoices(pf.value);
            VObjTimer.addListener(this);
        }
    }

    public void setShowValue(ParamIF pf) {
        bWaitShow = false;
        if (pf != null && pf.value != null) {
            String  s = pf.value.trim();
            isActive = Integer.parseInt(s);
            if (panel_enabled && isActive >= 0) {
                setEnabled(true);
            }
            else
                setEnabled(false);
            bWaitValue = false;
            sendValueQuery();
        }
    }


    public void paint(Graphics g) {
        super.paint(g);

        // Iff the file has been modified, get the current version.
        // getCurrentFile();

        if (!isEditing)
            return;
        // if(!menuValid && !menuUpdate && !fileExpr)
        //    getChoices(fileName);

        Dimension  psize = getSize();
        if (isFocused)
            g.setColor(Color.yellow);
        else
            g.setColor(Color.green);
        g.drawLine(0, 0, psize.width, 0);
        g.drawLine(0, 0, 0, psize.height);
        g.drawLine(0, psize.height-1, psize.width-1, psize.height-1);
        g.drawLine(psize.width -1, 0, psize.width-1, psize.height-1);
    }

    /**
     *  Checks if the file has been modified by comparing the modify stamp.
     */
    private void getCurrentFile() {
        long lCurrTime = 0;
        if (bWaitContent)
            return;
        if (m_objFile != null) { // type if file
            if (menuFilePath != null)
                m_objFile = new File(menuFilePath);
            lCurrTime = m_objFile.lastModified();
        } else {
            File[] files = new File[0];
            if (m_objFileList.size() > 0) {
                files = m_objFileList.toArray(files);
                for (int i = 0; i < files.length; i++) {
                    long t = files[i].lastModified();
                    if (lCurrTime < t) {
                        lCurrTime = t;
                    }
                }
            }
        }

        if (m_lTime < lCurrTime) {
            menuValid = false;
            if (m_objFile != null) {
                if (getMenuFile(menuFile)) {
                    return;
                }
            }
            fileExpr = false;
            if (isfileExpr(fileName))
                fileExpr = true;
            // updateValue();
            updateContent();
        }
    }

    public void timerRinging() {
        if (!isShowing())
            return;
        if (bUpdateValue)
            return;
        getCurrentFile();
    }

    public void refresh() {}

    public void destroy() {
        DisplayOptions.removeChangeListener(this);
    }

    public void addDefChoice(String c) {}
    public void addDefValue(String c) {}

    public void itemStateChanged(ItemEvent e){}

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    }

    public void setModalMode(boolean s) {
        inModalMode = s;
    }

    public void sendVnmrCmd() {
        if (vnmrCmd == null || vnmrIf == null)
            return;
        if (bDebug)
            Messages.postDebug("VFileMenu", "VFileMenu.sendVnmrCmd");
        if (inModalMode) {
            if (!bModalClose) {
               if (!bValueChanged)
                   return;
            }
        }
        bValueChanged = false;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }

    private void sendVC() {
        if (inModalMode)
            return;
        if (vnmrCmd == null || vnmrIf == null)
            return;
        vnmrIf.sendVnmrCmd(this, vnmrCmd);
    }


    /**
     * Send an ActionEvent to all the action listeners, only if the
     * actionCmd has been set.
     */
    private void sendActionCmd() {
        if (inModalMode)
            return;
        if (m_actionCmd != null) {
            String value = getAttribute(VALUE);
            ActionEvent event = new ActionEvent(this, hashCode(), m_actionCmd);
            for (ActionListener listener : m_actionListenerList) {
                listener.actionPerformed(event);
            }
        }
    }

    /**
     * Sets the action command to the given string.
     * @param actionCommand The new action command.
     */
    public void setActionCommand(String actionCommand) {
        m_actionCmd = actionCommand;
    }

    /**
     * Add an action listener to the list of listeners to be notified
     * of actions.
     */
    public void addActionListener(ActionListener listener) {
        m_actionListenerList.add(listener);
    }


    private final static String[] m_types = {"file", "directory" };

    protected final static String[] m_aStrEditable = { "Yes", "No" };
    private final static Object[][] attributes = {
    {new Integer(LABEL),        "Label of item:"},
    {new Integer(VARIABLE),             "Selection variables:"},
    {new Integer(VAR2),             "Content variables:"},
    {new Integer(SETVAL),               "Value of item:"},
    {new Integer(SHOW),             "Enable condition:"},
    {new Integer(CMD),                  "Vnmr command:"},
    {new Integer(PANEL_FILE),   "Menu source:"},
    {new Integer(PANEL_TYPE),   "Menu type:","radio",m_types},
    {new Integer(STATSHOW),     "Show Dot Files:","radio",m_aStrShow},
    { new Integer(EDITABLE), "Editable:", "radio", m_aStrEditable },
    };
    public Object[][] getAttributes()  { return attributes; }

    public void setSizeRatio(double x, double y) {
        double xRatio =  x;
        double yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
    if (defDim.width <= 0) {
        if (!isPreferredSizeSet()) {
            bNeedSetDef = true;
                setDefSize();
            }
            else
            defDim = getPreferredSize();
    }
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void calSize() {
        if (fm == null) {
            font = getFont();
            fm = getFontMetrics(font);
        }
        rHeight = font.getSize();
        rWidth = 0;
//      Dimension ss = getMinimumSize();
        int fw;
    Object obj;
        String str;
        for (int n = 0; n < getItemCount(); n++) {
            obj = (Object)getItemAt(n);
        if (obj != null) {
                str = obj.toString();
                if (str != null) {
                    fw = fm.stringWidth(str);
                    if (fw > rWidth)
                        rWidth = fw;
        }
            }
        }
        rWidth += uiMinWidth;
        rWidth2 = rWidth;
    }

    public void setBounds(int x, int y, int w, int h) {
        if (inEditMode) {
           defLoc.x = x;
           defLoc.y = y;
           defDim.width = w;
           defDim.height = h;
        }
        curDim.width = w;
        curDim.height = h;
        curLoc.x = x;
        curLoc.y = y;
        if (!inEditMode) {
            if (rWidth <= 0) {
                calSize();
            }
            if ((w != nWidth) || (w < rWidth2) || (h != nHeight)) {
                adjustFont(w, h);
            }
        }
        super.setBounds(x, y, w, h);
    }

    public void setDefLoc(int x, int y) {
        defLoc.x = x;
        defLoc.y = y;
    }

    public Point getDefLoc() {
        tmpLoc.x = defLoc.x;
        tmpLoc.y = defLoc.y;
        return tmpLoc;
    }

    /**
     *  Returns true if the value does not match the strings in the combobox,
     *  otherwise false.
     *  If it's true then the ui would display the default label
     *  as the title of the combobox.
     */

    public boolean getDefault()
    {
        if (bSelMenu && titleStr != null)
            return false;
        return m_bDefault;
    }

    public String getTitleLabel() {
        if (bSelMenu)
            return titleStr; 
        return null;
    }


    /**
     *  Returns the default label for the title of the combobox.
     */
    public String getDefaultLabel()
    {
        return m_strDefault;
    }

    public Point getLocation() {
        if (inEditMode) {
           tmpLoc.x = defLoc.x;
           tmpLoc.y = defLoc.y;
        }
        else {
           tmpLoc.x = curLoc.x;
           tmpLoc.y = curLoc.y;
        }
        return tmpLoc;
    }


    public void adjustFont(int w, int h) {
        Font  curFont = null;

        if (w <= 0)
           return;
        nWidth = w;
        nHeight = h;
        if (fontRH > 0)
        {
           myInset.top = (h - fontRH) / 2;
           if (myInset.top < 0)
                myInset.top = 0;
        }
        h -= 4;
        if (w > rWidth2) {
           if ((fontRatio >= 1.0f) && (h > fontH) && (fontH >= rHeight))
              return;
        }
        float s = (float) w / (float) rWidth;
        if (rWidth > w) {
           if (s > 0.98f)
                s = 0.98f;
           if (s < 0.5f)
                s = 0.5f;
        }
        if (s > 1.0f)
            s = 1.0f;
        if ((s == fontRatio) && (h > fontH) && (fontH >= rHeight))
            return;
        fontRatio = s;
        s = (float) rHeight * fontRatio;
        if (s >= h)
            s = h - 1;
        if ((s < 9.0f) && (rHeight > 9))
            s = 9.0f;
        if (fontRatio < 1.0f) {
            if (font2 == null) {
                String fname = font.getName();
                if (!fname.equals("Dialog"))
                    //font2 = new Font("Dialog", font.getStyle(), rHeight);
                    font2 = DisplayOptions.getFont("Dialog",
                                                   font.getStyle(),
                                                   rHeight);
                else
                    font2 = font;
            }
            if (s < (float) rHeight)
                s++;
            curFont = DisplayOptions.getFont(font2.getName(),
                                                 font2.getStyle(),
                                                 (int)s);
        }
        else
            curFont = DisplayOptions.getFont(font.getName(),
                                             font.getStyle(),
                                             (int)s);
        String strfont = curFont.getName();
        int nstyle = curFont.getStyle();
        FontMetrics fm2 = getFontMetrics(curFont);
        while (s > 9.0f) {
            rWidth2 = 0;
            int fw = 0;
            Object obj;
            String str;
            for (int n = 0; n < getItemCount(); n++) {
                obj = (Object)getItemAt(n);
                if (obj != null) {
                    str = obj.toString();
                    if (str != null) {
                        fw = fm2.stringWidth(str);
                        if (fw > rWidth2)
                            rWidth2 = fw;
                    }
                }
            }
            rWidth2 += uiMinWidth;
            if (rWidth2 <= nWidth)
                 break;
             if (s < 10.0f)
                break;
            s = s - 1.0f;
            //curFont = curFont.deriveFont(s);
            curFont = DisplayOptions.getFont(strfont, nstyle, (int)s);
            fm2 = getFontMetrics(curFont);
        }
        fontH = curFont.getSize();
        fontRH = fm2.getHeight();
        myInset.top = (nHeight - fontRH) / 2;
        if (myInset.top < 0)
            myInset.top = 0;

        if(rWidth2 > w)
            rWidth2 = w;
        setFont(curFont);
    }

    public static void setMinWidth(int n) {
         uiMinWidth = n;
    }

}


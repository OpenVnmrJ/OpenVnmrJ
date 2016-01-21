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
import java.awt.dnd.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.io.*;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.Vector;

import javax.swing.*;
import java.awt.event.*;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import org.xml.sax.Attributes;
import org.xml.sax.SAXParseException;
import org.xml.sax.helpers.DefaultHandler;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.admin.util.*;
import vnmr.util.Util;

// Each button in each tab panel can be either a protocol, or a menu.
// There can be two levels of menu.  The ExperimentSelector.xml file can
// specify for each protocol, a tab, menu1 and menu2 for its location.

// The central information structure for this is as follows:
// All tab, menu1, menu2 and protocols will be have ExpSelEntry items
// created for them.  Each tab item will contain an entryList of
// ExpSelEntry items that will exist in that tab panel.  An item with
// an empty entryList will be a protocol.  If entryList is not empty, this
// item will be a menu and its entryList is the items in this menu.  Menus within
// the tab level are from the menu1 designation.  Each of these menu items
// will contain an entryList of ExpSelEntry items. Within these lists can
// be one more level of list from the menu2 designations.  Each of those
// can contain a list of ExpSelEntry item, but no further menu levels are
// allowed.  If there are menu2 level menus, then tabList will be a list
// four deep of HashArrayList's

// Shared directories (not system nor user) can also have ExperimentSelector.xml
// files in the interface directory.  After reading the user file and the
// system file, go through the shared directories and find those files
// and read them.  

// After reading all ExperimentSelector.xml, we need to look through all
// user and shared protocols for protocols that do not have enties from
// the ExperimentSelector.xml files.  If any user protocols are found,
// put them in a tab called User.  If any shared protocols are found, put
// them in a tab by the directory name/label.

// The ExperimentSelector_user.xml file can contain a first line such as 
// <ProtocolList userOnly="false" tooltip="label" labelWithCategory="false" iconForCategory='false'>
// To set various flags.  These were enabled like this for applab testing
// so not all have been fully tested.  The defaults within the code are
// the values given in the example line above.

public class ExpSelector extends JPanel implements DragSourceListener,
                                                   DragGestureListener,
                                                   PropertyChangeListener {
    public static ExpSelector expSelector=null;
    // tabList is a list of ExpSelEntry items which are each, one of
    // the tab panels. Each one contains a list of ExpSelEntry items in
    // that panel.
    static HashArrayList tabList = new HashArrayList();

    // Force an update the first call, and then anytime this
    // gets set, such as when the UI needs to be updated for a new operator
    static private boolean forceUpdate = true;

    // List of protocols from the ExperimentSelector file
    static public ArrayList<ExpSelEntry> protocolList = new ArrayList<ExpSelEntry>();
    
    
    // List of the names for each level that exist
    static public ArrayList<String> tabNameList = new ArrayList<String>();
    static public ArrayList<String> tabOrderList = new ArrayList<String>();
    static ArrayList<String> menu1NameList = new ArrayList<String>();
    static ArrayList<String> menu2NameList = new ArrayList<String>();
    
    static final int NOTDUP = 0;
    static final int COMPLETEDUP = 1;
    static final int DUPALLBUTPROTO = 2;
    static final int FILENUM = 12;
    static final int PITEMS = 7;

    // true -> Allow only protocols from this users ExperimentSelector_xxx.xml
    // file and not the system files
    static boolean userOnly = false;

    // true -> do not use menu1 and menu2
    static boolean sharedFlat = false;
    
    // true -> do not use menu1 and menu2
    static boolean userFlat = false;

    // true -> Show protocols that are not specified in ExperimentSelector.xml
    // in a tab with the name of that directory (ie., a catchall tab)
    static boolean showNonUserNonSpecifiedAppdirProtoInCatchAll = false;
    static boolean showUserNonSpecifiedAppdirProtoInCatchAll = true;

    // true -> put user and shared protocols in the normal panel with the
    // system protocols.  false -> put User protocols in User menus and
    // shared protocols in tabs with directory label name.
    static boolean allWithSystem = true;

    static String tooltipMode = "label";
    static String operatorXml = null;

    static boolean labelWithCategory = false;

    static boolean iconForCategory = false;
    static private boolean bCreatingProtocol = false;
    static private boolean bSuspendPnew = false;

    static private long lastMod[] = new long[FILENUM];
    static private long fileLength[] = new long[FILENUM];
    static private String[] pitems = new String[PITEMS];
    static private String[] pvalues = new String[PITEMS];

    Dimension zero = new Dimension(0, 0);

    Font labelFont;
    
    String fontName;

    static JTabbedPane jtp = null;

    SessionShare sshare;

    String protocolType, protocolName;

    private static Timer times = null;

    DroppableList listPanel;

    protected MouseAdapter m_mouseListener;

    public static UpdateCards updateCards = null;

    private static final ImageIcon downIcon = Util.getImageIcon("downblack.gif");    
    private static final ImageIcon userIcon = Util.getVnmrImageIcon("user_16.png");
    private static final ImageIcon shareIcon = Util.getVnmrImageIcon("users_16.png");
    private static final ImageIcon varianIcon = Util.getVnmrImageIcon("varian16.png");

    static int timerCount = 0;
    static String setPosition = "center";

    // protected boolean m_bSetInvisible = false;
    JPopupMenu popup = null;

    DragSource dragSource;
    LocalRefSelection lrs;

    public static ExpSelEntry lastDraggedEntry=null;
    public static int lastSelectedTab=0;
    
    public static String trashWhat="cancel";
    
    static public ArrayList<String> locTabList = new ArrayList<String>();
    private String uiOperator = null;


    // constructor
    public ExpSelector(SessionShare sshare) {
        // JPanel contructor
        super(new BorderLayout());
        this.sshare = sshare;
        expSelector = this;
        
        // Make the buttons have a bold font (Heading3), non bold (PlainText)
        fontName = new String("PlainText");
        labelFont = DisplayOptions.getFont(fontName,fontName,fontName);

        jtp = new JTabbedPane();
        jtp.setFont(labelFont);
//        jtp.setForeground(Color.BLUE);
//        Util.setBgColor(jtp);
        this.add(jtp, BorderLayout.CENTER);

        DisplayOptions.addChangeListener(this);

        // Create the ActionListener which is called every 10 sec to see
        // if anything needs to be updated. Save this for use by
        // external functions that need to force the update.

        updateCards = new UpdateCards();

        // Set the timer to call updateCards every 5 seconds to check
        // for needed update
        times = new Timer(5000, updateCards);

        // Set this time long enough that the program has time to fill the
        // appDirLabels.  If it is too short, we will catch that in 
        // UpdateCards and will abort until the next timer call.  So, too
        // short is not fatal, just causes an extra 10 sec before the next
        // call.  On my Dell 380, 5 sec is enough.
        times.setInitialDelay(5000);

        times.start();
    }

    static public ExpSelector getInstance() {
        return expSelector;
    }

    /* PropertyChangeListener interface */
    public void propertyChange(PropertyChangeEvent pce) {
        if(DisplayOptions.isUpdateUIEvent(pce)){
            SwingUtilities.updateComponentTreeUI(this);
        }

//        jtp.setForeground(DisplayOptions.getColor("PlainText"));
//        String strProperty = pce.getPropertyName();
//        if (strProperty == null)
//            return;
//        strProperty = strProperty.toLowerCase();
//        if (strProperty.indexOf("vjbackground") >= 0) {
//            Color bgColor = Util.getBgColor();
//            setBackground(bgColor);
//            WUtil.doColorAction(bgColor, "", this);
 //       }
        //setFont(labelFont);
    }

    // Utility to get the AppDirs list
    public ArrayList<String> getAppDirs() {
        ArrayList<String> dirs = FileUtil.getAppDirs();
        return (dirs);
    }

    // Catch the button click
    protected MouseAdapter getMouselistener() {
        if (m_mouseListener == null) {
            m_mouseListener = new MouseAdapter() {
                public void mousePressed(MouseEvent e) {
                    // Get the entry info stashed in the label so we can
                    // keep it available in case this item is being dragged
                    // to the trash.
                    DraggableLabel label = (DraggableLabel) e.getSource();
                    ExpSelEntry entry = parseActionCmd(label.getInfo());
                    if(entry != null)
                        lastDraggedEntry = entry;
                }
                public void mouseClicked(MouseEvent e) {
                    DraggableLabel label = (DraggableLabel) e.getSource();

                    // See if this is a menu or a protocol. Menus will
                    // have an icon associated with the text and protocols
                    // protocols will not.
                    HashArrayList subList = label.getSubList();
                    if (subList == null) {
                        // Protocol
                        // Don't try to use the info in label to make the
                        // ShufflerItem, just let the constructor fill it in.
                        // Set the source to EXPSELECTOR. 

                        // If the Ctrl key was held down, go for the system
                        // protocol if there is one
                        ShufflerItem shufflerItem;
                        int modifierMask = e.getModifiers();
                        if((modifierMask & ActionEvent.CTRL_MASK) != 0) {
                            // We need to get the system protocol
                            String systemProtPath = getSystemProtPath(label.name);
                            if(systemProtPath != null)
                                shufflerItem = new ShufflerItem(systemProtPath,
                                                            "EXPSELECTOR");
                            else
                                // If null was returned, a system protocol was
                                // not found, just use the default one
                                shufflerItem = new ShufflerItem(label.getFullpath(),
                                "EXPSELECTOR");
                        }
                        else
                            shufflerItem = new ShufflerItem(label.getFullpath(),
                                                            "EXPSELECTOR", label.bykeywords);
                        QueuePanel queuePanel = Util.getStudyQueue();
                        if (queuePanel != null) {
                            StudyQueue studyQueue = queuePanel.getStudyQueue();
                            if (studyQueue != null) {
                                ProtocolBuilder protocolBuilder = studyQueue
                                        .getMgr();
                                // Add this item to the studyQueue
                                protocolBuilder.appendProtocol(shufflerItem);
                            }
                        }
                        // There is no studyQ
                        else {
                            shufflerItem.actOnThisItem("Canvas", "DragNDrop", "");
                        }
                    }
                    else {
                        // Menu
                        boolean up = Util.isMenuUp();
                        // If not already up, bring up the popup
                        if (up)
                            Util.setMenuUp(false);
                        else
                            createMenuPopup(label, e, subList);
                    }
                }

            };
        }
        return m_mouseListener;
    }

    public void createMenuPopup(DraggableLabel label, MouseEvent evt,
            HashArrayList subList) {
        PopActionListener popActionListener = new PopActionListener(label);

        if (popup == null)
            popup = new JPopupMenu();
        else
            popup.removeAll();
        
        // Setting to null will cause the vnmrj system colors to work
        popup.setBackground(null);

        // Loop through items in the subList. If it is a protocol, create
        // a JMenuItem. If it is a menu, create a JMenu and then go through
        // the protocols for that JMenu and add them at that level.
        for (int i = 0; i < subList.size(); i++) {
            // This should be a protocol or a menu2
            ExpSelEntry entry = (ExpSelEntry) subList.get(i);
            if (entry.entryList == null) {
                // Must be a protocol, add a menuitem to the popup
                JMenuItem item = popup.add(entry.label);
                // Setting to null will cause the vnmrj system colors to work
                item.setBackground(null);

                dragSource = new DragSource();
                dragSource.createDefaultDragGestureRecognizer(
				item, DnDConstants.ACTION_COPY, this);


                // Set the ActionCommand to a comma separated list containing
                // all of the entry information
                String cmd = createActionCmd(entry);
                item.setActionCommand(cmd);
                item.addActionListener(popActionListener);
              
                item.addMouseListener(new MouseAdapter() {
                    public void mousePressed(MouseEvent e) {
                        Component c = e.getComponent();
                        c.addMouseListener(new MouseAdapter() {
                            public void mouseExited(MouseEvent e) {
                                // Setting a mouseExited event catcher after
                                // the mouse is pressed, allows me to come here
                                // when the mouse is dragged out of the item.
                                // At this point,
                                // I want to stop the JPopupMenu from continuing
                                // to select and highlight menu items as we drag
                                // down the list.  Else, it looks like we have
                                // changed active items when in fact we have
                                // not.  unshowing the popup causes the D&D to
                                // not work.  For some reason, it works to just
                                // unshow the JMenuItem component itself.  All
                                // of the components and the popup go away, but
                                // the drag is still valid
                                Component c = e.getComponent();
                                c.setVisible(false);
                            }
                        });
                   }
                });


                if(tooltipMode.equals("label")) {
                    item.setToolTipText(getAppDirLabel(entry.fullpath));
                }
                else if(tooltipMode.equals("fullpath"))
                    item.setToolTipText(entry.fullpath);
                
                // Determine and set the icon 
                if(iconForCategory) {
                    String category = getAppDirLabel(entry.fullpath);
                    if(category.equals("User"))
                        item.setIcon(userIcon);
                    else if(category.equals("System"))
                        item.setIcon(varianIcon);
                    else
                        item.setIcon(shareIcon);
                }
                
            }
            else {
                // Must be a menu2, create a JMenu and then fill it
                // with the protocols beneath this.
                JMenu mitem = new JMenu(entry.label);
//                mitem.setBackground(null);
                popup.add(mitem);
                
                // Loop through the lists below this which at this level
                // will only be protocols. Create menu items and add to
                // this jmenu
                for (int k = 0; k < entry.entryList.size(); k++) {
                    ExpSelEntry pentry = (ExpSelEntry) entry.entryList.get(k);
                    JMenuItem item = mitem.add(pentry.label);
                    // Setting to null will cause the vnmrj system colors to work
//                    item.setBackground(null);

                    dragSource = new DragSource();
                    dragSource.createDefaultDragGestureRecognizer(
                                         item, DnDConstants.ACTION_COPY, this);

                    String cmd = createActionCmd(pentry);
                    item.setActionCommand(cmd);
                    item.addActionListener(popActionListener);

                    item.addMouseListener(new MouseAdapter() {
                        public void mousePressed(MouseEvent e) {
                            Component c = e.getComponent();
                            c.addMouseListener(new MouseAdapter() {
                                    public void mouseExited(MouseEvent e) {
                                        // Setting a mouseExited event catcher after
                                        // the mouse is pressed, allows me to come here
                                        // when the mouse is dragged out of the item.
                                        // At this point,
                                        // I want to stop the JPopupMenu from continuing
                                        // to select and highlight menu items as we drag
                                        // down the list.  Else, it looks like we have
                                        // changed active items when in fact we have
                                        // not.  unshowing the popup causes the D&D to
                                        // not work.  For some reason, it works to just
                                        // unshow the JMenuItem component itself.  All
                                        // of the components and the popup go away, but
                                        // the drag is still valid
                                        Component c = e.getComponent();
                                        c.setVisible(false);
                                    }

                                });
                        }
                    });



                    if(tooltipMode.equals("label"))
                        item.setToolTipText(getAppDirLabel(pentry.fullpath));
                    else if(tooltipMode.equals("fullpath"))
                        item.setToolTipText(pentry.fullpath);
                    
                    if(iconForCategory) {
                        // Determine and set the icon if not system protocol
                        String category = getAppDirLabel(pentry.fullpath);
                        if(category.equals("User"))
                            item.setIcon(userIcon);
                        else if(category.equals("System"))
                            item.setIcon(varianIcon);
                        else
                            item.setIcon(shareIcon); 
                    }
                }
            }
        }

        // Get the location of the mouse click within the btn
        int x = evt.getX();
        int y = evt.getY();
        
        // set the popup menu location at the mouse click using the
        // show for this JPopupMenu.  Showing this way allows it to be
        // unshown by the system when the mouse is clicked anyplace on
        // the screen
        popup.show(evt.getComponent(), x, y);

        // I am afraid I don't really understand, but without this
        // MouseAdapter, if you click once to bring up the popup, and then
        // click again in exactly the same place, the menu will not go away.
        // Every other location seems to be caught by another listener.
        // This is simply to make the popup toggle if you just click several
        // times in the same place.
        popup.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                popup.setVisible(false);
            }
        });
    }

    // Decide whether a fullpath is System, User or Shared (Other)
    // If Shared, return the appDirLabel for this directory
    public String getAppDirLabel(String path) {
        // If no label found, default to Shared
        String label = "Shared";
        if(path.startsWith(FileUtil.sysdir()))
            return "System";
        else if(path.startsWith(FileUtil.usrdir()))
            return "User";
        else {
            // Must be a Shared directory.  Findout which index in the
            // appDirs this is and then get the label for that index.
            ArrayList<String> appDirs = FileUtil.getAppDirs();
            ArrayList<String> labels = FileUtil.getAppDirLabels();
            for (int k = 0; k < appDirs.size(); k++) {
                String dir = appDirs.get(k);
                if(path.startsWith(dir)) {
                    label = labels.get(k);
                    return label;
                }
            }
        }
        return label;
        
    }

    // This is to get the 'system' protocol of this name even though
    // the appdirs scheme may give a user or a shared protocol.
    // Go through the appdirs and only look in the system ones.
    String getSystemProtPath(String name) {
        ArrayList<String> appDirs = FileUtil.getAppDirs();
        String relativePath = FileUtil.getSymbolicPath("PROTOCOLS");
        for (int k = 0; k < appDirs.size(); k++) {
            String dir = appDirs.get(k);
            // Just take the system dirs
            if (dir.startsWith(FileUtil.sysdir())) {
                String path = dir  + File.separator + relativePath 
                    + File.separator + name;
                // Be sure it has the .xml suffix
                if(!path.endsWith(".xml"))
                    path = path + ".xml";
                UNFile file = new UNFile(path);
                if(file.exists())
                    return path;
            }
        }
        // Not found
        return null;
    }



    // Cleanup
    public void finalize() {
        if (times != null)
            times.stop();
        times = null;
        if (jtp != null)
            jtp.removeAll();
        jtp = null;
    }

    // Call this to add a protocol with its display info to the user's
    // ExperimentSelector_user.xml
    // Append a line to the end of the file
    // First create the line to be appended
    // Then check for duplicate lines and look for final line of file.
    static public void updateExpSelFile(String protocol, String label, String tab,
                                    String menu1, String menu2, String expSelNoShow,
                                    String opOnly) {
        String filepath;
        String line;
        boolean foundEnd=false;
        long point=0;
        RandomAccessFile raf=null;
        

        // If menu1 or menu2 is null or 'None', we need to convert that to
        // an empty string
        if(menu1 == null || menu1.equals("None")) {
            menu1 = "";
            menu2 = "";
        }
        if(menu2 == null || menu2.equals("None"))
            menu2 = "";
        
        // If label is null, use the protocol name
        if(label == null || label.length() == 0)
            label = protocol;
        
        // If protocol or tab is null, error out
        if(protocol == null || protocol.length() == 0) {
            Messages.postError("protocol must be set");
            return;
        }
        if(tab == null || tab.equals("Enter Tab Name")) {
            Messages.postError("tab must be set to have an entry for the " +
            		"Experiment Selector.\n   To create an entry, set the desired" +
            		" tab and use Update Protocol.");
            return;
        }
        // If the ExperimentSelector_op.xml file does not exist,
        // then we need to clear the protocol list so it will be refilled
        String curOperator = Util.getCurrOperatorName();
        filepath =  FileUtil.savePath("USER/INTERFACE/ExperimentSelector_"
                                      + curOperator + ".xml");
        try {
            UNFile file = new UNFile(filepath);
            if (!file.exists()) {
                protocolList.clear();
                tabList.clear();
                tabNameList.clear();
                tabOrderList.clear();
            }
        }
        catch(Exception ex) {
        
        }

        forceUpdate = true;

        // Do not allow a protocol label of the same name as a menu1
        // or a menu2.  Loop through the existing protocolList checking
        // all menu1 and menu2 values.  This assumes the protocolList has
        // been filled by a previous call to update the panel.
        // Don't have checkProtocolList() output an error message on failure,
        // create our own message.
        boolean status;
        status = checkProtocolList(label, tab, menu1, menu2, protocolList);
        // If there is a duplicate, bail out without writing to the file.
        if(status == false) 
            return;
        
        // Do not allow two entries at the same location with the same
        // label.  Since there is already an entry, they may be updating
        // a protocol.  Else they may be changing the protocol behind the
        // entry
        int dupStatus = isNotDup(protocol, tab, label, menu1, menu2, opOnly);
        
        if(dupStatus == DUPALLBUTPROTO) {
            // All is dup besides protocol.  They apparently want to change
            // the protocol related to this entry.
            Messages.postWarning("This new ES entry seems to be a duplicate of"
                    + " an existing entry\n  except the protocol has been changed."
                    + " If you intend to change the protocol for this entry,\n"
                    + "  first drag the entry to the trash, then make the new one.\n"
                    + "  protocol=" + protocol + "  tab=" + tab + "  label=" + label);
            return;      
        }
        else if(dupStatus == COMPLETEDUP) {
            // This is a complete duplicate.
            // Just bail out of here and leave the entry in
            // the ES file alone since it is already there.
            return;
        }
        
        // Create the line to be appended
        String newLine = new String("\t<protocol name=\"" + protocol 
                                    + "\"  label=\"" + label 
                                    + "\"  tab=\"" + tab
                                    + "\"  menu1=\"" + menu1
                                    + "\"  menu2=\"" + menu2 
                                    + "\"  ExpSelNoShow=\"" +  expSelNoShow
                                    + "\" />");
        
        filepath =  FileUtil.savePath("USER/INTERFACE/ExperimentSelector_"
                                      + curOperator + ".xml");
        try {
            UNFile file = new UNFile(filepath);
            if (file.exists()) {
                // The file exists, open it and add a line to it
                raf = new RandomAccessFile(file, "rw");
                while ((line = raf.readLine()) != null) {
                    // Find the closing line of the file
                    if (line.contains("</ProtocolList")) {
                        foundEnd = true;
                        break;
                    }
                    // We are not at the end, compare the current line
                    // with the new line and if duplicate, skip it
                    if(line.equals(newLine)) {
                        raf.close();
                        return;
                    }
                }
                if(foundEnd) {
                    // Back up in the file to the start of this line
                    point = raf.getFilePointer() - line.length() -1;
                    raf.seek(point);
                }
            }
            else {
                // The file does not exist, create it and write the opening
                // lines to it
                raf = new RandomAccessFile(file, "rw");
                raf.writeBytes("<ProtocolList userOnly=\"false\" tooltip=\"label\" iconForCategory=\"false\"> \n");

            }

            
            // Now, whether we found a file or create one, write the
            // the new entry and the ending to the file
            raf.writeBytes(newLine + "\n</ProtocolList>\n");
            raf.close();
            if(DebugOutput.isSetFor("expselector"))
                Messages.postDebug("Writing to ES_op.xml, " + newLine);

        }
        catch (Exception er) {
            Messages.postError("Problem creating  " + filepath);
            Messages.writeStackTrace(er);

        }  
        // cleanup the ExperimentSelector_operator.xml file
// Don't need to cleanup if we are going to remove the file every time
//        removeProtocolFromExpSelFile(null);
        
        // Force an update of the ExpSel
//        ActionEvent aevt = new ActionEvent(updateCards, 1, "force");
//        updateCards.actionPerformed(aevt);
        // Try not doing the update, but just setting the flag
        setForceUpdate(true);

    }

   public static int isNewItem(String name, String label, String tab,
                                String menu1, String menu2, String opOnly) {
        ExpSelEntry eSEntry;

        int psize=protocolList.size();
        for (int i = 0; i < protocolList.size(); i++) {
            eSEntry = protocolList.get(i);
            // Check label, tab, menu1 and menu2.  We do not want
            // dup location and diff protocol, so don't check protocol
            if (eSEntry.tab.equals(tab)
                    && eSEntry.label.equals(label)
                    && ((eSEntry.menu1 != null) && eSEntry.menu1
                            .equals(menu1))
                    && ((eSEntry.menu2 != null) && eSEntry.menu2
                            .equals(menu2))) {
                // It is a dup, excluding the protocol name, check that
                if( eSEntry.name.equals(name)) {
                    return COMPLETEDUP;
                }
                else {
                    return DUPALLBUTPROTO;
                }
            }
        }
        return NOTDUP;
    }

    static void setItemValue(int i, String str) {
        StringTokenizer tok;

        if (str == null || str.length() < 2)
            return;
        tok = new StringTokenizer(str, "\"");
        if (!tok.hasMoreTokens())
            return;
        tok.nextToken();   // skip label
        if (!tok.hasMoreTokens())
            return;
        pvalues[i] = tok.nextToken();
    }

    static public synchronized void buildProtocolFile(String dest, String src) {

        if (dest == null || src == null)
            return;

        bCreatingProtocol = true;
        forceUpdate = true;

        BufferedReader reader = null;
        PrintWriter    writer = null;
        String line, data;
        int i, rows;
        StringTokenizer tok;

        // Messages.postDebug("### build expSel file: "+dest);
        File f = new File(src);
        // Messages.postDebug("###  src: "+ src+"  size: "+f.length());

        operatorXml = dest;

        // the order of items: name, label, tab, menu1, menu2, noshow, opOnly
        try {
             writer = new PrintWriter( new FileWriter( dest ));
             writer.println("<ProtocolList userOnly=\"false\" tooltip=\"label\" iconForCategory=\"false\">");
             writer.println("</ProtocolList>");
             writer.close();
        }
        catch (Exception e) {
             bCreatingProtocol = false;
             return;
        }

        if (times != null)
             times.stop();

        protocolList.clear();
        tabList.clear();
        tabNameList.clear();
        tabOrderList.clear();

        fillProtocolList(false, "opOnly");

        // the order of items: name, label, tab, menu1, menu2, noshow, opOnly
        rows = 0;

        try {
             writer = new PrintWriter( new FileWriter( dest ));
             reader = new BufferedReader(new FileReader(src));
             writer.println("<ProtocolList userOnly=\"false\" tooltip=\"label\" iconForCategory=\"false\">");
             while ((line = reader.readLine()) != null) {
                 for (i = 0; i < PITEMS; i++) {
                      pvalues[i] = null;
                 }
                 tok = new StringTokenizer(line, ",:\n");
                 for (i = 0; i < PITEMS; i++) {
                      if (!tok.hasMoreTokens())
                          break;
                      setItemValue(i, tok.nextToken());
                 }
                 if (pvalues[0] == null || pvalues[0].length() < 1)
                     continue;
                 if (pvalues[2] == null || pvalues[2].equals("Enter Tab Name"))
                     continue;
                 if (pvalues[1] == null || pvalues[1].length() < 1)
                     pvalues[1] = pvalues[0];
                 if (pvalues[3] == null || pvalues[3].equals("None")) {
                     pvalues[3] = "";
                     pvalues[4] = "";
                 }
                 if (pvalues[4] == null || pvalues[4].equals("None"))
                     pvalues[4] = "";
                 int dupStatus = isNewItem(pvalues[0], pvalues[1], pvalues[2],
                     pvalues[3], pvalues[4], pvalues[6]);
                 if (dupStatus != NOTDUP)
                     continue;
                 StringBuffer sb = new StringBuffer("   <protocol name=\"");
                 sb.append(pvalues[0]).append("\" label=\"");
                 sb.append(pvalues[1]).append("\" tab=\"");
                 sb.append(pvalues[2]).append("\" menu1=\"");
                 sb.append(pvalues[3]).append("\" menu2=\"");
                 sb.append(pvalues[4]).append("\" ExpSelNoShow=\"");
                 sb.append(pvalues[5]).append("\" />");

                 writer.println(sb.toString());
                 rows++;
             }
             writer.println("</ProtocolList>");
        }
        catch (Exception e) { }
        finally {
            try {
               if (reader != null)
                   reader.close();
               if (writer != null)
                   writer.close();
               f = new File(src);
               // Messages.postDebug("###   src size: "+f.length()+"  items: "+ rows);
            }
            catch (Exception e2) {}
         }
         bCreatingProtocol = false;

         setForceUpdate(true);
    }

    // In the menu, we need to know all of the information in the ExpSelEntry
    // object.  So, create a string with all of the info separated by ','.
    // Then we can parse out the info we want when we need it.  Create it
    // with 'fullpath,name,label,tab,menu1,menu2,bykeywords'
    private String createActionCmd(ExpSelEntry entry) {
        String CmdStr;

        CmdStr = entry.fullpath + "," + entry.name + "," + entry.label 
                 + "," + entry.tab + "," + entry.menu1 + "," + entry.menu2 
                 + "," + entry.bykeywords;
        return CmdStr;
    }


    // Create an ExpSelEntry from the comma separate command string
    // used as the ActionCmd
    private ExpSelEntry parseActionCmd(String cmd) {
        StringTokenizer tok;
        ExpSelEntry entry = new ExpSelEntry();

        if(cmd == null)
            return null;
        tok = new StringTokenizer(cmd, ",");
        int count = tok.countTokens();
        if(count < 4) {
            Messages.postWarning("Problem removing Exp Sel entry");
            return null;
        }
        entry.fullpath = tok.nextToken();
        entry.name = tok.nextToken();
        entry.label = tok.nextToken();
        entry.tab = tok.nextToken();
        if(tok.hasMoreTokens())
            entry.menu1 = tok.nextToken();
        else
            entry.menu1 = "";
        
        if(tok.hasMoreTokens())
            entry.menu2 = tok.nextToken();
        else
            entry.menu2 = "";
       
        return entry;

    }


    // Routine to remove one entry of the given protocol from the
    // ExperimentSelector_user.xml file.  Assume it is the last protocol
    // which was selected/clicked as they were dragging it to the trash.
    static public void removeProtocolEntryFromExpSelFile(String name) {
        RandomAccessFile input=null;
        FileWriter fw;
        PrintWriter output=null;
        String infilepath, outfilepath;
        String line;
        UNFile infile, outfile;

        // Be sure the name of the last dragged etnry item matches
        // the name of the protocol entry to be removed.
        // Accept the protocol name with or without the .xml, then is it
        // has a .xml, remove it.
        int index = name.indexOf(".xml");
        if(index != -1) 
            name = name.substring(0, index);        
        if(!lastDraggedEntry.name.equals(name)) {
            Messages.postDebug("lastDraggedEntry.name does not match name");
            return;
        }

        // Short convienient name
        ExpSelEntry lde = lastDraggedEntry;
        
        // Now we need to search through the ExperimentSelector_user.xml
        // file for an entry will all of the same attributes as the
        // lastDraggedEntry.  If found, remove it.

        // Get the path for the current file
        String curOperator = Util.getCurrOperatorName();
        infilepath =  FileUtil.savePath("USER/INTERFACE/ExperimentSelector_"
                                      + curOperator + ".xml");

        // Make a new file for output with 'new' appended to the name
        outfilepath = infilepath + ".new";
        
        try {
            // Open an output file writer
            outfile = new UNFile(outfilepath);
            fw = new FileWriter(outfile);
            output = new PrintWriter(fw);
            if(output == null) {
                Messages.postError("Problem creating " + outfilepath);
                return;
            }
        }
        catch (Exception er) {
            Messages.postError("Problem opening  " + outfilepath);
            Messages.writeStackTrace(er);
            return;
        }
        boolean skipit = false;
        try {
            // Open the input file
            infile = new UNFile(infilepath);
            if (infile.exists()) {
                // The file exists, open it
                input = new RandomAccessFile(infile, "r");
                while ((line = input.readLine()) != null) {
                    skipit = false;
                    if(line.indexOf("protocol name=\"" + lde.name + "\"") != -1) {
                        // found entry with the right name, try label
                        if(line.indexOf("label=\"" + lde.label + "\"") != -1) {
                            // found entry with the right label, try tab
                            if(line.indexOf("tab=\"" + lde.tab + "\"") != -1) {
                                // found entry with the right tab, try menu1
                                if(line.indexOf("menu1=\"" + lde.menu1 + "\"") != -1) {
                                    // found entry with the right menu1, try menu2
                                    if(line.indexOf("menu2=\"" + lde.menu2 + "\"") != -1) {
                                        // Found a matching line, don't write it
                                        skipit = true;
                                    }
                                }
                            }
                        }
                    }
                    if(!skipit)
                        output.println(line);

                }
                output.close();
                input.close();
            }
            else
                // No file, just bail out
                return;
        }        
        catch (Exception er) {
            Messages.postError("Problem opening  " + infilepath);
            Messages.writeStackTrace(er);
            return;
        }
        // Remove the original file and rename the new one into its place
        try {
            if(infile != null && infile.exists())
                infile.delete();
            if(outfile != null && outfile.exists())
                outfile.renameTo(infile);
        }
        catch (Exception er) {
            Messages.postError("Problem renaming " + outfilepath 
                    + " to " + infilepath);
            Messages.writeStackTrace(er);
            return;
        }
              
        // Force an update of the ExpSel
        updateExpSel();

    }

    
    // Routine to remove all entries of the given protocol from the 
    // ExperimentSelector_user.xml file.
    // Read the file one line at a time.  If the line has the string
    // 'protocol name="protocolName"'(without ".xml", then don't write it 
    // back out.  Else,  write out all the other lines to a new file.
    // If arg name == null, then check ExperimentSelector_user.xml file
    // for entries whose protocol no longer exists and remove those lines
    static public void removeProtocolFromExpSelFile(String name) {
        RandomAccessFile input=null;
        FileWriter fw;
        PrintWriter output=null;
        String infilepath, outfilepath;
        String line;
        UNFile infile, outfile;
        String protocol;

        
        // accept the protocol name with or without the .xml, then is it
        // has a .xml, remove it.
        if(name != null) {
            int index = name.indexOf(".xml");
            if(index == -1) 
                protocol = name;
            else
                protocol = name.substring(0, index);
        }
        else
            protocol = "";
        
        // Get the path for the current file
        String curOperator = Util.getCurrOperatorName();
        infilepath =  FileUtil.savePath("USER/INTERFACE/ExperimentSelector_"
                                      + curOperator + ".xml");

        // Make a new file for output with 'new' appended to the name
        outfilepath = infilepath + ".new";
        
        try {
            // Open an output file writer
            outfile = new UNFile(outfilepath);
            fw = new FileWriter(outfile);
            output = new PrintWriter(fw);
            if(output == null) {
                Messages.postError("Problem creating " + outfilepath);
                return;
            }
        }
        catch (Exception er) {
            Messages.postError("Problem opening  " + outfilepath);
            Messages.writeStackTrace(er);
            return;
        }
        
        try {
            // Open the input file
            infile = new UNFile(infilepath);
            if (infile.exists()) {
                // The file exists, open it
                input = new RandomAccessFile(infile, "r");
                while ((line = input.readLine()) != null) {
                    // If arg name == null, then check ExperimentSelector_user.xml file
                    // for entries whose protocol no longer exists and remove those lines
                    if(name == null || name.length() == 0) {
                        // Get the protocol name on this line
                        int index1 = line.indexOf("protocol name=");
                        if(index1 > 0) {
                            index1 = index1 + "protocol name=".length();
                            int index2 = line.indexOf("label=");
                            String curProto = line.substring(index1, index2-1);
                            curProto = curProto.trim();
                            // If the protocol name had double quotes, we will
                            // will have them now.  We need to remove them
                            if(curProto.indexOf("\"") > -1)
                                curProto = curProto.substring(1, curProto.length() -1);
                            
                            String ppath;
                            String fullpath=null;
                            ppath = "PROTOCOLS" + File.separator + curProto
                            + ".xml";
                            fullpath = FileUtil.openPath(ppath);
                            if (fullpath != null){
                                // Found, Keep this line
                                output.println(line);
                            }
                        } 
                        else
                            // Not a protocol name line
                            output.println(line);
                    }
                    else if(line.indexOf("protocol name=\"" + protocol + "\"") == -1) {
                        // The protocol is not found on this line
                        output.println(line);
                    }
                    else {
                        // This line contains this protocol, skip it
                    }

                }
                output.close();
                input.close();
            }
            else
                // No file, just bail out
                return;
        }        
        catch (Exception er) {
            Messages.postError("Problem opening  " + infilepath);
            Messages.writeStackTrace(er);
            return;
        }
        // Remove the original file and rename the new one into its place
        try {
            if(infile != null && infile.exists())
                infile.delete();
            if(outfile != null && outfile.exists())
                outfile.renameTo(infile);
        }
        catch (Exception er) {
            Messages.postError("Problem renaming " + outfilepath 
                    + " to " + infilepath);
            Messages.writeStackTrace(er);
            return;
        }
        // Force an update of the ExpSel
        updateExpSel();

    }

    static public void updateExpSel() {
        // Force an update of the ExpSel
        /***
        if(updateCards != null) {
            ActionEvent aevt = new ActionEvent(updateCards, 1, "force");
            updateCards.actionPerformed(aevt);
        }
        ***/
        setForceUpdate(true);
    }

    /* DragSourceListener */
    public void dragDropEnd (DragSourceDropEvent e) {
    }

    public void dragEnter (DragSourceDragEvent e) {
    }

    public void dragExit (DragSourceEvent e) {
    }

    public void dragOver (DragSourceDragEvent e) {
    }

    public void dropActionChanged (DragSourceDragEvent e) {
    }

    /* DragGestureListener */
    public void dragGestureRecognized (DragGestureEvent e) {
        // Get the fullpath for this item
        JMenuItem item = (JMenuItem)e.getComponent();
        String cmd = item.getActionCommand();
        ExpSelEntry entry = parseActionCmd(cmd);
        String fullpath = entry.fullpath;

        // Save the last item dragged so that if it goes to the trash,
        // we will know what item was dragged.
        lastDraggedEntry = entry;
        
        // Must not be a protocol, don't allow dragging of menus
        if(fullpath == null || fullpath.length() == 0) {
            return;
        }
        
        // Since the code is accustom to dealing with 
        // ShufflerItem's being D&D, I will just use that 
        // type and fill it in.
        ShufflerItem si = new ShufflerItem(fullpath, "EXPSELECTOR");
        lrs = new LocalRefSelection(si);
        dragSource.startDrag ( e, DragSource.DefaultCopyDrop, lrs, this);
	    
        popup.setVisible(false);
    }
    
    static public synchronized void setForceUpdate(boolean value) {
        if (value) {
           forceUpdate = true;
           timerCount = 0;

           if (times != null) {
               times.setInitialDelay(3000);
               times.restart();
           }
        }
        else {
           if (timerCount > 3)
               timerCount = 3;
           if (times != null) {
               times.restart();
           }
        }
        ExpSelTree.setForceUpdate(value);
    }

    static public synchronized boolean needUpdate() {
        return forceUpdate;
    }

    static public synchronized void waitLogin(boolean bWait) {
        bSuspendPnew = bWait;
        if (bWait) {
            if (times != null)
                times.stop();
        }
        ExpSelTree.waitLogin(bWait);
    }

    static public synchronized void startLogin() {
        operatorXml = null;
        for (int i = 0; i < FILENUM; i++) {
             lastMod[i] = 0;
             fileLength[i] = 0;
        }
        setForceUpdate(true);
    }

    static public String getOperatorFileName() {
         return operatorXml;
    }

    // Called when pnew happens so we can check expselnu and expselu
    // for changes. Set the local variable and force an update of the exp selector
    
    // 3/16/10, the variables showUser... and showNon... are currently not
    // used.  The logic was moved to the macro updateExpSelector.  Here we
    // still need to flag the "update" so that changes to the vnmr parameters
    // will be caught and cause an update to the ES.
    static public void updatePnewValues(Vector<String> parVal) {
        int size;
        boolean update=false;

        if (bSuspendPnew)
            return;
        size = parVal.size();
        for (int i = 0; i < size; i++) {
            String paramName = (String) parVal.elementAt(i);
            if (paramName.equals("expselnu")) {
                // Found it, now get the value from the last half of the Vector
                /********
                String val = (String)parVal.elementAt(size/2 +i);
                if(val.equals("1"))
                    showNonUserNonSpecifiedAppdirProtoInCatchAll = true;
                else
                    showNonUserNonSpecifiedAppdirProtoInCatchAll = false;
                ********/
                break;

            }
            else if (paramName.equals("expselu")) {
                // Found it, now get the value from the last half of the Vector
                /********
                String val = (String)parVal.elementAt(size/2 +i);
                if(val.equals("1"))
                    showUserNonSpecifiedAppdirProtoInCatchAll = true;
                else
                    showUserNonSpecifiedAppdirProtoInCatchAll = false;
                ********/
                // Unfortunately, we get a call from pnew for each parameter
                // so only trigger an update for one of them
                update=true;
                break;
            }
        }

        if(update) {
            // Run the updateExpSelector macro to create (if needed) a new ES_op.xml file
            if (times != null)
                times.stop();
            Util.getAppIF().sendToVnmr("updateExpSelector");

            // the updateExpEelector will write a new ExperimentSelector_op.xm
            // file which will trigger the ES to update.
        }

    }


    static public synchronized void fillTabList() {
        // clear out the list of protocols and start over
        protocolList.clear();
        tabList.clear();
        tabNameList.clear();
        tabOrderList.clear();

            
            
        // Clear/default the mode flags
        // true -> Allow only protocols from this users
        // ExperimentSelector_xxx.xml file and not the system files
        userOnly = false;
        // true -> do not use menu1 and menu2
        sharedFlat = false;
        // true -> do not use menu1 and menu2
        userFlat = false;
        allWithSystem = true;
        tooltipMode = "label";
        labelWithCategory = false;
        iconForCategory = false;

        // Now we are ready to really start updating if that was
        // determined to be necessary

        if (DebugOutput.isSetFor("expselector")) {
            String operator = Util.getCurrOperatorName();
            Messages.postDebug("UpdateCards: updating Exp Selector for "
                               + operator);
        }

        // fill the protocolList 
        fillProtocolList();
            
        // check the list for protocol names that are duplicated of 
        // menu names and disallow.  checkProtocolList() will output an
        // error message.
        if(!checkProtocolList(protocolList))
            return;
            
        // Now we should have all protocols added into the protocolList
        // Be sure each one exists, be sure each one is approved and
        // then make buttons and menus.


        // rightsList will contain the list of protocols that the user
        // does not want display with an approve = false
        boolean all = true;
        RightsList rightsList = new RightsList(all);

        // Messages.postDebug(" protocols: "+ protocolList.size());
        // Now go through the list of protocols we have and see if they exist
        // and if they are supposed to be displayed. If so, create the buttons
        for (int i = 0; i < protocolList.size(); i++) {
            ExpSelEntry protocol = protocolList.get(i);

            // If the operator has a RightsConfig.txt persistence file,
            // it may request that some of these protocols NOT be
            // displayed. Test the list from persistence, and if a
            // protocol is tagged 'false', leave it out of this list.
            // For protocols, the keyword in rightsList is equal to name,
            // so use name here for the keyword
            if (rightsList.approveIsFalse(protocol.name))
                continue;
                
            if(protocol.noShow != null && protocol.noShow.equals("true"))
                continue;

            // See if the protocol exists in one of the appdir directories
            // If not, don't put it into the exp selector

            String ppath = "PROTOCOLS" + File.separator + protocol.name
                + ".xml";
            // Returns null if file not found
            String fullpath = FileUtil.openPath(ppath);
            if (fullpath != null) {
                // File found, set the fullpath in the item, since we have
                // it here.
                protocol.fullpath = fullpath;
                // If the tab is not set, then skip this one
                if (protocol.tab == null || protocol.tab.equals("")) {
                    Messages.postWarning(protocol.name
                                         + " Missing 'tab' designation.");
                    continue;
                }
                try {
                    // Does the tab designated exist yet?
                    ExpSelEntry tab;
                    if (!tabList.containsKey(protocol.tab)) {
                        // This tab does not exist yet, make it
                        tab = new ExpSelEntry();
                        tab.label = new String(protocol.tab);
                        tab.entryList = new HashArrayList();
                        // Add to tabList
                        tabList.put(protocol.tab, tab);
                            
                    }
                    else {
                        // The tab already exists, get this one
                        tab = (ExpSelEntry) tabList.get(protocol.tab);
                            
                    }
                    // Save a list of tab labels for the panel menu
                    // and for the tab order editing panel
                    if(!locTabList.contains(tab.label))
                        locTabList.add(tab.label);

                    // Now we have the ExpSelEntry item for this tab,
                    // Check to see if the current protocolList item goes
                    // directly in this tab, or if a menu1 is specified.
                    ExpSelEntry menu1;
                    ExpSelEntry menu2;
                    if (protocol.menu1 == null || protocol.menu1.equals("")) {
                        // The protocol goes directly in this tab
                        // See if there is already a menu1 by this name
                        // in this tab, we cannot have two items in the
                        // hashtable with the same key.
                        ExpSelEntry entry = (ExpSelEntry) tab.entryList.get(protocol.label);
                        if(entry == null)
                            // Nothing by that name yet, add it
                            tab.entryList.put(protocol.label, protocol);
                        else {
                            // There is already something by that name.  Is
                            // is the same type item?  If not, give error.
                            // In this case it should either be a protocol
                            // or a menu1.  If it is a menu, fullpath will
                            // not be set.
                            if((entry.fullpath == null  && protocol.fullpath != null) ||
                               (protocol.fullpath == null && entry.fullpath != null)) {
                                // They are not the same type, error out
                                Messages.postError("Cannot specify a protocol and " 
                                                   + "a \'menu1\' of the same label ("
                                                   + protocol.label + ")");
                                continue;
                            }
                        }
                    }
                    else {
                        // The protocol is at least one level down, so we need
                        // to see if the specified menu1 already exists.
                        // If not, create one.
                        ExpSelEntry entry = (ExpSelEntry) tab.entryList.get(protocol.menu1);
                        if (entry == null) {
                            // It does not exist yet
                            menu1 = new ExpSelEntry();
                            menu1.label = new String(protocol.menu1);
                            menu1.entryList = new HashArrayList();
                            // Add to the tab
                            tab.entryList.put(protocol.menu1, menu1);
                                
                            // Save a list of menu1 labels for the panel menu
                            if(!menu1NameList.contains(menu1.label))
                                menu1NameList.add(menu1.label);
  
                        }
                        else {
                            // It exists, is the existing entry a menu1
                            // or a protocol?  If it is a protocol, error out.
                            // We cannot add a menu1 and a protocol of the
                            // same name.  Protocols have fullpath != null
                            if(entry.fullpath != null) {
                                // It is a protocol, error out
                                Messages.postError("Cannot specify a protocol and " 
                                                   + "a \'menu1\' of the same name ("
                                                   + protocol.menu1 + ")");
                                continue;
                            }
                                
                            // It exists, save it
                            menu1 = entry;
                        }
                        // We need to know if the protocol goes directly into
                        // menu1 or is there is another level.
                        // Does it have a menu2 set?
                        if (protocol.menu2 == null
                            || protocol.menu2.equals("")) {
                            // No menu2, it goes in the menu1 level
                            // See if there is already a menu2 by this name.
                            // we cannot have a protocol and a menu2 by the
                            // same name.
                            entry = (ExpSelEntry) menu1.entryList.get(protocol.label);
                            if(entry == null)
                                // Nothing by that name yet, add it
                                menu1.entryList.put(protocol.label, protocol);
                            else {
                                // There is already something by that name.  Is
                                // is the same type item?  If not, give error.
                                // In this case it should either be a protocol
                                // or a menu.  If it is a menu, fullpath will
                                // not be set.
                                if((entry.fullpath == null  && protocol.fullpath != null) ||
                                   (protocol.fullpath == null && entry.fullpath != null)) {
                                    // They are not the same type, error out
                                    Messages.postError("Cannot specify a protocol and " 
                                                       + "a \'menu2\' of the same label ("
                                                       + protocol.label + ")");
                                    continue;
                                }
                            }
                        }
                        else {
                            // Okay, there is a menu2, lets go one more
                            // level.
                            // Does this menu2 exists yet within menu1?
                            entry = (ExpSelEntry) menu1.entryList.get(protocol.menu2);
                            if (entry == null) {
                                // It does not exist yet
                                menu2 = new ExpSelEntry();
                                menu2.label = new String(protocol.menu2);
                                menu2.entryList = new HashArrayList();
                                // Add to menu1
                                menu1.entryList.put(protocol.menu2, menu2);
                                    
                                // Save a list of menu2 labels for the panel menu
                                if(!menu2NameList.contains(menu2.label))
                                    menu2NameList.add(menu2.label);
                            }
                            else {
                                // It exists, is the existing entry a menu2
                                // or a protocol?  If it is a protocol, error out.
                                // We cannot add a menu2 and a protocol of the
                                // same name.  Protocols have fullpath != null
                                if(entry.fullpath != null) {
                                    // They are not the same type, error out
                                    Messages.postError("Cannot specify a protocol and " 
                                                       + "a \'menu2\' of the same name ("
                                                       + protocol.menu2 + ")");
                                    continue;
                                }
                                
                                // It exists, save it
                                menu2 = entry;
                            }
                            // There cannot be any more levels, if we reach
                            // this point, add the protocol.
                            menu2.entryList.put(protocol.label, protocol);
                        }
                    }

                }
                catch (Exception e) {
                    Messages.writeStackTrace(e);
                    // check the next appdir directory
                    continue;
                }

            }
            else {
                // If we did not find this protocol, then log that fact
                Messages.postDebug("Cannot find protocol " + protocol.name
                                   + ".xml" + " for Experiment Selector."
                                   + "\n   Possibly in user\'s profile "
                                   + "or ExperimentSelector.xml file, but not "
                                   + "in protocols dir.");
            }
        }
    }








    
//    static public HashArrayList getTabList() {
//        if(tabList.size() == 0)
//            // If empty, try to fill it
//            fillTabList();
//        
//        if(tabList.size() == 0)
//            // If still empty, return null
//            return null;
//        else
//            return tabList;
//    }

    private boolean isFileChanged(int index, String path) {
        long modTime, len;
        File file;
        boolean bChanged = false;

        if (path != null) {
             file = new File(path);
             modTime = file.lastModified();
             len = file.length();
             if (modTime != lastMod[index] || len != fileLength[index]) {
                 bChanged = true;
                 lastMod[index] = modTime;
                 fileLength[index] = len;
            }
        }
        else if (fileLength[index] != 0) {
            bChanged = true;
            fileLength[index] = 0;
        }

        return bChanged;
    }


    public synchronized boolean checkAndBuild() {
            boolean update = false;
            String profile;
            // File profileFile = null;
            int  index;
            
            if (bCreatingProtocol)
                return false;

            // When the UI is first coming up, AppDirLabels may not have been 
            // filled yet. Get the appdir list and see if the number of entries 
            // is the same.
            ArrayList<String> labels1;
            // Get the appDirLabels list
            labels1 = FileUtil.getAppDirLabels();
        
            // bail out if appdirs not filled yet.  The timer will bring us back
            // shortly to try again.  If the initial timer start is set
            // long enough, we will have this set when we arrive the first
            // time.  This test is just in case things are running slow.
            if(labels1.size() == 0) {
                ArrayList<String> appdirs = FileUtil.getAppDirs();

                if (DebugOutput.isSetFor("expselector"))
                    Messages.postDebug("ExpSelector: AppDirLabels empty, "
                                       + "skipping update this time.  appdirs size = "
                                       + appdirs.size() );
                return false;
            }

            if (forceUpdate) {
                forceUpdate = false;
                update = true;
            }

            // Check to see if this user has a profile set in the operator
            // login info
            String curOperator = Util.getCurrOperatorName();
            if (uiOperator == null || !(curOperator.equals(uiOperator))) {
                uiOperator = curOperator;
                update = true;
            }

            // Get Profile Name column string for access to the operator data
            String pnStr = vnmr.util.Util.getLabel("_admin_Profile_Name");
            profile = WUserUtil.getOperatordata(curOperator, pnStr);

            // This could be temporary, but DefaultApproveAll does not exist
            // as a profile and AllLiquids does exist, so use it.
            if (profile == null || profile.length() == 0) {
                profile = "AllLiquids";
            }
            else if (profile.equals("DefaultApproveAll")) {
                profile = "AllLiquids";
            }

            // We arrive here about every 10 sec.

            // There is a persistence file for each operator

            index = 0;
            // Check the operators RightsConfig.txt file for changes
            String filepath = FileUtil.openPath("USER" + File.separator
                    + "PERSISTENCE" + File.separator + "RightsConfig_"
                    + curOperator + ".txt");
            if (isFileChanged(index, filepath))
                update = true;

            // Check the users protocol directory for date change
            // Get the current user object
            String curuser = System.getProperty("user.name");
            LoginService loginService = LoginService.getDefault();
            Hashtable userHash = loginService.getuserHash();
            User user = (User) userHash.get(curuser);
            filepath = FileUtil.userDir(user, "PROTOCOLS");
            index = 1;
            if (isFileChanged(index, filepath))
                update = true;

            // Check the users assigned profile for changes
            /**********
            index = 2;
            if (profileFile != null) {
                if (isFileChanged(index, filepath))
                    update = true;
            }
            **********/

            index = 3;
            // Check the users ExperimentSelector_xxx.xml file for changes
            filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                    + curOperator + ".xml");
            if (isFileChanged(index, filepath))
                 update = true;

            index++;
                // The operator must not have a file.  Try the User's file
            String userName = user.getAccountName();
            filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                                             + userName + ".xml");
            if (isFileChanged(index, filepath))
                 update = true;

            index++;
            // Check the users appdir file for changes
            filepath = FileUtil.openPath("USER" + File.separator
                    + "PERSISTENCE" + File.separator + "appdir_"
                    + curOperator);

            if (isFileChanged(index, filepath))
                 update = true;

            index++;
            // Check the system ExperimentSelector.xml file for changes
            filepath = FileUtil.openPath("INTERFACE/ExperimentSelector.xml");

            if (isFileChanged(index, filepath))
                 update = true;

            index++;
            if (isFileChanged(index, operatorXml))
                 update = true;

            if (!update) {
                return false;
            }
            
            fillTabList();

            DraggableLabel tmpLabel;
            JPanel panel;
            
            // If we have a valid jtp, save its selected tab so that we can
            // try to return the user to this tab after rebuilding the
            // whole thing.  Sometimes the index will be a different
            // tab than the user had, but Oh Well, we will try.
            if(jtp != null) {
                lastSelectedTab = jtp.getSelectedIndex();
                jtp.removeAll();
           }

            // Now we have all of the information we need in the tabList tree
            // We need to create all of the tabs and menus as specified.
            // Go through the list of tabs

            // Messages.postDebug(" expSel tabs: "+ tabList.size());
            for (int i = 0; i < tabList.size(); i++) {
                ExpSelEntry tab;

                // Get a tab from the list
                tab = (ExpSelEntry) tabList.get(i);
                // Be sure the tab has something in it, else don't display it
                HashArrayList hal = tab.getEntryList();
                if (hal.size() == 0)
                    continue;
                
                // Create a tab panel
                panel = new JPanel(new GridLayout(0, 2));
                JScrollPane scroll = new JScrollPane(panel);
                jtp.addTab(Util.getLabelString(tab.label), scroll);

                // Go through the protocols and menus in the tab and create
                // necessary buttons. Level 1 (goes in tab panel)
                HashArrayList list1 = tab.entryList;
                String btn1String;
                for (int k = 0; k < list1.size(); k++) {
                    ExpSelEntry entry1 = (ExpSelEntry) list1.get(k);
                    // If the entry has no list specified, it is a protocol
                    if (entry1.entryList == null) {
                        // Get and set the fullpath
                        String ppath = "PROTOCOLS" + File.separator
                                + entry1.name + ".xml";
                        String fullpath = FileUtil.openPath(ppath);
                        entry1.fullpath = fullpath;
                        // Protocol
                        
                        String category = getAppDirLabel(fullpath);
                        
                        btn1String = entry1.label;
                        
                        // Allow the user to specify multiple lines for the
                        // Label.  The Sax Parser will not allow html syntax
                        // in the ExperimentSelector.xml file, so have the user
                        // use \n.  Then we need to catch those and replace
                        // with <br> and add <html> at the beginning.
                        // To further make it hard, the String.replaceAll()
                        // method will not work on the "\\n" string even 
                        // though indexOf() does work.  Allow more that one
                        // newline giving any multiple of lines.
                        
                        // Allow "\\n".  If these, catch in the first loop
                        // and replace.  If "\n" Catch in the second loop.
                        while((index = btn1String.indexOf("\\\\n"))>= 0) {
                            String begStr = btn1String.substring(0, index);
                            String endStr = btn1String.substring(index +3);
                            // If second pass, we don't need another <html>
                            if(btn1String.indexOf("<html>") == -1)
                                btn1String = "<html>" + begStr + "<br>" + endStr;
                            else
                                btn1String = begStr + "<br>" + endStr;
                        }
                        while((index = btn1String.indexOf("\\n"))>= 0) {
                            String begStr = btn1String.substring(0, index);
                            String endStr = btn1String.substring(index +2);
                            // If second pass, we don't need another <html>
                            if(btn1String.indexOf("<html>") == -1)
                                btn1String = "<html>" + begStr + "<br>" + endStr;
                            else
                                btn1String = begStr + "<br>" + endStr;
                        }

                        // Specify the font in making the label
                        tmpLabel = new DraggableLabel(btn1String, fontName);
                        tmpLabel.setName(entry1.name);
                        tmpLabel.setFileName(fullpath);
                        tmpLabel.setBykeywords(entry1.bykeywords);
                        tmpLabel.setSource("EXPSELECTOR");
                        tmpLabel.setAuthor(curOperator);
                        tmpLabel.addMouseListener(getMouselistener());
                        // Set the font for the buttons
                        tmpLabel.setFont(labelFont);
                        
                        // Save the entry info for D&D
                        String info = createActionCmd(entry1);
                        tmpLabel.setInfo(info);

                        // Set the tooltip
                        if(tooltipMode.equals("label"))
                            tmpLabel.setToolTipText(getAppDirLabel(fullpath));
                        else if(tooltipMode.equals("fullpath"))
                            tmpLabel.setToolTipText(fullpath);
                        
                        // Set the icon/text combination position
                        if(setPosition.equals("left"))
                            tmpLabel.setHorizontalAlignment(JButton.LEFT);
                        else if(setPosition.equals("center"))
                            tmpLabel.setHorizontalAlignment(JButton.CENTER);
                        else if(setPosition.equals("right"))
                            tmpLabel.setHorizontalAlignment(JButton.RIGHT);
                        if(iconForCategory) {
                            if(category.equals("User"))
                                tmpLabel.setIcon(userIcon);
                            else if(category.equals("System"))
                                tmpLabel.setIcon(varianIcon);
                            else
                                tmpLabel.setIcon(shareIcon);
                        }
                        
                        // I made changes to allow the setting of foreground and
                        // background colors individually for each protocol label.  
                        // The colors can be entered in the ExperimentSelector.xml file as
                        // 'fgColor="mycolor"' and 'bgcolor="anothercolor"'.  
                        // Where the colors can be any of the colors found in the 
                        // DisplayOptions panel (like burlywood) or can be a color 
                        // like red, blue, green or it can take symbolic names such as 
                        // "Menu1", "Heading3", "Label2" etc.
                        // If a symbolic name is given, the item will track the
                        // color for that name when changed in the Display Options panel.
                        // The values given for 'fgcolor' and bgcolor are saved in each
                        // DraggableLable object so they can be used by that
                        // object's propertyChange().  
                        
                        // The color of the menu1 and menu2 items along side of
                        // the protocols, is using the color of "plain text".  Thus
                        // changing that in the DisplayOptions panel will change the
                        // color of the menu items.
                        
                        // If no color is specified, it will use the "plain text" color
                        
                        // Currently, (3/09) the Create Protocols panel, has not been modified
                        // to allow a color choice when creating a protocol.  The 
                        // ExperimentSelector.xml file must be hand edited.
                        
                        // Without Opaque set, the setBackground does not work
                        tmpLabel.setOpaque(true);

                        // If a color was set for the label, use it here
                        // Save color in each item for DraggableLabel.propertyChange()
                        tmpLabel.fgColorTx = entry1.fgColorTx;
                        tmpLabel.bgColorTx = entry1.bgColorTx;
                        if(entry1.fgColorTx != null && !entry1.fgColorTx.equals("")) {
                            Color fgColor=DisplayOptions.getColor(entry1.fgColorTx);
                            tmpLabel.setForeground(fgColor);
                        }
                        else {
                            // This is the default color used in DraggableLabel.propertyChange()
                            Color fgColor=DisplayOptions.getColor("PlainText");
                            tmpLabel.setForeground(fgColor);
                        }
                        // If a background color is specified, use it
                        if(entry1.bgColorTx != null && !entry1.bgColorTx.equals("")) {
                            Color bgColor=DisplayOptions.getColor(entry1.bgColorTx);
                            tmpLabel.setBackground(bgColor);
                        }
                        else {
                            // Default
                            Color bgColor = Util.getBgColor();
                            tmpLabel.setBackground(bgColor);
                        }
                        panel.add(tmpLabel);
                    }
                    else {
                        // It must be a menu1
                        // We need the label and a downIcon
                        if(iconForCategory) 
                            btn1String = "    " + entry1.label;
                        else if(setPosition.equals("center"))
                            // space over so that label text is centered
                            btn1String = "   " + entry1.label;     
                        else
                            btn1String =  entry1.label; 
                        
                        tmpLabel = new DraggableLabel(btn1String, fontName);
                        tmpLabel.setName(btn1String);
                        tmpLabel.setFont(labelFont);

                        if(setPosition.equals("left"))
                            tmpLabel.setHorizontalAlignment(JButton.LEFT);
                        else if(setPosition.equals("center")) 
                            tmpLabel.setHorizontalAlignment(JButton.CENTER);
                        else if(setPosition.equals("right"))
                            tmpLabel.setHorizontalAlignment(JButton.RIGHT);
                        

                        // Set the menu icon
                        tmpLabel.setIcon(downIcon);
                        // Text first, then icon follows on the line
                        tmpLabel.setHorizontalTextPosition(JLabel.LEADING);
                        tmpLabel.addMouseListener(getMouselistener());
                        tmpLabel.setSubList(entry1.entryList);

                        // Without Opaque set, the setBackground does not work
                        tmpLabel.setOpaque(true);

                        // If a color was set for the label, use it here
                        // Save color in each item for DraggableLabel.propertyChange()
                        // ** 5/7/09 the entry1.fgColorTx and bg, are not being set
                        // ** right now.  So adding color to the ExpSel.xml file does
                        // ** not get to here yet.  I will leave this code in for
                        // ** future reference.
                        tmpLabel.fgColorTx = entry1.fgColorTx;
                        tmpLabel.bgColorTx = entry1.bgColorTx;
                        if(entry1.fgColorTx != null && !entry1.fgColorTx.equals("")) {
                            Color fgColor=DisplayOptions.getColor(entry1.fgColorTx);
                            tmpLabel.setForeground(fgColor);
                        }
                        else {
                            // This is the default color used in DraggableLabel.propertyChange()
                            Color fgColor=DisplayOptions.getColor("PlainText");
                            tmpLabel.setForeground(fgColor);
                        }
                        // If a background color is specified, use it
                        if(entry1.bgColorTx != null && !entry1.bgColorTx.equals("")) {
                            Color bgColor=DisplayOptions.getColor(entry1.bgColorTx);
                            tmpLabel.setBackground(bgColor);
                        }
                        else {
                            // Default
                            Color bgColor = Util.getBgColor();
                            tmpLabel.setBackground(bgColor);
                        }

                        panel.add(tmpLabel);
                    }
                }
            }
            
            // We have the tabs needed in locTabList.  We should also have
            // tabOrderList filled from the tabOrder entry in ES_op.xml
            // Go through tabOrderList and remove the entries not found
            // in locTabList.  This removes any entries in the xml file that
            // are not used.  Put the result in tabNameList for use by
            // the menus and by the Exp Sel tab Editor.
            for(int i=0; i < tabOrderList.size(); i++) {
                String tab = tabOrderList.get(i);
                if(!locTabList.contains(tab)) {
                    tabOrderList.remove(i);
                    // tabOrderList indexes will be shifted now, just
                    // start the loop back at the beginning.
                    i = -1;
                }
            }
            // We have the tarOrderList pruned so that it does not
            // have tabs not still active.  Now we need to add any
            // tab that tabOrderList is missing.  They will be ordered
            // after the ones specified by ES_op.xml file
            for(int i=0; i < locTabList.size(); i++) {
                String tab = locTabList.get(i);
                if(!tabOrderList.contains(tab)) {
                    tabOrderList.add(tab);
                }
            }
            // Save this new pruned list
            tabNameList = tabOrderList;

            writeMenuFiles();
            
            // Try to restore the selected tab to where it was if it was
            // anyplace.
            try {
                if(lastSelectedTab >= 0)
                    jtp.setSelectedIndex(lastSelectedTab);
            }
            catch (Exception e) {
                // Ignore error. Probably just means that the tab that was being
                // shown, is now gone
            }

            return true;
    }

    static public synchronized void update() {
           forceUpdate = true;
           boolean bUpdate = true;
           if (expSelector != null)
                bUpdate = expSelector.checkAndBuild();
           if (bUpdate)
                ExpSelTree.update();
    }

    // ActionListener called every 10 sec to see if anything needs
    // to be updated. If cmd = force, then update anyway
    public class UpdateCards implements ActionListener {

        public void actionPerformed(ActionEvent ae) {
            String cmd = ae.getActionCommand();
            if (cmd != null && cmd.equals("force")) {
                forceUpdate = true;
            }
            if (bCreatingProtocol) {
                bCreatingProtocol = false;
                setForceUpdate(true);
                return;
            }

            timerCount++;
            if (timerCount > 3) {
                if (times != null) {
                    if (times.isRunning())
                        times.stop();
                }
            }

            boolean bUpdate = checkAndBuild();
            if (bUpdate)
                ExpSelTree.update();
        }
    }

    public static void fillProtocolList() {
        // default to not being called for admin
        fillProtocolList(false, "false");
    }
    
    public static void fillProtocolList(boolean admin) {
        fillProtocolList(admin, "false");
    }
    
    public static void fillProtocolList(String opOnly) {
        fillProtocolList(false,  opOnly);
    }
    
    public static void fillProtocolList(boolean admin, String opOnly) {
        
        String curOperator = Util.getCurrOperatorName();
        String curuser = System.getProperty("user.name");
        LoginService loginService = LoginService.getDefault();
        Hashtable userHash = loginService.getuserHash();
        User user = (User) userHash.get(curuser);
        ArrayList<String> appDirs;
        ArrayList<String> labels;

        
        // Get the SAX parser to call. We pass this our local class that
        // will know what to do with the parsed information
        SAXParser parser = getSAXParser();

        // Set up for parsing the ExperimentSelector_operator.xml file
        // Our local class to deal with parsed info from
        // ExperimentSelector.xml type files
        DefaultHandler expSelSAXHandler = new ExpSelector.ExpSelSaxHandler(false,
                                                        protocolList);

        // Parse the ExpSelOrder_op.xml file if it exists
        String filepath = FileUtil.openPath("USER/PERSISTENCE/ExpSelOrder_"
                + curOperator + ".xml");

        Boolean foundESOrder = false;

        if (filepath != null) {
            File file = new UNFile(filepath);
            // Parse this operators ExpSelOrder_operator.xml file
            // This will add ExpSelEntry items to protocolList
            // Messages.postDebug("loading expSel: "+filepath);
            if (file.length() > 20) {
              try {
                parser.parse(file, expSelSAXHandler);
                foundESOrder = true;
              }
              catch (Exception e) {
                Messages.writeStackTrace(e);
              }
            }
        }
        
        else  {
            // The operator must not have a file.  Try the User's file
            String userName = user.getAccountName();
            filepath = FileUtil.openPath("USER/PERSISTENCE/ExpSelOrder_"
                                         + userName + ".xml");
            if (filepath != null) {
                File file = new UNFile(filepath);
                // Parse this user's ExperimentSelector_user.xml file
                // This will add ExpSelEntry items to protocolList
                // Messages.postDebug("loading expSel: "+filepath);
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                    foundESOrder = true;
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            } 
        }

        // Look for a ES_operator.xml file
        String es_op_filepath = null;
        if (operatorXml != null)
            es_op_filepath = operatorXml;
        else
            es_op_filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                + curOperator + ".xml");

        if (es_op_filepath != null) {
            File file = new UNFile(es_op_filepath);
            // Parse this operators ExperimentSelector_operator.xml file
            // This will add ExpSelEntry items to protocolList
            // Messages.postDebug("loading expSel: "+ es_op_filepath);
            // Messages.postDebug("  file size: "+ file.length());
            if (file.length() > 20) {
              try {
                parser.parse(file, expSelSAXHandler);
              }
              catch (Exception e) {
                Messages.writeStackTrace(e);
              }
            }
        }
        // If opOnly is true, then don't use the users file
        // What happens is that while the macro updateExpSelector is filling
        // the operator's file, it first removes it.  That was causing the
        // code here to open the user's file and that is not what was wanted.
        else if(opOnly.equals("false")) {
            // The operator must not have a file.  Try the User's file
            String userName = user.getAccountName();
            filepath = FileUtil.openPath("USER/INTERFACE/ExperimentSelector_"
                                         + userName + ".xml");
            if (filepath != null) {
                File file = new UNFile(filepath);
                // Parse this user's ExperimentSelector_user.xml file
                // This will add ExpSelEntry items to protocolList
                // Messages.postDebug("loading expSel: "+ filepath);
                // Messages.postDebug("  file size: "+ file.length());
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            } 
        }

        // I don't think we want to use this any longer since we have the Tree mechanism to
        // rearrange items
        // // Parse the tabOrder.xml file if it exists.  This info used to be in
        // // the ES_op.xml file, but because the macro updateExpSelector, removed
        // // that file and recreates it, this info was being lost.
        // String tabOrderPath = FileUtil.openPath("USER/INTERFACE/tabOrder_"
        //                                         + curOperator + ".xml");
        // if (tabOrderPath != null) {
        //     File file = new UNFile(tabOrderPath);
        //     // Parse this operators tabOrder_operator.xml file
        //     // to fill tabOrderList
        //     try {
        //         parser.parse(file, expSelSAXHandler);
        //     }
        //     catch (Exception e) {
        //         Messages.writeStackTrace(e);
        //     }
        // }


        
        // If admin, it means use all possible appdirs, not just the ones
        // this user has set.

        if(admin) {
            // Get all possible appdirs
            appDirs = Util.getAllPossibleAppDirs();
            labels = Util.getAllPossibleAppDirLabels();
        }
        else {
            // Get appdirs for this user
            appDirs = FileUtil.getAppDirs();
            labels = FileUtil.getAppDirLabels();
        }

        
        
        // Now parse the  ExperimentSelector.xml file.
        // If we have already parsed an ESOrder file, then we want
        // to use the ES.xml file to be sure we have the right protocols.
        if (!userOnly && !admin && !foundESOrder) {
            // No ExpSelOrder_ file found, just use the ES.xml file
            filepath = FileUtil
                    .openPath("INTERFACE/ExperimentSelector.xml");
            if (filepath != null) {
                File file = new UNFile(filepath);
                // This will add ExpSelEntry items to protocolList
                // Messages.postDebug("loading expSel: "+ filepath);
                // Messages.postDebug("  file size: "+ file.length());
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            }
            else {
                // There should be an ExperimentSelector.xml in some appdir
                Messages.postWarning("No ExperimentSelector.xml file found");
            }
        }
        else if(foundESOrder) {
            // An ExpSelOrder_ file was found.  Thus we want to use the ES.xml
            // file for only two things:
            // - If a protocol is not in the ES.xml file, remove it from the list
            // - If ES.xml has a protocol which is not already in the list,
            //   add it to the end.

            // First, parse the ES.xml file into a separate list (pList) so we
            // can compare it with protocolList.
            ArrayList<ExpSelEntry> prList = new ArrayList<ExpSelEntry>();

            expSelSAXHandler = new ExpSelTree.ExpSelSaxHandler(false, prList);
            filepath = FileUtil.openPath("INTERFACE/ExperimentSelector.xml");
            if (filepath != null) {
                if((DebugOutput.isSetFor("expselector")))
                    Messages.postDebug("ES parsing " + filepath);
                File file = new UNFile(filepath);
                // Messages.postDebug("loading expSel: "+ filepath);
                // Messages.postDebug("  file size: "+ file.length());
                // This will add ExpSelEntry items to pLrist
                if (file.length() > 20) {
                  try {
                    parser.parse(file, expSelSAXHandler);
                  }
                  catch (Exception e) {
                    Messages.writeStackTrace(e);
                  }
                }
            }
            // Add to the list the protocols in ES_op as protocols to keep
            if(es_op_filepath != null) {
                if((DebugOutput.isSetFor("expselector")))
                    Messages.postDebug("ES parsing " + es_op_filepath);
                File file = new UNFile(es_op_filepath);
                // Messages.postDebug("loading expSel: "+ es_op_filepath);
                // Messages.postDebug("  file size: "+ file.length());
                // This will add ExpSelEntry items to pLrist
                try {
                    parser.parse(file, expSelSAXHandler);
                }
                catch (Exception e) {
                    Messages.writeStackTrace(e);
                }
            }
            
            // Now go through the list and check each protocol to be sure
            // it is in the ES.xml.  If not, remove it from the list
            for (int i = 0; i < protocolList.size(); i++) {
                boolean matchfound = false;
                ExpSelEntry protocol = protocolList.get(i);
                for (int k = 0; k < prList.size(); k++) {
                    ExpSelEntry prot = prList.get(k);
                    if(protocol.name.equals(prot.name)) {
                        matchfound = true;
                        break;
                    }    
                }
                if(!matchfound) {
                    // The protocol in ESOrder was not found in the ES.xml
                    // file so remove it from the list
                    protocolList.remove(i);
                    // fix counter since we just removed item "i" from the list
                    i--;
                }
            }
            // Now, if ES.xml has a protocol not in the EXOrder, then add it
            // to the end.
            for (int i = 0; i < prList.size(); i++) {
                boolean matchfound = false;
                ExpSelEntry protocol = prList.get(i);
                for (int k = 0; k < protocolList.size(); k++) {
                    ExpSelEntry prot = protocolList.get(k);
                    if(protocol.name.equals(prot.name)) {
                        matchfound = true;
                        break;
                    }    
                }
                if(!matchfound) {
                    // This protocol is in ES.xml, but not ESOrder.  First off,
                    // that means that the ES.xml has changed since the ESOrder
                    // was created.  Nonetheless, we want to go ahead and add
                    // this protocol to the list.
                    protocolList.add(protocol);
                }
            }
            
        }
        if (admin) {
            // Look for shared directory ExperimentSelector.xml files
            // parse them, but don't dup and let any previous entry hold.
            // Loop through the appdirs and skip user directories
            String relativePath = FileUtil.getSymbolicPath("INTERFACE");
            // The arg, true, means the parser will ignore the 'tab' entry
            // in the file and instead use the directory name/label.
            expSelSAXHandler = new ExpSelSaxHandler(true, protocolList);

            for (int k = 0; k < appDirs.size(); k++) {
                String adir = appDirs.get(k);
                
                // Skip user dirs
                if (!adir.startsWith(FileUtil.usrdir())) {
                    // We must have what we call a shared dir or a system dir, create the
                    // fullpath for the ExperimentSelector.xml file
                   
                    // Get a list of all ExperimentSelector_*.xml files
                    String dir = adir + File.separator + relativePath;
                    File dirFile = new UNFile(dir);
                    ESFileFilter filter = new ESFileFilter();
                    File files[] = dirFile.listFiles(filter);

                    if(files != null) {
                        // Loop through the file list and parse each of them
                        for(int i=0; i < files.length; i++) {

                            filepath = FileUtil.openPath(files[i].getAbsolutePath());
                            if (DebugOutput.isSetFor("expselector")) {
                                Messages.postDebug("ExpSelector: Parsing " + filepath);
                            }

                            if (filepath != null) {
                                File file = new UNFile(filepath);
                                // This will add ExpSelEntry items to protocolList
                                try {
                                    parser.parse(file, expSelSAXHandler);
                                }
                                catch (Exception e) {
                                    Messages.writeStackTrace(e);
                                }
                            }
                        }
                    }
                }
            }
        }     
        
        // Now we have finished with all of the possible
        // ExperimentSelector.xml
        // files. For backwards compatibility and just to be sure protocols
        // are not forgotten, we will go through all non system protocols
        // and see we have them in protocolList. If not, we need to add them
        String relativePath = FileUtil.getSymbolicPath("PROTOCOLS");
        // Go through appdirs, skipping user directories
        
// Stop it looking for protocols for now
// updateExpSelector macro should be picking up the users protocols
//      for (int k = 0; k < appDirs.size(); k++) {
//            String dir = appDirs.get(k);
// 
//            
//
//            // Skip system dirs and shared dirs if userOnly is true
//            // skip if showNonUserNonSpecifiedAppdirProtoInCatchAll = false
//
//            boolean skip=false;
//
//            // Skip if this is the sysdir or an appdir within the sysdir
//            // That is, possibly allow non sysdir appdirs and userdir
//            if(dir.startsWith(FileUtil.sysdir()))
//                continue;
//
//// This logic is now done in the macro updateExpSelector
////            // If neither NonUser nor User is specified, skip
////            if(!showNonUserNonSpecifiedAppdirProtoInCatchAll && 
////               !showUserNonSpecifiedAppdirProtoInCatchAll) {
////                skip=true;
////            }
////            // If show User but not his dir, skip
////            if(!showNonUserNonSpecifiedAppdirProtoInCatchAll &&
////               (showUserNonSpecifiedAppdirProtoInCatchAll && 
////                !dir.startsWith(FileUtil.usrdir()))) {
////                skip=true;
////            }
////            // If show NonUser but don't show User and it is userdir, skip
////            if(showNonUserNonSpecifiedAppdirProtoInCatchAll &&
////               (!showUserNonSpecifiedAppdirProtoInCatchAll &&
////                dir.startsWith(FileUtil.usrdir()))) {
////                skip=true;
////            }
//            // If userOnly, then only allow userdir
//            if(userOnly && !dir.startsWith(FileUtil.usrdir())) {
//                skip=true;
//            }
//
//            if (!skip) {
//                String pPath = dir + File.separator + relativePath;
//                UNFile file = new UNFile(pPath);
//                if (file != null && file.isDirectory()) {
//
//                    // Get the list of protocols in this directory
//                    File[] files = file.listFiles();
//                    for (int i = 0; i < files.length; i++) {
//                        String name = files[i].getName();
//                        // We need to strip off the .xml from the end of
//                        // the actual filename to get the name we use
//                        // for the protocol
//                        if (name.endsWith(".xml") && files[i].isFile())
//                            name = name.substring(0, name.length() - 4);
//                        else
//                            // If not .xml or not normal file, skip it,
//                            // bogus file
//                            continue;
//
//                        // This call to isNotDup() only looks at the
//                        // protocol name since that is all we have.
//                        if (isNotDup(name) == NOTDUP) {
//                            // Not a dup, we need to add it unless it has
//                            // ExpSelNoShow=true.  Open the .xml and see if
//                            // is has this set.
//                            RandomAccessFile input=null;
//                            String line;
//                            boolean noshow = false;
//                            String paramName = "ExpSelNoShow";
//                            int paramLen = paramName.length();
//                            try {
//                                input = new RandomAccessFile(files[i], "r");
//                                while ((line = input.readLine()) != null) {
//                                    int index = line.indexOf(paramName);
//                                    if(index > -1) {
//                                        String val = line.substring(index + paramLen, index + paramLen + 7);
//                                        if(val.indexOf("true") > -1) {
//                                            noshow = true;
//                                            input.close();
//                                            break;
//                                        }
//                                    }
//                                }
//
//                            }
//                            catch(Exception ex) {
//                                try {
//                                    noshow = true;
//                                    input.close();
//                                }
//                                catch (Exception exc) {
//                                
//                                }
//                            }
//
//                            if(noshow == false) {
//                                ExpSelEntry eSEntry = new ExpSelEntry();
//                                eSEntry.name = new String(name);
//                                // We don't have a label specification, so use
//                                // the protocol name
//                                eSEntry.label = new String(name);
//                                // Get the appdir label for this directory
//                                String shareLabel = labels.get(k);
//
//                                eSEntry.tab = shareLabel;
//                                // Add the new entry
//                                protocolList.add(eSEntry);
//                            }
//                        }
//                    }
//                }
//            }
//        }

    }

    // If called with no arg, simply call the other one with null for args
    // and return whatever it returns
    private static boolean checkProtocolList(ArrayList<ExpSelEntry> pList) {
        return checkProtocolList(null, null, null, null, pList);
    }
    
    // check the protocolList to be sure there are no protocol labels that
    // are the same as a menu name within a given tab.  That is, you can
    // have duplicate protocol/menu names in different tabs.  
    // protocolList must have been filled before this is called.
    private static boolean checkProtocolList(String label, String tab, String menu1, 
                                             String menu2, ArrayList<ExpSelEntry> pList) {
        ArrayList<String> menuNameList = new ArrayList<String>();
        ArrayList<String> tabLList  = new ArrayList<String>();


        // Loop through the protocolList and create a list of all of the
        // Tab names.
        for (int i = 0; i < pList.size(); i++) {
            ExpSelEntry protocol = pList.get(i);
            if(!tabLList.contains(protocol.tab))
                tabLList.add(protocol.tab);
        }
        // If one arrived in the args, add it
        if(tab != null)
            tabLList.add(tab);

        // Loop through the tab name list
        for(int k=0; k < tabLList.size(); k++) {
            String tabName = tabLList.get(k);
            // Clear the menuNameList list and fill it for this tab
            menuNameList.clear();
            
            // Loop through the protocolList and create a list of all of the
            // menu1 and menu2 names within this tab name.
            for (int i = 0; i < pList.size(); i++) {
                ExpSelEntry protocol = pList.get(i);
                if(protocol.tab.equals(tabName)) {
                    if(!menuNameList.contains(protocol.menu1))
                        menuNameList.add(protocol.menu1);
                    if(!menuNameList.contains(protocol.menu2))
                        menuNameList.add(protocol.menu2);
                }
            }
            // If menus arrived in the args for this tab, add them
            if(tabName.equals(tab)) {
                if(menu1 != null &&  !menuNameList.contains(menu1))
                    menuNameList.add(menu1);
                if(menu2 != null &&  !menuNameList.contains(menu2))
                    menuNameList.add(menu2);
            }

            // Now we have a list of all menu names in this tab.  Loop through the
            // protocolList again and see if any protocol names are duplicates
            // of any menu names within this tab.
            for (int i = 0; i < pList.size(); i++) {
                ExpSelEntry protocol = pList.get(i);
                if(protocol.tab.equals(tabName)) {
                    if(menuNameList.contains(protocol.label)) {                            
                        // If the dup is with an input arg, don't remove
                        // an  entry from protocolList, else remove it.
                        if(!protocol.label.equals(menu1) && !protocol.label.equals(menu2)) {
                            pList.remove(i--);
                            Messages.postError("Found protocol name the same as a menu "
                                    + "name (" + protocol.label  + ")");
                        }
                        else {
                            Messages.postError("Found protocol name the same as a menu "
                                    + "name (" + protocol.label + ")");
                            return false;
                        }
                        
                        continue;
                    }
                    // Check the arg label
                    if(label != null && menuNameList.contains(label)) {
                        Messages.postError("Found menu name the same as a protocol "
                                               + "name (" + label + ")");
                        return false;

                    }
                }
            }
        }
        
        // No duplicates found
        return true;
    }
    
    // Write out three files with the menu choices for the panel
    // which saves protocols.  Put them in the users persistence dir
    // although, they are not persistent, they are temporary.
   private void writeMenuFiles() {
        
        // TabList
        String path = FileUtil.savePath("USER/MENULIB" +
                                        File.separator + "tabList");
        FileWriter fw;
        PrintWriter os=null;
        if(path != null) {
            try {
                UNFile file = new UNFile(path);
                fw = new FileWriter(file);
                os = new PrintWriter(fw);
                for(int i=0; i < tabNameList.size(); i++)
                    os.println("\"" + tabNameList.get(i) + "\"" + "    " 
                            + "\"" + tabNameList.get(i) + "\"" );
                os.println("\"Enter Tab Name\"    \"Enter Tab Name\"");
                os.close();
            }
            catch (Exception er) {
                Messages.postError("Problem creating  " + path);
                Messages.writeStackTrace(er);
                if (os != null)
                    os.close();
            }
        }
        // menu1List
        path = FileUtil.savePath("USER/MENULIB" +
                                        File.separator + "menu1List");
        os=null;
        if(path != null) {
            try {
                UNFile file = new UNFile(path);
                fw = new FileWriter(file);
                os = new PrintWriter(fw);
                // Create a menu item for no sub menu
                os.println("None    None");
                for(int i=0; i < menu1NameList.size(); i++)
                    os.println("\"" + menu1NameList.get(i) + "\"" + "    " 
                            + "\"" + menu1NameList.get(i) + "\"" );
                os.close();
                
            }
            catch (Exception er) {
                Messages.postError("Problem creating  " + path);
                Messages.writeStackTrace(er);
                if (os != null)
                    os.close();
            }
        }
        // menu2List
        path = FileUtil.savePath("USER/MENULIB" +
                                        File.separator + "menu2List");
        os=null;
        if(path != null) {
            try {
                UNFile file = new UNFile(path);
                fw = new FileWriter(file);
                os = new PrintWriter(fw);
                // Create a menu item for no sub menu
                os.println("None    None");
                for(int i=0; i < menu2NameList.size(); i++)
                    os.println("\"" + menu2NameList.get(i) + "\"" + "    " 
                            + "\"" + menu2NameList.get(i) + "\"" );
                os.close();
            }
            catch (Exception er) {
                Messages.postError("Problem creating  " + path);
                Messages.writeStackTrace(er);
                if (os != null)
                    os.close();
            }
        }  
    }
    
    
    
    // Call when we only have a protocol name to go on.
    // If there is any entry by this name, then consider it a dup
    public static int isNotDup(String protocolName, ArrayList<ExpSelEntry> protlist) {
        ExpSelEntry eSEntry;
        for (int i = 0; i < protlist.size(); i++) {
            eSEntry = protlist.get(i);
            // Check name
            if (eSEntry.name.equals(protocolName)) {
                // It is a dup
                return COMPLETEDUP;
            }
        }
        return NOTDUP;
    }
    // We want to allow duplicate protocol entries provided they
    // are not in the exact location in the panel/tab/menu system.
    // Check to see if this is really an exact duplicate.
    // It is permissible to have the same protocol in the same
    // location but with two different labels.
    public static int isNotDup(ExpSelEntry newESE, ArrayList<ExpSelEntry> protlist) {
        ExpSelEntry eSEntry;
        for (int i = 0; i < protlist.size(); i++) {
            eSEntry = protlist.get(i);
            // Check label, tab, menu1 and menu2.  We do not want
            // dup location and diff protocol, so don't check protocol
            if (eSEntry.tab.equals(newESE.tab)
                    && eSEntry.label.equals(newESE.label)
                    && ((eSEntry.menu1 != null) && eSEntry.menu1
                            .equals(newESE.menu1))
                    && ((eSEntry.menu2 != null) && eSEntry.menu2
                            .equals(newESE.menu2))) {
                
                // It is a dup, excluding the protocol name, check that
                if(eSEntry.name.equals(newESE.name))
                    return COMPLETEDUP;
                else
                    return DUPALLBUTPROTO;
            }
        }
        // It must not be a dup
        return NOTDUP;
    }
    
    public static int isNotDup(String name, String tab, String label, 
            String menu1, String menu2) {
        // If called with 5 args, default to opOnly false
        return isNotDup(name, tab, label, menu1, menu2, "false");
    }
    
    public static int isNotDup(String name, String tab, String label, 
                                String menu1, String menu2, String opOnly) {
        ExpSelEntry eSEntry;
        
// Does this really need to be completely redone on every call?
// TODO        
        ExpSelector.fillProtocolList(opOnly);
        int psize=protocolList.size();
        for (int i = 0; i < protocolList.size(); i++) {
            eSEntry = protocolList.get(i);
            // Check label, tab, menu1 and menu2.  We do not want
            // dup location and diff protocol, so don't check protocol
            if (eSEntry.tab.equals(tab)
                    && eSEntry.label.equals(label)
                    && ((eSEntry.menu1 != null) && eSEntry.menu1
                            .equals(menu1))
                    && ((eSEntry.menu2 != null) && eSEntry.menu2
                            .equals(menu2))) {
                // It is a dup, excluding the protocol name, check that
                if(eSEntry.name.equals(name)) {
                    // Messages.postDebug("COMPLETEDUP of " + label);
                    return COMPLETEDUP;
                }
                else {
                    return DUPALLBUTPROTO;
                }
            }
        }
        // It must not be a dup
        return NOTDUP;
    }    
    
    static public void makeTrashWhatQuery(String protocol) {
        new TrashWhatQuery(protocol);
    }

    static public class TrashWhatQuery extends ModalDialog implements ActionListener
    {
        public TrashWhatQuery(String protocol) {
            super("What to Trash");


            okButton.addActionListener(this);
            cancelButton.addActionListener(this);
//            helpButton.addActionListener(this);

            // Okay, this is a little wierd, but is faster than making  
            // a new panel.  They want Cancel on the right where as
            // in the ModalDialog, it is in the middle.  So, just change
            // the Text labels and make helpButton be Cancel and 
            // cancelButton be Protocol
            okButton.setText("Remove Protocol");
            cancelButton.setText("Cancel");
//            helpButton.setText("Cancel");
            helpButton.setVisible(false);



            JTextArea msg = new JTextArea("Remove the actual protocol from the Disk?");
            
            getContentPane().add(msg, BorderLayout.NORTH);
            pack();
            
            // Set the popup position just above the trash can
            TrashCan trash = Util.getTrashCan();
            Point p = trash.getLocationOnScreen();
            p.translate(0, -100);
            setLocation(p);

            setVisible(true, true);

        }


        public void actionPerformed(ActionEvent e) {

            String cmd = e.getActionCommand();

            if (cmd.equalsIgnoreCase("Remove Protocol")) {
                trashWhat = "protocol";
            }
            else if (cmd.equalsIgnoreCase("Cancel"))
            {
                trashWhat = "cancel";
            }
            else if (cmd.equalsIgnoreCase("Entry Only"))
            {
                trashWhat = "entry";
            }
            setVisible(false);
        }
    }

    static public ArrayList<String> getTabNameList() {
        return tabNameList;
    }


    static public ArrayList<ExpSelEntry> getProtocolList() {
        // If not filled yet, fill it
        if(protocolList != null && protocolList.size() == 0) {
            fillProtocolList(isAdmin());
        }

        return protocolList;
    }

    static public boolean isAdmin() {
        boolean admin;
        AppIF appIf = Util.getAppIF();
        if (appIf instanceof VAdminIF)
            admin = true;
        else
            admin = false;
        return admin;
    }
    
    static public SAXParser getSAXParser() {
        SAXParser parser = null;
        try {
            SAXParserFactory spf = SAXParserFactory.newInstance();
            spf.setValidating(false); // set to true if we get DOCTYPE
            spf.setNamespaceAware(false); // set to true with referencing
            parser = spf.newSAXParser();
        }
        catch (ParserConfigurationException pce) {
            System.out.println(new StringBuffer().append(
                    "The underlying parser does not support ").append(
                    "the requested feature(s).").toString());
        }
        catch (FactoryConfigurationError fce) {
            System.out.println("Error occurred obtaining SAX Parser Factory.");
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
        return (parser);
    }

    // Deal with parsed output from the ExperimentSelector.xml files
    // Fill a 'eSEntry' for each protocol and put the eSEntry into protocolList.
    // Also fill userOnly, sharedFlat and userFlat mode variables
    static public class ExpSelSaxHandler extends DefaultHandler {

        ExpSelEntry eSEntry;
        ArrayList<ExpSelEntry> protList;

        // If sharedType = true, then use the directory name/label in place
        // of the tab specified
        boolean sharedType = false;

        public ExpSelSaxHandler(boolean sharedType, ArrayList<ExpSelEntry> protlist) {
            this.sharedType = sharedType;
            // List where to put the results of the parsing
            protList = protlist;
        }

        public void endDocument() {
            // System.out.println("End of Document");
        }

        public void endElement(String uri, String localName, String qName) {
            // System.out.println("End of Element '"+qName+"'");
        }

        public void startDocument() {
            // System.out.println("Start of Document");
        }

        public void startElement(String uri, String localName, String qName,
                Attributes attr) {
            if (qName.compareTo("ProtocolList") == 0) {
                // These were defaulted to false, so if they are not set
                // then they will remain defaulted. The system file should
                // not have these, so they will be whatever was left from
                // the default or the users file
                String mode = attr.getValue("userOnly");
                if (mode != null && mode.equals("true"))
                    userOnly = true;
                else if (mode != null && mode.equals("false"))
                    userOnly = false;
                mode = attr.getValue("sharedFlat");
                if (mode != null && mode.equals("false"))
                    sharedFlat = false;
                else if (mode != null && mode.equals("true"))
                    sharedFlat = true;
                mode = attr.getValue("userFlat");
                if (mode != null && mode.equals("false"))
                    userFlat = false;
                else if (mode != null && mode.equals("true"))
                    userFlat = true;
                mode = attr.getValue("allWithSystem");
                if (mode != null && mode.equals("false"))
                    allWithSystem = false;
                else if (mode != null && mode.equals("true"))
                    allWithSystem = true;
                mode = attr.getValue("tooltip");
                if (mode != null && (mode.equals("false") || mode.equals("none")))
                    tooltipMode = "false";
                else if (mode != null && mode.equals("label"))
                     tooltipMode = "label";
                else if (mode != null && mode.equals("fullpath"))
                     tooltipMode = "fullpath";
                mode = attr.getValue("labelWithCategory");
                if (mode != null && mode.equals("false"))
                    labelWithCategory = false;
                else if (mode != null && mode.equals("true"))
                    labelWithCategory = true;
                mode = attr.getValue("iconForCategory");
                if (mode != null && mode.equals("false")) {
                    iconForCategory = false;
                    setPosition = "center";
                }
                else if (mode != null && mode.equals("true")) {
                    iconForCategory = true;
                    setPosition = "left";
                }

            }
            // Add a line in the ES_op.xml file to define the order of the
            // tabs in the panel.  The tabOrder name is expected to be a
            // comma separated list like
            // <tabOrder list="New Tab, Sel2D, Dosy 2D, (HH)Homo2D " />
            else if (qName.compareTo("tabOrder") == 0) {
                String list = attr.getValue("list");
                StringTokenizer tok;
                tok = new StringTokenizer(list, ",");
                int count = tok.countTokens();
                for(int i=0; i < count; i++) {
                    String tabname = tok.nextToken().trim();
                    ExpSelEntry tab;
                    if (!tabList.containsKey(tabname)) {
                        // This tab does not exist yet, make it
                        tab = new ExpSelEntry();
                        tab.label = new String(tabname);
                        tab.entryList = new HashArrayList();
                        // Add to tabList
                        tabList.put(tabname, tab);
                                
                        // Save a list of tab labels for the panel menu
                        if(!tabOrderList.contains(tab.label))
                            tabOrderList.add(tab.label);
                    }
                }
            }
            else if (qName.compareTo("protocol") == 0) {
                eSEntry = new ExpSelEntry();
                eSEntry.name = Util.getLabel(attr.getValue("name"));
                eSEntry.label = Util.getLabel(attr.getValue("label"));
                // If no label it given, use the protocol name
                if(eSEntry.label.length() == 0)
                    eSEntry.label = new String(eSEntry.name);

                eSEntry.fgColorTx = attr.getValue("fgcolor");
                eSEntry.bgColorTx = attr.getValue("bgcolor");
                eSEntry.noShow = attr.getValue("ExpSelNoShow");
                if(eSEntry.noShow == null)
                    eSEntry.noShow = "false";

                // Should the actual protocol file be chosen by keywords
                // (parameters)?
                eSEntry.bykeywords = attr.getValue("bykeywords");
                
                // Try all appdirs until this protocol is found, then
                // continue.  It used to just check the appdir path where
                // the ExperimentSelector.xml file was located, but that
                // ended up missing some protocols (Like ATP)
                ArrayList<String> appDirs;
                if(isAdmin()) {
                    // Get all possible appdirs
                    appDirs = Util.getAllPossibleAppDirs();
                }
                else {
                    // Get appdirs for this user
                    appDirs = FileUtil.getAppDirs();
                }
                String ppath;
                String fullpath=null;
                for (int k = 0; k < appDirs.size(); k++) {
                    String dir = appDirs.get(k);
                    String relativePath = FileUtil.getSymbolicPath("PROTOCOLS");
                    ppath = dir + File.separator + relativePath + File.separator 
                    + eSEntry.name + ".xml";
                    fullpath = FileUtil.openPath(ppath);
                    // If file is found, continue below with this path
                    if (fullpath != null)
                        break;
                    // If null and we are out of appDirs to look in,
                    // abort the addition of this protocol
                    else if(k == appDirs.size() -1)
                        return;
                }
                if(fullpath == null)
                    return;
                
                // Assume that system files start with FileUtil.sysdir()
                // And User files start with FileUtil.usrdir()
                // 'Shared' Dir
                if (!fullpath.startsWith(FileUtil.sysdir())
                        && !fullpath.startsWith(FileUtil.usrdir())) {
                    if(allWithSystem) {
                        // Put Shared protocols with system, but add
                        // '(Sh)' to the label if requested
                        if(labelWithCategory)
                            eSEntry.label = eSEntry.label + "(Sh)";
                            
                        eSEntry.tab = attr.getValue("tab");
                        eSEntry.menu1 = attr.getValue("menu1");
                        eSEntry.menu2 = attr.getValue("menu2");
                        // Allow level1 and 2 to be the same as menu1,2
                        // and level0 same as tab
                        String level0 = attr.getValue("level0");
                        String level1 = attr.getValue("level1");
                        String level2 = attr.getValue("level2");
                        if(level0 != null)
                            eSEntry.tab = level0;
                        if(level1 != null)
                            eSEntry.menu1 = level1;
                        if(level2 != null)
                            eSEntry.menu2 = level2;
                        
                        if(eSEntry.tab == null)
                            eSEntry.tab = "Shared";
                    }

                    // Not system and not user so must be 'shared'. They don't
                    // want shared mixed with system, so put into a tab called
                    // the directory's name/label
                    else if (sharedFlat) {
                        // Flat, so put all of them into a single tab
                        eSEntry.tab = "Shared";
                    }
                    else {
                        // Get the appdir label list
                        ArrayList<String> labels = FileUtil.getAppDirLabels();
                        // Get the appdir list
                        ArrayList<String> appdirs = FileUtil.getAppDirs();
                        // Determine which appdir this protocol was found in
                        // by going through the list until it matches
                        int k;
                        for (k = 0; k < appdirs.size(); k++) {
                            if (fullpath.startsWith(appdirs.get(k)))
                                break;
                        }
                        String shareLabel;
                        if (labels.size() > k)
                            shareLabel = labels.get(k);
                        else
                            shareLabel = "Shared";

                        eSEntry.tab = shareLabel;

                        // Not flat, so use the menus specified
                        eSEntry.menu1 = attr.getValue("menu1");
                        eSEntry.menu2 = attr.getValue("menu2");
                        // Allow level1 and 2 to be the same as menu1,2
                        // and level0 same as tab
                        String level1 = attr.getValue("level1");
                        String level2 = attr.getValue("level2");
                        if(level1 != null)
                            eSEntry.menu1 = level1;
                        if(level2 != null)
                            eSEntry.menu2 = level2;
                    }

                }
                // Assume that User files start with FileUtil.usrdir()
                // 'User' Dir
                else if (fullpath.startsWith(FileUtil.usrdir())) {
                    if(allWithSystem) {
                        // Put User protocols with system, but add
                        // '(U)' to the label if requested
                        if(labelWithCategory)
                            eSEntry.label = eSEntry.label + "(U)";
                        eSEntry.tab = attr.getValue("tab");
                        if(eSEntry.tab == null)
                            eSEntry.tab = "User";
                        eSEntry.menu1 = attr.getValue("menu1");
                        eSEntry.menu2 = attr.getValue("menu2");

                        // Allow level1 and 2 to be the same as menu1,2
                        // and level0 same as tab
                        String level0 = attr.getValue("level0");
                        String level1 = attr.getValue("level1");
                        String level2 = attr.getValue("level2");
                        if(level0 != null)
                            eSEntry.tab = level0;
                        if(level1 != null)
                            eSEntry.menu1 = level1;
                        if(level2 != null)
                            eSEntry.menu2 = level2;                    }
                    else {
                        // so put into the appropriate tab, but then put into
                        // a menu called User
                        eSEntry.tab = attr.getValue("tab");
                        eSEntry.menu1 = "User";
                        // If not flat, then go ahead and take the user specified
                        // menu2
                        if (!userFlat)
                            eSEntry.menu2 = attr.getValue("menu2");
                    }
                }
                // Else, continue below to add normally
                else {
                    eSEntry.tab = attr.getValue("tab");
                    eSEntry.menu1 = attr.getValue("menu1");
                    eSEntry.menu2 = attr.getValue("menu2");
                    // Allow level1 and 2 to be the same as menu1,2
                    // and level0 same as tab
                    String level0 = attr.getValue("level0");
                    String level1 = attr.getValue("level1");
                    String level2 = attr.getValue("level2");
                    if(level0 != null)
                        eSEntry.tab = level0;
                    if(level1 != null)
                        eSEntry.menu1 = level1;
                    if(level2 != null)
                        eSEntry.menu2 = level2;
                }
                // Disallow  duplicate entries where they have the same
                // label, tab, menu1 and menu2
                int status = isNotDup(eSEntry, protList);
                if (status == NOTDUP)
                    protList.add(eSEntry);
            }
        }
    }

    public void warning(SAXParseException spe) {
        System.out.println("Warning: SAX Parse Exception:");
        System.out.println(new StringBuffer().append("    in ").append(
                spe.getSystemId()).append(" line ").append(spe.getLineNumber())
                .append(" column ").append(spe.getColumnNumber()).toString());
    }

    public void error(SAXParseException spe) {
        System.out.println("Error: SAX Parse Exception:");
        System.out.println(new StringBuffer().append("    in ").append(
                spe.getSystemId()).append(" line ").append(spe.getLineNumber())
                .append(" column ").append(spe.getColumnNumber()).toString());
    }

    public void fatalError(SAXParseException spe) {
        System.out.println("FatalError: SAX Parse Exception:");
        System.out.println(new StringBuffer().append("    in ").append(
                spe.getSystemId()).append(" line ").append(spe.getLineNumber())
                .append(" column ").append(spe.getColumnNumber()).toString());
    }

    /**
     * listens to menu selections
     */
    class PopActionListener implements ActionListener {
        /** button */
        private DraggableLabel label;

        /**
         * constructor
         * 
         * @param popButton
         *            PopButton
         */
        PopActionListener(DraggableLabel label) {
            this.label = label;
        } // PopActionListener()

        /**
         * action performed
         * 
         * @param evt
         *            event
         */
        public void actionPerformed(ActionEvent evt) {
            Util.setMenuUp(false);
            String cmd = evt.getActionCommand();
            // We should have a protocol here and the cmd is the fullpath of the
            // protocol along with a comma separated list of info.  We just
            // want the fullpath up to the first comma
            int index = cmd.indexOf(",");
            String fullpath = cmd.substring(0, index);

            ShufflerItem shufflerItem;
            // If the Ctrl key was held down, go for the system
            // protocol if there is one
            int modifierMask = evt.getModifiers();
            if((modifierMask & ActionEvent.CTRL_MASK) != 0) {
                // We need to get the system protocol instead of the
                // one given in the fullpath in cmd.  Get the protocol name
                // and then get the path we want
                index = fullpath.lastIndexOf(File.separator);
                String name = fullpath.substring(index +1);
                String systemProtPath = getSystemProtPath(name);
                if(systemProtPath != null)
                    shufflerItem = new ShufflerItem(systemProtPath,
                                                "EXPSELECTOR");
                else
                    // If null was returned, a system protocol was
                    // not found, just use the default one
                    shufflerItem = new ShufflerItem(fullpath, "EXPSELECTOR");
            }
            else
                shufflerItem = new ShufflerItem(fullpath, "EXPSELECTOR");
            
            QueuePanel queuePanel = Util.getStudyQueue();
            if (queuePanel != null) {
                StudyQueue studyQueue = queuePanel.getStudyQueue();
                if (studyQueue != null) {
                    ProtocolBuilder protocolBuilder = studyQueue.getMgr();
                    // Add this item to the studyQueue
                    protocolBuilder.appendProtocol(shufflerItem);
                }
            }
            // No StudyQ, load it.
            else {
                shufflerItem.actOnThisItem("Canvas", "DragNDrop", "");
            }

        } // actionPerformed()
    } // class PopActionListener



    // Allow us to get a list of all ExperimentSelector_*.xml files
    static public class ESFileFilter implements FilenameFilter
    {
        // Allow all ExperimentSelector_*.xml files
        public boolean accept(File dir, String name) {
            if(name.endsWith(".xml")  &&  name.indexOf("ExperimentSelector") != -1)
                return true;
            else
                return false;
        }
    }


}

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
import java.util.*;
import java.beans.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;
import javax.swing.table.*;
import javax.swing.border.*;

import vnmr.util.*;
import vnmr.ui.*;
import vnmr.admin.ui.*;

import static vnmr.bo.EasyJTable.*;


public class VMessageBox extends JPanel
    implements VObjIF, VObjDef, DropTargetListener,ActionListener,
    PropertyChangeListener,StatusListenerIF,MessageListenerIF
{
    private String type = null;
    private String label = null;
    private String dispOptions = null;
    private boolean inAddMode = false;
    private boolean isEditing = false;
    private boolean inEditMode = false;
    private boolean inModalMode = false;
    private boolean isFocused = false;
    private boolean bNewAcqMessage = false;
    private boolean bNewInfoMessage = false;
    private boolean bNewStatusMessage = false;
    private MouseAdapter ml;
    private ButtonIF vnmrIf;
    private boolean first_show=true;
    private VObjIF vobj;

    private MessageButton acq_button=null;
    private MessageButton info_button=null;
    private MessageButton stat_button=null;
    // When setText to JLabel will generate UI layout event
    // private VLabel showline=null;
    private VTextMsg showline=null;
    private MessageWindow m_window = null;
    protected EasyJTable m_table;
    protected ArrayList<VMessage> m_vector=null;
    private ArrayList<VMessage> acqList;
    private ArrayList<VMessage> infoList;
    private ArrayList<VMessage> statusList;
    private ArrayList<VMessage> lastList = null;
    protected int m_nAcq = 0;
    protected int m_nInfo = 0;
    protected int m_nStatus = 0;
    private Color onColor=Color.green;
    private Color offColor=null;
    private Dimension psize;

    private static int STATE_EMPTY=-1;
    private static int STATE_ON=0;
    private static int STATE_OFF=1;
    private static int STATE_INTERACTIVE=2;
    private static int STATE_READY=3;
    private static int STATE_INFO=4;
    private static int STATE_WARN=5;
    private static int STATE_ERR=6;

    private static String[] names = {"On","Off","Interactive","Ready","Info","Warning","Error"};
    private String stateStr=null;
    public  String objName = null;
    private int state=STATE_EMPTY;

    private boolean showCount=true;
    private boolean showDate=false;
    private boolean invertColors=true;
    private boolean showStatus=false;
    private boolean rightButtons=false;

    private String show_count_str;
    private String show_date_str;
    private String invert_colors_str;
    private String clr_history_str;

    public static String persistence_file = "MessageBox";
    private static String WINHT = "windowHeight";
    private static String WINWIDTH = "windowWidth";
    private static String WINLOCX = "windowLocX";
    private static String WINLOCY = "windowLocY";
    private int m_nWinHt = 100;
    private int m_nWinWidth = 400;
    private int m_nMinWinHt = 50;
    private int m_nMinWinWidth = 200;
    private int m_nWinLocX = 0;
    private int m_nWinLocY = 0;

    private Dimension defDim = new Dimension(0,0);
    private Dimension curDim = new Dimension(0,0);
    private Point defLoc = new Point(0, 0);
    private Point curLoc = new Point(0, 0);
    private Point tmpLoc = new Point(0, 0);

    public VMessageBox(){
        this((SessionShare)null,(ButtonIF)null,"messagebox");
    }

    public VMessageBox(SessionShare sshare, ButtonIF vif, String typ) {
        this.type = typ;
        this.vnmrIf = vif;
        vobj = this;
        show_count_str = Util.getLabel("_VMessageBox_ShowCount", "Show Count");
        show_date_str = Util.getLabel("_VMessageBox_ShowDate", "Show Date");
        invert_colors_str = Util.getLabel("_VMessageBox_InvertColors", "Invert Colors");
        clr_history_str = Util.getLabel("_VMessageBox_ClearHistory", "Clear History");
        ml = new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                if (m_window == null) {
                    m_window = new MessageWindow(m_table);
                }
                int clicks = evt.getClickCount();
                int modifier = evt.getModifiers();
                if ((modifier & InputEvent.BUTTON3_MASK) != 0) {
                    m_window.menuAction(evt);
                } else if ((modifier & (1 << 4)) != 0) {
                    if (clicks >= 2)
                        ParamEditUtil.setEditObj(vobj);
                }
            }

            public void mouseReleased(MouseEvent evt) {
                if(inModalMode || inAddMode || inEditMode) {
                    return;
                }
                if (m_window == null) {
                    m_window = new MessageWindow(m_table);
                }
                Object obj = evt.getSource();
                if (obj instanceof MessageButton) {
                    int modifiers = evt.getModifiers();
                    int onMask = MouseEvent.BUTTON1_MASK;
                    if ((modifiers & onMask) == onMask) {
                        MessageButton button = (MessageButton)obj;
                        int x = evt.getX();
                        int y = evt.getY();
                        if (x < 0 || x >= button.getWidth()
                            || y < 0 || y >= button.getHeight())
                        {
                            // Released mouse outside of button
                            restoreButtonState(button);
                        }
                    }
                }
            }
        };
        new DropTarget(this, this);
        setLayout(new MessageLayout());

        acqList=new ArrayList<VMessage>();
        infoList=new ArrayList<VMessage>();
        statusList=new ArrayList<VMessage>();

        m_table = new EasyJTable(new DefaultTableModel(0, 1));

        // showline=new VLabel(sshare,vif,"label");
        showline=new VTextMsg(sshare,vif,"label");
        showline.setBorder(BorderFactory.createLoweredBevelBorder());
        showline.setOpaque(true);
        showline.setToolTipLocation(new Point(0, 0));
        add(showline);
        showline.addMouseListener(ml);

        acq_button=new MessageButton();
        acq_button.addActionListener(this);
        acq_button.setToolTipText(Util.getLabel("_VMessageBox_Acquisition_messages"));
        add(acq_button);
        acq_button.addMouseListener(ml);

        info_button=new MessageButton();
        info_button.addActionListener(this);
        // info_button.setToolTipText(Util.getLabel("_VMessageBox_Spectrometer_messages"));
        info_button.setToolTipText(Util.getLabel("_VMessageBox_Vnmr_messages", "Vnmr message"));
        add(info_button);
        info_button.addMouseListener(ml);

        stat_button=new MessageButton();
        stat_button.addActionListener(this);
        stat_button.setToolTipText(Util.getLabel("_VMessageBox_Status_messages"));
        add(stat_button);
        stat_button.addMouseListener(ml);

        setPreferredSize(new Dimension(150,22));
        DisplayOptions.addChangeListener(this);
        Messages.addMessageListener(this);

        onColor=Color.black;
        setBg(Util.getBgColor());
        readPersistence();
    }

    /** read persistence file ~/vnmrsys/persistence/MessageBox */
    private void readPersistence() {
        BufferedReader in;
        String line;
        String symbol;
        String value;
        StringTokenizer tok;
        String filepath = FileUtil.openPath("USER/PERSISTENCE/"+persistence_file);
        if(filepath==null)
            return;
        try {
            in = new BufferedReader(new FileReader(filepath));
            while ((line = in.readLine()) != null) {
                tok = new StringTokenizer(line, " ");
                if (!tok.hasMoreTokens())
                    continue;
                symbol = tok.nextToken().trim();
                if (tok.hasMoreTokens()){
                    value = tok.nextToken().trim();
                    symbol=symbol.replace('_',' ');
                    boolean state=value.equals("true")?true:false;
                    if(symbol.equals(show_count_str)) {
                        showCount=state;
                        VMessage.setShowCount(state);
                    } else if(symbol.equals(show_date_str)) {
                        showDate=state;
                        VMessage.setShowDate(state);
                    } else if(symbol.equals(invert_colors_str)) {
                        invertColors=state;
                    } else if(symbol.equals(WINHT)) {
                        int h=Integer.parseInt(value);
                        if(h>m_nMinWinHt)
                            m_nWinHt = h;
                    } else if(symbol.equals(WINWIDTH)) {
                        int w=Integer.parseInt(value);
                        if(w>m_nMinWinWidth)
                            m_nWinWidth = w;
                    } else if(symbol.equals(WINLOCX)) {
                        m_nWinLocX = Integer.parseInt(value);
                    } else if(symbol.equals(WINLOCY)) {
                        m_nWinLocY = Integer.parseInt(value);
                    }
                }
            }
            in.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }

    /** write persistence file ~/vnmrsys/persistence/MessageBox */
    private void writePersistence() {
        String filepath = FileUtil.savePath("USER/PERSISTENCE/"+persistence_file, false);
        if (filepath == null)
           return;

        PrintWriter os;
        try {
            os = new PrintWriter(new FileWriter(filepath));
            String value;
            value=showCount?"true":"false";
            os.println(show_count_str.replace(' ','_')+" "+value);

            value=showDate?"true":"false";
            os.println(show_date_str.replace(' ','_')+" "+value);

            value=invertColors?"true":"false";
            os.println(invert_colors_str.replace(' ','_')+" "+value);

            // Window Height
            int h = m_window.getHeight();
            if (h >= m_nMinWinHt) {
                m_nWinHt = h;
            }
            os.println(WINHT + " " + m_nWinHt);

            // WindowWidth
            int w = m_window.getWidth();
            if (w >= m_nMinWinWidth) {
                m_nWinWidth = w;
            }
            os.println(WINWIDTH + " " + m_nWinWidth);

            // Window Location
            Point ptLoc = m_window.getLocation();
            os.println(WINLOCX + " " + ptLoc.x);
            os.println(WINLOCY + " " + ptLoc.y);
            os.close();

        }
        catch(IOException er) {
            System.out.println("Error: could not create "+filepath);
        }
    }

     // MessageListenerIF

    /** Message Update from Line3. */
    public synchronized void newMessage(int code, String msg){

        if((code & Messages.MBOX)==0)
            return;
        StringTokenizer tok = new StringTokenizer(msg);
        String key=tok.nextToken();
        int priority=STATE_INFO;

        if(key.equals("echo:"))
            return;

        int group=code & Messages.GROUP;
        int type =code & Messages.TYPE;

        if(key.equals("Warning:"))
            type=Messages.WARN;

        if(key.equals("Error:"))
            type=Messages.ERROR;

        state=STATE_INFO;
        if(type==Messages.WARN)
            state=STATE_WARN;
        else if(type==Messages.ERROR)
            state=STATE_ERR;

        setState(state);

        if(group==Messages.ACQ)
            priority=acq_button.getState();
        else if (group==Messages.OTHER)
            priority=info_button.getState();

        if(state>priority)
            priority=state;
        if(group==Messages.ACQ){
            acq_button.setMessageFlag(priority);
            VMessage m=new VMessage(msg,state,m_nAcq+1);
            listSize(acqList);
            m_nAcq = m_nAcq+1;
            if(getList()==acqList)
                addMessage(m);
            bNewAcqMessage = true;
            acqList.add(m);
        }
        else if (group==Messages.OTHER) {
            info_button.setMessageFlag(priority);
            VMessage m=new VMessage(msg,state,m_nInfo+1);
            listSize(infoList);
            m_nInfo = m_nInfo+1;
            if(getList()==infoList)
                addMessage(m);
            bNewInfoMessage = true;
            infoList.add(m);
        }
        showMessage(msg);
    }

    //StatusListenerIF

    /** Message Update from Infostat. */
    public void updateStatus(String msg) {
        if(!showStatus)
            return;
        StringTokenizer tok = new StringTokenizer(msg);
        if (tok.hasMoreTokens()) {
            String parm = tok.nextToken();
            if (parm.equals("status")) {
                parm=tok.nextToken();
                if(stateStr==null || !parm.equals(stateStr)){
                    stateStr=parm;
                    if(parm.equals("Inactive"))
                        state=STATE_OFF;
                    else if(parm.equals("SpinReg"))
                        state=STATE_ON;
                    else if(parm.equals("VTReg"))
                        state=STATE_ON;
                    else if(parm.equals("Interactive"))
                        state=STATE_INTERACTIVE;
                    else if(parm.equals("Acquiring"))
                        state=STATE_ON;
                    else
                        state=STATE_READY;
                    setState(state);
                    addStatusMessage(parm,state);
                }
            }
        }
    }

    protected void listSize(ArrayList aList) {
        int nLength = Util.getStatusMessagesNumber();
        int nLength2 = aList.size();
        if (nLength2 > nLength)
            aList.remove(0);
    }

    // private methods

    private void setState(int choice) {
        if (choice == STATE_EMPTY) {
            showline.setBackground(offColor);
            repaint();
            return;
        }
        String var = names[choice];
        // Color c=DisplayOptions.getColor(var);
        Color c = getColor(var);
        Color bg = getColor(var + "Bkg");

        if (invertColors) {
            showline.setBackground(c);
            if (bg == null) 
                bg = Util.getContrastingColor(c);
            showline.setForeground(bg);
        } else {
            if (c == null)
                c = Util.getContrastingColor(bg);

            showline.setForeground(c);
            showline.setBackground(bg);
        }
        Font f = DisplayOptions.getFont(var, var, var);
        showline.setFont(f);
        repaint();
    }

    // PropertyChangeListener interface

    public void propertyChange(PropertyChangeEvent evt) {
        if(DisplayOptions.isUpdateUIEvent(evt)){
            SwingUtilities.updateComponentTreeUI(this);
            if(m_window !=null){
                SwingUtilities.updateComponentTreeUI(m_window);
                SwingUtilities.updateComponentTreeUI(m_window.menu);
            }
        }
        setBg(Util.getBgColor());
        setState(state);
    }

    public void setPreferredSize(Dimension d) {
        super.setPreferredSize(d);
        psize = d;
        if (psize.width < 30)
            psize.width = 30;
        defDim.width = d.width;
        defDim.height = d.height;
        setSize(psize);
        acq_button.setSize(20, psize.height - 2);
    }

    public Dimension getPreferredSize() {
        return psize;
    }

    // ActionListener interface

    public void actionPerformed(ActionEvent  e) {
        if(inModalMode || inAddMode || inEditMode)
            return;
        int modifiers = e.getModifiers();
        if ((modifiers & ActionEvent.META_MASK) == ActionEvent.META_MASK) {
            // This is used to just bring up the menu, so we may have to
            // set the button back to its proper look.
            Object button = e.getSource();
            if (button instanceof MessageButton) {
                restoreButtonState((MessageButton)button);
            }
            return;
        }
        MessageButton button=(MessageButton)e.getSource();
        if (m_window == null) {
            m_window = new MessageWindow(m_table);
        }
        if(button==acq_button){
            m_window.setTitle(Util.getLabel("_VMessageBox_Acquisition",
                                            "Acquisition"));
            showHideWindow(acqList,button);
        }
        else if(button==info_button){
            m_window.setTitle(Util.getLabel("_VMessageBox_Information",
                                            "Information"));
            showHideWindow(infoList,button);
        }
        else if(button==stat_button && showStatus){
            m_window.setTitle(Util.getLabel("_VMessageBox_HWStatus",
                                            "HW Status"));
            showHideWindow(statusList,button);
        }
    }

    private void restoreButtonState(MessageButton button) {
        if (button == acq_button) {
            boolean select = (getList() == acqList && m_window.isVisible());
            acq_button.getModel().setSelected(select);
            acq_button.getModel().setArmed(select);
            acq_button.getModel().setPressed(select);
        } else if (button == info_button) {
            boolean select = (getList() == infoList && m_window.isVisible());
            info_button.getModel().setSelected(select);
            info_button.getModel().setArmed(select);
            info_button.getModel().setPressed(select);
        } else if (button == stat_button) {
            boolean select = (getList() == statusList && m_window.isVisible());
            stat_button.getModel().setSelected(select);
            stat_button.getModel().setArmed(select);
            stat_button.getModel().setPressed(select);
        }
    }

    /** show/hide message window. */
    private void showHideWindow(ArrayList<VMessage> v, MessageButton button) {
        if (inEditMode)
            return;
        if (m_window == null) {
            m_window = new MessageWindow(m_table);
        }
        if (first_show) {
            Point pt = getLocationOnScreen();
            Point pc;
            if (rightButtons)
                pc = new Point((int) pt.getX() + getWidth(), (int) pt.getY());
            else
                pc = new Point((int) pt.getX() + 40, (int) pt.getY() + 30);

            if (m_nWinLocX > 0)
                pc.x = m_nWinLocX;
            if (m_nWinLocY > 0)
                pc.y = m_nWinLocY;
            else
                pc.y -= m_nWinHt;
            m_window.setLocation(pc);
            m_window.setSize(m_nWinWidth, m_nWinHt);
            m_window.validate();
            m_window.setVisible(true);
            first_show = false;
        }
        if (getList() != v) {
            setList(v);
            if (!m_window.isVisible())
                m_window.setVisible(true);
            m_window.toFront(); // Assume user wants to see it
        } else {
            if (m_window.isVisible()) {
                doCloseAction();               
                m_window.setVisible(false);
            } else
                m_window.setVisible(true);
        }
        lastList = v;
        state = STATE_EMPTY;
        showMessage("");
        // NB: setPressed() controls the border, setSelected() the bg color
        acq_button.getModel().setPressed(false);
        acq_button.getModel().setSelected(false);
        acq_button.getModel().setArmed(false);
        info_button.getModel().setPressed(false);
        info_button.getModel().setSelected(false);
        info_button.getModel().setArmed(false);
        stat_button.getModel().setPressed(false);
        stat_button.getModel().setSelected(false);
        stat_button.getModel().setArmed(false);
        button.getModel().setPressed(m_window.isVisible());
        button.getModel().setSelected(m_window.isVisible());
        refresh();
    }

    protected void doCloseAction() {
        clearStatus();
        checkWindowGeometry();
    }

    protected void checkWindowGeometry() {
        Point ptLoc = new Point(m_nWinLocX, m_nWinLocY);
        int h = m_window.getHeight();
        int w = m_window.getWidth();
        Point p = m_window.getLocation();
        if (h != m_nWinHt || w != m_nWinWidth || p != ptLoc)
            writePersistence();
    }

    protected void addStatusMessage(String m,int s){
        listSize(statusList);
        m_nStatus = m_nStatus+1;
        statusList.add(new VMessage(m,s,m_nStatus));
        stat_button.setMessageFlag(state);
        if(getList()==statusList)
            setList(statusList);
        bNewStatusMessage = true;
        showMessage(m);
    }

    /** show latest message in message box. */
    protected void showMessage(String msg){
        StringTokenizer tok= new StringTokenizer(msg,"\n");
        if(tok.hasMoreTokens())
                msg=tok.nextToken();
        // showline.setAttribute(LABEL,msg);
        showline.setText(msg);
        showline.setToolTipText(msg);
    }

    /** clear all elements in a message vector. */
    protected void clearVector() {
        if (lastList == null) {
            state = STATE_EMPTY;
            infoList.clear();
            acqList.clear();
            statusList.clear();
            showMessage("");
            infoList.trimToSize();
            acqList.trimToSize();
            statusList.trimToSize();
            acq_button.clrMessageFlag();
            info_button.clrMessageFlag();
            stat_button.clrMessageFlag();
        } else {
            lastList.clear();
            lastList.trimToSize();
            if (getList() == lastList) {
                setList(lastList);
                state = STATE_EMPTY;
                showMessage("");
                if (getList() == acqList)
                    acq_button.clrMessageFlag();
                else if (getList() == infoList)
                    info_button.clrMessageFlag();
                else if (getList() == statusList)
                    stat_button.clrMessageFlag();
            }
        }
        refresh();
    }

    /** clear all elements in a message vector. */
    protected void clearStatus() {
        if (lastList == null) {
            acq_button.clrMessageFlag();
            info_button.clrMessageFlag();
            stat_button.clrMessageFlag();
        } else {
            if (getList() == lastList) {
                if (getList() == acqList)
                    acq_button.clrMessageFlag();
                else if (getList() == infoList)
                    info_button.clrMessageFlag();
                else if (getList() == statusList)
                    stat_button.clrMessageFlag();
            }
        }
        refresh();
    }

    // VObjIF interface

    protected Color getColor(String strColor)
    {
        Color c = null;
        if (Util.getAppIF() instanceof VAdminIF)
            c = WFontColors.getColor(strColor);
        else
            c = DisplayOptions.getColor(strColor);
        return c;
    }

    public void setEditStatus(boolean s) {isEditing = s;}
    public void setEditMode(boolean s) {
        if (s) {
            acq_button.setEnabled(false);
            info_button.setEnabled(false);
            stat_button.setEnabled(false);
        defDim = getPreferredSize();
            curLoc.x = defLoc.x;
            curLoc.y = defLoc.y;
            curDim.width = defDim.width;
            curDim.height = defDim.height;
       }
        else{
            acq_button.setEnabled(true);
            info_button.setEnabled(true);
            stat_button.setEnabled(true);
       }
       inEditMode = s;
    }
    public void addDefChoice(String s) {}
    public void addDefValue(String s) {}
    public void setDefLabel(String s) {}
    public void setDefColor(String s) {}
    public void setDefLoc(int x, int y) {
    defLoc.x = x;
    defLoc.y = y;
    }

    public Point getDefLoc() {
    tmpLoc.x = defLoc.x;
        tmpLoc.y = defLoc.y;
        return tmpLoc;
    }

    public void setSizeRatio(double x, double y) {
    double xRatio =  x;
        double yRatio =  y;
        if (x > 1.0)
            xRatio = x - 1.0;
        if (y > 1.0)
            yRatio = y - 1.0;
    if (defDim.width <= 0)
        defDim = getPreferredSize();
        curLoc.x = (int) ((double) defLoc.x * xRatio);
        curLoc.y = (int) ((double) defLoc.y * yRatio);
        curDim.width = (int) ((double)defDim.width * xRatio);
        curDim.height = (int) ((double)defDim.height * yRatio);
        if (!inEditMode)
            setBounds(curLoc.x, curLoc.y, curDim.width, curDim.height);
    }

    public void setBounds(int x, int y, int w, int h) {
        if (inEditMode) {
           defLoc.x = x;
           defLoc.y = y;
           defDim.width = w;
           defDim.height = h;
        }
        curLoc.x = x;
        curLoc.y = y;
        curDim.width = w;
        curDim.height = h;
        super.setBounds(x, y, w, h);
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

    public void refresh() {
        setState(state);
        acq_button.paintIcon();
        info_button.paintIcon();
        stat_button.paintIcon();
        repaint();
        if(m_window.isVisible())
            m_window.repaint();
    }
    public void updateValue() {}
    public void setValue(ParamIF p) {}
    public void setShowValue(ParamIF p) { }
    public void changeFocus(boolean s) {isFocused = s;}
    public ButtonIF getVnmrIF() {return vnmrIf;}
    public void setVnmrIF(ButtonIF vif) {vnmrIf=vif;}
    public void  destroy() {
        DisplayOptions.removeChangeListener(this);
        Messages.removeMessageListener(this);
        for (int i=0; i<getComponentCount(); i++) {
            Component comp = getComponent(i);
            if (comp instanceof VObjIF){
                ((VObjIF)comp).destroy();
            }
        }
    }
    public void changeFont() { }

    public String getAttribute(int attr) {
        switch (attr) {
        case TYPE:
            return type;
        case LABEL:
            return label;
        case DISPLAY:
            return dispOptions;
        case PANEL_NAME:
            return objName;
        }
        return null;
    }

    public void setAttribute(int attr, String c) {
        switch (attr) {
        default:
            break;
        case TYPE:
            type = c;
            break;
        case DISPLAY:
            dispOptions=c;
            StringTokenizer tok = new StringTokenizer(c);
            while(tok.hasMoreTokens()){
                String s=tok.nextToken();
                if(s.equals("status"))
                    showStatus=true;
                else if(s.equals("rightbuttons"))
                    rightButtons=true;
            }
            break;
        case PANEL_NAME:
            objName = c;
            break;
       }
    }

    public void setBg(Color c){
        setBackground(c);
        //showline.setBackground(c);
        showline.setBorder(Util.entryBorder());
        acq_button.setBackground(c);
        info_button.setBackground(c);
        stat_button.setBackground(c);
        if (m_window != null) {
            m_window.setBackground(c);
            m_window.menu.setBackground(c);
        }
    }

    public void addMessage(VMessage obj){
        DefaultTableModel tableModel = (DefaultTableModel)m_table.getModel();
        while (tableModel.getRowCount() > Util.getStatusMessagesNumber()) {
            tableModel.removeRow(0);
        }
        m_table.addRow(obj);
    }

    public ArrayList getList(){
        return m_vector;
    }

    public void setList(ArrayList<VMessage> list){
        m_vector=list;
        DefaultTableModel tableModel = (DefaultTableModel)m_table.getModel();
        tableModel.setRowCount(0);
        int nLength = list.size();
        for (int i = 0; i < nLength; i++)
        {
            VMessage[] aVMessage = {(VMessage)list.get(i)};
            m_table.addRow(aVMessage, false);
        }
        // m_table.positionScroll(REASON_INITIAL_VIEW);
        if (nLength > 1)
           m_table.scrollToRowLater(nLength - 1);
    }

    // DropTargetListener interface

    public void dragEnter(DropTargetDragEvent e) { }
    public void dragExit(DropTargetEvent e) {}
    public void dragOver(DropTargetDragEvent e) {}
    public void dropActionChanged (DropTargetDragEvent e) {}

    public void drop(DropTargetDropEvent e) {
        VObjDropHandler.processDrop(e, this, inEditMode);
    } // drop

    public void setModalMode(boolean s) {}
    public void sendVnmrCmd() {}

    //============== Local classes ===============================

    class MessageButton extends JToggleButton{
        int state=STATE_EMPTY;
        boolean newflag=false;
        VRectIcon icon;
        public MessageButton(){
            setBorder(new VButtonBorder(false));
            icon=new VRectIcon(11, 11);
            setIcon(icon);
            setFocusPainted(false);
        }

        void setMessageFlag(int s) {
            newflag=true;
            state=s;
            paintIcon();
        }

        void clrMessageFlag() {
            newflag=false;
            state=STATE_EMPTY;
            paintIcon();
        }

        void paintIcon(){
            if(state==STATE_EMPTY)
                icon.setColor(offColor);
            else if(newflag){
                String var=names[state] + "HiLite";
                icon.setColor(DisplayOptions.getColor(var));
            }
            else
                icon.setColor(offColor);
        }

        int getState(){
            return state;
        }

        // NB: The default ToolTip location seems good (didn't used to work)
        // public Point getToolTipLocation(MouseEvent event) {
        //     return new Point(0,0);
        // }

    } // class MessageButton


    class MessageLayout implements LayoutManager {
        public void addLayoutComponent(String name, Component comp) {}
        public void removeLayoutComponent(Component comp) {}
        public Dimension preferredLayoutSize(Container target) {
            return  psize; // unused
        } // preferredLayoutSize()

        public Dimension minimumLayoutSize(Container target) {
            return new Dimension(80, 20); // unused
        } // minimumLayoutSize()
        public void layoutContainer(Container target) {
        synchronized (target.getTreeLock()) {
            int x;
            if(showStatus){
                x=psize.width-60;
                stat_button.setVisible(true);
            }
            else{
                x=psize.width-40;
                stat_button.setVisible(false);
            }
            if(x<10)
                x=10;
            int y=psize.height;
            if(!rightButtons){
                acq_button.setBounds(0, 1, 20, y-2);
                info_button.setBounds(20, 1, 20, y-2);
                if(showStatus){
                    stat_button.setBounds(40, 1, 20, y-2);
                    showline.setBounds(60, 0, x, y);
                }
                else
                    showline.setBounds(40, 0, x, y);
            }
            else{
                showline.setBounds(0, 0, x, y);
                acq_button.setBounds(x, 1, 20, y-2);
                info_button.setBounds(x+20, 1, 20, y-2);
                if(showStatus)
                    stat_button.setBounds(x+40, 1, 20, y-2);
            }
          }
       }
    } // class MessageLayout

    class MessageWindow extends JDialog {
        private JPanel panel=null;
        protected TableCellRenderer m_messageCellRenderer;
        private MouseAdapter ma=null;
        private SubMenu menu=null;
        private JScrollPane scrollPane = null;

        public MessageWindow(EasyJTable table) {
            super(VNMRFrame.getVNMRFrame()); // Attach dialog to main VJ frame
            if (VNMRFrame.getVNMRFrame() == null) {
                throw(new NullPointerException("Null owner for Dialog window"));
            }
            panel=new JPanel();
            panel.setLayout(new BorderLayout());
                panel.setPreferredSize(new Dimension(300,50));
            if (table == null)
                m_table = new EasyJTable(new DefaultTableModel(0, 1));
            else
                m_table = table;
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
            getContentPane().add(panel);
            menu=new SubMenu();
            ma=new MouseAdapter() {
                public void mouseClicked(MouseEvent evt) {
                    int modifier = evt.getModifiers();
                    if((modifier & InputEvent.BUTTON3_MASK) !=0)
                        menuAction(evt);
                }
            };
            scrollPane.addMouseListener(ma);
            m_table.addMouseListener(ma);
            addWindowListener (new WindowAdapter() {
                public void  windowClosing(WindowEvent e) {
                    acq_button.getModel().setPressed(false);
                    info_button.getModel().setPressed(false);
                    stat_button.getModel().setPressed(false);
                    doCloseAction();
                    }
                }
            );
            addComponentListener (new ComponentAdapter() {
                public void componentMoved(ComponentEvent e) {
                    // checkWindowGeometry();
                }
                public void componentResized(ComponentEvent e) {
                    checkWindowGeometry();
                }
                });
        }

        /** show popup menu. */
        public void menuAction(MouseEvent  e) {
            menu.show((Component)e.getSource(), e.getX(), e.getY());
        }

        public void setBackground(Color colorbg)
        {
            if (m_table != null)
                m_table.setBackground(colorbg);
            super.setBackground(colorbg);
        }

        public void setVisible(boolean b){
            if(b){
                DefaultTableModel tableModel = (DefaultTableModel)m_table.getModel();
                boolean bNewMessage = false;
                int nRows = tableModel.getRowCount();
                for (int i = 0; i < nRows; i++)
                {
                    VMessage vMessage = (VMessage)tableModel.getValueAt(i, 0);
                    vMessage.cnt = i+1;
                    tableModel.setValueAt(vMessage, i, 0);
                }
                if (m_vector==infoList)
                {
                    m_nInfo = nRows;
                    bNewMessage = bNewInfoMessage;
                    bNewInfoMessage = false;
                }
                else if (m_vector == acqList)
                {
                    m_nAcq = nRows;
                    bNewMessage = bNewAcqMessage;
                    bNewAcqMessage = false;
                }
                else if (m_vector == statusList)
                {
                    m_nStatus = nRows;
                    bNewMessage = bNewStatusMessage;
                    bNewStatusMessage = false;
                }
                if (bNewMessage && nRows > 1) {
                    Rectangle r = m_table.getCellRect(nRows-1, 0, false);
                    m_table.scrollRectToVisible(r);
                }
            }
            super.setVisible(b);
        }

        class MessageCellRenderer extends JTextArea
            implements TableCellRenderer {

            final Border mm_narrowMargin = new EmptyBorder(0, 2, 0, 0);

            public MessageCellRenderer() {
                setOpaque(true);
                setLineWrap(true);
                setWrapStyleWord(true);
            }

            public Component getTableCellRendererComponent(
                JTable table,
                Object value,
                boolean isSelected,
                boolean cellHasFocus, int row, int column)
            {
                setOpaque(true);
                VMessage m = (VMessage)value;
                String s = m.toString();
                setTabSize(2);
                if (showCount) {
                    int iLast = -1;
                    if (m_vector != null && (iLast = m_vector.size() - 1) >= 0) {
                        int nDigits = (int)Math.log10(m_vector.get(iLast).cnt);
                        setTabSize(nDigits + 1);
                    }
                }
                if(showDate) {
                    // Print (count and) date on a separate line.
                    // Insert a newline before the (second or) first TAB.
                    int idx = s.indexOf('\t');
                    if (idx > 0) {
                        if (!showCount || (idx = s.indexOf('\t', ++idx)) > 0) {
                            s = s.substring(0, idx) + "\n" + s.substring(idx);
                        }
                    }
                }
                setText(s);
                String var = names[m.type];
                //Color c=DisplayOptions.getColor(var);
                Color c = getColor(var);
                Color bg = getColor(var + "Bkg");
                Color curbg = bg;
                Color curfg;

                if (invertColors) {
                    setBackground(c);
                    curbg = c;
                    if (bg == null) {
                        bg = Util.getContrastingColor(c);
                    }
                    setForeground(bg);
                    curfg = bg;
                } else {
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
                }
                Font f=DisplayOptions.getFont(var,var,var);
                setFont(f);
                setBorder(mm_narrowMargin);
                if (isSelected) {
                    //selectAll(); // Does not highlight
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


        class SubMenu extends JPopupMenu {
            ActionListener ItemListener;
            public SubMenu() {
                String string;
                readPersistence();
                ItemListener = new ActionListener() {
                    public void actionPerformed(ActionEvent evt) {
                        JMenuItem item=(JMenuItem)evt.getSource();
                        String s=evt.getActionCommand();
                        if(s.equals(show_count_str)){
                            boolean state = item.isSelected();
                            showCount = state;
                            VMessage.setShowCount(state);
                            refresh();
                        }
                        else if(s.equals(show_date_str)){
                            boolean state = item.isSelected();
                            showDate = state;
                            VMessage.setShowDate(state);
                            //setList(m_vector);
                            refresh();
                        }
                        else if(s.equals(invert_colors_str)){
                            invertColors=item.isSelected()?true:false;
                            refresh();
                        }
                        else if(s.equals(clr_history_str)){
                            clearVector();
                        }
                        writePersistence();
                    }
                };

                // Show/Hide Count

                JCheckBoxMenuItem citem;

                citem=new JCheckBoxMenuItem(show_count_str,showCount);
                citem.addActionListener(ItemListener);
                add(citem);

                // Show/Hide Date

                citem=new JCheckBoxMenuItem(show_date_str,showDate);
                citem.addActionListener(ItemListener);
                add(citem);

                // Invert colors

                citem=new JCheckBoxMenuItem(invert_colors_str,invertColors);
                citem.addActionListener(ItemListener);
                add(citem);

                // Clear data

                JMenuItem item=new JMenuItem(clr_history_str);
                item.addActionListener(ItemListener);
                add(item);
            }
        } // class SubMenu
    }   // class MessageWindow
}


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
import java.awt.datatransfer.*;

import java.io.*;
import java.util.*;
import java.beans.*;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.text.*;
import javax.swing.border.*;

import vnmr.ui.*;
import vnmr.bo.*;
import vnmr.ui.shuf.*;

/**
 * Command Input
 * accept command and send to Vnmr
 */
public class CommandInput extends JPanel
implements DropTargetListener, ListSelectionListener, MouseListener, PropertyChangeListener {
    /** session share */
    private SessionShare m_sshare;
    private AppIF m_appIf;
    private JTextField  m_txfCmdInput;
    private JLabel      m_prompt;
    private JButton     m_historyBtn;
    private int  m_nStartPos = 0;
    private int  nCmdIndex = -1;
    private String m_strPrompt;
    private CaretListener m_cl;
    private Document m_objDoc;

    private JList        m_list;
    private JPanel       m_listPanel;
    private JPanel       m_pnlContent;
    /*
     * Note: JWindow jams up in focus-follows-mouse mode.
     *       Seems to continuously generate events.
     *       Don't know how to get rid of title bar on JFrame, but
     *       at least it works.
     */
    //private JWindow      m_winShowList    = new JWindow();
    private JDialog      m_winShowList;
    private ArrayList    m_cmdList     = new ArrayList();
   //  private ExperimentIF m_theFrame       = null;
    private String       m_strDir         = null;
    private String       m_strPerdir      = null;
    private File         m_file           = null;
    private String       m_strFilepath    = null;
    private boolean	 m_bIsListVisible = false;
    // private Insets orgInsets = new Insets(1, 3, 1, 2);

    /**
     * constructor
     */
    public CommandInput(SessionShare sshare, AppIF ep, AppInstaller appInstaller) {
        this.m_sshare = sshare;
        this.m_appIf = ep;
        JFrame frame = null;
        if (appInstaller instanceof JFrame)
            frame = (JFrame)appInstaller;
        m_winShowList = new JDialog(frame, "", false);
        putClientProperty(VnmrjUiNames.PanelTexture, "no");

        createHistoryList();

        // setLayout(new BorderLayout());
        setLayout(new cmdinputLayout());
        // setBorder(new EtchedBorder(EtchedBorder.LOWERED));
        m_historyBtn = new JButton( Util.getImageIcon( "down.gif" ));
        m_historyBtn.setBorder(new BevelBorder( BevelBorder.RAISED ));
        add( m_historyBtn, BorderLayout.WEST );
        m_winShowList.setUndecorated(true);
        m_historyBtn.addActionListener( new ActionListener()
        {
           public void actionPerformed( ActionEvent e )
           {
               if( m_bIsListVisible )
               {
                   m_winShowList.setVisible( false );
                   m_winShowList.dispose();
                   m_winShowList.validate();
                   m_bIsListVisible = false;
                   setFocus();
               }
               else
               {
                   m_pnlContent =  ( JPanel ) m_winShowList.getContentPane();

                   m_list.setListData( m_cmdList.toArray() );
                   m_listPanel.removeAll();
                   m_listPanel.add( new JScrollPane( m_list ), BorderLayout.CENTER);
                   m_pnlContent.add(m_listPanel, BorderLayout.CENTER );
                   //m_pnlContent.setBorder( BorderFactory.createLineBorder( Color.lightGray, 5 ));

                   // Rectangle rect = m_theFrame.getCommandLineLoc();
                   Rectangle rect = getBounds();
                   Point pt = m_historyBtn.getLocationOnScreen();
                   m_winShowList.setBounds( pt.x, pt.y+25,
                                            rect.width, rect.height + 200 );
                   m_winShowList.setVisible( true );
                   m_bIsListVisible = true;
                   //setFocus();
               }
           }
        });

        readHistoryFile();

        m_txfCmdInput = new JTextField();
        //  m_txfCmdInput.setOpaque(false);
        m_txfCmdInput.setBorder(new EmptyBorder(1,4,1,4));
        m_prompt = new JLabel();
        m_prompt.setVisible(false);
        m_prompt.setOpaque(false);
        add(m_prompt, BorderLayout.EAST);
        add(m_txfCmdInput, BorderLayout.CENTER);
        m_nStartPos = 0;
        m_txfCmdInput.addActionListener(new ActionListener()
        {
           public void actionPerformed(ActionEvent ev)
           {
               sendCmd(ev);
           }
        });

        m_txfCmdInput.addKeyListener(new KeyAdapter() {
          public void keyTyped(KeyEvent e) { }
          public void keyPressed(KeyEvent e) {
               processKeyEvent(e);
          }
          public void keyReleased(KeyEvent e) { }
        });

         m_txfCmdInput.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent evt) {
                 nextFocus();
                 setFocus();
            }
        });

/******
        m_objDoc = m_txfCmdInput.getDocument();
        m_cl = new CaretListener() {
            public void caretUpdate(CaretEvent e) {
                int x = m_txfCmdInput.getCaretPosition();
                if (m_nStartPos > 0 && x < m_nStartPos) {
                   if (m_objDoc != null && m_objDoc.getLength() >= m_nStartPos)
                       m_txfCmdInput.setCaretPosition(m_nStartPos);
                }
            }
        };

        KeyStroke upKey = KeyStroke.getKeyStroke(KeyEvent.VK_UP, 0, true);
        KeyStroke dnKey = KeyStroke.getKeyStroke(KeyEvent.VK_DOWN, 0, true);
        KeyStroke backKey = KeyStroke.getKeyStroke(KeyEvent.VK_BACK_SPACE,
                0, true);
        m_txfCmdInput.registerKeyboardAction( new cmdKeyListener("up"), upKey, JComponent.WHEN_IN_FOCUSED_WINDOW);
        m_txfCmdInput.registerKeyboardAction( new cmdKeyListener("down"), dnKey,
                JComponent.WHEN_IN_FOCUSED_WINDOW);
        m_txfCmdInput.registerKeyboardAction( new cmdKeyListener("back"), backKey,
                JComponent.WHEN_IN_FOCUSED_WINDOW);
********/
        setUiColor();
        setHistoryIcon();
        new DropTarget(m_txfCmdInput, this);
        DisplayOptions.addChangeListener(this);
    } // CommandInput

    private void setUiColor() {
        Color c = m_txfCmdInput.getBackground();
        if (c != null)
            setBackground(new Color(c.getRGB()));
        c = Util.getInputFg();
        if (c != null) {
            m_txfCmdInput.setForeground(c);
            m_txfCmdInput.setCaretColor(c);
            m_prompt.setForeground(c);
        }
    }

    private void setHistoryIcon() {
        JButton btn = Util.getArrowButton();
        if (btn != null) {
             Icon icon = btn.getIcon();
             if (icon != null) {
                 m_historyBtn.setIcon(icon);
                 Border b = btn.getBorder();
                 if (b != null)
                     m_historyBtn.setBorder(b);
                 int w = icon.getIconWidth();
                 int h = icon.getIconHeight();
                 m_historyBtn.setPreferredSize(new Dimension(w + 8, h + 4));
             }
        }
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        if (DisplayOptions.isUpdateUIEvent(evt)) {
            setUiColor();
            setHistoryIcon();
            SwingUtilities.updateComponentTreeUI(m_winShowList);
            if (m_listPanel != null)
                SwingUtilities.updateComponentTreeUI(m_listPanel);
        }
    }

    private void sendCmd(ActionEvent ev)
    {
        String newCmd = m_txfCmdInput.getText();
	if (newCmd == null)
	    return;

        newCmd = newCmd.trim();
        if (newCmd.length() >= 1)
            updateCmdList( newCmd );

/****
        if (m_nStartPos > 0) {
            if (newCmd.length() < m_nStartPos)
                newCmd = "\n";
            else
                newCmd = newCmd.substring(m_nStartPos)+"\n";
            m_appIf.sendToVnmr("M@inp"+newCmd);
            m_txfCmdInput.removeCaretListener(m_cl);
        }
        else {
            newCmd = m_txfCmdInput.getText().trim()+"\n";
            if (newCmd.length() <= 1)
                return;
            m_appIf.sendCmdToVnmr(newCmd);
        }
***/
        m_nStartPos = 0;
        newCmd = newCmd + "\n";
        m_txfCmdInput.setText("");

        if (m_prompt.isVisible()) {
            m_appIf.sendToVnmr("M@inp"+newCmd);
            m_prompt.setVisible(false);
        }
        else {
            if (newCmd.length() > 1)
              m_appIf.sendCmdToVnmr(newCmd);
        }
    }

    public void setOutput(String  s) {
        if (m_nStartPos > 0) {
            // m_prompt.setVisible(false);
            // m_txfCmdInput.removeCaretListener(m_cl);
        }
        if (m_prompt.isVisible())
            m_prompt.setVisible(false);
        if (s == null)
            s = "";
        m_nStartPos = 0;
        m_txfCmdInput.setText(s);
        m_txfCmdInput.setCaretPosition(s.length());
    }

    public void setFocus() {
        m_txfCmdInput.requestFocus();
    }

    public void setPrompt(String  s) {
        m_nStartPos = 0;
	if (s != null)
            m_nStartPos = s.length();
        if (m_nStartPos > 0) {
            m_prompt.setText(s);
            // Dimension d = m_prompt.getPreferredSize();
            m_prompt.setVisible(true);
/*
            int oldPos = m_nStartPos;
            m_nStartPos = s.length();
            m_strPrompt = s;
            m_txfCmdInput.setText(s);
            m_txfCmdInput.setCaretPosition(m_nStartPos);
            if (oldPos <= 0)
                m_txfCmdInput.addCaretListener(m_cl);
*/
	}
        else if (m_prompt.isVisible()) {
            m_prompt.setVisible(false);
	}
        m_txfCmdInput.setText("");
        m_txfCmdInput.setCaretPosition(0);
    }

    /* Write to the history command persistent file */
    public void writePersistence()
    {
        m_strFilepath=FileUtil.savePath(new StringBuffer().append("USER").append(
                        File.separator).append("PERSISTENCE").append(
                        File.separator).append("CommandHistory").toString());
        PrintWriter os;
        String cmd = null;

        if (m_strFilepath == null)
        {
            Messages.postDebug("File not found USER/PERSISTENCE/CommandHistory");
            return;
        }

        try
        {
           os = new PrintWriter( new FileWriter( m_strFilepath ));
           int nLength = m_cmdList.size();
           for( int i = 0; i < nLength; i++ )
           {
              cmd = ( String ) m_cmdList.get(i);
              os.println( cmd );
           }
           os.close();
        }
        catch( IOException e )
        {
           System.err.println( "can't create persistence file" );
        }
    }

    public void readHistoryFile()
    {
        BufferedReader in;
        String line = null;

        try
        {
            m_strFilepath = FileUtil.openPath( "USER/PERSISTENCE/CommandHistory");
            if(m_strFilepath==null)
                return;
            in = new BufferedReader( new FileReader( m_strFilepath ));
            while(( line = in.readLine()) != null )
                m_cmdList.add( line );
            in.close();
        }
        catch( Exception e )
        {
            System.err.println(e.toString());
            Messages.writeStackTrace(e);
        }
    }

    public String getFilePath()
    {
        m_strDir=FileUtil.usrdir();
        m_strPerdir=FileUtil.savePath(new StringBuffer().append("USER").append(
                        File.separator).append("PERSISTENCE").toString());
        m_strFilepath = FileUtil.savePath(new StringBuffer().append("USER").append(
                            File.separator).append("PERSISTENCE").append(
                            File.separator).append("CommandHistory").toString());
        return m_strFilepath;
    }

    private void createHistoryList()
    {
        m_listPanel = new JPanel();
        m_listPanel.setLayout( new BorderLayout() );

        m_list = new JList();
        // m_list.setBackground( Color.white );

        m_list.setListData( m_cmdList.toArray() );
        m_list.addListSelectionListener( this );
        m_list.addMouseListener( this );
        m_listPanel.add( new JScrollPane( m_list ), BorderLayout.CENTER );
    }

    public void valueChanged( ListSelectionEvent e )
    {
        Object selValue = m_list.getSelectedValue();
        m_txfCmdInput.setText(( String ) selValue );

        m_winShowList.setVisible( false );
        m_winShowList.dispose();
        m_winShowList.validate();
        m_bIsListVisible = false;
        setFocus();
    }

/*
    public void setFrame( ExperimentIF frame )
    {
        m_theFrame = frame;
    }
*/

    private void updateCmdList( String cmd )
    {
        if (m_cmdList.contains(cmd))
            m_cmdList.remove(cmd);
        m_cmdList.add(0, cmd);

        int nLength = m_cmdList.size();
        if (nLength > 100)
            m_cmdList.remove(nLength-1);

        nCmdIndex = -1;
     }

     public void dispHistory( boolean bPrevious ) {
         if (m_nStartPos > 0)
            return;
         if (DebugOutput.isSetFor("fullCommandHistory")) {
             if (bPrevious)
                m_appIf.sendToVnmr("M@prev\n");
            else
                m_appIf.sendToVnmr("M@next\n");
            return;
        }
        String strCmd = "";
        if (bPrevious) {
            nCmdIndex++;
            if (nCmdIndex < 0)
                nCmdIndex = 0;
        }
        else {
            nCmdIndex--;
            if (nCmdIndex >= m_cmdList.size())
                nCmdIndex =  m_cmdList.size() - 1;
        }
        if (nCmdIndex >= 0 && nCmdIndex < m_cmdList.size())
            strCmd = ( String ) m_cmdList.get(nCmdIndex);
    	setOutput(strCmd);
     }


     public void processXKeyEvent(KeyEvent  e) {
        if (!m_txfCmdInput.isShowing())
            return;
        int id = e.getID();
        Keymap binding = m_txfCmdInput.getKeymap();
        if (binding == null) {
            return;
        }
        KeyStroke k = KeyStroke.getKeyStrokeForEvent(e);
        Action a = binding.getAction(k);

        if (a == null && id == KeyEvent.KEY_TYPED)
            a = binding.getDefaultAction();
        if (a == null)  {
            id = e.getKeyCode();
            if (id == KeyEvent.VK_UP) {
		dispHistory(true);
	    }
            else if (id == KeyEvent.VK_DOWN) {
		dispHistory(false);
	    }
            return;
	}
        Caret c = m_txfCmdInput.getCaret();
        c.setVisible(true);
        String command = null;
        if (e.getKeyChar() != KeyEvent.CHAR_UNDEFINED) {
                    command = String.valueOf(e.getKeyChar());
        }
        ActionEvent ae = new ActionEvent(m_txfCmdInput,
                              ActionEvent.ACTION_PERFORMED,
                              command,
                              e.getModifiers());
        e.consume();
        a.actionPerformed(ae);
    }

    private boolean controlEvent(int num) {
        boolean  ret = true;

        switch (num) {
            case KeyEvent.VK_C:  // cancelCmd
                    m_appIf.sendToVnmr("M@cancel\n");
                    break;
/*
            case KeyEvent.VK_H:  // move_left
                    break;
            case KeyEvent.VK_F:  // move_right
                    break;
*/
            case KeyEvent.VK_N:  // nextCommand
		    dispHistory(false);
                    break;
            case KeyEvent.VK_P:  // previousCommand
		    dispHistory(true);
                    break;
            case KeyEvent.VK_U:  // clear line
                    m_txfCmdInput.setText("");
                    break;
            case KeyEvent.VK_X:  // delete selection
                    m_txfCmdInput.replaceSelection("");
                    break;
            default:
                    ret = false;
                    break;
        }
        return (ret);
    }


    public void processKeyEvent(KeyEvent  e) {
        ExpPanel exp;
        boolean  toConsume;
        int code = e.getKeyCode();
        int id = e.getID();

        toConsume = false;
        if (e.isControlDown()) {
           toConsume = controlEvent(code);
        }
        else if (code >= KeyEvent.VK_F1 && code <= KeyEvent.VK_F12) {
           code = code - KeyEvent.VK_F1;
           exp = Util.getActiveView();
           if (exp != null)
               exp.funcKeyCall(code);
        }
        else {
           switch (code) {
              case KeyEvent.VK_DOWN:
		    dispHistory(false);
                    toConsume = true;
                    break;
              case KeyEvent.VK_UP:
		    dispHistory(true);
                    toConsume = true;
                    break;
              // case KeyEvent.VK_BACK_SPACE:
              //      break;
            }
         }
         if (toConsume)
            e.consume();
    }

    private class cmdKeyListener implements ActionListener {
        String key;
        cmdKeyListener (String s) {
            key = s;
        }

        public void actionPerformed(ActionEvent ev) {
            if (!m_txfCmdInput.hasFocus())
                return;
            if (m_nStartPos > 0) {
               /*****
                if (key.equals("back")) {
                   int x = m_txfCmdInput.getCaretPosition();
                   if (x <= m_nStartPos) {
                        m_txfCmdInput.setText(m_strPrompt);
                        m_txfCmdInput.setCaretPosition(m_nStartPos);
                   }
                }
                *****/
                return;
            }
            if (key.equals("up")) {
		dispHistory(true);
	    }
            else if (key.equals("down")) {
		dispHistory(false);
	    }

        }
    }

    /****************************************************************
     ********   Mouse listeners interface                   ********
     ***************************************************************/

     public void mouseClicked( MouseEvent evt )
     {
        if( m_cmdList.size() == 0 )
        {
           m_winShowList.setVisible( false );
           m_winShowList.dispose();
           m_winShowList.validate();
           m_bIsListVisible = false;
        }
     }

     public void mouseEntered( MouseEvent evt )  {}
     public void mouseExited( MouseEvent evt )   {}
     public void mousePressed( MouseEvent evt )  {}
     public void mouseReleased( MouseEvent evt ) {}

     public void dragEnter(DropTargetDragEvent e) { }
     public void dragExit(DropTargetEvent e) {}
     public void dragOver(DropTargetDragEvent e) {}
     public void dropActionChanged (DropTargetDragEvent e) {}

     public void drop(DropTargetDropEvent e) {
       try {
           Transferable tr = e.getTransferable();
           e.acceptDrop(DnDConstants.ACTION_MOVE);
           e.getDropTargetContext().dropComplete(true);
           if (tr.isDataFlavorSupported(LocalRefSelection.LOCALREF_FLAVOR)) {
              Object obj = tr.getTransferData(LocalRefSelection.LOCALREF_FLAVOR);
              if (obj instanceof ShufflerItem) {
                  // Get the ShufflerItem which was dropped.
                  ShufflerItem item = (ShufflerItem) obj;

                  if (item.objType.equals(Shuf.DB_WORKSPACE)) {
                      m_txfCmdInput.setText("j" + item.filename);
                  }
                  else if (item.objType.endsWith(Shuf.DB_MACRO)) {
                      m_txfCmdInput.setText( item.filename );
                  }
                  else {
                    item.actOnThisItem("Line3", "DragNDrop", "");
                  }
              }
              setFocus();
           }
	}
        catch (IOException io) {io.printStackTrace();}
        catch (UnsupportedFlavorException ufe) {System.out.println(ufe.toString()); }
    }

    private class cmdinputLayout extends BorderLayout {
      public void layoutContainer(Container target) {
        synchronized (target.getTreeLock()) {
          Insets insets = target.getInsets();
          Dimension tSize = target.getSize();
          int top = insets.top;
          int bottom = tSize.height - insets.bottom;
          int left = insets.left;
          int right = tSize.width - insets.right;
          int height;
          Dimension d;

          if (tSize.width < 20 || tSize.height < 6)
              return;
          left = 0;
          height = tSize.height;
          if (height > 24)
             height = 24;
          m_historyBtn.setBounds(left, 0, height, tSize.height);
          left += height;
          height = bottom - top;
          if (m_prompt.isVisible()) {
             left += 4;
             d = m_prompt.getPreferredSize();
             m_prompt.setBounds(left, top, d.width, height);
             left += d.width;
          }
          insets = m_txfCmdInput.getMargin();
          if ((right - left) <  20)
              left = right - 20;
          m_txfCmdInput.setBounds(left, top, right - left, height);
          if (insets != null) {
              if (insets.left > 10 || insets.top > 10)
                  m_txfCmdInput.setMargin(new Insets(0, 0, 0, 0)); 
          } 
        }
      }
    }

} // class CommandInput


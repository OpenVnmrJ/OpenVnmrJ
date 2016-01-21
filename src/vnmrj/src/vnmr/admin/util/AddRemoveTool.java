/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.io.*;
import java.awt.*;
import javax.swing.*;
import java.util.*;
import java.awt.event.*;
import javax.swing.plaf.basic.*;

import vnmr.util.*;
import vnmr.admin.ui.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class AddRemoveTool extends ModelessDialog implements ActionListener
{

    /** The split pane used for display.  */
    protected JSplitPane m_spDisplay;

    /** The panel on the left. */
    protected JPanel     m_pnlLeft      = new JPanel();

    /** The panel for the right. */
    protected JPanel     m_pnlRight     = new JPanel();

    /** The list for the left. */
    protected JList      m_listLeft     = new JList();

    /**  The list for the right. */
    protected JList      m_listRight    = new JList();

    /** The arraylist that has values for the left.  */
    protected ArrayList  m_aListLeft    = new ArrayList();

    /** The arraylist that has values for the right.   */
    protected ArrayList  m_aListRight   = new ArrayList();

    /** The ui object for the split pane. */
    protected ButtonDividerUI m_objDividerUI;

    protected ActionListener m_alBtn;

    /** The east arrow button.  */
    protected JButton m_btnEast = new JButton(); /*new BasicArrowButton(BasicArrowButton.EAST,
                                                Color.green, Color.gray, Color.darkGray,
                                                Color.white);*/

    /** The west arrow button.  */
    protected JButton m_btnWest = new JButton(); /*new BasicArrowButton(BasicArrowButton.WEST,
                                                Color.green, Color.gray, Color.darkGray,
                                                Color.white);*/

    protected Container m_contentPane;

    protected java.util.Timer timer;
    protected String labelString;       // The label for the window
    protected int delay;                // the delay time between blinks


    public AddRemoveTool(String strToolLbl)
    {
        super(strToolLbl);

        setVisible(false);
        m_pnlLeft.setLayout(new BorderLayout());
        m_pnlRight.setLayout(new BorderLayout());
        dolayout();
        m_contentPane = getContentPane();

        setAbandonEnabled(true);
        setCloseEnabled(true);

        abandonButton.setActionCommand("cancel");
        abandonButton.addActionListener(this);
        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

        m_alBtn = new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                updateListData(e);
                if (timer != null)
                    timer.cancel();
            }
        };

        m_btnEast.setIcon(Util.getImageIcon("eastarrow.gif"));
        m_btnEast.addActionListener(m_alBtn);
        m_btnEast.setActionCommand("east");
        m_btnEast.setBackground(Color.white);
        m_btnEast.setForeground(Color.green);
        m_btnWest.setIcon(Util.getImageIcon("westarrow.gif"));
        m_btnWest.addActionListener(m_alBtn);
        m_btnWest.setActionCommand("west");
        m_btnWest.setBackground(Color.white);
        m_btnWest.setForeground(Color.yellow);
        m_objDividerUI = new ButtonDividerUI();

        initBlink();

        setResizable(true);
        setTitle(strToolLbl);

    }

    protected void dolayout()
    {
        JScrollPane scpLeft = new JScrollPane(m_listLeft);
        JScrollPane scpRight = new JScrollPane(m_listRight);

        m_pnlLeft.add(scpLeft);
        m_pnlRight.add(scpRight);

        m_spDisplay = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, true,
                                        m_pnlLeft, m_pnlRight);
        m_spDisplay.setUI(m_objDividerUI);
        getContentPane().add(m_spDisplay);

        m_aListLeft = WUserUtil.readUserListFile();
        m_aListRight.add("color");

        m_listLeft.setListData(m_aListLeft.toArray());
        m_listRight.setListData(m_aListRight.toArray());

        setLocation( 300, 500 );

        int nWidth = buttonPane.getWidth();
        int nHeight = buttonPane.getPreferredSize().height +
                        m_spDisplay.getPreferredSize().height + 100;
        setSize(500, 300);

    }

    protected void initBlink()
    {
        String blinkFrequency = null;
        delay = (blinkFrequency == null) ? 400 :
            (1000 / Integer.parseInt(blinkFrequency));
        labelString = null;
        if (labelString == null)
            labelString = "Blink";
        Font font = new java.awt.Font("TimesRoman", Font.PLAIN, 24);
        setFont(font);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if (timer != null)
            timer.cancel();

        if(cmd.equals("close"))
        {
            // run the script to save data
            saveData();
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("cancel"))
        {
            undoAction();
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("help"))
            displayHelp();
    }

    /**
     *  Returns the list that has values for the left.
     */
    public ArrayList getLeftList()
    {
        return m_aListLeft;
    }

    /**
     *  Sets the arraylist for the left.
     */
    public void setLeftList(ArrayList aList)
    {
        m_aListLeft = aList;
        m_listLeft.setListData(m_aListLeft.toArray());
    }

    /**
     *  Returns the list that has values for the right.
     */
    public ArrayList getRightList()
    {
        return m_aListRight;
    }

    /**
     *  Sets the list that has values for the right.
     */
    public void setRightList(ArrayList aList)
    {
        m_aListRight = aList;
        m_listRight.setListData(m_aListRight.toArray());
    }

    /**
     *  Returns the dividerui for the split pane.
     */
    public BasicSplitPaneUI getDividerUI()
    {
        return m_objDividerUI;
    }

    protected void saveData()
    {
        // child classes overwrite this method to save data on close action.
    }

    protected void undoAction()
    {
        // child classes overwrite this method to undo action.
    }

    /**
     *  Updates the list data.
     */
    protected void updateListData(ActionEvent e)
    {
        if (e.getActionCommand().equals("east"))
        {
            Object[] aItems = m_listLeft.getSelectedValues();
            doAddRemove(false, aItems);
        }
        else
        {
            Object[] aItems = m_listRight.getSelectedValues();
            doAddRemove(true, aItems);
        }

        m_listLeft.setListData(m_aListLeft.toArray());
        m_listRight.setListData(m_aListRight.toArray());
    }

    protected void doAddRemove(boolean bLeftAdd, Object[] objItems)
    {
        String strName = null;

        for (int i = 0; i < objItems.length; i++)
        {
            strName = (String)objItems[i];
            if (bLeftAdd)
            {
                m_aListLeft.add(strName);
                m_aListRight.remove(strName);
            }
            else
            {
                m_aListRight.add(strName);
                m_aListLeft.remove(strName);
            }
        }
        Collections.sort(m_aListLeft);
        Collections.sort(m_aListRight);
    }

    class ButtonDividerUI extends BasicSplitPaneUI
    {

        public BasicSplitPaneDivider createDefaultDivider()
        {
            BasicSplitPaneDivider divider = new BasicSplitPaneDivider(this)
            {
                public int getDividerSize()
                {
                    return m_btnEast.getPreferredSize().width;
                }
            };

            int nHeight = m_pnlLeft.getSize().height / 4;
            nHeight = (nHeight <= 0) ? 30 : nHeight;
            divider.setLayout(new BoxLayout(divider, BoxLayout.Y_AXIS));
            divider.add(Box.createVerticalStrut(nHeight));
            divider.add(m_btnEast);
            divider.add(Box.createVerticalStrut(nHeight));
            divider.add(m_btnWest);
            divider.add(Box.createVerticalStrut(nHeight));

            return divider;
        }
    }


}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import javax.swing.plaf.basic.*;
import java.awt.event.*;
import javax.swing.event.*;

import vnmr.admin.ui.*;

/**
 * <p>Title: JComboButton </p>
 * <p>Description: This button has the regular button, and an arrow button with it.
 *                  The arrow button has a list of items which are selected,
 *                  and the behavior of the regular button changes depending on the
 *                  item selected. The behavior of the regular button is implemented
 *                  by the classes that use this class by adding an action listener
 *                  to the regular button. </p>
 * <p>Copyright: Copyright (c) 2002</p>
 *
 */

public class JComboButton extends JPanel
{

    protected JComboButton m_cmbBtn;
    protected JPopupMenu m_popMenu = new JPopupMenu();
    protected JButton m_btnReg = new JButton();
    protected BasicArrowButton m_btnArrow = new BasicArrowButton(BasicArrowButton.SOUTH);
    protected boolean m_bVisible = false;
    protected JList m_list;
    protected Color m_bgColor;


    public JComboButton()
    {
        this(null);
    }

    public JComboButton(String[] items)
    {
        this(items, null);
    }

    public JComboButton(String[] items, JButton btnReg)
    {
        super();
        m_cmbBtn = this;
        m_btnReg = (btnReg != null) ? btnReg : new JButton();
        setLayout(new WGridLayout(1, 0));
        add(m_btnReg);
        add(m_btnArrow);

        if (items != null)
        {
            dolayout(items);
        }
    }

    protected void dolayout(String[] items)
    {
        m_list = new JList(items);
        m_popMenu.add(m_list);

        m_list.addListSelectionListener(new ListSelectionListener()
        {
            public void valueChanged(ListSelectionEvent e)
            {
                m_btnArrow.doClick();
            }
        });

        m_btnArrow.addActionListener(new ActionListener()
        {
            public void actionPerformed(ActionEvent e)
            {
                int nXPos = m_btnReg.getWidth();
                int nYPos = m_btnReg.getHeight();
                if (!m_bVisible && !m_popMenu.isVisible())
                {
                    m_popMenu.show(m_btnReg, nXPos, nYPos);
                    m_bVisible = true;
                }
                else
                {
                    m_popMenu.setVisible(false);
                    m_bVisible = false;
                }
            }
        });
    }


    public JButton getArrowBtn()
    {
        return m_btnArrow;
    }

    public void setArrowBtn(BasicArrowButton btn)
    {
        m_btnArrow = btn;
    }

    public JButton getRegBtn()
    {
        return m_btnReg;
    }

    public void setRegBtn(JButton btn)
    {
        m_btnReg = btn;
    }

    public JPopupMenu getMenu()
    {
        return m_popMenu;
    }

    public JList getList()
    {
        return m_list;
    }

}

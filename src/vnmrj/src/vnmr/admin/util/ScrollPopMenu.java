/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.event.*;

import vnmr.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class ScrollPopMenu implements ListSelectionListener,
						PopupMenuListener, AdjustmentListener
{

    protected JList m_list;
    protected JScrollPane m_spList;
    protected JPopupMenu m_popMenu;
    protected boolean m_bScrolling =false;
    protected String m_strSelection;

    public ScrollPopMenu(Object[] items)
    {
        m_list = new JList(items);
        m_spList = new JScrollPane(m_list);
    }

    public void makePopMenuScroll(JPopupMenu popMenu)
    {
	m_popMenu = popMenu;
	m_list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        m_list.addListSelectionListener(this);
	m_spList.getVerticalScrollBar().addAdjustmentListener(this);
	popMenu.addPopupMenuListener(this);
	popMenu.add(m_spList);
    }

    public void valueChanged(ListSelectionEvent e)
    {
	int nSelected = m_list.getMaxSelectionIndex();
        m_strSelection = (String)(m_list.getModel().getElementAt(nSelected));
        m_bScrolling = false;
        doAction(m_strSelection);
	m_popMenu.setVisible(false);

    }

    public void doAction(String strSelection){}

    public void popupMenuCanceled(PopupMenuEvent e){}
    public void popupMenuWillBecomeInvisible(PopupMenuEvent e)
    {
	if(!m_bScrolling)
        {
	    Util.setMenuUp(false);
            m_bScrolling = false;
        }
        else
        {
	    Util.setMenuUp(true);
            m_bScrolling = false;
        }
    }

    public void popupMenuWillBecomeVisible(PopupMenuEvent e)
    {
	Util.setMenuUp(true);
    }

    public void adjustmentValueChanged(AdjustmentEvent e)
    {
	m_bScrolling = true;
    }
}

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
import java.beans.*;
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.table.*;
import javax.swing.text.*;
import vnmr.ui.*;

/**
 * Message output
 */
public class MessageTool extends JScrollPane implements ChangeListener, PropertyChangeListener
{ /** session share */
    private SessionShare m_sshare;
    private AppIF m_appIf;
    private int  m_nVInc;
    private Point m_point;
    protected JTable m_table = null;
    private JTextArea textArea = null;
    private JScrollBar vScrollBar = null;
    private boolean bEndReturn = false;
    private Runnable scrollUpdater;


    /**
     * constructor
     */

    public MessageTool(SessionShare sshare, AppIF ep) {
        this.m_sshare = sshare;
        this.m_appIf = ep;

        textArea = new JTextArea();
        setViewportView(textArea);
        textArea.setEditable(false);
        textArea.setLineWrap(true);
        // textArea.setWrapStyleWord(true);

       /********************************
        DefaultTableModel tableModel = new DefaultTableModel(0, 1);
        m_table = new JTable(tableModel);
        m_table.setShowGrid(false);
        m_table.setTableHeader(null);
        m_table.setDefaultEditor(m_table.getColumnClass(0), null);
        setViewportView(m_table);
        setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
        setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        m_nVInc = this.getVerticalScrollBar().getUnitIncrement(SwingConstants.VERTICAL);
        tableModel.addTableModelListener(new TableModelListener()
        {
            public void tableChanged(TableModelEvent e)
            {
                doAction(e);
            }
        });
       **************************/

        setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        vScrollBar = getVerticalScrollBar();
        getViewport().addChangeListener( this );
        DisplayOptions.addChangeListener(this);
        scrollUpdater = new Runnable() {
           public void run() {
                  updateScrollBar();
           }
        };
        setUiColor();
    } // MessageTool

    private void setUiColor() {
        /**********
        Color c = DisplayOptions.getVJColor("VJBackground");
        if (c != null) {
            textArea.setBackground(c);
            setBackground(c);
        }
        c = DisplayOptions.getVJColor("VJTextFG");
        if (c != null) {
            textArea.setForeground(c);
        }
        **********/
    }

    public void propertyChange(PropertyChangeEvent evt)
    {
        if (DisplayOptions.isUpdateUIEvent(evt))
            setUiColor();
    }

    public void insert(String str) {
    }

    private void updateScrollBar() {
        // Dimension dim = textArea.getPreferredScrollableViewportSize();
        // vScrollBar.setValue(dim.height);
        int vMax = vScrollBar.getMaximum();
        vScrollBar.setValue(vMax);
    }

    public void append(String str) {
        int rows;
        if (str == null)
            return;
        if (textArea != null) {
            if (bEndReturn) {
                textArea.append("\n");
                bEndReturn = false;
            }
            // the last line should not be an empty line. 
            rows = str.length() - 1;
            if (rows > 0 && str.endsWith("\n")) {
                str = str.substring(0, rows);
                bEndReturn = true;
            }
            textArea.append(str);
            if (vScrollBar == null || !vScrollBar.isVisible())
                return;
	    rows = textArea.getLineCount();
            if (rows >= Util.getStatusMessagesNumber()) {
                int offset = 1;
                if (rows > 60) {
                    offset = 6;
                    if (rows > 120)
                        offset = 12; 
                }
                try {
                    offset = textArea.getLineStartOffset(offset);
                }
                catch(Exception e) { offset = 0; }
                if (offset > 0) {
                    textArea.replaceRange("", 0, offset);
                }
            }
            if (rows > 1)
                SwingUtilities.invokeLater(scrollUpdater);
            return;
        }
        DefaultTableModel model = (DefaultTableModel)m_table.getModel();
        rows = model.getRowCount();
        if (rows >= Util.getStatusMessagesNumber())
            model.removeRow(0);
        String[] aStrLine = {str};
        model.addRow(aStrLine);
    }

    protected void doAction(TableModelEvent e)
    {
        if (e.getType() == TableModelEvent.INSERT)
        {
            int row = m_table.getModel().getRowCount() - 1;
            if (row < 0)
                return;

            //m_table.scrollRectToVisible(rectangle);

            getViewport().invalidate();
            getViewport().validate();
            getVerticalScrollBar().setValue(getVerticalScrollBar().getMaximum());
        }
    }

    public void showBottomLine() {
        getViewport().invalidate();
        getViewport().validate();
        getVerticalScrollBar().setValue(getVerticalScrollBar().getMaximum());

        /*Point pt = getViewport().getViewPosition();
        Rectangle  rect = getBounds();
        Rectangle  rect2 = m_taMessageTool.getBounds();
        if (rect2.height > rect.height) {
            pt.y = rect2.height - rect.height;
            getViewport().setViewPosition(pt);
        }*/
    }

    public void pushupMessages( int scrolled )
    {
	showBottomLine();
/*
	int x = point.x;
	int y = point.y + scrolled;
	point = new Point( x, y );
	this.getViewport().setViewPosition( point );
*/
    }

    public void stateChanged( ChangeEvent e )
    {
        if( e.getSource() == this.getViewport() )
        {
           m_point = this.getViewport().getViewPosition();
        }
    }

    public void moveUp() {
    }

    public void moveDown() {
    }

    public void clear() {
        if (textArea != null) {
            textArea.setText("");
            return;
        }
        DefaultTableModel model = (DefaultTableModel)m_table.getModel();
        while(model.getRowCount() > 0)
        {
            model.removeRow(0);
        }
    }

    public int getRowHeight() {
        FontMetrics metrics;
        if (textArea != null)
           metrics = textArea.getFontMetrics(textArea.getFont());
        else
           metrics = getFontMetrics(m_table.getFont());
        int rowHeight = metrics.getHeight();
        return rowHeight;
    }

    public int getColumnWidth() {
        FontMetrics metrics;
        if (textArea != null)
           metrics = textArea.getFontMetrics(textArea.getFont());
        else
           metrics = getFontMetrics(m_table.getFont());
        int cw = metrics.charWidth('m');
        return cw;
    }

} // class MessageTool


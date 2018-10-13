/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.beans.*;
import javax.swing.*;
import javax.swing.border.*;

import vnmr.bo.*;
import vnmr.ui.*;
import vnmr.util.*;

/**
 * This is the container for shuffler components.
 *
 */
public class Shuffler extends JPanel{
    static private boolean shufflerInUse = false;

    /** static final string definitions MUST be keep in sync with db_manager.h*/

    // ==== instance variables
    /** spotter button */
    private SpotterButton spotterButton;
    /** statement view */
    private StatementView statementView;
    /** result scroll pane */
    private JScrollPane resultScrollPane;
    /** ResultTable in this scroll pane */
    static private ResultTable resultTable;
    /** shuffler toolbar */
    public ShufflerToolBar shufflerToolBar;

    /**
     * constructor
     * @param sshare session share
     */
    public Shuffler(SessionShare sshare) {
        // We can have only one Locator at a time, keep track of it.
        shufflerInUse = true;
        
        setLayout(new ShufflerLayout());

        setOpaque(true);

        statementView = new StatementView(sshare);
        add(statementView);

        resultTable = new ResultTable(sshare);
        spotterButton = new SpotterButton(sshare);
        add(spotterButton);
        // Put into sshare for access to update the spotter menu
        sshare.setSpotterButton(spotterButton);


        shufflerToolBar = new ShufflerToolBar(sshare);
        add(shufflerToolBar);

        resultScrollPane = new JScrollPane(resultTable
           /* ,
                         JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
                         JScrollPane.HORIZONTAL_SCROLLBAR_NEVER
           */
                                                         );

        // Border for this JPanel, should be same for resultScrollPane
        setBorder(BorderFactory.createEtchedBorder(BevelBorder.LOWERED));
        resultScrollPane.setBorder(BorderFactory.createEtchedBorder(
                                                    BevelBorder.LOWERED));
        add(resultScrollPane);

        // This is done to cause MountPaths to init itself.
        // This needs to be done before the first locator shuffle.
        MountPaths.getusingNetServer();

        // put a new, default statement into the history buffer if it
        // is empty.
        StatementHistory history = sshare.statementHistory();
        if(history!=null){       	
	        if(history.getNumInHistory() <= 0) {
	            String statementType;
	            statementType = sshare.shufflerService().getDefaultStatementType();
	            sshare.statementHistory().appendLastOfType(statementType);
	        }
	        else {
	            history.updateWithoutNewHistory();
	        }
        }

        Util.setShuffler(this);
        //DisplayOptions.addChangeListener(this);

    } // Shuffler()

    static public ResultTable getresultTable() {
        return resultTable;
    }

    public void setWaitCursor() {
        setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));
    }

    public void setDefaultCursor() {
        setCursor(Cursor.getDefaultCursor());
    }

    static public boolean shufflerInUse() {
        return shufflerInUse;
    }

    /**
     * Layout manager for shuffler. A special layout is needed because
     * there's no good standard way of accounting for the preferred size
     * of the StatementView component.
     */
    class ShufflerLayout implements LayoutManager {

        public void addLayoutComponent(String name, Component comp) {}

        public void removeLayoutComponent(Component comp) {}

        /**
         * calculate the preferred size
         * @param parent component to be laid out
         * @see #minimumLayoutSize
         */
        public Dimension preferredLayoutSize(Container parent) {
            return new Dimension(0, 0); // unused
        } // preferredLayoutSize()

        /**
         * calculate the minimum size
         * @param parent component to be laid out
         * @see #preferredLayoutSize
         */
        public Dimension minimumLayoutSize(Container parent) {
            return new Dimension(0, 0); // unused
        } // minimumLayoutSize()

        /**
         * do the layout
         * @param parent component to be laid out
         */
        public void layoutContainer(Container parent) {
            /* Algorithm is as follows:
             *   - let spotterButton take preferred size
             *   - let statementView take preferred height, relative to
             *     a fixed width!
             *   - let shufflerToolBar take preferred height
             *   - give resultScrollPane all of remaining height
             */
            synchronized (parent.getTreeLock()) {
                Dimension parentSize = parent.getSize();
                Insets insets = parent.getInsets();
                int usableWidth =
                    parentSize.width - insets.left - insets.right;
                int usableHeight =
                    parentSize.width - insets.left - insets.right;

// To remove SpotterButton, set this to 0,0
                Dimension spotterSize = spotterButton.getPreferredSize();

                // horizontal partitions
                int h0 = insets.left;
                int h1 = h0 + spotterSize.width;
// To remove SpotterButton, stop subtracting insets.right
                int h2 = parentSize.width - insets.right;

                // vertical partitions
                int v0 = insets.top;
                int v1 = v0 + spotterSize.height;
                int v2 = v0 + statementView.
                    getPreferredHeight(usableWidth - spotterSize.width);
                int v3 = Math.max(v1, v2);
                int v4 = v3 + shufflerToolBar.getPreferredSize().height;
                int v5 = parentSize.height - insets.bottom;

                spotterButton.
                    setBounds(new Rectangle(h0, v0, h1 - h0, v1 - v0));
                statementView.
                    setBounds(new Rectangle(h1, v0, h2 - h1, v2 - v0));
                shufflerToolBar.
                    setBounds(new Rectangle(h0, v3, h2 - h0, v4 - v3));
                resultScrollPane.
                    setBounds(new Rectangle(h0, v4, h2 - h0, v5 - v4));
            }
        } // layoutContainer()

    } // class ShufflerLayout

} // class Shuffler

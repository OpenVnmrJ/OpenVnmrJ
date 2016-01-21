/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui.shuf;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.tree.*;
import javax.swing.table.*;

import java.awt.*;
import java.awt.Dimension;
import java.awt.Component;
import java.awt.Graphics;
import java.awt.Rectangle;

import java.awt.event.MouseEvent;

import java.util.*;

import vnmr.util.*;

public class JTreeTableImport extends JTable {
    /** A subclass of JTree. */
    protected TreeTableCellRenderer tree;
    protected MyTreeRenderer treeRenderer;
    public ArrayList selectedfiles = new ArrayList();
    public ArrayList dirs;   // Directory in the form host:fullpath

    public JTreeTableImport() {
        super();

        treeRenderer = new MyTreeRenderer();
    }

    public void setTreeTableModel(TreeTableModel treeTableModel, ArrayList dirs) {

        this.dirs = dirs;

        // Create the tree. It will be used as a renderer and editor.
        tree = new TreeTableCellRenderer(treeTableModel);

        // Install a tableModel representing the visible rows in the tree.
        super.setModel(new TreeTableModelAdapter(treeTableModel, tree));

        // Force the JTable and JTree to share their row selection models.
        ListToTreeSelectionModelWrapper selectionWrapper =
                        new ListToTreeSelectionModelWrapper();

        tree.setSelectionModel(selectionWrapper);
        ListSelectionModel lsm;
        lsm = selectionWrapper.getListSelectionModel();
        setSelectionModel(lsm);

        // Install the tree editor renderer and editor.
        setDefaultRenderer(TreeTableModel.class, tree);
        setDefaultEditor(TreeTableModel.class, new TreeTableCellEditor());

        // No grid.
        setShowGrid(false);

        // No intercell spacing
        setIntercellSpacing(new Dimension(0, 0));

        // And update the height of the trees row to match that of
        // the table.
        if (tree.getRowHeight() < 1) {
            // Metal looks better like this.
            setRowHeight(18);
        }

        getColumnModel().getColumn(0).setMinWidth(260);
        getColumnModel().getColumn(0).setPreferredWidth(280);
        getColumnModel().getColumn(1).setPreferredWidth(60);
        getColumnModel().getColumn(2).setPreferredWidth(100);
        sizeColumnsToFit(0);

    }

    public void setTreeTableFont(Font f) {
        setFont(f);
        treeRenderer.setFont(f);
    }

    public ArrayList getSelectedFiles() {
            return selectedfiles;
    }

    /**
     * Overridden to message super and forward the method to the tree.
     * Since the tree is not actually in the component hieachy it will
     * never receive this unless we forward it in this manner.
     */
    public void updateUI() {
        super.updateUI();
        if(tree != null) {
            tree.updateUI();
        }
        // Use the tree's default foreground and background colors in the
        // table.
// This messes up display in different Themes.  Without these lines, the tree
// display does not update properly on the fly when the theme is changed.  However
// it is correct when vnmrj is re-run.  With this line it is wrong all of the time
// for dark themes.
//        LookAndFeel.installColorsAndFont(this, "Tree.background",
//                                         "Tree.foreground", "Tree.font");
    }

    /* Workaround for BasicTableUI anomaly. Make sure the UI never tries to
     * paint the editor. The UI currently uses different techniques to
     * paint the renderers and editors and overriding setBounds() below
     * is not the right thing to do for an editor. Returning -1 for the
     * editing row in this case, ensures the editor is never painted.
     */
    public int getEditingRow() {
        return (getColumnClass(editingColumn) == TreeTableModel.class) ? -1 :
                editingRow;
    }

    /**
     * Overridden to pass the new rowHeight to the tree.
     */
    public void setRowHeight(int rowHeight) {
        super.setRowHeight(rowHeight);
        if (tree != null && tree.getRowHeight() != rowHeight) {
            tree.setRowHeight(getRowHeight());
        }
    }

    /**
     * Returns the tree that is being shared between the model.
     */
    public JTree getTree() {
        return tree;
    }

    class MyTreeRenderer extends DefaultTreeCellRenderer {

        public MyTreeRenderer() {
            super();
        }

        public Component getTreeCellRendererComponent(
                        JTree tree,
                        Object value,
                        boolean sel,
                        boolean expanded,
                        boolean leaf,
                        int row,
                        boolean hasFocus) {

            super.getTreeCellRendererComponent(
                                    tree, value, sel,
                                    expanded, leaf, row,
                                    hasFocus);

            FileNode fn = (FileNode)value;
            String path;
            if(fn.isNew) {
                fn.isNew = false;
                path = fn.getPath();
                // If necessary, convert this path to unix style
                // On windows, the path from FileNode a bit screwy, it looks
                // like a unix path, but with backslashes.  Convert backslashes
                // to forward slashes.
                path = UtilB.windowsPathToUnix(path);

                // Be sure to keep path and dirs in the same form
                String hostFullpath = MountPaths.getCanonicalHostFullpath(path);
                if(isLocatorDir(hostFullpath, dirs))
                    fn.isExist = true;
            }

            if(fn.isExist) {
//              if (!leaf)
//                    setIcon(Util.getImageIcon("open.gif"));
//              else
                    setIcon(Util.getImageIcon("middle.gif"));
            }

            return this;
        }
    }


    // This is sometimes called with both path and dirs in the form of
    // host:fullpath, and sometimes with both in the form of just fullpath.
    // The important thing is that both be in the same form.
    public boolean isLocatorDir(String path, ArrayList dirs) {

        if(path.equals("/"))
            return false;

        if(isFileLocatorDir(path, dirs))
            return true;

        UNFile f = new UNFile(path);
        String[] files = f.list();

        if(files == null || files.length == 0)
            return false;

        if(path.endsWith("/")) path = path.substring(0,path.length()-2);

        // if all files under this path is in the Locator return true;
        for(int i=0; i<files.length; i++) {
            if(!isFileLocatorDir(path +"/"+ files[i], dirs))
                return false;
        }

        return true;
    }

    public boolean isFileLocatorDir(String path, ArrayList dirs) {
            for(int i=0; i<dirs.size(); i++) {
                String dirPath = (String)dirs.get(i);
              if (path.startsWith((String)dirs.get(i)))
                return true;
            }

            return false;
    }

    /**
     * A TreeCellRenderer that displays a JTree.
     */
    public class TreeTableCellRenderer extends JTree implements
                 TableCellRenderer {
        /** Last table/tree row asked to renderer. */
        protected int visibleRow;

        public TreeTableCellRenderer(TreeModel model) {
            super(model);
            setCellRenderer(treeRenderer);
        }

        /**
         * updateUI is overridden to set the colors of the Tree's renderer
         * to match that of the table.
         */
        public void updateUI() {
            super.updateUI();
            // Make the tree's cell renderer use the table's cell selection
            // colors.
            TreeCellRenderer tcr = getCellRenderer();
            if (tcr instanceof DefaultTreeCellRenderer) {
                DefaultTreeCellRenderer dtcr = ((DefaultTreeCellRenderer)tcr);
                // For 1.1 uncomment this, 1.2 has a bug that will cause an
                // exception to be thrown if the border selection color is
                // null.
                // dtcr.setBorderSelectionColor(null);
                dtcr.setTextSelectionColor(UIManager.getColor
                                           ("Table.selectionForeground"));
                dtcr.setBackgroundSelectionColor(UIManager.getColor
                                                ("Table.selectionBackground"));
            }
        }

        /**
         * Sets the row height of the tree, and forwards the row height to
         * the table.
         */
        public void setRowHeight(int rowHeight) {
            if (rowHeight > 0) {
                super.setRowHeight(rowHeight);
                if (JTreeTableImport.this != null &&
                    JTreeTableImport.this.getRowHeight() != rowHeight) {
                    JTreeTableImport.this.setRowHeight(getRowHeight());
                }
            }
        }

        /**
         * This is overridden to set the height to match that of the JTable.
         */
        public void setBounds(int x, int y, int w, int h) {
            super.setBounds(x, 0, w, JTreeTableImport.this.getHeight());
        }

        /**
         * Sublcassed to translate the graphics such that the last visible
         * row will be drawn at 0,0.
         */
        public void paint(Graphics g) {
            g.translate(0, -visibleRow * getRowHeight());
            super.paint(g);
        }

        /**
         * TreeCellRenderer method. Overridden to update the visible row.
         */

        public Component getTableCellRendererComponent(JTable table,
                                                       Object value,
                                                       boolean isSelected,
                                                       boolean hasFocus,
                                                       int row, int column) {

          if(isSelected)
            setBackground(table.getSelectionBackground());
          else
            setBackground(table.getBackground());

          visibleRow = row;

          return this;
        }
    }

    /**
     * TreeTableCellEditor implementation. Component returned is the
     * JTree.
     */
    public class TreeTableCellEditor extends AbstractCellEditor implements
                 TableCellEditor {
        public Component getTableCellEditorComponent(JTable table,
                                                     Object value,
                                                     boolean isSelected,
                                                     int r, int c) {
            return tree;
        }

        /**
         * Overridden to return false, and if the event is a mouse event
         * it is forwarded to the tree.<p>
         * The behavior for this is debatable, and should really be offered
         * as a property. By returning false, all keyboard actions are
         * implemented in terms of the table. By returning true, the
         * tree would get a chance to do something with the keyboard
         * events. For the most part this is ok. But for certain keys,
         * such as left/right, the tree will expand/collapse where as
         * the table focus should really move to a different column. Page
         * up/down should also be implemented in terms of the table.
         * By returning false this also has the added benefit that clicking
         * outside of the bounds of the tree node, but still in the tree
         * column will select the row, whereas if this returned true
         * that wouldn't be the case.
         * <p>By returning false we are also enforcing the policy that
         * the tree will never be editable (at least by a key sequence).
         */
        public boolean isCellEditable(EventObject e) {
            if (e instanceof MouseEvent) {
                for (int counter = getColumnCount() - 1; counter >= 0;
                     counter--) {
                    if (getColumnClass(counter) == TreeTableModel.class) {
                        MouseEvent me = (MouseEvent)e;
                        MouseEvent newME = new MouseEvent(tree, me.getID(),
                                   me.getWhen(), me.getModifiers(),
                                   me.getX() - getCellRect(0, counter, true).x,
                                   me.getY(), me.getClickCount(),
                                   me.isPopupTrigger());
                        tree.dispatchEvent(newME);
                        break;
                    }
                }
            }
            return false;
        }
    }


    /**
     * ListToTreeSelectionModelWrapper extends DefaultTreeSelectionModel
     * to listen for changes in the ListSelectionModel it maintains. Once
     * a change in the ListSelectionModel happens, the paths are updated
     * in the DefaultTreeSelectionModel.
     */
    class ListToTreeSelectionModelWrapper extends DefaultTreeSelectionModel {
        /** Set to true when we are updating the ListSelectionModel. */
        protected boolean updatingListSelectionModel = false;

        public ListToTreeSelectionModelWrapper() {
            super();
            setSelectionMode(DISCONTIGUOUS_TREE_SELECTION);
            getListSelectionModel().addListSelectionListener
                                    (createListSelectionListener());
        }

        /**
         * Returns the list selection model. ListToTreeSelectionModelWrapper
         * listens for changes to this model and updates the selected paths
         * accordingly.
         */
        ListSelectionModel getListSelectionModel() {
            // I am making a local copy and returning it to get around
            // some strange bug with dasho
            ListSelectionModel lsm = listSelectionModel;
            return lsm;
        }

        /**
         * This is overridden to set <code>updatingListSelectionModel</code>
         * and message super. This is the only place DefaultTreeSelectionModel
         * alters the ListSelectionModel.
         */

        public void resetRowSelection() {
            if(!updatingListSelectionModel) {
                updatingListSelectionModel = true;
                try {
                    //super.resetRowSelection();
        //super.resetRowSelection() resets only table selection,
        //but tree selection didn't get reset.
                    resetRowSelection();
                }
                finally {
                    updatingListSelectionModel = false;
                }
            }
            // Notice how we don't message super if
            // updatingListSelectionModel is true. If
            // updatingListSelectionModel is true, it implies the
            // ListSelectionModel has already been updated and the
            // paths are the only thing that needs to be updated.
        }

        /**
         * Creates and returns an instance of ListSelectionHandler.
         */
        protected ListSelectionListener createListSelectionListener() {
            return new ListSelectionHandler();
        }

        /**
         * If <code>updatingListSelectionModel</code> is false, this will
         * reset the selected paths from the selected rows in the list
         * selection model.
         */
        protected ArrayList updateSelectedPathsFromSelectedRows() {
            ArrayList selectedFiles = new ArrayList();
            if(!updatingListSelectionModel) {
                updatingListSelectionModel = true;
                try {
                    // This is way expensive, ListSelectionModel needs an
                    // enumerator for iterating.
                    int        min = listSelectionModel.getMinSelectionIndex();
                    int        max = listSelectionModel.getMaxSelectionIndex();

                    clearSelection();
                    if(min != -1 && max != -1) {
                        for(int counter = min; counter <= max; counter++) {
                            if(listSelectionModel.isSelectedIndex(counter)) {
                                TreePath selPath = tree.getPathForRow(counter);

                                if(selPath != null) {
                                    addSelectionPath(selPath);

                                int np = selPath.getPathCount();
                                FileNode fn = (FileNode) selPath.getPathComponent(np-1);
                                String path = fn.getPath();

                                if(!fn.isLeaf()) path = path + "/";
                                selectedFiles.add(path);

                                }
                            }
                        }
                    }
                }
                finally {
                    updatingListSelectionModel = false;
                }
            }
            return selectedFiles;
        }

        /**
         * Class responsible for calling updateSelectedPathsFromSelectedRows
         * when the selection of the list changse.
         */
        class ListSelectionHandler implements ListSelectionListener {
            public void valueChanged(ListSelectionEvent e) {
                selectedfiles = updateSelectedPathsFromSelectedRows();
            }
        }
    }

}

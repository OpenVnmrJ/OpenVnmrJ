/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Modification of GridLayout.java where it fills down the columns
   first and then into the next column.  The constructors are simply
   passing the args through.  The only thing really changed is the
   layoutContainer() method.
*/


package  vnmr.util;

import java.awt.*;

public class GridLayoutCol extends GridLayout {

    public GridLayoutCol() {
	this(1, 0, 0, 0);
    }

    /**
     * Creates a grid layout with the specified number of rows and 
     * columns. All components in the layout are given equal size. 
     * <p>
     * One, but not both, of <code>rows</code> and <code>cols</code> can 
     * be zero, which means that any number of objects can be placed in a 
     * row or in a column. 
     * @param     rows   the rows, with the value zero meaning 
     *                   any number of rows.
     * @param     cols   the columns, with the value zero meaning 
     *                   any number of columns.
     */
    public GridLayoutCol(int rows, int cols) {
	this(rows, cols, 0, 0);
    }

    /**
     * Creates a grid layout with the specified number of rows and 
     * columns. All components in the layout are given equal size. 
     * <p>
     * In addition, the horizontal and vertical gaps are set to the 
     * specified values. Horizontal gaps are placed at the left and 
     * right edges, and between each of the columns. Vertical gaps are 
     * placed at the top and bottom edges, and between each of the rows. 
     * <p>
     * One, but not both, of <code>rows</code> and <code>cols</code> can 
     * be zero, which means that any number of objects can be placed in a 
     * row or in a column. 
     * @param     rows   the rows, with the value zero meaning 
     *                   any number of rows.
     * @param     cols   the columns, with the value zero meaning 
     *                   any number of columns.
     * @param     hgap   the horizontal gap. 
     * @param     vgap   the vertical gap. 
     * @exception   IllegalArgumentException  if the of <code>rows</code> 
     *                   or <code>cols</code> is invalid.
     */
    public GridLayoutCol(int rows, int cols, int hgap, int vgap) {
	super(rows, cols, hgap, vgap);

    }

    /** 
     * Lays out the specified container using this layout. 
     * <p>
     * This method reshapes the components in the specified target 
     * container in order to satisfy the constraints of the 
     * <code>GridLayoutCol</code> object. 
     * <p>
     * The grid layout manager determines the size of individual 
     * components by dividing the free space in the container into 
     * equal-sized portions according to the number of rows and columns 
     * in the layout. The container's free space equals the container's 
     * size minus any insets and any specified horizontal or vertical 
     * gap. All components in a grid layout are given the same size. 
     *  
     * @param      target   the container in which to do the layout.
     * @see        java.awt.Container
     * @see        java.awt.Container#doLayout
     */
    public void layoutContainer(Container parent) {
      synchronized (parent.getTreeLock()) {
	Insets insets = parent.getInsets();
	int ncomponents = parent.getComponentCount();
	int nrows=getRows();
	int ncols=getColumns();
	int hgap=getHgap();
	int vgap=getVgap();

	if (ncomponents == 0) {
	    return;
	}


	if (nrows > 0) {
	    ncols = (ncomponents + nrows - 1) / nrows;
	} else {
	    nrows = (ncomponents + ncols - 1) / ncols;
	}
	/******* The following  2 rows of code have been changed
		 when converting GridLayout to the column based
		 GridLayoutCol.  I did this to get the compile to work.
		 They were:
		   int w = parent.width - (insets.left + insets.right);
		   int h = parent.height - (insets.top + insets.bottom);
	***********/
	int w = parent.getPreferredSize().width - (insets.left + insets.right);
	int h = parent.getPreferredSize().height - (insets.top + insets.bottom);

	w = (w - (ncols - 1) * hgap) / ncols;
	h = (h - (nrows - 1) * vgap) / nrows;

	for (int c = 0, x = insets.left ; c < ncols ; c++, x += w + hgap) {
	    for (int r = 0, y = insets.top ; r < nrows ; r++, y += h + vgap) {
		/******* The following row of code has been changed
			 when converting GridLayout to the column based
			 GridLayoutCol.  It was "int i = r * ncols + c;"
			 GRS 1/21/2000
		**********/
		int i = c * nrows + r;
		if (i < ncomponents) {
		    parent.getComponent(i).setBounds(x, y, w, h);
		}
	    }
	}
      }
    }
}

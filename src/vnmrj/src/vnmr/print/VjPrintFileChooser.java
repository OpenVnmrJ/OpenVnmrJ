/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.io.File;
import javax.swing.JFileChooser;


public class VjPrintFileChooser extends JFileChooser {

    public VjPrintFileChooser() {
         super();
    }

    public VjPrintFileChooser(String path) {
         super(path);
    }

    public VjPrintFileChooser(File fd) {
         super(fd);
    }

    /*****
    public void approveSelection() {
            File selected = getSelectedFile();
            boolean exists;

            try {
                exists = selected.exists();
            } catch (SecurityException e) {
                exists = false;
            }

            if (exists) {
                int val;
                val = JOptionPane.showConfirmDialog(this,
                       ServiceDialog.getMsg("dialog.overwrite"),
                       ServiceDialog.getMsg("dialog.owtitle"),
                       JOptionPane.YES_NO_OPTION);
                if (val != JOptionPane.YES_OPTION) {
                    return;
                }
            }

            try {
                if (selected.createNewFile()) {
                    selected.delete();
                }
            }  catch (IOException ioe) {
                JOptionPane.showMessageDialog(this,
                       ServiceDialog.getMsg("dialog.writeerror")+" "+selected,
                       ServiceDialog.getMsg("dialog.owtitle"),
                       JOptionPane.WARNING_MESSAGE);
                return;
            } catch (SecurityException se) {
                //There is already file read/write access so at this point
                // only delete access is denied.  Just ignore it because in
                // most cases the file created in createNewFile gets
                // overwritten anyway.
            }
            File pFile = selected.getParentFile();
            if ((selected.exists() &&
                     (!selected.isFile() || !selected.canWrite())) ||
                     ((pFile != null) &&
                     (!pFile.exists() || (pFile.exists() && !pFile.canWrite())))) {
                JOptionPane.showMessageDialog(this,
                         ServiceDialog.getMsg("dialog.writeerror")+" "+selected,
                         ServiceDialog.getMsg("dialog.owtitle"),
                         JOptionPane.WARNING_MESSAGE);
                return;
            }

            super.approveSelection();
    }
   ***********/

} // endof VnmrFileChooser


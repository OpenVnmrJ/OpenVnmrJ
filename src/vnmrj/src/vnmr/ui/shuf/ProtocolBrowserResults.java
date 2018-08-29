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
import javax.swing.table.TableCellRenderer;

import java.awt.*;
import java.awt.event.*;
import java.util.*;

import vnmr.bo.SearchResults;
import vnmr.ui.*;
import vnmr.util.DisplayOptions;
import vnmr.util.Util;


/********************************************************** <pre>
 * Summary: Protocol Browser Results panel.  Shows list of matching protocols
 * and columns of keyword values.  This is to be added to the tabbed pane
 * in the ProtocolBrowser.
 *
 </pre> **********************************************************/

public class ProtocolBrowserResults extends JScrollPane {
    private ProtocolBrowserTable resultTable = null;
    private String objType;
    
    // Allow calling without fullpath defined.  Call constructor with fullpath=null
    public ProtocolBrowserResults(ProtocolKeywords keywords, String objType) {
        this(keywords, objType, null);
    }
    
    // We need to create a JPanel to put into the JScrollPane and then
    // put two panels into that top level JPanel.
    public ProtocolBrowserResults(ProtocolKeywords keywords, String objType, 
                                    String fullpath) {
        boolean studyCard = false;
        this.objType = objType;
        
        if(fullpath != null && fullpath.length() > 0)
            studyCard = true;
        
        resultTable = createPanel(keywords, fullpath);       

        // Set the result panel into this scroll pane
        this.setViewportView(resultTable);
        
        if(studyCard)
            this.setName(Util.getLabel("Study Card", "Study Card"));
        else if(objType.equals(Shuf.DB_PROTOCOL))
            this.setName(Util.getLabel("Protocols", "Protocols"));
        else
            this.setName(Util.getLabel("Data", "Data"));

    }
    


    // Do the actual panel creation and filling.
    private ProtocolBrowserTable createPanel(ProtocolKeywords keywords, String fullpath) {
        SessionShare sshare;
        sshare = Util.getSessionShare();
        ProtocolBrowserTable protocolTable = new ProtocolBrowserTable(sshare, keywords, 
                objType, fullpath) {
            // Set every other row to a little color (alternate row background)
            // Set all selected rows to the selection color
            public Component prepareRenderer(TableCellRenderer renderer, int rowIndex, int vColIndex) {
                Component c = super.prepareRenderer(renderer, rowIndex, vColIndex);
// Changing background colors screws up DARK themes
                // if (isCellSelected(rowIndex, vColIndex)) {
                //     c.setBackground(getSelectionBackground());
                // } 
                // else if (rowIndex % 2 == 0) {
                //     Color clr = DisplayOptions.getColor("beige");
                //     c.setBackground(clr);
                // } 
                // else {
                //     c.setBackground(getBackground());
                // }

                return c;
            }
        };

        return protocolTable;
    }
    
    public void updateProtocolBrowser(SearchResults[] results, 
                                      ProtocolKeywords keywords) {
        ProtocolBrowserDataModel dataModel = resultTable.getdataModel();
        dataModel.setResults(results);
                
    }

}

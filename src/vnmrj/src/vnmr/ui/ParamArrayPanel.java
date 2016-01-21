/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import javax.swing.event.*;
import javax.swing.*;
import javax.swing.border.*;
import javax.swing.table.*;
import javax.swing.JTable.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.beans.*;
import java.io.*;

import vnmr.util.*;
import vnmr.bo.*;

/********************************************************** <pre>
 * Summary: Panel for setting up Parameter Arrays for Acquisition.
 *
 *      There is only one of these.  The actual instantitation of
 *      this class, is contained within the class as a static
 *      member.  Users needing this object, can call the static method
 *      ParamArrayPanel.getParamArrayPanel() to get it.  If it has
 *      not been created yet, it will be created at that time, and then the
 *      object returned.  It could be instantitated at startup, but that just
 *      makes it take longer, so this way it takes longer the first
 *      time this obj is needed.
 *
 *      The interaction with vnmrbg depends on the following vnmrbg
 *      behavior:
 *
 *      When the value of a parameter is changed, vnmrbg sends pnew
 *      and the parameter name.
 *      If arrayed values are assigned to a parameter, or a single value
 *      is assigned to an arrayed parameter, vnmrbg sends also "array"
 *      as pnew parameter. The panel is updated from ExpPanel by catching
 *      pnew parameters. the update methods (requestUpdateArrayParamTable
 *      and requestUpdateValueTable) are seldom called directly.
 *
 *      If this behavior is changed, the code has to be changed to
 *      directly call the update methods.
 *
 *      Most of user interactions of the ParamArrayPanel result in
 *      assigning arrayed values to a parameter, or assigning a single
 *      to an arrayed parameter. Then panel is updated when array and
 *      paramName are sent back as pnew.
 *
 *      When vnmrbg sends pnew = array, the upPanel is updated
 *      by calling vnmrbgUpdateArrayedParams(ARRAYstr),
 *      which in turn calls updateArrayParamTable(values)
 *
 *      When vnmrbg sends pnew = paramName, the centerPanel is updated
 *      by calling vnmrbgUpdateArrayValues(paramName, avalStr),
 *      which in turn calls updateValueTable(paramName, values, type).
 *
 *      The following methods set array values or single value
 *      for a parameter and cause vnmrbg sends pnew array and paramName:
 *
 *      requestVnmrbgCalArrayValues(paramName)
 *      sendArrayValuesToVnmr(paramName, values)
 *      sendCurrentValueToVnmr(paramName)
 *
 *      Here are all the user interactive widgets and the method they use:
 *
 *      requestVnmrbgCalArrayValues(paramName):
 *
 *              firstText
 *              incText
 *              incStyle
 *              lastText (called through recalFirstOrIncrementWhenLastValueChanged())
 *              size column of paramTable (when size > 1)
 *
 *      sendCurrentValueToVnmr(paramName)
 *
 *              size column of paramTable (when size = 1)
 *              on/off column of paramTable (when off)
 *              UnArrayButton*
 *
 *      sendArrayValuesToVnmr(paramName, values)
 *
 *              on/off column of paramTable (when on)
 *              randomizeButton
 *              valueTableChanged
 *
 *      The methods used by the above widgets update both upPanel
 *      and centerPanel. They don't need to update each other,
 *      i.e., updateArrayParamTable (upPanel) does not cause the
 *      updating of centerPanel unless the activeParam is not "valid".
 *
 *      The following widgets behave slightly different:
 *
 *              NewArrayButton
 *                      add a blank row, assign it the max order.
 *                      and sortByOrder, setRowSelectionInterval.
 *
 *              UnArrayButton
 *                      remove parameter from arrayedParams.
 *                      paramName is no long recognized when
 *                      vnmrbg sends it to pnew, so the center
 *                      Panel is not updated, i.e., replaced
 *                      by an arrayed parameter without special
 *                      care (in updateArrayParamTable, the activeParamName
 *                      is checked, if not an arrayed parameter, replace it.)
 *
 *              paramName column of paramTable
 *                      if a new parameter name, create and initialize an
 *                      ArrayedParam object for it and add it to arrayedParams,
 *                      then call the following methods:
 *                      updateParamTable() (which calls updateArrayParamTable)
 *                      requestUpdateValueTable(paramName)
 *
 *              order column of paramTable
 *                      call sortByOrder then
 *                      sendArrayParamsToVnmr
 *                      No need to update the centerPanel.
 *              currentValue
 *                      set the currentValue to one of the arrayed values
 *                      or any typed value. When a parameter is unarrayed,
 *                      or array mode is turned off, or size is set to 1,
 *                      the parameter is set to the currentValue.
 *                      By default, currentValue is the first value.
 *
 </pre> **********************************************************/

public class ParamArrayPanel extends ModelessDialog implements ActionListener, VObjDef {

    private static ParamArrayPanel paramArrayPanel=null;
    private static ParamArrayTable paramTable=null;
    private static ParamArrayValueTable valueTable=null;
    protected JButton unarrayButton;
    protected JButton newArrayButton;
    protected JLabel sizeLabel;
    protected JScrollPane paramScrollpane=null;
    protected JLabel activeLabel;
    protected JLabel activeValLabel;
    protected JLabel currentLabel;
    protected JTextField currentText;
    protected JLabel arraysizeLabel;
    protected JTextField arraysizeText;
    protected JLabel firstLabel;
    protected JTextField firstText;
    protected JLabel incLabel;
    protected JTextField incText;
    protected JLabel lastLabel;
    protected JTextField lastText;
    protected JLabel timeLabel;
    protected JLabel incStyleValueLabel;
    protected MPopButton incStylePopButton;
    protected JButton randomButton;
    protected JButton readButton;
    protected JScrollPane valueScrollpane=null;
    protected VParamArray vArrayParams;
    protected VParamArray vArrayValues;
    protected VParamArray vsendCmd;
    protected TableModel paramTableModel;
    protected TableModel valueTableModel;
    protected VParamArray vtotalTime;
    protected Vector arrayedParams = null;
    private ButtonIF vnmrIf;
    public static final int MAXLENGTH = 256;
    protected boolean origArrays = true;
    String expStyle = Util.getLabel("_ArrayExponential");
    String linearStyle = Util.getLabel("_Linear");

    /************************************************** <pre>
     * Summary: Constructor to build ParmaArrayPanel.
     *
     *
     </pre> **************************************************/
    public ParamArrayPanel(ButtonIF vnmrIf) {
	super(Util.getLabel("_Array_Parameter"));

	this.vnmrIf = vnmrIf;

	/* contains array parameter table and topPanel1 */
	JPanel topPanel = new JPanel();
	topPanel.setLayout(new BoxLayout(topPanel, BoxLayout.Y_AXIS));

	/* contains textFields Array size, Total time, */
	/* and UnArray and New Array buttons */
	JPanel topPanel1 = new JPanel(new GridLayout(2, 2, 10, 5));

	/* contains centerPanel1, centerPanel2 and centerPanel3 */
	JPanel centerPanel = new JPanel(new BorderLayout());

	/* contains Active Param name and Current Value */
	JPanel centerPanel1 = new JPanel(new GridLayout(1, 4, 10, 5));

	/* contain Array Size, First, Last Value, Increment, Inc Style, Randomize */
	JPanel centerPanel2 = new JPanel(new GridLayout(7, 2, 10, 5));

	/* contains array values (table) of the Active Param */
	JPanel centerPanel3 = new JPanel();
	centerPanel3.setLayout(new BoxLayout(centerPanel3, BoxLayout.X_AXIS));

	// It looks better with a border
	// Use etched border and then blank space inside of that.
	CompoundBorder cb = new CompoundBorder(
	    		BorderFactory.createEtchedBorder(),
			BorderFactory.createEmptyBorder(10,20,10,20));
	topPanel.setBorder(cb);
	topPanel1.setBorder(BorderFactory.createEmptyBorder(0,70,0,70));
	centerPanel1.setBorder(cb);
	centerPanel2.setBorder(BorderFactory.createEmptyBorder(30,0,30,30));
	centerPanel3.setBorder(cb);

	// TOP Panel

	// Add things to sub panels.
    Font font = DisplayOptions.getFont("Dialog", Font.PLAIN, 14);
	sizeLabel = new  JLabel();
	sizeLabel.setFont(font);
	setArraySize(" ");
	topPanel1.add(sizeLabel);

	timeLabel = new JLabel(Util.getLabel("_Total_Time"));
	timeLabel.setFont(font);
	topPanel1.add(timeLabel);
	setTotalTime("1");

	unarrayButton = new  JButton(Util.getLabel("_UnArray"));
	unarrayButton.addActionListener(new UnArrayButtonListener());
	topPanel1.add(unarrayButton);

	newArrayButton = new  JButton(Util.getLabel("_New_Array"));
	newArrayButton.addActionListener(new NewArrayButtonListener());
	topPanel1.add(newArrayButton);

	// Create and initialize paramTable
	initializeParamTable();

	// Add sub panels to a main panel
	topPanel.add(paramScrollpane);
	topPanel.add(topPanel1);


	// CENTER Panel

	// Add things to sub panels.
	// Even though Active Param is not editable, make it two lables
	// so that grid layout works.
	activeLabel = new JLabel(Util.getLabel("_Active_Param"));
	activeLabel.setFont(font);
	centerPanel1.add(activeLabel);
	activeValLabel = new JLabel();
	activeValLabel.setFont(font);
	setActiveParam("phase");
	centerPanel1.add(activeValLabel);

	currentLabel = new JLabel(Util.getLabel("_Current_Value"));
	currentLabel.setFont(font);
        centerPanel1.add(currentLabel);
        currentText = new JTextField();
        setCurrentValue("");
        centerPanel1.add(currentText);

        currentText.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
	      if(currentText.hasFocus()) currentTextAction();
            }
        });

	currentText.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
		currentTextAction();
            }
        });

	arraysizeLabel = new JLabel(Util.getLabel("_Array_Size"));
	arraysizeLabel.setFont(font);
	centerPanel2.add(arraysizeLabel);
	arraysizeText = new JTextField();
	setSizeValue("1");
	centerPanel2.add(arraysizeText);

	arraysizeText.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent e) {
	        if(arraysizeText.hasFocus()) arraysizeTextAction();
	    }
	});

	arraysizeText.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
		arraysizeTextAction();
            }
        });

	firstLabel = new JLabel(Util.getLabel("_First_Value"));
	firstLabel.setFont(font);
	centerPanel2.add(firstLabel);
	firstText = new JTextField();
	setFirstValue("0");
	centerPanel2.add(firstText);

	firstText.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent e) {
	        if(firstText.hasFocus()) firstTextAction();
	    }
	});

	firstText.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
		firstTextAction();
            }
        });

	incLabel = new JLabel(Util.getLabel("_Increment"));
	incLabel.setFont(font);
	centerPanel2.add(incLabel);
	incText = new JTextField();
	setIncrement("0");
	centerPanel2.add(incText);

	incText.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent e) {
	        if(incText.hasFocus()) incTextAction();
	    }
	});

	incText.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
		incTextAction();
            }
        });

	lastLabel = new JLabel(Util.getLabel("_Last_Value"));
	lastLabel.setFont(font);
	centerPanel2.add(lastLabel);
	lastText = new JTextField();
	setLastValue("63");
	centerPanel2.add(lastText);

	lastText.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent e) {
	        if(lastText.hasFocus()) lastTextAction();
	    }
	});

	lastText.addFocusListener(new FocusAdapter() {
            public void focusLost(FocusEvent evt) {
		lastTextAction();
            }
        });

        ArrayList styles = new ArrayList();
        styles.add(linearStyle);
        styles.add(expStyle);
        //styles.add("None");
        incStylePopButton = new MPopButton(styles);
        incStylePopButton.setForeground(Color.black);

        incStylePopButton.setText(Util.getLabel("_Inc_Style"));
        incStylePopButton.addPopListener(new StyleButtonListener());
        centerPanel2.add(incStylePopButton);
        incStyleValueLabel = new JLabel();
	incStyleValueLabel.setFont(font);
        setIncStyle(linearStyle);
        centerPanel2.add(incStyleValueLabel);

	randomButton = new JButton(Util.getLabel("_Randomize"));
	centerPanel2.add(randomButton);

	randomButton.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent e) {

		savePrevArrayedParams();

		String paramName = getActiveParamName();
		randomizeValueList(paramName);
	    }
	});

	// added a blank JLabel so there are 10 instead of 9 widgets so
	// the layout mamager won't force it into 3x3 layout.
	JLabel blank1 = new JLabel();
	centerPanel2.add(blank1);

/*
	readButton = new JButton("Read From List");
readButton.setEnabled(false);
	centerPanel2.add(readButton);
*/
	// Create and initialize valueTable
	initializeValueTable();

	centerPanel3.add(centerPanel2);
	centerPanel3.add(valueScrollpane);

	centerPanel.add(centerPanel1, BorderLayout.NORTH);
	centerPanel.add(centerPanel3, BorderLayout.SOUTH);

	// Add panels to Main Dialog
	getContentPane().add(topPanel, BorderLayout.NORTH);
	getContentPane().add(centerPanel, BorderLayout.CENTER);

	// To make the bottom panel look like the others, get the JPanel
	// and set a border for it like the others.
	JPanel modelessPanel = (JPanel)getContentPane().getComponent(0);
	modelessPanel.setBorder(cb);

	// Set the buttons and the text item up with Listeners
        historyButton.setActionCommand("history");
        historyButton.addActionListener(this);
        undoButton.setActionCommand("undo");
        undoButton.addActionListener(this);
        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("abandon");
        abandonButton.addActionListener(this);
        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);

        setHistoryEnabled(false);
        setUndoEnabled(false);
        setHelpEnabled(false);
        setAbandonEnabled(true);
        setCloseEnabled(true);

        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent we) {
/*
                updateCurpar();
                vsendCmd.setAttribute(CMD, "flush");
                vsendCmd.updateValue();
*/
                setVisible(false);
            }
        });

	// Make the frame fit its contents.
	pack();
    }
    /************************************************** <pre>
     * Summary: If static object does not exist yet, create one
     *	        the first time showParamArrayPanel() is called,
     *		then return the object.
     *
     </pre> **************************************************/
    public static ParamArrayPanel showParamArrayPanel (ButtonIF vnmrIf) {

	getParamArrayPanel(vnmrIf);

	// UnIconify if necessary
	paramArrayPanel.setState(Frame.NORMAL);

	paramArrayPanel.origArrays = true;

	// requestUpdateArrayParamTable will update the up Panel (paramTable,
	// totalSize and totalTime) and use the first array parameter to
	// update the center Panel (entries and valueTable)
	paramArrayPanel.requestUpdateArrayParamTable();
	paramArrayPanel.saveOrigCurparArrayedParams();

	return paramArrayPanel;
    }

     /************************************************** <pre>
     * Summary: If static object does not exist yet, create one
     *	        the first time showParamArrayPanel() is called,
     *		then return the object.
     *
     </pre> **************************************************/
    public static ParamArrayPanel getParamArrayPanel (ButtonIF vnmrIf) {

	if( paramArrayPanel == null) {
	    paramArrayPanel = new ParamArrayPanel(vnmrIf);

	    // read array parameters from curpar;
	    paramArrayPanel.readARRAYEDfromCurpar();
	    // paramArrayPanel.undoButton.setEnabled(false);
	    paramArrayPanel.setUndoEnabled(false);

	    // create vArrayParams for vnmrbg to update paramTable
            paramArrayPanel.vArrayParams = new VParamArray(null, vnmrIf, null);
	    paramArrayPanel.vArrayParams.setPanel((Component) paramArrayPanel);

            // create vtotalTime for vnmrbg to update total time with exptime result
            paramArrayPanel.vtotalTime = new VParamArray(null, vnmrIf, null);
	    paramArrayPanel.vtotalTime.setPanel((Component) paramArrayPanel);

            paramArrayPanel.vsendCmd = new VParamArray(null, vnmrIf, null);
	    paramArrayPanel.vsendCmd.setAttribute(CMD, "settime");
	    paramArrayPanel.vsendCmd.updateValue();

	    // create vArrayValues for vnmrbg to update valueTable
	    paramArrayPanel.vArrayValues = new VParamArray(null, vnmrIf, null);
 	    paramArrayPanel.vArrayValues.setPanel((Component) paramArrayPanel);

            paramArrayPanel.setVisible(true);

        } else {

            paramArrayPanel.arrayedParams.clear();
            paramArrayPanel.readARRAYEDfromCurpar();
            paramArrayPanel.requestUpdateArrayParamTable();
	    String paramName = paramArrayPanel.getActiveParamName();
            paramArrayPanel.requestUpdateValueTable(paramName);
            paramArrayPanel.requestUpdateTotalTime();
            paramArrayPanel.saveOrigCurparArrayedParams();

            paramArrayPanel.setVisible(true);
            paramArrayPanel.repaint();
        }


	return paramArrayPanel;
    }

    public static ParamArrayPanel getParamArrayPanel () {
	return paramArrayPanel;
    }

    public static ParamArrayTable getParamArrayTable() {
	return paramTable;
    }

    public static ParamArrayValueTable getParamArrayValueTable() {
	return valueTable;
    }


   /************************************************** <pre>
     * Summary: Take care of Buttons etc in the panel.
     *
     </pre> **************************************************/
    public void actionPerformed(ActionEvent e) {
	String cmd = e.getActionCommand();
	// OK
	if(cmd.equals("history")) {
	    // Do not set setVisible(false) for history, it brings up a menu.

	}
	else if(cmd.equals("undo")) {
	    sendPrevToVnmr();
	    //undoButton.setEnabled(false);
	}
	else if(cmd.equals("close")) {
/*
	    updateCurpar();
	    vsendCmd.setAttribute(CMD, "flush");
	    vsendCmd.updateValue();
*/
	    vsendCmd.updateValue();
	    setVisible(false);

	}
	else if(cmd.equals("abandon")) {
	    sendOrigToVnmr();
/*
	    updateCurpar();
	    vsendCmd.setAttribute(CMD, "flush");
	    vsendCmd.updateValue();
*/
	    setVisible(false);
	}
	// Help
	else if(cmd.equals("help")) {
	    // Do not call setVisible(false);  That will cause
	    // the Block to release and the code which create
	    // this object will try to use userText.  This way
	    // the panel stays up and the Block stays in effect.
	    displayHelp();
	}

    }

    public void initializeParamTable() {

	Vector values2D = new Vector();
        // Create a vector for each row.

        Vector row1 = new Vector(5);
	for(int i=0; i < 5; i++) {
	    row1.add("");
	}
	values2D.add(row1);

        makeArrayParamTable(values2D);
    }

    /************************************************** <pre>
     * Summary: Request an update of the array parameter table.
     *
     *	This method will make a request to vnmrbg to get currently
     *	arrayed parameters.  When the results come back, then
     *	updateArrayParamTable() will be	called to do the update.
     </pre> **************************************************/

    public void requestUpdateArrayParamTable() {
	// when vnmrbg sends back ARRAY value, vnmrbgUpdateArrayedParams()
	// is called (which in turn calls updateArrayParamTable())

        vArrayParams.setAttribute(CMD, "jFunc(25)");
        vArrayParams.updateValue();

    }


    /************************************************** <pre>
     * Summary: Update the array parameter table.
     *
     *	Given the list of arrayed parameters and some other info
     *	update the array parameter table, 'paramTable'.
     *	The normal sequence would be to call requestUpdateArrayParamTable()
     *	first.  When the results come back from vnmrbg, this method
     *	can be called to do the Table update.
     </pre> **************************************************/

    public void updateArrayParamTable(Vector values) {

        // test whether update is necessary.

        if(paramTable != null) {
             Vector currentValues = getTableValues(paramTable);
             if(!has2DValueChanged(currentValues, values)) return;
        }

	makeArrayParamTable(values);

	setArraySize(getTotalSize());
	requestUpdateTotalTime();

	// update valueTable only if the active parameter is not valid.
	// because updateArrayParamTable is called only when the action
	// of centerPane generates pnew = array. i.e., the centerPane
	// is already updated.
/*
        String paramName = getActiveParamName();
        System.out.println("ActiveParamName "+paramName);
        int selectedRow = getRowByParamName(paramName);
        if(selectedRow < 0) { // active parameter is not one of the arrayed params.
            selectedRow = 0;
	    paramName = (String) paramTable.getValueAt(0,paramTable.PARAMCOL);
            if(paramName.length() > 0) {
		requestUpdateValueTable(paramName);
            } else initializeValueTable();
	}
        paramTable.setRowSelectionInterval(selectedRow, selectedRow);
*/
    }

    public void makeArrayParamTable(Vector values) {
	Vector param=new Vector();
	Vector order=new Vector();
	Vector size =new Vector();
	JTable oldTable;

	// Table
	Vector header;
	header = new Vector(5);
	header.add(Util.getLabel("_Param_Name"));
	header.add(Util.getLabel("_Description"));
	header.add(Util.getLabel("_Size"));
	header.add(Util.getLabel("_Order"));
	header.add(Util.getLabel("_On/Off"));

	oldTable = paramTable;
	paramTable = new ParamArrayTable(values, header);
	paramTable.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));

	// use TableModelListener to listener to the change of paramTable
        // this catches editing changes, not removing row(s).
        // removing a row (a parameter) will be catched by UnArrayButtonListener.

        paramTableModel = paramTable.getModel();
        paramTableModel.addTableModelListener(new TableModelListener() {
   	    public void tableChanged(TableModelEvent e) {
	    // The table gets changed in ParamArrayTable.mouseClicked().

	        int col = e.getColumn();
	        int row = e.getFirstRow();
	        paramTableAction(row, col);
            }

	});

	if(paramScrollpane == null || oldTable == null) {
	    paramScrollpane = new JScrollPane(paramTable,
				     JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
				     JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
	    Border border = BorderFactory.createLineBorder(Color.gray);
	    paramScrollpane.setBorder(border);

	    // Set preferred size of table
	    paramScrollpane.setPreferredSize(new Dimension(
		//paramScrollpane.getPreferredSize().width, 90));
		600, 120));
	}
	else {
	    Dimension dim = paramScrollpane.getSize();
	    paramScrollpane.setPreferredSize(dim);

	    paramScrollpane.remove(oldTable);
	    paramScrollpane.setViewportView(paramTable);
	}

	pack();

    }

    public void initializeValueTable() {
	makeValueTable("None", null);
	setSizeValue("");
	setFirstValue("");
	setLastValue("");
	setIncrement("");
	setIncStyle("None");
	setActiveParam("None");
	setCurrentValue("");
    }

    /************************************************** <pre>
     * Summary: Request an update of the array param values table.
     *
     *	This method will make a request to vnmrbg to get the values
     *	for the array parameter 'paramName'.  When the results
     *	come back, they should be matched up with this paramName,
     *	and put into a Vector.  Then updateValueTable() will be
     *	called to do the update.
     </pre> **************************************************/

    public void requestUpdateValueTable(String paramName) {
	VParamArray vp = getVparam(paramName);
	if(vp != null) {
    	   vp.setAttribute(CMD, new StringBuffer().append("jFunc(28, id, ").
                                      append(paramName).append(")").toString());

   	   vp.updateValue();
	} else {
    	   vArrayValues.setAttribute(CMD, new StringBuffer().append("jFunc(28, id, ").
                                                append(paramName).append(")").toString());

   	   vArrayValues.updateValue();
	}

    }

    /************************************************** <pre>
     * Summary: Update the array param values table.
     *
     *	Given the paramName and a Vector of values, update the
     *	array param values table 'valueTable'.
     *	The normal sequence would be to call requestUpdateValueTable()
     *	first.  When the results come back from vnmrbg, this method
     *	can be called to do the Table update.
     </pre> **************************************************/

    public void updateValueTable(String paramName, Vector values, String type) {

	int index = getRowByParamName(paramName);
        if(index == -1) return;

	updateArrayedParams(paramName, values, type);

	// update centerPane only if changed.

	String activeParam = getActiveParamName();
        if(valueTable != null && activeParam.equals(paramName)) {
             Vector currentValues = getTableColumnValues(valueTable, 1);
             if(!hasValueChanged(currentValues, values)) return;
        }

	updateCenterPane(paramName);
    }

    public void updateCenterPane(String paramName) {

        updateEntries(paramName);
        makeValueTable(paramName);
    }

    public void makeValueTable(String paramName) {

        int index = getArrayParamIndex(paramName);
        if(index == -1) initializeValueTable();
        else {
            if(((ArrayedParam)arrayedParams.elementAt(index)).on_off.equals("On")) {
                Vector values = getValues(paramName);
                makeValueTable(paramName, values);
            } else {
                Vector values = new Vector();
                values.add(((ArrayedParam)arrayedParams.elementAt(index)).currentValue);
                makeValueTable(paramName, values);
            }
        }
    }

    public void makeValueTable(String paramName, Vector values) {
	JTable oldTable;

	Vector values2D = new Vector();
	// Table
	Vector header;
	header = new Vector(2);
	header.add(Util.getLabel("_Position"));
	header.add(Util.getLabel("_Value"));

	// Now put the values into a 2D vector with the first column
	// counting up from 1.
	if(values == null) {
	    values = new Vector();
	    values.add("NA");
	}
	for(int i=1; i <= values.size(); i++) {
	    // Each vector has the values for one row.
	    Vector row = new Vector(2);
	    row.add(String.valueOf(i));
	    row.add(values.elementAt(i-1));
	    values2D.add(row);
	}

	oldTable = valueTable;
	valueTable = new ParamArrayValueTable(values2D, header);
	valueTable.setFont(DisplayOptions.getFont("Dialog", Font.BOLD, 14));


	// use TableModelListener to listener the change of valueTable
	// and update vnmrbg.
        valueTableModel = valueTable.getModel();
        valueTableModel.addTableModelListener(new TableModelListener() {
       	    public void tableChanged(TableModelEvent e) {
	       int col = e.getColumn();
	       int row = e.getFirstRow();
	       valueTableAction(row, col);
            }
	});

	if(valueScrollpane == null || oldTable == null) {
	    valueScrollpane = new JScrollPane(valueTable,
				     JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED,
				     JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
	    // Without large left and right border, the columns are too wide.
	    Border border = BorderFactory.createLineBorder(Color.gray);
	    valueScrollpane.setBorder(border);
	}
	else {
	    valueScrollpane.remove(oldTable);
	    valueScrollpane.setViewportView(valueTable);
	}

	// Set fixed height of table.  It does not seem to listen to the width.
	valueScrollpane.setPreferredSize(new Dimension(160, 220));

	pack();
    }


    /**************************************************
     * Summary: Update the Array Size item, 'sizeLabel'.
     *
     **************************************************/
    public void setArraySize (String value) {
	sizeLabel.setText(new StringBuffer().append(Util.getLabel("_Array_Size")).append( value).toString());

    }

    /**************************************************
     * Summary: Update the two Total Time items, 'timeLabel'.
     *
     **************************************************/
    public void setTotalTime (String value) {

     	//value is time in sec returned by vnmrbg command exptime.

        if(!isDigits(value)) return;

	int time = Float.valueOf(value).intValue();
	int hr = 0;
	int min = time/60;
	int sec = time%60;
	if(min > 60) {
	    hr = min/60;
	    min = min%60;
	}
	String str = new StringBuffer().append(String.valueOf(hr)).append(":").
                      append(String.valueOf(min)).append(":").
                      append(String.valueOf(sec)).toString();
	timeLabel.setText(new StringBuffer().append(Util.getLabel("_Total_Time")).append( str).toString());
    }


    /**************************************************
     * Summary: Update the Active Param item, 'activeValLabel'.
     *
     **************************************************/
    public void setActiveParam (String value) {
	activeValLabel.setText(value);

	int selectedRow = getRowByParamName(value);
	if(selectedRow == -1) return;
	paramTable.setRowSelectionInterval(selectedRow, selectedRow);
    }

    public void setCurrentValue (String value) {
        currentText.setText(value);
        String paramName = getActiveParamName();
        if(getArrayParamIndex(paramName) != -1)
        updateArrayedParam(paramName, value, ArrayedParam.CURRENT);
    }

    /**************************************************
     * Summary: Update the First Value item, 'firstText'.
     *
     **************************************************/
    public void setFirstValue (String value) {
	firstText.setText(value);
    }

    /**************************************************
     * Summary: Update the Increment item, 'incText'.
     *
     **************************************************/
    public void setIncrement (String value) {
	incText.setText(value);
    }

    /**************************************************
     * Summary: Update the Last Value item, 'lastText'.
     *
     **************************************************/
    public void setLastValue (String value) {
	lastText.setText(value);
    }

    /**************************************************
     * Summary: Update the Inc Style item, 'incStyleValueLabel'.
     *
     **************************************************/
    public void setIncStyle (String value) {
	incStyleValueLabel.setText(value);
    }

    /**************************************************
     * Summary: Return active parameter name.
     *
     *
     **************************************************/
    public String getActiveParamName() {
	return (activeValLabel.getText());
    }

    public String getCurrentValue() {
        return (currentText.getText());
    }

    /**************************************************
     * Summary: Return the value of the First Value item.
     *
     *
     **************************************************/
    public String getFirstValue() {
	return (firstText.getText());
    }

    /**************************************************
     * Summary: Return the value of the Increment item.
     *
     *
     **************************************************/
    public String getIncrement() {
	return (incText.getText());
    }

    /**************************************************
     * Summary: Return the value of the Last Value item.
     *
     *
     **************************************************/
    public String getLastValue() {
	return (lastText.getText());
    }

    /**************************************************
     * Summary: Return the value of the Inc Style item.
     *
     *
     **************************************************/
    public String getIncStyle() {
	return (incStyleValueLabel.getText());
    }


    private void sendArrayParamsToVnmr() {

	// use this method to update vnmrbg if On/Off state
	// or the order is changed.

    StringBuffer sbData = new StringBuffer();
	String str = "";
	if(paramTable.getRowCount() > 0)
        for(int i=0; i<paramTable.getRowCount(); i++) {
            if(((String) paramTable.getValueAt(i,paramTable.ONOFFCOL)).equals("On")) {
                String str1 = (String) paramTable.getValueAt(i,paramTable.PARAMCOL);
                String str2 = (String) paramTable.getValueAt(i,paramTable.ORDERCOL);
                if(str1.length() > 0 && isDigits(str2))
			sbData.append(str1).append(" ").append(str2).append(" ");
	    }
        }
    str = sbData.toString();
	if(str.endsWith(" "))
	    str = str.substring(0,str.length()-1);

	sendArrayParamsToVnmr(str);
    }

    private void sendArrayParamsToVnmr(String str) {

	if(str.length() <= 0) return;

	// str is the second argument of jFunc(26, arg2), which is
	// constructed from paramTable, i.e.,
	// paramTable[i][0] paramTable[i][3] where i=0, #_of_colmns.

        vArrayParams.setAttribute(CMD, new StringBuffer().append("jFunc(26, '" ).
                                            append(str).append( "')").toString());
        vArrayParams.updateValue();
    }

    private void sendCurrentValueToVnmr(String paramName) {
	int index = getArrayParamIndex(paramName);
	if(index == -1) return;

	String order = ((ArrayedParam)arrayedParams.elementAt(index)).order;
	if(order.equals("0")) sendValueToVnmr(paramName, "1");
	else {
	    String value = ((ArrayedParam)arrayedParams.elementAt(index)).currentValue;
	    sendValueToVnmr(paramName, value);
	}
    }

    private void sendArrayValuesToVnmr(String paramName, Vector v) {

	// use this method to update vnmrbg whenever the values
	// of valueTable changed.

        if(v == null || v.size() == 0) return;

	int index = getArrayParamIndex(paramName);
	if(index == -1) return;

	String type = getArrayedParamValue(paramName, ArrayedParam.TYPE);

    StringBuffer sbData = new StringBuffer();
	String values = "";
    int nSize = v.size();
	if(type.equals("s")) {
	    for(int i=0; i<nSize; i++) {
	        sbData.append("'").append((String) v.elementAt(i)).append("',");
	    }
	} else {
	    for(int i=0; i<nSize; i++) {
	        sbData.append((String) v.elementAt(i)).append(",");
	    }
	}
    values = sbData.toString();
	if(values.endsWith(","))
	    values = values.substring(0,values.length()-1);

	if(values.length() == 0 && !type.equals("s")) return;

	if(!values.endsWith(",") && values.indexOf(",,") == -1)
	sendArrayValuesToVnmr(paramName, values);

    }

    private void sendArrayValuesToVnmr(String paramName, String values) {

	// values is a String contains the values of the second column of
	// valueTable. The values are separated by commas, e.g.
	// "5.5, 6.0, 6.5, 7.0", or "'Direct Synthesis', 'H1 only'"

	vsendCmd.setAttribute(CMD, new StringBuffer().append(paramName).append(" = ").
                                    append( values).toString());
        vsendCmd.updateValue();
    }


    /**************************************************
     * Summary: Class to listen to New Array button.
     *
     **************************************************/
    public class NewArrayButtonListener implements ActionListener {

        public void  actionPerformed(ActionEvent e) {

	    Dimension dim = paramScrollpane.getSize();
	    paramScrollpane.setPreferredSize(dim);

        // set default order as max order + 1.
	     Vector orders = getTableColumnValues(paramTable, paramTable.ORDERCOL);
	     String newOrder = getMaxValue(orders);
	     if(!isDigits(newOrder)) newOrder = "1";
	     else newOrder = String.valueOf(1+Float.valueOf(newOrder).intValue());

	     paramTable.addBlankRow();
	     paramTable.setValueAt((Object) newOrder,paramTable.getSelectedRow(),paramTable.ORDERCOL);


	     if(paramTable.getRowCount() > 1) {
		paramTable.sortByOrder(paramTable.arrayedParams);
	        paramTable.resizeAndRepaint();
	     }
	     int selectedRow = getNewRow();
	     if(selectedRow != -1) {
		paramTable.setRowSelectionInterval(selectedRow, selectedRow);
		paramTable.grabFocus();
		paramTable.editCellAt(selectedRow, paramTable.PARAMCOL);
	     }

	     initializeValueTable();
        }
    }

    public int getNewRow() {

	int newRow = -1;
	if(paramTable == null) return newRow;

	if(paramTable.getRowCount() > 0)
	for(int i=0; i<paramTable.getRowCount(); i++)
	    if(isDigits((String)paramTable.getValueAt(i, paramTable.ORDERCOL))) newRow++;

	return newRow;
    }

    /**************************************************
     * Summary: Class to listen to UnArray button.
     *
     **************************************************/
    public class UnArrayButtonListener implements ActionListener {

        public void  actionPerformed(ActionEvent e) {

	        savePrevArrayedParams();

	    // Get selected row
	    int row = paramTable.getSelectedRow();
	    if(row < 0) return;

	    // Remove this row from the list.
	    //paramTable.removeOneRow(row);    // paramTable will get updated
					       // when vnmrbg send back pnew = array
					       // after the removed parameter is
					       // set to a single value.
	    // Tell vnmrbg
	    // send first value of the removed parameter to vnmrbg
	    // (assuming the values of the removed row are still displayed
	    // in the centerPanel). centerPanel will be updated
	    // with a single (current) value of the removed parameter.
	    // the array parameter of vnmrbg will update itself.

	    String paramName = (String)paramTable.getValueAt(row,paramTable.PARAMCOL);
	    String ord = (String)paramTable.getValueAt(row,paramTable.ORDERCOL);
	    String current = getCurrentValue(paramName);
            int size = getArraySize(paramName);

            if(current.length() > 0) sendCurrentValueToVnmr(paramName);

	    // display current value in centerPane
	    if(ord.equals("0")) {
		current = "1";
	        setCurrentValue(current);
	    }
	    Vector values = new Vector();
	    values.add(String.valueOf(current));
	    makeValueTable(paramName, values);
	    setSizeValue("1");
	    setFirstValue(current);
	    setLastValue(current);
	    setIncrement("0");

	    removeARRAYEDparam(paramName); // remove param after
            if(size < 2 || current.length() == 0) // no array sent back as pnew
                    updateParamTable();

	    // cannot undo UnArray
	    // undoButton.setEnabled(false);
	    setUndoEnabled(false);
        }
    }

    /**************************************************
     * Summary: Class to listen to Inc Style pop button.
     *
     **************************************************/
    public class StyleButtonListener implements PopListener {
        public void popHappened(String popStr) {

	    savePrevArrayedParams();

	    setIncStyle(popStr);
	    String paramName = getActiveParamName();
	    String ord = getValueByParamNameAndCol(paramName, paramTable.ORDERCOL);
	    if(!ord.equals("0")) {
	    	updateArrayedParam(paramName, popStr, ArrayedParam.INCSTYLE);
	    	requestVnmrbgCalArrayValues(paramName);
	    }
        }
    }

    public void updatePnewParams(Vector paramVector) {
	if(paramArrayPanel == null || paramVector == null) return;
	if(!paramArrayPanel.isVisible()) return;

        for (int k = 0; k < paramVector.size(); k++) {
            String paramName = (String) paramVector.elementAt(k);
	//System.err.println("pnew = " + k +" "+ paramName);
	    if(paramName.equals("array")) requestUpdateArrayParamTable();
	    if(paramName.equals("seqfil")) joinNewExp();
            if(getRowByParamName(paramName) != -1) {
		requestUpdateArrayParamTable();
		requestUpdateValueTable(paramName);
	    }
	    if(paramName.equals("nt") ||
		    paramName.equals("np") ||
		    paramName.equals("d1") ||
		    paramName.equals("ss") ||
		    paramName.equals("pw") ||
		    paramName.equals("p1") )
		requestUpdateTotalTime();
        }
    }

    /**************************************************
     * Summary: Calculate the total size using all array params.
     *
     *	Take into account that params of the same order are counted
     *	only once.
     *
     *	Also, check to see that all params of the same order have
     *	the same size.
     **************************************************/
    public String getTotalSize() {
	int onoff = ParamArrayTable.ONOFFCOL;
	int order = ParamArrayTable.ORDERCOL;
	int size = ParamArrayTable.SIZECOL;
	ParamArrayTable table = paramTable;

	// Update the Array Size item and be sure items of same order
	// are same size.
	if(!checkSize()) {
	    return "0";
	}

        ArrayList orders = new ArrayList();

	// Now calc array size
	int totalSize = 1;

	if(paramTable.getRowCount() > 0)
	  for(int i=0; i < paramTable.getRowCount(); i++) {
	    String ord =  (String)table.getValueAt(i,order);
	    String on = (String)table.getValueAt(i,onoff);
	    if(!(orders.contains(ord)) && on.equals("On")) {
		orders.add(ord);
		String str = (String)table.getValueAt(i,size);
		totalSize*= Integer.parseInt(str);
	    }
	  }

	return String.valueOf(totalSize);
    }

    /**************************************************
     * Summary: Be sure the sizes are the same for parameters of the same order.
     *
     *	Return true if sizes are okay, false if not equal.
     **************************************************/

    public boolean checkSize() {
	int order = ParamArrayTable.ORDERCOL;
	int size = ParamArrayTable.SIZECOL;
	int param = ParamArrayTable.PARAMCOL;
	ParamArrayTable table = paramTable;

	if(paramTable.getRowCount() > 0)
	for(int i=0; i < paramTable.getRowCount(); i++) {
	  if((String)table.getValueAt(i,order) != "0") {
	    // check that items of same order are also the same size.
	    for(int k=i+1; k < paramTable.getRowCount(); k++) {
		if(table.getValueAt(i,order) == table.getValueAt(k,order)) {
		    if(table.getValueAt(i,size) != table.getValueAt(k,size) ) {
			System.out.println(new StringBuffer().append("Error: Parameters " ).
			          append(table.getValueAt(i,param)).append(" and ").
			          append(table.getValueAt(k,param)).
			          append(" are the same order but different size.").toString());
			return false;
		    }
		}
	    }
	  }
	}
	return true;

    }

    public void requestUpdateTotalTime() {

        vsendCmd.setAttribute(CMD, "exptime:$totalTime");
        vsendCmd.updateValue();
        vtotalTime.setAttribute(SETVAL, "$VALUE=$totalTime");
        vtotalTime.updateValue();

    }

    private String getMaxValue(Vector values) {

        if(values == null || values.size() == 0) return "";

        int first = 0;
        String str = "";
        for(int i=first; i<values.size(); i++) {
            str = (String) values.elementAt(i);
            if(isDigits(str)) {first = i; break;}
            return "";
        }

        float max = Float.valueOf(str).floatValue();

        for(int i=first+1; i<values.size(); i++) {
            str = (String) values.elementAt(i);
            if(isDigits(str)) {
                float current = Float.valueOf(str).floatValue();
                if(current > max) max = current;
            }
        }

        return String.valueOf(max);
    }

    private String getIncrement(Vector values, String style) {

	// used only in valueTableAction and initilizing arrayedParams.
	//sort needed if values are randomized.

        if(values == null) return "";
        if(values.size() == 1) return "";

        int first = 0;
        String str = "";
        for(int i=first; i<values.size(); i++) {
            str = (String) values.elementAt(i);
            if(isDigits(str)) {first = i; break;}
            return "";
        }
        float prev = Float.valueOf(str).floatValue();

        int second = first + 1;
        for(int i=second; i<values.size(); i++) {
            str = (String) values.elementAt(i);
            if(isDigits(str)) {second = i; break;}
            return "";
        }

	int precision = getPrecision(values);

        float current = Float.valueOf(str).floatValue();
	String delta = calIncrement(2, prev, current, precision, style);

        String prevDelta = delta;
        prev = current;

        if(values.size() > second) {
            for(int i=second+1; i<values.size(); i++) {
                str = (String) values.elementAt(i);
                if(isDigits(str)) {
                    current = Float.valueOf(str).floatValue();
                    delta = calIncrement(2, prev, current, precision, style);
                    if(!delta.equals(prevDelta)) return "";
                    prevDelta = delta;
                    prev = current;
                }
            }
        }

        return prevDelta;
    }

    public Vector getTableRowValues(JTable t, int row) {

        Vector values = new Vector();

	if(t.getColumnCount() > 0)
        for(int i=0; i<t.getColumnCount(); i++) values.add(t.getValueAt(row,i));

        return values;
    }

    public Vector getTableColumnValues(JTable t, int col) {

        Vector values = new Vector();
	if(t.getRowCount() > 0)
        for(int i=0; i<t.getRowCount(); i++) values.add(t.getValueAt(i,col));

        return values;
    }

    public Vector getTableValues(JTable t) {

        Vector values2D = new Vector();
	if(t.getRowCount() > 0)
        for(int i=0; i<t.getRowCount(); i++)
            values2D.add(getTableRowValues(t, i));

        return values2D;
    }

    public boolean hasValueChanged(Vector v1, Vector v2) {

        if(v1.size() != v2.size()) return true;

        for(int i=0; i<v1.size(); i++) {
           if(!((String)v1.elementAt(i)).equals((String)v2.elementAt(i)))
                return true;
	}

        return false;
    }

    public boolean has2DValueChanged(Vector v1, Vector v2) {

        if(v1.size() != v2.size()) return true;

        for(int i=0; i<v1.size(); i++) {
            Vector row1 = (Vector) v1.elementAt(i);
            Vector row2 = (Vector) v2.elementAt(i);
            if(row1.size() != row2.size()) return true;
            for(int j=0; j<row1.size(); j++) {
                if(!((String)row1.elementAt(j)).equals((String)row2.elementAt(j)))
                    return true;
	    }
        }

        return false;

    }

    public static boolean isDigits(String str) {

        if(str == null || str.length() == 0) return false;

        try {
           Float.valueOf(str);
           return true;
        } catch (NumberFormatException e) {
           return false;
        }
    }

    public void recalFirstOrIncrementWhenLastValueChanged() {

	String paramName = getActiveParamName();
	if(getArrayedParamValue(paramName, ArrayedParam.TYPE).equals("s")) return;

        String first = getFirstValue();
        if(!isDigits(first)) first = null;

        String last = getLastValue();
        if(!isDigits(last)) last = null;

        String increment = getIncrement();
        if(!isDigits(increment)) increment = null;

	int size = 0;
        String str = getSizeValue();
        if(isDigits(str)) size = Float.valueOf(str).intValue();

	String style = getIncStyle();
	if(!style.equals(linearStyle) && !style.equals(expStyle)) {
	    style = linearStyle;
	    setIncStyle(linearStyle);
	    updateArrayedParam(paramName, linearStyle, ArrayedParam.INCSTYLE);
	}

	if(size > 0 && first != null && last != null) {
	    float firstValue = Float.valueOf(first).floatValue();
	    float lastValue = Float.valueOf(last).floatValue();
	    Vector values = new Vector();
	    values.add(first);
	    values.add(last);
	    if(style.equals(expStyle)) {
	      float inc = ((float)(Math.log(lastValue) - (float)Math.log(firstValue))
			/(size-1));
	      increment = String.valueOf(inc);
	    } else {
              float inc = (lastValue - firstValue)/(size-1);
	      increment = String.valueOf(inc);
	    }
            if(isDigits(increment)) {
		setIncrement(increment);
	        updateArrayedParam(paramName, increment, ArrayedParam.INCREMENT);
	    }
            requestVnmrbgCalArrayValues(paramName);
	} else if(size > 0 && increment != null && last != null) {
	    float lastValue = Float.valueOf(last).floatValue();
	    float incrementValue = Float.valueOf(increment).floatValue();
            first = calFirstValue(size, incrementValue, lastValue, style);
	    setFirstValue(first);
	    requestVnmrbgCalArrayValues(paramName);
	}
    }

    public String getValueByParamNameAndCol(String paramName, int col) {

        int row = getRowByParamName(paramName);

	if(row == -1) return "";

        return (String) paramTable.getValueAt(row,col);
    }

    public int getRowByParamName(String paramName) {

	if(paramName.length() == 0) return -1;

	if(paramTable.getRowCount() > 0)
        for(int i=0; i<paramTable.getRowCount(); i++)
            if(paramTable.getValueAt(i,paramTable.PARAMCOL).equals(paramName))
		return i;

        return -1;
    }

    private void requestVnmrbgCalArrayValues(String paramName) {

	if(getArrayParamIndex(paramName) == -1) return;
	if(getArrayedParamValue(paramName, ArrayedParam.TYPE).equals("s")) return;

    	String num = getSizeValue();
    	if(!isDigits(num)) num = null;

	String first = getFirstValue();
	if(!isDigits(first)) first = null;

	String increment = getIncrement();
	if(!isDigits(increment)) increment = null;

	if(paramName.length() > 0 && num != null && first != null && increment != null) {

	    int size = Float.valueOf(num).intValue();
	    if(valueTable.getRowCount() < size)
		valueTable.setNumRows(size);

	    String style = getIncStyle();

	    if(!style.equals(linearStyle) && !style.equals(expStyle)) {
		style = linearStyle;
		setIncStyle(linearStyle);
		updateArrayedParam(paramName, linearStyle, ArrayedParam.INCSTYLE);
	    }

	    if(style.equals(linearStyle)) {
	    	String cmd = new StringBuffer().append("array('").append(paramName).
                              append( "'," ).append(num).append( "," ).append(first).
                              append( "," ).append(increment).append( ",0)").toString();
	    	vsendCmd.setAttribute(CMD, cmd);
            	vsendCmd.updateValue();
	    	//requestUpdateValueTable(paramName);

	    } else if(style.equals(expStyle)) {
	    	String cmd = new StringBuffer().append("arrayExp('" ).append(paramName).
                              append( "'," ).append(num).append( "," ).append(first).
                              append( "," ).append(increment).append( ",0)").toString();
	    	vsendCmd.setAttribute(CMD, cmd);
            	vsendCmd.updateValue();
	    	//requestUpdateValueTable(paramName);
	    }
	}
    }

    public String calFirstValue(int size, float increment, float last, String style) {

	if(size < 2 || increment == 0) return String.valueOf(last);

	String first = "";
	if(style.equals(linearStyle))
            first = String.valueOf(last - (size-1) * increment);
	else if(style.equals(expStyle))
	    first = String.valueOf(last/(float)Math.exp((size-1) * increment));

	return first;
    }

    public String calIncrement(int size, float first, float last,
		int precision, String style) {

	if(size < 2 || last == first) return "0.0";

        String increment = "";

	if(style.equals(expStyle))
	    increment = calExpIncrement(size, first, last, precision);
 	else if(style.equals(linearStyle))
	    increment = calLinearIncrement(size, first, last, precision);

	return increment;
    }

    public String calLinearIncrement(int size, float first, float last, int precision) {

	if(size < 2 || last == first) return "0.0";

	float delta = (float)Math.pow(10,-precision);

        float increment = (last - first)/(size-1);

	increment = Math.round(increment/delta)*delta;

	String str = String.valueOf(increment);
	int p = str.indexOf(".");
	if(p >= 0 && p+precision+1 < str.length())
	    str = str.substring(0,p+precision+1);

	return str;
    }

    public String calExpIncrement(int size, float first, float last, int precision) {

	if(size < 2 || last == first || last == 0.0 || first == 0.0) return "0.0";

	float delta = (float)Math.pow(10,-precision);

	first = Math.abs(first);
	last = Math.abs(last);
        float increment = ((float)(Math.log(last) - (float)Math.log(first))/(size-1));

	increment = Math.round(increment/delta)*delta;

	String str = String.valueOf(increment);
	int p = str.indexOf(".");
	if(p >= 0 && p+precision+1 < str.length())
	    str = str.substring(0,p+precision+1);

	return str;
    }

    public int getPrecision(Vector values) {

	// precision is the number of digits after the decimal point.
	// the biggest precision of the Vector is returned.
	// minimum precision is 1.
	int precision = 1;

	for(int i=0; i<values.size(); i++) {
	    String str = (String)values.elementAt(i);
	    if(str.indexOf(".") < 0) str = "";
	    else str = str.substring(str.indexOf(".")+1);
	    if(str.length() > precision) precision = str.length();
	}

	return precision;
    }

    public String getIncStyle(Vector values) {

	// used only in valueTableAction and initilizing arrayedParams.
	//default is "None"
	//sort may be needed if values are randomized.

        if(values == null) return "None";
	if(values.size() < 3) return linearStyle;

	String style = linearStyle;
	String intc = getIncrement(values, style);
	if(intc.length() == 0) {
	    style = expStyle;
	    intc = getIncrement(values, style);
	    if(intc.length() == 0 || !isDigits(intc) ||
		Float.valueOf(intc).floatValue() == 0.0) style = "None";
	}

	return style;
    }

    public String getFirstValue(Vector v) {

        if(v == null || v.size() == 0) return "";

        return String.valueOf((String) v.elementAt(0));
    }

    public String getLastValue(Vector v) {

        if(v == null || v.size() == 0) return "";

        return String.valueOf((String) v.elementAt(v.size()-1));
    }

    class ArrayedParam {
	protected String paramName = "";
	protected String description = "";
	protected String type = "";
	protected String size = "";
	protected String order = "";
	protected String on_off = "Off";
	protected String currentValue = "";
	protected Vector values = new Vector();
	protected String firstValue = "";
	protected String lastValue = "";
	protected String increment = "";
	protected String incrementStyle = "";
	// vparam is a VObj its method will be used to send cmd to vnmrbg
	protected VParamArray vparam = null;
	// newArray is false if the param is already arrayed before
	// this dialog is popped up or jexp, otherwise true.
	protected boolean newArray = true;
	// unArray is true if the param is unarrayed
	protected boolean unArray = false;
	// true if the original value(s) of this param is saved.
	protected boolean origSaved = false;

	protected String origSize = "";
	protected String origOrder = "";
	protected String origOnOff = "Off";
	protected String origCurrentValue = "";
	protected Vector origValues = new Vector();
	protected String origFirstValue = "";
	protected String origLastValue = "";
	protected String origIncrement = "";
	protected String origIncrementStyle = "";

	protected String prevSize = "";
	protected String prevOrder = "";
	protected String prevOnOff = "Off";
	protected String prevCurrentValue = "";
	protected Vector prevValues = new Vector();
	protected String prevFirstValue = "";
	protected String prevLastValue = "";
	protected String prevIncrement = "";
	protected String prevIncrementStyle = "";

	public static final String NAME = "NAME";
	public static final String TYPE = "TYPE";
	public static final String ARRAYSIZE = "ARRAYSIZE";
	public static final String ARRAYORDER = "ARRAYORDER";
	public static final String ONOFF = "ONOFF";
	public static final String CURRENT = "CURRENT";
	public static final String ARRAYVALUES = "ARRAYVALUES";
	public static final String FIRST = "FIRST";
	public static final String LAST = "LAST";
	public static final String INCREMENT = "INCREMENT";
	public static final String INCSTYLE = "INCSTYLE";

	public ArrayedParam(String arrayInfo) {

	    StringTokenizer tok = new StringTokenizer(arrayInfo, ",\n");
	    while(tok.hasMoreTokens()) {
	      String str = tok.nextToken();
	      str = str.trim();
	      if(str.indexOf(":") != -1 && !str.endsWith(":")) {
              if(str.startsWith(NAME)) paramName = str.substring(str.indexOf(":")+1);
              if(str.startsWith(TYPE)) type = str.substring(str.indexOf(":")+1);
              if(str.startsWith(ARRAYSIZE)) size = str.substring(str.indexOf(":")+1);
              if(str.startsWith(ARRAYORDER)) order = str.substring(str.indexOf(":")+1);
              if(str.startsWith(ONOFF)) on_off = str.substring(str.indexOf(":")+1);
              if(str.startsWith(CURRENT)) currentValue = str.substring(str.indexOf(":")+1);
              if(str.startsWith(FIRST)) firstValue = str.substring(str.indexOf(":")+1);
              if(str.startsWith(LAST)) lastValue = str.substring(str.indexOf(":")+1);
              if(str.startsWith(INCREMENT)) increment = str.substring(str.indexOf(":")+1);
              if(str.startsWith(INCSTYLE)) incrementStyle = str.substring(str.indexOf(":")+1);
	        if(str.startsWith(ARRAYVALUES)) {
		    str = str.substring(str.indexOf(":")+1);
		    StringTokenizer token;
		    if(type.equals("s")) token = new StringTokenizer(str,";\t");
		    else token = new StringTokenizer(str," \t");
		    while(token.hasMoreTokens()) {
			String value = token.nextToken();
		 	value = value.trim();
			if(value.length() > 0) values.add(value);
		    }
		    //size = String.valueOf(values.size());
		}
	      }
	    }

	    description = Util.getParamDescription(paramName);

	    vparam = new VParamArray(null, vnmrIf, null);
	    vparam.setPanel((Component) paramArrayPanel);
	}

        public boolean origCurrentValueChanged() {
	    if(!size.equals(origSize) ||
	       !order.equals(origOrder) ||
	       !on_off.equals(origOnOff) ||
	       !currentValue.equals(origCurrentValue) ) return true;

	    return false;
	}

        public boolean origValuesChanged() {
	    if(!size.equals(origSize) ||
	       !order.equals(origOrder) ||
	       !on_off.equals(origOnOff) ||
	       !values.equals(origValues) ) return true;

	    return false;
	}

        public boolean prevChanged() {
	    if(!size.equals(prevSize) ||
	       !order.equals(prevOrder) ||
	       !on_off.equals(prevOnOff) ||
	       !values.equals(prevValues) ||
	       !currentValue.equals(prevCurrentValue) ||
	       !firstValue.equals(prevFirstValue) ||
	       !lastValue.equals(prevLastValue) ||
	       !increment.equals(prevIncrement) ||
	       !incrementStyle.equals(prevIncrementStyle) ) return true;

	    return false;
	}

	public void saveOrig() {

	// make sure it is a new copy, not point to the same string.

	    char[] chars = size.toCharArray();
	    origSize = String.valueOf(chars);

	    chars = order.toCharArray();
	    origOrder = String.valueOf(chars);

	    chars = on_off.toCharArray();
	    origOnOff = String.valueOf(chars);

	    chars = currentValue.toCharArray();
	    origCurrentValue = String.valueOf(chars);

	    origValues = copyVector(values);

	    chars = firstValue.toCharArray();
	    origFirstValue = String.valueOf(chars);

	    chars = lastValue.toCharArray();
	    origLastValue = String.valueOf(chars);

	    chars = increment.toCharArray();
	    origIncrement = String.valueOf(chars);

	    chars = incrementStyle.toCharArray();
	    origIncrementStyle = String.valueOf(chars);

	}

	public void savePrev() {

	    char[] chars = size.toCharArray();
	    prevSize = String.valueOf(chars);
	    chars = order.toCharArray();
	    prevOrder = String.valueOf(chars);
	    chars = on_off.toCharArray();
	    prevOnOff = String.valueOf(chars);
	    chars = currentValue.toCharArray();
	    prevCurrentValue = String.valueOf(chars);
	    prevValues = copyVector(values);
	    chars = firstValue.toCharArray();
	    prevFirstValue = String.valueOf(chars);
	    chars = lastValue.toCharArray();
	    prevLastValue = String.valueOf(chars);
	    chars = increment.toCharArray();
	    prevIncrement = String.valueOf(chars);
	    chars = incrementStyle.toCharArray();
	    prevIncrementStyle = String.valueOf(chars);
	}

	public void copyOrig() {

	    char[] chars = origSize.toCharArray();
	    size = String.valueOf(chars);
	    chars = origOrder.toCharArray();
	    order = String.valueOf(chars);
	    chars = origOnOff.toCharArray();
	    on_off = String.valueOf(chars);
	    chars = origCurrentValue.toCharArray();
	    currentValue = String.valueOf(chars);
	    values = copyVector(origValues);
	    chars = origFirstValue.toCharArray();
	    firstValue = String.valueOf(chars);
	    chars = origLastValue.toCharArray();
	    lastValue = String.valueOf(chars);
	    chars = origIncrement.toCharArray();
	    increment = String.valueOf(chars);
	    chars = origIncrementStyle.toCharArray();
	    incrementStyle = String.valueOf(chars);
	}

	public void copyPrev() {

	// swab prev with current

	    String tmp = "";

	    tmp = size.substring(0);
	    size = prevSize.substring(0);
	    prevSize = tmp.substring(0);

	    tmp = order.substring(0);
	    order = prevOrder.substring(0);
	    prevOrder = tmp.substring(0);

	    tmp = on_off.substring(0);
	    on_off = prevOnOff.substring(0);
	    prevOnOff = tmp.substring(0);

	    tmp = currentValue.substring(0);
	    currentValue = prevCurrentValue.substring(0);
	    prevCurrentValue = tmp.substring(0);

	    tmp = firstValue.substring(0);
	    firstValue = prevFirstValue.substring(0);
	    prevFirstValue = tmp.substring(0);

	    tmp = lastValue.substring(0);
	    lastValue = prevLastValue.substring(0);
	    prevLastValue = tmp.substring(0);

	    tmp = increment.substring(0);
	    increment = prevIncrement.substring(0);
	    prevIncrement = tmp.substring(0);

	    tmp = incrementStyle.substring(0);
	    incrementStyle = prevIncrementStyle.substring(0);
	    prevIncrementStyle = tmp.substring(0);

	    Vector tmpV = copyVector(values);
	    values = copyVector(prevValues);
	    prevValues = copyVector(tmpV);
/*
	    char[] chars = prevSize.toCharArray();
	    size = String.valueOf(chars);
	    chars = prevOrder.toCharArray();
	    order = String.valueOf(chars);
	    chars = prevOnOff.toCharArray();
	    on_off = String.valueOf(chars);
	    chars = prevCurrentValue.toCharArray();
	    currentValue = String.valueOf(chars);
	    values = copyVector(prevValues);
	    chars = prevFirstValue.toCharArray();
	    firstValue = String.valueOf(chars);
	    chars = prevLastValue.toCharArray();
	    lastValue = String.valueOf(chars);
	    chars = prevIncrement.toCharArray();
	    increment = String.valueOf(chars);
	    chars = prevIncrementStyle.toCharArray();
	    incrementStyle = String.valueOf(chars);
*/
	}

	public Vector copyVector(Vector v) {
	    Vector c = new Vector();
	    if(v.size() == 0) return c;
	    char[] chars;
	    for(int i=0; i<v.size(); i++) {
		chars = ((String)v.elementAt(i)).toCharArray();
		c.add(String.valueOf(chars));
	    }

	    return c;
	}

	public void emptyCurrent() {
	    unArray = true;
	    size = "";
	    order = "";
	    on_off = "";
	    currentValue = "";
	    values.clear();
	    firstValue = "";
	    lastValue = "";
	    increment = "";
	    incrementStyle = "";
	}

	public Vector makeRowForParamTable() {

	    Vector row = new Vector();
	    row.add(paramName);
	    row.add(description);
	    row.add(size);
	    row.add(order);
	    row.add(on_off);

	    return row;
	}

	public int sizeOfArray() {
	    return values.size();
	}

	public void display() {

	    if(newArray) System.out.println(" newArray ");
	    if(unArray) System.out.println(" unArray ");
	    if(origSaved) System.out.println(" origSaved ");
	    System.out.println(getValueForCurpar());

        StringBuffer sbData = new StringBuffer();
	    String array = "";
        int nSize = origValues.size();
            for(int i=0; i<nSize; i++) {
                String value = (String)origValues.elementAt(i);
                sbData.append(value).append(" ");
            }
            array = sbData.toString();

            sbData =new StringBuffer().append( NAME).append(":").append(paramName).
                         append(",").append(TYPE).append(":").append(type).
                         append(",").append(CURRENT).append(":").append(origCurrentValue).
                         append(",").append(ONOFF).append(":").append(origOnOff).
                         append(",").append(ARRAYORDER).append(":").append(origOrder).
                         append(",").append(ARRAYSIZE).append(":").append(origSize).
                         append(",").append(FIRST).append(":").append(origFirstValue).
                         append(",").append(LAST).append(":").append(origLastValue).
                         append(",").append(INCREMENT).append(":").append(origIncrement).
                         append(",").append(INCSTYLE).append(":").append(origIncrementStyle).
                         append(",").append(ARRAYVALUES).append(":").append(array);

	    System.out.println(sbData.toString());

        sbData = new StringBuffer();
	    array = "";
        nSize = prevValues.size();
            for(int i=0; i<nSize; i++) {
                String value = (String)prevValues.elementAt(i);
                sbData.append(value).append(" ");
            }
            array = sbData.toString();

            sbData =new StringBuffer().append( NAME).append(":").append(paramName).
                         append(",").append(TYPE).append(":").append(type).
                         append(",").append(CURRENT).append(":").append(prevCurrentValue).
                         append(",").append(ONOFF).append(":").append(prevOnOff).
                         append(",").append(ARRAYORDER).append(":").append(prevOrder).
                         append(",").append(ARRAYSIZE).append(":").append(prevSize).
                         append(",").append(FIRST).append(":").append(prevFirstValue).
                         append(",").append(LAST).append(":").append(prevLastValue).
                         append(",").append(INCREMENT).append(":").append(prevIncrement).
                         append(",").append(INCSTYLE).append(":").append(prevIncrementStyle).
                         append(",").append(ARRAYVALUES).append(":").append(array);

	    System.out.println(sbData.toString());

	}

	public String getValueForCurpar() {
	    String array = "";
        StringBuffer sbData = new StringBuffer();
        int nSize = values.size();
	    for(int i=0; i<nSize; i++) {
            String value = (String)values.elementAt(i);
            sbData.append(value).append(" ");
	    }
        array = sbData.toString();

	    String str =new StringBuffer().append( NAME).append(":").append(paramName).
                         append(",").append(TYPE).append(":").append(type).
                         append(",").append(CURRENT).append(":").append(currentValue).
                         append(",").append(ONOFF).append(":").append(on_off).
                         append(",").append(ARRAYORDER).append(":").append(order).
                         append(",").append(ARRAYSIZE).append(":").append(size).
                         append(",").append(FIRST).append(":").append(firstValue).
                         append(",").append(LAST).append(":").append(lastValue).
                         append(",").append(INCREMENT).append(":").append(increment).
                         append(",").append(INCSTYLE).append(":").append(incrementStyle).
                         append(",").append(ARRAYVALUES).append(":").append(array).toString();

	    return str;
	}

	public void updateValues(Vector v) {
	    values = v;
	}

	public void update(String str, String key) {
	    if(key.equals(NAME)) paramName = str;
	    if(key.equals(TYPE)) type = str;
	    if(key.equals(ARRAYSIZE)) size = str;
	    if(key.equals(ARRAYORDER)) order = str;
	    if(key.equals(ONOFF)) on_off = str;
            if(key.equals(CURRENT)) currentValue = str;
	    if(key.equals(FIRST)) firstValue = str;
	    if(key.equals(LAST)) lastValue = str;
	    if(key.equals(INCREMENT)) increment = str;
	    if(key.equals(INCSTYLE)) incrementStyle = str;
	    if(key.equals(ARRAYVALUES)) {
		StringTokenizer token;
		if(type.equals("s")) token = new StringTokenizer(str,";\t");
		else token = new StringTokenizer(str," \t");
		values.clear();
		while(token.hasMoreTokens()) {
		    String value = token.nextToken();
		    value = value.trim();
		    if(value.length() > 0) values.add(value);
		}
		//size = String.valueOf(values.size());
	    }
	}
    }

    public void readARRAYEDfromCurpar() {

	arrayedParams = new Vector();
/*
	String expdir = Util.getExpDir();
	String filepath = new String (expdir + "/curpar");

	BufferedReader in;
        String line;
        try {
            in = new BufferedReader(new FileReader(filepath));
            while ((line = in.readLine()) != null) {
	        if(line.startsWith("ARRAYED") && line.lastIndexOf("Off") < 0) {
	            line = in.readLine();
		    line = line.substring(1); // remove first char, which is 1;
		    line = line.substring(2,line.length()-2); // remove double quotes.
		    ArrayedParam param = new ArrayedParam(line);
		    param.update("Off",ArrayedParam.ONOFF);
		    arrayedParams.add(param);
		}
	    }
	} catch (IOException ioe) { }
*/
    }

    public int getArrayParamIndex(String paramName) {

	if(arrayedParams == null || arrayedParams.size() == 0) return -1;

	for(int i=0; i<arrayedParams.size(); i++) {
	    String name = ((ArrayedParam)arrayedParams.elementAt(i)).paramName;
	    if(paramName.equals(name)) return i;
	}
	return -1;
    }

    public void saveOrigCurparArrayedParams() {

	// this is to save values in curpar ARRAYparams
	// saveOrig will be called once more when updateArrayedParams
	// is first executed.

	if(arrayedParams == null || arrayedParams.size() == 0) return;

	for(int i=0; i<arrayedParams.size(); i++) {
	    ((ArrayedParam)arrayedParams.elementAt(i)).saveOrig();
	    ((ArrayedParam)arrayedParams.elementAt(i)).newArray = false;
	// origSaved is false because saveOrig will be called again
	    ((ArrayedParam)arrayedParams.elementAt(i)).origSaved = false;
	}
    }

    public void savePrevArrayedParams() {

	if(arrayedParams == null || arrayedParams.size() == 0) return;

	for(int i=0; i<arrayedParams.size(); i++)
	    ((ArrayedParam)arrayedParams.elementAt(i)).savePrev();

	// undoButton.setEnabled(true);
	setUndoEnabled(true);
    }

    public int getActualArraySize(int row) {
	String paramName = (String) paramTable.getValueAt(row, paramTable.PARAMCOL);
	return getArraySize(paramName);
    }

    public int getArraySize(String paramName) {
        int arraySize = -1;
        int index = getArrayParamIndex(paramName);
        if(index != -1 )
           arraySize = ((ArrayedParam)arrayedParams.elementAt(index)).sizeOfArray();
	return arraySize;
    }

    public Vector getValues(String paramName) {
	Vector values = new Vector();
	int index = getArrayParamIndex(paramName);
        if(index != -1 )
	    values = ((ArrayedParam)arrayedParams.elementAt(index)).values;
	return values;
    }

    public String getCurrentValue(String paramName) {
	String current = "";
	int index = getArrayParamIndex(paramName);
        if(index != -1 )
	    current = ((ArrayedParam)arrayedParams.elementAt(index)).currentValue;
	return current;
    }

    public void updateArrayedParam(String paramName, String str, String key) {

	String NAME = ArrayedParam.NAME;
	String TYPE = ArrayedParam.TYPE;
        String ARRAYORDER = ArrayedParam.ARRAYORDER;
        String ARRAYSIZE = ArrayedParam.ARRAYSIZE;
        String ONOFF = ArrayedParam.ONOFF;
	String CURRENT = ArrayedParam.CURRENT;
	String ARRAYVALUES = ArrayedParam.ARRAYVALUES;
	String FIRST = ArrayedParam.FIRST;
	String LAST = ArrayedParam.LAST;
	String INCREMENT = ArrayedParam.INCREMENT;
	String INCSTYLE = ArrayedParam.INCSTYLE;

	int index = getArrayParamIndex(paramName);
	if(index == -1) {
 	    ArrayedParam newParam = new ArrayedParam(new StringBuffer().append(NAME ).
                                                      append(":").append( paramName).toString());

	    if(key.equals(ARRAYORDER)) newParam.update(str,ARRAYORDER);
	    if(key.equals(ARRAYSIZE)) newParam.update(str,ARRAYSIZE);
	    if(key.equals(ONOFF)) newParam.update(str,ONOFF);

	    if(key.equals(TYPE)) newParam.update(str,TYPE);
	    if(key.equals(CURRENT)) newParam.update(str,CURRENT);
	    if(key.equals(ARRAYVALUES)) newParam.update(str,ARRAYVALUES);
	    if(key.equals(FIRST)) newParam.update(str,FIRST);
	    if(key.equals(LAST)) newParam.update(str,LAST);
	    if(key.equals(INCREMENT)) newParam.update(str,INCREMENT);
	    if(key.equals(INCSTYLE)) newParam.update(str,INCSTYLE);

	    arrayedParams.add(newParam);
	} else {
	    if(key.equals(ARRAYORDER))
		((ArrayedParam)arrayedParams.elementAt(index)).update(str,ARRAYORDER);
	    if(key.equals(ARRAYSIZE))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,ARRAYSIZE);
	    if(key.equals(ONOFF))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,ONOFF);
	    if(key.equals(TYPE))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,TYPE);
	    if(key.equals(CURRENT))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,CURRENT);
	    if(key.equals(ARRAYVALUES))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,ARRAYVALUES);
	    if(key.equals(FIRST))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,FIRST);
	    if(key.equals(LAST))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,LAST);
	    if(key.equals(INCREMENT))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,INCREMENT);
	    if(key.equals(INCSTYLE))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(str,INCSTYLE);
	}
    }

    public void initializeArrayedParams() {
	for(int i=0; i<arrayedParams.size(); i++) {
	    ((ArrayedParam)arrayedParams.elementAt(i)).update("1",ArrayedParam.ARRAYORDER);
	    ((ArrayedParam)arrayedParams.elementAt(i)).update("1",ArrayedParam.ARRAYSIZE);
	    ((ArrayedParam)arrayedParams.elementAt(i)).update("Off",ArrayedParam.ONOFF);

	}
    }

    public void vnmrbgUpdateArrayedParams(String ARRAYstr) {

	initializeArrayedParams();

	//System.out.println("ARRAYstr "+ARRAYstr);

	StringTokenizer tok = new StringTokenizer(ARRAYstr, " ,\n");
        int size = tok.countTokens();

	if(size < 3) {
	   updateParamTable();
	   return;
	}

        int col = 3;
        int row = 0;
        if(size % 3 == 0) row = size/3;
        else {
            System.err.println(" Error: jFunc(25) results.");
            return;
        }

	String NAME = ArrayedParam.NAME;
	String ARRAYORDER = ArrayedParam.ARRAYORDER;
	String ARRAYSIZE = ArrayedParam.ARRAYSIZE;
	String ONOFF = ArrayedParam.ONOFF;

        for (int i=0; i<row; i++) {
            String param = tok.nextToken();
            String order = tok.nextToken();
            String count = tok.nextToken();

	    int index = getArrayParamIndex(param);
	    if(index != -1) {
		((ArrayedParam)arrayedParams.elementAt(index)).update(param,NAME);
		((ArrayedParam)arrayedParams.elementAt(index)).update(order,ARRAYORDER);
		((ArrayedParam)arrayedParams.elementAt(index)).update(count,ARRAYSIZE);
		((ArrayedParam)arrayedParams.elementAt(index)).update("On",ONOFF);
	    } else {
		String str =new StringBuffer().append( NAME ).append(":").append( param ).
                         append(",").append( ARRAYORDER ).append(":").append( order ).
                         append(",").append(ARRAYSIZE ).append(":").append( count ).
                         append(",").append( ONOFF ).append(":").append( "On").toString();

		ArrayedParam newParam = new ArrayedParam(str);
		arrayedParams.add(newParam);
	        index = getArrayParamIndex(param);
	    }

	// do this once to get and save the values of all original arrayed params
	    if(!((ArrayedParam)arrayedParams.elementAt(index)).origSaved) {
		requestUpdateValueTable(param);
	    }
	}

	updateParamTable();

    }

    public void updateParamTable() {

	// origArrays is set true in showParamArrayPanel
	// so everytime paramArrayPanel is popped up, this
	// is called (only once) to remember the original arrayed
	// parameters.
	if(origArrays) {
	   origArrays = false;
	   if(arrayedParams != null && arrayedParams.size() > 0)
	     for(int i=0; i<arrayedParams.size(); i++)
		((ArrayedParam)arrayedParams.elementAt(i)).newArray = false;
	}

	Vector values = getValuesOfArrayedParams();
	if(values.size() > 0) updateArrayParamTable(values);
	else { initializeParamTable();
	       initializeValueTable();
	}
    }

    public Vector getValuesOfArrayedParams() {

	Vector values = new Vector();
	if(arrayedParams != null)
	    for(int i=0; i<arrayedParams.size(); i++)
	        if(!((ArrayedParam)arrayedParams.elementAt(i)).unArray)
		  values.add(((ArrayedParam)arrayedParams.elementAt(i)).makeRowForParamTable());

	if(values.size() > 0) ParamArrayTable.sortByOrder(values);

	return values;
    }

    public void vnmrbgUpdateArrayValues(String paramName, String avalStr) {

	//System.out.println("paramName, avalStr "+paramName +" "+avalStr);

	if(avalStr.indexOf("NOVALUE") != -1) { // not a valid vnmr param.
	    System.out.println(new StringBuffer().append("paramName " ).
                           append( paramName ).append(" is not defined.").toString());
	    int row = getRowByParamName(paramName);
	    if(row != -1) {
		paramTable.removeOneRow(row);
	        removeARRAYEDparam(paramName);
		row = getRowByParamName(getActiveParamName());
		if(row != -1)
		paramTable.setRowSelectionInterval(row, row);
	    }
	    return;
	}

	StringTokenizer tok = new StringTokenizer(avalStr, " ,\n");
        int size = tok.countTokens();

	// parameter type: s for string, r for real, i for integer.
	String type = tok.nextToken();

	if(type.equals("s")) {
	    StringTokenizer token = new StringTokenizer(avalStr, ";\n");
	    size = token.countTokens();
	} else size = size -2;

        int row = (Integer.valueOf(tok.nextToken())).intValue();
        if(row != size) {
            System.err.println(" Error: jFunc(28) output.");
            return;
        }
        Vector values = new Vector();
	String value;
        for (int i=0; i<row; i++) {
	    if(type.equals("s")) value = tok.nextToken(";\n");
	    else value = tok.nextToken(" ,\n");
	    value = value.trim();
	    if(value.length() > 0) values.add(value);
        }

	updateValueTable(paramName, values, type);
    }

    public void updateCurpar() {
/*
	for(int i=0; i<arrayedParams.size(); i++) {
	  if(!((ArrayedParam)arrayedParams.elementAt(i)).unArray &&
		((ArrayedParam)arrayedParams.elementAt(i)).on_off.equals("On")) {
	    String paramName = "ARRAYED"+((ArrayedParam)arrayedParams.elementAt(i)).paramName;
	    String value = ((ArrayedParam)arrayedParams.elementAt(i)).getValueForCurpar();
	    if(value.length() > MAXLENGTH) value = value.substring(0,MAXLENGTH);
	    vsendCmd.setAttribute(CMD, "updateARRAYEDparam('" +paramName+ "','" +value+ "')");
	    vsendCmd.updateValue();
          }
	}
*/
    }

    public void removeARRAYEDparam(String paramName) {
/*
	vsendCmd.setAttribute(CMD, "removeARRAYEDparam('ARRAYED" +paramName+ "')");
	vsendCmd.updateValue();
*/
	int index = getArrayParamIndex(paramName);
	if(index != -1)
		((ArrayedParam)arrayedParams.elementAt(index)).emptyCurrent();
	if(currentArrayedParamsSize() == 0) {
	    initializeParamTable();
	    initializeValueTable();
	}
    }

    public int currentArrayedParamsSize() {

	int size = 0;
	for(int i=0; i<arrayedParams.size(); i++)
	    if(!((ArrayedParam)arrayedParams.elementAt(i)).unArray)
            size++;

	return size;
    }

    public int currentInArray(String value) {

	if(valueTable == null || valueTable.getRowCount() <= 0) return -1;

	for(int i=0; i<valueTable.getRowCount(); i++) {
	    String str = (String)valueTable.getValueAt(i,1);
	    if(str.equals(value)) return i;
	}
	return -1;
    }

    public void randomizeValueList(String paramName) {

	//first value is not changed.
	//no randomiation if list size < 3.

	int index = getArrayParamIndex(paramName);
	if(index == -1) return;
        Vector values = ((ArrayedParam)arrayedParams.elementAt(index)).values;
	if(values == null || values.size() < 3) return;
	int size = values.size();
	for(int i=0; i<size; i++) {
	    // Size-1 will keep the last value from changing.
	    int p = (int) ((size-1) * Math.random());

	    if(p > 0) {
	        Object obj = values.elementAt(p);
	        values.remove(p);
		// Insert above last item if from first half, and after
		// first item if from last half.  This is to be sure
		// that the value in position 1 changes even if the
		// number 1 does not come up.
		if(p < size/2)
		    values.add(size-2, obj);
		else
		    values.add(1, obj);
	    }
	}

	((ArrayedParam)arrayedParams.elementAt(index)).updateValues(values);
	setIncStyle("None");
	((ArrayedParam)arrayedParams.elementAt(index)).update("None", ArrayedParam.INCSTYLE);
	sendArrayValuesToVnmr(paramName, values);
	//requestUpdateValueTable(paramName);
    }

    public VParamArray getVparam(String paramName) {
	int index = getArrayParamIndex(paramName);
        if(index == -1) return null;
	else return ((ArrayedParam)arrayedParams.elementAt(index)).vparam;
    }

    public String getArrayedParamValue(String paramName, String key) {

	int index = getArrayParamIndex(paramName);
        if(index == -1) return null;

	String value = null;

	if(key.equals(ArrayedParam.TYPE))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).type;
	if(key.equals(ArrayedParam.ARRAYSIZE))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).size;
	if(key.equals(ArrayedParam.ARRAYORDER))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).order;
	if(key.equals(ArrayedParam.ONOFF))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).on_off;
	if(key.equals(ArrayedParam.CURRENT))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).currentValue;
	if(key.equals(ArrayedParam.FIRST))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).firstValue;
	if(key.equals(ArrayedParam.LAST))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).lastValue;
	if(key.equals(ArrayedParam.INCREMENT))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).increment;
	if(key.equals(ArrayedParam.INCSTYLE))
	    value = ((ArrayedParam)arrayedParams.elementAt(index)).incrementStyle;

	return value;
    }

    public void updateArrayedParams(String paramName, Vector values, String type) {

	//update arrayedParams and entries

	int index = getArrayParamIndex(paramName);
	if(index == -1) return;

	updateArrayedParam(paramName, type, ArrayedParam.TYPE);

	if(values == null || values.size() == 0) return;

	String CURRENT = ArrayedParam.CURRENT;
	String ONOFF = ArrayedParam.ONOFF;
	String ARRAYORDER = ArrayedParam.ARRAYORDER;
	String ARRAYSIZE = ArrayedParam.ARRAYSIZE;
	String FIRST = ArrayedParam.FIRST;
	String LAST = ArrayedParam.LAST;
	String INCREMENT = ArrayedParam.INCREMENT;
	String INCSTYLE = ArrayedParam.INCSTYLE;

	String str;
	int size = values.size();
	if(size > 1) {

	    // update values
	    ((ArrayedParam)arrayedParams.elementAt(index)).updateValues(values);

	    // set array on if was off
	    if(((ArrayedParam)arrayedParams.elementAt(index)).on_off.equals("Off")) {
		updateArrayedParam(paramName, "On", ONOFF);
		updateArrayedParam(paramName,
			String.valueOf(currentArrayedParamsSize()), ARRAYORDER);
		updateArrayedParam(paramName, String.valueOf(size), ARRAYSIZE);
	        updateParamTable();
	    }

	} else if(size == 1) {

	  // update current value
	  updateArrayedParam(paramName, (String) values.elementAt(0), CURRENT);

	  String ord = ((ArrayedParam)arrayedParams.elementAt(index)).order;

	  if(!ord.equals("0")) {

	    int currentArraySize = getArraySize(paramName);

	    // if was arrayed, turn off, but don't update values
	    if(currentArraySize > 1 &&
		((ArrayedParam)arrayedParams.elementAt(index)).on_off.equals("On")) {
		updateArrayedParam(paramName, "Off", ArrayedParam.ONOFF);
		updateArrayedParam(paramName, "1", ArrayedParam.ARRAYORDER);
		updateArrayedParam(paramName, "1", ArrayedParam.ARRAYSIZE);
	        updateParamTable();
	    } else if (currentArraySize <= 1) {
		// update values regardless on/off
	        ((ArrayedParam)arrayedParams.elementAt(index)).updateValues(values);
	    }

	  } else {

	    Vector v = ((ArrayedParam)arrayedParams.elementAt(index)).values;
	    int currentArraySize = 0;
	    if(v != null && v.size() > 0) {
	     currentArraySize = Float.valueOf((String)v.elementAt(0)).intValue();
	    }

	    str = (String)values.elementAt(0);
	    int arraySize = Float.valueOf(str).intValue();

	    if(arraySize > 1 || currentArraySize <= 1) {
	        ((ArrayedParam)arrayedParams.elementAt(index)).updateValues(values);
	    }

	    if(isDigits(str) && arraySize > 1) {
	        if(((ArrayedParam)arrayedParams.elementAt(index)).on_off.equals("Off")) {
		    updateArrayedParam(paramName, "On", ONOFF);
		    updateArrayedParam(paramName, "0", ARRAYORDER);
		    updateArrayedParam(paramName, str, ARRAYSIZE);
	            updateParamTable();
		}
	    } else if(currentArraySize > 1 &&
		((ArrayedParam)arrayedParams.elementAt(index)).on_off.equals("On")) {
		updateArrayedParam(paramName, "Off", ArrayedParam.ONOFF);
		updateArrayedParam(paramName, "0", ArrayedParam.ARRAYORDER);
		updateArrayedParam(paramName, "1", ArrayedParam.ARRAYSIZE);
	        updateParamTable();
	    }
	  }
	}

	// set current value as the first value only if not exists.

	if(((ArrayedParam)arrayedParams.elementAt(index)).currentValue.length() == 0) {
	    str = (String) values.elementAt(0);
	    ((ArrayedParam)arrayedParams.elementAt(index)).update(str,CURRENT);
	}

	str = getFirstValue(values);
	((ArrayedParam)arrayedParams.elementAt(index)).update(str,FIRST);

	str = getLastValue(values);
	((ArrayedParam)arrayedParams.elementAt(index)).update(str,LAST);

	//calculate increment or increment style only if not exist.

	if(!((ArrayedParam)arrayedParams.elementAt(index)).origSaved) {

	  if(type.equals("s")) {
            ((ArrayedParam)arrayedParams.elementAt(index)).update("None",INCSTYLE);
            ((ArrayedParam)arrayedParams.elementAt(index)).update("",INCREMENT);
	  } else if(size == 1) {
	    // set increment = 0 when size is 1, so valueTable will be filled
	    // with the same value instead of being empty.
            ((ArrayedParam)arrayedParams.elementAt(index)).update(linearStyle,INCSTYLE);
            ((ArrayedParam)arrayedParams.elementAt(index)).update("0",INCREMENT);
	  } else {

            String style = getIncStyle(values);
            if(style.length() > 0 && !style.equals("None"))
                ((ArrayedParam)arrayedParams.elementAt(index)).update(style,INCSTYLE);

            String inc = getIncrement(values,style);
            if(inc.length() > 0)
                ((ArrayedParam)arrayedParams.elementAt(index)).update(inc,INCREMENT);
	  }

	  ((ArrayedParam)arrayedParams.elementAt(index)).saveOrig();
	  ((ArrayedParam)arrayedParams.elementAt(index)).origSaved = true;
	}
    }

    public void updateEntries(String paramName) {

	int index = getArrayParamIndex(paramName);
        if(index != -1) {

	    setActiveParam(paramName);
	    String str = ((ArrayedParam)arrayedParams.elementAt(index)).currentValue;
	    setCurrentValue(str);
	    int currentRow = currentInArray(str);
	    if(currentRow != -1) valueTable.setRowSelectionInterval(currentRow, currentRow);

	    str = ((ArrayedParam)arrayedParams.elementAt(index)).size;
            setSizeValue(str);
	    str = ((ArrayedParam)arrayedParams.elementAt(index)).firstValue;
            setFirstValue(str);
	    str = ((ArrayedParam)arrayedParams.elementAt(index)).lastValue;
            setLastValue(str);
            str = ((ArrayedParam)arrayedParams.elementAt(index)).incrementStyle;
            setIncStyle(str);
            str = ((ArrayedParam)arrayedParams.elementAt(index)).increment;
            setIncrement(str);
	}
    }


    private void paramTableAction(int row, int col) {

    	String paramName =
		(String) paramTableModel.getValueAt(row, paramTable.PARAMCOL);

	int num = 0;
	String str = (String) paramTableModel.getValueAt(row,paramTable.SIZECOL);
	if(isDigits(str)) num = Float.valueOf(str).intValue();

	String ord = (String) paramTableModel.getValueAt(row, paramTable.ORDERCOL);
	if(ord.length() == 0) ord = "1";

	String onoff = (String) paramTableModel.getValueAt(row,paramTable.ONOFFCOL);
	if(onoff.length() == 0) onoff = "On";

	if(col == paramTable.PARAMCOL) {
	// create array parameter if not exists, otherwise do nothing.
	// should check whether paramName is a valid vnmr parameter and is arrayable.

	    int index = getArrayParamIndex(paramName);
            if(index == -1) {
	        //System.out.println(" create new array parameter.");
            	ArrayedParam newParam = new ArrayedParam(new StringBuffer().
                                                              append(ArrayedParam.NAME ).
                                                              append(":").
                                                              append( paramName).toString());
                newParam.update(ord,ArrayedParam.ARRAYORDER);
            	newParam.update("1",ArrayedParam.ARRAYSIZE);
            	newParam.update("0",ArrayedParam.INCREMENT);
            	newParam.update(linearStyle,ArrayedParam.INCSTYLE);
            	newParam.update(onoff,ArrayedParam.ONOFF);
            	arrayedParams.add(newParam);
            } else if(((ArrayedParam)arrayedParams.elementAt(index)).unArray) {
		((ArrayedParam)arrayedParams.elementAt(index)).unArray = false;
		((ArrayedParam)arrayedParams.elementAt(index)).order = ord;
	    	((ArrayedParam)arrayedParams.elementAt(index)).size = "1";
		((ArrayedParam)arrayedParams.elementAt(index)).increment = "0";
		((ArrayedParam)arrayedParams.elementAt(index)).incrementStyle = linearStyle;
		((ArrayedParam)arrayedParams.elementAt(index)).on_off = onoff;
	    }

	    updateParamTable();
	    setActiveParam(paramName);
	    requestUpdateValueTable(paramName);
    	}

	// for the following, update arrayedParams and vnmrbg
	// only if paramName is not empty

	else if(col == paramTable.ONOFFCOL && paramName.length() > 0) {

	    updateArrayedParam(paramName, onoff, ArrayedParam.ONOFF);

	    if(onoff.equals("On") && (getArraySize(paramName) > 1 || ord.equals("0"))) {

	    //do nothing if arraySize = 1 unless order == 0.
	    //vnmrbg will send back pnew array and {paramName}, that will
	    //update paramTable, valueTable, as well as arrayedParams.

                int index = getArrayParamIndex(paramName);
                if(index != -1) {
                    Vector values =
			((ArrayedParam)arrayedParams.elementAt(index)).values;
                    sendArrayValuesToVnmr(paramName, values);
                }

	    } else if(onoff.equals("Off") && getCurrentValue(paramName).length() > 0) {
	        sendCurrentValueToVnmr(paramName);
	    }
	}
	else if(col == paramTable.ORDERCOL && paramName.length() > 0) {
	    //paramTable's isOrderValid method made sure the change is ok.
	    //so no need to check it here.
	    // update only if paramName and size (num) are valid.
	    paramTable.sortByOrder(paramTable.arrayedParams);
	    paramTable.resizeAndRepaint();
	    // order matters only if array size > 1.
	    if(num > 1) {
	        sendArrayParamsToVnmr();
		setArraySize(getTotalSize());
		requestUpdateTotalTime();
	    }
	}
	int selectedRow = getRowByParamName(paramName);
	if(selectedRow != -1)
	paramTable.setRowSelectionInterval(selectedRow, selectedRow);
    }

    private void valueTableAction(int row, int col) {

	if(col != 1) return;

	savePrevArrayedParams();

	String value = (String) valueTable.getValueAt(row, col);

	String paramName = getActiveParamName();
	int index = getArrayParamIndex(paramName);

	if(index != -1) {
	  if(value.length() > 0) {
	    int i = row+1;
	    sendAValueToVnmr(paramName, value, i);
	  } else if(valueTable.getRowCount() > 1) {
	    valueTable.removeOneRow(row);
 	    Vector valuelist = getTableColumnValues(valueTable, col);
      	    sendArrayValuesToVnmr(paramName,valuelist);
	  }

 	  Vector valuelist = getTableColumnValues(valueTable, col);
	  ((ArrayedParam)arrayedParams.elementAt(index)).updateValues(valuelist);

	  //should the following entries be updated???
/*
	  String type = getArrayedParamValue(paramName, ArrayedParam.TYPE);
	  if(type.equals("s")) {
       	      setIncStyle("None");
	      setIncrement("");
	  } else if(valuelist.size() == 1) {
       	      setIncStyle(linearStyle);
	      setIncrement("0");
	  } else {
       	      setIncStyle(getIncStyle(valuelist));
	      setIncrement(getIncrement(valuelist,getIncStyle()));
	  }
*/
	  setSizeValue(String.valueOf(valuelist.size()));
	  setFirstValue(getFirstValue(valuelist));
	  setLastValue(getLastValue(valuelist));

	  updateArrayedParam(paramName, getIncrement(), ArrayedParam.INCREMENT);
	  updateArrayedParam(paramName, getIncStyle(), ArrayedParam.INCSTYLE);
	  updateArrayedParam(paramName, getSizeValue(), ArrayedParam.ARRAYSIZE);
	  updateArrayedParam(paramName, getFirstValue(), ArrayedParam.FIRST);
	  updateArrayedParam(paramName, getLastValue(), ArrayedParam.LAST);
	}
    }

    private void joinNewExp() {

	arrayedParams.clear();
	readARRAYEDfromCurpar();
	requestUpdateArrayParamTable();
	saveOrigCurparArrayedParams();
    }

    public void sendValueToVnmr(String paramName, String value) {

	if(value == null) return;

	int index = getArrayParamIndex(paramName);
        if(index == -1) return;

	String type = getArrayedParamValue(paramName, ArrayedParam.TYPE);
	if(!type.equals("s") && value.length() == 0) return;

    	String cmd;
	if(type.equals("s")) cmd =new StringBuffer().append( paramName ).append("='").
                                   append( value ).append("'").toString();
	else cmd =new StringBuffer().append( paramName ).append("=").append( value).toString();
	vsendCmd.setAttribute(CMD, cmd);
        vsendCmd.updateValue();
    }

    public void sendAValueToVnmr(String paramName, String value, int i) {

	int index = getArrayParamIndex(paramName);
        if(index == -1) return;

	String type = getArrayedParamValue(paramName, ArrayedParam.TYPE);
	if(!type.equals("s") && value.length() == 0) return;

	String cmd;
        if(type.equals("s")) cmd =new StringBuffer().append( paramName ).
                                       append("[").append( i ).append("]='").
                                       append( value ).append("'").toString();
        else cmd =new StringBuffer().append( paramName ).append("[").append( i ).
                       append("]=").append( value).toString();
        vsendCmd.setAttribute(CMD, cmd);
        vsendCmd.updateValue();
    }

    private void currentTextAction() {

        String paramName = getActiveParamName();
	String current = getCurrentValue();
	String prev = getArrayedParamValue(paramName, ArrayedParam.CURRENT);
	if(prev == null || prev.equals(current)) return;

        String ord = getValueByParamNameAndCol(paramName, paramTable.ORDERCOL);
        if(ord.equals("0")) savePrevArrayedParams();
	updateArrayedParam(paramName, current, ArrayedParam.FIRST);
        updateArrayedParam(paramName, current, ArrayedParam.CURRENT);
        if(ord.equals("0")) sendValueToVnmr(paramName, current);
    }

    private void firstTextAction() {

        String paramName = getActiveParamName();
	String current = getFirstValue();
	String prev = getArrayedParamValue(paramName, ArrayedParam.FIRST);
	if(prev == null || prev.equals(current)) return;

	savePrevArrayedParams();

	updateArrayedParam(paramName, current, ArrayedParam.FIRST);
	String ord = getValueByParamNameAndCol(paramName, paramTable.ORDERCOL);
	String type = getArrayedParamValue(paramName, ArrayedParam.TYPE);

	if(ord.equals("0")) sendValueToVnmr(paramName, current);
	else if(type.equals("s")) sendAValueToVnmr(paramName, current, 1);
	else requestVnmrbgCalArrayValues(paramName);

	// arrayed values will be updated when vnmrbg sends back the values
	// and updateValueTable is called.
    }

    private void incTextAction() {

	String paramName = getActiveParamName();
	String current = getIncrement();
	String prev = getArrayedParamValue(paramName, ArrayedParam.INCREMENT);
	if(prev == null || prev.equals(current)) return;

	savePrevArrayedParams();

	String ord = getValueByParamNameAndCol(paramName, paramTable.ORDERCOL);
	if(!ord.equals("0")) {
	    updateArrayedParam(paramName, current, ArrayedParam.INCREMENT);
	    requestVnmrbgCalArrayValues(paramName);
        }
    }

    private void lastTextAction() {

	String paramName = getActiveParamName();
	String current = getLastValue();
	String prev = getArrayedParamValue(paramName, ArrayedParam.LAST);
	if(prev == null || prev.equals(current)) return;

	savePrevArrayedParams();

	updateArrayedParam(paramName, current, ArrayedParam.LAST);
	String ord = getValueByParamNameAndCol(paramName, paramTable.ORDERCOL);
	String type = getArrayedParamValue(paramName, ArrayedParam.TYPE);
	int size = getArraySize(paramName);

	if(ord.equals("0")) sendValueToVnmr(paramName, current);
	else if(type.equals("s")) sendAValueToVnmr(paramName, current, size);
	else recalFirstOrIncrementWhenLastValueChanged();
    }

    public void sendStringValuesToVnmr(String paramName, int num) {

	int index = getArrayParamIndex(paramName);
	if(index == -1) return;

	Vector v = ((ArrayedParam)arrayedParams.elementAt(index)).values;
        if(v == null) return;

	if(v.size() == 0) v.add(((ArrayedParam)arrayedParams.elementAt(index)).currentValue);

        StringBuffer sbData = new StringBuffer();
        String values = "";

	int size = v.size();
	if(num < size) size = num;
        for(int i=0; i<size; i++) {
                sbData.append("'").append((String) v.elementAt(i)).append("',");
        }
        values = sbData.toString();
	if(num > size) for(int i=size; i<num; i++)
		sbData.append("'").append((String) v.elementAt(size-1)).append("',");
        values = sbData.toString();
        if(values.endsWith(","))
            values = values.substring(0,values.length()-1);

        if(!values.endsWith(",") && values.indexOf(",,") == -1)
        sendArrayValuesToVnmr(paramName, values);

    }

    protected void sendOrigToVnmr() {

        Vector params = new Vector();

	for(int i=arrayedParams.size()-1; i>-1; i--) {

	    String paramName = ((ArrayedParam)arrayedParams.elementAt(i)).paramName;

	    if(((ArrayedParam)arrayedParams.elementAt(i)).newArray) {

		((ArrayedParam)arrayedParams.elementAt(i)).unArray = true;
                ((ArrayedParam)arrayedParams.elementAt(i)).copyOrig();
		String value = ((ArrayedParam)arrayedParams.elementAt(i)).currentValue;

		sendValueToVnmr(paramName, value);

	    } else {

		((ArrayedParam)arrayedParams.elementAt(i)).unArray = false;

		String size = ((ArrayedParam)arrayedParams.elementAt(i)).origSize;
                int sizeInt = 0;
                if(isDigits(size)) sizeInt = Float.valueOf(size).intValue();

	        if(sizeInt > 1 &&
		    ((ArrayedParam)arrayedParams.elementAt(i)).origValuesChanged()) {

                    ((ArrayedParam)arrayedParams.elementAt(i)).copyOrig();
		    params.add(((ArrayedParam)arrayedParams.elementAt(i)).makeRowForParamTable());
		    Vector values = ((ArrayedParam)arrayedParams.elementAt(i)).values;
		    sendArrayValuesToVnmr(paramName, values);

	        } else if(sizeInt == 1 &&
		    ((ArrayedParam)arrayedParams.elementAt(i)).origCurrentValueChanged()) {

                    ((ArrayedParam)arrayedParams.elementAt(i)).copyOrig();
		    String value = ((ArrayedParam)arrayedParams.elementAt(i)).currentValue;
		    sendValueToVnmr(paramName, value);
		} else if(sizeInt > 1)
		    params.add(((ArrayedParam)arrayedParams.elementAt(i)).makeRowForParamTable());
	    }
	}

	if(params.size() > 0) {
	    paramTable.sortByOrder(params);
	    sendOrderToVnmr(params);
	}
    }

    protected void sendPrevToVnmr() {

	Vector params = new Vector();

	for(int i=arrayedParams.size()-1; i>-1; i--) {

	    if(((ArrayedParam)arrayedParams.elementAt(i)).prevChanged()) {

		((ArrayedParam)arrayedParams.elementAt(i)).unArray = false;
                ((ArrayedParam)arrayedParams.elementAt(i)).copyPrev();
		params.add(((ArrayedParam)arrayedParams.elementAt(i)).makeRowForParamTable());

	        String paramName = ((ArrayedParam)arrayedParams.elementAt(i)).paramName;
		String size = ((ArrayedParam)arrayedParams.elementAt(i)).size;
                int sizeInt = 0;
                if(isDigits(size)) sizeInt = Float.valueOf(size).intValue();

	        if(sizeInt > 1) {
		    Vector values = ((ArrayedParam)arrayedParams.elementAt(i)).values;
		    sendArrayValuesToVnmr(paramName, values);

	        } else if(sizeInt == 1) {

		    String value = ((ArrayedParam)arrayedParams.elementAt(i)).currentValue;
		    sendValueToVnmr(paramName, value);
		}

	    } else if(!((ArrayedParam)arrayedParams.elementAt(i)).unArray)
		params.add(((ArrayedParam)arrayedParams.elementAt(i)).makeRowForParamTable());
	}

	if(params.size() > 0) {
	    paramTable.sortByOrder(params);
	    sendOrderToVnmr(params);
	}
    }

    public void sendOrderToVnmr(Vector v) {

	String str = "";
    StringBuffer sbData = new StringBuffer();
    int nSize = v.size();
	for(int i=0; i<nSize; i++) {
	    Vector row = (Vector) v.elementAt(i);
	    if(((String) row.elementAt(paramTable.ONOFFCOL)).equals("On")) {
	        String name = (String) row.elementAt(paramTable.PARAMCOL);
	        String order = (String) row.elementAt(paramTable.ORDERCOL);
	        if(isDigits(order) && !order.equals("0") && name.length() > 0)
		    sbData.append(name).append(" ").append(order).append(" ");
	    }
	}
    str = sbData.toString();

	if(str.endsWith(" "))
            str = str.substring(0,str.length()-1);

        sendArrayParamsToVnmr(str);
    }

    public void arraysizeTextAction() {

        String paramName = getActiveParamName();
        String current = getSizeValue();
        String prev = getArrayedParamValue(paramName, ArrayedParam.ARRAYSIZE);
        if(prev == null || prev.equals(current)) return;

        savePrevArrayedParams();

        String ord = getArrayedParamValue(paramName, ArrayedParam.ARRAYORDER);
	if(ord.equals("0")) {
            int index = getArrayParamIndex(paramName);
            if(index != -1) {
		Vector values = new Vector();
		values.add(current);
		((ArrayedParam)arrayedParams.elementAt(index)).updateValues(values);
	        updateArrayedParam(paramName, current, ArrayedParam.CURRENT);
	        updateArrayedParam(paramName, current, ArrayedParam.ARRAYSIZE);
	    }
	    sendValueToVnmr(paramName, current);
	} else {
	    updateArrayedParam(paramName, current, ArrayedParam.ARRAYSIZE);

	    int num = 1;
	    if(isDigits(current)) num = Float.valueOf(current).intValue();

	    if(num > 1) {
	        String type = getArrayedParamValue(paramName, ArrayedParam.TYPE);
	        if(type.equals("s")) sendStringValuesToVnmr(paramName, num);
	        else requestVnmrbgCalArrayValues(paramName);
	    } else if(getCurrentValue(paramName).length() > 0)
                    sendCurrentValueToVnmr(paramName);
	}
    }

    public void setSizeValue (String value) {
	arraysizeText.setText(value);
    }

    public String getSizeValue() {
	return (arraysizeText.getText());
    }

    public void setVnmrIF(ButtonIF vif) {

        vnmrIf = vif;        
        vArrayParams.setVnmrIF(vif);
        vArrayValues.setVnmrIF(vif);
        vsendCmd.setVnmrIF(vif);
        vtotalTime.setVnmrIF(vif);

        joinNewExp();
    }
}


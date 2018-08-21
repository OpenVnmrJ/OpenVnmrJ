/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import java.util.*;
import java.io.*;

import vnmr.ui.*;
import vnmr.ui.shuf.*;
import vnmr.bo.*;
import vnmr.util.*;

import org.w3c.dom.*;
import com.sun.xml.tree.*;
import javax.swing.tree.*;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;

/**
 * A Dialog class for editing Protocols
 *  @author		Dean Sindorf
 */
public class ProtocolEditor extends ModelessDialog
    implements VObjDef, ActionListener,TreeSelectionListener
{
    String 				itemName="";
    ProtocolBuilder 	mgr;
    JTree            	jtree=null;
    VElement 			selected=null;
    JPanel 				panel=null;
    JPanel 				action_panel=null;
    JPanel 				protocol_panel=null;
    JPanel 				template_panel=null;
	Hashtable        	keys=null;
	Hashtable        	action_keys=new Hashtable();
	Hashtable        	protocol_keys=new Hashtable();
	Hashtable        	template_keys=new Hashtable();
    boolean      		expand_tree=true;
    JButton 			save_button;
    JScrollPane         panel_scroll;
    String              filename="NewProtocol";
    String              title="New Protocol";

    public boolean      test=false;
	static public String AUTHOR 	= "author";
 	static public String FILE 	    = "file";
 	static public String TYPE 	    = "type";
 	static public String NAME 	    = "name";

    public ProtocolEditor(String helpFile) {
        super("Protocol Editor");
        m_strHelpFile = helpFile;

        mgr=new ProtocolBuilder(this);
        mgr.newTree();
        mgr.setExpandNew(expand_tree);
        mgr.setTimeShown(false);
        mgr.setShowIcons(false);

        Dimension screenDim = Toolkit.getDefaultToolkit().getScreenSize();
        if (screenDim.width > 1000)
            setLocation(500, 200);
        else
            setLocation(400, 300);

       	// build the panel

		Container frame=getContentPane();

		JPanel content=new JPanel();

		SimpleH2Layout xLayout=new SimpleH2Layout(SimpleH2Layout.LEFT, 10, 0,false);
		content.setLayout(xLayout);

		jtree=mgr.buildTreePanel();
		jtree.addTreeSelectionListener(this);

 	    keys=action_keys;
 	    action_panel=buildPanel("ProtocolEditorActions.xml");
		if(action_panel==null)
		    return;
 	    keys=protocol_keys;
 	    protocol_panel=buildPanel("ProtocolEditorProtocols.xml");
		if(protocol_panel==null)
		    return;

 	    keys=template_keys;
 	    template_panel=buildPanel("ProtocolEditorTemplate.xml");
		if(template_panel==null)
		    return;
		panel=action_panel;

		JSplitPane splitpane=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
		splitpane.setBorder (BorderFactory.createEmptyBorder (5, 5, 5, 5));
		splitpane.setLeftComponent(new JScrollPane (jtree));

		panel_scroll=new JScrollPane (panel);
		splitpane.setRightComponent (panel_scroll);
		splitpane.setDividerLocation (200);

		splitpane.setPreferredSize (new Dimension (520, 250));
		content.add(splitpane);

		JPanel controls=new JPanel(new GridLayout(8,1));
		controls.setPreferredSize (new Dimension (80, 250));

		save_button=new JButton("Save");
		save_button.addActionListener(this);
		save_button.setEnabled(false);
		controls.add(save_button);

		JButton button=new JButton("File");
		button.addActionListener(this);
		controls.add(button);

		button=new JButton("Delete");
		button.addActionListener(this);
		controls.add(button);

		button=new JButton("New");
		button.addActionListener(this);
		controls.add(button);

		content.add(controls);

		frame.add(content, BorderLayout.CENTER);
		frame.validate();

        helpButton.setActionCommand("help");
        helpButton.addActionListener(this);
        helpButton.setEnabled(false);

        historyButton.setActionCommand("history");
        historyButton.addActionListener(this);
        historyButton.setEnabled(false);

        undoButton.setActionCommand("undo");
        undoButton.setEnabled(false);
        undoButton.addActionListener(this);

        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);

        abandonButton.setActionCommand("abandon");
        abandonButton.addActionListener(this);

		setSize(650,300);
		pack();
		setBgColor(getContentPane().getBackground());
		setVisible(true);
    }

   	//----------------------------------------------------------------
	/** get attribute keys from panel */
	//----------------------------------------------------------------
  	 private JPanel buildPanel (String file) {
  	     JPanel pnl=new JPanel();
	     pnl.setLayout(new TemplateLayout());
  	     String xfile=FileUtil.openPath("INTERFACE/"+file);
  	 	 LayoutBuilder builder=new LayoutBuilder(pnl,Util.getDefaultExp());
	     try {
		     builder.open(xfile);
	     }
	     catch(Exception e) {
             Messages.postError("ProtocolEditor: could not open "+file);
             return null;
	     }

      	 ElementTree etree=builder.getTree();
         VElement elem;
		 while((elem=etree.nextAttribute("key")) != null){
		    String key=elem.getAttribute("key");
    		if(elem instanceof VObjElement){
     		    VObjElement obj=(VObjElement)elem;
   				keys.put(key,obj);
   				VObjIF vcomp=obj.vcomp();
    			if(vcomp !=null && vcomp instanceof VEntry){
    		    	((VEntry)vcomp).addActionListener(this);
    		    	((VEntry)vcomp).setActionCommand("apply");
    		    }
    		}
    	 }
    	 return pnl;
 	 }


   	//----------------------------------------------------------------
	/** return ProtocolBuilder  */
	//----------------------------------------------------------------
	public ProtocolBuilder getMgr() {
        return mgr;
	}

   	//----------------------------------------------------------------
	/** return JTree  */
	//----------------------------------------------------------------
	public JTree getTree() {
        return jtree;
	}

   	//----------------------------------------------------------------
	/** show dialog  */
	//----------------------------------------------------------------
	public void setVisible() {
		setVisible(true);
        SessionShare sshare=Util.getSessionShare();
		if (sshare != null ) {
        	LocatorHistoryList lhl = sshare.getLocatorHistoryList();
	    	// Set History Active Object type to this type.
	    	// (locator_statements_protocols.xml)
	    	lhl.setLocatorHistory(LocatorHistoryList.PROTOCOLS_LH);
	    }
	}

	//----------------------------------------------------------------
	/** restore previous locator state  */
	//----------------------------------------------------------------
	private void restoreLocatorHistory()
	{
		SessionShare sshare=Util.getSessionShare();
		if (sshare != null ) {
            LocatorHistoryList lhl = sshare.getLocatorHistoryList();
            if(lhl != null)
                lhl.setLocatorHistoryToPrev();
	    }
	}

	//----------------------------------------------------------------
	/** handler for button events  */
	//----------------------------------------------------------------
	public void actionPerformed(ActionEvent ae) {
		String cmd=ae.getActionCommand();
		//System.out.println(cmd);
		if(cmd.equals("close")){
		    setVisible(false);
            dispose();
            if(test)
				System.exit(0);
			restoreLocatorHistory();
		}
        else if(cmd.equals("abandon"))  {
            setVisible(false);
            dispose();
            if(test)
				System.exit(0);
			restoreLocatorHistory();
        }
  		else if(cmd.equals("apply")){
		    updateTreeNode(selected);
		}
      	else if(cmd.equals("Save")){
       		saveTree();
        }
       	else if(cmd.equals("New")){
         	mgr.clearTree();
			mgr.selectElement(mgr.rootElement());
        }
       	else if(cmd.equals("File")){
       	    mgr.selectElement(mgr.rootElement());
        }
       	else if(cmd.equals("Delete")){
         	mgr.removeSelected();
        }
        else if (cmd.equals("help"))
            displayHelp();
 	}


   	//----------------------------------------------------------------
	/** save tree  */
	//----------------------------------------------------------------
	public void saveTree() {
		if(mgr.isEmpty())
		    return;
        VElement root=mgr.rootElement();
        updateTreeNode(root);
		filename=root.getAttribute(FILE);
		String fn=filename+".xml";

        ExpPanel exp = Util.getDefaultExp();
        String dir=dir=FileUtil.savePath("PROTOCOLS/"+fn);
  	    String user=root.getAttribute(AUTHOR);

		//System.out.println("save "+title+"  "+filename);

        mgr.save(dir);
        if(exp !=null)
            exp.addToShuffler(Shuf.DB_PROTOCOL, fn, dir, user, true);
	}
   	//----------------------------------------------------------------
	/** initialize template panel  */
	//----------------------------------------------------------------
	public void setTemplatePanel(VElement root) {
		if(selected !=null)
		    updateTreeNode(selected);

  	    if(panel!=template_panel){
  	        panel=template_panel;
  	        panel_scroll.setViewportView(panel);
    	    keys=template_keys;
   	        save_button.setEnabled(true);
 	    }

		selected=mgr.rootElement();

       	VObjIF elem=(VObjIF)keys.get(FILE);
		if(elem != null)
			 elem.setAttribute(KEYVAL,filename);
	    elem=(VObjIF)keys.get(AUTHOR);
		if(elem != null)
			 elem.setAttribute(KEYVAL,System.getProperty("user.name"));
	    elem=(VObjIF)keys.get(NAME);
		if(elem != null)
			 elem.setAttribute(KEYVAL,title);
	    elem=(VObjIF)keys.get(TYPE);
		if(elem != null){
			VElement etype=mgr.firstElement();
			if(etype !=null){
				String str;
			    if(mgr.isAction(etype))
					str="actions";
			    else
					str="protocols";
			    elem.setAttribute(KEYVAL,str);
			}
		}
		mgr.selectElement(selected);
	}

   	//----------------------------------------------------------------
	/** set save name text  */
	//----------------------------------------------------------------
	public void setTitle(VElement obj) {
	    if(obj==null)
	        return;

	    filename="new Protocol";

	    String file=obj.getAttribute(FILE);
	    String name=obj.getAttribute(NAME);

		if(name==null || name.length()==0)
		    title="newProtocol";
		else
	        title=name;

		if(file==null || file.length()==0){
	    	filename="";
	    	StringTokenizer tok=new StringTokenizer(name);
	    	while(tok.hasMoreTokens()){
	        	String s=tok.nextToken();
	        	String a=s.substring(0,1);
	        	a=a.toUpperCase();
	        	filename+=a;
	        	String b=s.substring(1);
	        	filename+=b;
	    	}
		}
		else
		    filename=file;

	    if(template_panel !=null)
	    	setTemplatePanel(obj);
	}

    //----------------------------------------------------------------
	/** tree listener  */
	//----------------------------------------------------------------
	public void valueChanged (TreeSelectionEvent e) {
		TreePath    path;
 		VElement    elem;

		if(selected !=null)
		    updateTreeNode(selected);

		path = jtree.getSelectionPath();

		if (path == null)
			return;

		elem = (VElement)path.getLastPathComponent ();
		if (elem !=selected){
			selected=elem;
		    updatePanel(selected);
		}
	 }

   	//----------------------------------------------------------------
	/** update Editor panel from treenode */
	//----------------------------------------------------------------
  	 private void updatePanel (VElement obj) {
  	    JPanel newpnl=null;
  	    if(mgr.isProtocol(obj)){
  	        newpnl=protocol_panel;
  	        keys=protocol_keys;
   	        save_button.setEnabled(false);
  	    }
  	    else if(mgr.isAction(obj)){
  	        newpnl=action_panel;
  	        keys=action_keys;
   	        save_button.setEnabled(false);
  	    }
  	    else{
  	        newpnl=template_panel;
   	        keys=template_keys;
   	        save_button.setEnabled(true);
 	    }
  	    if(newpnl != panel){
  	        panel=newpnl;
  	        panel_scroll.setViewportView(panel);
  	    }

   	    Enumeration elist=keys.keys();
   	    while(elist.hasMoreElements()){
  	        String key=(String)elist.nextElement();
    		VObjIF elem=(VObjIF)keys.get(key);
    		String val=obj.getAttribute(key);
    		if(val !=null && val.length()>0)
    		    elem.setAttribute(KEYVAL,val);
			else
    		    elem.setAttribute(KEYVAL,"");
  	    }
	 }

   	//----------------------------------------------------------------
	/** update treenode from Editor panel  */
	//----------------------------------------------------------------
  	 public void updateTreeNode (VElement obj) {
  	 	if(obj==null)
  	 		return;
  	    Enumeration elist=keys.keys();
  	    String key;
  	    while(elist.hasMoreElements()){
  	        key=(String)elist.nextElement();
    		VObjIF elem=(VObjIF)keys.get(key);
  	        obj.setAttribute(key,elem.getAttribute(KEYVAL));
  	    }
  	    if(obj != mgr.rootElement()){
  	        DefaultTreeModel model= (DefaultTreeModel)jtree.getModel();
  	        model.nodeChanged(selected);
 	    }
 	 }
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.*;
import java.util.*;
import java.awt.*;
import javax.swing.*;
import java.beans.*;
import java.awt.event.*;

import vnmr.bo.*;
import vnmr.util.*;
import vnmr.templates.*;
import vnmr.admin.vobj.*;
import vnmr.admin.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public class WFontColors extends ModelessDialog implements ActionListener
{

    protected static WFontColors m_objFontColors = null;
    protected static ArrayList   m_aListTypes=new ArrayList();
    protected static HashArrayList m_haVars = new HashArrayList();
    private static String[]   m_aStrLibs=null;
    protected static JPanel m_pnlDisplay = new JPanel();
    protected static boolean m_bBuilt = false;
    protected static PropertyChangeSupport m_pcsTypesMgr = null;

    public static final String  XMLFILE="INTERFACE/AdminOptions.xml"; // xml file


    public WFontColors()
    {
        super(vnmr.util.Util.getLabel("_admin_Edit_Background_Colors"));
        // dolayout();
        m_pcsTypesMgr=new PropertyChangeSupport(this);

        m_aStrLibs=new String[4];
        m_aListTypes.add(DisplayOptions.FONT);
        m_aListTypes.add(DisplayOptions.STYLE);
        m_aListTypes.add(DisplayOptions.SIZE);
        m_aListTypes.add(DisplayOptions.COLOR);

        readPersistence();
        buildXml();
        dolayout();
    }

    public void setVisible(boolean bVis)
    {
        if (!m_bBuilt && bVis)
        {
            buildXml();
        }
        super.setVisible(bVis);
    }

    public static void firePropertyChng()
    {
        m_pcsTypesMgr.firePropertyChange(WGlobal.FONTSCOLORS, "all", "update");
    }

    protected void buildXml()
    {
        try
        {
            LayoutBuilder.build(m_pnlDisplay, null, FileUtil.openPath(XMLFILE));
            m_bBuilt = true;
            getVariables(m_pnlDisplay);
        }
        catch(Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    /** return menu list of show catagories */
    public static String[] getShowTypes()
    {
        String string;
        m_aStrLibs[DisplayOptions.SYSTEM]=Util.getLabel("dlSystem","System");
        m_aStrLibs[DisplayOptions.SELSYS]=Util.getLabel("dlSelected","Selected");
        m_aStrLibs[DisplayOptions.VNMR]=Util.getLabel("dlVnmr","Vnmr");
        m_aStrLibs[DisplayOptions.ALL]=Util.getLabel("dlAll","All");
        return m_aStrLibs;
    }

    protected void dolayout()
    {
        JScrollPane spDisplay = new JScrollPane(m_pnlDisplay);
        m_pnlDisplay.setLayout(new WGridLayout(0, 1));
        getContentPane().add(spDisplay);

        // historyButton.setEnabled(false);
        // helpButton.setEnabled(false);
        // undoButton.setEnabled(false);
        setCloseEnabled(true);
        setAbandonEnabled(true);

        closeButton.setActionCommand("close");
        closeButton.addActionListener(this);
        abandonButton.setActionCommand("cancel");
        abandonButton.addActionListener(this);

        setResizable(true);

        setLocation( 300, 500 );
        Dimension dim1 = m_pnlDisplay.getPreferredSize();
        Dimension dim2 = buttonPane.getPreferredSize();
        
        int nWidth = dim1.width + 20;
        int nHeight = dim1.height + dim2.height + 40;
        if (nWidth < dim2.width)
            nWidth = dim2.width;
        if (nWidth < 300)
            nWidth = 300;
        if (nHeight < 300)
            nHeight = 300;
       // setSize(nWidth, 500);
        setSize(nWidth, nHeight);
    }

    public void actionPerformed(ActionEvent e)
    {
        String cmd = e.getActionCommand();
        if(cmd.equals("close"))
        {
            // run the script to save data
            saveData();
            setVisible(false);
            dispose();
        }
        else if (cmd.equals("cancel"))
        {
            setVisible(false);
            dispose();
        }
    }

    /** return a VJ Color */
    public static Color getColor(String strName)
    {
       String strValue = getOption(DisplayOptions.COLOR,strName);
       if(strValue!=null)
           strName=strValue;
       return VnmrRgb.getColorByName(strName);
    }

    /** return Option string for keyword attr (or null)  */
    public static String getOption(String attr, String strName)
    {
        if(strName==null)
            return null;
        DVar var=(DVar)m_haVars.get(strName+attr);
        if(var !=null)
        {
            return var.value;
        }
        return null;
    }

    /** return list of system variables for type keyword  */
    public static ArrayList getTypes(String attr)
    {
        ArrayList list=new ArrayList();
        for(int i=0;i<m_haVars.size();i++)
        {
            String key=(String)m_haVars.getKey(i);
            DVar var=(DVar)m_haVars.get(key);
            if(var.type.equals(attr))
                list.add(var.family);
        }
        return list;
    }

    public static String getColorOption(String strName)
    {
        String strColor = null;

        if (strName == null)
            return null;

        DVar var = (DVar)m_haVars.get(strName);
        if (var != null)
            strColor = var.value;

        return strColor;
    }

    public static void setColorOption(String strAttr, String strValue)
    {
        if (strValue == null)
            return;

        DVar var = (DVar)m_haVars.get(strAttr);
        if (var != null)
            var.value = strValue;
        writePersistence();
    }

    /** read persistence file in ~/vnmrsys/persistence */
    public void readPersistence()
    {
        BufferedReader in;
        String line;
        String symbol;
        String type;
        String value;
        StringTokenizer tok;

        String strPath = FileUtil.openPath(WGlobal.OPT_PER_FILE);
        in = WFileUtil.openReadFile(strPath);

        if (strPath == null)
        {
            return;
        }

        try
        {
            while ((line = in.readLine()) != null)
            {
                tok = new StringTokenizer(line, " \t");
                if (!tok.hasMoreTokens())
                    continue;
                symbol = tok.nextToken();
                if (!tok.hasMoreTokens())
                    continue;
                type=optionType(symbol);
                if(type==null)
                    continue;
                if (!tok.hasMoreTokens())
                    continue;

                value = tok.nextToken();
                DVar var=new DVar(symbol,type,value);
                m_haVars.put(symbol,var);
            }
            in.close();

        }
        catch (Exception e)
        {
            //e.printStackTrace();
            Messages.writeStackTrace(e);
            Messages.postDebug(e.toString());
        }
    }

    /** write persistence file in ~/vnmrsys/persistence */
    public static void writePersistence()
    {
        String strPath = FileUtil.openPath(WGlobal.OPT_PER_FILE);
        if(strPath==null)
            strPath = FileUtil.savePath(WGlobal.OPT_PER_FILE);

        Messages.postDebug("WFontColors.writePersistence");

        PrintWriter os;
        try
        {
            //getVariables(m_pnlDisplay);
            os = new PrintWriter(new FileWriter(strPath));
            for(int i=0;i<m_haVars.size();i++)
            {
                String key=(String)m_haVars.getKey(i);
                DVar var=(DVar)m_haVars.get(key);
                if (var != null)
                {
                    os.println(var.toString());
                }
            }

            os.close();
        }
        catch(IOException er)
        {
            er.printStackTrace();
        }
    }

    /** get type (e.g."SmallTitle" from symbol "SmallTitleStyle") */
    public static String optionFamily(String symbol)
    {
        for(int i=0;i<m_aListTypes.size();i++)
        {
            String str=(String)m_aListTypes.get(i);
            int indx=symbol.indexOf(str);
            if(indx>=0)
                return symbol.substring(0,indx);
        }
        return null;
    }

    /** get type (e.g."Style" from symbol "SmallTitleStyle") */
    public static String optionType(String symbol)
    {
        for(int i=0;i<m_aListTypes.size();i++)
        {
            String str=(String)m_aListTypes.get(i);
            int indx=symbol.indexOf(str);
            if(indx>=0)
                return str;
        }
        return null;
    }

    protected void saveData()
    {
        getVariables(m_pnlDisplay);
        writePersistence();
        m_pcsTypesMgr.firePropertyChange(WGlobal.FONTSCOLORS, "all", "update");
    }

    protected void writeXMLFile()
    {
        String strXmlPath = FileUtil.openPath(XMLFILE);
        if (strXmlPath != null)
        {
            try
            {
                LayoutBuilder.writeToFile(m_pnlDisplay, strXmlPath);
            }
            catch(Exception e)
            {
                e.printStackTrace();
            }
        }
    }

    /** identify variable container objects  */
    private void  getVariables(JComponent parent)
    {
        for (int i=0; i<parent.getComponentCount(); i++)
        {
            Component comp = parent.getComponent(i);
            if (comp instanceof VObjIF)
            {
                VObjIF obj = (VObjIF) comp;
                String symbol=obj.getAttribute(VObjDef.KEYSTR);
                if(symbol!=null)
                {
                    String strValue=obj.getAttribute(VObjDef.KEYVAL);
                    DVar var=(DVar)m_haVars.get(symbol);
                    if(var != null)
                    {
                        var.value = strValue;
                    }
                    else
                    {
                        String type=optionType(symbol);
                        if(type !=null)
                        {
                            //newvars=true;
                            // Messages.postDebug("Adding Style Variable: "+symbol+" "+strValue);
                            var=new DVar(symbol,type,strValue);
                            var.obj=obj;
                            m_haVars.put(symbol,var);
                        }
                    }
                }
                if(obj instanceof WGroup || obj instanceof VTabbedPane)
                    getVariables((JComponent)obj);
            }
        }
    }

    //==============================================================================
   //   PropertyChange methods follows ...
   //============================================================================

    public static void addChangeListener(PropertyChangeListener l){
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(l);
    }

    public static void addChangeListener(String strProperty, PropertyChangeListener l)
    {
        if (m_pcsTypesMgr != null)
            m_pcsTypesMgr.addPropertyChangeListener(strProperty, l);
    }

    public static void removeChangeListener(PropertyChangeListener l){
        m_pcsTypesMgr.removePropertyChangeListener(l);
    }

    // inner class DVar

    protected class DVar
    {
         protected String symbol;
         protected String family;
         protected String type;
         protected String value;
         protected VObjIF obj;

         public DVar(String s, String t,String v)
         {
            family=optionFamily(s);
            symbol=s;
            type=t;
            value=v;
            obj=null;
         }
         public String toString()
         {
            return symbol+" "+value;
         }
     }

}

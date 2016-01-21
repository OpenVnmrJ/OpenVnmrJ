/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.templates;

import vnmr.util.*;

import java.awt.*;
import javax.swing.*;
import javax.swing.border.*;
import java.awt.event.*;
import java.util.*;
import java.io.*;

import org.xml.sax.ErrorHandler;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

import org.w3c.dom.*;
import com.sun.xml.tree.*;
import com.sun.xml.parser.Resolver;
import com.sun.xml.parser.ValidatingParser;

/**
 * A manager class for XML based templates
 * includes Template methods that do not require a GUI
 * @author Dean Sindorf
 */
public class Template implements Types {
    private XmlDocumentBuilder builder;
    private Properties xmlkeys;
    private SimpleElementFactory factory;
    private ValidatingParser parser;

    protected XmlDocument doc=null;

    protected String dtdfile=null;
    protected String keyfile=null;

    public String docname="template";

    /** debug element initialization */
    public static boolean  debug_init=false;
    /** debug element selection */
    public static boolean  debug_select=false;
    /** debug element action listeners */
    public static boolean  debug_actions=false;
    private VElement selected=null;

    //+++++++++++++ public methods +++++++++++++++++++++++++++++++++++

    //----------------------------------------------------------------
    /** <pre>Constructor for xml Template parser components (no swing).
     *
     *  Java command line (-D) argument options
     *  name        value    function
     *  -------------------------------------------------------------
     *  xmlkey path     sets an external class bindings file
     *  xmldtd path     sets an external dtd file
     *
     *  e.g. -Ddtd=/vnmr/xml/templates.dtd
     * </pre>
     */
    //----------------------------------------------------------------
    public Template() {
        keyfile=System.getProperty("xmlkey");
        dtdfile=System.getProperty("xmldtd");
        init_xml();
    }

    //----------------------------------------------------------------
    /** <pre>Set default class bindings for xml keys.
     *
     *  - Called only by Template constructor.
     *  - To create XML parsers for different document types extend this
     *    class or use the "xmlkey" command line argument to use an
     *    external properties file.
     *
     *  - Add a statement of the following type for each binding:
     *    setKey(String key, String classname);
     *
     *   note: extended method "setKey" calls override base method calls
     *         for the same key string.
     *
     *  <B>Examples of usage</B>
     *   setKey("*Element", "vnmr.templates.VElement");  <I>default binding</I>
     *
     *   <B>this (base) method sets the following bindings</B>
     *   -------------------------------------------------
     *   key         classname
     *   -------------------------------------------------
     *   *Element    vnmr.templates.VElement     <I>default</I>
     *   template    vnmr.templates.VElement
     *   ref         vnmr.templates.Reference
     * </pre>
     */
    //----------------------------------------------------------------
    protected void setDefaultKeys(){
        setKey("*Element", vnmr.templates.VElement.class);
        setKey("template", vnmr.templates.VElement.class);
        setKey("ref",     vnmr.templates.Reference.class);
    }

    //----------------------------------------------------------------
    /** Set a xml document key=classname binding relationship. */
    //----------------------------------------------------------------
    protected void setKey(String key, String classname){
        xmlkeys.setProperty(key, classname);
    }

    protected void setKey(String key, Class cls){
        xmlkeys.setProperty(key, cls.getName());
    }


    //~~~~~~~~~~~~~~~~ document I/O ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //----------------------------------------------------------------
    /** Return current document. */
    //----------------------------------------------------------------
    public XmlDocument getDocument() {
        return doc;
    }

    //----------------------------------------------------------------
    /** Set current document. */
    //----------------------------------------------------------------
    public void setDocument(XmlDocument xdoc) {
        doc=xdoc;
    }

    //----------------------------------------------------------------
    /** Return true if template has no active document. */
    //----------------------------------------------------------------
    public boolean noDocument(){
        return doc==null ? true : false;
    }

    //----------------------------------------------------------------
    /** Return true if Template has an active document. */
    //----------------------------------------------------------------
    public boolean hasDocument(){
        return doc==null ? false : true;
    }

    //----------------------------------------------------------------
    /** Return: active document ? false : true. */
    //----------------------------------------------------------------
    public boolean testdoc(boolean err){
        if(noDocument()){
            if(err)
                Messages.postError("Template error: no document");
            return true;
        }
        return false;
    }

    //----------------------------------------------------------------
    /** Remove all attributes from a node. */
    //----------------------------------------------------------------
    public void removeAttributes(Element node)  {
        NamedNodeMap attrs=node.getAttributes();
        for(int i=0;i<attrs.getLength();i++){
            Node a=attrs.item(i);
            node.removeAttribute(a.getNodeName());
        }
    }

    //----------------------------------------------------------------
    /** return a String vector of root attributes. */
    //----------------------------------------------------------------
    public Vector getAttributes()  {
        Vector v=new Vector();
        if(testdoc(false))
            return v;
        VElement root=(VElement)(doc.getDocumentElement());
        NamedNodeMap attrs=root.getAttributes();
        for(int i=0;i<attrs.getLength();i++){
            Node a=attrs.item(i);
            v.add(a.getNodeName());
            v.add(a.getNodeValue());
        }
        return v;
    }

    //~~~~~~~~~~~~~~~~ Document editor public API ~~~~~~~~~~~~~~~~~~~~

    //----------------------------------------------------------------
    /** Start a new document. */
    //----------------------------------------------------------------
    public XmlDocument newDocument() {
        doc= builder.createDocument();
        doc.setDoctype(null,dtdfile,null);
        Element root=doc.createElement(docname);
        doc.appendChild(root);
        return doc;
    }

    //----------------------------------------------------------------
    /** Add a processing instruction. */
    //----------------------------------------------------------------
    public void addProcInst(String pt, String exp) {
        if(noDocument())
            return;
        ProcessingInstruction node=doc.createProcessingInstruction(pt,exp);
        doc.insertBefore(node,doc.getDocumentElement());
    }

    //----------------------------------------------------------------
    /** Return the root element of the document. */
    //----------------------------------------------------------------
    public VElement rootElement() {
        if(testdoc(false))
            return null;
        VElement root=(VElement)(doc.getDocumentElement());
        if(root != null)
            root.template=this;
        return root;
    }

    //----------------------------------------------------------------
    /** Return a file reference element (with keep flag).
     *     keep=true:   on open, retain reference node in tree
     *                  on save, write ref node only. (do not expand branch)
     *     keep=false:  on open, expand file nodes inline. remove ref node
     *                  on save, ref node gone. branch saved.
     */
    //----------------------------------------------------------------
    public VElement newReference(String file, boolean keep) {
        if(testdoc(true))
            return null;
        VElement ref=newElement("ref");
        ref.setAttribute("file",file);
        if(keep)
            ref.setAttribute("keep","yes");
        return ref;
    }

    //----------------------------------------------------------------
    /** parse and add a reference node*/
    //----------------------------------------------------------------
    public VElement addReference(VElement p, String file, boolean keep) {
        if(testdoc(true))
            return null;
        VElement ref=newReference(file,keep);
        p.add(ref);
        return ref;
    }

    //----------------------------------------------------------------
    /** Return a file reference element (keep=false). */
    //----------------------------------------------------------------
    public VElement newReference(String file) {
        return newReference(file, false);
    }

    //----------------------------------------------------------------
    /** Make a new element node. */
    //----------------------------------------------------------------
    public VElement newElement(String s) {
        VElement elem=(VElement)doc.createElement(s);
        elem.template=this;
        return elem;
    }

    //----------------------------------------------------------------
    /** Add an element node with text. */
    //----------------------------------------------------------------
    public VElement newElement(String s,String t) {
        VElement elem=(VElement)doc.createElement(s);
        Text text=doc.createTextNode(s);
        elem.appendChild(text);
        elem.template=this;
        return elem;
    }

    //----------------------------------------------------------------
    /** Initialize the active document tree. */
    //----------------------------------------------------------------
    public void init(){
        init(rootElement());
    }

    //----------------------------------------------------------------
    /** Initialize a document tree starting at a node. */
    //  note : cannot use TreeWalker since init() may expand ref nodes.
    //----------------------------------------------------------------
    public void init(VElement elem){
        elem.init(this);
        Enumeration cnodes=elem.children();
        while(cnodes.hasMoreElements()){
            VElement child=(VElement)cnodes.nextElement();
            init(child); // note: recursive call to this method
        }
    }

    //----------------------------------------------------------------
    /** <pre>Open and parse an XML file.
     *    <b>path</b>   absolute pathname to the file.
     *</pre>
     */
    //----------------------------------------------------------------
    public void open(String path) throws Exception {
        if(path==null){
            throw new Exception();
        }
        String uri;
        uri = "file:" + path;  // later we can allow net links
        try {
            if(DebugOutput.isSetFor("traceXML"))
                Messages.postDebug("open: "+uri);
            doc=open_xml(uri);
            init(rootElement());
        }
        catch (Exception e) {
            Messages.postError("error opening "+path);
        }
    }

    //----------------------------------------------------------------
    /** <pre>Save the document as a file.
     *    <b>path</b>   absolute pathname to the file.
     *</pre>
     */
    //----------------------------------------------------------------
    public void save(String path)  {
        if(noDocument())
            return;
        try{
            if(DebugOutput.isSetFor("traceXML"))
                Messages.postDebug("save: "+path);
            FileOutputStream out = new FileOutputStream(path);
            doc.write(out);
            out.close();
        }
        catch (java.io.IOException e) {
            Messages.postError("error writing file "+path);
        }
    }

    //----------------------------------------------------------------
    /** Return a document tree for enumeration (start at branch). */
    //----------------------------------------------------------------
    public ElementTree getTree(VElement e){
        if(noDocument())
            return null;
        return new ElementTree(e);
    }

    //----------------------------------------------------------------
    /** Return a document tree for enumeration (start at root). */
    //----------------------------------------------------------------
    public ElementTree getTree(){
        return getTree(rootElement());
    }

    //----------------------------------------------------------------
    /** Output document to System.out */
    //----------------------------------------------------------------
    public void show()  {
        if(noDocument())
            return;
        try{
            doc.write(System.out);
        }
        catch (java.io.IOException e) {
            Messages.postError("error showing xml document");
        }
    }

    //----------------------------------------------------------------
    /** Dump tree data to System.out */
    //----------------------------------------------------------------
    public void dump()  {
        if(noDocument())
            return;
        ElementTree  tree=getTree();
        VElement   elem=tree.rootElement();

        while(elem !=null){
            if(elem.isActive())
                elem.dump();
            elem=tree.nextElement();
        }
    }

    //~~~~~~~~~~~~~~~~ selection methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    //----------------------------------------------------------------
    /** Set selected element. */
    //----------------------------------------------------------------
    public void setSelected(VElement e){
        selected=e;
    }

    //----------------------------------------------------------------
    /** Return selected element (or null). */
    //----------------------------------------------------------------
    public VElement getSelected(){
        return selected;
    }

    //----------------------------------------------------------------
    /** Remove selected element. */
    //----------------------------------------------------------------
    public void removeSelected(){
        if(selected !=null) {
            selected.remove();
            selected=null;
        }
    }

    //+++++++++++++ protected-private ++++++++++++++++++++++++++++++++

    //----------------------------------------------------------------
    /** Open and parse an XML file */
    //----------------------------------------------------------------
    protected XmlDocument open_xml(String uri) throws Exception {
        try {
            parser.parse (uri);
            XmlDocument xdoc = builder.getDocument ();
            xdoc.getDocumentElement().normalize();
            return xdoc;
        } catch (SAXParseException err) {
            Messages.postError ("** SAXParseException"
                                + ", line " + err.getLineNumber ()
                                + ", uri " + err.getSystemId ()
                                +"   " + err.getMessage ());
        } catch (SAXException se) {
            Messages.postError("SAXException opening file "+uri);
        } catch (java.io.IOException e) {
            Messages.postError("error opening file "+uri);
        }
        catch (Exception e) {
            Messages.writeStackTrace(e,"Exception opening "+uri);
        }
        return null;
    }

    //----------------------------------------------------------------
    /** set class bindings for xml keys using an external properties file*/
    //----------------------------------------------------------------
    private void setKeys(String path){
        try {
            xmlkeys=new Properties();
            FileInputStream in=new FileInputStream(path);
            xmlkeys.load(in);
            factory.addMapping (xmlkeys, VElement.class.getClassLoader());
            in.close();
        }
        catch(IOException e){
            Messages.postError("error opening xmlkeys file "+path);
        }
    }

    //----------------------------------------------------------------
    /** initialize the xml interface */
    //----------------------------------------------------------------
    private void init_xml(){
        factory = new SimpleElementFactory ();
        xmlkeys=new Properties();
        if(keyfile==null)
            setDefaultKeys();
        else
            setKeys(keyfile);

        factory.addMapping (xmlkeys, VElement.class.getClassLoader());
        builder = new XmlDocumentBuilder();
        builder.setElementFactory (factory);
        parser = new ValidatingParser ();
        parser.setDocumentHandler (builder);
        builder.setParser (parser);
    }

    //----------------------------------------------------------------
    /** Dump tree data to System.out */
    //----------------------------------------------------------------
    protected void dump(XmlDocument xdoc)  {
        VElement   elem=(VElement)(xdoc.getDocumentElement());
        ElementTree  tree=new ElementTree(elem);

        while((elem=tree.nextElement()) !=null){
            if(elem.isActive())
                elem.dump();
        }
    }
}

/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**
 * Title:       AcqPanelUtility.java
 * Description: AcqPanelUtility class is used by by SpinCAD & JPsg for handling all SpinCAD
 *              pulse sequence parameter panel creation tasks. This is a singleton class
 */

package vnmr.util;

import java.io.*;
import java.util.*;

import org.xml.sax.helpers.AttributesImpl;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
import org.xml.sax.ContentHandler;
import org.xml.sax.InputSource;
import org.xml.sax.*;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;

// For XML write operation
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerConfigurationException;

import javax.xml.transform.sax.SAXSource;
import javax.xml.transform.stream.StreamResult;



public class AcqPanelUtility implements XMLReader {

    /** This class is a singleton instance */
    public static AcqPanelUtility INSTANCE = new AcqPanelUtility();

    /** Content Handler for XML operations */
    private ContentHandler handler;

    /** indents and newline for readability */
    private char[] indentChars  = "\n   ".toCharArray();
    private int indentLength    = "\n   ".length() ;
    private char[] newlineChars = "\n".toCharArray();

    /** String that holds the pulse sequence name, read in from spincadparams.txt */
    private String PSname    = null;
    private String layoutDir    = FileUtil.usrdir()+"/templates/layout/";
    private String layoutSysDir = FileUtil.sysdir()+"/templates/layout/";

    /** private constructor as the class is a singleton */
    private AcqPanelUtility() { }


   /** Coordinate the creation of panel directories and panel pages starting from
    *  a user specified source experiment
    *
    *  @param sourceName name of experiment to start panel building for the current
    *         pulse sequence
    */
   public void makeAcqPanels(String sourceName) {

      try {

        /* set up input text and output XML files */
        File inpf         = new File(FileUtil.usrdir()+"/spincad/info/spincadpanel.txt");
        FileReader fr     = new FileReader(inpf);
        BufferedReader br = new BufferedReader(fr);
        String PSname     = br.readLine();
        br.close();

        /* set up directory ~/vnmrsys/templates/layout/"psname" and other panel
           files starting from the source experiment specified by user sourceName */
        boolean madePanels=false;
        madePanels=setupAcqPanelDirs(sourceName, PSname);
        if ( !madePanels ) {
          String str = "Panels not made since source panel files did not exist or file write permission problems";
          Messages.postError(str);
          Util.getAppIF().appendMessage(str+"\n");
          return;
        }

        String xmlOutName = "SpinCAD_PS.xml";
        File xmlOut = new File(FileUtil.usrdir()+"/templates/layout/"+PSname+"/"+xmlOutName);
        makePanels(inpf,xmlOut);
        inpf.delete();

        /* append the SpinCAD PS page name group into acq.xml */
        String acqXmlName = FileUtil.usrdir()+"/templates/layout/"+PSname+"/acq.xml";
        String groupType  = "group";
        /* name of SpinCAD panel reference page */
        String groupName  = "SpinCAD_PS";
        /* label of SpinCAD tab */
        String groupLabel = "SpinCAD PS";

        /* define the "SpinCAD PS" group attributes */
        AttributesImpl gAtts = new AttributesImpl();
        gAtts.addAttribute("","loc","loc","loc","0 0");
        gAtts.addAttribute("","size","size","size",GroupedLayout.getMaxGroupDimension());
        gAtts.addAttribute("","label","label","label",groupLabel);
        gAtts.addAttribute("","style","style","style","PlainText");
        gAtts.addAttribute("","bg","bg","bg","transparent");
        gAtts.addAttribute("","side","side","side","Top");
        gAtts.addAttribute("","justify","justify","justify","Left");
        gAtts.addAttribute("","tab","tab","tab","yes");
        gAtts.addAttribute("","reference","reference","reference","SpinCAD_PS");
        gAtts.addAttribute("","useref","useref","useref","yes");
        gAtts.addAttribute("","border","border","border","None");

        (new AcqPanelTransformer()).insertGroup(acqXmlName, groupType, groupName, gAtts);

      } catch (IOException e) {
        Messages.writeStackTrace(e,"I/O Error in during manipulation of Acquisition panels");
        // e.printStackTrace();
      }

    }      // makeAcqPanels()



    /** Read in XML panel files and transform
     *
     *  @param fname  xml input  file
     *  @param xmlOut xml output file
     *
     */
    public void makePanels(File fname, File xmlOut) {
        try {
            // Get an instance of AcqPanelUtility
            AcqPanelUtility saxReader = AcqPanelUtility.INSTANCE;

            // Use a Transformer for output
            TransformerFactory tFactory = TransformerFactory.newInstance();
            Transformer transformer = tFactory.newTransformer();


            // Use the parser as a SAX source for input
            FileReader fr = new FileReader(fname);
            BufferedReader br = new BufferedReader(fr);
            InputSource inputSource = new InputSource(br);
            SAXSource source = new SAXSource(saxReader, inputSource);

            // set up output file
            OutputStream fout     = new FileOutputStream(xmlOut);
            OutputStream bout     = new BufferedOutputStream(fout);
            OutputStreamWriter os = new OutputStreamWriter(bout,"UTF-8");
            StreamResult result   = new StreamResult(os);
            transformer.transform(source, result);
            os.flush();
            os.close();

        } catch (TransformerConfigurationException tce) {
           // Error generated by the parser
           Messages.postError("Error in handling Acquisition panels by the XML parser");
           Messages.postDebug("   " + tce.getMessage() );

           // Use the contained exception, if any
           Throwable x = tce;
           if (tce.getException() != null)
               x = tce.getException();
           Messages.writeStackTrace((Exception)x,"Error in handling Acquisition panels by the XML parser");
           // x.printStackTrace();

        } catch (TransformerException te) {
           // Error generated by the parser
           Messages.postError("Error in handling Acquisition panels by the XML parser");
           Messages.postDebug("   " + te.getMessage() );

           // Use the contained exception, if any
           Throwable x = te;
           if (te.getException() != null)
               x = te.getException();
           Messages.writeStackTrace((Exception)x,"Error in handling Acquisition panels by the XML parser");
           // x.printStackTrace();

        } catch (IOException ioe) {
           Messages.writeStackTrace(ioe,"I/O Error in handling Acquisition panels by the XML parser");
           // ioe.printStackTrace();
        }

    } // makePanels()



    /** setup the acquisition panel directories from a source experiment in layout dir
     *
     * @param sourceExp source   pulse sequence to  start panels from
     * @param currentExp current pulse sequence for which panels are being made
     *
     */
    public boolean setupAcqPanelDirs(String sourceExp, String currentExp) {

      boolean useTemplate = true;
      if ( sourceExp.equals("NO TEMPLATE") || sourceExp.equals(currentExp) ) useTemplate = false;

      try {
        /* does the current dir ~/vnmrsys/templates/layout/"psname" exist?
           if not, create it.                                             */
        File curDir = new File(layoutDir+currentExp);
        if ( ! curDir.isDirectory() ) {
          boolean ok = curDir.mkdirs();
          if ( !ok ) {
            String errStr = "Unable to create "+layoutDir+currentExp;
            Messages.postError(errStr);
            Util.getAppIF().appendMessage(errStr+"\n");
            curDir=null;
            return false;
          }
          String errStr2 = layoutDir+currentExp+" directory created";
          Messages.postInfo(errStr2);
          // Util.getAppIF().appendMessage(errStr2+"\n");
        }


      if ( useTemplate ) {
        // use panel template

        /* Does the source directory exist in user or system path ? */
        boolean srcDirFound = true;
        File srcDir = new File(layoutDir+sourceExp);
        if ( ! srcDir.isDirectory() ) {
          File srcSysDir = new File(layoutSysDir+sourceExp);
          if ( ! srcSysDir.isDirectory() ) {
            String wrnStr = "Panel directory "+sourceExp+" does not exist in "+layoutDir+" or "+layoutSysDir;
            Messages.postWarning(wrnStr);
            // Util.getAppIF().appendMessage(wrnStr+"\n");

            srcSysDir=null; srcDir=null;
            srcDirFound = false;
          }
          srcDir=null;
        }

        /* Is the acq.xml file writable in the destination directory? */
        File acq = new File(layoutDir+currentExp+"/acq.xml");
        if (acq.exists() && !acq.canWrite() ) {
          String errStr = layoutDir+currentExp+"/acq.xml cannot be modified";
          Messages.postError(errStr);
          Util.getAppIF().appendMessage(errStr+"\n");
          acq=null;
          return false;
        }

        /* Copy the acq.xml file from the appropriate heirarchical path  */
        String sourceDirName=null;
        String[] sourceDirs = {layoutDir+sourceExp,layoutSysDir+sourceExp,layoutDir+"default",layoutSysDir+"default"};
        for (int i=0; i<sourceDirs.length; i++) {
          File tempDir = new File(sourceDirs[i]);
          if (tempDir.isDirectory()) {
            File tempAcqFile = new File(tempDir+"/acq.xml");
            if (tempAcqFile.exists()) {
              sourceDirName = sourceDirs[i];
              String sFile  = sourceDirName+"/acq.xml";
              String dFile  = layoutDir+currentExp+"/acq.xml";
              copyFile(sFile,dFile);
              tempDir=null; tempAcqFile=null;
              String infoStr = "using base panels from "+sourceDirName;
              Messages.postInfo(infoStr);
              // Util.getAppIF().appendMessage(infoStr+"\n");
              break;
            }
          }
        }
        sourceDirs = null;

        /* now copy all other panel files from source to destination if the source directory
           exists. The files are not copied if the source directory was "default"       */

        boolean usingDefault=false;
        if ( sourceDirName.equals(layoutDir+"default") || sourceDirName.equals(layoutSysDir+"default") ) {
          usingDefault = true;
        }
        if (srcDirFound && !usingDefault ) {
          String[] sourceDirs2 = { layoutDir+sourceExp, layoutSysDir+sourceExp };
          for (int i=0; i<sourceDirs2.length; i++) {
            if ( (new File(sourceDirs2[i])).exists() ) {
              sourceDirName=sourceDirs2[i];
              String[] fileList = (new File(sourceDirName)).list();
              for (int j=0; j<fileList.length; j++) {
                String sFile = sourceDirName+"/"+fileList[j];
                String dFile = layoutDir+currentExp+"/"+fileList[j];
                copyFile(sFile,dFile);
                String infoStr = "copied "+sFile+" to "+dFile;
                Messages.postInfo(infoStr);
                // Util.getAppIF().appendMessage(infoStr+"\n");
              }
              break;
            }
          }
          sourceDirs2 = null;
        }

      } else {

        /* no panel template, check for existance & write option on acq.xml */
        File acq = new File(layoutDir+currentExp+"/acq.xml");
        if (! acq.canWrite() ) {

          if ( acq.exists() ) {
            String errStr = layoutDir+currentExp+"/acq.xml cannot be modified";
            Messages.postError(errStr);
            Util.getAppIF().appendMessage(errStr+"\n");
            acq = null;
            return false;

          } else {

            /* Copy the acq.xml file from the appropriate heirarchical path  */
           String sourceDirName=null;
           String[] sourceDirs = {layoutDir+sourceExp,layoutSysDir+sourceExp,layoutDir+"default",layoutSysDir+"default"};
           for (int i=0; i<sourceDirs.length; i++) {
            File tempDir = new File(sourceDirs[i]);
            if (tempDir.isDirectory()) {
              File tempAcqFile = new File(tempDir+"/acq.xml");
              if (tempAcqFile.exists()) {
                sourceDirName = sourceDirs[i];
                String sFile  = sourceDirName+"/acq.xml";
                String dFile  = layoutDir+currentExp+"/acq.xml";
                copyFile(sFile,dFile);
                tempDir=null; tempAcqFile=null;
                String infoStr = "using base acq.xml panel from "+sourceDirName;
                Messages.postInfo(infoStr);
                // Util.getAppIF().appendMessage(infoStr+"\n");
                break;
              }
            }
          }
          sourceDirs = null;
         }

        }
        acq = null;

      } // if (useTemplate)

      } catch (Exception e)
      {
        //e.printStackTrace();
        String errStr = "File I/O error encountered during making panel directories and files";
        Messages.postError(errStr);
        Messages.writeStackTrace(e,errStr);
        return false;
      }

      return true;

    }


    /**
     * Copies files
     *
     * @param inps input  file
     * @param outs output file
     */
    private void copyFile(String inps, String outs)
    {
      try {
	BufferedReader in = new BufferedReader(new FileReader(inps));
	if (in != null)
	{
             BufferedWriter out = new BufferedWriter(new FileWriter(outs));
             writeToFile(in, out);
             in.close();
             out.close();
	}
      } catch(IOException e)
      {
        String wrnStr = "File "+inps+" could not be copied to "+outs;
        Messages.postWarning(wrnStr);
        // Util.getAppIF().appendMessage(wrnStr+"\n");
      }
    }

    /**
     * Reads from the input file and writes to the output file.
     *
     * @param in    the input  buffer that is used for reading.
     * @param out   the output buffer that is used for writing.
     */
    private void writeToFile(BufferedReader in, BufferedWriter out)
    {
	String strLine;
	try
	{
	    while ((strLine = in.readLine()) != null)
	    {
	        out.write(strLine);
		out.newLine();
	    }
	    out.flush();
	}
	catch (IOException e)
	{
	    String errStr = "I/O Error in copying panel XML files";
            Messages.writeStackTrace(e,errStr);
            // e.printStackTrace();
	}
    }


    /* XML related methods follows */

    /** Parse the input panel XML file */
    public void parse( InputSource input )
                   throws IOException, SAXException
    {
      // We're not doing namespaces
      String nsu = "";  // NamespaceURI

      String rootElement = "template";
      AttributesImpl rootAtts  = new AttributesImpl();
      AttributesImpl groupAtts = new AttributesImpl();
      AttributesImpl entryAtts = new AttributesImpl();
      AttributesImpl labelAtts = new AttributesImpl();

        try {

            // read input file
            Reader r = input.getCharacterStream();

            BufferedReader br = new BufferedReader(r);

            // read the pulse sequence name
            PSname = br.readLine();

            if (handler==null) {
              throw new SAXException("No content handler");
            }

            rootAtts.addAttribute("","name","name","name","newexp");
            rootAtts.addAttribute("","element","element","element","pages");
            rootAtts.addAttribute("","type","type","type","acquisition");

            handler.startDocument();
            handler.startElement(nsu, rootElement, rootElement, (Attributes)rootAtts);
            handler.ignorableWhitespace(newlineChars,0,1);

            String groupElement = "group";
            groupAtts.addAttribute("","loc","loc","loc","0 0");
            groupAtts.addAttribute("","size","size","size",GroupedLayout.fullSizeX+" "+GroupedLayout.fullSizeY);
            groupAtts.addAttribute("","label","label","label","SpinCAD");
            groupAtts.addAttribute("","style","style","style","Heading2");
            groupAtts.addAttribute("","bg","bg","bg","transparent");
            groupAtts.addAttribute("","side","side","side","Top");
            groupAtts.addAttribute("","justify","justify","justify","Left");
            groupAtts.addAttribute("","tab","tab","tab","yes");
            groupAtts.addAttribute("","reference","reference","reference","PulseSequence");
            groupAtts.addAttribute("","useref","useref","useref","no");
            groupAtts.addAttribute("","expanded","expanded","expanded","yes");
            groupAtts.addAttribute("","border","border","border","Etched");
            handler.startElement(nsu, groupElement, groupElement, (Attributes)groupAtts);
            handler.ignorableWhitespace(newlineChars,0,1);

            entryAtts.addAttribute("","loc","loc","loc","loc");
            entryAtts.addAttribute("","size","size","size",GroupedLayout.sizeEx+" "+GroupedLayout.sizeEy);
            entryAtts.addAttribute("","vq","vq","vq","vq");
            entryAtts.addAttribute("","vc","vc","vc","vc");
            entryAtts.addAttribute("","set","set","set","set");
            entryAtts.addAttribute("","style","style","style","PlainText");

            labelAtts.addAttribute("","loc","loc","loc","loc");
            labelAtts.addAttribute("","size","size","size",GroupedLayout.sizeLx+" "+GroupedLayout.sizeLy);
            labelAtts.addAttribute("","style","style","style","Label3");
            labelAtts.addAttribute("","justify","justify","justify","Left");
            labelAtts.addAttribute("","label","label","label","Label");

            String line;
            GroupedLayout.resetAllWidgets();

          while ( null != (line = br.readLine()) ) {

            // update group attributes from file
            StringTokenizer st = new StringTokenizer(line," ");

            // check startGroup token in future version
            String startGroup = st.nextToken();

            String groupName = st.nextToken();
            int groupNum = (new Integer(st.nextToken()) ).intValue();
            String[] gInfo = GroupedLayout.getGroupInfo(groupNum);
            groupAtts.setAttribute(0,"","loc","loc","loc",gInfo[0]);
            groupAtts.setAttribute(1,"","size","size","size",gInfo[1]);
            groupAtts.setAttribute(2,"","label","label","label",groupName);
            handler.ignorableWhitespace(newlineChars,0,1);
            handler.startElement(nsu, groupElement, groupElement, (Attributes)groupAtts);

            for (int i=0; i<groupNum; i++) {

              // update label attributes from file
              String name = br.readLine();
              String[] locs = GroupedLayout.getWidgetLocation();
              labelAtts.setAttribute(0,"","loc","loc","loc",locs[0]);
              labelAtts.setAttribute(4,"","label","label","label",name);

              // update entry attributes from file
              entryAtts.setAttribute(0,"","loc","loc","loc",locs[1]);
              entryAtts.setAttribute(2,"","vq","vq","vq",name);
              entryAtts.setAttribute(3,"","vc","vc","vc",name+"=$VALUE");
              entryAtts.setAttribute(4,"","set","set","set","$VALUE="+name);

              // write label and entry tags & attributes
              output("label", (Attributes)labelAtts, nsu);
              output("entry", (Attributes)entryAtts, nsu);

            } // for loop

            handler.ignorableWhitespace(newlineChars,0,1);
            handler.endElement(nsu, groupElement, groupElement);
            handler.ignorableWhitespace(newlineChars,0,1);

          }  // while loop

          handler.ignorableWhitespace(newlineChars,0,1);
          // end group and root element tags
          handler.endElement(nsu, groupElement, groupElement);
          handler.ignorableWhitespace(newlineChars,0,1);
          handler.endElement(nsu, rootElement, rootElement);
          handler.endDocument();

          br.close();
        }
        catch (Exception e) {
            Messages.writeStackTrace(e);
        }
    }


    /** output a complete element */
    private void output(String name, Attributes atts, String nsu)
                 throws SAXException {
      handler.ignorableWhitespace(indentChars,0,indentLength);
      handler.startElement(nsu, name, name /*"qName"*/, atts);
      handler.endElement(nsu, name, name);
    }


    /** Allow an application to register a content event handler. */
    public void setContentHandler(ContentHandler handler) {
      this.handler = handler;
    }

    /** Return the current content handler. */
    public ContentHandler getContentHandler() {
      return this.handler;
    }

    //=============================================
    // IMPLEMENT THESE FOR A ROBUST APP
    //=============================================
    /** Allow an application to register an error event handler. */
    public void setErrorHandler(ErrorHandler handler)
    { }

    /** Return the current error handler. */
    public ErrorHandler getErrorHandler()
    { return null; }

    //=============================================
    // IGNORE THESE
    //=============================================
    /** Parse an XML document from a system identifier (URI). */
    public void parse(String systemId)
    throws IOException, SAXException
    { }

    /** Return the current DTD handler. */
    public DTDHandler getDTDHandler()
    { return null; }

    /** Return the current entity resolver. */
    public EntityResolver getEntityResolver()
    { return null; }

    /** Allow an application to register an entity resolver. */
    public void setEntityResolver(EntityResolver resolver)
    { }

    /** Allow an application to register a DTD event handler. */
    public void setDTDHandler(DTDHandler handler)
    { }

    /** Look up the value of a property. */
    public Object getProperty(String name)
    { return null; }

    /** Set the value of a property. */
    public void setProperty(String name, Object value)
    { }

    /** Set the state of a feature. */
    public void setFeature(String name, boolean value)
    { }

    /** Look up the value of a feature. */
    public boolean getFeature(String name)
    { return false; }




/**
 * GroupedLayout class is used by by AcqPanelUtility class to layout the parameters
 * in a grouped linear fashion.
 *
 */

static class GroupedLayout {

  /** GroupedLayout make sub groups for parameters according to their general types,
   *  RF, Gradients, Delays etc. Parameters within each subgroup are laid out in a
   *  linear fashion. When a subgroup is done, a new one is started. Current version
   *  works with labels and entry fields.
   */

  static final int fullSizeX = 1280;
  static final int fullSizeY = 272;

  static final int widthOneGroup = 135;
  static final int htOneWidget   = 28;

  static final int startGX= 0;
  static final int startGY= 0;
  static int currGX = startGX;
  static int currGY = startGY;

  static final int startX = 5;
  static final int startY = 20;
  static int currX   = startX;
  static int currY   = startY;
  static int gapX    = 10;
  static int gapY    = 4;
  static int gapPair = 2;
  static int sizeEx  = 64;
  static int sizeEy  = 24;
  static int sizeLx  = 58;
  static int sizeLy  = 24;


  /** the size and location of the current widget
   */
  public static String[] getWidgetLocation() {
    int locLx, locLy, locEx, locEy;

    locLx = currX;
    locLy = currY;
    locEx = currX + sizeLx + gapPair;
    locEy = currY ;

    // update currX and currY positions
    // currX =currX;
    currY = currY + sizeLy + gapY;

    if ( currY >= fullSizeY ) {
      currX = currX + sizeLx + gapX + sizeEx + gapPair;
      currY = startY;
    }
    if ( currX >= fullSizeX ) {
      String wrnStr = "Acquisition SpinCAD PS panel may be getting too wide !";
      Messages.postWarning(wrnStr);
      // Util.getAppIF().appendMessage(wrnStr+"\n");
    }

    return new String[]{ locLx+" "+locLy, locEx+" "+locEy};
  }


  public static String[] getGroupInfo(int numWidgets) {
    int locGx, locGy, sizeGx, sizeGy;

    locGx = currGX;
    locGy = currGY;

    // compute size of group
    sizeGy = startY + (numWidgets * htOneWidget) ;
    double DsizeGy = sizeGy; double DfullSizeY = fullSizeY;
    int numGCol = (int) (Math.ceil(DsizeGy/DfullSizeY));
    sizeGx = numGCol*widthOneGroup;
    sizeGy = fullSizeY;

    // update currGX and currGY positions
    currGX = currGX + sizeGx + gapPair;
    currGY = startGY;

    GroupedLayout.resetWidgetLocation();
    return new String[]{ locGx+" "+locGy, sizeGx+" "+sizeGy };
  }


  public static void resetAllWidgets() {
    currX  = startX;   currY = startY;
    currGX = startGX; currGY = startGY;
  }

  public static void resetWidgetLocation() {
    currX = startX; currY = startY;
  }

  /** keeps track of the current dimensions of the overall group
   *  and helps in trimming the main SpinCAD PS page
   */
  public static String getMaxGroupDimension() {
    return new String(currGX+" "+fullSizeY);
  }

 } // GroupedLayout class

}  // AcqPanelUtility class


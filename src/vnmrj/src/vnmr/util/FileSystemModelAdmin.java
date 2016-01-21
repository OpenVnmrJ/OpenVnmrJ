/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.File;
import java.util.Date;
import java.util.*;
import java.io.*;

import vnmr.ui.shuf.*;


/**
 * FileSystemModel is a TreeTableModel representing a hierarchical file 
 * system. Nodes in the FileSystemModel are FileNodes which, when they 
 * are directory nodes, cache their children to avoid repeatedly querying 
 * the real file system. 
 */

public class FileSystemModelAdmin extends AbstractTreeTableModel 
implements TreeTableModel 
{
    /** Names of the columns. **/
    static protected String[]  cNames = {"File System", "Available Disk Space(bytes)"};

    /** Types of the columns. **/
    static protected Class[]  cTypes = {TreeTableModel.class, Integer.class, String.class};

    // The the returned file length for directories. 
    public static final Integer ZERO = new Integer( 0 ); 

    public FileSystemModelAdmin() { 
	super(new FileNode(new File(File.separator))); 

	getAvailDiskSpace();
    }

    /** Some convenience methods.  **/
    protected File getFile( Object node ) 
    {
       FileNode fileNode = (( FileNode ) node ); 
       return fileNode.getFile();       
    }

    protected Object[] getChildren( Object node ) 
    {
       FileNode fileNode = (( FileNode ) node ); 
       return fileNode.getChildren(); 
    }

    /** The TreeModel interface **/
    public int getChildCount( Object node ) 
    { 
	Object[] children = getChildren( node ); 
	return ( children == null ) ? 0 : children.length;
    }

    public Object getChild( Object node, int i ) 
    { 
	return getChildren( node )[i]; 
    }

    /* The superclass's implementation would work, but this is more efficient. */
    public boolean isLeaf( Object node ) 
    { 
       return getFile( node ).isFile(); 
    }

    /**  The TreeTableNode interface.  **/
    public int getColumnCount() 
    {
	return cNames.length;
    }

    public String getColumnName( int column ) 
    {
	return cNames[column];
    }

    public Class getColumnClass( int column ) 
    {
	return cTypes[column];
    }
 
    public Object getValueAt( Object node, int column ) 
    {
	File file = getFile( node ); 
	String bytes = null;
	String spaces = "                                ";

	try 
        {
	   switch( column ) 
	   {
	      case 0:
		return file.getName();
	      case 1:
		if( !file.isFile() )
		{
		  bytes = (new Integer((int)file.length())).toString() + spaces;
		  return bytes;
	        }
	    }
	 }
	 catch( SecurityException se ) { }
   
	 return null; 
    }

    /*******************************************************************************
     *  This Method is used to get available disk sapces by running the unix command      
     *  df -k.  It only get file system names and available disk spaces.  We can use
     *  it while processing tree structures.
     ******************************************************************************/  
    public void getAvailDiskSpace() 
    {
       String line = null;
       Process prcs = null;
       try {
          String[] cmd = {"/usr/bin/sh", "-c", "df -k | awk ' { print $4 \"\t\" $6}'" };
 
          Runtime rt = Runtime.getRuntime();
          prcs = rt.exec(cmd);
 
          InputStream istrm = prcs.getInputStream();
          BufferedReader bfr = new BufferedReader(new InputStreamReader(istrm));
 
          while((line = bfr.readLine()) != null) 
             System.out.println( line );
        }
        catch (IOException e)
        {
           System.out.println(e);
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

    }
}

/* A FileNode is a derivative of the File class - though we delegate to 
 * the File object rather than subclassing it. It is used to maintain a 
 * cache of a directory's children and therefore avoid repeated access 
 * to the underlying file system during rendering. 
 */
class FileNode 
{ 
   File     file; 
   Object[] children; 

   public FileNode( File file ) 
   { 
      this.file = file; 
   }

    // Used to sort the file names.
    static private MergeSort  fileMS = new MergeSort() 
    {
       public int compareElementsAt( int a, int b ) 
       {
	  return(( String )toSort[a] ).compareTo(( String )toSort[b] );
       }
    };

    /**
     * Returns the the string to be used to display this leaf in the JTree.
     */
    public String toString() 
    { 
       return file.getName();
    }

    public File getFile() 
    {
       return file; 
    }

    /**
     * Loads the children, caching the results in the children ivar.
     */
    protected Object[] getChildren() 
    {
       if( children != null ) 
	  return children; 
       	
       Vector tmpchildren = new Vector();
 
       try 
       {
	  String[] files = getFiles( file );

	  if( files != null ) 
	  {
	     fileMS.sort( files ); 
	      String path = file.getPath();
	      for( int i = 0; i < files.length; i++ ) 
	      {
		 File childFile = new File( path, files[i] ); 
	         if( !childFile.isFile() && childFile.length() != 0 )
		   tmpchildren.add( new FileNode( childFile ));
	      }
	    }
	 } 
         catch( SecurityException se ) {}

	 children = new FileNode[tmpchildren.size()];
	 tmpchildren.copyInto( children );

	 return children; 
    }

    public String[] getFiles( File file )
    {
       String[] dffiles = {"usr", "var", "export", "opt", "tmp", "sw", "usr25"};
       Vector existedfiles = new Vector();
       boolean isindf = false;

       String[] sysfiles = file.list();
       for( int i = 0; i < sysfiles.length; i++ )
       {
	  isindf = checkfiles( dffiles, sysfiles[i], file );
  	  if( isindf ) 
	     existedfiles.addElement( sysfiles[i] );
       }

       String[] files = new String[existedfiles.size()];
       existedfiles.copyInto( files );

       return files;
    }

    protected boolean checkfiles( String[] dffiles, String cmpFile, File file )
    {
       String parentdir = null;
 
       StringTokenizer token = new StringTokenizer( file.getPath(), "/" );
       if( token.hasMoreTokens())
       {
	  parentdir = token.nextToken();
	  for( int k = 0; k < dffiles.length; k++ )
	  {
	     if( dffiles[k].equals( parentdir ))
		return true;
	  }
       }
       else
       {
          for( int i = 0; i < dffiles.length; i++ )
          {
	     if( dffiles[i].equals( cmpFile ))
	        return true;
          }
       }

       return false;
    }
}


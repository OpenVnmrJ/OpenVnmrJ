/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.util;

import java.io.*;
import java.util.*;

/**
 * Title:
 * Description:
 * Copyright:    Copyright (c) 2002
 * Company:
 * @author
 * @version 1.0
 */

public abstract class WMRUCache
{

    protected Map cache;
    protected LinkedList mrulist;
    protected int cachesize;

    public static final String LASTMODIFIED = "lastModified";

    public WMRUCache(int max)
    {
        cache = new HashMap();
        mrulist= new LinkedList();
        cachesize=max;
    }

    /**
     *  Returns the data for the given string.
     *  @param fname    name of the file.
     */
    public Object getData(String fname)
    {
        return getData(fname, true);
    }

    /**
     *  Returns the data for the given string.
     *  @param fname        name of the file.
     *  @param bCreateOk    if the file is not in the cache, then is it ok to
     *                      read the file from the system, and create an entry
     *                      in the cache for this file.
     */
    public Object getData(String fname, boolean bCreateOk)
    {
        if(!cache.containsKey(fname))
        {
            // if the file is not in the cache and if it is ok to create a new
            // entry in the cache, then read the file from the database, and
            // return the data.
            if (bCreateOk)
            {
                synchronized(cache)
                {
                    if(mrulist.size() >=cachesize)
                    {
                        cache.remove(mrulist.getLast());
                        mrulist.removeLast();
                    }
                    cache.put(fname, readFile(fname));
                    mrulist.addFirst(fname);
                }
            }
            else
                return null;
        }
        else
        {
            if(isFileModified(fname))
            {
                //System.out.println("REading file *********");
                synchronized(cache)
                {
                    cache.put(fname, readFile(fname));
                }
            }
            synchronized(cache)
            {
                mrulist.remove(fname);
                mrulist.addFirst(fname);
            }
        }
        return ((WFile)cache.get(fname)).getContents();
    }

    /**
     *  Sets the data for the given file.
     *  @param strFName name of the file.
     *  @param objData  data to be set for this file.
     */
    public void setData(String strFName, Object objData)
    {
        WFile objFile = (WFile)cache.get(strFName);
        if (objFile != null)
            objFile.setContents(objData);
    }

    /**
     *  Looks up the file in the cache, and returns when it was last modified.
     *  @param strFName name of the file.
     */
    public long getLastModified(String strFName)
    {
        long lastMod = 0;
        WFile objFile = (WFile)cache.get(strFName);

        if (objFile != null)
            lastMod = objFile.getLastModified();

        return lastMod;
    }

    /**
     *  Sets the last modified stamp for the given file.
     *  @param strFName name of the file
     *  @param lastMod  the last modified value.
     */
    public void setLastModified(String strFName, long lastMod)
    {
        WFile objFile = (WFile)cache.get(strFName);
        if (objFile != null)
            objFile.setLastModified(lastMod);
    }

    /**
     *  Removes the given file from the cache.
     *  @param fName    the name of the file.
     */
    public void remove(String fName)
    {
        cache.remove(fName);
        mrulist.remove(fName);
    }

    /**
     *  Reads the given file.
     */
    public abstract WFile readFile(String strFName);

    /**
     *  Checks if the file is been modified.
     *  @param fname    name of the file.
     */
    protected boolean isFileModified(String fname)
    {
        boolean bModified = false;
        String path1 = null;
        String path2 = null;

        // Gets the lastmodified stamp for the file in the cache, and
        // the file from the system, and compares.
        long lModDateCache = ((WFile)cache.get(fname)).getLastModified();
        long lModDate = new File(fname).lastModified();
        if (lModDate != lModDateCache)
            bModified = true;

        // if the file name is in the form => path1:path2
        // then check the modified date on each path
        int nIndex = fname.indexOf(File.pathSeparator);
        if (nIndex > 0)
        {
            path1 = fname.substring(0, nIndex);
            path2 = fname.substring(nIndex+1);
            if (path1 != null && path2 != null)
            {
                if ((new File(path1).lastModified()) != lModDateCache
                        || (new File(path2).lastModified()) != lModDateCache)
                {
                    bModified = true;
                }
            }
        }

        return bModified;
    }

}

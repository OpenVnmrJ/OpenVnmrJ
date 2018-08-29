/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Collection;
import java.util.Vector;

public class ProbeIdAppdirs implements ProbeId {
    public static class Appdir {
        final public String TagFileName = "probeid";
        String key, owner, label;
        File getTagFile(File dir) {
            File tagFile = null;
            if (dir.isDirectory() && dir.canRead())
                tagFile = new File(dir, TagFileName);
            return tagFile;
        }
        
        String getTag(File tagFile) throws IOException {
            String tag = null;
             if (tagFile != null && tagFile.canRead()) {
                FileInputStream fis = new FileInputStream(tagFile);
                InputStreamReader isr = new InputStreamReader(fis);
                BufferedReader reader = new BufferedReader(isr);
                tag = reader.readLine();
            }
            return tag;
        }
        
        String getTag(ProbeIdStore db, File dir) throws IOException {
            // check if one already exists
            File tagFile = getTagFile(dir).getCanonicalFile();
            String tag = getTag(tagFile);
            if (tag == null)
                tag = ProbeIdDb.createAppdirTag(db, dir);
            
            return tag;
        }
    
        Appdir(final String key, final String owner, final String label) {
            this.key = key; 
            this.owner = owner;
            this.label = label;
        }
    }
    
    private String m_appdirs  = null;
    private String m_labels   = null;
    Collection<Appdir> m_tags = null;
    
    String appdirs() { return m_appdirs; }
    
    ProbeIdAppdirs(String path, String labels) {
        if (path == null) {
            path = new String(ProbeIdDb.getUserDir()+File.pathSeparator+ProbeIdDb.getSystemDir());
            labels = new String(SYSTEM_LABEL+File.pathSeparator+USER_LABEL);
        }
        m_labels  = labels == null ? path : labels;
        m_appdirs = path;
        m_tags    = getAppdirTags();
    }
    
    ProbeIdAppdirs() { this(null, null); }
    
    /**
     * Return a suspected copy (rather than path alias) of an appdir tag.
     * A match probably means that the caller has copied a tag and now
     * has a duplicate, which could lead to problems, since the corresponding
     * database entry was not copied.
     * @param set
     * @param match
     * @return
     * @throws IOException
     */
    Appdir matchAppDir(Collection<Appdir> set, Appdir match) throws IOException {
        for (Appdir element : set)
            if (match.equals(element.key)) {
                String matchPath = (new File(match.owner)).getCanonicalPath();
                String elementPath = (new File(element.owner)).getCanonicalPath();
                if (!matchPath.equals(elementPath))
                    return element;
            }
        return null;
    }
    
    boolean knownAppDir(File dir, File knownDir) {
        try {
            File appDir = dir.getCanonicalFile();
            return appDir.equals(knownDir.getCanonicalFile());
        } catch (IOException e) {
            ProbeIdIO.error("I/O error accessing "+dir+" "+e.getMessage());
        }
        return false;
    }
          
    /**
     * Get an ordered list of tags for appdir-ordered search.
     * Redundant tags may be the result of aliases resulting from
     * symbolic links (same inode), which are ok, or as a result
     * of copying, which is more problematic.  In the latter case
     * we generate an error message and then drop the copy from
     * the search order.  It is possible to fool the system by
     * making a copy and then removing it form the search order
     * and then adding it back.  Don't do that.
     * @return
     * @throws IOException 
     */
    Collection<Appdir> getAppdirTags() {
        String[] appdirs = m_appdirs.split(":");
        String[] labels  = m_labels.split(":");
        Collection<Appdir> tags = new Vector<Appdir>();
        for (int i=0; i<appdirs.length; i++) {
            String appdir = appdirs[i];
            String label  = labels[i];
            File dir = new File(appdir);
            String tag = null;
            if (dir.canRead() && dir.isDirectory()) {
                File tagFile = new File(dir, ".probeid");
                if (tagFile.canRead()) {
                    try {
                        BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream(tagFile)));
                        tag = reader.readLine();
                        if (tag != null) {
                            Appdir match = new Appdir(tag, appdir, label);
                            if (match != null) {
                                ProbeIdIO.error("duplicate appdir tag: is "+
                                        match.owner + " a copy of " + appdir + "?");
                            }
                        }
                        tags.add(new Appdir(tag, appdir, label));
                    } catch (IOException e) {
                        ProbeIdIO.error("I/O error accessing "+tagFile.getPath()+" tag file");
                    }
                }
            }
            if (tag==null) {
                String sysdir = ProbeIdDb.getSystemDir();
                String usrdir = ProbeIdDb.getUserDir();
                if (knownAppDir(dir, new File(sysdir))) {
                    // treat system directory as a special case, just in case
                    // it doesn't have a .probeid tag file
                    tags.add(new Appdir(ProbeIdDb.getSysTag(), appdir, label));
                } else if (knownAppDir(dir, new File(usrdir))) {
                    tags.add(new Appdir(ProbeIdDb.getUsrTag(), appdir, label));
                }
            }
        } 
        return tags;
    }
    
    public void getAppdirKeys(Collection<Appdir> keys, String subdir, String file) {
        m_tags = getAppdirTags();
        for (Appdir tag : m_tags) {
            String key = ProbeIdDb.getAppdirKeyFromHostFile(tag.key, subdir, file);
            keys.add(new Appdir(key, tag.owner, tag.label));
        }
    }
    
    public static void getAppdirKeys(Collection<Appdir> keys, 
                                     ProbeIdAppdirs appdirs,
                                     String subdir, String file) 
    {
        if (appdirs == null)
            appdirs = new ProbeIdAppdirs();
        appdirs.getAppdirKeys(keys, subdir, file);
    }
}


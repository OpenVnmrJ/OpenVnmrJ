/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.util;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.zip.CRC32;
import java.util.zip.CheckedInputStream;

import static vnmr.util.FileChangeEvent.*;

/**
 * Construct objects that will watch for changes in any of a list of files
 * and report when a change occurs.
 */
public class FileWatcher extends Thread {

    public static final long UNKNOWN_CKSUM = -1;
    public static final long NO_FILE = -2;
    public static final long BIG_FILE = -3;
    public static final long PENDING_CKSUM = -4;
    public static final long INIT_CKSUM = -5;

    private static final int TIME_BETWEEN_CHECKS = 500; // ms
    private static final int RECENT_CHANGE_TIME = TIME_BETWEEN_CHECKS + 2000;
    private static final long MAX_CKSUM_FILESIZE = 100000000; // bytes

//    /** The set of file paths to watch. */
//    private Set<String> m_watchPaths = new TreeSet<String>();

    /** The file paths to watch, and their information. */
    private Map<String, FileInfo> m_watchMap = new TreeMap<String, FileInfo>();

    /** The set of listeners to notify of changes. */
    private Set<FileListener> m_listeners = new TreeSet<FileListener>();

    /** Run until this becomes true. */
    private boolean m_quit = false;
    private Set<String> m_pathsToRemove = new TreeSet<String>();
    private Set<String> m_pathsToAdd = new TreeSet<String>();

    /**
     * Test routine watches a list of files and prints out the change events.
     * @param args A list of paths -- the files to watch.
     */
    public static void main(String[] args) {
        FileListener listener = new TestFileListener();
        FileWatcher watcher = new FileWatcher(listener, args);
        watcher.start();
    }

    /**
     * Construct a FileWatcher with a given listener and an array of files
     * to watch.
     * @param listener The listener to be notified of changes.
     * @param paths The paths to the files to be watched.
     */
    public FileWatcher(FileListener listener, String[] paths) {
        m_listeners.add(listener);
        for (String path : paths) {
            m_watchMap.put(path, new FileInfo());
            Messages.postDebug("FileWatcher", "Added " + path);
        }
    }

    /**
     * Signal the file watcher to stop watching.
     */
    public void quit() {
        m_quit = true;
    }

    public void run() {
        // Start watching
        while (!m_quit ) {
            checkFiles();
            try {
                Thread.sleep(500);
            } catch (InterruptedException ie) {
                m_quit = true;
            }
        }
    }

    /**
     * Check all the files on the watch list.
     */
    synchronized private void checkFiles() {
        updatePaths();
        Set<String> keys = m_watchMap.keySet();
        for (String path : keys) {
            File file = new File(path);
            FileInfo info = m_watchMap.get(path);
            FileChangeEvent event = null;
            if (!file.exists()) {
                if (info.checksum != NO_FILE) {
                    int changeType = (info.checksum == INIT_CKSUM)
                             ? FILE_INITIAL : FILE_DELETED;
                    info.checksum = NO_FILE;
                    event = new FileChangeEvent(changeType, path);
                }
                info.timeChecked = System.currentTimeMillis();
            } else {
                // File exists
                event = getChangeEvent(path, info);
            }

            // If something happened, notify the listeners
            if (event != null) {
                for (FileListener listener : m_listeners) {
                    listener.fileChanged(event);
                }
            }
        }
        updatePaths();
    }

    synchronized private void updatePaths() {
        for (String path : m_pathsToRemove) {
            if (path != null) {
                removePathSync(path);
            }
        }
        m_pathsToRemove.clear();
        for (String path : m_pathsToAdd) {
            if (path != null) {
                addPathSync(path);
            }
        }
        m_pathsToAdd.clear();
    }

    /**
     * See if a given file has changed.
     * @param path The path of the file to check.
     * @param info The previous state of the given file.
     * @return The type of change, or null if there was no change.
     */
    private FileChangeEvent getChangeEvent(String path, FileInfo info) {
        FileChangeEvent event = null;
        File file = new File(path);
        long now = System.currentTimeMillis();
        long modTime = file.lastModified();
        long timeDiff = modTime - info.timeModified;
        if (info.checksum == NO_FILE) {
            // File just created
            event = new FileChangeEvent(FILE_CREATED, path);
            info.checksum = getCksum(file);
        } else if (timeDiff != 0) {
            // File mod time changed - did contents change?
            long cksum = getCksum(file);
            if (cksum == BIG_FILE) {
                // File too big to calculate cksum
                // Send change event later
                info.changePending = true;
            } else if (info.checksum == INIT_CKSUM) {
                // First time file is checked
                event = new FileChangeEvent(FILE_INITIAL, path);
            } else if (info.checksum == UNKNOWN_CKSUM) {
                // Probably first time file is checked
                //event = new FileChangeEvent(FILE_CHANGED, path);
            } else if (cksum != info.checksum) {
                // File changed
                event = new FileChangeEvent(FILE_CHANGED, path);
            } else {
                // File unchanged
                event = new FileChangeEvent(FILE_TOUCHED, path);
            }
            info.checksum = cksum;
        } else if (now - modTime <= RECENT_CHANGE_TIME) {
            // Mod time is same, but file changed recently
            // Test the checksums
            // (2nd change could have same timestamp as first)
            long cksum = getCksum(file);
            if (cksum != info.checksum) {
                // File changed
                event = new FileChangeEvent(FILE_CHANGED, path);
                info.checksum = cksum;
            }
        } else if (info.changePending) {
            event = new FileChangeEvent(FILE_CHANGED, path);
            info.changePending = false;
        }
        info.timeChecked = now;
        info.timeModified = modTime;
        return event;
    }

    /**
     * Add a file to the list of files to watch.
     * The file's "checksum" is initialized to INIT_CKSUM.
     * @param path The path to the file.
     */
    synchronized public void addPath(String path) {
        if (path != null) {
            Messages.postDebug("FileWatch", "Adding path: " + path);
            m_pathsToAdd.add(path);
        }
    }

    /**
     * Add a file to the list of files to watch.
     * The file's "checksum" is initialized to INIT_CKSUM.
     * @param path The path to the file.
     * @return True if the file was added (i.e., was not already present).
     */
    synchronized public boolean addPathSync(String path) {
        if (m_watchMap.containsKey(path)) {
            return false;
        } else {
            m_watchMap.put(path, new FileInfo());
            Messages.postDebug("FileWatcher", "Added " + path);
            return true;
        }
    }

    /**
     * Remove a file from the watch list.
     * @param path The path to the file.
     */
    synchronized public void removePath(String path) {
        if (path != null) {
            Messages.postDebug("FileWatch", "Removing path: " + path);
            m_pathsToRemove.add(path);
        }
    }

    /**
     * Remove a file from the watch list.
     * @param path The path to the file.
     * @return True if the file was removed (i.e., was being watched).
     */
    synchronized public boolean removePathSync(String path) {
        FileInfo value = m_watchMap.remove(path);
        Messages.postDebug("FileWatcher", "Removed " + path);
        return value != null;
    }

    /**
     * Calculate a checksum for a given file.
     * An empty file gets a checksum of 1.
     * @param filepath The path to the file.
     * @return The CRC32 checksum.
     */
    public static long getCksum(String filepath) {
        File file = new File(filepath);
        return getCksum(file);
    }

    /**
     * Calculate a checksum for a given file.
     * An empty file gets a checksum of 1.
     * @param file The file to check.
     * @return The CRC32 checksum.
     */
    public static long getCksum(File file) {
        // CRC32 measured at 220 ms for 15 MB file
        // Adler32 measured at 188 ms for 15 MB file
        long checksum = UNKNOWN_CKSUM;
        //long t0 = System.currentTimeMillis();
        if (file != null) {
            if (file.length() > MAX_CKSUM_FILESIZE) {
                checksum = BIG_FILE;
            } else {
                try {
                    FileInputStream instream = new FileInputStream(file);
                    CheckedInputStream check;
                    //check = new CheckedInputStream(file, new Adler32());
                    check = new CheckedInputStream(instream, new CRC32());
                    BufferedInputStream in = new BufferedInputStream(check);
                    while (in.read() != -1) ; // Read entire file
                    checksum = check.getChecksum().getValue();
                } catch (FileNotFoundException e) {
                    checksum = NO_FILE;
                } catch (IOException e) {
                    checksum = UNKNOWN_CKSUM;
                }
            }
        }
        //long time = System.currentTimeMillis() - t0;
        //System.out.println("Checksum is " + checksum + ", " + time + " ms");
        return checksum;
    }


    /**
     * A container for information about a file.
     */
    static class FileInfo {
        public long timeChecked;
        public long timeModified;
        public long checksum;
        public boolean changePending = false;

        /**
         * Create a FileInfo with default information indicating that
         * nothing is known.
         */
        public FileInfo() {
            this(0, 0, INIT_CKSUM);
        }

        /**
         * Create a FileInfo with the given initial information.
         * @param checkTime Last time file was checked.
         * @param modTime The file's last-modification time.
         * @param cksum The file's check sum.
         */
        public FileInfo(long checkTime, long modTime, long cksum) {
            timeChecked = checkTime;
            timeModified = modTime;
            checksum = cksum;
        }
    }


    /**
     * A FileListener for debugging. Simply prints out a message
     * when a file changes.
     */
    static class TestFileListener implements FileListener {

        private SimpleDateFormat dateFormat = new SimpleDateFormat("HH:mm:ss");

        @Override
        public void fileChanged(FileChangeEvent event) {
            String path = event.getPath();
            String type = "FILE_???????";
            int etype = event.getEventType();
            switch (etype) {
            case FILE_CHANGED:
                type = "FILE_CHANGED";
                break;
            case FILE_CREATED:
                type = "FILE_CREATED";
                break;
            case FILE_DELETED:
                type = "FILE_DELETED";
                break;
            case FILE_TOUCHED:
                type = "FILE_TOUCHED";
                break;
            }
            print(type, System.currentTimeMillis(), path);
            if (etype == FILE_CHANGED || etype == FILE_CREATED) {
                printFile(path);
            }
        }

        private void print(String tag, long time, String path) {
            String strTime = dateFormat.format(new Date(time));
            System.out.println(strTime + " " + tag + ": " + path);
        }

        private void printFile(String path) {
            if (true) {
                System.out.println("Length=" + new File(path).length());
            }
            if (true) {
                BufferedReader in = null;
                try {
                    in = new BufferedReader(new FileReader(path));
                    String line;
                    while ((line = in.readLine()) != null) {
                        System.out.println(line);
                    }
                } catch (FileNotFoundException e) {
                } catch (IOException e) {
                } finally {
                    System.out.println("==============");
                    try {
                        in.close();
                    } catch (Exception e) {}
                }
            }
        }
    }
}

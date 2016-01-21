/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collection;

public interface ProbeIdStore {
    String getProbeId();         // return the associated probe ID
	String getPath();            // return the mount point
	File   getPath(String key);  // return the path to the corresponding file
	OutputStream getOutputStreamFromKey(String key, boolean append) throws IOException;
	InputStream  getInputStreamFromKey(String key) throws IOException;
	InputStream  getInputStreamFromKey(String key, boolean cache) throws IOException;
    public void list(Collection<String> results, String path, boolean sys);
	boolean isCached(String key);
	boolean delete(String key) throws IOException, ProbeIdMismatchException;
	boolean flush(String id) throws IOException, ProbeIdMismatchException;
	boolean copy(String keySrc, String keyDst) throws IOException;
}

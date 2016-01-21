/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.io.FileInputStream;
import java.io.FilterInputStream;
import java.util.zip.ZipInputStream;

import javax.crypto.CipherInputStream;

public class ProbeIdInputStream extends FilterInputStream implements ProbeIdStream {
	protected class ProbeIdStreamInfo {
		protected boolean m_isZipped = false;
		protected boolean m_isEncrypted = false;
		protected boolean m_isCached = false;
		protected boolean m_isRemote = false;
	};
	ProbeIdStreamInfo m_info;
	public boolean isZipped()    { return m_info.m_isZipped; }
	public boolean isEncrypted() { return m_info.m_isEncrypted; }
	public boolean isRemote()    { return m_info.m_isRemote; }
	public boolean isCached()    { return m_info.m_isCached; }
	
	ProbeIdInputStream(CipherInputStream in) {
		super(in);
		m_info.m_isEncrypted = true;
	}
	
	ProbeIdInputStream(ZipInputStream in) {
		super(in);
		m_info.m_isZipped = true;
	}
	
	ProbeIdInputStream(FileInputStream in) {
		super(in);
		m_info.m_isCached = true;
	}
	
	ProbeIdInputStream(ProbeIdInputStream in) {
		super(in);
		m_info = in.m_info;
	}
}

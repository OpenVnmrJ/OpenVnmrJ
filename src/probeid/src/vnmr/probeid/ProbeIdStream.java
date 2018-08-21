/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

public interface ProbeIdStream {
	public boolean isZipped();
	public boolean isEncrypted();
	public boolean isCached();
	public boolean isRemote();
}

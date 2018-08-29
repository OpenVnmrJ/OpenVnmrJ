/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package  vnmr.util;

public interface VColorImageListenerIF {
    public void clearImageInfo();
    public void addImageInfo(int id, int displaOrder, int colormapId,
                             int transparency, String imgName);
    public void selectImageInfo(int id);
}


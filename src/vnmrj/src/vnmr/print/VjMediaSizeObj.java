/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import javax.print.attribute.standard.*;

public class VjMediaSizeObj {

   private MediaSize mediaSize;
   private String subName;

   public VjMediaSizeObj(MediaSize media, String name) {
       this.mediaSize = media;
       this.subName = name;
   }

   public MediaSize getMediaSize() {
       return mediaSize;
   }

   public String getMediaName() {
       if (mediaSize == null)
          return subName;
       return mediaSize.getMediaSizeName().toString();
   }
    
   public String getName() {
       return subName;
   }
    
   public String toString() {
       return subName;
   }

}

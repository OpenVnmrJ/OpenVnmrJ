/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.print;

import java.awt.Dimension;
import javax.print.attribute.standard.*;
import javax.print.attribute.Size2DSyntax;


public class VjPaperMedia {

    public static final MediaSize paperMedium[] = {
        MediaSize.NA.LETTER,  MediaSize.NA.LEGAL,
        MediaSize.Other.LEDGER, MediaSize.Other.EXECUTIVE,
        MediaSize.ISO.A0, MediaSize.ISO.A1, MediaSize.ISO.A2,
        MediaSize.ISO.A3, MediaSize.ISO.A4, MediaSize.ISO.A5,
        MediaSize.ISO.A6,
        MediaSize.ISO.B0, MediaSize.ISO.B1, MediaSize.ISO.B2,
        MediaSize.ISO.B3, MediaSize.ISO.B4, MediaSize.ISO.B5,
        MediaSize.ISO.C3, MediaSize.ISO.C4, MediaSize.ISO.C5,
        MediaSize.ISO.C6
    };

    public static final String paperNames[] = {
        "Letter", "Legal",
        "Ledger", "Executive",
        "A0", "A1", "A2",
        "A3", "A4", "A5",
        "A6",
        "B0", "B1", "B2",
        "B3", "B4", "B5",
        "C3", "C4", "C5",
        "C6"
    };

    public static final MediaSize extraPaperMedium[] = {
        MediaSize.NA.NA_8X10,  MediaSize.NA.NA_5X7,
        MediaSize.ISO.A7, MediaSize.ISO.A8,
        MediaSize.ISO.DESIGNATED_LONG,
        MediaSize.JIS.B0, MediaSize.JIS.B1,
        MediaSize.JIS.B2, MediaSize.JIS.B3,
        MediaSize.JIS.B4, MediaSize.JIS.B5,
        MediaSize.JIS.B6, MediaSize.JIS.B5,
        MediaSize.Engineering.A, MediaSize.Engineering.B,
        MediaSize.Engineering.C, MediaSize.Engineering.D,
        MediaSize.Other.TABLOID, MediaSize.Other.FOLIO
    };

    public static double getPixelWidth(MediaSize media) {
        float dim[] = media.getSize(1); //units == 1 to avoid FP error
        double w = Math.rint((dim[0]*72.0f)/Size2DSyntax.INCH);
        return w;
    }

    public static double getPixelHeight(MediaSize media) {
        float dim[] = media.getSize(1); //units == 1 to avoid FP error
        double h = Math.rint((dim[1]*72.0f)/Size2DSyntax.INCH);
        return h;
    }

    public static MediaSize getPaperMedia(String name, boolean useDefault) {
        MediaSize md;
        int n;
        boolean bMatch = false;

        if (name == null || (name.length() < 1)) {
            if (useDefault)
               return paperMedium[0];
            return null;
        }
            
        md = null;
        for (n = 0; n < paperMedium.length; n++) {
            md = paperMedium[n];
            if (name.equals(md.getMediaSizeName().toString())) {
                bMatch = true;
                break;
            }
            if (name.equalsIgnoreCase(paperNames[n])) {
                bMatch = true;
                break;
            }
        }
        if (!bMatch) {
            for (n = 0; n < extraPaperMedium.length; n++) {
                md = extraPaperMedium[n];
                if (name.equals(md.getMediaSizeName().toString())) {
                    bMatch = true;
                    break;
                }
            }
        }
        if (!bMatch) {
           if (useDefault)
               return paperMedium[0];
           return null;
        }
        return md;
    }

    public static MediaSize getPaperMedia(String name) {
        return getPaperMedia(name, true);
    }

    // width and height are MM
    public static MediaSize getPaperMedia(double w, double h) {
        MediaSize md;
        int n, pnum, xpnum;
        float dim[];
        float sw, sh;
        float rw = 100.0f;
        float rh = 100.0f;
        float pw, ph;
        float dw, dh;

        sw = (float) w - 1.0f;
        sh = (float) h - 1.0f;
        pnum = 0;
        for (n = 0; n < paperMedium.length; n++) {
            md = paperMedium[n];
            dim = md.getSize(Size2DSyntax.MM);
            pw = dim[0];
            ph = dim[1];
            if (pw >= sw && ph >= sh) {
                dw = Math.abs(pw - sw);
                dw = dw / pw;
                if (dw <= rw) {
                    dh = Math.abs(ph - sh);
                    dh = dh / ph;
                    if (dh < rh) {
                         rh = dh;
                         rw = dw;
                         pnum = n;
                    }
                }
            }
        }
        if (rw < 0.06f && rh < 0.06f)
        {
            return paperMedium[pnum];
        }
        xpnum = -1;
        for (n = 0; n < extraPaperMedium.length; n++) {
            md = extraPaperMedium[n];
            dim = md.getSize(Size2DSyntax.MM);
            pw = dim[0];
            ph = dim[1];
            if (pw >= sw && ph >= sh) {
                dw = Math.abs(pw - sw);
                dw = dw / pw;
                if (dw < rw) {
                    dh = Math.abs(ph - sh);
                    dh = dh / ph;
                    if (dh < rh) {
                         rh = dh;
                         rw = dw;
                         xpnum = n;
                    }
                }
            }
        }
        if (xpnum >= 0)
        {
            return extraPaperMedium[xpnum];
        }
        return paperMedium[pnum];
    }

    public static String getPaperName(double w, double h) {
        MediaSize md = getPaperMedia(w, h);
        return md.getMediaSizeName().toString();
    }

    public static String getPaperName(MediaSize md) {
        return md.getMediaSizeName().toString();
    }

    // in pixels, postscript units
    public static Dimension getPaperPS(String paperName, boolean useDefault) {
        MediaSize md = getPaperMedia(paperName, useDefault);
        if (md == null)
            return null;
        float dim[] = md.getSize(Size2DSyntax.MM);
        dim[0] = dim[0] * 72.0f / 25.4f + 0.5f;
        dim[1] = dim[1] * 72.0f / 25.4f + 0.5f;
        int dw = (int) dim[0];
        int dh = (int) dim[1];
        
        return new Dimension(dw, dh);
    }

    public static Dimension getPaperPS(String paperName) {
        return getPaperPS(paperName, true);
    }

} // end of VjPaperMedia


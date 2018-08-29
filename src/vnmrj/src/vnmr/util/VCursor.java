/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.awt.*;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.util.*;

/**
 * This class provides a static method for getting a custom cursor from
 * a GIF format image file and Java built-in cursors.
 * The image files will usually be found in the vnmrj. jar file under
 * vnmr/images.
 * (The master copies are currently kept in
 * <code>/vcommon/vnmrj/vnmr_images</code>.)
 * <p>
 * GIF files containing cursors should use the following procedure to
 * put "hotspot" information into the file.
 * The "hotspot" is the coordinate, measured from the upper-left corner
 * of the cursor, of the precise cursor location.
 * Since GIF files have no built-in way of specifying hotspot information,
 * we resort to simply putting a string into the file of the form:
 * <pre>
 * hotspot=<i>x</i>,<i>y</i>;
 * </pre>
 * where <i>x</i> and <i>y</i> are the hotspot coordinates.
 * This string can be anywhere in the file that doesn't interfere with
 * the image encoding. For image editors, such as the Gimp, that allow
 * you to specify the comment field when storing the file, simply type
 * the hotspot specification string into the comment field.
 * If you are unable to set the comment string, the other method is
 * to simply concatenate the hotspot specification onto the end of
 * a pre-written GIF file, e.g.:
 * <pre>
 * echo 'hotspot=3,5;' | cat infile.gif - >outfile.gif
 * </pre>
 * This second method relies on the fact that the GIF format has an
 * explicit end-of-file indicator, so anything after that is ignored
 * by any valid GIF reader.
 * On the other hand, the first method has the advantage that the
 * comment is preserved when an image is edited and saved with the
 * ImageMagick "display" or "convert" commands (but not by the
 * Solaris "Image Viewer").
 */
public class VCursor {

    /** A table of cursors we have already constructed. */
    private static Map<String, Cursor> m_cursorTable =
            Collections.synchronizedMap(new HashMap<String, Cursor>(32));


    /**
     * Constructor is private so no instances can be made.
     */
    private VCursor() {
    }

    /**
     * Returns the cursor for the given name.
     * If the name starts with the string "builtin-", looks for a Java
     * built-in cursor. The following built-in cursors from the
     * java.awt.Cursor class are supported:
     * <pre>
     *  builtin-crosshair
     *  builtin-e-resize
     *  builtin-hand
     *  builtin-move
     *  builtin-n-resize
     *  builtin-ne-resize
     *  builtin-nw-resize
     *  builtin-s-resize
     *  builtin-se-resize
     *  builtin-sw-resize
     *  builtin-text
     *  builtin-w-resize
     *  builtin-wait
     *  </pre>
     * If no built-in cursor is found
     * looks for a .gif file of the given base name -- typically found in
     * a jar file.
     * Looks for a hotspot location in the cursor image file. This is a
     * string of the form <code>hotspot=<i>x</i>,<i>y</i>;</code>,
     * where <i>x</i> and <i>y</i> are the coordinates of the hotspot
     * relative to the upper-left corner of the cursor in pixels.
     * No spaces allowed in this string!
     * This may be inserted in the comment field of the GIF file, or
     * just appended to the end of the file
     * (<code>cat in.gif hotstring >out.gif</code>).
     * <p>
     * Keeps a cache of cursors, so that any cursor is only created once.
     * @param strName The base name of the cursor.
     * @return The cursor for the specified name, or the default cursor
     * if no valid image file is found.
     */
    public static Cursor getCursor(String strName)
    {
        Cursor curs = (Cursor)m_cursorTable.get(strName);
        if (curs == null && strName.startsWith("builtin-")) {
            strName = strName.substring(8);
            if (strName.equals("crosshair")) {
                curs = Cursor.getPredefinedCursor(Cursor.CROSSHAIR_CURSOR);
            } else if (strName.equals("e-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.E_RESIZE_CURSOR);
            } else if (strName.equals("hand")) {
                curs = Cursor.getPredefinedCursor(Cursor.HAND_CURSOR);
            } else if (strName.equals("move")) {
                curs = Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR);
            } else if (strName.equals("n-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.N_RESIZE_CURSOR);
            } else if (strName.equals("ne-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.NE_RESIZE_CURSOR);
            } else if (strName.equals("nw-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.NW_RESIZE_CURSOR);
            } else if (strName.equals("s-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.S_RESIZE_CURSOR);
            } else if (strName.equals("se-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR);
            } else if (strName.equals("sw-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.SW_RESIZE_CURSOR);
            } else if (strName.equals("text")) {
                curs = Cursor.getPredefinedCursor(Cursor.TEXT_CURSOR);
            } else if (strName.equals("w-resize")) {
                curs = Cursor.getPredefinedCursor(Cursor.W_RESIZE_CURSOR);
            } else if (strName.equals("wait")) {
                curs = Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR);
            }
        }
        if (curs != null) {
            return curs;
        }

        // Don't have this cursor yet - try to make it.
        Point hotspot = null;
        String filename = strName + ".gif";
        Image img = Util.getVnmrImage(filename);

        if (img != null) {
            // See if hotspot is defined in image file
            InputStream in = Util.getVnmrImageStream(filename);
            if (in != null) {
                try {
                    // Avoid multi-byte character interpretations
                    BufferedReader bufin = new BufferedReader
                            (new InputStreamReader(in, "US-ASCII"));
                    String line;
                    boolean ok = true;
                    while ((line = bufin.readLine()) != null) {
                        int idx = line.indexOf("hotspot=");
                        if (idx >= 0) {
                            try {
                                StringTokenizer toker = new StringTokenizer
                                        (line.substring(idx + 8), ",;", true);
                                int x = Integer.parseInt(toker.nextToken());
                                ok &= toker.nextToken().equals(",");
                                int y = Integer.parseInt(toker.nextToken());
                                ok &= toker.nextToken().equals(";");
                                if (ok) {
                                    hotspot = new Point(x, y);
                                    break;
                                }
                            } catch (NoSuchElementException nsee) {
                                // Invalid hotspot string
                            } catch (NumberFormatException nfe) {
                                // No valid hotspot string
                            }
                        }
                    }
                } catch (UnsupportedEncodingException uee) {
                    // All systems required to support this, but:
                    Messages.postDebug("US-ASCII encoding not supported");
                } catch (IOException e1) {}
                try {
                    in.close();
                } catch (IOException e1) {}
            }

            Messages.postDebug("hotspot",
                               "Hotspot in file " + strName + ": " + hotspot);
                           
            if (hotspot == null) {
                // Try to get a hardcoded hotspot location
                hotspot = getPoint(strName);
                Messages.postDebug("Warning: No hotspot found in cursor image: "
                                   + filename
                                   + "  Using hardcoded hotspot: " + hotspot);
            }
            Toolkit tk = Toolkit.getDefaultToolkit();
            try {
                curs = tk.createCustomCursor(img, hotspot, strName);
            } catch (Exception e) {
                Messages.writeStackTrace(e);
            }
        }
        if (curs == null) {
            curs = Cursor.getPredefinedCursor(Cursor.DEFAULT_CURSOR);
        }
        m_cursorTable.put(strName, curs);

        return curs;
    }

    /**
     * Get hardcoded hotspot information for a particular cursor.
     * This method should only be called if the image file for the
     * cursor does not contain a hotspot specification.
     * @param strName The base file name (without extension).
     * @return A Point containing the hotspot coordinates.
     */
    private static Point getPoint(String strName)
    {
        Point pt = new Point(0, 0);
        if (strName != null) {
            if (strName.equals("zoom")) {
                pt.setLocation(10, 10);
            } else if (strName.equals("intensity")) {
                pt.setLocation(14, 14);
            } else if (strName.equals("intensity2")) {
                pt.setLocation(14, 18);
            } else if (strName.equals("horizontalMove")) {
                pt.setLocation(11, 5);
            } else if (strName.equals("verticalMove")) {
                pt.setLocation(5, 11);
            } else if (strName.equals("northResize")) {
                pt.setLocation(8, 2);
            } else if (strName.equals("southResize")) {
                pt.setLocation(8, 17);
            } else if (strName.equals("eastResize")) {
                pt.setLocation(17, 8);
            } else if (strName.equals("westResize")) {
                pt.setLocation(2, 8);
            }
        }
        return pt;
    }

}

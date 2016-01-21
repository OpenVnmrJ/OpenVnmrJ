/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.lc.jumbuck;

import java.io.*;


/**
 * Reads data from the stream assuming it is in Little-Endian byte order.
 * Except for byte order, similar to java.io.DataInputStream.
 * Code is largely copied from javax.imageio.stream.ImageInputStreamImpl.
 */
public class JbInputStream extends BufferedInputStream {

    /**
     * Constructor only calls the constructor for the underlying
     * BufferedInputStream.
     */
    public JbInputStream(InputStream stream) {
        super(stream);
    }

    public int readUnsignedByte() throws IOException {
        int ch = this.read();
        if (ch < 0) {
            throw new EOFException();
        }
        return ch;
    }

    public short readShort() throws IOException {
        int ch1 = this.read();
        int ch2 = this.read();
        if ((ch1 | ch2) < 0) {
            throw new EOFException();
        }
        return (short)((ch2 << 8) + (ch1 << 0));
    }

    public int readUnsignedShort() throws IOException {
        return ((int)readShort()) & 0xffff;
    }

    public int readInt() throws IOException {
        int ch1 = this.read();
        int ch2 = this.read();
        int ch3 = this.read();
        int ch4 = this.read();
        if ((ch1 | ch2 | ch3 | ch4) < 0) {
            throw new EOFException();
        }
        return ((ch4 << 24) + (ch3 << 16) + (ch2 << 8) + (ch1 << 0));
    }

    public float readFloat() throws IOException {
        return Float.intBitsToFloat(readInt());
    }

    public void readFully(byte[] b, int off, int len) throws IOException {
        // Fix 4430357 - if off + len < 0, overflow occurred
        if (off < 0 || len < 0 || off + len > b.length || off + len < 0) {
            throw new IndexOutOfBoundsException
                ("off < 0 || len < 0 || off + len > b.length!");
        }

        while (len > 0) {
            int nbytes = read(b, off, len);
            if (nbytes == -1) {
                throw new EOFException();
            }
            off += nbytes;
            len -= nbytes;
        }
    }

    public void readFully(byte[] b) throws IOException {
        readFully(b, 0, b.length);
    }

}

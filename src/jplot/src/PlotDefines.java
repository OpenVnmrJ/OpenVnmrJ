/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * 
 *
 */

public interface PlotDefines
{
    static final String vprog = "jplot";
    static final String vcommand = "jplot(";
    static final int IDLE = 0;
    static final int TEXT = 1;
    static final int LINE = 2;
    static final int ARROW = 3;
    static final int DLINE = 4;
    static final int SQUARE = 5;
    static final int MSQUARE = 6;
    static final int ARROWUP = 7;
    static final int ARROWDN = 8;
    static final int ARROWL = 9;
    static final int ARROWR = 10;
    static final int ARROWRDN = 11;
    static final int ARROWLDN = 12;
    static final int ARROWRUP = 13;
    static final int ARROWLUP = 14;
    static final int IMOVE = 15;
    static final int TEXTBUT = 16;
    static final int COLORBUT = 17;
    static final int ERASE = 18;
    static final int ERASEALL = 19;
    static final int ARROWRR = 20;
    static final int ARROWLL = 21;
    static final int ARROWRR2 = 22;
    static final int ARROWLL2 = 23;
    static final int GARROW = 24;
    static final int PRINT = 25;
    static final int PRESS = 0x01;
    static final int RELEASE = 0x02;
    static final int MOVE = 0x04;
    static final int DRAG = 0x08;
    static final int EXIT = 0x10;
    static final int NEW = 0x20;
    static final int RESIZE = 0x40;
    static final int HMOVE = 0x80;
    static final int MODIFY = 0x100;
}


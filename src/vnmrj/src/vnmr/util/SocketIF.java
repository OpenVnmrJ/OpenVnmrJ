/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.util;

import java.io.*;

/**
 * An SocketIF provides the interface for caller and socket.
 *
 */
public interface  SocketIF {
    public void processGraphData(DataInputStream in, int type);
    public void processComData(String str);
    public void processAlphaData(String str);
    public void processMasterData(String str);
    public void childProcessExit();
    public void graphSocketError(Exception e);
    public void comSocketError(Exception e);
    public void alphaSocketError(Exception e);
    public void graphSocketReady();
    public void statusProcessExit();
    public void statusProcessError();
    public void processStatusData(String str);
    public void processPrintData(DataInputStream in, int type);
} // SocketIF

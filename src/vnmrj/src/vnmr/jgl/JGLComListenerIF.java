/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

public interface  JGLComListenerIF {
    // Command return codes sent to vnmrj via graphToVnmrJ
    // NOTE: these definitions must match those in graphics3D.c (vnmrbg)


    static final int SETSTATUS = 0;

    static final int READFDF = 21;
    static final int BGNXFER = 22;
    static final int GETDATA = 23;
    static final int GETFDF = 24;
    static final int GETPOINT = 25;
    static final int SETSEM = 26;
    static final int GETERROR = 100;
    

    public static final int CMND=          0; 
    public static final int SHOWFIELD=     1; 
    public static final int STATUSFIELD=   2; 
    public static final int INTEGER=       3;
    public static final int FLOAT=         4; 
    public static final int POINT=         5; 
    public static final int STRING=        6; 
    public static final int PREFS=         7; 
    
    public static final int G3DI=          1; 
    public static final int G3DS=          2; 
    public static final int G3DGI=         3; 
    public static final int G3DGF=         4; 
    public static final int G3DF=          5; 
    public static final int G3DP=          6; 


    //  command codes sent from vnmrj via vnmrjcmd('g3d',<key>,..)
    //                                or  java('g3d',<key>,..)

    static final int G3DINIT    = 3;  // key='init'
    static final int G3DRESET   = 4;  // key='reset'
    static final int G3DSTEP    = 5;  // key='step'
    static final int G3DCONNECT = 6;  // key='connect'
    static final int G3DREPAINT = 7;  // key='repaint'
    static final int G3DSHOWREV = 8;  // key='version'
    static final int G3DSETSEM  = 9;  // key='setsem'    
    static final int G3DRUN     = 10; // key='run'    
    static final int G3DREVERSE = 11; // key='reverse'    
    static final int G3DBATCH   = 12; // key='batch'    
    static final int G3DSHOWING = 13; // key='showing'    
    static final int G3DIMAGE   = 14; // key='image'    
    
    // internal command codes
    
    static final int G3DSTATUS  = 20;
    static final int G3DSHOW    = 21;
    static final int G3DPREFS   = 22;

    // STRING variables
    static final int G3DAXIS=        0; // g3daxis
    static final int G3DVERSION=     1; // g3dversion
    static final int VOLMAPDIR=      2; // volmapdir
    // POINT variables
    static final int G3DPNT=         0; // g3dpnt
    static final int G3DROT=         1; // g3drot
    static final int G3DSCL=         2; // g3dscl
    // FLOAT variables (global)
    static final int G3DSXP=         1; // g3dgf[1]
    static final int G3DPXP=         2; // g3dgf[2]
    // FLOAT variables (current)
    static final int G3DINTENSITY=   1; // g3df[1]
    static final int G3DCONTRAST=    2; // g3df[2]
    static final int G3DTHRESHOLD=   3; // g3df[3]
    static final int G3DLIMIT=       4; // g3df[4]
    static final int G3DXPARENCY=    5; // g3df[5]
    static final int G3DDSCALE=      6; // g3df[6]
    static final int G3DZSCALE=      7; // g3df[7]
    static final int G3DSWIDTH=      8; // g3df[8]
    static final int G3DCWIDTH=      9; // g3df[9]
    static final int G3DCBIAS=       10;// g3df[10]
    // INTEGER variables (global)
    static final int G3DMAXS=        1; // g3dgi[1]
    static final int G3DMINS=        2; // g3dgi[2]

    public void setData(int flags, JGLData data);
    public void setComCmd(int id, int value);
    public void setIntValue(int id, int index, int value);
    public void setFloatValue(int id, int index, float value);
    public void setStringValue(int id, String value);
    public void setPointValue(int id, Point3D value);
}

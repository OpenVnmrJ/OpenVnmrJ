/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.jgl;

import javax.media.opengl.*;
import javax.media.opengl.glu.*;
import java.io.*;
import java.util.*;
import java.nio.*;

public class JGLVertexShader extends JGLShader
{
    public JGLVertexShader(){
        super(GL2.GL_VERTEX_SHADER,GL2.GL_VERTEX_PROGRAM_ARB);
    }
    String  name () {
        return "VertexShader";
    }
    public boolean isSupported (){
        return GLUtil.isExtensionSupported ("GL2.GL_ARB_vertex_program");
    }
}

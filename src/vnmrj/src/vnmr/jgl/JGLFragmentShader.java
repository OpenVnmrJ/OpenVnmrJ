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

public class JGLFragmentShader extends JGLShader
{
    // overrides

    public JGLFragmentShader(){
        super(GL2.GL_FRAGMENT_SHADER,GL2.GL_FRAGMENT_PROGRAM_ARB);
    }
    String name() {
        return "FragmentShader";
    }
    public boolean isSupported(){
        return GLUtil.isExtensionSupported ("GL.GL_ARB_fragment_program");
    }
}

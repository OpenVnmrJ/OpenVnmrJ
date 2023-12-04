/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.ui;

import java.io.*;
import java.util.*;
import javax.xml.bind.*;

import java.security.*;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2002</p>
 * <p> </p>
 *  not attributable
 *
 */

public class PasswordService
{

    private static PasswordService instance;

    private PasswordService()
    {
    }

    public synchronized String encrypt(String plaintext) throws UnsupportedEncodingException
    {
        MessageDigest md = null;
        try
        {
            md = MessageDigest.getInstance("SHA"); //step 2
        }
        catch(NoSuchAlgorithmException e)
        {
            throw new UnsupportedEncodingException(e.getMessage());
        }
        try
        {
            md.update(plaintext.getBytes("UTF-8")); //step 3
        }
        catch(UnsupportedEncodingException e)
        {
            throw new UnsupportedEncodingException(e.getMessage());
        }

        byte raw[] = md.digest(); //step 4
        String hash = DatatypeConverter.printBase64Binary(raw); //step 5
        plaintext = "";
        return hash; //step 6
    }

    public static synchronized PasswordService getInstance() //step 1
    {
        if(instance == null)
            return new PasswordService();
        else
            return instance;
    }
}

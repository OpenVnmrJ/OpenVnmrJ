/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

package vnmr.admin.ui;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

import vnmr.util.Messages;
import vnmr.util.FileUtil;


public class WSharedConsoleCommands {

    static String status() {
        String output=null;
        String cmd = WGlobal.SUDO + " " + FileUtil.sysdir() + "/bin/dtsharcntrl status";
        Runtime rt = Runtime.getRuntime();
        Process prcs = null;
        try {
            prcs = rt.exec(cmd);
            // Wait for it to complete
            prcs.waitFor();

            // Get any feedback from the command
            BufferedReader str = (new BufferedReader
                    (new InputStreamReader
                            (prcs.getInputStream())));

            output = str.readLine();
        }
        catch (Exception ex) {
            Messages.postError("Problem getting Shared Console status.");
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

        return output;
         
    }

    static String enable(String password) {
        String output=null;
        String cmd = WGlobal.SUDO + " " + FileUtil.sysdir() + "/bin/dtsharcntrl start "
                     + password;
        Runtime rt = Runtime.getRuntime();
        Process prcs = null;
        try {
            prcs = rt.exec(cmd);
            // Wait for it to complete
            prcs.waitFor();

            // Get any feedback from the command
            BufferedReader str = (new BufferedReader
                    (new InputStreamReader
                            (prcs.getInputStream())));

            output = str.readLine();
        }
        catch (Exception ex) {
            Messages.postError("Problem Starting Shared Console.");
            Messages.writeStackTrace(ex);
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }
        }

        return output;        
    }

    static String disable() {
        String output=null;
        String cmd = WGlobal.SUDO + " " + FileUtil.sysdir() + "/bin/dtsharcntrl stop";
        Runtime rt = Runtime.getRuntime();
        Process prcs = null;
        try {
            prcs = rt.exec(cmd);
            // Wait for it to complete
            prcs.waitFor();

            // Get any feedback from the command
            BufferedReader str = (new BufferedReader
                    (new InputStreamReader
                            (prcs.getInputStream())));

            output = str.readLine();
        }
        catch (Exception ex) {
            Messages.postError("Problem Stopping Shared Console.");
        }
        finally {
            // It is my understanding that these streams are left
            // open sometimes depending on the garbage collector.
            // So, close them.
            try {
                if(prcs != null) {
                    OutputStream os = prcs.getOutputStream();
                    if(os != null)
                        os.close();
                    InputStream is = prcs.getInputStream();
                    if(is != null)
                        is.close();
                    is = prcs.getErrorStream();
                    if(is != null)
                        is.close();
                }
            }
            catch (Exception ex) {
                Messages.writeStackTrace(ex);
            }

        }

        return output;   
    }


}

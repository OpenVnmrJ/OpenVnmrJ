
import java.io.*;
import java.util.*;

/**
 *
 * @author  mrani
 */

public class LoadNmrDir 
{
    
    /** Creates a new instance of LoadNmrDir */
    public LoadNmrDir(String[] args) 
    {
        String strPath = System.getProperty("vnmrcd");
        System.out.println(strPath);
        if (strPath == null)
        {       
            File file = new File("/rdvnmr");
            String[] files = file.list();
            ArrayList aListVj = new ArrayList();
            int nSize = files.length;
            for (int i = 0; i < nSize; i++)
            {
                String strfile = files[i];
                if (strfile.startsWith(".cd") && 
                    !strfile.endsWith(".passwords") &&
                    !strfile.endsWith("latest"))
                {
                    aListVj.add(strfile);
                }
            }
            
            Collections.sort(aListVj);
            
            int nLength = aListVj.size();
            strPath = "/rdvnmr/"+(String)aListVj.get(nLength-1);
        }
        
        String strPassword = "";
        if (args != null && args.length > 0)
            strPassword = args[0];
        System.out.println(strPath);
        String strtype = System.getProperty("installtype");
        if (strtype == null || strtype.equals(""))
            strtype = "inova";
        try
        {
            String[] cmd = {"/bin/sh", "-c", "./netbeans installvj " + strPath + 
                                                " \""  + strPassword + "\"" +
                                                strtype};
            
            Runtime rt = Runtime.getRuntime();
            Process prcs = rt.exec(cmd);
            InputStream input = prcs.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(input));
            
            String strline;
            while ((strline = reader.readLine()) != null)
            {
                System.out.println(strline);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) 
    {
        LoadNmrDir loadnmrdir = new LoadNmrDir(args);
        
    }
    
}

:: Windows Batch file
:: create the various exe wrapper for vnmrj
"C:\Program Files\JSmooth 0.9.9-7\jsmoothcmd.exe" ./vnmrj_jsmooth.xml.jsmooth
"C:\Program Files\JSmooth 0.9.9-7\jsmoothcmd.exe" ./vnmrj_jsmooth_adm.xml.jsmooth
"C:\Program Files\JSmooth 0.9.9-7\jsmoothcmd.exe" ./vnmrj_jsmooth_debug.xml.jsmooth
"C:\Program Files\JSmooth 0.9.9-7\jsmoothcmd.exe" ./vnmrj_jsmooth_ja.xml.jsmooth
"C:\Program Files\JSmooth 0.9.9-7\jsmoothcmd.exe" ./vnmrj_jsmooth_zh.xml.jsmooth
cp vnmrj_adm.exe  vnmrj_debug.exe  vnmrj.exe  vnmrj_ja.exe  vnmrj_zh.exe ../../../vnmr/bin

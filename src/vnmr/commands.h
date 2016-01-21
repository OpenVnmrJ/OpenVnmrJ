/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------------------------
|	commands.h
|
|	This include file contains the names, addresses, and other
|	information about all commands.
+-------------------------------------------------------------------------*/
#include <stdio.h>
#define REEXEC 		1
#define NO_REEXEC	0


/* commands MUST be alphabetized */
extern acqproc_msge();
extern ai();
extern acosy();
extern acq();
extern acqdisp();
extern acqhwcmd();
extern acqsend();
extern acqstatus();
extern addsub();
extern adept();
extern analyze();
extern aph();
extern autocmd();
extern autogo();
extern autora();
extern autosa();
extern averag();
extern axis_info();
extern banner();
extern bc();
extern beeper();
extern calcdim();
extern cexp();
extern clear();
extern confirm();
extern create();
extern data_dir();		/* magnetom */
extern dc2d();
extern dcon();
extern dconi();
extern ddf();
extern debuger();
extern delem();
extern delexp();
extern dels();
extern destroy();
extern destroygroup();
extern devicenames();
extern df2d();
extern dfid();
extern dfww();
extern display();
extern dg();
extern dlalong();
extern dli();
extern dll();
extern dpcon();
extern dpf();
extern dpir();
extern dps();
extern pps();
extern dres();
extern ds();
extern dsn();
extern dsp();
extern dsww();
extern ds2d();
extern dscale();
extern echo();
extern eleminfo();
extern enumeral();
extern ernst();
extern errlog();
extern exec();
extern exists();
extern expactive();
extern export();
extern export_files();			/* magnetom */
extern exports();
extern expl_cmd();
extern f();
extern fiddle();
extern files();
extern filesinfo();
extern fitspec();
extern flashc();			/* flashc reformatting */
extern flip();
extern flush();
extern foldcc();
extern foldj();
extern foldt();
extern format();
extern fp();
extern f_read();
extern fsave();
extern ft();
extern ft2d();
extern full();
extern getfile();
extern getvalue();
extern glide();
extern graph_is();
extern groupcopy();
extern gzfit();
extern h2cal();
extern hpa();
extern help();
extern ia_start();
extern ilfid();
extern import_files();			/* magnetom */
extern inset();
extern interact();
extern jexp();
extern large();
extern length();
extern ll2d();
extern ln();
extern lookup();
extern macroCat();
extern macroCp();
extern macroDir();
extern macroLd();
extern macroRm();
extern magnetom();			/* magnetom */
extern makefid();
extern mark();
extern menu();
extern mp();
extern mstat();
extern noise();
extern nmrExit();
extern nmr_draw();
extern nmr_write();
extern Off();
extern output_sdf();			/* ImageBrowser */
extern page();
extern peak2d();
extern print_on();
extern printoff();
extern proj();
extern prune();
extern purgeCache();
extern quadtt();
extern raster_dump();
extern readhw();
extern readlk();
extern real();
extern recon();
extern resume();
extern region();
extern rotate();
extern rt();
extern rtphf();
extern rts();
extern Rinput();
extern s();
extern selbuf();
extern selecT();
extern set_mag_login();			/* magnetom */
extern set_node_name();			/* magnetom */
extern set_password();			/* magnetom */
extern set_remote_dir();		/* magnetom */
extern set_user_name();			/* magnetom */
extern set3dproc();
extern setenumeral();
extern setfrq();
extern setgroup();
extern sethw();
extern setlimit();
extern setplotdev();
extern setprintdev();
extern setprotect();
extern settype();
extern setvalue();
extern shellcmds();
extern show_local_files();		/* magnetom */
extern show_remote_files();		/* magnetom */
extern showbuf();
extern small();
extern solvinfo();
extern spa();
extern spins();
extern sread();
extern status();
extern string();
extern substr();
extern svf();
extern svphf();
extern svs();
extern swrite();
extern tempcal();
extern text();
extern text_is();
extern timeit();
extern tune();
extern unixtime();
extern vnmr_unlock();
extern vnmrInfo();
extern werr();
extern writefid();
extern writeparam();
extern wrspec();
extern wsram();
extern wti();

cmd table[] = { {"aa"         , acqproc_msge,	NO_REEXEC},
		{"abortallacqs", acqproc_msge,	NO_REEXEC},
		{"acos"       , ln,		NO_REEXEC},
		{"acosy"      , acosy,		NO_REEXEC},
		{"Acq"        , acq,		NO_REEXEC},
		{"acqdebug"   , acqproc_msge,	NO_REEXEC},
		{"acqdisp"    , acqdisp,	NO_REEXEC},
		{"acqipctst"  , acqproc_msge,	NO_REEXEC},
		{"acqsend"    , acqsend,	NO_REEXEC},
		{"acqstatus"  , acqstatus,	NO_REEXEC},
		{"add"        , addsub,		NO_REEXEC},
		{"addi"       , addsub,		NO_REEXEC},
		{"adept"      , adept,		NO_REEXEC},
		{"ai"         , ai,		NO_REEXEC},
		{"analyze"    , analyze,	NO_REEXEC},
		{"ap"         , dg,		NO_REEXEC},
		{"aph"        , aph,		NO_REEXEC},
		{"aph0"       , aph,		NO_REEXEC},
		{"asin"       , ln,		NO_REEXEC},
		{"atan"       , ln,		NO_REEXEC},
		{"atan2"      , ln,		NO_REEXEC},
		{"auto"       , autocmd,	NO_REEXEC},
		{"autogo"     , autogo, 	NO_REEXEC},
		{"autora"     , autora, 	NO_REEXEC},
		{"autosa"     , autosa, 	NO_REEXEC},
		{"av"         , ai,		NO_REEXEC},
		{"av1"	      ,	ai,		NO_REEXEC},
		{"av2"	      ,	ai,		NO_REEXEC},
		{"averag"     , averag,		NO_REEXEC},
		{"axis"       , axis_info,	NO_REEXEC},
		{"banner"     , banner,		NO_REEXEC},
		{"bc"         , bc,		NO_REEXEC},
		{"bc2d"       ,	bc,		NO_REEXEC},
		{"beepoff"    , beeper,		NO_REEXEC},
		{"beepon"     , beeper,		NO_REEXEC},
		{"calcdim"    , calcdim,	NO_REEXEC},
		{"cat"	      , shellcmds,	NO_REEXEC},
		{"cd"	      , shellcmds,	NO_REEXEC},
		{"cdc"        , ai,		NO_REEXEC},
		{"center"     , full,		NO_REEXEC},
		{"cexp"       , cexp,		NO_REEXEC},
		{"change"     , acq,		NO_REEXEC},
		{"clear"      , clear,		NO_REEXEC},
		{"clradd"     , addsub,		NO_REEXEC},
		{"confirm"    , confirm,	NO_REEXEC},
		{"copy"	      , shellcmds,	NO_REEXEC},
		{"cos"	      , ln,		NO_REEXEC},
		{"cp"	      , shellcmds,	NO_REEXEC},
		{"create"     , create,		NO_REEXEC},
		{"ctext"      , text,		NO_REEXEC},
		{"cz"         , dli,		NO_REEXEC},
		{"da"         , dg,		NO_REEXEC},
                {"data_dir"   , data_dir,       NO_REEXEC},
		{"dc"         , ai,		NO_REEXEC},
		{"dc2d"       , dc2d,		NO_REEXEC},
		{"dcon"       , dcon,	        NO_REEXEC},
		{"dconi"      , dconi,	        REEXEC},
		{"dconn"      , dcon,	        NO_REEXEC},
		{"ddfp"       , ddf,		NO_REEXEC},
		{"ddf"        , ddf,		NO_REEXEC},
		{"ddff"       , ddf,		NO_REEXEC},
		{"debug"      , debuger,	NO_REEXEC},
		{"delem"      , delem,		NO_REEXEC},
		{"delete"     , shellcmds,	NO_REEXEC},
		{"delexp"     , delexp,		NO_REEXEC},
		{"dels"       , dels,		NO_REEXEC},
		{"destroy"    , destroy,	NO_REEXEC},
		{"delbuf"     , selbuf,		NO_REEXEC},
		{"destroygroup",destroygroup,	NO_REEXEC},
		{"devicenames",devicenames,	NO_REEXEC},
		{"df2d"       , df2d,		NO_REEXEC},
		{"df"         , dfid,	        REEXEC},
		{"dfid"       , dfid,	        REEXEC},
		{"dfs"        , dfww,		NO_REEXEC},
		{"dfsa"       , dfww,		NO_REEXEC},
		{"dfsan"      , dfww,		NO_REEXEC},
		{"dfsh"       , dfww,		NO_REEXEC},
		{"dfshn"      , dfww,		NO_REEXEC},
		{"dfsn"       , dfww,		NO_REEXEC},
		{"dfww"       , dfww,		NO_REEXEC},
		{"dfwwn"      , dfww,		NO_REEXEC},
		{"dir"	      , shellcmds,	NO_REEXEC},
		{"display"    , display,	NO_REEXEC},
		{"dg"         , dg,		REEXEC},
		{"dlalong"    , dlalong,	NO_REEXEC},
		{"dli"        , dli,		NO_REEXEC},
		{"dlni"       , dli,		NO_REEXEC},
		{"dll"        , dll,		NO_REEXEC},
		{"dpcon"      , dpcon,	        NO_REEXEC},
		{"dpconn"     , dpcon,	        NO_REEXEC},
		{"dpf"        , dpf,	        NO_REEXEC},
		{"dpir"       , dpir,	        NO_REEXEC},
		{"dpirn"      , dpir,	        NO_REEXEC},
		{"dps"        , dps,	        NO_REEXEC},
		{"pps"        , dps,	        NO_REEXEC},
		{"draw"       , nmr_draw,	NO_REEXEC},
		{"box"        , nmr_draw,	NO_REEXEC},
		{"dres"       , dres,		NO_REEXEC},
		{"ds"         , ds,		REEXEC},
		{"dsn"        , dsn,		NO_REEXEC},
		{"dsp"        , dsp,		NO_REEXEC},
		{"dss"        , dsww,		NO_REEXEC},
		{"dssa"       , dsww,		NO_REEXEC},
		{"dssan"      , dsww,		NO_REEXEC},
		{"dssh"       , dsww,		NO_REEXEC},
		{"dsshn"      , dsww,		NO_REEXEC},
		{"dssn"       , dsww,		NO_REEXEC},
		{"dsww"       , dsww,		NO_REEXEC},
		{"dswwn"      , dsww,		NO_REEXEC},
		{"ds2d"       , ds2d,		NO_REEXEC},
		{"ds2dn"      , ds2d,		NO_REEXEC},
		{"dscale"     , dscale,		NO_REEXEC},
		{"echo"       , echo,		NO_REEXEC},
		{"eleminfo"   , eleminfo,	NO_REEXEC},
		{"ernst"      , ernst,		NO_REEXEC},
		{"errlog"     , errlog,		NO_REEXEC},
		{"exec"       , exec,		NO_REEXEC},
		{"exists"     , exists,		NO_REEXEC},
		{"vnmrexit"   , nmrExit,	NO_REEXEC},
		{"exp"        , ln,		NO_REEXEC},
		{"expactive"  , expactive,	NO_REEXEC},
		{"export"     , export,		NO_REEXEC},
		{"exports"    , exports,	NO_REEXEC},
		{"expl"       , expl_cmd,	NO_REEXEC},
		{"f"          , f,		NO_REEXEC},
		{"fiddle"     , fiddle,		NO_REEXEC},
		{"fiddled"    , fiddle,		NO_REEXEC},
		{"fiddleu"    , fiddle,		NO_REEXEC},
		{"fiddle2d"   , fiddle,		NO_REEXEC},
		{"fiddle2D"   , fiddle,		NO_REEXEC},
		{"fiddle2Dd"  , fiddle,		NO_REEXEC},
		{"fiddle2dd"  , fiddle,		NO_REEXEC},
		{"fitspec"    , fitspec,	NO_REEXEC},
		{"files"      , files,		NO_REEXEC},
		{"filesinfo"  , filesinfo,	NO_REEXEC},
                {"flashc"     , flashc,         NO_REEXEC},
		{"flip"       , flip,		NO_REEXEC},
		{"flush"      , flush,		NO_REEXEC},
		{"fr"         , s,		NO_REEXEC},
		{"foldcc"     , foldcc,		NO_REEXEC},
		{"foldj"      , foldj,		NO_REEXEC},
		{"foldt"      , foldt,		NO_REEXEC},
		{"format"     , format,		NO_REEXEC},
		{"fp"         , fp,		NO_REEXEC},
		{"fread"      , f_read,		NO_REEXEC},
		{"fsave"      , fsave,		NO_REEXEC},
		{"ft"         , ft,		NO_REEXEC},
		{"ft1d"       , ft2d,		NO_REEXEC},
		{"ft2d"       , ft2d,		NO_REEXEC},
		{"full"       , full,		NO_REEXEC},
		{"fullt"      , full,		NO_REEXEC},
		{"getfile"    , getfile,	NO_REEXEC},
		{"getll"      , dll,		NO_REEXEC},
                {"getmag"     , import_files,   NO_REEXEC},
		{"getreg"     , region,		NO_REEXEC},
		{"gettxt"     , rt,		NO_REEXEC},
		{"getvalue"   , getvalue,	NO_REEXEC},
		{"glide"      , glide,		NO_REEXEC},
		{"graphis"    , graph_is,	NO_REEXEC},
		{"groupcopy"  , groupcopy,	NO_REEXEC},
		{"gzfit"      , gzfit,		NO_REEXEC},
		{"h2cal"      , h2cal,		NO_REEXEC},
		{"halt"       , acqproc_msge,	NO_REEXEC},
		{"hpa"        , hpa,		NO_REEXEC},
		{"help"       , help,		NO_REEXEC},
		{"acqi"       , ia_start,	NO_REEXEC},
		{"ilfid"      , ilfid,		NO_REEXEC},
		{"input"      , Rinput,		NO_REEXEC},
		{"inset"      , inset,		NO_REEXEC},
		{"integ"      , dli,		NO_REEXEC},
		{"interact"   , interact,	NO_REEXEC},
		{"jexp"       , jexp,		NO_REEXEC},
		{"large"      , large,		NO_REEXEC},
		{"left"       , full,		NO_REEXEC},
		{"length"     , length,		NO_REEXEC},
		{"lock"       , acq,		NO_REEXEC},
		{"lf"         , shellcmds,	NO_REEXEC},
		{"ln"         , ln,		NO_REEXEC},
		{"ll2d"       , ll2d,		NO_REEXEC},
		{"lookup"     , lookup,		NO_REEXEC},
		{"ls"         , shellcmds,	NO_REEXEC},
		{"macrocat"   , macroCat,	NO_REEXEC},
		{"macrocp"    , macroCp,	NO_REEXEC},
		{"macrodir"   , macroDir,	NO_REEXEC},
		{"macrold"    , macroLd,	NO_REEXEC},
		{"macrorm"    , macroRm,	NO_REEXEC},
		{"macrosyscat", macroCat,	NO_REEXEC},
		{"macrosyscp" , macroCp,	NO_REEXEC},
		{"macrosysdir", macroDir,	NO_REEXEC},
		{"macrosysrm" , macroRm,	NO_REEXEC},
                {"magnetom"   , magnetom,       NO_REEXEC},
		{"makefid"    , makefid,	NO_REEXEC},
		{"mark"       , mark,		NO_REEXEC},
		{"md"         , mp,		NO_REEXEC},
		{"menu"       , menu,		NO_REEXEC},
		{"mf"         , mp,		NO_REEXEC},
		{"mkdir"      , shellcmds,	NO_REEXEC},
		{"move"       , nmr_draw,	NO_REEXEC},
		{"hztomm"     , nmr_draw,	NO_REEXEC},
		{"mp"         , mp,		NO_REEXEC},
		{"mstat"      , mstat,		NO_REEXEC},
		{"mv"         , shellcmds,	NO_REEXEC},
		{"newmenu"    , menu,		NO_REEXEC},
		{"nl"         , dll,		NO_REEXEC},
		{"nli"        , dli,		NO_REEXEC},
		{"nll"        , dll,		NO_REEXEC},
		{"nlni"       , dli,		NO_REEXEC},
		{"nm"         , ai,		NO_REEXEC},
		{"noise"      , noise,		NO_REEXEC},
		{"numreg"     , region,		NO_REEXEC},
		{"off"        , Off,		NO_REEXEC},
		{"on"         , Off,		NO_REEXEC},
		{"p1"         , ernst,		NO_REEXEC},
		{"pacosy"     , acosy,		NO_REEXEC},
		{"padept"     , adept,		NO_REEXEC},
		{"page"       , page,		NO_REEXEC},
		{"pap"        , dg,		NO_REEXEC},
		{"pcon"       , dpcon,	        NO_REEXEC},
		{"peak"       , dll,		NO_REEXEC},
		{"peak2d"     , peak2d,		NO_REEXEC},
		{"pen"        , nmr_draw,	NO_REEXEC},
		{"pexpl"      , expl_cmd,	NO_REEXEC},
		{"pfww"       , dfww,	        NO_REEXEC},
		{"ph"         , ai,		NO_REEXEC},
		{"ph1"	      ,	ai,		NO_REEXEC},
		{"ph2"	      ,	ai,		NO_REEXEC},
		{"pir"        , dpir,	        NO_REEXEC},
		{"pirn"       , dpir,	        NO_REEXEC},
		{"pl"         , dsww,		NO_REEXEC},
		{"pll2d"      , ll2d,		NO_REEXEC},
		{"plww"       , dsww,		NO_REEXEC},
		{"pl2d"       , ds2d,		NO_REEXEC},
		{"plfid"      , dfww,	        NO_REEXEC},
		{"ppf"        , dpf,	        NO_REEXEC},
		{"printon"    , print_on,	NO_REEXEC},
		{"printoff"   , printoff,	NO_REEXEC},
		{"proj"       , proj,		NO_REEXEC},
		{"prune"      , prune,		NO_REEXEC},
		{"pscale"     , dscale,		NO_REEXEC},
		{"purge"      , purgeCache,	NO_REEXEC},
                {"putf"       , export_files,   NO_REEXEC},
                {"puti"       , export_files,   NO_REEXEC},
                {"putp"       , export_files,   NO_REEXEC},
		{"puttxt"     , rt,		NO_REEXEC},
		{"pw"         , ernst,		NO_REEXEC},
		{"pwd"        , shellcmds,	NO_REEXEC},
		{"pwr"	      , ai,		NO_REEXEC},
		{"pwr1"	      , ai,		NO_REEXEC},
		{"pwr2"	      , ai,		NO_REEXEC},
		{"r"          , s,		NO_REEXEC},
		{"ra"         , acq,		NO_REEXEC},
		{"quadtt"     , quadtt, 	NO_REEXEC},
		{"readhw"     , readhw, 	NO_REEXEC},
		{"readlk"     , readlk, 	NO_REEXEC},
		{"real"       , real,		NO_REEXEC},
		{"recon"      , recon,		NO_REEXEC},
		{"rename"     , shellcmds,	NO_REEXEC},
		{"region"     , region,		NO_REEXEC},
		{"resume"     , resume,         NO_REEXEC},
		{"right"      , full,		NO_REEXEC},
		{"rm"         , shellcmds,	NO_REEXEC},
		{"rmdir"      , shellcmds,	NO_REEXEC},
		{"rotate"     , rotate,		NO_REEXEC},
		{"RT"         , rt,		NO_REEXEC},
		{"RTP"        , rt,		NO_REEXEC},
		{"rtphf"      , rtphf,		NO_REEXEC},
		{"rts"        , rts,		NO_REEXEC},
		{"rtv"        , rt,		NO_REEXEC},
		{"s"          , s,		NO_REEXEC},
		{"sa"         , acqproc_msge,	NO_REEXEC},
		{"sample"     , acq,		NO_REEXEC},
		{"selbuf"     , selbuf,		NO_REEXEC},
		{"select"     , selecT,		NO_REEXEC},
                {"set_mag_login", set_mag_login, NO_REEXEC},
                {"set_node_name", set_node_name, NO_REEXEC},
                {"set_password", set_password,   NO_REEXEC},
                {"set_remote_dir", set_remote_dir,  NO_REEXEC},
                {"set_user_name", set_user_name, NO_REEXEC},
		{"set3dproc"  ,	set3dproc,	NO_REEXEC},
		{"setenumeral", setenumeral,	NO_REEXEC},
		{"setfrq"     , setfrq, 	NO_REEXEC},
		{"setgroup"   , setgroup,	NO_REEXEC},
		{"sethw"      , sethw,		NO_REEXEC},
		{"setlimit"   , setlimit,	NO_REEXEC},
		{"setplotdev" , setplotdev,	NO_REEXEC},
		{"setprintdev", setprintdev,	NO_REEXEC},
		{"setprotect" , setprotect,	NO_REEXEC},
		{"settype"    , settype,	NO_REEXEC},
		{"setvalue"   , setvalue,	NO_REEXEC},
		{"shell"      , shellcmds,	NO_REEXEC},
		{"shelli"     , shellcmds,	NO_REEXEC},
		{"shim"       , acq,		NO_REEXEC},
                {"show_local_files",  show_local_files,   NO_REEXEC},
                {"show_remote_files", show_remote_files,  NO_REEXEC},
		{"showbuf"    , showbuf,	NO_REEXEC},
		{"sin"        , ln,	 	NO_REEXEC},
		{"small"      , small,		NO_REEXEC},
		{"solvinfo"   , solvinfo,	NO_REEXEC},
		{"spa"        , spa,		NO_REEXEC},
		{"spadd"      , addsub,		NO_REEXEC},
		{"spin"       , acq,		NO_REEXEC},
		{"spins"      , spins,		NO_REEXEC},
		{"spmin"      , addsub,		NO_REEXEC},
		{"spsub"      , addsub,		NO_REEXEC},
		{"sread"      , sread,		NO_REEXEC},
		{"status"     , status,		NO_REEXEC},
		{"string"     , string,		NO_REEXEC},
		{"su"         , acq,		NO_REEXEC},
		{"sub"        , addsub,		NO_REEXEC},
		{"substr"     , substr,		NO_REEXEC},
		{"svd"        , raster_dump,	NO_REEXEC},
		{"SVF"        , svf,		NO_REEXEC},
		{"svs"        , svs,		NO_REEXEC},
		{"SVP"        , svf,		NO_REEXEC},
		{"svdat"      , output_sdf,     NO_REEXEC},
		{"svsdfd"     , output_sdf,     NO_REEXEC},
		{"svphf"      , svphf,		NO_REEXEC},
		{"swrite"     , swrite,		NO_REEXEC},
		{"sync"       , flush,		NO_REEXEC},
		{"systemtime" , unixtime,       NO_REEXEC},
		{"tan"        , ln,	 	NO_REEXEC},
		{"tempcal"    , tempcal,	NO_REEXEC},
		{"text"       , text,		NO_REEXEC},
		{"textis"     , text_is,	NO_REEXEC},
		{"timeit"     , timeit,		NO_REEXEC},
		{"tune"       , tune,	        NO_REEXEC},
		{"unixtime"   , unixtime,       NO_REEXEC},
		{"vnmr_unlock", vnmr_unlock,	NO_REEXEC},
		{"vnmrinfo"   , vnmrInfo,	NO_REEXEC},
		{"w"          , shellcmds,	NO_REEXEC},
		{"wbs"        , werr,		NO_REEXEC},
		{"werr"       , werr,		NO_REEXEC},
		{"wexp"       , werr,		NO_REEXEC},
		{"wft"        , ft,		NO_REEXEC},
		{"wft1d"      , ft2d,		NO_REEXEC},
		{"wft2d"      , ft2d,		NO_REEXEC},
		{"wnt"        , werr,		NO_REEXEC},
		{"write"      , nmr_write,	NO_REEXEC},
		{"writefid"   , writefid,	NO_REEXEC},
		{"writeparam" , writeparam,	NO_REEXEC},
		{"wrspec"     , wrspec,		NO_REEXEC},
		{"wsram"      , wsram,		NO_REEXEC},
		{"wti"        , wti,		NO_REEXEC},
		{"z"          , dli,		NO_REEXEC},
		{NULL         ,  NULL,              0    }	
	      };	


/*
 * acqinfo.x 1. Remote reading of acqinfo via RPC.
 *           2. Pinging of a PID to establish if process is running.
 * generates acqinfo.h & acqinfo.c via "rpcgen acqinfo.x"
 * generates acqinfo_svc.c via "rpcgen -s tcp acqinfo.x -o acqinfo_svc.c"
 *
 * 9-12-89 Altered for SUN OS 4.0.3  Greg Brissey
 * generates acqinfo.h, acqinfo_xdr.c, acqinfo_clnt.c & acqinfo_svc.c (udp) 
 *   via "rpcgen acqinfo.x"
 *
 * generates acqinfo_svc.c using tcp via "rpcgen -s tcp acqinfo.x -o acqinfo_svc.c"
 *
 * These generated files have been modified and put into sccs and are no longer
 * generated via rpcgen.
 *
 */

typedef string a_string<>;

struct acqdata { 
		int pid;
		int pid_active;
		int rdport;
		int wtport;
		int msgport;
		string host<>;
	      };

struct ft3ddata	{
		 string autofilepath<>;
		 string procmode<>;
		 string username<>;
		 string pathenv<>;
	       };

program ACQINFOPROG {
   version ACQINFOVERS {
      acqdata ACQINFO_GET(int) = 1;
      int ACQPID_PING(int) = 2;
      int FT3D_START(ft3ddata) = 3;
   } = 2;
} = 99;


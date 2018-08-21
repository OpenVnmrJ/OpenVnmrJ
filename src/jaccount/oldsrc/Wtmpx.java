/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
/* char                 ut_user[32];   /* user login name */
/* char                 ut_id[4];      /* /etc/inittab id (usually line #) */
/* char                 ut_line[32];   /* device name (console, lnxx) */
/* pid_t                ut_pid;        /* process id */
/* short                ut_type;       /* type of entry */
/* struct exit_status   ut_exit;       /* exit status of a process */
/*                                     /* marked as DEAD_PROCESS */
/* struct timeval       ut_tv;         /* time entry was made */
/* int                  ut_session;    /* session ID, used for windowing */
/* short                ut_syslen;     /* significant length of ut_host */
/*                                     /* including terminating null */
/* char                 ut_host[257];  /* host name, if remote */
/*
/* The exit_status stru    cture includes the following members:
/* short   e_termination;   /* termination status */
/* short   e_exit;          /* exit status */
/*
/* and the timeval structure:
/* struct timeval {
/*           time_t    tv_sec;	      /* seconds since Jan. 1, 1970 */
/*           long tv_usec;	      /* and microseconds */
/*           };
*/
package accounting;

import java.awt.*;
import java.util.Date;

public class Wtmpx {

  final int LOGIN  = 7;
  final int LOGOUT = 8;
  final int GO     = 3;
  final int GONE   = 4;

  char[]     ut_user = new char[32];
  char[]     ut_id   = new char[4];
  char[]     ut_line = new char[32];
  int        ut_pid;
  short      ut_type;
  ExitStatus ut_exit = new ExitStatus();
  TimeVal    ut_tv   = new TimeVal();
  int        ut_session;
  short      ut_syslen;
  char[]     ut_host = new char[257];

  class ExitStatus {
    short e_termination;
    short e_exit;
  }
  class TimeVal {
    int   tv_sec;
    int   tv_usec;
  }

  public Wtmpx() {
  }

  StringBuffer sb;
  public String toString() {
    sb = new StringBuffer();
    sb.append("user=");
    sb.append(ut_user);
    sb.append(",id=");
    sb.append(ut_id);
//    sb.append(",ut_line='");
//    sb.append(ut_line);
    sb.append(",pid=");
    sb.append(ut_pid);
    sb.append(", type=");
    sb.append(ut_type);
//    sb.append(",termination=");
//    sb.append(ut_exit.e_termination);
//    sb.append(",exit=");
//    sb.append(ut_exit.e_exit);
//    sb.append(",tv_sec=");
//    sb.append(ut_tv.tv_sec);
//    sb.append(",tv_usec=");
//    sb.append(ut_tv.tv_usec+", ");
    sb.append(new Date((long)ut_tv.tv_sec*1000));
//    sb.append(",session=");
//    sb.append(ut_session);
    sb.append(",syslen=");
    sb.append(ut_syslen);
    sb.append(",host=");
    sb.append(ut_host);

    return sb.toString();
  }

  char[] allChars = new char[372];


  public void fromChars(char[] allChar) {
    char tmp;
    int i; boolean echo=false;
    if ((allChar[0] =='g') && (allChar[1]=='r') && (allChar[2]=='e')) echo=true;
//    System.out.println("Ready to copy");
    ut_user = new char[32];
    i=0;
    while ((allChar[i] != 0) && (i<32))  {
      ut_user[i] = allChar[i]; i++;
    }
//    System.out.println("ut_user done");
    ut_id = new char[4];
    i=32;
    while ( (allChar[i] != 0) && (i<36)) {
      ut_id[i-32] = allChar[i]; i++;
    }
//    System.out.println("ut_id done");
    ut_line = new char[32];
    i=36;
    while ((allChar[i] != 0) && (i<68)) {
      ut_line[i-36] = allChar[i]; i++;
    }
//    System.out.println("ut_line done");
    ut_pid         = (allChar[68]<<24) + (allChar[69]<<16) + (allChar[70]<<8) + allChar[71];
    ut_type        = (short) ((allChar[72]<<8)  + allChar[73]);
    ut_exit.e_termination = (short)((allChar[74]<<8) + allChar[75]);
    ut_exit.e_exit = (short)((allChar[76]<<8)  + allChar[77]);
    ut_tv.tv_sec   = (allChar[80]<<24) + (allChar[81]<<16) + (allChar[82]<<8) + allChar[83];
    ut_tv.tv_usec  = (allChar[84]<<24) + (allChar[85]<<16) + (allChar[86]<<8) + allChar[87];
    ut_session     = (allChar[88]<<24) + (allChar[89]<<16) + (allChar[90]<<8) + allChar[91];
    ut_syslen      = (short)((allChar[112]<<8)  + allChar[113]);
    ut_host = new char[257];
    i=114;
    while ((allChar[i] != 0) && (i<372)) {
      ut_host[i-114] = allChar[i]; i++;
    }
  }
}
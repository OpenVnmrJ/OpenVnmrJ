/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SMAGIC_H
#define SMAGIC_H
extern void insertAcqMsgEntry(char *acqmptr );
extern void read_jcmd();
extern void setCancel(int doit, char *str);
extern void gotInterupt();
extern int get_vnmrj_port();
extern int get_vnmrj_socket();
extern int init_vnmrj_comm(char *hostname, int port_num);
extern void openVSocket(int fd);
extern void net_write(char *netAddr, char *netPort, char *message);
extern int smagicSendJvnmraddr(int portno); /* send Java vnmraddr */
extern int vnmr(int argc, char *argv[]);
extern char *getInputBuf(char *buf, int len);
extern void long_event();
extern void resetMasterSockets(); /* reset master sockets */
extern void JSocketIsReadNew(char tbuff[], int readchars);
extern void readVSocket(int fd);
extern void AcqSocketIsRead(int (*reader)());
extern void insertAcqMsgEntry(char *acqmptr );
extern void setVjGUiPort(int on);
extern void setVjUiPort(int on);
extern void setVjPrintMode(int on);
extern int writelineToVnmrJ(const char *cmd, const char *message );
extern void openPrintPort();
extern int graphToVnmrJ( char *message, int messagesize );
extern void resetjEvalGlo();
extern void appendJvarlist(const char *name);
extern void releaseJvarlist();
extern void appendTopPanelParam();
extern void jAutoRedisplayList();
extern void jsendParamMaxMin(int num, char *param, char *tree );
extern void jExpressionError(); /* if jexprDummyVar exists, error from evaluating dg-expr */
extern char *AnnotateExpression(char *exp, char *format);
extern char *AnnotateShow(char *exp);
#endif

/* © Copyright 1995, 2005, Richard M. Troth, all rights reserved.
 *                                                           <plaintext>
 *
 *        Name: uft.h
 *              Unsolicited File Transfer general C code header
 *      Author: Richard Troth, Houston, Texas, USA
 *        Date: 1995-Jan-15, 2005-Jun-05
 */

#ifndef         _UFT_HEADER_

#include "tcpio.h"

/*       define UFT_ANONYMOUS to use 'uftd' via Tor                   */

/* the version number and copyright */
#define         UFT_PROTOCOL    "UFT/2"
#define         UFT_VERSION     "POSIXUFT/1.10.6"
#define         UFT_COPYRIGHT   "© Copyright 1995-2025 Richard M. Troth"
#define         UFT_VRM         "1.10.6"

/* server constants */
/* the SPOOLDIR has a sub-directory for each recipient */
#ifndef         UFT_SPOOLDIR
#define         UFT_SPOOLDIR    "/var/spool/uft"
#endif

#ifndef         UFT_GID
#define         UFT_GID         0
#endif

#ifndef         UFT_PIPESDIR
#define         UFT_PIPESDIR    "/usr/lib/uft"
#endif

/* the SEQuence file name may be platform dependent */
#define         UFT_SEQFILE             ".seq"
#define         UFT_SEQFILE_ALT         "seqno"

/* file name extensions */
#define         UFT_EXT_CONTROL         ".cf" /* control, metadata */
#define         UFT_EXT_DATA            ".df" /* data */
#define         UFT_EXT_EXTRA           ".ef" /* auxdata, resource */
#define         UFT_EXT_LIST            ".lf" /* 'ls -l' format */
#define         UFT_EXT_WORK            ".wf"

/* client constants */
/* flag bits */
#define         UFT_BINARY      0x8000
#define         UFT_VERBOSE     0x4000

/* registered port for this service */
#define         UFT_PORT        608
#define         IDENT_PORT      113

#ifndef         BUFSIZ
#define         BUFSIZ          64512
#endif

#ifndef         NULL
#define         NULL            0x0000
#endif

#define         UFT_SYSLOG_FACILITY     LOG_UUCP

struct          UFTFILE
                        {
                /* to-and-from spool space */
                int     cfd;    /* control file descriptor */
                int     dfd;    /* data file descriptor */
                int     efd;    /* ext attr file descriptor */
                int     lfd;    /* log file descriptor */
                /* client-server interaction */
                int     sfd;    /* server stream */
                int     rfd;    /* response stream */
                /* (daemon uses both sets of fds) */
                char    *cfn;   /* control file path */
                char    *dfn;   /* data file path */
                char    *efn;   /* ext attr file path */
                char    *lfn;   /* log file path */

                char    from[64],
                        name[64],
                        type[8],
                        cc[8],
                        hold[8],
                        class[8],
                        devtype[8], keep[4], msg[4],
                        form[16], dist[16], dest[16];
                int     size, copies;
                char    title[64];
                        } ;

#define         UFT_B64_CODE    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

/* RDR FILE >this< SENT FROM $from RDR WAS yadda yadda */
#define         UFT_TYPE_A_EXPANSION    /* "ASCII" */ "TXT"
#define         UFT_TYPE_B_EXPANSION    "BIN"
#define         UFT_TYPE_C_EXPANSION    "PRT"
#define         UFT_TYPE_I_EXPANSION    /* "IMAGE" */ "BIN"
#define         UFT_TYPE_M_EXPANSION    "MAIL"
#define         UFT_TYPE_N_EXPANSION    "NETDATA"
#define         UFT_TYPE_P_EXPANSION    "PRT"
#define         UFT_TYPE_T_EXPANSION    "TXT"
#define         UFT_TYPE_U_EXPANSION    /* "UNSPEC" */ "BIN"
#define         UFT_TYPE_V_EXPANSION    "VAR" /* or "V16" */
/*
RDR FILE $FILE SENT FROM $RSCS RDR WAS #### ####
xxx FILE nnnn  SEND FROM u@h
 */

static char *uft_copyright = UFT_COPYRIGHT;


/* © Copyright 1996, Richard M. Troth, all rights reserved.  <plaintext>
 *              (casita sourced)
 *
 *        Name: msghndlr.h
 *              header file for  msgd.c  and  msgcat.c
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1994-Jul-26, 1996-Mar-24
 */

/* flags */
#define         MSG_IDENT               0x0001
#define         MSG_VERBOSE             0x0002

#define         MSG_MSP_HOST            "localhost"
#define         MSG_MSP_PORT            18

#define         MSG_UFT_HOST            "localhost"
#define         MSG_UFT_PORT            608



#include <unistd.h>
#include <time.h>
#include <fcntl.h>

struct uft_stat {
    int         uft_ino;        /* UFT spoolid */
    mode_t      uft_mode;       /* UFT "xperm" protection */
    int         uft_nlink;      /* UFT copy count */
    uid_t       uft_uid;        /* UFT user ID of owner */
    gid_t       uft_gid;        /* UFT group ID of owner */
    int         uft_size;       /* UFT "data" size, in bytes */
    int         uft_blksize;    /* UFT blocksize (record length) */
    time_t      uft_mtime;      /* UFT time of last mod, as sent */
/*                  mtime=date|xdate     */

    /* ... */

    char        uft_type,       /* UFT type (A, I, so on) */
                name[64],
                from[64],

                uft_class,      /* "spool class" letter */
                uft_hold,

                cc[8],
                devtype[8],
                keep[4],
                msg[4],

                form[16],
                dist[16],
                dest[16],
                title[64];

};



int uft_stat(const char *,struct uft_stat *);

/*

uft send <file> <target>
uft list
uft receive
uft stat <spoolid> <varname>

#
# atime
# ctime
# size
# dev
#
# from
# class
# formcode
# hold|nohold
# distcode
# destcode
# fcbcode
# formcode
#

uft_title == an arbitrary string labelling this file
uft_ver == a version number or revision index
uft_recfmt == F | V
uft_reclen == record length

uft_hold.html ==
uft_host.html ==
uft_name.html ==
uft_note.html ==
uft_noti.html ==
uft_osid.html ==
uft_seq.html ==
uft_ucs.html ==
uft_umsg.html ==
uft_user.html ==

 */


ssize_t getuftentries(int,char*,size_t,off_t*);
/* ssize_t getuftentries(int fd, char *buf, size_t nbytes , off_t *basep); */

int uftopen(const char *,int,mode_t);
/* int uftopen(const char *pathname, int flags, mode_t mode); */

int uft_getline(int,char*);
int uft_putline(int,char*);
int uft_readspan(int,char*,int);

int uftddata(int,int,int);
int uftdnext();
int uftduser(char*);
int uftdmove(int,int);
int uftdimsg(char*,char*,char*,char*);
int uftdlmsg(char*,char*,char*,char*);
int uftdlist(int,char*);

int uftcwack(int,char*,int);
int uftctext(int,char*,int);

char*uftcprot(mode_t);

int abbrev(char*,char*,int);

#define         _UFT_HEADER_
#endif



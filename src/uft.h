/* © Copyright 1995, 2005, 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uft.h (C program header)
 *              Unsolicited File Transfer common header for all C code
 *      Author: Richard Troth, Houston, Texas, USA
 *        Date: 1995-Jan-15, 2005-Jun-05
 */

#ifndef         _UFT_HEADER_

#include <time.h>
#include <sys/types.h>

#if defined(_WIN32) || defined(_WIN64)
 typedef int uid_t;
 typedef int gid_t;
#else
 #define UFT_POSIX
#endif

#include "tcpio.h"

/*        Note: define UFT_ANONYMOUS to use 'uftd' via Tor            *
 * What that does is mute certain announcements from UFTD which       *
 * would render the server identifyable (means to de-anonymize it).   */

/* the version number and copyright */
#define         UFT_PROTOCOL    "UFT/2"
#ifndef         UFT_VERSION
 #define        UFT_VERSION     "POSIXUFT/2.0.12"
#endif
#define         UFT_COPYRIGHT   "© Copyright 1995-2025 Richard M. Troth"
#define         UFT_VRM         "2.0.12"
#define    UFT_VERINT    (((2) << 24) + ((0) << 16) + ((12) << 8) + (0))

#ifndef         UFT_TAG
 #define        UFT_TAG         "UFT"
#endif

/* server constants follow */

/* the SPOOLDIR has a sub-directory for each recipient */
#ifndef         UFT_SPOOLDIR
 #define        UFT_SPOOLDIR    "/var/spool/uft"
             /*                 "C:/ProgramData/uft" */
#endif

#ifndef         UFT_GID
 #define        UFT_GID         0
 /* possibly follow UUCP for this number */
 /* the TCP port for UUCP is 540 (see /etc/services) for one example  */
#endif

#ifndef         UFT_PIPESDIR
 #define        UFT_PIPESDIR    "/usr/libexec/uft"
#endif

/* the SEQuence file name may be platform dependent */
#define         UFT_SEQFILE             ".seq"
#define         UFT_SEQFILE_ALT         "seqno"

/* file name extensions */
#define         UFT_EXT_CONTROL         ".cf" /* control, metadata */
#define         UFT_EXT_DATA            ".df" /* data */
#define         UFT_EXT_EXTRA           ".ef" /* auxdata, resource fork */
#define         UFT_EXT_LIST            ".lf" /* 'ls -l' format */
#define         UFT_EXT_WORK            ".wf"

/* client constants follow */

/* flag bits */
#define         UFT_BINARY      0x8000
#define         UFT_VERBOSE     0x4000

/* registered port for this service */
#define         UFT_PORT        608
#define         IDENT_PORT      113

#define         UFT_BUFSIZ      64512

#define         UFT_SYSLOG_FACILITY     LOG_UUCP

/* the following struct is best used for active UFT files             */
typedef struct  UFTFILE {
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

                char    from[64],       /* same in UFTSTAT struct     */
                        name[64],       /* same in UFTSTAT struct     */
                        type[8],                /* should be one byte */
                        cc[8],                  /* should be one byte */
                        hold[8],                /* should be one byte */
                        class[8],               /* should be one byte */
                        devtype[8], keep[4], msg[4],   /* uft_rudev, uft_keep, uft_msg */
                        form[16], dist[16], dest[16];         /* same */
                int     size, copies;   /* uft_size, uft_nlink        */
                char    title[64];      /* same in UFTSTAT struct     */
                        } UFTFILE ;             /* see also: UFTSTAT  */

#define         UFT_B64_CODE    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

/*    RDR FILE >this< SENT FROM $from RDR WAS yadda yadda */
#define         UFT_TYPE_A_EXPANSION    "TXT" /* "ASCII" */ 
#define         UFT_TYPE_B_EXPANSION    "BIN"
#define         UFT_TYPE_C_EXPANSION    "PRT"
#define         UFT_TYPE_I_EXPANSION    "BIN" /* "IMAGE" */ 
#define         UFT_TYPE_M_EXPANSION    "MAIL"
#define         UFT_TYPE_N_EXPANSION    "NETDATA"
#define         UFT_TYPE_P_EXPANSION    "PRT"
#define         UFT_TYPE_T_EXPANSION    "TXT"
#define         UFT_TYPE_U_EXPANSION    "BIN" /* "UNSPEC" */ 
#define         UFT_TYPE_V_EXPANSION    "VAR" /* or "V16" */
/*
      RDR FILE $FILE SENT FROM $RSCS RDR WAS #### ####
      xxx FILE nnnn  SENT FROM u@h
  93      File &1 received from &2 sent as &3
 */

/* constants in support of IBM NETDATA encoding */
#define   UFT_ND_FIRST    0x80
#define   UFT_ND_LAST     0x40
#define   UFT_ND_CTRL     0x20
#define   UFT_ND_NEXT     0x10
#define   UFT_ND_INMR01   "\xc9\xd5\xd4\xd9\xf0\xf1"   /* first record of transmission */
#define   UFT_ND_INMR02   "\xc9\xd5\xd4\xd9\xf0\xf2"
#define   UFT_ND_INMR03   "\xc9\xd5\xd4\xd9\xf0\xf3"
#define   UFT_ND_INMR04   "\xc9\xd5\xd4\xd9\xf0\xf4"
#define   UFT_ND_INMR06   "\xc9\xd5\xd4\xd9\xf0\xf6"   /* last record in transmission */
#define   UFT_ND_INMR07   "\xc9\xd5\xd4\xd9\xf0\xf7"

/* This is a struct for NETDATA processing.                           *
 * The idea is to isolate "records" in Netdata speak from the stream  *
 * and then reconstruct longer records (of the original file).        *
 * See uftcndd.c source for additional comments.                      */
typedef struct  UFTNDIO {
            void *buffer;
            int   bufmax;
            int   buflen;
            int   bufdex;
                        } UFTNDIO ;

int uft_getndr(int,struct UFTNDIO*,int*,char**,int*);

/*

UFT_ND_FIRST|UFT_ND_LAST is a single record
UFT_ND_FIRST|UFT_ND_LAST|UFT_ND_CTRL is a single control record

0       1       Length of segment including two-byte header
                (length is in the range of 2 to 255)

1       1       Segment descriptor flags:
                    X'80' - First segment of original record.
                    X'40' - Last segment of original record.
                    X'20' - This is (part of) a control record.
                    X'10' - This is record number of next record.

2       n-2     Data (n is in the range of 0 to 253).
                Control records have a control record identifier
                (for example, INMR01) in bytes 2-7. Text units generally
                begin in byte 8. Data records begin directly in byte 2.

https://www.ibm.com/docs/en/zvm/7.2?topic=reference-netdata-format

66E0C9D5D4D9F0F1 always the first record of a transmission
 " " I N M R 0 1
                  if (memcmp("\311\325\324\331\360\361",line+4,6)==0)
                    is_netdata = 1;      ** INMR01 at start of record **

75E0C9D5D4D9F0F2
 " " I N M R 0 2

30E0C9D5D4D9F0F3
 " " I N M R 0 3

08E0C9D5D4D9F0F6
 " " I N M R 0 6

time
    year (4)
    month (2)
    day (2)
    hour (2)
    minute (2)
    second (2)
    fraction of seconds (n).

 */

static char *uft_copyright = UFT_COPYRIGHT;

/* © Copyright 1996, Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: msghndlr.h
 *              header file for  msgd.c  and  msgcat.c
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1994-Jul-26, 1996-Mar-24
 *
 *        Note: this is NOT for the message FORMATTER
 */

/* flags */
#define         MSG_IDENT               0x0001
#define         MSG_VERBOSE             0x0002

#define         MSG_MSP_HOST            "localhost"
#define         MSG_MSP_PORT            18

#define         MSG_UFT_HOST            "localhost"
#define         MSG_UFT_PORT            608

/* The following struct is best used to describe a UFT "spool file".  *
 * It is populated by uft_stat() and referenced by routines which     *
 * routines which need to know attributes of a "spooled" UFT file.    *
 * Some of the members are named to follow POSIX "stat" struct.       */
typedef struct  UFTSTAT {
    int         uft_ino;        /* UFT spoolid */
    mode_t      uft_mode;       /* UFT "xperm" protection */
    int         uft_nlink;      /* UFT copy count, "copies" */
    uid_t       uft_uid;        /* UFT user ID of owner */
    gid_t       uft_gid;        /* UFT group ID of owner */
    int         uft_size;       /* UFT total "data" size, in bytes    */
    int         uft_blksize;    /* UFT blocksize (record length)      */
    time_t      uft_mtime,      /* UFT time of last mod, as sent      */
                uft_stime;      /* time stamp on spool file           */

    char        uft_type,       /* UFT type (A, I, so on) */
                uft_cc,         /* ASA, machine, or none */
                uft_class,      /* "spool class" letter */
                uft_rudev,      /* record-unit device type, "devtype" */
                uft_hold,       /* a z/VM or other mainframe concept  */
                uft_keep,       /* a z/VM or other mainframe concept  */
                uft_msg,        /* a z/VM or other mainframe concept  */
                uft_recfm;      /* a z/VM or other mainframe concept  */

    char        uft_user[64],   /* who this file is sent *to*         */
                uft_from[64],   /* who this file is received *from*   */
                uft_name[64],   /* name of file (optional in UFT)     */
                uft_form[16],   /* z/VM, mainframe, or print concept  */
                uft_dist[16],   /* z/VM, mainframe, or print concept  */
                uft_dest[16],   /* z/VM, mainframe, or print concept  */
                uft_title[64];  /* z/VM, mainframe, or print concept  */

/*

    CLASS               char uft_class
    COPY | COPIES       int uft_nlink
    DATE | XDATE        time_t uft_mtime
    DEST                char uft_dest[]
    DIST                char uft_dist[]
    FORM                char uft_form[]
    GROUP               int uft_gid
    HOLD                char uft_hold
    NAME                char uft_name[]
    OWNER               int uft_uid
    RECFMT              char uft_recfm
    RECLEN              int uft_blksize
    TITLE               char uft_title[]
    PROT | XPERM        char uft_mode[]
    FCB
    MSG | NOTIFY
    SEQ
    UCS
    VERSION

tqcdhkm--- cpy user     host         size year mm dd time  sid  name
---------- --- -------- -------- -------- ---- -- -- --:-- ---- ----------------
||||||\___ msg (M) nomsg (-)
|||||\____ keep (K) consume (-)
||||\_____ hold (H) nohold (-)
|||\______ devtype (prT, Con, pUn)      uft_rudev
||\_______ class (A, B, C, etc.)
|\________ CC (Asa, Machine, none)
\_________ type (I or A or N)
 */

    char        uft_sidp[64];           /* SID full path              */
    void        *uft_prev, *uft_next;   /* pointers if chaining       */
                        } UFTSTAT ;

/*

        unsigned long  st_dev;          see uft_rudev
        unsigned long  st_ino;          OKAY
        unsigned short st_mode;         OKAY
        unsigned short st_nlink;        OKAY copy count
        unsigned short st_uid;          OKAY
        unsigned short st_gid;          OKAY
        unsigned long  st_rdev;         see uft_rudev
        unsigned long  st_size;         OKAY
        unsigned long  st_blksize;      OKAY
        unsigned long  st_blocks;       N/A
        unsigned long  st_atime;        N/A
        unsigned long  st_atime_nsec;   N/A
        unsigned long  st_mtime;        OKAY
        unsigned long  st_mtime_nsec;   N/A
        unsigned long  st_ctime;        see uft_stime
        unsigned long  st_ctime_nsec;   N/A
 */

ssize_t getuftentries(int,char*,size_t,off_t*);
/* ssize_t getuftentries(int fd,char*buf,size_t nbytes,off_t*basep);  */

int uftopen(const char*,int,mode_t);
/* int uftopen(const char*pathname,int flags,mode_t mode); */

int uft_readspan(int,char*,int);

int uftddata(int,int,int);
int uftdnext();
int uftduser(char*);
int uftdmove(int,int);
int uftdimsg(char*,char*,char*,char*);                  /* deprecated */
int uftdlmsg(char*,char*,char*,char*);                  /* deprecated */
int uftdlist(int,char*);

int uftctext(int,char*,int);

char*uftcprot(mode_t);

int abbrev(char*,char*,int);

/* functions from the library */
int uftx_message(char*,int,int,char*,int,char*[]);
int uftd_message(char*,char*);          /* FKA msglocal(user,text)    */
char*uftx_home(char*);
int msgd_umsg(char*,char*,char*);                 /* user, text, from */
int uftx_getline(int,char*,int);        /* sock or fd, buffer, buflen */
int uftx_putline(int,char*,int);        /* sock or fd, buffer, buflen */

char*uftx_user();
char*uftx_getenv(char*,char*);
char*uftx_basename(char*);
char*uftx_parse1(char*);

int msgc_uft(char*,char*);
int msgc_rdm(char*,char*);
int msgc_msp(char*,char*);

int uftd_fann(char*,char*,char*);
int uftc_wack(int,char*,int);
int uftd_agck(char*);

int uftx_proxy(char*,char*,int*);
int uft_stat(char*,struct UFTSTAT*);
int uft_purge(struct UFTSTAT*);
int uftx_atoi(char*);

void uftdstat(int,char*);

int uftdcpq(char*,char*,int);
int uftdl699(int,char*);

int uftc_open(char*,char*,int*);
int uftc_close(int*);

#define         _UFT_HEADER_
#endif

/*
int msgwrite(char*,char*);                                            *
int msgsmtps(char*,char*);                                            *
int msgsmtpm(char*,char*);                                            *
int msgmail(char*,chat*);                                             *
 */



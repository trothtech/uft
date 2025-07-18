/* Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftdcpq.c (C program source)
 *              a 'cpq' command for UFT to emulate RSCS
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-05-30 (Friday)
 *
 *        Note: The purpose of this routine, and of the "CPQ" verb
 *              in the protocol, is entirely for compatibility with
 *              RSCS on z/VM. The responses provided are harmless
 *              in a context where anonymity is not required.
 */

#include <stdio.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "uft.h"

/* ---------------------------------------------------- UFTD_CPQ_CPLEVEL
 *    provide 'uname' and boot time to emulate CPQ CPLEVEL
z/VM Version 7 Release 3.0, service level 2401 (64-bit)
Generated at 2024-03-22 17:29:19 EDT
IPL at 2025-01-02 13:16:56 EDT
 */
int uftdcpq_cplevel(char*cpqstr,int cpqsl)                 /* CPLEVEL */
  { static char _eyecatcher[] = "uftdcpq_cplevel()";       /* CPLEVEL */
    int rc;
    struct utsname cpquts;

#ifdef          UFT_ANONYMOUS
    snprintf(cpqstr,cpqsl,"CPLEVEL: N/A");                 /* CPLEVEL */
#else

    cpqstr[0] = 0x00;
    rc = uname(&cpquts);
    if (rc < 0) return rc;

    snprintf(cpqstr,cpqsl,"%s %s %s %s %s",
             cpquts.sysname,    /* -s, --kernel-name */
             cpquts.nodename,   /* -n, --nodename */
             cpquts.release,    /* -r, --kernel-release */
             cpquts.version,    /* -v, --kernel-version */
             cpquts.machine);   /* -m, --machine */

    /* if we are on Linux then we have sysinfo */
      /* /proc/uptime */
      /* IPL at 2025-01-02 13:16:56 EDT */
/* uptime 13:36:30  up 195 days 16:01,  3 users,  load average: 0.07, 0.03, 0.00 */

#endif

    return 0;
  }

/* ------------------------------------------------------ UFTD_CPQ_CPUID
 *    e.g.: CPUID = FF0818E885618000
 */
int uftdcpq_cpuid(char*cpqstr,int cpqsl)                     /* CPUID */
  { static char _eyecatcher[] = "uftdcpq_cpuid()";           /* CPUID */
    int fd, i;
    char uuid[256];

#ifdef          UFT_ANONYMOUS
    snprintf(cpqstr,cpqsl,"CPUID: N/A");                     /* CPUID */
#else

    fd = open("/proc/sys/kernel/random/boot_id",O_RDONLY); if (fd < 0)
    fd = open("/proc/sys/kernel/random/uuid",O_RDONLY); if (fd < 0)
    strcpy(uuid,"N/A"); else
      { i = read(fd,uuid,sizeof(uuid)-1); close(fd);
        if (i > 0) if (uuid[i-1] == '\n') i--;
        uuid[i] = 0x00; }

    snprintf(cpqstr,cpqsl,"CPUID = %s",uuid);

#endif

    return 0;
  }

/* ------------------------------------------------------ UFTD_CPQ_FILES
 *    not sure how to derive a 'CPQ FILES' response on Unix/Linux/POSIX
 *    e.g.: FILES: 0016 RDR, 0001 PRT,   NO PUN
 */
int uftdcpq_files(char*cpqstr,int cpqsl)                     /* FILES */
  { static char _eyecatcher[] = "uftdcpq_files()";           /* FILES */
    char uuid[256];

    snprintf(cpqstr,cpqsl,"FILES: N/A");                     /* FILES */

    return 0;
  }

/* --------------------------------------------------- UFTD_CPQ_INDICATE
 *    provide 'uptime' to emulate INDICATE
AVGPROC-027% 0017
MDC READS-000000/SEC WRITES-000000/SEC HIT RATIO-000%
PAGING-3/SEC
Q0-00001 Q1-00007           Q2-00002 EXPAN-002 Q3-00068 EXPAN-002
 */
int uftdcpq_indicate(char*cpqstr,int cpqsl)                /* INDICATE */
  { static char _eyecatcher[] = "uftdcpq_indicate()";      /* INDICATE */
    char uuid[256];

    snprintf(cpqstr,cpqsl,"INDICATE: N/A");                /* INDICATE */

/* uptime 13:36:30  up 195 days 16:01,  3 users,  load average: 0.07, 0.03, 0.00 */

    return 0;
  }

/* ----------------------------------------------------- UFTD_CPQ_LOGMSG
 *    motd by any other name
 */
int uftdcpq_logmsg(char*cpqstr,int cpqsl)                   /* LOGMSG */
  { static char _eyecatcher[] = "uftdcpq_logmsg()";         /* LOGMSG */
    int fd, i;

#ifdef          UFT_ANONYMOUS
    snprintf(cpqstr,cpqsl,"LOGMSG: N/A");                   /* LOGMSG */
#else

    fd = open("/etc/motd",O_RDONLY); if (fd < 0)            /* LOGMSG */
    strncpy(cpqstr,"N/A",cpqsl); else
      { i = read(fd,cpqstr,cpqsl); close(fd);               /* LOGMSG */
        if (i > 0) cpqstr[i] = 0x00; }

#endif

    return 0;
  }

/* ------------------------------------------------------ UFTD_CPQ_NAMES
 *    e.g.: LCLEF02  - DSC , LTROTH1  - DSC , LSTAGE1  - DSC , ...
 */
int uftdcpq_names(char*cpqstr,int cpqsl)                     /* NAMES */
  { static char _eyecatcher[] = "uftdcpq_names()";           /* NAMES */

    snprintf(cpqstr,cpqsl,"NAMES: N/A");                     /* NAMES */

    return 0;
  }

/* ------------------------------------------------------- UFTD_CPQ_TIME
 *    e.g.: TIME IS 16:45:28 EDT FRIDAY 2025-05-30
 */
int uftdcpq_time(char*cpqstr,int cpqsl)                       /* TIME */
  { static char _eyecatcher[] = "uftdcpq_time()";             /* TIME */
    char *tz;
    struct tm *tmstamp;
    time_t tmval;
    char *wday[8] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL };

#ifdef          UFT_ANONYMOUS
    snprintf(cpqstr,cpqsl,"TIME: N/A");                       /* TIME */
#else

    /* get the current time and convert it to a tm struct             */
    tmval = time(NULL);
    tmstamp = gmtime(&tmval);
/*  tmstamp = localtime(&tmval);                                   // */

    /* fix-up the year because this is the 21st century already       */
    if (tmstamp->tm_year < 1900) tmstamp->tm_year += 1900;
    tmstamp->tm_mon = tmstamp->tm_mon + 1;

    /* the tm_zone member is preferred but not all systems have it    */
#if !defined(__sun) && !defined(sun) && !defined(__SUNPRO_C) && !defined(__SUNPRO_CC)
    tz = (char*) tmstamp->tm_zone;        /* <bits/types/struct_tm.h> */
#else
    tz = tzname[0];                         // timezone name <time.h> */
    if (tz == NULL || *tz == 0x00) tz = tzname[1];
#endif
    if (tz == NULL || *tz == 0x00) tz = "-";

    /* render it in the same format as the IBM systems render it      */
    snprintf(cpqstr,cpqsl,
        "TIME IS %02d:%02d:%02d %s %s %04d-%02d-%02d",
            tmstamp->tm_hour,                  /* Hours        [0-23] */
            tmstamp->tm_min,                   /* Minutes      [0-59] */
            tmstamp->tm_sec,                   /* Seconds      [0-60] */
            tz,
            wday[tmstamp->tm_wday],            /* Day of week  [0-6]  */
            tmstamp->tm_year,                  /* Year - 1900         */
            tmstamp->tm_mon,                   /* Month        [0-11] */
            tmstamp->tm_mday);                 /* Day          [1-31] */

#endif

    return 0;
  }

/* ------------------------------------------------------ UFTD_CPQ_USERS
 *    e.g.:    53 USERS,      0 DIALED,      0 NET
 */
int uftdcpq_users(char*cpqstr,int cpqsl)                     /* USERS */
  { static char _eyecatcher[] = "uftdcpq_users()";           /* USERS */
/* uptime 13:36:30  up 195 days 16:01,  3 users,  load average: 0.07, 0.03, 0.00 */
    snprintf(cpqstr,cpqsl,"USERS: N/A");                     /* USERS */
    return 0;
  }

/* ------------------------------------------------------- UFTD_CPQ_USER
 *    e.g.: LTROTH1  - DSC
 */
int uftdcpq_user(char*cpqstr,int cpqsl,char*user)        /* USER uuuu */
  { static char _eyecatcher[] = "uftdcpq_user()";        /* USER uuuu */
    snprintf(cpqstr,cpqsl,"USER: N/A");                  /* USER uuuu */
    return 0;
  }

/* ------------------------------------------------------------ UFTD_CPQ
 *    This routine mimics some of the functions of RSCS "CPQ" command.
 */
int uftdcpq(char*a,char*cpqstr,int cpqsl)
  { static char _eyecatcher[] = "uftdcpq()";
    char *msgv[4], msg2[16], *b;

    strncpy(msg2,a,sizeof(msg2)-1); msg2[sizeof(msg2)-1] = 0x00;
    a = msg2; while /* (*a != 0x00) */ (*a > ' ')
      { if (islower(*a)) *a = toupper(*a);   a++; }
    *a++ = 0x00; b = a; a = msg2;

    if (abbrev("CPLEVEL",a,3))  return uftdcpq_cplevel(cpqstr,cpqsl);
    if (abbrev("CPUID",a,3))    return uftdcpq_cpuid(cpqstr,cpqsl);
    if (abbrev("FILES",a,1))    return uftdcpq_files(cpqstr,cpqsl);
    if (abbrev("INDICATE",a,3)) return uftdcpq_indicate(cpqstr,cpqsl);
    if (abbrev("LOGMSG",a,3))   return uftdcpq_logmsg(cpqstr,cpqsl);
    if (abbrev("NAMES",a,1))    return uftdcpq_names(cpqstr,cpqsl);
    if (abbrev("TIME",a,1))     return uftdcpq_time(cpqstr,cpqsl);
    if (abbrev("USERS",a,5) && *b == 0x00) return uftdcpq_users(cpqstr,cpqsl);
    if (abbrev("USER",a,1) && *b != 0x00) return uftdcpq_user(cpqstr,cpqsl,b);

    /* CPQ USER requires an additional operand                        */
    if (abbrev("USER",a,1)) {
    uftx_message(cpqstr,cpqsl,416,"CPQ",0,NULL);
    return 4; }

    /* otherwise return 433 "invalid option"                          */
    msgv[1] = msg2;
    uftx_message(cpqstr,cpqsl,433,"CPQ",2,msgv);

    return 4;
  }

#ifdef BYPASS

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  {
    int rc;
    char cpqstr[256], *p;

/*  rc = uftdcpq("CPLEVEL",cpqstr,sizeof(cpqstr)-1);               // */
/*  rc = uftdcpq("CPUID",cpqstr,sizeof(cpqstr)-1);                 // */

/* #uftdcpq("FILES",cpqstr,sizeof(cpqstr)-1);                      // */
/* #rc = uftdl699(1,cpqstr);                                       // */

/* #uftdcpq("INDICATE",cpqstr,sizeof(cpqstr)-1);                   // */
/* #rc = uftdl699(1,cpqstr);                                       // */

    rc = uftdcpq("LOGMSG",cpqstr,sizeof(cpqstr)-1);
    switch (rc) { case 0:
                      uftdl699(1,cpqstr);
                      uftdstat(1,"200 ACK");
                      break;
                  case 2: case 3: case 4: case 5:
                      p = cpqstr; while (*p > ' ') p++;
                                  while (*p == ' ') p++;
                      uftdstat(1,p);
                      break;
                  default:
                      uftdstat(1,"500 internal error");
                      break; }
return 0;

/*  rc = uftdcpq("NAMES",cpqstr,sizeof(cpqstr)-1);                 // */
/*  rc = uftdl699(1,cpqstr);                                       // */

/*  rc = uftdcpq("TIME",cpqstr,sizeof(cpqstr)-1);                  // */
/*  rc = uftdl699(1,cpqstr);                                       // */

/* #uftdcpq("USERS",cpqstr,sizeof(cpqstr)-1);                      // */
/* #rc = uftdl699(1,cpqstr);                                       // */

/* #uftdcpq("USER",cpqstr,sizeof(cpqstr)-1);                       // */
/* #rc = uftdl699(1,cpqstr);                                       // */

    rc = uftdcpq("blaH",cpqstr,sizeof(cpqstr)-1);
    switch (rc) { case 0:
                      uftdl699(1,cpqstr);
                      uftdstat(1,"200 ACK");
                      break;
                  case 2: case 3: case 4: case 5:
                      p = cpqstr; while (*p > ' ') p++;
                                  while (*p == ' ') p++;
                      uftdstat(1,p);
                      break;
                  default:
                      uftdstat(1,"500 internal error");
                      break; }

    return 0;
  }

#endif



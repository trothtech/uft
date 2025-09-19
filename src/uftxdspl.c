/* Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftxdspl.c (C program source)
 *              Unsolicited File Transfer "de-spooling" utility
 *              follows the behavior of the UFTXDSPL REXX gem from CMS
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-09-10 (Wed)
 *
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#include "uft.h"

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftxdspl.c main()";
    int rc, fd, i, hold, keep;
    char *arg0, *arg1, sidn[64], buff[65536], *ptitle, *mv[16];
    struct UFTSTAT us;
    struct tm *gmtstamp;   /* not thread safe but we are not threaded */

    ptitle = "Internet SENDFILE de-spooler";         /* program title */

    /* note command name and set defaults */
    mv[0] = arg0 = uftx_basename(argv[0]);
    hold = keep = 0;         /* "treat as held" and "keep (no purge)" */

    /* process command-line options */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      { switch (argv[i][1])
          { case '?':   argc = i;       /* help                       */
            case 'v':   case 'V':       /* verbose                    */
                        uftcflag |= UFT_VERBOSE;
                        break;
            case 'k':   case 'K':       /* keep the file (no purge)   */
                        keep = 1;
                        break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (abbrev("--version",argv[i],6) > 0)
                  { sprintf(buff,"%s: %s %s",arg0,UFT_VERSION,ptitle);
                    uftx_putline(2,buff,0);
                    return 0; } else           /* exit from help okay */
                if (abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else
                if (abbrev("--keep",argv[i],6) > 0) keep = 1; else
                if (abbrev("--hold",argv[i],6) > 0) hold = 1; else
                  { mv[1] = argv[i];
                rc = uftx_message(buff,sizeof(buff)-1,3,"DSP",2,mv);
                if (rc >= 0) fprintf(stderr,"%s\n",buff); else
                fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default:    mv[1] = argv[i];
                rc = uftx_message(buff,sizeof(buff)-1,3,"DSP",2,mv);
                if (rc >= 0) fprintf(stderr,"%s\n",buff); else
                fprintf(stderr,"%s: invalid option %s",arg0,argv[i]);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

    /* announcement (iff verbose option requested) */
    if (uftcflag & UFT_VERBOSE)
      { sprintf(buff,"%s: %s %s",arg0,UFT_VERSION,ptitle);
        uftx_putline(2,buff,0); buff[0] = 0x00; }

    /* be sure we still have enough args */
    if ((argc - i) < 1)
      { /* system("xmitmsg -2 386"); */
        sprintf(buff,"Usage: %s <spid>",arg0);
        uftx_putline(2,buff,0); buff[0] = 0x00;
        if (uftcflag & UFT_VERBOSE) return 0;  /* exit from help okay */
                              else  return 1; }       /* missing args */
    arg1 = argv[i];

    /* stat the spool file ------------------------------------------ */
    rc = uft_stat(arg1,&us);
    if (rc < 0)
      { mv[1] = uftx_user(); mv[2] = arg1;
        rc = uftx_message(buff,sizeof(buff)-1,42,"DSP",3,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",buff);
        return 1; }                      /* exit the program non-zero */

    /* open the "data file" early in case there is a problem -------- */
    snprintf(sidn,sizeof(sidn)-1,"%s.df",us.uft_sidp);
    rc = fd = open(sidn,O_RDONLY);
    if (rc < 0) {
fprintf(stderr,"%s: open('%s.df',) returned %d\n",arg0,us.uft_sidp,rc);
        mv[1] = uftx_user(); mv[2] = arg1;
        rc = uftx_message(buff,sizeof(buff)-1,42,"DSP",3,mv);
        if (rc >= 0) fprintf(stderr,"%s\n",buff);
        return 1; }                      /* exit the program non-zero */

    /* report all header and metadata ------------------------------- */

    /* start the transaction */
    snprintf(buff,sizeof(buff),"FILE %d %s -",us.uft_size,us.uft_from);
    uftx_putline(1,buff,0);      /* indicates begining of transaction */

    /* tell the server who it's for */
    snprintf(buff,sizeof(buff),"USER %s",us.uft_user);   /* mandatory */
    uftx_putline(1,buff,0);     /* usually the owner but maybe unqual */

    /* signal the type for canonization */
    sprintf(buff,"TYPE %c",us.uft_type);     /* a mandatory attribute */
    uftx_putline(1,buff,0);       /* uft_stat() will have vetted this */

    /* does this file have a name? */
    if (us.uft_name[0] != 0x00)
      { snprintf(buff,sizeof(buff),"NAME %s",us.uft_name);
        uftx_putline(1,buff,0); }              /* optional but common */

    /* do we have a time stamp for this file? */
    if (us.uft_mtime != 0)       /* alternative stamp is us.uft_stime */
/*    { gmtstamp = localtime(&us.uft_mtime);                          */
      { gmtstamp = gmtime(&us.uft_mtime);          /* not thread safe */
        if (gmtstamp->tm_year < 1900)    /* (but we are not threaded) */
            gmtstamp->tm_year += 1900;
        gmtstamp->tm_mon = gmtstamp->tm_mon + 1;
/*                              %Y-%m-%d, the ISO 8601 date format    */
        sprintf(buff,"META DATE %04d-%02d-%02d %02d:%02d:%02d %s",
                gmtstamp->tm_year, gmtstamp->tm_mon,
                gmtstamp->tm_mday, gmtstamp->tm_hour,
                gmtstamp->tm_min, gmtstamp->tm_sec, tzname[0]);
        uftx_putline(1,buff,0);
        /* also send it as number-of-seconds Unix epoch offset value  */
        sprintf(buff,"META XDATE %ld",us.uft_mtime);
        uftx_putline(1,buff,0); }            /* "seconds since epoch" */

    /* do we have a protection bit pattern on this file?              */
    if (us.uft_mode != 0)
      { sprintf(buff,"META PROT %s",uftcprot(us.uft_mode));
        uftx_putline(1,buff,0);                         /* VMS format */
        /* also send it as bits in octal format                       */
        sprintf(buff,"META XPERM 0%lo",us.uft_mode);
        uftx_putline(1,buff,0); }                /* octal Unix format */

    /* does this file have a specific class?                          */
    if (us.uft_class != 0x00 && us.uft_class != '-' && us.uft_class != ' ')
      { switch (us.uft_rudev)    /* optional "record unit" device */
          { case 'R': case 'T':                      /* PRT <== R */
                sprintf(buff,"CLASS %c PRT",us.uft_class);      break;
            case 'U': case 'N':                      /* PUN <== U */
                sprintf(buff,"CLASS %c PUN",us.uft_class);      break;
            default:
                sprintf(buff,"CLASS %c",us.uft_class);          break; }
        uftx_putline(1,buff,0); }

    /* is there a copy count?                       (for print files) */
    if (us.uft_nlink > 1)
      { sprintf(buff,"COPY %d",us.uft_nlink);
        uftx_putline(1,buff,0); }                  /* for print files */

    /* does this file have a forms code?            (for print files) */
    if (us.uft_form[0] != 0x00)
      { snprintf(buff,sizeof(buff),"FORM %s",us.uft_form);
        uftx_putline(1,buff,0); }                  /* for print files */

    /* does this file have a distribution code?     (for print files) */
    if (us.uft_dist[0] != 0x00)
      { snprintf(buff,sizeof(buff),"DIST %s",us.uft_dist);
        uftx_putline(1,buff,0); }                  /* for print files */

    /* does this file have a hold on it? */
    if (us.uft_hold != 0x00 && us.uft_hold != '-' && us.uft_hold != ' ')
      { sprintf(buff,"HOLD %c",us.uft_hold); uftx_putline(1,buff,0);
        if (us.uft_hold != 'N' && us.uft_hold != 'n') hold = 1; }

    /* is this file marked for keeping? */
    if (us.uft_keep != 0x00 && us.uft_keep != '-' && us.uft_keep != ' ')
      { sprintf(buff,"KEEP %c",us.uft_keep); uftx_putline(1,buff,0);
        if (us.uft_keep != 'N' && us.uft_keep != 'N') keep = 1; }

    /* does this file have a destination code?      (for print files) */
    if (us.uft_dest[0] != 0x00)
      { snprintf(buff,sizeof(buff),"DEST %s",us.uft_dest);
        uftx_putline(1,buff,0); }                  /* for print files */

    /* does this printed file have a title?         (for print files) */
    if (us.uft_title[0] != 0x00)
      { snprintf(buff,sizeof(buff),"TITLE %s",us.uft_title);
        uftx_putline(1,buff,0); }                  /* for print files */

#ifdef _DEVELOPMENT
    uid_t    us.uft_uid;        /* UFT user ID of OWNER */
    gid_t    us.uft_gid;        /* UFT group ID of GROUP */
    int      us.uft_blksize;    /* UFT blocksize (record length)      */
    char     us.uft_cc;         /* ASA, machine, or none */
    char     us.uft_msg;        /* a z/VM or other mainframe concept  */
#endif

    /* now send the file down the pipe ------------------------------ */
    uftx_putline(1,"DATA",0);

    /* process the data (everything following the "DATA" statement)   */

    /* switch TYPE=A versus TYPE=I ---------------------------------- */
    switch (us.uft_type)
      {
        case 'A': case 'a': uftcflag &= ~UFT_BINARY;
            if (uftcflag & UFT_VERBOSE) fprintf(stderr,"UFTXDSPL: plain text\n");
            break;
        case 'N': case 'n':
        case 'I': case 'i': uftcflag |= UFT_BINARY;
            if (uftcflag & UFT_VERBOSE) fprintf(stderr,"UFTXDSPL: binary\n");
            break;
        default:
            fprintf(stderr,"UFTXDSPL: bogus canonization '%c'\n",us.uft_type);
            break;
      }

    /* process the data (everything following the "DATA" statement)   */
    if (uftcflag & UFT_BINARY) while (1)
      { rc = i = uft_readspan(fd,buff,sizeof(buff)); if (rc == 0)
        rc = i = uft_readspan(fd,buff,sizeof(buff)); if (rc < 1) break;
        rc = write(1,buff,i); }
    else  while (1)
      { rc = i = uftctext(fd,buff,sizeof(buff)); if (rc == 0)
        rc = i = uftctext(fd,buff,sizeof(buff)); if (rc < 1) break;
        rc = uftx_putline(1,buff,0); }

    /* if the spool file is not held nor kept then delete it */
    if (hold || keep) { if (uftcflag & UFT_VERBOSE)
fprintf(stderr,"UFTXDSPL: keeping the spool file and not removing it\n");
                      } else rc = uft_purge(&us);

    /* get outta here */
    return 0;
  }



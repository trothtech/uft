/* Copyright 2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: uftxdspl.c (C program source)
 *              Unsolicited File Transfer "de-spooling" utility
 *              follows the behavior of UFTXDSPL REXX from CMS
 *      Author: Rick Troth, Cedarville, Ohio, USA
 *        Date: 2025-09-10 (Wed)
 *
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#include "uft.h"

extern int uftcflag;

/* ------------------------------------------------------------------ */
int main(int argc,char*argv[])
  { static char _eyecatcher[] = "uftc.c     main()";
    int rc, fd, i;
    char *arg0, *arg1, sidn[64], buff[65536];
    struct UFTSTAT us;
    struct tm *gmtstamp;     /* not thread safe here but not threaded */

    /* note command name and set defaults */
    arg0 = uftx_basename(argv[0]);

    /* process command-line options */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      { switch (argv[i][1])
          { case '?':   argc = i;       /* help                       */
            case 'v':   case 'V':       /* verbose                    */
                        uftcflag |= UFT_VERBOSE;
                        break;
//          case '#':   /* COPY -or- COPIES                           */
//                      i++;
//                      copy = atoi(argv[i]);
//                      break;

/* ------------------------------------------------------------------ */
            case '-':                          /* long format options */
                if (abbrev("--version",argv[i],6) > 0)
                  { sprintf(buff,"%s: %s Internet SENDFILE de-spooler",
                                arg0,UFT_VERSION);
                    uftx_putline(1,buff,0);
                    return 0; } else           /* exit from help okay */
//              if (abbrev("--ascii",argv[i],5) > 0 ||
//                  abbrev("--text",argv[i],6) > 0)
//                { uftcflag &= ~UFT_BINARY; type = "A"; } else
                if (abbrev("--verbose",argv[i],6) > 0)
                  { uftcflag |= UFT_VERBOSE; } else
//              if (abbrev("--class",argv[i],4) > 0)
//                { i++; class = argv[i]; } else
                  { snprintf(buff,sizeof(buff),"%s: invalid option %s",
                                arg0,argv[i]);
                    uftx_putline(1,buff,0);
                    return 1; }             /* exit on invalid option */
                    break;
/* ------------------------------------------------------------------ */

            default:    snprintf(buff,sizeof(buff),"%s: invalid option %s",
                                arg0,argv[i]);
                        uftx_putline(1,buff,0);
                        return 1;           /* exit on invalid option */
                        break;
          }
      }

    /* announcement (iff verbose option requested) */
    if (uftcflag & UFT_VERBOSE)
      { sprintf(buff,"%s: %s Internet SENDFILE de-spooler",
                arg0,UFT_VERSION);
        uftx_putline(1,buff,0); buff[0] = 0x00; }

    /* be sure we still have enough args */
    if ((argc - i) < 1)
      { /* system("xmitmsg -2 386"); */
        sprintf(buff,"Usage: %s <spid>",arg0);
        uftx_putline(1,buff,0); buff[0] = 0x00;
        if (uftcflag & UFT_VERBOSE) return 0;  /* exit from help okay */
                              else  return 1; }       /* missing args */
    arg1 = argv[i];

    /* stat the spool file ------------------------------------------ */
    rc = uft_stat(arg1,&us);
    if (rc < 0) return 1;

    /* report all header and metadata ------------------------------- */

    /* start the transaction */
    snprintf(buff,sizeof(buff),"FILE %d %s -",us.uft_size,us.uft_from);
    uftx_putline(1,buff,0);

    /* tell the server who it's for */
    snprintf(buff,sizeof(buff),"USER %s",us.uft_user);
    uftx_putline(1,buff,0);     /* usually the owner but maybe unqual */

    /* signal the type for canonization */
    sprintf(buff,"TYPE %c",us.uft_type);     /* a mandatory attribute */
    uftx_putline(1,buff,0);       /* uft_stat() will have vetted this */

    /* does this file have a name? */
    if (us.uft_name != 0x0000) if (us.uft_name[0] != 0x00)
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
        uftx_putline(1,buff,0); }

    /* do we have a protection bit pattern on this file?              */
    if (us.uft_mode != 0)
      { sprintf(buff,"META PROT %s",uftcprot(us.uft_mode));
        uftx_putline(1,buff,0);
        /* also send it as bits in octal format                       */
        sprintf(buff,"META XPERM %lo",us.uft_mode);
        uftx_putline(1,buff,0); }

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

    /* is there a copy count?                                         */
    if (us.uft_nlink > 1)
      { sprintf(buff,"COPY %d",us.uft_nlink);
        uftx_putline(1,buff,0); }

#ifdef _DEVELOPMENT
typedef struct  UFTSTAT {
    uid_t    us.uft_uid;        /* UFT user ID of owner */
    gid_t    us.uft_gid;        /* UFT group ID of owner */
    int      us.uft_blksize;    /* UFT blocksize (record length)      */
    char     us.uft_cc,         /* ASA, machine, or none */
             us.uft_hold,       /* a z/VM or other mainframe concept  */
             us.uft_keep,       /* a z/VM or other mainframe concept  */
             us.uft_msg,        /* a z/VM or other mainframe concept  */

//           us.user[16],       /* from[] - parsed, NOT who it's to/for */
//           us.host[16],       /* from[] - parsed */
             us.form[16],       /* z/VM, mainframe, or print concept  */
             us.dist[16],       /* z/VM, mainframe, or print concept  */
             us.dest[16],       /* z/VM, mainframe, or print concept  */
             us.title[64];      /* z/VM, mainframe, or print concept  */
                        } UFTSTAT ;
#endif

    /* now send the file down the pipe ------------------------------ */
    uftx_putline(1,"DATA",0);

    /* switch TYPE=A versus TYPE=I ---------------------------------- */
    snprintf(sidn,sizeof(sidn)-1,"%s.df",us.uft_sidp);
    rc = fd = open(sidn,O_RDONLY);
// FIXME: check the return code
//fprintf(stderr,"UFTXDSPL: open('.df',) returned %d\n",rc);

    /* switch TYPE=A versus TYPE=I ---------------------------------- */
    switch (us.uft_type)
      {
        case 'A':
//fprintf(stderr,"UFTXDSPL: plain text\n");
while (1) { rc = uftctext(fd,buff,sizeof(buff)); if (rc == 0)
            rc = uftctext(fd,buff,sizeof(buff)); if (rc < 1) break;
            uftx_putline(1,buff,0); }
            break;
        case 'I':
//fprintf(stderr,"UFTXDSPL: binary\n");
while (1) { rc = uft_readspan(fd,buff,sizeof(buff)); if (rc == 0)
            rc = uft_readspan(fd,buff,sizeof(buff)); if (rc < 1) break;
 }
            break;
        default:
//fprintf(stderr,"UFTXDSPL: bogus canonization '%c'\n",us.uft_type);
            break;
      }

    /* if the spool file is not held then delete it */    
    if (us.uft_hold == 0x00 || us.uft_hold == '-'
                            || us.uft_hold == ' ')
//fprintf(stderr,"UFTXDSPL: we would normally remove the spool file\n");

    /* get outta here */
    return 0;
  }



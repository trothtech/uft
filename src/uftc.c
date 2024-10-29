/* © Copyright 1995, Richard M. Troth, all rights reserved.  <plaintext>
 *
 *        Name: uftc.c, sendfile.c
 *              Unsolicited File Transfer client
 *              *finally* an Internet SENDFILE for UNIX
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1994-Jun-30, 1995-Jan-22
 *
 */

#include	<string.h>
#include	<stdio.h>
#include        <fcntl.h>
#include        <sys/stat.h>
#include	<stdlib.h>
#include	<time.h>
#include        <errno.h>

#include        "uft.h"

extern  int     errno;
char           *arg0;
int             uftv;           /*  protocol level  (1 or 2)  */
int             uftcflag;

/* ------------------------------------------------------------------ */
int main(argc,argv)
  int     argc;
  char   *argv[];
  {
    int         i, fd0, s, size, r, copy;
    char        temp[256], targ[256], b[BUFSIZ],
               *from, *host, *name, *type, *auth, *class;
    extern  char   *getenv(), *userid();
    struct  stat    uftcstat;
    time_t      mtime;
    mode_t	prot;
    struct tm *gmtstamp;

    /*  note command name and set defaults  */
    arg0 = argv[0];
    uftcflag = UFT_BINARY;      /*  default  */
    name = type = class = "";
    auth = "-";         /*  no particular authentication scheme  */
    copy = 0;

    /*  process options  */
    for (i = 1; i < argc && argv[i][0] == '-' &&
                            argv[i][1] != 0x00; i++)
      {
        switch (argv[i][1])
          {
            case 'a':   case 'A':	/*  ASCII (ie: plain text)  */
			uftcflag &= ~UFT_BINARY;
                        type = "A";
                        break;

            case 'b':	case 'B':	/*  BINARY  */
            case 'i':	case 'I':	/*  aka IMAGE  */
			uftcflag |= UFT_BINARY;
                        type = "I";
                        break;

#ifdef  OECS
            case 'e':	case 'E':	/*  EBCDIC  (IBM plain text)  */
			uftcflag |= UFT_BINARY;
                        type = "E";
                        break;
#endif

            case 't':   case 'T':	/*  TYPE  */
			i++;   
                        type = argv[i];
                        break;

            case 'n':	case 'N':	/*  NAME  */
			i++;
                        name = argv[i];
                        break;

            case '?':
			argc = i;
	    case 'v':	case 'V':
			uftcflag |= UFT_VERBOSE;
                        break;

            case 'c':	case 'C':	/*  CLASS  */
			i++;   
                        class = argv[i];
                        break;

            case 'm':	case 'M':	/*  TYPE=M  */
			uftcflag &= ~UFT_BINARY;
                        type = "M";
                        break;

	    case '#':	/*  COPY -or- COPIES  */
			i++;	
			copy = atoi(argv[i]);
                        break;

            default:    (void) sprintf(temp,
                                "%s: invalid option %s",
                                arg0,argv[i]);
                        (void) uft_putline(2,temp);
                        return 20;
                        break;
          }
      }

    /*  announcement  (iff verbose option requested)  */
    if (uftcflag & UFT_VERBOSE)
      {
	(void) sprintf(temp,"%s: %s Internet SENDFILE client",
		arg0,UFT_VERSION);
	(void) uft_putline(2,temp);
      }

    /*  be sure we still have enough args (min 2) left over  */
    if ((argc - i) < 2)
      {
        (void) sprintf(temp,
		"Usage: %s [ -a | -i ] <file> [to] <someone>",arg0);
	(void) uft_putline(2,temp);
	(void) sprintf(temp,
		"          [ -n name ] [ -c class ]");
	(void) uft_putline(2,temp);
	if (uftcflag & UFT_VERBOSE) return 0;
			      else  return 24;
      }

    /*  flag some known canonicalization types  */
    switch (type[0])
      {
        case 'a':   case 'A':
        case 'm':   case 'M':
        case 't':   case 'T':   uftcflag &= ~UFT_BINARY;
                                break;
        case 'b':   case 'B':
        case 'i':   case 'I':
        case 'n':   case 'N':
        case 'u':   case 'U':   uftcflag |= UFT_BINARY;
                                break;
      }

    /*  open the input file,  if not stdin  */
    if (argv[i][0] == '-' && argv[i][1] == 0x00)
      {
        fd0 = dup(0);
      }
    else
      {
        if (name[0] == 0x00) name = argv[i];
        fd0 = open(argv[i],O_RDONLY);
      }

    /*  verify that the open() worked  */
    if (fd0 < 0)
      {
        if (*name != 0x00) (void) perror(name);
                else    (void) perror("stdin");
        return fd0;
      }

    /*  do we have any ideas about this file?  */
    if (fstat(fd0,&uftcstat) == 0)
      {
        size = uftcstat.st_size;
        mtime = uftcstat.st_mtime;
	prot = uftcstat.st_mode;
      }
    else
      {
        size = 0;  mtime = 0;  prot = 0;
      }

    /*  see if we're running IDENT locally  */
    (void) sprintf(temp,"%s:%d","localhost",IDENT_PORT);
    s = tcpopen(temp,0,0);
    if (s >= 0)
      {
        auth = "IDENT";
        (void) close(s);
      }

    /*  open a socket to the server  */
    (void) strcpy(targ,argv[argc-1]);
    host = targ;
    while (*host != 0x00 && *host != '@') host++;
    if (*host == '@') *host++ = 0x00; else host = "localhost";

    (void) sprintf(temp,"%s:%d",host,UFT_PORT);
    s = tcpopen(temp,0,0);
    if (s < 0)
      {
        (void) perror(host);
        return s;
      }
    r = s;

    /* wait for the herald from the server */
    i = tcpgets(r,temp,sizeof(temp));
    if (i < 0)
      {
        (void) perror(host);
        return i;
      }
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

    /*  figure out what protocol version the server likes  */
    uftv = temp[0] & 0x0F;
    if (uftcflag & UFT_VERBOSE)
      {
        (void) sprintf(temp,"%s: UFT protocol %d",arg0,uftv);
        (void) uft_putline(2,temp);
      }
    /*  (above is only good for UFT1 or UFT2)  */
    if (uftv < 1) uftv = 1;

    /*  identify this client to the server  */
    (void) sprintf(temp,"#%s client %s",UFT_PROTOCOL,UFT_VERSION);
    (void) tcpputs(s,temp);
    /*  NO ACK FOR COMMENTS SO DON'T WAIT FOR ONE HERE  */
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

    /* start the transaction */
    from = userid();
    (void) sprintf(temp,"FILE %d %s %s",size,from,auth);
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
    (void) tcpputs(s,temp);
    i = uftcwack(r,temp,sizeof(temp));
    if (i < 0)
      {
        if (errno != 0) (void) perror(arg0);
        else (void) uft_putline(2,temp);
        return i;
      }
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

    /*  tell the server who it's for  */
    (void) sprintf(temp,"USER %s",targ);
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
    (void) tcpputs(s,temp);
    i = uftcwack(r,temp,sizeof(temp));
    if (i < 0)
      {
        if (errno != 0) (void) perror(arg0);
        else (void) uft_putline(2,temp);
        return i;
      }
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

    /*  signal the type for canonicalization  */
    if (type == 0x0000 || type[0] == 0x00)
      {
        if (uftcflag & UFT_BINARY) type = "I";
                /*  "applcation/octet-stream"  */
        else type = "A";    /*  "text/plain"  */
      }
    (void) sprintf(temp,"TYPE %s",type);
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
    (void) tcpputs(s,temp);
    i = uftcwack(r,temp,sizeof(temp));
    if (i < 0)
      {
        if (errno != 0) (void) perror(arg0);
        else (void) uft_putline(2,temp);
        return i;
      }
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

    /* does this file have a name? */
    if (name != 0x0000 && name[0] != 0x00)
      {
        (void) sprintf(temp,"NAME %s",name);
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
        i = tcpputs(s,temp);
        i = uftcwack(r,temp,sizeof(temp));
        if (i < 0)
          {
            if (errno != 0) (void) perror(arg0);
            else (void) uft_putline(2,temp);
            return i;
          }
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
      }

    /*  do we have a time stamp on it?  */
    if (mtime != 0)
      {
	gmtstamp = localtime(&mtime);
	if (gmtstamp->tm_year < 1900)
	    gmtstamp->tm_year += 1900;
	sprintf(temp,"DATE %04d-%02d-%02d %02d:%02d:%02d %s",
		gmtstamp->tm_year, gmtstamp->tm_mon,
		gmtstamp->tm_mday, gmtstamp->tm_hour,
		gmtstamp->tm_min, gmtstamp->tm_sec, "GMT");
	if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
	i = tcpputs(s,temp);
	i = uftcwack(r,temp,sizeof(temp));
	if (i < 0 && temp[0] != '4')
	  {
	    if (errno != 0) (void) perror(arg0);
	    else (void) uft_putline(2,temp);
	    return i;
	  }
	if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

        (void) sprintf(temp,"XDATE %ld",mtime);
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
        i = tcpputs(s,temp);
        i = uftcwack(r,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          {
            if (errno != 0) (void) perror(arg0);
            else (void) uft_putline(2,temp);
            return i;
          }
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
      }

    /*  do we have a time stamp on it?  */
    if (prot != 0)
      {
        (void) sprintf(temp,"PROT %s",uftcprot(prot));
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
        i = tcpputs(s,temp);
        i = uftcwack(r,temp,sizeof(temp));
        if (i < 0 && temp[0] != '4')
          {
            if (errno != 0) (void) perror(arg0);
            else (void) uft_putline(2,temp);
            return i;
          }
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
      }

    /* does this file have a specific class? */
    if (class != 0x0000 && class[0] != 0x00)
      {
        (void) sprintf(temp,"CLASS %s",class);
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
        i = tcpputs(s,temp);
        i = uftcwack(r,temp,sizeof(temp));
        if (i < 0)
          {
            if (errno != 0) (void) perror(arg0);
            else (void) uft_putline(2,temp);
            return i;
          }
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
      }

    /*  now send the file down the pipe  */
    while (1)
      {
        if (uftcflag & UFT_BINARY)
          {
/*
            i = read(fd0,b,BUFSIZ); if (i == 0)
            i = read(fd0,b,BUFSIZ); if (i < 1) break;
 */
            i = uft_readspan(fd0,b,BUFSIZ); if (i < 1) break;
          }
        else
          {
            i = uftctext(fd0,b,BUFSIZ);  if (i == 0)
            i = uftctext(fd0,b,BUFSIZ);  if (i < 1) break;
          }
        (void) sprintf(temp,"DATA %d",i);
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
        (void) tcpputs(s,temp);
        (void) tcpwrite(s,b,i);
        i = uftcwack(r,temp,sizeof(temp));
        if (i < 0)
          {
            if (errno != 0) (void) perror(arg0);
            else (void) uft_putline(2,temp);
            return i;
          }
        if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
      }

    /*  close the file handle  */
    (void) close(fd0);

    /*  signal end-of-file to the server  */
    (void) sprintf(temp,"EOF");
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
    (void) tcpputs(s,temp);
    i = uftcwack(r,temp,sizeof(temp));
    if (i < 0)
      {
        if (errno != 0) (void) perror(arg0);
        else (void) uft_putline(2,temp);
        return i;
      }
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

    /*  tell the server we're done  */
    (void) sprintf(temp,"QUIT");
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);
    (void) tcpputs(s,temp);
    i = uftcwack(r,temp,sizeof(temp));
    if (i < 0)
      {
        if (errno != 0) (void) perror(arg0);
        else (void) uft_putline(2,temp);
        return i;
      }
    if (uftcflag & UFT_VERBOSE) (void) uft_putline(2,temp);

    /*  close the socket  */
    (void) close(s);

    /*  get outta here  */
    return 0;
 }


/*

	-#  copies
	-c  class
	-ms  dist  (aka "mail stop")
	-q
	-f

	DEST
	CLASS
	UCS
	FCB
 */



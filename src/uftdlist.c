/*
 *
 *	  Name: uftdlist.c
 *		list a network file after arrival
 *	Author: Rick Troth, Decatur, Alabama, USA
 *	  Date: 1995-Nov-22
 *
 *		This function writes a one-line file
 *		whose contents mimic  'ls -l'  output,
 *		but for "network files" available to the user
 *		rather than ordinary files in a filesystem.
 *
 */

#include	<string.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<time.h>

#include	"uft.h"

/*  there's gotta be a better way to do this than to hard-code it!  */
static  char  *mon[]  =
		{	"Jan",	"Feb",	"Mar",	"Apr",
			"May",	"Jun",	"Jul",	"Aug",
			"Sep",	"Oct",	"Nov",	"Dec"		} ; 
/*  but I haven't learned enough UNIX yet
    to know where months are localized ... what headers? functions?  */

extern struct  UFTFILE  uftfile0;

/*  ----------------------------------------------------------- UFTDLIST
 */
int uftdlist(int seqn,char*from)
  {
    static char *eyecatch = "uftdlist()";

    char	string[80];
    int 	fd, i;
    char	*p, user[9], host[9], name[17];

    time_t  t0 ;   struct  tm  *t1 ;

    t0 = time(NULL);
    t1 = localtime(&t0);

    /*  open a listing file for this UFT object  */
    (void) sprintf(string,"%04d.lf",seqn);
    fd = open(string,O_RDWR|O_CREAT,S_IREAD);
    if (fd < 0) return fd;

    /*  truncate excesses  */
    (void) strncpy(name,uftfile0.name,16);
    name[16] = 0x00;

    p = uftfile0.from;
    for (i = 0 ; i < 8 ; i++)
      {
	if (*p == 0x00) break;
	if (*p == '@') break;
	user[i] = *p++;
      }
    user[i] = 0x00;
    if (user[0] == 0x00) { user[0] = '-'; user[1] = 0x00; }

    if (*p == '@') p++;
    for (i = 0 ; i < 8 ; i++)
      {
	if (*p == 0x00) break;
	/*  if (*p == '.') break;  */
	host[i] = *p++;
      }
    host[i] = 0x00;
    if (host[0] == 0x00) { host[0] = '-'; host[1] = 0x00; }

    /*  build an 'ls'-style list entry for this UFT file  */
    (void) sprintf(string,
"%c%c%c%c%c%c%c%c%c%c %3d %-8s %-8s %8d %3s %02d %02d:%02d %04d %s",
		uftfile0.type[0], uftfile0.cc[0],
		uftfile0.hold[0], uftfile0.class[0],
		uftfile0.devtype[0], uftfile0.keep[0], uftfile0.msg[0],
		'-',	'-',	'-',
		uftfile0.copies, user, host, uftfile0.size,
		mon[t1->tm_mon], t1->tm_mday, t1->tm_hour, t1->tm_min,
		seqn, uftfile0.name);

    /*  write the record  */
    (void) uft_putline(fd,string);

    (void) close(fd);

    return 0;
  }

/*

	Assignments of the left 10 byte positions:

	TYPE

	r CC		ASA (A) or "machine" (M) or none (dash)
	w HOLD		none (dash), user (H), system (S), both (D)
	x CLASS 	first letter or none (dash)

	r DEVTYPE	PRT (T) or PUN (U) or none (dash)
	w KEEP
	x MSG

	r ...
	w ...
	x ...

	Not processed: FORM DIST DEST

 */

/*

for ordinary files:
-uuugggooo lnk owner... group... ....size mon dd time  name
trwxrwxrwx --- -------- -------- -------- --- -- ----- ----------------...
-rw-r--r--   1 troth    root          282 Oct 13 22:31 uftdlist.c
-rw-r--r--   1 troth    root          281 Nov 22  1995 uftdlist.c;1

for network spool files:
tqhcdkm--- cpy user     host         size mon dd time  sqid name
---------- --- -------- -------- -------- --- -- --:-- ---- ----------------
||||||\___ msg (M) nomsg (-)
|||||\____ keep (K) consume (-)
||||\_____ devtype (prT, Con, pUn)
|||\______ class (A, B, C, etc.)
||\_______ hold (H) nohold (-)
|\________ CC (Asa, Machine, none)
\_________ type (I or A)

 */



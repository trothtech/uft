/*
 *
 *        Name: xmmwin.h (C program header)
 *              special support for XMM/XMITMSGX on MS Windows
 *      Author: Rick Troth, rogue programmer
 *        Date: 2026-05-09 (Sat)
 *
 */

#define  LOG_EMERG     0   /* system is unusable */
#define  LOG_ALERT     1   /* action must be taken immediately */
#define  LOG_CRIT      2   /* critical conditions */
#define  LOG_ERR       3   /* error conditions */
#define  LOG_WARNING   4   /* warning conditions */
#define  LOG_NOTICE    5   /* normal but significant condition */
#define  LOG_INFO      6   /* informational */
#define  LOG_DEBUG     7   /* debug-level messages */

#define	LOG_USER	(1<<3)

#define LOG_PID         0x01    /* log the pid with each message */

#define	LOG_KERN	(0<<3)
#define	LOG_MAIL	(2<<3)
#define	LOG_DAEMON	(3<<3)
#define	LOG_AUTH	(4<<3)
#define	LOG_SYSLOG	(5<<3)
#define	LOG_LPR		(6<<3)
#define	LOG_NEWS	(7<<3)
#define	LOG_UUCP	(8<<3)
#define	LOG_CRON	(9<<3)
#define	LOG_AUTHPRIV	(10<<3)
#define LOG_FTP		(11<<3)

/* Codes through 15 are reserved for system use */
#define LOG_LOCAL0	(16<<3)
#define LOG_LOCAL1	(17<<3)
#define LOG_LOCAL2	(18<<3)
#define LOG_LOCAL3	(19<<3)
#define LOG_LOCAL4	(20<<3)
#define LOG_LOCAL5	(21<<3)
#define LOG_LOCAL6	(22<<3)
#define LOG_LOCAL7	(23<<3)

#define LOG_CONS        0x02    /* log on the console if errors in sending */
#define LOG_ODELAY      0x04    /* delay open until first syslog() (default) */
#define LOG_NDELAY      0x08    /* don't delay open */
#define LOG_NOWAIT      0x10    /* don't wait for console forks: DEPRECATED */
#define LOG_PERROR      0x20    /* log to stderr as well */

void closelog(void);
void openlog(char*,int,int);
void syslog(int,char*,char*);             /* see comments in xmmwin.c */

char*getlogin();



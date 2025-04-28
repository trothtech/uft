/* Copyright 1995-2025 Richard M. Troth, all rights reserved. <plaintext>
 *
 *        Name: tcpiolib.c
 *              various TCP utility functions
 *      Author: Rick Troth, Houston, Texas, USA
 *        Date: 1995-Apr-19 and following
 *
 *   Functions:
 *              character set conversion:
 *              htonc(char)
 *              ntohc(char)
 *              htonz(string)
 *              ntohz(string)
 *              htonb(target,source,length)
 *              ntohb(target,source,length)
 *              (above akin to htons(), ntohs(), htonl() and ntohl())
 *
 */

#ifndef         _TCPIO_HEADER_

#include <unistd.h>

#define         TCPSMALL        256
#define         TCPLARGE        4096

/* if we're on IBM OpenVM, define OECS */
#ifdef          __OPEN_VM
#ifndef         OECS
#define         OECS
#endif
#endif

/* if we're on IBM OpenEdition, define OECS */
#ifdef          _OE_SOCKETS
#ifndef         OECS
#define         OECS
#endif
#endif

int tcpopen(char*,int,int);
int mxopen(char*,int,int);
int tcpclose(int);
int tcpgets(int,char*,int);
int tcpputs(int,char*);
int tcpwrite(int,char*,int);
int tcpread(int,char*,int);
int tcpident(int,char*,int);
unsigned char htonc(unsigned char);
unsigned char ntohc(unsigned char);
int htonz(unsigned char*);
int ntohz(unsigned char*);
int htonb(unsigned char*,unsigned char*,size_t);
int ntohb(unsigned char*,unsigned char*,size_t);

#define         _TCPIO_HEADER_
#endif



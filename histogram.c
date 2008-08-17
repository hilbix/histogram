/* $Header$
 *
 * Copyright (C)2008 Valentin Hilbig <webmaster@scylla-charybdis.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 * $Log$
 * Revision 1.1  2008-08-17 21:33:44  tino
 * First version
 *
 */

#include "tino/getopt.h"
#include "tino/alloc.h"

#include "histogram_version.h"

static unsigned long long	count[256+1024];

static void
readfile(const char *name)
{
  FILE	*fd;
  int	c;

  fd	= stdin;
  if (strcmp(name, "-") && (fd=fopen(name, "rb"))==0)
    {
      perror(name);
      exit(1);
    }
  while ((c=getc(fd))!=EOF)
    count[c]++;
  if (fd!=stdin)
    fclose(fd);
}

/* print the histogram
 */
static void
histogram(void)
{
  int	i, values;
  unsigned long long	total;

  printf("[HISTOGRAM count char]\n");
  total	= 0;
  values= 0;
  for (i=0; i<256; i++)
    if (count[i])
      {
	printf("%llu %d\n", count[i], i);
	total+=count[i];
	values++;
      }
  
  printf("%llu total, %d values\n", total, values);
}

/* print a huffman tree
 */
static void
huffman(void)
{
  struct
    {
      unsigned long long	count[512];
      int			node[512], bits[512];
    }	*d;
  int	n, i, nodes, flg;
  unsigned long long	total, totbits;

  d	= tino_alloc0O(sizeof *d);

  n	= 0;
  for (i=256; --i>=0; )
    d->count[i]	= count[i];

  nodes	= 256;
  for (;;)
    {
      unsigned long long	min1, min2;
      int			n1, n2;

      n1	= -1;
      n2	= -1;
      min1	= 0;
      min2	= 0;
      for (i=nodes; --i>=0; )
	if (!d->node[i] && d->count[i])
	  {
	    if (!min1 || min1>=d->count[i])
	      {
		min2	= min1;
		n2	= n1;
		min1	= d->count[i];
		n1	= i;
	      }
	    else if (!min2 || min2>=d->count[i])
	      {
		min2	= d->count[i];
		n2	= i;
	      }
	  }
      if (min2<=0)
	break;

#if 0
      printf("%d %d %d %llu %llu\n", nodes, n1, n2, min1, min2);
#endif
      d->count[nodes]	= min1+min2;
      d->node[n1]	= nodes;
      d->node[n2]	= nodes;
      nodes++;
    }

  do
    {
      int	tmp;

      flg	= 0;
      for (i=nodes; --i>=0; )
	if (d->node[i] && d->bits[i]!=(tmp=d->bits[d->node[i]]+1))
	  {
	    d->bits[i]	= tmp;
	    flg		= 1;
	  }
    } while (flg);

  printf("[HUFFMAN bits char count]\n");
  total		= 0;
  totbits	= 0;
  for (i=0; i<256; i++)
    if (d->bits[i] || d->count[i])
      {
	printf("%d %d %llu\n", d->bits[i], i, count[i]);
	total	+= count[i];
	totbits	+= count[i]*d->bits[i];
      }
  printf("%llu bits, %llu bytes, %llu permille\n", totbits, total, (totbits*125)/total);

  tino_freeO(d);
}

int
main(int argc, char **argv)
{
  int	argn;
  int	do_huffman, do_hist;
  int	cnt;

  cnt	= 0;
  argn	= tino_getopt(argc, argv, 1, 0,
		      TINO_GETOPT_VERSION(HISTOGRAM_VERSION)
		      TINO_GETOPT_LLOPT
		      " file [..]\n"
		      "	Use - as filename to read stdin",

		      TINO_GETOPT_USAGE
		      "help	this help"
		      ,

		      TINO_GETOPT_FLAG
		      TINO_GETOPT_COUNT
		      "huff	print huffman tree"
		      , &cnt
		      , &do_huffman,
		      
		      TINO_GETOPT_FLAG
		      TINO_GETOPT_COUNT
		      "hist	print histogram (default if nothing else given)"
		      , &cnt
		      , &do_hist,
		      
		      NULL
		      );
  if (argn<=0)
    return 1;
  
  for (; argn<argc; argn++)
    readfile(argv[argn]);

  if (do_huffman)
    huffman();
  if (do_hist || !cnt)
    histogram();
  return 0;
}

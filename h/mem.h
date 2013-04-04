/* mem.h - freestk, roundew, truncew , roundmb, truncmb */

#ifndef _MEM_H_
#define _MEM_H_

/*----------------------------------------------------------------------
 * roundew, truncew - round or trunctate address to next even word
 *----------------------------------------------------------------------
 */
#define	roundew(x)	(WORD *)( (3 + (WORD)(x)) & ~03 )
#define	truncew(x)	(WORD *)( ((WORD)(x)) & ~03 )


/*----------------------------------------------------------------------
 * roundmb, truncmb -- round or truncate address up to size of mblock
 *----------------------------------------------------------------------
 */
#define	roundmb(x)	(WORD *)( (7 + (WORD)(x)) & ~07 )
#define	truncmb(x)	(WORD *)( ((WORD)(x)) & ~07 )


/*----------------------------------------------------------------------
 *  freestk  --  free stack memory allocated by getstk
 *----------------------------------------------------------------------
 */
#define freestk(p,len)	freemem((struct mblock*)((unsigned)(p)	\
				- (unsigned)(roundmb(len))	\
				+ (unsigned)sizeof(int)),	\
				(int)roundmb(len) )

// How does this list of free blocks work? 
//     - Since it is memory and the block of memory is free
//       then it is free to store the data elements mnext and 
//       mlen within the memory itself. No external data structure
//       is needed. 
struct	mblock	{
	struct	mblock	*mnext;
	unsigned int	mlen;
};
extern	struct	mblock	memlist;	/* head of free memory list	*/
extern	char	*maxaddr;		/* max memory address		*/
extern	WORD	_end;			/* address beyond loaded memory	*/
extern	WORD	*end;			/* &_end + FILLSIZE		*/

#endif

/* > netartic.c
 * Split up network at articulation points
 * Copyright (C) 1993-1999 Olly Betts
 */

#if 0
# define DEBUG_INVALID 1
# define DEBUG_ARTIC
#endif

#define DEBUG_ARTIC

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "matrix.h"
#include "out.h"

/* We want to split station list into a list of components, each of which
 * consists of a list of "articulations" - the first has all the fixed points
 * in that component, and the others attach sequentially to produce the whole
 * component
 */

typedef struct articulation {
   struct articulation *next;
   node *stnlist;
} articulation;

typedef struct component {
   struct component *next;
   articulation *artic;
} component;

static component *component_list = NULL;

static node *artlist = NULL;

static node *fixedlist = NULL;

/* list item visit */
/* FIXME: we shouldn't malloc each and every LIV separately! */
typedef struct LIV {
   struct LIV *next;
   uchar dirn;
} liv;

/* The goto iter/uniter avoids using recursion which could lead to stack
 * overflow.  Instead we use a linked list which will probably use
 * much less memory on most compilers.
 */

static long colour;

static ulong
visit(node *stn, int back)
{
   long min;
   int i;
   node *stn2;
   liv *livTos = NULL, *livTmp;
   int artic_flag = 0;
#ifdef DEBUG_ARTIC
   printf("visit(%p, %d) called\n", stn, back);
#endif

iter:
   min = ++colour;
#ifdef DEBUG_ARTIC
   printf("visit: stn [%p] ", stn);
   print_prefix(stn->name);
   printf(" set to colour %d -> min ", colour);
#endif
   for (i = 0; i <= 2 && stn->leg[i]; i++) {
      if (i != back) {
	 long c = stn->leg[i]->l.to->colour;
      
	 if (c < 0) {
	    /* we've found a fixed point */
	    c = -c;
	    stn->leg[i]->l.to->colour = c;
	    /* move it onto the normal stnlist - FIXME: we can avoid this
	     * step by removing the need to remove stn from a list - store
	     * a dummy node in the list head, then just reknit in the add_
	     * routine... */
	    remove_stn_from_list(&fixedlist, stn);
	    add_stn_to_list(&stnlist, stn);
	 }
	 
	 if (c && c < min) min = c;
      }
   }
   stn->colour = min;
#ifdef DEBUG_ARTIC
   printf("%d\n", min);
#endif
   for (i = 0; i <= 2 && stn->leg[i]; i++) {
      if (stn->leg[i]->l.to->colour == 0) {
	 livTmp = osnew(liv);
	 livTmp->next = livTos;
	 livTos = livTmp;
	 livTos->dirn = back = reverse_leg_dirn(stn->leg[i]);
	 stn = stn->leg[i]->l.to;
	 goto iter;
uniter:
	 i = reverse_leg_dirn(stn->leg[livTos->dirn]);
	 stn2 = stn->leg[livTos->dirn]->l.to;

#ifdef DEBUG_ARTIC
	 printf("unwind: stn [%p] ", stn2);
	 print_prefix(stn2->name);
	 printf(" colour %d, min %d, station after %d\n", stn2->colour,
		min, stn->colour);
#endif

	 /* FIXME: hmm, this code looks like it may get called more than once,
	  * or never - actually it seems to always get called exactly once! */
	 remove_stn_from_list(&stnlist, stn);
	 add_stn_to_list(&artlist, stn);

	 printf(">> %p\n", stn);
	 
	 if (artic_flag) {	    
	    articulation *art;
	    
	    artic_flag = 0;
	    /* DO: note down leg (<-), remove and replace:
	     *                 /\   /        /\
             * [fixed point(s)]  *-*  -> [..]  )
	     *                 \/   \        \/
	     *                stn2 stn
	     */
	    /* start new articulation */
	    component_list->artic->stnlist = artlist;
	    artlist = NULL;
	    
	    art = osnew(articulation);
	    art->next = component_list->artic;
	    component_list->artic = art;
/*
	    printf("Articulate *-");
	    print_prefix(stn2->name);
	    printf("-");
	    print_prefix(stn->name);
	    printf("-...\n");
 */
	 }

	 if (stn2->colour == min) {
	    artic_flag = 1;
	 } else if (stn2->colour < min) {
	    min = stn2->colour;
	 }

	 stn = stn2;
	 livTmp = livTos;
	 livTos = livTos->next;
	 osfree(livTmp);
      }
   }
   if (livTos) goto uniter;
   remove_stn_from_list(&stnlist, stn);
   add_stn_to_list(&artlist, stn);
   return min;
}

extern void
articulate(void)
{
   node *stn, *stnStart;
   node *matrixlist = NULL;
   int i;
#ifdef DEBUG_ARTIC
   ulong cFixed;
#endif
   /* find articulation points and components */
   cComponents = 0;
   colour = 0;
   stnStart = NULL;
   FOR_EACH_STN(stn, stnlist) {
      /* stn->fArtic = fFalse; */
      if (fixed(stn)) {
	 colour++;
	 stn->colour = -colour;
	 remove_stn_from_list(&stnlist, stn);
	 add_stn_to_list(&fixedlist, stn);
      } else {
	 stn->colour = 0;
      }
   }
   stnStart = fixedlist;
   ASSERT2(stnStart,"no fixed points!");
#ifdef DEBUG_ARTIC
   cFixed = colour;
#endif
   while (1) {
      int c;
      stn = stnStart;

      /* see if this is a fresh component - it may not be, we may be
       * processing the other way from a fixed point cut-line */
      if (stn->colour < 0) {	 
	 component *comp;
	 articulation *art;
	 
	 printf("new component\n");
	 stn->colour = -stn->colour; /* fixed points are negative until we colour from them */
	 cComponents++;

	 /* FIXME: logic to count components isn't the same as the logic
	  * to start a new one - we should start a new one for a fixed point
	  * cut-line I think */
	 if (component_list) component_list->artic->stnlist = artlist;
	 
	 art = osnew(articulation);
	 art->next = NULL;
	 artlist = NULL;

	 comp = osnew(component);
	 comp->next = component_list;
	 comp->artic = art;
	 component_list = comp;
      }

#ifdef DEBUG_ARTIC
      print_prefix(stn->name);
      printf(" [%p] is root of component %ld\n", stn, cComponents);
      printf(" and colour = %d/%d\n", stn->colour, cFixed);
#endif

      c = 0;
      for (i = 0; i <= 2 && stn->leg[i]; i++) {
	 node *stn2 = stn->leg[i]->l.to;
	 if (stn2->colour < 0) {
	    stn2->colour = -stn2->colour;
	 } else if (stn2->colour == 0) {
	    ulong n;
	    ulong colBefore = colour;

	    /* Special case to check if start station is an articulation point
	     * which it is iff we have to colour from it in more than one dirn
	     */
	    if (c) {
	       /* stn2->fArtic = fTrue; */
	    }
	    
	    c++;
	    visit(stn2, reverse_leg_dirn(stn->leg[i]));
	    n = colour - colBefore;
#ifdef DEBUG_ARTIC
	    printf("visited %lu nodes\n", n);
#endif
	    
#if 0 /* FIXME: */
	    if (n == 0) continue;
	    /* Solve chunk of net from stn in dirn i up to stations
	     * with fArtic set or fixed() true - hmm fixed() test
	     * causes problems if an equated fixed point spans 2
	     * articulations.
	     * Then solve stations up to next set of fArtic points,
	     * and repeat until all this bit done.
	     */
	    stn->status = statFixed;
more:
	    solve_matrix(stnlist);
	    FOR_EACH_STN(stn2, stnlist) {
	       if (stn2->fArtic && fixed(stn2)) {
		  int d;
		  for (d = 0; d <= 2 && stn->leg[d]; d++) {
		     node *stn3 = stn2->leg[d]->l.to;
		     if (!fixed(stn3)) {
			stn2 = stn3;
			goto more;
		     }
		  }
		  stn2->fArtic = fFalse;
	       }
	    }
#endif
	 }
      }

      remove_stn_from_list(&fixedlist, stn);
      add_stn_to_list(&artlist, stn);

#if 0 /* def DEBUG_ARTIC */
      printf("station colours:\n");
      FOR_EACH_STN(stn, stnlist) {
	 printf("%ld - ", stn->colour);
	 print_prefix(stn->name);
	 putnl();
      }
#endif
      if (stnStart->colour == 1) {
#ifdef DEBUG_ARTIC
	 printf("%ld components\n",cComponents);
#endif
	 break;
      }
      stnStart = fixedlist;
      if (!stnStart) break;
   }
   if (component_list) component_list->artic->stnlist = artlist;
   /* if (artlist) component_list->artic->stnlist = artlist; */

     {
	component *comp;
	articulation *art;
	node *stn;

	printf("\nDump of %d components:\n", cComponents);
	for (comp = component_list; comp; comp = comp->next) {
	   node *list = NULL, *listend = NULL;
	   printf("Component:\n");
	   for (art = comp->artic; art; art = art->next) {
	      printf("  Articulation (%p):\n", art->stnlist);
	      if (listend) {
		 listend->next = art->stnlist;
	      } else {
		 list = art->stnlist;
	      }
		 
	      FOR_EACH_STN(stn, art->stnlist) {
		 printf("    %d %p (", stn->colour, stn);
		 print_prefix(stn->name);
		 printf(")\n");
		 listend = stn;
	      }
	   }
	   FOR_EACH_STN(stn, list) {
	      printf("MX: %c %p (", fixed(stn)?'*':' ', stn);
	      print_prefix(stn->name);
	      printf(")\n");	      
	   }
	   solve_matrix(list);
	   listend = stnlist;
	   stnlist = list;
	}
	printf("\n");
     }
   
#if 0 /*def DEBUG_ARTIC*/
   FOR_EACH_STN(stn, stnlist) { /* high-light unfixed bits */
      if (stn->status && !fixed(stn)) {
	 print_prefix(stn->name);
	 printf(" [%p] UNFIXED\n", stn);
      }
   }
#endif
}

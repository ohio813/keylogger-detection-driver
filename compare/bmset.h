
#ifndef _BMSET_H_
#define _BMSET_H_

#include "ac.h"
#ifdef STRMAT
#include "stree_strmat.h"
#else
#include "stree.h"
#endif

typedef enum { BMSET_BADONLY, BMSET_2TREES, BMSET_1TREE } BMSETALG_TYPE;

typedef struct badlist {
  int position;
  struct badlist *next;
} BMSET_BADLIST_NODE;

typedef struct {
  BMSETALG_TYPE type;

  int num_patterns, patterns_size;
  char **patterns;
  int *ids, *lengths, minlen, totallen;

  AC_STRUCT *ktree;
  SUFFIX_TREE ptree;
  BMSET_BADLIST_NODE **R, *Rbuffer;
  int *d, *z;

  int initflag, endflag;
  int ispreprocessed, errorflag;

  char *T;
  int N, k, h, dshift;
  AC_TREE w;
  STREE_NODE wnode;

  int prep_tree_ops, prep_value_ops;
  int num_compares, num_edges, edge_cost;
  int num_shifts, shift_cost;
  int num_init_mismatch, total_matches;
} BMSET_STRUCT;


BMSET_STRUCT *bmset_badonly_alloc(void);
int bmset_badonly_add_string(BMSET_STRUCT *node, char *P, int M, int id);
int bmset_badonly_del_string(BMSET_STRUCT *node, char *P, int M, int id);
int bmset_badonly_prep(BMSET_STRUCT *node);
void bmset_badonly_search_init(BMSET_STRUCT *node, char *T, int N);
char *bmset_badonly_search(BMSET_STRUCT *node, int *length_out, int *id_out);
void bmset_badonly_free(BMSET_STRUCT *node);


BMSET_STRUCT *bmset_2trees_alloc(void);
int bmset_2trees_add_string(BMSET_STRUCT *node, char *P, int M, int id);
int bmset_2trees_del_string(BMSET_STRUCT *node, char *P, int M, int id);
int bmset_2trees_prep(BMSET_STRUCT *node);
void bmset_2trees_search_init(BMSET_STRUCT *node, char *T, int N);
char *bmset_2trees_search(BMSET_STRUCT *node, int *length_out, int *id_out);
void bmset_2trees_free(BMSET_STRUCT *node);

BMSET_STRUCT *bmset_1tree_alloc(void);
int bmset_1tree_add_string(BMSET_STRUCT *node, char *P, int M, int id);
int bmset_1tree_del_string(BMSET_STRUCT *node, char *P, int M, int id);
int bmset_1tree_prep(BMSET_STRUCT *node);
void bmset_1tree_search_init(BMSET_STRUCT *node, char *T, int N);
char *bmset_1tree_search(BMSET_STRUCT *node, int *length_out, int *id_out);
void bmset_1tree_free(BMSET_STRUCT *node);

#endif

/*
 * bmset.c
 *
 * Implementation of three variations of the Boyer-Moore Set matching
 * algorithm.  The first variation uses only an Aho-Corasick keyword tree and
 * the extended bad character shift rule.  The second variation uses a
 * keyword tree, a suffix tree and both the bad character and good suffix
 * rules.  The third variation only uses a compacted suffix tree, along
 * with the bad character and good suffix rules.
 *
 * NOTES:
 *    9/95  -  Original Implementation (James Knight)
 *    3/96  -  Modularized the code (James Knight)
 *    7/96  -  Finished the modularization (James Knight)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac.h"
#ifdef STRMAT
#include "stree_strmat.h"
#include "stree_ukkonen.h"
#else
#include "stree.h"
#endif
#include "bmset.h"


static int compute_d_and_z_values(BMSET_STRUCT *bmstruct, SUFFIX_TREE tree,
                                  STREE_NODE node);
static int compact_tree(BMSET_STRUCT *bmstruct, SUFFIX_TREE tree,
                        STREE_NODE node);


/*
 *
 *
 * The BMSET version using only the keyword tree.
 *
 *
 */

/*
 * bmset_badonly_alloc
 *
 * Creates a new BMSET_STRUCT structure and initializes its fields.
 *
 * Parameters:    none.
 *
 * Returns:  A dynamically allocated BMSET_STRUCT structure.
 */
BMSET_STRUCT *bmset_badonly_alloc(void)
{
  BMSET_STRUCT *node;

  if ((node = malloc(sizeof(BMSET_STRUCT))) == NULL)
    return NULL;
  memset(node, 0, sizeof(BMSET_STRUCT));

  node->type = BMSET_BADONLY;

  if ((node->ktree = ac_alloc()) == NULL) {
    free(node);
    return NULL;
  }

  return node;
}


/*
 * bmset_badonly_add_string
 *
 * Adds a string to the BMSET_STRUCT structure.
 *
 * NOTE:  The `id' value given must be unique to any of the strings
 *        added to the tree, and must be a small integer greater than
 *        0 (since it is used to index an array holding information
 *        about each of the strings).
 *
 *        The best id's to use are to number the strings from 1 to K.
 *
 *        NOTE:  Parameter `copyflag' is not used here, because the
 *               patterns are always copied as part of the keyword tree.
 *               It's included for compatibility with the other set
 *               matching algorithms.
 *
 * Parameters:   node      -  an BMSET_STRUCT structure
 *               P         -  the sequence
 *               M         -  the sequence length
 *               id        -  the sequence identifier
 *               copyflag  -  make a copy of the sequence?
 *
 * Returns:  non-zero on success, zero on error.
 */
int bmset_badonly_add_string(BMSET_STRUCT *node, char *P, int M, int id)
{
  int i;
  char *Pbar;

  if (node->errorflag)
    return 0;

  if (node->type != BMSET_BADONLY) {
    node->errorflag = 1;
    return 0;
  }

  P--;            /* Shift to make sequence be P[1],...,P[M] */

  /*
   * Check for duplicates.
   */
  for (i=0; i < node->num_patterns; i++) {
    if (node->ids[i] == id) {
      fprintf(stderr, "Error in Boyer-Moore Set preprocessing.  "
              "Duplicate identifiers\n");
      node->errorflag = 1;
      return 0;
    }
  }

  /*
   * Allocate space for the new string's information.
   */
  if (node->num_patterns == node->patterns_size) {
    if (node->patterns_size == 0) {
      node->patterns_size = 16;
      node->patterns = malloc(node->patterns_size * sizeof(char *));
      node->lengths = malloc(node->patterns_size * sizeof(int));
      node->ids = malloc(node->patterns_size * sizeof(int));
    }
    else {
      node->patterns_size += node->patterns_size;
      node->patterns = realloc(node->patterns,
                               node->patterns_size * sizeof(char *));
      node->lengths = realloc(node->lengths,
                              node->patterns_size * sizeof(int));
      node->ids = realloc(node->ids, node->patterns_size * sizeof(int));
    }
    if (node->patterns == NULL ||
        node->lengths == NULL || node->ids == NULL) {
      node->errorflag = 1;
      return 0;
    }
  }

  /*
   * Allocate space for and reverse the new sequence
   */
  if ((Pbar = malloc(M + 2)) == NULL) {
    node->errorflag = 1;
    return 0;
  }
  Pbar[0] = Pbar[M+1] = '\0';
  for (i=1; i <= M; i++)
    Pbar[i] = P[M-i+1];

  /*
   * Initialize the appropriate trees and the fields for the new sequence.
   */
  if (ac_add_string(node->ktree, Pbar+1, M, node->num_patterns+1) == 0) {
    free(Pbar);
    node->errorflag = 1;
    return 0;
  }

#ifdef STATS
  node->prep_tree_ops += node->ktree->prep_new_edges +
                         node->ktree->prep_old_edges;
  node->ktree->prep_new_edges = node->ktree->prep_old_edges = 0;
#endif

  node->patterns[node->num_patterns] = Pbar;
  node->lengths[node->num_patterns] = M;
  node->ids[node->num_patterns] = id;
  node->num_patterns++;
  node->ispreprocessed = 0;

  return 1;
}


/*
 * bmset_badonly_del_string
 *
 * Deletes a string from the set of patterns.
 *
 * Parameters:   node  -  an BMSET_STRUCT structure
 *               P     -  the sequence to be deleted
 *               M     -  its length
 *               id    -  its identifier
 *
 * Returns:  non-zero on success, zero on error.
 */
int bmset_badonly_del_string(BMSET_STRUCT *node, char *P, int M, int id)
{
  int i;

  if (node->errorflag)
    return 0;

  if (node->type != BMSET_BADONLY) {
    node->errorflag = 1;
    return 0;
  }

  /*
   * Find the pattern to be deleted.
   */
  for (i=0; i < node->num_patterns; i++)
    if (node->ids[i] == id)
      break;

  if (i == node->num_patterns) {
    fprintf(stderr, "Error in Boyer-Moore Set preprocessing.  String to be "
            "deleted is not in tree.\n");
    node->errorflag = 1;
    return 0;
  }

  ac_del_string(node->ktree, node->patterns[i] + 1, node->lengths[i], i+1);
  free(node->patterns[i]);

  for (i++; i < node->num_patterns; i++) {
    node->patterns[i-1] = node->patterns[i];
    node->lengths[i-1] = node->lengths[i];
    node->ids[i-1] = node->ids[i];
  }
  node->ispreprocessed = 0;

  return 1;
}


/*
 * bmset_badonly_prep
 *
 * Preprocesses the keyword tree and computes the bad character table.
 *
 * Parameters:  node  -  an BMSET_STRUCT structure
 *
 * Returns: non-zero on success, zero on error.
 */
int bmset_badonly_prep(BMSET_STRUCT *node)
{
  int i, count, pos, minlen, maxlen, totallen, oldtotal;
  char ch;
  BMSET_BADLIST_NODE **R, *Rbuffer;

  if (node->errorflag || node->num_patterns == 0)
    return 0;

  /*
   * Compute the minimal, maximal and total length of the patterns.
   */
  minlen = maxlen = totallen = node->lengths[0];
  for (i=1; i < node->num_patterns; i++) {
    if (node->lengths[i] < minlen)
      minlen = node->lengths[i];
    if (node->lengths[i] > maxlen)
      maxlen = node->lengths[i];

    totallen += node->lengths[i];
  }

  oldtotal = node->totallen;
  node->totallen = totallen;
  node->minlen = minlen;

  /*
   * Allocate space for the R, Rpos and Rnext buffers.
   */
  if (node->R != NULL)
    R = node->R;
  else 
    R = node->R = malloc(128 * sizeof(BMSET_BADLIST_NODE *));

  if (node->Rbuffer != NULL && totallen <= oldtotal)
    Rbuffer = node->Rbuffer;
  else {
    if (node->Rbuffer != NULL)
      free(node->Rbuffer);
    Rbuffer = node->Rbuffer = malloc(totallen * sizeof(BMSET_BADLIST_NODE));
  }

  if (R == NULL || Rbuffer == NULL) {
    node->errorflag = 1;
    return 0;
  }

  for (i=0; i < 128; i++)
    R[i] = NULL;

  /*
   * Extended bad character rule.  The cells of R point to the heads of
   * the lists allocated in Rbuffer.
   *
   * The values stored in the lists are distances from the rightmost
   * character of the patterns.  Remember, the patterns are reversed
   * when this computation occurs.
   */
  count = 0;
  for (pos=maxlen; pos >= 1; pos--) {
    for (i=0; i < node->num_patterns; i++) {
      if (node->lengths[i] < pos)
        continue;

      ch = node->patterns[i][pos];
      if (R[(int)ch] == NULL || R[(int)ch]->position > pos - 1) {
        Rbuffer[count].position = pos - 1;     /* Subtract 1 here to get */
                                               /* correct distance from  */
                                               /* the beginning of the   */
                                               /* pattern.               */
        Rbuffer[count].next = R[(int)ch];
        R[(int)ch] = &Rbuffer[count];
        count++;
      }
#ifdef STATS
      node->prep_value_ops++;
#endif
    }
  }

  node->ispreprocessed = 1;
  node->initflag = 0;

  return 1;
}


/*
 * bmset_badonly_search_init
 *
 * Initializes the variables used during an Aho-Corasick search.
 * See bmset_search for an example of how it should be used.
 *
 * Parameters:  node  -  an BMSET_STRUCT structure
 *              T     -  the sequence to be searched
 *              N     -  the length of the sequence
 *
 * Returns:  nothing.
 */
void bmset_badonly_search_init(BMSET_STRUCT *node, char *T, int N)
{
  if (node->errorflag)
    return;
  else if (!node->ispreprocessed) {
    fprintf(stderr, "Error in Boyer-Moore Set search.  The preprocessing "
            "has not been completed.\n");
    return;
  }

  node->T = T - 1;          /* Shift to make sequence be T[1],...,T[N] */
  node->N = N;
  node->k = node->minlen;
  node->h = 0;
  node->w = NULL;
  node->initflag = 1;
  node->endflag = 0;
}


/*
 * bmset_badonly_search
 *
 * Scans a text to look for the next occurrence of one of the patterns
 * in the text.  An example of how this search should be used is the
 * following:
 *
 *    s = T;
 *    len = N; 
 *    contflag = 0;
 *    bmset_search_init(node, T, N);
 *    while ((s = bmset_search(node, &matchlen, &matchid) != NULL) {
 *      >>> Pattern `matchid' matched from `s' to `s + matchlen - 1'. <<<
 *    }
 *
 * where `node', `T' and `N' are assumed to be initialized appropriately.
 *
 * Parameters:  node           -  a preprocessed BMSET_STRUCT structure
 *              length_out     -  where to store the new match's length
 *              id_out         -  where to store the identifier of the
 *                                pattern that matched
 *
 * Returns:  the left end of the text that matches a pattern, or NULL
 *           if no match occurs.  (It also stores values in `*length_out',
 *           and `*id_out' giving the match's length and pattern identifier.
 */
char *bmset_badonly_search(BMSET_STRUCT *node, int *length_out, int *id_out)
{
  int i, k, h, N, minlen, bshift;
  char *T;
  AC_TREE w, wprime, root;
  BMSET_BADLIST_NODE **R, *Rnode;

  if (node->errorflag)
    return NULL;
  else if (!node->ispreprocessed) {
    fprintf(stderr, "Error in Aho-Corasick search.  The preprocessing "
            "has not been completed.\n");
    return NULL;
  }
  else if (!node->initflag) {
    fprintf(stderr, "Error in Aho-Corasick search.  bmset_search_init was not "
            "called.\n");
    return NULL;
  }
  else if (node->endflag)
    return NULL;

  T = node->T;
  N = node->N;
  R = node->R;
  minlen = node->minlen;

  /*
   * Execute the Boyer-Moore Set Matching algorithm using only the
   * extended bad character shift rule.  Stop at the first occurrence
   * of a match to one of the patterns.
   */
  k = node->k;
  root = node->ktree->tree;
  while (k <= N) {
    /*
     * Perform the test at k (or continue the test if node->w != NULL,
     * which implies that the last search was in the middle of a test when
     * it found a match).
     */
    if (node->w == NULL) {
      h = k;
      w = root;
    }
    else {
      h = node->h;   node->h = 0;
      w = node->w;   node->w = NULL;
    }

    while (h >= 1) {
      /*
       * Find the child of w with label T[h].
       */
#ifdef STATS
      wprime = w->children;
      while (wprime != NULL && wprime->ch < T[h]) {
        wprime = wprime->sibling;
        node->edge_cost++;
      }
      node->edge_cost++;
      node->num_compares++;
#else
      wprime = w->children;
      while (wprime != NULL && wprime->ch < T[h])
        wprime = wprime->sibling;
#endif

      /*
       * Stop if no child's character matches T[h].  Return a match if the
       * new child specifies the end of a pattern.  Otherwise, continue the
       * comparison.
       */
      if (wprime == NULL || wprime->ch > T[h])
        break;

#ifdef STATS
      node->num_edges++;
#endif

      if (wprime->matchid != 0) {
        node->k = k;
        node->h = h - 1;
        node->w = wprime;

        if (id_out)
          *id_out = node->ids[wprime->matchid-1];
        if (length_out)
          *length_out = node->lengths[wprime->matchid-1];

        return &T[h];
      }

      h--;
      w = wprime;
    }

#ifdef STATS
    if (h < 1)
      node->num_compares++;
#endif

    /*
     * Compute the shift distance.  Here, `i' is the number of
     * characters that matched in the test at `k'.
     *
     * If we've matched to the beginning of the text or we've matched more
     * characters than the smallest pattern, no bad character shift is
     * possible and we should just shift by 1.
     *
     * Otherwise, compute the extended bad character shift.  This differs
     * slightly from the book, in that the bad character table `R[ch]'
     * (for a character `ch') contains the distances, from the patterns' 
     * right ends, of every occurrence of character `ch' in the patterns.
     * Since the distance of the mismatch from the patterns' right ends is i,
     * finding the smallest distance in R[T[h]]'s list which is larger
     * than i, and then shifting by the difference between that distance
     * and i, will align that next occurrence of a T[h] character in the
     * shifted patterns with T[h] in the text.
     *
     * One additional note.  Since the patterns cannot be shifted farther
     * than shifting the smallest pattern one character past the mismatch,
     * check the bad shift against shifting the smallest pattern completely
     * past the mismatch and take the smaller of the two shifts.
     */
#ifdef STATS

    i = k - h;
    if (h < 1 || i >= minlen) {
      bshift = 1;
      node->shift_cost++;
    }
    else {
      if (i == 0)
        node->num_init_mismatch++;
      else
        node->total_matches += i;

      Rnode = R[(int) T[h]];
      while (Rnode != NULL && Rnode->position <= i) {
        Rnode = Rnode->next;
        node->shift_cost++;
      }
      node->shift_cost++;

      if (Rnode != NULL && Rnode->position < minlen)
        bshift = Rnode->position - i;
      else
        bshift = minlen - i;

      node->shift_cost++;
    }

    k += bshift;
    node->num_shifts++;

#else

    i = k - h;
    if (h < 1 || i >= minlen)
      bshift = 1;
    else {
      Rnode = R[(int) T[h]];
      while (Rnode != NULL && Rnode->position <= i)
        Rnode = Rnode->next;

      if (Rnode != NULL && Rnode->position < minlen)
        bshift = Rnode->position - i;
      else
        bshift = minlen - i;
    }

    k += bshift;

#endif
  }

  node->endflag = 1;
  return NULL;
}


/*
 * bmset_badonly_free
 *
 * Free up the allocated BMSET_STRUCT structure.
 *
 * Parameters:   node  -  a BMSET_STRUCT structure
 *
 * Returns:  nothing.
 */
void bmset_badonly_free(BMSET_STRUCT *node)
{
  int i;

  if (node == NULL)
    return;

  if (node->ktree)
    ac_free(node->ktree);
  if (node->ptree)
    stree_delete_tree(node->ptree);

  if (node->patterns != NULL) {
    for (i=0; i < node->num_patterns; i++)
      if (node->patterns[i] != NULL)
        free(node->patterns[i]);
    free(node->patterns);
  }
  if (node->lengths != NULL)
    free(node->lengths);
  if (node->ids != NULL)
    free(node->ids);
  if (node->R != NULL)
    free(node->R);
  if (node->Rbuffer != NULL)
    free(node->Rbuffer);
  if (node->d != NULL)
    free(node->d);
  if (node->z != NULL)
    free(node->z);

  free(node);
}




/*
 *
 *
 * The BMSET version using a keyword tree and a suffix tree.
 *
 *
 */

/*
 * bmset_2trees_alloc
 *
 * Creates a new BMSET_STRUCT structure and initializes its fields.
 *
 * Parameters:    none.
 *
 * Returns:  A dynamically allocated BMSET_STRUCT structure.
 */
BMSET_STRUCT *bmset_2trees_alloc(void)
{
  BMSET_STRUCT *node;

  if ((node = malloc(sizeof(BMSET_STRUCT))) == NULL)
    return NULL;
  memset(node, 0, sizeof(BMSET_STRUCT));

  node->type = BMSET_2TREES;

  if ((node->ktree = ac_alloc()) == NULL) {
    free(node);
    return NULL;
  }

#ifdef STRMAT
  node->ptree = stree_new_tree(128, 0, SORTED_LIST, 0);
#else
  node->ptree = stree_new_tree(128, 0);
#endif
  if (node->ptree == NULL) {
    ac_free(node->ktree);
    free(node);
    return NULL;
  }

  return node;
}


/*
 * bmset_2trees_add_string
 *
 * Adds a string to the BMSET_STRUCT structure.
 *
 * NOTE:  The `id' value given must be unique to any of the strings
 *        added to the tree, and must be a small integer greater than
 *        0 (since it is used to index an array holding information
 *        about each of the strings).
 *
 *        The best id's to use are to number the strings from 1 to K.
 *
 *        NOTE:  Parameter `copyflag' is not used here, because the
 *               patterns are always copied as part of the keyword tree.
 *               It's included for compatibility with the other set
 *               matching algorithms.
 *
 * Parameters:   node      -  an BMSET_STRUCT structure
 *               P         -  the sequence
 *               M         -  the sequence length
 *               id        -  the sequence identifier
 *               copyflag  -  make a copy of the sequence?
 *
 * Returns:  non-zero on success, zero on error.
 */
int bmset_2trees_add_string(BMSET_STRUCT *node, char *P, int M, int id)
{
  int i, flag;
  char *Pbar;

  if (node->errorflag)
    return 0;

  if (node->type != BMSET_2TREES) {
    node->errorflag = 1;
    return 0;
  }

  P--;            /* Shift to make sequence be P[1],...,P[M] */

  /*
   * Check for duplicates.
   */
  for (i=0; i < node->num_patterns; i++) {
    if (node->ids[i] == id) {
      fprintf(stderr, "Error in Boyer-Moore Set preprocessing.  "
              "Duplicate identifiers\n");
      node->errorflag = 1;
      return 0;
    }
  }

  /*
   * Allocate space for the new string's information.
   */
  if (node->num_patterns == node->patterns_size) {
    if (node->patterns_size == 0) {
      node->patterns_size = 16;
      node->patterns = malloc(node->patterns_size * sizeof(char *));
      node->lengths = malloc(node->patterns_size * sizeof(int));
      node->ids = malloc(node->patterns_size * sizeof(int));
    }
    else {
      node->patterns_size += node->patterns_size;
      node->patterns = realloc(node->patterns,
                               node->patterns_size * sizeof(char *));
      node->lengths = realloc(node->lengths,
                              node->patterns_size * sizeof(int));
      node->ids = realloc(node->ids, node->patterns_size * sizeof(int));
    }
    if (node->patterns == NULL ||
        node->lengths == NULL || node->ids == NULL) {
      node->errorflag = 1;
      return 0;
    }
  }

  /*
   * Allocate space for and reverse the new sequence
   */
  if ((Pbar = malloc(M + 2)) == NULL) {
    node->errorflag = 1;
    return 0;
  }
  Pbar[0] = Pbar[M+1] = '\0';
  for (i=1; i <= M; i++)
    Pbar[i] = P[M-i+1];

  /*
   * Initialize the appropriate trees and the fields for the new sequence.
   */
  if (ac_add_string(node->ktree, Pbar+1, M, node->num_patterns+1) == 0) {
    free(Pbar);
    node->errorflag = 1;
    return 0;
  }

#ifdef STATS
  node->prep_tree_ops += node->ktree->prep_new_edges +
                         node->ktree->prep_old_edges;
  node->ktree->prep_new_edges = node->ktree->prep_old_edges = 0;
#endif

#ifdef STRMAT
  flag = stree_ukkonen_add_string(node->ptree, Pbar+1, Pbar+1, M,
                                  node->num_patterns+1);
#else
  flag = stree_add_string(node->ptree, Pbar+1, M, node->num_patterns+1);
#endif
  if (flag <= 0) {
    free(Pbar);
    node->errorflag = 1;
    return 0;
  }

#ifdef STATS
  node->prep_tree_ops += node->ptree->num_compares +
                         node->ptree->creation_cost;
  node->ptree->num_compares = node->ptree->creation_cost = 0;
#endif

  node->patterns[node->num_patterns] = Pbar;
  node->lengths[node->num_patterns] = M;
  node->ids[node->num_patterns] = id;
  node->num_patterns++;
  node->ispreprocessed = 0;

  return 1;
}


/*
 * bmset_2trees_del_string
 *
 * Deletes a string from the set of patterns.
 *
 * NOTE:  This procedure should not be used from inside strmat.
 *
 * Parameters:   node  -  an BMSET_STRUCT structure
 *               P     -  the sequence to be deleted
 *               M     -  its length
 *               id    -  its identifier
 *
 * Returns:  non-zero on success, zero on error.
 */
int bmset_2trees_del_string(BMSET_STRUCT *node, char *P, int M, int id)
{
  int i;

  if (node->errorflag)
    return 0;

  if (node->type != BMSET_2TREES) {
    node->errorflag = 1;
    return 0;
  }

  /*
   * Find the pattern to be deleted.
   */
  for (i=0; i < node->num_patterns; i++)
    if (node->ids[i] == id)
      break;

  if (i == node->num_patterns) {
    fprintf(stderr, "Error in Boyer-Moore Set preprocessing.  String to be "
            "deleted is not in tree.\n");
    node->errorflag = 1;
    return 0;
  }

  /*
   * Remove it from the trees, and then free up the memory.
   */
  ac_del_string(node->ktree, node->patterns[i] + 1, node->lengths[i], i+1);
#ifndef STRMAT
  stree_delete_string(node->ptree, i+1);
#endif

  free(node->patterns[i]);

  /*
   * Shift the other patterns over one.
   */
  for (i++; i < node->num_patterns; i++) {
    node->patterns[i-1] = node->patterns[i];
    node->lengths[i-1] = node->lengths[i];
    node->ids[i-1] = node->ids[i];
  }
  node->ispreprocessed = 0;

  return 1;
}


/*
 * bmset_2trees_prep
 *
 * Preprocesses the keyword tree and computes the bad character table.
 *
 * Parameters:  node  -  an BMSET_STRUCT structure
 *
 * Returns: non-zero on success, zero on error.
 */
int bmset_2trees_prep(BMSET_STRUCT *node)
{
  int num_nodes;

  if (node->errorflag || node->num_patterns == 0)
    return 0;

  if (bmset_badonly_prep(node) == 0)
    return 0;

  /*
   * Compute the d and z values using the suffix tree.
   */
  num_nodes = stree_get_num_nodes(node->ptree);
  if ((node->d = malloc(num_nodes * sizeof(int))) == NULL ||
      (node->z = malloc(num_nodes * sizeof(int))) == NULL) {
    node->errorflag = 1;
    return 0;
  }

  compute_d_and_z_values(node, node->ptree, stree_get_root(node->ptree));

  return 1;
}


static int compute_d_and_z_values(BMSET_STRUCT *bmstruct, SUFFIX_TREE tree,
                                  STREE_NODE node)
{
  int i, id, least_z, zval, least_d, pos;
  char *str;
  STREE_NODE child;

  child = stree_get_children(tree, node);
  least_z = 0;
  while (child != NULL) {
    zval = compute_d_and_z_values(bmstruct, tree, child);
    if (zval > 1 && (least_z == 0 || zval < least_z))
      least_z = zval;

#ifdef STATS
    bmstruct->prep_value_ops++;
#endif

    child = stree_get_next(tree, child);
  }
  
  least_d = 0;
  for (i=1; stree_get_leaf(tree, node, i, &str, &pos, &id); i++) {
    if (least_d == 0 || pos + 1 < least_d)
      least_d = pos + 1;

    if (pos + 1 > 1 && (least_z == 0 || pos + 1 < least_z))
      least_z = pos + 1;

#ifdef STATS
    bmstruct->prep_value_ops++;
#endif
  }

  id = stree_get_ident(tree, node);
  bmstruct->z[id] = least_z;
  bmstruct->d[id] = least_d;

  return least_z;
}




/*
 * bmset_2trees_search_init
 *
 * Initializes the variables used during an Aho-Corasick search.
 * See bmset_search for an example of how it should be used.
 *
 * Parameters:  node  -  an BMSET_STRUCT structure
 *              T     -  the sequence to be searched
 *              N     -  the length of the sequence
 *
 * Returns:  nothing.
 */
void bmset_2trees_search_init(BMSET_STRUCT *node, char *T, int N)
{
  if (node->errorflag)
    return;
  else if (!node->ispreprocessed) {
    fprintf(stderr, "Error in Boyer-Moore Set search.  The preprocessing "
            "has not been completed.\n");
    return;
  }

  node->T = T - 1;          /* Shift to make sequence be T[1],...,T[N] */
  node->N = N;
  node->k = node->minlen;
  node->h = 0;
  node->w = NULL;
  node->initflag = 1;
  node->endflag = 0;
}


/*
 * bmset_2trees_search
 *
 * Scans a text to look for the next occurrence of one of the patterns
 * in the text.  An example of how this search should be used is the
 * following:
 *
 *    s = T;
 *    len = N; 
 *    contflag = 0;
 *    bmset_search_init(node, T, N);
 *    while ((s = bmset_search(node, &matchlen, &matchid) != NULL) {
 *      >>> Pattern `matchid' matched from `s' to `s + matchlen - 1'. <<<
 *    }
 *
 * where `node', `T' and `N' are assumed to be initialized appropriately.
 *
 * Parameters:  node           -  a preprocessed BMSET_STRUCT structure
 *              length_out     -  where to store the new match's length
 *              id_out         -  where to store the identifier of the
 *                                pattern that matched
 *
 * Returns:  the left end of the text that matches a pattern, or NULL
 *           if no match occurs.  (It also stores values in `*length_out',
 *           and `*id_out' giving the match's length and pattern identifier.
 */
char *bmset_2trees_search(BMSET_STRUCT *node, int *length_out, int *id_out)
{
  int i, k, h, N, minlen, bshift, gshift, dshift, zshift, dval, walklen;
  char *T;
  AC_TREE w, wprime, root;
  STREE_NODE treenode;
  BMSET_BADLIST_NODE **R, *Rnode;

  if (node->errorflag)
    return NULL;
  else if (!node->ispreprocessed) {
    fprintf(stderr, "Error in Aho-Corasick search.  The preprocessing "
            "has not been completed.\n");
    return NULL;
  }
  else if (!node->initflag) {
    fprintf(stderr, "Error in Aho-Corasick search.  bmset_search_init was not "
            "called.\n");
    return NULL;
  }
  else if (node->endflag)
    return NULL;

  T = node->T;
  N = node->N;
  R = node->R;
  minlen = node->minlen;

  /*
   * Execute the Boyer-Moore Set Matching algorithm using only the
   * extended bad character shift rule.  Stop at the first occurrence
   * of a match to one of the patterns.
   *
   * NOTE:  Everything here is the same as the BMSET_BADONLY search,
   *        except for the computation of the "good" shift value.
   */
  k = node->k;
  root = node->ktree->tree;
  while (k <= N) {
    /*
     * Perform the test at k (or continue the test if node->w != NULL,
     * which implies that the last search was in the middle of a test when
     * it found a match).
     */
    if (node->w == NULL) {
      h = k;
      w = root;
    }
    else {
      h = node->h;   node->h = 0;
      w = node->w;   node->w = NULL;
    }

    while (h >= 1) {
      /*
       * Find the child of w with label T[h].
       */
#ifdef STATS
      wprime = w->children;
      while (wprime != NULL && wprime->ch < T[h]) {
        wprime = wprime->sibling;
        node->edge_cost++;
      }
      node->edge_cost++;
      node->num_compares++;
#else
      wprime = w->children;
      while (wprime != NULL && wprime->ch < T[h])
        wprime = wprime->sibling;
#endif

      /*
       * Stop if no child's character matches T[h].  Return a match if the
       * new child specifies the end of a pattern.  Otherwise, continue the
       * comparison.
       */
      if (wprime == NULL || wprime->ch > T[h])
        break;

#ifdef STATS
      node->num_edges++;
#endif

      if (wprime->matchid != 0) {
        node->k = k;
        node->h = h - 1;
        node->w = wprime;
        
        if (id_out)
          *id_out = node->ids[wprime->matchid-1];
        if (length_out)
          *length_out = node->lengths[wprime->matchid-1];

        return &T[h];
      }

      h--;
      w = wprime;
    }

#ifdef STATS
    if (h < 1)
      node->num_compares++;
#endif

    /*
     * Compute the shift distance.  Here, `i' is the number of
     * characters that matched in the test at `k'.
     *
     * If we've matched to the beginning of the text or we've matched more
     * characters than the smallest pattern, no bad character shift is
     * possible and we should just shift by 1.
     *
     * Otherwise, compute the extended bad character shift.  This differs
     * slightly from the book, in that the bad character table `R[ch]'
     * (for a character `ch') contains the distances, from the patterns' 
     * right ends, of every occurrence of character `ch' in the patterns.
     * Since the distance of the mismatch from the patterns' right ends is i,
     * finding the smallest distance in R[T[h]]'s list which is larger
     * than i, and then shifting by the difference between that distance
     * and i, will align that next occurrence of a T[h] character in the
     * shifted patterns with T[h] in the text.
     *
     * One additional note.  Since the patterns cannot be shifted farther
     * than shifting the smallest pattern one character past the mismatch,
     * check the bad shift against shifting the smallest pattern completely
     * past the mismatch and take the smaller of the two shifts.
     */
#ifdef STATS

    i = k - h;
    if (h < 1 || i >= minlen) {
      bshift = 1;
      node->shift_cost++;
    }
    else {
      if (i == 0)
        node->num_init_mismatch++;
      else
        node->total_matches += i;

      Rnode = R[(int) T[h]];
      while (Rnode != NULL && Rnode->position <= i) {
        Rnode = Rnode->next;
        node->shift_cost++;
      }
      node->shift_cost++;

      if (Rnode != NULL && Rnode->position < minlen)
        bshift = Rnode->position - i;
      else
        bshift = minlen - i;

      node->shift_cost++;
    }

#else

    i = k - h;
    if (h < 1 || i >= minlen)
      bshift = 1;
    else {
      Rnode = R[(int) T[h]];
      while (Rnode != NULL && Rnode->position <= i)
        Rnode = Rnode->next;

      if (Rnode != NULL && Rnode->position < minlen)
        bshift = Rnode->position - i;
      else
        bshift = minlen - i;
    }

#endif

    /*
     * Use the suffix tree to compute the zshift, dshift and then the
     * good shift values.
     */
    gshift = 0;
    if (i > 0) {
      dshift = zshift = 0;

#ifdef STATS
      node->ptree->child_cost = 0;
#endif

      /*
       * Use the skip/count trick to walk down the tree edges.
       */
      treenode = stree_get_root(node->ptree);
      walklen = 0;
      while (walklen < i) {
        treenode = stree_find_child(node->ptree, treenode, T[k-walklen]);
        if (treenode == NULL) {
          node->errorflag = 1;
          return NULL;
        }

#ifdef STATS
        node->num_compares++;
        node->num_edges++;
        node->edge_cost += node->ptree->child_cost;
        node->ptree->child_cost = 0;
#endif

        walklen += stree_get_edgelen(node->ptree, treenode);
        if (walklen > i)
          break;

        /*
         * Pick up the smallest d value along the path.
         */
        dval = node->d[stree_get_ident(node->ptree, treenode)];
        if (dval > 0 && (dshift == 0 || dval < dshift))
          dshift = dval;

#ifdef STATS
        node->shift_cost++;
#endif
      }

      /*
       * Pick up the z value at the end of the path.
       */
      zshift = node->z[stree_get_ident(node->ptree, treenode)];

#ifdef STATS
      node->shift_cost++;
#endif

      /*
       * Compute the good shift.
       */
      gshift = node->minlen;
      if (zshift > 1 && zshift - 1 < gshift)
        gshift = zshift - 1;
      if (dshift > 1 && dshift - 1 < gshift)
        gshift = dshift - 1;

#ifdef STATS
      node->shift_cost += 2;
#endif
    }

    /*
     * Compute the final shift from the good and bad shifts.
     */
    k += (bshift > gshift ? bshift : gshift);

#ifdef STATS
    node->num_shifts++;
#endif
  }

  node->endflag = 1;
  return NULL;
}


/*
 * bmset_2trees_free
 *
 * Free up the allocated BMSET_STRUCT structure.
 *
 * Parameters:   node  -  a BMSET_STRUCT structure
 *
 * Returns:  nothing.
 */
void bmset_2trees_free(BMSET_STRUCT *node)
{
  bmset_badonly_free(node);
}




/*
 *
 *
 * The BMSET version using just a compacted suffix tree.
 *
 *
 */

/*
 * bmset_1tree_alloc
 *
 * Creates a new BMSET_STRUCT structure and initializes its fields.
 *
 * Parameters:    none.
 *
 * Returns:  A dynamically allocated BMSET_STRUCT structure.
 */
BMSET_STRUCT *bmset_1tree_alloc(void)
{
  BMSET_STRUCT *node;

  if ((node = malloc(sizeof(BMSET_STRUCT))) == NULL)
    return NULL;
  memset(node, 0, sizeof(BMSET_STRUCT));

  node->type = BMSET_1TREE;

#ifdef STRMAT
  node->ptree = stree_new_tree(128, 0, SORTED_LIST, 0);
#else
  node->ptree = stree_new_tree(128, 0);
#endif
  if (node->ptree == NULL) {
    free(node);
    return NULL;
  }

  return node;
}


/*
 * bmset_1tree_add_string
 *
 * Adds a string to the BMSET_STRUCT structure.
 *
 * NOTE:  The `id' value given must be unique to any of the strings
 *        added to the tree, and must be a small integer greater than
 *        0 (since it is used to index an array holding information
 *        about each of the strings).
 *
 *        The best id's to use are to number the strings from 1 to K.
 *
 *        NOTE:  Parameter `copyflag' is not used here, because the
 *               patterns are always copied as part of the keyword tree.
 *               It's included for compatibility with the other set
 *               matching algorithms.
 *
 * Parameters:   node      -  an BMSET_STRUCT structure
 *               P         -  the sequence
 *               M         -  the sequence length
 *               id        -  the sequence identifier
 *               copyflag  -  make a copy of the sequence?
 *
 * Returns:  non-zero on success, zero on error.
 */
int bmset_1tree_add_string(BMSET_STRUCT *node, char *P, int M, int id)
{
  int i, flag;
  char *Pbar;

  if (node->errorflag)
    return 0;

  if (node->type != BMSET_1TREE) {
    node->errorflag = 1;
    return 0;
  }

  P--;            /* Shift to make sequence be P[1],...,P[M] */

  /*
   * Check for duplicates.
   */
  for (i=0; i < node->num_patterns; i++) {
    if (node->ids[i] == id) {
      fprintf(stderr, "Error in Boyer-Moore Set preprocessing.  "
              "Duplicate identifiers\n");
      node->errorflag = 1;
      return 0;
    }
  }

  /*
   * Allocate space for the new string's information.
   */
  if (node->num_patterns == node->patterns_size) {
    if (node->patterns_size == 0) {
      node->patterns_size = 16;
      node->patterns = malloc(node->patterns_size * sizeof(char *));
      node->lengths = malloc(node->patterns_size * sizeof(int));
      node->ids = malloc(node->patterns_size * sizeof(int));
    }
    else {
      node->patterns_size += node->patterns_size;
      node->patterns = realloc(node->patterns,
                               node->patterns_size * sizeof(char *));
      node->lengths = realloc(node->lengths,
                              node->patterns_size * sizeof(int));
      node->ids = realloc(node->ids, node->patterns_size * sizeof(int));
    }
    if (node->patterns == NULL ||
        node->lengths == NULL || node->ids == NULL) {
      node->errorflag = 1;
      return 0;
    }
  }

  /*
   * Allocate space for and reverse the new sequence
   */
  if ((Pbar = malloc(M + 2)) == NULL) {
    node->errorflag = 1;
    return 0;
  }
  Pbar[0] = Pbar[M+1] = '\0';
  for (i=1; i <= M; i++)
    Pbar[i] = P[M-i+1];

  /*
   * Initialize the appropriate trees and the fields for the new sequence.
   */
#ifdef STRMAT
  flag = stree_ukkonen_add_string(node->ptree, Pbar+1, Pbar+1, M,
                                  node->num_patterns+1);
#else
  flag = stree_add_string(node->ptree, Pbar+1, M,
                          node->num_patterns+1);
#endif
  if (flag <= 0) {
    free(Pbar);
    node->errorflag = 1;
    return 0;
  }

#ifdef STATS
  node->prep_tree_ops += node->ptree->num_compares +
                         node->ptree->creation_cost;
  node->ptree->num_compares = node->ptree->creation_cost = 0;
#endif

  node->patterns[node->num_patterns] = Pbar;
  node->lengths[node->num_patterns] = M;
  node->ids[node->num_patterns] = id;
  node->num_patterns++;
  node->ispreprocessed = 0;

  return 1;
}


/*
 * bmset_1tree_del_string
 *
 * Deletes a string from the set of patterns.
 *
 * NOTE:  This procedure should not be used from inside strmat.
 *
 * Parameters:   node  -  an BMSET_STRUCT structure
 *               P     -  the sequence to be deleted
 *               M     -  its length
 *               id    -  its identifier
 *
 * Returns:  non-zero on success, zero on error.
 */
int bmset_1tree_del_string(BMSET_STRUCT *node, char *P, int M, int id)
{
  int i;

  if (node->errorflag)
    return 0;

  if (node->type != BMSET_1TREE) {
    node->errorflag = 1;
    return 0;
  }

  /*
   * Find the pattern to be deleted.
   */
  for (i=0; i < node->num_patterns; i++)
    if (node->ids[i] == id)
      break;

  if (i == node->num_patterns) {
    fprintf(stderr, "Error in Boyer-Moore Set preprocessing.  String to be "
            "deleted is not in tree.\n");
    node->errorflag = 1;
    return 0;
  }

  /*
   * Remove it from the trees, and then free up the memory.
   */
#ifndef STRMAT
  stree_delete_string(node->ptree, i+1);
#endif

  free(node->patterns[i]);

  /*
   * Shift the other patterns over one.
   */
  for (i++; i < node->num_patterns; i++) {
    node->patterns[i-1] = node->patterns[i];
    node->lengths[i-1] = node->lengths[i];
    node->ids[i-1] = node->ids[i];
  }
  node->ispreprocessed = 0;

  return 1;
}


/*
 * bmset_1tree_prep
 *
 * Preprocesse the suffix tree and compute the bad character table.
 *
 * Parameters:  node  -  an BMSET_STRUCT structure
 *
 * Returns: non-zero on success, zero on error.
 */
int bmset_1tree_prep(BMSET_STRUCT *node)
{
  if (node->errorflag || node->num_patterns == 0)
    return 0;

  if (bmset_2trees_prep(node) == 0)
    return 0;

  /*
   * Compact the suffix tree.
   */
  compact_tree(node, node->ptree, stree_get_root(node->ptree));

  return 1;
}


static int compact_tree(BMSET_STRUCT *bmstruct, SUFFIX_TREE tree,
                        STREE_NODE node)
{
  int flag, leafflag;
  STREE_INTLEAF back, intleaf, nextleaf;
  STREE_NODE child, next;

  /*
   * Take care of STREE_LEAF structured nodes in the tree, keeping
   * the ones whose leaf position equals 0 and removing the others.
   */
  if (int_stree_isaleaf(tree, node)) {
#ifdef STATS
    bmstruct->prep_tree_ops++;
#endif

    if (int_stree_get_leafpos(tree, (STREE_LEAF) node) == 0)
      return 1;
    else {
      int_stree_disc_from_parent(tree, stree_get_parent(tree, node), node);
      int_stree_delete_subtree(tree, node);
      return 0;
    }
  }

  /*
   * For STREE_NODE structured nodes, first cull the leaves, leaving
   * only leaves whose position equals 0.
   *
   * Since leaves are being deleted during the traversal, the next
   * leaf in the list must be found before checking the current leaf.
   */
  leafflag = 0;
  if (int_stree_has_intleaves(tree, node)) {
    back = NULL;
    intleaf = int_stree_get_intleaves(tree, node);

    while (intleaf != NULL) {
      nextleaf = intleaf->next;

      if (intleaf->pos != 0)
        int_stree_remove_intleaf(tree, node, intleaf->strid, intleaf->pos);
      else {
        back = intleaf;
        leafflag = 1;
      }

      intleaf = nextleaf;

#ifdef STATS
      bmstruct->prep_tree_ops++;
#endif
    }
#ifdef STATS
    bmstruct->prep_tree_ops++;
#endif
  }

  /*
   * Compact the subtrees for the children.
   * 
   * Since children may be deleting themselves during the recursive
   * calls, the next child must always be found before the recursion.
   */
  flag = 0;
  child = stree_get_children(tree, node);
  while (child != NULL) {
    next = stree_get_next(tree, child);

    if (compact_tree(bmstruct, tree, child))
      flag++;

#ifdef STATS
    bmstruct->prep_tree_ops++;
#endif

    child = next;
  }
#ifdef STATS
  bmstruct->prep_tree_ops++;
#endif

  /*
   * Determine whether to remove the current node, as well.
   */
  if (flag == 0 && leafflag == 0) {
    int_stree_disc_from_parent(tree, stree_get_parent(tree, node), node);
    int_stree_delete_subtree(tree, node);
    return 0;
  }
  else
    return 1;
}





/*
 * bmset_1tree_search_init
 *
 * Initializes the variables used during an Aho-Corasick search.
 * See bmset_search for an example of how it should be used.
 *
 * Parameters:  node  -  an BMSET_STRUCT structure
 *              T     -  the sequence to be searched
 *              N     -  the length of the sequence
 *
 * Returns:  nothing.
 */
void bmset_1tree_search_init(BMSET_STRUCT *node, char *T, int N)
{
  if (node->errorflag)
    return;
  else if (!node->ispreprocessed) {
    fprintf(stderr, "Error in Boyer-Moore Set search.  The preprocessing "
            "has not been completed.\n");
    return;
  }

  node->T = T - 1;          /* Shift to make sequence be T[1],...,T[N] */
  node->N = N;
  node->k = node->minlen;
  node->h = 0;
  node->dshift = 0;
  node->wnode = NULL;
  node->initflag = 1;
  node->endflag = 0;
}


/*
 * bmset_1tree_search
 *
 * Scans a text to look for the next occurrence of one of the patterns
 * in the text.  An example of how this search should be used is the
 * following:
 *
 *    s = T;
 *    len = N; 
 *    contflag = 0;
 *    bmset_search_init(node, T, N);
 *    while ((s = bmset_search(node, &matchlen, &matchid) != NULL) {
 *      >>> Pattern `matchid' matched from `s' to `s + matchlen - 1'. <<<
 *    }
 *
 * where `node', `T' and `N' are assumed to be initialized appropriately.
 *
 * Parameters:  node           -  a preprocessed BMSET_STRUCT structure
 *              length_out     -  where to store the new match's length
 *              id_out         -  where to store the identifier of the
 *                                pattern that matched
 *
 * Returns:  the left end of the text that matches a pattern, or NULL
 *           if no match occurs.  (It also stores values in `*length_out',
 *           and `*id_out' giving the match's length and pattern identifier.
 */
char *bmset_1tree_search(BMSET_STRUCT *node, int *length_out, int *id_out)
{
  int i, k, h, N, minlen, bshift, gshift, dshift, dval, zshift, id, pos;
  int edgelen;
  char *T, *edgestr, *str;
  STREE_NODE w, wprime;
  SUFFIX_TREE tree;
  BMSET_BADLIST_NODE **R, *Rnode;

  if (node->errorflag)
    return NULL;
  else if (!node->ispreprocessed) {
    fprintf(stderr, "Error in Aho-Corasick search.  The preprocessing "
            "has not been completed.\n");
    return NULL;
  }
  else if (!node->initflag) {
    fprintf(stderr, "Error in Aho-Corasick search.  bmset_search_init was not "
            "called.\n");
    return NULL;
  }
  else if (node->endflag)
    return NULL;

  T = node->T;
  N = node->N;
  R = node->R;
  minlen = node->minlen;
  tree = node->ptree;

#ifdef STATS
  tree->child_cost = 0;
#endif

  /*
   * Execute the Boyer-Moore Set Matching algorithm using only the
   * extended bad character shift rule.  Stop at the first occurrence
   * of a match to one of the patterns.
   */
  k = node->k;
  while (k <= N) {
    /*
     * Perform the test at k (or continue the test if node->w != NULL,
     * which implies that the last search was in the middle of a test when
     * it found a match).
     */
    if (node->wnode == NULL) {
      h = k;
      w = stree_get_root(tree);
      dshift = 0;
    }
    else {
      h = node->h;            node->h = 0;
      w = node->wnode;        node->wnode = NULL;
      dshift = node->dshift;  node->dshift = 0;
    }

    while (h >= 1) {
      /*
       * Find the child of w with label T[h].
       */
      wprime = stree_find_child(tree, w, T[h]);

#ifdef STATS
      node->edge_cost += tree->child_cost;
      tree->child_cost = 0;

      node->num_compares++;
#endif

      /*
       * Stop if no child's character matches T[h] or if the characters
       * on the rest of the tree edge don't match the characters before
       * T[h].
       */
      if (wprime == NULL)
        break;

      w = wprime;
      edgestr = stree_get_edgestr(tree, w);
      edgelen = stree_get_edgelen(tree, w);

#ifdef STATS
      node->num_edges++;
#endif

      pos = 1;
      h--;
      while (h >= 1 && pos < edgelen && edgestr[pos] == T[h]) {
#ifdef STATS
        node->num_compares++;
#endif

        pos++;
        h--;
      }
#ifdef STATS
      node->num_compares++;
#endif

      if (pos < edgelen)
        break;

      /*
       * If the complete edge matches, compute the dshift contribution
       * of the new node (if any), then return a match if there's a leaf
       * occurring at this suffix tree node.  Otherwise, continue the
       * comparison.
       */
      dval = node->d[stree_get_ident(tree, w)];
      if (dval > 0 && (dshift == 0 || dval < dshift))
        dshift = dval;

#ifdef STATS
      node->shift_cost++;
#endif

      if (stree_get_leaf(tree, w, 1, &str, &pos, &id)) {
        node->k = k;
        node->h = h;
        node->wnode = w;
        node->dshift = dshift;

        if (id_out)
          *id_out = node->ids[id-1];
        if (length_out)
          *length_out = node->lengths[id-1];

        return &T[h+1];
      }
    }

    /*
     * Compute the shift distance.  Here, `i' is the number of
     * characters that matched in the test at `k'.
     *
     * If we've matched to the beginning of the text or we've matched more
     * characters than the smallest pattern, no bad character shift is
     * possible and we should just shift by 1.
     *
     * Otherwise, compute the extended bad character shift.  This differs
     * slightly from the book, in that the bad character table `R[ch]'
     * (for a character `ch') contains the distances, from the patterns' 
     * right ends, of every occurrence of character `ch' in the patterns.
     * Since the distance of the mismatch from the patterns' right ends is i,
     * finding the smallest distance in R[T[h]]'s list which is larger
     * than i, and then shifting by the difference between that distance
     * and i, will align that next occurrence of a T[h] character in the
     * shifted patterns with T[h] in the text.
     *
     * One additional note.  Since the patterns cannot be shifted farther
     * than shifting the smallest pattern one character past the mismatch,
     * check the bad shift against shifting the smallest pattern completely
     * past the mismatch and take the smaller of the two shifts.
     */
#ifdef STATS

    i = k - h;
    if (h < 1 || i >= minlen) {
      bshift = 1;
      node->shift_cost++;
    }
    else {
      if (i == 0)
        node->num_init_mismatch++;
      else
        node->total_matches += i;

      Rnode = R[(int) T[h]];
      while (Rnode != NULL && Rnode->position <= i) {
        Rnode = Rnode->next;
        node->shift_cost++;
      }
      node->shift_cost++;

      if (Rnode != NULL && Rnode->position < minlen)
        bshift = Rnode->position - i;
      else
        bshift = minlen - i;

      node->shift_cost++;
    }

#else

    i = k - h;
    if (h < 1 || i >= minlen)
      bshift = 1;
    else {
      Rnode = R[(int) T[h]];
      while (Rnode != NULL && Rnode->position <= i)
        Rnode = Rnode->next;

      if (Rnode != NULL && Rnode->position < minlen)
        bshift = Rnode->position - i;
      else
        bshift = minlen - i;
    }

#endif

    /*
     * Compute the good shift values, and then the true value of the shift.
     */
#ifdef STATS

    zshift = node->z[stree_get_ident(tree, w)];
    node->shift_cost++;

    gshift = node->minlen;
    if (zshift > 1 && zshift - 1 < gshift)
      gshift = zshift - 1;
    if (dshift > 1 && dshift - 1 < gshift)
      gshift = dshift - 1;

    node->shift_cost += 2;

    k += (bshift > gshift ? bshift : gshift);
    node->num_shifts++;

#else

    zshift = node->z[stree_get_ident(tree, w)];

    gshift = node->minlen;
    if (zshift > 1 && zshift - 1 < gshift)
      gshift = zshift - 1;
    if (dshift > 1 && dshift - 1 < gshift)
      gshift = dshift - 1;

    k += (bshift > gshift ? bshift : gshift);

#endif
  }

  node->endflag = 1;
  return NULL;
}


/*
 * bmset_1tree_free
 *
 * Free up the allocated BMSET_STRUCT structure.
 *
 * Parameters:   node  -  a BMSET_STRUCT structure
 *
 * Returns:  nothing.
 */
void bmset_1tree_free(BMSET_STRUCT *node)
{
  bmset_badonly_free(node);
}


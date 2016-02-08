/*
 * The Holly Programming Language
 *
 * Author  - Matthew Levenstein
 * License - MIT
 */
 
/*
 * Code guidelines 
 *
 * All typedefs take the form hl<Type>_t, where <Type> is capitalized
 *
 * All functions (or function-like macros) take the form 
 *      hl_<namespace><function>, where
 *      where namesespace is (as often as possible) a single letter denoting the 
 *      class of function. Example hl_hset() for the 'set' function of the hash table.
 *      No camelcase for functions.
 *
 * All global variables and #defines take the same form as functions 
 *      without the underscores
 * 
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * System wide definitions
 */
 
typedef unsigned char hlByte_t;
typedef unsigned long hlWord_t;

#define HL_WORD_SIZE 8 /* bytes in a word */
#define HL_BYTE_SIZE 8 /* bits in a byte */

/* error codes */
#define HL_MALLOC_FAIL 0x1

typedef struct {
  int error;
} hlState_t;

void* hl_malloc(hlState_t* h, int s ){
  void* buf;
  if( !(buf = malloc(s)) ){
     h->error = HL_MALLOC_FAIL;
  } else {
     memset(buf, 0, s);
  }
  return buf;
}

/*
 * Hash Table
 * Quadratic Probing Hash Table
 */
 
typedef struct {
  int      l; /* key length */
  unsigned h; /* hash value */
  hlByte_t k[HL_WORD_SIZE]; /* key */
  hlWord_t v; /* value */
} hlHashEl_t; 

typedef struct {
  int         s; /* table size */
  int         c; /* table count */
  hlHashEl_t* t;
  hlState_t*  state; /* compiler state */
} hlHashTable_t;


int hlhprimes[] = {
  5, 11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 
  12853, 25717, 51437, 102877, 205759, 411527, 823117, 
  1646237, 3292489, 6584983, 13169977, 26339969, 52679969, 
  105359939, 210719881, 421439783, 842879579, 1685759167
};


/* forward declarations */
void          hl_hw2b( hlByte_t*, hlWord_t, int );
hlWord_t      hl_hb2w( hlByte_t*, int );
hlHashTable_t hl_hinit( hlState_t*  );
hlHashEl_t    hl_hinitnode( hlByte_t*, int, hlWord_t );
unsigned      hl_hsax( hlByte_t *, int );
void          hl_hset( hlHashTable_t*, hlByte_t*, int, hlWord_t );
int           hl_hget( hlHashTable_t*, hlByte_t*, int );
int           hl_hmatch( hlHashEl_t*, hlByte_t*, int );

/* string hash function */
unsigned hl_hsax( hlByte_t* k, int l ){
  unsigned h = 0;
  int i;
  for( i = 0; i < l; i++ )
    h ^= (h << 5) + (h >> 2) + k[i];
  return h;
}

/* stores the data from a word into an array of bytes */
void hl_hw2b( hlByte_t* b, hlWord_t w, int l ){
  int idx, i, s = (HL_BYTE_SIZE * l);
  for( i = HL_BYTE_SIZE; i <= s; i += HL_BYTE_SIZE ){
    idx = (i/HL_WORD_SIZE) - 1;
    b[idx] = (w >> (s - i)) & 0xff;
  }
}

/* converts a stored word back into a word */
hlWord_t hl_hb2w( hlByte_t* b, int l ){
  int idx, i, s = (HL_BYTE_SIZE * l);
  hlWord_t w = 0, n;
  for( i = HL_BYTE_SIZE; i <= s; i += HL_BYTE_SIZE ){
    idx = (i/HL_BYTE_SIZE) - 1;
    n = (hlWord_t)b[idx];
    w |= (n << (s - i));
  }
  return w;
} 

/* create a new hash table */
hlHashTable_t hl_hinit( hlState_t* s ){
  hlHashTable_t h;
  h.c = 0;
  h.s = 0; /* index in the prime table */
  h.t = hl_malloc(s, hlhprimes[0] * sizeof(hlHashEl_t));
  h.state = s;
  return h;
} 

/* create a hash table node */
hlHashEl_t hl_hinitnode( hlByte_t* k, int l, hlWord_t v ){
  hlHashEl_t n;
  if( l < HL_WORD_SIZE ){
    memcpy(n.k, k, l);
  } else {
    hl_hw2b(n.k, (hlWord_t)k, HL_WORD_SIZE);
  }
  n.l = l;
  n.v = v;
  n.h = hl_hsax(k, l);
  return n;
}

/* check if a node matches a key */
int hl_hmatch( hlHashEl_t* n, hlByte_t* k, int l ){
  hlByte_t* c;
  hlWord_t w;
  if( n->l != l ) return 0;
  if( n->l < HL_WORD_SIZE ){
    c = n->k;
  } else {
    w = hl_hb2w(n->k, HL_WORD_SIZE);
    c = (hlByte_t *)w;
  }
  return !(memcmp(c, k, l));
}

/* resize a hash table either up or down */
void hl_hresize( hlHashTable_t* h, int dir ){
  hlHashEl_t* t = h->t;
  int i = 0, s = hlhprimes[h->s];
  int ns, slot, j, idx;
  h->s += dir;
  ns = hlhprimes[h->s];
  h->t = hl_malloc(h->state, hlhprimes[h->s] * sizeof(hlHashEl_t));
  for( ; i < s; i++ ){ /* for each node in the old array */
    if( !t[i].l ) continue;
    slot = t[i].h % ns;
    for( j = 0; j < ns; j++ ){ /* insert into the new array */
      idx = (slot + j * j) % ns; /* probe for new slot */
      if( h->t[idx].l ) continue;
      h->t[idx] = t[i];
      break;
    }
  }
  free(t);
}

/* add entry to the hash table */
void hl_hset( hlHashTable_t* h, hlByte_t* k, int l, hlWord_t v ){
  int i, s = hlhprimes[h->s], idx;
  hlHashEl_t n = hl_hinitnode(k, l, v);
  unsigned slot = n.h % s;
  if( !h->t[slot].l ){
    h->t[slot] = n;
  } else {
    for( i = 0; i < s; i++ ){
      idx = (slot + i * i) % s;
      if( h->t[idx].l )
        continue;
      h->t[idx] = n;
      break;
    }
  }
  h->c++;
  if( h->c >= (s/2) ){
    hl_hresize(h, 1);
  }
}

/* find the slot of the node */
int hl_hget( hlHashTable_t* h, hlByte_t* k, int l ){
  int idx, i, s = hlhprimes[h->s];
  unsigned slot = hl_hsax(k, l) % s;
  if( hl_hmatch(&(h->t[slot]), k, l) )
    return slot;
  for( i = 0; i < s; i++ ){
    idx = (slot + i * i) % s;
    if( hl_hmatch(&(h->t[idx]), k, l) )
      return idx;
  }
  return -1;
}

/*
 * end hash table
 */ 

/*
 * Types
 */

typedef hlByte_t* hlString_t;
typedef double    hlNum_t;
typedef hlByte_t  hlBool_t;

/*
 * NaN-Boxed Values
 */
 
#define hlnan  0x7ff8000000000000
#define hlnnan 0xfff8000000000000
#define hlinf  0x7ff0000000000000
#define hlninf 0xfff0000000000000
 
/*
 * end values
 */
 
/*
 * a bunch of tests for data structures 
 */
 
#include <math.h>
 
int isprime( unsigned n ){
  int i, r;
  if(  n < 2   ) return 0;
  if(  n < 4   ) return 1;
  if( !(n % 2) ) return 0;
  r = sqrt(n);
  for( i = 3; i <= r; i += 2 ){
    if( !(n % i ) ) return 0;
  }
  return 1;
} 

void primelist( void ){
  unsigned s = 2;
  unsigned ptest;
  unsigned prev;
  unsigned p = 2;
  int i = 0;
  
  printf("\nunsigned hlhprimes[] = {");
  for( ; i < 29; i++ ){
    ptest = s + 1;
    prev = p;
    for( ;; ){
      p = ptest += 2;
      if( p < (prev << 1) ) continue; 
      if( isprime(p) )
        break;
    }
    printf("%d", p);
    if( i < 28 ) printf(", ");
    s <<= 1;
  }
  printf("};\n\n");
}

#include <time.h>

/* get word from file */
hlByte_t* getWord (FILE* f) {
  hlByte_t* buf = NULL;
  int c = 0, i = 0, bufsize = 10;
  buf = malloc(bufsize + 1);
  memset(buf, 0, bufsize + 1);
  while ((c = fgetc(f)) != '\n') {
    if (c == EOF) return NULL;
    if (i == bufsize)
      buf = realloc(buf, (bufsize += 10) + 1);
    buf[i++] = (hlByte_t)c;
  }
  buf[i] = 0;
  return buf;
} 

void loadWords (hlHashTable_t* root) {
  FILE* in = fopen("../cuckoo/words.txt", "r");
  hlByte_t* word = NULL;
  int i = 0, total = 0;
  while ((word = getWord(in))) {
    int l = strlen((char *)word);
    hl_hset(root, word, l, (hlWord_t)word);
  }
  fclose(in);
  
  puts("inserted");
  getchar();
  
  in = fopen("../cuckoo/words.txt", "r");
  while ((word = getWord(in))) {
    int l = strlen((char *)word);
    i = hl_hget(root, word, l);
    if( i > -1 ){
      total++;
    } else {
      printf("Missing: %s\n", (char *)word);
    }
  }
  printf("Total: %d\n", total);
}

void hashtest( void ){
  hlState_t state;
  double end, start;
  hlHashTable_t table = hl_hinit(&state);

  start = (float)clock()/CLOCKS_PER_SEC;
  hl_hset(&table, (hlByte_t *)"matthew", 7, (hlWord_t)"levenstein");
  loadWords(&table);
  hl_hset(&table, (hlByte_t *)"matthew", 7, (hlWord_t)"barfenbutt");
  end = (float)clock()/CLOCKS_PER_SEC;

  printf("%s\n", (char *)(table.t[hl_hget(&table, (hlByte_t *)"matthew", 7)].v));
  printf("Inserted %d objects in %fs - Table size: %d\n", table.c, end - start, hlhprimes[table.s]);
}

int main(void) {
  return 0;
}

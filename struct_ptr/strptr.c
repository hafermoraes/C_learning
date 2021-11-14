#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct policy
{
  char *id;
  char *date_of_birth;
  char *gender;
  char *issue_date;
  char *status_code;
  char *status_date;
} policy;

policy *p = NULL;

// prototype of functions
void muda(policy **);
void calloc_null(policy **);

int main(void) {

  //printf( "p->id (endereço antes de chamar muda()): %p\n", p->id);

  muda(&p);
  printf( "p->id: %s\n", p->id);

  calloc_null(&p);
  printf( "p->id: %s\n", p->id);

  free(p->id);
  free(p);
  
  
  return 0;
}

// function declarations
void muda(policy **p){

  *p = NULL;
  *p = (policy *) malloc( sizeof(policy) );
  
  (*p)->id = (char *) calloc( 10, sizeof(char) );
  printf( "p->id (endereço criado de dentro da muda(): %p.\n", (*p)->id);

  strcpy( (*p)->id , "teste.\n" );
  printf( "p->id (chamado de dentro da muda(): %s.\n", (*p)->id);

  (*p)->id = (char *) realloc( (*p)->id, 100*sizeof(char) );
  strcpy((*p)->id, "mudou via muda()");
  printf( "p->id (chamado de dentro da muda(): %s.\n", (*p)->id);
  printf( "p->id (chamado de dentro da muda(): %p.\n", (*p)->id);
}

void calloc_null(policy **p){

  free((*p)->id);
  (*p)->id = (char *) calloc( 3, sizeof(char) );
  printf( "p->id (calloc reduzido a 3 posições de dentro da calloc_null(): %p.\n", (*p)->id);

  strcpy( (*p)->id , "123" );
  printf( "p->id (chamado de dentro da calloc_null(): %s.\n", (*p)->id);

}

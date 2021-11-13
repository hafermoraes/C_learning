#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct policy
{
  char id[50];
  char date_of_birth[50];
  char gender[50];
  char issue_date[50];
  char status_code[50];
  char status_date[50];
} policy;

policy p;

void muda();
void suja();
void limpa();
void ind(char*);

int main(void) {


  printf("sizeof(p.id) = %lu\n",sizeof(p.id) );

  //printf("p.id = %5s | p.date_of_birth = %10s\n",p.id, p.date_of_birth);
  muda();
  //printf("p.id = %5s | p.date_of_birth = %10s\n",p.id, p.date_of_birth);
  limpa();
  suja();
  limpa();
  
  return 0;
}

// function declarations
void muda(){
  strcpy( p.id, "123");
  strcpy( p.date_of_birth, "17.11.1982");
}

void suja(){
  strcpy( p.id, "123" );
  strcpy( p.date_of_birth, "456" );
  strcpy( p.gender, "789" );
  strcpy( p.issue_date, "101112" );
  strcpy( p.status_code, "131415" );
  strcpy( p.status_date, "161718" );
}

void limpa(){
  ind(p.id);
}

void ind(char *s){
  // memset( s, 0, sizeof(s) );
  strncpy( s, "", sizeof(s) );
}

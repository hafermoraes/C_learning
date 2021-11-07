#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <assert.h>

/*   run as 
	 echo 'teste de stdin stdout' | ./stdio 
*/

#define BUFSIZE 1024

int main(int argc, char **argv) {
  
  char a[BUFSIZE];
  
  // leitura do conteúdo de stdin
  fgets( a, BUFSIZE, stdin );

  // tratamento da string
  snprintf(a, BUFSIZE, "%lu", strlen(a) ); 

  // escrita do resultado em stdout
  puts( a );
  
  return EXIT_SUCCESS;
}


/* https://stackoverflow.com/a/35695762 */

/*   run as 

	 printf 'dob;policy_issue_date;policy_status_code;policy_status_date\n' > linhas.txt
	 printf '1982-11-17;2010-01-01;1;\n' >> linhas.txt 
	 printf '1977-06-23;2012-03-04;3;2015-09-17\n' >> linhas.txt 
	 cat linhas.txt | ./getline
*/

#include <stdio.h>
#include <stdlib.h>

int main(void) {

  /* ponteiro para sequência de strings por linha */
  char *line = NULL;
  size_t len = 0;
  ssize_t read = 0;

  while (1) {
	read = getline(&line, &len, stdin);

	if (read == -1)
	  break;

	printf("line = %s", line);
	printf("line length = %zu\n", read);
	puts("");
  }
  /* libera a memória alocada para variável line */
  free(line);

  return EXIT_SUCCESS;
}

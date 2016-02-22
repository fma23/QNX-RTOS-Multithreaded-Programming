#include <stdlib.h>
#include <stdio.h>
#include "pi.h"

int my_putc(int c) {
	return putchar(c);
}

int main(int argc, char *argv[]) 
{
/*	
	char *piArgv[2];
	piArgv[1]="100";
	myPi(2,piArgv);
*/
	myPi(argc,argv);
	return EXIT_SUCCESS;
}

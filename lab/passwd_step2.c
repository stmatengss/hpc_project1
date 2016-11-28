// CONFIGURATION
#define ITER 200000000                     // max. number of iterations
#define MAX_CRYPT_LEN	200				// max. length of the cyphertext

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
//#include <memory.h>
//#include <malloc.h>

/*----------------------------------------------------------------------------
 *
 *				Procedure of decrypting the cyphertext with the cracked password
 *              ----------------------------------------------------------------
 *	ARG:
 *		pswd		:	password
 *		crypteddata	:	cyphertext
 *
 *	RET:
 *		*crypreddata:	decrypted cyphertext
 *
 *	NOTE:
 *		none
-----------------------------------------------------------------------------*/
int DeCrypt(char *pswd, char *crypteddata)
{
	unsigned int p = 0;	// pointer to the current position of the data being decrypted

	// * * * MAIN DECRYPTION LOOP * * *
	do {
		// decrypting the current character
		crypteddata[p] ^= pswd[p % strlen(pswd)];

		// proceeding with decryption of the next character
	} while(++p < strlen(crypteddata));

	return 0;
}


/*----------------------------------------------------------------------------
 *
 *				Procedure of calculating the password checksum
 *				---------------------------------------------
 *	ARG:
 *		pswd		:	password
 *
 *	RET:			CRC of this password
 *
 *	NOTE:
 *		none
-----------------------------------------------------------------------------*/
int CalculateCRC(char *pswd)
{
	unsigned int a;
	int x = -1;			// CRC calculation error
	char *ptr = pswd;
	while (*ptr != '\0') {
		int temp = *(int *)(ptr);
		x += temp;
		++ptr;
	}
	return x;
}

/*----------------------------------------------------------------------------
 *
 *				Procedure for checking the password checksum
 *				---------------------------------------------
 *	ARG:
 *		pswd		:	password
 *		validCRC	:	valid CRC
 *
 *	RET:			0	password is wrong
 *					!=0	validCRC
 *
 *	NOTE:
 *		none
---------------------------------------------------------------------------*/
int CheckCRC(char *pswd, int validCRC)
{
	if (CalculateCRC(pswd) == validCRC)
		return validCRC;

	return 0;
}


/*----------------------------------------------------------------------------
 *
 *				Procedure for processing the current password
 *				---------------------------------------------
 *	ARG:
 *		crypteddata	:	encrypted data to be decrypted
 *		pswd		:	test password
 *		validCRC	:	valid CRC
 *		progress	:	percentage of passwords already tested
 *
 *	RET:
 *		none
 *
 *	NOTE:
 *      The do_pswd function checks the CRC of the password being tested, and,
 *      if it is valid, attempts to decrypt the cyphertext using this password
 *      and outputs the result of decryption on the screen. The function also
 *      displays the percentage of rejected passwords and the password
 *      currently being tested.
-----------------------------------------------------------------------------*/
int do_pswd(char *crypteddata, char *pswd, int validCRC, int progress)
{
	char *buff;

	// displaying the current status

        static int x=0;
        if (++x>6666)
	{
	    printf("Current pswd : %10s [%d%%]\r",&pswd[0],progress);
	    x=0;
	}

	// checking the password CRC
	if (CheckCRC(pswd, validCRC))
	{								// <- CRC match

		// copying the crypted data into temporary buffer
		buff = (char *) malloc(strlen(crypteddata));
		strcpy(buff, crypteddata);

		// decrypting
		DeCrypt(pswd, buff);

		// displaying the decryption result
		printf("CRC %8X: try to decrypt: \"%s\"\n",CheckCRC(pswd, validCRC),buff);
	}

	return 0;
}


/*----------------------------------------------------------------------------
 *
 *						Password-cracking procedure
 *						---------------------------
 *	ARG:
 *		crypteddata	:	encrypted data that need to be decrypted
 *		pswd		:	starting password, from which cracking begins
 *		max_iter	:	max. number of generated passwords
 *		validCRC	:	valid CRC
 *
 *	RET:
 *		none
 *
 *	NOTE:
 *		The do_pswd function checks the CRC of the password currently
 *      being tested, and, if it is valid, attempts to decrypt
 *      the cyphertext using this password and then display the
 *      decryption results. The function also displays the percentage
 * 		of incorrect passwords and the one currently being checked.
-----------------------------------------------------------------------------*/
int gen_pswd(char *crypteddata, char *pswd, int max_iter, int validCRC)
{
	int a;
	int p = 0;

	// generate passwords
	for(a = 0; a < max_iter; a++)
	{
		// process the current password
		do_pswd(crypteddata, pswd, validCRC, 100*a/max_iter);

		// * main password-generation loop *
		// according to the "latch" or "counter" algorithm
		while((++pswd[p])>'z')
		{	/* ^^^^ -	increasing the first character to the right by one */
			/*			if it exceeds'z' - enter the loop */
			/*			this loop is needed to react */
			/*			to the fact that the next character exceeds 'z'.  */

			// The character that exceeds 'z' must be reset to initial state
			pswd[p] = '!';

			// next character
			p++; 
			if (!pswd[p])
			{					// <--	special handling for the next character
								//		if it is equal to zero, i.e., the string end is reached
								//		we extend the string

			pswd[p]=' ';		//		attention! initialization by MIN_CHAR-1
								//		since it is increased in the while loop!

			pswd[p+1]=0;		//		new sting end!
			}
		} // end while(pswd)

		// return the pointer to initial position
		p = 0;
	} // end for(a)

	return 0;
}

/*----------------------------------------------------------------------------
 *
 *				This function displays the number using dot as separator
 *				------------------------------------------------
 *	ARG:
 *		per			:	number for output
 *
 *	RET:
 *		none
 *
 *	NOTE:
 *		The function displays the number truncated to is integer part
 *
-----------------------------------------------------------------------------*/
int print_dot(float per)
{
	// * * * CONFIGURATION * * *
	#define N			3		// separate by three positions
								// when displaying HEX separate by two positions

	#define DOT_SIZE	1		// size of the separating dot

	#define	DOT			"."		// separator

	int		a;
	char	buff[666];

	sprintf(buff,"%0.0f", per);
	/* ^^^^^^^^^^^^^^^^ output format */

	// * * * loop for parsing the number by digits * * *
	for(a = strlen(buff) - N; a > 0; a -= N)		// <-- displacing
	{ /* ^^^^^^^^^^^^^^^^ - this is a silly code, - do not call this function frequently */

			memmove(buff + a + DOT_SIZE, buff + a, 66);
			/* attention!						^^^^^^^^^ */

			if(buff[a]==' ') break;	// blank character encountered - end of work
				else
			// copying the separator
			memcpy(buff + a, DOT, DOT_SIZE);
	}
	// displaying on screen
	printf("%s\n",buff);

	return 0;
}


int main(int argc, char **argv)
{
	// variables
    FILE *f;                // for reading the source file (if there is any)
    char *buff;             // for reading the data from the source file
    char *pswd;             // password currently being tested (needed by gen_pswd)
	int validCRC;			// for storing original password CRC
	unsigned int t;			// for measuring execution time for cracking
	int iter = ITER;		// max. number of passwrods to test
	char *crypteddata;		// for storing encrypted text

	//	built-in default crypt
	//	The one who reads what is encrypted here will know the great secret
	//	Kris Kaspersky ;)
	char _DATA_[] = "\x4B\x72\x69\x73\x20\x4B\x61\x73\x70\x65\x72\x73\x6B"\
					"\x79\x20\x44\x65\x6D\x6F\x20\x43\x72\x79\x70\x74\x3A"\
					"\xB9\x50\xE7\x73\x20\x39\x3D\x30\x4B\x42\x53\x3E\x22"\
					"\x27\x32\x53\x56\x49\x3F\x3C\x3D\x2C\x73\x73\x0D\x0A";

	// TITLE
	printf("= = = VTune profiling demo = = =\n==================================\n");

	// HELP
	if (argc==2)
	{
			printf("USAGE:\n\tpswd.exe [StartPassword MAX_ITER]\n");
			return 0;
	}

	// allocating memory
	printf("memory malloc\t\t");
	buff = (char *) malloc(MAX_CRYPT_LEN);
	if (buff) printf("+OK\n"); else {printf("-ERR\n"); return -1;}

	// getting cyphpertext for decryption
	printf("get source from\t\t");
	if ((f=fopen("crypted.dat","r"))!=0)
	{
		printf("crypted.dat\n");
		fgets(buff,MAX_CRYPT_LEN, f);
	}
	else
	{
        printf("built-in data\n");
		buff=_DATA_;
	}

	// calculating CRC
	validCRC=*(int *)(strstr(buff,":")+1);
	printf("calculate CRC\t\t%X\n",validCRC);
	if (!validCRC)
	{
		printf("-ERR: CRC is invalid\n");
		return -1;
	}

	// separating encrypted data
	crypteddata=strstr(buff,":") + 5;
	//printf("cryptodata\t\t%s\n",crypteddata);

	// allocating memory for password buffer
	printf("memory malloc\t\t");
	pswd = (char *) malloc(512*1024);

	memset(pswd,0,666);		// <-- initalization

	if (pswd) printf("+OK\n"); else {printf("-ERR\n"); return -1;}

	// parsing the command line arguments
	// getting the initial password and max. number of iterations
	printf("get arg from\t\t");
	if (argc>2)
	{
		printf("command line\n");
		if(atol(argv[2])>0) iter=atol(argv[2]);
		strcpy(pswd,argv[1]);
	}
		else
	{
        printf("built-in default\n");
		strcpy(pswd,"!");
	}
	printf("start password\t\t%s\nmax iter\t\t%d\n",pswd,iter);


	// starting password cracking
	printf("==================================\ntry search... wait!\n");
	t=clock();
		gen_pswd(crypteddata,pswd,iter,validCRC);
	t=clock()-t;

	// displaying the number of passwords tested per second
	printf("                                       \rPassword per sec:\t");
	print_dot(iter/(float)t*CLOCKS_PER_SEC);

	return 0;
}


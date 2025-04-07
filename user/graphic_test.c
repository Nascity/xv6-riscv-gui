#include "kernel/types.h"
#include "user/user.h"

char* nascity[5] = {
	"O   O  OOOO OOOOO",
	"OO  O O       O",
	"O O O  OOO    O",
	"O  OO     O   O",
	"O   O OOOO    O"
};

char* hlwd[3] = {
	"O O O   O O O OO",
	"OOO O   O O O O O",
	"O O OOO  O O  OO"
};

int
main(void)
{
	int i, j;

	for (i = 0; i < 3; i++)
		for (j = 0; hlwd[i][j]; j++)
			if (hlwd[i][j] == 'O')
				draw_fill(j * 25 + 200, i * 25 + 475, 25, 25, 0xFF55FF);

	for (i = 0; i < 5; i++)
		for (j = 0; nascity[i][j]; j++)
			if (nascity[i][j] == 'O')
				draw_fill(j * 50 + 200, i * 50 + 200, 50, 50, 0x55FFFF);

	exit(0);
}

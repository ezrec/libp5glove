#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <p5glove.h>

/* Brain-Dead P5 data dump.
 */
int main(int argc, char **argv)
{
	P5Glove glove;
	struct p5glove_data info;
	int sample;

	glove=p5glove_open();
	if (glove == NULL) {
		fprintf(stderr, "%s: Can't open glove interface\n", argv[0]);
		return 1;
	}

	printf("Dumping the next 100 samples...\n");
	memset(&info,0,sizeof(info));
	for (sample=0; sample < 100; ) {
		int i,err;

		err=p5glove_sample(glove,&info);
		if (err < 0 && errno == EAGAIN)
			continue;

		printf("%2d: ",sample);
		sample++;
		if (err < 0) {
			perror("Glove Failure");
			exit(1);
		}

		/* Buttons */
		printf("%c%c%c ",
			(info.buttons & P5GLOVE_BUTTON_A) ? 'A' : '.',
			(info.buttons & P5GLOVE_BUTTON_B) ? 'B' : '.',
			(info.buttons & P5GLOVE_BUTTON_C) ? 'C' : '.');

		/* Fingers */
		printf("%2d,%2d.%2d.%2d.%2d ",
			info.finger[P5GLOVE_THUMB],
			info.finger[P5GLOVE_INDEX],
			info.finger[P5GLOVE_MIDDLE],
			info.finger[P5GLOVE_RING],
			info.finger[P5GLOVE_PINKY]);

		/* Visible IRs */
		for (i = 0; i < 8; i++) {
			if (! info.ir[i].visible)
				continue;

			printf("%d:(%4d,%4d,%4d) ",i,
					info.ir[i].x,info.ir[i].y,info.ir[i].z);
		}

		printf("\n");
	}

	p5glove_close(glove);
}

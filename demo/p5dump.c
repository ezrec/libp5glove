#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h> /* exit */
#include <string.h> /* memset */

#include <p5glove.h>

static void dump_cooked(P5Glove glove,struct p5glove_data *info)
{
	printf("[%.4f, %.4f, %.4f] ",
		info->position[0],
		info->position[1],
		info->position[2]);

	printf("(%.4f, %.4f, %.4f, %.4f deg) ",
		info->rotation.axis[0],
		info->rotation.axis[1],
		info->rotation.axis[2],
		info->rotation.angle);
}

static void dump_raw(P5Glove glove,struct p5glove_data *info)
{
	int i;

	/* Visible IRs */
	for (i = 0; i < 8; i++) {
		if (! info->ir[i].visible)
			continue;

		printf("%d:(%4d,%4d,%4d) ",i,
				info->ir[i].h,info->ir[i].v1,info->ir[i].v2);
	}

}

/* Brain-Dead P5 data dump.
 */
int main(int argc, char **argv)
{
	P5Glove glove;
	struct p5glove_data info;
	int sample,cooked=0;

	glove=p5glove_open();
	if (glove == NULL) {
		fprintf(stderr, "%s: Can't open glove interface\n", argv[0]);
		return 1;
	}

	if (argc>1 && !strcmp(argv[1],"-c"))
		cooked=1;

	printf("Dumping the next 100 samples...\n");
	memset(&info,0,sizeof(info));
	for (sample=0; ; ) {
		int i,err;

		err=p5glove_sample(glove,&info);
		if (err < 0 && errno == EAGAIN)
			continue;
		if (err < 0) {
			perror("Glove Failure");
			exit(1);
		}

		printf("%2d: ",sample);
		sample++;

		/* Buttons */
		printf("%c%c%c ",
			(info.buttons & P5GLOVE_BUTTON_A) ? 'A' : '.',
			(info.buttons & P5GLOVE_BUTTON_B) ? 'B' : '.',
			(info.buttons & P5GLOVE_BUTTON_C) ? 'C' : '.');

		/* Fingers */
		printf("%2d,%2d,%2d,%2d,%2d ",
			info.finger[P5GLOVE_THUMB],
			info.finger[P5GLOVE_INDEX],
			info.finger[P5GLOVE_MIDDLE],
			info.finger[P5GLOVE_RING],
			info.finger[P5GLOVE_PINKY]);

		if (cooked) {
			p5glove_process_sample(glove,&info);
			dump_cooked(glove,&info);
		} else {
			dump_raw(glove,&info);
		}

		printf("\n");
	}

	p5glove_close(glove);

    return 0;
}

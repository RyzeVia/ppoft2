/*
 * testappli.c
 *
 *  Created on: 2012/07/08
 *      Author: RyzeVia
 */
#define DEBUG_ENABLE
#define DEBUG_LEVEL DEBUG_LEVEL_MUST
#define GLOBAL_DEFINITION
#include <math.h>
#include "libxpt_cp.h"
#include "mpi.h"

#define BW	200

XPT_info_t xptinfo;
struct timeval now;
int shwid;
int size;
int particle;
int effect;
char fname[64];

/* tentative */
int grank;

typedef struct particle_ {
	double x;
	double y;
	double vx;
	double vy;
} particle_t;

typedef struct direction_ {
	int up;
	int down;
	int left;
	int right;
} direction_t;

int ckpt_candidate(XPT_info_t *sh, int rank, int iter, int phase);


//direction_t dir;
particle_t *sdata[4], *rdata[4];
particle_t *data;
MPI_Request srequest[4], rrequest[4];

int main(int argc, char** argv) {
	int i, t, j;
	int rank;
	int nshwid;
	int neffect;
	int steps;
	int dir[4];
	particle_t dummy1, dummy2;
	double dummy_accelx, dummy_accely;
	int prob;
	int ret;

	if (argc < 5) {
		ERRORF(
				"usage: testappli size(np = size^2) shared_width(parcentage) nparticle effect(parcentage) steps\n"
				"	\t shared_width: sode, effect: how many particles is effected to one particle\n");
		exit(EXIT_FAILURE);
	}

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &prob);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	sprintf(fname, "ckpt.%d", rank);

	DEBUGF(100, "STAMP1");

	grank = rank;

	size = atoi(argv[1]);
	shwid = atoi(argv[2]);
	particle = atoi(argv[3]);
	effect = atoi(argv[4]);
	steps = atoi(argv[5]);

	nshwid = (int) ((particle / 100) * shwid);
	neffect = (int) ((particle / 100) * effect);

	/*
	 dir.up = (rank - size) % (size*size);
	 dir.down = (rank + size) % (size*size);
	 dir.left = (rank - 1) % size + ((int)(rank/size) * size);
	 dir.right = (rank + 1) % size + ((int)(rank/size) * size);
	 */
	dir[0] = (rank - size + size*size) % (size * size);
	dir[1] = (rank + size + size*size) % (size * size);
	dir[2] = (rank - 1 + size) % size + ((int) (rank / size) * size);
	dir[3] = (rank + 1 + size) % size + ((int) (rank / size) * size);

	printf("%d, %d, %d, %d, %d\n", rank, dir[0], dir[1], dir[2], dir[3]);

	DEBUGF(100, "STAMP2");

	dummy1.x = (double) rand();
	dummy1.y = (double) rand();
	dummy1.vx = (double) rand();
	dummy1.vy = (double) rand();
	dummy2.x = (double) rand();
	dummy2.y = (double) rand();
	dummy2.vx = (double) rand();
	dummy2.vy = (double) rand();

	CALLOC(particle_t *, data, particle, sizeof(particle_t));
	for (i = 0; i < 4; i++) {
		CALLOC(particle_t *, (sdata[i]), nshwid, sizeof(particle_t));
		CALLOC(particle_t *, (rdata[i]), nshwid, sizeof(particle_t));
	}

	XPT_init(&xptinfo, NULL);

	DEBUGF(200, "STAMP3");


	XPT_CP_cli_init(&xptinfo);

	if(rank == 0){
		XPT_CP_publish_to_srv(&xptinfo, "FI_TIMESTAMP", "STREAM=0,MSG=PROGRAM_START");
//		XPTI_publish(&(xptinfo.iinfo), "FI_TIMESTAMP", 0,
//				"STREAM=0 MSG=PROGRAM START");
	}


	if(rank == 0){
		XPT_CP_ready_root(&xptinfo, 0.5, 4);
	}else{
		XPT_CP_ready_nodes(&xptinfo, 0.5, 4);
	}

	XPT_TRY(&xptinfo){
	for (t = 0; t < steps; t++) {
/*
		if(rank == 0){
			ret = XPT_CP_publish_to_srv(&xptinfo, "FI_TIMESTAMP", "STREAM=0,MSG=PROGRAM_TICK");
//			XPTI_publish(&(xptinfo.iinfo), "FI_TIMESTAMP", 0
//					"STREAM=0 MSG=PROGRAM TICK");
		}
*/
		XPT_poll_push_event(&xptinfo);

		for (i = 0; i < 4; i++) {
			MPI_Isend(sdata[i], nshwid, MPI_DOUBLE, dir[i], 0, MPI_COMM_WORLD,
					&(srequest[i]));
			MPI_Irecv(rdata[i], nshwid, MPI_DOUBLE, dir[i], 0, MPI_COMM_WORLD,
					&(rrequest[i]));
		}

		XPT_poll_push_event(&xptinfo);

		/* accel */
		for (j = 0; j < particle; j++){
			for (i = 0; i < neffect; i++) {
			double norm = sqrt((dummy1.x - dummy2.x) * (dummy1.x - dummy2.x) + (dummy1.y - dummy2.y) * (dummy1.y - dummy2.y));
			dummy_accelx += norm*dummy1.x;
			dummy_accely += norm*dummy1.y;
			}
		}

l0:		//CKPT
		XPT_poll_push_event(&xptinfo);
		ckpt_candidate(&xptinfo, rank, t, 0);

		/* velocity and position */
		for (i = 0; i < particle; i++) {
			dummy1.vx += dummy_accelx;
			dummy1.vy += dummy_accely;
			dummy1.x += dummy1.vx;
			dummy1.y += dummy1.vy;
		}

l1:		//CKPT
		XPT_poll_push_event(&xptinfo);
		ckpt_candidate(&xptinfo, rank, t, 1);

		for (i = 0; i < 4; i++) {
			MPI_Waitall(4, srequest, MPI_STATUSES_IGNORE);
			MPI_Waitall(4, rrequest, MPI_STATUSES_IGNORE);
		}

		XPT_poll_push_event(&xptinfo);

		/* sode */
		/* accel */
		for (j = 0; j < shwid; j++){
			for (i = 0; i < neffect; i++) {
			double norm = sqrt((dummy1.x - dummy2.x) * (dummy1.x - dummy2.x) + (dummy1.y - dummy2.y) * (dummy1.y - dummy2.y));
			dummy_accelx += norm*dummy1.x;
			dummy_accely += norm*dummy1.y;
			}
		}

l2:		//CKPT
		XPT_poll_push_event(&xptinfo);
		ckpt_candidate(&xptinfo, rank, t, 2);

		/* velocity and position */
		for (i = 0; i < shwid; i++) {
			dummy1.vx += dummy_accelx;
			dummy1.vy += dummy_accely;
			dummy1.x += dummy1.vx;
			dummy1.y += dummy1.vy;
		}

l3:		//CKPT
		XPT_poll_push_event(&xptinfo);
		ckpt_candidate(&xptinfo, rank, t, 3);
	}
	}XPT_CATCH(&xptinfo, "FI_RESTART"){
		int phase, iter;
		char fn[64];
		char header[32];
		FILE *fp;

		sscanf(xptinfo.pushed_payload, "%d", &phase);
		sprintf(fn, "%s.%d", fname, phase);

		fp = fopen(fn, "r");
		fread(header, sizeof(char), 32, fp);
		sscanf(header, "%d,%d", &phase, &iter);
		t = iter;
//		fread(data, sizeof(particle_t), particle, fp);
		fclose(fp);

		XPT_CATCHED(&xptinfo);
		switch(phase){
		case 0:
			goto l0;
		case 1:
			goto l1;
		case 2:
			goto l2;
		case 3:
			goto l3;
		default:
			break;
		}

	}
	if(rank == 0){
		XPT_CP_publish_to_srv(&xptinfo, "FI_TIMESTAMP", "STREAM=0,MSG=PROGRAM_FINISH");

//		XPTI_publish(&(xptinfo.iinfo), "FI_TIMESTAMP", 0,
//				"STREAM=0 MSG=PROGRAM FINISH");
	}
	MPI_Finalize();

	return EXIT_SUCCESS;
}

void ckptwrite(FILE *fp, int phase){
	FILE* sce;
	double wtime;


	if(grank == 0){
		sce = fopen("scephase", "w");
		fwrite(&phase, sizeof(int), 1, sce);
		fclose(sce);
	}

//	fwrite(data, sizeof(particle_t), particle, fp);
	wtime = particle * sizeof(particle_t) / (BW * 1000. * 1000.);
//	fprintf(stdout, "wait %f sec\n", wtime);

	struct timespec ts, rts;
	misc_dbl2ts(wtime, &ts);

remain:
	if(nanosleep(&ts, &rts) == -1){
		ts = rts;
		goto remain;
	}



}

int ckpt_candidate(XPT_info_t *sh, int rank, int iter, int phase){
//	static double cur = 0.;
//	struct timeval tv;
//	FILE* fp;
//	char header[32];
	char fn[64];
	int rv;

//	fprintf(stderr, "candidate\n");

	if(rank == 0){
		XPT_CP_preexec(sh, iter, phase);
	}

	sprintf(fn, "%s.%d", fname, phase);

	rv = XPT_CP_ckpt(sh, fn, iter, phase, ckptwrite);

	if(rank == 0)
		XPT_CP_postexec(&xptinfo, iter, phase);

	XPT_CP_pull_poll(sh, phase, iter);

	return XPT_TRUE;
}



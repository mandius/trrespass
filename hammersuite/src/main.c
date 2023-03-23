#include "stdio.h"
// #include <x86intrin.h> /* for rdtsc, rdtscp, clflush */
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>

#include "include/utils.h"
#include "include/types.h"
#include "include/allocator.h"
#include "include/memory.h"
#include "include/dram-address.h"
#include "include/hammer-suite.h"
#include "include/params.h"

ProfileParams *p;

// DRAMLayout     g_mem_layout = {{{0x4080,0x88000,0x110000,0x220000,0x440000,0x4b300}, 6}, 0xffff80000, ((1<<13)-1)};
// DRAMLayout 			g_mem_layout = { {{0x2040, 0x44000, 0x88000, 0x110000, 0x220000}, 5}, 0xffffc0000, ((1 << 13) - 1) };
// DRAMLayout 			g_mem_layout = {{{0x2040,0x24000,0x48000,0x90000},4}, 0xffffe0000, ((1<<13)-1)};

DRAMLayout   g_mem_layout = {{{0x12000,0x24000,0x48000}, 3}, 0xffff0000, 8};
void read_config(SessionConfig * cfg, char *f_name)
{
	FILE *fp = fopen(f_name, "rb");
	int p_size;
	size_t res;
	assert(fp != NULL);
	res = fread(cfg, sizeof(SessionConfig), 1, fp);
	assert(res == 1);
	fclose(fp);
	return;
}

void write_config(SessionConfig  cfg) 

{
	//Export the configuration in binary to a file
	FILE* cfg_f;

	cfg_f = fopen("config.bin", "wb");
	
	size_t res;
	assert(cfg_f != NULL);
	printf("cfg=  %0d\n", cfg);
	res = fwrite(&cfg, sizeof(SessionConfig), 1, cfg_f);
	assert(res == 1);
	fclose(cfg_f);
}

void gmem_dump()
{
	FILE *fp = fopen("g_mem_dump.bin", "wb+");
	fwrite(&g_mem_layout, sizeof(DRAMLayout), 1, fp);
	fclose(fp);

#ifdef DEBUG
	DRAMLayout tmp;
	fp = fopen("g_mem_dump.bin", "rb");
	fread(&tmp, sizeof(DRAMLayout), 1, fp);
	fclose(fp);

	assert(tmp->h_fns->len == g_mem_layout->h_fns->len);
	assert(tmp->bank == g_mem_layout->bank);
	assert(tmp->row == g_mem_layout->row);
	assert(tmp->col == g_mem_layout->col);

#endif
}

int main(int argc, char **argv)
{
	srand(time(NULL));
	p = (ProfileParams*)malloc(sizeof(ProfileParams));
	if (p == NULL) {
		fprintf(stderr, "[ERROR] Memory allocation\n");
		exit(1);
	}

	if(process_argv(argc, argv, p) == -1) {
		free(p);
		exit(1);
	}

	MemoryBuffer mem = {
		.buffer = NULL,
		.physmap = NULL,
		.fd = p->huge_fd,
		.size = p->m_size,
		.align = p->m_align,
		.flags = p->g_flags & MEM_MASK
	};

	alloc_buffer(&mem);
	set_physmap(&mem);
	gmem_dump();  //MK: A debug function to dump the memory layout

	SessionConfig s_cfg;
	memset(&s_cfg, 0, sizeof(SessionConfig));
	if (p->g_flags & F_CONFIG) {
		printf("cfg file name = %s\n", p->conf_file);
		read_config(&s_cfg, p->conf_file);
	} else {
		// HARDCODED values
		s_cfg.h_rows = 4;
		s_cfg.h_rounds = p->rounds;
		
		//Hammer Config	
		//ASSISTED_DOUBLE_SIDED
		//FREE_TRIPLE_SIDED
		//N_SIDED
		
		s_cfg.h_cfg = TEST;

		//Hammer Data
		//RANDOM,
		//ONE_TO_ZERO = O2Z,
		//ZERO_TO_ONE = Z2O,
		//REVERSE = REVERSE_VAL

		s_cfg.d_cfg = RANDOM;
		s_cfg.base_off = p->base_off;
		s_cfg.aggr_n = p->aggr;

		printf("s_cfg.h_rows =%0d\n", s_cfg.h_rows);
		printf("s_cfg.h_rounds = %0d\n", s_cfg.h_rounds);
		printf("s_cfg.h_cfg = %0d\n", s_cfg.h_cfg);
		printf("s_cfg.d_cfg = %0d\n", s_cfg.d_cfg)  ;
		printf("s_cfg.base_off = %0d\n", s_cfg.base_off)  ;
		printf("s_cfg.aggr_n = %0d\n", s_cfg.aggr_n) ;
		write_config(s_cfg);
	
	}

	if (p->fuzzing) {
		fuzzing_session(&s_cfg, &mem);
	} else {
		hammer_session(&s_cfg, &mem);
	}

	close(p->huge_fd);
	return 0;
}

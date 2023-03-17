#include "addr-mapper.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static size_t g_rmap_len = 0;
static size_t g_base_row = 0;
static size_t g_bks = 0;
static size_t g_rows = 0;

RowMap get_row_map(ADDRMapper * mapper, DRAMAddr * d_addr)
{
	size_t idx =
	    (d_addr->row - g_base_row) * get_banks_cnt() + d_addr->bank;
	assert(idx < g_bks * g_rows);
	return mapper->row_maps[idx];
}

DRAM_pte get_dram_pte(ADDRMapper * mapper, DRAMAddr * d_addr)
{
	RowMap rmap = get_row_map(mapper, d_addr);
	return rmap.lst[(d_addr->col) >> 6];
}

RowMap gen_row_map(DRAMAddr d_src, MemoryBuffer * mem) //dram_addr contains base_row base_col
{
	RowMap rmap;
	DRAM_pte *dst = (DRAM_pte *) malloc(sizeof(DRAM_pte) * g_rmap_len); //MK size 128
	d_src.col = 0;

	for (size_t col = 0; col < g_rmap_len; col++, d_src.col += (1 << 6)) {  ///Why d_src.col is increased this much?
		dst[col].d_addr = d_src;
		dst[col].v_addr = phys_2_virt(dram_2_phys(d_src), mem); //When does it store an entry ffffffffffff?
		//printf("gen_row_map:: bank=%0d, row=%0d, col=%0d, physical_addr=%0llx, virtual_addr=%0llx\n", d_src.bank, d_src.row, d_src.col, dram_2_phys(d_src), phys_2_virt(dram_2_phys(d_src), mem));
		//fflush(stdout);
	}
	rmap.lst = dst;
	rmap.len = g_rmap_len;
	return rmap;

}

size_t rmap_idx(size_t bk, size_t row)
{
	return row * get_banks_cnt() + bk;
}
//MK init_addr_mapper(suite->mapper, mem, &suite->d_base, cfg->h_rows)
void init_addr_mapper(ADDRMapper * mapper, MemoryBuffer * mem,
		      DRAMAddr * d_base, size_t h_rows)
{
	mapper->row_maps =
	    (RowMap *) malloc(sizeof(RowMap) * h_rows * get_banks_cnt());  //MK How many entries are here? cfg->h_rows * Total number of banks
	mapper->base_row = d_base->row;
	g_base_row = d_base->row;
	g_bks = get_banks_cnt();
	g_rows = h_rows;
	// set global rmap_len
	g_rmap_len = ROW_SIZE / CL_SIZE;  //MK 8KB/64  = 128

	// create ptes list for every
	DRAMAddr d_tmp = {.bank = 0,.row = 0,.col = 0 };
	for (size_t bk = 0; bk < get_banks_cnt(); bk++) {
		d_tmp.bank = bk;
		for (size_t row = 0; row < h_rows; row++) {
			d_tmp.row = g_base_row + row;          //MK This shows that we are setting the number of rows off the base row.
			mapper->row_maps[rmap_idx(bk, row)] =    //MK rmap_idx = row * get_banks_cnt() + bk
			    gen_row_map(d_tmp, mem);
		}
	}
}

void tear_down_addr_mapper(ADDRMapper * mapper)
{
	for (int i = 1; i < g_rows * g_bks; i++) {
		free(mapper->row_maps[i].lst);
	}
	free(mapper->row_maps);
}

/*
 * This kernel scans within a block and writes the result to out_data
 */
__kernel
void hillis_steele_scan(__global int* data, __local int* scratch_data)
{
  int global_idx = get_global_id(0); // where in all threads
  int local_idx = get_local_id(0); // where in this block
  int block_size = get_local_size(0); // size of this block == size of scratch_data

  /* Copy a block-size data into local memory */
  scratch_data[local_idx] = data[global_idx];
  barrier(CLK_LOCAL_MEM_FENCE);

#ifndef SCAN_INCLUSIVE
    int swap = 0;

    if(local_idx > 0)
    {
      swap = scratch_data[local_idx - 1];
    }

    barrier(CLK_LOCAL_MEM_FENCE);
    scratch_data[local_idx] = swap;
#endif

  /* Execute scan according to hillis steele method */
  int steps, idx;

  for(int steps = 1; steps < block_size; steps <<=1)
  {
    idx = local_idx - steps;

    if(idx >= 0)
    {
      scratch_data[local_idx] += scratch_data[idx];
    }

    barrier(CLK_LOCAL_MEM_FENCE);

  }

  /* Copy data back to global memory */
  data[global_idx] = scratch_data[local_idx];
}

/*
 * This kernel performs the first step in a multi-block scan
 * It scan a block in shared memory and compute the sum of the block as well
 */
 __kernel
 void hillis_steele_scan_sum(__global int* data, __global int* sum, __local int* scratch_data)
{
  int global_idx = get_global_id(0); // where in all threads
  int local_idx = get_local_id(0); // where in this block
  int block_size = get_local_size(0); // size of this block == size of scratch_data

  /*
   * Copy data from global to shared memory
   */
  scratch_data[local_idx] = data[global_idx];
  barrier(CLK_LOCAL_MEM_FENCE);

#ifndef SCAN_INCLUSIVE

  int swap = 0;

  if(local_idx > 0)
    swap = scratch_data[local_idx - 1];

  barrier(CLK_LOCAL_MEM_FENCE);

  scratch_data[local_idx] = swap;
#endif

  /*
   * Execute scan according to hillis steele method
   */
  int steps, idx;

  for(int steps = 1; steps < block_size; steps <<=1)
  {
    idx = local_idx - steps;

    if(idx >= 0)
      scratch_data[local_idx] += scratch_data[idx];

    barrier(CLK_LOCAL_MEM_FENCE);
  }

  /*
  * Copy data back to global memory
  */
  data[global_idx] = scratch_data[local_idx];

  /*
   * Save the sum of the block for next step
   */
  if(local_idx == block_size - 1)
  {
    // In an inclusive scan, the sum of the block is simply the last element
#ifdef SCAN_INCLUSIVE
    sum[global_idx / block_size] = scratch_data[local_idx];
#else
    // In an exclusive scan, the sum of the block is second to last + last
    sum[global_idx / block_size] = scratch_data[local_idx] + data[global_idx];
#endif
  }
}

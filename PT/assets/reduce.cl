__kernel
void reduce(__global  float* data,
            __local   float* partial_sums,
            __global  float* output)
{
  // where the work item is in the work group
  int local_id = get_local_id(0);

  // how many work items in the work group
  int group_size = get_local_size(0);

  // each work item copies its data to the local memory
  partial_sums[local_id] = data[get_global_id(0)];

  // wait for all work items in group to have acquired their data
  barrier(CLK_LOCAL_MEM_FENCE);

  // the logN "recursive" sum
  for(int i = group_size/2; i > 0; i >>= 1)
  {
    if(local_id < i)
    {
      partial_sums[local_id] += partial_sums[local_id + i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  // first work item in group collects result
  if(local_id == 0)
    output[get_group_id(0)] = partial_sums[0];

}

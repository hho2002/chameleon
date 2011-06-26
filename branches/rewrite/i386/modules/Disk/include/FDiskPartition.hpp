/*
 * Copyright (c) 2011 Evan Lojewski. All rights reserved.
 *
 */
#ifndef FDISK_PARTITION_H
#define FDISK_PARTITION_H

#include <Partition.hpp>
#include <IOKit/IOTypes.h>
#include <IOKit/storage/IOFDiskPartitionScheme.h>

class FDiskPartition : public Partition
{
public:
    FDiskPartition(Disk* disk, UInt8 partitionNumber);
    ~FDiskPartition();

protected:
    bool                isMBRDisk();

    struct disk_blk0    mLBA0;
    struct fdisk_part*  mFdiskEntry;
    
private:
};

#endif /* FDISK_PARTITION_H */

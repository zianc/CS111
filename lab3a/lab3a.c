// NAME: Xuesen Cui
// EMAIL:cuixuesen@ucla.edu
// ID:604941626
// NAME: Haiying Huang
// EMAIL: hhaiying1998@outlook.com
// ID: 804757410
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include "ext2_fs.h"
#include <time.h>
const int SUPERBLOCK_OFFSET = 1024;
const int SUPERBLOCK_SIZE = 1024;

struct ext2_super_block sb_fields;
__u32 block_size;
// super block paramters

int num_groups = 1;
struct ext2_group_desc* group_desc_arr;
int image_fd;

// a helper function that reports an error message and exit 1
void report_error_and_exit(const char* msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

void print_superblock()
{
    printf("%s,%u,%u,%u,%u,%u,%u,%u\n", "SUPERBLOCK",
           sb_fields.s_blocks_count,
           sb_fields.s_inodes_count,
           block_size,
           sb_fields.s_inode_size,
           sb_fields.s_blocks_per_group,
           sb_fields.s_inodes_per_group,
           sb_fields.s_first_ino);
}

void print_group(int num)
{
    int real_block_number;
    if ( sb_fields.s_blocks_per_group >  sb_fields.s_blocks_count)
        real_block_number=sb_fields.s_blocks_count;
    else
        real_block_number=sb_fields.s_blocks_per_group;
    
    printf("%s,%u,%u,%u,%u,%u,%u,%u,%u\n", "GROUP",
           num,
           real_block_number,
           sb_fields.s_inodes_per_group,
           group_desc_arr[num].bg_free_blocks_count,
           group_desc_arr[num].bg_free_inodes_count,
           group_desc_arr[num].bg_block_bitmap,
           group_desc_arr[num].bg_inode_bitmap,
           group_desc_arr[num].bg_inode_table);
}

int get_group_offset(int num)
{
    return SUPERBLOCK_OFFSET + (int) block_size + num * sizeof(struct ext2_group_desc);
}

int get_block_offset(int block_index)
{
    return block_index * (int) block_size;
}

void process_group_bitmap(int num)
{
    __u32 index = group_desc_arr[num].bg_block_bitmap;
    char* bitmap = malloc(block_size * sizeof(char));
    if (pread(image_fd, bitmap, block_size, get_block_offset(index)) < 0)
        report_error_and_exit("Error reading from the bit map");
    
    unsigned i;
    int j;
    int mask = 1;
    for (i = 0; i < block_size; ++i)
    {
        char byte = bitmap[i];
        for (j = 0; j < 8; ++j)
        {
            int bit = byte & (mask << j);
            int block_num = num * sb_fields.s_blocks_per_group + i * 8 + j + 1;
            // find the number of the block corresponding the current bit
            if (bit == 0)
            {
                printf("%s,%u\n", "BFREE", block_num);
            }
        }
    }
    
}
//---------------------------------------------------------//
int get_indir_offset(int offset)
{
    return block_size*offset;
}
void print_indirect_block_references(__u32 block_number, int inode_number, int offset, int level)
{
    
    unsigned i;
    __u32 indi_block_number=block_size/4;
    __u32 *element=malloc(block_size*sizeof(__u32));
    
    if (pread(image_fd, element, block_size, get_indir_offset(block_number)) < 0)
    { //report_error_and_exit("Error reading from the indirect block");
        
    }
    
    for (i=0;i<indi_block_number;i++)
    {
        
        
        if(element[i]!=0)
        {
            printf("%s,%d,%d,%d,%u,%u\n", "INDIRECT",
                   inode_number,
                   level,
                   offset,
                   block_number,
                   element[i]);
            if (level==2 || level==3)
                print_indirect_block_references(element[i], inode_number, offset, level-1);
            
        }
        
        if (level == 1)
        {
            offset++;
        }
        else if (level == 2)
        {
            offset += 256;
        }
        
        else if (level == 3)
        {
            offset += 256 * 256;
        }
    }
}

//there are 256 indirect blocks per doubly-indirect block
//there are 65536 indirect blocks per triply-indirect block
//---------------------------------------------------------//
int get_dir_offset(int inodeindex, int logical_offset)
{
    return inodeindex*block_size+logical_offset;
}
// beginning logical_offset is 0
void print_directory_entries(struct ext2_inode* inode, int inode_number, int logical_offset)
{
    struct ext2_dir_entry *dir_entry=malloc(sizeof(struct ext2_dir_entry));
    unsigned i;
    // there are only 12 directory blocks
    for (i=0;i<EXT2_NDIR_BLOCKS;i++)
    {
        if(inode->i_block[i]!=0)
        {
            while((unsigned) logical_offset < block_size)
            {
                if (pread(image_fd, dir_entry, sizeof(struct ext2_dir_entry), get_dir_offset(inode->i_block[i],logical_offset)) < 0)
                    report_error_and_exit("Error reading directory entries");
                if(dir_entry->inode!=0)
                {
                    printf("%s,%u,%d,%u,%u,%u,'%s'\n", "DIRENT",
                           inode_number,
                           logical_offset,
                           dir_entry->inode,
                           dir_entry->rec_len,
                           dir_entry->name_len,
                           dir_entry->name);
                }
                //rec_len is the displacement to the next directory entry
                logical_offset+=dir_entry->rec_len;
            }
        }
        
    }
    
}

void process_inode_bitmap(int num)
{
    __u32 bitmap_num = group_desc_arr[num].bg_inode_bitmap;
    char* bitmap = malloc(block_size * sizeof(char));
    if (pread(image_fd, bitmap, block_size, get_block_offset(bitmap_num)) < 0)
        report_error_and_exit("Error reading from the bit map");
    
    unsigned i;
    int j;
    int mask = 0x01;
    for (i = 0; i < block_size; ++i)
    {
        char byte = bitmap[i];
        for (j = 0; j < 8; ++j)
        {
            int bit = byte & (mask << j);
            int inode_num = num * sb_fields.s_inodes_per_group + i * 8 + j + 1;
            // find the number of the block corresponding the current bit
            if (bit == 0)
                // if the block is free
            {
                printf("%s,%u\n", "IFREE", inode_num);
            }
        }
    }
    
}

#define EXT2_S_IFDIR    0x4000
#define EXT2_S_IFLNK    0xA000
#define EXT2_S_IFREG    0x8000

char get_file_type(__u16 i_mode)
{
    char type;
    __u16 mask = 0xF000;
    __u16 format = i_mode & mask;
    
    if (format == EXT2_S_IFREG)
        type = 'f';
    else if (format == EXT2_S_IFDIR)
        type = 'd';
    else if (format == EXT2_S_IFLNK)
        type = 'l';
    else
        type = '?';
    
    return type;
}

void decode_time(char* buf, __u32 i_time)
{
    struct tm* time_gm;
    time_t temp_time=i_time;
    
    time_gm = gmtime(&temp_time);
    if (time_gm == NULL)
        report_error_and_exit("Error decoding inode time to GMT");
    
    sprintf(buf, "%02d/%02d/%02d %02d:%02d:%02d",  time_gm->tm_mon + 1,
            time_gm->tm_mday,
            (time_gm->tm_year) % 100,
            time_gm->tm_hour,
            time_gm->tm_min,
            time_gm->tm_sec);
    
}

void process_inode_table(int num)
{
    __u32 inode_table_num = group_desc_arr[num].bg_inode_table;
    struct ext2_inode* inode_table;
    size_t inode_table_size = sb_fields.s_inodes_per_group * sizeof(struct ext2_inode);
    inode_table = (struct ext2_inode*) malloc(inode_table_size);
    
    // fill in the inode table per block
    if (pread(image_fd, inode_table, inode_table_size, get_block_offset(inode_table_num)) < 0)
        report_error_and_exit("Error reading from the inode table");
    
    unsigned i;
    int j;
    
    for (i = 0; i < sb_fields.s_inodes_per_group; ++i)
    {
        struct ext2_inode inode_curr = inode_table[i];
        int inode_num =  num * sb_fields.s_inodes_per_group + i + 1;
        if (inode_curr.i_mode != 0 && inode_curr.i_links_count != 0)
        {
            char file_type = get_file_type(inode_curr.i_mode);
            __u16 mask = 0xFFF;
            __u16 mode = inode_curr.i_mode & mask;
            // retrieve the latest creation, modification, and access time
            char ctime_str[30];
            char mtime_str[30];
            char atime_str[30];
            
            decode_time(ctime_str, inode_curr.i_ctime);
            decode_time(mtime_str, inode_curr.i_mtime);
            decode_time(atime_str, inode_curr.i_atime);
            
            printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u",  inode_num,
                   file_type,
                   mode,
                   inode_curr.i_uid,
                   inode_curr.i_gid,
                   inode_curr.i_links_count,
                   ctime_str,
                   mtime_str,
                   atime_str,
                   inode_curr.i_size,
                   inode_curr.i_blocks);
            
            for (j = 0; j < 15; ++j)
            {
                __u32 block_num = inode_curr.i_block[j];
                printf(",%u",block_num);
            }
            printf("\n");
            
            
            if (file_type=='d')
            {
                print_directory_entries(&inode_curr, inode_num,0);
            }
            
            // The 13th entry in i_block is the first indirect block
            // offset 12 for first indirect, 268 for double--indirect block, and 65536+268=65804 for triply-indirect block
            // http://www.nongnu.org/ext2-doc/ext2.html
            if (inode_curr.i_block[EXT2_IND_BLOCK] !=0)
            { print_indirect_block_references(inode_curr.i_block[EXT2_IND_BLOCK] ,inode_num,12,1);    }
            
            // The 14th entry in i_block is the first double-indirect block
            if (inode_curr.i_block[EXT2_DIND_BLOCK]!=0)
            { print_indirect_block_references(inode_curr.i_block[EXT2_DIND_BLOCK],inode_num,268,2);   }
            
            // The 15th entry in i_block is the first triply-indirect block
            if (inode_curr.i_block[EXT2_TIND_BLOCK]!=0)
            { print_indirect_block_references(inode_curr.i_block[EXT2_TIND_BLOCK],inode_num,65804,3);  }
            
        }
        
    }
}
    
    //---------------------------------------------------------//
    int main(int argc, char* argv[])
    {
        if (argc != 2)
        {
            fprintf(stderr, "Find bad argument!");
            fprintf(stderr, "Usage: ./lab3a filename");
            
            exit(1);
        }
        
        image_fd = open(argv[1], O_RDONLY);
        if (image_fd < 0)
            report_error_and_exit("Error opening the iamge file");
        
        if (pread(image_fd, &sb_fields, sizeof(struct ext2_super_block), SUPERBLOCK_SIZE) < 0)
            report_error_and_exit("Error reading from super block");
        // fill in super block parameters
        block_size =  EXT2_MIN_BLOCK_SIZE << sb_fields.s_log_block_size;
        print_superblock();
        
        // determine block size from super block
        
        group_desc_arr = (struct ext2_group_desc*) malloc(num_groups * sizeof(struct ext2_group_desc));
        // allocate an array to hold all group descriptors
        int i;
        for (i = 0; i < num_groups; ++i)
        {
            if (pread(image_fd, &group_desc_arr[i], sizeof(struct ext2_group_desc), get_group_offset(i)) < 0)
                report_error_and_exit("Error reading group descriptors");
            // read the ith group descriptor
            print_group(i);
            
        }
        
        for (i = 0; i < num_groups; ++i)
        {
            process_group_bitmap(i);
        }
        for (i = 0; i < num_groups; ++i)
        {
            process_inode_bitmap(i);
        }
        for (i = 0; i < num_groups; ++i)
        {
            process_inode_table(i);
        }
        
        exit(0);
        
    }



# NAME:Xuesen Cui
# EMAIL:cuixuesen@ucla.edu
# ID:604941626
# NAME:Haiying Huang
# EMAIL:hhaiying1998@outlook.com
# ID:804757410
import sys,csv,math,numpy

superblock_element=None
group_element=None
inode_element=[]
dirent_element=[]
indirect_element=[]
bfree_element=[]
ifree_element=[]
balloc_element={} # a dictionary mapping allocated block to list of inodes that point to this point\
bdup_element=set()  # a list of all detected duplicate blocks


# constants
MAX_BLOCK_COUNT =0
FISRT_DATA_BLOCK_NUM=0

def print_error(error_message):
    sys.stderr.write(error_message)
    sys.exit(1)
#class for superblock's information read from the csv file
class superblock:
    def __init__(self,element):
        self.s_blocks_count     =int(element[1])
        self.s_inodes_count     =int(element[2])
        self.block_size         =int(element[3])
        self.s_inode_size       =int(element[4])
        self.s_blocks_per_group =int(element[5])
        self.s_inodes_per_group =int(element[6])
        self.s_first_ino        =int(element[7])

#class for superblock's inode read from the csv file
class inode:
    def __init__(self,element):
        self.inode_num          =int(element[1])
        self.file_type          =element[2]
        self.mode               =int(element[3])
        self.i_uid              =int(element[4])
        self.i_gid              =int(element[5])
        self.i_links_count      =int(element[6])
        self.ctime_str          =element[7]
        self.mtime_str          =element[8]
        self.atime_str          =element[9]
        self.i_size             =int(element[10])
        self.i_blocks           =int(element[11])
        self.dirent_blocks      =[int(block_num) for block_num in element[12:24]]
        self.indirect_blocks    =[int(block_num) for block_num in element[24:27]]

#class for group's information read from the csv file
class group:
    def __init__(self,element):
        self.s_inodes_per_group = int(element[3])
        self.bg_inode_table = int(element[-1])

#class for dirent's information read from the csv file
class dirent:
    def __init__(self,element):
        self.inode_num          =int(element[1])
        self.logical_offset     =int(element[2])
        self.inode              =int(element[3])
        self.rec_len            =int(element[4])
        self.name_len           =int(element[5])
        self.name               =element[6]

#class for indirect's information read from the csv file
class indirect:
    def __init__(self,element):
        self.inode_num          =int(element[1])
        self.level              =int(element[2])
        self.offset             =int(element[3])
        self.block_number       =int(element[4])
        self.element            =int(element[5])


def check_blocks(MAX_BLOCK_COUNT, FISRT_DATA_BLOCK_NUM):

    # scan the inode table
    for inode in inode_element:
        if inode.file_type == 's' and inode.i_size <=MAX_BLOCK_COUNT :
            continue
        offset = 0
        for block_num in inode.dirent_blocks:
            if block_num < 0 or block_num >= MAX_BLOCK_COUNT:
                print("INVALID BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, offset))
            elif block_num > 0 and block_num < FISRT_DATA_BLOCK_NUM:
                print("RESERVED BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, offset))
            elif block_num != 0: # find used blocks
                if block_num not in balloc_element:
                    balloc_element[block_num] = [[inode.inode_num, offset, 0]]
                else:
                    bdup_element.add(block_num)
                    balloc_element[block_num].append([inode.inode_num, offset, 0])
            offset += 1

        # check the singly indirect block
        block_num=inode.indirect_blocks[0]
        if inode.indirect_blocks[0] < 0 or inode.indirect_blocks[0] >= MAX_BLOCK_COUNT:
            print("INVALID INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, 12))
        elif block_num > 0 and block_num < FISRT_DATA_BLOCK_NUM:
            print("RESERVED INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, 12))
        elif block_num != 0:
            if block_num not in balloc_element:
                balloc_element[block_num] = [[inode.inode_num, 12, 1]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append([inode.inode_num, 12, 1])

        # check the doubly indirect block
        block_num=inode.indirect_blocks[1]
        if inode.indirect_blocks[1] < 0 or inode.indirect_blocks[1] >= MAX_BLOCK_COUNT:
            print("INVALID DOUBLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, 268))
        elif block_num > 0 and block_num < FISRT_DATA_BLOCK_NUM:
            print("RESERVED DOUBLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, 268))
        elif block_num != 0:
            if block_num not in balloc_element:
                balloc_element[block_num] = [[inode.inode_num, 268, 2]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append([inode.inode_num, 268, 2])

        # check the triply indirect block
        block_num=inode.indirect_blocks[2]
        if inode.indirect_blocks[2] < 0 or inode.indirect_blocks[2] >= MAX_BLOCK_COUNT:
            print("INVALID TRIPLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, 65804))
        elif block_num > 0 and block_num < FISRT_DATA_BLOCK_NUM:
            print("RESERVED TRIPLE INDIRECT BLOCK %d IN INODE %d AT OFFSET %d" %(block_num, inode.inode_num, 65804))
        elif block_num != 0:
            if block_num not in balloc_element:
                balloc_element[block_num] = [[inode.inode_num, 65804, 3]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append([inode.inode_num, 65804, 3])


    # check the inner indirect block_size
    for indirect in indirect_element:
        block_num = indirect.element
        level = ""
        if indirect.level == 1:
            level = "INDIRECT"
        elif indirect.level == 2:
            level = "DOUBLE INDIRECT"
        elif indirect.level == 3:
            level = "TRIPLE INDIRECT"

        if block_num < 0 or block_num >= MAX_BLOCK_COUNT:
            print("INVALID %s BLOCK %d IN INODE %d AT OFFSET %d" %(level, block_num, indirect.inode_num, indirect.offset))
        elif block_num > 0 and block_num < FISRT_DATA_BLOCK_NUM:
            print("RESERVED %s BLOCK %d IN INODE %d AT OFFSET %d" %(level, block_num, indirect.inode_num, indirect.offset))
        elif block_num != 0: # find used blocks
            if block_num not in balloc_element:
                balloc_element[block_num] = [[indirect.inode_num, indirect.offset, indirect.level]]
            else:
                bdup_element.add(block_num)
                balloc_element[block_num].append([indirect.inode_num, indirect.offset, indirect.level])

    # scan all data blocks for unreferenced / allocated
    for num in range(FISRT_DATA_BLOCK_NUM, MAX_BLOCK_COUNT):
        if num not in bfree_element and num not in balloc_element:
            print("UNREFERENCED BLOCK %d" %num)
        if num in bfree_element and num in balloc_element:
            print("ALLOCATED BLOCK %d ON FREELIST" %num)

    # print information of all duplicated blocks
    for num in bdup_element:
        for ref in balloc_element[num]:
            indirect = ""
            if ref[-1] == 1:
                indirect = "INDIRECT"
            elif ref[-1] == 2:
                indirect = "DOUBLE INDIRECT"
            elif ref[-1] == 3:
                indirect = "TRIPLE INDIRECT"
            if indirect == "":
                print("DUPLICATE BLOCK %d IN INODE %d AT OFFSET %d" %(num, ref[0], ref[1]))
            else:
                print("DUPLICATE %s BLOCK %d IN INODE %d AT OFFSET %d" %(indirect, num, ref[0], ref[1]))


def I_node_and_Directory_Allocation_Audits(first_inode_pos,inode_count):

    allocated_inode=[]
    links=numpy.zeros(inode_count + first_inode_pos)
    parentof=numpy.zeros(inode_count + first_inode_pos)
    parentof[2]=2

    #scan the inode and print the inode numnber when it is a valid number and also in the FREELIST
    for allocated_inode_num in inode_element:
        if allocated_inode_num.inode_num != 0:
           allocated_inode.append(allocated_inode_num.inode_num)
           if allocated_inode_num.inode_num in ifree_element:
               print("ALLOCATED INODE %d ON FREELIST" %(allocated_inode_num.inode_num))

    #scan all the valid inode number to detect the UNALLOCATED inode and not in the free list
    for inode_num in range(first_inode_pos,inode_count):
        if inode_num not in allocated_inode and inode_num not in ifree_element:
            print("UNALLOCATED INODE %d NOT ON FREELIST" %(inode_num))

    #if inode's file type is zero then the it should in the freelist
    for allocated_inode_num in inode_element:
        if allocated_inode_num.inode_num not in allocated_inode and allocated_inode_num.inode_num not in ifree_element:
            emptylist="empty"
        elif allocated_inode_num.file_type == '0' and allocated_inode_num.inode_num not in ifree_element:
            print("UNALLOCATED INODE %d NOT ON FREELIST" %(allocated_inode_num.inode_num))


    #if the inode number of the directory is out of boundary or not in the allocated inode set, output error_message
    #else count the corresponding inode number in the links list
    for dir in dirent_element:
        if dir.inode > inode_count or dir.inode < 1:
            print("DIRECTORY INODE %d NAME %s INVALID INODE %d"     %(dir.inode_num, dir.name, dir.inode))
        elif dir.inode not in allocated_inode:
            print("DIRECTORY INODE %d NAME %s UNALLOCATED INODE %d" %(dir.inode_num, dir.name, dir.inode))
        else:
            links[dir.inode] += 1

    #scan the all the inode for incorrect i_links_count
    for inode in inode_element:
        if links[inode.inode_num] != inode.i_links_count:
            print("INODE %d HAS %d LINKS BUT LINKCOUNT IS %d" %(inode.inode_num, links[inode.inode_num], inode.i_links_count ))

    #scan the directory element and record their parent's inode number
    for dir in dirent_element:
        if dir.name != "'.'" and dir.name != "'..'" :
            parentof[dir.inode]=dir.inode_num

    #detect inconsistency for '.' and '..'
    for dir_element in dirent_element:
        if dir_element.name == "'.'" and dir_element.inode != dir_element.inode_num:
            print("DIRECTORY INODE %d NAME '.' LINK TO INODE %d SHOULD BE %d"  %(dir_element.inode_num, dir_element.inode,dir_element.inode_num))
        if dir_element.name == "'..'" and parentof[dir_element.inode_num] != dir_element.inode:
            print("DIRECTORY INODE %d NAME '..' LINK TO INODE %d SHOULD BE %d" %(dir_element.inode_num, dir_element.inode, parentof[dir_element.inode_num]))



def Block_Consistency_Audits(MAX_BLOCK_COUNT, FISRT_DATA_BLOCK_NUM):
    check_blocks(MAX_BLOCK_COUNT, FISRT_DATA_BLOCK_NUM)



def main ():

    if len(sys.argv) != 2:
        print_error("Error: the input argument is not two \n")

    #open the input csv file
    try:
        input_csvfile=open(sys.argv[1], 'r')
    except IOError:
        print_error("Error: cannot open the input csv file \n")

    #read the csv infromation and stored them to corresponding class element
    filereader=csv.reader(input_csvfile)

    for element in filereader:
        if element[0] == "SUPERBLOCK":
            superblock_element=superblock(element)
        elif element[0] == "GROUP":
            group_element=group(element)
        elif element[0] == "BFREE":
            bfree_element.append(int(element[1]))
        elif element[0] == "IFREE":
            ifree_element.append(int(element[1]))
        elif element[0] == "INODE":
            inode_element.append(inode(element))
        elif element[0] == "DIRENT":
            dirent_element.append(dirent(element))
        elif element[0] == "INDIRECT":
            indirect_element.append(indirect(element))
        else:
            print_error("Error: the input csv file contains invild element: \n")

    #the maximum number of block
    MAX_BLOCK_COUNT =superblock_element.s_blocks_count
    #first block position index
    FISRT_DATA_BLOCK_NUM = int(group_element.bg_inode_table + math.ceil(group_element.s_inodes_per_group * superblock_element.s_inode_size / superblock_element.block_size))

    #scan the blocks and print error_message
    Block_Consistency_Audits(MAX_BLOCK_COUNT, FISRT_DATA_BLOCK_NUM)
    #scan the inode and directory, and print error_message
    I_node_and_Directory_Allocation_Audits(superblock_element.s_first_ino, superblock_element.s_inodes_count)


if __name__ == '__main__':
    main()

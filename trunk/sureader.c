#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#define MASTER_HASH_OFFSET 0x032C
#define HASH_DATA_START 0x344
#define HASH_DATA_END 0xB000
#define HASH_DATA_LEN (HASH_DATA_END-HASH_DATA_START)

#define PEC_HASH_OFFSET 0x228
#define PEC_DATA_START 0x23C
#define PEC_DATA_END 0x1000
#define PEC_DATA_LEN (PEC_DATA_END-PEC_DATA_START)

#define BLOCK_HASH_START 0xB000
#define FILE_HEADER_OFFSET 0xC000
// start of file data described by file headers
#define FILE_START_OFFSET 0xD000
#define BLOCK_SIZE 0x1000

inline unsigned short endian_swap16(unsigned short x)
{
    return (unsigned short)( (x>>8) | 
        (x<<8));
}

inline unsigned int endian_swap32(unsigned int x)
{
    return (unsigned int)((x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24));
}

#define UInt32 unsigned int

#define Endian32_Swap(value) \
         ((((UInt32)((value) & 0x000000FF)) << 24) | \
         (((UInt32)((value) & 0x0000FF00)) << 8) | \
         (((UInt32)((value) & 0x00FF0000)) >> 8) | \
         (((UInt32)((value) & 0xFF000000)) >> 24))

//#define Endian24_Swap(value) (((value&0xFF)<<16)|(value&0xFF00)|((value>>16)&0xFF))

// each embedded file starts at a 4096 byte boundary
typedef struct {
    // maximum filename length: 40 bytes
    char filename[40];
    unsigned char namelen; // length of filename with upper two bits as attrib bits
    unsigned int len1:24; // 3 byte file length rounded to block size (little endian)
    unsigned int len2:24; // 3 byte file length repeated (little endian)
    unsigned int start_block:24; // starting block of file (little endian)
    signed short path; // -1 if root
    unsigned int fsize; // file size (big endian)
    signed int update_timestamp; // uses FAT format
    signed int access_timestamp; // uses FAT format
} __attribute__((packed)) FILE_INFO;

// starts at offset 0x340
typedef struct {
    signed int mentry_id;
    signed int content_type;
    signed long long content_size;
    unsigned int media_id;
    signed int version; // system/title updates
    signed int base_version; // system/title updates
    unsigned int title_id;
    unsigned char platform;
    unsigned char exec_type;
    unsigned char disc_num;
    unsigned char disc_in_set;
    unsigned int savegame_id;
    unsigned char console_id[5];
    unsigned char profile_id[8];
    unsigned char volume_desc_size; // usually 0x24
} __attribute__((packed)) META_DATA;


typedef struct {
    unsigned char hash[20]; // SHA1 hash of block
    unsigned int desc;      // descriptor for where the block is (big endian)
} __attribute__((packed)) BLOCK_HASH;

int BlockToOffset(int block)
{
    int cluster = FILE_START_OFFSET + block * BLOCK_SIZE;
    while (block >= 170) {
        block /= 170;
        cluster += (block + 1) * 0x2000;
    }
    return cluster;
}

void print_data(unsigned char *data, int len)
{
    int i;
    for (i=0; i<len; i++)
    {
        printf("%.2X ", (unsigned int)(data[i]));
    }
    printf("\n");
}

unsigned int get_file_size(FILE *f)
{
    unsigned int val, back = ftell(f);
    fseek(f, 0, SEEK_END);
    val = ftell(f);
    fseek(f, back, SEEK_SET);
    return val;
}

void verify_hash(char *name, FILE *f, unsigned int hashoff, unsigned int hash_data_start, unsigned int hash_data_len)
{
    unsigned char *tmpdata = NULL; //[HASH_DATA_LEN];
    unsigned char hash[20], tmphash[20];

    tmpdata = malloc(hash_data_len);

    fseek(f, hashoff, SEEK_SET);
    fread(hash, 1, 20, f);
    //fread(&mentry_id, 1, sizeof(unsigned int), f);
    //fread(&content_type, 1, sizeof(unsigned int), f);
    fseek(f, hash_data_start, SEEK_SET);
    fread(tmpdata, 1, hash_data_len, f);

    memset(tmphash, 20, 0);
    // compute hash
    SHA1(tmpdata, hash_data_len, tmphash);
    if (!memcmp(hash, tmphash, 20))
    {
        printf("HASH %s MATCHES!\n", name);
    }
    else
    {
        printf("hash %s doesn't match\n", name);
        //printf("hash1: ");
        //print_data(hash, 20);
        //printf("hash2: ");
        //print_data(tmphash, 20);
    }

    free(tmpdata);
}

int verify_xex(FILE *f, unsigned int offset)
{
    int found = 0;
    char magic[4];
    unsigned int curoff = ftell(f);
    fseek(f, offset, SEEK_SET);
    fread(magic,1,4,f);
    if (magic[0] == 'X' && magic[1] == 'E' && magic[2] == 'X' && magic[3] == '2')
    {
        found = 1;
    }
    fseek(f, curoff, SEEK_SET);
    return found;
}

int verify_xttf(FILE *f, unsigned int offset)
{
    int found = 0;
    char magic[4];
    unsigned int curoff = ftell(f);
    fseek(f, offset, SEEK_SET);
    fread(magic,1,4,f);
    if (magic[0] == 'x' && magic[1] == 't' && magic[2] == 't' && magic[3] == 'f')
    {
        found = 1;
    }
    fseek(f, curoff, SEEK_SET);
    return found;

}

unsigned int get_cluster(unsigned int block)
{
    unsigned int roff = 0;
    while (block >= 170) {
        block /= 170;
        roff += (block+1);
    }
    return roff;
}

void print_file_list(FILE *f)
{
    int cnt = 1;
    unsigned int offset;
    unsigned int blockoff = 0;
    unsigned int start_block = 0;
    unsigned int len;
    FILE_INFO fileinfo;
    fseek(f, FILE_HEADER_OFFSET, SEEK_SET);
    while (1)
    {
        fread(&fileinfo, 1, sizeof(FILE_INFO), f);
        if (fileinfo.filename[0] != '$') break;
	fileinfo.namelen = fileinfo.namelen & 0x1F; // extract filename size
        blockoff = get_cluster(fileinfo.start_block);
        offset = FILE_START_OFFSET + (fileinfo.start_block + blockoff - 1) * 4096;
        len = fileinfo.len1;
        printf("%d: file name: %s namelen: %d len: %X start block: %X offset: %X", cnt, fileinfo.filename, fileinfo.namelen, fileinfo.len1, fileinfo.start_block, offset);
        if (verify_xex(f, offset)) {
            printf(" (XEX)");
        } else if (verify_xttf(f, offset)) {
            printf(" (xttf)");
        }
        printf("\n");
	// $flash_dash.xex has 0x8000 of data after it
        //if (!strcmp(fileinfo.filename, "$flash_dash.xex")) blockoff = 7;
	//else if (!strcmp(fileinfo.filename, "$flash_marketplace.xexp")) blockoff += 1;
        //else if (!strcmp(fileinfo.filename, "$flash_xam.xexp")) blockoff += 2;
        cnt++;
    }
}

void verify_all_hash_blocks(FILE *f)
{
    unsigned int i, j=1, off;
    unsigned int filesize = get_file_size(f);

    //jmax = (((off - FILE_START_OFFSET)/BLOCK_SIZE + 1)/0xAB);

    while(1)
    {
         off = FILE_START_OFFSET + (0xAA * BLOCK_SIZE * j) + (BLOCK_SIZE * (j-1));
         if (off > filesize) break;
         printf("off %X\n", off);
         for(i = 0; i < 1; i++)
         {
             verify_hash("block", f, (off+24*i), FILE_START_OFFSET+(BLOCK_SIZE*((0xAB*j)+i)), BLOCK_SIZE);
         }
 
         j++;
     }
}

void write_file_data(FILE *inf, char *filename, unsigned int offset, unsigned int len)
{
    FILE *outf;
    char c;
    unsigned int i;
    unsigned int back = ftell(inf);
    fseek(inf, offset, SEEK_SET);
    outf = fopen(filename, "wb");
    for (i = 0; i < len; i++)
    {
        c = fgetc(inf);
        fputc(c,outf);
    }
    fclose(outf);
    fseek(inf, back, SEEK_SET);
}

void dump_files(FILE *f, char *dirname)
{
    int cnt, i;
    unsigned int offset;
    unsigned int blockoff = 0;
    unsigned int start_block = 0;
    FILE_INFO fileinfo;
    char filepath[255];

    sprintf(filepath, "%s/%s", dirname, "header.bin");
    write_file_data(f, filepath, 0, 0x104); 

    sprintf(filepath, "%s/%s", dirname, "hash.bin");
    write_file_data(f, filepath, MASTER_HASH_OFFSET, 0x70);

    // SUPD at 0x971A (12 bytes)
    // 0xB000 - 0xBFFF is hash of blocks (can be recreated)
    // 0xC000 - 0xCFFF is file info index (zero padded at end of file info)

    fseek(f, FILE_HEADER_OFFSET, SEEK_SET); 
    cnt = 0;
    while (fgetc(f) == '$') {
        fseek(f, 0x40-1, SEEK_CUR);
        cnt++;
    }

    sprintf(filepath, "%s/%s", dirname, "file_index.bin");
    write_file_data(f, filepath, FILE_HEADER_OFFSET, cnt * 0x40);

    printf("%d files\n", cnt);

    fseek(f, FILE_HEADER_OFFSET, SEEK_SET);

    for(i = 0; i < cnt; i++)
    {
        fread(&fileinfo, 1, sizeof(FILE_INFO), f);
        blockoff = get_cluster(fileinfo.start_block);
        offset = FILE_START_OFFSET + (fileinfo.start_block + blockoff - 1) * 4096;

        sprintf(filepath, "%s/%s", dirname, fileinfo.filename);

        write_file_data(f, filepath, offset, fileinfo.len1 * BLOCK_SIZE);
    }
}

int main(int argc, char **argv)
{
    unsigned char buf[256];
    char c1, c2;
    unsigned int diffpos, pos = 0;
    FILE *file;
    if (argc < 3)
    {
        printf("sureader <xbox_su_file.bin> <output dir>\n");
        return 0;
    }
    file = fopen(argv[1], "rb");
    fread(buf, 1, 4, file);
    printf("update type: %c%c%c%c\n", buf[0], buf[1], buf[2], buf[3]);

    dump_files(file, argv[2]);

    verify_hash("master", file, MASTER_HASH_OFFSET, HASH_DATA_START, HASH_DATA_LEN);
    verify_all_hash_blocks(file);
    print_file_list(file);
    fclose(file);
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#define MASTER_HASH_OFFSET 0x032C
#define HASH_DATA_START 0x344
#define HASH_DATA_END 0xB000
#define HASH_DATA_LEN (HASH_DATA_END-HASH_DATA_START)

#define SECOND_HASH_OFFSET 0x0381
#define SECOND_HASH_START 0xB6000
// SECOND_HASH_LEN is BLOCK_SIZE

#define PEC_HASH_OFFSET 0x228
#define PEC_DATA_START 0x23C
#define PEC_DATA_END 0x1000
#define PEC_DATA_LEN (PEC_DATA_END-PEC_DATA_START)

#define DASH_VERSION_OFFSET 0x971A

#define BLOCK_HASH_START 0xB000
#define FILE_HEADER_OFFSET 0xC000
// start of file data described by file headers
#define FILE_START_OFFSET 0xD000
#define BLOCK_SIZE 0x1000

#define SWAP32(x)((x>>24)|(x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x<<24)

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

unsigned int get_file_size(FILE *f)
{
    unsigned int val, back = ftell(f);
    fseek(f, 0, SEEK_END);
    val = ftell(f);
    fseek(f, back, SEEK_SET);
    return val;
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

unsigned int write_data_file(FILE *outf, char *dir, char *filename, unsigned int offset)
{
    char filepath[255];
    FILE *inf;
    char c;
    unsigned int i;
    unsigned int size = 0;
    fseek(outf, offset, SEEK_SET);
    sprintf(filepath, "%s/%s", dir, filename);

    inf = fopen(filepath, "rb");
    if (inf == NULL)
    {
        printf("Error unable to open %s\n", filepath);
        return(-1);
    }

    size = get_file_size(inf);

    printf("writing %X bytes from %s to %X\n", size, filepath, offset);

    for(i=0; i < size; i++)
    {
        c = fgetc(inf);
        fputc(c,outf);
    }
    fclose(inf);
    return size;
}

void write_zero_data(FILE *outf, unsigned int size)
{
    unsigned char data[BLOCK_SIZE];
    memset(data, 0, BLOCK_SIZE);
    unsigned int offset=0;
    while(offset < size)
    {
        fwrite(data, 1, BLOCK_SIZE, outf);
        offset+=BLOCK_SIZE;
    }
}

void write_misc_data(FILE *outf)
{
    unsigned char buf[128];

    memset(buf, 0xFF, 8);
    fseek(outf, 0x22C, SEEK_SET);
    fwrite(buf, 1, 8, outf);

    fseek(outf, 0x412, SEEK_SET);
    memcpy(buf, "S\00y\00s\00t\00e\00m\00 \00U\00p\00d\00a\00t\00e\00", 26);
    fwrite(buf, 1, 26, outf);

/*
    fseek(outf, 0x971A, SEEK_SET);
    memcpy(buf, "SUPD\00\00\00\00\x20\x1A\x1B", 11);
    fwrite(buf, 1, 11, outf);
*/
}

void write_files(FILE *outf, char *dir)
{
    unsigned int fileindex_size, blockoff=0, offset=0;
    unsigned char *fileindex = NULL;
    unsigned int i, fsize;
    FILE_INFO fileinfo;    
    write_data_file(outf, dir, "header.bin", 0);
    write_data_file(outf, dir, "version.bin", DASH_VERSION_OFFSET);
    write_data_file(outf, dir, "hash.bin", MASTER_HASH_OFFSET);
    fileindex_size = write_data_file(outf, dir, "file_index.bin", FILE_HEADER_OFFSET);

    fileindex = malloc(fileindex_size);

    fseek(outf, FILE_HEADER_OFFSET, SEEK_SET);
    fread(fileindex, 1, fileindex_size, outf);

    for(i=0; i < fileindex_size; i+=0x40)
    {
        memcpy(&fileinfo, fileindex+i, sizeof(FILE_INFO));
        blockoff = get_cluster(fileinfo.start_block);
        offset = FILE_START_OFFSET + (fileinfo.start_block + blockoff - 1) * BLOCK_SIZE;

        if (write_data_file(outf, dir, fileinfo.filename, offset) < 0) break;
        fsize = SWAP32(fileinfo.fsize);
        //printf("%s file size: %X\n", fileinfo.filename, fsize);
    }
    free(fileindex);
}

void write_hash(FILE *f, unsigned int hashoff, unsigned int hash_data_start, unsigned int hash_data_len, unsigned int ID)
{
    unsigned char tmpdata[BLOCK_SIZE];
    unsigned char tmphash[20];
    unsigned int pos, len = hash_data_len;
    SHA_CTX ctx;

    //fseek(f, hash_data_start, SEEK_SET);
    //fread(tmpdata, 1, hash_data_len, f);

    // compute hash

    if (hash_data_len != BLOCK_SIZE)
    {
    SHA1_Init(&ctx);
    for(pos = 0; pos < hash_data_len; pos += BLOCK_SIZE)
    {
        fseek(f, hash_data_start+pos, SEEK_SET);
        if (len >= BLOCK_SIZE) {
            fread(tmpdata, 1, BLOCK_SIZE, f);
            SHA1_Update(&ctx, tmpdata, BLOCK_SIZE);
            len-=BLOCK_SIZE;
        } else {
            fread(tmpdata, 1, len, f);
            SHA1_Update(&ctx, tmpdata, len);
        }
    }
    SHA1_Final(tmphash, &ctx);
    }
    else
    {
        fseek(f, hash_data_start, SEEK_SET);
        fread(tmpdata, 1, BLOCK_SIZE, f);
        SHA1(tmpdata, hash_data_len, tmphash);
    }

    //SHA1(tmpdata, hash_data_len, tmphash);
    fseek(f, hashoff, SEEK_SET);
    fwrite(tmphash, 1, 20, f);
    if (ID)
    {
        fwrite(&ID, 1, sizeof(unsigned int), f);
    }
}

unsigned int get_id(FILE *f, unsigned int block)
{
    unsigned int end_block, val = block;
    int fnum = 0;
    FILE_INFO fileinfo;

    if (block == 1) return 0x80FFFFFF;

    while(1) {
        fseek(f, FILE_HEADER_OFFSET+(fnum*0x40), SEEK_SET);
        fread(&fileinfo, 1, sizeof(FILE_INFO), f);

        if (fileinfo.filename[0] == 0x00) return 0;

        end_block = fileinfo.start_block+fileinfo.len1-1;

        if (block >= fileinfo.start_block && block <= end_block+1)
        {
            if (block == end_block+1) {
                 val = 0x00FFFFFF;
            }
            break;
        }

        fnum++;
    }

    return (val | 0x80000000);
}

void write_all_hash_blocks(FILE *f, unsigned int filesize)
{
    unsigned int i, j=1, off, blockoff;
    unsigned int idval, block = 1;

    off = BLOCK_HASH_START;
    for(i = 0; i < 0xAA; i++)
    {
        idval = get_id(f, block);
        write_hash(f, (off+24*i), (FILE_HEADER_OFFSET+(BLOCK_SIZE*i)), BLOCK_SIZE, SWAP32(idval));
        block++;
    }

    while(1)
    {
         off = FILE_START_OFFSET + (0xAA * BLOCK_SIZE * j) + (BLOCK_SIZE * (j-1));
         if (off > filesize) break;
         for(i = 0; i < 0xAA; i++)
         {
             idval = get_id(f, block);

             blockoff = FILE_START_OFFSET+(BLOCK_SIZE*((0xAB*j)+i));
             if (blockoff > (filesize-1)) break;

             write_hash(f, (off+24*i), blockoff, BLOCK_SIZE, SWAP32(idval));
             block++;
         }

         j++;
     }
}

int main(int argc, char **argv)
{
    unsigned int output_size;
    unsigned char buf[256];
    char c1, c2;
    unsigned int diffpos, pos = 0;
    FILE *file;
    if (argc < 4)
    {
        printf("sucreate <input dir> <xbox_su_file.bin> <output size>\n");
        return 0;
    }
    file = fopen(argv[2], "w+b");
    output_size = atoi(argv[3]);

    //    write_zero_data(file, output_size);

    write_misc_data(file);
    write_files(file, argv[1]);
    write_all_hash_blocks(file, output_size);
    //write top hash table hash
    write_hash(file, SECOND_HASH_OFFSET, SECOND_HASH_START, BLOCK_SIZE, 0);
    // write master hash
    write_hash(file, MASTER_HASH_OFFSET, HASH_DATA_START, HASH_DATA_LEN, 0);
    fclose(file);
    return 0;
}


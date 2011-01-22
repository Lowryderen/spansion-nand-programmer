#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    unsigned int cnt = 0;
    fseek(outf, offset, SEEK_SET);
    sprintf(filepath, "%s/%s", dir, filename);
    printf("writing %s to %X\n", filepath, offset);
    inf = fopen(filepath, "rb");
    if (inf == NULL)
    {
        printf("Error unable to open %s\n", filepath);
        exit(-1);
    }

    do
    {
        c = fgetc(inf);
        fputc(c,outf);
        cnt++;
    } while (!feof(inf));
    fclose(inf);
    return cnt;
}

write_zero_data(FILE *outf, unsigned int size)
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

void write_files(FILE *outf, char *dir)
{
    unsigned int fileindex_size, blockoff=0, offset=0;
    unsigned char *fileindex = NULL;
    unsigned int i;
    FILE_INFO fileinfo;    
    write_data_file(outf, dir, "header.bin", 0);
    write_data_file(outf, dir, "hash.bin", MASTER_HASH_OFFSET);
    fileindex_size = write_data_file(outf, dir, "file_index.bin", FILE_HEADER_OFFSET);

    fileindex = malloc(fileindex_size);

    fseek(outf, FILE_HEADER_OFFSET, SEEK_SET);
    fread(fileindex, 1, fileindex_size, outf);

    for(i=0; i < fileindex_size; i+=0x40)
    {
        memcpy(&fileinfo, fileindex+i, sizeof(FILE_INFO));
        blockoff = get_cluster(fileinfo.start_block);
        offset = FILE_START_OFFSET + (fileinfo.start_block + blockoff - 1) * 4096;

        write_data_file(outf, dir, fileinfo.filename, offset);
    }
    free(fileindex);
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
    write_zero_data(file, output_size);
    write_files(file, argv[1]);
    fclose(file);
    return 0;
}


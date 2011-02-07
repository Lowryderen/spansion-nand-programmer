#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#define MASTER_HASH_OFFSET 0x032C
#define HASH_DATA_START 0x344
#define HASH_DATA_END 0xB000
#define HASH_DATA_LEN (HASH_DATA_END-HASH_DATA_START)

#define DASH_VERSION_OFFSET 0x9722
#define BLOCK_SIZE 0x1000

#define SWAP32(x) ((unsigned int)((x>>24)|(x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x<<24))

unsigned int get_file_size(FILE *f)
{
    unsigned int val, back = ftell(f);
    fseek(f, 0, SEEK_END);
    val = ftell(f);
    fseek(f, back, SEEK_SET);
    return val;
}

void write_hash(FILE *f, unsigned int hashoff, unsigned int hash_data_start, unsigned int hash_data_len)
{
    unsigned char tmpdata[BLOCK_SIZE];
    unsigned char tmphash[20];
    unsigned int pos, len = hash_data_len;
    SHA_CTX ctx;

    //fseek(f, hash_data_start, SEEK_SET);
    //fread(tmpdata, 1, hash_data_len, f);

    // compute hash
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

    // write hash
    fseek(f, hashoff, SEEK_SET);
    fwrite(tmphash, 1, 20, f);
}

void write_dashboard_version(FILE *f, char *newdashstr)
{
    unsigned int dashver, vermajor, verminor, newdashver;

    fseek(f, DASH_VERSION_OFFSET, SEEK_SET); 
    fread(&dashver, 1, 4, f);
    vermajor = dashver&0xFF;
    verminor = (SWAP32(dashver)>>8)&0xFFFF;

    sscanf(newdashstr, "%d", &newdashver);

    // sanity check on newdashver
    if (newdashver < 1888 || newdashver > 13000)
    {
        printf("Invalid dashboard version %d\n", newdashver);
        exit(-1);
    }

    //printf("Offset %X Dashboard version: %X Major: %X Minor: %d, new version: %X\n", DASH_VERSION_OFFSET, dashver, vermajor, verminor, newdashver);
    printf("Erased old dashboard version %d\n", verminor);

    dashver = SWAP32(newdashver<<8) | vermajor;

    fseek(f, DASH_VERSION_OFFSET, SEEK_SET);
    fwrite(&dashver, 1, 4, f);
}

int main(int argc, char **argv)
{
    unsigned int output_size;
    unsigned char buf[256];
    char c1, c2;
    unsigned int diffpos, pos = 0;
    FILE *file;
    if (argc < 3)
    {
        printf("sudashver <xbox_su_file.bin> <xbox dashboard version>\n");
        return 0;
    }
    file = fopen(argv[1], "r+b");
    if (file == NULL)
    {
        printf("Failed to open file %s\n", argv[1]);
        return -1;
    }
    printf("Opened file %s\n", argv[1]);

    write_dashboard_version(file, argv[2]);
    // write master hash
    write_hash(file, MASTER_HASH_OFFSET, HASH_DATA_START, HASH_DATA_LEN);
    printf("Wrote new dashboard version %s\n", argv[2]);

    fclose(file);
    return 0;
}


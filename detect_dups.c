// add any other includes in the detetc_dups.h file
#include "detect_dups.h"

// define any other global variable you may need over here
#define MAX_LEN 4096
unsigned char md5_value[EVP_MAX_MD_SIZE];
unsigned int md5_len;

// open ssl, this will be used to get the hash of the file
EVP_MD_CTX *mdctx;
const EVP_MD *EVP_md5(); // use md5 hash!!

int main(int argc, char *argv[]) {
    // perform error handling, "exit" with failure incase an error occurs

    // initialize the other global variables you have, if any

    // add the nftw handler to explore the directory
    // nftw should invoke the render_file_info function
    if (argc != 2) {
        fprintf(stderr, "Usage: ./detect_dups <directory>\n");
        exit(EXIT_FAILURE);
    }
    if (nftw(argv[1], render_file_info, 20, FTW_PHYS) == -1) { // last 2 values passed into nftw used to be 20 and 0
        perror("nftw");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

// render the file information invoked by nftw
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    if (tflag == FTW_SL) {
        printf("Softlink\n");
    } else if (tflag == FTW_F) {
        printf("Regular file\n");
        hashFunction(fpath);
    }
    // perform the inode operations over here

    // invoke any function that you may need to render the file information
    return 0;
}

// add any other functions you may need over here
int hashFunction(const char * fpath) {
    // int j;
    
    int err;
    /* create a new MD5 context structure */
    mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        fprintf(stderr, "%s::%d::Error allocating MD5 context %d\n", __func__,
                __LINE__, errno);
        exit(EXIT_FAILURE);
    }
    md5_len = 0; /* the length is returned from the library */
    err = compute_file_hash(fpath, mdctx, md5_value, &md5_len);
    if (err < 0)
    {
        fprintf(stderr, "%s::%d::Error computing MD5 hash %d\n", __func__, __LINE__,
                errno);
        exit(EXIT_FAILURE);
    }
    printf("\tMD5 Hash: ");
    for (int i = 0; i < md5_len; i++)
    {
        printf("%02x", md5_value[i]);
    }
    printf("\n");
    EVP_MD_CTX_free(mdctx); // don't create a leak!
    return 0;
}
int compute_file_hash(const char *path, EVP_MD_CTX *mdctx, unsigned char *md_value,
                      unsigned int *md5_len) {
    FILE *fd = fopen(path, "rb");
    if (fd == NULL)
    {
        fprintf(stderr, "%s::%d::Error opening file %d: %s\n", __func__, __LINE__,
                errno, path);
        return -1;
    }
    char buff[MAX_LEN];
    size_t n;
    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
    while ((n = fread(buff, 1, MAX_LEN, fd)))
    {
        EVP_DigestUpdate(mdctx, buff, n);
    }
    EVP_DigestFinal_ex(mdctx, md_value, md5_len);
    EVP_MD_CTX_reset(mdctx);
    fclose(fd);
    return 0;
}
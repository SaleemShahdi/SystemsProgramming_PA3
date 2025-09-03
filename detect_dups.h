#define _XOPEN_SOURCE 500
#include <ftw.h>
/*
    Add any other includes you may need over here...
*/
#include <openssl/evp.h>
#include "uthash.h"
#include <errno.h>

// define the structure required to store the file paths

typedef struct PathNode{
    char *path;
    struct PathNode *next;
}PathNode;

typedef struct SoftLinkGroup{
    ino_t SLInode;
    int ref_count;
    PathNode *paths;
    struct SoftLinkGroup *next;
}SoftLinkGroup;

typedef struct HardLinkGroup{
    ino_t inode;
    int ref_count;
    PathNode *paths;
    SoftLinkGroup *softLinks;
    struct HardLinkGroup *next;
}HardLinkGroup;

typedef struct FileGroup{
    int fileNum;
    char md5str[33];
    HardLinkGroup *hards;
    struct FileGroup *next;
}FileGroup;

PathNode *unprocessedSL = NULL;

FileGroup *FGHead = NULL;

// process nftw files using this function
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf);

// add any other function you may need over here

char *compute_md5(const char *filename);

void addUnprocessed(const char *path);

FileGroup *getFG(char *md5);
HardLinkGroup *getHL(FileGroup *current, int currentInode);
void addPath(PathNode **head, const char* paths);
void processUnprocessedSoftLinks();
void printFileGroups();
void freePaths(PathNode *head);
void freeAll();
// add any other includes in the detetc_dups.h file
#include "detect_dups.h"

// define any other global variable you may need over here

// open ssl, this will be used to get the hash of the file
EVP_MD_CTX *mdctx;
const EVP_MD *EVP_md5(); // use md5 hash!!



int main(int argc, char *argv[]) {
    // perform error handling, "exit" with failure incase an error occurs
     if(argc < 2){
        printf("Usage: ./detect_dups <directory>\n");
        exit(EXIT_FAILURE);
    }
    struct stat sb;
    if (stat(argv[1], &sb) == -1 || !S_ISDIR(sb.st_mode)) {
        fprintf(stderr, "Error %d: %s is not a valid directory\n", errno, argv[1]);
        exit(EXIT_FAILURE);
    }


    // initialize the other global variables you have, if any

    // add the nftw handler to explore the directory
    nftw(argv[1],render_file_info, 100, FTW_PHYS);
    // nftw should invoke the render_file_info function


    //deal with unprocessed
    processUnprocessedSoftLinks();

    //print
    printFileGroups();

    //free
    freeAll();

    

}

// render the file information invoked by nftw
static int render_file_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    if(tflag == FTW_F){
        //get MD5
        char *md5 = compute_md5(fpath);
        if (md5 == NULL) {
            fprintf(stderr, "Failed to compute MD5 for: %s\n", fpath);
            return 0; // skip this file
        }

        FileGroup *curr = getFG(md5);
        if (curr == NULL) {
            fprintf(stderr, "Failed to allocate FileGroup for: %s\n", fpath);
            free(md5);
            return 0;
        }

        ino_t curIn = sb->st_ino;

        //get HardLink Group based on Inode
        HardLinkGroup *currHL = getHL(curr,curIn);
        if (!currHL) {
            fprintf(stderr, "getHL() returned NULL for inode %lu\n", (unsigned long)sb->st_ino);
            free(md5);
            return 0;
        }

        //Add path to HardLink group
        addPath(&currHL->paths,fpath);

        currHL->ref_count++;
        
        free(md5);

    }
    else if(tflag == FTW_SL){
        addUnprocessed(fpath);
    }

    return 0;
}

// add any other functions you may need over here

void addUnprocessed(const char *path){
    PathNode *newNode = malloc(sizeof(PathNode));
    newNode->path = strdup(path);
    if (!newNode->path) {
        free(newNode);
        return;
    }
    newNode->next = NULL;

    if(unprocessedSL == NULL){
        unprocessedSL = newNode;
    }
    else{
        PathNode *ptr = unprocessedSL;
        while(ptr->next != NULL){
            ptr = ptr->next;
        }

        ptr->next = newNode;
    }
}

FileGroup *getFG(char *md5){
    if(FGHead == NULL){ //LL does not exist
        FileGroup *newNode = malloc(sizeof(FileGroup));
        if (!newNode) return NULL;
        strcpy(newNode->md5str,md5);
        newNode->fileNum = 1;
        newNode->next = NULL;
        newNode->hards = NULL;  
        FGHead = newNode;
        return FGHead;
    }
    else{ //LL exists and need to traverse to check
        int count = 0;
        FileGroup *ptr = FGHead;
        FileGroup *prev = NULL;
        while(ptr != NULL){
            count++;
            if(strcmp(ptr->md5str,md5) == 0){
                return ptr;
            }
            prev = ptr;
            ptr = ptr->next;
        }

        //Node doesn't exist in LL yet
        FileGroup *newNode = malloc(sizeof(FileGroup));
        if (!newNode) return NULL;
        strcpy(newNode->md5str,md5);
        newNode->fileNum = count + 1;
        newNode->next = NULL;
        newNode->hards = NULL; 
        prev->next = newNode;
        return newNode;

    }
}

HardLinkGroup *getHL(FileGroup *current, int currentInode){ //ONLY TO CREATE AND/OR RETURN THE HARDLINK
    //should only malloc memory, set inode
    if(current->hards == NULL){ //LL does not exist
        HardLinkGroup *newNode = malloc(sizeof(HardLinkGroup));
        newNode->inode = currentInode;
        newNode->ref_count = 0;
        newNode->next = NULL;
        newNode->paths = NULL;
        newNode->softLinks = NULL;
        current->hards = newNode;
        return current->hards;
    }
    else{ //LL exists and need to traverse to check
        HardLinkGroup *ptr = current->hards;
        HardLinkGroup *prev = NULL;
        while(ptr != NULL){
            if(ptr->inode == currentInode){
                return ptr;
            }
            prev = ptr;
            ptr = ptr->next;
        }

        // Node doesn't exist in LL yet
        HardLinkGroup *newNode = malloc(sizeof(HardLinkGroup));
        if (!newNode) return NULL;
        newNode->inode = currentInode;
        newNode->ref_count = 0;
        newNode->paths = NULL;
        newNode->softLinks = NULL;
        newNode->next = NULL;

        if (prev) {
            prev->next = newNode;
        } else {
            current->hards = newNode;
        }
        return newNode;

    }
}


void addPath(PathNode **head, const char* paths) {
    if (head == NULL || paths == NULL) {
        fprintf(stderr, "addPath called with NULL pointer\n");
        return;
    }

    PathNode *newNode = malloc(sizeof(PathNode));
    if (!newNode) {
        perror("malloc failed in addPath");
        return;
    }

    newNode->path = strdup(paths);
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
    } else {
        PathNode *ptr = *head;
        while (ptr->next != NULL) {
            ptr = ptr->next;
        }
        ptr->next = newNode;
    }
}



char *compute_md5(const char *filename){
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    unsigned char buffer[1024];
    size_t bytes;

    char *md5_str = malloc(33);
    if (!md5_str) return NULL;

    mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        free(md5_str);
        return NULL;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        free(md5_str);
        return NULL;
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        EVP_MD_CTX_free(mdctx);
        free(md5_str);
        return NULL;
    }

    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        EVP_DigestUpdate(mdctx, buffer, bytes);
    }

    EVP_DigestFinal_ex(mdctx, md_value, &md_len);

    for (unsigned int i = 0; i < md_len; ++i) {
        sprintf(&md5_str[i * 2], "%02x", md_value[i]);
    }
    md5_str[32] = '\0';

    EVP_MD_CTX_free(mdctx);
    fclose(file);

    return md5_str;
}

void processUnprocessedSoftLinks() {
    PathNode *cur = unprocessedSL;
    while (cur != NULL) {
        char targetPath[PATH_MAX];
        ssize_t len = readlink(cur->path, targetPath, sizeof(targetPath) - 1);
        if (len == -1) {
            perror("readlink");
            cur = cur->next;
            continue;
        }

        targetPath[len] = '\0'; // Null-terminate

        struct stat sb;
        // Check if symlink target is valid
        if (stat(targetPath, &sb) == -1) {
            perror("stat on symlink target");
            cur = cur->next;
            continue;
        }

        // Skip if symlink points to a directory
        if (S_ISDIR(sb.st_mode)) {
            cur = cur->next;
            continue;
        }

        // Compute MD5 of target
        char *md5 = compute_md5(targetPath);
        if (!md5) {
            fprintf(stderr, "Failed to compute MD5 for symlink target: %s\n", targetPath);
            cur = cur->next;
            continue;
        }

        FileGroup *fg = getFG(md5);
        if (!fg) {
            fprintf(stderr, "getFG() returned NULL in processUnprocessedSoftLinks\n");
            free(md5);
            cur = cur->next;
            continue;
        }
        HardLinkGroup *hl = getHL(fg, sb.st_ino);

        if (!hl) {
            fprintf(stderr, "getHL() returned NULL in processUnprocessedSoftLinks\n");
            free(md5);
            cur = cur->next;
            continue;
        }

        // Check if SoftLinkGroup for this symlink inode already exists
        struct stat link_sb;
    if (lstat(cur->path, &link_sb) == -1) {
        perror("lstat on symlink itself");
        cur = cur->next;
        continue;
    }

    // Check if this specific symlink (by its own inode) already exists
    SoftLinkGroup *sl = hl->softLinks;
    SoftLinkGroup *prev = NULL;
    while (sl != NULL && sl->SLInode != link_sb.st_ino) {
        prev = sl;
        sl = sl->next;
    }

    if (sl == NULL) {
        // Create a new SoftLinkGroup with the symlink's own inode
        sl = malloc(sizeof(SoftLinkGroup));
        sl->SLInode = link_sb.st_ino;
        sl->ref_count = 0;
        sl->paths = NULL;
        sl->next = NULL;

        if (prev == NULL) {
            hl->softLinks = sl;
        } else {
            prev->next = sl;
        }
    }

            // Add path to the soft link group
            addPath(&sl->paths, cur->path);
            sl->ref_count++;

            free(md5);
            cur = cur->next;
        }
}

void printFileGroups() {
    FileGroup *fg = FGHead;

    while (fg != NULL) {
        printf("File %d\n", fg->fileNum);
        printf("\tMD5 Hash: %s\n", fg->md5str);

        HardLinkGroup *hl = fg->hards;
        while (hl != NULL) {
            printf("\t\tHard Link (%d): %lu\n", hl->ref_count, (unsigned long)hl->inode);
            printf("\t\t\tPaths:\t");

            PathNode *p = hl->paths;
            if (p != NULL) {
                printf("%s\n", p->path);
                p = p->next;
            }

            while (p != NULL) {
                printf("\t\t\t\t%s\n", p->path);
                p = p->next;
            }

            SoftLinkGroup *sl = hl->softLinks;
            int slIndex = 1;
            while (sl != NULL) {
                printf("\t\t\tSoft Link %d(%d): %lu\n", slIndex, sl->ref_count, (unsigned long)sl->SLInode);
                printf("\t\t\t\tPaths:\t");

                PathNode *sp = sl->paths;
                if (sp != NULL) {
                    printf("%s\n", sp->path);
                    sp = sp->next;
                }

                while (sp != NULL) {
                    printf("\t\t\t\t\t%s\n", sp->path);
                    sp = sp->next;
                }

                sl = sl->next;
                slIndex++;
            }

            hl = hl->next;
        }

        fg = fg->next;
    }
}

void freePaths(PathNode *head) {
    while (head != NULL) {
        PathNode *temp = head;
        head = head->next;
        free(temp->path);
        free(temp);
    }
}

void freeAll() {
    // Free unprocessed symlink path list
    freePaths(unprocessedSL);
    unprocessedSL = NULL;

    // Free the nested structure starting from FileGroup
    FileGroup *fg = FGHead;
    while (fg != NULL) {
        FileGroup *fgTemp = fg;
        fg = fg->next;

        HardLinkGroup *hl = fgTemp->hards;
        while (hl != NULL) {
            HardLinkGroup *hlTemp = hl;
            hl = hl->next;

            // Free paths under hard link
            freePaths(hlTemp->paths);

            // Free each SoftLinkGroup under this HardLinkGroup
            SoftLinkGroup *sl = hlTemp->softLinks;
            while (sl != NULL) {
                SoftLinkGroup *slTemp = sl;
                sl = sl->next;

                // Free paths under soft link
                freePaths(slTemp->paths);
                free(slTemp);
            }

            free(hlTemp);
        }

        free(fgTemp);
    }

    FGHead = NULL;
}
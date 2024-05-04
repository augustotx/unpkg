#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <getopt.h>

typedef struct {
    char* name;
    char* srcType;
    char* repo;
    char* outDir;
    char** deps;
    int deps_count;
    char** conflicts;
    int conflicts_count;
    char** commands;
    int commands_count;
} Package;

char** getPkgStringArray(lua_State *L, int index, int* count) {
    int len = luaL_len(L, index);
    char** array = (char**)malloc(sizeof(char*) * len);
    *count = len;
    for (int i = 0; i < len; i++) {
        lua_rawgeti(L, index, i + 1);
        array[i] = strdup(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return array;
}

int freePkgStringArray(char** array, int count) {
    for (int i = 0; i < count; i++) {
        free(array[i]);
    }
    free(array);
    return 0;
}

// Function to populate a Package struct from a Lua table
int getPkgInfo(lua_State *L, Package *pkg) {
    lua_getglobal(L, "pkg"); // Assuming the Lua table 'pkg' is a global variable
    if (!lua_istable(L, -1)) {
        printf("Error: 'pkg' is not a table\n");
        return 0;
    }

    lua_getfield(L, -1, "name");
    pkg->name = strdup(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "srcType");
    pkg->srcType = strdup(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "repo");
    pkg->repo = strdup(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "outDir");
    pkg->outDir = strdup(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, -1, "deps");
    pkg->deps = getPkgStringArray(L, lua_gettop(L), &(pkg->deps_count));
    lua_pop(L, 1);

    lua_getfield(L, -1, "conflicts");
    pkg->conflicts = getPkgStringArray(L, lua_gettop(L), &(pkg->conflicts_count));
    lua_pop(L, 1);

    lua_getfield(L, -1, "commands");
    pkg->commands = getPkgStringArray(L, lua_gettop(L), &(pkg->commands_count));
    lua_pop(L, 1);

    lua_pop(L, 1); // Pop 'pkg' table
    return 1;
}

// Remember to free allocated memory for the Package
void freePkgInfo(Package *pkg) {
    free(pkg->name);
    free(pkg->srcType);
    free(pkg->repo);
    free(pkg->outDir);
    freePkgStringArray(pkg->deps, pkg->deps_count);
    freePkgStringArray(pkg->conflicts, pkg->conflicts_count);
    freePkgStringArray(pkg->commands, pkg->commands_count);
}


lua_State *setupLua(){
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    // Modify package.path to include the path to the script directory
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path"); // get field "path" from table "package"
    const char *current_path = lua_tostring(L, -1); // grab path string
    lua_pop(L, 1); // pop the path string
    lua_pushfstring(L, "%s;%s%s/?.lua", current_path, getenv("UNPKG_ROOT"), "./unpkg/pkgs"); // append our path
    lua_setfield(L, -2, "path"); // set the field "path" in table "package"
    lua_pop(L, 1); // pop the package table
    return L;
}

int computeSHA256(const char *str, unsigned char hash[SHA256_DIGEST_LENGTH]) {
    SHA256((const unsigned char *)str, strlen(str), hash);
    return 0;
}

int compareSHA256(unsigned char hash[SHA256_DIGEST_LENGTH], unsigned char hash2[SHA256_DIGEST_LENGTH]){
    return memcmp(hash, hash2, SHA256_DIGEST_LENGTH) == 0;
}

int getSHA256(char * file, unsigned char hash[SHA256_DIGEST_LENGTH]){
    FILE *f = fopen(file, "rb");
    if (f == NULL) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    unsigned char buffer[SHA256_DIGEST_LENGTH];
    fread(buffer, 1, file_size, f);
    fclose(f);
    return memcpy(hash, buffer, file_size) == 0;
}

int syncWithNetwork(){
    // TODO
    return EXIT_FAILURE;
}

int checkDeps(char *pkg){
    // TODO
    return EXIT_SUCCESS;
}

int installPkg(char *pkg){
    lua_State *L = setupLua();

    char *pkgfile, *pkglua, *pkgsum;
    sprintf(pkgfile, "%s/unpkg/pkgs/%s", getenv("UNPKG_ROOT"), pkg);
    sprintf(pkglua, "%s.lua", pkgfile);
    sprintf(pkgsum, "%s.sha256", pkgfile);

    if (luaL_dofile(L, pkglua) != LUA_OK) {
        fprintf(stderr, "Error loading script: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return EXIT_FAILURE;
    }

    

    Package *pkgInfo;
    if (!getPkgInfo(L,pkgInfo))
        return EXIT_FAILURE;

    if(checkDeps(pkg) == EXIT_FAILURE){
        return EXIT_FAILURE;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH], localHash[SHA256_DIGEST_LENGTH];
    computeSHA256(pkgfile, hash);
    getSHA256(pkgsum, localHash);

    if (!compareSHA256(hash, localHash)){
        printf("Error comparing checksum for package %s", pkg);
    }
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    int opt;
    int long_index = 0;
    if (getenv("UNPKG_ROOT") == NULL)
        printf("UNPKG_ROOT is not set.\n");
    if (getenv("UNPKG_CACHE") == NULL)
        printf("UNPKG_CACHE is not set.\n");

    static struct option long_options[] = {
        {"install", required_argument, 0, 'i'},
        {"sync",    no_argument,       0, 's'},
        {0,         0,                 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "i:s", long_options, &long_index)) != -1) {

        switch (opt) {
            case 'i':
                return installPkg(optarg);
            case 's':
                return syncWithNetwork();
            case '?':
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    return 0;
}

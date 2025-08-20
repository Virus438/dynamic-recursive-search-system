#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include "plugin_api.h"

#define MAX_PLUGINS 64

typedef struct {
    void *handle;
    plugin_info_t *info;
    int (*process_file)(const char *, const option_t *);
    option_t *selected_options;
    int num_selected_options;
} plugin_t;

static plugin_t plugins[MAX_PLUGINS];
static int num_plugins = 0;

static int use_and = 1;
static int use_not = 0;

static const char *plugin_dir = NULL;

static void load_plugins(const char *dir) {
    DIR *dp = opendir(dir);
    if (!dp) {
        fprintf(stderr, "Cannot open plugin directory %s: %s\n", dir, strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (strstr(entry->d_name, ".so")) {
            char path[PATH_MAX];
            snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
            void *handle = dlopen(path, RTLD_NOW);
            if (!handle) {
                fprintf(stderr, "Failed to load plugin %s: %s\n", path, dlerror());
                continue;
            }
            plugin_info_t *(*get_info)();
            get_info = dlsym(handle, "plugin_get_info");
            int (*process_file)(const char *, const option_t *) = dlsym(handle, "plugin_process_file");
            if (!get_info || !process_file) {
                fprintf(stderr, "Invalid plugin interface in %s\n", path);
                dlclose(handle);
                continue;
            }
            if (num_plugins < MAX_PLUGINS) {
                plugins[num_plugins].handle = handle;
                plugins[num_plugins].info = get_info();
                plugins[num_plugins].process_file = process_file;
                plugins[num_plugins].selected_options = NULL;
                plugins[num_plugins].num_selected_options = 0;
                num_plugins++;
            }
        }
    }
    closedir(dp);
}

static void print_help(const char *prog_name) {
    printf("Usage: %s [options] [directory]\n", prog_name);
    printf("Options:\n");
    printf("  -P <dir>       Specify plugin directory\n");
    printf("  -A             Use AND logic (default)\n");
    printf("  -O             Use OR logic\n");
    printf("  -N             Invert match\n");
    printf("  -v             Print version and author info\n");
    printf("  -h             Show this help message\n");
    for (int i = 0; i < num_plugins; i++) {
        for (int j = 0; j < plugins[i].info->supplied_opt_count; j++) {
            printf("  --%s\t%s\n", plugins[i].info->supplied_opts[j].name, plugins[i].info->supplied_opts[j].description);
        }
    }
}

static void process_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        if (getenv("LAB1DEBUG")) fprintf(stderr, "Failed to open directory %s: %s\n", dir_path, strerror(errno));
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(path, &st) < 0) {
            if (getenv("LAB1DEBUG")) fprintf(stderr, "Cannot stat %s: %s\n", path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            process_directory(path);
        } else if (S_ISREG(st.st_mode)) {
            int result = use_and;
            for (int i = 0; i < num_plugins; i++) {
                for (int j = 0; j < plugins[i].num_selected_options; j++) {
                    int matched = plugins[i].process_file(path, &plugins[i].selected_options[j]);
                    if (use_and)
                        result &= matched;
                    else
                        result |= matched;
                }
            }
            if (use_not)
                result = !result;
            if (result)
                printf("%s\n", path);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    const char *search_dir = NULL;
    plugin_dir = getenv("LAB1PLUGINSDIR");
    if (!plugin_dir) plugin_dir = ".";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) {
            plugin_dir = argv[++i];
        } else if (strcmp(argv[i], "-A") == 0) {
            use_and = 1;
        } else if (strcmp(argv[i], "-O") == 0) {
            use_and = 0;
        } else if (strcmp(argv[i], "-N") == 0) {
            use_not = 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            printf("lab12ofbN3250 by Огом Фэйт Блессинг , группа N3250\n");
            return 0;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            // plugin options processed later
        } else {
            search_dir = argv[i];
        }
    }

    load_plugins(plugin_dir);
    if (!search_dir) {
        print_help(argv[0]);
        return 0;
    }

    process_directory(search_dir);
    return 0;
}


#ifndef PLUGIN_API_H
#define PLUGIN_API_H

typedef struct {
    const char *name;
    const char *description;
} plugin_option_t;

typedef struct {
    plugin_option_t opt;
    char *value;
    int is_set;
} option_t;

typedef struct {
    const char *plugin_name;
    const char *plugin_version;
    const char *plugin_author;
    const char *plugin_description;
    int supplied_opt_count;
    plugin_option_t *supplied_opts;
} plugin_info_t;

#endif


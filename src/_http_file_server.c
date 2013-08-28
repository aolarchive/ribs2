static inline void http_file_server_gzip_ext(struct http_file_server *fs, const char *ext) {
    hashtable_insert(&fs->ht_ext_whitelist, ext, strlen(ext), NULL, 0);
}


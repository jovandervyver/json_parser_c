#ifndef __JSON__PARSER__H__
#define __JSON__PARSER__H__

  #include <stddef.h>
  #include "hash_table.h"

  hash_table_t parse_json(
    const char* json,
    const size_t json_length);

  void free_json_hash_table(hash_table_t table);

#endif

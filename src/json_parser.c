#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "json_parser.h"

#define JSMN_STRICT 1
#include "jsmn.h"

#define DEFAULT_JSON_TOKENS_SIZE 32
#define MAXIMUM_JSON_TOKENS      10240

static int parse_json_(
    const char* json,
    const jsmntok_t* tokens,
    const char* base_path,
    hash_table_t table);

static int parse_json_tokens_(
    const char* json,
    const size_t json_length,
    jsmntok_t** result_tokens);


hash_table_t parse_json(
    const char* json,
    const size_t json_length) {

  if ((json == 0) || (json_length <= 0)) {
    return 0;
  }

  jsmntok_t* tokens = 0;
  const unsigned int tokens_length = parse_json_tokens_(json, json_length, &tokens);
  if (tokens_length > 0) {
    hash_table_t table = hash_table();
    const int result = parse_json_(json, tokens, 0, table);
    free(tokens);
    if (result >= 0) {
      return table;
    } else {
      free_json_hash_table(table);
      return 0;
    }
  } else if (tokens_length == 0) {
    return hash_table();
  } else {
    return 0;
  }
}

void free_json_hash_table(hash_table_t table) {
  if (table != 0) {
    hash_table_iterator_t iterator = hash_table_iterator(table);
    while(hash_table_next(iterator)) {
      free((char*) hash_table_key(iterator));
      void* value = hash_table_value(iterator);
      if (value != 0) free(value);
    }
    hash_table_free_iterator(iterator);
    hash_table_free(table);
  }
}

static char* json_token_path_cstring_(
    const char* base_path,
    char seperator,
    const char* json,
    const jsmntok_t *token) {

  if (token == 0) {
    return 0;
  }

  const int token_start = token->start;
  const int token_end = token->end;
  const int token_len = token_end - token_start;
  if ((token_start < 0) || (token_end < 0) || (token_len <= 0)) {
    return 0;
  }

  const int base_path_len = (base_path != 0) ? strlen(base_path) : 0;

  char* path;
  if (base_path_len <= 0) {
    path = malloc(sizeof(char) * (token_len + 1));
    memcpy(path, &json[token_start], sizeof(char) * token_len);
    path[token_len] = 0;
  } else {
    const unsigned int length = token_len + base_path_len + 1;
    path = malloc(sizeof(char) * (length + 1));
    memcpy(path, base_path, sizeof(char) * base_path_len);
    path[base_path_len] = seperator;
    memcpy(&path[base_path_len + 1], &json[token_start], sizeof(char) * token_len);
    path[length] = 0;
  }

  return path;
}

static inline char* json_token_cstring_(
    const char* json,
    const jsmntok_t *token) {

  return json_token_path_cstring_(0, 0, json, token);
}

static int parse_json_object_(
    const char* json,
    const jsmntok_t* tokens,
    const char* base_path,
    hash_table_t table) {

  if (tokens->type != JSMN_OBJECT) {
    return -1;
  }

  const int size = tokens->size;
  if (size <= 0) {
    return (size == 0) ? 1 : -1;
  }

  tokens = &tokens[1];
  int count = -1;
  while(++count < size) {
    if (tokens->type != JSMN_STRING) {
      return -1;
    }

    char* concat_key = json_token_path_cstring_(base_path, '.', json, tokens);
    const char* table_key = hash_table_get_key(table, concat_key);
    const char* final_key = concat_key;
    if (table_key != 0) {
      free(concat_key);
      final_key = table_key;
    }

    const jsmntok_t* value = &tokens[1];
    const int result = parse_json_(json, value, final_key, table);
    if (result < 0) {
      if (table_key == 0) {
        free(concat_key);
      }
      return result;
    }

    tokens = &tokens[result + 1];
  }

  return (size * 2) + 1;
}

static int parse_json_cstring_(
    const char* json,
    const jsmntok_t* tokens,
    const char* path,
    hash_table_t table) {

  char* value = json_token_cstring_(json, tokens);
  void* old = hash_table_put(table, path, value);
  if (old != 0) {
    free(old);
  }

  return 1;
}

static int parse_json_(
    const char* json,
    const jsmntok_t* tokens,
    const char* path,
    hash_table_t table) {

  if (tokens != 0) {
    switch(tokens[0].type) {
      case JSMN_OBJECT: {
        return parse_json_object_(json, tokens, path, table);
      }

      case JSMN_PRIMITIVE:
      case JSMN_STRING: {
        return parse_json_cstring_(json, tokens, path, table);
      }

      default: {
      }
    }
  }

  return -1;
}

static int parse_json_tokens_(
    const char* json,
    const size_t json_length,
    jsmntok_t** result_tokens) {

  jsmn_parser parser;
  jsmn_init(&parser);

  jsmntok_t* tokens = 0;
  int tokens_length = DEFAULT_JSON_TOKENS_SIZE;
  while(1) {
    tokens = realloc(tokens, sizeof(jsmntok_t) * tokens_length);
    const int parse_result = jsmn_parse(
      &parser, json, json_length, tokens, tokens_length - 1);

    if (parse_result < 0) {
      if ((parse_result == JSMN_ERROR_NOMEM) &&
          (tokens_length < MAXIMUM_JSON_TOKENS)) {

        tokens_length = tokens_length << 1;
      } else {
        free(tokens);
        return -1;
      }
    } else {
      tokens_length = parse_result;
      break;
    }
  }

  if (tokens_length != 0) {
    tokens[tokens_length].type = 0;
    tokens[tokens_length].start = -1;
    tokens[tokens_length].end = -1;
    tokens[tokens_length].size = -1;

    *result_tokens = tokens;
  } else {
    free(tokens);
  }

  return tokens_length;
}

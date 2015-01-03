#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

/*
 * The tokenizer structure. Contains the delimiters, the stream,
 * and a pointer to the current stream index.
 */
typedef struct tokenizer {
    char *delimiters;
    char *stream;
    char *stream_index;
} tokenizer_t;

/*
 * Constructor for a tokenizer_t. The caller is responsible
 * for using destroy(tokenizer_t) to free the allocated memory.
 */
tokenizer_t *create(char*, char*);

/*
 * Destructor for a tokenizer_t. This should be called when you
 * have finished using the tokenizer.
 */
void destroy(tokenizer_t*);

/*
 * Finds the next token, given a tokenizer_t, and returns it.
 */
char *next_token(tokenizer_t*);

#endif

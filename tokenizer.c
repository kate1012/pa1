#include "tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * A byte map used for converting a nibble into a hex character.
 */
static char byte_map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'};
static int byte_map_length = sizeof(byte_map);

/*
 * Converts a nibble (4 bits) to its hex character equivalent.
 */
static char nibble_to_char(u_int8_t nibble) {
    if (nibble < byte_map_length) return byte_map[nibble];
    return '*';
}

/*
 * A structure containing the expected character for a sequence,
 * and the actual escape sequence. Used for recognizing sequences
 * within a string.
 */
typedef struct escape_sequence {
    const char expected;
    const char actual;
} escape_sequence_t;

/*
 * Map of the escape sequences that we're using.
 */
static const escape_sequence_t escape_sequences[] = {
        {'t', '\t'}, {'n', '\n'}, {'v', '\v'},
        {'b', '\b'}, {'r', '\r'}, {'f', '\f'},
        {'a', '\a'}, {'\"', '\"'}, {'\\', '\\'}
};

/*
 * Gets an escape sequence given the character after the backslash.
 * i.e.: To get '\n', you would pass 'n' as the parameter.
 */
static char get_escape_sequence(char value) {
    int i = 0;
    for (; i < sizeof(escape_sequences) / sizeof(escape_sequence_t); i++) {
        escape_sequence_t esc_sequence = escape_sequences[i];
        if (esc_sequence.expected == value) {
            return esc_sequence.actual;
        }
    }
    return -1;
}

/*
 * Finds an escape character within a string, and returns a pointer to the index.
 * If it doesn't find anything, it returns a null pointer.
 */
static char *find_escape_character(char *string) {
    int i = 0;
    for (; i < sizeof(escape_sequences) / sizeof(escape_sequence_t); i++) {
        escape_sequence_t esc_sequence = escape_sequences[i];
        char *tmp = strchr(string, esc_sequence.actual);
        if (tmp != NULL) {
            return tmp;
        }

    }
    return NULL;
}

/*
 * Checks if a given character is a delimiter, given the tokenizer_t.
 */
static bool is_delimiter(tokenizer_t *tk, char c) {
    int i = 0;
    for (i = 0; i < strlen(tk->delimiters); i++) {
        char separator = tk->delimiters[i];
        if (c == separator) {
            return true;
        }
    }
    return false;
}

/*
 * Finds escape characters in a string and replaces them with their sequence equivalents. The
 * caller is responsible for freeing the allocated memory.
 * i.e.: 'h' 'e' 'l' '\' 'n' 'o' becomes 'h' 'e' 'l' '\n' 'o'
 */
static char *parse_escape_sequences(char *string) {
    /* we allocate new memory for the result and copy the string over */
    char *result = malloc(strlen(string));
    strcpy(result, string);

    char *backslash_index = result;
    /* iterates through the result, finding indices of a backslash. */
    /* each iteration starts from the previous backslash index (or the start of the string) */
    while ((backslash_index = strstr(backslash_index, "\\")) != NULL) {
        size_t index = backslash_index - result;
        size_t string_size = strlen(string);
        char *new_result;

        /* once we find a backslash, we check to see if the next character makes it an escape sequence */
        char escape_character = get_escape_sequence(result[index + sizeof(char)]);
        if (escape_character != -1) {
            /* we found an escape sequence, time to replace it */
            new_result = malloc(string_size - sizeof(char));
            strcpy(new_result, result);
            new_result[index] = escape_character;
            new_result[index + 1] = '\0';

            char *current_index = result + index + (2 * sizeof(char));
            char *end_index = result + string_size;
            if (current_index < end_index) {
                char *buf = malloc(end_index - current_index);
                strcpy(buf, current_index);
                strncat(new_result, buf, end_index - current_index);
                free(buf);
            }
        } else {
            /* it's not an escape sequence, so we truncate the backslash */
            new_result = malloc(string_size - sizeof(char));
            memcpy(new_result, result, index);
            new_result[index] = '\0';

            char *current_index = result + index + sizeof(char);
            char *end_index = result + string_size;
            if (current_index < end_index) {
                char *buf = malloc(end_index - current_index);
                strcpy(buf, current_index);
                strncat(new_result, buf, end_index - current_index);
                free(buf);
            }
        }
        free(result); /* free the previous result from memory, we don't need it anymore */
        result = new_result; /* set the result pointer to our new, updated result */
        backslash_index = new_result + index + sizeof(char); /* set last index to our result pointer plus the index */
    }
    return result;
}

/*
 * Parses a string token. This method replaces every escape character in a string
 * with their hex equivalents. The caller is responsible for freeing the allocated
 * memory.
 */
static char *parse_token(char *token) {
    char *escape_sequence_index;

    /* iterates through the token, finding indices of an escape sequence */
    while ((escape_sequence_index = find_escape_character(token)) != NULL) {
        /* we've found an escape sequence. time to replace it with its hex equivalent */
        size_t index = escape_sequence_index - token;
        size_t token_size = strlen(token);
        char *new_token;

        /* first, we allocate space for the new token */
        new_token = malloc(token_size + (5 * sizeof(char)));
        /* next, we copy over the beginning part of the token, up to the escape sequence */
        memcpy(new_token, token, index);
        new_token[index] = '\0';

        char escape_sequence = escape_sequence_index[0]; /* our escape sequence */

        /* now we use nibble_to_char to get the first and second hex characters for the sequence */
        /* this is done by passing the first four bits and the last four bits of the sequence, respectively */
        char first = nibble_to_char((u_int8_t) ((escape_sequence >> 4) & 0x0f));
        char second = nibble_to_char((u_int8_t) (escape_sequence & 0x0f));

        /* next, we append the hex characters to the end of our new token */
        char *buf = malloc(6 * sizeof(char));
        sprintf(buf, "[0x%c%c]", first, second);
        strncat(new_token, buf, 6 * sizeof(char));
        free(buf);

        /* finally, we re-append the characters that are after the escape sequence */
        char *current_index = token + index + sizeof(char);
        char *end_index = current_index + token_size;
        if (current_index < end_index) {
            buf = malloc(end_index - current_index);
            strcpy(buf, current_index);
            strncat(new_token, buf, end_index - current_index);
            free(buf);
        }
        free(token); /* free the previous token from memory, we don't need it anymore */
        token = new_token; /* set the token pointer to our new, updated token */
    }
    return token;
}

/*
 * Finds the next token, given a tokenizer_t, and returns it. The caller is responsible
 * for freeing the allocated memory.
 */
char *next_token(tokenizer_t *tk) {
    char *start_index = tk->stream_index;
    char *current_index = start_index;
    char *end_index = start_index + strlen(tk->stream_index);
    if (start_index == end_index) { //TODO: is this necessary?
        return NULL;
    }
    /* iterates through the string, finding indices of a delimiter character */
    while (current_index < end_index + 1) {
        char current_char = current_index[0];
        if (is_delimiter(tk, current_char)) {
            break; /* we've found our delimiter index */
        }
        current_index += 1;
    }
    /* now we allocate space for the token */
    size_t token_size = current_index - start_index;
    char *token = malloc(token_size);
    /* next we copy over memory from the beginning index to the separator index */
    memcpy(token, tk->stream_index, token_size);
    token[token_size] = '\0';
    /* we add one to the stream index (in order to ignore the delimiter) */
    tk->stream_index = current_index + 1;
    return parse_token(token); /* finally, we parse the token's escape sequences and return it */
}

/*
 * Constructor for a tokenizer_t. The caller is responsible
 * for using destroy(tokenizer_t) to free the allocated memory.
 */
tokenizer_t *create(char *delimiters, char *stream) {
    /* we allocate a tokenizer based upon the size of a tokenizer_t */
    tokenizer_t *tokenizer = malloc(sizeof(tokenizer_t));
    /* next, we replace any escape characters in the delimiters and the stream */
    tokenizer->delimiters = parse_escape_sequences(delimiters);
    tokenizer->stream = parse_escape_sequences(stream);
    /* we set the default stream index to the beginning of the stream */
    tokenizer->stream_index = tokenizer->stream;
    return tokenizer;
}

/*
 * Destructor for a tokenizer_t. This should be called when you
 * have finished using the tokenizer.
 */
void destroy(tokenizer_t *tk) {
    free(tk->delimiters);
    free(tk->stream);
    free(tk);
}

int main(int argc, char **argv) {
   // printf("creating tokenizer\n");
	if(argc < 3) {
		printf("too few arguments\n");
		exit(0);
	}
	if(argc > 3) {
		printf("too many arguments\n");
		exit(0);
	}
    tokenizer_t *tokenizer = create(argv[1], argv[2]);
   // printf("getting first token\n");
    char *current_token = next_token(tokenizer);
    while (current_token) {
        if (strlen(current_token) > 0) {
            /* we only care about the token if its length is not 0 */
            printf("got token: %s\n", current_token);
        }
        free(current_token);
        current_token = next_token(tokenizer);
    }
   // printf("freeing tokenizer from memory...\n");
    destroy(tokenizer);
   // printf("complete!\n");
    return 0;
}

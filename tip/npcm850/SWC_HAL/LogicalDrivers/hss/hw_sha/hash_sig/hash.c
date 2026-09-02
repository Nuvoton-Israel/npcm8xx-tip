#include <string.h>
#include "hash.h"


#define ALLOW_VERBOSE 0  /* 1 -> we allow the dumping of intermediate */
                         /*      states.  Useful for debugging; horrid */
                         /*      for security */

/*
 * This is the file that implements the hashing APIs we use internally.
 * At the present, our parameter sets support only one hash function
 * (SHA-256, using full 256 bit output), however, that is likely to change
 * in the future
 */
#include <stdio.h>
#if ALLOW_VERBOSE
#include <stdio.h>
#include <stdbool.h>
/*
 * Debugging flag; if this is set, we chat about what we're hashing, and what
 * the result is it's useful when debugging; however we probably don't want to
 * do this if we're multithreaded...
 */
bool hss_verbose = false;
#endif


extern ROM_NCL_SHA_Init			   pROM_NCL_SHA_Init;
extern ROM_NCL_SHA_Update		   pROM_NCL_SHA_Update;
extern ROM_NCL_SHA_Finish		   pROM_NCL_SHA_Finish;
extern ROM_NCL_SHA_Start		   pROM_NCL_SHA_Start;
/*
 * This will hash the message, given the hash type. It assumes that the result
 * buffer is large enough for the hash
 */
void hss_hash_ctx(void *result, int hash_type, union hash_context *ctx,
          const void *message, size_t message_len) {
#if ALLOW_VERBOSE
    if (hss_verbose) {
        int i; for (i=0; i< message_len; i++) printf( " %02x%s", ((unsigned char*)message)[i], (i%16 == 15) ? "\n" : "" );
    }
    printf("%d\n",message_len);
#endif
    switch (hash_type) {
    case HASH_SHA256: {
        pROM_NCL_SHA_Init( &ctx->ncl_sha );
        pROM_NCL_SHA_Start(&ctx->ncl_sha,NCL_SHA_TYPE_2_256);
        pROM_NCL_SHA_Update(&ctx->ncl_sha, message, message_len);
        pROM_NCL_SHA_Finish(&ctx->ncl_sha , result);
#if ALLOW_VERBOSE
        if (hss_verbose) {
            printf( " ->" );
            int i; for (i=0; i<32; i++) printf( " %02x", ((unsigned char *)result)[i] ); printf( "\n" );
        }
#endif
        break;
    }
    }
}

/*
 * This provides an API to do incremental hashing.  We use it when hashing the
 * message; since we don't know how long it could be, we don't want to
 * allocate a buffer that's long enough for that, plus the decoration we add
 */
void hss_init_hash_context(int h, union hash_context *ctx) {
    switch (h) {
    case HASH_SHA256:
        pROM_NCL_SHA_Init( &ctx->ncl_sha );
		pROM_NCL_SHA_Start( &ctx->ncl_sha , NCL_SHA_TYPE_2_256 );
        break;
    }
}
void hss_update_hash_context(int h, union hash_context *ctx,
                         const void *msg, size_t len_msg) {
#if ALLOW_VERBOSE
    if (hss_verbose) {
        int i; for (i=0; i<len_msg; i++) printf( " %02x", ((unsigned char*)msg)[i] );
    }
#endif
    switch (h) {
    case HASH_SHA256:
		pROM_NCL_SHA_Update (&ctx->ncl_sha, msg, len_msg);
        break;
    }
}

void hss_finalize_hash_context(int h, union hash_context *ctx, void *buffer) {
    switch (h) {
    case HASH_SHA256:
		pROM_NCL_SHA_Finish(&ctx->ncl_sha , buffer);

#if ALLOW_VERBOSE
    if (hss_verbose) {
        printf( " -->" );
        int i; for (i=0; i<32; i++) printf( " %02x", ((unsigned char*)buffer)[i] );
        printf( "\n" );
    }
#endif
        break;
    }
}



unsigned hss_hash_length(int hash_type) {
    switch (hash_type) {
    case HASH_SHA256: return 32;
    }
    return 0;
}

unsigned hss_hash_blocksize(int hash_type) {
    switch (hash_type) {
    case HASH_SHA256: return 64;
    }
    return 0;
}


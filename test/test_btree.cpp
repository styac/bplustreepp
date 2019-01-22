/*
 * Copyright (c) 2012 Jon Mayo <jon@cobra-kai.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "btree.h"

static int passes=0;
static int failures=0;

/* a copy of a very old rand() */
#define ANS_RAND_MAX 0x7fffffff

static unsigned long ans_seed;


static void
ans_srand(unsigned long seed)
{
    ans_seed = seed;
}

static unsigned
ans_rand(void)
{
    ans_seed = (1103515245 * ans_seed + 12345) & ANS_RAND_MAX;
    return ans_seed;
}

static const char *
randname(unsigned len)
{
    static char buf[64];
    const char set[] = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLM"
        "NOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    unsigned i;

    assert(len > 0);
    if (len >= sizeof(buf) )
        len = sizeof(buf) - 1;

    if (len <= 0) {
        fprintf(stdout, "ERROR:requested length is zero\n");
        return NULL;
    }
//    fprintf(stdout, "INFO:len=%d\n", len);

    for (i = 0; i < len; i++) {
        buf[i] = set[ans_rand() % (sizeof(set) - 1)];
    }
    buf[i] = 0;
    return buf;
}

static int
do_writes(btree *bt, int count, int min, int max, size_t datasize )
{
    const size_t maxdatasize = 1<<16;
    const size_t mindatasize = 1<<6;
    if( datasize > maxdatasize ) {
        datasize = maxdatasize;
    } else if( datasize < mindatasize ) {
        datasize = mindatasize;
    }
    
    char * value = (char *)malloc(datasize);
    int i;

    ans_srand(0);
    for (i = 0; i < count; i++) {
         btval key, data;
        int rc;

        key.data = (void*)randname(ans_rand() % (max - min + 1) + min);
        key.size = strlen((char*)key.data);
        if (key.size <= 0) {
            fprintf(stdout, "ERROR:key is zero length\n");
            free(value);
            return 0;
        }
        data.data = value;
        data.size = snprintf(value, datasize, "%X", i);
        fprintf(stdout, "INFO:put key '%s'  ----  '%s'\n", (char*)key.data, (char*)data.data  );
        rc = btree_put(bt, &key, &data, 0);
        if (rc != BT_SUCCESS) {
            fprintf(stdout, "ERROR:failed to write key '%s'\n",
                (char*)key.data);
            free(value);
            return 0;
        }
    }
    free(value);
    return 1;
}


static int
do_writes_wprefix(btree *bt)
{
    char kvalue[64] = "0123456789ABCDEFGHIJKLMNHOPQRST";
    char dvalue[64] = "0x1x2x3x4x5x6x7x8x9xAxBxCxDxExFxGxHxIxJxKxLxMxNxHxOxPxQxRxSxTx";
    int i;

    ans_srand(0);
    btval key, data;
    key.data = kvalue;
    data.data = dvalue;
    for (i = 20; i > 4; --i) {
        int rc;
        key.size = i;
        data.size = 2*key.size;
        kvalue[key.size] = 0;
        dvalue[data.size] = 0;
        fprintf(stdout, "INFO:put key '%s'  ----  '%s'\n", (char*)key.data, (char*)data.data  );
        rc = btree_put(bt, &key, &data, 0);
        if (rc != BT_SUCCESS) {
            fprintf(stdout, "ERROR:failed to write key '%s'\n",
                (char*)key.data);
            return 0;
        }
    }

    return 1;
}


static int
do_reads(btree *bt, int count, int min, int max)
{
    char value[64];
    size_t value_len;
    int i;

    ans_srand(0);
    for (i = 0; i < count; i++) {
         btval key, data;
        int rc;

        key.data = (void*)randname(ans_rand() % (max - min + 1) + min);
        key.size = strlen((char*)key.data);
        if (key.size <= 0) {
            fprintf(stdout, "ERROR:key is zero length\n");
            return 0;
        }
        data.data = NULL;
        data.size = 0;
        value_len = snprintf(value, sizeof(value), "%X", i);
        fprintf(stdout, "INFO:get key '%s'\n", (char*)key.data);
        rc = btree_get(bt, &key, &data);
        if (rc != BT_SUCCESS) {
            fprintf(stdout, "ERROR:failed to read key '%s'\n",
                (char*)key.data);
            return 0;
        }
        if (value_len != data.size || memcmp(value, data.data, value_len)) {
            fprintf(stdout, "ERROR:unexpected value '%.*s' from key '%s'\n",
                (int)data.size, (char*)data.data,
                (char*)key.data);
            return 0;
        }
    }

    return 1;
}

static int
do_reads_cursor_all(btree *bt)
{
    int i;
    int count = 100;
    int rc;
    btval key, data;

    btree_txn * txn = btree_txn_begin( bt, 1 );
    if(txn==NULL) {
        fprintf(stdout, "ERROR:tx error\n");
        return 0;            
    }
    struct cursor *  cursor =  btree_txn_cursor_open( bt, txn);
    if(cursor==NULL) {
        fprintf(stdout, "ERROR:cursor error 1\n");
        return 0;            
    }
    fprintf(stdout, "INFO:btree_txn_cursor_open 1 ok\n");
    rc = btree_cursor_get(cursor, &key, &data, BT_FIRST);
    if(rc==BT_FAIL) {
        fprintf(stdout, "ERROR:cursor BT_CURSOR_FIRST\n");
        return 0;            
    }
    fprintf(stdout, "INFO:BT_CURSOR_FIRST ok\n");
    for (i = 0; i < count; i++) {
        rc = btree_cursor_get(cursor, &key, &data, BT_NEXT);
        if(rc==BT_FAIL) {
            fprintf(stdout, "ERROR:cursor BT_CURSOR_NEXT %d\n", i);
            return 0;            
        }               
    }
    fprintf(stdout, "INFO:BT_CURSOR_NEXT ok\n");
    btree_cursor_close(cursor);
    return 1;
}


static int
do_deletes(btree *bt, int count, int min, int max)
{
    int i;

    ans_srand(0);
    for (i = 0; i < count; i++) {
         btval key;
        int rc;

        key.data = (void*)randname(ans_rand() % (max - min + 1) + min);
        key.size = strlen((char*)key.data);
        if (key.size <= 0) {
            fprintf(stdout, "ERROR:key is zero length\n");
            return 0;
        }
        fprintf(stdout, "INFO:delete key '%s'\n", (char*)key.data);
        rc = btree_del(bt, &key, NULL);
        if (rc != BT_SUCCESS) {
            fprintf(stdout, "ERROR:failed to delete key '%s'\n",
                (char*)key.data);
            return 0;
        }
    }

    return 1;
}

static void test(const char *name, int status)
{
    if (status) {
        passes++;
        fprintf(stdout, "\n ** %s:%s\n\n", status ? "PASS" : "FAILURE", name);
    } else {
        failures++;
        fprintf(stdout, "\n ******************* %s:%s\n\n", status ? "PASS" : "FAILURE", name);
    }
}

static void report(void)
{
    fprintf(stdout, "RESULTS:total=%d failures=%d\n", passes + failures, failures);
}

int main()
{
    btree *bt;
    const char * filename = "/tmp/test-btree-ksjdhflksjdfh.db";
    remove(filename);

    fprintf(stdout, " -- PASS 1 --\n" );

    test("btree_open(BT_NOSYNC)", (bt = btree_open(filename, BT_NOSYNC, 0644)) != NULL);
    test("do_writes(50, 1, 33)", do_writes(bt, 50, 1, 33, 64));
    test("do_reads(50, 1, 33)", do_reads(bt, 50, 1, 33));
    test("do_deletes(50, 1, 33)", do_deletes(bt, 50, 1, 33));
    test("btree_close", (btree_close(bt),1));

    fprintf(stdout, " -- PASS 2 --\n" );

    test("btree_open(!BT_NOSYNC)", (bt = btree_open(filename, 0, 0644)) != NULL);
    test("do_writes(50, 1, 33)", do_writes(bt, 50, 1, 33, 64));
    test("do_reads(50, 1, 33)", do_reads(bt, 50, 1, 33));
    test("btree_close", (btree_close(bt),1));

    fprintf(stdout, " -- PASS 3 --\n" );

    test("btree_open(BT_RDONLY)", (bt = btree_open(filename, BT_RDONLY, 0644)) != NULL);
    test("do_reads(50, 1, 33)", do_reads(bt, 50, 1, 33));
    test("btree_close", (btree_close(bt),1));
    test("btree_open(!BT_NOSYNC)", (bt = btree_open(filename, 0, 0644)) != NULL);
    test("do_deletes(50, 1, 33)", do_deletes(bt, 50, 1, 33));
    test("btree_close", (btree_close(bt),1));

    fprintf(stdout, " -- PASS 4 --\n" );

    test("btree_open(BT_NOSYNC | BT_REVERSEKEY)", (bt = btree_open(filename, BT_NOSYNC | BT_REVERSEKEY, 0644)) != NULL);
    test("do_writes(50, 1, 33)", do_writes(bt, 50, 1, 33, 64));
    test("do_reads(50, 1, 33)", do_reads(bt, 50, 1, 33));
    test("do_deletes(50, 1, 33)", do_deletes(bt, 50, 1, 33));
    test("do_writes_wprefix", do_writes_wprefix(bt));

    test("btree_close", (btree_close(bt),1));


    struct stat st;
    stat(filename, &st);

    fprintf(stdout, " -- PASS 5 -- file size: %lu \n", st.st_size );

    test("btree_open(!BT_NOSYNC)", (bt = btree_open(filename, 0, 0644)) != NULL);
    test("btree_compact", (btree_compact(bt)==0));
    test("btree_close", (btree_close(bt),1));

    stat(filename, &st);
    fprintf(stdout, " -- PASS 6 -- file size: %lu \n", st.st_size );

    fprintf(stdout, " -- PASS 7 -- BIGDATA\n" );
    
    test("btree_open(BT_NOSYNC)", (bt = btree_open(filename, BT_NOSYNC, 0644)) != NULL);
    test("do_writes(50, 1, 33)", do_writes(bt, 50, 1, 33, 1<<16));
    test("do_reads(50, 1, 33)", do_reads(bt, 50, 1, 33));
    test("do_deletes(50, 1, 33)", do_deletes(bt, 50, 1, 33));
    test("btree_close", (btree_close(bt),1));
    
    fprintf(stdout, " -- PASS 8 - update\n" );
    
    test("btree_open(BT_NOSYNC)", (bt = btree_open(filename, BT_NOSYNC, 0644)) != NULL);
    test("do_writes(50, 1, 33)", do_writes(bt, 5000, 1, 33, 64));
    test("do_reads(50, 1, 33)", do_reads(bt, 50, 1, 33)); // owerrite may happen
    test("do_writes(50, 1, 33)", do_writes(bt, 1000, 1, 33, 64));
    test("do_reads(50, 1, 33)", do_reads(bt, 50, 1, 33));// owerrite may happen
    test("do_reads_cursor_all", do_reads_cursor_all(bt));    
    test("btree_close", (btree_close(bt),1));
    stat(filename, &st);
    fprintf(stdout, " -- PASS 8 -- file size: %lu \n", st.st_size );

    report();

    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

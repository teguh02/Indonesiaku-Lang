#ifndef indonesiaku_table_h
#define indonesiaku_table_h

#include "common.h"
#include "value.h"

typedef struct {
    ObjString* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;

// K8: dictionaries may be keyed by anything hashable (number, boolean, kosong,
// teks, tupel, instance), so they use a table keyed by Value. The VM's global,
// string-interning, method and instance-field tables stay keyed by ObjString*
// for speed — this is deliberately a separate type.
typedef struct {
    Value key;
    Value value;
    // The key's hash, kept so a probe compares hashes before running
    // _sama_ and a resize never re-runs _hash_ (both are user code).
    uint32_t hash;
    uint8_t state;   // 0 = empty, 1 = filled, 2 = tombstone
} ValueEntry;

typedef struct {
    int count;
    int capacity;
    ValueEntry* entries;
} ValueTable;

void initValueTable(ValueTable* table);
void freeValueTable(ValueTable* table);
bool valueTableGet(ValueTable* table, Value key, Value* value);
bool valueTableSet(ValueTable* table, Value key, Value value);
bool valueTableDelete(ValueTable* table, Value key, Value* removedKey);
void markValueTable(ValueTable* table);

void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value);
bool tableDelete(Table* table, ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);
void tableRemoveWhite(Table* table);
void markTable(Table* table);

#endif

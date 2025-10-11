/* 
* @file symtab.c
* @brief A Symbol table impletation for the Wisp programming language
*/
#include "symtab.h"
#include "src/arena.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

/* ----------------- Initialization & Cleanup ----------------- */

SymTab*
symtab_new(size_t initial_capacity, SymTab* parent, Arena* arena) {
    if (initial_capacity == 0) {
        initial_capacity = 16;
    }

    SymTab* st = calloc(1, sizeof(SymTab) + sizeof(Symbol*) * initial_capacity);

    if (!st) {
        return NULL;
    }

    st->symbol_count = 0;
    st->bucket_count = initial_capacity;
    st->parent = parent;

    st->arena = arena;

    st->depth = (parent == NULL) ? 0 : parent->depth + 1;

    return st;
}


void
symtab_destroy(SymTab** table, bool recursive) {
    if (!table || !*table) return;

    if (!recursive){
        free(*table);
        *table = NULL;
        return;
    }

    SymTab* current = *table;
    while (current) {
        SymTab* next_to_free = current->parent;

        free(current);

        current = next_to_free;
    }
    *table = NULL;
}

int
symtab_define(SymTab* table, const char* name, size_t name_len,
                  SymbolType type, SymbolValue value,
                  SymbolFlags flags) {
    if (!table) return -1;
    if (!name || name_len == 0) return -2;

    uint32_t hash = symtab_hash(name, name_len);
    size_t index = hash % table->bucket_count;

    Symbol* current_sym = table->buckets[index];
    while (current_sym) {
        if (current_sym->name_hash == hash &&
            current_sym->name_len == name_len &&
            strncmp(current_sym->name, name, name_len) == 0)
        {
            if (current_sym->flags & SYM_FLAG_CONST) {
                return -3;
            }
            current_sym->type = type;
            current_sym->value = value;
            current_sym->flags = flags;
            return 0;
        }
        current_sym = current_sym->next;
    }

    Symbol* new_sym = arena_alloc( &table->arena, sizeof(Symbol));
    if (!new_sym) {
        return -4;
    }

    new_sym->name = arena_alloc(&table->arena, name_len + 1);
    memcpy((void*)name, new_sym->name,name_len);
    new_sym->name_len = name_len;
    new_sym->name_hash = hash;

    new_sym->type = type;

    new_sym->value = value;
    new_sym->flags = flags;

    new_sym->next = table->buckets[index];
    table->buckets[index] = new_sym;

    table->symbol_count++;

    return 0;
}

int symtab_set(SymTab* table, const char* name, size_t name_len,
               SymbolType type, SymbolValue value) {
    if (!table) return -1;
    if (!name || name_len == 0) return -1;

    uint32_t hash = symtab_hash(name, name_len);

    for (SymTab* current_scope = table; current_scope != NULL; current_scope = current_scope->parent) {
        size_t index = hash % current_scope->bucket_count;
        Symbol* sym = current_scope->buckets[index];

        while (sym) {
            if (sym->name_hash == hash &&
                sym->name_len == name_len &&
                memcmp(sym->name, name, name_len) == 0) {
                if (sym->flags & SYM_FLAG_CONST) {
                    return -3;
                }

                sym->type = type;

                switch (type) {
                    case SYM_STRING:
                        char* new_str = arena_alloc(&current_scope->arena, name_len + 1);
                        if (!new_str) {
                            return -4; ///< Allocation error
                        }
                        memcpy(new_str, value.str_val, name_len);
                        new_str[name_len] = '\0';
                        sym->value.str_val = new_str;
                        break;
                    default:
                        sym->value = value;
                        break;
                }
                return 0;
            }
            sym = sym->next;
        }
    }

    return -2;
}

Symbol* symtab_lookup(const SymTab* table, const char* name, size_t name_len) {
    if (!table || !name || name_len == 0) return NULL;

    uint32_t hash = symtab_hash(name, name_len);

    for (const SymTab* current_scope = table; current_scope != NULL; current_scope = current_scope->parent) {
        size_t index = hash % current_scope->bucket_count;
        Symbol* sym = current_scope->buckets[index];

        while (sym) {
            if (sym->name_hash == hash &&
                sym->name_len == name_len &&
                memcmp(sym->name, name, name_len) == 0)
            {
                return sym;
            }
            sym = sym->next;
        }
    }

    return NULL;
}


Symbol* symtab_lookup_local(const SymTab* table, const char* name, const size_t name_len) {
    if (!table || !name || name_len <= 0) return NULL;

    uint32_t hash = symtab_hash(name, name_len);

    size_t index = hash % table->bucket_count;
    Symbol* sym = table->buckets[index];

    while (sym) {
        if (sym->name_hash == hash &&
            sym->name_len == name_len &&
            memcmp(sym->name, name, name_len) == 0)
        {
            return sym;
        }
        sym = sym->next;
    }

    return NULL;
}


/* --------- Promise Helper ---------- */

/**
 * @brief Unregister a promise from the tracker
 * 
 * Called when a symbol with SYM_PROMISE is removed.
 */
void promise_unregister(promise* p) {
    if (!g_tracker || !p) return;

    promise** prev_next = &g_tracker->pending_head;
    promise* current = g_tracker->pending_head;
    bool found = false;

    while (current) {
        if (current == p) {
            *prev_next = current->next;
            found = true;
            break;
        }
        prev_next = &current->next;
        current = current->next;
    }

    if (!found) return;

    g_tracker->pending_count--;

    int status = aio_error(&p->cb);

    if (status == EINPROGRESS) {
        int cancel_status = aio_cancel(p->cb.aio_fildes, &p->cb);

        if (cancel_status == AIO_CANCELED) {
            status = aio_error(&p->cb);
        } else {
            for (int i = 0; i < 3; ++i) {
                status = aio_error(&p->cb);
                if (status != EINPROGRESS) break;

                const struct aiocb* list[1] = { &p->cb };
                struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };
                aio_suspend(list, 1, &timeout);
            }
        }
    }
    status = aio_error(&p->cb);
    if (status == EINPROGRESS) {
        fprintf(stderr, "WARNING: AIO operation for fd %d could not be cancelled. "
                        "Leaking promise and buffer to prevent crash.\n", p->cb.aio_fildes);
        return;
    }

    aio_return(&p->cb);

    if (p->cb.aio_fildes >= 0) {
        close(p->cb.aio_fildes);
    }

    if (p->buffer) {
        free(p->buffer);
    }

    free(p);
}

promise* promise_create(Arena** arena_ptr, int fd, size_t buffer_size, off_t offset,
                        Symbol* target_symbol, SymbolType result_type) {

    if (!arena_ptr || !*arena_ptr || fd < 0 || buffer_size == 0 || !target_symbol) {
        return NULL;
    }

    promise* p = arena_alloc(arena_ptr, sizeof(promise));
    if (!p) {
        return NULL;
    }

    p->buffer = arena_alloc(arena_ptr, buffer_size);
    if (!p->buffer) {
        return NULL;
    }

    memset(&p->cb, 0, sizeof(struct aiocb));

    p->buffer_size = buffer_size;
    p->target_symbol = target_symbol;
    p->result_type = result_type;
    p->next = NULL;

    p->cb.aio_fildes = fd;
    p->cb.aio_buf = p->buffer;
    p->cb.aio_nbytes = buffer_size;
    p->cb.aio_offset = offset;

    p->cb.aio_sigevent.sigev_notify = SIGEV_NONE;

    return p;
}

int symtab_remove(SymTab* table, const char* name, size_t name_len) {
    if (!table || !name || name_len == 0) return -1;
    if (table->bucket_count == 0) return -1;

    uint32_t hash = symtab_hash(name, name_len);
    size_t index = hash % table->bucket_count;

    Symbol* current = table->buckets[index];
    Symbol* prev = NULL;

    while (current) {
        if (current->name_hash == hash &&
            current->name_len == name_len &&
            memcmp(current->name, name, name_len) == 0)
        {
            if (current->flags & SYM_FLAG_CONST) {
                return -3;
            }

            if (prev) {
                prev->next = current->next;
            } else {
                table->buckets[index] = current->next;
            }

            switch (current->type) {
                case SYM_PROMISE:
                    if (current->value.promise) {
                        promise_unregister(current->value.promise);
                    }
                    break;
                default:
                    break;
            }

            free(current->name);
            free(current);

            table->symbol_count--;
            return 0;
        }

        prev = current;
        current = current->next;
    }
    return -2;
}


SymTab* symtab_push_scope(SymTab* parent) {
    if (!parent) return NULL;

    SymTab* new_st = symtab_new(parent->bucket_count, parent, parent->arena);
    if (!new_st) return NULL;

    new_st->parent = parent;

    return new_st;
}

SymTab* symtab_pop_scope(SymTab* table) {
    if (!table) return NULL;

    SymTab* parent = table->parent;

    arena_free(table->arena);

    symtab_destroy(&table, false);

    return parent;
}

size_t symtab_size(const SymTab* table) {
    if (!table) {
        return 0;
    }

    size_t total_count = 0;

    for (size_t i = 0; i < table->bucket_count; ++i) {
        Symbol* current = table->buckets[i];

        while (current != NULL) {
            total_count++;
            current = current->next;
        }
    }

    return total_count;
}

void symtab_dump(const SymTab* table) {
    if (!table) return;

    printf("\n---------------- SYMTAB DUMP (depth: %zu) ----------------\n", table->depth);

    for (size_t i = 0; i < table->bucket_count; i++) {
        Symbol* current = table->buckets[i];
        if (current) {
            printf("Bucket[%zu]:\n", i);
        }

        for (size_t j = 0; current != NULL; j++, current = current->next) {
            printf("  Symbol[%zu]:\n", j);
            printf("    - name:      '%.*s'\n", (int)current->name_len, current->name);
            printf("    - name_len:  %zu\n", current->name_len);
            printf("    - name_hash: %u\n", current->name_hash);
            printf("    - type:      %d\n", current->type);
            printf("    - value:     ");
            switch (current->type) {
                case SYM_INTEGER:   printf("%lld\n", current->value.int_val); break;
                case SYM_FLOAT:     printf("%f\n", current->value.float_val); break;
                case SYM_STRING:    printf("\"%s\"\n", current->value.str_val); break;
                case SYM_BOOLEAN:   printf("%s\n", current->value.bool_val ? "#t" : "#f"); break;
                case SYM_FUNCTION:  printf("<function> @%p\n", (void*)current->value.func_val); break;
                case SYM_BUILTIN:   printf("<builtin> @%p\n", (void*)current->value.ptr_val); break;
                case SYM_LIST:      printf("<list> @%p\n", (void*)current->value.list_val); break;
                case SYM_PROMISE:   printf("<promise> @%p\n", (void*)current->value.promise); break;
                case SYM_UNDEFINED: printf("<undefined>\n"); break;
                default:            printf("<unknown type>\n"); break;
            }
            printf("    - next:      %p\n", (void*)current->next);
        }
    }
    printf("---------------- END SYMTAB DUMP ----------------\n");
}

bool symtab_exists(const SymTab* table, const char* name, const size_t name_len) {
    if (!table || !name) return false;

    uint32_t hash = symtab_hash(name, name_len);
    size_t i = hash % table->bucket_count;

    Symbol* current_sym = table->buckets[i];
    while (current_sym) {
        if (current_sym->name_hash == hash &&
            current_sym->name_len == name_len &&
            memcmp(name, current_sym->name, current_sym->name_len)) {
            return true;
        }
        current_sym = current_sym->next;
    }
    return false;
}

#include "libkern.h"        // KERNEL_BASE_OR_GTFO, kernel_read
#include <stdio.h>          // printf
#include <stdlib.h>         // free, malloc
#include <string.h>         // strnlen
#include <stddef.h>         // offsetof

static void print_usage(const char* name) {
    fprintf(stderr, 
        "Usage:\n"
        "   %s mac_policy_list_offset\n",
        name);
}

struct mac_policy_ops {
    void* pad[122];
    void *mpo_iokit_check_nvram_get;
    void *mpo_iokit_check_nvram_set;
    void *mpo_iokit_check_nvram_delete;
};

typedef unsigned int u_int;

struct mac_policy_list {
    u_int numloaded;
    u_int max;
    u_int maxindex;
    u_int staticmax;
    u_int chunks;
    u_int freehint;
    struct mac_policy_list_element *entries;
};

struct mac_policy_list_element {
    struct mac_policy_conf *mpc;
};

struct mac_policy_conf {
    const char *mpc_name;
    const char *mpc_fullname;
    char const *const *mpc_labelnames;
    unsigned int mpc_labelname_count;
    struct mac_policy_ops *mpc_ops;
    int mpc_loadtime_flags;
    int *mpc_field_off;
    int mpc_runtime_flags;
    struct mac_policy_conf *mpc_list;
    void *mpc_data;
};

#define UNSLID_KERNEL_BASE 0xffffff8000200000
#define KERN_ADDR_MASK     0xffffff0000000000

#define KERNEL_READ_OR_GTFO(addr, len, buf) \
do \
{ \
    if (kernel_read(addr, len, buf) != len) { \
        fprintf(stderr, "kernel_read(0x%llx, 0x%zx) failed\n", (uint64_t) addr, (size_t) len); \
        return 1; \
    } \
} while(0)

size_t kstrlen(vm_address_t addr) {
    const u_int PSHFT = 10;
    const u_int PSIZE = 1 << PSHFT;
    const u_int PMASK = PSIZE - 1;

    char buff[0x100];
    size_t full_len = 0;

    while (1) {
        vm_size_t to_read = sizeof(buff);

        if ((addr & PMASK) + to_read > PSIZE) {
            to_read = PSIZE - (addr & PMASK) - 1;
        }

        if (kernel_read(addr, to_read, buff) != to_read) {
            fprintf(stderr, "kernel_read(0x%lx, 0x%zx) failed\n", addr, to_read);
            return -1;
        }

        size_t chunk_len = strnlen(buff, to_read);

        full_len += chunk_len;

        if (chunk_len != to_read) {
            break;
        }

        addr += to_read;
    }

    return full_len;
}

char* kstrdup(vm_address_t addr) {
    size_t len = kstrlen(addr);
    if (len == -1) {
        return NULL;
    }

    char *str = malloc(len + 1);
    if (str == NULL) {
        return NULL;
    }

    if (kernel_read(addr, len, str) != len) {
        fprintf(stderr, "kernel_read(0x%lx, 0x%zx) failed\n", addr, len);
        free(str);
        return NULL;
    }

    str[len] = '\0';

    return str;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    uint64_t kbase = 0;
    KERNEL_BASE_OR_GTFO(kbase);

    uint64_t mac_policy_list_addr = 0;
    if (sscanf(argv[1], " %llx", &mac_policy_list_addr) != 1) {
        fprintf(stderr, "%s doesn't look like hex integer\n", argv[1]);
        return 1;
    }

    if (mac_policy_list_addr & KERN_ADDR_MASK) {
        fprintf(stderr, "Assuming 0x%llx isn't offset but unslid address\n", mac_policy_list_addr);
        mac_policy_list_addr -= UNSLID_KERNEL_BASE;
        fprintf(stderr, "Offset is 0x%llx\n", mac_policy_list_addr);
    }

    mac_policy_list_addr += kbase;

    struct mac_policy_list list;
    KERNEL_READ_OR_GTFO(mac_policy_list_addr, sizeof(list), &list);
    printf("%u policies loaded\n", list.numloaded);

    for (int i = 0; i != list.numloaded; ++i) {
        vm_address_t entries_addr = (vm_address_t) list.entries;
        vm_address_t policy_addr = 0;
        KERNEL_READ_OR_GTFO(entries_addr + sizeof(struct mac_policy_list_element) * i, sizeof(policy_addr), &policy_addr);

        struct mac_policy_conf policy;
        KERNEL_READ_OR_GTFO(policy_addr, sizeof(policy), &policy);

        char *policy_name = kstrdup((vm_address_t) policy.mpc_name);
        printf("Policy #%d: '%s'\n", i + 1, policy_name);

        if (strcmp(policy_name, "Sandbox") == 0) {
            printf("Found Sandbox\n");
            vm_address_t mpc_ops_addr = 0;
            KERNEL_READ_OR_GTFO(policy_addr + offsetof(struct mac_policy_conf, mpc_ops), sizeof(mpc_ops_addr), &mpc_ops_addr);

            printf("Policies at 0x%llx\n", (uint64_t) mpc_ops_addr);

            void* nullp = NULL;
            size_t wrote = 0;

            wrote += kernel_write(mpc_ops_addr + offsetof(struct mac_policy_ops, mpo_iokit_check_nvram_get), sizeof(nullp), &nullp);
            wrote += kernel_write(mpc_ops_addr + offsetof(struct mac_policy_ops, mpo_iokit_check_nvram_set), sizeof(nullp), &nullp);
            wrote += kernel_write(mpc_ops_addr + offsetof(struct mac_policy_ops, mpo_iokit_check_nvram_delete), sizeof(nullp), &nullp);

            if (wrote != sizeof(nullp) * 3) {
                printf("Nulling out policies failed!\n");
            }

            printf("Successfully nulled out Sandbox nvram policies\n");
        }

        free(policy_name);
    }

    return 0;
}
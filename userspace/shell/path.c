#include <uapi/errno.h>
#include <userspace/libc/string.h>
#include <userspace/shell/path.h>

static void drop_component(const char* output, u32* len) {
    while (*len > 0u && output[*len - 1u] != '/')
        (*len)--;

    if (*len > 0u)
        (*len)--;
}

i32 path_resolve(const char* cwd, const char* input, char* output, u32 size) {
    u32 len = 0;
    if (*input == '/') {
        input++;
    } else {
        u32 base = (u32)strlen(cwd);
        if (base >= size)
            return -(i32)E_NAMELONG;

        memcpy(output, cwd, base);
        len = (base > 1u) ? base : 0u;
    }

    for (;;) {
        while (*input == '/')
            input++;

        if (*input == '\0')
            break;

        const char* component = input;
        u32 clen = 0;
        while (component[clen] != '\0' && component[clen] != '/')
            clen++;

        input += clen;
        if (clen == 1u && component[0] == '.')
            continue;

        if (clen == 2u && component[0] == '.' && component[1] == '.') {
            drop_component(output, &len);
            continue;
        }

        if (len + clen + 2u > size)
            return -(i32)E_NAMELONG;

        output[len++] = '/';
        memcpy(output + len, component, clen);
        len += clen;
    }

    if (len == 0)
        output[len++] = '/';

    output[len] = '\0';
    return E_OK;
}

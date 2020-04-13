#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096


// original code was written for x86
// modification are for arm.
struct asmbuf {
    uint8_t code[PAGE_SIZE - sizeof(uint64_t)];
    uint64_t count;
};

struct asmbuf *
asmbuf_create(void)
{
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_ANONYMOUS | MAP_PRIVATE;
    return mmap(NULL, PAGE_SIZE, prot, flags, -1, 0);
}

void
asmbuf_free(struct asmbuf *buf)
{
    munmap(buf, PAGE_SIZE);
}

void
asmbuf_finalize(struct asmbuf *buf)
{
    mprotect(buf, PAGE_SIZE, PROT_READ | PROT_EXEC);
}

// In ARM code is also little endian
// So no reverse copy.
void
asmbuf_ins(struct asmbuf *buf, int size, uint64_t ins)
{
    for ( int i = 0 ; i < size ; i++ )
        buf->code[buf->count++] = (ins >> (i * 8)) & 0xff;
}

void
asmbuf_immediate(struct asmbuf *buf, int size, const unsigned long int *value)
{
// REM : need to handle the case where immediate value greater than 0xFFFF
    unsigned int mov_x1_imm = 0xD2800001 ;
    unsigned int inst ;
    if (*value <= 0xffff ) {
    	inst = mov_x1_imm | (*value<<5) ;
    	asmbuf_ins(buf, 4 , inst );   
    }   
}

int main(void)
{

    /* Compile input program */
    struct asmbuf *buf = asmbuf_create();

    // as per ARM abi x0 is the first argument also the return value.
    //asmbuf_ins(buf, 3, 0x4889f8); // mov   %rdi, %rax
    int c;
    while ((c = fgetc(stdin)) != '\n' && c != EOF) {
        if (c == ' ')
            continue;
        char operator = c;
        unsigned long int operand;
        scanf("%lu", &operand);
        asmbuf_immediate(buf, 4 , &operand);
        switch (operator) {
        case '+':
            asmbuf_ins(buf, 4 , 0x8B000020);   // add   x0 , x0 , x1
            break;
        case '-':
            asmbuf_ins(buf, 4 , 0xCB010000);   // sub   x0 , x0 , x1
            break;
        case '*':
            asmbuf_ins(buf, 4 , 0x9B007C20);   // mul  x0 , x0 , x1
            break;
        case '/':
            asmbuf_ins(buf, 4 , 0x9AC00820);   // div  x0 , x0 , x1
            break;
        }
    }
    asmbuf_ins(buf,4, 0xD65F03C0 ) ; // ret 
    asmbuf_finalize(buf);

    long init;
    unsigned long term;
    scanf("%ld %lu", &init, &term);
    long (*recurrence)(long) = (void *)buf->code;
    for (unsigned long i = 0, x = init; i <= term; i++, x = recurrence(x))
        fprintf(stderr, "Term %lu: %ld\n", i, x);
    asmbuf_free(buf);
    return 0;
}

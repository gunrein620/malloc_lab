/*
 * mm-naive.c - The fastest, least memory-efficient malloc package. / mm-naive.c - 가장 빠르지만 메모리 효율은 가장 낮은 malloc 패키지.
 *
 * In this naive approach, a block is allocated by simply incrementing / 이 단순한 방식에서는 블록을 단순히 증가시키는 방식으로 할당한다.
 * the brk pointer.  A block is pure payload. There are no headers or / brk 포인터를 증가시킨다. 블록은 순수 payload만 가지며, 헤더나
 * footers.  Blocks are never coalesced or reused. Realloc is / 푸터가 없다. 블록은 병합되거나 재사용되지 않으며, realloc은
 * implemented directly using mm_malloc and mm_free. / mm_malloc과 mm_free를 직접 사용해 구현된다.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header / 학생들에게: 이 헤더 주석을 여러분의 방식에 대한
 * comment that gives a high level description of your solution. / 상위 수준 설명을 담은 주석으로 바꾸세요.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please / 학생들에게: 다른 작업을 하기 전에 먼저
 * provide your team information in the following struct. / 아래 구조체에 팀 정보를 입력하세요.
 ********************************************************/
team_t team = {
    /* Team name / 팀 이름 */
    "ateam",
    /* First member's full name / 첫 번째 팀원의 전체 이름 */
    "Harry Bovik",
    /* First member's email address / 첫 번째 팀원의 이메일 주소 */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) / 두 번째 팀원의 전체 이름 (없으면 비워 두세요) */
    "",
    /* Second member's email address (leave blank if none) / 두 번째 팀원의 이메일 주소 (없으면 비워 두세요) */
    ""};

/* single word (4) or double word (8) alignment / single word(4) 또는 double word(8) 단위 정렬 */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT / ALIGNMENT의 가장 가까운 배수로 올림 */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

static char *heap_listp;

/*
 * mm_init - initialize the malloc package. / mm_init - malloc 패키지를 초기화한다.
 */
int mm_init(void)
{
    
    
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer. / mm_malloc - brk 포인터를 증가시켜 블록을 할당한다.
 *     Always allocate a block whose size is a multiple of the alignment. / 항상 정렬 단위의 배수 크기를 갖는 블록을 할당한다.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing. / mm_free - 블록을 해제해도 아무 일도 하지 않는다.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free / mm_realloc - mm_malloc과 mm_free를 이용해 단순하게 구현된다.
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
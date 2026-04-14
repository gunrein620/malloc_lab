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
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);

/*
 * extend_heap - 힙을 words 워드만큼 늘리고 새 free block을 만든다.
 */
static void *extend_heap(size_t words)
{
    char *bp;                                                 /* 새 free block의 payload 시작 주소 */
    size_t size;                                              /* 실제로 늘릴 바이트 수 */

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; /* 8-byte 정렬을 위해 홀수 word면 1 word 추가 */

    if ((bp = mem_sbrk(size)) == (void *)-1)                  /* 힙 확장 실패 시 NULL 반환 */
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));                             /* 새 free block의 header 기록 */
    PUT(FTRP(bp), PACK(size, 0));                             /* 새 free block의 footer 기록 */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));                     /* 힙 끝에 새 epilogue header 기록 */

    return coalesce(bp);                                      /* 앞뒤 free block과 합칠 수 있으면 합쳐서 반환 */
}

/*
 * coalesce - 인접한 free block이 있으면 현재 block과 합친다.
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));        /* 이전 블록의 footer를 보고 할당 여부 확인 */
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));        /* 다음 블록의 header를 보고 할당 여부 확인 */
    size_t size = GET_SIZE(HDRP(bp));                          /* 현재 블록의 전체 크기 */

    if (prev_alloc && next_alloc)                              /* 이전/다음이 둘 다 allocated면 합칠 블록이 없음 */
        return bp;

    else if (prev_alloc && !next_alloc)                        /* 다음 블록만 free면 현재 + 다음을 합침 */
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));                 /* 다음 블록 크기를 현재 크기에 더함 */
        PUT(HDRP(bp), PACK(size, 0));                          /* 합쳐진 블록의 새 header 기록 */
        PUT(FTRP(bp), PACK(size, 0));                          /* 합쳐진 블록의 새 footer 기록 */
    }

    else if (!prev_alloc && next_alloc)                        /* 이전 블록만 free면 이전 + 현재를 합침 */
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));                 /* 이전 블록 크기를 현재 크기에 더함 */
        PUT(FTRP(bp), PACK(size, 0));                          /* 합쳐진 블록의 끝 footer를 새 크기로 기록 */
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));               /* 이전 블록 header를 합쳐진 블록의 시작 header로 갱신 */
        bp = PREV_BLKP(bp);                                    /* 반환 포인터는 합쳐진 블록의 시작 payload로 이동 */
    }

    else                                                       /* 이전/다음이 모두 free면 세 블록을 한 번에 합침 */
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); /* 이전과 다음 블록 크기를 모두 더함 */
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));               /* 합쳐진 큰 블록의 시작 header 기록 */
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));               /* 합쳐진 큰 블록의 끝 footer 기록 */
        bp = PREV_BLKP(bp);                                    /* 반환 포인터는 가장 왼쪽 블록의 payload가 됨 */
    }

    return bp;                                                 /* 최종 free block의 payload 주소 반환 */
}

/*
 * find_fit - first-fit 방식으로 asize 이상인 free block을 찾는다.
 */
static void *find_fit(size_t asize)
{
    void *bp;                                                  /* 힙을 순회할 현재 block의 payload 주소 */

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) /* epilogue를 만날 때까지 한 블록씩 전진 */
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))     /* free 상태이고 요청 크기 이상이면 */
            return bp;                                                 /* 이 블록을 즉시 반환(first-fit) */
    }

    return NULL;                                                /* 끝까지 못 찾았으면 맞는 free block이 없음 */
}

/*
 * mm_init - initialize the malloc package. / mm_init - malloc 패키지를 초기화한다.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) /* 패딩/프롤로그/에필로그를 위한 초기 힙 공간 확보 */
        return -1;

    PUT(heap_listp, 0);                             /* 정렬을 맞추기 위한 패딩 워드 */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* 프롤로그 헤더(크기 DSIZE, 할당됨) */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* 프롤로그 푸터(크기 DSIZE, 할당됨) */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* 초기 에필로그 헤더(크기 0, 할당됨) */
    heap_listp += (2 * WSIZE);                     /* heap_listp를 프롤로그 payload 위치로 이동 */

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)    /* 첫 free block을 만들기 위해 힙 확장 */
        return -1;

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

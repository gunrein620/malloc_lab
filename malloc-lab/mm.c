/*
 * mm.c - implicit free list + boundary tag allocator. / mm.c - implicit free list와 boundary tag를 사용하는 allocator.
 *
 * Each block stores its total size and allocation bit in both the / 각 블록은 전체 크기와 할당 여부를 헤더와 푸터에 함께 저장한다.
 * header and footer. malloc uses first-fit search, free coalesces / malloc은 first-fit으로 탐색하고, free는 인접 free block을 병합하며,
 * adjacent free blocks, and place splits a large free block when / place는 충분히 큰 free block을 요청 크기에 맞게 쪼개서 사용한다.
 * needed. / 필요할 때만 split한다.
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

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))                    /* starter 코드에서 쓰던 정렬된 size_t 크기 */

#define WSIZE 4                                               /* header/footer 한 칸의 크기(word size) */
#define DSIZE 8                                               /* double word 크기이자 기본 정렬 단위 */
#define CHUNKSIZE (1 << 12)                                   /* 힙이 부족할 때 기본으로 4096바이트를 추가로 확보 */
#define PACK(size, alloc) ((size) | (alloc))                  /* size와 alloc bit를 한 워드로 합친 값 생성 */
#define GET(p) (*(unsigned int *)(p))                         /* 주소 p에 저장된 header/footer 값을 읽음 */
#define PUT(p, val) (*(unsigned int *)(p) = (val))            /* 주소 p에 header/footer 값을 기록 */
#define GET_SIZE(p) (GET(p) & ~0x7)                           /* header/footer에서 하위 alloc 비트를 제거하고 size만 추출 */
#define GET_ALLOC(p) (GET(p) & 0x1)                           /* header/footer에서 마지막 1비트만 읽어 alloc 여부 확인 */
#define HDRP(bp) ((char *)(bp) - WSIZE)                       /* payload 포인터 bp로부터 현재 블록의 header 주소 계산 */
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)  /* payload 포인터 bp로부터 현재 블록의 footer 주소 계산 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))     /* 현재 블록 크기만큼 전진해서 다음 블록 payload 주소 계산 */
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)) /* 이전 블록 footer의 size를 이용해 이전 블록 payload 주소 계산 */

static char *heap_listp;                                      /* implicit free list 순회의 시작점으로 쓰는 포인터 */
static void *extend_heap(size_t words);                       /* 힙을 늘려 새 free block을 만드는 helper */
static void *coalesce(void *bp);                              /* 현재 free block을 앞뒤 free block과 합치는 helper */
static void *find_fit(size_t asize);                          /* 요청 크기에 맞는 free block을 찾는 helper */
static void place(void *bp, size_t asize);                    /* 찾은 free block에 실제 할당을 배치하는 helper */

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
            return bp;                                                 /* 현재 free block이 first-fit 결과가 되므로 즉시 반환 */
    }

    return NULL;                                                /* 끝까지 못 찾았으면 맞는 free block이 없음 */
}

/*
 * place - 찾은 free block에 요청 블록을 배치하고, 남는 공간이 충분하면 split한다.
 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));                          /* 현재 free block의 전체 크기 */

    if ((csize - asize) >= (2 * DSIZE))                        /* 남는 조각이 최소 block 크기 이상이면 split 수행 */
    {
        PUT(HDRP(bp), PACK(asize, 1));                         /* 앞부분을 allocated block으로 표시 */
        PUT(FTRP(bp), PACK(asize, 1));                         /* 앞부분 footer도 allocated로 기록 */
        bp = NEXT_BLKP(bp);                                    /* 남은 뒷부분 free block의 payload로 이동 */
        PUT(HDRP(bp), PACK(csize - asize, 0));                 /* 남은 뒷부분을 새 free block header로 기록 */
        PUT(FTRP(bp), PACK(csize - asize, 0));                 /* 남은 뒷부분을 새 free block footer로 기록 */
    }
    else                                                       /* 남는 공간이 너무 작으면 통째로 할당 */
    {
        PUT(HDRP(bp), PACK(csize, 1));                         /* 전체 block을 allocated로 표시 */
        PUT(FTRP(bp), PACK(csize, 1));                         /* 전체 block footer도 allocated로 기록 */
    }
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
 * mm_malloc - first-fit으로 free block을 찾아 배치하고, 없으면 힙을 확장한다. / mm_malloc - first-fit으로 free block을 찾아 배치하고, 없으면 힙을 확장한다.
 */
void *mm_malloc(size_t size)
{
    size_t asize;                                              /* 정렬과 overhead가 반영된 실제 요청 block 크기 */
    size_t extendsize;                                         /* fit을 못 찾았을 때 확장할 힙 크기 */
    char *bp;                                                  /* 배치할 free block의 payload 주소 */

    if (size == 0)                                             /* 크기 0 요청은 바로 NULL 반환 */
        return NULL;

    if (size <= DSIZE)                                         /* 아주 작은 요청도 최소 block 크기 16바이트로 맞춤 */
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE); /* header/footer 포함 후 8-byte 배수로 올림 */

    if ((bp = find_fit(asize)) != NULL)                        /* 먼저 기존 free list(implicit)에서 first-fit 탐색 */
    {
        place(bp, asize);                                      /* 찾은 free block에 실제 배치 */
        return bp;
    }

    extendsize = (asize > CHUNKSIZE) ? asize : CHUNKSIZE;      /* 요청이 크면 그만큼, 아니면 기본 chunk 크기만큼 확장 */

    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)        /* 힙 확장에 실패하면 NULL 반환 */
        return NULL;

    place(bp, asize);                                          /* 새로 늘린 free block에 요청 block 배치 */
    return bp;
}

/*
 * mm_free - block을 free 상태로 바꾸고 가능한 경우 coalesce한다. / mm_free - block을 free 상태로 바꾸고 가능한 경우 coalesce한다.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL)                                            /* free(NULL)은 아무 일도 하지 않음 */
        return;

    size_t size = GET_SIZE(HDRP(ptr));                         /* 해제할 block의 전체 크기 */

    PUT(HDRP(ptr), PACK(size, 0));                             /* header를 free 상태로 변경 */
    PUT(FTRP(ptr), PACK(size, 0));                             /* footer를 free 상태로 변경 */
    coalesce(ptr);                                             /* 인접 free block이 있으면 즉시 병합 */
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free / mm_realloc - mm_malloc과 mm_free를 이용해 단순하게 구현된다.
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    if (ptr == NULL)                                           /* realloc(NULL, size)는 malloc(size)와 동일 */
        return mm_malloc(size);                               /* 새 block을 처음부터 할당하는 경우와 동일 */

    if (size == 0)                                             /* realloc(ptr, 0)은 free 후 NULL 반환 */
    {
        mm_free(ptr);
        return NULL;                                          /* 크기 0 요청은 해제 의미이므로 NULL 반환 */
    }

    newptr = mm_malloc(size);                                 /* 새 크기를 담을 block을 먼저 새로 할당 */
    if (newptr == NULL)                                       /* 새 block 확보 실패 시 realloc도 실패 */
        return NULL;
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE;                 /* payload에서 실제로 복사 가능한 기존 데이터 크기 */
    if (size < copySize)                                      /* 새 요청 크기가 더 작으면 그만큼만 복사 */
        copySize = size;
    memcpy(newptr, oldptr, copySize);                         /* 이전 payload 내용을 새 payload로 복사 */
    mm_free(oldptr);                                          /* 이전 block은 더 이상 필요 없으므로 free 처리 */
    return newptr;                                            /* 새 payload 주소를 사용자에게 반환 */
}

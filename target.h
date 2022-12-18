
struct timeval
{
    /* typedef time_t -> __int_least64_t */ long long int tv_sec; /*     0     8 */
    /* typedef suseconds_t -> __suseconds_t */ long int tv_usec;  /*     8     4 */

    /* size: 16, cachelines: 1, members: 2 */
    /* padding: 4 */
    /* last cacheline: 16 bytes */
};
static_assert(sizeof(struct timeval) == 16);
static_assert(offsetof(struct timeval, tv_usec) == 8);

struct timespec
{
    /* typedef time_t -> __int_least64_t */ long long int tv_sec; /*     0     8 */
    long int tv_nsec;                                             /*     8     4 */

    /* size: 16, cachelines: 1, members: 2 */
    /* padding: 4 */
    /* last cacheline: 16 bytes */
};
static_assert(sizeof(struct timespec) == 16);
static_assert(offsetof(struct timespec, tv_nsec) == 8);

#ifdef _MSC_VER
#pragma pack(push, 2)
#endif

struct stat {
    /* typedef dev_t -> __dev_t */ short int                  st_dev;                /*     0     2 */
    /* typedef ino_t -> __ino_t */ short unsigned int         st_ino;                /*     2     2 */
    /* typedef mode_t -> __mode_t -> __uint32_t */ unsigned int               st_mode; /*     4     4 */
    /* typedef nlink_t -> __nlink_t */ short unsigned int         st_nlink;          /*     8     2 */
    /* typedef uid_t -> __uid_t */ short unsigned int         st_uid;                /*    10     2 */
    /* typedef gid_t -> __gid_t */ short unsigned int         st_gid;                /*    12     2 */
    /* typedef dev_t -> __dev_t */ short int                  st_rdev;               /*    14     2 */
    /* typedef off_t -> __off_t -> _off_t */ long int                   st_size;     /*    16     4 */

    /* XXX 4 bytes hole, try to pack */
    long int _;

    struct timespec st_atim; /*    24    16 */

    /* XXX last struct has 4 bytes of padding */

    struct timespec st_mtim; /*    40    16 */

    /* XXX last struct has 4 bytes of padding */

    struct timespec st_ctim; /*    56    16 */

    /* XXX last struct has 4 bytes of padding */

    /* typedef blksize_t -> __blksize_t */ long int                   st_blksize;    /*    72     4 */
    /* typedef blkcnt_t -> __blkcnt_t */ long int                   st_blocks;       /*    76     4 */
    long int                   st_spare4[2];                                         /*    80     8 */

    /* size: 88, cachelines: 2, members: 14 */
    /* sum members: 84, holes: 1, sum holes: 4 */
    /* paddings: 3, sum paddings: 12 */
    /* last cacheline: 24 bytes */
}
#ifndef _MSC_VER
__attribute__((__packed__))
#endif
;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
static_assert(sizeof(struct stat) == 88);
static_assert(offsetof(struct stat, st_atim) == 24);



struct timeval
{
    /* typedef time_t -> __int_least64_t */ long long int tv_sec; /*     0     8 */
    /* typedef suseconds_t -> __suseconds_t */ long int tv_usec;  /*     8     4 */

    /* size: 16, cachelines: 1, members: 2 */
    /* padding: 4 */
    /* last cacheline: 16 bytes */
};

struct timespec
{
    /* typedef time_t -> __int_least64_t */ long long int tv_sec; /*     0     8 */
    long int tv_nsec;                                             /*     8     4 */

    /* size: 16, cachelines: 1, members: 2 */
    /* padding: 4 */
    /* last cacheline: 16 bytes */
};

struct stat
{
    /* typedef dev_t -> __dev_t */ short int st_dev;                     /*     0     2 */
    /* typedef ino_t -> __ino_t */ short unsigned int st_ino;            /*     2     2 */
    /* typedef mode_t -> __mode_t -> __uint32_t */ unsigned int st_mode; /*     4     4 */
    /* typedef nlink_t -> __nlink_t */ short unsigned int st_nlink;      /*     8     2 */
    /* typedef uid_t -> __uid_t */ short unsigned int st_uid;            /*    10     2 */
    /* typedef gid_t -> __gid_t */ short unsigned int st_gid;            /*    12     2 */
    /* typedef dev_t -> __dev_t */ short int st_rdev;                    /*    14     2 */
    /* typedef off_t -> __off_t -> _off_t */ long int st_size;           /*    16     4 */
    struct timespec st_atim;                                             /*    20    16 */

    /* XXX last struct has 4 bytes of padding */

    struct timespec st_mtim; /*    36    16 */

    /* XXX last struct has 4 bytes of padding */

    struct timespec st_ctim; /*    52    16 */ /*    52    16 */

    /* XXX last struct has 4 bytes of padding */

    /* --- cacheline 1 boundary (64 bytes) was 4 bytes ago --- */
    /* typedef blksize_t -> __blksize_t */ long int st_blksize; /*    68     4 */
    /* typedef blkcnt_t -> __blkcnt_t */ long int st_blocks;    /*    72     4 */
    long int st_spare4[2];                                      /*    76     8 */

    /* size: 84, cachelines: 2, members: 14 */
    /* paddings: 3, sum paddings: 12 */
    /* last cacheline: 20 bytes */
} __attribute__((__packed__));

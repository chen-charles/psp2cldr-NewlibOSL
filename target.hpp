#ifndef NEWLIBOSL_TARGET_H
#define NEWLIBOSL_TARGET_H

namespace target
{
#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long

    typedef int _off_t;

    typedef unsigned __int8 __uint8_t;

    typedef unsigned int __uint32_t;

    typedef __uint8_t uint8_t;

    typedef __int64 __int_least64_t;

    typedef __int_least64_t time_t;

    typedef _off_t __off_t;

    typedef __off_t off_t;

    typedef int __blkcnt_t;

    typedef int __blksize_t;

    typedef __int16 __dev_t;

    typedef unsigned __int16 __uid_t;

    typedef unsigned __int16 __gid_t;

    typedef unsigned __int16 __ino_t;

    typedef __uint32_t __mode_t;

    typedef unsigned __int16 __nlink_t;

    struct __attribute__((aligned(8))) timespec
    {
        time_t tv_sec;
        int tv_nsec;
    };

    typedef __blkcnt_t blkcnt_t;

    typedef __blksize_t blksize_t;

    typedef __ino_t ino_t;

    typedef __dev_t dev_t;

    typedef __uid_t uid_t;

    typedef __gid_t gid_t;

    typedef __mode_t mode_t;

    typedef __nlink_t nlink_t;

    struct stat
    {
        dev_t st_dev;
        ino_t st_ino;
        mode_t st_mode;
        nlink_t st_nlink;
        uid_t st_uid;
        gid_t st_gid;
        dev_t st_rdev;
        off_t st_size;
        timespec st_atim;
        timespec st_mtim;
        timespec st_ctim;
        blksize_t st_blksize;
        blkcnt_t st_blocks;
        int st_spare4[2];
    };

    typedef unsigned int clock_t;

    struct tms
    {
        clock_t tms_utime;
        clock_t tms_stime;
        clock_t tms_cutime;
        clock_t tms_cstime;
    };

};
#endif

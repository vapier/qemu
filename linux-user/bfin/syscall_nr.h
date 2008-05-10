/*
 * This file contains the system call numbers.
 */
#define TARGET_NR_restart_syscall	  0
#define TARGET_NR_exit		  1
#define TARGET_NR_fork		  2
#define TARGET_NR_read		  3
#define TARGET_NR_write		  4
#define TARGET_NR_open		  5
#define TARGET_NR_close		  6
				/* 7 TARGET_NR_waitpid obsolete */
#define TARGET_NR_creat		  8
#define TARGET_NR_link		  9
#define TARGET_NR_unlink		 10
#define TARGET_NR_execve		 11
#define TARGET_NR_chdir		 12
#define TARGET_NR_time		 13
#define TARGET_NR_mknod		 14
#define TARGET_NR_chmod		 15
#define TARGET_NR_chown		 16
				/* 17 TARGET_NR_break obsolete */
				/* 18 TARGET_NR_oldstat obsolete */
#define TARGET_NR_lseek		 19
#define TARGET_NR_getpid		 20
#define TARGET_NR_mount		 21
				/* 22 TARGET_NR_umount obsolete */
#define TARGET_NR_setuid		 23
#define TARGET_NR_getuid		 24
#define TARGET_NR_stime		 25
#define TARGET_NR_ptrace		 26
#define TARGET_NR_alarm		 27
				/* 28 TARGET_NR_oldfstat obsolete */
#define TARGET_NR_pause		 29
				/* 30 TARGET_NR_utime obsolete */
				/* 31 TARGET_NR_stty obsolete */
				/* 32 TARGET_NR_gtty obsolete */
#define TARGET_NR_access		 33
#define TARGET_NR_nice		 34
				/* 35 TARGET_NR_ftime obsolete */
#define TARGET_NR_sync		 36
#define TARGET_NR_kill		 37
#define TARGET_NR_rename		 38
#define TARGET_NR_mkdir		 39
#define TARGET_NR_rmdir		 40
#define TARGET_NR_dup		 41
#define TARGET_NR_pipe		 42
#define TARGET_NR_times		 43
				/* 44 TARGET_NR_prof obsolete */
#define TARGET_NR_brk		 45
#define TARGET_NR_setgid		 46
#define TARGET_NR_getgid		 47
				/* 48 TARGET_NR_signal obsolete */
#define TARGET_NR_geteuid		 49
#define TARGET_NR_getegid		 50
#define TARGET_NR_acct		 51
#define TARGET_NR_umount2		 52
				/* 53 TARGET_NR_lock obsolete */
#define TARGET_NR_ioctl		 54
#define TARGET_NR_fcntl		 55
				/* 56 TARGET_NR_mpx obsolete */
#define TARGET_NR_setpgid		 57
				/* 58 TARGET_NR_ulimit obsolete */
				/* 59 TARGET_NR_oldolduname obsolete */
#define TARGET_NR_umask		 60
#define TARGET_NR_chroot		 61
#define TARGET_NR_ustat		 62
#define TARGET_NR_dup2		 63
#define TARGET_NR_getppid		 64
#define TARGET_NR_getpgrp		 65
#define TARGET_NR_setsid		 66
				/* 67 TARGET_NR_sigaction obsolete */
#define TARGET_NR_sgetmask		 68
#define TARGET_NR_ssetmask		 69
#define TARGET_NR_setreuid		 70
#define TARGET_NR_setregid		 71
				/* 72 TARGET_NR_sigsuspend obsolete */
				/* 73 TARGET_NR_sigpending obsolete */
#define TARGET_NR_sethostname	 74
#define TARGET_NR_setrlimit		 75
				/* 76 TARGET_NR_old_getrlimit obsolete */
#define TARGET_NR_getrusage		 77
#define TARGET_NR_gettimeofday	 78
#define TARGET_NR_settimeofday	 79
#define TARGET_NR_getgroups		 80
#define TARGET_NR_setgroups		 81
				/* 82 TARGET_NR_select obsolete */
#define TARGET_NR_symlink		 83
				/* 84 TARGET_NR_oldlstat obsolete */
#define TARGET_NR_readlink		 85
				/* 86 TARGET_NR_uselib obsolete */
				/* 87 TARGET_NR_swapon obsolete */
#define TARGET_NR_reboot		 88
				/* 89 TARGET_NR_readdir obsolete */
				/* 90 TARGET_NR_mmap obsolete */
#define TARGET_NR_munmap		 91
#define TARGET_NR_truncate		 92
#define TARGET_NR_ftruncate		 93
#define TARGET_NR_fchmod		 94
#define TARGET_NR_fchown		 95
#define TARGET_NR_getpriority	 96
#define TARGET_NR_setpriority	 97
				/* 98 TARGET_NR_profil obsolete */
#define TARGET_NR_statfs		 99
#define TARGET_NR_fstatfs		100
				/* 101 TARGET_NR_ioperm */
				/* 102 TARGET_NR_socketcall obsolete */
#define TARGET_NR_syslog		103
#define TARGET_NR_setitimer		104
#define TARGET_NR_getitimer		105
#define TARGET_NR_stat		106
#define TARGET_NR_lstat		107
#define TARGET_NR_fstat		108
				/* 109 TARGET_NR_olduname obsolete */
				/* 110 TARGET_NR_iopl obsolete */
#define TARGET_NR_vhangup		111
				/* 112 TARGET_NR_idle obsolete */
				/* 113 TARGET_NR_vm86old */
#define TARGET_NR_wait4		114
				/* 115 TARGET_NR_swapoff obsolete */
#define TARGET_NR_sysinfo		116
				/* 117 TARGET_NR_ipc oboslete */
#define TARGET_NR_fsync		118
				/* 119 TARGET_NR_sigreturn obsolete */
#define TARGET_NR_clone		120
#define TARGET_NR_setdomainname	121
#define TARGET_NR_uname		122
				/* 123 TARGET_NR_modify_ldt obsolete */
#define TARGET_NR_adjtimex		124
#define TARGET_NR_mprotect		125
				/* 126 TARGET_NR_sigprocmask obsolete */
				/* 127 TARGET_NR_create_module obsolete */
#define TARGET_NR_init_module	128
#define TARGET_NR_delete_module	129
				/* 130 TARGET_NR_get_kernel_syms obsolete */
#define TARGET_NR_quotactl		131
#define TARGET_NR_getpgid		132
#define TARGET_NR_fchdir		133
#define TARGET_NR_bdflush		134
				/* 135 was sysfs */
#define TARGET_NR_personality	136
				/* 137 TARGET_NR_afs_syscall */
#define TARGET_NR_setfsuid		138
#define TARGET_NR_setfsgid		139
#define TARGET_NR__llseek		140
#define TARGET_NR_getdents		141
				/* 142 TARGET_NR__newselect obsolete */
#define TARGET_NR_flock		143
				/* 144 TARGET_NR_msync obsolete */
#define TARGET_NR_readv		145
#define TARGET_NR_writev		146
#define TARGET_NR_getsid		147
#define TARGET_NR_fdatasync		148
#define TARGET_NR__sysctl		149
				/* 150 TARGET_NR_mlock */
				/* 151 TARGET_NR_munlock */
				/* 152 TARGET_NR_mlockall */
				/* 153 TARGET_NR_munlockall */
#define TARGET_NR_sched_setparam		154
#define TARGET_NR_sched_getparam		155
#define TARGET_NR_sched_setscheduler		156
#define TARGET_NR_sched_getscheduler		157
#define TARGET_NR_sched_yield		158
#define TARGET_NR_sched_get_priority_max	159
#define TARGET_NR_sched_get_priority_min	160
#define TARGET_NR_sched_rr_get_interval	161
#define TARGET_NR_nanosleep		162
#define TARGET_NR_mremap		163
#define TARGET_NR_setresuid		164
#define TARGET_NR_getresuid		165
				/* 166 TARGET_NR_vm86 */
				/* 167 TARGET_NR_query_module */
				/* 168 TARGET_NR_poll */
#define TARGET_NR_nfsservctl		169
#define TARGET_NR_setresgid		170
#define TARGET_NR_getresgid		171
#define TARGET_NR_prctl		172
#define TARGET_NR_rt_sigreturn	173
#define TARGET_NR_rt_sigaction	174
#define TARGET_NR_rt_sigprocmask	175
#define TARGET_NR_rt_sigpending	176
#define TARGET_NR_rt_sigtimedwait	177
#define TARGET_NR_rt_sigqueueinfo	178
#define TARGET_NR_rt_sigsuspend	179
#define TARGET_NR_pread		180
#define TARGET_NR_pwrite		181
#define TARGET_NR_lchown		182
#define TARGET_NR_getcwd		183
#define TARGET_NR_capget		184
#define TARGET_NR_capset		185
#define TARGET_NR_sigaltstack	186
#define TARGET_NR_sendfile		187
				/* 188 TARGET_NR_getpmsg */
				/* 189 TARGET_NR_putpmsg */
#define TARGET_NR_vfork		190
#define TARGET_NR_getrlimit		191
#define TARGET_NR_mmap2		192	/* xxx: this is mmap2 !? */
#define TARGET_NR_truncate64		193
#define TARGET_NR_ftruncate64	194
#define TARGET_NR_stat64		195
#define TARGET_NR_lstat64		196
#define TARGET_NR_fstat64		197
#define TARGET_NR_chown32		198
#define TARGET_NR_getuid32		199
#define TARGET_NR_getgid32		200
#define TARGET_NR_geteuid32		201
#define TARGET_NR_getegid32		202
#define TARGET_NR_setreuid32		203
#define TARGET_NR_setregid32		204
#define TARGET_NR_getgroups32	205
#define TARGET_NR_setgroups32	206
#define TARGET_NR_fchown32		207
#define TARGET_NR_setresuid32	208
#define TARGET_NR_getresuid32	209
#define TARGET_NR_setresgid32	210
#define TARGET_NR_getresgid32	211
#define TARGET_NR_lchown32		212
#define TARGET_NR_setuid32		213
#define TARGET_NR_setgid32		214
#define TARGET_NR_setfsuid32		215
#define TARGET_NR_setfsgid32		216
#define TARGET_NR_pivot_root		217
				/* 218 TARGET_NR_mincore */
				/* 219 TARGET_NR_madvise */
#define TARGET_NR_getdents64		220
#define TARGET_NR_fcntl64		221
				/* 222 reserved for TUX */
				/* 223 reserved for TUX */
#define TARGET_NR_gettid		224
#define TARGET_NR_readahead		225
#define TARGET_NR_setxattr		226
#define TARGET_NR_lsetxattr		227
#define TARGET_NR_fsetxattr		228
#define TARGET_NR_getxattr		229
#define TARGET_NR_lgetxattr		230
#define TARGET_NR_fgetxattr		231
#define TARGET_NR_listxattr		232
#define TARGET_NR_llistxattr		233
#define TARGET_NR_flistxattr		234
#define TARGET_NR_removexattr	235
#define TARGET_NR_lremovexattr	236
#define TARGET_NR_fremovexattr	237
#define TARGET_NR_tkill		238
#define TARGET_NR_sendfile64		239
#define TARGET_NR_futex		240
#define TARGET_NR_sched_setaffinity	241
#define TARGET_NR_sched_getaffinity	242
				/* 243 TARGET_NR_set_thread_area */
				/* 244 TARGET_NR_get_thread_area */
#define TARGET_NR_io_setup		245
#define TARGET_NR_io_destroy		246
#define TARGET_NR_io_getevents	247
#define TARGET_NR_io_submit		248
#define TARGET_NR_io_cancel		249
				/* 250 TARGET_NR_alloc_hugepages */
				/* 251 TARGET_NR_free_hugepages */
#define TARGET_NR_exit_group		252
#define TARGET_NR_lookup_dcookie     253
#define TARGET_NR_bfin_spinlock      254

#define TARGET_NR_epoll_create	255
#define TARGET_NR_epoll_ctl		256
#define TARGET_NR_epoll_wait		257
				/* 258 TARGET_NR_remap_file_pages */
#define TARGET_NR_set_tid_address	259
#define TARGET_NR_timer_create	260
#define TARGET_NR_timer_settime	261
#define TARGET_NR_timer_gettime	262
#define TARGET_NR_timer_getoverrun	263
#define TARGET_NR_timer_delete	264
#define TARGET_NR_clock_settime	265
#define TARGET_NR_clock_gettime	266
#define TARGET_NR_clock_getres	267
#define TARGET_NR_clock_nanosleep	268
#define TARGET_NR_statfs64		269
#define TARGET_NR_fstatfs64		270
#define TARGET_NR_tgkill		271
#define TARGET_NR_utimes		272
#define TARGET_NR_fadvise64_64	273
				/* 274 TARGET_NR_vserver */
				/* 275 TARGET_NR_mbind */
				/* 276 TARGET_NR_get_mempolicy */
				/* 277 TARGET_NR_set_mempolicy */
#define TARGET_NR_mq_open 		278
#define TARGET_NR_mq_unlink		279
#define TARGET_NR_mq_timedsend	280
#define TARGET_NR_mq_timedreceive	281
#define TARGET_NR_mq_notify		282
#define TARGET_NR_mq_getsetattr	283
#define TARGET_NR_kexec_load		284
#define TARGET_NR_waitid		285
#define TARGET_NR_add_key		286
#define TARGET_NR_request_key	287
#define TARGET_NR_keyctl		288
#define TARGET_NR_ioprio_set		289
#define TARGET_NR_ioprio_get		290
#define TARGET_NR_inotify_init	291
#define TARGET_NR_inotify_add_watch	292
#define TARGET_NR_inotify_rm_watch	293
				/* 294 TARGET_NR_migrate_pages */
#define TARGET_NR_openat		295
#define TARGET_NR_mkdirat		296
#define TARGET_NR_mknodat		297
#define TARGET_NR_fchownat		298
#define TARGET_NR_futimesat		299
#define TARGET_NR_fstatat64		300
#define TARGET_NR_unlinkat		301
#define TARGET_NR_renameat		302
#define TARGET_NR_linkat		303
#define TARGET_NR_symlinkat		304
#define TARGET_NR_readlinkat		305
#define TARGET_NR_fchmodat		306
#define TARGET_NR_faccessat		307
#define TARGET_NR_pselect6		308
#define TARGET_NR_ppoll		309
#define TARGET_NR_unshare		310

/* Blackfin private syscalls */
#define TARGET_NR_sram_alloc		311
#define TARGET_NR_sram_free		312
#define TARGET_NR_dma_memcpy		313

/* socket syscalls */
#define TARGET_NR_accept		314
#define TARGET_NR_bind		315
#define TARGET_NR_connect		316
#define TARGET_NR_getpeername	317
#define TARGET_NR_getsockname	318
#define TARGET_NR_getsockopt		319
#define TARGET_NR_listen		320
#define TARGET_NR_recv		321
#define TARGET_NR_recvfrom		322
#define TARGET_NR_recvmsg		323
#define TARGET_NR_send		324
#define TARGET_NR_sendmsg		325
#define TARGET_NR_sendto		326
#define TARGET_NR_setsockopt		327
#define TARGET_NR_shutdown		328
#define TARGET_NR_socket		329
#define TARGET_NR_socketpair		330

/* sysv ipc syscalls */
#define TARGET_NR_semctl		331
#define TARGET_NR_semget		332
#define TARGET_NR_semop		333
#define TARGET_NR_msgctl		334
#define TARGET_NR_msgget		335
#define TARGET_NR_msgrcv		336
#define TARGET_NR_msgsnd		337
#define TARGET_NR_shmat		338
#define TARGET_NR_shmctl		339
#define TARGET_NR_shmdt		340
#define TARGET_NR_shmget		341

#define TARGET_NR_splice		342
#define TARGET_NR_sync_file_range	343
#define TARGET_NR_tee		344
#define TARGET_NR_vmsplice		345

#define TARGET_NR_epoll_pwait	346
#define TARGET_NR_utimensat		347
#define TARGET_NR_signalfd		348
#define TARGET_NR_timerfd_create	349
#define TARGET_NR_eventfd		350
#define TARGET_NR_pread64		351
#define TARGET_NR_pwrite64		352
#define TARGET_NR_fadvise64		353
#define TARGET_NR_set_robust_list	354
#define TARGET_NR_get_robust_list	355
#define TARGET_NR_fallocate		356
#define TARGET_NR_semtimedop		357
#define TARGET_NR_timerfd_settime	358
#define TARGET_NR_timerfd_gettime	359
#define TARGET_NR_signalfd4		360
#define TARGET_NR_eventfd2		361
#define TARGET_NR_epoll_create1		362
#define TARGET_NR_dup3			363
#define TARGET_NR_pipe2			364
#define TARGET_NR_inotify_init1		365
#define TARGET_NR_preadv		366
#define TARGET_NR_pwritev		367
#define TARGET_NR_rt_tgsigqueueinfo	368
#define TARGET_NR_perf_event_open	369
#define TARGET_NR_recvmmsg		370
#define TARGET_NR_fanotify_init	371
#define TARGET_NR_fanotify_mark	372
#define TARGET_NR_prlimit64		373
#define TARGET_NR_cacheflush		374
#define TARGET_NR_name_to_handle_at	375
#define TARGET_NR_open_by_handle_at	376
#define TARGET_NR_clock_adjtime	377
#define TARGET_NR_syncfs		378
#define TARGET_NR_setns		379
#define TARGET_NR_sendmmsg		380
#define TARGET_NR_process_vm_readv	381
#define TARGET_NR_process_vm_writev	382
#define TARGET_NR_kcmp		383
#define TARGET_NR_finit_module	384
#define TARGET_NR_sched_setattr	385
#define TARGET_NR_sched_getattr	386
#define TARGET_NR_renameat2		387
#define TARGET_NR_seccomp		388
#define TARGET_NR_getrandom		389
#define TARGET_NR_memfd_create	390
#define TARGET_NR_bpf		391
#define TARGET_NR_execveat		392

#define TARGET_NR_syscall		393

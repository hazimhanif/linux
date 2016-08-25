# lock_test module

- Provide locking of a process into the RAM.
- Mainly developed to avoid swappiness.
- 3 flags : MCL_CURRENT, MCL_FUTURE, MCL_CURRENT | MCL_FUTURE.

### How to check:

- ```cat /proc/$(pgrep mysqld)/status | grep Vm```
- ```cat /proc/$(pgrep mysqld)/smaps | grepVmSwap```

### Main segment pages:

- Code Pages: RX
- Data Pages: RW

Refer: http://duartes.org/gustavo/blog/post/how-the-kernel-manages-your-memory/

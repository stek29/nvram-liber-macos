# Liberate your macOS

- Disable SIP (Boot into recovery, open terminal, `csrutil disable`)

- Build and load [hsp4](https://github.com/siguza/hsp4)

- Run this tool:
```sh
$ make clean all
$ sudo bin/nvram-liber-macos $(nm /System/Library/Kernels/kernel | egrep 'mac_policy_list$' | cut -d' ' -f1)
```

- Fully disable SIP: `sudo nvram csr-active-config='%ff%00%00%00'`

- Set bootargs to get amfi out of your way
	My favourites are `-v cs_enforcement_disable=1 amfi_get_out_of_my_way=1 keepsyms=1 intcoproc_unrestricted=1`

Also see [this thread on newosxbook forum](http://www.newosxbook.com/forum/viewtopic.php?t=16798).

-- Copyright 2016-2022 Christian Hesse. See LICENSE.
-- fstab LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('fstab', {lex_by_line = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- Basic filesystem-independent mount options.
  'async', 'atime', 'auto', 'comment', 'context', 'defaults', 'defcontext', 'dev', 'dirsync',
  'exec', 'fscontext', 'group', 'iversion', 'lazytime', 'loud', 'mand', '_netdev', 'noatime',
  'noauto', 'nodev', 'nodiratime', 'noexec', 'nofail', 'noiversion', 'nolazytime', 'nomand',
  'norelatime', 'nostrictatime', 'nosuid', 'nouser', 'owner', 'relatime', 'remount', 'ro',
  'rootcontext', 'rw', 'silent', 'strictatime', 'suid', 'sync', 'user', 'users',
  -- Mount options for systemd see systemd.mount(5).
  'x-systemd.automount', 'x-systemd.device-timeout', 'x-systemd.idle-timeout',
  'x-systemd.mount-timeout', 'x-systemd.requires', 'x-systemd.requires-mounts-for',
  'x-initrd.mount',
  -- Mount options for adfs.
  'uid', 'gid', 'ownmask', 'othmask',
  -- Mount options for affs.
  'uid', 'gid', 'setuid', 'setgid', 'mode', 'protect', 'usemp', 'verbose', 'prefix', 'volume',
  'reserved', 'root', 'bs', 'grpquota', 'noquota', 'quota', 'usrquota',
  -- Mount options for btrfs.
  'alloc_start', 'autodefrag', 'check_int', 'check_int_data', 'check_int_print_mask', 'commit',
  'compress', 'zlib', 'lzo', 'no', 'compress-force', 'degraded', 'device', 'discard',
  'enospc_debug', 'fatal_errors', 'bug', 'panic', 'flushoncommit', 'inode_cache', 'max_inline',
  'metadata_ratio', 'noacl', 'nobarrier', 'nodatacow', 'nodatasum', 'notreelog', 'recovery',
  'rescan_uuid_tree', 'skip_balance', 'nospace_cache', 'clear_cache', 'ssd', 'nossd', 'ssd_spread',
  'subvol', 'subvolid', 'subvolrootid', 'thread_pool', 'user_subvol_rm_allowed',
  -- Mount options for devpts.
  'uid', 'gid', 'mode', 'newinstance', 'ptmxmode',
  -- Mount options for ext2.
  'acl', 'noacl', 'bsddf', 'minixdf', 'check', 'nocheck', 'debug', 'errors', 'continue',
  'remount-ro', 'panic', 'grpid', 'bsdgroups', 'nogrpid', 'sysvgroups', 'grpquota', 'noquota',
  'quota', 'usrquota', 'nouid32', 'oldalloc', 'orlov', 'resgid', 'resuid', 'sb', 'user_xattr',
  'nouser_xattr',
  -- Mount options for ext3.
  'journal', 'update', 'journal_dev', 'journal_path', 'norecoverynoload', 'data', 'journal',
  'ordered', 'writeback', 'data_err', 'ignore', 'abort', 'barrier', 'commit', 'user_xattr', 'acl',
  'usrjquota', 'grpjquota', 'jqfmt',
  -- Mount options for ext4.
  'journal_checksum', 'journal_async_commit', 'barrier', 'nobarrier', 'inode_readahead_blks',
  'stripe', 'delalloc', 'nodelalloc', 'max_batch_time', 'min_batch_time', 'journal_ioprio', 'abort',
  'auto_da_alloc', 'noauto_da_alloc', 'noinit_itable', 'init_itable', 'discard', 'nodiscard',
  'nouid32', 'block_validity', 'noblock_validity', 'dioread_lock', 'dioread_nolock',
  'max_dir_size_kb', 'i_version',
  -- Mount options for fat (common part of msdos umsdos and vfat).
  'blocksize', 'uid', 'gid', 'umask', 'dmask', 'fmask', 'allow_utime', 'check', 'relaxed', 'normal',
  'strict', 'codepage', 'conv', 'binary', 'text', 'auto', 'cvf_format', 'cvf_option', 'debug',
  'discard', 'dos1xfloppy', 'errors', 'panic', 'continue', 'remount-ro', 'fat', 'iocharset', 'nfs',
  'stale_rw', 'nostale_ro', 'tz', 'time_offset', 'quiet', 'rodir', 'showexec', 'sys_immutable',
  'flush', 'usefree', 'dots', 'nodots', 'dotsOK',
  -- Mount options for hfs.
  'creator', 'type', 'uid', 'gid', 'dir_umask', 'file_umask', 'umask', 'session', 'part', 'quiet',
  -- Mount options for hpfs.
  'uid', 'gid', 'umask', 'case', 'lower', 'asis', 'conv', 'binary', 'text', 'auto', 'nocheck',
  -- Mount options for iso9660.
  'norock', 'nojoliet', 'check', 'relaxed', 'strict', 'uid', 'gid', 'map', 'normal', 'offacorn',
  'mode', 'unhide', 'block', 'conv', 'auto', 'binary', 'mtext', 'text', 'cruft', 'session',
  'sbsector', 'iocharset', 'utf8',
  -- Mount options for jfs.
  'iocharset', 'resize', 'nointegrity', 'integrity', 'errors', 'continue', 'remount-ro', 'panic',
  'noquota', 'quota', 'usrquota', 'grpquota',
  -- Mount options for ntfs.
  'iocharset', 'nls', 'utf8', 'uni_xlate', 'posix', 'uid', 'gid', 'umask',
  -- Mount options for overlay.
  'lowerdir', 'upperdir', 'workdir',
  -- Mount options for reiserfs.
  'conv', 'hash', 'rupasov', 'tea', 'r5', 'detect', 'hashed_relocation', 'no_unhashed_relocation',
  'noborder', 'nolog', 'notail', 'replayonly', 'resize', 'user_xattr', 'acl', 'barrier', 'none',
  'flush',
  -- Mount options for tmpfs.
  'size', 'nr_blocks', 'nr_inodes', 'mode', 'uid', 'gid', 'mpol', 'default', 'prefer', 'bind',
  'interleave',
  -- Mount options for ubifs.
  'bulk_read', 'no_bulk_read', 'chk_data_crc', 'no_chk_data_crc.', 'compr', 'none', 'lzo', 'zlib',
  -- Mount options for udf.
  'gid', 'umask', 'uid', 'unhide', 'undelete', 'nostrict', 'iocharset', 'bs', 'novrs', 'session',
  'anchor', 'volume', 'partition', 'lastblock', 'fileset', 'rootdir',
  -- Mount options for ufs.
  'ufstype', 'old', '44bsd', 'ufs2', '5xbsd', 'sun', 'sunx86', 'hp', 'nextstep', 'nextstep-cd',
  'openstep', 'onerror', 'lock', 'umount', 'repair',
  -- Mount options for vfat.
  'uni_xlate', 'posix', 'nonumtail', 'utf8', 'shortname', 'lower', 'win95', 'winnt', 'mixed',
  -- Mount options for usbfs.
  'devuid', 'devgid', 'devmode', 'busuid', 'busgid', 'busmode', 'listuid', 'listgid', 'listmode',
  -- Filesystems.
  'adfs', 'ados', 'affs', 'anon_inodefs', 'atfs', 'audiofs', 'auto', 'autofs', 'bdev', 'befs',
  'bfs', 'btrfs', 'binfmt_misc', 'cd9660', 'cfs', 'cgroup', 'cifs', 'coda', 'configfs', 'cpuset',
  'cramfs', 'devfs', 'devpts', 'devtmpfs', 'e2compr', 'efs', 'ext2', 'ext2fs', 'ext3', 'ext4',
  'fdesc', 'ffs', 'filecore', 'fuse', 'fuseblk', 'fusectl', 'hfs', 'hpfs', 'hugetlbfs', 'iso9660',
  'jffs', 'jffs2', 'jfs', 'kernfs', 'lfs', 'linprocfs', 'mfs', 'minix', 'mqueue', 'msdos', 'ncpfs',
  'nfs', 'nfsd', 'nilfs2', 'none', 'ntfs', 'null', 'nwfs', 'overlay', 'ovlfs', 'pipefs', 'portal',
  'proc', 'procfs', 'pstore', 'ptyfs', 'qnx4', 'reiserfs', 'ramfs', 'romfs', 'securityfs', 'shm',
  'smbfs', 'squashfs', 'sockfs', 'sshfs', 'std', 'subfs', 'swap', 'sysfs', 'sysv', 'tcfs', 'tmpfs',
  'udf', 'ufs', 'umap', 'umsdos', 'union', 'usbfs', 'userfs', 'vfat', 'vs3fs', 'vxfs', 'wrapfs',
  'wvfs', 'xenfs', 'xfs', 'zisofs'
}))

-- Numbers.
local uuid = lexer.xdigit^8 * ('-' * lexer.xdigit^4)^-3 * '-' * lexer.xdigit^12
local dec = lexer.digit^1 * ('_' * lexer.digit^1)^0
local oct_num = '0' * S('01234567_')^1
local integer = S('+-')^-1 * (lexer.hex_num + oct_num + dec)
lex:add_rule('number', token(lexer.NUMBER, uuid + lexer.float + integer))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + '_') * (lexer.alnum + S('_.'))^0))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.starts_line(lexer.to_eol('#'))))

-- Directories.
lex:add_rule('directory', token(lexer.VARIABLE, '/' * (1 - lexer.space)^0))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=,')))

return lex

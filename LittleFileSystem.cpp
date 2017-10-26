/* mbed Microcontroller Library
 * Copyright (c) 2006-2012 ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "mbed.h"
#include "LittleFileSystem.h"
#include "errno.h"


////// Conversion functions //////
static int lfs_toerror(int err) {
    switch (err) {
        case LFS_ERR_OK:        return 0;
        case LFS_ERR_IO:        return -EIO;
        case LFS_ERR_NOENT:     return -ENOENT;
        case LFS_ERR_EXISTS:    return -EEXIST;
        case LFS_ERR_NOTDIR:    return -ENOTDIR;
        case LFS_ERR_ISDIR:     return -EISDIR;
        case LFS_ERR_INVAL:     return -EINVAL;
        case LFS_ERR_NOSPC:     return -ENOSPC;
        case LFS_ERR_NOMEM:     return -ENOMEM;
        default:                return err;
    }
}

static int lfs_fromflags(int flags) {
    return (
        (((flags & 3) == O_RDONLY) ? LFS_O_RDONLY : 0) |
        (((flags & 3) == O_WRONLY) ? LFS_O_WRONLY : 0) |
        (((flags & 3) == O_RDWR)   ? LFS_O_RDWR   : 0) |
        ((flags & O_CREAT)  ? LFS_O_CREAT  : 0) |
        ((flags & O_EXCL)   ? LFS_O_EXCL   : 0) |
        ((flags & O_TRUNC)  ? LFS_O_TRUNC  : 0) |
        ((flags & O_APPEND) ? LFS_O_APPEND : 0));
}

static int lfs_fromwhence(int whence) {
    switch (whence) {
        case SEEK_SET: return LFS_SEEK_SET;
        case SEEK_CUR: return LFS_SEEK_CUR;
        case SEEK_END: return LFS_SEEK_END;
        default: return whence;
    }
}

static int lfs_tomode(int type) {
    int mode = S_IRWXU | S_IRWXG | S_IRWXO;
    switch (type) {
        case LFS_TYPE_DIR: return mode | S_IFDIR;
        case LFS_TYPE_REG: return mode | S_IFREG;
        default: return 0;
    }
}

static int lfs_totype(int type) {
    switch (type) {
        case LFS_TYPE_DIR: return DT_DIR;
        case LFS_TYPE_REG: return DT_REG;
        default: return DT_UNKNOWN;
    }
}


////// Block device operations //////
static int lfs_bd_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size) {
    BlockDevice *bd = (BlockDevice *)c->context;
    return bd->read(buffer, block*c->block_size + off, size);
}

static int lfs_bd_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size) {
    BlockDevice *bd = (BlockDevice *)c->context;
    return bd->program(buffer, block*c->block_size + off, size);
}

static int lfs_bd_erase(const struct lfs_config *c, lfs_block_t block) {
    BlockDevice *bd = (BlockDevice *)c->context;
    return bd->erase(block*c->block_size, c->block_size);
}

static int lfs_bd_sync(const struct lfs_config *c) {
    return 0;
}


////// Generic filesystem operations //////

// Filesystem implementation (See LittleFileSystem.h)
LittleFileSystem::LittleFileSystem(const char *name, BlockDevice *bd,
        lfs_size_t read_size, lfs_size_t prog_size,
        lfs_size_t block_size, lfs_size_t lookahead)
        : FileSystem(name)
        , _read_size(read_size)
        , _prog_size(prog_size)
        , _block_size(block_size)
        , _lookahead(lookahead) {
    if (bd) {
        mount(bd);
    }
}

LittleFileSystem::~LittleFileSystem() {
    // nop if unmounted
    unmount();
}

int LittleFileSystem::mount(BlockDevice *bd) {
    _bd = bd;
    int err = _bd->init();
    if (err) {
        return err;
    }

    memset(&_config, 0, sizeof(_config));
    _config.context = bd;
    _config.read  = lfs_bd_read;
    _config.prog  = lfs_bd_prog;
    _config.erase = lfs_bd_erase;
    _config.sync  = lfs_bd_sync;
    _config.read_size   = bd->get_read_size();
    if (_config.read_size < _read_size) {
        _config.read_size = _read_size;
    }
    _config.prog_size   = bd->get_program_size();
    if (_config.prog_size < _prog_size) {
        _config.prog_size = _prog_size;
    }
    _config.block_size  = bd->get_erase_size();
    if (_config.block_size < _block_size) {
        _config.block_size = _block_size;
    }
    _config.block_count = bd->size() / _config.block_size;
    _config.lookahead = _config.block_count - _config.block_count % 32;
    if (_config.lookahead > _lookahead) {
        _config.lookahead = _lookahead;
    }

    err = lfs_mount(&_lfs, &_config);
    return lfs_toerror(err);
}

int LittleFileSystem::unmount() {
    if (_bd) {
        int err = lfs_unmount(&_lfs);
        if (err) {
            return lfs_toerror(err);
        }

        err = _bd->deinit();
        if (err) {
            return err;
        }

        _bd = NULL;
    }
    
    return 0;
}

int LittleFileSystem::format(BlockDevice *bd,
        lfs_size_t read_size, lfs_size_t prog_size,
        lfs_size_t block_size, lfs_size_t lookahead) {
    int err = bd->init();
    if (err) {
        return err;
    }

    lfs_t _lfs;
    struct lfs_config _config;
    
    memset(&_config, 0, sizeof(_config));
    _config.context = bd;
    _config.read  = lfs_bd_read;
    _config.prog  = lfs_bd_prog;
    _config.erase = lfs_bd_erase;
    _config.sync  = lfs_bd_sync;
    _config.read_size   = bd->get_read_size();
    if (_config.read_size < read_size) {
        _config.read_size = read_size;
    }
    _config.prog_size   = bd->get_program_size();
    if (_config.prog_size < prog_size) {
        _config.prog_size = prog_size;
    }
    _config.block_size  = bd->get_erase_size();
    if (_config.block_size < block_size) {
        _config.block_size = block_size;
    }
    _config.block_count = bd->size() / _config.block_size;
    _config.lookahead = _config.block_count - _config.block_count % 32;
    if (_config.lookahead > lookahead) {
        _config.lookahead = lookahead;
    }

    err = lfs_format(&_lfs, &_config);
    if (err) {
        return lfs_toerror(err);
    }

    err = bd->deinit();
    if (err) {
        return err;
    }

    return 0;
}

int LittleFileSystem::reformat(BlockDevice *bd) {
    if (_bd) {
        if (!bd) {
            bd = _bd;
        }

        int err = unmount();
        if (err) {
            return err;
        }
    }

    if (!bd) {
        return -ENODEV;
    }

    int err = LittleFileSystem::format(bd,
            _read_size, _prog_size, _block_size, _lookahead);
    if (err) {
        return err;
    }

    return mount(bd);
}

int LittleFileSystem::remove(const char *filename) {
    int err = lfs_remove(&_lfs, filename);
    return lfs_toerror(err);
}

int LittleFileSystem::rename(const char *oldname, const char *newname) {
    int err = lfs_rename(&_lfs, oldname, newname);
    return lfs_toerror(err);
}

int LittleFileSystem::mkdir(const char *name, mode_t mode) {
    int err = lfs_mkdir(&_lfs, name);
    return lfs_toerror(err);
}

int LittleFileSystem::stat(const char *name, struct stat *st) {
    struct lfs_info info;
    int err = lfs_stat(&_lfs, name, &info);
    st->st_size = info.size;
    st->st_mode = lfs_tomode(info.type);
    return lfs_toerror(err);
}


////// File operations //////
int LittleFileSystem::file_open(fs_file_t *file, const char *path, int flags) {
    lfs_file_t *f = new lfs_file_t;
    *file = f;
    int err = lfs_file_open(&_lfs, f, path, lfs_fromflags(flags));
    return lfs_toerror(err);
}

int LittleFileSystem::file_close(fs_file_t file) {
    lfs_file_t *f = (lfs_file_t *)file;
    int err = lfs_file_close(&_lfs, f);
    delete f;
    return lfs_toerror(err);
}

ssize_t LittleFileSystem::file_read(fs_file_t file, void *buffer, size_t len) {
    lfs_file_t *f = (lfs_file_t *)file;
    int res = lfs_file_read(&_lfs, f, buffer, len);
    return lfs_toerror(res);
}

ssize_t LittleFileSystem::file_write(fs_file_t file, const void *buffer, size_t len) {
    lfs_file_t *f = (lfs_file_t *)file;
    int res = lfs_file_write(&_lfs, f, buffer, len);
    return lfs_toerror(res);
}

int LittleFileSystem::file_sync(fs_file_t file) {
    lfs_file_t *f = (lfs_file_t *)file;
    int err = lfs_file_sync(&_lfs, f);
    return lfs_toerror(err);
}

off_t LittleFileSystem::file_seek(fs_file_t file, off_t offset, int whence) {
    lfs_file_t *f = (lfs_file_t *)file;
    off_t res = lfs_file_seek(&_lfs, f, offset, lfs_fromwhence(whence));
    return lfs_toerror(res);
}

off_t LittleFileSystem::file_tell(fs_file_t file) {
    lfs_file_t *f = (lfs_file_t *)file;
    off_t res = lfs_file_tell(&_lfs, f);
    return lfs_toerror(res);
}

off_t LittleFileSystem::file_size(fs_file_t file) {
    lfs_file_t *f = (lfs_file_t *)file;
    off_t res = lfs_file_size(&_lfs, f);
    return lfs_toerror(res);
}


////// Dir operations //////
int LittleFileSystem::dir_open(fs_dir_t *dir, const char *path) {
    lfs_dir_t *d = new lfs_dir_t;
    *dir = d;
    int err = lfs_dir_open(&_lfs, d, path);
    return lfs_toerror(err);
}

int LittleFileSystem::dir_close(fs_dir_t dir) {
    lfs_dir_t *d = (lfs_dir_t *)dir;
    int err = lfs_dir_close(&_lfs, d);
    delete d;
    return lfs_toerror(err);
}

ssize_t LittleFileSystem::dir_read(fs_dir_t dir, struct dirent *ent) {
    lfs_dir_t *d = (lfs_dir_t *)dir;
    struct lfs_info info;
    int res = lfs_dir_read(&_lfs, d, &info);
    if (res == 1) {
        ent->d_type = lfs_totype(info.type);
        strcpy(ent->d_name, info.name);
    }
    return lfs_toerror(res);
}

void LittleFileSystem::dir_seek(fs_dir_t dir, off_t offset) {
    lfs_dir_t *d = (lfs_dir_t *)dir;
    lfs_dir_seek(&_lfs, d, offset);
}

off_t LittleFileSystem::dir_tell(fs_dir_t dir) {
    lfs_dir_t *d = (lfs_dir_t *)dir;
    return lfs_dir_tell(&_lfs, d);
}

void LittleFileSystem::dir_rewind(fs_dir_t dir) {
    lfs_dir_t *d = (lfs_dir_t *)dir;
    lfs_dir_rewind(&_lfs, d);
}


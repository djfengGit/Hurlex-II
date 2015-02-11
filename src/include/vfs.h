/*
 * =====================================================================================
 *
 *       Filename:  vfs.h
 *
 *    Description:  虚拟文件系统
 *
 *        Version:  1.0
 *        Created:  2015年02月06日 09时56分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Qianyi.lh (liuhuan), qianyi.lh@alibaba-inc.com
 *        Company:  Alibaba-Inc Aliyun
 *
 * =====================================================================================
 */

#ifndef INCLUDE_VFS_H_
#define INCLUDE_VFS_H_

#include <types.h>
#include <spinlock.h>
#include <block_dev.h>

#define ROOTFS_T        0xA0
#define SFS_T           0xAA

// 前置声明
struct super_block;
struct super_ops;
struct inode;
struct inode_ops;
struct dentry;
struct dentry_ops;
struct file;
struct file_ops;

struct filesystem {
        const char *name;                                         // 文件系统名称
        uint8_t type;                                             // 文件系统类型
        struct super_block *(*read_super)(struct super_block *);  // 读取super_block
        struct list_head fs_supers;                               // super_block指针
        struct filesystem *next;                                  // 下一个文件系统
};

struct super_block {
        struct list_head s_list;        // super_block指针
        block_dev_t *bdev;              // 对应的块设备指针
        uint16_t s_type;                // 文件系统类型
        uint32_t s_inode_count;         // inode数量
        uint32_t s_block_count;         // block数量
        uint32_t s_block_size;          // block大小
        uint32_t s_max_file;            // 文件最大尺寸
        struct dentry *s_root;          // 根dentry
        struct super_block_ops *s_ops;  // super_block操作
};

struct super_block_ops {
        struct inode *(*alloc_inode)(struct super_block *);     // 获取inode
        void (*destroy_inode)(struct inode *);                  // 释放inode
        void (*write_super)(struct super_block *);              // 写回super_block
        int (*sync_fs)(struct super_block *);                   // 同步文件系统
        void (*delete_inode)(struct inode *);                   // 删除inode及其对应的文件数据
};

#define S_DIR    0x1    // inode 目录类型
#define S_FILE   0x2    // inode 文件类型

struct inode {
        spinlock_t i_lock;              // inode自旋锁
        atomic_t i_count;               // 索引节点引用计数
        struct super_block *i_sb;       // super_blcok指针
        struct list_head i_list;        // inode 链
        uint32_t i_type;                // inode 类型
        uint32_t i_ino;                 // 索引节点号
        time_t i_atime;                 // 文件最后一次访问时间
        time_t i_mtime;                 // 文件最后一次修改时间
        time_t i_ctime;                 // 文件首次创建时间
        uint32_t i_size;                // 文件字节数
        uint32_t i_blocks;              // 文件使用block数
        uint32_t i_bytes;               // 文件最后一个block的字节数
};

// inode相关操作
struct inode_ops {
        int (*create)(struct inode *, struct dentry *);         // 创建文件
        int (*rm)(struct inode *, struct dentry *);             // 删除文件
        int (*mkdir)(struct inode *, struct dentry *);          // 创建目录
        int (*rmdir)(struct inode *, struct dentry *);          // 删除目录
        int (*rename)(struct inode *, struct dentry *,          // 重命名
                        struct inode *, struct dentry *);
};

// 最长文件名
#define MAX_FILE_NAME   128

struct dentry {
        atomic_t d_count;               // 引用计数
        spinlock_t d_lock;              // dentry项的自旋锁
        char d_name[MAX_FILE_NAME+1];   // 目录项名称
        uint32_t d_status;              // 目录项状态
        struct dentry *d_parent;        // 父目录指针
        struct list_head d_brother;     // 相同层级的目录链
        struct list_head d_subdirs;     // 子目录链表头
        bool is_mounted;                // 是否被挂载设备
        struct super_block *d_sb;       // 目录项对应的super_blcok
        struct inode *d_inode;          // 链接到目录项对应的inode
        struct dentry_ops *d_ops;       // dentry相关操作
};

struct dentry_ops {
       int (*del)(struct dentry *); 
};

// 最长路径长度(含文件名)
#define MAX_PATH    1024

struct file {
        atomic_t f_count;               // 引用计数
        spinlock_t f_lock;              // file项的自旋锁
        char f_path[MAX_PATH+1];        // 文件路径
        struct dentry *f_dentry;        // 与文件相关的dentry
        uint32_t f_openflag;            // 打开文件时的标记
        uint32_t f_pos;                 // 文件操作的偏移
        struct file_ops *f_ops;         // 文件操作
};

struct file_ops {
        int (*read)(struct file *, char *, uint32_t);
        int (*write)(struct file *, const char *, uint32_t);
        int (*open)(struct inode *, struct file *);
        int (*flush)(struct file *);
        int (*close)(struct inode *, struct file *);
};

struct vfsmount {
        const char *mnt_devname;         // 挂载的设备名称
        struct super_block *mnt_sb;      // 挂载的 super_block
        struct dentry *mnt_root;         // 挂载的根目录 dentry
        struct dentry *mnt_mountpoint;   // 挂载点 dentry
        struct vfsmount *mnt_parent;     // 父vfsmount指针
};

// 全局的文件系统指针
extern struct filesystem *file_systems; 

// 根文件系统
extern struct filesystem fs_rootfs;

// vfs 初始化
void vfs_init(void);

// 内核注册文件系统
int add_filesystem(struct filesystem *fs);

// 获取 super_block 结构
struct super_block *alloc_super_block(void);

// 获取 super_pos 结构
struct super_block_ops *alloc_super_block_ops(void);

// 获取 inode 结构
struct inode *alloc_inode(void);

// 获取 inode_ops 结构
struct inode_ops *alloc_inode_ops(void);

// 获取 dentry 结构
struct dentry *alloc_dentry(void);

// 获取 dentry_ops 结构
struct dentry_ops *alloc_dentry_ops(void);

// 获取 file 结构
struct file *alloc_file(void);

// 获取 file_ops 结构
struct file_ops *alloc_file_ops(void);

// 释放 super_block 结构
void free_super_block(struct super_block *sb);

// 释放 super_block_ops 结构
void free_super_block_ops(struct super_ops *sb_ops);

// 释放 inode 结构
void free_inode(struct inode *inode);

// 释放 inode_ops 结构
void free_inode_ops(struct inode_ops *inode_ops);

// 释放 dentry 结构
void free_dentry(struct dentry *dentry);

// 释放 dentry_ops 结构
void free_dentry_ops(struct dentry_ops *dentry_ops);

// 释放 file 结构
void free_file(struct file *file);

// 释放 file_ops 结构
void free_file_ops(struct file_ops *file_ops);

#endif  // INCLUDE_VFS_H_

/*****************************************************************************
 *   Copyright(C)2009-2019 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/



#ifndef __VSF_LINUX_H__
#define __VSF_LINUX_H__

/*============================ INCLUDES ======================================*/
#include "./vsf_linux_cfg.h"

#if VSF_USE_LINUX == ENABLED

#include "vsf.h"

#include <unistd.h>
#include <signal.h>

#if     defined(VSF_LINUX_IMPLEMENT)
#   define __PLOOC_CLASS_IMPLEMENT
#elif   defined(VSF_LINUX_INHERIT)
#   define __PLOOC_CLASS_INHERIT
#endif

#include "utilities/ooc_class.h"

/*============================ MACROS ========================================*/

#if VSF_KERNEL_CFG_EDA_SUPPORT_ON_TERMINATE != ENABLED
#   error VSF_KERNEL_CFG_EDA_SUPPORT_ON_TERMINATE MUST be enabled to use vsf_linux
#endif

#ifndef VSF_LINUX_CFG_MAX_ARG_NUM
#   define VSF_LINUX_CFG_MAX_ARG_NUM        31
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/

declare_simple_class(vsf_linux_process_t)
declare_simple_class(vsf_linux_thread_t)
declare_simple_class(vsf_linux_fd_t)

struct vsf_linux_process_arg_t {
    int argc;
    char const *argv[VSF_LINUX_CFG_MAX_ARG_NUM + 1];
};
typedef struct vsf_linux_process_arg_t vsf_linux_process_arg_t;

typedef int (*vsf_linux_main_entry_t)(int, char **);
typedef int (*vsf_linux_process_arg_parser_t)(vsf_linux_process_arg_t *arg);

struct vsf_linux_thread_op_t {
    int priv_size;
    void (*on_run)(vsf_thread_cb_t *cb);
    void (*on_terminate)(vsf_linux_thread_t *thread);
};
typedef struct vsf_linux_thread_op_t vsf_linux_thread_op_t;

struct vsf_linux_process_ctx_t {
    vsf_linux_process_arg_t arg;
    vsf_linux_main_entry_t entry;
};
typedef struct vsf_linux_process_ctx_t vsf_linux_process_ctx_t;

def_simple_class(vsf_linux_thread_t) {
    public_member(
        implement(vsf_thread_t)
        implement(vsf_thread_cb_t)
        const vsf_linux_thread_op_t *op;
    )

    protected_member(
        int retval;
        int tid;
        vsf_linux_thread_t *thread_pending;
    )

    private_member(
        vsf_linux_process_t *process;
        vsf_dlist_node_t thread_node;
        union {
            vsf_stream_t *stream;
        } pending;
    )
};

struct vsf_linux_sig_handler_t {
    vsf_dlist_node_t node;
    uint_fast8_t sig;
    void (*handler)(int, siginfo_t *, void *);
};
typedef struct vsf_linux_sig_handler_t vsf_linux_sig_handler_t;

struct vsf_linux_stdio_stream_t {
    vsf_stream_t *in;
    vsf_stream_t *out;
    vsf_stream_t *err;
};
typedef struct vsf_linux_stdio_stream_t vsf_linux_stdio_stream_t;

def_simple_class(vsf_linux_process_t) {
    public_member(
        vsf_linux_process_ctx_t ctx;
    )

    protected_member(
        struct {
            pid_t pid;
            pid_t ppid;
        } id;
    )

    private_member(
        vsf_dlist_node_t process_node;
        vsf_dlist_t thread_list;
        vsf_dlist_t fd_list;
        int cur_fd;
        vsf_linux_thread_t *thread_pending;
        vsf_linux_stdio_stream_t stdio_stream;

        struct {
            sigset_t pending;
            sigset_t mask;
            vsf_dlist_t handler_list;
        } sig;

//        struct termios term;

        vsf_prio_t prio;
    )
};

struct vsf_linux_fd_op_t {
    int priv_size;
    int (*fcntl)(vsf_linux_fd_t *sfd, int cmd, long arg);
    ssize_t (*read)(vsf_linux_fd_t *sfd, void *buf, size_t count);
    ssize_t (*write)(vsf_linux_fd_t *sfd, void *buf, size_t count);
    int (*close)(vsf_linux_fd_t *sfd);
};
typedef struct vsf_linux_fd_op_t vsf_linux_fd_op_t;

def_simple_class(vsf_linux_fd_t) {
    protected_member(
        int fd;
        int flags;
        const vsf_linux_fd_op_t *op;

        vsf_trig_t *txpend, *rxpend;
        uint_fast8_t txevt      : 1;
        uint_fast8_t rxevt      : 1;
        uint_fast8_t locked     : 1;
    )

    private_member(
        vsf_dlist_node_t fd_node;
        int priv[0];
    )
};

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/

extern vsf_err_t vsf_linux_init(vsf_linux_stdio_stream_t *stdio_stream);

extern int vsf_linux_fs_bind_target(int fd, void *target, vsf_param_eda_evthandler_t read,
        vsf_param_eda_evthandler_t write);
extern int vsf_linux_fs_bind_executable(int fd, vsf_linux_main_entry_t entry);

#if defined(VSF_LINUX_IMPLEMENT) || defined(VSF_LINUX_INHERIT)
extern int vsf_linux_fs_get_executable(const char *pathname, vsf_linux_main_entry_t *entry);

extern vsf_linux_process_t * vsf_linux_create_process(int stack_size);
extern int vsf_linux_start_process(vsf_linux_process_t *process);

extern vsf_linux_thread_t * vsf_linux_create_thread(vsf_linux_process_t *process,
        int stack_size, const vsf_linux_thread_op_t *op);
extern int vsf_linux_start_thread(vsf_linux_thread_t *thread);

extern void vsf_linux_thread_on_terminate(vsf_linux_thread_t *thread);
extern vsf_linux_thread_t * vsf_linux_get_cur_thread(void);
extern vsf_linux_process_t * vsf_linux_get_cur_process(void);
extern vsf_linux_thread_t * vsf_linux_get_thread(int tid);
extern vsf_linux_process_t * vsf_linux_get_process(pid_t pid);

extern int vsf_linux_create_fd(vsf_linux_fd_t **sfd, const vsf_linux_fd_op_t *op);
extern vsf_linux_fd_t * vsf_linux_get_fd(int fd);

extern void vsf_linux_delete_fd(int fd);
extern int vsf_linux_fd_tx_pend(int fd);
extern int vsf_linux_fd_rx_pend(int fd);
extern int vsf_linux_fd_tx_trigger(int fd);
extern int vsf_linux_fd_rx_trigger(int fd);

extern vk_vfs_file_t * vsf_linux_fs_get_vfs(int fd);
#endif

#undef VSF_LINUX_IMPLEMENT
#undef VSF_LINUX_INHERIT

#endif      // VSF_USE_LINUX
#endif      // __VSF_LINUX_H__
/* EOF */
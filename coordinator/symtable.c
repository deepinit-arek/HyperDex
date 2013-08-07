/* Copyright (c) 2012, Cornell University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Replicant nor the names of its contributors may be
 *       used to endorse or promote products derived from this software without
 *       specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Replicant */
#include <replicant_state_machine.h>

/* HyperDex */
#include "visibility.h"
#include "coordinator/transitions.h"

struct replicant_state_machine HYPERDEX_API rsm = {
    hyperdex_coordinator_create,
    hyperdex_coordinator_recreate,
    hyperdex_coordinator_destroy,
    hyperdex_coordinator_snapshot,
    {{"get-config", hyperdex_coordinator_get_config},
     {"ack-config", hyperdex_coordinator_ack_config},
     {"xfer-begin", hyperdex_coordinator_xfer_begin},
     {"xfer-go-live", hyperdex_coordinator_xfer_go_live},
     {"xfer-complete", hyperdex_coordinator_xfer_complete},

     {"server-register", hyperdex_coordinator_server_register},
     {"server-reregister", hyperdex_coordinator_server_reregister},
     {"server-suspect", hyperdex_coordinator_server_suspect},
     {"server-shutdown1", hyperdex_coordinator_server_shutdown1},
     {"server-shutdown2", hyperdex_coordinator_server_shutdown2},

     {"add-space", hyperdex_coordinator_add_space},
     {"rm-space", hyperdex_coordinator_rm_space},

     {"init", hyperdex_coordinator_init},
     {NULL, NULL}}
};

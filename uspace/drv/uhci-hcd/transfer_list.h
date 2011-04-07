/*
 * Copyright (c) 2011 Jan Vesely
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** @addtogroup drvusbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver transfer list structure
 */
#ifndef DRV_UHCI_TRANSFER_LIST_H
#define DRV_UHCI_TRANSFER_LIST_H

#include <fibril_synch.h>

#include "batch.h"
#include "hw_struct/queue_head.h"
#include "utils/malloc32.h"

typedef struct transfer_list
{
	fibril_mutex_t guard;
	qh_t *queue_head;
	uint32_t queue_head_pa;
	const char *name;
	link_t batch_list;
} transfer_list_t;

/** Dispose transfer list structures.
 *
 * @param[in] instance Memory place to use.
 *
 * Frees memory for internal qh_t structure.
 */
static inline void transfer_list_fini(transfer_list_t *instance)
{
	assert(instance);
	free32(instance->queue_head);
}

int transfer_list_init(transfer_list_t *instance, const char *name);

void transfer_list_set_next(transfer_list_t *instance, transfer_list_t *next);

void transfer_list_add_batch(transfer_list_t *instance, usb_transfer_batch_t *batch);

void transfer_list_remove_finished(transfer_list_t *instance, link_t *done);

void transfer_list_abort_all(transfer_list_t *instance);
#endif
/**
 * @}
 */

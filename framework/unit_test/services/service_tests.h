/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __SERVICE_TESTS__
#define __SERVICE_TESTS__

#include "util/cunit_test.h"
#include "os/os.h"
#include "infra/port.h"

#define SRV_WAIT(condition, timeout) \
	do { \
		OS_ERR_TYPE err = E_OS_ERR_BUSY; \
		uint32_t start = get_time_ms();	\
		while ((condition) && (err != E_OS_ERR_TIMEOUT)) { \
			if (timeout == OS_WAIT_FOREVER) { \
				queue_process_message_wait( \
					get_test_queue(), OS_WAIT_FOREVER, &err); \
			} else { \
				uint32_t elapsed = get_time_ms() - start; \
				if (elapsed > timeout) { \
					break; \
				} \
				queue_process_message_wait( \
					get_test_queue(), timeout - elapsed, \
					&err); \
			} \
		} \
	} while (0)

/**
 * Set the test queue
 *
 * \param the queue to use
 */
void set_test_queue(T_QUEUE queue);

/**
 * Get the test queue
 *
 * \return the test queue
 */
T_QUEUE get_test_queue();

extern int cfw_service_registered(int);

#endif /* __SERVICE_TESTS__ */

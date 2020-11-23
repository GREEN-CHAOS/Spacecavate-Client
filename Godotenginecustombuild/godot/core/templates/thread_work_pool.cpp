/*************************************************************************/
/*  thread_work_pool.cpp                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "thread_work_pool.h"

#include "core/os/os.h"

void ThreadWorkPool::_thread_function(ThreadData *p_thread) {
	while (true) {
		p_thread->start.wait();
		if (p_thread->exit.load()) {
			break;
		}
		p_thread->work->work();
		p_thread->completed.post();
	}
}

void ThreadWorkPool::init(int p_thread_count) {
	ERR_FAIL_COND(threads != nullptr);
	if (p_thread_count < 0) {
		p_thread_count = OS::get_singleton()->get_processor_count();
	}

	thread_count = p_thread_count;
	threads = memnew_arr(ThreadData, thread_count);

	for (uint32_t i = 0; i < thread_count; i++) {
		threads[i].exit.store(false);
		threads[i].thread = memnew(std::thread(ThreadWorkPool::_thread_function, &threads[i]));
	}
}

void ThreadWorkPool::finish() {
	if (threads == nullptr) {
		return;
	}

	for (uint32_t i = 0; i < thread_count; i++) {
		threads[i].exit.store(true);
		threads[i].start.post();
	}
	for (uint32_t i = 0; i < thread_count; i++) {
		threads[i].thread->join();
		memdelete(threads[i].thread);
	}

	memdelete_arr(threads);
	threads = nullptr;
}

ThreadWorkPool::~ThreadWorkPool() {
	finish();
}

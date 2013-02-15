/* Copyright (C) 2002 Jean-Marc Valin */
/**
   @file stack_alloc.h
   @brief Temporary memory allocation on stack
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef cc6_STACK_ALLOC_H
#define cc6_STACK_ALLOC_H

#ifdef USE_ALLOCA
# ifdef WIN32
#  include <malloc.h>
# else
#  ifdef HAVE_ALLOCA_H
#   include <alloca.h>
#  else
#   include <stdlib.h>
#  endif
# endif
#endif

/**
 * @def cc6_ALIGN(stack, size)
 *
 * Aligns the stack to a 'size' boundary
 *
 * @param stack Stack
 * @param size  New size boundary
 */

/**
 * @def cc6_PUSH(stack, size, type)
 *
 * Allocates 'size' elements of type 'type' on the stack
 *
 * @param stack Stack
 * @param size  Number of elements
 * @param type  Type of element
 */

/**
 * @def cc6_VARDECL(var)
 *
 * Declare variable on stack
 *
 * @param var Variable to declare
 */

/**
 * @def cc6_ALLOC(var, size, type)
 *
 * Allocate 'size' elements of 'type' on stack
 *
 * @param var  Name of variable to allocate
 * @param size Number of elements
 * @param type Type of element
 */


#if defined(VAR_ARRAYS)

#define cc6_VARDECL(type, var)
#define cc6_ALLOC(var, size, type) type var[size]
#define cc6_SAVE_STACK
#define cc6_RESTORE_STACK
#define cc6_ALLOC_STACK

#elif defined(USE_ALLOCA)

#define cc6_VARDECL(type, var) type *var
#define cc6_ALLOC(var, size, type) var = ((type*)alloca(sizeof(type)*(size)))
#define cc6_SAVE_STACK
#define cc6_RESTORE_STACK
#define cc6_ALLOC_STACK

#else

#ifdef CELT_C
char *cc6_global_stack=0;
#else
extern char *cc6_global_stack;
#endif /*CELT_C*/

#ifdef ENABLE_VALGRIND

#include <valgrind/memcheck.h>

#ifdef CELT_C
char *cc6_global_stack_top=0;
#else
extern char *cc6_global_stack_top;
#endif /*CELT_C*/

#define cc6_ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))
#define cc6_PUSH(stack, size, type) (VALGRIND_MAKE_MEM_NOACCESS(stack, cc6_global_stack_top-stack),cc6_ALIGN((stack),sizeof(type)/sizeof(char)),VALGRIND_MAKE_MEM_UNDEFINED(stack, ((size)*sizeof(type)/sizeof(char))),(stack)+=(2*(size)*sizeof(type)/sizeof(char)),(type*)((stack)-(2*(size)*sizeof(type)/sizeof(char))))
#define cc6_RESTORE_STACK ((cc6_global_stack = _saved_stack),VALGRIND_MAKE_MEM_NOACCESS(cc6_global_stack, cc6_global_stack_top-cc6_global_stack))
#define cc6_ALLOC_STACK ((cc6_global_stack = (cc6_global_stack==0) ? ((cc6_global_stack_top=cc6_celt_alloc_scratch(cc6_GLOBAL_STACK_SIZE*2)+(cc6_GLOBAL_STACK_SIZE*2))-(cc6_GLOBAL_STACK_SIZE*2)) : cc6_global_stack),VALGRIND_MAKE_MEM_NOACCESS(cc6_global_stack, cc6_global_stack_top-cc6_global_stack))

#else 

#define cc6_ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))
#define cc6_PUSH(stack, size, type) (cc6_ALIGN((stack),sizeof(type)/sizeof(char)),(stack)+=(size)*(sizeof(type)/sizeof(char)),(type*)((stack)-(size)*(sizeof(type)/sizeof(char))))
#define cc6_RESTORE_STACK (cc6_global_stack = _saved_stack)
#define cc6_ALLOC_STACK (cc6_global_stack = (cc6_global_stack==0) ? cc6_celt_alloc_scratch(cc6_GLOBAL_STACK_SIZE) : cc6_global_stack)

#endif /*ENABLE_VALGRIND*/ 

#include "os_support.h"
#define cc6_VARDECL(type, var) type *var
#define cc6_ALLOC(var, size, type) var = cc6_PUSH(cc6_global_stack, size, type)
#define cc6_SAVE_STACK char *_saved_stack = cc6_global_stack;

#endif /*VAR_ARRAYS*/


#endif /*cc6_STACK_ALLOC_H*/

/*
    This file is part of Ionowatch.

    (c) 2011 Gonzalo J. Carracedo <BatchDrake@gmail.com>
    
    Ionowatch is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ionowatch is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

# define _GNU_SOURCE
# include <stdio.h>
# include <string.h>
# include <stdlib.h>

# include "id3.h"
# include <unistd.h>
# include <sys/mman.h>

#if (defined(_X86) || defined(__i386) || defined(i386)) && defined (linux)

# define PAGE_SIZE 4096

struct jit_code *
jit_code_new (int size)
{
  struct jit_code *new;
  void *base;
  
  if ((base = mmap (NULL, 
                    size, 
                    PROT_EXEC | PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    0, 0)) == (caddr_t) - 1)
  {
    /* WEIRD */
    return NULL;
  }
  
  new = xmalloc (sizeof (struct jit_code));
  new->base = base;
  new->size = size;
  new->ptr  = 0;
  
  return new;
}

/* This will increase the code pointer */
unsigned long
jit_alloc_chunk (struct jit_code *code, size_t size)
{
  unsigned long current;
  
  current = code->ptr;
  code->ptr += size;
  
  if (__UNITS (code->size, PAGE_SIZE) < __UNITS (current + size, PAGE_SIZE))
  {
    if ((code->base = mremap (code->base, code->size, code->size + PAGE_SIZE, 0)) ==
      (void *) -1)
      return 0xffffffff;
    
    code->size += PAGE_SIZE;
  }
  
  return current;
}

void *
jit_ptr_to_addr (struct jit_code *code, unsigned long ptr)
{
  return code->base + ptr;
}

unsigned long
jit_addr_to_ptr (struct jit_code *code, void *addr)
{
  return (unsigned long) addr - (unsigned long) code->base;
}

static inline int
__jit_codegen (struct jit_code *code, struct node *node)
{
  char *buf, *jmpbuf;
  unsigned int ptr, jmp;
  unsigned int dword;
  unsigned int buf_pc, jmp_pc;
  
  switch (node->type)
  {
    case NODE_TYPE_LEAF:
      if ((ptr = jit_alloc_chunk (code, 6)) == 0xffffffff)
      {
        ERROR ("cannot allocate chunk!\n");
        return -1;
      }
      
      dword = (unsigned int) node->output;
      buf = jit_ptr_to_addr (code, ptr);
      
      /* mov XX XX XX XX, %eax */
      buf[0] = '\xb8';
      buf[5] = '\xc3'; /* ret */
      
      memcpy (buf + 1, &dword, sizeof (unsigned int));
      break;
      
    case NODE_TYPE_THRESHOLD:
    /*
     804839c:       d9 05 e4 95 04 08       flds   0x80495e4
     80483a2:       dd 05 c0 84 04 08       fldl   0x80484c0
     80483a8:       d9 c9                   fxch   %st(1)
     80483aa:       da e9                   fucompp
     80483ac:       df e0                   fnstsw %ax
     80483ae:       9e                      sahf
     80483af:       0f 93 c0                setae  %al
     80483b2:       84 c0                   test   %al,%al
     80483b4:       0f 84 XX XX XX XX       je eip + XX XX XX XX - 6           
    */
      if ((ptr = jit_alloc_chunk (code, 30)) == 0xffffffff)
      {
        ERROR ("cannot allocate chunk!\n");
        return -1;
      }
      
      buf_pc = code->ptr;
      
      buf = jit_ptr_to_addr (code, ptr);
      
      memcpy (buf, 
        "\xd9\x05PARM\xd9\x05THRS\xd9\xc9\xda\xe9"
        "\xdf\xe0\x9e\x0f\x93\xc0\x84\xc0\x0f\x84OFFS", 30);
      
      
      
      dword = (unsigned int) &node->param->value;
      memcpy (buf + 2, &dword, sizeof (unsigned int));
      
      dword = (unsigned int) &node->threshold;
      memcpy (buf + 8, &dword, sizeof (unsigned int));
      
      if (__jit_codegen (code, node->true_node) == -1)
        return -1;
        
      if ((jmp = jit_alloc_chunk (code, 5)) == 0xffffffff)
      {
        ERROR ("cannot allocate chunk!\n");
        return -1;
      }
      
      jmp_pc = code->ptr;
      
      jmpbuf = jit_ptr_to_addr (code, jmp);
      memcpy (jmpbuf, "\xe9OFF2", 5);
      
      dword = code->ptr - buf_pc;
      
      memcpy (buf + 26, &dword, sizeof (unsigned int));
      
      if (__jit_codegen (code, node->false_node) == -1)
        return -1;
      
      dword = code->ptr - jmp_pc;
      
      memcpy (jmpbuf + 1, &dword, sizeof (unsigned int));
        
      break;
      
    default:
      ERROR ("unknown node type (corruption)\n");
      abort ();
      break;
  }
  
  return 0;
}

int
jit_compile (struct jit_code *code, struct decision_tree *tree)
{
  if (__jit_codegen (code, tree->root) == -1)
    return -1;
    
  return 0;
}

struct output *
jit_code_exec (struct jit_code *code)
{
  return ((struct output * (*) (void)) code->base) ();
}

void
jit_code_destroy (struct jit_code *code)
{
  munmap (code->base, code->size);
  free (code);
}

#else
struct jit_code *
jit_code_new (int unused)
{
  return NULL;
}

int 
jit_compile (struct jit_code *u1, struct decision_tree *u2)
{
  return -1;
}

struct output* 
jit_code_exec (struct jit_code *unused)
{
  return NULL;
}

void 
jit_code_destroy (struct jit_code *unused)
{
}

#endif


/* Initialization of PPC64 specific backend library.
   Copyright (C) 2004 Red Hat, Inc.
   Written by Ulrich Drepper <drepper@redhat.com>, 2004.

   This program is Open Source software; you can redistribute it and/or
   modify it under the terms of the Open Software License version 1.0 as
   published by the Open Source Initiative.

   You should have received a copy of the Open Software License along
   with this program; if not, you may obtain a copy of the Open Software
   License version 1.0 from http://www.opensource.org/licenses/osl.php or
   by writing the Open Source Initiative c/o Lawrence Rosen, Esq.,
   3001 King Ranch Road, Ukiah, CA 95482.   */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libebl_ppc64.h>


int
ppc64_init (elf, machine, eh, ehlen)
     Elf *elf;
     GElf_Half machine;
     Ebl *eh;
     size_t ehlen;
{
  /* Check whether the Elf_BH object has a sufficent size.  */
  if (ehlen < sizeof (Ebl))
    return 1;

  /* We handle it.  */
  eh->name = "PowerPC 64-bit";
  eh->reloc_type_name = ppc64_reloc_type_name;
  eh->reloc_type_check = ppc64_reloc_type_check;
  eh->reloc_valid_use = ppc64_reloc_valid_use;
  eh->dynamic_tag_name = ppc64_dynamic_tag_name;
  eh->destr = ppc64_destr;

  return 0;
}

/* 
 * Copyright (C) 2000 Rich Wareham <richwareham@users.sourceforge.net>
 * 
 * This file is part of libdvdnav, a DVD navigation library.
 * 
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: navigation.c,v 1.21 2004/01/07 19:34:28 mroi Exp $
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dvdnav_internal.h"

/* Navigation API calls */

dvdnav_status_t dvdnav_still_skip(dvdnav_t *this) {
  if(!this) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }

  this->position_current.still = 0;
  this->skip_still = 1;
  this->sync_wait = 0;
  this->sync_wait_skip = 1;

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_wait_skip(dvdnav_t *this) {
  if(!this) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }

  this->sync_wait = 0;
  this->sync_wait_skip = 1;

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_number_of_titles(dvdnav_t *this, int32_t *titles) {
  if(!this || !titles) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }
  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return DVDNAV_STATUS_ERR;
  }

  (*titles) = vm_get_vmgi(this->vm)->tt_srpt->nr_of_srpts;

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_get_number_of_parts(dvdnav_t *this, int32_t title, int32_t *parts) {
  if(!this || !parts) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }
  if(!this->started) {
    printerr("Virtual DVD machine not started.");
    return DVDNAV_STATUS_ERR;
  }
  if ((title < 1) || (title > vm_get_vmgi(this->vm)->tt_srpt->nr_of_srpts) ) {
    printerr("Passed a title number out of range.");
    return DVDNAV_STATUS_ERR;
  }

  (*parts) = vm_get_vmgi(this->vm)->tt_srpt->title[title-1].nr_of_ptts;

  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_current_title_info(dvdnav_t *this, int32_t *title, int32_t *part) {
  int32_t retval;
  
  if(!this || !title || !part) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }
  
  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->vtsi || !this->vm->vmgi) {
    printerr("Bad VM state.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  if ( (this->vm->state.domain == VTSM_DOMAIN)
      || (this->vm->state.domain == VMGM_DOMAIN) ) {
    /* Get current Menu ID: into *part. */
    vm_get_current_menu(this->vm, part);
    if (*part > -1) {
      *title = 0;
      pthread_mutex_unlock(&this->vm_lock);
      return DVDNAV_STATUS_OK;
    }
  }
  if (this->vm->state.domain == VTS_DOMAIN) {
    retval = vm_get_current_title_part(this->vm, title, part);
    pthread_mutex_unlock(&this->vm_lock);
    return retval ? DVDNAV_STATUS_OK : DVDNAV_STATUS_ERR;
  }
  printerr("Not in a title or menu.");
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_title_play(dvdnav_t *this, int32_t title) {
  if(!this) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }
  return dvdnav_part_play(this, title, 1);
}

dvdnav_status_t dvdnav_part_play(dvdnav_t *this, int32_t title, int32_t part) {
  int32_t retval;

  if(!this) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }
  
  pthread_mutex_lock(&this->vm_lock);
  if (!this->vm->vmgi) {
    printerr("Bad VM state.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  if (!this->vm->state.pgc) {
    printerr("No current PGC.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  if((title < 1) || (title > this->vm->vmgi->tt_srpt->nr_of_srpts)) {
    printerr("Title out of range.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }
  if((part < 1) || (part > this->vm->vmgi->tt_srpt->title[title-1].nr_of_ptts)) {
    printerr("Part out of range.");
    pthread_mutex_unlock(&this->vm_lock);
    return DVDNAV_STATUS_ERR;
  }

  retval = vm_jump_title_part(this->vm, title, part);
  if (retval)
    this->vm->hop_channel++;
  pthread_mutex_unlock(&this->vm_lock);

  return retval ? DVDNAV_STATUS_OK : DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_part_play_auto_stop(dvdnav_t *this, int32_t title,
					   int32_t part, int32_t parts_to_play) {
  /* FIXME: Implement auto-stop */
 if (dvdnav_part_play(this, title, part) == DVDNAV_STATUS_OK)
   printerr("Not implemented yet.");
 return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_time_play(dvdnav_t *this, int32_t title,
				 uint64_t time) {
  if(!this) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }
  
  /* FIXME: Implement */
  printerr("Not implemented yet.");
  return DVDNAV_STATUS_ERR;
}

dvdnav_status_t dvdnav_stop(dvdnav_t *this) {
  if(!this) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }
  
  pthread_mutex_lock(&this->vm_lock);
  this->vm->stopped = 1;
  pthread_mutex_unlock(&this->vm_lock);
  return DVDNAV_STATUS_OK;
}

dvdnav_status_t dvdnav_go_up(dvdnav_t *this) {
  if(!this) {
    printerr("Passed a NULL pointer.");
    return DVDNAV_STATUS_ERR;
  }

  /* A nice easy function... delegate to the VM */
  pthread_mutex_lock(&this->vm_lock);
  vm_jump_up(this->vm);
  pthread_mutex_unlock(&this->vm_lock);

  return DVDNAV_STATUS_OK;
}

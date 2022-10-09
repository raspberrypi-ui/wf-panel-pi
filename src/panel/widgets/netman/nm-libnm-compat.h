/* NetworkManager -- Network link manager
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * (C) Copyright 2016 Red Hat, Inc.
 */

#ifndef __NM_LIBNM_COMPAT_H__
#define __NM_LIBNM_COMPAT_H__

#define NM_LIBNM_COMPAT_UNDEPRECATE(cmd) \
	({ \
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS \
		(cmd); \
		G_GNUC_END_IGNORE_DEPRECATIONS \
	})

#define _NMU_SEC_SAE ((NMUtilsSecurityType) 9)
#if !NM_CHECK_VERSION (1,22,0)
#define NMU_SEC_SAE _NMU_SEC_SAE
#endif
G_STATIC_ASSERT(NMU_SEC_SAE == _NMU_SEC_SAE);

#define _NMU_SEC_OWE ((NMUtilsSecurityType) 10)
#if !NM_CHECK_VERSION (1,24,0)
#define NMU_SEC_OWE _NMU_SEC_OWE
#endif
G_STATIC_ASSERT(NMU_SEC_OWE == _NMU_SEC_OWE);

#endif /* __NM_LIBNM_COMPAT_H__ */

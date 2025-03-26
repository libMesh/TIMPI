// The TIMPI Message-Passing Parallelism Library.
// Copyright (C) 2002-2025 Benjamin S. Kirk, John W. Peterson, Roy H. Stogner

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



#ifndef TIMPI_MACROS_H
#define TIMPI_MACROS_H


// Local includes
#include "timpi/timpi_config.h"

#ifdef TIMPI_HAVE_CXX20
#  define timpi_pure [[nodiscard("Pure-function return values should not be discarded")]]
#elif defined(TIMPI_HAVE_CXX17)
#  define timpi_pure [[nodiscard]]
#else
#  define timpi_pure
#endif

#endif // TIMPI_MACROS_H
